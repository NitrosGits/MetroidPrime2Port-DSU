// The F1 debug overlay. It is deliberately small: every control here is wired to something that
// exists, so nothing in the window lies about what it does. The teleport buttons drive the same
// PortWarp requests as the F5-F8 hotkeys and the MP2_INPUT tokens, and the aim controls drive
// PortMouse, which the game reads every tick.
//
// Built by `mp_port_audio` (it needs ImGui, which Aurora brings in with SDL).

#include "port_overlay.h"

#include "port_binds.h"
#include "port_remap_ui.h"
#include "port_logfile.h"
#include "port_mouse.h"
#include "port_settings.h"
#include "port_ui_common.h"
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
// `MP2_LOG_FILE`: whether stdout and stderr also go to a file (F1 > Testing, console `logfile`).
// PortLogFile::Start reads it before Aurora comes up, so the row only reports the state the next
// launch will take.
bool sLogFile = true;
bool sGodMode = false; // never saved: a cheat must not persist, so it is not a registered setting
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

bool MsaaValid(int samples) { return samples == 1 || samples == 4; }

// Registers every setting; PortSettings resolves each as default, then <user>/settings.cfg, then its
// MP2_<KEY> environment variable (the variable wins). Cheats (god mode) are not settings.
void ReadEnv() {
  if (sReadEnv) {
    return;
  }
  sReadEnv = true;
  sGodMode = EnvFlag("MP2_GODMODE");
  PortSettings::AddBool("skip_cinematics", &sSkipCinematics, false);
  PortSettings::AddBool("fast_morph", &sFastMorph, false);
  PortSettings::AddBool("spring_ball", &sSpringBall, false);
  PortSettings::AddBool("lockon_toggle", &sLockOnToggle, false);
  PortSettings::AddBool("sticky_charge", &sStickyCharge, false);
  PortSettings::AddBool("card_backup", &sCardBackup, false);
  PortSettings::AddBool("log_file", &sLogFile, true);
  PortSettings::AddBool("no_helmet", &sHideHelmet, false);
  PortSettings::AddBool("no_visor_fx", &sHideVisorEffects, false);
  // 0 keeps the game's own FOV.
  PortSettings::AddFloat("fov", &sFirstPersonFov, 0.f, 30.f, 120.f, true);
  PortSettings::AddFloat3("gun_offset", sGunOffset, -PortOverlay::kGunOffsetMax,
                          PortOverlay::kGunOffsetMax);
  PortSettings::AddInt("msaa", &sMsaa, 1, 1, 4, MsaaValid);
  // A max, not a step: any value in range is kept, and Aurora clamps what a sampler asks for.
  PortSettings::AddInt("aniso", &sAnisotropy, 16, 1, 16);
  // 0 means the full-size HUD.
  PortSettings::AddInt("hud_scale", &sHudScale, PortOverlay::kHudScaleMax, PortOverlay::kHudScaleMin,
                       PortOverlay::kHudScaleMax, nullptr, PortOverlay::kHudScaleMax);
}

// A setter's change is saved at once (a no-op when nothing differs from the file).
void Persist() {
  PortSettings::NoteChanged();
  PortSettings::Save();
}

// A UI row's change is saved by Draw once the mouse is up, not on every frame of a slider drag.
bool Changed(bool changed) {
  if (changed) {
    PortSettings::NoteChanged();
  }
  return changed;
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
  PortUi::Help("MP2_WORLD=\"<world>[,<area>]\" and MP2_TELEPORT=\"<area>\" at start-up");
}

void DrawAimSection() {
  ImGui::SeparatorText("Aim");
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
  float sensitivity = PortMouse::Sensitivity();
  // Radians per pixel is a poor thing to read; degrees per pixel is not much better, so the
  // slider shows a 1x-100x multiplier over the default instead.
  float scale = sensitivity / 0.0035f;
  char sensText[64];
  std::snprintf(sensText, sizeof(sensText), "MP2_MOUSE_SENS=%.5f (radians per pixel)",
                static_cast< double >(PortMouse::Sensitivity()));
  if (PortUi::SliderFloat("Sensitivity", &scale, 0.1f, 5.f, "%.2fx", nullptr, sensText)) {
    PortMouse::SetSensitivity(0.0035f * scale);
  }

  bool twinStick = PortMouse::StickEnabled();
  if (PortUi::Toggle("Twin stick", &twinStick, "MP2_TWIN_STICK=1")) {
    PortMouse::SetStickEnabled(twinStick);
  }
  float rate = PortMouse::StickAimRate();
  if (PortUi::SliderFloat("Stick aim rate", &rate, 200.f, 2000.f, "%.0f px/s", nullptr,
                          "the right stick aims through the same path as the mouse")) {
    PortMouse::SetStickAimRate(rate);
  }
}

