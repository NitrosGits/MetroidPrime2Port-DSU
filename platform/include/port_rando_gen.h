#ifndef METROID_PRIME_PORT_PORT_RANDO_GEN_H
#define METROID_PRIME_PORT_PORT_RANDO_GEN_H
#include <string>
#include <vector>

namespace PortRandomizer {

// The seed generator (docs/RANDOMIZER.md): an assumed fill over PortRandoWorld's locations with
// their vanilla items, checked with PortRandoLogic::Beatable. Deterministic in `seedName` on every
// platform (own PRNG, no standard shuffles or distributions).

struct GenStats {
  int attempts = 0; // fills tried, the successful one included
  int spheres = 0;  // spheres of the accepted placement
};

// source[i] = the location whose vanilla item (with its amount and capacity) location i gets.
// False, with the reason in `error`, when no attempt within the bound is beatable.
bool GeneratePlacement(const std::string& seedName, std::vector<int>& source, GenStats& stats,
                       std::string& error);
// The seed JSON that port_randomizer.cpp loads, keyed as FormatLocationKey.
std::string PlacementJson(const std::string& seedName, const std::vector<int>& source);
bool GenerateSeedJson(const std::string& seedName, std::string& outJson, GenStats& stats,
                      std::string& error);
// Writes the seed JSON to `outPath`. Logs to stderr.
bool GenerateSeedFile(const std::string& seedName, const std::string& outPath);

} // namespace PortRandomizer

#endif
