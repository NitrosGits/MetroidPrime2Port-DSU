// Debug command console, modelled on MetroidPrimePort's `platform/port_console.cpp` - same protocol
// (`MP2_CONSOLE=<port>`, one command per line, replies ending in `=> ok`/`=> err: ...`), the same
// one-client-at-a-time TCP listener, and the same rule that everything which reads the game runs on
// the game thread. See platform/include/port_console.h.
//
// What is here is the set of commands this tree can serve today through its existing port APIs
// (PortWarp, PortOverlay) plus the world/player state a state manager gives for free. Adding one is a
// row in `kCommands`.

#include "port_harness.h"
#include "port_console.h"
#include "port_mouse.h"
#include "port_freecam.h"
#include "port_livesplit.h"
#include "port_photo.h"

#include "port_input.h"
#include "port_logfile.h"
#include "port_overlay.h"
#include "port_save.h"
#include "port_savestate.h"
#include "port_env.h"
#include "port_overlay_internal.h"
#include "port_ui_common.h"
#include "port_warp.h"

#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CMemoryCard.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "Kyoto/CResFactory.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Graphics/CGraphics.hpp"
#include "Kyoto/SObjectTag.hpp"
#include "Kyoto/Text/CStringTable.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/Player/CGameState.hpp"
#include "MetroidPrime/Player/CWorldTransManager.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerGun.hpp"
#include "MetroidPrime/Weapons/CGunWeapon.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/CActor.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CEntity.hpp"
#include "MetroidPrime/CEntityInfo.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "MetroidPrime/CObjectList.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/CGenericFSM2State.hpp"
#include "MetroidPrime/ScriptObjects/CScriptConditionalRelay.hpp"
#include "MetroidPrime/ScriptObjects/CScriptDoor.hpp"
#include "MetroidPrime/ScriptObjects/CScriptGrapplePoint.hpp"
#include "Collision/CMaterialFilter.hpp"

#include <aurora/gfx.h>

#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <typeinfo>
#include <vector>

#if defined(__GNUC__)
#include <cxxabi.h>
#endif

namespace aurora {
void request_screenshot() noexcept;
}

#ifndef _WIN32
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

namespace PortConsole {
namespace {

constexpr int kDefaultPort = 4780;
// Replies are sent from the game thread: a client that stops reading must not stall it.
constexpr int kSendBudgetMs = 200;
// A line longer than this is not a command.
constexpr size_t kMaxLine = 64 * 1024;
// Lines waiting for the game; more are answered with an error and dropped.
constexpr size_t kMaxQueued = 256;

bool sStarted = false;
bool sEnabled = false;

#ifndef _WIN32
std::mutex sClientMutex;
int sClient = -1;
unsigned sGeneration = 0;

struct SQueued {
  std::string line;
  unsigned generation;
};

std::mutex sQueueMutex;
std::deque< SQueued > sQueue;

// macOS has no MSG_NOSIGNAL; SO_NOSIGPIPE is set on the client socket instead.
#ifdef MSG_NOSIGNAL
constexpr int kSendFlags = MSG_NOSIGNAL;
#else
constexpr int kSendFlags = 0;
#endif

void SendRaw(unsigned generation, const std::string& text) {
  std::lock_guard< std::mutex > lock(sClientMutex);
  if (sClient < 0 || generation != sGeneration) {
    return;
  }
  size_t done = 0;
  int budget = kSendBudgetMs;
  while (done < text.size()) {
    const ssize_t n =
        send(sClient, text.data() + done, text.size() - done, kSendFlags | MSG_DONTWAIT);
    if (n > 0) {
      done += static_cast< size_t >(n);
      budget = kSendBudgetMs;
      continue;
    }
    if (n < 0 && errno == EINTR) {
      continue;
    }
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK) && budget > 0) {
      pollfd descriptor{};
      descriptor.fd = sClient;
      descriptor.events = POLLOUT;
      const int wait = budget < 50 ? budget : 50;
      budget -= wait;
      poll(&descriptor, 1, wait);
      continue;
    }
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      std::fprintf(stderr, "[console] client is not reading; disconnecting it\n");
      shutdown(sClient, SHUT_RDWR);
    }
    return;
  }
}

// "GET / HTTP/1.1": a web page can POST to a loopback port, and its request line would otherwise run
// as a command. No console client speaks HTTP.
bool LooksLikeHttp(const std::string& line) {
  const size_t method = line.find(' ');
  if (method == std::string::npos || method == 0) {
    return false;
  }
  for (size_t i = 0; i < method; ++i) {
    if (line[i] < 'A' || line[i] > 'Z') {
      return false;
    }
  }
  const size_t version = line.rfind(" HTTP/");
  return version != std::string::npos && version > method;
}

void ListenThread(int listener) {
  std::string buffer;
  for (;;) {
    const int client = accept(listener, nullptr, nullptr);
#ifdef SO_NOSIGPIPE
    if (client >= 0) {
      const int on = 1;
      setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
    }
#endif
    if (client < 0) {
      if (errno != EINTR) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      continue;
    }
    unsigned generation;
    {
      std::lock_guard< std::mutex > lock(sClientMutex);
      if (sClient >= 0) {
        close(sClient);
      }
      sClient = client;
      generation = ++sGeneration;
    }
    std::fprintf(stderr, "[console] client connected\n");
    buffer.clear();
    bool firstLine = true;
    bool refused = false;
    char chunk[512];
    for (;;) {
      const ssize_t n = recv(client, chunk, sizeof(chunk), 0);
      if (n <= 0) {
        break;
      }
      buffer.append(chunk, static_cast< size_t >(n));
      size_t eol;
      while ((eol = buffer.find('\n')) != std::string::npos) {
        std::string line = buffer.substr(0, eol);
        buffer.erase(0, eol + 1);
        if (!line.empty() && line.back() == '\r') {
          line.pop_back();
        }
        if (firstLine && LooksLikeHttp(line)) {
          std::fprintf(stderr, "[console] HTTP request refused; disconnecting the client\n");
          refused = true;
          break;
        }
        firstLine = false;
        bool queued = false;
        {
          std::lock_guard< std::mutex > lock(sQueueMutex);
          if (sQueue.size() < kMaxQueued) {
            sQueue.push_back(SQueued{line, generation});
            queued = true;
          }
        }
        if (!queued) {
          SendRaw(generation, "=> err: too many commands waiting\n");
        }
      }
      if (refused) {
        break;
      }
      if (buffer.size() > kMaxLine) {
        std::fprintf(stderr, "[console] line too long; disconnecting the client\n");
        break;
      }
    }
    {
      std::lock_guard< std::mutex > lock(sClientMutex);
      if (sClient == client) {
        close(sClient);
        sClient = -1;
      }
    }
    std::fprintf(stderr, "[console] client disconnected\n");
  }
}
#endif

// ---- commands ---------------------------------------------------------------------------------
//
// Each handler appends its output to `out` (one line per item) and returns false with `out` holding
// the reason on error. They run on the game thread with `mgr` live.

using Args = std::vector< std::string_view >;

Args Split(const std::string& line) {
  Args args;
  size_t i = 0;
  while (i < line.size()) {
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) {
      ++i;
    }
    const size_t begin = i;
    while (i < line.size() && line[i] != ' ' && line[i] != '\t') {
      ++i;
    }
    if (i > begin) {
      args.emplace_back(line.data() + begin, i - begin);
    }
  }
  return args;
}

std::string Str(std::string_view text) { return std::string(text); }

int ArgToInt(std::string_view text, int fallback) {
  const std::string copy(text);
  char* end = nullptr;
  const long value = std::strtol(copy.c_str(), &end, 0);
  return (end != nullptr && *end == '\0') ? static_cast< int >(value) : fallback;
}

const char* VisorName(CPlayerState::EPlayerVisor visor) {
  switch (visor) {
  case CPlayerState::kPV_Combat:
    return "combat";
  case CPlayerState::kPV_Echo:
    return "echo";
  case CPlayerState::kPV_Dark:
    return "dark";
  case CPlayerState::kPV_Scan:
    return "scan";
  }
  return "?";
}

std::string Lower(std::string_view text) {
  std::string out(text);
  for (char& c : out) {
    if (c >= 'A' && c <= 'Z') {
      c = static_cast< char >(c - 'A' + 'a');
    }
  }
  return out;
}

// A handler that cannot do its job calls Fail(why): Execute then answers `=> err: <why>` in place of
// `=> ok`. Handlers run one at a time on the game thread, so one slot is enough.
std::string sFailure;

void Fail(const std::string& why) { sFailure = why; }

// The name tables are generated from the upstream enums by tools/gen_console_names.py.
struct SEnumName {
  uint value;
  const char* name;
};
#include "console_names.inc"

constexpr int kMessageCount = static_cast< int >(sizeof(kMessageNames) / sizeof(SEnumName));
constexpr int kStateCount = static_cast< int >(sizeof(kStateNames) / sizeof(SEnumName));
constexpr int kItemCount = static_cast< int >(sizeof(kItemNames) / sizeof(SEnumName));

const char* EnumName(const SEnumName* table, int count, uint value) {
  for (int i = 0; i < count; ++i) {
    if (table[i].value == value) {
      return table[i].name;
    }
  }
  return nullptr;
}

// The table's name for `value`, or its hex code when the table has no entry for it.
std::string NameOrHex(const SEnumName* table, int count, uint value) {
  const char* name = EnumName(table, count, value);
  if (name != nullptr) {
    return name;
  }
  char hex[16];
  std::snprintf(hex, sizeof(hex), "%08x", value);
  return hex;
}

// The value an argument names: an entry of `table` (case-insensitive), else a number (base 0), else
// -1.
int EnumValue(const SEnumName* table, int count, std::string_view name) {
  const std::string wanted = Lower(name);
  for (int i = 0; i < count; ++i) {
    if (Lower(table[i].name) == wanted) {
      return static_cast< int >(table[i].value);
    }
  }
  const std::string copy(name);
  char* end = nullptr;
  const long value = std::strtol(copy.c_str(), &end, 0);
  return (!copy.empty() && end != nullptr && *end == '\0') ? static_cast< int >(value) : -1;
}

bool ParseFloat(std::string_view text, float& out) {
  const std::string copy(text);
  char* end = nullptr;
  const double value = std::strtod(copy.c_str(), &end);
  if (copy.empty() || end == nullptr || *end != '\0') {
    return false;
  }
  out = static_cast< float >(value);
  return true;
}

// The id as the game packs it (TUniqueId::value); Value() would drop the version bits. Read as hex,
// which is how every reply prints an id (`u046e`, and 0x046e/u046e are accepted too): `obj 400` is
// u0400, not decimal 400, so an id can always be typed back exactly as it was printed.
bool ParseUid(std::string_view text, TUniqueId& out) {
  std::string copy(text);
  if (!copy.empty() && (copy[0] == 'u' || copy[0] == 'U')) {
    copy.erase(0, 1);
  }
  if (copy.size() > 2 && copy[0] == '0' && (copy[1] == 'x' || copy[1] == 'X')) {
    copy.erase(0, 2);
  }
  char* end = nullptr;
  const unsigned long value = std::strtoul(copy.c_str(), &end, 16);
  if (copy.empty() || end == nullptr || *end != '\0' || value > 0xffff) {
    return false;
  }
  out = TUniqueId(static_cast< ushort >(value));
  return true;
}

// The class of an entity, demangled where the compiler can do it.
std::string ClassName(const CEntity& ent) {
  const char* mangled = typeid(ent).name();
#if defined(__GNUC__)
  int status = 0;
  char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
  const std::string name = status == 0 && demangled != nullptr ? demangled : mangled;
  std::free(demangled);
  return name;
#else
  return mangled;
#endif
}

// `visor` and `beam` take a name or an index in these orders, which are CPlayerState's enum orders
// and not the item order.
constexpr CPlayerState::EPlayerVisor kVisorOrder[] = {
    CPlayerState::kPV_Combat,
    CPlayerState::kPV_Echo,
    CPlayerState::kPV_Scan,
    CPlayerState::kPV_Dark,
};

struct SBeamChoice {
  const char* name;
  CPlayerState::EBeamId beam;
  CPlayerState::EItemType item; // The power-up that owns the beam.
};

