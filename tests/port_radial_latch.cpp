#include "port_radial.h"
#include "port_test.h"
#include <cstdio>

using namespace PortRadial;

// Hold-delay and eligibility gate for keyboard/pad buttons (Latch, GateButtons).
int main() {
  const bool ok[2] = {true, true};
  const bool noBeam[2] = {false, true};
  {
    Latch l;
    // Below 0.175 s the button is not latched; past it, it is.
    REQUIRE(!l.Tick(true, false, 10.00, ok) && l.active == -1);
    l.Tick(true, false, 10.17, ok);
    REQUIRE(l.active == -1);
    l.Tick(true, false, 10.18, ok);
    REQUIRE(l.Latched(kBeam));
    // Release unlatches; a tap shorter than the delay never latches.
    l.Tick(false, false, 10.20, ok);
    REQUIRE(l.active == -1);
    l.Tick(true, false, 11.00, ok);
    l.Tick(false, false, 11.10, ok);
    l.Tick(true, false, 11.15, ok);
    l.Tick(true, false, 11.30, ok);
    REQUIRE(l.active == -1); // the delay restarts at the new press (11.15)
    l.Tick(true, false, 11.34, ok);
    REQUIRE(l.Latched(kBeam));
  }
  {
    // Ineligible: never latches while held; becoming ineligible drops a latch and reports it.
    Latch l;
    l.Tick(true, true, 0., noBeam);
    REQUIRE(!l.Tick(true, true, 1., noBeam));
    REQUIRE(l.Latched(kVisor)); // visor (eligible) latches, beam does not
    REQUIRE(!l.Latched(kBeam));
    const bool none[2] = {false, false};
    REQUIRE(l.Tick(true, true, 1.1, none));
    REQUIRE(l.active == -1);
  }
  {
    // GateButtons through the real transport: Poll sees nothing until latched and eligible.
    Cancel();
    Transport().SetAvailable(true);
    SetEligible(true, true);
    bool b = true, v = false;
    GateButtons(b, v, 100.0);
    REQUIRE(!b);
    Poll(b, v, 0, 0);
    REQUIRE(!Capturing());
    b = true; GateButtons(b, v, 100.2);
    REQUIRE(b);
    Poll(b, v, 0, 0);
    REQUIRE(Capturing() && Transport().Held(kBeam));
    // Losing eligibility mid-hold cancels the wheel without selecting.
    SetEligible(false, true);
    b = true; GateButtons(b, v, 100.3);
    REQUIRE(!b);
    REQUIRE(!Capturing());
    // Scan visor-style gate: beam ineligible, visor eligible.
    SetEligible(false, true);
    b = false; v = true; GateButtons(b, v, 200.0);
    v = true; GateButtons(b, v, 200.2);
    REQUIRE(v && !b);
    Poll(b, v, 0, 0);
    REQUIRE(Transport().Held(kVisor));
    // Centre release still selects (Resolve picks slot 0 = current -> no change, but resolves).
    v = false; GateButtons(b, v, 200.4);
    Poll(b, v, 0, 0);
    REQUIRE(Capturing() && !Transport().Held(kVisor));
  }
  std::puts("port_radial_latch_tests OK");
  return 0;
}
