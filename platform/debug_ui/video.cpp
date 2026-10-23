// F1 overlay: The Video page's Display, Quality and Compatibility tabs, and the shader-compilation
// and F1-hint toasts. State and helpers shared
// with the other parts are in debug_ui_internal.h.

#include "debug_ui_internal.h"

#include "port_input_remap_ui.h"
#include "port_overlay.h"
#include "port_video.h"

#include <aurora/aurora.h>
#include <aurora/gfx.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>

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
  // The two toasts are drawn by DrawUI (overlays.cpp), with the panel open or closed.
  Changed(ImGui::Checkbox("Show shader compilation", &sShowShaderCompilation));
  ImGui::SetItemTooltip("A small progress bar while the shader cache compiles at startup (longest on the first\n"
                        "start after an install or update). Draws whose shader isn't ready yet are skipped.");
  Changed(ImGui::Checkbox("Show F1 hint at launch", &sShowMenuHint));
  ImGui::SetItemTooltip("A note in the top left corner for the first seconds of each launch saying how to\n"
                        "open this panel.");
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
  // Applied by PortVideo::Frame through aurora_set_per_pixel_lighting, on change.
  Changed(ImGui::Checkbox("Per-pixel lighting", &sPerPixelLighting));
  ImGui::SetItemTooltip("Sums the game's lights per pixel instead of per vertex, like Dolphin: smoother\n"
                        "on low-poly models, not the original look.");
  // Applied by ApplyRenderScale from Draw, through PortVideo::DrawScale (dynamic resolution's while
  // it has lowered the scale).
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
      PortVideo::ResetDynamicRes();
      Changed(true);
    }
    // Run by PortVideo::Frame once a second (port_dynamic_res.cpp decides the steps).
    if (Changed(ImGui::Checkbox("Dynamic resolution", &sDynamicRes))) {
      PortVideo::ResetDynamicRes();
    }
    ImGui::SetItemTooltip("Lowers the EFB scale (down to the lowest scale below) while the GPU can't hold\n"
                          "the target frame rate, and raises it back when there is room. Each change\n"
                          "stutters for a frame, so it moves only when that is worth it.");
    if (sDynamicRes) {
      static const int kTargets[] = {0, 30, 60, 90, 120};
      int target = 0;
      for (int i = 0; i < 5; ++i) {
        if (kTargets[i] == sDynamicResTarget) {
          target = i;
        }
      }
      ImGui::SameLine();
      ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7.f);
      if (ImGui::Combo("Target", &target, "Display\0" "30 fps\0" "60 fps\0" "90 fps\0" "120 fps\0")) {
        sDynamicResTarget = kTargets[target];
        PortVideo::ResetDynamicRes();
        Changed(true);
      }
      ImGui::SetItemTooltip("Display: the screen's refresh rate, or 60 with the 60 FPS cap on.");
      static const float kLowest[] = {0.5f, 0.75f, 1.f};
      int lowest = 2;
      for (int i = 0; i < 3; ++i) {
        if (kLowest[i] == sDynamicResMin) {
          lowest = i;
        }
      }
      ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7.f);
      if (ImGui::Combo("Lowest scale", &lowest, "0.5x\0" "0.75x\0" "1x\0")) {
        sDynamicResMin = kLowest[lowest];
        PortVideo::ResetDynamicRes();
        Changed(true);
      }
      ImGui::SetItemTooltip("Below 1x it draws fewer pixels than the GameCube did and looks softer.");
      const float dynScale = PortVideo::DynamicScale();
      ImGui::Text("Drawing at %.2fx (target %.0f fps)", dynScale > 0.f ? dynScale : sRenderScale,
                  PortVideo::DynamicResTargetFps());
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
      const char* reason = PortVideo::DynamicResReason();
      const double gpuMs = PortVideo::DynamicResGpuMs();
      if (PortVideo::DynamicResGpuTimesDropped()) {
        ImGui::Text("GPU times not used (none arrived, or lowering on them gained nothing): %s", reason);
      } else if (gpuMs > 0.0) {
        ImGui::Text("GPU %.1f of %.1f ms a frame: %s", gpuMs, 1000.0 / PortVideo::DynamicResTargetFps(), reason);
      } else if (gpuMs < 0.0) {
        ImGui::Text("No GPU times (unsupported, or MP2_DYNRES_GPU_TIMES=0); following the frame rate: %s", reason);
      } else {
        ImGui::Text("%s", reason);
      }
      ImGui::PopStyleColor();
    }
    // Applied by PortVideo::ApplyPresentSharpness, on change.
    int sharpen = sSharpenMode;
    if (ImGui::Combo("Sharpen when upscaled", &sharpen, "Off\0" "With dynamic resolution\0" "Always\0")) {
      sSharpenMode = sharpen;
      PortVideo::ApplyPresentSharpness();
      Changed(true);
    }
    ImGui::SetItemTooltip("Sharpens a frame drawn smaller than the window, so a lowered scale looks less\n"
                          "soft. With dynamic resolution: only while it has lowered the scale. Always:\n"
                          "also a fixed low scale on a large window.");
    if (sSharpenMode != 0) {
      if (Changed(ImGui::SliderInt("Sharpening", &sSharpenAmount, 0, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp))) {
        PortVideo::ApplyPresentSharpness();
      }
    }
  }
}

