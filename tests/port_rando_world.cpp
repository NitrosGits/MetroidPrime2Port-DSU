// The randomizer's world table: the shape logic relies on. Its contents are read from the disc
// by tools/rando_world.py; this checks what survives compilation.

#include "port_rando_world.h"

#include <cstdio>
#include <cstring>

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

  // Dock locks, one per dock edge; totals measured from the disc by tools/rando_world.py.
  int beams[6] = {}, shields[5] = {}, translators[5] = {}, scriptLocks[3] = {}, layered = 0;
  for (int i = 0; i < DockCount(); ++i) {
    const DockLock* l = DockLockAt(i);
    Check(l != nullptr && l->area == DockAt(i)->area && l->dock == DockAt(i)->dock, "dock lock row matches its dock", i);
    beams[int(l->beam)]++;
    shields[int(l->shield)]++;
    translators[int(l->translator)]++;
    scriptLocks[int(l->scriptLock)]++;
    layered += l->translatorLayered;
    Check(l->shield == DockShield::None || l->beam == DockBeam::Any, "shield doors are Any", i);
  }
  Check(DockLockAt(DockCount()) == nullptr, "dock lock out of range is null", DockCount());
  Check(beams[int(DockBeam::NoDoor)] == 110 && beams[int(DockBeam::Any)] == 434, "110 doorless, 434 Any", 0);
  Check(beams[int(DockBeam::Light)] == 60 && beams[int(DockBeam::Dark)] == 48, "60 Light, 48 Dark", 0);
  Check(beams[int(DockBeam::Annihilator)] == 14 && beams[int(DockBeam::Morph)] == 12, "14 Annihilator, 12 Morph", 0);
  Check(shields[int(DockShield::Missile)] == 35 && shields[int(DockShield::SuperMissile)] == 24, "35 Missile, 24 Super", 0);
  Check(shields[int(DockShield::Seeker)] == 14 && shields[int(DockShield::PowerBomb)] == 24, "14 Seeker, 24 PB", 0);
  Check(translators[int(DockTranslator::Violet)] == 7 && translators[int(DockTranslator::Amber)] == 4 &&
            translators[int(DockTranslator::Emerald)] == 3 && translators[int(DockTranslator::Cobalt)] == 2,
        "16 translator gates", 0);
  Check(layered == 2, "2 translator checks on default-off layers", layered);
  Check(scriptLocks[int(DockScriptLock::Temporary)] == 54 && scriptLocks[int(DockScriptLock::Permanent)] == 3,
        "54 temporary, 3 permanent script locks", 0);

  auto lockOf = [](uint32_t mrea, int dock) -> const DockLock* {
    for (int i = 0; i < DockCount(); ++i)
      if (DockLockAt(i)->dock == dock && AreaAt(DockLockAt(i)->area)->mrea == mrea)
        return DockLockAt(i);
    return nullptr;
  };
  const DockLock* l = lockOf(0xA8391EADu, 1); // Temple Grounds 0o_temple_hall dock 1
  Check(l && l->shield == DockShield::Missile, "0o_temple_hall missile shield", 0);
  l = lockOf(0xB1140C24u, 0); // Temple Grounds 0a_temple_hall dock 0
  Check(l && l->translator == DockTranslator::Violet && !l->translatorLayered, "0a_temple_hall Violet gate", 0);
  l = lockOf(0x914F1381u, 0); // Sanctuary 04_cliff_contraption dock 0: isLocked at load
  Check(l && l->scriptLock == DockScriptLock::Permanent && l->scriptLockDefaultLayer, "04_cliff_contraption lock", 0);

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

  // Obstacles: totals by kind, then two known rows.
  Check(ObstacleCount() == 708, "708 obstacles", ObstacleCount());
  int perKind[9] = {};
  for (int i = 0; i < ObstacleCount(); ++i) {
    const Obstacle* o = ObstacleAt(i);
    Check(o->area >= 0 && o->area < AreaCount(), "obstacle area in range", i);
    Check(o->needs < (1u << kObstacleNeedBits), "obstacle needs known bits", i);
    perKind[static_cast<int>(o->kind)]++;
  }
  const int kKinds[9] = {122, 15, 32, 23, 46, 197, 29, 60, 184};
  for (int k = 0; k < 9; ++k)
    Check(perKind[k] == kKinds[k], "obstacles of kind", k);
  auto obstacleOf = [](const char* area, uint32_t id) -> const Obstacle* {
    for (int i = 0; i < ObstacleCount(); ++i)
      if (ObstacleAt(i)->editorId == id && std::strcmp(AreaAt(ObstacleAt(i)->area)->name, area) == 0)
        return ObstacleAt(i);
    return nullptr;
  };
  // Agon 0i_sand_hall_dark: the PLAT is hurt by ball weapons and screw, its "Kill webbing" DTRG by
  // light (incl. Sunburst) and screw; the DTRG's set is the requirement.
  const Obstacle* o = obstacleOf("0i_sand_hall_dark", 0x003700E2u);
  Check(o && o->kind == ObstacleKind::TriggeredBarrier && o->needs == (kNeedLightBeam | kNeedScrewAttack) &&
            o->layerDefault && o->nearestDock == 1,
        "Dark Webbing needs Light or Screw", 0);
  o = obstacleOf("0r_temple_hall", 0x00160097u);
  Check(o && o->kind == ObstacleKind::GrapplePoint && o->needs == kNeedGrappleBeam && o->layerDefault,
        "Temple Grounds 0r_temple_hall grapple point", 0);
  Check(ObstacleNeedItem(kNeedScrewAttack) == 27 && ObstacleNeedItem(kNeedMissile) == 44 &&
            ObstacleNeedItem(kNeedLightBeam | kNeedScrewAttack) == -1,
        "need bits map to items", 0);

  if (g_failures == 0)
    std::puts("port_rando_world_tests: ok");
  return g_failures == 0 ? 0 : 1;
}
