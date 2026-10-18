// Debug command console, modelled on MetroidPrimePort's `platform/port_console.cpp` - same protocol
// (`MP2_CONSOLE=<port>`, one command per line, replies ending in `=> ok`/`=> err: ...`), the same
// one-client-at-a-time TCP listener, and the same rule that everything which reads the game runs on
// the game thread. See platform/include/port_console.h.
//
// What is here is the set of commands this tree can serve today through its existing port APIs
// (PortWarp, PortOverlay) plus the world/player state a state manager gives for free. Adding one is a
// row in `kCommands`.

#include "port_console.h"

#include "port_input.h"
#include "port_overlay.h"
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
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerGun.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/CActor.hpp"
#include "MetroidPrime/CEntity.hpp"
#include "MetroidPrime/CEntityInfo.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CObjectList.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include <aurora/gfx.h>

#include <atomic>
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
      "layer <n>                   activate a layer of the current area\n"
      "god <0|1>                   god mode (the F1 cheat)\n"
      "skipcine <0|1>              skip cutscenes the player has not seen (the F1 setting)\n"
      "gunoffset [<r> <f> <u>]     arm cannon offset in metres (+-1), as the F1 Render row does\n"
      "msaa <1|4> | aniso <1..16>  anti-aliasing and anisotropic filtering (applied next frame)\n"
      "hudscale <50..100>          HUD scale in percent, as the F1 HUD row does\n"
      "objs [filter]               objects of the world: uid, editor id, class, area, position\n"
      "obj <id>                    one object (an id as `objs` prints it: u046e): health, messages\n"
      "send <id> <msg>             deliver a script message to an object (name or number)\n"
      "visor [name|0-3]            show or switch the visor (combat, echo, scan, dark)\n"
      "beam [name|0-3]             show or switch the beam (power, dark, light, annihilator)\n"
      "tp <x> <y> <z>              teleport the player, keeping its facing\n"
      "face <yaw deg>              turn the player where it stands (0 = +y, 90 = +x)\n"
      "items                       power-up amounts and capacities\n"
      "world <world> [area]        load another world (the MP1 console's `warp <world>`)\n"
      "worlds                      the memory card's world ids, areas and name tables\n"
      "strg <id|name> [index]      a string table as the game loads it\n"
      "give <item> [n] | take      add an item (capacity and amount), or remove capacity\n"
      "press <a+b+...> [updates]   hold pad buttons (0 updates: until the next press/stick)\n"
      "stick <x> <y> [updates]     hold the main stick, -127..127\n"
      "screenshot                  the F12 capture; the log names the file it wrote\n"
      "help                        this list\n";
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
  char line[160];
  for (int i = 0; i < world->IGetAreaCount(); ++i) {
    const CGameArea* area = world->Area(TAreaId(i));
    const IGameArea* any = world->IGetAreaAlways(TAreaId(i));
    std::snprintf(line, sizeof(line), "%2d  %08x  %s\n", i,
                  any != nullptr ? any->IGetAreaAssetId() : 0u,
                  area != nullptr ? "loaded" : "");
    out += line;
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
void CmdKill(CStateManager&, const Args&, std::string&) { PortWarp::Kill(); }

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

void CmdSkipCine(CStateManager&, const Args& args, std::string& out) {
  const bool enabled = args.size() < 2 ? !PortOverlay::SkipCinematics() : ArgToInt(args[1], 1) != 0;
  PortOverlay::SetSkipCinematics(enabled);
  out += enabled ? "skip cutscenes on\n" : "skip cutscenes off\n";
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
  mgr.DeliverScriptMsg(
      CScriptMsg(kInvalidUniqueId, uid, static_cast< EScriptObjectMessage >(msg)));
  char line[160];
  std::snprintf(line, sizeof(line), "sent %s to u%04x %08x\n",
                NameOrHex(kMessageNames, kMessageCount, static_cast< uint >(msg)).c_str(),
                uid.value, editorId);
  out += line;
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
  PortInput::Hold(mask, 0, 0, updates);
  char line[128];
  std::snprintf(line, sizeof(line), "holding %s for %u updates\n", wanted.c_str(), updates);
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
  PortInput::Hold(0, static_cast< s8 >(x), static_cast< s8 >(y), updates);
  char line[96];
  std::snprintf(line, sizeof(line), "holding stick %d,%d for %u updates\n", x, y, updates);
  out += line;
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
    {"layer", CmdLayer, true},
    {"god", CmdGod, true},
    {"skipcine", CmdSkipCine, true},
    {"gunoffset", CmdGunOffset, true},
    {"msaa", CmdMsaaAniso, true},
    {"aniso", CmdMsaaAniso, true},
    {"hudscale", CmdHudScale, true},
    {"objs", CmdObjs, true},
    {"obj", CmdObj, true},
    {"send", CmdSend, true},
    {"visor", CmdVisor, true},
    {"beam", CmdBeam, true},
    {"tp", CmdTp, true},
    {"items", CmdItems, true},
    {"world", CmdWorld, true},
    {"worlds", CmdWorlds, true},
    {"strg", CmdStrg, true},
    {"face", CmdFace, true},
    {"give", CmdGiveAdd, true},
    {"take", CmdGiveTake, true},
    {"press", CmdPress, true},
    {"stick", CmdStick, true},
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
    std::string out;
    Execute(mgr, queued.line, out);
    SendRaw(queued.generation, out);
  }
#else
  (void)mgr;
#endif
}

} // namespace PortConsole
