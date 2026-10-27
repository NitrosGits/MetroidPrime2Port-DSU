// The randomizer's reachability evaluator. The rule edges are injected (there may be no world rule
// files yet); the dock locks and teleporters come from the disc data in PortRandoWorld.

#include "port_rando_logic.h"
#include "port_rando_world.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>

namespace {

int g_failures = 0;

void Check(bool condition, const char* what, int detail = 0) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s (%d)\n", what, detail);
    g_failures++;
  }
}

using namespace PortRandoLogic;

Inventory Items(std::initializer_list<int> items) {
  Inventory inv;
  for (int i : items)
    inv.Add(i);
  return inv;
}

bool Contains(const std::vector<Node>& nodes, const Node& n) {
  return std::find(nodes.begin(), nodes.end(), n) != nodes.end();
}

Node DockNode(int area, int dock) { return {area, NodeKind::Dock, static_cast<uint32_t>(dock)}; }

void TestRequirements() {
  const Requirement spaceJump = Requirement::Item(24);
  const Requirement bombs = And(Requirement::Item(15), Requirement::Item(18));
  const Requirement either = Or(spaceJump, bombs);
  Check(Requirement::Any().Satisfied(Inventory{}), "any holds with nothing");
  Check(!Requirement::Never().Satisfied(Items({24, 15, 18})), "never never holds");
  Check(!either.Satisfied(Inventory{}), "or: empty fails");
  Check(either.Satisfied(Items({24})), "or: first branch");
  Check(!either.Satisfied(Items({15})), "or: half of the second branch fails");
  Check(either.Satisfied(Items({15, 18})), "or: second branch");
  Inventory counts;
  const Requirement three = Requirement::Item(kItemMissile, 3);
  counts.Set(kItemMissile, 2);
  Check(!three.Satisfied(counts), "count: 2 < 3");
  counts.Set(kItemMissile, 3);
  Check(three.Satisfied(counts), "count: 3 >= 3");
  // And distributes over Or and merges a repeated item to the larger count.
  const Requirement mixed = And(either, Requirement::Item(kItemMissile, 2));
  Check(mixed.terms.size() == 2, "and distributes over or", int(mixed.terms.size()));
  counts = Items({24});
  counts.Set(kItemMissile, 1);
  Check(!mixed.Satisfied(counts), "and: one missile is short");
  counts.Set(kItemMissile, 2);
  Check(mixed.Satisfied(counts), "and: both halves");
  Check(And(Requirement::Any(), Requirement::Never()).terms.empty(), "any & never = never");
  Check(Inventory{}.Count(-1) == 0 && Inventory{}.Count(kMaxItems) == 0, "out-of-range item counts 0");
}

void TestRuleEdges() {
  // Landing Site (area 0): docks 0 and 1, and any location of the world for the item edge.
  const int site = 0;
  const PortRandoWorld::Location* loc = PortRandoWorld::LocationAt(0);
  const int locArea = loc->area;
  Check(PortRandoWorld::AreaCount() > 0 && PortRandoWorld::LocationCount() > 0, "world data present");

  // One-way: dock0 > dock1 only.
  RuleSet oneWay;
  oneWay.push_back({DockNode(site, 0), DockNode(site, 1), Requirement::Any()});
  Graph g(oneWay);
  const Node n0 = DockNode(site, 0), n1 = DockNode(site, 1);
  Check(Contains(g.Reachable(Inventory{}, n0), n1), "one-way edge forward");
  // Backwards: the dock edges lead out of the room, but nothing leads back to dock 0 from dock 1
  // without rules in the neighbouring rooms.
  Check(!Contains(g.Reachable(Inventory{}, n1), n0), "one-way edge not backwards");

  // Two-way is two edges.
  RuleSet twoWay = oneWay;
  twoWay.push_back({DockNode(site, 1), DockNode(site, 0), Requirement::Any()});
  Check(Contains(Graph(twoWay).Reachable(Inventory{}, n1), n0), "two-way edge backwards");

  // An item-gated edge to a location, and an in-room named node.
  const Node locNode{locArea, NodeKind::Location, loc->editorId};
  const Node ledge{locArea, NodeKind::Named, 0};
  RuleSet gated;
  const int someDock = [&] {
    for (int i = 0; i < PortRandoWorld::DockCount(); ++i)
      if (PortRandoWorld::DockAt(i)->area == locArea)
        return PortRandoWorld::DockAt(i)->dock;
    return 0;
  }();
  gated.push_back({DockNode(locArea, someDock), ledge, Requirement::Item(kItemMorphBall)});
  gated.push_back({ledge, locNode, Requirement::Item(24)});
  const int locIndex = 0;
  Check(ReachableLocations(gated, Inventory{}, locArea, someDock).empty(), "gated: nothing with nothing");
  Check(ReachableLocations(gated, Items({kItemMorphBall}), locArea, someDock).empty(),
        "gated: the ledge alone is not the location");
  const std::vector<int> got = ReachableLocations(gated, Items({kItemMorphBall, 24}), locArea, someDock);
  Check(got.size() == 1 && got[0] == locIndex, "gated: both items reach the location", int(got.size()));
  Check(ReachableLocations(gated, Items({24}), locArea, someDock).empty(), "gated: second edge alone is not enough");
  Check(Contains(Graph(gated).Reachable(Items({kItemMorphBall}), DockNode(locArea, someDock)), ledge),
        "gated: named node reached");
}

