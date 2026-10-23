// F1 overlay: The Save states tab. State and helpers shared with the other parts are in
// debug_ui_internal.h.

#include "debug_ui_internal.h"

#include "port_input_remap_ui.h"
#include "port_overlay.h"
#include "port_savestate.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cstdio>
#include <string>

namespace PortDebug {

namespace {

// The slot's room by its English name, else the name the state file recorded.
std::string Where(const PortSaveState::Info& info) {
  std::string room = PortOverlay::RoomName(info.worldId, info.areaIndex);
  return room.empty() ? info.room : room;
}

} // namespace

void DrawSaveStatesTab() {
  ImGui::TextDisabled("Save anywhere and load back to the same spot.");
  ItemHelp("A state holds what a memory card save does, plus where Samus stands and whether she's "
           "in morph ball. Loading rebuilds the room, as a card load does, so enemies and doors "
           "start over. Loading first writes the undo state.");
  if (PortSaveState::Folder().empty()) {
    ImGui::TextDisabled("No user folder, so save states are off.");
    return;
  }
  const std::string saveKeys = PortInputRemap::BindingsText(PortInput::Action::SaveState, " / ");
  const std::string loadKeys = PortInputRemap::BindingsText(PortInput::Action::LoadState, " / ");
  ImGui::TextDisabled("%s saves, %s loads the selected slot",
                      saveKeys.empty() ? "Save state binding" : saveKeys.c_str(),
                      loadKeys.empty() ? "Load state binding" : loadKeys.c_str());

  const int selected = PortSaveState::SelectedSlot();
  if (ImGui::BeginTable("##savestates", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Where");
    ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableHeadersRow();
    for (int slot = 1; slot <= PortSaveState::kSlotCount; ++slot) {
      const PortSaveState::Info info = PortSaveState::SlotInfo(slot);
      ImGui::PushID(slot);
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      char label[16];
      std::snprintf(label, sizeof(label), "%d", slot);
      if (ImGui::RadioButton(label, selected == slot)) {
        PortSaveState::SetSelectedSlot(slot);
      }
      ImGui::TableNextColumn();
      if (info.exists) {
        ImGui::Text("%s%s", Where(info).c_str(), info.morphed ? " (ball)" : "");
      } else {
        ImGui::TextDisabled("empty");
      }
      ImGui::TableNextColumn();
      if (info.exists) {
        const int total = static_cast< int >(info.playTime);
        ImGui::Text("%d:%02d:%02d", total / 3600, total / 60 % 60, total % 60);
      }
      ImGui::TableNextColumn();
      if (ImGui::SmallButton("Save")) {
        PortSaveState::SetSelectedSlot(slot);
        PortSaveState::RequestSave(slot);
      }
      ImGui::SameLine();
      ImGui::BeginDisabled(!info.exists);
      if (ImGui::SmallButton("Load")) {
        PortSaveState::SetSelectedSlot(slot);
        PortSaveState::RequestLoad(slot);
      }
      ImGui::EndDisabled();
      ImGui::PopID();
    }
    ImGui::EndTable();
  }

  const PortSaveState::Info undo = PortSaveState::SlotInfo(PortSaveState::kUndoSlot);
  ImGui::BeginDisabled(!undo.exists);
  if (ImGui::Button("Undo last load")) {
    PortSaveState::RequestLoad(PortSaveState::kUndoSlot);
  }
  ImGui::EndDisabled();
  if (undo.exists) {
    ImGui::SameLine();
    ImGui::TextDisabled("back to %s", Where(undo).c_str());
  }
  const std::string message = PortSaveState::LastMessage();
  if (!message.empty()) {
    ImGui::TextWrapped("%s", message.c_str());
  }
  ImGui::TextDisabled("Folder: %s", PortSaveState::Folder().c_str());
}

} // namespace PortDebug
