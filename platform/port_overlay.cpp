// The F1 debug overlay. It is deliberately small: every control here is wired to something that
// exists, so nothing in the window lies about what it does. The teleport buttons drive the same
// PortWarp requests as the F5-F8 hotkeys and the MP2_INPUT tokens, and the aim controls drive
// PortMouse, which the game reads every tick.
//
// Built by `mp_port_audio` (it needs ImGui, which Aurora brings in with SDL).

#include "port_overlay.h"

#include "port_binds.h"
#include "port_env.h"
#include "port_overlay_internal.h"
#include "port_remap_ui.h"
#include "port_logfile.h"
#include "port_mouse.h"
#include "port_settings.h"
#include "port_ui_common.h"
#include "port_warp.h"

#include <SDL3/SDL.h>
#include <aurora/aurora.h>
#include <aurora/gfx.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <cstdlib>
#include <cstring>


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
  PortUi::BeginSection("Game");
  bool any = false;
  if (PortUi::Matches("World and area")) {
    ImGui::Text("world %08X  area %d", sWorldId, sAreaId);
  }
  bool rowStart = true;
  if (PortUi::Matches("Previous area (F6)")) {
    any = true;
    if (ImGui::Button("Previous area (F6)")) {
      PortWarp::CycleArea(-1);
    }
    rowStart = false;
  }
  if (PortUi::Matches("Next area (F5)")) {
    any = true;
    if (!rowStart) {
      ImGui::SameLine();
    }
    if (ImGui::Button("Next area (F5)")) {
      PortWarp::CycleArea(1);
    }
  }
  rowStart = true;
  if (PortUi::Matches("Previous world (F8)")) {
    any = true;
    if (ImGui::Button("Previous world (F8)")) {
      PortWarp::CycleWorld(-1);
    }
    rowStart = false;
  }
  if (PortUi::Matches("Next world (F7)")) {
    any = true;
    if (!rowStart) {
      ImGui::SameLine();
    }
    if (ImGui::Button("Next world (F7)")) {
      PortWarp::CycleWorld(1);
    }
  }
  if (any) {
    PortUi::Help("MP2_WORLD=\"<world>[,<area>]\" and MP2_TELEPORT=\"<area>\" at start-up");
  }
  PortUi::EndSection();
}

void DrawAimSection() {
  PortUi::BeginSection("Aim");
  bool mouseAim = PortMouse::Enabled();
  if (PortUi::Matches("Mouse aim") && PortUi::Toggle("Mouse aim", &mouseAim, "MP2_MOUSE_AIM=1")) {
    PortMouse::SetEnabled(mouseAim);
  }
  bool shownX = false;
  if (PortUi::Matches("Invert X")) {
    shownX = true;
    bool invertX = PortMouse::InvertX();
    if (ImGui::Checkbox("Invert X", &invertX)) {
      PortMouse::SetInvertX(invertX);
    }
  }
  if (PortUi::Matches("Invert Y")) {
    if (shownX) {
      ImGui::SameLine();
    }
    bool invertY = PortMouse::InvertY();
    if (ImGui::Checkbox("Invert Y", &invertY)) {
      PortMouse::SetInvertY(invertY);
    }
  }
  if (PortUi::Matches("Sensitivity")) {
    // Radians per pixel is a poor thing to read; degrees per pixel is not much better, so the
    // slider shows a 1x-100x multiplier over the default instead.
    float scale = PortMouse::Sensitivity() / 0.0035f;
    char sensText[64];
    std::snprintf(sensText, sizeof(sensText), "MP2_MOUSE_SENS=%.5f (radians per pixel)",
                  static_cast< double >(PortMouse::Sensitivity()));
    if (PortUi::SliderFloat("Sensitivity", &scale, 0.1f, 5.f, "%.2fx", nullptr, sensText)) {
      PortMouse::SetSensitivity(0.0035f * scale);
    }
  }

  bool twinStick = PortMouse::StickEnabled();
  if (PortUi::Matches("Twin stick") &&
      PortUi::Toggle("Twin stick", &twinStick, "MP2_TWIN_STICK=1")) {
    PortMouse::SetStickEnabled(twinStick);
  }
  float rate = PortMouse::StickAimRate();
  if (PortUi::Matches("Stick aim rate") &&
      PortUi::SliderFloat("Stick aim rate", &rate, 200.f, 2000.f, "%.0f px/s", nullptr,
                          "the right stick aims through the same path as the mouse")) {
    PortMouse::SetStickAimRate(rate);
  }
  PortUi::EndSection();
}

