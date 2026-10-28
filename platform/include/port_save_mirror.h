#pragma once

#include <string_view>

namespace PortSave::Mirror {

// Several releases can share a region's Card A folder. Only the mounted
// release's file can supply framing/layout for an import or its export.
inline bool IsCardForRelease(std::string_view header, std::string_view game) {
  const bool supported = game == "G2ME" || game == "G2MP" || game == "R3ME" || game == "R3MP";
  return supported && header.size() >= 6 && header.substr(0, 4) == game &&
         header.substr(4, 2) == "01";
}

} // namespace PortSave::Mirror
