// The F1 debug overlay. It is deliberately small: every control here is wired to something that
// exists, so nothing in the window lies about what it does. The teleport buttons drive the same
// PortWarp requests as the F5-F8 hotkeys and the MP2_INPUT tokens, and the aim controls drive
// PortMouse, which the game reads every tick.
//
// Built by `mp_port_audio` (it needs ImGui, which Aurora brings in with SDL).

#include "port_overlay.h"

#include "port_mouse.h"
#include "port_warp.h"

#include <SDL3/SDL.h>
#include <aurora/aurora.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>


namespace {

bool sVisible = false;
bool sSkipCinematics = false;
bool sGodMode = false; // never saved: there is no settings file, and a cheat must not persist
// First-person field of view in degrees, or 0 for the game's own 55 (CCameraManager::sFirstPersonFOV).
float sFirstPersonFov = 0.f;
// Draw the world without the HUD's view overlays (`MP2_NO_HELMET`, `MP2_NO_VISOR_FX`): for
// screenshots of an area, and for looking at what a visor effect covers.
bool sHideHelmet = false;
bool sHideVisorEffects = false;
// Arm cannon offset in metres along the camera's right, forward and up axes (`MP2_GUN_OFFSET`).
float sGunOffset[3] = {0.f, 0.f, 0.f};
// Anti-aliasing and filtering (`MP2_MSAA`, `MP2_ANISO`). The defaults are Aurora's own, so an
// untouched port applies nothing.
int sMsaa = 1;
int sAnisotropy = 16;
// HUD scale in percent (`MP2_HUD_SCALE`), and whether the HUD's own draw is the one being scaled.
int sHudScale = PortOverlay::kHudScaleMax;
bool sHudScaling = false;
bool sReadEnv = false;
uint32_t sWorldId = 0;
int sAreaId = -1;

// Frames per second, averaged over a second, from the presented-frame clock.
float sFps = 0.f;
uint64_t sFpsWindowStart = 0;
int sFpsFrames = 0;

bool EnvFlag(const char* name) {
  const char* value = std::getenv(name);
  return value != nullptr && *value != '\0' && std::strcmp(value, "0") != 0;
}

// Handed to aurora_set_graphics_quality only when they change; 0 means "never applied yet".
int sAppliedMsaa = 0;
int sAppliedAnisotropy = 0;

void ReadEnv() {
  if (sReadEnv) {
    return;
  }
  sReadEnv = true;
  sSkipCinematics = EnvFlag("MP2_SKIP_CINEMATICS");
  sGodMode = EnvFlag("MP2_GODMODE");
  sHideHelmet = EnvFlag("MP2_NO_HELMET");
  sHideVisorEffects = EnvFlag("MP2_NO_VISOR_FX");
  if (const char* fov = std::getenv("MP2_FOV")) {
    const float value = static_cast< float >(std::atof(fov));
    if (value == 0.f || (value >= 30.f && value <= 120.f)) {
      sFirstPersonFov = value;
    }
  }
  if (const char* offset = std::getenv("MP2_GUN_OFFSET")) {
    float parsed[3];
    if (std::sscanf(offset, "%f,%f,%f", &parsed[0], &parsed[1], &parsed[2]) == 3) {
      PortOverlay::SetGunOffset(parsed);
    }
  }
  if (const char* msaa = std::getenv("MP2_MSAA")) {
    const int samples = std::atoi(msaa);
    if (samples == 1 || samples == 4) {
      sMsaa = samples;
    }
  }
  if (const char* aniso = std::getenv("MP2_ANISO")) {
    // A max, not a step: any value in range is kept, and Aurora clamps what a sampler asks for.
    const int value = std::atoi(aniso);
    if (value >= 1 && value <= 16) {
      sAnisotropy = value;
    }
  }
  if (const char* hudScale = std::getenv("MP2_HUD_SCALE")) {
    const int percent = std::atoi(hudScale);
    if (percent == 0) {
      sHudScale = PortOverlay::kHudScaleMax;
    } else if (percent >= PortOverlay::kHudScaleMin && percent <= PortOverlay::kHudScaleMax) {
      sHudScale = percent;
    }
  }
}

void ApplyGraphicsQuality() {
  ReadEnv();
  if (sAppliedMsaa == sMsaa && sAppliedAnisotropy == sAnisotropy) {
    return;
  }
  sAppliedMsaa = sMsaa;
  sAppliedAnisotropy = sAnisotropy;
  // A request: Aurora rebuilds the framebuffers at the next frame start (a rebuild inside a frame
  // would invalidate the binds that frame already recorded).
  aurora_set_graphics_quality(static_cast< uint32_t >(sMsaa),
                              static_cast< uint16_t >(sAnisotropy));
}

void TickFps() {
  const uint64_t now = SDL_GetTicksNS();
  if (sFpsWindowStart == 0) {
    sFpsWindowStart = now;
  }
  ++sFpsFrames;
  const uint64_t elapsed = now - sFpsWindowStart;
  if (elapsed >= 1000000000ull) {
    sFps = static_cast< float >(sFpsFrames) / (static_cast< float >(elapsed) / 1e9f);
    sFpsFrames = 0;
    sFpsWindowStart = now;
  }
}

void DrawGameSection() {
  ImGui::SeparatorText("Game");
  ImGui::Text("world %08X  area %d", sWorldId, sAreaId);
  if (ImGui::Button("Previous area (F6)")) {
    PortWarp::CycleArea(-1);
  }
  ImGui::SameLine();
  if (ImGui::Button("Next area (F5)")) {
    PortWarp::CycleArea(1);
  }
  if (ImGui::Button("Previous world (F8)")) {
    PortWarp::CycleWorld(-1);
  }
  ImGui::SameLine();
  if (ImGui::Button("Next world (F7)")) {
    PortWarp::CycleWorld(1);
  }
  ImGui::TextDisabled("MP2_WORLD=\"<world>[,<area>]\" and MP2_TELEPORT=\"<area>\" at start-up");
}

void DrawAimSection() {
  ImGui::SeparatorText("Aim");
  bool mouseAim = PortMouse::Enabled();
  if (ImGui::Checkbox("Mouse aim", &mouseAim)) {
    PortMouse::SetEnabled(mouseAim);
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_MOUSE_AIM=1)");
  bool invertX = PortMouse::InvertX();
  if (ImGui::Checkbox("Invert X", &invertX)) {
    PortMouse::SetInvertX(invertX);
  }
  ImGui::SameLine();
  bool invertY = PortMouse::InvertY();
  if (ImGui::Checkbox("Invert Y", &invertY)) {
    PortMouse::SetInvertY(invertY);
  }
  float sensitivity = PortMouse::Sensitivity();
  // Radians per pixel is a poor thing to read; degrees per pixel is not much better, so the
  // slider shows a 1x-100x multiplier over the default instead.
  float scale = sensitivity / 0.0035f;
  if (ImGui::SliderFloat("Sensitivity", &scale, 0.1f, 5.f, "%.2fx")) {
    PortMouse::SetSensitivity(0.0035f * scale);
  }
  ImGui::TextDisabled("MP2_MOUSE_SENS=%.5f (radians per pixel)", PortMouse::Sensitivity());

  bool twinStick = PortMouse::StickEnabled();
  if (ImGui::Checkbox("Twin stick", &twinStick)) {
    PortMouse::SetStickEnabled(twinStick);
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_TWIN_STICK=1)");
  float rate = PortMouse::StickAimRate();
  if (ImGui::SliderFloat("Stick aim rate", &rate, 200.f, 2000.f, "%.0f px/s")) {
    PortMouse::SetStickAimRate(rate);
  }
  ImGui::TextDisabled("the right stick aims through the same path as the mouse");
}

void DrawRenderSection() {
  ImGui::SeparatorText("Render");
  if (ImGui::SliderFloat("Field of view (1st person)", &sFirstPersonFov, 0.f, 120.f,
                         sFirstPersonFov <= 0.f ? "game (%.0f)" : "%.0f deg")) {
    // Applied by CCameraManager::UpdateCameras: the first-person camera takes its FOV when it is
    // built, so the live change goes through CCameraManager::mFirstPersonFov.
  }
  ImGui::TextDisabled("(MP2_FOV=<deg>; 0 keeps the game's 55)");
  float offset[3] = {sGunOffset[0], sGunOffset[1], sGunOffset[2]};
  bool gunChanged = false;
  gunChanged |= ImGui::SliderFloat("Arm cannon right", &offset[0], -PortOverlay::kGunOffsetMax,
                                   PortOverlay::kGunOffsetMax, "%.2f m");
  gunChanged |= ImGui::SliderFloat("Arm cannon forward", &offset[1], -PortOverlay::kGunOffsetMax,
                                   PortOverlay::kGunOffsetMax, "%.2f m");
  gunChanged |= ImGui::SliderFloat("Arm cannon up", &offset[2], -PortOverlay::kGunOffsetMax,
                                   PortOverlay::kGunOffsetMax, "%.2f m");
  if (gunChanged) {
    // Read every frame by CPlayer::UpdateArmAndGunTransforms, so the gun moves on the next update
    // and the shots leave from the muzzle that is drawn.
    PortOverlay::SetGunOffset(offset);
  }
  if (ImGui::Button("Reset arm cannon")) {
    const float zero[3] = {0.f, 0.f, 0.f};
    PortOverlay::SetGunOffset(zero);
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_GUN_OFFSET=\"right,forward,up\" in metres, +-1)");
  int msaa = sMsaa >= 4 ? 1 : 0;
  if (ImGui::Combo("Anti-aliasing", &msaa, "Off\0" "4x MSAA\0")) {
    // Applied by PortOverlay::Draw through aurora_set_graphics_quality: the framebuffers are rebuilt
    // at the next frame start.
    sMsaa = msaa == 1 ? 4 : 1;
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_MSAA=1|4)");
  // A max, so the combo shows the smallest offered step that covers it (a hand-written MP2_ANISO=3
  // reads as 4x); picking a row sets that step.
  static const int kAnisoSteps[] = {1, 2, 4, 8, 16};
  int aniso = 4;
  for (int i = 0; i < 5; ++i) {
    if (kAnisoSteps[i] >= sAnisotropy) {
      aniso = i;
      break;
    }
  }
  if (ImGui::Combo("Anisotropic filtering", &aniso, "1x\0" "2x\0" "4x\0" "8x\0" "16x\0")) {
    sAnisotropy = kAnisoSteps[aniso];
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_ANISO=1..16)");
}

void DrawHudSection() {
  ImGui::SeparatorText("HUD");
  ImGui::Checkbox("Hide helmet", &sHideHelmet);
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_NO_HELMET=1)");
  ImGui::Checkbox("Hide visor effects", &sHideVisorEffects);
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_NO_VISOR_FX=1)");
  ImGui::TextDisabled("the scan visor's window and edges are what a visor effect draws here");
  if (ImGui::SliderInt("HUD scale", &sHudScale, PortOverlay::kHudScaleMin,
                       PortOverlay::kHudScaleMax, "%d%%")) {
    // Read by CGuiCamera::Draw while CSamusHud::Draw is drawing the HUD
    // (PortOverlay::HudScalingActive): the HUD's projection is scaled about the screen centre.
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_HUD_SCALE=50..100)");
  ImGui::TextDisabled("shrinks the HUD toward the middle; the helmet and menus keep their size");
}

