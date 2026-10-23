#pragma once

#include "types.hpp"

namespace aurora::gfx::scan_peek {

// One CPU read-back of a region of the current EFB, requested from the FIFO (see
// GXRequestScanPeek) and copied out of the frame that contains the request once that frame has
// been submitted. This is the missing half of GXCopyTex for callers that want the copied bytes
// on the CPU rather than in a texture: Aurora resolves the copy into a texture, so the console's
// CPU-pointer destination is never written.
//
// The write happens on whichever thread completes the WebGPU buffer mapping, so `sequenceOut`
// follows a seqlock: it is odd while the buffers are being written, and even and increased by
// two once a complete snapshot is there. A reader must re-read it after copying the data and
// discard the copy if it changed.
//
// Every output buffer is caller-owned and must stay alive; each is written with one entry per
// output pixel (width x height), resampled with a nearest-neighbour pick from the source region.
struct Request {
  void* idOut = nullptr;        // 1 byte per pixel: the EFB's alpha, where scan ids live
  void* depthHighOut = nullptr; // 2 bytes per pixel, laid out as a Z16 EFB copy writes them
  void* depthLowOut = nullptr;  // 1 byte per pixel: the low 8 bits of the 24-bit z
  uint32_t* sequenceOut = nullptr;
  int32_t srcX = 0;    // region to read, in framebuffer texels
  int32_t srcY = 0;
  int32_t srcWidth = 0;
  int32_t srcHeight = 0;
  uint32_t width = 0;  // pixels the caller wants out
  uint32_t height = 0;
  // Port: GX_SCAN_PEEK_DEPTH_ONLY / GX_SCAN_PEEK_IF_RENDERED (GXCpu2Efb.h).
  bool depthOnly = false;  // no EFB-alpha diagnostics: the caller reads only depth
  bool ifRendered = false; // do not force the frame rendered; dropped on a frame that is not
};

// Copies a snapshot of a region of `color` and `depth` to the buffers in `req`, once the frame
// being encoded has been submitted. Called from the frame encoder, between passes.
void encode(const wgpu::CommandEncoder& cmd, const wgpu::Texture& color, const wgpu::Texture& depth,
            uint32_t msaaSamples, const Request& req) noexcept;

void after_submit() noexcept;
void shutdown() noexcept;

} // namespace aurora::gfx::scan_peek
