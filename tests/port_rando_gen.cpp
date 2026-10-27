// The seed generator: deterministic in the seed name, keeps the vanilla item pool, keys locations
// as the in-game hook does, and every seed it writes is beatable under the compiled-in logic.

#include "port_rando_gen.h"
#include "port_rando_logic.h"
#include "port_rando_world.h"
#include "port_randomizer.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <tuple>
#include <vector>

namespace {

int g_failures = 0;

void Check(bool condition, const char* what, int detail = 0) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s (%d)\n", what, detail);
    g_failures++;
  }
}

using PortRandomizer::GenStats;

struct Entry {
  unsigned world, area, entity;
  std::string item;
  int amount, capacity;
};

// The generator's own JSON, read back with the shape it writes (one flat object per key).
std::vector<Entry> ParseSeed(const std::string& json) {
  std::vector<Entry> out;
  size_t at = json.find("\"locations\":{");
  if (at == std::string::npos)
    return out;
  at += 13;
  for (;;) {
    Entry e{};
    char item[64];
    int used = 0;
    if (std::sscanf(json.c_str() + at, "\"%8X:%8X:%8X\":{\"item\":\"%63[^\"]\",\"amount\":%d,\"capacity\":%d}%n",
                    &e.world, &e.area, &e.entity, item, &e.amount, &e.capacity, &used) != 6 || used == 0)
      break;
    e.item = item;
    out.push_back(e);
    at += used;
    if (json[at] != ',')
      break;
    ++at;
  }
  return out;
}

using Multiset = std::vector<std::tuple<std::string, int, int>>;

Multiset VanillaPool() {
  Multiset out;
  for (int i = 0; i < PortRandoWorld::LocationCount(); ++i) {
    const PortRandoWorld::Location* l = PortRandoWorld::LocationAt(i);
    out.emplace_back(l->itemName, l->amount, l->capacity);
  }
  std::sort(out.begin(), out.end());
  return out;
}

void TestDeterminismAndFormat() {
  std::string a, b, c, error;
  GenStats stats;
  Check(PortRandomizer::GenerateSeedJson("echoes1", a, stats, error), "generate echoes1");
  Check(PortRandomizer::GenerateSeedJson("echoes1", b, stats, error), "generate echoes1 again");
  Check(PortRandomizer::GenerateSeedJson("echoes2", c, stats, error), "generate echoes2");
  Check(!a.empty() && a == b, "same name, same JSON");
  Check(a != c, "another name, another JSON");
  Check(a.rfind("{\"seed\":\"echoes1\",\"locations\":{", 0) == 0, "JSON header");
  Check(a.size() > 3 && a.compare(a.size() - 3, 3, "}}\n") == 0, "JSON trailer");

  const std::vector<Entry> entries = ParseSeed(a);
  Check(int(entries.size()) == PortRandoWorld::LocationCount(), "one key per location", int(entries.size()));
  // Keys: exactly the hook's FormatLocationKey(MLVL, MREA, editor id) of each location, each once.
  std::vector<int> seen(PortRandoWorld::LocationCount(), 0);
  Multiset got;
  for (const Entry& e : entries) {
    const int i = PortRandoWorld::FindLocation(e.world, e.area, e.entity);
    Check(i >= 0, "key names a location", int(e.entity));
    if (i < 0)
      continue;
    seen[i]++;
    char key[40];
    PortRandomizer::FormatLocationKey(e.world, e.area, e.entity, key, sizeof(key));
    Check(a.find(std::string("\"") + key + "\":{") != std::string::npos, "key spelled as FormatLocationKey", i);
    got.emplace_back(e.item, e.amount, e.capacity);
  }
  for (int i = 0; i < PortRandoWorld::LocationCount(); ++i) {
    Check(seen[i] == 1, "every location keyed once", i);
    const PortRandoWorld::Location* l = PortRandoWorld::LocationAt(i);
    const PortRandoWorld::Area* area = PortRandoWorld::AreaAt(l->area);
    char key[40];
    PortRandomizer::FormatLocationKey(PortRandoWorld::WorldAt(area->world)->mlvl, area->mrea, l->editorId, key,
                                      sizeof(key));
    Check(a.find(std::string("\"") + key + "\":{") != std::string::npos, "LocationAt's key is in the seed", i);
  }
  std::sort(got.begin(), got.end());
  Check(got == VanillaPool(), "item multiset (with amount and capacity) preserved");
  char key[40];
  PortRandomizer::FormatLocationKey(0x3BFA3EFFu, 0x0000ABCDu, 0x18060234u, key, sizeof(key));
  Check(std::strcmp(key, "3BFA3EFF:0000ABCD:18060234") == 0, "key format: upper-case, zero-padded");
}

