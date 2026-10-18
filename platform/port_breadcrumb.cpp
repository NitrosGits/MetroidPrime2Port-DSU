#include "port_breadcrumb.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#if defined(_WIN32)
#include <io.h>
#define PORT_WRITE _write
#else
#include <unistd.h>
#define PORT_WRITE write
#endif

namespace PortBreadcrumb {
namespace {

// A fixed ring: entries are written in place under a spin lock, and a dump reads them in order.
// Nothing here allocates, which is what makes it usable from a faulting process.
char sLines[kEntries][kLine];
// Monotonic: the slot is total % kEntries, and everything older than kEntries has been overwritten.
std::atomic< unsigned > sTotal{0};
std::atomic_flag sLock = ATOMIC_FLAG_INIT;

void Lock() {
  while (sLock.test_and_set(std::memory_order_acquire)) {
  }
}

void Unlock() { sLock.clear(std::memory_order_release); }

void WriteRaw(const char* text, size_t size) {
  if (size == 0) {
    return;
  }
  PORT_WRITE(2, text, static_cast< unsigned >(size));
}

} // namespace

void Add(const char* format, ...) {
  if (format == nullptr) {
    return;
  }
  char buffer[kLine];
  va_list args;
  va_start(args, format);
  const int used = std::vsnprintf(buffer, sizeof buffer, format, args);
  va_end(args);
  if (used <= 0) {
    return;
  }
  Lock();
  const unsigned slot = sTotal.load(std::memory_order_relaxed) % unsigned(kEntries);
  std::memcpy(sLines[slot], buffer, sizeof buffer);
  sTotal.fetch_add(1, std::memory_order_relaxed);
  Unlock();
}

void Dump() {
  const unsigned total = sTotal.load(std::memory_order_relaxed);
  if (total == 0) {
    return;
  }
  const int count = total < unsigned(kEntries) ? int(total) : kEntries;
  const int first = total < unsigned(kEntries) ? 0 : int(total % unsigned(kEntries));
  Lock();
  char header[64];
  const int headerSize = std::snprintf(header, sizeof header, "[crumb] last %d action(s):\n", count);
  if (headerSize > 0) {
    WriteRaw(header, size_t(headerSize));
  }
  for (int i = 0; i < count; ++i) {
    const char* line = sLines[(first + i) % kEntries];
    WriteRaw("[crumb] ", 8);
    WriteRaw(line, std::strlen(line));
    WriteRaw("\n", 1);
  }
  Unlock();
}

unsigned Count() { return sTotal.load(std::memory_order_relaxed); }

void Clear() {
  Lock();
  sTotal.store(0, std::memory_order_relaxed);
  Unlock();
}

} // namespace PortBreadcrumb
