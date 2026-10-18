#include "port_input_devices.h"

#include "port_mouse.h"
#include "port_overlay.h"

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>

namespace PortInputDevices {
namespace {

using PortInput::Action;
using PortInput::Binding;
using PortInput::Device;
using PortInput::Input;
using PortInput::Profile;

constexpr u32 kPort = 0;

// Aurora's emulated trigger clicks (a pulled analog past its activation zone presses L/R when no
// button is mapped to them), as raw pad button bits above SDL's buttons.
constexpr int kTriggerClickLeft = 30;
constexpr int kTriggerClickRight = 31;
static_assert(SDL_GAMEPAD_BUTTON_COUNT <= kTriggerClickLeft, "trigger click bits overlap SDL's buttons");

std::string sPath;

// The user's bindings (controls.toml), loaded on first use and reloaded when the file changes
// (checked once a second), so hand edits apply without a restart. The game thread polls them and
// the console/F1 write them, so they're published as immutable snapshots under a lock.
std::mutex sUserMutex;
std::shared_ptr<const PortInput::UserBindings> sUser = std::make_shared<const PortInput::UserBindings>();
bool sUserLoaded = false;
std::filesystem::file_time_type sUserTime{};
std::chrono::steady_clock::time_point sUserChecked{};

std::string ScancodeName(int scancode) {
  const char* name = SDL_GetScancodeName(static_cast<SDL_Scancode>(scancode));
  return name != nullptr ? name : "";
}

int ScancodeFromName(std::string_view name) {
  const SDL_Scancode code = SDL_GetScancodeFromName(std::string(name).c_str());
  return code == SDL_SCANCODE_UNKNOWN ? -1 : static_cast<int>(code);
}

Action PadBitAction(unsigned padButton) {
  switch (padButton) {
  case PAD_BUTTON_A: return Action::PadA;
  case PAD_BUTTON_B: return Action::PadB;
  case PAD_BUTTON_X: return Action::PadX;
  case PAD_BUTTON_Y: return Action::PadY;
  case PAD_TRIGGER_Z: return Action::PadZ;
  case PAD_BUTTON_START: return Action::PadStart;
  case PAD_TRIGGER_L: return Action::PadL;
  case PAD_TRIGGER_R: return Action::PadR;
  case PAD_BUTTON_UP: return Action::PadUp;
  case PAD_BUTTON_DOWN: return Action::PadDown;
  case PAD_BUTTON_LEFT: return Action::PadLeft;
  case PAD_BUTTON_RIGHT: return Action::PadRight;
  default: return Action::None;
  }
}

// Aurora's PAD axes, as the engine's actions (PAD_AXIS_LEFT_Y_POS is up).
Action PadAxisAction(PADAxis axis) {
  switch (axis) {
  case PAD_AXIS_LEFT_X_POS: return Action::MainRight;
  case PAD_AXIS_LEFT_X_NEG: return Action::MainLeft;
  case PAD_AXIS_LEFT_Y_POS: return Action::MainUp;
  case PAD_AXIS_LEFT_Y_NEG: return Action::MainDown;
  case PAD_AXIS_RIGHT_X_POS: return Action::CRight;
  case PAD_AXIS_RIGHT_X_NEG: return Action::CLeft;
  case PAD_AXIS_RIGHT_Y_POS: return Action::CUp;
  case PAD_AXIS_RIGHT_Y_NEG: return Action::CDown;
  case PAD_AXIS_TRIGGER_L: return Action::LAnalog;
  case PAD_AXIS_TRIGGER_R: return Action::RAnalog;
  default: return Action::None;
  }
}

Input Make(Device device, int code, int dir = 0) {
  Input in;
  in.device = device;
  in.code = static_cast<uint16_t>(code);
  in.dir = static_cast<int8_t>(dir);
  return in;
}

void Bind(Profile& p, Action action, const Input& in) {
  if (action == Action::None || !in.Valid()) return;
  Binding b;
  b.action = action;
  b.inputs[0] = in;
  b.count = 1;
  p.bindings.push_back(b);
}

Input NativeInput(s64 native) {
  if (native < 0 || native >= SDL_GAMEPAD_BUTTON_COUNT) return {};
  return Make(Device::PadButton, static_cast<int>(native));
}

// The port's keyboard layout (what port_events.cpp installed into Aurora's keyboard map before the
// engine took pad 0): Q/E press L/R and pull the analog triggers fully. Mouse buttons fire under
// mouse aim only (see ReadMouseButtons).
Profile MakeKeyboardProfile() {
  Profile p;
  const auto key = [&p](Action a, SDL_Scancode sc) { Bind(p, a, Make(Device::Key, sc)); };
  key(Action::PadA, SDL_SCANCODE_X);
  key(Action::PadB, SDL_SCANCODE_Z);
  key(Action::PadX, SDL_SCANCODE_C);
  key(Action::PadY, SDL_SCANCODE_V);
  key(Action::PadStart, SDL_SCANCODE_RETURN);
  key(Action::PadZ, SDL_SCANCODE_F);
  key(Action::PadL, SDL_SCANCODE_Q);
  key(Action::PadR, SDL_SCANCODE_E);
  key(Action::PadUp, SDL_SCANCODE_UP);
  key(Action::PadDown, SDL_SCANCODE_DOWN);
  key(Action::PadLeft, SDL_SCANCODE_LEFT);
  key(Action::PadRight, SDL_SCANCODE_RIGHT);
  key(Action::MainUp, SDL_SCANCODE_W);
  key(Action::MainDown, SDL_SCANCODE_S);
  key(Action::MainLeft, SDL_SCANCODE_A);
  key(Action::MainRight, SDL_SCANCODE_D);
  key(Action::CUp, SDL_SCANCODE_I);
  key(Action::CDown, SDL_SCANCODE_K);
  key(Action::CLeft, SDL_SCANCODE_J);
  key(Action::CRight, SDL_SCANCODE_L);
  key(Action::LAnalog, SDL_SCANCODE_Q);
  key(Action::RAnalog, SDL_SCANCODE_E);
  Bind(p, Action::PadA, Make(Device::MouseButton, 0));
  Bind(p, Action::PadY, Make(Device::MouseButton, 1));
  Bind(p, Action::PadL, Make(Device::MouseButton, 2));
  return p;
}

// Aurora's mapping for port 0 (buttons, axes, emulated trigger clicks) as bindings, read the way
// PADRead reads the controller. No controller on the port: no pad bindings.
void BindController(Profile& p) {
  u32 buttonCount = 0;
  u32 axisCount = 0;
  const PADButtonMapping* buttons = PADGetButtonMappings(kPort, &buttonCount);
  const PADAxisMapping* axes = PADGetAxisMappings(kPort, &axisCount);
  if (buttons == nullptr || axes == nullptr) return;
  const PADDeadZones* dz = PADGetDeadZones(kPort);
  bool lSet = false;
  bool rSet = false;
  for (u32 i = 0; i < buttonCount; ++i) {
    if (buttons[i].nativeButton == PAD_NATIVE_BUTTON_INVALID) continue;
    if (buttons[i].padButton == PAD_TRIGGER_L) lSet = true;
    if (buttons[i].padButton == PAD_TRIGGER_R) rSet = true;
    Bind(p, PadBitAction(buttons[i].padButton), NativeInput(buttons[i].nativeButton));
  }
  for (u32 i = 0; i < axisCount; ++i) {
    const PADAxisMapping& m = axes[i];
    const Action action = PadAxisAction(m.padAxis);
    if (m.nativeAxis.nativeAxis >= 0 && m.nativeAxis.nativeAxis < PortInput::kPadAxisCount) {
      Bind(p, action, Make(Device::PadAxis, m.nativeAxis.nativeAxis, m.nativeAxis.sign < 0 ? -1 : 1));
    } else if (m.nativeAxis.nativeAxis == -1) {
      Bind(p, action, NativeInput(m.nativeButton));
    }
  }
  if (dz != nullptr && dz->emulateTriggers) {
    if (!lSet) Bind(p, Action::PadL, Make(Device::PadButton, kTriggerClickLeft));
    if (!rSet) Bind(p, Action::PadR, Make(Device::PadButton, kTriggerClickRight));
  }
}

SDL_Gamepad* PortPad() {
  const s32 index = PADGetIndexForPort(kPort);
  return index >= 0 ? PADGetSDLGamepadForIndex(static_cast<u32>(index)) : nullptr;
}

bool ActivePad(std::string& guid, std::string& type) {
  guid.clear();
  type.clear();
  SDL_Gamepad* pad = PortPad();
  if (pad == nullptr) return false;
  char buf[33] = {};
  SDL_GUIDToString(SDL_GetJoystickGUID(SDL_GetGamepadJoystick(pad)), buf, sizeof(buf));
  guid = buf;
  if (const char* t = SDL_GetGamepadStringForType(SDL_GetGamepadType(pad))) type = t;
  return true;
}

void BuildProfile(Profile& out) {
  BuildBaseProfile(out);
  std::string guid;
  std::string type;
  ActivePad(guid, type);
  const std::shared_ptr<const PortInput::UserBindings> user = UserBindings();
  const PortInput::Selection sel = PortInput::Select(*user, guid, type);
  if (sel.base != nullptr) PortInput::Overlay(out, *sel.base);
  if (sel.pad != nullptr) PortInput::Overlay(out, *sel.pad);
}

float StickValue(s16 raw, u16 deadZone, bool useDeadZones) {
  if (useDeadZones && std::abs(static_cast<int>(raw)) <= deadZone) return 0.f;
  return std::clamp(static_cast<float>(raw) / 32767.f, -1.f, 1.f);
}

// _get_axis_value in Aurora's pad.cpp: one PAD axis's mapped input, 0..32767.
s16 MappedAxis(SDL_Gamepad* pad, const PADAxisMapping* axes, u32 count, PADAxis axis) {
  for (u32 i = 0; i < count; ++i) {
    if (axes[i].padAxis != axis) continue;
    if (axes[i].nativeAxis.nativeAxis != -1) {
      const int v = SDL_GetGamepadAxis(pad, static_cast<SDL_GamepadAxis>(axes[i].nativeAxis.nativeAxis)) *
                    axes[i].nativeAxis.sign;
      return static_cast<s16>(std::min(v, static_cast<int>(SDL_JOYSTICK_AXIS_MAX)));
    }
    if (axes[i].nativeButton >= 0 &&
        SDL_GetGamepadButton(pad, static_cast<SDL_GamepadButton>(axes[i].nativeButton))) {
      return SDL_JOYSTICK_AXIS_MAX;
    }
    return 0;
  }
  return 0;
}

void ReadPad(PortInput::RawState& raw, u32& extButtons) {
  SDL_Gamepad* pad = PortPad();
  if (pad == nullptr) return;
  for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
    if (SDL_GetGamepadButton(pad, static_cast<SDL_GamepadButton>(b))) raw.padButtons |= 1u << b;
  }
  const PADDeadZones* dz = PADGetDeadZones(kPort);
  const bool useDz = dz != nullptr && dz->useDeadzones;
  for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT && a < PortInput::kPadAxisCount; ++a) {
    const s16 v = SDL_GetGamepadAxis(pad, static_cast<SDL_GamepadAxis>(a));
    if (a == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || a == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
      raw.padAxes[a] = std::max(0.f, static_cast<float>(v) / 32767.f);
    } else {
      const bool left = a == SDL_GAMEPAD_AXIS_LEFTX || a == SDL_GAMEPAD_AXIS_LEFTY;
      raw.padAxes[a] = StickValue(v, dz == nullptr ? 0 : left ? dz->stickDeadZone : dz->substickDeadZone, useDz);
    }
  }
  u32 axisCount = 0;
  if (const PADAxisMapping* axes = PADGetAxisMappings(kPort, &axisCount); axes != nullptr && dz != nullptr) {
    if (MappedAxis(pad, axes, axisCount, PAD_AXIS_TRIGGER_L) > dz->leftTriggerActivationZone)
      raw.padButtons |= 1u << kTriggerClickLeft;
    if (MappedAxis(pad, axes, axisCount, PAD_AXIS_TRIGGER_R) > dz->rightTriggerActivationZone)
      raw.padButtons |= 1u << kTriggerClickRight;
  }
  // Aurora's extended buttons, which have no engine action (nothing in the game reads them).
  static constexpr std::pair<SDL_GamepadButton, PADExtButton> kExt[] = {
      {SDL_GAMEPAD_BUTTON_BACK, PAD_BUTTON_BACK},
      {SDL_GAMEPAD_BUTTON_GUIDE, PAD_BUTTON_GUIDE},
      {SDL_GAMEPAD_BUTTON_MISC1, PAD_BUTTON_MISC1},
      {SDL_GAMEPAD_BUTTON_MISC2, PAD_BUTTON_MISC2},
      {SDL_GAMEPAD_BUTTON_MISC3, PAD_BUTTON_MISC3},
      {SDL_GAMEPAD_BUTTON_MISC4, PAD_BUTTON_MISC4},
      {SDL_GAMEPAD_BUTTON_MISC5, PAD_BUTTON_MISC5},
      {SDL_GAMEPAD_BUTTON_MISC6, PAD_BUTTON_MISC6},
      {SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1, PAD_BUTTON_RIGHT_PADDLE1},
      {SDL_GAMEPAD_BUTTON_LEFT_PADDLE1, PAD_BUTTON_LEFT_PADDLE1},
      {SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2, PAD_BUTTON_RIGHT_PADDLE2},
      {SDL_GAMEPAD_BUTTON_LEFT_PADDLE2, PAD_BUTTON_LEFT_PADDLE2},
      {SDL_GAMEPAD_BUTTON_RIGHT_STICK, PAD_BUTTON_RIGHT_STICK},
      {SDL_GAMEPAD_BUTTON_LEFT_STICK, PAD_BUTTON_LEFT_STICK},
      {SDL_GAMEPAD_BUTTON_TOUCHPAD, PAD_BUTTON_TOUCHPAD},
  };
  for (const auto& [native, button] : kExt) {
    if (SDL_GetGamepadButton(pad, native)) extButtons |= button;
  }
}

