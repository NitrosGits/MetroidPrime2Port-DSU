// F1 overlay: The Randomizer page. It reports what the running game loaded and can write a seed
// from the port's logic-based generator (docs/RANDOMIZER.md). State and helpers shared with the other parts are in debug_ui_internal.h.

#include "debug_ui_internal.h"

#include "port_randomizer.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <string>

namespace PortDebug {

void DrawRandomizerTab() {
  ImGui::SeparatorText("Seed");
  ImGui::TextUnformatted(PortRandomizer::StatusText());
  const std::string seedPath = PortRandomizer::SeedFilePath();
  const std::string dumpPath = PortRandomizer::DumpLogFilePath();
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
  ImGui::TextWrapped("Seed file: %s", seedPath.c_str());
  ImGui::TextWrapped("Dump log: %s", dumpPath.c_str());
  ImGui::PopStyleColor();

  ImGui::SeparatorText("Generate");
  static char sSeedName[64] = "echoes1";
  static std::string sGenerateResult;
  static bool sGenerateOk = false;
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.f);
  ImGui::InputText("Seed name", sSeedName, sizeof(sSeedName));
  if (ImGui::Button("Generate")) {
    std::string error;
    sGenerateOk = PortRandomizer::GenerateSeed(sSeedName, error);
    sGenerateResult = sGenerateOk ? "Seed written. It loads on the next launch." : error;
  }
  ItemHelp("Shuffles the items of all 114 pickup locations with the port's own logic, so every "
           "location can be reached from the start. Generate here, then restart to play the seed.");
  if (!sGenerateResult.empty()) {
    ImGui::PushTextWrapPos(0.f);
    ImGui::TextColored(sGenerateOk ? ThemeGoodColor() : ThemeBadColor(), "%s", sGenerateResult.c_str());
    ImGui::PopTextWrapPos();
  }
}

} // namespace PortDebug
