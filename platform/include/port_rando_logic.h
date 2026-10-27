#ifndef METROID_PRIME_PORT_PORT_RANDO_LOGIC_H
#define METROID_PRIME_PORT_PORT_RANDO_LOGIC_H
#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

// The randomizer's reachability logic: which pickup locations an inventory can reach from a start
// dock. Edges inside a room are hand-written rules (tools/rando_rules/*.rules, compiled into
// port_rando_logic.inc by tools/rando_rules.py); edges between rooms are the MLVL docks of
// PortRandoWorld with their door locks applied here; teleporters link to the matching teleporter
// of the destination area. An area with no rule block connects nothing (fail closed). Pure data and
// logic: no game dependencies, so a test can call it. Format: docs/RANDOMIZER.md, "Hand rules".
namespace PortRandoLogic {

// EItemType values the logic itself names (the full table is PortRandomizer's kItems).
enum : int {
  kItemDarkBeam = 1,
  kItemLightBeam = 2,
  kItemAnnihilatorBeam = 3,
  kItemSuperMissile = 4,
  kItemMorphBall = 15,
  kItemSeekerLauncher = 26,
  kItemPowerbomb = 43,
  kItemMissile = 44,
  kItemVioletTranslator = 97,
  kItemAmberTranslator = 98,
  kItemEmeraldTranslator = 99,
  kItemCobaltTranslator = 100,
};

constexpr int kMaxItems = 128;

struct Inventory {
  int counts[kMaxItems] = {};
  int Count(int item) const { return item >= 0 && item < kMaxItems ? counts[item] : 0; }
  void Set(int item, int count) {
    if (item >= 0 && item < kMaxItems)
      counts[item] = count;
  }
  void Add(int item, int n = 1) {
    if (item >= 0 && item < kMaxItems)
      counts[item] += n;
  }
};

struct Literal {
  int item;
  int count; // the inventory must hold at least this many
};
using Term = std::vector<Literal>; // an AND; empty = always true

// An OR of Terms (DNF). No terms = never; one empty term = any.
struct Requirement {
  std::vector<Term> terms;
  static Requirement Any() { return Requirement{{Term{}}}; }
  static Requirement Never() { return Requirement{}; }
  static Requirement Item(int item, int count = 1) { return Requirement{{Term{{item, count}}}}; }
  bool Satisfied(const Inventory& inventory) const;
};
Requirement And(const Requirement& a, const Requirement& b);
Requirement Or(const Requirement& a, const Requirement& b);

enum class NodeKind { Dock, Location, Teleporter, Named };

// Dock: id is the area's dock number. Location / Teleporter: the script object's editor id.
// Named: an index into the area's @names (the rules' in-room nodes).
struct Node {
  int area;
  NodeKind kind;
  uint32_t id;
  bool operator==(const Node& o) const { return area == o.area && kind == o.kind && id == o.id; }
};

struct RuleEdge {
  Node from, to;
  Requirement requirement;
};
using RuleSet = std::vector<RuleEdge>;

// The rules compiled into port_rando_logic.inc.
const RuleSet& DefaultRules();
// Number of areas that have a rule block in the .inc.
int RuleBlockCount();

// What crossing PortRandoWorld::DockAt(dockIndex) from its own side needs: both sides' beam and
// shield, the translator, never for a Permanent script lock on the leaving side.
Requirement DockRequirement(int dockIndex);

// Rule edges plus dock and teleporter edges, indexed for repeated reachability queries.
class Graph {
public:
  explicit Graph(const RuleSet& rules);
  // Nodes reachable from the start node with this inventory (the start included if it exists).
  std::vector<Node> Reachable(const Inventory& inventory, const Node& start) const;
  // Indices into PortRandoWorld's locations of the Location nodes among them.
  std::vector<int> ReachableLocations(const Inventory& inventory, const Node& start) const;

private:
  struct Out {
    int to;
    Requirement requirement;
  };
  int NodeIndex(const Node& node);
  int FindNode(const Node& node) const;
  std::vector<Node> mNodes;
  std::map<std::tuple<int, int, uint32_t>, int> mIndex;
  std::vector<int> mLocation; // per node, PortRandoWorld location index or -1
  std::vector<std::vector<Out>> mOut;
};

// Location indices reachable from (startArea, startDock) with the compiled-in rules; sorted.
std::vector<int> ReachableLocations(const Inventory& inventory, int startArea, int startDock);
// Same with injected rules (tests).
std::vector<int> ReachableLocations(const RuleSet& rules, const Inventory& inventory, int startArea,
                                    int startDock);
// Lower level: all reachable nodes, compiled-in rules.
std::vector<Node> ReachableNodes(const Inventory& inventory, int startArea, int startDock);

} // namespace PortRandoLogic

#endif
