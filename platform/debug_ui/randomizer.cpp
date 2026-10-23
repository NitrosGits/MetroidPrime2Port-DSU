// F1 overlay: The Randomizer page. This port's randomizer is the dump-then-shuffle one
// (docs/RANDOMIZER.md), so the page reports what the running game loaded and can write a seed from
// the dump log. State and helpers shared with the other parts are in debug_ui_internal.h.

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
  if (ImGui::Button("Generate from the dump log")) {
    std::string error;
    sGenerateOk = PortRandomizer::GenerateSeedFromDumpLog(sSeedName, error);
    sGenerateResult = sGenerateOk ? "Seed written. It loads on the next launch." : error;
  }
  ItemHelp("MP_RANDO_DUMP=1 logs every pickup the areas you visit hold; generate here, then restart "
           "without MP_RANDO_DUMP to play the seed. A seed only covers what was dumped, and the "
           "shuffle has no logic, so a seed may not be beatable.");
  if (!sGenerateResult.empty()) {
    ImGui::PushTextWrapPos(0.f);
    ImGui::TextColored(sGenerateOk ? ThemeGoodColor() : ThemeBadColor(), "%s", sGenerateResult.c_str());
    ImGui::PopTextWrapPos();
  }
}

} // namespace PortDebug
