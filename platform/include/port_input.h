#pragma once

#include <dolphin/pad.h>

// Pad 0 as something other than a person driving it: the virtual pad `MP2_INPUT` holds buttons on,
// reachable at run time so the debug console can press what a script token would (`press`, `stick`).
// The port has no player-side input scripting beyond `MP2_INPUT`'s tokens, which are fixed at
// startup; a session that wants to dismiss a menu, walk into a room or hold a button down for a
// moment gets it from here instead.
//
// The hold merges into the same virtual pad the script writes (buttons OR together, stick axes add),
// so a console press and an `MP2_INPUT` token can be held at once. The game thread is the only
// caller: console commands run inside `CStateManager::PortWarpTick`.

namespace PortInput {

// A pad button as both the console's `press` and the `MP2_INPUT` script name it.
struct SButtonName {
  const char* name;
  u16 mask;
};

constexpr SButtonName kButtonNames[] = {
    {"START", PAD_BUTTON_START}, {"A", PAD_BUTTON_A},         {"B", PAD_BUTTON_B},
    {"X", PAD_BUTTON_X},         {"Y", PAD_BUTTON_Y},         {"Z", PAD_TRIGGER_Z},
    {"L", PAD_TRIGGER_L},        {"R", PAD_TRIGGER_R},        {"UP", PAD_BUTTON_UP},
    {"DOWN", PAD_BUTTON_DOWN},   {"LEFT", PAD_BUTTON_LEFT},   {"RIGHT", PAD_BUTTON_RIGHT},
};

// Holds `buttons` (a PAD_BUTTON_*/PAD_TRIGGER_* mask) and the main-stick deflection for `updates`
// game updates, counting the next one - the pad state set between two updates is what the next
// update reads. `updates` 0 holds them until the next Hold() or Clear(), which is the shape a console
// user needs to walk with a stick held.
void Hold(u16 buttons, s8 stickX, s8 stickY, unsigned updates);

// Releases whatever Hold() set on its next game update.
void Clear();

} // namespace PortInput
