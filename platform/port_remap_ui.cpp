// The keyboard remap page: PortBinds' table as a list, with capture by polling SDL's keyboard state.

#include "port_remap_ui.h"

#include "port_binds.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <cstring>

namespace PortRemapUi {
namespace {

int sArmed = -1;
bool sPrevKeys[SDL_SCANCODE_COUNT];
SDL_MouseButtonFlags sPrevMouse = 0;
bool sPrimed = false;

void Prime() {
  int count = 0;
  const bool* keys = SDL_GetKeyboardState(&count);
  for (int i = 0; i < SDL_SCANCODE_COUNT; ++i) {
    sPrevKeys[i] = i < count && keys[i];
  }
  sPrevMouse = SDL_GetMouseState(nullptr, nullptr);
  sPrimed = true;
}

void Arm(int row) {
  sArmed = row;
  Prime(); // keys and buttons already down when the row is armed (the click itself) do not count
}

struct SCapture {
  bool done = false;
  bool cancel = false;
  int scancode = PAD_KEY_INVALID;
};

// The first key newly down this frame, or a mouse button: right/middle/X1/X2 anywhere, left only
// over the armed row (`rowHovered`), since a left click elsewhere is the user moving on.
SCapture Poll(bool rowHovered) {
  SCapture result;
  int count = 0;
  const bool* keys = SDL_GetKeyboardState(&count);
  for (int i = SDL_SCANCODE_A; i < SDL_SCANCODE_COUNT && i < count; ++i) {
    const bool down = keys[i];
    const bool fresh = down && !sPrevKeys[i];
    sPrevKeys[i] = down;
    if (fresh && !result.done) {
      result.done = true;
      if (i == SDL_SCANCODE_ESCAPE) {
        result.cancel = true;
      } else if (std::strcmp(PortBinds::KeyName(i), "none") != 0) {
        result.scancode = i;
      } else {
        result.done = false; // a key SDL cannot name: stay armed
      }
    }
  }
  const SDL_MouseButtonFlags mouse = SDL_GetMouseState(nullptr, nullptr);
  const SDL_MouseButtonFlags fresh = mouse & ~sPrevMouse;
  sPrevMouse = mouse;
  if (!result.done && fresh != 0) {
    struct SButton {
      SDL_MouseButtonFlags flag;
      int scancode;
      bool needsHover;
    };
    static const SButton kButtons[] = {
        {SDL_BUTTON_LMASK, PAD_KEY_MOUSE_LEFT, true},
        {SDL_BUTTON_MMASK, PAD_KEY_MOUSE_MIDDLE, false},
        {SDL_BUTTON_RMASK, PAD_KEY_MOUSE_RIGHT, false},
        {SDL_BUTTON_X1MASK, PAD_KEY_MOUSE_X1, false},
        {SDL_BUTTON_X2MASK, PAD_KEY_MOUSE_X2, false},
    };
    for (const SButton& button : kButtons) {
      if ((fresh & button.flag) != 0 && (!button.needsHover || rowHovered)) {
        result.done = true;
        result.scancode = button.scancode;
        break;
      }
    }
    if (!result.done && (fresh & SDL_BUTTON_LMASK) != 0) {
      result.done = true; // a left click away from the row cancels
      result.cancel = true;
    }
  }
  return result;
}

// Another row bound to the same key, or -1. A shared key is legal (Q is L and the analog trigger),
// so this only marks it.
int ConflictWith(int row) {
  const int key = PortBinds::Scancode(row);
  if (key == PAD_KEY_INVALID) {
    return -1;
  }
  for (int i = 0; i < PortBinds::Count(); ++i) {
    if (i != row && PortBinds::Scancode(i) == key) {
      return i;
    }
  }
  return -1;
}

} // namespace

bool Capturing() { return sArmed >= 0; }

void Draw() {
  if (!sPrimed) {
    Prime();
  }
  ImGui::TextWrapped("Click a key, then press the new one. Esc cancels; right/middle/side mouse "
                     "buttons bind too.");
  if (ImGui::Button("Reset all")) {
    PortBinds::Reset();
    sArmed = -1;
  }
  ImGui::SameLine();
  ImGui::TextDisabled("%s", PortBinds::Path()[0] != '\0' ? PortBinds::Path() : "(not saved)");
  ImGui::Separator();

  const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
                                ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
  if (!ImGui::BeginTable("##remap", 3, flags, ImVec2(0.f, 0.f))) {
    return;
  }
  ImGui::TableSetupScrollFreeze(0, 1);
  ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch, 1.f);
  ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch, 1.4f);
  ImGui::TableSetupColumn("##reset", ImGuiTableColumnFlags_WidthFixed);
  ImGui::TableHeadersRow();

  const int count = PortBinds::Count();
  for (int row = 0; row < count; ++row) {
    const PortBinds::SInput& input = PortBinds::Input(row);
    const int key = PortBinds::Scancode(row);
    const bool armed = sArmed == row;
    const bool isDefault = key == input.defaultScancode;
    ImGui::PushID(row);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(input.label);

    ImGui::TableSetColumnIndex(1);
    char text[96];
    const int other = armed ? -1 : ConflictWith(row);
    if (armed) {
      std::snprintf(text, sizeof(text), "Press a key...");
    } else if (key == PAD_KEY_INVALID) {
      std::snprintf(text, sizeof(text), "--");
    } else {
      std::snprintf(text, sizeof(text), "%s%s", PortBinds::KeyName(key), other >= 0 ? "  (!)" : "");
    }
    if (other >= 0) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.75f, 0.2f, 1.f));
    }
    const bool clicked = ImGui::Selectable(text, armed, 0, ImVec2(0.f, 0.f));
    if (other >= 0) {
      ImGui::PopStyleColor();
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Also bound to %s", PortBinds::Input(other).label);
      }
    } else if (!armed && PortBinds::IsPortHotkey(key) && ImGui::IsItemHovered()) {
      ImGui::SetTooltip("A port hotkey: it fires whenever the key is pressed");
    }
    const bool hovered = ImGui::IsItemHovered();
    if (clicked && !armed) {
      Arm(row);
    }

    ImGui::TableSetColumnIndex(2);
    ImGui::BeginDisabled(isDefault && !armed);
    if (ImGui::SmallButton("Default")) {
      PortBinds::Set(row, input.defaultScancode);
      if (armed) {
        sArmed = -1;
      }
    }
    ImGui::EndDisabled();

    if (armed && !clicked) {
      const SCapture capture = Poll(hovered);
      if (capture.done) {
        if (!capture.cancel) {
          PortBinds::Set(row, capture.scancode);
        }
        sArmed = -1;
      }
    }
    ImGui::PopID();
  }
  ImGui::EndTable();
}

} // namespace PortRemapUi