void DrawRenderSection() {
  PortUi::BeginSection("Render");
  // Applied by CCameraManager::UpdateCameras: the first-person camera takes its FOV when it is
  // built, so the live change goes through CCameraManager::mFirstPersonFov.
  if (PortUi::Matches("Field of view (1st person)")) {
    Changed(PortUi::SliderFloat("Field of view (1st person)", &sFirstPersonFov, 0.f, 120.f,
                                sFirstPersonFov <= 0.f ? "game (%.0f)" : "%.0f deg",
                                "MP2_FOV=<deg>; 0 keeps the game's 55"));
  }
  float offset[3] = {sGunOffset[0], sGunOffset[1], sGunOffset[2]};
  bool gunChanged = false;
  bool gunShown = false;
  static const char* const kGunLabels[3] = {"Arm cannon right", "Arm cannon forward",
                                            "Arm cannon up"};
  for (int i = 0; i < 3; ++i) {
    if (PortUi::Matches(kGunLabels[i])) {
      gunShown = true;
      gunChanged |= ImGui::SliderFloat(kGunLabels[i], &offset[i], -PortOverlay::kGunOffsetMax,
                                       PortOverlay::kGunOffsetMax, "%.2f m");
    }
  }
  if (gunChanged) {
    // Read every frame by CPlayer::UpdateArmAndGunTransforms, so the gun moves on the next update
    // and the shots leave from the muzzle that is drawn.
    StoreGunOffset(offset);
    Changed(true);
  }
  if (PortUi::Matches("Reset arm cannon")) {
    gunShown = true;
    if (ImGui::Button("Reset arm cannon")) {
      const float zero[3] = {0.f, 0.f, 0.f};
      StoreGunOffset(zero);
      Changed(true);
    }
  }
  if (gunShown) {
    PortUi::Note("MP2_GUN_OFFSET=\"right,forward,up\" in metres, +-1", nullptr, true);
  }
  int msaa = sMsaa >= 4 ? 1 : 0;
  // Applied by PortOverlay::Draw through aurora_set_graphics_quality: the framebuffers are rebuilt
  // at the next frame start.
  if (PortUi::Matches("Anti-aliasing") &&
      PortUi::Combo("Anti-aliasing", &msaa, "Off\0" "4x MSAA\0", "MP2_MSAA=1|4")) {
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
  if (PortUi::Matches("Anisotropic filtering") &&
      PortUi::Combo("Anisotropic filtering", &aniso, "1x\0" "2x\0" "4x\0" "8x\0" "16x\0",
                    "MP2_ANISO=1..16")) {
    sAnisotropy = kAnisoSteps[aniso];
    Changed(true);
  }
  // Applied by PortOverlay::Draw through aurora_enable_vsync, every frame it changes.
  if (PortUi::Matches("VSync")) {
    Changed(PortUi::Toggle("VSync", &sVsync, "MP2_VSYNC=1",
                           "waits for the display's refresh before presenting a frame"));
  }
  PortUi::EndSection();

  PortUi::BeginSection("Interface");
  // Multiplies the display's own scale (ApplyUiScale); the product is clamped to 1..4, so on a
  // desktop at scale 1 only values above 100% make the window larger.
  if (PortUi::Matches("UI scale")) {
    Changed(PortUi::SliderInt("UI scale", &sUiScale, PortOverlay::kUiScaleMin, PortOverlay::kUiScaleMax, "%d%%",
                              "MP2_UI_SCALE=50..200",
                              "the size of this window's text and controls"));
  }
  PortUi::EndSection();
}

void DrawHudSection() {
  PortUi::BeginSection("HUD");
  if (PortUi::Matches("Hide helmet")) {
    Changed(PortUi::Toggle("Hide helmet", &sHideHelmet, "MP2_NO_HELMET=1"));
  }
  if (PortUi::Matches("Hide visor effects")) {
    Changed(PortUi::Toggle("Hide visor effects", &sHideVisorEffects, "MP2_NO_VISOR_FX=1",
                 "the scan visor's window and edges are what a visor effect draws here"));
  }
  if (PortUi::Matches("Aim crosshair")) {
    Changed(PortUi::Toggle(
        "Aim crosshair", &sAimCrosshair, "MP2_AIM_CROSSHAIR=0",
        "the game's crosshair input is a look control the port never presses, so the port holds "
        "the reticle on while its own aim owns the view"));
  }
  if (sAimCrosshair) {
    int style = sCrosshairStyle;
    if (PortUi::Matches("Crosshair style") &&
        PortUi::Combo("Crosshair style", &style, "Classic\0" "Remastered\0",
                      "MP2_CROSSHAIR_STYLE=classic|remastered",
                      "Remastered is the one this port can draw: the classic reticle's model does "
                      "not load here")) {
      sCrosshairStyle = style == PortOverlay::kCrosshairStyle_Remastered
                            ? PortOverlay::kCrosshairStyle_Remastered
                            : PortOverlay::kCrosshairStyle_Classic;
      Changed(true);
    }
    if (PortUi::Matches("Crosshair size")) {
      Changed(PortUi::SliderInt("Crosshair size", &sCrosshairSize, PortOverlay::kCrosshairSizeMin,
                                PortOverlay::kCrosshairSizeMax, "%d%%",
                                "MP2_CROSSHAIR_SIZE=25..100",
                                "a percentage of the reticle's own scale, while the port is aiming"));
    }
    if (PortUi::Matches("Crosshair opacity")) {
      Changed(PortUi::SliderInt("Crosshair opacity", &sCrosshairOpacity,
                                PortOverlay::kCrosshairOpacityMin, PortOverlay::kCrosshairOpacityMax,
                                "%d%%", "MP2_CROSSHAIR_OPACITY=0..100"));
    }
  }
  // Read by CGuiCamera::Draw while CSamusHud::Draw is drawing the HUD
  // (PortOverlay::HudScalingActive): the HUD's projection is scaled about the screen centre.
  if (PortUi::Matches("HUD scale")) {
    Changed(PortUi::SliderInt("HUD scale", &sHudScale, PortOverlay::kHudScaleMin,
                              PortOverlay::kHudScaleMax, "%d%%", "MP2_HUD_SCALE=50..100",
                              "shrinks the HUD toward the middle; the helmet and menus keep "
                              "their size"));
  }
  PortUi::EndSection();
}

void DrawTestingSection() {
  PortUi::BeginSection("Testing");
  // The game reads this every frame in its cinematic flow state.
  if (PortUi::Matches("Skip cinematics")) {
    Changed(PortUi::Toggle("Skip cinematics", &sSkipCinematics, "MP2_SKIP_CINEMATICS=1",
                 "ends every cinematic at once, seen or not; makes tests 3-5x faster"));
  }
  if (PortUi::Matches("Log file")) {
    Changed(PortUi::Toggle("Log file", &sLogFile, "MP2_LOG_FILE=1",
                 "stdout and stderr go to metroid_prime2_port.log in the user folder; takes "
                 "effect next launch"));
  }
  PortUi::EndSection();
}

void DrawGameplaySection() {
  PortUi::BeginSection("Gameplay");
  if (PortUi::Matches("Fast morph")) {
    Changed(PortUi::Toggle("Fast morph", &sFastMorph, "MP2_FAST_MORPH=1",
                 "the morph and unmorph transitions take 0.2 s instead of the game's second"));
  }
  if (PortUi::Matches("Spring Ball")) {
    Changed(PortUi::Toggle("Spring Ball", &sSpringBall, "MP2_SPRING_BALL=1",
                 "C-stick up on the ground bomb-jumps the ball (needs Morph Ball Bombs)"));
  }
  if (PortUi::Matches("Toggle lock-on")) {
    Changed(PortUi::Toggle("Toggle lock-on", &sLockOnToggle, "MP2_LOCK_ON_TOGGLE=1",
                 "an L press latches the lock until the next; one that ends lets go"));
  }
  if (PortUi::Matches("Sticky charge")) {
    Changed(PortUi::Toggle("Sticky charge", &sStickyCharge, "MP2_STICKY_CHARGE=1",
                 "letting go of a long A hold keeps charging until the next press fires it"));
  }
  PortUi::EndSection();
}

void DrawSavesSection() {
  PortUi::BeginSection("Saves");
  if (PortUi::Matches("Back up the card")) {
    Changed(PortUi::Toggle("Back up the card", &sCardBackup, "MP2_CARD_BACKUP=1",
                 "copies the save file into save-backups before each card write; `save import "
                 "latest` on the console puts one back"));
  }
  PortUi::EndSection();
}

void DrawCheatsSection() {
  PortUi::BeginSection("Cheats");
  if (PortUi::Matches("God mode")) {
    PortUi::Toggle("God mode", &sGodMode, "MP2_GODMODE=1",
                   "the player takes no damage; other actors still do. Not saved");
  }
  PortUi::EndSection();
}

// The window's size multiplier for fixed widths: the window's UI scale (1 at 100% on a 1x display).
float UiScale() { return ImGui::GetIO().FontGlobalScale; }

// One row of the Bindings table: the pad input's name, then its key as a button that arms the row.
void DrawBindRow(int row) {
  const PortBinds::SInput& input = PortBinds::Input(row);
  const bool capturing = PortBinds::CapturingRow() == row;
  ImGui::TableNextColumn();
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(input.label);
  ImGui::TableNextColumn();
  const char* text = capturing ? "press a key..." : PortBinds::KeyName(PortBinds::Scancode(row));
  ImGui::PushID(row);
  if (capturing) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
  }
  if (ImGui::Button(text, ImVec2(150.f * UiScale(), 0.f))) {
    // A second click on the armed row is the way out of it without touching the keyboard.
    if (capturing) {
      PortBinds::CancelCapture();
    } else {
      PortBinds::BeginCapture(row);
    }
  }
  if (capturing) {
    ImGui::PopStyleColor();
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s = %s", input.action, text);
  }
  ImGui::PopID();
}

