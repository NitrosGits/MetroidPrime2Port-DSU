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

  // Read by the main loop's draw: with the cap on it changes nothing.
  ImGui::BeginDisabled(sFrameLimit);
  bool smooth = sSmoothFrames;
  if (ImGui::Checkbox("Smooth uncapped frames", &smooth)) {
    PortOverlay::SetSmoothFrames(smooth);
  }
  ItemHelp("With the FPS cap off, draws the camera between the game's 60 Hz ticks. Aim and shots "
           "are unchanged.");
  // Each needs smooth frames, so it changes nothing without them.
  ImGui::BeginDisabled(!sSmoothFrames);
  // Read by CAnimData::PortPresentedPose.
  bool pose = sPoseInterpolation;
  if (ImGui::Checkbox("Smooth animation", &pose)) {
    PortOverlay::SetPoseInterpolation(pose);
  }
  ItemHelp("With smooth frames, also draws animated models between the game's ticks.");
  // Read by CElementGen::PortBeginPresent and CEnvFxManager::Render.
  bool particles = sParticleInterpolation;
  if (ImGui::Checkbox("Smooth effects", &particles)) {
    PortOverlay::SetParticleInterpolation(particles);
  }
  ItemHelp("With smooth frames, also draws particles and weather between the game's ticks.");
  // Read by CCameraManager::GetCurrentCameraTransform.
  bool look = sLookInterpolation;
  if (ImGui::Checkbox("Smooth look", &look)) {
    PortOverlay::SetLookInterpolation(look);
  }
  ItemHelp("With smooth frames, also turns the view between the game's ticks as the mouse, stick or "
           "touch aims.");
  ImGui::EndDisabled();
  ImGui::EndDisabled();
}

} // namespace PortDebug
