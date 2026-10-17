// The warp request slot. See `include/port_warp.h` for what the kinds mean and who fills it in.

#include "port_warp.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

PortWarp::SRequest sRequest;
bool sReadEnv = false;

void CopyText(char* out, const size_t size, const char* text) {
  if (text == nullptr) {
    out[0] = '\0';
    return;
  }
  size_t i = 0;
  for (; text[i] != '\0' && i + 1 < size; ++i) {
    out[i] = text[i];
  }
  out[i] = '\0';
}

// "something,else" -> first and second halves.
void Split(const char* text, char* first, const size_t firstSize, char* second,
           const size_t secondSize) {
  first[0] = '\0';
  second[0] = '\0';
  if (text == nullptr || *text == '\0') {
    return;
  }
  const char* comma = std::strchr(text, ',');
  if (comma == nullptr) {
    CopyText(first, firstSize, text);
    return;
  }
  const size_t length = static_cast< size_t >(comma - text);
  size_t i = 0;
  for (; i < length && i + 1 < firstSize; ++i) {
    first[i] = text[i];
  }
  first[i] = '\0';
  CopyText(second, secondSize, comma + 1);
}

} // namespace

namespace PortWarp {

namespace {
// A boot request (MP2_WORLD / MP2_TELEPORT) has to survive the `MP2_INPUT` tokens that follow it:
// they share one slot, and a sink that advances by *loop iteration* can fire its first gameplay
// token (a `HEAL`, an `A`) before the game's first Update has consumed the boot request - which then
// vanishes and the run quietly tests the save's own area instead of the one it asked for (measured:
// a run booted `MP2_WORLD="3bfa3eff,5b92d146"` and played area 0, with no line saying why). So while
// a boot request is pending, a token goes to one deferred slot instead and takes the queue once the
// boot request has been consumed. With no boot request pending the old rule stands: latest wins.
SRequest sDeferred;
bool sBootPending = false;

void Queue(const SRequest& request) {
  if (sBootPending && sRequest.kind != EKind::kNone) {
    if (sDeferred.kind == EKind::kNone) {
      std::fprintf(stderr,
                   "[warp] a token arrived while the boot request was pending; serving it after\n");
    }
    sDeferred = request;
    return;
  }
  sRequest = request;
}
} // namespace

void Area(const char* area) {
  SRequest request;
  request.kind = EKind::kArea;
  CopyText(request.area, sizeof request.area, area);
  Queue(request);
}

void World(const char* world, const char* area) {
  SRequest request;
  request.kind = EKind::kWorld;
  CopyText(request.world, sizeof request.world, world);
  CopyText(request.area, sizeof request.area, area);
  Queue(request);
}

void CycleArea(int delta) {
  SRequest request;
  request.kind = EKind::kCycleArea;
  request.delta = delta;
  Queue(request);
}

void CycleWorld(int delta) {
  SRequest request;
  request.kind = EKind::kCycleWorld;
  request.delta = delta;
  Queue(request);
}

void Dock() {
  SRequest request;
  request.kind = EKind::kDock;
  Queue(request);
}

void Door() {
  SRequest request;
  request.kind = EKind::kDoor;
  Queue(request);
}

void Station() {
  SRequest request;
  request.kind = EKind::kStation;
  Queue(request);
}

void WorldPortal() {
  SRequest request;
  request.kind = EKind::kWorldPortal;
  Queue(request);
}

void Kill() {
  SRequest request;
  request.kind = EKind::kKill;
  Queue(request);
}

void Heal() {
  SRequest request;
  request.kind = EKind::kHeal;
  Queue(request);
}
void Layer(int layer) {
  SRequest request;
  request.kind = EKind::kLayer;
  request.delta = layer;
  Queue(request);
}

void ApplyEnvOnce() {
  if (sReadEnv) {
    return;
  }
  sReadEnv = true;
  // MP2_WORLD="<world name or hex>[,<area name or hex>]" jumps worlds at start-up;
  // MP2_TELEPORT="<area name or hex>" jumps within the world the save is in. The one that was
  // set most recently in this function wins, and both are consumed on the first game update,
  // so a normal launch never sees them.
  if (const char* world = std::getenv("MP2_WORLD")) {
    char worldText[80];
    char areaText[80];
    Split(world, worldText, sizeof worldText, areaText, sizeof areaText);
    sBootPending = true;
    World(worldText, areaText);
    std::fprintf(stderr, "[warp] boot request: world '%s' area '%s'\n", worldText, areaText);
  }
  if (const char* area = std::getenv("MP2_TELEPORT")) {
    // MP2_WORLD and MP2_TELEPORT share the slot, and the teleport is applied second, so it wins -
    // which is the documented way to combine them (see platform/README.md).
    sBootPending = true;
    Area(area);
    std::fprintf(stderr, "[warp] boot request: area '%s'\n", area);
  }
}

bool Pending() { return sRequest.kind != EKind::kNone; }

bool PendingIsBoot() { return sBootPending && sRequest.kind != EKind::kNone; }

bool Consume(SRequest& out) {
  if (sRequest.kind == EKind::kNone) {
    return false;
  }
  out = sRequest;
  sRequest = SRequest();
  if (sBootPending) {
    // The boot request has been served (or dropped, which is the game's business, not ours): a token
    // that was held back while it was pending takes the slot now.
    sBootPending = false;
    if (sDeferred.kind != EKind::kNone) {
      sRequest = sDeferred;
      sDeferred = SRequest();
    }
  }
  return true;
}

void Clear() { sRequest = SRequest(); }

} // namespace PortWarp
