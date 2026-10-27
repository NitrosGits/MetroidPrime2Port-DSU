#include "port_tri_reticle.h"
#include "port_test.h"

using namespace PortTriReticle;
static bool Near(float a, float b) { return a > b - .00001f && a < b + .00001f; }
int main() {
  for (int state = 0; state < 6; ++state) {
    CHECK_EQ(AimVisible(state), state == 0 || state == 2 || state == 3);
    CHECK_EQ(LockVisible(state), state == 0);
  }
  CHECK(Near(ArmResponse(0), 1)); CHECK(Near(ArmResponse(10), 1));
  CHECK(Near(ArmResponse(20), .75f)); CHECK(Near(ArmResponse(30), .5f)); CHECK(Near(ArmResponse(99), .5f));
  CHECK(Near(CentreResponse(5), 1)); CHECK(Near(CentreResponse(20), .8f));
  CHECK(Near(CentreResponse(30), .6f)); CHECK(Near(CentreResponse(99), .6f));
  float v = 0;
  for (int i = 0; i < 45; ++i) v = ScaleStep(v, 1.f);
  CHECK(Near(v, 1.f));
  CHECK(Near(ScaleStep(.5f, .5f + 1.f / 90.f), .5f + 1.f / 90.f));
  CHECK(Near(ScaleStep(0.f, -1.f), 0.f));
  CHECK(Near(FadeStep(0.f, true, .125f, .25f, .125f), .5f));
  CHECK(Near(FadeStep(1.f, false, .125f, .25f, .125f), 0.f));
  CHECK(Near(FadeStep(.9f, true, 1.f, .25f, .125f), 1.f));
  CHECK(Near(LockBaseScale(0.f), 3.f)); CHECK(Near(LockBaseScale(.25f), .35f)); CHECK(Near(LockBaseScale(1.f), .35f));
  CHECK(Near(LockBracketAlpha(.25f), 0.f)); CHECK(Near(LockBracketAlpha(.5f), 50.f / 255.f));
  CHECK(Near(LockTechTime(.1f), 0.f)); CHECK(Near(LockTechTime(.5f), .5f > .496f ? .496f : .5f));
  CHECK(!LockTechSpins(.4f)); CHECK(LockTechSpins(.5f));
  CHECK(Near(LockArmAlpha(.5f, .1f, .016f), 200.f / 255.f));
  CHECK(Near(LockArmAlpha(200.f / 255.f, 1.f, 100.f), .35f));
  CHECK(Near(ScanSegment(0.f, 5), 0.f)); CHECK(Near(ScanSegment(1.f, 0), 1.f));
  CHECK(Near(ScanSegment(1.f / 6.f, 5), 1.f)); CHECK(Near(ScanSegment(1.f / 6.f, 4), 0.f));
  CHECK(Near(ScanSegment(.5f, 3), 1.f)); CHECK(Near(ScanSegment(.5f, 2), 0.f));
  return 0;
}
