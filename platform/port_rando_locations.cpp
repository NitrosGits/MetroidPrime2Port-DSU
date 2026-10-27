#include "port_rando_locations.h"

namespace PortRandoLocations {
namespace {

constexpr Location kLocations[] = {
#include "port_rando_locations.inc"
};
constexpr int kCount = static_cast< int >(sizeof(kLocations) / sizeof(kLocations[0]));

} // namespace

int Count() { return kCount; }

const Location* ByPickupIndex(int pickupIndex) {
  if (pickupIndex < 0 || pickupIndex >= kCount)
    return nullptr;
  return &kLocations[pickupIndex];
}

int PickupIndexFor(uint32_t worldAssetId, uint32_t areaAssetId, uint32_t entityId) {
  if (entityId == 0)
    return -1;
  for (const Location& location : kLocations) {
    if (location.entityId == entityId && location.areaAssetId == areaAssetId &&
        location.worldAssetId == worldAssetId)
      return location.pickupIndex;
  }
  return -1;
}

} // namespace PortRandoLocations
