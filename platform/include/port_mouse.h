#pragma once

#include <cmath>
#include <cstdint>

namespace PortMouse {
constexpr float kMaxPitch = 1.52f; // approximately 87 degrees, short of the pole
constexpr float kPi = 3.14159265358979323846f;

struct Planar {
  float right;
  float forward;
};
inline Planar ClampPlanar(Planar value, float maximum) {
  const float length = std::hypot(value.right, value.forward);
  if (!std::isfinite(length) || maximum <= 0.f) return {0.f, 0.f};
  if (length > maximum) {
    const float scale = maximum / length;
    value.right *= scale;
    value.forward *= scale;
  }
  return value;
}

// The retail forward-force law applied to either horizontal axis. Damping and
// collision integration remain in the player; this is not teleport movement.
inline float AxisForce(float input, float velocity, float maxSpeed, float friction,
                       float mass, float dt, float acceleration) {
  if (input == 0.f || maxSpeed <= 0.f || dt <= 0.f || acceleration <= 0.f) return 0.f;
  const float frictionSpeed = friction * mass * maxSpeed / (dt * acceleration);
  const float desired = input * (maxSpeed - frictionSpeed) + (input > 0.f ? frictionSpeed : -frictionSpeed);
  const float fraction = (desired - velocity) / maxSpeed;
  return (fraction < -1.f ? -1.f : fraction > 1.f ? 1.f : fraction) * acceleration;
}

struct AimState {
  float yaw = 0.f;
  float pitch = 0.f;
  bool initialized = false;

  void Reset() { initialized = false; }

  void Synchronize(float x, float y, float z) {
    const float length = std::sqrt(x * x + y * y + z * z);
    if (!std::isfinite(length) || length < 0.00001f) {
      Reset();
      return;
    }
    // A target directly overhead has no horizontal heading. Retain the last
    // heading rather than snapping to atan2(0, 0) when that lock is released.
    if (x * x + y * y > length * length * 0.00000001f) yaw = std::atan2(-x, y);
    const float up = z / length;
    pitch = ClampPitch(std::asin(up < -1.f ? -1.f : up > 1.f ? 1.f : up));
    initialized = true;
  }

