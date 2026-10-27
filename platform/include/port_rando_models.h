#ifndef METROID_PRIME_PORT_PORT_RANDO_MODELS_H
#define METROID_PRIME_PORT_PORT_RANDO_MODELS_H
#include <cstdint>

// Donor models for randomized pickups: for each item the model (CMDL) and animation set a
// vanilla Pickup of that item carries, and the worlds whose pak holds them. Echoes has no item ->
// model table in code, so the table is read from the disc by tools/rando_models.py (which
// documents the donor rule). Pure data: no game dependencies, so a test can call it.
namespace PortRandoModels {

constexpr uint32_t kInvalidAsset = 0xFFFFFFFFu;

struct Donor {
  int item;                // EItemType value
  uint32_t model;          // Pickup model property (CMDL)
  uint32_t anim[3];        // animation set: ANCS (kInvalidAsset: none), character index, initial animation
  uint32_t worlds[6];      // MLVL ids whose pak holds the model (and ANCS); 0-terminated
  const char* name;        // item name, for logs
};

// Number of rows.
int Count();
const Donor* ByIndex(int index);
// True when `world` is listed in the donor's resident worlds.
bool ResidentIn(const Donor& donor, uint32_t worldAssetId);
// The donor for `item` that is resident in `worldAssetId`, else the item's first donor, else
// nullptr (no Pickup on disc gives this item).
const Donor* ForItem(int item, uint32_t worldAssetId);

} // namespace PortRandoModels

#endif
