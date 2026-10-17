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
float sTouchAimSpeed = 2.25f; // aim pixels per dp, the Prime port's touch_aim_speed default
bool sInvertX = false;
bool sInvertY = false;
// The aim's down state is set by the UI thread (a finger lift) and read every tick by the game
// thread, so it is atomic rather than mutex-guarded: it is a flag, not a value to accumulate.
std::atomic<bool> sTouchAimDown{false};
std::atomic<uint64_t> sTouchHoldUntilNs{0};
float sPendingX = 0.f;
float sPendingY = 0.f;
float sFrameX = 0.f;
float sFrameY = 0.f;

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

void AddStickAim(float x, float y, float dt) {
  if (!sStickEnabled || sUiActive || dt <= 0.f || !std::isfinite(x) || !std::isfinite(y)) {
    return;
  }
  if (x * x + y * y < 0.04f) {
    return; // inside the dead zone; a resting stick must not creep the view
  }
  sPendingX += x * sStickAimRate * dt;
  // The aim's dy is screen-down positive, the stick's y is up positive.
  sPendingY -= y * sStickAimRate * dt;
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
  std::lock_guard< std::mutex > lock(sPendingMutex);
  sPendingX += dxDp * sTouchAimSpeed;
  sPendingY += dyDp * sTouchAimSpeed;
}

void SetTouchAimDown(bool down) { sTouchAimDown = down; }

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

void BeginFrame() {
  // The UI thread may be adding travel right now; latch and clear in one lock so a late
  // addition lands in this tick or the next, never in neither.
  std::lock_guard< std::mutex > lock(sPendingMutex);
  sFrameX = sPendingX;
  sFrameY = sPendingY;
  sPendingX = 0.f;
  sPendingY = 0.f;
}

bool UpdateAim(bool active, bool locked, float x, float y, float z) {
  ReadEnv();
  if ((!sEnabled && !sStickEnabled && !sTouchEnabled.load()) || sUiActive) {
    sAiming = false;
    SetCapture(false);
    return false;
  }
  // The touch source is on for a whole run but only owns the view while a finger is down (or a
  // flick's hold is live). Without that check the idle overlay would own it: CPlayer would turn
  // the body to the last drag's yaw every tick, pinning it so the C-stick and stick-free
  // movement could no longer turn the player - so idle must read as `locked` below.
  const bool touchIdle = sTouchEnabled.load() && !TouchAimActive();
  // `locked` means something else owns the view (a lock-on, the morph ball, a cinematic): the
  // aim tracks that direction and the caller must not turn the player. `touchIdle` is the same
  // situation, so a drag that begins there does not snap the view either.
  const bool held = locked || touchIdle;
  const bool owns = active && !held;
  sAiming = owns;
  // Only mouse aim needs the pointer grabbed; twin-stick and touch leave the cursor alone.
  SetCapture(owns && sEnabled);
  const bool applied = sAim.Update(active, held, x, y, z, sFrameX, sFrameY, sSensitivity,
                                   sInvertX, sInvertY);
  if (applied) {
    sFrameX = sFrameY = 0.f;
  }
  return applied && owns;
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
  // A finger's state belongs to the aim being reset: a held spring must not re-open the view
  // the next frame, and a later down must not inherit a stale hold.
  sTouchAimDown = false;
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
