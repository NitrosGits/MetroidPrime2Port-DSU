#include "port_input_pad.h"

#include <cstdio>
#include <cstring>

#define REQUIRE(cond)                                                                              \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      std::fprintf(stderr, "%s:%d: REQUIRE failed: %s\n", __FILE__, __LINE__, #cond);              \
      return 1;                                                                                    \
    }                                                                                              \
  } while (0)

namespace {
// Splits "<entry>@..." and returns the pad; `rest` is what is left of the name.
int Split(const char* entry, const char*& rest) {
  rest = entry;
  return PortInputPad::SplitPad(rest, std::strchr(entry, '@'));
}
} // namespace

int main() {
  const char* rest = nullptr;
  REQUIRE(Split("A@10", rest) == 0 && std::strncmp(rest, "A@", 2) == 0);
  REQUIRE(Split("p1.A@10", rest) == 1 && std::strncmp(rest, "A@", 2) == 0);
  REQUIRE(Split("P2:START@5+4", rest) == 2 && std::strncmp(rest, "START@", 6) == 0);
  REQUIRE(Split("p3.SUP@1", rest) == 3 && std::strncmp(rest, "SUP@", 4) == 0);
  REQUIRE(Split("p0.B@1", rest) == 0 && std::strncmp(rest, "B@", 2) == 0);
  // Not a prefix: pad out of range, wrong separator, nothing after it.
  REQUIRE(Split("p4.A@1", rest) == 0 && std::strncmp(rest, "p4.", 3) == 0);
  REQUIRE(Split("p1A@1", rest) == 0 && std::strncmp(rest, "p1A", 3) == 0);
  REQUIRE(Split("p1.@1", rest) == 0 && std::strncmp(rest, "p1.", 3) == 0);
  REQUIRE(Split("PORTAL@1", rest) == 0 && std::strncmp(rest, "PORTAL", 6) == 0);
  return 0;
}