constexpr SBeamChoice kBeamOrder[] = {
    {"power", CPlayerState::kBI_Power, CPlayerState::kIT_PowerBeam},
    {"dark", CPlayerState::kBI_Dark, CPlayerState::kIT_DarkBeam},
    {"light", CPlayerState::kBI_Light, CPlayerState::kIT_LightBeam},
    {"annihilator", CPlayerState::kBI_Annihilator, CPlayerState::kIT_AnnihilatorBeam},
};

constexpr int kVisorCount = static_cast< int >(sizeof(kVisorOrder) / sizeof(kVisorOrder[0]));
constexpr int kBeamCount = static_cast< int >(sizeof(kBeamOrder) / sizeof(kBeamOrder[0]));

const SBeamChoice* BeamChoice(CPlayerState::EBeamId beam) {
  for (const SBeamChoice& choice : kBeamOrder) {
    if (choice.beam == beam) {
      return &choice;
    }
  }
  return nullptr;
}

bool ParseVisor(std::string_view text, CPlayerState::EPlayerVisor& out) {
  const std::string wanted = Lower(text);
  for (const CPlayerState::EPlayerVisor visor : kVisorOrder) {
    if (wanted == VisorName(visor)) {
      out = visor;
      return true;
    }
  }
  const int index = ArgToInt(text, -1);
  if (index < 0 || index >= kVisorCount) {
    return false;
  }
  out = kVisorOrder[index];
  return true;
}

const SBeamChoice* ParseBeam(std::string_view text) {
  const std::string wanted = Lower(text);
  for (const SBeamChoice& choice : kBeamOrder) {
    if (wanted == choice.name) {
      return &choice;
    }
  }
  const int index = ArgToInt(text, -1);
  return (index >= 0 && index < kBeamCount) ? &kBeamOrder[index] : nullptr;
}

void CmdHelp(CStateManager&, const Args&, std::string& out) {
  out +=
      "status                      world, area, player position, health, visor, beam, morph state\n"
      "areas                       the current world's area slots (index, MREA id, name if loaded)\n"
      "warp <slot|id|name>         load an area of the current world (what MP2_TELEPORT takes)\n"
      "nextarea | prevarea         the next or previous existing area of this world\n"
      "door | dock | station       stand in front of the nearest door / doorway / save station\n"
      "heal | kill                 refill energy, or kill the player (a respawn test)\n"
      "damage <id> <n> [kb] [w]    hit an actor for n as a player shot (w = power, missile,\n"
      "                            cannonball, dark, light, annihilator, powerbomb, darkburst,\n"
      "                            sunburst, sonicboom; kb = knockback power)\n"
      "layer <n>                   activate a layer of the current area\n"
      "god <0|1>                   god mode (the F1 cheat)\n"
      "skipcine <0|1>              skip cutscenes the player has not seen (the F1 setting)\n"
      "gunoffset [<r> <f> <u>]     arm cannon offset in metres (+-1), as the F1 Render row does\n"
      "msaa <1|4> | aniso <1..16>  anti-aliasing and anisotropic filtering (applied next frame)\n"
      "hudscale <50..100>          HUD scale in percent, as the F1 HUD row does\n"
      "crosshair [<25..100>]       aim crosshair size, `style classic|remastered`, opacity,\n"
      "                            color <r> <g> <b>|clear, aim <0|1> (F1 HUD)\n"
      "uiscale <50..200>           the F1 window's size in percent, as the Render row does\n"
      "vsync <0|1>                 vertical sync, as the F1 Render row does\n"
      "interp [0|1]                smooth frames: the camera between ticks with the cap off\n"
      "aspect [4:3|16:9|window]    aspect ratio, as the F1 Video > Display row does\n"
      "boost                       boost-ball release on the morphed player\n"
      "fastmorph <0|1>             fast morph: the morph transition takes 0.2 s, not a second\n"
      "springball <0|1>            spring ball: C-stick up in Morph Ball bomb-jumps (needs Bombs)\n"
      "lockon <0|1>                toggle lock-on: an L press latches until the next press\n"
      "sticky <0|1>                sticky charge: a long A hold keeps charging after the release\n"
      "pointeraim [on|off|<x> <y>] pointer aim (the F1 row), or place its pointer (-1..1)\n"
      "save [info] [<file>]        the card's save slots, or one .gci on disk: world, items, time\n"
      "save export [<path>]        write the card's save file out (default: save-backups/)\n"
      "save import <path|latest>   put a .gci back on the card; the file it replaces is kept\n"
      "save backup | backups       copy the card now, or list the copies in save-backups\n"
      "cardbackup <0|1>            back the card up before each write (the F1 Saves row)\n"
      "settings [<key> <value>|self-test]  settings.cfg's entries; set one (and save), or check every key round-trips\n"
      "logfile <0|1>               stdout and stderr also go to metroid_prime2_port.log (next launch)\n"
      "objs [filter]               objects of the world: uid, editor id, class, area, position\n"
      "obj <id>                    one object (an id as `objs` prints it: u046e): health, fsm, messages\n"
      "send <id> <msg>             deliver a script message to an object (name or number)\n"
      "hurt <id> <amount> [weapon] damage an object as a shot would (weapon 0 = power beam)\n"
      "frame                       the game updates run so far (what MP2_FRAMES counts)\n"
      "fsm <id> <state>            force a patterned actor's AI state by name (as `obj` prints it; FSM2 sub-machines too)\n"
      "place <id> <x y z>|player [d]  move an actor to a point, or d (6) units in front of the player\n"
      "shoot <id> [charged] [d] [dir x y z]  fire the beam in hand at an actor from d (3) units\n"
      "                            (travelling along dir; default from the player): a real projectile\n"
      "visor [name|0-3]            show or switch the visor (combat, echo, scan, dark)\n"
      "beam [name|0-3]             show or switch the beam (power, dark, light, annihilator)\n"
      "tp <x> <y> <z>              teleport the player, keeping its facing\n"
      "face <yaw deg>              turn the player where it stands (0 = +y, 90 = +x)\n"
      "aim <yaw> <pitch>          point the view outright, in degrees; negative pitch looks down\n"
      "items                       power-up amounts and capacities\n"
      "world <world> [area]        load another world (the MP1 console's `warp <world>`)\n"
      "worlds                      the memory card's world ids, areas and name tables\n"
      "strg <id|name> [index]      a string table as the game loads it\n"
      "give <item> [n] | take      add an item (capacity and amount), or remove capacity\n"
      "press <a+b+...> [updates]   hold pad buttons (0 updates: until the next press/stick)\n"
      "grapple <id> [d]            wake a grapple point, stand d (8) units from it facing it, hold L\n"
      "stick <x> <y> [updates]     hold the main stick, -127..127\n"
      "cstick <x> <y> [updates]    hold the C-stick (Spring Ball in Morph Ball), -127..127\n"
      "screenshot                  the F12 capture; the log names the file it wrote\n"
      "help                        this list\n"
      "a; b; c                     run several commands in the same game tick (one reply each)\n";
}

void CmdStatus(CStateManager& mgr, const Args&, std::string& out) {
  CWorld* world = mgr.World();
  CPlayer* player = mgr.GetPlayer(0);
  const CPlayerState* state = mgr.GetPlayerState(0);
  char line[320];
  std::snprintf(line, sizeof(line), "world %08x", world != nullptr ? world->IGetWorldAssetId() : 0u);
  out += line;
  if (world != nullptr) {
    const TAreaId area = world->GetCurrentAreaId();
    const CGameArea* loaded = world->Area(area);
    std::snprintf(line, sizeof(line), " area %d (mrea %08x)", area.Value(),
                  loaded != nullptr ? loaded->GetAreaAssetId() : 0u);
    out += line;
  }
  out += '\n';
  if (player != nullptr) {
    const CVector3f p = player->GetTranslation();
    std::snprintf(line, sizeof(line), "position %.2f %.2f %.2f  speed %.2f", p.GetX(), p.GetY(),
                  p.GetZ(), player->GetVelocityWR().Magnitude());
    out += line;
    std::snprintf(line, sizeof(line), "  morph %d  in-turret %d\n", static_cast< int >(player->GetMorphballTransitionState()),
                  player->IsInTurret() ? 1 : 0);
    out += line;
    // The input the game is acting on, which is not the pad's own: Toggle Lock-On and Sticky
    // Charge rewrite L and A after the pad has been read, so a latched button shows here and not
    // in `press`.
    const CFinalInput& in = mgr.mFinalInputs[0];
    std::snprintf(line, sizeof(line), "  orbit %d  game input L=%d A=%d\n",
                  static_cast< int >(player->GetOrbitState()), in.DL() || in.DLTrigger() ? 1 : 0,
                  in.DA() ? 1 : 0);
    out += line;
  }
  if (state != nullptr) {
    std::snprintf(line, sizeof(line), "health %.0f/%.0f  visor %s  beam %d\n",
                  state->GetHealthInfo().GetHP(), state->GetHealthInfo().GetInitialHP(),
                  VisorName(state->GetActiveVisor(mgr)), static_cast< int >(state->GetCurrentBeam()));
    out += line;
  }
  // Where the camera the frame is drawn from actually is: the player's position says nothing about
  // a detached camera, which is what a "nothing renders" report usually turns out to be.
  if (const CCameraManager* cameras = mgr.GetCameraManager(0)) {
    const CGameCamera* camera = cameras->GetCurrentCamera(mgr, false);
    const CTransform4f cameraXf = cameras->GetCurrentCameraTransform(mgr, false);
    const CVector3f at = cameraXf.GetTranslation();
    const char* kind = cameras->IsInCinematicCamera() ? "cinematic"
                       : cameras->IsInFPCamera()      ? "first-person"
                       : cameras->IsInBallCamera()    ? "ball"
                                                      : "other";
    std::snprintf(line, sizeof(line), "camera %s fov %.1f at (%.2f, %.2f, %.2f)\n", kind,
                  camera != nullptr ? camera->GetFov() : 0.f, at.GetX(), at.GetY(), at.GetZ());
    out += line;
    {
      // The view's pitch, so an aim drag's release can be read as a number.
      const CVector3f fwd = cameraXf.GetForward();
      std::snprintf(line, sizeof(line), "camera pitch %.1f deg (aim %s)\n",
                    asinf(fwd.GetZ()) * (180.f / 3.14159265f), PortMouse::Aiming() ? "on" : "off");
      out += line;
    }
    // The area's fog, because a white screen with the HUD drawn on it is what a fog range of
    // "everything" looks like from inside.
    const CGameArea::CAreaFog& fog = cameras->GetFog();
    const CColor fogColor = fog.GetColor();
    const CVector2f fogRange = fog.GetRange();
    std::snprintf(line, sizeof(line), "fog mode %d colour (%.2f, %.2f, %.2f) range %.1f..%.1f%s\n",
                  static_cast< int >(fog.GetFogMode()), fogColor.GetRed(), fogColor.GetGreen(),
                  fogColor.GetBlue(), fogRange.GetX(), fogRange.GetY(),
                  fog.IsFogDisabled() ? " disabled" : "");
    out += line;
  }
  // What the renderer will walk: CStateManager::GatherVisibleAreas only takes areas whose occlusion
  // state is kOS_Visible, so a current area that is not visible is a blank world with a normal HUD.
  if (CWorld* world = mgr.World()) {
    int alive = 0;
    std::string visibleList;
    for (CGameArea::CConstChainIterator it = world->GetChainHead(CWorld::kC_Alive);
         it != CWorld::GetAliveAreasEnd(); ++it) {
      ++alive;
      if (it->GetOcclusionState() == CGameArea::kOS_Visible) {
        visibleList += ' ';
        visibleList += std::to_string(it->GetId().Value());
      }
    }
    const CGameArea* current = world->Area(world->GetCurrentAreaId());
    std::snprintf(line, sizeof(line), "areas alive %d, visible:%s; current occlusion %s\n", alive,
                  visibleList.c_str(),
                  current == nullptr   ? "(not loaded)"
                  : current->GetOcclusionState() == CGameArea::kOS_Visible ? "visible"
                                                                           : "occluded");
    out += line;
    // How much the last frame asked the GPU to do. A world that is not drawn at all shows up here
    // as a handful of draw calls (the HUD) instead of hundreds.
    // The escape sequence's timer: while it is in (0, 1) CStateManager::DrawAdditionalFilters adds a
    // full-screen white filter whose alpha is 1 - timer, and the timer's own update clamps it to
    // FLT_EPSILON rather than 0 - so a timer left running is a permanently white screen under the
    // HUD. One line here saves the whole hunt.
    std::snprintf(line, sizeof(line), "escape timer %.6f%s\n",
                  gpGameState != nullptr ? gpGameState->GetEscapeTime() : 0.f,
                  gpGameState != nullptr && gpGameState->GetEscapeTime() > 0.f
                      ? "  <- the screen goes white"
                      : "");
    out += line;
    // The world transition manager's own state: kTT_Portal left running draws an additive white
    // full-screen quad (CWorldTransManager::DrawPortalTransition) that hides the world.
    if (!gpGameState->WorldTransitionManager().IsNull()) {
      const CWorldTransManager* trans = gpGameState->WorldTransitionManager().GetPtr();
      std::snprintf(line, sizeof(line), "transition type %d finished %d\n",
                    static_cast< int >(trans->GetTransType()),
                    trans->IsTransitionFinished() ? 1 : 0);
      out += line;
    }
    const CViewport& vp = CGraphics::GetViewport();
    std::snprintf(line, sizeof(line), "viewport %d,%d %dx%d\n", vp.mLeft, vp.mTop, vp.mWidth,
                  vp.mHeight);
    out += line;
    const AuroraStats* stats = aurora_get_stats();
    std::snprintf(line, sizeof(line), "frame %.1f fps, %u draw calls, %u verts%s\n",
                  aurora_get_fps(), stats != nullptr ? stats->drawCallCount : 0u,
                  stats != nullptr ? stats->lastVertSize : 0u,
                  stats != nullptr && stats->drawCallCount < 40 ? "  <- almost nothing drawn" : "");
    out += line;
  }
}

