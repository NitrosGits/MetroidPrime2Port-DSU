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

void Area(const char* area) {
  sRequest = SRequest();
  sRequest.kind = EKind::kArea;
  CopyText(sRequest.area, sizeof sRequest.area, area);
}

void World(const char* world, const char* area) {
  sRequest = SRequest();
  sRequest.kind = EKind::kWorld;
  CopyText(sRequest.world, sizeof sRequest.world, world);
  CopyText(sRequest.area, sizeof sRequest.area, area);
}

void CycleArea(int delta) {
  sRequest = SRequest();
  sRequest.kind = EKind::kCycleArea;
  sRequest.delta = delta;
}

void CycleWorld(int delta) {
  sRequest = SRequest();
  sRequest.kind = EKind::kCycleWorld;
  sRequest.delta = delta;
}

void Dock() {
  sRequest = SRequest();
  sRequest.kind = EKind::kDock;
}

void Door() {
  sRequest = SRequest();
  sRequest.kind = EKind::kDoor;
}

void Station() {
  sRequest = SRequest();
  sRequest.kind = EKind::kStation;
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
    World(worldText, areaText);
    std::fprintf(stderr, "[warp] boot request: world '%s' area '%s'\n", worldText, areaText);
  }
  if (const char* area = std::getenv("MP2_TELEPORT")) {
    Area(area);
    std::fprintf(stderr, "[warp] boot request: area '%s'\n", area);
  }
}

bool Consume(SRequest& out) {
  if (sRequest.kind == EKind::kNone) {
    return false;
  }
  out = sRequest;
  sRequest = SRequest();
  return true;
}

void Clear() { sRequest = SRequest(); }

} // namespace PortWarp
