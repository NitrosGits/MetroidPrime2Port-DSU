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

namespace PortEfbCopy {

// Asks for the CPU half of the `GXCopyTex` the game is about to issue: the next copy into `dest`
// will also be read back into the port's staging buffer, `width` x `height` pixels in the copy's
// own texel format. `texelBytes` is that format's size in bytes (1 for A8/R8/I8, 2 for RGB565).
void Request(void* dest, int width, int height, int texelBytes);

// Copies the most recent completed read-back for `dest` into it, if one is there and newer than
// the last one consumed. `width`/`height` must be the ones `Request` was given.
void Fill(void* dest, int width, int height);

} // namespace PortEfbCopy

#endif
