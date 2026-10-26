#include "port_entry.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <vector>

namespace port {
namespace entry {

DiscVersion IdentifyDisc(const DVDDiskID& id) {
  if (std::memcmp(id.company, "01", 2) != 0 || id.diskNumber != 0 || id.gameVersion != 0) {
    return DiscVersion::Unknown;
  }
  if (std::memcmp(id.gameName, "G2ME", 4) == 0) {
    return DiscVersion::Usa100;
  }
  if (std::memcmp(id.gameName, "G2MP", 4) == 0) {
    return DiscVersion::Pal;
  }
  if (std::memcmp(id.gameName, "R3ME", 4) == 0) {
    return DiscVersion::TrilogyUsa;
  }
  if (std::memcmp(id.gameName, "R3MP", 4) == 0) {
    return DiscVersion::TrilogyPal;
  }
  return DiscVersion::Unknown;
}

bool IsSupportedDisc(const DVDDiskID& id) { return IdentifyDisc(id) != DiscVersion::Unknown; }

const char* DiscVersionName(DiscVersion version) {
  switch (version) {
  case DiscVersion::Usa100: return "USA 1.00";
  case DiscVersion::Pal: return "PAL";
  case DiscVersion::TrilogyUsa: return "Metroid Prime Trilogy (USA)";
  case DiscVersion::TrilogyPal: return "Metroid Prime Trilogy (PAL)";
  default: return "unknown";
  }
}

const char* DiscGameCode(DiscVersion version) {
  switch (version) {
  case DiscVersion::Pal: return "G2MP";
  case DiscVersion::TrilogyUsa: return "R3ME";
  case DiscVersion::TrilogyPal: return "R3MP";
  default: return "G2ME";
  }
}

bool IsTrilogy(DiscVersion version) {
  return version == DiscVersion::TrilogyUsa || version == DiscVersion::TrilogyPal;
}

bool IsPalRegion(DiscVersion version) {
  return version == DiscVersion::Pal || version == DiscVersion::TrilogyPal;
}

bool IsDiscImageName(const std::string& name) {
  std::string lower = name;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return lower.size() > 4 && (lower.ends_with(".iso") || lower.ends_with(".gcm") || lower.ends_with(".rvz") ||
                         lower.ends_with(".wia") || lower.ends_with(".gcz") || lower.ends_with(".ciso") ||
                         lower.ends_with(".wbfs"));
}

std::string ResolveDiscPath(int argc, char** argv, const char* envDiscPath,
                            const std::string& executableDir) {
  for (int i = 1; i < argc; i++) {
    if (argv[i] != nullptr && argv[i][0] != '-' && IsDiscImageName(argv[i])) {
      return argv[i];
    }
  }

  if (envDiscPath != nullptr && envDiscPath[0] != '\0') {
    return envDiscPath;
  }

  if (!executableDir.empty()) {
    std::error_code error;
    std::filesystem::directory_iterator entries(executableDir, error);
    if (!error) {
      std::vector<std::string> candidates;
      for (const auto& entry : entries) {
        if (!entry.is_regular_file(error)) {
          continue;
        }
        const std::string name = entry.path().filename().string();
        if (IsDiscImageName(name)) {
          candidates.push_back(entry.path().string());
        }
      }
      // Deterministic when several images sit side by side.
      std::sort(candidates.begin(), candidates.end());
      if (!candidates.empty()) {
        return candidates.front();
      }
    }
  }

  return {};
}

} // namespace entry
} // namespace port
