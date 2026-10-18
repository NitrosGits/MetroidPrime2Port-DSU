// Keyboard bindings: the port's own table, `keybinds.cfg`, and applying it to Aurora's pad.

#include "port_binds.h"

#include <SDL3/SDL.h>
#include <dolphin/pad.h>

#include <cstdio>
#include <fstream>
#include <string>

namespace PortBinds {
namespace {

// The port's layout (MetroidPrimePort's `port_controls.cpp`): WASD moves, IJKL is the C-stick
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

// The file the map is saved in, under MP2_USER_PATH beside the memory card and touch.cfg.
constexpr const char* kFileName = "keybinds.cfg";

constexpr int kPort = 0;

bool sLoaded = false;
bool sInstalled = false;
int sScancodes[kRowCount];
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

void ResetToDefaults() {
  for (int i = 0; i < kRowCount; ++i) {
    sScancodes[i] = kRows[i].defaultScancode;
  }
}

// Reads keybinds.cfg: `action=key` lines, `#` comments, anything else ignored with a line each. A
// file that is missing leaves the port's layout in place, so deleting it is how a fresh start is
// asked for; a line that names no row or no key is skipped rather than ending the load.
void Load() {
  if (sLoaded) {
    return;
  }
  sLoaded = true;
  ResetToDefaults();
  if (sPath.empty()) {
    std::fprintf(stderr, "[binds] no user directory; the port's keyboard layout, not saved\n");
    return;
  }
  std::ifstream file(sPath);
  if (!file) {
    std::fprintf(stderr, "[binds] %s does not exist; the port's keyboard layout\n", sPath.c_str());
    return;
  }
  int applied = 0;
  int ignored = 0;
  std::string line;
  bool firstLine = true;
  while (std::getline(file, line)) {
    std::string_view entry = Trim(line);
    // A hand-edited file saved by an editor that writes a UTF-8 BOM would otherwise fail on the
    // first row only, which reads as "one binding silently forgotten".
    if (firstLine) {
      firstLine = false;
      if (entry.starts_with("\xEF\xBB\xBF")) {
        entry.remove_prefix(3);
        entry = Trim(entry);
      }
    }
    if (entry.empty() || entry[0] == '#') {
      continue;
    }
    const size_t equals = entry.find('=');
    if (equals == std::string_view::npos) {
      ++ignored;
      if (ignored <= 8) {
        std::fprintf(stderr, "[binds] ignoring '%.*s': no '=' in it\n",
                     static_cast< int >(entry.size()), entry.data());
      }
      continue;
    }
    const std::string_view action = Trim(entry.substr(0, equals));
    const std::string_view key = Trim(entry.substr(equals + 1));
    const int row = Find(action);
    int scancode = PAD_KEY_INVALID;
    if (row < 0 || !ParseKey(key, scancode)) {
      ++ignored;
      if (ignored <= 8) {
        std::fprintf(stderr, "[binds] ignoring '%.*s': %s\n", static_cast< int >(entry.size()),
                     entry.data(), row < 0 ? "no such pad input" : "no such key");
      }
      continue;
    }
    sScancodes[row] = scancode;
    ++applied;
  }
  std::fprintf(stderr, "[binds] %s: %d bindings%s\n", sPath.c_str(), applied,
               ignored != 0 ? " (some lines ignored, see above)" : "");
}

// Pushes the whole table into Aurora's pad keyboard map. It is 22 tiny calls, so a change reapplies
// everything rather than tracking which row moved.
void Apply() {
  PADKeyButtonBinding buttons[PAD_BUTTON_COUNT] = {};
  PADKeyAxisBinding axes[PAD_AXIS_COUNT] = {};
  int buttonRows = 0;
  int axisRows = 0;
  for (int i = 0; i < kRowCount; ++i) {
    const SInput& row = kRows[i];
    if (row.axis) {
      if (axisRows < PAD_AXIS_COUNT) {
        axes[axisRows++] = PADKeyAxisBinding{sScancodes[i], row.padAxis, 1};
      }
    } else if (buttonRows < PAD_BUTTON_COUNT) {
      buttons[buttonRows++] = PADKeyButtonBinding{sScancodes[i], row.button};
    }
  }
  // The table covers every pad button and every axis, which is what these two arrays are sized for;
  // a row added without its pad input would land here as a rejected map rather than silently.
  if (!PADSetKeyButtonBindings(kPort, buttons) || !PADSetKeyAxisBindings(kPort, axes)) {
    std::fprintf(stderr, "[binds] the pad rejected the map (a row names no pad input)\n");
  }
  PADSetKeyboardActive(kPort, TRUE);
}

// Writes `<path>.tmp` and renames it over the file: a write that dies part way must not leave a
// truncated keybinds.cfg for the next launch to read as a half-layout.
bool Save() {
  if (sPath.empty()) {
    return false;
  }
  const std::string temporary = sPath + ".tmp";
  {
    std::ofstream file(temporary, std::ios::trunc);
    if (!file) {
      std::fprintf(stderr, "[binds] cannot write %s\n", temporary.c_str());
      return false;
    }
    file << "# Metroid Prime 2 port keyboard bindings.\n"
            "# One pad input per line: <action>=<key>, or none. Actions are what `binds` prints;\n"
            "# keys are SDL scancode names (any case), or MOUSE_LEFT/MIDDLE/RIGHT/X1/X2.\n"
            "# Edited by the F1 window and by the console's `bind`; `bind reset` rewrites this.\n";
    for (int i = 0; i < kRowCount; ++i) {
      file << kRows[i].action << '=' << KeyName(sScancodes[i]) << '\n';
    }
    file.flush();
    if (!file) {
      std::fprintf(stderr, "[binds] cannot write %s\n", temporary.c_str());
      std::remove(temporary.c_str());
      return false;
    }
  }
  if (std::rename(temporary.c_str(), sPath.c_str()) != 0) {
    std::fprintf(stderr, "[binds] cannot replace %s\n", sPath.c_str());
    std::remove(temporary.c_str());
    return false;
  }
  return true;
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

void Initialize(const std::string& userDirectory) {
  if (userDirectory.empty()) {
    return;
  }
  sPath = userDirectory + "/" + kFileName;
}

void Install() {
  Load();
  if (sInstalled) {
    return;
  }
  sInstalled = true;
  Apply();
}

int Scancode(int row) {
  Load();
  return row >= 0 && row < kRowCount ? sScancodes[row] : PAD_KEY_INVALID;
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

bool Set(int row, int scancode) {
  Load();
  if (row < 0 || row >= kRowCount) {
    return false;
  }
  sScancodes[row] = scancode;
  Apply();
  const bool saved = Save();
  std::fprintf(stderr, "[binds] %s = %s%s%s\n", kRows[row].action, KeyName(scancode),
               IsPortHotkey(scancode) ? " (a port hotkey: it fires whenever the key is pressed)"
                                      : "",
               saved ? "" : " (not saved)");
  return saved;
}

void Reset() {
  Load();
  ResetToDefaults();
  Apply();
  const bool saved = Save();
  std::fprintf(stderr, "[binds] the port's keyboard layout%s\n", saved ? "" : " (not saved)");
}

const char* Path() { return sPath.c_str(); }

void BeginCapture(int row) {
  Load();
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
