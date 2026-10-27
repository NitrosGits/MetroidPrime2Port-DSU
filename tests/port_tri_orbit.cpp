#include "port_tri_orbit.h"
#include "port_test.h"

namespace {

void TestExtents() {
  // Float fmuls then fctiwz: 0.65f * 180 is 116.99999, which truncates to 116.
  CHECK_EQ(PortTriOrbit::HalfHeight(180.f), 116);
  CHECK_EQ(PortTriOrbit::HalfHeight(100.f), 65);
  CHECK_EQ(PortTriOrbit::HalfWidth(200.f), 130);
}

void TestPointerCentre() {
  CHECK_EQ(PortTriOrbit::PointerCentre(0.f, 640.f), 320);
  CHECK_EQ(PortTriOrbit::PointerCentre(0.f, 448.f), 224);
  CHECK_EQ(PortTriOrbit::PointerCentre(-1.f, 640.f), 0);
  CHECK_EQ(PortTriOrbit::PointerCentre(1.f, 640.f), 640);
  CHECK_EQ(PortTriOrbit::PointerCentre(0.5f, 641.f), 480); // 480.75 truncates
}

void TestFade() {
  // Ramps while orbiting, clamps at the duration, decays to 0 otherwise.
  float t = PortTriOrbit::FadeTimerStep(0.f, true, 0.25f, 0.5f);
  CHECK_EQ(t, 0.25f);
  CHECK_EQ(PortTriOrbit::FadeRatio(t, true, 0.5f), 0.5f);
  t = PortTriOrbit::FadeTimerStep(t, true, 1.f, 0.5f);
  CHECK_EQ(t, 0.5f);
  CHECK_EQ(PortTriOrbit::FadeRatio(t, true, 0.5f), 1.f);
  t = PortTriOrbit::FadeTimerStep(t, false, 0.25f, 0.5f);
  CHECK_EQ(t, 0.25f);
  t = PortTriOrbit::FadeTimerStep(t, false, 1.f, 0.5f);
  CHECK_EQ(t, 0.f);
  // A zero duration is an instant lock.
  CHECK_EQ(PortTriOrbit::FadeTimerStep(0.f, true, 1.f, 0.f), 0.f);
  CHECK_EQ(PortTriOrbit::FadeRatio(0.f, true, 0.f), 1.f);
  CHECK_EQ(PortTriOrbit::FadeRatio(0.f, false, 0.f), 0.f);
}

} // namespace

int main() {
  TestFade();
  TestExtents();
  TestPointerCentre();
  return PORT_TEST_RESULT();
}
