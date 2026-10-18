#pragma once

// The device layer between SDL/Aurora and the binding engine (port_input_engine.h), ported from
// the Prime 1 port's platform/include/port_input_devices.h: it reads port 0's controller (a real
// pad or the touch overlay's virtual one), keyboard, mouse buttons, wheel and touch controls into
// a RawState, builds the engine profile (the port's keyboard layout, Aurora's controller mapping,
// the port's own controls and the Menu rows) with `<user>/controls.toml` on top, and turns the
// engine's output back into the PADStatus that port_events.cpp hands Aurora through
// PADSetPortOverride. The console's and MP2_INPUT's virtual status still merges on top of it.
// Mouse motion is not an input here: it feeds only PortMouse::AddMotion.
//
// Not carried over from MP1 (no backend or setting here): the beam-shift chords' settings-driven shifts
// (only the twin-stick ones exist) and turbo's key slots, the pad's alt buttons, gyro, save states, photo mode, the Original experience toggle, pointer menus'
// Escape, and Aurora's key slots (MP2 has its own keyboard layout, BaseKeyboardProfile).

#include "port_input_bindings.h"
#include "port_input_engine.h"

#include <dolphin/pad.h>

#include <memory>
#include <string>
#include <vector>

namespace PortInputDevices {

// Mouse button codes (Device::MouseButton): each run holds SDL's five buttons
// (left, middle, right, X1, X2), read a different way.
constexpr int kMouseGameplay = 0; // under mouse aim, captured (ReadMouseButtons' gate)
constexpr int kMouseKeySlot = 8;  // SDL_GetMouseState, ungated (an input capture reads these)
constexpr int kMouseMenu = 16;    // the A/B buttons out of aim (menus, text boxes)
constexpr int kMouseHeld = 24;    // MP1's beam shift / turbo key slots; unused here (reserved)

// Touch overlay codes (Device::Touch). 0..kTouchControlCount-1 are the overlay's
// controls, numbered as its layout editor (TouchControlsView's C_* ids).
constexpr int kTouchControlCount = 29;

struct SPoll {
  PADStatus status{};
};

// The user directory, from main: controls.toml lives there. Nothing is read until first use.
void Initialize(const std::string& userDirectory);
// `<user>/controls.toml`, or "" when the run has no user directory (nothing is saved then).
const std::string& UserBindingsPath();

// One poll of port 0: reads the devices, rebuilds the profile when the bindings
// changed, runs the engine and synthesises the status. mouseHeld: the held mouse
// buttons (SDL mask order); focused: the window takes input.
SPoll Poll(unsigned mouseHeld, bool focused);
// The same, reading mouseHeld and focus from SDL: pad 0's status, err always PAD_ERR_NONE.
// Call once per loop iteration, before the game reads the pad.
PADStatus Poll();

// Runs the port's own actions (port menu, screenshot) from their bindings; called once a
// loop iteration from port_events.cpp, so they work while the game is paused.
void PollPortActions();

// The user's bindings from <user>/controls.toml (port_input_bindings.h), loaded on
// first use and reloaded when the file changes (checked at most once a second). A
// parse error is logged and keeps the last good bindings. Thread-safe; the snapshot
// stays valid while held.
std::shared_ptr<const PortInput::UserBindings> UserBindings();
// Changes whenever the bindings do (a load, a reload or SetUserBindings).
uint64_t UserBindingsVersion();
// Writes controls.toml (temp + rename) and publishes the new set; the set applies for the
// session even when the write fails. False when there is no user directory or the write failed.
bool SetUserBindings(const PortInput::UserBindings& bindings);
// The user profiles that apply now: the base one and port 0's pad (GUID, then type),
// with the snapshot they point into.
struct SActiveProfiles {
  std::shared_ptr<const PortInput::UserBindings> bindings;
  PortInput::Selection sel;
};
SActiveProfiles ActiveUserProfiles();
// The touch overlay's controls (Android): whether one is held, and whether a user
// binding reads it (then the overlay sends only the touch code, not its pad button).
void SetTouchControl(int control, bool down);
bool TouchControlBound(int control);
// Port 0's pad: its SDL GUID (32 hex digits), gamepad type string and name; false
// (all empty) without one.
bool ActivePad(std::string& guid, std::string& type, std::string& name);
// The devices as they are, for an input capture: keys, port 0's pad (axes without
// deadzones) and SDL's mouse buttons (code kMouseKeySlot..).
void ReadRaw(PortInput::RawState& raw);

// A connected controller (virtual touch pads excluded): SDL GUID, gamepad type
// string and name.
struct SPadInfo {
  std::string guid;
  std::string type;
  std::string name;
};
std::vector<SPadInfo> ConnectedPads();
// The port's keyboard + mouse layout, before any user change (the Controls page's defaults).
const PortInput::Profile& BaseKeyboardProfile();
// The profile without the user's bindings: what each action inherits on a remap page.
void BuildBaseProfile(PortInput::Profile& out);

// The profile with the user's bindings on top (exposed for tests and a remap UI).
void BuildProfile(PortInput::Profile& out);
// For the button prompts: the single input of the last polled profile that presses a PAD
// button or pushes a PAD axis from `family`, preferring one that applies in the current
// context. False when none.
bool PromptButtonInput(unsigned padButton, PortInput::Family family, PortInput::Input& out);
bool PromptAxisInput(PADAxis axis, PortInput::Family family, PortInput::Input& out);
// A pad input as the native button the prompts name (an SDL_GamepadButton or
// PAD_NATIVE_BUTTON_TRIGGER_*), or PAD_NATIVE_BUTTON_INVALID.
u32 PromptNativeButton(const PortInput::Input& in);
// The PAD status an engine output describes, as Aurora's own mapping would.
PADStatus Synthesize(const PortInput::Output& out);

} // namespace PortInputDevices
