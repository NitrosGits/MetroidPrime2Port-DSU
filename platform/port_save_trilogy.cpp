#include "port_save_trilogy.h"

#include <algorithm>
#include <cstring>

namespace PortSave::Trilogy {
namespace {

constexpr size_t kEntry = 0x11;
constexpr size_t kSummary[kFiles] = {0x64, 0x2BC, 0x514};
constexpr size_t kOwner = 0x3F;
constexpr size_t kHash = 0x76C;
constexpr uint32_t kHashValue = 0x4F54DF87;
constexpr size_t kCrcEnd = 0x1980C;
constexpr uint32_t kFormat = 0x030D1624;

uint32_t Be32(const uint8_t* p) {
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}
void PutBe32(uint8_t* p, uint32_t v) {
  p[0] = uint8_t(v >> 24);
  p[1] = uint8_t(v >> 16);
  p[2] = uint8_t(v >> 8);
  p[3] = uint8_t(v);
}
uint64_t Be64(const uint8_t* p) { return (uint64_t(Be32(p)) << 32) | Be32(p + 4); }
void PutBe64(uint8_t* p, uint64_t v) {
  PutBe32(p, uint32_t(v >> 32));
  PutBe32(p + 4, uint32_t(v));
}
double ToDouble(uint64_t bits) {
  double d;
  std::memcpy(&d, &bits, sizeof d);
  return d;
}
uint64_t FromDouble(double d) {
  uint64_t bits;
  std::memcpy(&bits, &d, sizeof bits);
  return bits;
}

uint32_t Crc32(const uint8_t* p, size_t n) {
  uint32_t c = 0xFFFFFFFFu;
  for (size_t i = 0; i < n; ++i) {
    c ^= p[i];
    for (int k = 0; k < 8; ++k) {
      c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
    }
  }
  return ~c;
}

} // namespace

uint32_t Checksum(const uint8_t* bin) { return Crc32(bin + 4, kCrcEnd - 4) ^ 0xFFFFFFFFu; }

bool Validate(const uint8_t* bin, size_t size, std::string& error) {
  if (size != kSize) {
    error = "save.bin is " + std::to_string(size) + " bytes, not " + std::to_string(kSize);
    return false;
  }
  if (Be32(bin + 4) != kFormat) {
    error = "save.bin has an unknown format id";
    return false;
  }
  if (Be32(bin) != Checksum(bin)) {
    error = "save.bin fails its checksum";
    return false;
  }
  return true;
}

bool FileUsed(const uint8_t* bin, int file) {
  const uint8_t* e = bin + kEntry + 7 * size_t(file);
  return e[0] != 0 && e[4] != 0;
}

bool FileOwned(const uint8_t* bin, int file) {
  const uint8_t* id = bin + kSummary[file] + kOwner;
  return std::any_of(id, id + 8, [](uint8_t b) { return b != 0xFF; });
}

int TargetFile(const uint8_t* bin) {
  const uint32_t current = Be32(bin + 8);
  if (current < kFiles && FileOwned(bin, int(current))) {
    return int(current);
  }
  for (int i = 0; i < int(kFiles); ++i) {
    if (FileOwned(bin, i)) {
      return i;
    }
  }
  return -1;
}

bool HasEchoes(const uint8_t* bin, int file) {
  const uint8_t* m = bin + kSummary[file] + kSummaryEchoes;
  return Be32(m + 4) != 0xFFFFFFFFu && Be32(m + 24) == 3;
}

bool Read(const uint8_t* bin, int file, Payload& out) {
  if (file < 0 || file >= int(kFiles) || !FileUsed(bin, file) || !HasEchoes(bin, file)) {
    return false;
  }
  const uint8_t* slot = bin + kSlotBase + kSlotSize * size_t(file);
  std::memcpy(out.partA, slot, kPartSize);
  std::memcpy(out.partB, slot + kPartSize, kPartSize);
  const uint8_t* m = bin + kSummary[file] + kSummaryEchoes;
  out.summary.difficulty = Be32(m);
  out.summary.percent = Be32(m + 8);
  out.summary.playTime = ToDouble(Be64(m + 12));
  out.summary.tanks = Be32(m + 20);
  out.summary.mlvl = Be32(m + 35);
  return true;
}

bool Write(const std::vector<uint8_t>& base, int file, const Payload& payload, std::vector<uint8_t>& out,
           std::string& error) {
  if (!Validate(base.data(), base.size(), error)) {
    error = "the Trilogy save to write into: " + error;
    return false;
  }
  if (file < 0 || file >= int(kFiles)) {
    error = "no Trilogy file to write into";
    return false;
  }
  out = base;
  uint8_t* d = out.data();
  uint8_t* slot = d + kSlotBase + kSlotSize * size_t(file);
  uint8_t* sum = d + kSummary[file];
  uint8_t* entry = d + kEntry + 7 * size_t(file);
  if (!FileUsed(d, file)) {
    // A used file is the model for an empty one; the owner id (in the summary, which the MP1 copy
    // would cover) stays the file's own.
    int model = -1;
    for (int i = 0; i < int(kFiles) && model < 0; ++i) {
      if (i != file && FileUsed(d, i)) {
        model = i;
      }
    }
    if (model >= 0) {
      std::memcpy(entry, d + kEntry + 7 * size_t(model), 7);
    } else {
      static constexpr uint8_t kNew[7] = {1, 1, 0, 0, 1, 0, 0}; // a real PAL save's
      std::memcpy(entry, kNew, 7);
    }
    std::memset(slot, 0, kSlotSize);
  }
  std::memcpy(slot, payload.partA, kPartSize);
  std::memcpy(slot + kPartSize, payload.partB, kPartSize);

  // The summary line: difficulty, a u32 (9 for Echoes), the item percentage, the play time (a
  // double), the energy tanks, a count of 3 and three bytes behind it, a u32, the world, and two
  // trailing bytes. The bytes of unknown meaning stay as the file had them.
  uint8_t* m = sum + kSummaryEchoes;
  const bool had = HasEchoes(d, file);
  uint8_t keep[5] = {0, 1, 1, 0, 1};
  uint32_t second = 9;
  if (had) {
    std::memcpy(keep, m + 28, 3);
    std::memcpy(keep + 3, m + 39, 2);
    second = Be32(m + 4);
  }
  std::memset(m, 0, kSummaryBytes);
  PutBe32(m, payload.summary.difficulty);
  PutBe32(m + 4, second);
  PutBe32(m + 8, payload.summary.percent);
  PutBe64(m + 12, FromDouble(payload.summary.playTime));
  PutBe32(m + 20, payload.summary.tanks);
  PutBe32(m + 24, 3);
  std::memcpy(m + 28, keep, 3);
  PutBe32(m + 35, payload.summary.mlvl);
  std::memcpy(m + 39, keep + 3, 2);

  if (Be32(d + kHash) == 0) {
    PutBe32(d + kHash, kHashValue);
  }
  PutBe32(d, Checksum(d));
  return true;
}

} // namespace PortSave::Trilogy
