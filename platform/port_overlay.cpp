// The F1 debug overlay. It is deliberately small: every control here is wired to something that
// exists, so nothing in the window lies about what it does. The teleport buttons drive the same
// PortWarp requests as the Game page's buttons and the MP2_INPUT tokens, and the aim controls drive
// PortMouse, which the game reads every tick.
//
// Built by `mp_port_audio` (it needs ImGui, which Aurora brings in with SDL).

#include "port_overlay.h"

#include "port_controls.h"
#include "port_freecam.h"
#include "port_photo.h"
#include "port_env.h"
#include "port_overlay_internal.h"
#include "port_input_remap_ui.h"
#include "port_logfile.h"
#include "port_mouse.h"
#include "port_touch.h"
#include "port_settings.h"
#include "port_ui_common.h"
#include "port_warp.h"

#include "MetroidPrime/Player/CGameOptions.hpp"
#include "MetroidPrime/Player/CGameState.hpp"

#include <SDL3/SDL.h>
#include <aurora/aurora.h>
#include <aurora/gfx.h>
#include <dolphin/pad.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <cstdlib>
#include <cstring>
#include <iterator>


namespace PortOverlayState {

bool sSkipCinematics = false;
// `MP2_LOG_FILE`: whether stdout and stderr also go to a file (F1 > Testing, console `logfile`).
// PortLogFile::Start reads it before Aurora comes up, so the row only reports the state the next
// launch will take.
bool sLogFile = true;
// The gameplay tweaks (`MP2_FAST_MORPH`, `MP2_SPRING_BALL`, `MP2_LOCK_ON_TOGGLE`,
// `MP2_STICKY_CHARGE`), saved in settings.cfg.
bool sFastMorph = false;
bool sSpringBall = false;
bool sLockOnToggle = false;
bool sStickyCharge = false;
bool sRapidCharge = false;
bool sMouseButtons = true;
// `MP2_CARD_BACKUP`: copy the card's save file into save-backups before every card write (F1 >
// Saves, `cardbackup`).
bool sCardBackup = false;
// First-person field of view in degrees, or 0 for the game's own 55 (CCameraManager::sFirstPersonFOV).
float sFirstPersonFov = 0.f;
// Draw the world without the HUD's view overlays (`MP2_NO_HELMET`, `MP2_NO_VISOR_FX`): for
// screenshots of an area, and for looking at what a visor effect covers.
bool sHideHelmet = false;
bool sHideVisorEffects = false;
// `aim_crosshair`: draw the reticle while the port's own aim owns the body heading (see
// PortOverlay::AimCrosshair). On by default: without it the reticle never appears under mouse,
// twin-stick or touch aim, which is the only way the phone plays.
bool sAimCrosshair = true;
// The Prime port's crosshair settings: style (classic = the game's reticle model, remastered = the
// cursor CTargetReticles draws itself), size in percent, opacity, and an optional custom colour.
// MP2's classic model does not resolve in this port, so `remastered` is the default here.
int sCrosshairStyle = PortOverlay::kCrosshairStyle_Remastered;
int sCrosshairSize = PortOverlay::kCrosshairSizeDefault;
int sCrosshairOpacity = PortOverlay::kCrosshairOpacityMax;
bool sCrosshairCustomColor = false;
float sCrosshairColor[3] = {1.f, 1.f, 1.f};
// Arm cannon offset in metres along the camera's right, forward and up axes (`MP2_GUN_OFFSET`).
float sGunOffset[3] = {0.f, 0.f, 0.f};
// Anti-aliasing and filtering (`MP2_MSAA`, `MP2_ANISO`). The defaults are Aurora's own, so an
// untouched port applies nothing.
int sMsaa = 1;
int sAnisotropy = 16;
// HUD scale in percent (`MP2_HUD_SCALE`), and whether the HUD's own draw is the one being scaled.
int sHudScale = PortOverlay::kHudScaleMax;
// Interface scale in percent (`MP2_UI_SCALE`) and vsync (`MP2_VSYNC`).
int sUiScale = 100;
bool sVsync = false;
// MP1's sOverlayWindowed (desktop: the old floating window instead of the page window) and sUiTheme
// (0 auto, 1 Prime, 2 Plain; MP1's 2 is Remastered, which MP2 has no theme for).
bool sOverlayWindowed = false;
int sUiTheme = 0;
} // namespace PortOverlayState

using namespace PortOverlayState;

