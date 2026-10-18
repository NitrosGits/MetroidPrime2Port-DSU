#include "port_input_devices.h"

// Ported from the Prime 1 port's platform/input/port_input_devices.cpp. What MP2 has no backend or
// setting for is left out, each at the place it would go: see the header for the list.

#include "port_input_remap_ui.h"
#include "port_freecam.h"
#include "port_gyro_motion.h"
#include "port_harness.h"
#include "port_input_map.h"
#include "port_mouse.h"
#include "port_overlay.h"
#include "port_touch.h"
#include "MetroidPrime/Player/CGameState.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_sensor.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_touch.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>

namespace aurora {
void request_screenshot() noexcept;
}

namespace PortInputDevices {
namespace {

using PortInput::Action;
using PortInput::Binding;
using PortInput::Device;
using PortInput::Input;
using PortInput::Profile;

constexpr u32 kPort = 0;

// `<user>/controls.toml`, set by Initialize; "" without a user directory.
std::string sPath;

// The user's bindings (controls.toml), loaded on first use and reloaded when the file
// changes (checked once a second), so hand edits apply without a restart. The game
// thread polls them and the console/F1 write them, so they're published as immutable
// snapshots under a lock.
std::mutex sUserMutex;
std::shared_ptr<const PortInput::UserBindings> sUser = std::make_shared<const PortInput::UserBindings>();
uint64_t sUserVersion = 0;
bool sUserLoaded = false;
std::filesystem::file_time_type sUserTime{};
std::chrono::steady_clock::time_point sUserChecked{};

// Touch overlay controls held now (codes 0..kTouchControlCount-1), set from Android's
// UI thread.
std::atomic<uint64_t> sTouchControls{0};
std::atomic<uint64_t> sTouchBound{0};

// Mouse wheel notches since the last poll (x1000, vertical then horizontal), added
// by an SDL event watch on the event thread.
std::atomic<int> sWheelMilli[2]{};

bool SDLCALL WheelWatch(void*, SDL_Event* event) {
  if (event->type != SDL_EVENT_MOUSE_WHEEL || event->wheel.which == SDL_TOUCH_MOUSEID) return true;
  const float flip = event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.f : 1.f;
  sWheelMilli[0].fetch_add(static_cast<int>(std::lround(event->wheel.y * flip * 1000.f)), std::memory_order_relaxed);
  sWheelMilli[1].fetch_add(static_cast<int>(std::lround(event->wheel.x * flip * 1000.f)), std::memory_order_relaxed);
  return true;
}

std::string ScancodeName(int scancode) {
  const char* name = SDL_GetScancodeName(static_cast<SDL_Scancode>(scancode));
  return name != nullptr ? name : "";
}

int ScancodeFromName(std::string_view name) {
  const SDL_Scancode code = SDL_GetScancodeFromName(std::string(name).c_str());
  return code == SDL_SCANCODE_UNKNOWN ? -1 : static_cast<int>(code);
}

void InstallKeyNames() { PortInput::SetKeyNames(ScancodeName, ScancodeFromName); }

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

// Aurora's PAD axes, as the port's actions (PAD_AXIS_LEFT_Y_POS is up).
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

// A native button code (SDL_GamepadButton or a trigger's half pull), as the raw
// state's pad button bit; -1 for none.
int NativeButtonCode(u32 native) {
  if (native == PAD_NATIVE_BUTTON_TRIGGER_LEFT) return PAD_RAW_BUTTON_TRIGGER_LEFT;
  if (native == PAD_NATIVE_BUTTON_TRIGGER_RIGHT) return PAD_RAW_BUTTON_TRIGGER_RIGHT;
  if (native < SDL_GAMEPAD_BUTTON_COUNT) return static_cast<int>(native);
  return -1;
}

Input Make(Device device, int code, int dir = 0) {
  Input in;
  in.device = device;
  in.code = static_cast<uint16_t>(code);
  in.dir = static_cast<int8_t>(dir);
  return in;
}

void Bind(Profile& p, Action action, const Input& in, uint16_t turboHz = 0) {
  if (action == Action::None || !in.Valid()) return;
  Binding b;
  b.action = action;
  b.inputs[0] = in;
  b.count = 1;
  b.turboHz = turboHz;
  p.bindings.push_back(b);
}

void BindIn(Profile& p, Action action, const Input& in, uint16_t contexts) {
  if (action == Action::None || !in.Valid()) return;
  Binding b;
  b.action = action;
  b.inputs[0] = in;
  b.count = 1;
  b.contexts = contexts;
  p.bindings.push_back(b);
}

Input NativeInput(s32 native) {
  const int code = native < 0 ? -1 : NativeButtonCode(static_cast<u32>(native));
  return code < 0 ? Input{} : Make(Device::PadButton, code);
}

// trigger_axis_taken in Aurora's pad.cpp: the L/R analog's input also presses
// another PAD button, so it drives only that button.
bool TriggerAxisTaken(const PADAxisMapping* axes, u32 axisCount, const PADButtonMapping* buttons,
                      u32 buttonCount, PADAxis axis) {
  const PADAxisMapping* m = nullptr;
  for (u32 i = 0; i < axisCount; ++i) {
    if (axes[i].padAxis == axis) {
      m = &axes[i];
      break;
    }
  }
  if (m == nullptr) return false;
  u32 native = static_cast<u32>(m->nativeButton);
  if (m->nativeAxis.nativeAxis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER) {
    native = PAD_NATIVE_BUTTON_TRIGGER_LEFT;
  } else if (m->nativeAxis.nativeAxis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
    native = PAD_NATIVE_BUTTON_TRIGGER_RIGHT;
  } else if (m->nativeAxis.nativeAxis != -1) {
    return false;
  }
  if (native == PAD_NATIVE_BUTTON_INVALID) return false;
  bool other = false;
  for (u32 i = 0; i < buttonCount; ++i) {
    if (buttons[i].nativeButton != native) continue;
    if (buttons[i].padButton == PAD_TRIGGER_L || buttons[i].padButton == PAD_TRIGGER_R) return false;
    other = true;
  }
  return other;
}

// One Aurora PAD mapping (buttons, axes, emulated trigger clicks) as bindings.
void BindMapping(Profile& p, const PADButtonMapping* buttons, u32 buttonCount, const PADAxisMapping* axes,
                 u32 axisCount, bool emulateTriggers) {
  bool lSet = false;
  bool rSet = false;
  for (u32 i = 0; i < buttonCount; ++i) {
    if (buttons[i].nativeButton == PAD_NATIVE_BUTTON_INVALID) continue;
    if (buttons[i].padButton == PAD_TRIGGER_L) lSet = true;
    if (buttons[i].padButton == PAD_TRIGGER_R) rSet = true;
    Bind(p, PadBitAction(buttons[i].padButton), NativeInput(static_cast<s32>(buttons[i].nativeButton)));
  }

  for (u32 i = 0; i < axisCount; ++i) {
    const PADAxisMapping& m = axes[i];
    const Action action = PadAxisAction(m.padAxis);
    const bool trigger = m.padAxis == PAD_AXIS_TRIGGER_L || m.padAxis == PAD_AXIS_TRIGGER_R;
    if (trigger && TriggerAxisTaken(axes, axisCount, buttons, buttonCount, m.padAxis)) continue;
    if (m.nativeAxis.nativeAxis >= 0 && m.nativeAxis.nativeAxis < PAD_RAW_AXIS_COUNT) {
      Bind(p, action, Make(Device::PadAxis, m.nativeAxis.nativeAxis, m.nativeAxis.sign < 0 ? -1 : 1));
    } else if (m.nativeAxis.nativeAxis == -1) {
      Bind(p, action, NativeInput(m.nativeButton));
    }
    // Aurora's emulated trigger click: a pulled L/R analog presses L/R when no
    // button is mapped to them.
    if (trigger && emulateTriggers) {
      const bool left = m.padAxis == PAD_AXIS_TRIGGER_L;
      if (!(left ? lSet : rSet)) {
        Bind(p, left ? Action::PadL : Action::PadR,
             Make(Device::PadButton,
                  left ? PAD_RAW_BUTTON_TRIGGER_LEFT_CLICK : PAD_RAW_BUTTON_TRIGGER_RIGHT_CLICK));
      }
    }
  }
}

void BindController(Profile& p) {
  u32 buttonCount = 0;
  u32 axisCount = 0;
  const PADButtonMapping* buttons = PADGetButtonMappings(kPort, &buttonCount);
  const PADAxisMapping* axes = PADGetAxisMappings(kPort, &axisCount);
  const PADDeadZones* dz = PADGetDeadZones(kPort);
  // No controller on the port: bind the standard defaults anyway, so a remap page
  // shows what a pad would get (nothing reads them until one connects).
  PADDefaultMapping fallback;
  if (buttons == nullptr || axes == nullptr) {
    PADGetDefaultMapping(&fallback, PAD_TYPE_STANDARD);
    buttons = fallback.buttons;
    axes = fallback.axes;
    buttonCount = PAD_BUTTON_COUNT;
    axisCount = PAD_AXIS_COUNT;
  }
  BindMapping(p, buttons, buttonCount, axes, axisCount, dz == nullptr || dz->emulateTriggers);
}

// MP1's BindKeyboard reads Aurora's key slots; MP2 never installs any (it has its own layout,
// below), so the keyboard's defaults are this profile instead. Q/E press L/R and pull the analog
// triggers fully. The mouse buttons are DefaultMouseAction's (left A, middle Y, right L) and fire
// under mouse aim only (ReadMouseButtons). The F1 Controls page edits it.
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
  for (int i = 0; i < PortInputMap::kMouseButtonCount; ++i) {
    const unsigned pad = PortInputMap::MouseActionInfo(PortInputMap::DefaultMouseAction(i)).padButton;
    Bind(p, PadBitAction(pad), Make(Device::MouseButton, kMouseGameplay + i));
  }
  return p;
}

bool IsPadFamily(const Input& in) { return in.device == Device::PadButton || in.device == Device::PadAxis; }

bool IsKeyboardFamily(const Input& in) { return in.device == Device::Key || in.device == Device::MouseButton; }

// LB is the pad's beam shift under twin stick, unless it presses another PAD button: held as the
// shift, it would turn that D-pad press into a beam. (MP1's PadButtonBoundBesidesL; MP2 has no
// alt-button setting to check, only Aurora's mappings.)
bool PadButtonBoundBesidesL(s32 native) {
  u32 count = 0;
  const PADButtonMapping* list = PADGetButtonMappings(kPort, &count);
  for (u32 i = 0; list != nullptr && i < count; ++i) {
    if (list[i].nativeButton == static_cast<u32>(native) && list[i].padButton != PAD_TRIGGER_L) return true;
  }
  return false;
}

// MP1's BindBeamChords: a shift plus a D-pad direction picks a beam, the direction the retail
// C-stick picks them with (Power up, Dark left, Light right, Annihilator down), for the layouts
// that take the C-stick away (the touch overlay's twin stick). A shift pairs with the D-pad inputs
// of its own family: the keyboard's shift with the keyboard's D-pad keys, the pad's shift with the
// pad's D-pad. MP1's shifts come from settings (key slots, mouse shift action, pad shift) that MP2
// does not have, so only its twin-stick shifts exist: Left Shift unless a key takes it, and LB.
// The overlay's own Beam button works inside PortTouch. MP1 also makes the shifts spring the ball
// in morph ball; MP2's RouteDirectActions has no spring request (MP2_SPRING_BALL is CMorphBall's).
void BindBeamChords(Profile& p) {
  if (!PortTouch::TwinStick()) return;
  std::vector<Input> keyShifts;
  std::vector<Input> padShifts;
  bool lshiftTaken = false;
  for (const Binding& b : p.bindings) {
    if (b.count == 1 && b.inputs[0].device == Device::Key && b.inputs[0].code == SDL_SCANCODE_LSHIFT) {
      lshiftTaken = true;
    }
  }
  if (!lshiftTaken) keyShifts.push_back(Make(Device::Key, SDL_SCANCODE_LSHIFT));
  if (!PortTouch::ControlsEnabled() && !PadButtonBoundBesidesL(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)) {
    padShifts.push_back(NativeInput(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
  }

  // The engine's beam actions are MP1's names for MP2's slots, in the order RouteDirectActions
  // requests them: Power, Ice (Dark), Wave (Light), Plasma (Annihilator).
  static constexpr Action kDirs[4] = {Action::PadUp, Action::PadDown, Action::PadLeft, Action::PadRight};
  static constexpr Action kBeams[4] = {Action::BeamPower, Action::BeamPlasma, Action::BeamIce, Action::BeamWave};
  const std::vector<Binding> base = p.bindings;
  const uint16_t noBall = static_cast<uint16_t>(PortInput::kCtxAll & ~PortInput::kCtxMorphBall);
  for (int d = 0; d < 4; ++d) {
    for (const Binding& dir : base) {
      if (dir.action != kDirs[d] || dir.count != 1) continue;
      const bool keyboard = IsKeyboardFamily(dir.inputs[0]);
      if (!keyboard && !IsPadFamily(dir.inputs[0])) continue;
      for (const Input& shift : keyboard ? keyShifts : padShifts) {
        if (!shift.Valid() || shift == dir.inputs[0]) continue;
        Binding b;
        b.action = kBeams[d];
        b.inputs[0] = shift;
        b.inputs[1] = dir.inputs[0];
        b.count = 2;
        b.contexts = noBall;
        p.bindings.push_back(b);
      }
    }
  }
}

// MP1's turbo (A on one poll and off the next from the turbo key and pad slots) hangs off settings
// MP2 does not have; the engine's per-binding turbo_hz (Bind's turboHz, controls.toml, the F1
// Controls page's turbo rows) is MP1's and works. The touch overlay's Turbo is PortTouch's own.
// Not carried either: the pad's alt buttons (a setting), save states on F5/F9 (no backend; F5-F10
// were MP2's warp keys), gyro aim (no gyro sensor reader or aim consumer), and
// pointer menus' Escape (no pointer menus).
void BindPortControls(Profile& p) {
  // The port's own keys and the pad's overlay chord. Back + Start is ordered both
  // ways, so either can be held first; the chord also keeps Start from pausing.
  Bind(p, Action::PortMenu, Make(Device::Key, SDL_SCANCODE_F1));
  const Input back = NativeInput(SDL_GAMEPAD_BUTTON_BACK);
  const Input start = NativeInput(SDL_GAMEPAD_BUTTON_START);
  if (back.Valid() && start.Valid()) {
    for (int order = 0; order < 2; ++order) {
      Binding b;
      b.action = Action::PortMenu;
      b.inputs[0] = order == 0 ? back : start;
      b.inputs[1] = order == 0 ? start : back;
      b.count = 2;
      p.bindings.push_back(b);
    }
  }
  Bind(p, Action::Screenshot, Make(Device::Key, SDL_SCANCODE_F12));
  // Photo mode toggles the free camera (PortFreeCam; stage 1 has no photo panel).
  Bind(p, Action::PhotoMode, Make(Device::Key, SDL_SCANCODE_F8));

  BindBeamChords(p);

  // The wheel cycles beams, as in PrimeHack; retail has no wheel.
  Bind(p, Action::BeamPrev, Make(Device::MouseWheel, 0, 1));
  Bind(p, Action::BeamNext, Make(Device::MouseWheel, 0, -1));

  // Mouse buttons: under mouse aim each does its action (BaseKeyboardProfile's rows);
  // out of it the ones on A and B still press them (bombs, boosts, text boxes),
  // and in morph ball the ones on Y and R too (power bombs, spider ball).
  for (int i = 0; i < PortInputMap::kMouseButtonCount; ++i) {
    const int action = PortInputMap::DefaultMouseAction(i);
    if (action == PortInputMap::kMA_Shift) continue;
    const unsigned pad = PortInputMap::MouseActionInfo(action).padButton;
    const Action a = PadBitAction(pad);
    if (pad == PAD_BUTTON_A || pad == PAD_BUTTON_B) {
      Bind(p, a, Make(Device::MouseButton, kMouseMenu + i));
    } else if (pad == PAD_BUTTON_Y || pad == PAD_TRIGGER_R) {
      BindIn(p, a, Make(Device::MouseButton, kMouseMenu + i), PortInput::kCtxMorphBall);
    }
  }
}

float StickValue(s16 raw, u16 deadZone, bool useDeadZones) {
  if (useDeadZones && std::abs(static_cast<int>(raw)) <= deadZone) return 0.f;
  return std::clamp(static_cast<float>(raw) / 32767.f, -1.f, 1.f);
}

uint16_t GameContexts();

// The mouse buttons drive the pad's gameplay rows only while the port's mouse aim holds the
// capture (the F1 window and the port's own UI take the cursor back), and only from a neutral
// start, so the click that takes the capture is not also a shot. That also keeps the phone's
// touch-synthesized mouse off A. (MP1's MouseWeaponButtons.)
uint32_t ReadMouseButtons(unsigned mouseHeld) {
  static PortMouse::ButtonGate sGate;
  SDL_Window* window = SDL_GetMouseFocus();
  const bool live = PortMouse::Enabled() && PortOverlay::MouseButtons() && !PortMouse::UiActive() && !PortOverlay::Visible() &&
                    window != nullptr && SDL_GetWindowRelativeMouseMode(window);
  return sGate.Poll(live, mouseHeld & PortInputMap::kMouseButtonMask);
}

// SDL's mouse button state, in mouse button code order (left, middle, right, X1, X2).
unsigned SdlMouseHeld() {
  const SDL_MouseButtonFlags state = SDL_GetMouseState(nullptr, nullptr);
  unsigned held = 0;
  if (state & SDL_BUTTON_LMASK) held |= 1u << 0;
  if (state & SDL_BUTTON_MMASK) held |= 1u << 1;
  if (state & SDL_BUTTON_RMASK) held |= 1u << 2;
  if (state & SDL_BUTTON_X1MASK) held |= 1u << 3;
  if (state & SDL_BUTTON_X2MASK) held |= 1u << 4;
  return held;
}

void FillRawState(PortInput::RawState& raw, unsigned mouseHeld, bool focused) {
  int numKeys = 0;
  if (const bool* keys = SDL_GetKeyboardState(&numKeys)) {
    for (int i = 0; i < numKeys && i < PortInput::kKeyCount; ++i) {
      if (keys[i]) raw.keys.set(static_cast<size_t>(i));
    }
  }

  PADRawState pad{};
  if (PADGetRawState(kPort, &pad)) {
    raw.padButtons = pad.buttons;
    const PADDeadZones* dz = PADGetDeadZones(kPort);
    const bool useDz = dz != nullptr && dz->useDeadzones;
    for (int a = 0; a < PAD_RAW_AXIS_COUNT; ++a) {
      if (a == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || a == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
        raw.padAxes[a] = std::max(0.f, static_cast<float>(pad.axes[a]) / 32767.f);
      } else {
        const bool left = a == SDL_GAMEPAD_AXIS_LEFTX || a == SDL_GAMEPAD_AXIS_LEFTY;
        raw.padAxes[a] = StickValue(pad.axes[a], dz == nullptr ? 0 : left ? dz->stickDeadZone : dz->substickDeadZone,
                                    useDz);
      }
    }
  }

  uint32_t mouse = 0;
  // An input capture reads the mouse buttons as SDL has them.
  mouse |= (SDL_GetMouseState(nullptr, nullptr) & PortInputMap::kMouseButtonMask) << kMouseKeySlot;
  int actions[PortInputMap::kMouseButtonCount];
  for (int i = 0; i < PortInputMap::kMouseButtonCount; ++i) {
    actions[i] = PortInputMap::DefaultMouseAction(i);
  }
  // Its gate also waits for a release, so a held charge carried into morph ball
  // doesn't drop a bomb; only the buttons with something to do there feed it
  // (A/B, plus Y/R in morph ball), so a held side button doesn't hold it shut.
  // Live out of gameplay (MP1's MouseMenuButtons), with the mouse aim on.
  unsigned menuPad = PAD_BUTTON_A | PAD_BUTTON_B;
  const uint16_t contexts = GameContexts();
  if ((contexts & PortInput::kCtxMorphBall) != 0) menuPad |= PAD_BUTTON_Y | PAD_TRIGGER_R;
  const unsigned menuMask = PortInputMap::MouseButtonsFor(actions, menuPad);
  static PortMouse::ButtonGate sMenuGate;
  const bool menuLive =
      PortMouse::Enabled() && (contexts & PortInput::kCtxGameplay) == 0 && !PortOverlay::Visible() && focused;
  mouse |= (sMenuGate.Poll(menuLive, mouseHeld & menuMask) & PortInputMap::kMouseButtonMask) << kMouseMenu;
  mouse |= (ReadMouseButtons(mouseHeld) & PortInputMap::kMouseButtonMask) << kMouseGameplay;
  raw.mouseButtons = mouse;

  // A whole notch is a press for one poll; a smooth-scrolling wheel's fractions add
  // up to one, and extra notches in one poll are dropped. Scrolling over the F1
  // overlay or an unfocused window is dropped too.
  static bool sWheelWatch = false;
  if (!sWheelWatch) sWheelWatch = SDL_AddEventWatch(WheelWatch, nullptr);
  const bool wheelLive = focused && !PortOverlay::Visible();
  for (int i = 0; i < 2; ++i) {
    const int milli = sWheelMilli[i].exchange(0, std::memory_order_relaxed);
    if (!wheelLive) continue;
    if (std::abs(milli) >= 1000) {
      raw.wheel[i] = milli > 0 ? 1.f : -1.f;
    } else if (milli != 0) {
      sWheelMilli[i].fetch_add(milli, std::memory_order_relaxed);
    }
  }

  // MP1 also sets its beam shift, turbo and map-tap codes here; PortTouch does those itself.
  raw.touch = sTouchControls.load(std::memory_order_relaxed);
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

// The scan text is up (the scan freeze): it reads A while the lock stays held.
bool ScanTextOpen(uint16_t contexts) {
  return (contexts & PortInput::kCtxMenu) != 0 && (contexts & PortInput::kCtxScan) != 0;
}

// The scan text's Menu context comes and goes under the held lock: switching it
// must not consume the lock's input, which the Menu rows also use.
uint16_t QuietContexts(uint16_t before, uint16_t after) {
  return ScanTextOpen(before) || ScanTextOpen(after) ? PortInput::kCtxMenu : 0;
}

// Player 0's state while the game is in play (PortTouch's PlayerInPlay), else null.
CPlayerState* PlayerInPlay() {
  if (gpGameState == nullptr || PortHarness::FlowState() != 1 /* CMFGame::kFS_InGame */) return nullptr;
  return gpGameState->PlayerState(0).GetPtr();
}

// Menu (front end, pause screen, or anything that is not in play) or gameplay; the
// scan visor adds scan to gameplay. MP1 also tells the map screen and morph ball
// apart and adds menu while the scan text soft-pauses the game: those read the
// CStateManager and CPlayer, which nothing outside src/ holds in MP2.
uint16_t GameContexts() {
  const CPlayerState* state = PlayerInPlay();
  if (state == nullptr || PortTouch::PauseScreenOpen()) return PortInput::kCtxMenu;
  uint16_t contexts = PortInput::kCtxGameplay;
  if (state->GetCurrentVisor() == CPlayerState::kPV_Scan) contexts |= PortInput::kCtxScan;
  return contexts;
}

// A next/previous pick the game hasn't taken yet.
struct CyclePending {
  int target = -1;
  uint64_t untilNs = 0;
};
CyclePending sCyclePending[2]; // [0] visor, [1] beam

// The beam or visor the game is on or switching to; -1 out of a game.
int CurrentPick(bool beam) {
  const CPlayerState* state = PlayerInPlay();
  if (state == nullptr) return -1;
  return beam ? static_cast<int>(state->GetCurrentBeam()) : static_cast<int>(state->GetTransitioningVisor());
}

bool OwnsPick(const CPlayerState& state, bool beam, int pick) {
  static constexpr CPlayerState::EItemType kBeamItems[] = {CPlayerState::kIT_PowerBeam, CPlayerState::kIT_DarkBeam,
                                                           CPlayerState::kIT_LightBeam,
                                                           CPlayerState::kIT_AnnihilatorBeam};
  if (beam) return state.HasPowerUp(kBeamItems[pick]);
  return state.HasVisor(static_cast<CPlayerState::EPlayerVisor>(pick)) != 0;
}

// Next/previous beam or visor: the owned one `dir` steps from the current pick, in
// the pause screen's order (Power, Dark, Light, Annihilator; Combat, Echo, Scan, Dark),
// as PrimeHack cycles them. -1 when there's nothing else to pick.
int CycleTarget(bool beam, int dir) {
  // EBeamId / EPlayerVisor values in cycle order.
  static constexpr int kBeamOrder[] = {CPlayerState::kBI_Power, CPlayerState::kBI_Dark, CPlayerState::kBI_Light,
                                       CPlayerState::kBI_Annihilator};
  static constexpr int kVisorOrder[] = {CPlayerState::kPV_Combat, CPlayerState::kPV_Echo, CPlayerState::kPV_Scan,
                                        CPlayerState::kPV_Dark};
  const CPlayerState* state = PlayerInPlay();
  const int dest = CurrentPick(beam);
  if (state == nullptr || dest < 0) return -1;
  const int* order = beam ? kBeamOrder : kVisorOrder;
  // Count from a pick the game hasn't taken yet, so quick notches keep stepping.
  const int current = sCyclePending[beam].target >= 0 ? sCyclePending[beam].target : dest;
  int pos = 0;
  while (pos < 4 && order[pos] != current) ++pos;
  if (pos == 4) pos = 0;
  for (int step = 1; step < 4; ++step) {
    const int i = ((pos + dir * step) % 4 + 4) % 4;
    if (OwnsPick(*state, beam, order[i])) return order[i];
  }
  return -1;
}

// Starts a cycle pick, or re-sends the pending one each poll until the game has
// taken it (a request the visor or gun ignores while still switching is dropped).
// `beam` picks which; `target` -1 just keeps the pending one alive.
void PushCyclePick(bool beam, int target) {
  CyclePending& p = sCyclePending[beam];
  const uint64_t now = SDL_GetTicksNS();
  if (target >= 0) {
    p.target = target;
    p.untilNs = now + 1'500'000'000ull;
  }
  if (p.target < 0) return;
  if (now > p.untilNs || CurrentPick(beam) == p.target) {
    p.target = -1;
    return;
  }
  if (beam) {
    PortTouch::RequestBeam(p.target);
  } else {
    PortTouch::RequestVisor(p.target);
  }
}

// The actions that aren't pad inputs go straight to the code that does them.
void RouteDirectActions(const PortInput::Output& out) {
  // EBeamId and EPlayerVisor order, as RequestBeam/RequestVisor take them. The engine's
  // action names are MP1's; each stands for the MP2 beam or visor in its slot (Ice is
  // Dark, Wave Light, Plasma Annihilator; X-ray is Echo, Thermal Dark).
  static constexpr Action kBeams[] = {Action::BeamPower, Action::BeamIce, Action::BeamWave, Action::BeamPlasma};
  static constexpr Action kVisors[] = {Action::VisorCombat, Action::VisorXray, Action::VisorScan,
                                       Action::VisorThermal};
  for (int i = 0; i < 4; ++i) {
    // A direct pick replaces a pending cycle pick.
    if (out.Pressed(kBeams[i])) {
      sCyclePending[1].target = -1;
      PortTouch::RequestBeam(i);
    }
    if (out.Pressed(kVisors[i])) {
      sCyclePending[0].target = -1;
      PortTouch::RequestVisor(i);
    }
  }
  const int beamDir = out.Pressed(Action::BeamNext) ? 1 : out.Pressed(Action::BeamPrev) ? -1 : 0;
  PushCyclePick(true, beamDir != 0 ? CycleTarget(true, beamDir) : -1);
  const int visorDir = out.Pressed(Action::VisorNext) ? 1 : out.Pressed(Action::VisorPrev) ? -1 : 0;
  PushCyclePick(false, visorDir != 0 ? CycleTarget(false, visorDir) : -1);
  // MP1's SpringBall action requests a spring ball here; MP2 has no request for one
  // (MP2_SPRING_BALL is CMorphBall's own).
}

// The port's own actions: polled every frame by PollPortActions, not with the game
// ticks, so they work while the game is paused or the F1 overlay is open.
bool IsPortAction(Action a) { return a == Action::PortMenu || a == Action::Screenshot || a == Action::PhotoMode; }

// Keys pressed since the last PollPortActions, added by an SDL event watch: a short
// tap can begin and end between two frames, so the key state alone misses it.
std::atomic<uint64_t> sKeyDowns[(PortInput::kKeyCount + 63) / 64]{};

bool SDLCALL KeyDownWatch(void*, SDL_Event* event) {
  if (event->type != SDL_EVENT_KEY_DOWN || event->key.repeat) return true;
  const int code = static_cast<int>(event->key.scancode);
  if (code >= 0 && code < PortInput::kKeyCount) {
    sKeyDowns[code / 64].fetch_or(uint64_t{1} << (code % 64), std::memory_order_relaxed);
  }
  return true;
}

// A digital L/R reads as 180 the way Aurora's controller path sets it; a fully pulled analog (a
// key on LAnalog, as Q is) still reads 255, as Aurora's keyboard map gave it. (MP1 returns 180
// for any click.)
u8 Trigger(float analog, bool click) {
  const long v = std::clamp(std::lround(analog * 32767.f) / 128L, 0L, 255L);
  return static_cast<u8>(click ? std::max(v, 180L) : v);
}

void PublishLocked(PortInput::UserBindings bindings) {
  sUser = std::make_shared<const PortInput::UserBindings>(std::move(bindings));
  ++sUserVersion;
}

// Loads controls.toml on first use and reloads it when it changed. Call with
// sUserMutex held.
void RefreshLocked() {
  const auto now = std::chrono::steady_clock::now();
  if (sUserLoaded && now - sUserChecked < std::chrono::seconds(1)) return;
  sUserChecked = now;
  if (!sUserLoaded) InstallKeyNames();
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
  int dropped = 0;
  if (PortInput::ParseUserBindings(text.str(), parsed, &error, &dropped)) {
    std::fprintf(stderr, "[input] %s loaded (%zu profiles)\n", sPath.c_str(), parsed.profiles.size());
    if (dropped > 0) {
      std::fprintf(stderr, "[input] %s: skipped %d beam_shift row(s)\n", sPath.c_str(), dropped);
    }
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

uint64_t UserBindingsVersion() {
  std::lock_guard lock(sUserMutex);
  RefreshLocked();
  return sUserVersion;
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

void SetTouchControl(int control, bool down) {
  if (control < 0 || control >= kTouchControlCount) return;
  const uint64_t bit = uint64_t{1} << control;
  if (down) sTouchControls.fetch_or(bit, std::memory_order_relaxed);
  else sTouchControls.fetch_and(~bit, std::memory_order_relaxed);
}

// Read from the Android UI thread: the mask is published by the game thread's
// poll, so no SDL calls or file reads happen there.
bool TouchControlBound(int control) {
  if (control < 0 || control >= kTouchControlCount) return false;
  return (sTouchBound.load(std::memory_order_relaxed) >> control) & 1;
}

static uint64_t TouchBoundMask(const SActiveProfiles& active) {
  uint64_t mask = 0;
  for (const PortInput::UserProfile* p : {active.sel.base, active.sel.pad}) {
    if (p == nullptr) continue;
    for (const Binding& b : p->bindings) {
      for (int i = 0; i < b.count; ++i) {
        const Input& in = b.inputs[size_t(i)];
        if (in.device == Device::Touch && in.code < kTouchControlCount) mask |= uint64_t{1} << in.code;
      }
    }
  }
  return mask;
}

bool ActivePad(std::string& guid, std::string& type, std::string& name) {
  guid.clear();
  type.clear();
  name.clear();
  const s32 index = PADGetIndexForPort(kPort);
  SDL_Gamepad* pad = index >= 0 ? PADGetSDLGamepadForIndex(static_cast<u32>(index)) : nullptr;
  if (pad == nullptr) return false;
  char buf[33] = {};
  SDL_GUIDToString(SDL_GetJoystickGUID(SDL_GetGamepadJoystick(pad)), buf, sizeof(buf));
  guid = buf;
  if (const char* t = SDL_GetGamepadStringForType(SDL_GetGamepadType(pad))) type = t;
  if (const char* n = SDL_GetGamepadName(pad)) name = n;
  return true;
}

SActiveProfiles ActiveUserProfiles() {
  std::string guid;
  std::string type;
  std::string name;
  ActivePad(guid, type, name);
  SActiveProfiles active;
  active.bindings = UserBindings();
  active.sel = PortInput::Select(*active.bindings, guid, type);
  return active;
}

void ReadRaw(PortInput::RawState& raw) {
  raw = PortInput::RawState{};
  int numKeys = 0;
  if (const bool* keys = SDL_GetKeyboardState(&numKeys)) {
    for (int i = 0; i < numKeys && i < PortInput::kKeyCount; ++i) {
      if (keys[i]) raw.keys.set(static_cast<size_t>(i));
    }
  }
  PADRawState pad{};
  if (PADGetRawState(kPort, &pad)) {
    raw.padButtons = pad.buttons;
    for (int a = 0; a < PAD_RAW_AXIS_COUNT && a < PortInput::kPadAxisCount; ++a) {
      raw.padAxes[a] = std::clamp(static_cast<float>(pad.axes[a]) / 32767.f, -1.f, 1.f);
    }
  }
  raw.mouseButtons = (SDL_GetMouseState(nullptr, nullptr) & PortInputMap::kMouseButtonMask) << kMouseKeySlot;
  raw.touch = sTouchControls.load(std::memory_order_relaxed);
}

std::vector<SPadInfo> ConnectedPads() {
  std::vector<SPadInfo> pads;
  int count = 0;
  SDL_JoystickID* ids = SDL_GetGamepads(&count);
  for (int i = 0; ids != nullptr && i < count; ++i) {
    if (SDL_IsJoystickVirtual(ids[i])) continue;
    SPadInfo info;
    char buf[33] = {};
    SDL_GUIDToString(SDL_GetJoystickGUIDForID(ids[i]), buf, sizeof(buf));
    info.guid = buf;
    if (const char* t = SDL_GetGamepadStringForType(SDL_GetGamepadTypeForID(ids[i]))) info.type = t;
    if (const char* n = SDL_GetGamepadNameForID(ids[i])) info.name = n;
    pads.push_back(std::move(info));
  }
  SDL_free(ids);
  return pads;
}

const Profile& BaseKeyboardProfile() {
  static const Profile sProfile = MakeKeyboardProfile();
  return sProfile;
}

static void BindMenuDefaults(Profile& p);

void BuildBaseProfile(Profile& out) {
  out = Profile{};
  BindController(out);
  const Profile& keyboard = BaseKeyboardProfile();
  out.bindings.insert(out.bindings.end(), keyboard.bindings.begin(), keyboard.bindings.end());
  BindPortControls(out);
  BindMenuDefaults(out);
}

// The Port menu can't be unbound for good: with no key or mouse input left for it
// (and no pad one while a pad is connected), F1 opens it again, so a hand-edited
// controls.toml or an unplugged pad can't lock anyone out of the settings.
static void KeepPortMenuReachable(Profile& p) {
  bool pad = false;
  for (const Binding& b : p.bindings) {
    if (b.action != Action::PortMenu || b.count == 0) continue;
    const PortInput::Family family = PortInput::BindingFamily(b);
    if (family == PortInput::Family::Keyboard) return;
    pad = pad || family == PortInput::Family::Pad;
  }
  if (pad && SDL_HasGamepad()) return;
  Bind(p, Action::PortMenu, Make(Device::Key, SDL_SCANCODE_F1));
}

void BuildProfile(Profile& out) {
  BuildBaseProfile(out);
  const SActiveProfiles user = ActiveUserProfiles();
  sTouchBound.store(TouchBoundMask(user), std::memory_order_relaxed);
  if (user.sel.base != nullptr) PortInput::Overlay(out, *user.sel.base);
  if (user.sel.pad != nullptr) PortInput::Overlay(out, *user.sel.pad);
  KeepPortMenuReachable(out);
}

// The PAD buttons menus and the map screen read (confirm, back, page, zoom, the
// D-pad), each with its own "Menu" action. Start (pause) and Z (map) have none:
// they keep their gameplay inputs, so the button that opens a screen closes it.
struct SMenuAction {
  Action game;
  Action menu;
  unsigned padButton; // 0 for the analogs
};
constexpr SMenuAction kMenuActions[] = {
    {Action::PadA, Action::MenuA, PAD_BUTTON_A},
    {Action::PadB, Action::MenuB, PAD_BUTTON_B},
    {Action::PadX, Action::MenuX, PAD_BUTTON_X},
    {Action::PadY, Action::MenuY, PAD_BUTTON_Y},
    {Action::PadL, Action::MenuL, PAD_TRIGGER_L},
    {Action::PadR, Action::MenuR, PAD_TRIGGER_R},
    {Action::LAnalog, Action::MenuLAnalog, 0},
    {Action::RAnalog, Action::MenuRAnalog, 0},
    {Action::PadUp, Action::MenuUp, PAD_BUTTON_UP},
    {Action::PadDown, Action::MenuDown, PAD_BUTTON_DOWN},
    {Action::PadLeft, Action::MenuLeft, PAD_BUTTON_LEFT},
    {Action::PadRight, Action::MenuRight, PAD_BUTTON_RIGHT},
};
constexpr uint16_t kMenus = PortInput::kCtxMenu | PortInput::kCtxMap;

static const SMenuAction* FindMenuAction(Action a, bool menu) {
  for (const SMenuAction& m : kMenuActions) {
    if ((menu ? m.menu : m.game) == a) return &m;
  }
  return nullptr;
}

// Port 0's pad, unless it's missing or the touch overlay's virtual one.
static SDL_Gamepad* RealPad() {
  const s32 index = PADGetIndexForPort(kPort);
  SDL_Gamepad* pad = index >= 0 ? PADGetSDLGamepadForIndex(static_cast<u32>(index)) : nullptr;
  return pad == nullptr || SDL_IsJoystickVirtual(SDL_GetGamepadID(pad)) ? nullptr : pad;
}

// The Menu rows' defaults: the pad type's standard mapping (A confirms, B backs
// out, the triggers page and zoom), less the buttons and axes the pad lacks (a
// GameCube-typed pad's L/R clicks, say, so L/R fall back to the trigger click).
// Without a pad, the standard pad's, so a remap page shows what one would get.
static void BindMenuDefaults(Profile& p) {
  PADDefaultMapping def;
  SDL_Gamepad* pad = RealPad();
  PADGetDefaultMapping(&def, pad != nullptr ? PADGetControllerType(kPort) : PAD_TYPE_STANDARD);
  if (pad != nullptr) {
    for (PADButtonMapping& m : def.buttons) {
      if (m.nativeButton < SDL_GAMEPAD_BUTTON_COUNT &&
          !SDL_GamepadHasButton(pad, static_cast<SDL_GamepadButton>(m.nativeButton))) {
        m.nativeButton = PAD_NATIVE_BUTTON_INVALID;
      }
    }
    for (PADAxisMapping& m : def.axes) {
      if (m.nativeAxis.nativeAxis >= 0 && m.nativeAxis.nativeAxis < SDL_GAMEPAD_AXIS_COUNT &&
          !SDL_GamepadHasAxis(pad, static_cast<SDL_GamepadAxis>(m.nativeAxis.nativeAxis))) {
        m.nativeAxis.nativeAxis = -1;
        m.nativeButton = PAD_NATIVE_BUTTON_INVALID;
      }
    }
  }
  Profile standard;
  const PADDeadZones* dz = PADGetDeadZones(kPort);
  BindMapping(standard, def.buttons, PAD_BUTTON_COUNT, def.axes, PAD_AXIS_COUNT,
              dz == nullptr || dz->emulateTriggers);
  for (Binding b : standard.bindings) {
    const SMenuAction* m = FindMenuAction(b.action, false);
    if (m == nullptr) continue;
    b.action = m->menu;
    b.contexts = kMenus;
    p.bindings.push_back(b);
  }
}

// MP1's "Separate menu buttons" setting (MenuStandardButtons, off under touch). MP2 has no such
// setting yet, so it is off: menus read the gameplay layout, as they did before the engine.
static bool MenuButtonsActive() { return false; }

// With the option on, menus and the map read the Menu rows instead of the gameplay
// layout: pad bindings meant for everywhere stop applying there for the buttons
// those rows cover, and keyboard and mouse bindings for everywhere stop applying
// there on an input a Menu row uses (bindings the user scoped to menus or the map,
// Start/Z, the sticks and touch are left alone). Menu rows apply only in menus and
// the map, and never on Start's or Z's input. With it off, the rows do
// nothing. Only the game's poll applies this, so a remap page lists the bindings
// as saved.
static void ApplyMenuButtons(Profile& p) {
  const auto isMenu = [](const Binding& b) { return FindMenuAction(b.action, true) != nullptr; };
  if (!MenuButtonsActive()) {
    std::erase_if(p.bindings, isMenu);
    return;
  }
  std::vector<Input> kept;
  for (Binding& b : p.bindings) {
    if (isMenu(b)) {
      b.contexts &= kMenus;
      if (b.contexts == 0) b.contexts = kMenus;
      continue;
    }
    if (b.count != 1 || (b.contexts & kMenus) == 0) continue;
    if (b.action == Action::PadStart || b.action == Action::PadZ) {
      kept.push_back(b.inputs[0]);
    } else if (IsPadFamily(b.inputs[0]) && FindMenuAction(b.action, false) != nullptr &&
               b.contexts == PortInput::kCtxAll) {
      b.contexts &= ~kMenus;
    }
  }
  std::erase_if(p.bindings, [&](const Binding& b) {
    return isMenu(b) && b.count == 1 && std::find(kept.begin(), kept.end(), b.inputs[0]) != kept.end();
  });
  // A key or mouse button on a Menu row replaces what it does in gameplay while in
  // menus and the map, rather than doing both.
  std::vector<Input> rows;
  for (const Binding& b : p.bindings) {
    if (isMenu(b) && b.count == 1 && !IsPadFamily(b.inputs[0])) rows.push_back(b.inputs[0]);
  }
  for (Binding& b : p.bindings) {
    if (isMenu(b) || b.count != 1 || b.contexts != PortInput::kCtxAll) continue;
    if (std::find(rows.begin(), rows.end(), b.inputs[0]) != rows.end()) b.contexts &= ~kMenus;
  }
}

// A single pad input as the native button the prompts look up, or invalid.
static u32 PromptNative(const Input& in) {
  if (in.device == Device::PadButton) {
    if (in.code < SDL_GAMEPAD_BUTTON_COUNT) return in.code;
    if (in.code == PAD_RAW_BUTTON_TRIGGER_LEFT || in.code == PAD_RAW_BUTTON_TRIGGER_LEFT_CLICK) {
      return PAD_NATIVE_BUTTON_TRIGGER_LEFT;
    }
    if (in.code == PAD_RAW_BUTTON_TRIGGER_RIGHT || in.code == PAD_RAW_BUTTON_TRIGGER_RIGHT_CLICK) {
      return PAD_NATIVE_BUTTON_TRIGGER_RIGHT;
    }
  } else if (in.device == Device::PadAxis && in.dir >= 0) {
    if (in.code == SDL_GAMEPAD_AXIS_LEFT_TRIGGER) return PAD_NATIVE_BUTTON_TRIGGER_LEFT;
    if (in.code == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) return PAD_NATIVE_BUTTON_TRIGGER_RIGHT;
  }
  return PAD_NATIVE_BUTTON_INVALID;
}

u32 PromptNativeButton(const Input& in) { return PromptNative(in); }

// The polled profile's single-input bindings, for the prompts. Set by Poll, read
// wherever prompts are drawn.
static std::mutex sPromptMutex;
static std::vector<Binding> sPromptBindings;

static void UpdatePromptBindings(const Profile& p) {
  std::vector<Binding> singles;
  for (const Binding& b : p.bindings) {
    if (b.count == 1) singles.push_back(b);
  }
  std::lock_guard lock(sPromptMutex);
  sPromptBindings = std::move(singles);
}

// The first of `actions` with a single input from `family`: one that applies in
// the current context, else one that applies anywhere outside the user layers.
static bool PromptInputFor(std::initializer_list<Action> actions, PortInput::Family family, Input& out) {
  const uint16_t contexts = GameContexts();
  std::lock_guard lock(sPromptMutex);
  for (const uint16_t want : {contexts, static_cast<uint16_t>(~PortInput::kCtxLayers)}) {
    for (const Action a : actions) {
      for (const Binding& b : sPromptBindings) {
        if (b.action == a && (b.contexts & want) != 0 && PortInput::BindingFamily(b) == family) {
          out = b.inputs[0];
          return true;
        }
      }
    }
  }
  return false;
}

bool PromptButtonInput(unsigned padButton, PortInput::Family family, Input& out) {
  // Menus and the map read the Menu rows first (present only while "Separate menu
  // buttons" applies, scoped to those contexts); Start and Z have none. L and R
  // fall back to their analog.
  const SMenuAction* menu = nullptr;
  for (const SMenuAction& m : kMenuActions) {
    if (m.padButton == padButton) menu = &m;
  }
  const Action game = PadBitAction(padButton);
  if (menu == nullptr) return PromptInputFor({game}, family, out);
  const bool l = padButton == PAD_TRIGGER_L;
  const bool r = padButton == PAD_TRIGGER_R;
  if (l || r) {
    return PromptInputFor({menu->menu, l ? Action::MenuLAnalog : Action::MenuRAnalog, game,
                           l ? Action::LAnalog : Action::RAnalog},
                          family, out);
  }
  return PromptInputFor({menu->menu, game}, family, out);
}

bool PromptAxisInput(PADAxis axis, PortInput::Family family, Input& out) {
  return PromptInputFor({PadAxisAction(axis)}, family, out);
}

PADStatus Synthesize(const PortInput::Output& out) {
  PADStatus s{};
  static constexpr unsigned kButtons[] = {PAD_BUTTON_A,    PAD_BUTTON_B,     PAD_BUTTON_X,    PAD_BUTTON_Y,
                                          PAD_TRIGGER_Z,   PAD_BUTTON_START, PAD_TRIGGER_L,   PAD_TRIGGER_R,
                                          PAD_BUTTON_UP,   PAD_BUTTON_DOWN,  PAD_BUTTON_LEFT, PAD_BUTTON_RIGHT};
  for (unsigned b : kButtons) {
    if (out.Held(PadBitAction(b))) s.button |= static_cast<u16>(b);
  }
  for (const SMenuAction& m : kMenuActions) {
    if (m.padButton != 0 && out.Held(m.menu)) s.button |= static_cast<u16>(m.padButton);
  }
  s.stickX = StickX(out.Value(Action::MainRight), out.Value(Action::MainLeft));
  s.stickY = StickY(out.Value(Action::MainDown), out.Value(Action::MainUp));
  s.substickX = StickX(out.Value(Action::CRight), out.Value(Action::CLeft));
  s.substickY = StickY(out.Value(Action::CDown), out.Value(Action::CUp));
  s.triggerLeft = Trigger(std::max(out.Value(Action::LAnalog), out.Value(Action::MenuLAnalog)),
                          (s.button & PAD_TRIGGER_L) != 0);
  s.triggerRight = Trigger(std::max(out.Value(Action::RAnalog), out.Value(Action::MenuRAnalog)),
                           (s.button & PAD_TRIGGER_R) != 0);
  // MP1 reports an idle status as PAD_ERR_NO_CONTROLLER; Aurora reported MP2's port present while
  // its keyboard map was installed, idle or not, and the game reads that.
  s.err = PAD_ERR_NONE;
  return s;
}

SPoll Poll(unsigned mouseHeld, bool focused) {
  static Profile sProfile;
  static PortInput::Runtime sRuntime;
  static bool sInit = false;

  Profile next;
  BuildProfile(next);
  ApplyMenuButtons(next);
  if (!sInit || next != sProfile) {
    UpdatePromptBindings(next);
    sProfile = std::move(next);
    sRuntime.SetProfile(&sProfile);
    sInit = true;
  }

  PortInput::RawState raw;
  FillRawState(raw, mouseHeld, focused);
  // MP1 steps its clock by the game's tick period; MP2's loop has none to read, so wall time.
  const double nowMs =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
  const uint16_t contexts = GameContexts();
  const PortInput::Output& out =
      sRuntime.Poll(raw, contexts, nowMs, QuietContexts(sRuntime.ActiveContexts(), contexts));
  SPoll r;
  if (PortInputRemap::Capturing()) {
    // The F1 Controls page is recording an input: it reaches no direct action (a bound
    // PortMenu would close the page mid-capture). The pad still reads it, as before the engine.
    r.status = Synthesize(out);
  } else {
    RouteDirectActions(out);
    r.status = Synthesize(out);
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
    if ((raw.padButtons >> native) & 1u) r.status.extButton |= button;
  }
  // MP1 also adjusts A and Escape while the scan text is up: that context needs the scan
  // soft-pause (above) and Escape is no pointer-menu key here.
  return r;
}

PADStatus Poll() {
  const bool focused = SDL_GetKeyboardFocus() != nullptr || SDL_GetMouseFocus() != nullptr;
  return Poll(SdlMouseHeld(), focused).status;
}

void PollPortActions() {
  static Profile sFull;
  static Profile sProfile;
  static PortInput::Runtime sRuntime;
  static bool sInit = false;
  static bool sWatch = false;
  if (!sWatch) sWatch = SDL_AddEventWatch(KeyDownWatch, nullptr);

  Profile next;
  BuildProfile(next);
  if (!sInit || next != sFull) {
    sFull = std::move(next);
    sProfile = sFull;
    std::erase_if(sProfile.bindings, [](const Binding& b) { return !IsPortAction(b.action); });
    sRuntime.SetProfile(&sProfile);
    sInit = true;
  }

  PortInput::RawState raw;
  ReadRaw(raw);
  for (size_t w = 0; w < std::size(sKeyDowns); ++w) {
    uint64_t bits = sKeyDowns[w].exchange(0, std::memory_order_relaxed);
    while (bits != 0) {
      const int bit = std::countr_zero(bits);
      bits &= bits - 1;
      raw.keys.set(w * 64 + static_cast<size_t>(bit));
    }
  }
  // F12 already has Aurora's window screenshot key, so this path takes it only when the user
  // bound it to something else and leaves the default to that handler rather than firing twice.
  // F1 is the Port menu's default binding and has no other handler.
  raw.keys.reset(SDL_SCANCODE_F12);
  const uint16_t contexts = GameContexts();
  const PortInput::Output& out = sRuntime.Poll(raw, contexts, static_cast<double>(SDL_GetTicksNS()) / 1e6,
                                               QuietContexts(sRuntime.ActiveContexts(), contexts));
  // The F1 Controls page is recording an input: a bound PortMenu would close it.
  if (PortInputRemap::Capturing()) return;
  // MP1 also routes gyro aim, reset aim, save/load state and the Original toggle here.
  if (out.Pressed(Action::PortMenu)) PortOverlay::Toggle();
  if (out.Pressed(Action::Screenshot)) aurora::request_screenshot();
  if (out.Pressed(Action::PhotoMode)) {
    PortFreeCam::SetActive(!PortFreeCam::Active(), nullptr);
  }
}

namespace {

// MP1's gyro reader (platform/debug_ui/input.cpp ReadGyroRates). The phone's sensors are opened
// once, on the first poll that wants them.
PortGyro::Motion sGyroMotion;
PortGyro::Filter sGyroFilter;
const void* sGyroMotionSource = nullptr;
bool sGyroHasAccel = false;
SDL_Sensor* sPhoneGyro = nullptr;
SDL_Sensor* sPhoneAccel = nullptr;
bool sPhoneGyroSearched = false;

int OrientationQuarters(SDL_DisplayOrientation orientation) {
  switch (orientation) {
  case SDL_ORIENTATION_LANDSCAPE:
    return 1;
  case SDL_ORIENTATION_PORTRAIT_FLIPPED:
    return 2;
  case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
    return 3;
  default:
    return 0;
  }
}

// Pitch (x, positive tilts the far edge up) and yaw (positive turns right) rates in rad/s from
// the chosen source, after calibration and gyro space; false when there is none. dt is the time
// since the last call.
bool ReadGyroRates(float& pitch, float& yaw, float dt) {
  const int gyroSource = PortOverlay::GyroSource();
  const bool wantController = gyroSource == 0 || gyroSource == 1;
  const bool wantPhone = gyroSource == 0 || gyroSource == 2;
  // SDL's sensor axes (x right, y up, z toward the player), rad/s and m/s^2.
  float gyro[3];
  float accel[3];
  bool haveAccel = false;
  const void* source = nullptr;

  if (wantController) {
    if (SDL_Gamepad* pad = PADGetSDLGamepadForIndex(0)) {
      if (SDL_GamepadHasSensor(pad, SDL_SENSOR_GYRO)) {
        // Asked per pad, so a reconnected or swapped pad gets its sensors on too.
        if (!SDL_GamepadSensorEnabled(pad, SDL_SENSOR_GYRO)) {
          SDL_SetGamepadSensorEnabled(pad, SDL_SENSOR_GYRO, true);
        }
        if (SDL_GetGamepadSensorData(pad, SDL_SENSOR_GYRO, gyro, 3)) {
          source = pad;
          if (SDL_GamepadHasSensor(pad, SDL_SENSOR_ACCEL)) {
            if (!SDL_GamepadSensorEnabled(pad, SDL_SENSOR_ACCEL)) {
              SDL_SetGamepadSensorEnabled(pad, SDL_SENSOR_ACCEL, true);
            }
            haveAccel = SDL_GetGamepadSensorData(pad, SDL_SENSOR_ACCEL, accel, 3);
          }
        }
      }
    }
  }

  if (source == nullptr && wantPhone) {
    if (sPhoneGyro == nullptr && !sPhoneGyroSearched) {
      sPhoneGyroSearched = true;
      int count = 0;
      if (SDL_SensorID* ids = SDL_GetSensors(&count)) {
        for (int i = 0; i < count; ++i) {
          const SDL_SensorType type = SDL_GetSensorTypeForID(ids[i]);
          if (type == SDL_SENSOR_GYRO && sPhoneGyro == nullptr) {
            sPhoneGyro = SDL_OpenSensor(ids[i]);
          } else if (type == SDL_SENSOR_ACCEL && sPhoneAccel == nullptr) {
            sPhoneAccel = SDL_OpenSensor(ids[i]);
          }
        }
        SDL_free(ids);
      }
    }
    if (sPhoneGyro != nullptr && SDL_GetSensorData(sPhoneGyro, gyro, 3)) {
      source = sPhoneGyro;
      haveAccel = sPhoneAccel != nullptr && SDL_GetSensorData(sPhoneAccel, accel, 3);
      // The phone reports its own axes (x right, y up in its natural orientation), so turn them
      // to the screen's, as SDL does for its accelerometer: landscape would otherwise swap pitch
      // and yaw. Then it is held like a pad.
      const SDL_DisplayID display = SDL_GetPrimaryDisplay();
      const int quarters = (OrientationQuarters(SDL_GetCurrentDisplayOrientation(display)) -
                            OrientationQuarters(SDL_GetNaturalDisplayOrientation(display)) + 4) %
                           4;
      const auto turn = [quarters](float v[3]) {
        const float x = v[0];
        const float y = v[1];
        switch (quarters) {
        case 1:
          v[0] = -y;
          v[1] = x;
          break;
        case 2:
          v[0] = -x;
          v[1] = -y;
          break;
        case 3:
          v[0] = y;
          v[1] = -x;
          break;
        default:
          break;
        }
      };
      turn(gyro);
      if (haveAccel) {
        turn(accel);
      }
    }
  }

  if (source == nullptr) {
    return false;
  }
  if (source != sGyroMotionSource || haveAccel != sGyroHasAccel) {
    sGyroMotion.Reset();
    sGyroFilter.Reset();
    sGyroMotionSource = source;
    sGyroHasAccel = haveAccel;
  }
  // 2 = only in menus: F1 open or no free aim running (paused, a frontend screen). Outside them
  // the learned drift is kept, only learning stops.
  const int autoCalibrate = PortOverlay::GyroAutoCalibrate();
  const bool inMenus = PortOverlay::Visible() || !PortMouse::Aiming();
  const bool calibrate = autoCalibrate == 1 || (autoCalibrate == 2 && inMenus);
  sGyroMotion.Process(gyro, haveAccel ? accel : nullptr, dt, PortOverlay::GyroSpace(), calibrate, pitch,
                      yaw, autoCalibrate == 2);
  sGyroFilter.Apply(pitch, yaw, dt, PortOverlay::GyroSmoothing(), PortOverlay::GyroTightening());
  return true;
}

} // namespace

void PollGyro() {
  // Called once per loop iteration, which is not once per tick when the frame limiter is off, so
  // the rates integrate over the real frame time.
  static uint64_t sLastNs = 0;
  // Gyro aim moves only the pointer, and only while pointer aim is applied (it needs mouse, twin
  // stick or touch aim, as in MP1). Off, the sensors are never touched.
  if (!PortOverlay::GyroAim() || !PortOverlay::PointerAim() || !PortMouse::PointerAimEnabled()) {
    sLastNs = 0;
    return;
  }
  const uint64_t nowNs = SDL_GetTicksNS();
  const float dt = sLastNs == 0 ? 0.f : std::min(float(double(nowNs - sLastNs) * 1e-9), 0.1f);
  sLastNs = nowNs;
  float yaw = 0.f;
  float pitch = 0.f;
  if (!ReadGyroRates(pitch, yaw, dt) || !std::isfinite(dt) || dt <= 0.f) {
    return;
  }
  PortMouse::AddGyroPointer(yaw, pitch, dt, PortOverlay::GyroCursorAngle() * (PortMouse::kPi / 180.f));
}

} // namespace PortInputDevices
