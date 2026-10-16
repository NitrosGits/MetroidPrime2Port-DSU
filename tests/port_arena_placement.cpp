// Where the REL arena lands, on the path Android takes. bionic defines
// MAP_FIXED_NOREPLACE but not MAP_32BIT, so Android's GuestArena fallback scans
// /proc/self/maps for a free gap below 4 GiB instead of asking the kernel for a
// 32-bit-addressable mapping. That fallback is selected on a host that *does*
// have MAP_32BIT by -DMP_ARENA_FORCE_PORTABLE=1 (the hook exists for this file),
// which is what makes the Android placement testable on Linux:
//
// The `port_arena_placement_tests` target in CMakeLists.txt builds this file with
// platform/rel.cpp and -DMP_ARENA_FORCE_PORTABLE=1, which is what makes the Android
// placement testable on Linux. Standalone, the same thing is:
//
//   g++ -std=c++20 -DMP_ARENA_FORCE_PORTABLE=1 -Iplatform/include tests/port_arena_placement.cpp
//       platform/rel.cpp -o /tmp/opencode/port_arena_placement
//
// The check this file cannot make for itself is that the *preferred* base is what
// the arena reports: (b) below is that check, on the address the real loader asks
// for (0x81000000, the console's module region).

#include "port_rel.h"

#include <cstdint>
#include <cstdio>

namespace {
using namespace port::rel;

int g_failures = 0;

void Check(bool condition, const char* what) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    g_failures++;
  }
}

bool InArena(const GuestArena& arena, const void* ptr, size_t size) {
  const auto address = reinterpret_cast<uintptr_t>(ptr);
  const auto base = reinterpret_cast<uintptr_t>(arena.Base());
  return address >= base && address + size <= base + arena.Size();
}
} // namespace

int main() {
#if !defined(MP_ARENA_FORCE_PORTABLE)
  std::fprintf(stderr,
               "port_arena_placement: build with -DMP_ARENA_FORCE_PORTABLE=1; without it this "
               "host uses MAP_32BIT and the Android fallback is not compiled in\n");
  return 2;
#else
  // (a) No preferred base, a size the scan has to find a gap for: the arena must
  // come out valid and below 4 GiB, or the REL runtime cannot encode a guest
  // address in a relocation at all.
  {
    GuestArena arena(32u << 20, 0);
    Check(arena.Valid(), "32 MiB arena with no preferred base is placed");
    if (arena.Valid()) {
      Check(reinterpret_cast<uintptr_t>(arena.Base()) + arena.Size() <= 0x100000000ull,
            "32 MiB arena is below 4 GiB");
      Check(reinterpret_cast<uintptr_t>(arena.Base()) % (1u << 20) == 0,
            "32 MiB arena starts 1 MiB-aligned");
      // Not an assertion: which of the two fallbacks placed it. 0x10000000 is the
      // hint, anything else came from the /proc/self/maps scan.
      std::printf("port_arena_placement: fallback placed 32 MiB at %p (%s)\n", arena.Base(),
                  reinterpret_cast<uintptr_t>(arena.Base()) == 0x10000000ull
                      ? "hinted mmap"
                      : "gap scan");
    }
  }

  // (b) A preferred base that is free: it must be honoured exactly, because the
  // module region's absolute addresses are what the REL branch range was chosen
  // for.
  {
    GuestArena arena(1 << 16, 0x81000000);
    Check(arena.Valid(), "arena at the preferred base is placed");
    if (arena.Valid()) {
      Check(reinterpret_cast<uintptr_t>(arena.Base()) == 0x81000000ull,
            "arena is placed exactly at its preferred base 0x81000000");
    }
  }

  // (c) The bump allocator's contract: aligned, inside the arena, and null when
  // the alignment is not a power of two.
  {
    GuestArena arena(1 << 16, 0x81000000);
    Check(arena.Valid(), "arena for the allocation checks is placed");
    if (arena.Valid()) {
      void* first = arena.Allocate(64, 32);
      Check(first != nullptr, "first allocation succeeds");
      Check(first != nullptr && reinterpret_cast<uintptr_t>(first) % 32 == 0,
            "first allocation is 32-byte aligned");
      Check(first != nullptr && InArena(arena, first, 64), "first allocation is inside the arena");

      void* second = arena.Allocate(96, 1);
      Check(second != nullptr, "second allocation succeeds");
      Check(second != nullptr && InArena(arena, second, 96), "second allocation is inside the arena");
      Check(second == nullptr || static_cast<uint8_t*>(second) >= static_cast<uint8_t*>(first) + 64,
            "allocations do not overlap");

      Check(arena.Allocate(64, 24) == nullptr, "a non-power-of-two alignment is refused");
      Check(arena.Allocate(1u << 20, 1u << 20) == nullptr,
            "an allocation larger than the arena is refused");
    }
  }

  // (d) Nothing fits: the arena stays invalid and says why, rather than handing
  // out a base whose guest addresses would not fit in the relocation encoding.
  {
    GuestArena arena(4ull << 30, 0);
    Check(!arena.Valid(), "an arena larger than the 4 GiB guest space is refused");
  }

  if (g_failures != 0) {
    std::fprintf(stderr, "port_arena_placement: %d checks failed\n", g_failures);
    return 1;
  }
  std::printf("port_arena_placement: all checks passed\n");
  return 0;
#endif
}
