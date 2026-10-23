#pragma once

#include "types.hpp"

#include <vector>

namespace aurora::gfx::depth_peek {

void initialize();
void shutdown();

bool enabled() noexcept;
void request_snapshot() noexcept;
// Whether a GXPeekZ is waiting for a snapshot; a frame recorded without one needs no depth stored.
bool snapshot_wanted() noexcept;
bool read_latest(uint16_t x, uint16_t y, uint32_t& z) noexcept;
// Copies a w x h region of the latest snapshot (z24 per texel, row-major) into `out`. False when
// there is none, it is older than `maxAgeMs`, or the region is outside it.
bool read_region(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t* out, uint32_t maxAgeMs) noexcept;

// `forced`: snapshot regardless of a request and of the 30 Hz throttle.
void encode_frame_snapshot(const wgpu::CommandEncoder& cmd, const wgpu::TextureView& depthView,
                           wgpu::Extent3D sourceSize, uint32_t msaaSamples, bool forced = false) noexcept;
void after_submit() noexcept;

namespace testing {
void reset() noexcept;
bool snapshot_requested() noexcept;
void set_latest(uint32_t width, uint32_t height, const std::vector<uint32_t>& data);
} // namespace testing

} // namespace aurora::gfx::depth_peek
