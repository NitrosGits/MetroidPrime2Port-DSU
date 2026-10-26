#include "port_radial.h"
#include "port_input_engine.h"
#include "port_test.h"
#include <cmath>
#include <cstdio>

using namespace PortRadial;
// Actual Echoes serialized X/Z positions, enum order (not MP1 slots).
const Point beam[] = {{0,.6f}, {-3.961959f,-2.037475f}, {3.961959f,-2.037475f}, {0,5.088521f}};
const Point visor[] = {{0,.6f}, {3.961959f,-2.037475f}, {0,5.088521f}, {-3.961959f,-2.037475f}};

void Select(State& s, Kind kind, const Point* positions, int item, unsigned owned, int current) {
  s.SetAvailable(true);
  s.Tick(false, false, 0, 0);
  s.Tick(kind == kBeam, kind == kVisor, 0, 0);
  REQUIRE(s.Capturing() && s.Held(kind));
  const float x = positions[item].x - positions[0].x;
  const float y = positions[item].y - positions[0].y;
  s.SetSelection(x, y);
  REQUIRE(s.Selection(positions) == item);
  s.Animate(.1f);
  REQUIRE(s.Open(kind) == 1.f);
  s.Tick(false, false, 0, 0);
  REQUIRE(s.Capturing() && !s.Held(kind));
  s.Resolve(positions, owned, current);
  const int expected = item != current && (owned & (1u << item)) ? item : -1;
  REQUIRE(s.TakePick(kind) == expected);
  REQUIRE(s.TakePick(kind) == -1);
  s.Resolve(positions, owned, current); // no second release
  REQUIRE(s.TakePick(kind) == -1);
  REQUIRE(s.Capturing()); // consumed release still suppresses pad/aim/presentation for this tick
  s.Tick(false, false, 0, 0);
  REQUIRE(!s.Capturing());
  s.Animate(.1f);
  REQUIRE(s.Open(kind) == 0.f);
}
int main() {
  static_assert(int(PortInput::Action::BallUnmorph) == 70); // last pre-radial persisted ID
  static_assert(int(PortInput::Action::BeamMenu) == 71);
  static_assert(int(PortInput::Action::VisorMenu) == 72);
  static_assert(int(PortInput::Action::BeamMenu) == int(PortInput::Action::BallUnmorph) + 1);
  static_assert(int(PortInput::Action::VisorMenu) == int(PortInput::Action::BeamMenu) + 1);
  for (int k = 0; k < 2; ++k) for (int i = 0; i < 4; ++i) {
    State s;
    Select(s, Kind(k), k == 0 ? beam : visor, i, 15, (i + 1) % 4);
    Select(s, Kind(k), k == 0 ? beam : visor, i, 15, i); // current is no-op, including center
    Select(s, Kind(k), k == 0 ? beam : visor, i, 15 & ~(1u << i), (i + 1) % 4);
  }
  State s;
  s.Tick(true, true, 1, 0); // unavailable assets: no capture
  REQUIRE(!s.Capturing());
  s.SetAvailable(true);
  s.Tick(true, true, 0, 1); // no spontaneous open while button still down
  REQUIRE(!s.Capturing());
  s.Tick(false, false, 0, 0);
  s.Tick(true, true, 0, 1);
  REQUIRE(s.Active() == kBeam); // deterministic conflict
  s.Tick(true, false, 1, 0);
  REQUIRE(s.Active() == kBeam);
  s.Cancel(); // pause/F1/cine/death/morph/world change all use cancellation, never Resolve
  REQUIRE(!s.Capturing() && s.TakePick(kBeam) == -1);
  s.Tick(true, true, 0, 1);
  REQUIRE(s.Active() == kVisor); // newly pressed other wheel is allowed
  s.SetAvailable(false); // disable or teardown also drops pending picks
  s.Resolve(visor, 15, 0);
  REQUIRE(!s.Capturing() && s.TakePick(kVisor) == -1);
  s.SetAvailable(true);
  s.Tick(false, false, 0, 0);
  s.Tick(true, false, 0, 0);
  s.AddTravel(0, -200);
  REQUIRE(s.Selection(beam) == 3);
  s.AddTravel(NAN, INFINITY); // invalid transport cannot poison selection
  REQUIRE(s.Selection(beam) == 3);
  s.SetSelection(0, 0);
  REQUIRE(s.Selection(beam) == 0); // center release selects Power, not cancel
  s.Tick(false, false, 0, 0);
  s.Resolve(beam, 15, 1);
  s.Cancel(); // resolved-but-not-consumed cancellation discards mailbox
  REQUIRE(s.TakePick(kBeam) == -1 && !s.Capturing());
  s.Tick(true, false, 0, 1);
  s.Tick(false, false, 0, 1);
  s.Resolve(beam, 15, 0);
  REQUIRE(s.TakePick(kBeam) == 3);
  s.Tick(false, false, 0, 1);
  REQUIRE(s.Capturing() && s.TakePick(kBeam) == -1); // held stick cannot leak a second swap
  s.Tick(false, false, 0, 0);
  REQUIRE(!s.Capturing());
  State conflict;
  conflict.SetAvailable(true);
  conflict.Tick(true, false, 0, 0);
  conflict.Animate(.1f);
  REQUIRE(conflict.Open(kBeam) == 1.f);
  conflict.Tick(false, false, 0, 0);
  conflict.Resolve(beam, 15, 0);
  conflict.TakePick(kBeam);
  conflict.Tick(false, true, 0, 0);
  REQUIRE(conflict.Active() == kVisor && conflict.Open(kBeam) == 0.f);
  std::puts("port_radial: owned/all slots, center/current, release once, teardown, capture/animation OK");
}
