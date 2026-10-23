// F1 overlay: The pages assembled from their sections (Game, Controls, Video, System). State and
// helpers shared with the other parts are in debug_ui_internal.h.

#include "debug_ui_internal.h"

#include "port_input_remap_ui.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace PortDebug {

void DrawGameTab() {
  DrawGameSection();
  DrawCutscenesSection();
  DrawGameAudio();
  DrawSpeedrunSection();
}

void DrawControlsTab() {
  if (!ImGui::BeginTabBar("controlsPages")) {
    return;
  }
  if (SubTab("Controls", "Options")) {
    DrawControlsOptions();
    ImGui::EndTabItem();
  }
  if (SubTab("Controls", "Mouse settings")) {
    DrawControlsKeyboardMouse();
    ImGui::EndTabItem();
  }
  if (SubTab("Controls", "Controller settings")) {
    DrawControlsController();
    ImGui::EndTabItem();
  }
  if (SubTab("Controls", "Touch & gyro")) {
    DrawControlsTouchGyro();
    ImGui::EndTabItem();
  }
  if (SubTab("Controls", "Remap")) {
    PortInputRemap::Draw();
    ImGui::EndTabItem();
  }
  ImGui::EndTabBar();
}

void DrawVideoTab() {
  if (!ImGui::BeginTabBar("videoPages")) {
    return;
  }
  if (SubTab("Video", "Display")) {
    DrawVideoDisplay();
    ImGui::EndTabItem();
  }
  if (SubTab("Video", "Quality")) {
    DrawVideoQuality();
    ImGui::EndTabItem();
  }
  if (SubTab("Video", "Frame rate")) {
    DrawPerformanceTab();
    ImGui::EndTabItem();
  }
  ImGui::EndTabBar();
}

void DrawSystemTab() {
  DrawOverlaySection();
  DrawSettingsSection();
  DrawMemoryCard();
  DrawLogSection();
  ImGui::SeparatorText("About");
  ImGui::TextDisabled("Metroid Prime 2: Echoes native port (G2ME01)");
  ImGui::TextDisabled("Aurora (SDL3 / WebGPU), Dear ImGui");
}

} // namespace PortDebug
