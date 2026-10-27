// The Randovania location table: the shape the seed importer relies on. Its contents are
// checked against the disc by tools/rando_locations.py; this checks what survives compilation.

#include "port_rando_locations.h"

#include <cstdio>
#include <set>
#include <tuple>

namespace {

int g_failures = 0;

void Check(bool condition, const char* what, int index) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s (pickup index %d)\n", what, index);
    g_failures++;
  }
}

} // namespace

int main() {
  using namespace PortRandoLocations;
  Check(Count() == 119, "Echoes has 119 Randovania locations", Count());
  std::set< std::tuple< uint32_t, uint32_t, uint32_t > > keys;
  int custom = 0;
  for (int i = 0; i < Count(); ++i) {
    const Location* location = ByPickupIndex(i);
    Check(location != nullptr && location->pickupIndex == i, "row order is pickup_index", i);
    if (location == nullptr)
      continue;
    if (location->entityId == 0) {
      custom++;
      continue;
    }
    Check(keys.insert({location->worldAssetId, location->areaAssetId, location->entityId}).second,
          "location key is unique", i);
    Check(PickupIndexFor(location->worldAssetId, location->areaAssetId, location->entityId) == i,
          "key looks up its own pickup index", i);
  }
  Check(custom == 4, "four custom (energy controller) locations", custom);
  Check(ByPickupIndex(-1) == nullptr && ByPickupIndex(Count()) == nullptr, "out of range", -1);
  // Mining Plaza energy tank: Agon Wastes MLVL, Mining Plaza MREA, Pickup 0x00020180.
  Check(PickupIndexFor(0x42B935E4u, 0x427FAD9Au, 0x00020180u) == 25, "known row", 25);
  Check(PickupIndexFor(0x42B935E4u, 0x427FAD9Au, 0x00020181u) == -1, "unknown entity", -1);
  Check(PickupIndexFor(0x42B935E4u, 0x02FC3717u, 0) == -1, "custom rows never match", 46);
  if (g_failures != 0)
    return 1;
  std::printf("port_rando_locations: %d locations OK\n", Count());
  return 0;
}
