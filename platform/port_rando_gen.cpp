#include "port_rando_gen.h"

#include "port_randomizer.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace PortRandomizer {
namespace {

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

struct Slot {
  std::string key;
  std::string item;
  int amount = 0;
  int capacity = 0;
};

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

} // namespace

bool GenerateFromDump(const std::string& dumpText, const std::string& seedName, std::string& outJson,
                      int& locationCount, std::string& error) {
  // "LOC <key> <ItemName> amount=<n> capacity=<n>", appended by every dump run; the last line for
  // a key wins so a re-dump after a code change replaces the old record.
  std::map<std::string, Slot> byKey;
  std::istringstream lines(dumpText);
  std::string line;
  while (std::getline(lines, line)) {
    char key[40], item[64];
    int amount = 0, capacity = 0;
    if (std::sscanf(line.c_str(), "LOC %39s %63s amount=%d capacity=%d", key, item, &amount,
                    &capacity) != 4)
      continue;
    if (ItemFromName(item) < 0)
      continue;
    byKey[key] = Slot{key, item, amount, capacity};
  }
  if (byKey.size() < 2) {
    error = "dump log has fewer than 2 locations; run with MP_RANDO_DUMP=1 and visit areas first";
    return false;
  }

  std::vector<Slot> slots;
  for (auto& kv : byKey)
    slots.push_back(kv.second);
  std::vector<Slot> pool = slots; // std::map order keeps this deterministic
  uint64_t rng = HashText(seedName);
  for (size_t i = pool.size() - 1; i > 0; --i) {
    const size_t j = static_cast<size_t>(SplitMix(rng) % (i + 1));
    std::swap(pool[i], pool[j]);
  }

  std::string json = "{\"seed\":\"" + JsonEscape(seedName) + "\",\"locations\":{";
  for (size_t i = 0; i < slots.size(); ++i) {
    char buf[160];
    std::snprintf(buf, sizeof(buf), "%s\"%s\":{\"item\":\"%s\",\"amount\":%d,\"capacity\":%d}",
                  i ? "," : "", slots[i].key.c_str(), pool[i].item.c_str(), pool[i].amount,
                  pool[i].capacity);
    json += buf;
  }
  json += "}}\n";
  outJson = std::move(json);
  locationCount = static_cast<int>(slots.size());
  return true;
}

bool GenerateSeedFile(const std::string& dumpPath, const std::string& seedName,
                      const std::string& outPath) {
  std::ifstream in(dumpPath, std::ios::binary);
  if (!in) {
    std::fprintf(stderr, "randomizer: generate: no dump log at %s\n", dumpPath.c_str());
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::string json, error;
  int count = 0;
  if (!GenerateFromDump(text, seedName, json, count, error)) {
    std::fprintf(stderr, "randomizer: generate: %s\n", error.c_str());
    return false;
  }
  std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
  if (!out) {
    std::fprintf(stderr, "randomizer: generate: cannot write %s\n", outPath.c_str());
    return false;
  }
  out << json;
  std::fprintf(stderr, "randomizer: generated seed '%s', %d locations -> %s\n", seedName.c_str(),
               count, outPath.c_str());
  return true;
}

} // namespace PortRandomizer