// The mouse buttons drive the pad only while the port's mouse aim holds the capture (the F1 window
// and the port's own UI take the cursor back), and only from a neutral start, so the click that
// takes the capture is not also a shot. That also keeps the phone's touch-synthesized mouse off A.
uint32_t ReadMouseButtons() {
  static PortMouse::ButtonGate sGate;
  SDL_Window* window = SDL_GetMouseFocus();
  const bool live = PortMouse::Enabled() && !PortMouse::UiActive() && !PortOverlay::Visible() &&
                    window != nullptr && SDL_GetWindowRelativeMouseMode(window);
  const SDL_MouseButtonFlags state = SDL_GetMouseState(nullptr, nullptr);
  uint32_t held = 0;
  if (state & SDL_BUTTON_LMASK) held |= 1u << 0;
  if (state & SDL_BUTTON_MMASK) held |= 1u << 1;
  if (state & SDL_BUTTON_RMASK) held |= 1u << 2;
  if (state & SDL_BUTTON_X1MASK) held |= 1u << 3;
  if (state & SDL_BUTTON_X2MASK) held |= 1u << 4;
  return sGate.Poll(live, held);
}

// Aurora's stick conversion: x/256 truncated, y negated (SDL's y points down).
s8 StickX(float right, float left) {
  const long v = std::clamp(std::lround((right - left) * 32767.f), -32768L, 32767L);
  return static_cast<s8>(std::clamp(static_cast<int>(v / 256), -127, 127));
}

