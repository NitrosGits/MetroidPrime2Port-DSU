// The randomizer's world table: the shape logic relies on. Its contents are read from the disc
// by tools/rando_world.py; this checks what survives compilation.

#include "port_rando_world.h"

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
  using namespace PortRandoWorld;
  Check(WorldCount() == 5, "5 single-player worlds", WorldCount());
  Check(AreaCount() == 284, "284 areas", AreaCount());
  Check(DockCount() == 678, "678 dock edges", DockCount());
  Check(TeleporterCount() == 53, "53 teleporters", TeleporterCount());
  Check(LocationCount() == 114, "114 locations", LocationCount());
  Check(WorldAt(0) != nullptr && WorldAt(0)->mlvl == 0x3BFA3EFFu, "Temple Grounds first", 0);
  Check(WorldAt(WorldCount()) == nullptr, "out of range is null", WorldCount());

  for (int i = 0; i < AreaCount(); ++i) {
    const Area* area = AreaAt(i);
    Check(area->world >= 0 && area->world < WorldCount(), "area world in range", i);
    Check(FindArea(WorldAt(area->world)->mlvl, area->mrea) == i, "area MREA unique in its world", i);
  }

  // Every dock edge has its reverse, within one world.
  for (int i = 0; i < DockCount(); ++i) {
    const Dock* d = DockAt(i);
    Check(d->area >= 0 && d->area < AreaCount() && d->toArea >= 0 && d->toArea < AreaCount(),
          "dock areas in range", i);
    Check(AreaAt(d->area)->world == AreaAt(d->toArea)->world, "dock stays in its world", i);
    bool reverse = false;
    for (int j = 0; j < DockCount() && !reverse; ++j) {
      const Dock* e = DockAt(j);
      reverse = e->area == d->toArea && e->dock == d->toDock && e->toArea == d->area && e->toDock == d->dock;
    }
    Check(reverse, "dock edge has its reverse", i);
  }

  for (int i = 0; i < TeleporterCount(); ++i) {
    const Teleporter* t = TeleporterAt(i);
    Check(t->area >= 0 && t->area < AreaCount(), "teleporter area in range", i);
  }

  // Locations are unique by WORLD:AREA:ENTITY; each major and each key appears exactly once.
  int perItem[110] = {};
  for (int i = 0; i < LocationCount(); ++i) {
    const Location* l = LocationAt(i);
    const Area* area = AreaAt(l->area);
    Check(FindLocation(WorldAt(area->world)->mlvl, area->mrea, l->editorId) == i, "location key unique", i);
    Check(l->item >= 0 && l->item < 110, "item in range", l->item);
    if (l->item >= 0 && l->item < 110)
      perItem[l->item]++;
  }
  for (int key = 29; key <= 40; ++key)
    Check(perItem[key] == 1, "one location per key", key);
  for (int key = 101; key <= 106; ++key)
    Check(perItem[key] == 1, "one location per sky temple key", key);
  Check(perItem[42] == 14, "14 energy tanks", perItem[42]);
  Check(perItem[44] == 49, "49 missile locations", perItem[44]);
  Check(perItem[18] == 1, "bombs from the Bomb Guardian's generated drop", perItem[18]);
  Check(FindLocation(0x3BFA3EFFu, 0x75318D5Eu, 0x001600CBu) >= 0, "Windchamber Gateway tank", 0);

  if (g_failures == 0)
    std::puts("port_rando_world_tests: ok");
  return g_failures == 0 ? 0 : 1;
}
