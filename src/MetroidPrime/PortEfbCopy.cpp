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

// One staging buffer and one destination buffer per copy identity: the game's shared buffer, the
// copy's size and its texel size. The game's copies all land in the same spare buffer and the scan
// visor copies the whole viewport as A8 while the scan window copies its pane as RGB565 in the same
// frame, so with the identity reduced to that shared pointer - which is what this did - one
// consumer's `Fill` consumed the other's read-back, and, worse, Aurora keys the texture it resolves
// a copy into by the destination pointer, so both consumers sampled whichever copy was recorded
// last. The visor's palette indices were the pane's bytes on some frames: the highlight colours on
// scannables flashed, and the pane showed noise.
//
// `Destination` is what makes the identity real: a consumer asks it for a per-copy buffer, issues
// its `GXCopyTex` into that buffer and binds its texture from it, so Aurora's resolved copy texture
// is keyed per consumer and the read-back lands somewhere only that consumer reads. The buffers
// keep their capacity for the life of the slot, so nothing is reallocated while the render thread
// may still be writing a read-back into it.
// Room for every visor flare's occlusion test (one small slot each) besides the visor and filters.
constexpr int kSlotCount = 32;

struct Slot {
  void* shared = nullptr; // the game's own buffer the consumer passed
  void* dest = nullptr;   // this copy's private destination, the pointer GXCopyTex and the texture use
  int width = 0;
  int height = 0;
  int texelBytes = 0;
  // Filled by Aurora's render thread, read here on the game thread. `sequence` is the seqlock
  // efb_copy.hpp describes: odd while the bytes are being written, even and increased by two once
  // a whole copy is there. It must never be read or written except through std::atomic_ref.
  uint32_t sequence = 0;
  uint32_t consumedSequence = 0;
  uint32_t lastUsed = 0;
  std::vector< unsigned char > staging;
  std::vector< unsigned char > destination;

  bool used() const { return !staging.empty(); }

  size_t bytes() const {
    return static_cast< size_t >(width) * static_cast< size_t >(height) *
           static_cast< size_t >(texelBytes);
  }

  bool matchesShared(void* s, int w, int h, int texels) const {
    return used() && shared == s && width == w && height == h && texelBytes == texels;
  }

  bool matchesDestination(void* d) const { return used() && dest == d; }

  uint32_t sequenceValue() const {
    return std::atomic_ref< uint32_t >(const_cast< uint32_t& >(sequence))
        .load(std::memory_order_acquire);
  }
};

Slot sSlots[kSlotCount];
uint32_t sUseCounter = 0;
uint32_t sRequests = 0;
uint32_t sFills = 0;
uint32_t sTornReads = 0;
uint32_t sDropped = 0;
uint32_t sRecycled = 0;
bool sLoggedFirstFill = false;

bool DebugEfbCopy() {
  static const bool enabled = std::getenv("MP2_DEBUG_EFB_COPY") != nullptr;
  return enabled;
}

Slot* FindByShared(void* shared, int width, int height, int texelBytes) {
  for (Slot& slot : sSlots) {
    if (slot.matchesShared(shared, width, height, texelBytes)) {
      return &slot;
    }
  }
  return nullptr;
}

Slot* FindByDestination(void* dest) {
  for (Slot& slot : sSlots) {
    if (slot.matchesDestination(dest)) {
      return &slot;
    }
  }
  return nullptr;
}

