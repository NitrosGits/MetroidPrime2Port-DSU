#include "port_rando_gen.h"

#include "port_rando_logic.h"
#include "port_rando_world.h"
#include "port_randomizer.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace PortRandomizer {
namespace {

// Fills tried before a seed name gives up. An assumed fill only fails when a progression item
// finds no empty reachable location, which the measured runs never hit (tests/port_rando_gen.cpp).
constexpr int kMaxAttempts = 64;

// Own PRNG and hash: std::shuffle and the standard distributions differ between libraries, and a
// seed name has to give the same placements on the desktop and the phone.
uint64_t SplitMix(uint64_t& state) {
  uint64_t z = (state += 0x9E3779B97F4A7C15ull);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}

uint64_t HashText(const std::string& text) {
  uint64_t h = 0xCBF29CE484222325ull;
  for (unsigned char c : text) {
    h ^= c;
    h *= 0x100000001B3ull;
  }
  return h;
}

size_t Below(uint64_t& rng, size_t n) { return static_cast<size_t>(SplitMix(rng) % n); }

void Shuffle(std::vector<int>& v, uint64_t& rng) {
  for (size_t i = v.size(); i > 1; --i)
    std::swap(v[i - 1], v[Below(rng, i)]);
}

std::string JsonEscape(const std::string& in) {
  std::string out;
  for (char c : in) {
    if (c == '"' || c == '\\')
      out.push_back('\\');
    if (static_cast<unsigned char>(c) >= 0x20)
      out.push_back(c);
  }
  return out;
}

// One assumed fill. The progression copies are shuffled into an order x1..xm, and each xk goes
// into a random empty location reachable with the start and x1..x(k-1): then x1 is reachable from
// the start, x2 once x1 is collected, and so on, so the placement is beatable by construction
// (the Beatable check after it guards this). Reachability depends only on the inventory, so the
// regions are computed first and the copies placed smallest region first; the regions are nested,
// so this finds a location for every copy whenever the order allows one (a region of k locations
// for at least k copies), where placing in collection order would fill the small early regions by
// chance (the start reaches 2 locations). The rest fill the remaining locations at random. A progression copy is a pool copy of an item some requirement names, up to the count
// the largest such requirement asks for (one Missile expansion holds 5 missiles and the rules ask
// for 1; Energy Tanks count one per tank): further copies unlock nothing, and treating them as
// progression would leave the fill's last steps (the start reaches 2 locations) no room.
// False when the order leaves a copy no location.
bool AssumedFill(uint64_t& rng, std::vector<int>& source) {
  using namespace PortRandoLogic;
  const int n = PortRandoWorld::LocationCount();
  const std::vector<int> need = RequiredCounts();
  std::vector<int> copies(n);
  for (int i = 0; i < n; ++i)
    copies[i] = i;
  Shuffle(copies, rng); // which copies of a counted item are the progression ones
  std::vector<int> progression, filler; // pool copies, by their vanilla location
  Inventory counted;
  for (int i : copies) {
    const PortRandoWorld::Location* l = PortRandoWorld::LocationAt(i);
    if (counted.Count(l->item) < need[l->item]) {
      AddPickup(counted, l->item, l->amount);
      progression.push_back(i);
    } else {
      filler.push_back(i);
    }
  }
  Shuffle(progression, rng);
  source.assign(n, -1);
  const auto openWith = [&](Inventory inventory) { // a copy: ReachableWithGrants adds the grants
    std::vector<int> open;
    for (int loc : ReachableWithGrants(inventory))
      if (source[loc] < 0)
        open.push_back(loc);
    return open;
  };
  const auto add = [](Inventory& inventory, int copy) {
    AddPickup(inventory, PortRandoWorld::LocationAt(copy)->item, PortRandoWorld::LocationAt(copy)->amount);
  };
  Inventory assumed = StartInventory();
  for (size_t k = 0; k < progression.size(); ++k) {
    const std::vector<int> open = openWith(assumed);
    if (open.empty())
      return false;
    // On the last empty location, place next the first remaining copy that opens another one,
    // when there is such a copy; the shuffled order alone runs out of room on most attempts.
    if (open.size() == 1 && k + 1 < progression.size()) {
      for (size_t j = k; j < progression.size(); ++j) {
        Inventory with = assumed;
        add(with, progression[j]);
        if (openWith(with).size() > 1) {
          std::swap(progression[k], progression[j]);
          break;
        }
      }
    }
    source[open[Below(rng, open.size())]] = progression[k];
    add(assumed, progression[k]);
  }
  Shuffle(filler, rng);
  for (int loc = 0; loc < n; ++loc)
    if (source[loc] < 0) {
      source[loc] = filler.back();
      filler.pop_back();
    }
  return true;
}

} // namespace

