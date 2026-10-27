#ifndef METROID_PRIME_PORT_PORT_RANDO_LOCATIONS_H
#define METROID_PRIME_PORT_PORT_RANDO_LOCATIONS_H
#include <cstdint>

// Randovania's Echoes pickup locations, in its pickup_index order, mapped to the keys the
// randomizer hook sees (PortRandomizer::ApplyPickup: world MLVL, area MREA, Pickup editor id).
// The table is generated and disc-checked by tools/rando_locations.py.
namespace PortRandoLocations {

struct Location {
  int pickupIndex;
  uint32_t worldAssetId;
  uint32_t areaAssetId;
  // The Pickup's editor id; 0 for a 'custom' location, which has no Pickup on disc (Randovania
  // spawns one, e.g. for an item a script grants) and so never reaches the hook.
  uint32_t entityId;
  const char* region;
  const char* area;
};

// Number of locations; pickup indices are 0..Count()-1.
int Count();
// The location with this pickup index, or nullptr when out of range.
const Location* ByPickupIndex(int pickupIndex);
// The pickup index of a hooked Pickup, or -1 when Randovania does not list it.
int PickupIndexFor(uint32_t worldAssetId, uint32_t areaAssetId, uint32_t entityId);

} // namespace PortRandoLocations

#endif