void CmdAreas(CStateManager& mgr, const Args&, std::string& out) {
  CWorld* world = mgr.World();
  if (world == nullptr) {
    out += "no world yet\n";
    return;
  }
  // Per area: phase/chain/active, the live objects it owns (what holds the 1024-entry id pool) and
  // the areas its docks attach, so "who loaded this neighbour" can be read off one reply.
  const int count = world->IGetAreaCount();
  std::vector< int > owned(count > 0 ? count : 0, 0);
  const CObjectList& list = mgr.GetObjectListById(kOL_All);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const int a = list[i]->GetCurrentAreaId().Value();
    if (a >= 0 && a < count) {
      ++owned[a];
    }
  }
  char line[160];
  for (int i = 0; i < count; ++i) {
    const CGameArea* area = world->Area(TAreaId(i));
    const IGameArea* any = world->IGetAreaAlways(TAreaId(i));
    std::snprintf(line, sizeof(line), "%2d  %08x  %s", i,
                  any != nullptr ? any->IGetAreaAssetId() : 0u,
                  area != nullptr ? "loaded" : "");
    out += line;
    if (area != nullptr) {
      std::snprintf(line, sizeof(line), " phase %d chain %d%s%s  %d objs  attached", area->GetPhase(),
                    area->GetCurChain(), area->IsActive() ? " active" : "",
                    area->IsLoaded() ? " ready" : "", owned[i]);
      out += line;
      for (int k = 0; k < area->GetNumAttachedAreas(); ++k) {
        std::snprintf(line, sizeof(line), " %d", area->GetAttachedAreaId(k).Value());
        out += line;
      }
    }
    out += '\n';
  }
}

void CmdWarp(CStateManager&, const Args& args, std::string& out) {
  if (args.size() < 2) {
    Fail("usage: warp <slot|id|name>");
    return;
  }
  const std::string area = Str(args[1]);
  PortWarp::Area(area.c_str());
  out += "warp queued for '" + area + "'\n";
}

void CmdNextArea(CStateManager&, const Args&, std::string&) { PortWarp::CycleArea(1); }
void CmdPrevArea(CStateManager&, const Args&, std::string&) { PortWarp::CycleArea(-1); }
void CmdDoor(CStateManager&, const Args&, std::string&) { PortWarp::Door(); }
void CmdDock(CStateManager&, const Args&, std::string&) { PortWarp::Dock(); }
void CmdStation(CStateManager&, const Args&, std::string&) { PortWarp::Station(); }
void CmdHeal(CStateManager&, const Args&, std::string&) { PortWarp::Heal(); }
void CmdKill(CStateManager&, const Args& args, std::string&) {
  if (args.size() > 1) {
    Fail("usage: kill   (kills the player; for an actor use `damage <id> <amount>`)");
    return;
  }
  PortWarp::Kill();
}

void CmdLayer(CStateManager&, const Args& args, std::string&) {
  if (args.size() < 2) {
    Fail("usage: layer <n>");
    return;
  }
  PortWarp::Layer(ArgToInt(args[1], 0));
}

// world <world> [area]: the MP1 console's `warp <world>`, under its own verb because this tree's
// `warp` is the area warp the sweeps and the docs use. Both texts are a name or a hex asset id and
// are resolved where the resource names live (PortWarpToWorld), so nothing here needs a table.
void CmdWorld(CStateManager&, const Args& args, std::string& out) {
  if (args.size() < 2) {
    Fail("usage: world <world name or hex> [area name or hex]");
    return;
  }
  const std::string world = Str(args[1]);
  const std::string area = args.size() > 2 ? Str(args[2]) : std::string();
  PortWarp::World(world.c_str(), area.c_str());
  out += "world warp queued for '" + world + "'" + (area.empty() ? "" : " area '" + area + "'") +
         "\n";
}

// worlds: the memory card's world table, which is what `world <id>` takes. The names are STRG assets
// (the card keeps the id, not the text), so `strg <that id>` prints one.
void CmdWorlds(CStateManager& mgr, const Args&, std::string& out) {
  if (gpMemoryCard == nullptr) {
    Fail("no memory card yet");
    return;
  }
  const rstl::vector< CMemoryCard::MemoryWorld >& worlds = gpMemoryCard->GetMemoryWorlds();
  const CWorld* world = mgr.World();
  const CAssetId current = world != nullptr ? world->IGetWorldAssetId() : kInvalidAssetId;
  char line[160];
  for (int i = 0; i < worlds.size(); ++i) {
    const CAssetId id = worlds[i].first;
    const CSaveWorldMemory& memory = worlds[i].second;
    std::snprintf(line, sizeof(line), "%08x  %2d areas  name STRG %08x%s\n", id,
                  memory.GetAreaCount(), memory.GetWorldNameId(),
                  id == current ? "  <- current" : "");
    out += line;
  }
}

// One wchar_t string as a console line. The tables are English ASCII; anything above that is a '?'
// rather than a byte soup, because the reply goes out as UTF-8 over the socket.
std::string Narrow(const wchar_t* text) {
  std::string out;
  if (text == nullptr) {
    return out;
  }
  for (const wchar_t* it = text; *it != 0; ++it) {
    out += *it < 0x80 ? static_cast< char >(*it) : '?';
  }
  return out;
}

// The string table `strg` is reading (see there).
TToken< CStringTable >* sTable = nullptr;
CAssetId sTableId = kInvalidAssetId;

// strg <id|name> [index|name]: a string table as the game loads it, for reading a message a level
// script refers to. The asset is a number or a resource name ("STRG_AllPickupsFound_2"), and the
// load is asynchronous, so a second call is what reads it when the first only locked it.
void CmdStrg(CStateManager&, const Args& args, std::string& out) {
  if (args.size() < 2) {
    Fail("usage: strg <asset id or STRG_... name> [index|name]");
    return;
  }
  CAssetId id = kInvalidAssetId;
  const int numeric = ArgToInt(args[1], -1);
  if (numeric >= 0) {
    id = static_cast< CAssetId >(numeric);
  } else if (gpResourceFactory != nullptr) {
    const SObjectTag* tag = gpResourceFactory->GetResourceIdByName(Str(args[1]).c_str());
    if (tag != nullptr) {
      id = tag->id;
    }
  }
  if (id == kInvalidAssetId) {
    Fail("no such resource '" + Str(args[1]) + "'");
    return;
  }
  // The table is kept alive between calls: the load is asynchronous, and a token that dies with the
  // command cancels it, so every call would answer "still loading".
  if (sTable == nullptr || sTableId != id) {
    delete sTable;
    sTable = rs_new TToken< CStringTable >(gpSimplePool->GetObj(SObjectTag('STRG', id)));
    sTableId = id;
    sTable->Lock();
  }
  if (!sTable->IsLoaded()) {
    out += "STRG " + std::to_string(id) + " is still loading; ask again\n";
    return;
  }
  TToken< CStringTable >& table = *sTable;
  const int count = table->GetStringCount();
  char line[320];
  if (args.size() > 2) {
    int index = ArgToInt(args[2], -1);
    if (index < 0) {
      index = table->GetStringIndex(Str(args[2]).c_str());
    }
    if (index < 0 || index >= count) {
      Fail("no string '" + Str(args[2]) + "' in STRG " + std::to_string(id));
      return;
    }
    std::snprintf(line, sizeof(line), "STRG %u [%d]: %s\n", id, index,
                  Narrow(table->GetString(index)).c_str());
    out += line;
    return;
  }
  std::snprintf(line, sizeof(line), "STRG %u: %d strings\n", id, count);
  out += line;
  // Enough to see what a table is without flooding the socket.
  for (int i = 0; i < count && i < 40; ++i) {
    std::snprintf(line, sizeof(line), "  [%2d] %s\n", i, Narrow(table->GetString(i)).c_str());
    out += line;
  }
  if (count > 40) {
    out += "  ...\n";
  }
}

void CmdGod(CStateManager&, const Args& args, std::string& out) {
  const bool enabled = args.size() < 2 ? !PortOverlay::GodMode() : ArgToInt(args[1], 1) != 0;
  PortOverlay::SetGodMode(enabled);
  out += enabled ? "god mode on\n" : "god mode off\n";
}

void CmdFreeCam(CStateManager& mgr, const Args& args, std::string& out) {
  const bool enabled = args.size() < 2 ? !PortFreeCam::Active() : ArgToInt(args[1], 1) != 0;
  PortFreeCam::SetActive(enabled, &mgr);
  out += enabled ? "free camera on\n" : "free camera off\n";
}

// `photo [1|2|4]`: saves a PNG of the free camera's picture, supersampled by the factor (default
// the supersample setting). The shot is taken a few frames later; its result is logged as `[photo] ...`.
void CmdPhoto(CStateManager&, const Args& args, std::string& out) {
  if (args.size() >= 2 && args[1] != "1" && args[1] != "2" && args[1] != "4") {
    out += "usage: photo [1|2|4]\n";
    return;
  }
  if (PortPhoto::Capturing()) {
    out += "a capture is already running\n";
    return;
  }
  if (!PortPhoto::Active()) {
    out += "photo needs the free camera: freecam 1\n";
    return;
  }
  const int factor = args.size() < 2 ? 0 : ArgToInt(args[1], 1);
  PortPhoto::RequestCapture(factor);
  const PortPhoto::CaptureResult last = PortPhoto::LastCapture();
  out += "capture requested (";
  out += factor > 0 ? std::to_string(factor) : std::to_string(PortPhoto::Supersample());
  out += "x); last: ";
  out += last.state == 0 ? "none" : last.state > 0 ? "saved" : "failed";
  out += "\n";
}

void CmdSkipCine(CStateManager&, const Args& args, std::string& out) {
  const bool enabled = args.size() < 2 ? !PortOverlay::SkipCinematics() : ArgToInt(args[1], 1) != 0;
  PortOverlay::SetSkipCinematics(enabled);
  out += enabled ? "skip cutscenes on\n" : "skip cutscenes off\n";
}

// The gameplay tweaks: the same shape as `god` and `skipcine`, an optional 0/1 and a print of the
// state it ends in, because the F1 row and the env var set the same thing.
void CmdFastMorph(CStateManager&, const Args& args, std::string& out) {
  const bool enabled = args.size() < 2 ? !PortOverlay::FastMorph() : ArgToInt(args[1], 1) != 0;
  PortOverlay::SetFastMorph(enabled);
  out += enabled ? "fast morph on\n" : "fast morph off\n";
}

// boost: start a boost-ball charge release on the morphed player (what releasing a held A does), so
// lanes reach boost-hit arms without timing pad input.
void CmdBoost(CStateManager& mgr, const Args&, std::string& out) {
  CPlayer* player = mgr.GetPlayer(0);
  if (player == nullptr || player->GetMorphBall() == nullptr ||
      player->GetMorphballTransitionState() != CPlayer::kMS_Morphed) {
    Fail("boost: the player is not morphed (press x, or 'press x 1')");
    return;
  }
  player->GetMorphBall()->EnterBoosting(mgr, false);
  out += "boosting\n";
}

