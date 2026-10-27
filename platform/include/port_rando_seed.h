#ifndef METROID_PRIME_PORT_PORT_RANDO_SEED_H
#define METROID_PRIME_PORT_PORT_RANDO_SEED_H
#include <cstdint>
#include <string>
#include <vector>

// Randovania's Metroid Prime 2: Echoes patcher JSON ("patch data"), parsed into plain data.
// Pure: no game, file or SDL dependencies, so a test can call it. Item ids are EItemType values.
namespace PortRandoSeed {

// Randovania's multiworld counter item; dropped on import.
constexpr int kMultiworldItem = 74;

struct ResourceGrant {
  int item = 0;
  int amount = 0;
};

struct ConditionalResource {
  int item = 0; // granted `resources` instead when the player already has this item
  std::vector<ResourceGrant> resources;
};

struct Conversion {
  int fromItem = 0;
  int toItem = 0;
  bool clearSource = false;
  bool overwriteTarget = false;
};

struct Pickup {
  int pickupIndex = 0;
  std::vector<ResourceGrant> resources; // Multiworld removed
  std::vector<ConditionalResource> conditionalResources;
  std::vector<Conversion> conversions;
  std::vector<std::string> hudText;
  std::string scan;
  std::string modelGame;
  std::string modelName;
  // Set instead of model when Randovania's adjust_model_name already ran; -1 when absent.
  int modelIndex = -1;
  int soundIndex = -1;
  int jingleIndex = -1;
};

struct SpawnPoint {
  bool present = false;
  uint32_t worldAssetId = 0;
  uint32_t areaAssetId = 0;
  std::vector<ResourceGrant> amount;
  std::vector<ResourceGrant> capacity;
};

struct Elevator {
  uint32_t instanceId = 0;
  uint32_t originWorldAssetId = 0;
  uint32_t originAreaAssetId = 0;
  uint32_t targetWorldAssetId = 0;
  uint32_t targetAreaAssetId = 0;
  std::string roomName;
};

struct TranslatorGate {
  int gateIndex = 0;
  int translatorIndex = 0;
};

struct Seed {
  std::string name;
  std::vector<Pickup> pickups;
  SpawnPoint spawn;
  std::vector<Elevator> elevators;
  std::vector<TranslatorGate> translatorGates;
};

// True when `json` is a valid JSON object with a top-level "pickups" key (cheap format sniff).
bool IsRandovania(const std::string& json);
// Parses Randovania patch data. Unknown keys are skipped. On failure returns false with the
// reason (and byte offset where known) in `error`; `out` is then unspecified.
bool ParseRandovania(const std::string& json, Seed& out, std::string& error);

} // namespace PortRandoSeed

#endif