s8 StickY(float down, float up) {
  const auto yl = static_cast<Sint16>(std::clamp(std::lround((down - up) * 32767.f), -32768L, 32767L));
  const auto y = static_cast<Sint16>(-(yl + 1u) / 256u);
  return static_cast<s8>(std::clamp(static_cast<int>(y), -127, 127));
}

// A digital L/R reads as 180 the way Aurora's controller path sets it; a fully pulled analog (a
// key on LAnalog, as Q is) still reads 255, as Aurora's keyboard map gave it.
u8 Trigger(float analog, bool click) {
  const long v = std::clamp(std::lround(analog * 32767.f) / 128L, 0L, 255L);
  return static_cast<u8>(click ? std::max(v, 180L) : v);
}

void PublishLocked(PortInput::UserBindings bindings) {
  sUser = std::make_shared<const PortInput::UserBindings>(std::move(bindings));
}

// Loads controls.toml on first use and reloads it when it changed. Call with sUserMutex held.
void RefreshLocked() {
  const auto now = std::chrono::steady_clock::now();
  if (sUserLoaded && now - sUserChecked < std::chrono::seconds(1)) return;
  sUserChecked = now;
  if (!sUserLoaded) PortInput::SetKeyNames(ScancodeName, ScancodeFromName);
  const bool first = !sUserLoaded;
  sUserLoaded = true;
  if (sPath.empty()) return;
  std::error_code ec;
  const auto time = std::filesystem::last_write_time(sPath, ec);
  if (ec) {
    // No file (or it was deleted): no user bindings.
    if (sUserTime != std::filesystem::file_time_type{}) {
      sUserTime = {};
      PublishLocked({});
    }
    return;
  }
  if (!first && time == sUserTime) return;
  sUserTime = time;
  std::ifstream file(sPath, std::ios::binary);
  if (!file) return;
  std::ostringstream text;
  text << file.rdbuf();
  PortInput::UserBindings parsed;
  std::string error;
  if (PortInput::ParseUserBindings(text.str(), parsed, &error)) {
    std::fprintf(stderr, "[input] %s loaded (%zu profiles)\n", sPath.c_str(), parsed.profiles.size());
    PublishLocked(std::move(parsed));
  } else {
    // A half-finished hand edit keeps the bindings that were loaded before it.
    std::fprintf(stderr, "[input] %s ignored, keeping the last good bindings: %s\n", sPath.c_str(), error.c_str());
  }
}

} // namespace

