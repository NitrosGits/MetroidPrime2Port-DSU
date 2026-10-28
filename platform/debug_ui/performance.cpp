// F1 overlay: The Performance tab (Video > Frame rate). State and helpers shared with the other
// parts are in debug_ui_internal.h.

#include "debug_ui_internal.h"

#include "port_overlay.h"
#include "port_video.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace PortDebug {

void DrawPerformanceTab() {
  ImGui::SeparatorText("Frame rate");
  // Paced by PortPaceFrame at the top of each main-loop iteration; F10 toggles it.
  bool frameLimit = sFrameLimit;
  if (ImGui::Checkbox("60 FPS cap (target)", &frameLimit)) {
    PortOverlay::SetFrameLimit(frameLimit);
  }
  ItemHelp("Holds the game to 60 frames a second, the rate it was written for (F10).");
  // Read by PortPaceFrame with the 60 FPS cap off; the top notch is Unlimited (0).
  ImGui::BeginDisabled(sFrameLimit);
  int cap = sFpsCap > 0 ? sFpsCap : PortOverlay::kFpsCapMax + 1;
  if (ImGui::SliderInt("FPS cap", &cap, PortOverlay::kFpsCapMin, PortOverlay::kFpsCapMax + 1,
                       cap > PortOverlay::kFpsCapMax ? "Unlimited" : "%d FPS", ImGuiSliderFlags_AlwaysClamp)) {
    sFpsCap = cap > PortOverlay::kFpsCapMax ? 0 : PortOverlay::ClampFpsCap(cap);
    PortVideo::ResetDynamicRes();
    Changed(true);
  }
  ImGui::EndDisabled();
  ItemHelp("The most frames drawn a second with the 60 FPS cap off. Vsync still limits it to the "
           "display's refresh rate.");
  ImGui::Text("Presented %.1f FPS    Frame %.2f ms", sFps,
              static_cast< double >(ImGui::GetIO().DeltaTime) * 1000.0);
  ImGui::Text("Throughput %.1f FPS", static_cast< double >(sThroughputFps));
  ItemHelp("Frames a second the machine produces with the pacing wait left out. Presented is what "
           "reaches the screen; the gap between them is the headroom at the current frame cost.");

  // Read by the main loop's draw: with the cap on it changes nothing.
  ImGui::BeginDisabled(sFrameLimit);
  // One switch for the camera and the three parts read beside it (CAnimData::PortPresentedPose,
  // CElementGen::PortBeginPresent / CEnvFxManager::Render, CCameraManager::GetCurrentCameraTransform);
  // their keys stay separate in settings.cfg.
  bool smooth = sSmoothFrames && sPoseInterpolation && sParticleInterpolation && sLookInterpolation;
  if (ImGui::Checkbox("Smooth frames", &smooth)) {
    PortOverlay::SetSmoothFrames(smooth);
    PortOverlay::SetPoseInterpolation(smooth);
    PortOverlay::SetParticleInterpolation(smooth);
    PortOverlay::SetLookInterpolation(smooth);
  }
  ItemHelp("With the FPS cap off, draws the camera, animated models, particles and weather between "
           "the game's 60 Hz ticks, and turns the view between them as the mouse, stick or touch "
           "aims. Aim and shots are unchanged.");
  ImGui::EndDisabled();
}

} // namespace PortDebug