// What PortVideo::BeginGpuStart sets up before the GPU device is made (so after a restart), and the
// GPU self-test. MP1's Renderer combo (Windows) and Vulkan driver picker (Android) are not here.
void DrawVideoCompatibility() {
  ImGui::SeparatorText("Compatibility");
  ImGui::PushTextWrapPos(0.f);
  ImGui::TextDisabled("For GPU driver problems, such as a black world or a crash at start. Most of "
                      "these take effect after a restart.");
  ImGui::PopTextWrapPos();
  size_t backendCount = 0;
  const AuroraBackend* backends = aurora_get_available_backends(&backendCount);
  if (std::find(backends, backends + backendCount, BACKEND_OPENGLES) != backends + backendCount) {
    Changed(ImGui::Checkbox("Use OpenGL ES", &sOpenGles));
    ImGui::SetItemTooltip("Renders through OpenGL ES instead of Vulkan. Try it if the world draws black\n"
                          "or untextured (some Adreno drivers). Takes effect after a restart; if the\n"
                          "driver crashes starting it, the next start goes back to Vulkan.");
    if (sOpenGles != sOpenGlesAtStart) {
      ImGui::SameLine();
      ImGui::TextColored(ThemeWarnColor(), "Restart to apply");
    } else if (sOpenGles && aurora_get_backend() != BACKEND_OPENGLES) {
      ImGui::SameLine();
      ImGui::TextColored(ThemeWarnColor(), "OpenGL ES failed to start");
    }
  }
  {
    static const char* const kClampNames[] = {"Auto", "Off", "On"};
    int clamp = sStorageClamp + 1;
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.f);
    if (ImGui::Combo("Adreno shader fix", &clamp, kClampNames, 3)) {
      sStorageClamp = clamp - 1;
      Changed(true);
    }
    ImGui::SetItemTooltip("Reads GPU buffers without the bounds check some Adreno Vulkan drivers\n"
                          "miscompile (black world). Auto turns it on for Adreno 7xx; try On\n"
                          "on another Adreno with a black world. Restart to apply.");
    if (sStorageClamp != sStorageClampAtStart) {
      ImGui::SameLine();
      ImGui::TextColored(ThemeWarnColor(), "Restart to apply");
    }
  }
  // Aurora reads MP2_GPU_POWER=low when it picks the adapter; MP1 has only the variable.
  Changed(ImGui::Checkbox("Low-power GPU", &sLowPowerGpu));
  ImGui::SetItemTooltip("Asks for the integrated GPU on a machine that also has a discrete one (longer\n"
                        "battery, less heat, lower frame rate). Restart to apply.");
  if (sLowPowerGpu != sLowPowerGpuAtStart) {
    ImGui::SameLine();
    ImGui::TextColored(ThemeWarnColor(), "Restart to apply");
  }
  {
    const int ticked = std::popcount(sDriverWorkarounds);
    char header[64];
    std::snprintf(header, sizeof(header),
                  ticked > 0 ? "Driver workarounds (%d on)###driver-workarounds" : "Driver workarounds###driver-workarounds",
                  ticked);
    const bool open = ImGui::TreeNode(header);
    if (sDriverWorkarounds != sDriverWorkaroundsAtStart) {
      ImGui::SameLine();
      ImGui::TextColored(ThemeWarnColor(), "Restart to apply");
    }
    if (open) {
      ImGui::PushTextWrapPos(0.f);
      ImGui::TextDisabled("For a GPU that draws wrong (flickering or missing geometry). Tick one, "
                          "restart, then run the GPU self-test below; leave them off otherwise. If "
                          "a start with them fails, the next start turns them off.");
      if (PortVideo::DriverWorkaroundsCleared()) {
        ImGui::TextColored(ThemeWarnColor(), "Turned off: the last start with them did not reach the game.");
      }
      if (PortVideo::DriverWorkaroundsEnv() != nullptr) {
        ImGui::TextDisabled("MP2_DRIVER_WORKAROUNDS=%s replaces these boxes this run.",
                            PortVideo::DriverWorkaroundsEnv());
      }
      if (PortVideo::DriverChecksThisRun()) {
        ImGui::TextColored(ThemeWarnColor(), "Driver error checks are on this run (slower); the next start "
                                             "turns them off.");
      }
      ImGui::PopTextWrapPos();
      for (size_t i = 0; i < PortVideo::kDriverWorkaroundCount; ++i) {
        bool on = (sDriverWorkarounds & (1u << i)) != 0;
        if (ImGui::Checkbox(PortVideo::kDriverWorkarounds[i].label, &on)) {
          sDriverWorkarounds = on ? sDriverWorkarounds | (1u << i) : sDriverWorkarounds & ~(1u << i);
          Changed(true);
        }
        ImGui::SetItemTooltip("%s", PortVideo::kDriverWorkarounds[i].tooltip);
      }
      ImGui::TreePop();
    }
  }
  {
    // Run by CGraphics::BeginScene (PortVideo::RunGpuSelfTestIfRequested).
    const bool pending = aurora_gpu_selftest_pending();
    ImGui::BeginDisabled(pending);
    if (ImGui::Button("GPU self-test")) {
      PortVideo::RequestGpuSelfTest();
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Renders test patterns offscreen and logs one PASS or FAIL line per feature\n"
                          "(vertices, TEV, textures, blend, depth, EFB copy), with checks that say which\n"
                          "stage fails. For GPU driver bugs such as a black world; attach the log to a report.");
    char summary[160];
    if (pending) {
      ImGui::SameLine();
      ImGui::TextDisabled("running...");
    } else if (aurora_gpu_selftest_summary(summary, sizeof(summary)) != 0) {
      ImGui::SameLine();
      ImGui::TextUnformatted(summary);
    }
  }
}