// The slot for one copy identity, claiming a free pair of buffers or recycling the least recently
// used one. A slot the render thread is writing into is never recycled: its sequence is odd while
// the bytes are in flight.
Slot* AcquireSlot(void* shared, int width, int height, int texelBytes, bool privateDestination) {
  if (width <= 0 || height <= 0 || texelBytes <= 0) {
    return nullptr;
  }
  const size_t needed = static_cast< size_t >(width) * height * texelBytes;
  if (Slot* existing = FindByShared(shared, width, height, texelBytes)) {
    existing->lastUsed = ++sUseCounter;
    return existing;
  }
  Slot* freeSlot = nullptr;
  for (Slot& slot : sSlots) {
    if (!slot.used()) {
      freeSlot = &slot;
      break;
    }
  }
  if (freeSlot == nullptr) {
    for (Slot& slot : sSlots) {
      if ((slot.sequenceValue() & 1u) != 0) {
        continue; // a read-back is being written into it
      }
      if (freeSlot == nullptr || slot.lastUsed < freeSlot->lastUsed) {
        freeSlot = &slot;
      }
    }
    if (freeSlot != nullptr) {
      ++sRecycled;
    }
  }
  if (freeSlot == nullptr) {
    return nullptr;
  }
  // Exact sizes, and an identity's buffers never change size once it exists, so nothing is
  // reallocated while the render thread may still be writing a read-back into it.
  freeSlot->staging.assign(needed, 0);
  if (privateDestination) {
    freeSlot->destination.assign(needed, 0);
    freeSlot->dest = freeSlot->destination.data();
  } else {
    // A copy whose call site has not been given a private destination (the camera filters, the gun,
    // the particles). It keeps the console's behaviour - the bytes land in the game's own buffer -
    // and is keyed by that buffer plus its size, which is still one slot per copy rather than one
    // per buffer.
    freeSlot->destination.clear();
    freeSlot->dest = shared;
  }
  freeSlot->shared = shared;
  freeSlot->width = width;
  freeSlot->height = height;
  freeSlot->texelBytes = texelBytes;
  freeSlot->sequence = 0;
  freeSlot->consumedSequence = 0;
  freeSlot->lastUsed = ++sUseCounter;
  return freeSlot;
}

} // namespace

void* Destination(void* shared, int width, int height, int texelBytes) {
  Slot* slot = AcquireSlot(shared, width, height, texelBytes, true);
  return slot != nullptr ? slot->dest : shared;
}

void Request(void* dest, int width, int height, int texelBytes) {
  // MP2_EFB_COPY=0 turns the read-back off, which is the A/B for anything inside it.
  static const bool enabled = std::getenv("MP2_EFB_COPY") == nullptr ||
                              std::strcmp(std::getenv("MP2_EFB_COPY"), "0") != 0;
  if (!enabled || dest == nullptr) {
    return;
  }
  Slot* slot = FindByDestination(dest);
  if (slot == nullptr) {
    slot = AcquireSlot(dest, width, height, texelBytes, false);
  }
  if (slot == nullptr) {
    if (sDropped++ % 120 == 0) {
      std::fprintf(stderr, "[efb-copy] no staging slot for %dx%d at %p (%u dropped)\n", width,
                   height, dest, sDropped);
    }
    return;
  }
  ++sRequests;
  GXRequestEfbCpuCopy(slot->staging.data(), &slot->sequence,
                      static_cast< uint32_t >(slot->staging.size()));
}

void Fill(void* dest, int width, int height, int texelBytes) {
  (void)width;
  (void)height;
  (void)texelBytes;
  Slot* slot = FindByDestination(dest);
  if (slot == nullptr) {
    return;
  }

  std::atomic_ref< uint32_t > sequence(slot->sequence);
  const uint32_t before = sequence.load(std::memory_order_acquire);
  if ((before & 1u) != 0 || before == 0 || before == slot->consumedSequence) {
    // Either the render thread is mid-write, no copy has landed yet, or this snapshot is the one
    // already in the buffer. The destination holds this copy's own last bytes either way, so there
    // is nothing to do: nothing else writes it.
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
    std::fprintf(stderr,
                 "[efb-copy] %u fills from %u requests (%dx%d tx=%d into %p), %u recycled slots\n",
                 sFills, sRequests, width, height, texelBytes, dest, sRecycled);
  }
}

} // namespace PortEfbCopy
