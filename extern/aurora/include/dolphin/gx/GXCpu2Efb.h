#ifndef _DOLPHIN_GX_GXCPU2EFB_H_
#define _DOLPHIN_GX_GXCPU2EFB_H_

#include "GXEnum.h"

void GXPokeAlphaMode(GXCompare func, u8 threshold);
void GXPokeAlphaRead(GXAlphaReadMode mode);
void GXPokeAlphaUpdate(GXBool update_enable);
void GXPokeBlendMode(GXBlendMode type, GXBlendFactor src_factor, GXBlendFactor dst_factor, GXLogicOp op);
void GXPokeColorUpdate(GXBool update_enable);
void GXPokeDstAlpha(GXBool enable, u8 alpha);
void GXPokeDither(GXBool dither);
void GXPokeZMode(GXBool compare_enable, GXCompare func, GXBool update_enable);
void GXPeekARGB(u16 x, u16 y, u32* color);
void GXPokeARGB(u16 x, u16 y, u32 color);
void GXPeekZ(u16 x, u16 y, u32* z);
void GXPokeZ(u16 x, u16 y, u32 z);
// Aurora-only: fills the id and depth buffers of a `width` x `height` region of the framebuffer
// as the (left, top, width, height) EFB copy would have, `height` frames after the atomic pointer
// `sequence` changes from even to its next even value. See lib/gfx/scan_peek.cpp.
void GXRequestScanPeek(s32 left, s32 top, u32 width, u32 height, void* idOut, void* depthHighOut,
                       void* depthLowOut, u32* sequence);
// Aurora-only: writes the bytes the next `GXCopyTex` would have copied to its main-memory
// destination into `staging`, converting them to the copy's texel format, a frame or two later.
// `sequence` is the seqlock (odd while the bytes are being written). Use the current copy state
// (GXSetTexCopySrc/GXSetTexCopyDst) for the geometry, as the copy itself does. See
// lib/gfx/efb_copy.cpp.
void GXRequestEfbCpuCopy(void* staging, u32* sequence, u32 capacity);
// Aurora-only: snapshot the framebuffer depth as it is now (one pass break, no copy), for
// GXPeekZRegion a frame or two later. Does nothing offscreen or with MSAA.
void GXSnapshotDepth(void);
// Aurora-only: the z24 values of a `width` x `height` region (render-mode framebuffer pixels,
// top-down, row-major) of the latest GXSnapshotDepth or GXPeekZ snapshot. GX_FALSE when there is
// none younger than `maxAgeMs` or the region is outside it.
GXBool GXPeekZRegion(u16 x, u16 y, u16 width, u16 height, u32* z, u32 maxAgeMs);
u32 GXCompressZ16(u32 z24, GXZFmt16 zfmt);
u32 GXDecompressZ16(u32 z16, GXZFmt16 zfmt);


#endif // _DOLPHIN_GX_GXCPU2EFB_H_
