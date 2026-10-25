#include "port_resume_logic.h"

#include <cstdio>

#define REQUIRE(cond)                                                                              \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      std::fprintf(stderr, "%s:%d: REQUIRE failed: %s\n", __FILE__, __LINE__, #cond);              \
      return 1;                                                                                    \
    }                                                                                              \
  } while (0)

using namespace PortResume;

int main() {
  // Container round trip, and every way a file can be wrong.
  {
    Header h;
    h.fileIdx = 2;
    h.cardSerial = 0x1122334455667788ull;
    h.savedAt = 1760000000;
    const std::string wrapped = WrapContainer(h, "state-bytes");
    Header back;
    std::string state;
    REQUIRE(UnwrapContainer(wrapped, back, state));
    REQUIRE(back.fileIdx == 2 && back.cardSerial == h.cardSerial && back.savedAt == h.savedAt);
    REQUIRE(state == "state-bytes");
    REQUIRE(!UnwrapContainer(wrapped.substr(0, kContainerHeaderSize - 1), back, state));
    std::string bad = wrapped;
    bad[0] = 'X';
    REQUIRE(!UnwrapContainer(bad, back, state));
    bad = wrapped;
    bad[4] = 2; // version
    REQUIRE(!UnwrapContainer(bad, back, state));
    h.fileIdx = 3;
    REQUIRE(!UnwrapContainer(WrapContainer(h, "x"), back, state));
    h.fileIdx = -1;
    REQUIRE(!UnwrapContainer(WrapContainer(h, "x"), back, state));
    h.fileIdx = 0;
    REQUIRE(UnwrapContainer(WrapContainer(h, ""), back, state) && state.empty());
  }
  REQUIRE(Matches(Header{0, 5, 0}, 5) && !Matches(Header{0, 5, 0}, 6));
  // What a closing game writes.
  REQUIRE(ChooseWrite(true, true) == Write::Fresh);
  REQUIRE(ChooseWrite(true, false) == Write::Fresh);
  REQUIRE(ChooseWrite(false, true) == Write::Snapshot);
  REQUIRE(ChooseWrite(false, false) == Write::Nothing);
  // When a snapshot is due.
  REQUIRE(!RefreshDue(false, false, false, 100.0, 100.0));
  REQUIRE(!RefreshDue(true, false, false, 1.9, 0.0));
  REQUIRE(RefreshDue(true, false, false, 2.0, 0.0));
  REQUIRE(RefreshDue(true, true, false, 2.0, 1.0));   // a new room
  REQUIRE(!RefreshDue(true, true, true, 50.0, 29.9)); // same room, too soon
  REQUIRE(RefreshDue(true, true, true, 50.0, 30.0));
  // Crash detection.
  REQUIRE(CrashedAgain(true, ResumedMarker(77), 77));
  REQUIRE(!CrashedAgain(true, ResumedMarker(76), 77));
  REQUIRE(!CrashedAgain(false, ResumedMarker(77), 77));
  REQUIRE(!CrashedAgain(true, "", 77));
  std::puts("port_resume_logic: ok");
  return 0;
}