// While the startup compile runs (the pipeline cache, queued before the first frame): after half a
// second, a count and a bar since the toast opened. Once that queue has drained, shaders met in
// game compile without it. From MP1's overlays.cpp, top centre.
void DrawShaderCompilationToast() {
  static double sQueuedSince = -1.0;
  static uint32_t sBase = 0; // createdPipelines when the toast opened
  static bool sStartupDone = false;
  const AuroraStats* stats = aurora_get_stats();
  if (stats != nullptr && stats->queuedPipelines == 0) {
    sStartupDone = true;
  }
  if (!PortOverlay::ShowShaderCompilation() || stats == nullptr || sStartupDone) {
    sQueuedSince = -1.0;
    return;
  }
  const double now = ImGui::GetTime();
  if (sQueuedSince < 0.0) {
    sQueuedSince = now;
    sBase = stats->createdPipelines;
  }
  if (now - sQueuedSince < 0.5) {
    return;
  }
  const uint32_t done = stats->createdPipelines >= sBase ? stats->createdPipelines - sBase : 0;
  const uint32_t total = done + stats->queuedPipelines;
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + 16.f),
                          ImGuiCond_Always, ImVec2(0.5f, 0.f));
  ImGui::SetNextWindowBgAlpha(0.7f);
  if (ImGui::Begin("##shader-compilation-toast", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                       ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings |
                       ImGuiWindowFlags_AlwaysAutoResize)) {
    char label[64];
    std::snprintf(label, sizeof(label), "Compiling shaders %u / %u", total, total); // widest, so the count doesn't jitter
    const float width = ImGui::CalcTextSize(label).x;
    std::snprintf(label, sizeof(label), "Compiling shaders %u / %u", done, total);
    ImGui::TextUnformatted(label);
    ImGui::ProgressBar(total != 0 ? static_cast< float >(done) / static_cast< float >(total) : 0.f,
                       ImVec2(width, std::max(6.f, ImGui::GetFontSize() * 0.4f)), "");
  }
  ImGui::End();
}

// Once per launch, for 8 s, until the window is first opened: how to open it. Top left.
void DrawMenuHintToast() {
  static double sShownAt = -1.0;
  static bool sDone = false;
  if (sDone || !PortOverlay::ShowMenuHint()) {
    return;
  }
  if (sVisible) {
    sDone = true;
    return;
  }
  const double now = ImGui::GetTime();
  if (sShownAt < 0.0) {
    sShownAt = now;
  }
  const double age = now - sShownAt;
  if (age > 8.0) {
    sDone = true;
    return;
  }
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 16.f, viewport->Pos.y + 16.f), ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.7f);
  // Fades out over the last second.
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, static_cast< float >(std::clamp(8.0 - age, 0.0, 1.0)));
  if (ImGui::Begin("##menu-hint-toast", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                       ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings |
                       ImGuiWindowFlags_AlwaysAutoResize)) {
    const std::string keys = PortInputRemap::BindingsText(PortInput::Action::PortMenu);
    if (keys.empty()) {
      ImGui::TextUnformatted("Open settings from the Port menu binding in Remap");
    } else {
      ImGui::Text("Press %s for settings", keys.c_str());
    }
  }
  ImGui::End();
  ImGui::PopStyleVar();
}

} // namespace PortDebug
