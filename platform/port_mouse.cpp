// Mouse look: SDL relative motion feeds the aim state in `port_mouse.h`, which the game reads
// from CPlayer (body yaw) and CFirstPersonCamera (view direction). The Prime port drives the
// same state from its own hooks; this is the part that only needs SDL, so it lives in the
// platform layer and is built by `mp_port_audio`, the target with SDL on its include path.
//
// `MP2_MOUSE_AIM=1` turns it on. Off by default: a pad, or the keyboard map in
// `port_events.cpp`, is the retail-shaped way to play, and mouse aim changes movement and
// turning, so it stays opt-in until it has had more play time.

#include "port_mouse.h"

#include <SDL3/SDL.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>

namespace {

PortMouse::AimState sAim;
bool sEnabled = false;
bool sStickEnabled = false;
// The Java UI thread can flip this through a layout change while the game thread reads it in
// UpdateAim, so it is atomic unlike the mouse's and the stick's flags.
std::atomic<bool> sTouchEnabled{false};
bool sReadEnv = false;
bool sCaptured = false;
bool sAiming = false;
bool sUiActive = false;
float sSensitivity = 0.0035f; // radians per pixel, the Prime port's default
float sStickAimRate = 900.f;  // aim pixels per second at full stick deflection
// MP1's "Separate vertical speed": with the split on, up and down use their own rate.
bool sStickAimSplit = false;
float sStickAimRateY = 900.f;
float sTouchAimSpeed = 2.25f; // aim pixels per dp, the Prime port's touch_aim_speed default
bool sInvertX = false;
bool sInvertY = false;
// The aim's down state is set by the UI thread (a finger lift) and read every tick by the game
// thread, so it is atomic rather than mutex-guarded: it is a flag, not a value to accumulate.
std::atomic<bool> sTouchAimDown{false};
// Set by the first finger-down and kept until Reset: like MP1's direct aim, a drag leaves the
// view where it was aimed instead of handing it back to the free-look pitch (which is level).
std::atomic<bool> sTouchAimLatched{false};
std::atomic<uint64_t> sTouchHoldUntilNs{0};
float sPendingX = 0.f;
float sPendingY = 0.f;
float sFrameX = 0.f;
float sFrameY = 0.f;
// "Gyro also turns the camera" (MP1's sGyroCameraPending): view travel in mouse pixels beside the
// pointer's, pending and latched with it.
float sCameraPendingX = 0.f;
float sCameraPendingY = 0.f;
float sCameraFrameX = 0.f;
float sCameraFrameY = 0.f;
// The stick's part of sPendingX/Y (PresentedAimDelta shows only a tick's fraction of it), and
// whether the last tick applied the aim, with the dt it was given.
float sStickPendingX = 0.f;
float sStickPendingY = 0.f;
bool sAppliedLastTick = false;
float sLastTickDt = 1.f / 60.f;
// Pointer aim (MP1's sPointerAim/sPointerAimFree), configured each tick by CPlayer.
PortMouse::PointerAim sPointer;
bool sPointerEnabled = false;
bool sPointerFree = false;
float sPointerBox = 0.7f;
float sPointerTurnRate = PortMouse::kPi;

// Pending travel is written by the UI thread (AddTouchAim) and latched by the game thread
// (BeginFrame, and the clears), so every read or write of it takes this lock. The mouse's
// AddMotion and the stick's AddStickAim run on the game thread itself, which is also the only
// thread that latches, so those two need no lock of their own. The clear lives here rather
// than at each call site so that SetCapture - which Reset calls - takes the lock on its own.
std::mutex sPendingMutex;

constexpr float kTouchSpringHoldSeconds = 0.35f;

void ClearPendingTravel() {
  std::lock_guard< std::mutex > lock(sPendingMutex);
  sPendingX = sPendingY = sFrameX = sFrameY = 0.f;
  sStickPendingX = sStickPendingY = 0.f;
  sCameraPendingX = sCameraPendingY = sCameraFrameX = sCameraFrameY = 0.f;
}

float ClampTouchAimSpeed(float pixelsPerDp) {
  return pixelsPerDp < 0.25f ? 0.25f : pixelsPerDp > 10.f ? 10.f : pixelsPerDp;
}

bool EnvFlag(const char* name) {
  const char* value = std::getenv(name);
  return value != nullptr && *value != '\0' && std::strcmp(value, "0") != 0;
}

void ReadEnv() {
  if (sReadEnv) {
    return;
  }
  sReadEnv = true;
  sEnabled = EnvFlag("MP2_MOUSE_AIM");
  sStickEnabled = EnvFlag("MP2_TWIN_STICK");
  sInvertX = EnvFlag("MP2_MOUSE_INVERT_X");
  sInvertY = EnvFlag("MP2_MOUSE_INVERT_Y");
  sStickAimSplit = EnvFlag("MP2_STICK_AIM_SPLIT");
  if (const char* sens = std::getenv("MP2_MOUSE_SENS")) {
    const float value = static_cast< float >(std::atof(sens));
    if (std::isfinite(value) && value > 0.f) {
      sSensitivity = value;
    }
  }
  // No MP2_TOUCH_AIM flag here: `PortTouch` owns that switch (its AimEnabled) and turns this
  // source on through SetTouchEnabled, so there is one answer to "is the layout aiming by
  // drag".
  if (const char* speed = std::getenv("MP2_TOUCH_AIM_SPEED")) {
    const float value = static_cast< float >(std::atof(speed));
    if (std::isfinite(value)) {
      sTouchAimSpeed = ClampTouchAimSpeed(value);
    }
  }
}

// Aurora owns the only window, so the first one SDL knows about is the game's (the same
// approach the Prime port's debug overlay uses).
SDL_Window* GameWindow() {
  static SDL_Window* sWindow = nullptr;
  if (sWindow == nullptr) {
    int count = 0;
    if (SDL_Window** windows = SDL_GetWindows(&count)) {
      for (int i = 0; i < count; ++i) {
        if ((SDL_GetWindowFlags(windows[i]) & SDL_WINDOW_INPUT_FOCUS) != 0) {
          sWindow = windows[i];
          break;
        }
      }
      if (sWindow == nullptr && count > 0) {
        sWindow = windows[0];
      }
    }
  }
  return sWindow;
}

// Relative capture is what makes the motion relative and keeps the desktop cursor out of the
// game; leaving it off outside gameplay keeps the cursor usable for menus and the desktop.
void SetCapture(bool captured) {
  if (captured == sCaptured) {
    return;
  }
  sCaptured = captured;
  // One line per change, so a run's log says whether the mouse ever took the view.
  std::fprintf(stderr, "[mouse] %s\n", captured ? "captured: aim owns the view" : "released");
  if (SDL_Window* window = GameWindow()) {
    SDL_SetWindowRelativeMouseMode(window, captured);
  }
  if (captured) {
    SDL_HideCursor();
  } else {
    SDL_ShowCursor();
    ClearPendingTravel();
  }
}

// MP1's PointerAimScale: aim travel (pixels at the reference sensitivity) to the pointer's
// -1..1 range, so a mouse crosses the screen in one window's width of travel.
void PointerAimScale(float& sx, float& sy) {
  constexpr float kReferenceSensitivity = 0.0035f;
  int width = 0, height = 0;
  SDL_Window* window = GameWindow();
  if (window == nullptr || !SDL_GetWindowSize(window, &width, &height) || width <= 0 ||
      height <= 0) {
    width = 640;
    height = 480;
  }
  const float speed = sSensitivity / kReferenceSensitivity;
  sx = 2.f / static_cast< float >(width) * speed;
  sy = 2.f / static_cast< float >(height) * speed;
}

} // namespace

