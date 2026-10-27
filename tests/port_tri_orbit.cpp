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

} // namespace

int main() {
  TestExtents();
  TestPointerCentre();
  return PORT_TEST_RESULT();
}
