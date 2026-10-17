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
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/CHealthInfo.hpp"

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
#include <vector>

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
    out += "usage: warp <slot|id|name>\n";
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

void CmdLayer(CStateManager&, const Args& args, std::string& out) {
  if (args.size() < 2) {
    out += "usage: layer <n>\n";
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
    command.run(mgr, args, body);
    out += body;
    out += "=> ok\n";
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