void DrawBindingsSection() {
  PortUi::BeginSection("Bindings");
  // The Prime 1 port's remap page: one row per input with its key, a click-to-arm capture, a
  // per-row reset and a conflict marker. It captures on its own (SDL scancodes) and writes
  // controls.toml through PortBinds, so the console's bind/binds stay in step.
  PortRemapUi::Draw();
  if (PortUi::Matches("Port hotkeys")) {
    PortUi::Help("F1/F5-F10/F12 are the port's own hotkeys; they still fire when bound");
  }
  PortUi::EndSection();
  return;
#if 0
  ImGui::SeparatorText("Bindings");
  PortUi::Help("click a key, then press the one you want (Escape cancels)");
  PortUi::Help("a mouse button is `bind A MOUSE_LEFT` on the console; this takes keys only");
  // The buttons on the left, the stick inputs on the right: two pad inputs a row keeps 22 rows
  // from filling the window.
  int buttonRows = 0;
  for (int i = 0; i < PortBinds::Count(); ++i) {
    buttonRows += PortBinds::Input(i).axis ? 0 : 1;
  }
  const ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg;
  if (ImGui::BeginTable("bindings", 4, flags)) {
    ImGui::TableSetupColumn("pad input", ImGuiTableColumnFlags_WidthFixed, 120.f * UiScale());
    ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthFixed, 170.f * UiScale());
    ImGui::TableSetupColumn("pad input", ImGuiTableColumnFlags_WidthFixed, 120.f * UiScale());
    ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthFixed, 170.f * UiScale());
    for (int i = 0; i < buttonRows; ++i) {
      ImGui::TableNextRow();
      DrawBindRow(i);
      if (buttonRows + i < PortBinds::Count()) {
        DrawBindRow(buttonRows + i);
      }
    }
    ImGui::EndTable();
  }
  if (ImGui::Button("Reset to defaults")) {
    PortBinds::Reset();
  }
  ImGui::SameLine();
  const char* path = PortBinds::Path();
  ImGui::TextDisabled("%s", *path != '\0' ? path : "(no user directory: not saved)");
  PortUi::Help("F1/F5-F10/F12 are the port's own hotkeys; they still fire when bound");