void DrawRenderSection() {
  ImGui::SeparatorText("Render");
  // Applied by CCameraManager::UpdateCameras: the first-person camera takes its FOV when it is
  // built, so the live change goes through CCameraManager::mFirstPersonFov.
  Changed(PortUi::SliderFloat("Field of view (1st person)", &sFirstPersonFov, 0.f, 120.f,
                              sFirstPersonFov <= 0.f ? "game (%.0f)" : "%.0f deg",
                              "MP2_FOV=<deg>; 0 keeps the game's 55"));
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
    StoreGunOffset(offset);
    Changed(true);
  }
  if (ImGui::Button("Reset arm cannon")) {
    const float zero[3] = {0.f, 0.f, 0.f};
    StoreGunOffset(zero);
    Changed(true);
  }
  PortUi::Note("MP2_GUN_OFFSET=\"right,forward,up\" in metres, +-1", nullptr, true);
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

void DrawHudSection() {
  ImGui::SeparatorText("HUD");
  Changed(PortUi::Toggle("Hide helmet", &sHideHelmet, "MP2_NO_HELMET=1"));
  Changed(PortUi::Toggle("Hide visor effects", &sHideVisorEffects, "MP2_NO_VISOR_FX=1",
                 "the scan visor's window and edges are what a visor effect draws here"));
  // Read by CGuiCamera::Draw while CSamusHud::Draw is drawing the HUD
  // (PortOverlay::HudScalingActive): the HUD's projection is scaled about the screen centre.
  Changed(PortUi::SliderInt("HUD scale", &sHudScale, PortOverlay::kHudScaleMin, PortOverlay::kHudScaleMax,
                    "%d%%", "MP2_HUD_SCALE=50..100",
                    "shrinks the HUD toward the middle; the helmet and menus keep their size"));
}

void DrawTestingSection() {
  ImGui::SeparatorText("Testing");
  // The game reads this every frame in its cinematic flow state.
  Changed(PortUi::Toggle("Skip cinematics", &sSkipCinematics, "MP2_SKIP_CINEMATICS=1",
                 "ends every cinematic at once, seen or not; makes tests 3-5x faster"));
  Changed(PortUi::Toggle("Log file", &sLogFile, "MP2_LOG_FILE=1",
                 "stdout and stderr go to metroid_prime2_port.log in the user folder; takes "
                 "effect next launch"));
}

void DrawGameplaySection() {
  ImGui::SeparatorText("Gameplay");
  Changed(PortUi::Toggle("Fast morph", &sFastMorph, "MP2_FAST_MORPH=1",
                 "the morph and unmorph transitions take 0.2 s instead of the game's second"));
  Changed(PortUi::Toggle("Spring Ball", &sSpringBall, "MP2_SPRING_BALL=1",
                 "C-stick up on the ground bomb-jumps the ball (needs Morph Ball Bombs)"));
  Changed(PortUi::Toggle("Toggle lock-on", &sLockOnToggle, "MP2_LOCK_ON_TOGGLE=1",
                 "an L press latches the lock until the next; one that ends lets go"));
  Changed(PortUi::Toggle("Sticky charge", &sStickyCharge, "MP2_STICKY_CHARGE=1",
                 "letting go of a long A hold keeps charging until the next press fires it"));
}

void DrawSavesSection() {
  ImGui::SeparatorText("Saves");
  Changed(PortUi::Toggle("Back up the card", &sCardBackup, "MP2_CARD_BACKUP=1",
                 "copies the save file into save-backups before each card write; `save import "
                 "latest` on the console puts one back"));
}

void DrawCheatsSection() {
  ImGui::SeparatorText("Cheats");
  PortUi::Toggle("God mode", &sGodMode, "MP2_GODMODE=1",
                 "the player takes no damage; other actors still do. Not saved");
}

