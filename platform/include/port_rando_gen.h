#ifndef METROID_PRIME_PORT_PORT_RANDO_GEN_H
#define METROID_PRIME_PORT_PORT_RANDO_GEN_H
#include <string>

namespace PortRandomizer {

// No-logic shuffle over the locations a dump run recorded (see docs/RANDOMIZER.md). Deterministic
// in `seedName`. Produces the seed JSON that port_randomizer.cpp loads.
bool GenerateFromDump(const std::string& dumpText, const std::string& seedName, std::string& outJson,
                      int& locationCount, std::string& error);
// Reads the dump log at `dumpPath`, writes the seed JSON to `outPath`. Logs to stderr.
bool GenerateSeedFile(const std::string& dumpPath, const std::string& seedName,
                      const std::string& outPath);

} // namespace PortRandomizer

#endif