void TestEvents() {
  // Landing Site: an event behind a Morph Ball edge opens dock0 > dock1 once reached.
  const Node n0 = DockNode(0, 0), n1 = DockNode(0, 1), tunnel{0, NodeKind::Named, 0};
  RuleSet rules;
  rules.push_back({n0, tunnel, Requirement::Item(kItemMorphBall)});
  rules.push_back({n0, n1, Requirement::Item(kFirstEventItem)});
  const EventSet events{{tunnel, kFirstEventItem}};
  Check(!Contains(Graph(rules).Reachable(Items({kItemMorphBall}), n0), n1), "event: no event set, no grant");
  Check(!Contains(Graph(rules, events).Reachable(Inventory{}, n0), n1), "event: its node unreached");
  Check(Contains(Graph(rules, events).Reachable(Items({kItemMorphBall}), n0), n1),
        "event: reaching its node opens the edge");
  Check(Contains(Graph(rules, events).Reachable(Items({kFirstEventItem}), n0), n1),
        "event: an inventory that already holds it");
  for (const RuleEvent& e : DefaultEvents())
    Check(e.item >= kFirstEventItem && e.item < kMaxItems, "compiled event ids are pseudo-items", e.item);
  Check(DefaultEvents().size() == 3, "the three temple boss events are compiled in",
        int(DefaultEvents().size()));
}