void DrawTestingSection() {
  ImGui::SeparatorText("Testing");
  if (ImGui::Checkbox("Skip cinematics", &sSkipCinematics)) {
    // The game reads this every frame in its cinematic flow state.
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_SKIP_CINEMATICS=1)");
  ImGui::TextDisabled("ends every cinematic at once, seen or not; makes tests 3-5x faster");
}

void DrawCheatsSection() {
  ImGui::SeparatorText("Cheats");
  ImGui::Checkbox("God mode", &sGodMode);
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_GODMODE=1)");
  ImGui::TextDisabled("the player takes no damage; other actors still do. Not saved");
}

void DrawKeyboardSection() {
  ImGui::SeparatorText("Keyboard map");
  ImGui::TextDisabled("WASD move, IJKL C-stick, X/Z/C/V = A/B/X/Y,");
  ImGui::TextDisabled("Q/E = L/R, F = Z, Enter = Start, arrows = D-pad");
}

} // namespace

namespace PortOverlay {

void SetGameState(uint32_t worldId, int areaId) {
  sWorldId = worldId;
  sAreaId = areaId;
}

void Toggle() { sVisible = !sVisible; }

bool Visible() {
  ReadEnv();
  return sVisible;
}

bool SkipCinematics() {
  ReadEnv();
  return sSkipCinematics;
}

void SetSkipCinematics(bool skip) {
  ReadEnv();
  sSkipCinematics = skip;
}

bool HideHelmet() {
  ReadEnv();
  return sHideHelmet;
}

bool HideVisorEffects() {
  ReadEnv();
  return sHideVisorEffects;
}

float FirstPersonFov() {
  ReadEnv();
  return sFirstPersonFov;
}

void GunOffset(float out[3]) {
  ReadEnv();
  out[0] = sGunOffset[0];
  out[1] = sGunOffset[1];
  out[2] = sGunOffset[2];
}

void SetGunOffset(const float offset[3]) {
  ReadEnv();
  for (int i = 0; i < 3; ++i) {
    // A NaN slider value or a hand-written env var must not reach the gun's transform.
    sGunOffset[i] = std::isfinite(offset[i])
                        ? std::clamp(offset[i], -kGunOffsetMax, kGunOffsetMax)
                        : 0.f;
  }
}

int Msaa() {
  ReadEnv();
  return sMsaa;
}

void SetMsaa(int samples) {
  ReadEnv();
  sMsaa = samples >= 4 ? 4 : 1;
}

int Anisotropy() {
  ReadEnv();
  return sAnisotropy;
}

void SetAnisotropy(int maxAnisotropy) {
  ReadEnv();
  sAnisotropy = std::clamp(maxAnisotropy, 1, 16);
}

int HudScale() {
  ReadEnv();
  return sHudScale;
}

void SetHudScale(int percent) {
  ReadEnv();
  sHudScale = std::clamp(percent, kHudScaleMin, kHudScaleMax);
}

bool HudScalingActive() { return sHudScaling; }

void SetHudScalingActive(bool active) { sHudScaling = active; }

bool GodMode() {
  ReadEnv();
  return sGodMode;
}

void SetGodMode(bool enabled) {
  ReadEnv();
  sGodMode = enabled;
}

void Draw() {
  TickFps();
  // Even with the window closed: the env vars are settings, and a headless run never opens it.
  ApplyGraphicsQuality();
  // While the window is open the cursor belongs to it, not to the game's aim.
  PortMouse::SetUiActive(sVisible);
  if (!sVisible) {
    return;
  }

  if (ImGui::Begin("Metroid Prime 2 port", &sVisible)) {
    ImGui::Text("%.1f fps", static_cast< double >(sFps));
    DrawGameSection();
    DrawAimSection();
    DrawRenderSection();
    DrawHudSection();
    DrawTestingSection();
    DrawCheatsSection();
    DrawKeyboardSection();
    ImGui::SeparatorText("Hotkeys");
    ImGui::TextDisabled("F1 overlay, F5/F6 area, F7/F8 world");
  }
  ImGui::End();
}

} // namespace PortOverlay
