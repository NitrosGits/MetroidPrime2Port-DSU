#pragma once

#include "types.hpp"

namespace aurora::gfx::efb_copy {

// One CPU read-back of an EFB copy that the game asked for with a main-memory destination
// (GXCopyTex with a CPU pointer). Aurora resolves such a copy into a texture - that is what every
// consumer that samples it as a texture uses - but on the console the same copy also wrote the
// bytes to main memory, and some of the game's consumers upload those bytes as a *new* texture or
// read them as data. This is the missing half, for the copies the port opts into (`GXRequestEfbCpuCopy`,
// issued just before the `GXCopyTex` it mirrors).
//
// The read-back source is the copy's own resolve target, i.e. exactly the pixels and the size the
// texture path already produced, resampled to the copy's logical destination size (the size the
// game uploads at) with a nearest-neighbour pick and converted to the copy's texel format on the
// CPU.
//
// The write happens on whichever thread completes the WebGPU buffer mapping, so `sequenceOut`
// follows a seqlock: it is odd while the bytes are being written, and even and increased by two
// once a whole copy is there. A reader must re-read it after copying the bytes and discard the
// copy if it changed. `out` is caller-owned and must stay alive; it is written with
// `width * height * bytes_per_texel(format)` bytes.
struct Request {
  void* out = nullptr;
  uint32_t* sequenceOut = nullptr;
  uint32_t capacity = 0;  // bytes available at `out`
  uint32_t width = 0;     // the copy's logical destination size, in pixels
  uint32_t height = 0;
  GXTexFmt format = GX_TF_RGBA8;  // the copy's texel format
};

// True for the copy formats the CPU side knows how to write into the game's buffer.
bool has_cpu_format(GXTexFmt format) noexcept;

// Bytes one output pixel of `format` takes when the console writes that copy to main memory.
uint32_t bytes_per_texel(GXTexFmt format) noexcept;

// Copies `source` (the copy's resolve target) to the caller's staging buffer once the frame being
// encoded has been submitted. Called from the frame encoder, after that pass's resolve.
void encode(const wgpu::CommandEncoder& cmd, const TextureHandle& source, const Request& req) noexcept;

void after_submit() noexcept;
void shutdown() noexcept;

} // namespace aurora::gfx::efb_copy
