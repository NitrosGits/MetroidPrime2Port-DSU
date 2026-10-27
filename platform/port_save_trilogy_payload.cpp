// The stream converter's seam for the Trilogy save.bin (port_save_trilogy.h). The container side is
// complete; what remains is the payload, which another piece of work owns (port_save_convert).
//
// What the converter has to do, measured on the real save.bins (docs/TRILOGY.md):
//  * part B is the G2ME slot's GMST stream in the Wii's framing - the timestamp, then 2 flag bits
//    (hard mode and one more), then the world id - where the GameCube's CGameState::PutTo writes 3
//    bits (hard, initial-powerups, dark) before the world, and the per-world tables follow;
//  * part A is the options (SYST) with the Wii's leading bytes;
//  * the two are 2000 bytes each against SGameFileSlot's 0xa38.
#include "port_save_trilogy.h"

namespace PortSave::Trilogy {

bool PayloadFromGci(const std::vector<uint8_t>&, int, uint8_t[kPartSize], uint8_t[kPartSize], std::string& error) {
  error = "the G2ME -> Trilogy slot stream conversion is not written yet";
  return false;
}

bool GciFromPayload(const Payload&, std::vector<uint8_t>&, std::string& error) {
  error = "the Trilogy -> G2ME slot stream conversion is not written yet";
  return false;
}

} // namespace PortSave::Trilogy