namespace PortMouse {

bool Enabled() {
  ReadEnv();
  return sEnabled;
}

void SetEnabled(bool enabled) {
  ReadEnv();
  sEnabled = enabled;
  if (!enabled && !sStickEnabled && !sTouchEnabled.load()) {
    Reset();
  }
}

bool StickEnabled() {
  ReadEnv();
  return sStickEnabled;
}

void SetStickEnabled(bool enabled) {
  ReadEnv();
  sStickEnabled = enabled;
  if (!enabled && !sEnabled && !sTouchEnabled.load()) {
    Reset();
  }
}

float StickAimRate() {
  ReadEnv();
  return sStickAimRate;
}

void SetStickAimRate(float pixelsPerSecond) {
  ReadEnv();
  if (std::isfinite(pixelsPerSecond) && pixelsPerSecond > 0.f) {
    sStickAimRate = pixelsPerSecond;
  }
}

bool StickAimSplit() {
  ReadEnv();
  return sStickAimSplit;
}

void SetStickAimSplit(bool enabled) {
  ReadEnv();
  sStickAimSplit = enabled;
}

float StickAimRateY() {
  ReadEnv();
  return sStickAimRateY;
}

void SetStickAimRateY(float pixelsPerSecond) {
  ReadEnv();
  if (std::isfinite(pixelsPerSecond) && pixelsPerSecond > 0.f) {
    sStickAimRateY = pixelsPerSecond;
  }
}

void AddStickAim(float x, float y, float dt) {
  if (!sStickEnabled || sUiActive || dt <= 0.f || !std::isfinite(x) || !std::isfinite(y)) {
    return;
  }
  if (x * x + y * y < 0.04f) {
    return; // inside the dead zone; a resting stick must not creep the view
  }
  const float dx = x * sStickAimRate * dt;
  // The aim's dy is screen-down positive, the stick's y is up positive.
  const float dy = -y * (sStickAimSplit ? sStickAimRateY : sStickAimRate) * dt;
  // Same thread as the latch (see sPendingMutex), but PresentedAimDelta reads both under the lock.
  std::lock_guard< std::mutex > lock(sPendingMutex);
  sPendingX += dx;
  sPendingY += dy;
  sStickPendingX += dx;
  sStickPendingY += dy;
}

bool TouchEnabled() {
  ReadEnv();
  return sTouchEnabled.load();
}

void SetTouchEnabled(bool enabled) {
  ReadEnv();
  sTouchEnabled = enabled;
  // Turning the layout's drag-to-aim off is the same shape as releasing the mouse: with no
  // source left, the aim must not keep a stale yaw for the next one to snap to.
  if (!enabled && !sEnabled && !sStickEnabled) {
    Reset();
  }
}

void AddTouchAim(float dxDp, float dyDp) {
  // A disabled source must not accumulate travel: the game thread early-outs before it
  // latches, so the pixels would sit here and burst the moment the source came back.
  if (!sTouchEnabled.load() || !std::isfinite(dxDp) || !std::isfinite(dyDp)) {
    return;
  }
  // Plain drags feed deltas only (MP1's model); the deltas themselves claim the view.
  sTouchAimLatched = true;
  std::lock_guard< std::mutex > lock(sPendingMutex);
  sPendingX += dxDp * sTouchAimSpeed;
  sPendingY += dyDp * sTouchAimSpeed;
}

void SetTouchAimDown(bool down) {
  sTouchAimDown = down;
  if (down) {
    sTouchAimLatched = true;
  }
}

void SpringSwipe() {
  sTouchHoldUntilNs = SDL_GetTicksNS() + static_cast< uint64_t >(kTouchSpringHoldSeconds * 1e9);
}

bool TouchAimActive() {
  return sTouchEnabled.load() &&
         (sTouchAimDown.load() || SDL_GetTicksNS() < sTouchHoldUntilNs.load());
}

float TouchAimSpeed() {
  ReadEnv();
  return sTouchAimSpeed;
}

void SetTouchAimSpeed(float pixelsPerDp) {
  ReadEnv();
  if (std::isfinite(pixelsPerDp)) {
    sTouchAimSpeed = ClampTouchAimSpeed(pixelsPerDp);
  }
}

void SetUiActive(bool active) {
  sUiActive = active;
  if (active) {
    sAiming = false;
    SetCapture(false);
  }
}

bool UiActive() { return sUiActive; }

void AddMotion(float dx, float dy) {
  if (!sCaptured || !std::isfinite(dx) || !std::isfinite(dy)) {
    return;
  }
  sPendingX += dx;
  sPendingY += dy;
}

void AddGyroPointer(float yaw, float pitch, float dt, float sweep) {
  if (!std::isfinite(yaw) || !std::isfinite(pitch) || !(dt > 0.f) || !(sweep > 0.f)) {
    return;
  }
  // MP1's PollGyro pointer branch: `sweep` radians of pad turn cross the screen (2 pointer units)
  // at the same pixels per degree both ways, so divide out the window and mouse scaling the
  // pointer applies to pending travel. No inversion.
  float px = 0.f, py = 0.f;
  PointerAimScale(px, py);
  if (!(px > 0.f)) {
    return;
  }
  const float cursor = 2.f / sweep * dt / px;
  std::lock_guard< std::mutex > lock(sPendingMutex);
  sPendingX += yaw * cursor;
  sPendingY -= pitch * cursor;
}

void AddGyroCamera(float yaw, float pitch, float dt, float sensitivity) {
  ReadEnv();
  if (!std::isfinite(yaw) || !std::isfinite(pitch) || !(dt > 0.f) || !(sSensitivity > 0.f)) {
    return;
  }
  // MP1's natural scale: the pending pixels become sSensitivity rad each, with the mouse inversion
  // applied, so divide that out and pre-undo the inversion: the camera turns `sensitivity` times
  // the pad's angle whatever the mouse is set to. x right / y up; the aim state expects right/down.
  const float sx = sInvertX ? -1.f : 1.f;
  const float sy = sInvertY ? -1.f : 1.f;
  const float scale = sensitivity * dt / sSensitivity;
  std::lock_guard< std::mutex > lock(sPendingMutex);
  sCameraPendingX += sx * yaw * scale;
  sCameraPendingY -= sy * pitch * scale;
}

void BeginTick() {
  // The UI thread may be adding travel right now; latch and clear in one lock so a late
  // addition lands in this tick or the next, never in neither.
  std::lock_guard< std::mutex > lock(sPendingMutex);
  sAppliedLastTick = false;
  sFrameX = sPendingX;
  sFrameY = sPendingY;
  sPendingX = 0.f;
  sPendingY = 0.f;
  sStickPendingX = 0.f;
  sStickPendingY = 0.f;
  sCameraFrameX = sCameraPendingX;
  sCameraFrameY = sCameraPendingY;
  sCameraPendingX = 0.f;
  sCameraPendingY = 0.f;
}

bool UpdateAim(bool active, bool locked, float x, float y, float z, float dt) {
  ReadEnv();
  sAppliedLastTick = false;
  if (std::isfinite(dt) && dt > 0.f) {
    sLastTickDt = dt;
  }
  if ((!sEnabled && !sStickEnabled && !sTouchEnabled.load()) || sUiActive) {
    sAiming = false;
    sPointerFree = false;
    sPointer.Center();
    SetCapture(false);
    return false;
  }
  // The touch source is on for a whole run but only owns the view while a finger is down (or a
  // flick's hold is live). Without that check the idle overlay would own it: CPlayer would turn
  // the body to the last drag's yaw every tick, pinning it so the C-stick and stick-free
  // movement could no longer turn the player - so idle must read as `locked` below.
  // Once a finger has aimed (or whenever mouse/stick aim is on) the aim stays the owner after
  // the lift - handing the view back snapped the pitch to the free-look level.
  const bool touchIdle = sTouchEnabled.load() && !sEnabled && !sStickEnabled &&
                         !sTouchAimLatched.load() && !TouchAimActive();
  // `locked` means something else owns the view (a lock-on, the morph ball, a cinematic): the
  // aim tracks that direction and the caller must not turn the player. `touchIdle` is the same
  // situation, so a drag that begins there does not snap the view either.
  const bool held = locked || touchIdle;
  const bool owns = active && !held;
  sAiming = owns;
  // Only mouse aim needs the pointer grabbed; twin-stick and touch leave the cursor alone.
  SetCapture(owns && sEnabled);
  if (sPointerEnabled) {
    // MP1's pointer path: the travel moves the pointer, not the view. A lock (or anything that
    // stops free look) re-centres it, so lock-on, scan and grapple still aim through the centre.
    if (owns) {
      float sx = 0.f, sy = 0.f;
      PointerAimScale(sx, sy);
      sPointer.Move(sFrameX * sx, sFrameY * sy);
    } else {
      sPointer.Center();
    }
    // The view turns only by the gyro's camera travel (zero unless it also turns the camera).
    const bool applied = sAim.Update(active, held, x, y, z, owns ? sCameraFrameX : 0.f,
                                     owns ? sCameraFrameY : 0.f, sSensitivity, sInvertX, sInvertY);
    if (applied && owns) {
      float dyaw = 0.f, dpitch = 0.f;
      PortMouse::PointerAim::TurnAt(sPointer.x, sPointer.y, sPointerBox, sPointerTurnRate, dt,
                                    dyaw, dpitch);
      sAim.Turn(dyaw, dpitch);
    }
    sPointerFree = applied && owns;
    if (!sPointerFree) {
      sPointer.Center();
    }
    if (applied) {
      sFrameX = sFrameY = 0.f;
      sCameraFrameX = sCameraFrameY = 0.f;
    }
    sAppliedLastTick = applied && owns;
    return applied && owns;
  }
  sPointerFree = false;
  sCameraFrameX = sCameraFrameY = 0.f;
  const bool applied = sAim.Update(active, held, x, y, z, sFrameX, sFrameY, sSensitivity,
                                   sInvertX, sInvertY);
  if (applied) {
    sFrameX = sFrameY = 0.f;
  }
  sAppliedLastTick = applied && owns;
  return applied && owns;
}

bool PresentedAimDelta(float fraction, float& dyaw, float& dpitch) {
  dyaw = dpitch = 0.f;
  if (!sAppliedLastTick || !sAiming || !sAim.initialized || !std::isfinite(fraction) ||
      fraction < 0.f) {
    return false;
  }
  if (fraction > 1.f) {
    fraction = 1.f;
  }
  float dx = 0.f, dy = 0.f, cx = 0.f, cy = 0.f;
  {
    std::lock_guard< std::mutex > lock(sPendingMutex);
    dx = sPendingX - sStickPendingX * (1.f - fraction);
    dy = sPendingY - sStickPendingY * (1.f - fraction);
    cx = sCameraPendingX;
    cy = sCameraPendingY;
  }
  // The aim the next tick would reach, through the same Update/Turn as UpdateAim.
  PortMouse::AimState next = sAim;
  if (sPointerEnabled) {
    if (!sPointerFree) {
      return false;
    }
    float sx = 0.f, sy = 0.f;
    PointerAimScale(sx, sy);
    PortMouse::PointerAim pointer = sPointer;
    pointer.Move(dx * sx, dy * sy);
    next.Update(true, false, 0.f, 0.f, 0.f, cx, cy, sSensitivity, sInvertX, sInvertY);
    float turnYaw = 0.f, turnPitch = 0.f;
    PortMouse::PointerAim::TurnAt(pointer.x, pointer.y, sPointerBox, sPointerTurnRate,
                                  fraction * sLastTickDt, turnYaw, turnPitch);
    next.Turn(turnYaw, turnPitch);
  } else {
    next.Update(true, false, 0.f, 0.f, 0.f, dx, dy, sSensitivity, sInvertX, sInvertY);
  }
  dyaw = static_cast< float >(std::remainder(double(next.yaw) - sAim.yaw, 2.0 * PortMouse::kPi));
  dpitch = next.pitch - sAim.pitch;
  return std::isfinite(dyaw) && std::isfinite(dpitch);
}

void SetPointerAim(bool enabled, float box, float turnRate) {
  if (enabled != sPointerEnabled) {
    // MP1's SetPointerAim: switching either way starts from the centre.
    sPointer.Center();
    sPointerFree = false;
    sPointerEnabled = enabled;
  }
  if (std::isfinite(box)) {
    sPointerBox = box < 0.f ? 0.f : box > 0.99f ? 0.99f : box;
  }
  if (std::isfinite(turnRate) && turnRate >= 0.f) {
    sPointerTurnRate = turnRate;
  }
}

bool PointerAimEnabled() { return sPointerEnabled; }

bool PointerAimPosition(float& x, float& y) {
  x = sPointer.x;
  y = sPointer.y;
  return sPointerFree;
}

void SetPointerAimPosition(float x, float y) {
  sPointer.Center();
  sPointer.Move(x, y);
}

void Synchronize(float x, float y, float z) {
  if (Enabled()) {
    sAim.Synchronize(x, y, z);
  }
}

void Reset() {
  sAim.Reset();
  sAiming = false;
  ClearPendingTravel();
  sPointer.Center();
  sPointerFree = false;
  // A finger's state belongs to the aim being reset: a held spring must not re-open the view
  // the next frame, and a later down must not inherit a stale hold.
  sTouchAimDown = false;
  sTouchAimLatched = false;
  sTouchHoldUntilNs = 0;
  SetCapture(false);
}

bool Aiming() { return sAiming; }
bool Initialized() { return sAim.initialized; }
float Yaw() { return sAim.yaw; }
float Pitch() { return sAim.pitch; }
float Sensitivity() {
  ReadEnv();
  return sSensitivity;
}

void SetSensitivity(float radiansPerPixel) {
  ReadEnv();
  if (std::isfinite(radiansPerPixel) && radiansPerPixel > 0.f) {
    sSensitivity = radiansPerPixel;
  }
}

bool InvertX() {
  ReadEnv();
  return sInvertX;
}

bool InvertY() {
  ReadEnv();
  return sInvertY;
}

void SetInvertX(bool invert) {
  ReadEnv();
  sInvertX = invert;
}

void SetInvertY(bool invert) {
  ReadEnv();
  sInvertY = invert;
}

} // namespace PortMouse