#endif
}

SDL_Window* UiWindow() {
  static SDL_Window* sWindow = nullptr;
  if (sWindow == nullptr) {
    int count = 0;
    if (SDL_Window** windows = SDL_GetWindows(&count)) {
      if (count > 0) {
        sWindow = windows[0];
      }
      SDL_free(windows);
    }
  }
  return sWindow;
}

// The window's scale: the display's own (capped at 2 on the phone, where 3 made the nine page rows
// taller than the list and Bindings sat below the fold; 2 keeps the page list on screen and still
// thumb-sized) times the `ui_scale` setting. Watched each frame since the display scale may not be
// known yet; ApplyTheme restyles only when the product changes and clamps it to 1..4.
void ApplyUiScale() {
  SDL_Window* window = UiWindow();
  if (window == nullptr) {
    return;
  }
  float base = SDL_GetWindowDisplayScale(window);
  if (!(base > 0.f)) {
    base = 1.f;
  }
#if defined(__ANDROID__)
  base = std::min(base, 2.f);
#endif
  PortUi::ApplyTheme(base * static_cast< float >(PortOverlay::UiScale()) / 100.f);
}

// The "Filter settings" box above the sections (desktop) or the page list (phone).
void DrawFilterBox() {
  char text[128];
  std::snprintf(text, sizeof(text), "%s", PortUi::Filter());
  ImGui::SetNextItemWidth(-FLT_MIN);
  if (ImGui::InputTextWithHint("##filter", "Filter settings", text, sizeof(text))) {
    PortUi::SetFilter(text);
  }
}

