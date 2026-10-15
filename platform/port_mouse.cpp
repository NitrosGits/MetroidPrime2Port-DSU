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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

PortMouse::AimState sAim;
bool sEnabled = false;
bool sStickEnabled = false;
bool sReadEnv = false;
bool sCaptured = false;
bool sAiming = false;
bool sUiActive = false;
float sSensitivity = 0.0035f; // radians per pixel, the Prime port's default
float sStickAimRate = 900.f;  // aim pixels per second at full stick deflection
bool sInvertX = false;
bool sInvertY = false;
float sPendingX = 0.f;
float sPendingY = 0.f;
float sFrameX = 0.f;
float sFrameY = 0.f;

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
    sPendingX = sPendingY = sFrameX = sFrameY = 0.f;
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
  if (!enabled && !sStickEnabled) {
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
  if (!enabled && !sEnabled) {
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
  sFrameX = sPendingX;
  sFrameY = sPendingY;
  sPendingX = 0.f;
  sPendingY = 0.f;
}

bool UpdateAim(bool active, bool locked, float x, float y, float z) {
  ReadEnv();
  if ((!sEnabled && !sStickEnabled) || sUiActive) {
    sAiming = false;
    SetCapture(false);
    return false;
  }
  // `locked` means something else owns the view (a lock-on, the morph ball, a cinematic): the
  // aim tracks that direction and the caller must not turn the player.
  const bool owns = active && !locked;
  sAiming = owns;
  // Only mouse aim needs the pointer grabbed; twin-stick leaves the cursor alone.
  SetCapture(owns && sEnabled);
  const bool applied = sAim.Update(active, locked, x, y, z, sFrameX, sFrameY, sSensitivity,
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
  sPendingX = sPendingY = sFrameX = sFrameY = 0.f;
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