void TestManySeeds() {
  const int seeds = 200;
  int beatable = 0, sphereSum = 0, attemptSum = 0, maxAttempts = 0, minSpheres = 1 << 30, maxSpheres = 0;
  const Multiset pool = VanillaPool();
  const auto t0 = std::chrono::steady_clock::now();
  for (int s = 0; s < seeds; ++s) {
    const std::string name = "seed-" + std::to_string(s);
    std::vector<int> source;
    GenStats stats;
    std::string error;
    if (!PortRandomizer::GeneratePlacement(name, source, stats, error)) {
      std::fprintf(stderr, "%s: %s\n", name.c_str(), error.c_str());
      Check(false, "generation succeeds", s);
      continue;
    }
    // Independent of the generator's own check: collect the placement again.
    std::vector<PortRandoLogic::Pickup> placement;
    Multiset got;
    for (int from : source) {
      const PortRandoWorld::Location* l = PortRandoWorld::LocationAt(from);
      placement.push_back({l->item, l->amount});
      got.emplace_back(l->itemName, l->amount, l->capacity);
    }
    std::sort(got.begin(), got.end());
    Check(got == pool, "placement keeps the pool", s);
    int spheres = 0;
    const bool ok = PortRandoLogic::Beatable(placement, &spheres);
    Check(ok, "seed beatable", s);
    Check(spheres == stats.spheres, "reported spheres match", s);
    beatable += ok;
    sphereSum += spheres;
    attemptSum += stats.attempts;
    maxAttempts = std::max(maxAttempts, stats.attempts);
    minSpheres = std::min(minSpheres, spheres);
    maxSpheres = std::max(maxSpheres, spheres);
  }
  const double ms =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / seeds;
  std::printf("gen: %d/%d seeds beatable, spheres avg %.2f (min %d, max %d), attempts avg %.2f (max %d), "
              "%.1f ms/seed (incl. re-check)\n",
              beatable, seeds, double(sphereSum) / seeds, minSpheres, maxSpheres, double(attemptSum) / seeds,
              maxAttempts, ms);
  Check(beatable == seeds, "all seeds beatable", beatable);
}

} // namespace

int main() {
  const std::vector<int> progression = PortRandoLogic::ProgressionItems();
  int copies = 0;
  for (int i = 0; i < PortRandoWorld::LocationCount(); ++i)
    copies += std::binary_search(progression.begin(), progression.end(), PortRandoWorld::LocationAt(i)->item);
  std::printf("gen: %zu progression item ids, %d of %d pool copies of one (the fill caps them at RequiredCounts)\n",
              progression.size(), copies, PortRandoWorld::LocationCount());
  Check(std::binary_search(progression.begin(), progression.end(), int(PortRandoLogic::kItemMissile)),
        "Missile is progression");
  Check(!progression.empty() && progression.back() < PortRandoLogic::kFirstEventItem, "no events in progression");
  TestDeterminismAndFormat();
  TestManySeeds();
  if (g_failures != 0) {
    std::fprintf(stderr, "%d failure(s)\n", g_failures);
    return 1;
  }
  std::printf("port_rando_gen_tests: OK\n");
  return 0;
}