#if defined(__ANDROID__)
// The phone's window, as the Prime 1 port draws it: full screen inside the display's safe area,
// no title bar, a Close button a thumb can hit, and a page list down the side in place of a
// scroll of nine sections.
struct Page {
  const char* name;
  void (*draw)();
};
const Page kPages[] = {
    {"Game", DrawGameSection},         {"Aim", DrawAimSection},
    {"Render", DrawRenderSection},     {"HUD", DrawHudSection},
    {"Testing", DrawTestingSection},   {"Gameplay", DrawGameplaySection},
    {"Saves", DrawSavesSection},       {"Cheats", DrawCheatsSection},
    {"Bindings", DrawBindingsSection},
};
constexpr int kPageCount = sizeof(kPages) / sizeof(kPages[0]);

// Returns false once Close is pressed.
bool DrawPageWindow() {
  ApplyUiScale();
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImVec2 pos = viewport->WorkPos;
  ImVec2 size = viewport->WorkSize;
  SDL_Window* window = UiWindow();
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
    DrawFilterBox();

    static int sPage = 0;
    float listWidth = 0.f;
    for (int i = 0; i < kPageCount; ++i) {
      listWidth = std::max(listWidth, ImGui::CalcTextSize(kPages[i].name).x);
    }
    listWidth += style.FramePadding.x * 2.f + style.WindowPadding.x * 2.f;
    const float rowHeight = ImGui::GetFrameHeight() * 1.2f;
    if (ImGui::BeginChild("##pages", ImVec2(listWidth, 0.f), ImGuiChildFlags_Borders)) {
      for (int i = 0; i < kPageCount; ++i) {
        if (ImGui::Selectable(kPages[i].name, sPage == i, ImGuiSelectableFlags_None,
                              ImVec2(0.f, rowHeight))) {
          sPage = i;
        }
      }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    // Keyed by page, so each page keeps its own scroll position.
    ImGui::PushID(sPage);
    if (ImGui::BeginChild("##page", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders)) {
      // A drag that no item took scrolls the page, like a wheel (a finger has no wheel).
      if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
          !ImGui::IsAnyItemActive()) {
        const float dy = ImGui::GetIO().MouseDelta.y;
        if (dy > 0.f || dy < 0.f) {
          ImGui::SetScrollY(ImGui::GetScrollY() - dy);
        }
      }
      kPages[sPage].draw();
    }
    ImGui::EndChild();
    ImGui::PopID();
  }
  ImGui::End();
  return open;
}
#endif

} // namespace

