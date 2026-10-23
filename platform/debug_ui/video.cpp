// F1 overlay: The Video page's Display and Quality tabs. State and helpers shared
// with the other parts are in debug_ui_internal.h.

#include "debug_ui_internal.h"

#include "port_overlay.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>

namespace PortDebug {

void DrawVideoDisplay() {
  ImGui::SeparatorText("Display");
  bool fullscreen = sFullscreen;
#if defined(__ANDROID__)
  if (ImGui::Checkbox("Fullscreen (hide the status and navigation bars)", &fullscreen)) {
#else
  if (ImGui::Checkbox("Fullscreen (F11)", &fullscreen)) {
#endif
    PortOverlay::SetFullscreen(fullscreen);
  }
  // Applied by PortOverlay::Draw through aurora_enable_vsync, every frame it changes.
  Changed(ImGui::Checkbox("Vsync", &sVsync));
  ImGui::SetItemTooltip("Waits for the display's refresh before presenting a frame.");
  // Applied by ApplyAspect on the next loop iteration: the render mode is widened and the world and
  // GUI cameras follow its viewport.
  int aspect = sAspectMode;
  if (ImGui::Combo("Aspect ratio", &aspect, "4:3\0" "16:9\0" "Follow window\0")) {
    sAspectMode = aspect;
    Changed(true);
  }
  ImGui::SetItemTooltip("4:3 keeps the game's own picture; Follow window fills the window's shape.");

  ImGui::SeparatorText("HUD and view");
  bool hudWide = sHudWide;
  if (ImGui::Checkbox("Widescreen HUD (spread to edges)", &hudWide)) {
    PortOverlay::SetHudWide(hudWide);
  }
  ImGui::SetItemTooltip("Spreads the HUD to the screen edges on a wide screen. Off keeps its 4:3\n"
                        "proportions, centred.");
  // Read by CGuiCamera::Draw while CSamusHud::Draw is drawing the HUD
  // (PortOverlay::HudScalingActive): the HUD's projection is scaled about the screen centre.
  Changed(ImGui::SliderInt("HUD scale", &sHudScale, PortOverlay::kHudScaleMin, PortOverlay::kHudScaleMax, "%d%%"));
  ImGui::SetItemTooltip("Shrinks the HUD toward the middle; the helmet and menus keep their size.");
  if (sAimCrosshair) {
    Changed(ImGui::SliderInt("Crosshair opacity", &sCrosshairOpacity, PortOverlay::kCrosshairOpacityMin,
                             PortOverlay::kCrosshairOpacityMax, "%d%%"));
    ImGui::SetItemTooltip("The crosshair in the middle of the screen (mouse aim, twin stick, touch).");
    int style = sCrosshairStyle;
    static const char* const kStyles[] = {"Classic", "Remastered"};
    if (ImGui::Combo("Crosshair style", &style, kStyles, 2)) {
      sCrosshairStyle = style == PortOverlay::kCrosshairStyle_Remastered ? PortOverlay::kCrosshairStyle_Remastered
                                                                         : PortOverlay::kCrosshairStyle_Classic;
      Changed(true);
    }
    ImGui::SetItemTooltip("Remastered is the one this port can draw: the classic reticle's model does\n"
                          "not load here.");
  }
  Changed(ImGui::Checkbox("Hide helmet", &sHideHelmet));
  Changed(ImGui::Checkbox("Hide visor effects", &sHideVisorEffects));
  ImGui::SetItemTooltip("The scan visor's window and edges are what a visor effect draws here.");
  // Applied by CCameraManager::UpdateCameras: the first-person camera takes its FOV when it is
  // built, so the live change goes through CCameraManager::mFirstPersonFov. 0 is the game's own 55.
  float fov = sFirstPersonFov > 0.f ? sFirstPersonFov : 55.f;
  if (ImGui::SliderFloat("Field of view", &fov, 30.f, 120.f, "%.0f deg")) {
    sFirstPersonFov = std::isfinite(fov) ? fov : 0.f;
    Changed(true);
  }
  ImGui::SameLine();
  ImGui::BeginDisabled(sFirstPersonFov <= 0.f);
  if (ImGui::Button("Retail##fov")) {
    sFirstPersonFov = 0.f;
    Changed(true);
  }
  ImGui::EndDisabled();
  ImGui::SetItemTooltip("Back to the retail 55 deg.");
  {
    float offset[3] = {sGunOffset[0], sGunOffset[1], sGunOffset[2]};
    bool changed = ImGui::SliderFloat("Arm cannon right", &offset[0], -PortOverlay::kGunOffsetMax,
                                      PortOverlay::kGunOffsetMax, "%.2f m");
    changed |= ImGui::SliderFloat("Arm cannon forward", &offset[1], -PortOverlay::kGunOffsetMax,
                                  PortOverlay::kGunOffsetMax, "%.2f m");
    changed |= ImGui::SliderFloat("Arm cannon up", &offset[2], -PortOverlay::kGunOffsetMax,
                                  PortOverlay::kGunOffsetMax, "%.2f m");
    if (changed) {
      // Read every frame by CPlayer::UpdateArmAndGunTransforms, so the gun moves on the next update
      // and the shots leave from the muzzle that is drawn.
      StoreGunOffset(offset);
      Changed(true);
    }
    if (ImGui::Button("Retail##gunoffset")) {
      const float zero[3] = {0.f, 0.f, 0.f};
      StoreGunOffset(zero);
      Changed(true);
    }
    ImGui::SetItemTooltip("Back to the retail place.");
  }
}

void DrawVideoQuality() {
  ImGui::SeparatorText("Quality");
  // Applied by PortOverlay::Draw through aurora_set_graphics_quality: the framebuffers are rebuilt
  // at the next frame start.
  int msaa = sMsaa >= 4 ? 1 : 0;
  if (ImGui::Combo("Anti-aliasing", &msaa, "Off\0" "4x MSAA\0")) {
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
  if (ImGui::Combo("Anisotropic filtering", &aniso, "1x\0" "2x\0" "4x\0" "8x\0" "16x\0")) {
    sAnisotropy = kAnisoSteps[aniso];
    Changed(true);
  }
  // MP1's render scale, without its dynamic resolution. Applied by ApplyRenderScale from Draw.
  bool autoScale = sRenderScale <= 0.f;
  if (ImGui::Checkbox("Auto render scale (native)", &autoScale)) {
    sRenderScale = autoScale ? 0.f : 1.f;
    Changed(true);
  }
  ItemHelp("Draws at the window's own size.");
  if (!autoScale) {
    // Applied when the slider is let go: each new scale reallocates the EFB targets, which a drag
    // would otherwise do every frame.
    static float sPendingScale = 0.f;
    float scale = sPendingScale > 0.f ? sPendingScale : sRenderScale;
    if (ImGui::SliderFloat("EFB scale", &scale, 1.f, 4.f, "%.2fx")) {
      sPendingScale = scale;
    }
    ItemHelp("Scales the internal EFB from the 480-line base; above 2x it supersamples, and each step "
             "costs much more GPU time.");
    if (sPendingScale > 0.f && !ImGui::IsAnyItemActive()) {
      sRenderScale = sPendingScale;
      sPendingScale = 0.f;
      Changed(true);
    }
  }
}

} // namespace PortDebug
