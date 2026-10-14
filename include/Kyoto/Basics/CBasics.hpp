#ifndef _CBASICS
#define _CBASICS

#include "types.h"

namespace CBasics {
bool Init();
char* Stringize(const char* fmt, ...);
// Guessed names. Cache-block (dcbz) accelerated fill/copy; G2ME01 0x8028BD3C/0x8028BE1C.
void ZeroMemory(void* dst, uint size);
void CopyMemory(void* dst, const void* src, uint size);
#ifdef TARGET_PC
// Port: game data is big-endian; convert it to host byte order.
inline uint SwapBytes(uint x) { return __builtin_bswap32(x); }
inline int SwapBytes(int x) { return static_cast< int >(__builtin_bswap32(static_cast< uint >(x))); }
inline short SwapBytes(short x) { return static_cast< short >(__builtin_bswap16(static_cast< ushort >(x))); }
inline ushort SwapBytes(ushort x) { return __builtin_bswap16(x); }
inline float SwapBytes(float x) {
  union {
    float f;
    uint u;
  };
  f = x;
  u = __builtin_bswap32(u);
  return f;
}
#else
inline uint SwapBytes(uint x) {
#if 0
  x = ((x << 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x >> 24));
#endif
  return x;
}
inline int SwapBytes(int x) { return x; }
inline short SwapBytes(short x) { return x; }
inline ushort SwapBytes(ushort x) { return x; }
inline float SwapBytes(float x) {
  union {
    float f;
    uint u;
  };
  f = x;
#if 0
  u = SwapBytes(u);
#endif
  return f;
}
#endif
}; // namespace CBasics

#endif // _CBASICS