namespace PortOverlay {

void SetGameState(uint32_t worldId, int areaId) {
  sWorldId = worldId;
  sAreaId = areaId;
}

void Toggle() { sVisible = !sVisible; }

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
  // Even with the window closed: the env vars are settings, and a headless run never opens it.
  ApplyGraphicsQuality();
  ApplyVsync();
  // A row's change is written once the mouse is up, so a slider drag is one write.
  if (PortOverlaySettings::Dirty() && ImGui::GetCurrentContext() != nullptr && !ImGui::IsMouseDown(0)) {
    PortOverlaySettings::SaveSettings();
  }
  // While the window is open the cursor belongs to it, not to the game's aim.
  PortMouse::SetUiActive(sVisible);
  if (sVisible) {
    ApplyUiScale();
  }
  if (!sVisible) {
    // A row armed in the table belongs to the window: closing it disarms, so a key pressed later in
    // the game is not silently bound to the row that was waiting.
    PortBinds::CancelCapture();
    return;
  }

#if defined(__ANDROID__)
  if (!DrawPageWindow()) {
    sVisible = false;
  }
  return;
#endif
  // Not movable: on the phone a drag on the window's empty space is how the list is scrolled (a
  // finger cannot turn a mouse wheel), and a window-drag would move it instead.
  if (ImGui::Begin("Metroid Prime 2 port", &sVisible, ImGuiWindowFlags_NoMove)) {
    ImGui::Text("%.1f fps", static_cast< double >(sFps));
    DrawFilterBox();
    // A drag on the window that no item took: scroll by the drag, like a wheel would. An item
    // that holds the mouse (a slider, a checkbox press) keeps its own drag.
    if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        !ImGui::IsAnyItemActive()) {
      const float dy = ImGui::GetIO().MouseDelta.y;
      if (dy > 0.f || dy < 0.f) {
        ImGui::SetScrollY(ImGui::GetScrollY() - dy);
      }
    }
    DrawGameSection();
    DrawAimSection();
    DrawRenderSection();
    DrawHudSection();
    DrawTestingSection();
    DrawGameplaySection();
    DrawSavesSection();
    DrawCheatsSection();
    DrawBindingsSection();
    PortUi::BeginSection("Hotkeys");
    if (PortUi::Matches("Hotkeys F1 overlay, F5/F6 area, F7/F8 world")) {
      ImGui::TextDisabled("F1 overlay, F5/F6 area, F7/F8 world");
    }
    PortUi::EndSection();
  }
  ImGui::End();
}

} // namespace PortOverlay
