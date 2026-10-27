// Port-only: the default-font half of retail `main` (src/MetroidPrime/Startup.cpp).
//
// Startup.cpp stays out of the host build because it is the console process entry (`main`,
// `CSaveRegion`, `CRelDebugSupport`); platform/main.cpp is the host one. But the font it loads
// before `InvokeCMain` is what `gpDefaultFont` points at, and `CSplashScreen::Draw` and
// `CErrorOutputWindow` dereference that. The two blobs (`sDefaultFontData`, `sDefaultFontTexture`,
// LZO-compressed) are retail bytes, so the host binary does not embed them: they are read at run
// time out of the mounted disc's boot DOL (`DVDGetDOLLocation`: the partition DOL, or
// `rs5mp2_p.dol` on a Trilogy disc). The DOL is searched for a 16-byte prefix of each blob and the
// candidate is accepted only when its SHA-256 matches, so no per-version address is hard-coded and
// USA, PAL and Trilogy all resolve. Only prefixes, sizes and hashes live here. A missing blob
// leaves `gpDefaultFont` null (logged); its users test for that under TARGET_PC.
#include "Kyoto/Graphics/CTexture.hpp"
#include "Kyoto/Streams/CLZOInputStream.hpp"
#include "Kyoto/Streams/CMemoryInStream.hpp"
#include "Kyoto/Text/CRasterFont.hpp"
#include "MetroidPrime/CGameGlobalObjects.hpp"

#include <dolphin/dvd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

const TToken< CRasterFont >* gpDefaultFont;