  // A locked camera owns orientation. Track its effective direction, but never
  // consume hidden mouse movement or rotate the player's body underneath it.
  bool Update(bool active, bool locked, float x, float y, float z,
              float dx, float dy, float sensitivity, bool invertX, bool invertY) {
    if (!active) {
      Reset();
      return false;
    }
    if (!initialized || locked) Synchronize(x, y, z);
    if (!initialized || locked) return false;
    if (!std::isfinite(dx) || !std::isfinite(dy) ||
        !std::isfinite(sensitivity) || sensitivity <= 0.f) return true;
    // SDL motion is right/down positive; world +pitch looks upward.
    const double nextYaw = yaw + double(dx) * sensitivity * (invertX ? 1.0 : -1.0);
    const double nextPitch = pitch + double(dy) * sensitivity * (invertY ? 1.0 : -1.0);
    yaw = static_cast<float>(std::remainder(nextYaw, 2.0 * kPi));
    pitch = ClampPitch(nextPitch);
    return true;
  }

private:
  static float ClampPitch(double angle) {
    return static_cast<float>(angle < -kMaxPitch ? -kMaxPitch : angle > kMaxPitch ? kMaxPitch : angle);
  }
};

class ButtonGate {
public:
  void Reset() { mReady = false; }
  uint32_t Poll(bool enabled, uint32_t held) {
    if (!enabled) {
      Reset();
      return 0;
    }
    // A UI click or a button held while acquiring capture is not a new shot.
    // Once neutral has been observed, preserve held states for charge/release.
    if (!mReady) {
      mReady = held == 0;
      return 0;
    }
    return held;
  }
private:
  bool mReady = false;
};

// Mouse look. The platform owns the aim angles and SDL's relative-mouse capture; the game
// reads them from CPlayer (which turns the body to Yaw()) and CFirstPersonCamera (which uses
// the angles for the view). Off unless MP2_MOUSE_AIM=1, so a pad or the keyboard alone stays
// the default. MP2_MOUSE_SENS sets radians per pixel and MP2_MOUSE_INVERT_X/_Y flip an axis.
bool Enabled();
void SetEnabled(bool enabled);
// Twin-stick: the game feeds the right stick here through AddStickAim, so the stick aims
// through the same state as the mouse. Works with or without mouse aim.
bool StickEnabled();
void SetStickEnabled(bool enabled);
float StickAimRate();
void SetStickAimRate(float pixelsPerSecond);
// Separate vertical speed: with the split on, the stick's up and down use StickAimRateY instead.
bool StickAimSplit();
void SetStickAimSplit(bool enabled);
float StickAimRateY();
void SetStickAimRateY(float pixelsPerSecond);
// Feeds a normalised right-stick vector (-1..1, x right, y up) for one tick.
void AddStickAim(float x, float y, float dt);
// Touch aim: the Android overlay's drag-to-aim, a third source alongside the mouse and the
// stick. `PortTouch` decides when it is on (its own MP2_TOUCH_AIM switch, or the active
// layout) and calls SetTouchEnabled; like twin-stick it works with or without mouse aim. The
// Java UI thread calls AddTouchAim/SetTouchAimDown/SpringSwipe, so those are safe from either
// thread and the game thread only ever sees a whole tick's travel.
bool TouchEnabled();
void SetTouchEnabled(bool enabled);
// Finger travel in density-independent pixels (right/down positive), scaled here by
// TouchAimSpeed() because the UI thread has no view of the aim's units.
void AddTouchAim(float dxDp, float dyDp);
// A finger is down on the aim area: this is what makes the touch aim own the view at all
// (see TouchAimActive). Lifting without SpringSwipe stops the aim where the finger left.
void SetTouchAimDown(bool down);
// A quick flick that lifted: keep the aim "down" for a short hold afterwards, so a flick
// turns the view by its whole travel instead of only the part seen while the finger was down.
void SpringSwipe();
// A finger is down, or a spring hold is live, and the source is on: the per-frame answer to
// "does the touch own the view right now". False while the overlay sits idle, which is most
// of a run.
bool TouchAimActive();
// Aim pixels per dp, the Prime port's `touch_aim_speed` (2.25 by default). Clamped to
// 0.25..10; MP2_TOUCH_AIM_SPEED overrides it the lazy way MP2_MOUSE_SENS does.
float TouchAimSpeed();
void SetTouchAimSpeed(float pixelsPerDp);
// The debug overlay owns the cursor while it is open: no capture, no aim applied.
void SetUiActive(bool active);
bool UiActive();
// Relative motion as it arrives from SDL's event queue, and the once-per-loop latch that
// turns the pending deltas into this tick's deltas.
void AddMotion(float dx, float dy);
void BeginFrame();
// Called every tick, from CPlayer::UpdateMouseAim. `active` means a pointer source may own the
// aim right now (first person, not morphed, not in a cinematic, not locked on); `locked` keeps
// the aim glued to the camera direction (x, y, z) instead of turning the player under it.
// An enabled touch source with no finger down counts as locked until TouchAimActive(), so the
// aim tracks the camera the rest of the time without turning the player.
// Returns true when the caller should rotate the player to Yaw().
bool UpdateAim(bool active, bool locked, float x, float y, float z);
void Synchronize(float x, float y, float z);
void Reset();
bool Aiming();
bool Initialized();
float Yaw();
float Pitch();
float Sensitivity();
void SetSensitivity(float radiansPerPixel);
bool InvertX();
bool InvertY();
void SetInvertX(bool invert);
void SetInvertY(bool invert);
} // namespace PortMouse
