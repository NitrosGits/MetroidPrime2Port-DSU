#include "gx.hpp"
#include "__gx.h"

#include "../../gfx/depth_peek.hpp"

#include <dolphin/gx/GXAurora.h>
#include <dolphin/gx/GXCpu2Efb.h>

void GXPeekZ(u16 x, u16 y, u32* z) {
  if (z != nullptr) {
    u32 value = 0;
    if (aurora::gfx::depth_peek::read_latest(x, y, value)) {
      *z = value;
    } else {
      *z = 0;
    }
  }

  GX_WRITE_AURORA(GX_AURORA_REQUEST_DEPTH_SNAPSHOT);
}

void GXSnapshotDepth(void) { GX_WRITE_AURORA(GX_AURORA_SNAPSHOT_DEPTH); }

GXBool GXPeekZRegion(u16 x, u16 y, u16 width, u16 height, u32* z, u32 maxAgeMs) {
  if (z == nullptr || width == 0 || height == 0) {
    return GX_FALSE;
  }
  return aurora::gfx::depth_peek::read_region(x, y, width, height, z, maxAgeMs) ? GX_TRUE : GX_FALSE;
}

void GXRequestScanPeek(s32 left, s32 top, u32 width, u32 height, void* idOut, void* depthHighOut,
                       void* depthLowOut, u32* sequence) {
  GX_WRITE_AURORA(GX_AURORA_REQUEST_SCAN_PEEK);
  GX_WRITE_U32(static_cast<u32>(left));
  GX_WRITE_U32(static_cast<u32>(top));
  GX_WRITE_U32(width);
  GX_WRITE_U32(height);
  GX_WRITE_U64(reinterpret_cast<u64>(idOut));
  GX_WRITE_U64(reinterpret_cast<u64>(depthHighOut));
  GX_WRITE_U64(reinterpret_cast<u64>(depthLowOut));
  GX_WRITE_U64(reinterpret_cast<u64>(sequence));
}

void GXRequestEfbCpuCopy(void* staging, u32* sequence, u32 capacity) {
  GX_WRITE_AURORA(GX_AURORA_REQUEST_EFB_COPY);
  GX_WRITE_U64(reinterpret_cast<u64>(staging));
  GX_WRITE_U64(reinterpret_cast<u64>(sequence));
  GX_WRITE_U32(capacity);
  GX_WRITE_U32(0); // padding: keep the command stream 64-bit aligned
}