void TestDockLocks() {
  using namespace PortRandoWorld;
  Check(DockRequirement(-1).terms.empty(), "bad dock index = never");
  int missileOnly = -1, superRow = -1, permanent = -1, seekerRow = -1, pbRow = -1, translatorRow = -1;
  for (int i = 0; i < DockCount(); ++i) {
    const DockLock* l = DockLockAt(i);
    const Requirement r = DockRequirement(i);
    if (l->scriptLock == DockScriptLock::Permanent && permanent < 0)
      permanent = i;
    if (l->scriptLock == DockScriptLock::Permanent)
      continue;
    if (r.terms.size() == 1 && r.terms[0].size() == 1 && r.terms[0][0].item == kItemMissile &&
        missileOnly < 0)
      missileOnly = i;
    if (l->shield == DockShield::SuperMissile && superRow < 0)
      superRow = i;
    if (l->shield == DockShield::Seeker && seekerRow < 0)
      seekerRow = i;
    if (l->shield == DockShield::PowerBomb && pbRow < 0)
      pbRow = i;
    if (l->translator == DockTranslator::Violet && translatorRow < 0)
      translatorRow = i;
  }
  Check(missileOnly >= 0, "a dock that needs only a Missile exists");
  Check(superRow >= 0 && seekerRow >= 0 && pbRow >= 0 && translatorRow >= 0, "lock kinds present");
  Check(permanent >= 0, "a Permanent script lock exists");

  const Graph none((RuleSet()));
  if (missileOnly >= 0) {
    const Dock* d = DockAt(missileOnly);
    const Node from = DockNode(d->area, d->dock), to = DockNode(d->toArea, d->toDock);
    Check(!Contains(none.Reachable(Inventory{}, from), to), "missile dock closed without a Missile");
    Check(Contains(none.Reachable(Items({kItemMissile}), from), to), "missile dock opens with a Missile");
    Check(!Contains(none.Reachable(Items({kItemSuperMissile}), from), to), "a Super Missile alone is not a Missile");
  }
  auto needs = [](int row, std::initializer_list<int> items) {
    Inventory inv;
    for (int i : items)
      inv.Add(i);
    return DockRequirement(row).Satisfied(inv);
  };
  if (superRow >= 0) {
    Check(!needs(superRow, {kItemSuperMissile}), "super shield also wants a Missile");
    Check(needs(superRow, {kItemSuperMissile, kItemMissile, kItemLightBeam, kItemDarkBeam,
                            kItemAnnihilatorBeam, kItemMorphBall, kItemVioletTranslator, kItemAmberTranslator,
                            kItemEmeraldTranslator, kItemCobaltTranslator}),
          "super shield opens with everything");
  }
  if (seekerRow >= 0)
    Check(!needs(seekerRow, {kItemSeekerLauncher}), "seeker shield also wants a Missile");
  if (pbRow >= 0)
    Check(!needs(pbRow, {kItemPowerbomb}), "power bomb shield also wants the Morph Ball");
  if (translatorRow >= 0) {
    Check(!needs(translatorRow, {kItemMissile, kItemSuperMissile, kItemLightBeam, kItemDarkBeam, kItemMorphBall}),
          "translator door needs the translator");
    Check(needs(translatorRow, {kItemVioletTranslator, kItemMissile, kItemSuperMissile, kItemSeekerLauncher,
                                 kItemPowerbomb, kItemMorphBall, kItemLightBeam, kItemDarkBeam,
                                 kItemAnnihilatorBeam}),
          "translator door opens with the translator");
    // A gate counts from both faces unless it is listed one-sided (Temple Sanctuary's North gate,
    // which only the boss fight raises): walking back in from the other room needs the translator
    // for an ordinary gate and not for a one-sided one.
    auto reverseOf = [](int i) {
      const Dock* d = DockAt(i);
      for (int j = 0; j < DockCount(); ++j) {
        const Dock* r = DockAt(j);
        if (r->area == d->toArea && r->dock == d->toDock && r->toArea == d->area && r->toDock == d->dock)
          return j;
      }
      return -1;
    };
    const std::initializer_list<int> allButTranslators = {
        kItemMissile, kItemSuperMissile, kItemSeekerLauncher, kItemPowerbomb, kItemMorphBall,
        kItemLightBeam, kItemDarkBeam, kItemAnnihilatorBeam};
    int back = -1, oneSidedBack = -1, oneSided = 0;
    for (int i = 0; i < DockCount(); ++i) {
      const DockLock* l = DockLockAt(i);
      oneSided += l->translator != DockTranslator::None && l->translatorOneSided;
      const int r = l->translator != DockTranslator::None ? reverseOf(i) : -1;
      if (r < 0 || DockLockAt(r)->translator != DockTranslator::None || DockLockAt(r)->shield != DockShield::None)
        continue;
      if (l->translatorOneSided && oneSidedBack < 0)
        oneSidedBack = r;
      else if (!l->translatorOneSided && back < 0)
        back = r;
    }
    Check(oneSided == 1, "exactly one one-sided translator gate (ONE_SIDED_GATES)");
    Check(back >= 0 && oneSidedBack >= 0, "both kinds of translator gate face an unlocked door");
    if (back >= 0)
      Check(!needs(back, allButTranslators), "an ordinary gate also blocks walking back in");
    if (oneSidedBack >= 0)
      Check(needs(oneSidedBack, allButTranslators), "a one-sided gate opens from behind");
    // A beam colour binds only the face you shoot: a row whose own face is Any opens with no
    // beam even when the far face is coloured (Main Reactor -> Storage D).
    int anyFace = -1;
    for (int i = 0; i < DockCount() && anyFace < 0; ++i) {
      const DockLock* l = DockLockAt(i);
      const int r = reverseOf(i);
      if (l->beam == DockBeam::Any && l->shield == DockShield::None && l->translator == DockTranslator::None &&
          l->scriptLock != DockScriptLock::Permanent && r >= 0 && DockLockAt(r)->beam == DockBeam::Dark &&
          DockLockAt(r)->shield == DockShield::None)
        anyFace = i;
    }
    Check(anyFace >= 0, "a door coloured on one face only exists");
    if (anyFace >= 0) {
      Check(needs(anyFace, {}), "beam lock is one-sided: the Any face opens with nothing");
      Check(!needs(reverseOf(anyFace), {}), "the coloured face still needs its beam");
    }
  }
  if (permanent >= 0) {
    Check(DockRequirement(permanent).terms.empty(), "permanent lock = never");
    Inventory all;
    for (int i = 0; i < kMaxItems; ++i)
      all.Set(i, 99);
    const Dock* d = DockAt(permanent);
    Check(!Contains(none.Reachable(all, DockNode(d->area, d->dock)), DockNode(d->toArea, d->toDock)),
          "permanent lock blocks even a full inventory");
  }
  // Every dock row's requirement is satisfiable by a full inventory unless Permanent.
  Inventory all;
  for (int i = 0; i < kMaxItems; ++i)
    all.Set(i, 99);
  for (int i = 0; i < DockCount(); ++i)
    Check(DockLockAt(i)->scriptLock == DockScriptLock::Permanent || DockRequirement(i).Satisfied(all),
          "full inventory passes every non-permanent dock", i);
}

