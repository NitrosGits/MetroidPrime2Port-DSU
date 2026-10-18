#pragma once

// Pad 0's devices, read through the input engine (adapted from the Prime 1 port's
// platform/input/port_input_devices.cpp, without its beam shift, turbo, mouse variants, photo mode,
// save states, port menu, gyro and game-state contexts).
//
// Each loop iteration `Poll` reads port 0's SDL gamepad (a real pad or the touch overlay's virtual
// one), the keyboard and - while the port's mouse aim holds the capture - the mouse buttons into a
// PortInput::RawState, runs it through the engine with the base profile (the port's keyboard
// layout plus Aurora's mapping for the pad) overlaid with `<user>/controls.toml`, and returns the
// PADStatus the game should read. port_events.cpp hands it to Aurora with PADSetPortOverride, which
// replaces Aurora's own controller + keyboard read for that PADRead; the console's and
// MP2_INPUT's virtual status still merges on top. Mouse motion is not an input here: it feeds only
// PortMouse::AddMotion.

#include "port_input_bindings.h"
#include "port_input_engine.h"

#include <dolphin/pad.h>

#include <memory>
#include <string>

namespace PortInputDevices {

// The user directory, from main: controls.toml lives there. Nothing is read until first use.
void Initialize(const std::string& userDirectory);
// `<user>/controls.toml`, or "" when the run has no user directory (nothing is saved then).
const std::string& UserBindingsPath();

// The user's bindings, loaded on first use and reloaded when the file changes (checked once a
// second). SetUserBindings writes the file (temp + rename) and publishes the new set; false when
// there is no user directory or the write failed.
std::shared_ptr<const PortInput::UserBindings> UserBindings();
bool SetUserBindings(const PortInput::UserBindings& bindings);

// The port's keyboard + mouse layout, before any user change (PortBinds' defaults).
const PortInput::Profile& BaseKeyboardProfile();
// The whole base profile: the keyboard layout plus port 0's pad mapping.
void BuildBaseProfile(PortInput::Profile& out);

// One poll: pad 0's status, err always PAD_ERR_NONE. Call once per loop iteration, before the game
// reads the pad.
PADStatus Poll();
PADStatus Synthesize(const PortInput::Output& out);

} // namespace PortInputDevices
