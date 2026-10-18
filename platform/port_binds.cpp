// Keyboard bindings: the port's 22 pad-input rows, stored as the input engine's keyboard bindings
// in controls.toml (platform/input/port_input_devices.cpp, which also drives pad 0 from them).

#include "port_binds.h"

#include "input/port_input_devices.h"

#include <SDL3/SDL.h>
#include <dolphin/pad.h>

#include <cstdio>
#include <string>

namespace PortBinds {
namespace {

// The port's layout (MetroidPrimePort's `port_controls.cpp`, and the engine's base keyboard profile): WASD moves, IJKL is the C-stick
// (which aims and switches the visor in Echoes), X/Z/C/V are A/B/X/Y, Q/E are L/R and the analog
// triggers, F is Z, Enter is Start and the arrows are the D-pad. One row type covers both kinds:
// `padAxis` means nothing on a button row and `button` nothing on a stick row, so the table fills
// both and the flag says which to read.
const SInput kRows[] = {
    // GameCube buttons.
    {"A", "A", SDL_SCANCODE_X, false, PAD_BUTTON_A, PAD_AXIS_LEFT_X_POS},
    {"B", "B", SDL_SCANCODE_Z, false, PAD_BUTTON_B, PAD_AXIS_LEFT_X_POS},
    {"X", "X", SDL_SCANCODE_C, false, PAD_BUTTON_X, PAD_AXIS_LEFT_X_POS},
    {"Y", "Y", SDL_SCANCODE_V, false, PAD_BUTTON_Y, PAD_AXIS_LEFT_X_POS},
    {"START", "Start", SDL_SCANCODE_RETURN, false, PAD_BUTTON_START, PAD_AXIS_LEFT_X_POS},
    {"Z", "Z", SDL_SCANCODE_F, false, PAD_TRIGGER_Z, PAD_AXIS_LEFT_X_POS},
    {"L", "L", SDL_SCANCODE_Q, false, PAD_TRIGGER_L, PAD_AXIS_LEFT_X_POS},
    {"R", "R", SDL_SCANCODE_E, false, PAD_TRIGGER_R, PAD_AXIS_LEFT_X_POS},
    {"UP", "D-pad up", SDL_SCANCODE_UP, false, PAD_BUTTON_UP, PAD_AXIS_LEFT_X_POS},
    {"DOWN", "D-pad down", SDL_SCANCODE_DOWN, false, PAD_BUTTON_DOWN, PAD_AXIS_LEFT_X_POS},
    {"LEFT", "D-pad left", SDL_SCANCODE_LEFT, false, PAD_BUTTON_LEFT, PAD_AXIS_LEFT_X_POS},
    {"RIGHT", "D-pad right", SDL_SCANCODE_RIGHT, false, PAD_BUTTON_RIGHT, PAD_AXIS_LEFT_X_POS},
    // Stick inputs. The same key may drive a button and its analog trigger (Q, E): a key is a
    // digital press, so the trigger would otherwise click rather than hold.
    {"STICK_UP", "Stick up", SDL_SCANCODE_W, true, PAD_BUTTON_A, PAD_AXIS_LEFT_Y_POS},
    {"STICK_DOWN", "Stick down", SDL_SCANCODE_S, true, PAD_BUTTON_A, PAD_AXIS_LEFT_Y_NEG},
    {"STICK_LEFT", "Stick left", SDL_SCANCODE_A, true, PAD_BUTTON_A, PAD_AXIS_LEFT_X_NEG},
    {"STICK_RIGHT", "Stick right", SDL_SCANCODE_D, true, PAD_BUTTON_A, PAD_AXIS_LEFT_X_POS},
    {"CSTICK_UP", "C-stick up", SDL_SCANCODE_I, true, PAD_BUTTON_A, PAD_AXIS_RIGHT_Y_POS},
    {"CSTICK_DOWN", "C-stick down", SDL_SCANCODE_K, true, PAD_BUTTON_A, PAD_AXIS_RIGHT_Y_NEG},
    {"CSTICK_LEFT", "C-stick left", SDL_SCANCODE_J, true, PAD_BUTTON_A, PAD_AXIS_RIGHT_X_NEG},
    {"CSTICK_RIGHT", "C-stick right", SDL_SCANCODE_L, true, PAD_BUTTON_A, PAD_AXIS_RIGHT_X_POS},
    {"TRIGGER_L", "L trigger (analog)", SDL_SCANCODE_Q, true, PAD_BUTTON_A, PAD_AXIS_TRIGGER_L},
    {"TRIGGER_R", "R trigger (analog)", SDL_SCANCODE_E, true, PAD_BUTTON_A, PAD_AXIS_TRIGGER_R},
};

constexpr int kRowCount = static_cast< int >(sizeof(kRows) / sizeof(kRows[0]));

// The engine action each row edits, in kRows' order.
const PortInput::Action kActions[] = {
    PortInput::Action::PadA,      PortInput::Action::PadB,      PortInput::Action::PadX,
    PortInput::Action::PadY,      PortInput::Action::PadStart,  PortInput::Action::PadZ,
    PortInput::Action::PadL,      PortInput::Action::PadR,      PortInput::Action::PadUp,
    PortInput::Action::PadDown,   PortInput::Action::PadLeft,   PortInput::Action::PadRight,
    PortInput::Action::MainUp,    PortInput::Action::MainDown,  PortInput::Action::MainLeft,
    PortInput::Action::MainRight, PortInput::Action::CUp,       PortInput::Action::CDown,
    PortInput::Action::CLeft,     PortInput::Action::CRight,    PortInput::Action::LAnalog,
    PortInput::Action::RAnalog,
};
static_assert(sizeof(kActions) / sizeof(kActions[0]) == kRowCount, "one action per row");

std::string sPath;
int sCapturing = -1;

bool EqualsIgnoreCase(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (size_t i = 0; i < a.size(); ++i) {
    char x = a[i];
    char y = b[i];
    if (x >= 'A' && x <= 'Z') {
      x = static_cast< char >(x - 'A' + 'a');
    }
    if (y >= 'A' && y <= 'Z') {
      y = static_cast< char >(y - 'A' + 'a');
    }
    if (x != y) {
      return false;
    }
  }
  return true;
}

std::string_view Trim(std::string_view text) {
  size_t first = 0;
  size_t last = text.size();
  while (first < last && (text[first] == ' ' || text[first] == '\t' || text[first] == '\r')) {
    ++first;
  }
  while (last > first &&
         (text[last - 1] == ' ' || text[last - 1] == '\t' || text[last - 1] == '\r')) {
    --last;
  }
  return text.substr(first, last - first);
}

// A row's input as a PAD_KEY_* value (scancode, or PAD_KEY_MOUSE_*), and back.
int ToScancode(const PortInput::Input& in) {
  if (in.device == PortInput::Device::Key) {
    return in.code;
  }
  if (in.device == PortInput::Device::MouseButton && in.code <= 4) {
    return PAD_KEY_MOUSE_LEFT - in.code;
  }
  return PAD_KEY_INVALID;
}

PortInput::Input FromScancode(int scancode) {
  PortInput::Input in;
  if (scancode >= 0) {
    in.device = PortInput::Device::Key;
    in.code = static_cast< uint16_t >(scancode);
  } else if (scancode <= PAD_KEY_MOUSE_LEFT && scancode >= PAD_KEY_MOUSE_X2) {
    in.device = PortInput::Device::MouseButton;
    in.code = static_cast< uint16_t >(PAD_KEY_MOUSE_LEFT - scancode);
  }
  return in;
}

// The binding a row shows: the first single-input key or mouse-button binding of its action.
bool IsRowBinding(const PortInput::Binding& b, PortInput::Action action) {
  return b.action == action && b.count == 1 && ToScancode(b.inputs[0]) != PAD_KEY_INVALID;
}

const PortInput::UserProfile* FindDefault(const PortInput::UserBindings& user) {
  for (const PortInput::UserProfile& profile : user.profiles) {
    if (profile.match.empty()) {
      return &profile;
    }
  }
  return nullptr;
}

PortInput::UserProfile& GetDefault(PortInput::UserBindings& user) {
  for (PortInput::UserProfile& profile : user.profiles) {
    if (profile.match.empty()) {
      return profile;
    }
  }
  PortInput::UserProfile profile;
  profile.name = "Default";
  user.profiles.insert(user.profiles.begin(), profile);
  return user.profiles.front();
}

} // namespace

int Count() { return kRowCount; }

const SInput& Input(int row) { return kRows[row]; }

int Find(std::string_view action) {
  const std::string_view wanted = Trim(action);
  for (int i = 0; i < kRowCount; ++i) {
    if (EqualsIgnoreCase(kRows[i].action, wanted)) {
      return i;
    }
  }
  return -1;
}

void Initialize(const std::string&) {}

// The engine reads the keyboard itself now; nothing goes into Aurora's keyboard map.
void Install() {}

int Scancode(int row) {
  if (row < 0 || row >= kRowCount) {
    return PAD_KEY_INVALID;
  }
  PortInput::Profile effective = PortInputDevices::BaseKeyboardProfile();
  const auto user = PortInputDevices::UserBindings();
  if (const PortInput::UserProfile* profile = FindDefault(*user)) {
    PortInput::Overlay(effective, *profile);
  }
  for (const PortInput::Binding& b : effective.bindings) {
    if (IsRowBinding(b, kActions[row])) {
      return ToScancode(b.inputs[0]);
    }
  }
  return PAD_KEY_INVALID;
}

const char* KeyName(int scancode) {
  switch (scancode) {
  case PAD_KEY_INVALID:
    return "none";
  case PAD_KEY_MOUSE_LEFT:
    return "MOUSE_LEFT";
  case PAD_KEY_MOUSE_MIDDLE:
    return "MOUSE_MIDDLE";
  case PAD_KEY_MOUSE_RIGHT:
    return "MOUSE_RIGHT";
  case PAD_KEY_MOUSE_X1:
    return "MOUSE_X1";
  case PAD_KEY_MOUSE_X2:
    return "MOUSE_X2";
  default:
    break;
  }
  if (scancode > PAD_KEY_INVALID && scancode < SDL_SCANCODE_COUNT) {
    const char* name = SDL_GetScancodeName(static_cast< SDL_Scancode >(scancode));
    if (name != nullptr && *name != '\0') {
      return name;
    }
  }
  return "none";
}

bool ParseKey(std::string_view name, int& scancode) {
  const std::string_view text = Trim(name);
  if (text.empty()) {
    return false;
  }
  if (EqualsIgnoreCase(text, "none")) {
    scancode = PAD_KEY_INVALID;
    return true;
  }
  struct SMouseName {
    const char* name;
    int scancode;
  };
  static const SMouseName kMouseNames[] = {
      {"MOUSE_LEFT", PAD_KEY_MOUSE_LEFT},   {"MOUSE_MIDDLE", PAD_KEY_MOUSE_MIDDLE},
      {"MOUSE_RIGHT", PAD_KEY_MOUSE_RIGHT}, {"MOUSE_X1", PAD_KEY_MOUSE_X1},
      {"MOUSE_X2", PAD_KEY_MOUSE_X2},
  };
  for (const SMouseName& mouse : kMouseNames) {
    if (EqualsIgnoreCase(text, mouse.name)) {
      scancode = mouse.scancode;
      return true;
    }
  }
  // Aurora reads a binding as `kbState[scancode]`, so a name has to become an SDL scancode; SDL's
  // own names are the ones the file writes back, and single characters ("x", "1") are among them.
  for (int sc = SDL_SCANCODE_A; sc < SDL_SCANCODE_COUNT; ++sc) {
    const char* candidate = SDL_GetScancodeName(static_cast< SDL_Scancode >(sc));
    if (candidate != nullptr && *candidate != '\0' && EqualsIgnoreCase(text, candidate)) {
      scancode = sc;
      return true;
    }
  }
  return false;
}

bool IsPortHotkey(int scancode) {
  switch (scancode) {
  case SDL_SCANCODE_F1:
  case SDL_SCANCODE_F5:
  case SDL_SCANCODE_F6:
  case SDL_SCANCODE_F7:
  case SDL_SCANCODE_F8:
  case SDL_SCANCODE_F9:
  case SDL_SCANCODE_F10:
  case SDL_SCANCODE_F12:
    return true;
  default:
    return false;
  }
}

// Edits the default user profile's keyboard bindings of the row's action: the binding the row shows
// becomes the new key (other bindings of the action, like A's mouse button, stay), and "none"
// removes it.
bool Set(int row, int scancode) {
  if (row < 0 || row >= kRowCount) {
    return false;
  }
  const PortInput::Action action = kActions[row];
  const int shown = Scancode(row);
  PortInput::UserBindings user = *PortInputDevices::UserBindings();
  PortInput::UserProfile& profile = GetDefault(user);
  PortInput::Materialize(profile, action, PortInput::Family::Keyboard,
                         PortInputDevices::BaseKeyboardProfile());
  size_t shownIndex = profile.bindings.size();
  bool present = false;
  for (size_t i = 0; i < profile.bindings.size(); ++i) {
    const PortInput::Binding& b = profile.bindings[i];
    if (!IsRowBinding(b, action)) {
      continue;
    }
    if (shownIndex == profile.bindings.size() && ToScancode(b.inputs[0]) == shown) {
      shownIndex = i;
    }
    if (ToScancode(b.inputs[0]) == scancode) {
      present = true;
    }
  }
  const PortInput::Input input = FromScancode(scancode);
  if (!input.Valid() || present) {
    if (shownIndex < profile.bindings.size() && shown != scancode) {
      PortInput::RemoveUserBinding(profile, shownIndex);
    }
  } else if (shownIndex < profile.bindings.size()) {
    profile.bindings[shownIndex].inputs[0] = input;
  } else {
    PortInput::Binding b;
    b.action = action;
    b.inputs[0] = input;
    b.count = 1;
    PortInput::AddUserBinding(profile, b);
  }
  const bool saved = PortInputDevices::SetUserBindings(user);
  std::fprintf(stderr, "[binds] %s = %s%s%s\n", kRows[row].action, KeyName(scancode),
               IsPortHotkey(scancode) ? " (a port hotkey: it fires whenever the key is pressed)"
                                      : "",
               saved ? "" : " (not saved)");
  return saved;
}

void Reset() {
  PortInput::UserBindings user = *PortInputDevices::UserBindings();
  PortInput::ClearFamily(GetDefault(user), PortInput::Family::Keyboard);
  const bool saved = PortInputDevices::SetUserBindings(user);
  std::fprintf(stderr, "[binds] the port's keyboard layout%s\n", saved ? "" : " (not saved)");
}

const char* Path() {
  sPath = PortInputDevices::UserBindingsPath();
  return sPath.c_str();
}

void BeginCapture(int row) {
  sCapturing = row >= 0 && row < kRowCount ? row : -1;
}

void CancelCapture() {
  if (sCapturing >= 0) {
    std::fprintf(stderr, "[binds] capture cancelled\n");
    sCapturing = -1;
  }
}

int CapturingRow() { return sCapturing; }

bool OnKeyDown(int scancode) {
  if (sCapturing < 0) {
    return false;
  }
  // A key SDL cannot name (scancode 0: some media and Fn keys) would be saved as "none" and be gone
  // at the next launch, so it is not a binding at all. The row stays armed for the next key.
  if (scancode <= PAD_KEY_INVALID || std::string_view(KeyName(scancode)) == "none") {
    std::fprintf(stderr,
                 "[binds] that key reports no name; nothing bound, the row is still armed\n");
    return true;
  }
  const int row = sCapturing;
  sCapturing = -1;
  if (scancode == SDL_SCANCODE_ESCAPE) {
    std::fprintf(stderr, "[binds] capture cancelled (Escape)\n");
    return true;
  }
  Set(row, scancode);
  return true;
}

} // namespace PortBinds