void TestFailClosed() {
  using namespace PortRandoWorld;
  // No rules at all: from a dock you can only walk through doors, never reach a location, and the
  // room's other docks are not reached.
  const Graph none((RuleSet()));
  Inventory all;
  for (int i = 0; i < kMaxItems; ++i)
    all.Set(i, 99);
  const Dock* d = DockAt(0);
  const std::vector<Node> reached = none.Reachable(all, DockNode(d->area, d->dock));
  Check(Contains(reached, DockNode(d->toArea, d->toDock)), "the neighbouring dock is reached through the door");
  for (const Node& n : reached)
    Check(n.kind == NodeKind::Dock || n.kind == NodeKind::Teleporter, "fail closed: only dock nodes", int(n.kind));
  Check(none.ReachableLocations(all, DockNode(d->area, d->dock)).empty(), "fail closed: no locations");
  // Rules for one area do not open its neighbours.
  RuleSet one;
  one.push_back({DockNode(d->toArea, d->toDock), DockNode(d->toArea, d->toDock == 0 ? 1 : 0), Requirement::Any()});
  Check(ReachableLocations(one, all, d->area, d->dock).empty(), "one block opens no location");
  Check(none.Reachable(all, DockNode(-5, 3)).empty(), "unknown start = nothing");
}

void TestTeleporters() {
  using namespace PortRandoWorld;
  const Graph none((RuleSet()));
  int paired = 0, unpaired = 0;
  for (int i = 0; i < TeleporterCount(); ++i) {
    const Teleporter* t = TeleporterAt(i);
    const int dest = FindArea(t->toMlvl, t->toMrea);
    bool partner = false;
    for (int j = 0; j < TeleporterCount() && dest >= 0; ++j) {
      const Teleporter* u = TeleporterAt(j);
      if (u->area != dest || FindArea(u->toMlvl, u->toMrea) != t->area)
        continue;
      partner = true;
      Check(Contains(none.Reachable(Inventory{}, {t->area, NodeKind::Teleporter, t->editorId}),
                     {u->area, NodeKind::Teleporter, u->editorId}),
            "teleporter reaches its partner pad", i);
    }
    (partner ? paired : unpaired)++;
  }
  std::printf("teleporters: %d with a partner pad, %d without (fail closed)\n", paired, unpaired);
  Check(paired > 0, "some teleporters pair up", paired);
}

