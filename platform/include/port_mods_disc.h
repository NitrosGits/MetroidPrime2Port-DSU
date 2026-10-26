#pragma once

#include <dolphin/dvd.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace PortMods::detail {

// Shared with the standalone DVD/FST regression test; uses the mounted DVD tree.
inline std::vector<std::pair<int32_t, std::string>> DiscPaks(bool trilogy) {
  const auto lower = [](std::string text) {
    for (char& c : text) {
      if (c >= 'A' && c <= 'Z') {
        c = char(c - 'A' + 'a');
      }
    }
    return text;
  };
  std::vector<std::pair<int32_t, std::string>> paks;
  std::vector<std::string> dirs{"/"};
  while (!dirs.empty()) {
    const std::string dirPath = dirs.back();
    dirs.pop_back();
    DVDDir dir;
    if (!DVDOpenDir(dirPath.c_str(), &dir)) {
      continue;
    }
    DVDDirEntry entry;
    while (DVDReadDir(&dir, &entry)) {
      if (entry.name == nullptr) {
        continue;
      }
      const std::string path = dirPath + entry.name;
      if (entry.isDir) {
        // Trilogy's MP2 root remap retains the menu and other games beside it.
        // Only exclude those root directories, never Echoes files or nested names.
        const std::string name = lower(entry.name);
        if (trilogy && dirPath == "/" && (name == "fe" || name == "mp1" || name == "mp3")) {
          continue;
        }
        dirs.push_back(path + "/");
      } else {
        const std::string name = lower(path);
        if (name.size() >= 4 && name.compare(name.size() - 4, 4, ".pak") == 0) {
          paks.emplace_back(int32_t(entry.entryNum), path);
        }
      }
    }
    DVDCloseDir(&dir);
  }
  std::sort(paks.begin(), paks.end());
  return paks;
}

} // namespace PortMods::detail
