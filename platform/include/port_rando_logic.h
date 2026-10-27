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
// Rule events (`event NAME: node` in the .rules) are pseudo-items from here up to kMaxItems.
constexpr int kFirstEventItem = 120;

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

// Reaching `at` grants `item` (an event such as a boss kill) for the rest of the search.
struct RuleEvent {
  Node at;
  int item;
};
using EventSet = std::vector<RuleEvent>;

// The rules compiled into port_rando_logic.inc.
const RuleSet& DefaultRules();
// The events compiled into port_rando_logic.inc.
const EventSet& DefaultEvents();
// Number of areas that have a rule block in the .inc.
int RuleBlockCount();

// What crossing PortRandoWorld::DockAt(dockIndex) from its own side needs: both sides' beam and
// shield, the translator, never for a Permanent script lock on the leaving side.
Requirement DockRequirement(int dockIndex);

// Rule edges plus dock and teleporter edges, indexed for repeated reachability queries.
class Graph {
public:
  explicit Graph(const RuleSet& rules, const EventSet& events = {});
  // Nodes reachable from the start node with this inventory (the start included if it exists).
  // A reached event node adds its event to the inventory and the search goes on until none is new.
  std::vector<Node> Reachable(const Inventory& inventory, const Node& start) const;
  // Indices into PortRandoWorld's locations of the Location nodes among them.
  std::vector<int> ReachableLocations(const Inventory& inventory, const Node& start) const;
  // Indices into PortRandoWorld's locations of the Location nodes among `nodes`; sorted.
  std::vector<int> LocationsAmong(const std::vector<Node>& nodes) const;

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
  std::vector<std::pair<int, int>> mEvents; // (node, event item)
};

// Location indices reachable from (startArea, startDock) with the compiled-in rules; sorted.
std::vector<int> ReachableLocations(const Inventory& inventory, int startArea, int startDock);
// Same with injected rules (tests).
std::vector<int> ReachableLocations(const RuleSet& rules, const Inventory& inventory, int startArea,
                                    int startDock);
// Lower level: all reachable nodes, compiled-in rules.
std::vector<Node> ReachableNodes(const Inventory& inventory, int startArea, int startDock);

// The playthrough model the vanilla test and the generator share: the post-steal start, the items
// scripts grant, and collecting a placement sphere by sphere with the compiled-in rules.

// The start: 01_Temple_LandingSite's first dock (area -1 if the world data lacks it), with Power
// Beam, Charge Beam, Combat and Scan Visor, Varia Suit and Morph Ball.
Node StartNode();
Inventory StartInventory();

// What one pickup adds: an expansion (EnergyTanks, Powerbomb, Missile) its amount, else one.
void AddPickup(Inventory& inventory, int item, int amount);

// Items scripts grant rather than pickups (RANDOMIZER.md step 5), granted once their area is
// reached: each world's energy controller gives its translator (and U-Mos the Energy Transfer
// Module's item 28 with the Violet one).
struct ScriptGrant {
  const char* area; // MLVL internal area name
  int item;
  int extra; // a second item, -1 for none
};
const std::vector<ScriptGrant>& ScriptGrants();

// Locations reachable from StartNode() with `inventory`; the script grants of every reached area
// are added to `inventory` (and the search repeated) first. Sorted.
std::vector<int> ReachableWithGrants(Inventory& inventory);

// One location's contents in a placement.
struct Pickup {
  int item;
  int amount;
};
// The vanilla placement: PortRandoWorld's locations' own items, in location order.
std::vector<Pickup> VanillaPlacement();

// A placement collected sphere by sphere from the start: a sphere takes every location reachable
// with the items of the spheres before it (and the grants and events those reach).
struct Playthrough {
  std::vector<int> sphere; // per location, its sphere (1-based); 0 = never reached
  int spheres = 0;
  int collected = 0;
  Inventory inventory; // at the end, grants and pickups (not events)
  bool Beatable() const { return collected == static_cast<int>(sphere.size()); }
};
// `placement` is indexed like PortRandoWorld's locations.
Playthrough Collect(const std::vector<Pickup>& placement);
// Every location is collectable; `spheres` gets the sphere count.
bool Beatable(const std::vector<Pickup>& placement, int* spheres = nullptr);

// Item ids any requirement names (the compiled rules with their atmosphere, and every dock lock),
// events excluded; sorted.
std::vector<int> ProgressionItems();
// Per item id (kMaxItems entries), the largest count any requirement asks for; 0 = never named.
// Pool copies of an item beyond what reaches this count unlock nothing.
std::vector<int> RequiredCounts();

} // namespace PortRandoLogic

#endif