void CmdSpringBall(CStateManager&, const Args& args, std::string& out) {
  const bool enabled = args.size() < 2 ? !PortOverlay::SpringBall() : ArgToInt(args[1], 1) != 0;
  PortOverlay::SetSpringBall(enabled);
  out += enabled ? "spring ball on\n" : "spring ball off\n";
}

void CmdLockOn(CStateManager&, const Args& args, std::string& out) {
  const bool enabled = args.size() < 2 ? !PortOverlay::LockOnToggle() : ArgToInt(args[1], 1) != 0;
  PortOverlay::SetLockOnToggle(enabled);
  out += enabled ? "toggle lock-on on\n" : "toggle lock-on off\n";
}

void CmdSticky(CStateManager&, const Args& args, std::string& out) {
  const bool enabled = args.size() < 2 ? !PortOverlay::StickyCharge() : ArgToInt(args[1], 1) != 0;
  PortOverlay::SetStickyCharge(enabled);
  out += enabled ? "sticky charge on\n" : "sticky charge off\n";
}

// pointeraim [on|off|<x> <y>]: MP1's console command. on/off is the F1 Controls row
// (`MP2_POINTER_AIM`); x, y place the pointer (-1..1 of the half view, x right, y down), which
// holds only while pointer aim is free. Every form prints the state it ends in.
void CmdPointerAim(CStateManager&, const Args& args, std::string& out) {
  if (args.size() == 2 && (args[1] == "on" || args[1] == "off")) {
    PortOverlay::SetPointerAim(args[1] == "on");
  } else if (args.size() == 3) {
    PortMouse::SetPointerAimPosition(static_cast< float >(std::atof(std::string(args[1]).c_str())),
                                     static_cast< float >(std::atof(std::string(args[2]).c_str())));
  } else if (args.size() != 1) {
    Fail("pointeraim [on|off|<x> <y>]   x, y in -1..1 of the half view");
    return;
  }
  float x = 0.f;
  float y = 0.f;
  const bool free = PortMouse::PointerAimPosition(x, y);
  char line[96];
  std::snprintf(line, sizeof(line), "pointeraim %s %s x=%.3f y=%.3f\n",
                PortOverlay::PointerAim() ? "on" : "off", free ? "free" : "centred", x, y);
  out += line;
}

// cardbackup [0|1]: the F1 Saves row (`MP2_CARD_BACKUP`) - copy the card's save file into
// save-backups before every write the game makes to it. Bare, it prints the state it is in.
void CmdCardBackup(CStateManager&, const Args& args, std::string& out) {
  bool enabled = PortOverlay::CardBackup();
  if (args.size() >= 2) {
    if (args.size() > 2 || (args[1] != "0" && args[1] != "1")) {
      Fail("cardbackup <0|1>");
      return;
    }
    enabled = args[1] == "1";
    PortOverlay::SetCardBackup(enabled);
  }
  out += enabled ? "card backup before writes: on (MP2_CARD_BACKUP)\n"
                 : "card backup before writes: off (MP2_CARD_BACKUP)\n";
}

// livesplit <0|1> | addr <host:port> | send <command> | status: the F1 Game > Speedrun rows
// (`MP2_LIVESPLIT`, `MP2_LIVESPLIT_ADDR`) - the LiveSplit Server client.
void CmdLiveSplit(CStateManager&, const Args& args, std::string& out) {
  const std::string action = args.size() > 1 ? std::string(args[1]) : "status";
  if (action == "0" || action == "1") {
    PortLiveSplit::SetEnabled(action == "1");
  } else if (action == "addr" && args.size() == 3) {
    PortLiveSplit::SetAddress(std::string(args[2]));
  } else if (action == "send" && args.size() > 2) {
    std::string line(args[2]);
    for (size_t i = 3; i < args.size(); ++i) {
      line += ' ';
      line.append(args[i]);
    }
    PortLiveSplit::SendRaw(line);
  } else if (action != "status") {
    Fail("livesplit <0|1> | addr <host:port> | send <command> | status");
    return;
  }
  static const char* const kStatusNames[] = {"off", "connecting", "connected", "failed"};
  out += std::string("livesplit ") + (PortLiveSplit::Enabled() ? "on " : "off ") +
         PortLiveSplit::Address() + " " + kStatusNames[PortLiveSplit::Status()] + "\n";
  const std::string error = PortLiveSplit::LastError();
  if (!error.empty()) {
    out += "last error: " + error + "\n";
  }
}

// logfile [0|1]: the F1 Testing row (`MP2_LOG_FILE`) - everything the process prints also goes
// to metroid_prime2_port.log in the user folder. PortLogFile::Start reads the setting before
// Aurora comes up, so a change here is what the next launch will do, not this one.
void CmdLogFile(CStateManager&, const Args& args, std::string& out) {
  bool enabled = PortOverlay::LogFile();
  if (args.size() >= 2) {
    if (args.size() > 2 || (args[1] != "0" && args[1] != "1")) {
      Fail("logfile <0|1>");
      return;
    }
    enabled = args[1] == "1";
    PortOverlay::SetLogFile(enabled);
  }
  const std::string path = PortLogFile::Path();
  out += enabled ? "log file: on (" : "log file: off (";
  out += path.empty() ? std::string("no file") : path;
  out += ")\n(takes effect next launch)\n";
}

// settings | settings <key> <value> | settings self-test: settings.cfg's entries (an MP2_<KEY>
// environment variable that overrides one is named); with a key and a value, sets it and saves
// settings.cfg; self-test writes every key, reads it back, and reports any that does not.
void CmdSettings(CStateManager&, const Args& args, std::string& out) {
  if (args.size() == 1) {
    for (const auto& [key, value] : PortOverlaySettings::SettingsValues(PortOverlaySettings::SettingsText())) {
      out += key + "=" + value;
      const std::string env = PortOverlaySettings::EnvName(key);
      if (const char* set = port::EnvString(env.c_str())) {
        out += "  (env " + env + "=" + set + " wins)";
      }
      out += "\n";
    }
    return;
  }
  if (args.size() == 2 && args[1] == "self-test") {
    // The test leaves the live variables at its samples: put back what was there, unmarked.
    const std::string before = PortOverlaySettings::SettingsText();
    PortOverlaySettings::SettingsSelfTest(out);
    std::istringstream restore(before);
    PortOverlaySettings::ApplySettingsText(restore);
    return;
  }
  if (args.size() != 3) {
    Fail("usage: settings [<key> <value> | self-test]");
    return;
  }
  if (!PortOverlaySettings::ApplySetting(std::string(args[1]), std::string(args[2]))) {
    Fail(("unknown setting " + std::string(args[1])).c_str());
    return;
  }
  PortOverlaySettings::MarkDirty();
  PortOverlaySettings::SaveSettings();
  const auto values = PortOverlaySettings::SettingsValues(PortOverlaySettings::SettingsText());
  out += std::string(args[1]) + "=" + values.at(std::string(args[1])) + "\n";
}

void CmdGunOffset(CStateManager&, const Args& args, std::string& out) {
  float offset[3];
  PortOverlay::GunOffset(offset);
  if (args.size() >= 4) {
    float parsed[3];
    for (int i = 0; i < 3; ++i) {
      if (!ParseFloat(args[i + 1], parsed[i])) {
        Fail("gunoffset takes three numbers: right forward up (metres, +-1)");
        return;
      }
      if (parsed[i] < -PortOverlay::kGunOffsetMax || parsed[i] > PortOverlay::kGunOffsetMax) {
        Fail("each offset is clamped to +-1 m");
        return;
      }
    }
    PortOverlay::SetGunOffset(parsed);
    PortOverlay::GunOffset(offset);
  } else if (args.size() != 1) {
    Fail("usage: gunoffset [<right> <forward> <up>]");
    return;
  }
  char line[128];
  std::snprintf(line, sizeof(line), "gun offset %.2f %.2f %.2f m\n", offset[0], offset[1],
                offset[2]);
  out += line;
}

void CmdMsaaAniso(CStateManager&, const Args& args, std::string& out) {
  if (args.size() < 2) {
    char line[128];
    std::snprintf(line, sizeof(line), "msaa %dx, anisotropy %dx\n", PortOverlay::Msaa(),
                  PortOverlay::Anisotropy());
    out += line;
    return;
  }
  // The command's own name is the setting (`msaa <n>`), the argument is the value.
  const bool isMsaa = args[0] == "msaa";
  const bool isAniso = args[0] == "aniso";
  if (!isMsaa && !isAniso) {
    Fail("usage: msaa <1|4> | aniso <1..16>");
    return;
  }
  const int value = ArgToInt(args[1], -1);
  if (isMsaa) {
    if (value != 1 && value != 4) {
      Fail("MSAA is 1 (off) or 4");
      return;
    }
    PortOverlay::SetMsaa(value);
  } else {
    if (value < 1 || value > 16) {
      Fail("anisotropy is 1..16");
      return;
    }
    PortOverlay::SetAnisotropy(value);
  }
  // PortOverlay::Draw hands it to aurora_set_graphics_quality, which rebuilds the framebuffers at the
  // next frame start.
  char line[128];
  std::snprintf(line, sizeof(line), "msaa %dx, anisotropy %dx (applied next frame)\n",
                PortOverlay::Msaa(), PortOverlay::Anisotropy());
  out += line;
}

void CmdHudScale(CStateManager&, const Args& args, std::string& out) {
  if (args.size() < 2) {
    char line[64];
    std::snprintf(line, sizeof(line), "hud scale %d%%\n", PortOverlay::HudScale());
    out += line;
    return;
  }
  const int value = ArgToInt(args[1], -1);
  if (value < PortOverlay::kHudScaleMin || value > PortOverlay::kHudScaleMax) {
    Fail("hud scale is 50..100 percent (100 = the game's own layout)");
    return;
  }
  PortOverlay::SetHudScale(value);
  char line[64];
  std::snprintf(line, sizeof(line), "hud scale %d%%\n", PortOverlay::HudScale());
  out += line;
}

// crosshair [<25..100> | style classic|remastered | opacity <0..100> | color <r> <g> <b> | color
// clear]: the Prime port's crosshair settings, same meanings.
void CmdCrosshair(CStateManager&, const Args& args, std::string& out) {
  if (args.size() >= 2) {
    const std::string_view what = args[1];
    if (what == "style" && args.size() >= 3) {
      const std::string_view style = args[2];
      if (style != "classic" && style != "remastered") {
        Fail("crosshair style is classic or remastered");
        return;
      }
      PortOverlay::SetCrosshairStyle(style == "remastered"
                                         ? PortOverlay::kCrosshairStyle_Remastered
                                         : PortOverlay::kCrosshairStyle_Classic);
    } else if (what == "opacity" && args.size() >= 3) {
      const int percent = ArgToInt(args[2], -1);
      if (percent < PortOverlay::kCrosshairOpacityMin ||
          percent > PortOverlay::kCrosshairOpacityMax) {
        Fail("crosshair opacity is 0..100 percent");
        return;
      }
      PortOverlay::SetCrosshairOpacity(percent);
    } else if (what == "color") {
      if (args.size() >= 5) {
        const float r = static_cast< float >(std::atof(std::string(args[2]).c_str()));
        const float g = static_cast< float >(std::atof(std::string(args[3]).c_str()));
        const float b = static_cast< float >(std::atof(std::string(args[4]).c_str()));
        PortOverlay::SetCrosshairColor(r, g, b);
      } else {
        PortOverlay::ClearCrosshairColor();
      }
    } else if (what == "aim" && args.size() >= 3) {
      PortOverlay::SetAimCrosshair(args[2] != "0");
    } else {
      const int percent = ArgToInt(args[1], -1);
      if (percent < PortOverlay::kCrosshairSizeMin || percent > PortOverlay::kCrosshairSizeMax) {
        Fail("usage: crosshair [<25..100> | style classic|remastered | opacity <0..100> | color "
             "<r> <g> <b> | color clear | aim <0|1>]");
        return;
      }
      PortOverlay::SetCrosshairSize(percent);
    }
  }
  char line[160];
  std::snprintf(line, sizeof(line), "crosshair %s, %d%%, %d%% opacity, aim %s\n",
                PortOverlay::CrosshairStyle() == PortOverlay::kCrosshairStyle_Remastered
                    ? "remastered"
                    : "classic",
                PortOverlay::CrosshairSize(), PortOverlay::CrosshairOpacity(),
                PortOverlay::AimCrosshair() ? "on" : "off");
  out += line;
}