bool GeneratePlacement(const std::string& seedName, std::vector<int>& source, GenStats& stats,
                       std::string& error) {
  stats = GenStats{};
  if (PortRandoWorld::LocationCount() < 2 || PortRandoLogic::StartNode().area < 0) {
    error = "no world data (locations or the start area) compiled in";
    return false;
  }
  uint64_t rng = HashText(seedName);
  while (stats.attempts < kMaxAttempts) {
    ++stats.attempts;
    if (!AssumedFill(rng, source))
      continue;
    std::vector<PortRandoLogic::Pickup> placement;
    for (int from : source)
      placement.push_back({PortRandoWorld::LocationAt(from)->item, PortRandoWorld::LocationAt(from)->amount});
    if (PortRandoLogic::Beatable(placement, &stats.spheres))
      return true;
  }
  error = "no beatable placement in " + std::to_string(kMaxAttempts) + " attempts";
  return false;
}

std::string PlacementJson(const std::string& seedName, const std::vector<int>& source) {
  std::map<std::string, int> byKey; // key order, as the dump-based generator wrote them
  for (int i = 0; i < static_cast<int>(source.size()); ++i) {
    const PortRandoWorld::Location* loc = PortRandoWorld::LocationAt(i);
    const PortRandoWorld::Area* area = PortRandoWorld::AreaAt(loc->area);
    char key[40];
    FormatLocationKey(PortRandoWorld::WorldAt(area->world)->mlvl, area->mrea, loc->editorId, key,
                      sizeof(key));
    byKey[key] = source[i];
  }
  std::string json = "{\"seed\":\"" + JsonEscape(seedName) + "\",\"locations\":{";
  bool first = true;
  for (const auto& [key, from] : byKey) {
    const PortRandoWorld::Location* item = PortRandoWorld::LocationAt(from);
    char buf[160];
    std::snprintf(buf, sizeof(buf), "%s\"%s\":{\"item\":\"%s\",\"amount\":%d,\"capacity\":%d}",
                  first ? "" : ",", key.c_str(), item->itemName, item->amount, item->capacity);
    json += buf;
    first = false;
  }
  json += "}}\n";
  return json;
}

bool GenerateSeedJson(const std::string& seedName, std::string& outJson, GenStats& stats,
                      std::string& error) {
  std::vector<int> source;
  if (!GeneratePlacement(seedName, source, stats, error))
    return false;
  outJson = PlacementJson(seedName, source);
  return true;
}

bool GenerateSeedFile(const std::string& seedName, const std::string& outPath) {
  std::string json, error;
  GenStats stats;
  if (!GenerateSeedJson(seedName, json, stats, error)) {
    std::fprintf(stderr, "randomizer: generate: %s\n", error.c_str());
    return false;
  }
  std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
  if (!out) {
    std::fprintf(stderr, "randomizer: generate: cannot write %s\n", outPath.c_str());
    return false;
  }
  out << json;
  std::fprintf(stderr, "randomizer: generated seed '%s', %d locations, %d spheres, %d attempt(s) -> %s\n",
               seedName.c_str(), PortRandoWorld::LocationCount(), stats.spheres, stats.attempts,
               outPath.c_str());
  return true;
}

// Here rather than in port_randomizer.cpp so the generator and its tests share the hook's key
// format without the SDL-dependent loader.
void FormatLocationKey(uint32_t worldAssetId, uint32_t areaAssetId, uint32_t entityId, char* out,
                       int outSize) {
  if (out == nullptr || outSize <= 0)
    return;
  std::snprintf(out, static_cast<size_t>(outSize), "%08X:%08X:%08X",
                static_cast<unsigned int>(worldAssetId), static_cast<unsigned int>(areaAssetId),
                static_cast<unsigned int>(entityId));
}

} // namespace PortRandomizer