// The window's size multiplier for fixed widths: the phone's themed scale, 1 on the desktop.
float UiScale() {
#if defined(__ANDROID__)
  return ImGui::GetIO().FontGlobalScale;
#else
  return 1.f;
#endif
}

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
  ImGui::SeparatorText("Bindings");
  // The Prime 1 port's remap page: one row per input with its key, a click-to-arm capture, a
  // per-row reset and a conflict marker. It captures on its own (SDL scancodes) and writes
  // keybinds.cfg through PortBinds, so the console's bind/binds stay in step.
  PortRemapUi::Draw();
  PortUi::Help("F1/F5-F10/F12 are the port's own hotkeys; they still fire when bound");
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

// The Prime 1 port's touch style and Prime theme, scaled to the display's density (about 3 on a
// phone), as its UpdateUiScale does; watches the scale each frame since it may not be known yet.
void ApplyTouchScale() {
  SDL_Window* window = UiWindow();
  if (window != nullptr) {
    // MP1 clamps the display scale to 1-4; on a 3x phone that makes the nine page rows taller than
    // the list, so Bindings sat below the fold and could not be tapped. 2 is the largest that
    // keeps the whole page list on screen here (and still thumb-sized).
    float scale = SDL_GetWindowDisplayScale(window);
    if (scale > 2.f) {
      scale = 2.f;
    }
    PortUi::ApplyTheme(scale);
  }
}

// Returns false once Close is pressed.
bool DrawPageWindow() {
  ApplyTouchScale();
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
  return sSkipCinematics;
}

void SetSkipCinematics(bool skip) {
  ReadEnv();
  sSkipCinematics = skip;
  Persist();
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
  StoreGunOffset(offset);
  Persist();
}

int Msaa() {
  ReadEnv();
  return sMsaa;
}

void SetMsaa(int samples) {
  ReadEnv();
  sMsaa = samples >= 4 ? 4 : 1;
  Persist();
}

int Anisotropy() {
  ReadEnv();
  return sAnisotropy;
}

void SetAnisotropy(int maxAnisotropy) {
  ReadEnv();
  sAnisotropy = std::clamp(maxAnisotropy, 1, 16);
  Persist();
}

int HudScale() {
  ReadEnv();
  return sHudScale;
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
  return sFastMorph;
}

void SetFastMorph(bool enabled) {
  ReadEnv();
  sFastMorph = enabled;
  Persist();
}

bool SpringBall() {
  ReadEnv();
  return sSpringBall;
}

void SetSpringBall(bool enabled) {
  ReadEnv();
  sSpringBall = enabled;
  Persist();
}

bool LockOnToggle() {
  ReadEnv();
  return sLockOnToggle;
}

void SetLockOnToggle(bool enabled) {
  ReadEnv();
  sLockOnToggle = enabled;
  Persist();
}

bool StickyCharge() {
  ReadEnv();
  return sStickyCharge;
}

void SetStickyCharge(bool enabled) {
  ReadEnv();
  sStickyCharge = enabled;
  Persist();
}

bool CardBackup() {
  ReadEnv();
  return sCardBackup;
}

void SetCardBackup(bool enabled) {
  ReadEnv();
  sCardBackup = enabled;
  Persist();
}

bool LogFile() {
  ReadEnv();
  return sLogFile;
}

void SetLogFile(bool enabled) {
  ReadEnv();
  sLogFile = enabled;
  Persist();
}

void Draw() {
  TickFps();
  // Even with the window closed: the env vars are settings, and a headless run never opens it.
  ApplyGraphicsQuality();
  // A row's change is written once the mouse is up, so a slider drag is one write.
  if (PortSettings::Dirty() && ImGui::GetCurrentContext() != nullptr && !ImGui::IsMouseDown(0)) {
    PortSettings::Save();
  }
  // While the window is open the cursor belongs to it, not to the game's aim.
  PortMouse::SetUiActive(sVisible);
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
    ImGui::SeparatorText("Hotkeys");
    ImGui::TextDisabled("F1 overlay, F5/F6 area, F7/F8 world");
  }
  ImGui::End();
}

} // namespace PortOverlay
