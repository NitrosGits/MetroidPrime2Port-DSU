// The game loop's per-iteration Aurora poll. It lives here rather than in
// src/MetroidPrime/main.cpp because aurora/event.h includes SDL3, which only the
// full build (mp_port_audio) has on its include path.

#include <aurora/aurora.h>
#include <aurora/event.h>
#include <dolphin/pad.h>

#include "port_mouse.h"
#include "port_overlay.h"
#include "port_warp.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

struct ButtonName {
  const char* name;
  u16 mask;
};

// Full stick deflections: S* is the main stick, C* the C-stick.
struct StickName {
  const char* name;
  s8 stickX, stickY, substickX, substickY;
};

constexpr StickName kSticks[] = {
    {"SUP", 0, 127, 0, 0},   {"SDOWN", 0, -127, 0, 0}, {"SLEFT", -127, 0, 0, 0},
    {"SRIGHT", 127, 0, 0, 0}, {"CUP", 0, 0, 0, 127},   {"CDOWN", 0, 0, 0, -127},
    {"CLEFT", 0, 0, -127, 0}, {"CRIGHT", 0, 0, 127, 0},
};

// Seconds since the first input-script poll, so screenshots can be matched to inputs.
double Seconds() {
  static const auto start = std::chrono::steady_clock::now();
  return std::chrono::duration< double >(std::chrono::steady_clock::now() - start).count();
}

bool NameIs(const char* name, const char* p, const char* at) {
  return std::strlen(name) == static_cast< size_t >(at - p) && std::strncmp(name, p, at - p) == 0;
}

constexpr ButtonName kButtons[] = {
    {"START", PAD_BUTTON_START}, {"A", PAD_BUTTON_A},         {"B", PAD_BUTTON_B},
    {"X", PAD_BUTTON_X},         {"Y", PAD_BUTTON_Y},         {"Z", PAD_TRIGGER_Z},
    {"L", PAD_TRIGGER_L},        {"R", PAD_TRIGGER_R},        {"UP", PAD_BUTTON_UP},
    {"DOWN", PAD_BUTTON_DOWN},   {"LEFT", PAD_BUTTON_LEFT},   {"RIGHT", PAD_BUTTON_RIGHT},
};

// Keyboard bindings, so the port is playable without a gamepad. Aurora's own
// defaults are empty (`g_defaultKeys` is all PAD_KEY_INVALID) and nothing else in
// the port installs a map, so pad 0 reports PAD_ERR_NO_CONTROLLER on a machine
// with no pad and the game gets no input at all. The layout is the Prime port's
// (`MetroidPrimePort/platform/port_controls.cpp`): WASD moves, IJKL is the C-stick
// (which aims and switches the visor in Echoes), the rest mirrors the pad.
//
// Keyboard state merges into the real controller's and into MP2_INPUT's virtual
// pad rather than replacing either, so this only adds input.
void InstallDefaultKeyBindings() {
  static bool installed = false;
  if (installed) {
    return;
  }
  installed = true;

  PADKeyButtonBinding buttons[PAD_BUTTON_COUNT] = {
      {SDL_SCANCODE_X, PAD_BUTTON_A},          {SDL_SCANCODE_Z, PAD_BUTTON_B},
      {SDL_SCANCODE_C, PAD_BUTTON_X},          {SDL_SCANCODE_V, PAD_BUTTON_Y},
      {SDL_SCANCODE_RETURN, PAD_BUTTON_START}, {SDL_SCANCODE_F, PAD_TRIGGER_Z},
      {SDL_SCANCODE_Q, PAD_TRIGGER_L},         {SDL_SCANCODE_E, PAD_TRIGGER_R},
      {SDL_SCANCODE_UP, PAD_BUTTON_UP},        {SDL_SCANCODE_DOWN, PAD_BUTTON_DOWN},
      {SDL_SCANCODE_LEFT, PAD_BUTTON_LEFT},    {SDL_SCANCODE_RIGHT, PAD_BUTTON_RIGHT},
  };
  // Q/E also drive the analog triggers, so L/R reach the game as held rather than
  // tapped when the button and axis bindings disagree about the frame.
  PADKeyAxisBinding axes[PAD_AXIS_COUNT] = {
      {SDL_SCANCODE_D, PAD_AXIS_LEFT_X_POS, 1},  {SDL_SCANCODE_A, PAD_AXIS_LEFT_X_NEG, 1},
      {SDL_SCANCODE_W, PAD_AXIS_LEFT_Y_POS, 1},  {SDL_SCANCODE_S, PAD_AXIS_LEFT_Y_NEG, 1},
      {SDL_SCANCODE_L, PAD_AXIS_RIGHT_X_POS, 1}, {SDL_SCANCODE_J, PAD_AXIS_RIGHT_X_NEG, 1},
      {SDL_SCANCODE_I, PAD_AXIS_RIGHT_Y_POS, 1}, {SDL_SCANCODE_K, PAD_AXIS_RIGHT_Y_NEG, 1},
      {SDL_SCANCODE_Q, PAD_AXIS_TRIGGER_L, 1},   {SDL_SCANCODE_E, PAD_AXIS_TRIGGER_R, 1},
  };

  if (PADSetKeyButtonBindings(0, buttons) && PADSetKeyAxisBindings(0, axes)) {
    PADSetKeyboardActive(0, TRUE);
  }
}

