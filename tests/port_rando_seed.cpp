// Randovania Echoes patch-data importer: parsing, Multiworld removal and the mapping of a
// pickup to the port's location key. Pure; no game or file access.

#include "port_rando_locations.h"
#include "port_rando_seed.h"

#include <cstdio>
#include <string>

namespace {

int g_failures = 0;

void Check(bool condition, const char* what) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    g_failures++;
  }
}

const char* kFixture = R"JSON({
  "unknown_top": {"a": [1, 2.5e3, null, true, false, -7, {"b": []}], "c": "x"},
  "banner_name": "Test Seed",
  "pickups": [
    {"pickup_index": 25, "resources": [{"index": 44, "amount": 5}, {"index": 74, "amount": 26}],
     "hud_text": ["Missile Expansion acquired! café", "second"], "scan": "A missile.",
     "model": {"game": "prime2", "name": "MissileExpansion"}, "extra": 1},
    {"pickup_index": 60, "resources": [{"index": 12, "amount": 1}, {"index": 74, "amount": 61}],
     "conditional_resources": [{"item": 12, "resources": [{"index": 13, "amount": 1}]}],
     "convert": [{"from_item": 28, "to_item": 107, "clear_source": true, "overwrite_target": false}],
     "model_index": 4, "sound_index": 5, "jingle_index": 6},
    {"pickup_index": 3, "resources": [{"index": 74, "amount": 4}]},
    {"pickup_index": 23, "resources": [{"index": 42, "amount": 1}, {"index": 74, "amount": 24}]},
    {"pickup_index": 999, "resources": []}
  ],
  "spawn_point": {"location": {"world_asset_id": 3145160548, "area_asset_id": 1115401626},
    "amount": [{"index": 42, "amount": 1}], "capacity": [{"index": 42, "amount": 1}, {"index": 44, "amount": 0}]},
  "elevators": [{"instance_id": 589860, "origin_location": {"world_asset_id": 1, "area_asset_id": 2},
    "target_location": {"world_asset_id": 4294967295, "area_asset_id": 4}, "room_name": "Temple"}],
  "translator_gates": [{"gate_index": 3, "translator_index": 2}]
})JSON";

} // namespace

int main() {
  using namespace PortRandoSeed;
  Seed seed;
  std::string error;
  Check(IsRandovania(kFixture), "fixture sniffs as Randovania");
  Check(!IsRandovania("{\"seed\": \"x\", \"locations\": {}}"), "native seed is not Randovania");
  Check(ParseRandovania(kFixture, seed, error), "fixture parses");
  if (g_failures != 0) {
    std::fprintf(stderr, "error: %s\n", error.c_str());
    return 1;
  }
  Check(seed.name == "Test Seed", "banner_name is the seed name");
  Check(seed.pickups.size() == 5, "five pickups");

  const Pickup& missile = seed.pickups[0];
  Check(missile.pickupIndex == 25, "pickup 25");
  Check(missile.resources.size() == 1 && missile.resources[0].item == 44 &&
            missile.resources[0].amount == 5,
        "Multiworld dropped, missile kept");
  Check(missile.hudText.size() == 2 && missile.hudText[0] == "Missile Expansion acquired! caf\xc3\xa9",
        "\\u escape decoded to UTF-8");
  Check(missile.scan == "A missile.", "scan");
  Check(missile.modelGame == "prime2" && missile.modelName == "MissileExpansion", "model");
  Check(missile.modelIndex == -1, "no model_index");

  const Pickup& prog = seed.pickups[1];
  Check(prog.resources.size() == 1 && prog.resources[0].item == 12, "progressive base resource");
  Check(prog.conditionalResources.size() == 1 && prog.conditionalResources[0].item == 12 &&
            prog.conditionalResources[0].resources.size() == 1 &&
            prog.conditionalResources[0].resources[0].item == 13,
        "conditional resources");
  Check(prog.conversions.size() == 1 && prog.conversions[0].fromItem == 28 &&
            prog.conversions[0].toItem == 107 && prog.conversions[0].clearSource &&
            !prog.conversions[0].overwriteTarget,
        "convert entry");
  Check(prog.modelIndex == 4 && prog.soundIndex == 5 && prog.jingleIndex == 6, "model indices");

  Check(seed.pickups[2].resources.empty(), "Multiworld-only pickup has no resources");
  Check(seed.pickups[3].pickupIndex == 23 && seed.pickups[3].resources.size() == 1, "custom row");

  Check(seed.spawn.present && seed.spawn.worldAssetId == 3145160548u &&
            seed.spawn.areaAssetId == 1115401626u,
        "spawn location (above INT32_MAX)");
  Check(seed.spawn.amount.size() == 1 && seed.spawn.capacity.size() == 2 &&
            seed.spawn.capacity[1].item == 44,
        "spawn lists");
  Check(seed.elevators.size() == 1 && seed.elevators[0].instanceId == 589860u &&
            seed.elevators[0].targetWorldAssetId == 0xFFFFFFFFu && seed.elevators[0].roomName == "Temple",
        "elevator");
  Check(seed.translatorGates.size() == 1 && seed.translatorGates[0].gateIndex == 3 &&
            seed.translatorGates[0].translatorIndex == 2,
        "translator gate");

  // The importer's mapping (LoadRandovania in port_randomizer.cpp) looks pickups up like this.
  const PortRandoLocations::Location* row = PortRandoLocations::ByPickupIndex(missile.pickupIndex);
  Check(row != nullptr && row->worldAssetId == 0x42B935E4u && row->areaAssetId == 0x427FAD9Au &&
            row->entityId == 0x00020180u,
        "pickup 25 maps to the Mining Plaza key");
  const PortRandoLocations::Location* custom = PortRandoLocations::ByPickupIndex(23);
  Check(custom != nullptr && custom->entityId == 0, "pickup 23 is a custom location");
  Check(PortRandoLocations::ByPickupIndex(seed.pickups[4].pickupIndex) == nullptr,
        "pickup 999 is unknown");

  Seed bad;
  Check(!ParseRandovania("{\"pickups\": [", bad, error) && !error.empty(), "truncated JSON fails");
  Check(!ParseRandovania("{\"pickups\": [] ,}", bad, error), "trailing comma fails");
  Check(!ParseRandovania("{\"nope\": 1}", bad, error), "missing pickups fails");
  Check(!ParseRandovania("{\"pickups\": [{\"resources\": []}]}", bad, error),
        "pickup without index fails");
  Check(ParseRandovania("{\"pickups\": []}", bad, error) && bad.name == "Randovania",
        "default banner name");

  if (g_failures != 0)
    return 1;
  std::printf("port_rando_seed: OK\n");
  return 0;
}