void Initialize(const std::string& userDirectory) {
  std::lock_guard lock(sUserMutex);
  sPath = userDirectory.empty() ? std::string() : userDirectory + "/controls.toml";
  sUserLoaded = false;
}

const std::string& UserBindingsPath() { return sPath; }

std::shared_ptr<const PortInput::UserBindings> UserBindings() {
  std::lock_guard lock(sUserMutex);
  RefreshLocked();
  return sUser;
}

bool SetUserBindings(const PortInput::UserBindings& bindings) {
  std::lock_guard lock(sUserMutex);
  RefreshLocked();
  if (sPath.empty()) {
    PublishLocked(bindings);
    return false;
  }
  const std::string tmp = sPath + ".tmp";
  std::error_code ec;
  bool written = false;
  {
    std::ofstream file(tmp, std::ios::binary | std::ios::trunc);
    if (file) {
      file << PortInput::SerializeUserBindings(bindings);
      written = static_cast<bool>(file.flush());
    }
  }
  if (written) std::filesystem::rename(tmp, sPath, ec);
  // The bindings apply for the session either way; only the file can fail.
  PublishLocked(bindings);
  if (!written || ec) {
    std::filesystem::remove(tmp, ec);
    return false;
  }
  sUserTime = std::filesystem::last_write_time(sPath, ec);
  return true;
}

