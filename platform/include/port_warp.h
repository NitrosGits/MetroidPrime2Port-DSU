#pragma once

// Warp requests: jump straight to an area - or another world - instead of walking there. The
// platform layer only queues them (`MP2_WORLD` / `MP2_TELEPORT`, the F5-F8 hotkeys, and the
// `MP2_INPUT` tokens `NEXTAREA`/`PREVAREA`/`NEXTWORLD`/`PREVWORLD`); the game consumes one per
// update in `CStateManager::PortWarpTick`, because only the game can read the resource factory,
// the world and the memory card's world table.
//
// The text fields take an asset name or a hex asset id, so nothing here needs a lookup table:
// the game resolves names where the resource names actually live.

namespace PortWarp {

enum class EKind {
  kNone,
  kArea,      // an area of the current world
  kWorld,     // reload into another world, optionally into one of its areas
  kCycleArea, // the next/previous existing area of the current world
  kCycleWorld, // the next/previous world of the game's world table
  kDock,      // stand in the nearest doorway of this area, facing through it
  kDoor,      // stand in front of the nearest door, facing it, to test opening it
  kStation    // stand on the nearest save station, to test saving in game
};

struct SRequest {
  EKind kind = EKind::kNone;
  char world[80] = {};
  char area[80] = {};
  int delta = 1;
};

void Area(const char* area);
void World(const char* world, const char* area);
void CycleArea(int delta);
void CycleWorld(int delta);
void Dock();
void Door();
void Station();
// Reads MP2_WORLD / MP2_TELEPORT once; called from the platform event loop.
void ApplyEnvOnce();
// Takes the pending request, if any. One at a time on purpose: a warp reloads the world, which
// invalidates anything resolved against the old one.
bool Consume(SRequest& out);
void Clear();

} // namespace PortWarp
