#pragma once

// Keyboard bindings. Aurora's pad keyboard map (`g_defaultKeys`) is empty, so the port installs a
// layout of its own (`port_events.cpp`'s InstallDefaultKeyBindings did, before this module
// existed); this is that layout as data, so it can be edited - the F1 window's Bindings table and
// the console's `bind`/`binds` - and stored in `keybinds.cfg` under MP2_USER_PATH, the directory
// the port owns beside the memory card (the same place touch.cfg lives).
//
// A row is one pad input a key drives: a GameCube button, or one direction/trigger of a stick. A
// key may drive more than one input (the port's own layout puts Q on L and on the analog L trigger,
// so a trigger reaches the game as held rather than tapped); nothing here forbids a key on two
// rows.
//
// The map is pad 0's, and it replaces Aurora's own keyboard file rather than reading it: Aurora's
// keyboard_bindings.dat is binary and has no names, so the port never writes it - this file is the
// port's keyboard settings.

#include <dolphin/pad.h>

#include <string>
#include <string_view>

namespace PortBinds {

// One pad input a key can drive. `action` is the name the console `bind` takes, `binds` prints and
// the file stores ("A", "STICK_LEFT"); `label` is the F1 row's text ("A", "Stick left");
// `defaultScancode` is the port's own layout, an SDL scancode or PAD_KEY_INVALID for unbound.
struct SInput {
  const char* action;
  const char* label;
  int defaultScancode;
  bool axis; // true: a stick direction (padAxis); false: a button (button's PAD_BUTTON_* bit)
  u16 button;
  PADAxis padAxis;
};

// The table's rows: every pad button, then every stick input.
int Count();
const SInput& Input(int row);
// The row an action names, in any case, or -1.
int Find(std::string_view action);

// The user directory, from main: `keybinds.cfg` goes there. Called before Aurora is up, so the file
// is read on the game thread's first Install rather than here.
void Initialize(const std::string& userDirectory);

// Applies the saved map to pad 0, reading the file the first time; once per process, cheap after
// that. The keyboard map is empty until this runs, which is why pad 0 reported
// PAD_ERR_NO_CONTROLLER on a machine with no gamepad.
void Install();

// The key bound to a row (PAD_KEY_INVALID when unbound), and its name as the file spells it.
int Scancode(int row);
const char* KeyName(int scancode);
// A key as `bind` and the file spell it: an SDL scancode name in any case ("x", "Left Shift",
// "space"), MOUSE_LEFT/-RIGHT/-MIDDLE/-X1/-X2, or "none". False when the text names no key.
bool ParseKey(std::string_view name, int& scancode);
// A key the port already uses (F1, F5-F10, F12): the game sees it too, so binding one makes the
// hotkey fire whenever the key is pressed.
bool IsPortHotkey(int scancode);

// Binds a row, applies it to the pad and writes the file. False only when the row is out of range
// or the file could not be written; a failed write leaves the binding in force for the session.
bool Set(int row, int scancode);
// The port's own layout, applied and written.
void Reset();
// `keybinds.cfg`'s path, or "" when the run has no user directory (nothing is saved then).
const char* Path();

// The F1 editor's capture: arms a row and the next key the event pump sees is bound to it; Escape
// cancels instead. The pump calls OnKeyDown for every key-down and skips its own hotkeys while that
// returns true, so the key that completes a capture does not also warp the game.
void BeginCapture(int row);
void CancelCapture();
int CapturingRow();
bool OnKeyDown(int scancode);

} // namespace PortBinds
