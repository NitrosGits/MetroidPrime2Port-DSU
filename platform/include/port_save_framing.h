#pragma once

#include "Kyoto/CCrc32.hpp"

// Only the framing words are endian-dependent. The payload is already serialized
// in console order; never byte-swap the whole block (including its CRC input).
namespace PortSave::Framing {

inline uint Read32(const uchar* p, bool little = false) {
  if (little) {
    return uint(p[0]) | (uint(p[1]) << 8) | (uint(p[2]) << 16) | (uint(p[3]) << 24);
  }
  return (uint(p[0]) << 24) | (uint(p[1]) << 16) | (uint(p[2]) << 8) | uint(p[3]);
}

inline void Write32(uchar* p, uint value) {
  p[0] = uchar(value >> 24);
  p[1] = uchar(value >> 16);
  p[2] = uchar(value >> 8);
  p[3] = uchar(value);
}

inline bool CheckBlockCrc(const uchar* block, uint size) {
  if (size < 4) {
    return false;
  }
  const uint crc = CCRC32::Calculate(block + 4, size - 4, 0xFFFFFFFF);
  return Read32(block) == crc || Read32(block, true) == crc;
}

inline bool ReadSlot(const uchar* block, uint size, uint& generation) {
  if (size < 8) {
    return false;
  }
  const uint crc = CCRC32::Calculate(block + 4, size - 4, 0xFFFFFFFF);
  // Generation is a toggle, not a counter. Reject other values before the
  // caller XORs two generations and uses the result as a two-element index.
  for (int little = 0; little != 2; ++little) {
    const uint value = Read32(block + 4, little);
    if (Read32(block, little) == crc && value <= 1) {
      generation = value;
      return true;
    }
  }
  return false;
}

inline void FinishBlock(uchar* block, uint size) {
  Write32(block, CCRC32::Calculate(block + 4, size - 4, 0xFFFFFFFF));
}

inline void FinishSlot(uchar* block, uint size, uint generation) {
  Write32(block + 4, generation);
  FinishBlock(block, size);
}

} // namespace PortSave::Framing
