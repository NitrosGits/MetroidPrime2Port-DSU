// Debug command console, modelled on MetroidPrimePort's `platform/port_console.cpp` - same protocol
// (`MP2_CONSOLE=<port>`, one command per line, replies ending in `=> ok`/`=> err: ...`), the same
// one-client-at-a-time TCP listener, and the same rule that everything which reads the game runs on
// the game thread. See platform/include/port_console.h.
//
// What is here is the set of commands this tree can serve today through its existing port APIs
// (PortWarp, PortOverlay) plus the world/player state a state manager gives for free. Adding one is a
// row in `kCommands`.

#include "port_console.h"

#include "port_overlay.h"
#include "port_warp.h"

#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerGun.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/CActor.hpp"
#include "MetroidPrime/CEntity.hpp"
#include "MetroidPrime/CEntityInfo.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CObjectList.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include <atomic>
#include <chrono>
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
      "objs [filter]               objects of the world: uid, editor id, class, area, position\n"
      "obj <id>                    one object (an id as `objs` prints it: u046e): health, messages\n"
      "send <id> <msg>             deliver a script message to an object (name or number)\n"
      "visor [name|0-3]            show or switch the visor (combat, echo, scan, dark)\n"
      "beam [name|0-3]             show or switch the beam (power, dark, light, annihilator)\n"
      "tp <x> <y> <z>              teleport the player, keeping its facing\n"
      "items                       power-up amounts and capacities\n"
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
    {"objs", CmdObjs, true},
    {"obj", CmdObj, true},
    {"send", CmdSend, true},
    {"visor", CmdVisor, true},
    {"beam", CmdBeam, true},
    {"tp", CmdTp, true},
    {"items", CmdItems, true},
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
