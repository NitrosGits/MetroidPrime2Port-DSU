#pragma once

#include <cstddef>

// A crash breadcrumb: the port records what it was doing into a small ring in memory, and the
// fault handler dumps it before re-raising. The point is that a crash on a phone names its own last
// actions in logcat (and in metroid_prime2_port.log), instead of the next session rebuilding with
// prints to find out which texture was freed or which resource was missing.
//
// Costs and rules: Add formats into a fixed buffer under a spin lock - no allocation, no stdio, no
// file access - so it is safe on the game thread, the render thread and inside a signal handler's
// window. The ring is fixed at compile time (kEntries * kLine bytes); dump gets what is there, not
// what has been overwritten.
namespace PortBreadcrumb {

constexpr int kEntries = 48;
constexpr size_t kLine = 160;

// printf-style. Keep it to one short line: the ring overwrites, so old entries go.
void Add(const char* format, ...) __attribute__((format(printf, 1, 2)));
// Dumps the ring oldest-first with direct write(2) calls: call it from the fault handler, before
// re-raising, so the lines are in logcat and in the log file.
void Dump();
// The number of entries recorded since the process started (for tests and for "was it recording").
unsigned Count();
void Clear();

} // namespace PortBreadcrumb