namespace {

bool sVisible = false;
bool sGodMode = false; // never saved: a cheat must not persist, so it is not a setting
bool sHudScaling = false;
bool sReadEnv = false;
uint32_t sWorldId = 0;
int sAreaId = -1;

// Frames per second, averaged over a second, from the presented-frame clock.
float sFps = 0.f;
uint64_t sFpsWindowStart = 0;
int sFpsFrames = 0;

// Handed to aurora_set_graphics_quality only when they change; 0 means "never applied yet".
int sAppliedMsaa = 0;
int sAppliedAnisotropy = 0;
// Handed to aurora_enable_vsync only when it changes; -1 means "never applied yet".
int sAppliedVsync = -1;

// Applies <user>/settings.cfg once per run. An MP2_<KEY> environment variable is not applied here:
// the accessors below read it each time, so it wins without ever reaching the file.
void ReadEnv() {
  if (sReadEnv) {
    return;
  }
  sReadEnv = true;
  sGodMode = port::EnvFlag("MP2_GODMODE");
  PortOverlaySettings::LoadSettings();
}

// A setter's change is saved at once (a no-op when nothing differs from the file).
void Persist() {
  PortOverlaySettings::MarkDirty();
  PortOverlaySettings::SaveSettings();
}

// A UI row's change is saved by Draw once the mouse is up, not on every frame of a slider drag.
bool Changed(bool changed) {
  if (changed) {
    PortOverlaySettings::MarkDirty();
  }
  return changed;
}

// The effective values: the file's/UI's variable, unless the environment names it.
bool EnvBool(const char* name, bool value) { return port::EnvFlag(name, value); }

int EnvIntIn(const char* name, int value, int min, int max) {
  const int env = port::EnvInt(name, value);
  return env >= min && env <= max ? env : value;
}

bool EnvFloat3(const char* name, float out[3], float min, float max) {
  const char* text = port::EnvString(name);
  float parsed[3];
  char extra = 0;
  if (text == nullptr || std::sscanf(text, "%f,%f,%f%c", &parsed[0], &parsed[1], &parsed[2], &extra) != 3) {
    return false;
  }
  for (int i = 0; i < 3; ++i) {
    if (!std::isfinite(parsed[i])) {
      return false;
    }
  }
  for (int i = 0; i < 3; ++i) {
    out[i] = std::clamp(parsed[i], min, max);
  }
  return true;
}

void StoreGunOffset(const float offset[3]) {
  for (int i = 0; i < 3; ++i) {
    // A NaN slider value or a hand-written env var must not reach the gun's transform.
    sGunOffset[i] = std::isfinite(offset[i])
                        ? std::clamp(offset[i], -PortOverlay::kGunOffsetMax, PortOverlay::kGunOffsetMax)
                        : 0.f;
  }
}

void ApplyGraphicsQuality() {
  ReadEnv();
  const int msaa = PortOverlay::Msaa();
  const int anisotropy = PortOverlay::Anisotropy();
  if (sAppliedMsaa == msaa && sAppliedAnisotropy == anisotropy) {
    return;
  }
  sAppliedMsaa = msaa;
  sAppliedAnisotropy = anisotropy;
  // A request: Aurora rebuilds the framebuffers at the next frame start (a rebuild inside a frame
  // would invalidate the binds that frame already recorded).
  aurora_set_graphics_quality(static_cast< uint32_t >(msaa),
                              static_cast< uint16_t >(anisotropy));
}

void ApplyVsync() {
  ReadEnv();
  const bool vsync = PortOverlay::Vsync();
  if (sAppliedVsync == (vsync ? 1 : 0)) {
    return;
  }
  sAppliedVsync = vsync ? 1 : 0;
  aurora_enable_vsync(vsync);
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
  if (ImGui::Button("Previous area")) {
    PortWarp::CycleArea(-1);
  }
  ImGui::SameLine();
  if (ImGui::Button("Next area")) {
    PortWarp::CycleArea(1);
  }
  if (ImGui::Button("Previous world")) {
    PortWarp::CycleWorld(-1);
  }
  ImGui::SameLine();
  if (ImGui::Button("Next world")) {
    PortWarp::CycleWorld(1);
  }
  PortUi::Help("MP2_WORLD=\"<world>[,<area>]\" and MP2_TELEPORT=\"<area>\" at start-up");

  ImGui::SeparatorText("Camera");
  {
    bool freeCam = PortFreeCam::Active();
    if (PortUi::Toggle("Free camera", &freeCam, nullptr,
                       "fly a camera of your own with the pad (left stick, C stick, L/R, Z); HUD and arm "
                       "cannon are hidden. The Photo mode key (F8) toggles it too")) {
      PortFreeCam::SetActive(freeCam, nullptr);
    }
  }

  if (PortFreeCam::Active()) {
    ImGui::SeparatorText("Photo");
    // 0 keeps the game's own FOV (55 in first person); the slider shows that until it is moved.
    const bool gameFov = PortPhoto::Fov() <= 0.f;
    float fov = gameFov ? 55.f : PortPhoto::Fov();
    if (PortUi::SliderFloat("Field of view", &fov, 30.f, 100.f, gameFov ? "game (%.0f)" : "%.0f deg",
                            nullptr, "the free camera's vertical field of view; untouched, it keeps "
                                     "the game's own")) {
      PortPhoto::SetFov(fov);
    }
    // The slider stops at 30, so the game's own FOV needs its own button once the slider is moved.
    if (!gameFov) {
      ImGui::SameLine();
      if (ImGui::SmallButton("Game FOV")) {
        PortPhoto::SetFov(0.f);
      }
    }
    float roll = PortPhoto::Roll();
    if (PortUi::SliderFloat("Roll", &roll, -180.f, 180.f, "%.0f deg", nullptr,
                            "tilts the picture about the view axis; movement ignores it")) {
      PortPhoto::SetRoll(roll);
    }
    float speed = PortPhoto::Speed();
    if (PortUi::SliderFloat("Camera speed", &speed, 0.5f, 50.f, "%.1f m/s", nullptr,
                            "metres per second at a full stick; the right trigger multiplies it by four")) {
      PortPhoto::SetSpeed(speed);
    }
    int ss = PortPhoto::Supersample() >= 4 ? 2 : PortPhoto::Supersample() >= 2 ? 1 : 0;
    if (PortUi::Combo("Supersample", &ss, "1x\0" "2x\0" "4x\0", nullptr,
                      "renders the photo at 2x or 4x the window's size; a size the GPU can't hold "
                      "steps down")) {
      PortPhoto::SetSupersample(ss == 2 ? 4 : ss == 1 ? 2 : 1);
    }
    bool showSamus = PortPhoto::ShowSamus();
    if (PortUi::Toggle("Show Samus", &showSamus, nullptr,
                       "draws Samus's body where the player stands")) {
      PortPhoto::SetShowSamus(showSamus);
    }
    ImGui::BeginDisabled(PortPhoto::Capturing());
    if (ImGui::Button("Take photo")) {
      PortPhoto::RequestCapture(0);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
      ImGui::SetTooltip("saves a PNG of the picture without the HUD to screenshots/");
    }
    const PortPhoto::CaptureResult last = PortPhoto::LastCapture();
    if (PortPhoto::Capturing()) {
      PortUi::Help("Capturing...");
    } else if (last.state > 0) {
      char line[512];
      std::snprintf(line, sizeof(line), "Saved %ux%u: %s", last.width, last.height, last.path.c_str());
      PortUi::Help(line);
    } else if (last.state < 0) {
      char line[512];
      std::snprintf(line, sizeof(line), "Capture failed; the log says why");
      PortUi::Help(line);
    }
  }

  ImGui::SeparatorText("Cutscenes");
  // The game reads this every frame in its cinematic flow state.
  Changed(PortUi::Toggle("Skip cinematics", &sSkipCinematics, "MP2_SKIP_CINEMATICS=1",
               "ends every cinematic at once, seen or not; makes tests 3-5x faster"));
}

// The Prime 1 port's Video page (debug_ui/tabs.cpp's DrawVideoTab) with its four sub-tabs. Display
// holds the view (FOV, arm cannon, helmet, HUD); Quality the render settings; Frame rate the vsync.
void DrawVideoDisplay() {
  ImGui::SeparatorText("HUD and view");
  // Applied by CCameraManager::UpdateCameras: the first-person camera takes its FOV when it is
  // built, so the live change goes through CCameraManager::mFirstPersonFov.
  Changed(PortUi::SliderFloat("Field of view (1st person)", &sFirstPersonFov, 0.f, 120.f,
                              sFirstPersonFov <= 0.f ? "game (%.0f)" : "%.0f deg",
                              "MP2_FOV=<deg>; 0 keeps the game's 55"));
  float offset[3] = {sGunOffset[0], sGunOffset[1], sGunOffset[2]};
  bool gunChanged = false;
  static const char* const kGunLabels[3] = {"Arm cannon right", "Arm cannon forward",
                                            "Arm cannon up"};
  for (int i = 0; i < 3; ++i) {
    gunChanged |= ImGui::SliderFloat(kGunLabels[i], &offset[i], -PortOverlay::kGunOffsetMax,
                                     PortOverlay::kGunOffsetMax, "%.2f m");
  }
  if (gunChanged) {
    // Read every frame by CPlayer::UpdateArmAndGunTransforms, so the gun moves on the next update
    // and the shots leave from the muzzle that is drawn.
    StoreGunOffset(offset);
    Changed(true);
  }
  if (ImGui::Button("Reset arm cannon")) {
    const float zero[3] = {0.f, 0.f, 0.f};
    StoreGunOffset(zero);
    Changed(true);
  }
  PortUi::Note("MP2_GUN_OFFSET=\"right,forward,up\" in metres, +-1", nullptr, true);
  Changed(PortUi::Toggle("Hide helmet", &sHideHelmet, "MP2_NO_HELMET=1"));
  Changed(PortUi::Toggle("Hide visor effects", &sHideVisorEffects, "MP2_NO_VISOR_FX=1",
               "the scan visor's window and edges are what a visor effect draws here"));
  if (sAimCrosshair) {
    int style = sCrosshairStyle;
    if (PortUi::Combo("Crosshair style", &style, "Classic\0" "Remastered\0",
                      "MP2_CROSSHAIR_STYLE=classic|remastered",
                      "Remastered is the one this port can draw: the classic reticle's model does "
                      "not load here")) {
      sCrosshairStyle = style == PortOverlay::kCrosshairStyle_Remastered
                            ? PortOverlay::kCrosshairStyle_Remastered
                            : PortOverlay::kCrosshairStyle_Classic;
      Changed(true);
    }
    Changed(PortUi::SliderInt("Crosshair opacity", &sCrosshairOpacity,
                              PortOverlay::kCrosshairOpacityMin, PortOverlay::kCrosshairOpacityMax,
                              "%d%%", "MP2_CROSSHAIR_OPACITY=0..100"));
  }
  // Read by CGuiCamera::Draw while CSamusHud::Draw is drawing the HUD
  // (PortOverlay::HudScalingActive): the HUD's projection is scaled about the screen centre.
  Changed(PortUi::SliderInt("HUD scale", &sHudScale, PortOverlay::kHudScaleMin,
                            PortOverlay::kHudScaleMax, "%d%%", "MP2_HUD_SCALE=50..100",
                            "shrinks the HUD toward the middle; the helmet and menus keep "
                            "their size"));
}

void DrawVideoQuality() {
  ImGui::SeparatorText("Quality");
  int msaa = sMsaa >= 4 ? 1 : 0;
  // Applied by PortOverlay::Draw through aurora_set_graphics_quality: the framebuffers are rebuilt
  // at the next frame start.
  if (PortUi::Combo("Anti-aliasing", &msaa, "Off\0" "4x MSAA\0", "MP2_MSAA=1|4")) {
    sMsaa = msaa == 1 ? 4 : 1;
    Changed(true);
  }
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
  if (PortUi::Combo("Anisotropic filtering", &aniso, "1x\0" "2x\0" "4x\0" "8x\0" "16x\0",
                    "MP2_ANISO=1..16")) {
    sAnisotropy = kAnisoSteps[aniso];
    Changed(true);
  }
}

void DrawVideoFrameRate() {
  ImGui::SeparatorText("Frame rate");
  // Applied by PortOverlay::Draw through aurora_enable_vsync, every frame it changes.
  Changed(PortUi::Toggle("VSync", &sVsync, "MP2_VSYNC=1",
                         "waits for the display's refresh before presenting a frame"));
}

// MP1 files its renderer workarounds (texture copy paths, backend and API choices) here; this port
// has no such knob on a page yet, so the tab says so rather than inventing one.
void DrawVideoCompatibility() {
  ImGui::BeginDisabled();
  ImGui::TextWrapped("No compatibility options in this port yet.");
  ImGui::EndDisabled();
}

void DrawVideoSection() {
  if (ImGui::BeginTabBar("videoPages")) {
    if (PortOverlaySearch::SubTab("Video", "Display")) {
      DrawVideoDisplay();
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Video", "Quality")) {
      DrawVideoQuality();
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Video", "Frame rate")) {
      DrawVideoFrameRate();
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Video", "Compatibility")) {
      DrawVideoCompatibility();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

// The Prime 1 port's System page (DrawSystemTab) with the sections this port has rows for, in its
// order: Overlay, Settings, MemoryCard, Log, About. Discord and Update have no MP2 backend.
void DrawSystemSection() {
  ImGui::SeparatorText("Overlay");
#if !defined(__ANDROID__)
  Changed(PortUi::Toggle("Overlay as a floating window", &sOverlayWindowed, nullptr));
#endif
  Changed(PortUi::Combo("Menu theme", &sUiTheme, "Auto\0Metroid Prime\0Plain\0", nullptr,
                        "The look of this overlay. Auto is the Metroid Prime pause-screen style; Plain is "
                        "stock Dear ImGui."));
  // Multiplies the display's own scale (PortUi::UpdateUiScale); the product is clamped to 1..4, so on a
  // desktop at scale 1 only values above 100% make the window larger.
  Changed(PortUi::SliderInt("UI scale", &sUiScale, PortOverlay::kUiScaleMin, PortOverlay::kUiScaleMax, "%d%%",
                            "MP2_UI_SCALE=50..200",
                            "the size of this window's text and controls"));

  ImGui::SeparatorText("Settings");
  const bool dirty = PortOverlaySettings::Dirty();
  ImGui::TextUnformatted(dirty ? "Unsaved changes" : "Saved");
  ImGui::SameLine();
  ImGui::BeginDisabled(!dirty);
  if (ImGui::Button("Save settings now")) {
    PortOverlaySettings::SaveSettings();
  }
  ImGui::EndDisabled();
  if (const std::string& path = PortSettings::FilePath(); !path.empty()) {
    ImGui::TextWrapped("%s", path.c_str());
  }
  PortUi::Help("Changes are written when no control is being edited; MP2_<KEY> environment "
               "variables override the file for one run");

  ImGui::SeparatorText("MemoryCard");
  Changed(PortUi::Toggle("Back up the card", &sCardBackup, "MP2_CARD_BACKUP=1",
               "copies the save file into save-backups before each card write; `save import "
               "latest` on the console puts one back"));

  ImGui::SeparatorText("Log");
  Changed(PortUi::Toggle("Log file", &sLogFile, "MP2_LOG_FILE=1",
               "stdout and stderr go to metroid_prime2_port.log in the user folder; takes "
               "effect next launch"));

  ImGui::SeparatorText("About");
  ImGui::TextUnformatted("Metroid Prime 2: Echoes native port (G2ME01)");
  ImGui::TextUnformatted("Aurora (SDL3 / WebGPU), Dear ImGui");
}

void DrawCheatsSection() {
  ImGui::SeparatorText("Cheats");
  PortUi::Toggle("God mode", &sGodMode, "MP2_GODMODE=1",
                 "the player takes no damage; other actors still do. Not saved");
}

void DrawControlsMouse() {
  ImGui::SeparatorText("Mouse aim");
  bool mouseAim = PortMouse::Enabled();
  if (PortUi::Toggle("Mouse aim", &mouseAim, "MP2_MOUSE_AIM=1")) {
    PortMouse::SetEnabled(mouseAim);
  }
  bool invertX = PortMouse::InvertX();
  if (ImGui::Checkbox("Invert X", &invertX)) {
    PortMouse::SetInvertX(invertX);
  }
  ImGui::SameLine();
  bool invertY = PortMouse::InvertY();
  if (ImGui::Checkbox("Invert Y", &invertY)) {
    PortMouse::SetInvertY(invertY);
  }
  // Radians per pixel is a poor thing to read; degrees per pixel is not much better, so the
  // slider shows a 1x-100x multiplier over the default instead.
  float scale = PortMouse::Sensitivity() / 0.0035f;
  char sensText[64];
  std::snprintf(sensText, sizeof(sensText), "MP2_MOUSE_SENS=%.5f (radians per pixel)",
                static_cast< double >(PortMouse::Sensitivity()));
  if (PortUi::SliderFloat("Sensitivity", &scale, 0.1f, 5.f, "%.2fx", nullptr, sensText)) {
    PortMouse::SetSensitivity(0.0035f * scale);
  }
  Changed(PortUi::Toggle("Weapon buttons", &sMouseButtons, "MP2_MOUSE_BUTTONS=0",
                         "the mouse buttons drive the pad's gameplay rows (fire, missile, jump...) "
                         "while mouse aim holds the cursor; bind them on the Remap tab"));
  ImGui::SeparatorText("Crosshair");
  Changed(PortUi::Toggle(
      "Aim crosshair", &sAimCrosshair, "MP2_AIM_CROSSHAIR=0",
      "the game's crosshair input is a look control the port never presses, so the port holds "
      "the reticle on while its own aim owns the view"));
  if (sAimCrosshair) {
    Changed(PortUi::SliderInt("Crosshair size", &sCrosshairSize, PortOverlay::kCrosshairSizeMin,
                              PortOverlay::kCrosshairSizeMax, "%d%%",
                              "MP2_CROSSHAIR_SIZE=25..100",
                              "a percentage of the reticle's own scale, while the port is aiming"));
  }
}

void DrawControlsStickAim() {
  ImGui::SeparatorText("Stick aim");
  bool twinStick = PortMouse::StickEnabled();
  if (PortUi::Toggle("Twin stick", &twinStick, "MP2_TWIN_STICK=1")) {
    PortMouse::SetStickEnabled(twinStick);
  }
  float rate = PortMouse::StickAimRate();
  if (PortUi::SliderFloat("Stick aim rate", &rate, 200.f, 2000.f, "%.0f px/s", nullptr,
                          "the right stick aims through the same path as the mouse")) {
    PortMouse::SetStickAimRate(rate);
  }
  bool split = PortMouse::StickAimSplit();
  if (PortUi::Toggle("Separate vertical speed", &split, "MP2_STICK_AIM_SPLIT=1",
                     "gives looking up and down its own speed; Stick aim rate then sets only left and right")) {
    PortMouse::SetStickAimSplit(split);
  }
  if (split) {
    float rateY = PortMouse::StickAimRateY();
    if (PortUi::SliderFloat("Vertical stick aim rate", &rateY, 200.f, 2000.f, "%.0f px/s", nullptr,
                            "how fast the right stick turns the camera up and down at full tilt")) {
      PortMouse::SetStickAimRateY(rateY);
    }
  }
  // The game's own option, saved with its settings (Reverse Y Axis in its pause menu).
  ImGui::BeginDisabled(gpGameState == nullptr);
  bool invertStickY = gpGameState != nullptr && gpGameState->GameOptions().GetInvertYAxis();
  if (PortUi::Toggle("Invert stick aim Y", &invertStickY, nullptr,
                     "the game's Reverse Y Axis option; the stick and free look use it") &&
      gpGameState != nullptr) {
    gpGameState->GameOptions().SetInvertYAxis(invertStickY);
  }
  ImGui::EndDisabled();
}

void DrawControlsOptions() {
  ImGui::SeparatorText("Buttons");
  Changed(PortUi::Toggle("Toggle lock-on", &sLockOnToggle, "MP2_LOCK_ON_TOGGLE=1",
               "an L press latches the lock until the next; one that ends lets go"));
  Changed(PortUi::Toggle("Sticky charge", &sStickyCharge, "MP2_STICKY_CHARGE=1",
               "letting go of a long A hold keeps charging until the next press fires it"));
  Changed(PortUi::Toggle("Remastered charge", &sRapidCharge, "MP2_RAPID_CHARGE=1",
               "holding fire after the press shot fires a few quick shots, Power Beam only"));
  ImGui::SeparatorText("Rumble");
  ImGui::BeginDisabled(gpGameState == nullptr);
  bool rumble = gpGameState != nullptr && gpGameState->GameOptions().GetIsRumbleEnabled();
  if (PortUi::Toggle("Rumble", &rumble, nullptr,
                     "controller vibration, the same switch as Rumble in the pause menu's options") &&
      gpGameState != nullptr) {
    gpGameState->GameOptions().SetIsRumbleEnabled(rumble);
  }
  ImGui::EndDisabled();
  ImGui::SeparatorText("Morph ball");
  Changed(PortUi::Toggle("Fast morph", &sFastMorph, "MP2_FAST_MORPH=1",
               "the morph and unmorph transitions take 0.2 s instead of the game's second"));
  Changed(PortUi::Toggle("Spring Ball", &sSpringBall, "MP2_SPRING_BALL=1",
               "C-stick up on the ground bomb-jumps the ball (needs Morph Ball Bombs)"));
}

void DrawControlsTouchGyro() {
  ImGui::SeparatorText("Touch controls");
  bool controls = PortTouch::ControlsEnabled();
  if (PortUi::Toggle("On-screen controls", &controls, "MP2_TOUCH_CONTROLS=0",
                     "the phone's touch overlay; touch.cfg in the user folder")) {
    PortTouch::SetControlsEnabled(controls);
  }
  bool aim = PortTouch::AimEnabled();
  if (PortUi::Toggle("Touch aim", &aim, "MP2_TOUCH_AIM=1",
                     "drag on the free screen to aim")) {
    PortTouch::SetAimEnabled(aim);
  }
  const bool classic = PortTouch::Classic();
  const bool twin = PortTouch::TwinStick();
  int layoutIndex = classic ? 1 : (twin ? 2 : 0);
  if (PortUi::Combo("Layout", &layoutIndex, "Default\0" "Classic GameCube\0" "Twin stick (Remastered)\0",
                    "MP2_TOUCH_CLASSIC=1|MP2_TOUCH_TWIN=1",
                    "Default: free-area drag aims, wheels pick beams and visors. Classic: the "
                    "C-stick and D-pad. Twin stick: a right stick that aims")) {
    // CycleLayout steps Default -> Classic -> Twin -> Default.
    for (int i = 0; i < 3 && (PortTouch::Classic() ? 1 : (PortTouch::TwinStick() ? 2 : 0)) != layoutIndex; ++i) {
      PortTouch::CycleLayout();
    }
    if (layoutIndex == 2) PortTouch::SetWheelsEnabled(false);
  }
  ImGui::SameLine();
  if (ImGui::Button("Edit layout")) {
    PortTouch::RequestEdit();
  }
  ImGui::BeginDisabled(!controls);
  bool b = PortTouch::Colors();
  if (PortUi::Toggle("Coloured buttons", &b, "MP2_TOUCH_COLORS=1",
                     "the on-screen buttons in the GameCube pad's colours")) PortTouch::SetColors(b);
  b = PortTouch::Labels();
  if (PortUi::Toggle("Button descriptions", &b, "MP2_TOUCH_LABELS=0",
                     "what each on-screen button does, next to its letter")) PortTouch::SetLabels(b);
  b = PortTouch::Turbo();
  if (PortUi::Toggle("Turbo fire button", &b, "MP2_TOUCH_TURBO=1",
                     "a Turbo button next to Fire that fires as fast as the game accepts")) PortTouch::SetTurbo(b);
  b = PortTouch::FloatingStick();
  if (PortUi::Toggle("Floating left stick", &b, "MP2_TOUCH_FLOATING=0",
                     "the left stick appears under the finger on the left half of the screen")) PortTouch::SetFloatingStick(b);
  ImGui::BeginDisabled(!classic && !twin);
  b = PortTouch::WheelsEnabled();
  if (PortUi::Toggle("Beam and visor wheels", &b, "MP2_TOUCH_WHEELS=0",
                     "replaces the D-pad with a Visor and a Beam button")) PortTouch::SetWheelsEnabled(b);
  ImGui::EndDisabled();
  ImGui::BeginDisabled(!((!classic && !twin) || PortTouch::WheelsEnabled()));
  b = PortTouch::VisorTapScan();
  if (PortUi::Toggle("Tap Visor for Scan Visor", &b, "MP2_TOUCH_VISOR_TAP_SCAN=1",
                     "a quick tap on the Visor button selects the Scan Visor")) PortTouch::SetVisorTapScan(b);
  ImGui::EndDisabled();
  float speed = PortTouch::AimSpeed();
  if (PortUi::SliderFloat("Touch aim speed", &speed, 0.5f, 6.f, "%.2f px/dp", nullptr,
                          "MP2_TOUCH_AIM_SPEED=0.25..10")) PortTouch::SetAimSpeed(speed);
  b = PortTouch::SpringSwipeEnabled();
  if (PortUi::Toggle("Spring Ball on swipe up", &b, "MP2_TOUCH_SPRING_SWIPE=0",
                     "in morph ball, a quick swipe up on the free area springs; needs the Spring Ball option")) PortTouch::SetSpringSwipe(b);
  float v = PortTouch::Opacity();
  if (PortUi::SliderFloat("Opacity", &v, 10.f, 100.f, "%.0f%%", "MP2_TOUCH_OPACITY=10..100")) PortTouch::SetOpacity(v);
  v = PortTouch::SideMarginDp();
  if (PortUi::SliderFloat("Side margin", &v, 0.f, 300.f, "%.0f dp", "MP2_TOUCH_SIDE_MARGIN")) PortTouch::SetSideMarginDp(v);
  v = PortTouch::StickInsetDp();
  if (PortUi::SliderFloat("Stick inset", &v, 0.f, 300.f, "%.0f dp", "MP2_TOUCH_STICK_INSET")) PortTouch::SetStickInsetDp(v);
  v = PortTouch::ButtonInsetDp();
  if (PortUi::SliderFloat("Button inset", &v, 0.f, 300.f, "%.0f dp", "MP2_TOUCH_BUTTON_INSET")) PortTouch::SetButtonInsetDp(v);
  if (ImGui::Button("Reset margins")) {
    PortTouch::SetSideMarginDp(16.f);
    PortTouch::SetStickInsetDp(32.f);
    PortTouch::SetButtonInsetDp(0.f);
  }
  ImGui::EndDisabled();
  ImGui::SeparatorText("Gyro aim");
  ImGui::BeginDisabled();
  ImGui::TextUnformatted("Gyro aim (not in this port yet)");
  ImGui::EndDisabled();
}

// The Prime 1 port's Controls page (debug_ui/tabs.cpp's DrawControlsTab) with its five sub-tabs in
// its order. Each draws this port's rows for what MP1 files there; the rows MP2 has no backend
// for (button glyphs, pointer aim, rumble, mouse buttons, gyro) are not drawn, gyro as a note.
void DrawControlsSection() {
  if (ImGui::BeginTabBar("controlsPages")) {
    if (PortOverlaySearch::SubTab("Controls", "Options")) {
      DrawControlsOptions();
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Controls", "Mouse settings")) {
      DrawControlsMouse();
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Controls", "Controller settings")) {
      DrawControlsStickAim();
      const s32 controllerIndex = PADGetIndexForPort(PAD_CHAN0);
      if (SDL_Gamepad* pad =
              controllerIndex >= 0 ? PADGetSDLGamepadForIndex(static_cast< u32 >(controllerIndex)) : nullptr;
          pad != nullptr && SDL_IsJoystickVirtual(SDL_GetGamepadID(pad))) {
        // Aurora ignores mappings on the virtual touch pad.
        ImGui::TextWrapped("The touch controls always use the GameCube layout; presets and remaps apply to "
                           "real controllers only.");
      } else {
        PortControls::DrawDeadZones();
      }
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Controls", "Touch & gyro")) {
      DrawControlsTouchGyro();
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Controls", "Remap")) {
      PortInputRemap::Draw();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  // MP1's HotkeyHint (debug_ui/overlays.cpp): the port's own hotkeys, named by what is bound.
  const std::string menu = PortInputRemap::BindingsText(PortInput::Action::PortMenu, " / ");
  const std::string shot = PortInputRemap::BindingsText(PortInput::Action::Screenshot, " / ");
  ImGui::TextDisabled("%s: this window   %s: screenshot   F5-F10: the port's other hotkeys",
                      menu.empty() ? "unbound" : menu.c_str(), shot.empty() ? "unbound" : shot.c_str());
}

// The phone's window, as the Prime 1 port draws it (debug_ui/window.cpp's DrawPageWindow): full
// screen inside the display's safe area, no title bar, a Close button a thumb can hit, the settings
// search, and a page list down the side in place of a tab strip too narrow to tap. Returns false
// once Close is pressed.
[[maybe_unused]] bool DrawPageWindow() {
  using namespace PortOverlaySearch;
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImVec2 pos = viewport->WorkPos;
  ImVec2 size = viewport->WorkSize;
  SDL_Window* window = PortUi::MainWindow();
  SDL_Rect safe;
  int windowWidth = 0, windowHeight = 0;
  if (window != nullptr && SDL_GetWindowSafeArea(window, &safe) &&
      SDL_GetWindowSize(window, &windowWidth, &windowHeight) && windowWidth > 0 &&
      windowHeight > 0 && safe.w > 0 && safe.h > 0) {
    // ImGui's display size need not be in window coordinates.
    const float sx = size.x / static_cast< float >(windowWidth);
    const float sy = size.y / static_cast< float >(windowHeight);
    pos = ImVec2(pos.x + safe.x * sx, pos.y + safe.y * sy);
    size = ImVec2(safe.w * sx, safe.h * sy);
  }
  ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(size, ImGuiCond_Always);
  constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                      ImGuiWindowFlags_NoSavedSettings |
                                      ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoScrollWithMouse;
  bool open = true;
  if (ImGui::Begin("Metroid Prime 2 port##touch", nullptr, kFlags)) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const char* const kClose = "Close";
    const float closeWidth = ImGui::CalcTextSize(kClose).x + style.FramePadding.x * 4.f;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Metroid Prime 2 Port");
    ImGui::SameLine(0.f, style.ItemSpacing.x * 2.f);
    ImGui::TextDisabled("%.1f fps", static_cast< double >(sFps));
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - closeWidth);
    if (ImGui::Button(kClose, ImVec2(closeWidth, 0.f))) {
      open = false;
    }
    ImGui::Separator();
    DrawSearchField();

    static int sPage = 0;
    if (const int jumpPage = TakeJumpPage(); jumpPage >= 0) {
      sPage = jumpPage;
    }
    float listWidth = 0.f;
    for (int i = 0; i < kPageCount; ++i) {
      listWidth = std::max(listWidth, ImGui::CalcTextSize(kPages[i].name).x);
    }
    listWidth += style.FramePadding.x * 2.f + style.WindowPadding.x * 2.f;
    const float rowHeight = ImGui::GetFrameHeight() * 1.2f;
    if (ImGui::BeginChild("##pages", ImVec2(listWidth, 0.f), ImGuiChildFlags_Borders)) {
      for (int i = 0; i < kPageCount; ++i) {
        if (ImGui::Selectable(kPages[i].name, sPage == i && !Searching(), ImGuiSelectableFlags_None,
                              ImVec2(0.f, rowHeight))) {
          sPage = i;
          sSearch[0] = '\0';
        }
      }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    // Keyed by page, so each page keeps its own scroll position.
    const bool searching = Searching();
    ImGui::PushID(searching ? -1 : sPage);
    if (ImGui::BeginChild("##page", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders)) {
      // A drag that no item took scrolls the page, like a wheel (a finger has no wheel).
      if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
          !ImGui::IsAnyItemActive()) {
        const float dy = ImGui::GetIO().MouseDelta.y;
        if (dy > 0.f || dy < 0.f) {
          ImGui::SetScrollY(ImGui::GetScrollY() - dy);
        }
      }
      if (searching) {
        DrawSearchResults();
      } else {
        DrawPageBody(kPages[sPage]);
      }
    }
    ImGui::EndChild();
    ImGui::PopID();
  }
  ImGui::End();
  return open;
}

// The desktop's window, as the Prime 1 port draws it (debug_ui/window.cpp's DrawDesktopWindow): the
// settings search over a tab per page, its results in place of the tabs while there is a query.
[[maybe_unused]] void DrawDesktopWindow() {
  using namespace PortOverlaySearch;
  // Not movable: on the phone a drag on the window's empty space is how the list is scrolled (a
  // finger cannot turn a mouse wheel), and a window-drag would move it instead.
  if (ImGui::Begin("Metroid Prime 2 port", &sVisible, ImGuiWindowFlags_NoMove)) {
    ImGui::Text("%.1f fps", static_cast< double >(sFps));
    DrawSearchField();
    // A drag on the window that no item took: scroll by the drag, like a wheel would. An item
    // that holds the mouse (a slider, a checkbox press) keeps its own drag.
    if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        !ImGui::IsAnyItemActive()) {
      const float dy = ImGui::GetIO().MouseDelta.y;
      if (dy > 0.f || dy < 0.f) {
        ImGui::SetScrollY(ImGui::GetScrollY() - dy);
      }
    }
    if (Searching()) {
      if (ImGui::BeginChild("##search-results")) {
        DrawSearchResults();
      }
      ImGui::EndChild();
    } else {
      if (ImGui::BeginTabBar("##overlay_tabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
        const int jumpPage = TakeJumpPage();
        for (int i = 0; i < kPageCount; ++i) {
          if (ImGui::BeginTabItem(kPages[i].name, nullptr,
                                  i == jumpPage ? ImGuiTabItemFlags_SetSelected : 0)) {
            DrawPageBody(kPages[i]);
            ImGui::EndTabItem();
          }
        }
        ImGui::EndTabBar();
      }
      ImGui::SeparatorText("Hotkeys");
      ImGui::TextDisabled("F1 overlay, F5/F6 area, F7/F8 world");
    }
  }
  ImGui::End();
}

} // namespace

namespace PortOverlaySearch {

const Page kPages[] = {
    {"Game", DrawGameSection},         {"Controls", DrawControlsSection},
    {"Video", DrawVideoSection},       {"Cheats", DrawCheatsSection},
    {"System", DrawSystemSection},
};
const int kPageCount = static_cast< int >(std::size(kPages));

} // namespace PortOverlaySearch

namespace PortOverlay {

void SetGameState(uint32_t worldId, int areaId) {
  sWorldId = worldId;
  sAreaId = areaId;
}

void Toggle() {
  sVisible = !sVisible;
  // MP1's Toggle: the cursor goes back to the window (releases the mouse capture), and opening
  // drops any button or key ImGui still holds (a press whose release never came would make it
  // ignore every later click), as a focus loss would.
  PortMouse::SetUiActive(sVisible);
  if (sVisible && ImGui::GetCurrentContext() != nullptr) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(false);
    io.AddFocusEvent(true);
  }
}

void Wheel(float steps) { ImGui::GetIO().AddMouseWheelEvent(0.f, steps); }

bool Visible() {
  ReadEnv();
  return sVisible;
}

bool SkipCinematics() {
  ReadEnv();
  return EnvBool("MP2_SKIP_CINEMATICS", sSkipCinematics);
}

void SetSkipCinematics(bool skip) {
  ReadEnv();
  sSkipCinematics = skip;
  Persist();
}

bool HideHelmet() {
  ReadEnv();
  return EnvBool("MP2_NO_HELMET", sHideHelmet);
}

bool HideVisorEffects() {
  ReadEnv();
  return EnvBool("MP2_NO_VISOR_FX", sHideVisorEffects);
}

bool AimCrosshair() {
  ReadEnv();
  return EnvBool("MP2_AIM_CROSSHAIR", sAimCrosshair);
}

void SetAimCrosshair(bool enabled) {
  ReadEnv();
  sAimCrosshair = enabled;
  Persist();
}

int CrosshairStyle() {
  ReadEnv();
  const char* env = port::EnvString("MP2_CROSSHAIR_STYLE");
  if (env != nullptr && std::strcmp(env, "classic") == 0) {
    return kCrosshairStyle_Classic;
  }
  if (env != nullptr && std::strcmp(env, "remastered") == 0) {
    return kCrosshairStyle_Remastered;
  }
  return EnvIntIn("MP2_CROSSHAIR_STYLE", sCrosshairStyle, kCrosshairStyle_Classic, kCrosshairStyle_Remastered);
}

void SetCrosshairStyle(int style) {
  ReadEnv();
  sCrosshairStyle = std::clamp(style, kCrosshairStyle_Classic, kCrosshairStyle_Remastered);
  Persist();
}

int CrosshairSize() {
  ReadEnv();
  return EnvIntIn("MP2_CROSSHAIR_SIZE", sCrosshairSize, kCrosshairSizeMin, kCrosshairSizeMax);
}

void SetCrosshairSize(int percent) {
  ReadEnv();
  sCrosshairSize = std::clamp(percent, kCrosshairSizeMin, kCrosshairSizeMax);
  Persist();
}

int CrosshairOpacity() {
  ReadEnv();
  return EnvIntIn("MP2_CROSSHAIR_OPACITY", sCrosshairOpacity, kCrosshairOpacityMin, kCrosshairOpacityMax);
}

void SetCrosshairOpacity(int percent) {
  ReadEnv();
  sCrosshairOpacity = std::clamp(percent, kCrosshairOpacityMin, kCrosshairOpacityMax);
  Persist();
}

bool CrosshairColor(float rgb[3]) {
  ReadEnv();
  if (!EnvBool("MP2_CROSSHAIR_CUSTOM_COLOR", sCrosshairCustomColor)) {
    return false;
  }
  if (!EnvFloat3("MP2_CROSSHAIR_COLOR", rgb, 0.f, 1.f)) {
    std::copy_n(sCrosshairColor, 3, rgb);
  }
  return true;
}

void SetCrosshairColor(float r, float g, float b) {
  ReadEnv();
  sCrosshairColor[0] = std::clamp(r, 0.f, 1.f);
  sCrosshairColor[1] = std::clamp(g, 0.f, 1.f);
  sCrosshairColor[2] = std::clamp(b, 0.f, 1.f);
  sCrosshairCustomColor = true;
  Persist();
}

void ClearCrosshairColor() {
  ReadEnv();
  sCrosshairCustomColor = false;
  Persist();
}

float FirstPersonFov() {
  ReadEnv();
  const float env = port::EnvFloat("MP2_FOV", sFirstPersonFov);
  return env == 0.f || (env >= 30.f && env <= 120.f) ? env : sFirstPersonFov;
}

void GunOffset(float out[3]) {
  ReadEnv();
  if (EnvFloat3("MP2_GUN_OFFSET", out, -kGunOffsetMax, kGunOffsetMax)) {
    return;
  }
  out[0] = sGunOffset[0];
  out[1] = sGunOffset[1];
  out[2] = sGunOffset[2];
}

void SetGunOffset(const float offset[3]) {
  ReadEnv();
  StoreGunOffset(offset);
  Persist();
}

int Msaa() {
  ReadEnv();
  const int env = port::EnvInt("MP2_MSAA", sMsaa);
  return env == 1 || env == 4 ? env : sMsaa;
}

void SetMsaa(int samples) {
  ReadEnv();
  sMsaa = samples >= 4 ? 4 : 1;
  Persist();
}

int Anisotropy() {
  ReadEnv();
  return EnvIntIn("MP2_ANISO", sAnisotropy, 1, 16);
}

void SetAnisotropy(int maxAnisotropy) {
  ReadEnv();
  sAnisotropy = std::clamp(maxAnisotropy, 1, 16);
  Persist();
}

int HudScale() {
  ReadEnv();
  // 0 means the full-size HUD.
  const int env = port::EnvInt("MP2_HUD_SCALE", sHudScale);
  return env == 0 ? kHudScaleMax : (env >= kHudScaleMin && env <= kHudScaleMax ? env : sHudScale);
}

void SetHudScale(int percent) {
  ReadEnv();
  sHudScale = std::clamp(percent, kHudScaleMin, kHudScaleMax);
  Persist();
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

bool FastMorph() {
  ReadEnv();
  return EnvBool("MP2_FAST_MORPH", sFastMorph);
}

void SetFastMorph(bool enabled) {
  ReadEnv();
  sFastMorph = enabled;
  Persist();
}

bool SpringBall() {
  ReadEnv();
  return EnvBool("MP2_SPRING_BALL", sSpringBall);
}

void SetSpringBall(bool enabled) {
  ReadEnv();
  sSpringBall = enabled;
  Persist();
}

bool LockOnToggle() {
  ReadEnv();
  return EnvBool("MP2_LOCK_ON_TOGGLE", EnvBool("MP2_LOCKON_TOGGLE", sLockOnToggle));
}

void SetLockOnToggle(bool enabled) {
  ReadEnv();
  sLockOnToggle = enabled;
  Persist();
}

bool StickyCharge() {
  ReadEnv();
  return EnvBool("MP2_STICKY_CHARGE", sStickyCharge);
}

void SetStickyCharge(bool enabled) {
  ReadEnv();
  sStickyCharge = enabled;
  Persist();
}

bool RapidCharge() {
  ReadEnv();
  return EnvBool("MP2_RAPID_CHARGE", sRapidCharge);
}

void SetRapidCharge(bool enabled) {
  ReadEnv();
  sRapidCharge = enabled;
  Persist();
}

bool MouseButtons() {
  ReadEnv();
  return EnvBool("MP2_MOUSE_BUTTONS", sMouseButtons);
}

bool CardBackup() {
  ReadEnv();
  return EnvBool("MP2_CARD_BACKUP", sCardBackup);
}

void SetCardBackup(bool enabled) {
  ReadEnv();
  sCardBackup = enabled;
  Persist();
}

bool LogFile() {
  ReadEnv();
  return EnvBool("MP2_LOG_FILE", sLogFile);
}

void SetLogFile(bool enabled) {
  ReadEnv();
  sLogFile = enabled;
  Persist();
}

int UiScale() {
  ReadEnv();
  return EnvIntIn("MP2_UI_SCALE", sUiScale, kUiScaleMin, kUiScaleMax);
}

void SetUiScale(int percent) {
  ReadEnv();
  sUiScale = std::clamp(percent, kUiScaleMin, kUiScaleMax);
  Persist();
}

bool Vsync() {
  ReadEnv();
  return EnvBool("MP2_VSYNC", sVsync);
}

void SetVsync(bool enabled) {
  ReadEnv();
  sVsync = enabled;
  Persist();
}

void Draw() {
  TickFps();
  PortPhoto::Frame();
  // Even with the window closed: the env vars are settings, and a headless run never opens it.
  ApplyGraphicsQuality();
  ApplyVsync();
  // A row's change is written once no item is active (MP1's window.cpp), so a slider drag is one write.
  if (PortOverlaySettings::Dirty() && ImGui::GetCurrentContext() != nullptr && !ImGui::IsAnyItemActive()) {
    PortOverlaySettings::SaveSettings();
  }
  PortUi::UpdateControllerNav();
  // While the window is open the cursor belongs to it, not to the game's aim.
  PortMouse::SetUiActive(sVisible);
  if (!sVisible) {
    return;
  }
  PortUi::UpdateUiScale(static_cast< float >(PortOverlay::UiScale()) / 100.f);
  PortUi::ApplyThemeIfChanged();

  PortOverlaySearch::BeginSearchJump();
  if (PortUi::PageLayout()) {
    if (!DrawPageWindow()) {
      sVisible = false;
    }
  } else {
    DrawDesktopWindow();
  }
  PortOverlaySearch::EndSearchJump();
}

} // namespace PortOverlay
