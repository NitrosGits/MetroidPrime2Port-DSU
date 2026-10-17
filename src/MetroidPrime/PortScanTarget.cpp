// Port-only: fills the scan visor's target buffers from a CPU read-back of the framebuffer.
// See include/MetroidPrime/PortScanTarget.hpp for why, and extern/aurora/lib/gfx/scan_peek.cpp
// for the read-back itself.

#include "MetroidPrime/PortScanTarget.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <dolphin/gx/GXCpu2Efb.h>

namespace PortScanTarget {
namespace {

// The game's capture is 32x64 pixels (kScanTargetTextureSize) and it reads the buffers back as
// height * width entries, so the staging copy is sized for exactly that.
constexpr uint32_t kMaxPixels = 32 * 64;

// Filled by Aurora's render thread when a read-back completes, read here on the game thread.
// `sequence` is the seqlock scan_peek.hpp describes: it is odd while the arrays are being
// written, and even and increased by two once a whole snapshot is there. It must never be read or
// written except through std::atomic_ref.
struct Staging {
  uint32_t sequence;
  uint8_t id[kMaxPixels];
  uint16_t depthHigh[kMaxPixels];
  uint8_t depthLow[kMaxPixels];
};

Staging sStaging;
uint32_t sConsumedSequence = 0;
uint32_t sTornReads = 0;
bool sLoggedFirstFill = false;
int sLastLoggedVisor = -1;
int sLastLoggedFrom = -1;

bool DebugScan() {
  static const bool enabled = std::getenv("MP2_DEBUG_SCAN") != nullptr;
  return enabled;
}

} // namespace

void Request(int left, int top, int width, int height) {
  // MP2_SCAN_PEEK=0 turns the read-back off, which is the A/B for a crash inside it: the scan
  // visor then behaves exactly as it did before this port work (no target can be selected).
  static const bool enabled = std::getenv("MP2_SCAN_PEEK") == nullptr ||
                              std::strcmp(std::getenv("MP2_SCAN_PEEK"), "0") != 0;
  if (!enabled || width <= 0 || height <= 0) {
    return;
  }
  if (static_cast<uint32_t>(width) * static_cast<uint32_t>(height) > kMaxPixels) {
    return;
  }
  GXRequestScanPeek(left, top, static_cast<uint32_t>(width), static_cast<uint32_t>(height), sStaging.id,
                    sStaging.depthHigh, sStaging.depthLow, &sStaging.sequence);
}

void FillBuffers(void* idData, void* depthHighData, void* depthLowData, int width, int height) {
  if (idData == nullptr || depthHighData == nullptr || depthLowData == nullptr || width <= 0 || height <= 0) {
    return;
  }
  const uint32_t pixels = static_cast<uint32_t>(width) * static_cast<uint32_t>(height);
  if (pixels > kMaxPixels) {
    return;
  }

  std::atomic_ref<uint32_t> sequence(sStaging.sequence);
  const uint32_t before = sequence.load(std::memory_order_acquire);
  if ((before & 1u) != 0 || before == sConsumedSequence) {
    // Either the render thread is mid-write or this snapshot is the one already in the buffers.
    return;
  }

  std::memcpy(idData, sStaging.id, pixels);
  std::memcpy(depthHighData, sStaging.depthHigh, pixels * sizeof(uint16_t));
  std::memcpy(depthLowData, sStaging.depthLow, pixels);

  const uint32_t after = sequence.load(std::memory_order_acquire);
  if (after != before) {
    // Torn: the buffers now hold a mix of two frames, which the histogram in FindScanTargetId
    // reads as a slightly noisy selection for one frame. Report the rate, not every one.
    if (sTornReads++ % 64 == 0) {
      std::fprintf(stderr, "[scan] read-back torn %u times (sequence %u -> %u)\n", sTornReads, before, after);
    }
    return;
  }
  sConsumedSequence = before;

  if (!sLoggedFirstFill) {
    sLoggedFirstFill = true;
    std::fprintf(stderr, "[scan] first scan-target read-back landed (%dx%d, sequence %u)\n", width, height,
                 before);
  }
}

void NoteVisorSelection(int currentVisor, int selectedVisor, bool requested) {
  if (!DebugScan() || selectedVisor == currentVisor) {
    return;
  }
  if (selectedVisor == sLastLoggedVisor && currentVisor == sLastLoggedFrom) {
    return;
  }
  sLastLoggedFrom = currentVisor;
  sLastLoggedVisor = selectedVisor;
  std::fprintf(stderr, "[scan] visor selection: current %d -> selected %d (direct mapping seen: %d)\n",
               currentVisor, selectedVisor, requested ? 1 : 0);
}

} // namespace PortScanTarget