const Profile& BaseKeyboardProfile() {
  static const Profile sProfile = MakeKeyboardProfile();
  return sProfile;
}

void BuildBaseProfile(Profile& out) {
  out = BaseKeyboardProfile();
  BindController(out);
}

PADStatus Synthesize(const PortInput::Output& out) {
  PADStatus s{};
  static constexpr unsigned kButtons[] = {PAD_BUTTON_A,    PAD_BUTTON_B,     PAD_BUTTON_X,    PAD_BUTTON_Y,
                                          PAD_TRIGGER_Z,   PAD_BUTTON_START, PAD_TRIGGER_L,   PAD_TRIGGER_R,
                                          PAD_BUTTON_UP,   PAD_BUTTON_DOWN,  PAD_BUTTON_LEFT, PAD_BUTTON_RIGHT};
  for (unsigned b : kButtons) {
    if (out.Held(PadBitAction(b))) s.button |= static_cast<u16>(b);
  }
  s.stickX = StickX(out.Value(Action::MainRight), out.Value(Action::MainLeft));
  s.stickY = StickY(out.Value(Action::MainDown), out.Value(Action::MainUp));
  s.substickX = StickX(out.Value(Action::CRight), out.Value(Action::CLeft));
  s.substickY = StickY(out.Value(Action::CDown), out.Value(Action::CUp));
  s.triggerLeft = Trigger(out.Value(Action::LAnalog), (s.button & PAD_TRIGGER_L) != 0);
  s.triggerRight = Trigger(out.Value(Action::RAnalog), (s.button & PAD_TRIGGER_R) != 0);
  // Aurora reported the port present while its keyboard map was installed, idle or not.
  s.err = PAD_ERR_NONE;
  return s;
}

PADStatus Poll() {
  static Profile sProfile;
  static PortInput::Runtime sRuntime;
  static bool sInit = false;

  Profile next;
  BuildProfile(next);
  if (!sInit || next != sProfile) {
    sProfile = std::move(next);
    sRuntime.SetProfile(&sProfile);
    sInit = true;
  }

  PortInput::RawState raw;
  int numKeys = 0;
  if (const bool* keys = SDL_GetKeyboardState(&numKeys)) {
    for (int i = 0; i < numKeys && i < PortInput::kKeyCount; ++i) {
      if (keys[i]) raw.keys.set(static_cast<size_t>(i));
    }
  }
  raw.mouseButtons = ReadMouseButtons();
  u32 ext = 0;
  ReadPad(raw, ext);

  const double nowMs =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
  PADStatus s = Synthesize(sRuntime.Poll(raw, PortInput::kCtxGameplay, nowMs));
  s.extButton = ext;
  return s;
}

} // namespace PortInputDevices
