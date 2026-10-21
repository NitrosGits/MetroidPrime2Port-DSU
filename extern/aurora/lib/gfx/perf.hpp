#pragma once

// Port addition: per-frame performance counters (see include/aurora/perf.h).

#include <aurora/perf.h>

#include <atomic>
#include <cstdint>

namespace aurora::perf {
extern std::atomic<uint32_t> g_passBreaks[AURORA_PERF_BREAK_COUNT];
extern std::atomic<uint32_t> g_counters[AURORA_PERF_COUNTER_COUNT];

inline void pass_break(int reason) noexcept { g_passBreaks[reason].fetch_add(1, std::memory_order_relaxed); }
inline void count(int counter, uint32_t n = 1) noexcept {
  g_counters[counter].fetch_add(n, std::memory_order_relaxed);
}

// One GXCopyTex from the caller at return address pc (game thread).
void copy_site(const void* pc) noexcept;
// End of a recorded frame (recording thread): publish the frame's totals and reset them.
void end_frame() noexcept;
} // namespace aurora::perf
