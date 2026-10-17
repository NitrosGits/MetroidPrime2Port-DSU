// Port-only: fills a main-memory EFB copy destination from a CPU read-back of the framebuffer.
// See include/MetroidPrime/PortEfbCopy.hpp for why, and extern/aurora/lib/gfx/efb_copy.cpp for the
// read-back itself.

#include "MetroidPrime/PortEfbCopy.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <dolphin/gx/GXCpu2Efb.h>

namespace PortEfbCopy {
namespace {

// One staging buffer per copy destination. The port's copies all land in the same spare buffer and
// their size changes as the scan window animates, so a slot is keyed by `dest` alone and its
// staging buffer is allocated once at a fixed capacity: reallocating it while the render thread may
// still be writing a read-back into it would be a use-after-free.
constexpr int kSlotCount = 4;
constexpr size_t kStagingCapacity = 1u << 20; // 1 MiB: covers 800x600 of 16-bit copy texels

struct Slot {
  void* dest = nullptr;
  int width = 0;
  int height = 0;
  int texelBytes = 0;
  // Filled by Aurora's render thread, read here on the game thread. `sequence` is the seqlock
  // efb_copy.hpp describes: odd while the bytes are being written, even and increased by two once
  // a whole copy is there. It must never be read or written except through std::atomic_ref.
  uint32_t sequence = 0;
  uint32_t consumedSequence = 0;
  std::vector< unsigned char > staging;

  size_t bytes() const {
    return static_cast< size_t >(width) * static_cast< size_t >(height) *
           static_cast< size_t >(texelBytes);
  }
};

Slot sSlots[kSlotCount];
uint32_t sRequests = 0;
uint32_t sFills = 0;
uint32_t sTornReads = 0;
uint32_t sDropped = 0;
bool sLoggedFirstFill = false;

bool DebugEfbCopy() {
  static const bool enabled = std::getenv("MP2_DEBUG_EFB_COPY") != nullptr;
  return enabled;
}

Slot* FindSlot(void* dest, int width, int height) {
  for (Slot& slot : sSlots) {
    if (slot.dest == dest && slot.width == width && slot.height == height && !slot.staging.empty()) {
      return &slot;
    }
  }
  return nullptr;
}

// The slot for one copy destination, allocated on first use. Its staging buffer keeps its capacity
// for the lifetime of the slot, so a size change never reallocates memory the render thread may be
// writing into.
Slot* AcquireSlot(void* dest, int width, int height, int texelBytes) {
  if (width <= 0 || height <= 0 || texelBytes <= 0) {
    return nullptr;
  }
  const size_t needed = static_cast< size_t >(width) * height * texelBytes;
  if (needed > kStagingCapacity) {
    return nullptr;
  }
  for (Slot& slot : sSlots) {
    if (slot.dest != dest || slot.staging.empty()) {
      continue;
    }
    slot.width = width;
    slot.height = height;
    slot.texelBytes = texelBytes;
    return &slot;
  }
  for (Slot& slot : sSlots) {
    if (!slot.staging.empty()) {
      continue;
    }
    slot.dest = dest;
    slot.width = width;
    slot.height = height;
    slot.texelBytes = texelBytes;
    slot.sequence = 0;
    slot.consumedSequence = 0;
    slot.staging.assign(kStagingCapacity, 0);
    return &slot;
  }
  return nullptr;
}

} // namespace

void Request(void* dest, int width, int height, int texelBytes) {
  // MP2_EFB_COPY=0 turns the read-back off, which is the A/B for anything inside it.
  static const bool enabled = std::getenv("MP2_EFB_COPY") == nullptr ||
                              std::strcmp(std::getenv("MP2_EFB_COPY"), "0") != 0;
  if (!enabled || dest == nullptr) {
    return;
  }
  Slot* slot = AcquireSlot(dest, width, height, texelBytes);
  if (slot == nullptr) {
    if (sDropped++ % 120 == 0) {
      std::fprintf(stderr, "[efb-copy] no staging slot free for %dx%d into %p (%u dropped)\n", width,
                   height, dest, sDropped);
    }
    return;
  }
  ++sRequests;
  GXRequestEfbCpuCopy(slot->staging.data(), &slot->sequence,
                      static_cast< uint32_t >(slot->staging.size()));
}

void Fill(void* dest, int width, int height) {
  Slot* slot = FindSlot(dest, width, height);
  if (slot == nullptr) {
    return;
  }

  std::atomic_ref< uint32_t > sequence(slot->sequence);
  const uint32_t before = sequence.load(std::memory_order_acquire);
  if ((before & 1u) != 0 || before == 0 || before == slot->consumedSequence) {
    // Either the render thread is mid-write, no copy has landed yet, or this snapshot is the one
    // already in the buffer.
    return;
  }

  const size_t bytes = slot->bytes();
  std::memcpy(dest, slot->staging.data(), bytes);

  const uint32_t after = sequence.load(std::memory_order_acquire);
  if (after != before) {
    // Torn: the buffer now holds a mix of two frames. Report the rate, not every one.
    if (sTornReads++ % 64 == 0) {
      std::fprintf(stderr, "[efb-copy] read-back torn %u times (sequence %u -> %u)\n", sTornReads,
                   before, after);
    }
    return;
  }
  slot->consumedSequence = before;
  ++sFills;

  if (!sLoggedFirstFill) {
    sLoggedFirstFill = true;
    std::fprintf(stderr, "[efb-copy] first spare-buffer copy landed (%dx%d, %zu bytes, sequence %u)\n",
                 width, height, bytes, before);
  }
  if (DebugEfbCopy() && sFills % 120 == 0) {
    std::fprintf(stderr, "[efb-copy] %u fills from %u requests (%dx%d into %p)\n", sFills, sRequests,
                 width, height, dest);
  }
}

} // namespace PortEfbCopy
