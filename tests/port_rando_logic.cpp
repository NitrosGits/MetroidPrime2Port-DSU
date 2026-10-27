// The randomizer's reachability evaluator. The rule edges are injected (there may be no world rule
// files yet); the dock locks and teleporters come from the disc data in PortRandoWorld.

#include "port_rando_logic.h"
#include "port_rando_world.h"

#include <algorithm>
#include <cstdio>

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

} // namespace

int main() {
  TestRequirements();
  TestRuleEdges();
  TestDockLocks();
  TestFailClosed();
  TestTeleporters();
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
