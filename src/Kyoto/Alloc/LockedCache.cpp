#include "Kyoto/Alloc/LockedCache.hpp"

// Guessed descriptive names for the target's temporary locked-cache allocator state.
#ifdef TARGET_PC
// The GC locked cache (0xe0000000) does not exist on the host; back it with a plain buffer.
alignas(32) static uchar sHostLockedCache[0x4000];
static uchar* sAllocationBase = sHostLockedCache;
#else
static uchar* sAllocationBase = reinterpret_cast< uchar* >(0xe0000000);
#endif
static void* sAllocations[16];
static uint sAllocatedBytes;
static uint sAllocationCount;
static bool sOutOfOrder;

void* AllocateLockedCache4(uint size) {
  const uint next = sAllocatedBytes + ((size + 3) & ~3u);
  if (next <= 0x4000) {
    void* ptr = sAllocationBase + sAllocatedBytes;
    sAllocatedBytes = next;
    if (sAllocationCount < 16) {
      sAllocations[sAllocationCount] = ptr;
    } else {
      sOutOfOrder = true;
    }
    ++sAllocationCount;
    return ptr;
  }
  return nullptr;
}

void* AllocateLockedCache32(uint size) {
  const uint next = sAllocatedBytes + ((size + 31) & ~31u);
  if (next <= 0x4000) {
    void* ptr = sAllocationBase + sAllocatedBytes;
    sAllocatedBytes = next;
    if (sAllocationCount < 16) {
      sAllocations[sAllocationCount] = ptr;
    } else {
      sOutOfOrder = true;
    }
    ++sAllocationCount;
    return ptr;
  }
  return nullptr;
}

void FreeLockedCache(void* ptr) {
  --sAllocationCount;
  if (!sOutOfOrder) {
    if (sAllocations[sAllocationCount] == ptr) {
      sAllocatedBytes = static_cast< uchar* >(ptr) - sAllocationBase;
    } else {
      sOutOfOrder = true;
    }
  }

  if (sAllocationCount == 0) {
    sAllocatedBytes = 0;
    sOutOfOrder = false;
  }
}

void* GetLockedCacheAllocationBase() { return sAllocationBase; }

#ifdef TARGET_PC
#include "rstl/locked_cache_allocator.hpp"

// Upstream declares this but has no definition yet. Host objects are larger than retail ones,
// so the 0x4000-byte cache fills sooner: always fall back to the heap rather than return null.
void rstl::locked_cache_allocator::Allocate(void*& out, int size) {
  mPreviousHeapAllocation = mHeapAllocation;
  out = mPreferHeap ? nullptr : AllocateLockedCache32(size);
  mHeapAllocation = out == nullptr;
  if (mHeapAllocation) {
    out = CMemory::Alloc(size);
  }
  ++mAllocationCount;
}
#endif