// Vanilla placement, sphere by sphere, from the Landing Site with the starting inventory: every
// location must open. A sphere takes everything reachable with the items of the spheres before it.
void TestVanillaSpheres() {
  using namespace PortRandoWorld;
  const auto areaNamed = [](const char* name) {
    for (int a = 0; a < AreaCount(); ++a)
      if (std::strcmp(AreaAt(a)->name, name) == 0)
        return a;
    return -1;
  };
  const int start = areaNamed("01_Temple_LandingSite");
  Check(start >= 0, "vanilla: the Landing Site exists");
  if (start < 0)
    return;
  Inventory inv;
  for (int item : {0, 22, 8, 9, 12, 15}) // Power Beam, Charge Beam, Combat/Scan Visor, Varia Suit, Morph Ball
    inv.Set(item, 1);
  // Items scripts grant rather than pickups (RANDOMIZER.md step 5), granted once their room is
  // reached. Each translator comes from its world's energy controller: U-Mos at the Main Energy
  // Controller gives the Violet Translator with the Energy Transfer Module (item 28, which the
  // Agon/Torvus/Sanctuary energy rules ask for), and the Agon, Torvus and Sanctuary controllers'
  // holograms give the Amber, Emerald and Cobalt Translators.
  const struct {
    const char* area;
    int item, extra;
  } grants[] = {
      {"08_temple_sungenerator", kItemVioletTranslator, 28},
      {"00_sand_generator", kItemAmberTranslator, -1},
      {"00_swamp_generator", kItemEmeraldTranslator, -1},
      {"00_cliff_generator", kItemCobaltTranslator, -1},
  };
  std::set<int> have;
  std::map<int, int> sphere; // location -> its sphere
  int spheres = 0;
  for (;;) {
    bool granted = false;
    for (const Node& n : ReachableNodes(inv, start, 0))
      for (const auto& g : grants)
        if (inv.Count(g.item) == 0 && std::strcmp(AreaAt(n.area)->name, g.area) == 0) {
          inv.Add(g.item);
          if (g.extra >= 0)
            inv.Add(g.extra);
          granted = true;
        }
    if (granted)
      continue;
    std::vector<int> fresh;
    for (int i : ReachableLocations(inv, start, 0))
      if (!have.count(i))
        fresh.push_back(i);
    if (fresh.empty())
      break;
    ++spheres;
    for (int i : fresh) {
      const Location* l = LocationAt(i);
      // Expansions give their amount (an Energy Tank pickup is one tank; ammo is many).
      const bool counted = l->item == 42 || l->item == kItemPowerbomb || l->item == kItemMissile;
      inv.Add(l->item, counted && l->amount > 0 ? l->amount : 1);
      have.insert(i);
      sphere[i] = spheres;
    }
  }
  // No exemptions: vanilla must reach every location.
  for (int i = 0; i < LocationCount(); ++i)
    if (!have.count(i))
      std::fprintf(stderr, "vanilla: stuck at %s 0x%08X (%s)\n", AreaAt(LocationAt(i)->area)->name,
                   LocationAt(i)->editorId, LocationAt(i)->itemName);
  Check(int(have.size()) == LocationCount(), "vanilla: every location in some sphere", int(have.size()));
  // The Light Suit (Main Energy Controller) opens only after the three temple bosses' drops.
  const auto sphereOf = [&](uint32_t editorId) {
    for (int i = 0; i < LocationCount(); ++i)
      if (LocationAt(i)->editorId == editorId)
        return sphere.count(i) ? sphere[i] : -1;
    return -1;
  };
  const int lightSuit = sphereOf(0x200702C7);
  for (uint32_t boss : {0x28240265u, 0x382000B3u, 0x14350117u}) // Amorbis, Chykka, Quadraxis
    Check(sphereOf(boss) > 0 && lightSuit >= sphereOf(boss), "vanilla: Light Suit not before a temple boss",
          sphereOf(boss));
  // Events are granted inside one search, so the last boss and the suit share a sphere; without
  // the events the suit stays shut even with everything else.
  const std::vector<int> noEvents = Graph(DefaultRules()).ReachableLocations(inv, DockNode(start, 0));
  Check(int(noEvents.size()) == LocationCount() - 1, "vanilla: only the Light Suit needs the events",
        int(noEvents.size()));
  std::printf("vanilla: %zu/%d locations in %d spheres, %d Energy Tanks\n", have.size(), LocationCount(),
              spheres, inv.Count(42));
}

} // namespace

int main() {
  TestRequirements();
  TestRuleEdges();
  TestDockLocks();
  TestFailClosed();
  TestTeleporters();
  TestEvents();
  TestVanillaSpheres();
  // The compiled-in rules load and answer (their content is another file's business).
  const Inventory none;
  (void)ReachableLocations(none, 0, 0);
  (void)ReachableNodes(none, 0, 0);
  if (g_failures != 0) {
    std::fprintf(stderr, "%d failure(s)\n", g_failures);
    return 1;
  }
  std::printf("port_rando_logic_tests: OK (%d rule blocks compiled in)\n", RuleBlockCount());
  return 0;
}
