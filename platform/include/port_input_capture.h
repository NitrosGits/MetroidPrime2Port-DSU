#pragma once

#include "port_input_bindings.h"
#include <dolphin/pad.h>
#include <SDL3/SDL_scancode.h>
#include <algorithm>
#include <cmath>

namespace PortInputCapture {
// Record physical controls, not the extra half-pull/click buttons Aurora derives
// from trigger axes. Those aliases remain available for manual bindings.
inline std::vector<PortInput::Input> DownInputs(const PortInput::RawState& raw) {
  using namespace PortInput;
  std::vector<Input> down;
  for (int k = 0; k < kKeyCount; ++k)
    if (raw.keys.test(size_t(k)) && k != SDL_SCANCODE_ESCAPE)
      down.push_back({Device::Key, 0, 50, uint16_t(k)});
  for (int m = 0; m < kMouseButtonCount; ++m)
    if ((raw.mouseButtons >> m) & 1u)
      down.push_back({Device::MouseButton, 0, 50, uint16_t(m)});
  for (int b = 0; b < kPadButtonCount; ++b) {
    if (b == PAD_RAW_BUTTON_TRIGGER_LEFT || b == PAD_RAW_BUTTON_TRIGGER_RIGHT ||
        b == PAD_RAW_BUTTON_TRIGGER_LEFT_CLICK || b == PAD_RAW_BUTTON_TRIGGER_RIGHT_CLICK)
      continue;
    if ((raw.padButtons >> b) & 1u)
      down.push_back({Device::PadButton, 0, 50, uint16_t(b)});
  }
  for (int a = 0; a < kPadAxisCount; ++a)
    if (std::fabs(raw.padAxes[a]) >= 0.6f)
      down.push_back({Device::PadAxis, int8_t(raw.padAxes[a] > 0.f ? 1 : -1), 50, uint16_t(a)});
  return down;
}

// Accumulate a same-family chord until release, preserving press order.
inline bool Accumulate(std::vector<PortInput::Input>& chord,
                       const std::vector<PortInput::Input>& down) {
  for (const auto& in : down) {
    if (!chord.empty() && PortInput::FamilyOf(in.device) != PortInput::FamilyOf(chord[0].device))
      continue;
    if (chord.size() < size_t(PortInput::kMaxChord) &&
        std::find(chord.begin(), chord.end(), in) == chord.end())
      chord.push_back(in);
  }
  return !chord.empty() && down.empty();
}
} // namespace PortInputCapture
