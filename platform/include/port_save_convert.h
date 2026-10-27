#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Converting a save between the two Echoes GameCube releases, G2ME (NTSC) and G2MP (PAL).
//
// The releases lay a slot's bitstream out the same way but disagree on what goes in it: the front
// end world has another MLVL ID, the scan union is 815 entries in G2ME and 818 in G2MP, the hints
// are the same 44 in another order, and Temple Grounds has a swapped relay and a ninth layer in
// one area. The converter is a bit-level transcoder (no model of the game state): it reads each
// field in the source release's layout and writes it in the target's, matching entries by ID (or
// by name for hints). The tables come from tools/savw_diff.py (port_save_convert_tables.inc).
//
// G2ME -> G2MP loses nothing. G2MP -> G2ME drops the three scans G2ME lacks, and the Temple Grounds
// layer G2ME lacks; Report counts only those that were set. See docs/SAVE_RELEASES.md.
namespace PortSave {
namespace Convert {

struct Report {
  std::string from, to;
  int slots = 0;
  int droppedScans = 0;
  int droppedRelays = 0;
  int droppedDoors = 0;
  int droppedUnmappable = 0;
  int droppedLayers = 0;

  bool Lossy() const {
    return droppedScans + droppedRelays + droppedDoors + droppedUnmappable + droppedLayers != 0;
  }
  // One line for a console or overlay: "G2MP -> G2ME, 2 slot(s); dropped: 1 scan(s)".
  std::string Summary() const;
};

// True for G2ME <-> G2MP, in either direction. Everything else (Trilogy, same release) is false.
bool CanConvert(const std::string& from, const std::string& to);

// The save data of one copy (0x1ff8 bytes at most: SSaveHeader, options, three slots), converted.
// `out` is the same size as `in`. Fails, with a reason in `error`, on any marker, ID or size that
// is not what the source release's layout says.
bool UserData(const uint8_t* in, size_t size, const std::string& from, const std::string& to,
              std::vector<uint8_t>& out, Report& report, std::string& error);

// A whole .gci converted to `targetCode` ("G2ME" or "G2MP"): the better of the source's two copies
// is converted and written as both copies of the result, signed for the target release, with the
// CRC and generation in the byte order the source file used. The header block is kept as is.
bool Gci(const std::vector<uint8_t>& in, const std::string& targetCode, std::vector<uint8_t>& out,
         Report& report, std::string& error);

} // namespace Convert
} // namespace PortSave
