#ifndef METROID_PRIME_PORT_PORTEFBCOPY_H
#define METROID_PRIME_PORT_PORTEFBCOPY_H

// Port-only: the CPU side of the game's EFB-into-main-memory copies.
//
// The console's `GXCopyTex(dest, clear)` copies a region of the framebuffer into main memory at
// `dest`, in the format the preceding `GXSetTexCopyDst` selected. The game relies on that twice
// over: it uploads those bytes as a texture (the scan visor's C8 palette indices, the camera
// filters' half-size RGB565 backdrop, the scan window) and, in a couple of places, reads them as
// data. Aurora resolves a copy into a *texture* instead, so `dest` was never written on the host:
// the spare buffer kept whatever the allocator left there, which the scan visor then sampled as
// palette indices - the wrong highlight colours the phone showed - and which the port zeroed as a
// stopgap so the indices at least stayed inside the palette.
//
// This is the port's half of the replacement: `Request` mirrors one `GXCopyTex` with a CPU
// read-back (through `GXRequestEfbCpuCopy`, the same encoder-task path the scan-target peek uses,
// so the bytes land a frame or two later), and `Fill` drops the latest completed copy into the
// game's buffer just before the game uploads it as a texture. Both run on the game thread; the
// render thread writes the staging buffer under a seqlock, so a reader never sees half a copy.
//
// Call `Request` immediately before the `GXCopyTex` it mirrors - the read-back takes the copy's
// source and destination geometry from the GX copy state that is current at that point - and
// `Fill` immediately before the texture upload of the same buffer.
//
// **A copy is identified by `dest`, its size and its texel size, and each identity owns its own
// staging buffer.** The game's copies all land in the same spare buffer, and the scan visor and the
// scan window copy *different* sizes into it in the same frame (796x448 A8 for the visor's palette
// indices, 208x92 RGB565 for the window's pane). Keying a staging slot by `dest` alone - which is
// what this did - let one consumer's `Fill` consume the other's completed read-back, so the
// visor's palette indices were the pane's bytes on some frames and stale on others: the highlight
// colours on scannables flashed, and the pane showed noise. Two read-backs land per frame, so the
// alternation is per-frame, which is exactly how it looked.

namespace PortEfbCopy {

// The destination a copy should use instead of the game's shared buffer: one stable buffer per copy
// identity (the shared pointer, the size, the texel size), so a consumer's `GXCopyTex`, its
// `Request`/`Fill` and its texture object all name memory no other consumer writes, and Aurora -
// which resolves a copy into a texture keyed by that destination pointer - resolves this copy and
// not whichever copy into the shared buffer came last. Call it once per copy, then use the returned
// pointer everywhere the console used the shared buffer. Falls back to `shared` when no port
// buffer is free.
void* Destination(void* shared, int width, int height, int texelBytes);

// Asks for the CPU half of the `GXCopyTex` the game is about to issue: the next copy into `dest`
// (a pointer `Destination` returned) will also be read back into the port's staging buffer,
// `width` x `height` pixels at `texelBytes` per texel (1 for A8/R8/I8, 2 for RGB565).
void Request(void* dest, int width, int height, int texelBytes);

// Copies the most recent completed read-back into `dest` (a pointer `Destination` returned), if one
// is there and newer than the last one consumed.
void Fill(void* dest, int width, int height, int texelBytes);

} // namespace PortEfbCopy

#endif