void CmdUiScale(CStateManager&, const Args& args, std::string& out) {
  if (args.size() >= 2) {
    const int value = ArgToInt(args[1], -1);
    if (value < PortOverlay::kUiScaleMin || value > PortOverlay::kUiScaleMax) {
      Fail("ui scale is 50..200 percent");
      return;
    }
    PortOverlay::SetUiScale(value);
  }
  char line[64];
  std::snprintf(line, sizeof(line), "ui scale %d%%\n", PortOverlay::UiScale());
  out += line;
}

void CmdVsync(CStateManager&, const Args& args, std::string& out) {
  if (args.size() >= 2) {
    const int value = ArgToInt(args[1], -1);
    if (value != 0 && value != 1) {
      Fail("vsync is 0 or 1");
      return;
    }
    PortOverlay::SetVsync(value == 1);
  }
  out += PortOverlay::Vsync() ? "vsync on\n" : "vsync off\n";
}

void CmdInterp(CStateManager&, const Args& args, std::string& out) {
  if (args.size() >= 2) {
    const int value = ArgToInt(args[1], -1);
    if (value != 0 && value != 1) {
      Fail("interp is 0 or 1");
      return;
    }
    PortOverlay::SetSmoothFrames(value == 1);
  }
  out += PortOverlay::SmoothFrames() ? "smooth frames on" : "smooth frames off";
  out += PortOverlay::FrameLimit() ? " (inactive: 60 FPS cap on)\n" : "\n";
}

// The Prime 1 port's CmdAspect; applied by PortOverlay::ApplyAspect on the next loop iteration.
void CmdAspect(CStateManager&, const Args& args, std::string& out) {
  if (args.size() >= 2) {
    std::string name(args[1]);
    for (char& c : name) {
      c = static_cast< char >(std::tolower(static_cast< unsigned char >(c)));
    }
    int mode = 0;
    if (!PortOverlay::ParseAspect(name.c_str(), mode)) {
      Fail("usage: aspect <4:3|16:9|window>");
      return;
    }
    PortOverlay::SetAspectMode(mode);
  }
  out += "aspect ";
  out += PortOverlay::AspectName(PortOverlay::AspectMode());
  out += '\n';
}

// One object as `objs` and `obj` show it: uid, editor id, class, area; an actor's position too when
// `withPosition`. `inactive` when it is not active.
void DescribeObject(const CEntity& ent, bool withPosition, std::string& out) {
  char line[320];
  std::snprintf(line, sizeof(line), "u%04x %08x  %-26s a%02d", ent.GetUniqueId().value,
                ent.GetEditorId().Value(), ClassName(ent).c_str(), ent.GetCurrentAreaId().Value());
  out += line;
  const CActor* actor = withPosition ? TCastToConstPtr< CActor >(&ent) : nullptr;
  if (actor != nullptr) {
    const CVector3f p = actor->GetTranslation();
    std::snprintf(line, sizeof(line), " (%.1f, %.1f, %.1f)", p.GetX(), p.GetY(), p.GetZ());
    out += line;
  }
  if (!ent.GetActive()) {
    out += "  inactive";
  }
  out += '\n';
}

// The object an id names: the raw TUniqueId first, then - because the port's own log lines print
// TUniqueId::Value(), which drops the version bits ("[door] 006e got kSM_Open") - the one object
// whose Value() matches, when exactly one does. `obj` and `send` both take their target from here.
const CEntity* ResolveObject(CStateManager& mgr, std::string_view text) {
  TUniqueId uid = kInvalidUniqueId;
  if (!ParseUid(text, uid)) {
    Fail("not an object id '" + Str(text) + "'");
    return nullptr;
  }
  if (const CEntity* exact = mgr.GetObjectById(uid)) {
    return exact;
  }
  const CObjectList& list = mgr.GetObjectListById(kOL_All);
  const CEntity* found = nullptr;
  int matches = 0;
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CEntity* ent = list[i];
    if (ent != nullptr && ent->GetUniqueId().Value() == uid.Value()) {
      found = ent;
      ++matches;
    }
  }
  if (matches == 1) {
    return found;
  }
  char why[48];
  std::snprintf(why, sizeof(why), matches == 0 ? "no object u%04x" : "ambiguous id u%04x",
                uid.value);
  Fail(why);
  return nullptr;
}

void CmdObjs(CStateManager& mgr, const Args& args, std::string& out) {
  const std::string filter = args.size() < 2 ? std::string() : Lower(args[1]);
  const CObjectList& list = mgr.GetObjectListById(kOL_All);
  int count = 0;
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CEntity* ent = list[i];
    if (ent == nullptr) {
      continue;
    }
    if (!filter.empty() && Lower(ClassName(*ent)).find(filter) == std::string::npos) {
      continue;
    }
    DescribeObject(*ent, true, out);
    ++count;
  }
  char line[64];
  std::snprintf(line, sizeof(line), "%d objects\n", count);
  out += line;
}

}  // namespace
}  // namespace PortConsole

// `obj <door>`: the door and lock state machines, which is what a door that will not open is stuck
// in (a lock that never finishes unlocking, an open request count that never rose).
struct PortConsoleDoor {
  static void Describe(const CScriptDoor& door, CStateManager& mgr, std::string& out) {
    static const char* const kDoor[] = {"closed", "waiting-for-area", "opening",
                                        "open",   "close-delay",      "closing"};
    static const char* const kLock[] = {"unlocked", "pending", "locking", "locked", "unlocking"};
    const CActor* lock = TCastToConstPtr< CActor >(mgr.GetObjectById(door.mLockActorId));
    char line[224];
    std::snprintf(line, sizeof(line),
                  "  door %s, lock %s (timer %.2f, lock actor u%04x%s), open requests %d%s\n",
                  door.mDoorState <= CScriptDoor::kDS_Closing ? kDoor[door.mDoorState] : "?",
                  door.mLockState <= CScriptDoor::kLS_Unlocking ? kLock[door.mLockState] : "?",
                  door.mLockTimer, door.mLockActorId.Value(),
                  lock == nullptr                          ? " missing"
                  : lock->GetModelData() == nullptr        ? ""
                  : lock->GetModelData()->IsAnimating()    ? " animating"
                                                           : " still",
                  door.mOpenRequestCount, door.mResetPending ? ", reset pending" : "");
    out += line;
  }
};