namespace port {

namespace {

struct Sha256 {
  uint32_t h[8];
  uint8_t buf[64];
  uint64_t total;
  size_t fill;
};

constexpr uint32_t kK[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

uint32_t Rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

void Sha256Block(Sha256& s, const uint8_t* p) {
  uint32_t w[64];
  for (int i = 0; i < 16; ++i) {
    w[i] = (uint32_t(p[i * 4]) << 24) | (uint32_t(p[i * 4 + 1]) << 16) |
           (uint32_t(p[i * 4 + 2]) << 8) | uint32_t(p[i * 4 + 3]);
  }
  for (int i = 16; i < 64; ++i) {
    const uint32_t s0 = Rotr(w[i - 15], 7) ^ Rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
    const uint32_t s1 = Rotr(w[i - 2], 17) ^ Rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }
  uint32_t v[8];
  memcpy(v, s.h, sizeof(v));
  for (int i = 0; i < 64; ++i) {
    const uint32_t t1 = v[7] + (Rotr(v[4], 6) ^ Rotr(v[4], 11) ^ Rotr(v[4], 25)) +
                        ((v[4] & v[5]) ^ (~v[4] & v[6])) + kK[i] + w[i];
    const uint32_t t2 = (Rotr(v[0], 2) ^ Rotr(v[0], 13) ^ Rotr(v[0], 22)) +
                        ((v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]));
    v[7] = v[6]; v[6] = v[5]; v[5] = v[4]; v[4] = v[3] + t1;
    v[3] = v[2]; v[2] = v[1]; v[1] = v[0]; v[0] = t1 + t2;
  }
  for (int i = 0; i < 8; ++i) {
    s.h[i] += v[i];
  }
}

void Sha256Digest(const uint8_t* data, size_t len, uint8_t out[32]) {
  Sha256 s = {{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab,
               0x5be0cd19},
              {},
              0,
              0};
  size_t i = 0;
  for (; i + 64 <= len; i += 64) {
    Sha256Block(s, data + i);
  }
  uint8_t tail[128] = {};
  const size_t rem = len - i;
  memcpy(tail, data + i, rem);
  tail[rem] = 0x80;
  const size_t tailLen = rem < 56 ? 64 : 128;
  const uint64_t bits = uint64_t(len) * 8;
  for (int b = 0; b < 8; ++b) {
    tail[tailLen - 1 - b] = uint8_t(bits >> (8 * b));
  }
  for (size_t o = 0; o < tailLen; o += 64) {
    Sha256Block(s, tail + o);
  }
  for (int k = 0; k < 8; ++k) {
    for (int b = 0; b < 4; ++b) {
      out[k * 4 + b] = uint8_t(s.h[k] >> (24 - 8 * b));
    }
  }
}

// What `FindBlob` looks for: the first bytes of the compressed blob, its size and the SHA-256 of
// all of it (retail G2ME01 sDefaultFontData 0x803AB1C0 / sDefaultFontTexture 0x803ABBF0; the bytes
// are identical in G2MP01). The decompression parameter is the retail one from `main`.
struct BlobSpec {
  const char* name;
  uint8_t prefix[16];
  uint32_t size;
  uint8_t sha256[32];
  uint32_t decompressedSize;
};

const BlobSpec kFontData = {
    "sDefaultFontData",
    {0x0a, 0x2c, 0x19, 0x46, 0x4f, 0x4e, 0x54, 0x00, 0x00, 0x00, 0x04, 0x4d, 0x00, 0x0f, 0x4d, 0x00},
    0xA2E,
    {0xc5, 0x6f, 0x5d, 0xc0, 0x92, 0xe1, 0x80, 0xe9, 0xd4, 0x0c, 0xa6, 0x8e, 0x8a, 0x8d, 0x11, 0xed,
     0xcb, 0xc4, 0x3b, 0xb5, 0x4e, 0x8e, 0x00, 0x3e, 0xd0, 0xa2, 0x49, 0xe0, 0x50, 0x9d, 0x25, 0x5d},
    0x1660};
const BlobSpec kFontTexture = {
    "sDefaultFontTexture",
    {0x07, 0xbc, 0x19, 0x00, 0x00, 0x00, 0x04, 0x00, 0x40, 0x00, 0x50, 0x5d, 0x00, 0x01, 0x4d, 0x00},
    0x7BE,
    {0xd6, 0xae, 0x91, 0x7d, 0x38, 0x9d, 0x2d, 0x84, 0xec, 0xd1, 0xa2, 0xcf, 0xcc, 0x40, 0x0b, 0x56,
     0x96, 0x32, 0x71, 0xfc, 0x22, 0xc5, 0x83, 0xaf, 0x0d, 0x35, 0x16, 0x53, 0xf4, 0x58, 0x9f, 0x15},
    0xa34};

// Scans the whole DOL for the prefix and accepts the first candidate whose hash matches.
bool FindBlob(const uint8_t* dol, size_t dolSize, const BlobSpec& spec, std::vector< uint8_t >& out) {
  for (size_t i = 0; i + spec.size <= dolSize; ++i) {
    if (dol[i] != spec.prefix[0] || memcmp(dol + i, spec.prefix, sizeof(spec.prefix)) != 0) {
      continue;
    }
    uint8_t digest[32];
    Sha256Digest(dol + i, spec.size, digest);
    if (memcmp(digest, spec.sha256, sizeof(digest)) == 0) {
      out.assign(dol + i, dol + i + spec.size);
      printf("port: default font: %s found in the disc's DOL at file offset 0x%zx\n", spec.name, i);
      return true;
    }
  }
  printf("port: default font: ERROR %s not found in the disc's DOL (no prefix+SHA-256 match); "
         "text that needs the default font is skipped\n",
         spec.name);
  return false;
}

// Blobs must outlive the streams (CMemoryInStream does not copy).
std::vector< uint8_t > sFontDataBytes;
std::vector< uint8_t > sFontTextureBytes;

} // namespace

// Same body as Startup.cpp's `LoadDefaultFont`, over the blobs read from the disc.
static CRasterFont* LoadDefaultFontObject() {
  CLZOInputStream fontStream(
      rs_new CMemoryInStream(sFontDataBytes.data(), u32(sFontDataBytes.size())),
      u32(sFontDataBytes.size()), kFontData.decompressedSize);
  CRasterFont* font = rs_new CRasterFont(fontStream, nullptr);
  CLZOInputStream textureStream(
      rs_new CMemoryInStream(sFontTextureBytes.data(), u32(sFontTextureBytes.size())),
      u32(sFontTextureBytes.size()), kFontTexture.decompressedSize);
  TToken< CTexture > texture(
      rs_new CTexture(textureStream, CTexture::kAM_Zero, CTexture::kBK_Zero));
  font->SetTexture(texture);
  return font;
}

// Retail keeps the token as a local of `main` that outlives `InvokeCMain`; the caller owns the
// returned token the same way and must free it before `CGraphicsSys` goes down. The handle is
// opaque so platform/main.cpp need not see the game's template headers.
void* LoadDefaultFont() {
  s32 dolSize = 0;
  const uint8_t* dol = DVDGetDOLLocation(&dolSize);
  if (dol == nullptr || dolSize <= 0) {
    printf("port: default font: ERROR the mounted disc's DOL is unreadable; "
           "text that needs the default font is skipped\n");
    return nullptr;
  }
  if (!FindBlob(dol, size_t(dolSize), kFontData, sFontDataBytes) ||
      !FindBlob(dol, size_t(dolSize), kFontTexture, sFontTextureBytes)) {
    return nullptr;
  }
  printf("port: default font: loaded from the disc's DOL\n");
  fflush(stdout);
  TToken< CRasterFont >* token = new TToken< CRasterFont >(LoadDefaultFontObject());
  gpDefaultFont = token;
  return token;
}

void FreeDefaultFont(void* token) {
  gpDefaultFont = nullptr;
  if (token == nullptr) {
    return;
  }
  delete static_cast< TToken< CRasterFont >* >(token);
}

} // namespace port
