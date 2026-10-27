#ifndef METROID_PRIME_PORT_PORT_RANDO_WORLD_H
#define METROID_PRIME_PORT_PORT_RANDO_WORLD_H
#include <cstdint>

// The randomizer's world graph and location list: the single-player worlds' areas, the MLVL
// dock connections between them, the WorldTeleporters, and the pickups a seed may change. Read
// from the disc by tools/rando_world.py (which documents what counts as a location). Pure data:
// no game dependencies, so a test can call it. Area indices index the flat Areas() list.
namespace PortRandoWorld {

struct World {
  uint32_t mlvl;
  const char* name;
};

struct Area {
  int world;        // index into Worlds()
  uint32_t mrea;
  bool dark;        // the area's name ends in `_dark` (Dark Aether)
  const char* name; // MLVL internal area name
};

struct Dock {
  int area, dock;     // this side
  int toArea, toDock; // the other side
  bool isVirtual;     // the area's DOCK object of this number is virtual (no door)
};

struct Teleporter {
  int area;
  int layer;
  uint32_t editorId;
  uint32_t toMlvl, toMrea;
};

struct Location {
  int area;
  int layer; // script layer, -1 for the MREA's generated-object section
  uint32_t editorId;
  int item;  // vanilla EItemType
  int amount;
  int capacity;
  const char* itemName;
};

int WorldCount();
int AreaCount();
int DockCount();
int TeleporterCount();
int LocationCount();
const World* WorldAt(int index);
const Area* AreaAt(int index);
const Dock* DockAt(int index);
const Teleporter* TeleporterAt(int index);
const Location* LocationAt(int index);

// Index of the area with this MLVL/MREA pair, else -1.
int FindArea(uint32_t mlvl, uint32_t mrea);
// Index of the location keyed WORLD:AREA:ENTITY (as PortRandomizer::FormatLocationKey), else -1.
int FindLocation(uint32_t mlvl, uint32_t mrea, uint32_t editorId);

} // namespace PortRandoWorld

#endif