namespace PortConsole {
namespace {

void CmdObj(CStateManager& mgr, const Args& args, std::string& out) {
  if (args.size() < 2) {
    Fail("usage: obj <id>");
    return;
  }
  const CEntity* ent = ResolveObject(mgr, args[1]);
  if (ent == nullptr) {
    return;
  }
  DescribeObject(*ent, false, out);
  const CActor* actor = TCastToConstPtr< CActor >(ent);
  const CHealthInfo* health = actor != nullptr ? actor->GetHealthInfo() : nullptr;
  if (health != nullptr) {
    char line[64];
    std::snprintf(line, sizeof(line), "  hp %.0f/%.0f\n", health->GetHP(),
                  health->GetInitialHP());
    out += line;
  }
  // A patterned's FSM state, which is what `hurt` is usually driven against. Upstream has only the
  // non-const accessor (CPatterned::StateMachineState), and it static_casts to TStateMachineState
  // even when the object holds a CGenericFSM2State (GetType() 1) - a different member layout, so
  // reading TStateMachineState's own mState through that cast reads a vector's guts as a CState*.
  // The base subobject sits at the same address under both, so take it as the base interface and
  // make only virtual calls; the state's own data is touched only on the AFSM type.
  if (const CScriptDoor* door = TCastToConstPtr< CScriptDoor >(ent)) {
    ::PortConsoleDoor::Describe(*door, mgr, out);
  }
  const CPatterned* patterned = TCastToConstPtr< CPatterned >(ent);
  if (patterned != nullptr) {
    TStateMachineStateBase< CPatterned >* machine =
        &const_cast< CPatterned* >(patterned)->StateMachineState();
    const char* name = machine->HasState() ? machine->GetName() : nullptr;
    if (name != nullptr && name[0] != '\0') {
      out += "  fsm ";
      out += name;
      out += '\n';
    } else if (!machine->HasState()) {
      out += "  fsm (none)\n";
    } else if (machine->GetType() == 0) {
      const CState* state =
          static_cast< TStateMachineState< CPatterned >* >(machine)->GetCurrentState();
      char line[32];
      std::snprintf(line, sizeof(line), "  fsm #%d\n", state != nullptr ? state->GetIndex() : -1);
      out += line;
    } else {
      out += "  fsm (unnamed)\n";
    }
  }
  for (const SConnection& conn : ent->GetConnectionList()) {
    const CEntity* target = mgr.GetObjectById(mgr.GetIdForScript(conn.objId));
    char line[256];
    std::snprintf(line, sizeof(line), "  on %s send %s -> %08x %s\n",
                  NameOrHex(kStateNames, kStateCount, static_cast< uint >(conn.state)).c_str(),
                  NameOrHex(kMessageNames, kMessageCount, static_cast< uint >(conn.msg)).c_str(),
                  conn.objId.Value(),
                  target != nullptr ? ClassName(*target).c_str() : "(not loaded)");
    out += line;
  }
}

// `damage <id> <amount> [knockback]`: what a player power-beam shot does, through ApplyDamage, so
// the target's TakeDamage / KnockBack / Death run. The player is the damager and the owner.
void CmdDamage(CStateManager& mgr, const Args& args, std::string& out) {
  float amount = 0.f;
  float knockback = 0.f;
  if (args.size() < 3 || !ParseFloat(args[2], amount)) {
    Fail("usage: damage <id> <amount> [knockback-power] [power|missile|cannonball|dark|light|"
         "annihilator|powerbomb|darkburst|sunburst|sonicboom]");
    return;
  }
  // The weapon mode is what a damageable trigger's vulnerability is looked up by, so a wall that
  // only a missile or the cannon ball may break cannot be tested with the default power beam.
  // The charge combos (darkburst, sunburst, sonicboom) are their beam with the combo flag, which is
  // what a knockback rule set tells them apart by (the Ing's death follow-ups, for instance).
  EWeaponType weapon = kWT_Power;
  bool comboed = false;
  for (size_t i = 3; i < args.size(); ++i) {
    float number = 0.f;
    if (ParseFloat(args[i], number)) {
      knockback = number;
      continue;
    }
    if (args[i] == "missile") {
      weapon = kWT_Missile;
    } else if (args[i] == "cannonball") {
      weapon = kWT_CannonBall;
    } else if (args[i] == "dark") {
      weapon = kWT_Dark;
    } else if (args[i] == "light") {
      weapon = kWT_Light;
    } else if (args[i] == "power") {
      weapon = kWT_Power;
    } else if (args[i] == "annihilator") {
      weapon = kWT_Annihilator;
    } else if (args[i] == "powerbomb") {
      weapon = kWT_PowerBomb;
    } else if (args[i] == "darkburst") {
      weapon = kWT_Dark;
      comboed = true;
    } else if (args[i] == "sunburst") {
      weapon = kWT_Light;
      comboed = true;
    } else if (args[i] == "sonicboom") {
      weapon = kWT_Annihilator;
      comboed = true;
    } else {
      Fail("damage: weapon is power, missile, cannonball, dark, light, annihilator, powerbomb, "
           "darkburst, sunburst or sonicboom");
      return;
    }
  }
  CEntity* ent = const_cast< CEntity* >(ResolveObject(mgr, args[1]));
  CActor* actor = ent != nullptr ? TCastToPtr< CActor >(ent) : nullptr;
  CPlayer* player = mgr.GetPlayer(0);
  if (actor == nullptr || actor->GetHealthInfo() == nullptr || player == nullptr) {
    Fail("not an actor with health (or no player)");
    return;
  }
  const float before = actor->GetHealthInfo()->GetHP();
  const CDamageInfo info(CWeaponMode(weapon, false, comboed), amount, 0.f, knockback);
  const CVector3f direction = actor->GetTranslation() - player->GetTranslation();
  mgr.ApplyDamage(player->GetUniqueId(), actor->GetUniqueId(), player->GetUniqueId(), info,
                  CMaterialFilter::GetPassEverything(), direction);
  // The actor may have been deleted by its own death; look it up again.
  const CActor* after = TCastToConstPtr< CActor >(mgr.GetObjectById(actor->GetUniqueId()));
  char line[96];
  if (after != nullptr && after->GetHealthInfo() != nullptr) {
    std::snprintf(line, sizeof(line), "hp %.1f -> %.1f\n", before, after->GetHealthInfo()->GetHP());
  } else {
    std::snprintf(line, sizeof(line), "hp %.1f -> (gone)\n", before);
  }
  out += line;
}

// frame: the game updates run so far (what MP2_FRAMES counts), so a console script can wait on
// game time rather than wall time (con.py's `@f<n>`), which a fast harness run does not keep.
void CmdFrame(CStateManager&, const Args&, std::string& out) {
  out += "frame " + std::to_string(PortHarness::GameUpdates()) + '\n';
}

// fsm <id> <state>: put a patterned actor's state machine in a named state, through the machine's
// own SetState (the AFSM and FSM2 dispatchers both take a name), so a run can enter a state whose
// trigger it cannot reach. The leave/enter callbacks run as they would in game.
void CmdFsm(CStateManager& mgr, const Args& args, std::string& out) {
  if (args.size() != 3) {
    Fail("usage: fsm <id> <state name>");
    return;
  }
  CEntity* ent = const_cast< CEntity* >(ResolveObject(mgr, args[1]));
  CPatterned* patterned = ent != nullptr ? TCastToPtr< CPatterned >(ent) : nullptr;
  if (patterned == nullptr) {
    Fail("not a patterned actor");
    return;
  }
  TStateMachineStateBase< CPatterned >* machine = &patterned->StateMachineState();
  const std::string before = machine->HasState() && machine->GetName() != nullptr ? machine->GetName() : "?";
  // string_l borrows the characters: keep the std::string alive for every use of `name`.
  const std::string wanted(args[2]);
  const rstl::string name = rstl::string_l(wanted.c_str());
  // An FSM2's SetState looks only at its own top-level states; a state inside one of its
  // sub-machines is entered the way a transition into that sub-machine would enter it.
  std::string where = "top machine";
  if (machine->GetType() == 1) {
    const char* path[8] = {};
    const int depth = static_cast< CGenericFSM2State< CPatterned >* >(machine)->PortSetStateDeep(
        mgr, *patterned, name, path, 8);
    if (depth > 0) {
      where = "sub-machine ";
      for (int i = 0; i < depth; ++i) {
        where += (i != 0 ? "/" : "");
        where += path[i] != nullptr ? path[i] : "?";
      }
    }
  } else {
    machine->SetState(mgr, *patterned, name);
  }
  const char* after = machine->HasState() ? machine->GetName() : nullptr;
  // An unknown name leaves the machine where it was; say so rather than report a no-op. An FSM2
  // evaluates the new state's transitions inside SetState, so a state that was entered and left
  // at once reads as another, new name: report that as entered.
  if (after != nullptr && std::string_view(after) != args[2] && before != after) {
    out += "fsm " + before + " -> " + Str(args[2]) + " [" + where + "] (entered; now " + after +
           ")\n";
    return;
  }
  if (after == nullptr || std::string_view(after) != args[2]) {
    Fail("state '" + Str(args[2]) + "' not entered (unknown name?); now " +
         (after != nullptr ? after : "?"));
    return;
  }
  out += "fsm " + before + " -> " + after + " [" + where + "]\n";
}

// shoot <id> [charged] [dist] [dir x y z]: fire the player's current beam from dist units (default 3)
// away, travelling along dir (default: from the player), straight at an actor's aim position (a joint collision actor's own position), homing on it. The
// shot is a real CEnergyProjectile with the beam's damage, so a module that only counts projectile
// hits (a weak-spot joint's last toucher) sees it, unlike `damage`.
void CmdShoot(CStateManager& mgr, const Args& args, std::string& out) {
  CEntity* ent = args.size() >= 2 ? const_cast< CEntity* >(ResolveObject(mgr, args[1])) : nullptr;
  CActor* actor = ent != nullptr ? TCastToPtr< CActor >(ent) : nullptr;
  CPlayer* player = mgr.GetPlayer(0);
  CGunWeapon* beam = player != nullptr && player->GetPlayerGun() != nullptr
                         ? player->GetPlayerGun()->PortCurrentBeam()
                         : nullptr;
  if (actor == nullptr || beam == nullptr) {
    Fail("usage: shoot <id> [charged] [dist]   (id must be an actor; the player needs a beam)");
    return;
  }
  CPlayerState::EChargeStage stage = CPlayerState::kCS_Normal;
  float dist = 3.f;
  const CVector3f target = actor->GetAimPosition(mgr, 0.f);
  CVector3f dir = target - player->GetTranslation();
  for (size_t i = 2; i < args.size(); ++i) {
    float x = 0.f, y = 0.f, z = 0.f;
    if (args[i] == "charged") {
      stage = CPlayerState::kCS_Charged;
    } else if (args[i] == "dir" && i + 3 < args.size() && ParseFloat(args[i + 1], x) &&
               ParseFloat(args[i + 2], y) && ParseFloat(args[i + 3], z)) {
      dir = CVector3f(x, y, z); // the shot's travel direction, e.g. `dir 0 0 1` comes from below
      i += 3;
    } else if (!ParseFloat(args[i], dist) || dist <= 0.f) {
      Fail("shoot: options are `charged`, a distance > 0 and `dir <x> <y> <z>`");
      return;
    }
  }
  dir = dir.CanBeNormalized() ? dir.AsNormalized() : CVector3f(0.f, 1.f, 0.f);
  const CVector3f start = target - dir * dist;
  TUniqueId shot = kInvalidUniqueId;
  beam->Fire(beam->GetProjectileToken(stage), false, 0.f, stage,
             CTransform4f::LookAt(start, target, CVector3f::Up()), mgr, actor->GetUniqueId(), 0,
             CSfxManager::kInternalInvalidSfxId, &shot, nullptr, 1.f, 1.f);
  char line[128];
  std::snprintf(line, sizeof(line), "shot u%04x at u%04x from %.1f units%s\n", shot.value,
                actor->GetUniqueId().value, dist, stage == CPlayerState::kCS_Charged ? " (charged)" : "");
  out += line;
}

// place <id> <x> <y> <z> | place <id> player [dist]: move an actor (keeping its rotation), to a
// point or `dist` units (default 6) in front of the player, and stop any physics motion.
void CmdPlace(CStateManager& mgr, const Args& args, std::string& out) {
  CEntity* ent = args.size() >= 3 ? const_cast< CEntity* >(ResolveObject(mgr, args[1])) : nullptr;
  CActor* actor = ent != nullptr ? TCastToPtr< CActor >(ent) : nullptr;
  if (actor == nullptr) {
    Fail("usage: place <id> <x> <y> <z> | place <id> player [dist]   (id must be an actor)");
    return;
  }
  CVector3f pos;
  if (args[2] == "player") {
    CPlayer* player = mgr.GetPlayer(0);
    float dist = 6.f;
    if (player == nullptr || (args.size() > 3 && !ParseFloat(args[3], dist))) {
      Fail("no player, or a bad distance");
      return;
    }
    CVector3f dir = player->GetTransform().GetForward();
    dir.SetZ(0.f);
    dir = dir.CanBeNormalized() ? dir.AsNormalized() : CVector3f(0.f, 1.f, 0.f);
    pos = player->GetTranslation() + dir * dist;
  } else {
    float x = 0.f, y = 0.f, z = 0.f;
    if (args.size() != 5 || !ParseFloat(args[2], x) || !ParseFloat(args[3], y) ||
        !ParseFloat(args[4], z)) {
      Fail("usage: place <id> <x> <y> <z> | place <id> player [dist]");
      return;
    }
    pos = CVector3f(x, y, z);
  }
  actor->SetTranslation(pos);
  if (CPhysicsActor* physics = TCastToPtr< CPhysicsActor >(ent)) {
    physics->Stop();
  }
  char line[64];
  std::snprintf(line, sizeof(line), "placed at %.2f %.2f %.2f\n", pos.GetX(), pos.GetY(), pos.GetZ());
  out += line;
}

void CmdSend(CStateManager& mgr, const Args& args, std::string& out) {
  if (args.size() < 3) {
    Fail("usage: send <id> <msg>");
    return;
  }
  const CEntity* target = ResolveObject(mgr, args[1]);
  if (target == nullptr) {
    return;
  }
  const int msg = EnumValue(kMessageNames, kMessageCount, args[2]);
  if (msg < 0) {
    Fail("unknown message '" + Str(args[2]) + "'");
    return;
  }
  const uint editorId = target->GetEditorId().Value();
  const TUniqueId uid = target->GetUniqueId();
  if (msg == kSM_Delete) {
    // A bare Delete message resets the object's FSM but leaves it alive, and its next Think writes
    // past mSubMachines; script deletes go through the state manager's delete request.
    mgr.DeleteObjectRequest(uid);
    out += "delete requested\n";
    return;
  }
  mgr.DeliverScriptMsg(
      CScriptMsg(kInvalidUniqueId, uid, static_cast< EScriptObjectMessage >(msg)));
  char line[160];
  std::snprintf(line, sizeof(line), "sent %s to u%04x %08x\n",
                NameOrHex(kMessageNames, kMessageCount, static_cast< uint >(msg)).c_str(),
                uid.value, editorId);
  out += line;
}

// hurt <id> <amount> [weaponType]: put a CDamageInfo on an object through the same path a shot takes,
// so an enemy's FSM reacts to it (a state change, a flinch, a death). The weapon type is the raw
// EWeaponType the vulnerability tables use; 0 is the power beam. No damager object, so the damage
// takes the target's plain vulnerability and no directional bonus (CStateManager::ApplyDamage reads
// the weapon mode only to reject kWT_None).
void CmdHurt(CStateManager& mgr, const Args& args, std::string& out) {
  if (args.size() < 3) {
    Fail("usage: hurt <id> <amount> [weaponType]");
    return;
  }
  const CEntity* ent = ResolveObject(mgr, args[1]);
  if (ent == nullptr) {
    return;
  }
  const float amount = static_cast< float >(std::atof(Str(args[2]).c_str()));
  if (!(amount > 0.f)) {
    Fail("amount must be > 0");
    return;
  }
  const int type = args.size() > 3 ? ArgToInt(args[3], 0) : 0;
  const TUniqueId uid = ent->GetUniqueId();
  mgr.ApplyDamage(kInvalidUniqueId, uid, kInvalidUniqueId,
                  CDamageInfo(CWeaponMode(static_cast< EWeaponType >(type)), amount, 0.f, 0.f),
                  CMaterialFilter::GetPassEverything(), CVector3f::Zero());
  char line[128];
  std::snprintf(line, sizeof(line), "hurt u%04x %.0f (weapon %d)\n", uid.value, amount, type);
  out += line;
  const CActor* actor = TCastToConstPtr< CActor >(ent);
  const CHealthInfo* health = actor != nullptr ? actor->GetHealthInfo() : nullptr;
  if (health != nullptr) {
    std::snprintf(line, sizeof(line), "  hp %.0f/%.0f\n", health->GetHP(),
                  health->GetInitialHP());
    out += line;
  }
}

void CmdVisor(CStateManager& mgr, const Args& args, std::string& out) {
  CPlayerState* state = mgr.PlayerState(0);
  if (state == nullptr) {
    Fail("no player state");
    return;
  }
  if (args.size() < 2) {
    const CPlayerState::EPlayerVisor current = state->GetCurrentVisor();
    char line[160];
    std::snprintf(line, sizeof(line), "visor %s (%d)  owned:", VisorName(current),
                  static_cast< int >(current));
    out += line;
    for (const CPlayerState::EPlayerVisor visor : kVisorOrder) {
      if (state->HasVisor(visor)) {
        out += ' ';
        out += VisorName(visor);
      }
    }
    out += '\n';
    return;
  }
  CPlayerState::EPlayerVisor visor = CPlayerState::kPV_Invalid;
  if (!ParseVisor(args[1], visor)) {
    Fail("unknown visor '" + Str(args[1]) + "'");
    return;
  }
  if (!state->HasVisor(visor)) {
    Fail("visor '" + Str(args[1]) + "' not owned");
    return;
  }
  state->StartTransitionToVisor(visor);
  // What CPlayer::UpdateVisorState does after a switch: the scan visor holsters the gun.
  CPlayer* player = mgr.GetPlayer(0);
  CPlayerGun* gun = player != nullptr ? player->GetPlayerGun() : nullptr;
  if (gun != nullptr) {
    if (visor == CPlayerState::kPV_Scan) {
      gun->HolsterGun(mgr);
    } else {
      gun->DrawGun(mgr);
    }
  }
  char line[64];
  std::snprintf(line, sizeof(line), "visor -> %s\n", VisorName(visor));
  out += line;
}

void CmdBeam(CStateManager& mgr, const Args& args, std::string& out) {
  CPlayerState* state = mgr.PlayerState(0);
  if (state == nullptr) {
    Fail("no player state");
    return;
  }
  if (args.size() < 2) {
    const CPlayerState::EBeamId current = state->GetCurrentBeam();
    const SBeamChoice* now = BeamChoice(current);
    char line[160];
    std::snprintf(line, sizeof(line), "beam %s (%d)  owned:", now != nullptr ? now->name : "?",
                  static_cast< int >(current));
    out += line;
    for (const SBeamChoice& choice : kBeamOrder) {
      if (state->HasPowerUp(choice.item)) {
        out += ' ';
        out += choice.name;
      }
    }
    out += '\n';
    return;
  }
  const SBeamChoice* choice = ParseBeam(args[1]);
  if (choice == nullptr) {
    Fail("unknown beam '" + Str(args[1]) + "'");
    return;
  }
  if (!state->HasPowerUp(choice->item)) {
    Fail("beam '" + Str(args[1]) + "' not owned");
    return;
  }
  // The game's own path: CPlayerGun::HandleWeaponChange reads the beam from the state.
  state->SetCurrentBeam(choice->beam);
  char line[64];
  std::snprintf(line, sizeof(line), "beam -> %s\n", choice->name);
  out += line;
}

void CmdTp(CStateManager& mgr, const Args& args, std::string& out) {
  float x = 0.f;
  float y = 0.f;
  float z = 0.f;
  if (args.size() < 4 || !ParseFloat(args[1], x) || !ParseFloat(args[2], y) ||
      !ParseFloat(args[3], z)) {
    Fail("usage: tp <x> <y> <z>");
    return;
  }
  CPlayer* player = mgr.GetPlayer(0);
  if (player == nullptr) {
    Fail("no player");
    return;
  }
  // Keep the facing, flattened onto the floor; a player looking straight up or down has no flat
  // heading, so it faces +y.
  const CVector3f pos(x, y, z);
  CVector3f dir = player->GetTransform().GetForward();
  dir.SetZ(0.f);
  if (dir.CanBeNormalized()) {
    dir.Normalize();
  } else {
    dir = CVector3f(0.f, 1.f, 0.f);
  }
  player->Teleport(CTransform4f::LookAt(pos, pos + dir, CVector3f::Up()), mgr, true);
  char line[64];
  std::snprintf(line, sizeof(line), "position %.2f %.2f %.2f\n", x, y, z);
  out += line;
}

// face <yaw deg>: turn the player where it stands, through the same Teleport the game uses for a
// respawn (which sets the player's look/move/control directions from the transform).
void CmdFace(CStateManager& mgr, const Args& args, std::string& out) {
  float yaw = 0.f;
  if (args.size() < 2 || !ParseFloat(args[1], yaw)) {
    Fail("usage: face <yaw deg>");
    return;
  }
  CPlayer* player = mgr.GetPlayer(0);
  if (player == nullptr) {
    Fail("no player");
    return;
  }
  const float radians = yaw * 3.14159265358979f / 180.f;
  // 0 = +y, 90 = +x (z is up), which is the order of the world's own axes.
  const CVector3f dir(std::sin(radians), std::cos(radians), 0.f);
  const CVector3f pos = player->GetTranslation();
  player->Teleport(CTransform4f::LookAt(pos, pos + dir, CVector3f::Up()), mgr, true);
  char line[64];
  std::snprintf(line, sizeof(line), "facing %.0f deg (%.2f, %.2f)\n", yaw, dir.GetX(), dir.GetY());
  out += line;
}

// aim <yaw_deg> <pitch_deg>: point the view where the run cannot turn it. Yaw reads as `face`
// does (0 = +y, 90 = +x) and a negative pitch looks down, which is how a level shot reaches a
// puddle on the floor. It turns mouse aim on for the rest of the run, since that is the source
// CFirstPersonCamera takes its pitch from; `aim` again re-places the view.
void CmdAim(CStateManager&, const Args& args, std::string& out) {
  float yaw = 0.f;
  float pitch = 0.f;
  if (args.size() != 3 || !ParseFloat(args[1], yaw) || !ParseFloat(args[2], pitch)) {
    Fail("usage: aim <yaw_deg> <pitch_deg>   (0 = +y, 90 = +x; negative pitch looks down)");
    return;
  }
  PortMouse::SetAimAngles(yaw, pitch);
  char line[96];
  std::snprintf(line, sizeof(line), "aim yaw %.0f pitch %.0f (yaw as `face`, pitch +up)\n",
                static_cast< double >(yaw), static_cast< double >(pitch));
  out += line;
}

// give adds capacity and fills it; take removes capacity, and the amount with it (which is how a
// suit upgrade is lost). This is CScriptPickup's own path (CScriptPickup.cpp:342-349).
void CmdGive(CStateManager& mgr, const Args& args, bool take, std::string& out) {
  if (args.size() < 2) {
    Fail(take ? "usage: take <item> [n]" : "usage: give <item> [n]");
    return;
  }
  CPlayerState* state = mgr.PlayerState(0);
  if (state == nullptr) {
    Fail("no player state");
    return;
  }
  const int item = EnumValue(kItemNames, kItemCount, args[1]);
  if (item < 0 || item >= CPlayerState::kIT_Max) {
    Fail("unknown item '" + Str(args[1]) + "'");
    return;
  }
  int amount = 1;
  if (args.size() > 2) {
    amount = ArgToInt(args[2], -1);
    if (amount < 1) {
      Fail("the amount is a positive number");
      return;
    }
  }
  const CPlayerState::EItemType type = static_cast< CPlayerState::EItemType >(item);
  if (take) {
    state->AddPowerUp(type, -amount);
  } else {
    state->AddPowerUp(type, amount);
    state->IncrPickUp(type, amount);
  }
  // A translator gate reads the inventory once, when its area loads; this has it read again.
  CScriptConditionalRelay::PortNoteInventoryCheat();
  const char* name = EnumName(kItemNames, kItemCount, static_cast< uint >(item));
  char line[128];
  std::snprintf(line, sizeof(line), "%s %s: %d/%d\n", take ? "took" : "gave",
                name != nullptr ? name : args[1].data(), state->GetItemAmount(type),
                state->GetItemCapacity(type));
  out += line;
}

void CmdGiveAdd(CStateManager& mgr, const Args& args, std::string& out) {
  CmdGive(mgr, args, false, out);
}

void CmdGiveTake(CStateManager& mgr, const Args& args, std::string& out) {
  CmdGive(mgr, args, true, out);
}

// press <a+b+...> [updates]: hold pad buttons on the virtual pad (the one MP2_INPUT writes), for
// `updates` game updates (default 8); 0 holds them until the next press or stick.
void CmdPress(CStateManager& mgr, const Args& args, std::string& out) {
  (void)mgr;
  if (args.size() < 2) {
    Fail("usage: press <a+b+...> [updates]   (0 holds until the next press/stick)");
    return;
  }
  u16 mask = 0;
  const std::string wanted = Str(args[1]);
  size_t begin = 0;
  while (begin <= wanted.size()) {
    const size_t plus = wanted.find('+', begin);
    const std::string name = Lower(wanted.substr(begin, plus == std::string::npos ? std::string::npos
                                                                                 : plus - begin));
    const PortInput::SButtonName* button = nullptr;
    for (const PortInput::SButtonName& candidate : PortInput::kButtonNames) {
      if (name == Lower(candidate.name)) {
        button = &candidate;
      }
    }
    if (button == nullptr) {
      Fail("unknown button '" + name + "'");
      return;
    }
    mask |= button->mask;
    if (plus == std::string::npos) {
      break;
    }
    begin = plus + 1;
  }
  const unsigned updates = args.size() > 2 ? static_cast< unsigned >(ArgToInt(args[2], 8)) : 8u;
  PortInput::Hold(mask, 0, 0, 0, 0, updates);
  char line[128];
  std::snprintf(line, sizeof(line), "holding %s for %u updates\n", wanted.c_str(), updates);
  out += line;
}

// grapple <id> [dist]: the lock-on a grapple needs, set up in one go. A lock-on press takes
// CPlayer's next orbit target, which comes from candidate lists rebuilt only every 20 updates, so a
// press right after a teleport finds no target and a held L never presses again; and a player put
// in the air falls out of the orbit zone in ~15 updates. This wakes the point (kSM_Activate, which
// also restores kMT_Orbit), puts the player dist units away with the eye level with the point,
// facing it (along the point's axis when it is axis-constrained), has the lists rebuilt on the next
// update and holds L from the update after that (`press` or `stick` ends the hold).
void CmdGrapple(CStateManager& mgr, const Args& args, std::string& out) {
  const CEntity* ent = args.size() >= 2 ? ResolveObject(mgr, args[1]) : nullptr;
  const CScriptGrapplePoint* point =
      ent != nullptr ? TCastToConstPtr< CScriptGrapplePoint >(ent) : nullptr;
  float dist = 8.f;
  if (point == nullptr || (args.size() > 2 && (!ParseFloat(args[2], dist) || !(dist > 2.f)))) {
    Fail("usage: grapple <grapple point id> [dist > 2]   (`objs grapple` lists them)");
    return;
  }
  CPlayer* player = mgr.GetPlayer(0);
  CPlayerState* state = mgr.PlayerState(0);
  if (player == nullptr || state == nullptr) {
    Fail("no player");
    return;
  }
  const bool woken = !point->GetActive();
  if (woken) {
    mgr.DeliverScriptMsg(CScriptMsg(kInvalidUniqueId, point->GetUniqueId(), kSM_Activate));
  }
  const CVector3f target = point->GetTranslation();
  CVector3f dir = player->GetTranslation() - target;
  if (point->GetGrappleParameters().GetConstrainToAxis()) {
    const CVector3f axis = point->GetTransform().GetForward();
    dir = CVector3f::Dot(axis, dir) < 0.f ? -axis : axis;
  }
  dir.SetZ(0.f);
  dir = dir.CanBeNormalized() ? dir.AsNormalized() : CVector3f(0.f, -1.f, 0.f);
  const float eyeHeight = player->GetEyePosition().GetZ() - player->GetTranslation().GetZ();
  CVector3f pos = target + dir * dist;
  pos.SetZ(target.GetZ() - eyeHeight);
  player->Teleport(CTransform4f::LookAt(pos, pos - dir, CVector3f::Up()), mgr, true);
  player->PortRefreshOrbitCandidates();
  PortInput::Hold(PAD_TRIGGER_L, 0, 0, 0, 0, 0, 2);
  char line[192];
  std::snprintf(line, sizeof(line),
                "grapple u%04x at (%.1f, %.1f, %.1f)%s: player at (%.1f, %.1f, %.1f), L held from "
                "2 updates on%s\n",
                point->GetUniqueId().value, target.GetX(), target.GetY(), target.GetZ(),
                woken ? " (woken)" : "", pos.GetX(), pos.GetY(), pos.GetZ(),
                state->HasPowerUp(CPlayerState::kIT_GrappleBeam) ? ""
                                                                 : "; no grapple beam (`give grappleBeam 1`)");
  out += line;
}

// stick <x> <y> [updates]: hold the main stick, -127..127 (0 holds until the next press/stick).
void CmdStick(CStateManager& mgr, const Args& args, std::string& out) {
  (void)mgr;
  if (args.size() < 3) {
    Fail("usage: stick <x> <y> [updates]   (-127..127; 0 updates holds until the next press/stick)");
    return;
  }
  const int x = ArgToInt(args[1], 1000);
  const int y = ArgToInt(args[2], 1000);
  if (x < -127 || x > 127 || y < -127 || y > 127) {
    Fail("the stick is -127..127 on each axis");
    return;
  }
  const unsigned updates = args.size() > 3 ? static_cast< unsigned >(ArgToInt(args[3], 8)) : 8u;
  PortInput::Hold(0, static_cast< s8 >(x), static_cast< s8 >(y), 0, 0, updates);
  char line[96];
  std::snprintf(line, sizeof(line), "holding stick %d,%d for %u updates\n", x, y, updates);
  out += line;
}

// cstick <x> <y> [updates]: hold the C-stick, -127..127 (0 holds until the next press/stick).
// Spring Ball reads C-stick up, so this is the one way to trigger it without a pad.
void CmdCstick(CStateManager& mgr, const Args& args, std::string& out) {
  (void)mgr;
  if (args.size() < 3) {
    Fail("usage: cstick <x> <y> [updates]   (-127..127; 0 updates holds until the next press/stick)");
    return;
  }
  const int x = ArgToInt(args[1], 1000);
  const int y = ArgToInt(args[2], 1000);
  if (x < -127 || x > 127 || y < -127 || y > 127) {
    Fail("the C-stick is -127..127 on each axis");
    return;
  }
  const unsigned updates = args.size() > 3 ? static_cast< unsigned >(ArgToInt(args[3], 8)) : 8u;
  PortInput::Hold(0, 0, 0, static_cast< s8 >(x), static_cast< s8 >(y), updates);
  char line[96];
  std::snprintf(line, sizeof(line), "holding C-stick %d,%d for %u updates\n", x, y, updates);
  out += line;
}

// The rest of the line from `first`, joined with spaces: a card path can hold one (`USA/Card A`).
std::string RestOf(const Args& args, size_t first) {
  std::string text;
  for (size_t i = first; i < args.size(); ++i) {
    if (i > first) {
      text += ' ';
    }
    text += Str(args[i]);
  }
  return text;
}

// save info [<file>] | export [<path>] | import <path|latest> | backup | backups: the card's save
// as the file it is (platform/port_save.cpp). `info` reads a .gci without mounting the card, so a
// backup can be checked while the game runs; with no file it reads the card's own.
void CmdSave(CStateManager&, const Args& args, std::string& out) {
  const std::string sub = args.size() < 2 ? std::string("info") : Lower(args[1]);
  // `save <file>` is `save info <file>`: an argument that names no subcommand is a path to read,
  // which is what the help line says (`save [info] [<file>]`).
  const bool known =
      sub == "export" || sub == "import" || sub == "backup" || sub == "backups";
  if (!known) {
    const size_t first = sub == "info" ? 2 : 1;
    std::string path;
    if (args.size() > first) {
      path = RestOf(args, first);
    } else {
      const std::vector< std::string > files = PortSave::Files();
      if (files.empty()) {
        Fail("the card has no save file");
        return;
      }
      path = files.front();
    }
    PortSave::File file;
    std::string error;
    if (!PortSave::Inspect(path, file, error)) {
      Fail(error);
      return;
    }
    out += PortSave::IsImage() ? "card image: " + PortSave::ImagePath()
                               : "card: " + PortSave::Directory();
    out += '\n';
    out += "file: " + file.path + " (" + file.note + "; the header points at slot " +
           std::to_string(file.saveIdx) + ")\n";
    char line[224];
    for (int i = 0; i < 3; ++i) {
      const PortSave::Slot& slot = file.slots[i];
      if (!slot.present) {
        std::snprintf(line, sizeof(line), "slot %d: empty\n", i);
        out += line;
        continue;
      }
      const int seconds = static_cast< int >(slot.playTime);
      std::snprintf(line, sizeof(line),
                    "slot %d: world %08x, %.0f%% items, %.0f%% scans, %02d:%02d:%02d, %.0f energy, "
                    "%u tanks%s\n",
                    i, slot.worldId, static_cast< double >(slot.itemPercent),
                    static_cast< double >(slot.scanPercent), seconds / 3600, (seconds / 60) % 60,
                    seconds % 60, static_cast< double >(slot.health), slot.energyTanks,
                    slot.hardMode ? ", hard" : "");
      out += line;
    }
    return;
  }
  if (sub == "export") {
    std::string message;
    if (!PortSave::Export(args.size() >= 3 ? RestOf(args, 2) : std::string(), message)) {
      Fail(message);
      return;
    }
    out += message;
    out += '\n';
    return;
  }
  if (sub == "import") {
    if (args.size() < 3) {
      Fail("usage: save import <path|latest>");
      return;
    }
    std::string message;
    if (!PortSave::Import(RestOf(args, 2), message)) {
      Fail(message);
      return;
    }
    out += message;
    out += '\n';
    return;
  }
  if (sub == "backup") {
    std::string written;
    std::string error;
    if (!PortSave::Backup(&written, error)) {
      Fail(error);
      return;
    }
    out += "card backup: " + written + "\n";
    return;
  }
  if (sub == "backups") {
    const std::vector< std::string > backups = PortSave::Backups();
    if (backups.empty()) {
      out += "no card backups\n";
      const std::string dir = PortSave::BackupsDirectory();
      out += dir.empty() ? "(no user directory: nothing can be kept)\n" : dir + "\n";
      return;
    }
    for (const std::string& path : backups) {
      out += path;
      out += '\n';
    }
    return;
  }
  Fail("save <info|export|import|backup|backups> ...");
}

// screenshot: the F12 capture. Aurora writes it under the working directory and logs the exact path.
void CmdScreenshot(CStateManager&, const Args&, std::string& out) {
  aurora::request_screenshot();
  out +=
      "screenshot requested: the log's `Screenshot saved to screenshots/metroid-prime-<stamp>.bmp`\n"
      "names the file (relative to the run's working directory)\n";
}

void CmdItems(CStateManager& mgr, const Args&, std::string& out) {
  const CPlayerState* state = mgr.GetPlayerState(0);
  if (state == nullptr) {
    Fail("no player state");
    return;
  }
  char line[96];
  for (int i = 0; i < CPlayerState::kIT_Max; ++i) {
    const CPlayerState::EItemType type = static_cast< CPlayerState::EItemType >(i);
    const int capacity = state->GetItemCapacity(type);
    if (capacity <= 0) {
      continue;
    }
    const char* name = EnumName(kItemNames, kItemCount, static_cast< uint >(i));
    const std::string label = name != nullptr ? name : std::to_string(i);
    std::snprintf(line, sizeof(line), "%3d %-26s %d/%d\n", i, label.c_str(),
                  state->GetItemAmount(type), capacity);
    out += line;
  }
}

struct SCommand {
  std::string_view name;
  void (*run)(CStateManager&, const Args&, std::string&);
  bool needsGame; // false: runs on the frame the game is not updating? (all of these need the world)
};

constexpr SCommand kCommands[] = {
    {"help", CmdHelp, true},
    {"status", CmdStatus, true},
    {"areas", CmdAreas, true},
    {"warp", CmdWarp, true},
    {"nextarea", CmdNextArea, true},
    {"prevarea", CmdPrevArea, true},
    {"door", CmdDoor, true},
    {"dock", CmdDock, true},
    {"station", CmdStation, true},
    {"heal", CmdHeal, true},
    {"kill", CmdKill, true},
    {"damage", CmdDamage, true},
    {"layer", CmdLayer, true},
    {"god", CmdGod, true},
    {"freecam", CmdFreeCam, true},
    {"photo", CmdPhoto, true},
    {"skipcine", CmdSkipCine, true},
    {"fastmorph", CmdFastMorph, true},
    {"boost", CmdBoost, true},
    {"springball", CmdSpringBall, true},
    {"lockon", CmdLockOn, true},
    {"sticky", CmdSticky, true},
    {"pointeraim", CmdPointerAim, true},
    {"cardbackup", CmdCardBackup, true},
    {"logfile", CmdLogFile, true},
    {"livesplit", CmdLiveSplit, true},
    {"settings", CmdSettings, true},
    {"gunoffset", CmdGunOffset, true},
    {"msaa", CmdMsaaAniso, true},
    {"aniso", CmdMsaaAniso, true},
    {"hudscale", CmdHudScale, true},
    {"crosshair", CmdCrosshair, true},
    {"uiscale", CmdUiScale, true},
    {"vsync", CmdVsync, true},
    {"interp", CmdInterp, true},
    {"aspect", CmdAspect, true},
    {"objs", CmdObjs, true},
    {"obj", CmdObj, true},
    {"send", CmdSend, true},
    {"hurt", CmdHurt, true},
    {"fsm", CmdFsm, true},
    {"frame", CmdFrame, true},
    {"place", CmdPlace, true},
    {"shoot", CmdShoot, true},
    {"visor", CmdVisor, true},
    {"beam", CmdBeam, true},
    {"tp", CmdTp, true},
    {"items", CmdItems, true},
    {"world", CmdWorld, true},
    {"worlds", CmdWorlds, true},
    {"strg", CmdStrg, true},
    {"face", CmdFace, true},
    {"aim", CmdAim, true},
    {"give", CmdGiveAdd, true},
    {"take", CmdGiveTake, true},
    {"press", CmdPress, true},
    {"grapple", CmdGrapple, true},
    {"stick", CmdStick, true},
    {"cstick", CmdCstick, true},
    {"save", CmdSave, true},
    {"screenshot", CmdScreenshot, true},
};

void Execute(CStateManager& mgr, const std::string& line, std::string& out) {
  const Args args = Split(line);
  if (args.empty()) {
    out += "=> err: empty command (try `help`)\n";
    return;
  }
  for (const SCommand& command : kCommands) {
    if (command.name.size() != args[0].size() ||
        std::memcmp(command.name.data(), args[0].data(), args[0].size()) != 0) {
      continue;
    }
    std::string body;
    sFailure.clear();
    command.run(mgr, args, body);
    out += body;
    if (sFailure.empty()) {
      out += "=> ok\n";
    } else {
      out += "=> err: " + sFailure + "\n";
    }
    return;
  }
  out += "=> err: unknown command '" + Str(args[0]) + "' (try `help`)\n";
}

} // namespace

void Start() {
  if (sStarted) {
    return;
  }
  sStarted = true;
  const char* value = std::getenv("MP2_CONSOLE");
  if (value == nullptr || value[0] == '\0' || std::strcmp(value, "0") == 0) {
    return;
  }
#ifdef _WIN32
  std::fprintf(stderr, "[console] MP2_CONSOLE is not supported on Windows\n");
  return;
#else
  int port = kDefaultPort;
  if (std::strcmp(value, "1") != 0) {
    const int parsed = std::atoi(value);
    if (parsed < 2 || parsed > 65535) {
      std::fprintf(stderr, "[console] MP2_CONSOLE=%s is not a port (2-65535)\n", value);
      return;
    }
    port = parsed;
  }
  const int listener = socket(AF_INET, SOCK_STREAM, 0);
  if (listener < 0) {
    std::fprintf(stderr, "[console] no socket: %s\n", std::strerror(errno));
    return;
  }
  int yes = 1;
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(static_cast< uint16_t >(port));
  if (bind(listener, reinterpret_cast< sockaddr* >(&addr), sizeof(addr)) != 0 ||
      listen(listener, 1) != 0) {
    std::fprintf(stderr, "[console] cannot listen on 127.0.0.1:%d: %s\n", port,
                 std::strerror(errno));
    close(listener);
    return;
  }
  sEnabled = true;
  std::fprintf(stderr, "[console] listening on 127.0.0.1:%d (try `help`)\n", port);
  std::thread(ListenThread, listener).detach();
#endif
}

bool Enabled() { return sEnabled; }

void Tick(CStateManager& mgr) {
  // Save states share this game-thread hook (CStateManager::PortWarpTick) and run without the console.
  PortSaveState::Tick(mgr);
#ifndef _WIN32
  if (!sEnabled) {
    return;
  }
  for (;;) {
    SQueued queued;
    {
      std::lock_guard< std::mutex > lock(sQueueMutex);
      if (sQueue.empty()) {
        return;
      }
      queued = std::move(sQueue.front());
      sQueue.pop_front();
    }
    // `a; b; c` runs every command in this one tick, so a forced arm can land inside a one-frame
    // window (separate lines can arrive a frame apart).
    size_t start = 0;
    for (;;) {
      const size_t sep = queued.line.find(';', start);
      std::string out;
      Execute(mgr, queued.line.substr(start, sep == std::string::npos ? std::string::npos : sep - start), out);
      SendRaw(queued.generation, out);
      if (sep == std::string::npos) {
        break;
      }
      start = sep + 1;
    }
  }
#else
  (void)mgr;
#endif
}

} // namespace PortConsole
