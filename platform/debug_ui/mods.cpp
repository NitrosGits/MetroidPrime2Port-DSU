// F1 overlay: The Mods page: the mods folder's list and reload. The texture pack section under it
// is drawn by game_sections.cpp. State and helpers shared with the other parts are in
// debug_ui_internal.h.

#include "debug_ui_internal.h"

#include "port_mods.h"
#include "port_savestate.h"

#include <SDL3/SDL_misc.h>
#include <imgui.h>

#include <algorithm>
#include <string>
#include <vector>

namespace PortDebug {

// The mods folder (port_mods.h): the list is what this launch (or the last reload) loaded.
void DrawMods() {
  ImGui::SeparatorText("Mods");
  const PortMods::Status& status = PortMods::CurrentStatus();
  bool enabled = PortMods::Enabled();
  if (ImGui::Checkbox("Load mods", &enabled)) {
    PortMods::SetEnabled(enabled);
    Changed(true);
  }
  ItemHelp("Layers the folders in the mods folder over the disc. Changes apply on Reload mods "
           "(which rebuilds the room) or after a restart.");
  std::vector< std::string > disabled = PortMods::SplitDisabled(PortMods::Disabled());
  bool changed = enabled != status.active;
  if (status.mods.empty()) {
    ImGui::TextDisabled("No mods in the folder.");
  }
  for (const PortMods::ModInfo& mod : status.mods) {
    const auto found = std::find(disabled.begin(), disabled.end(), mod.name);
    bool on = found == disabled.end();
    ImGui::BeginDisabled(!enabled);
    if (ImGui::Checkbox((mod.name + "##mod").c_str(), &on)) {
      if (on) {
        disabled.erase(std::remove(disabled.begin(), disabled.end(), mod.name), disabled.end());
      } else {
        disabled.push_back(mod.name);
      }
      PortMods::SetDisabled(PortMods::JoinDisabled(disabled));
      Changed(true);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (mod.enabled) {
      ImGui::TextDisabled("%d file(s), %d resource(s)", mod.files, mod.resources);
    } else {
      ImGui::TextDisabled("not loaded");
    }
    changed = changed || (enabled && on) != mod.enabled;
  }
  const bool reloading = PortSaveState::ModReloadActive();
  ImGui::BeginDisabled(reloading);
  if (ImGui::Button(reloading ? "Reloading..." : "Reload mods")) {
    PortSaveState::RequestModReload();
  }
  ImGui::EndDisabled();
  ImGui::SetItemTooltip("Reads the mods folder again and rebuilds the room, so replaced models,\n"
                        "textures and sounds show without a restart. Needs a running game.");
#if !defined(__ANDROID__)
  ImGui::SameLine();
  if (ImGui::Button("Open mods folder") && !status.folder.empty()) {
    const std::string url = "file://" + status.folder;
    SDL_OpenURL(url.c_str());
  }
#endif
  if (changed) {
    ImGui::TextColored(ThemeBadColor(), "Reload mods to apply.");
  }
  for (const std::string& message : status.messages) {
    ImGui::TextColored(ThemeBadColor(), "%s", message.c_str());
  }
  if (!status.folder.empty()) {
    ImGui::TextWrapped("Folder: %s", status.folder.c_str());
  }
  ImGui::TextWrapped("Each folder in the mods folder is a mod; later names win. A file at a disc "
                     "path (Metroid5.pak, Audio/..., Video/...) replaces that file, and a "
                     "resource named by id and type (1A2B3C4D.TXTR) replaces it in every PAK.");
}

} // namespace PortDebug
