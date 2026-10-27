#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Metroid Prime Trilogy's Wii save.bin, the container side (MP1 port: port_save_convert.cpp WriteWii
// and the Trilogy block; the layout is in docs/TRILOGY.md). Pure byte work with no game classes, so
// tests/port_save_trilogy.cpp builds it on the host against real save.bins.
//
// save.bin is 0x1C000 bytes: a big-endian CRC32 at 0 over [4, 0x1980C), a header (the current file
// at 8, a count of 3 at 0x10, a 7-byte entry per file at 0x11), one 0x258-byte summary per file that
// the file select shows (Echoes' part at +0x70), a constant at 0x76C that the load checks whenever a
// file is used, and the slots: Echoes' three at 0x3660, 4000 bytes each, part A (the options, 2000)
// then part B (the file's CGameState stream, 2000). A file is a Mii's, shared by all three games.
//
// What is *not* here: turning the port's G2ME stream into those two parts and back
// (port_save_trilogy_payload.cpp is the seam; the converter belongs to port_save_convert).
namespace PortSave::Trilogy {

constexpr size_t kSize = 0x1C000;
constexpr size_t kFiles = 3;
constexpr size_t kSlotBase = 0x3660;
constexpr size_t kSlotSize = 4000;
constexpr size_t kPartSize = kSlotSize / 2;
constexpr size_t kSummaryEchoes = 0x70;
constexpr size_t kSummaryBytes = 41;

// The Echoes line of a file's summary, as the file select shows it.
struct Summary {
  uint32_t difficulty = 0; // 0 Normal, 1 Hard
  uint32_t percent = 0;    // the item percentage
  double playTime = 0.0;   // seconds
  uint32_t tanks = 0;      // energy tanks
  uint32_t mlvl = 0;       // the world's asset id
};

// One file's Echoes data: the two halves of its slot and the summary line.
struct Payload {
  uint8_t partA[kPartSize] = {};
  uint8_t partB[kPartSize] = {};
  Summary summary;
};

uint32_t Checksum(const uint8_t* bin); // what offset 0 must hold
bool Validate(const uint8_t* bin, size_t size, std::string& error);
bool FileUsed(const uint8_t* bin, int file);  // the header's entry for it is set
bool FileOwned(const uint8_t* bin, int file); // a Mii's id is in its summary
// The file the newest save goes into: the header's current one when a Mii owns it, else the first
// owned one; -1 when none (the Trilogy has to make the Mii's file first).
int TargetFile(const uint8_t* bin);
// Whether the file's Echoes summary says it has progress.
bool HasEchoes(const uint8_t* bin, int file);

bool Read(const uint8_t* bin, int file, Payload& out);
// `base` (a valid save.bin) with `payload` written into `file`: the slot, the summary line, the
// header entry when the file was unused, the constant, the CRC. The MP1 and MP3 parts are untouched.
bool Write(const std::vector<uint8_t>& base, int file, const Payload& payload, std::vector<uint8_t>& out,
           std::string& error);

// The seam to the stream converter. Fill the two halves from slot `slot` of a G2ME/G2MP .gci, or
// build the .gci back from a payload (`gci` holds the card's file to update, or is empty).
bool PayloadFromGci(const std::vector<uint8_t>& gci, int slot, uint8_t partA[kPartSize], uint8_t partB[kPartSize],
                    std::string& error);
bool GciFromPayload(const Payload& payload, std::vector<uint8_t>& gci, std::string& error);

} // namespace PortSave::Trilogy