// MP2_INPUT="BUTTON@frame[+frames],...": hold BUTTON on pad 0 from that loop
// iteration for the given number of iterations (default 8), so boots can be
// driven past menus without a player. Iterations count PortPumpEvents calls.
// Stick names (kSticks) deflect fully while held; overlapping entries add up.
// Warp tokens (NEXTAREA, PREVAREA, NEXTWORLD, PREVWORLD, KILL, HEAL) queue a PortWarp request
// at their frame instead, which is how a run sweeps every area on its own.
void RunInputScript(unsigned frame) {
  static const char* script = std::getenv("MP2_INPUT");
  if (script == nullptr) {
    return;
  }
  Seconds();
  PADStatus status{};
  u16 held = 0;
  for (const char* p = script; *p != '\0';) {
    const char* at = std::strchr(p, '@');
    if (at == nullptr) {
      break;
    }
    char* end = nullptr;
    const unsigned start = static_cast< unsigned >(std::strtoul(at + 1, &end, 10));
    unsigned length = 8;
    if (*end == '+') {
      length = static_cast< unsigned >(std::strtoul(end + 1, &end, 10));
    }
    const bool down = frame >= start && frame < start + length;
    if (frame == start) {
      if (NameIs("NEXTAREA", p, at)) {
        PortWarp::CycleArea(1);
      } else if (NameIs("PREVAREA", p, at)) {
        PortWarp::CycleArea(-1);
      } else if (NameIs("ATDOOR", p, at)) {
        PortWarp::Dock();
      } else if (NameIs("DOOR", p, at)) {
        PortWarp::Door();
      } else if (NameIs("STATION", p, at)) {
        PortWarp::Station();
      } else if (NameIs("WORLDPORTAL", p, at)) {
        PortWarp::WorldPortal();
      } else if (NameIs("KILL", p, at)) {
        PortWarp::Kill();
      } else if (NameIs("HEAL", p, at)) {
        PortWarp::Heal();
      } else if (NameIs("NEXTWORLD", p, at)) {
        PortWarp::CycleWorld(1);
      } else if (NameIs("PREVWORLD", p, at)) {
        PortWarp::CycleWorld(-1);
      }
    }
    for (const ButtonName& button : kButtons) {
      if (NameIs(button.name, p, at)) {
        if (down) {
          held |= button.mask;
        }
        if (frame == start) {
          std::fprintf(stderr, "[input] %s at frame %u (%.1f s)\n", button.name, frame, Seconds());
        }
      }
    }
    for (const StickName& stick : kSticks) {
      if (NameIs(stick.name, p, at)) {
        if (down) {
          status.stickX += stick.stickX;
          status.stickY += stick.stickY;
          status.substickX += stick.substickX;
          status.substickY += stick.substickY;
        }
        if (frame == start) {
          std::fprintf(stderr, "[input] %s at frame %u (%.1f s)\n", stick.name, frame, Seconds());
        }
      }
    }
    p = *end == ',' ? end + 1 : end;
  }
  // Once the script ends, pad 0 stays connected and idle: clearing it would leave no controller,
  // and the game then gets no input messages at all (player dt stays 0, the gun never draws).
  status.err = PAD_ERR_NONE;
  status.button = held;
  PADSetVirtualStatus(0, &status);
}

} // namespace

// Runs aurora_update (window, input and GX housekeeping); true once the window asks to quit.
bool PortPumpEvents() {
  static unsigned frame = 0;
  InstallDefaultKeyBindings();
  PortWarp::ApplyEnvOnce();
  PortMouse::BeginFrame();
  RunInputScript(frame++);
  for (const AuroraEvent* event = aurora_update(); event != nullptr && event->type != AURORA_NONE;
       ++event) {
    if (event->type == AURORA_EXIT) {
      return true;
    }
    if (event->type == AURORA_SDL_EVENT && event->sdl.type == SDL_EVENT_MOUSE_MOTION) {
      // Relative motion only, which is what SDL reports while the port holds capture; the
      // game consumes the resulting aim in CPlayer/CFirstPersonCamera.
      PortMouse::AddMotion(event->sdl.motion.xrel, event->sdl.motion.yrel);
    }
    // Warp hotkeys, for testing the rest of the game by hand: F5/F6 step through the areas of
    // the current world, F7/F8 through the game's worlds. They are always live, and the game
    // ignores a request it cannot serve (no world loaded yet, bad area).
    if (event->type == AURORA_SDL_EVENT && event->sdl.type == SDL_EVENT_KEY_DOWN &&
        !event->sdl.key.repeat) {
      switch (event->sdl.key.scancode) {
      case SDL_SCANCODE_F1:
        PortOverlay::Toggle();
        break;
      case SDL_SCANCODE_F5:
        PortWarp::CycleArea(1);
        break;
      case SDL_SCANCODE_F6:
        PortWarp::CycleArea(-1);
        break;
      case SDL_SCANCODE_F9:
        PortWarp::Dock();
        break;
      case SDL_SCANCODE_F10:
        PortWarp::Station();
        break;
      case SDL_SCANCODE_F7:
        PortWarp::CycleWorld(1);
        break;
      case SDL_SCANCODE_F8:
        PortWarp::CycleWorld(-1);
        break;
      default:
        break;
      }
    }
  }
  return false;
}
