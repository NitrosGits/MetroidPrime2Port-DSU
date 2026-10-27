// The donor-model table: the shape the pickup model swap relies on. Its contents are read from
// the disc by tools/rando_models.py; this checks what survives compilation.

#include "port_rando_models.h"

#include <cstdio>

namespace {

int g_failures = 0;

void Check(bool condition, const char* what, int detail) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s (%d)\n", what, detail);
    g_failures++;
  }
}

} // namespace

int main() {
  using namespace PortRandoModels;
  Check(Count() == 44, "44 donor rows", Count());
  int previous = -1;
  for (int i = 0; i < Count(); ++i) {
    const Donor* donor = ByIndex(i);
    Check(donor != nullptr, "row exists", i);
    if (donor == nullptr)
      continue;
    Check(donor->item >= previous, "rows are ordered by item", i);
    previous = donor->item;
    Check(donor->model != kInvalidAsset, "no invalid model", i);
    Check(ForItem(donor->item, 0) != nullptr, "every row's item has a donor", i);
  }
  Check(ByIndex(-1) == nullptr && ByIndex(Count()) == nullptr, "out-of-range row is null", 0);

  // Space Jump Boots has one row, resident in the Agon pak (0x42B935E4) only.
  const Donor* jump = ForItem(24, 0x42B935E4u);
  Check(jump != nullptr && jump->model == 0xD47FE863u, "space jump boots donor", 24);
  Check(jump != nullptr && ResidentIn(*jump, 0x42B935E4u) && !ResidentIn(*jump, 0x3BFA3EFFu),
        "residency is per world", 24);
  // Another world gets the same (non-resident) donor rather than nothing.
  Check(ForItem(24, 0x3BFA3EFFu) == jump, "falls back to the item's first donor", 24);
  // A widely shared model is resident everywhere it should be.
  const Donor* missile = ForItem(44, 0x3BFA3EFFu);
  Check(missile != nullptr && ResidentIn(*missile, 0x3BFA3EFFu), "missile resident in temple grounds", 44);
  // Items no Pickup on disc gives have no donor.
  Check(ForItem(0, 0x3BFA3EFFu) == nullptr, "power beam has no donor", 0);
  Check(ForItem(-1, 0) == nullptr && ForItem(9999, 0) == nullptr, "unknown item is null", 9999);
  Check(ForItem(107, 0x3BFA3EFFu) == ForItem(28, 0x3BFA3EFFu) && ForItem(107, 0) != nullptr,
        "nothing (ETM 107) shows as the ETM location item", 107);
  return g_failures == 0 ? 0 : 1;
}
