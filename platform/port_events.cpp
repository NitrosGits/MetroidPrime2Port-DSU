// The game loop's per-iteration Aurora poll. It lives here rather than in
// src/MetroidPrime/main.cpp because aurora/event.h includes SDL3, which only the
// full build (mp_port_audio) has on its include path.

#include <aurora/aurora.h>
#include <aurora/event.h>
#include <dolphin/pad.h>

#include "port_harness.h"
#include "port_input.h"
#include "port_mouse.h"
#include "port_overlay.h"
#include "port_touch.h"
#include "port_warp.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

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

// The button names are shared with the console's `press` (port_input.h).
using PortInput::kButtonNames;

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

// One token of the MP2_INPUT script, "NAME@frame[+frames]". Parsed once, because a token fires once:
// `fired` is what makes a token whose frame has already gone by fire at the first frame the script
// sees instead of being lost, and what keeps a frame the counter skips from dropping a token.
struct InputToken {
  const char* name; // the token's text, e.g. "NEXTAREA"
  const char* at;   // the '@' that ends it
  unsigned start;   // the game update it fires on
  unsigned length;  // how many game updates a button or stick stays down
  bool fired;
};

// Far more than a sweep's own script (63 areas x 5 tokens); the rest of a longer one is ignored
// loudly rather than silently.
constexpr size_t kMaxTokens = 512;

InputToken* InputScript(size_t& count) {
  static InputToken tokens[kMaxTokens];
  static size_t sCount = 0;
  static bool sParsed = false;
  if (!sParsed) {
    sParsed = true;
    for (const char* p = std::getenv("MP2_INPUT"); p != nullptr && *p != '\0';) {
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
      if (sCount == kMaxTokens) {
        std::fprintf(stderr, "[input] MP2_INPUT holds more than %zu tokens; the rest are ignored\n",
                     kMaxTokens);
        break;
      }
      InputToken& token = tokens[sCount++];
      token.name = p;
      token.at = at;
      token.start = start;
      token.length = length;
      token.fired = false;
      p = *end == ',' ? end + 1 : end;
    }
  }
  count = sCount;
  return tokens;
}

// The frame the script is on, 1-based and counted in *game updates* (CStateManager::Update, the
// counter MP2_FRAMES uses): frame 1 is the game's first update, so a token at frame 0 or 1 is the
// pad state the game starts with - the state a token sets before the game has updated at all is the
// state its first update reads. Counting loop iterations instead put every early token in the boot,
// which is thousands of iterations long before the game's first update (~34 s): a token framed
// against the game's updates landed in the wrong place and one aimed at the start was lost
// (docs/STATUS.md wall 3, `NEXTAREA@1400` never hopping where `NEXTAREA@3500` did).
//
// The counter also never stands still once the game has started: an iteration the game spends *not*
// updating gains one frame of its own, because a screen that pauses the game performs no game update
// at all and a script that could not advance through that pause could never press the button that
// dismisses it (tools/sweep_areas.sh's periodic `B` depends on it). Before the first update the
// frame stays at 1 - the boot consumes none of the script's timeline, which is also why a screen
// that precedes every game update (the front end) cannot be driven by frame: MP2_SKIP_FRONTEND is
// what gets a run past it.
unsigned ScriptFrame() {
  static unsigned sFrame = 1;
  static unsigned sSeen = 0;
  const unsigned updates = PortHarness::GameUpdates();
  if (updates == 0) {
    return sFrame;
  }
  if (updates != sSeen) {
    sSeen = updates;
    // Back in step with the game: the pad state this call sets is what update `updates + 1` runs on.
    sFrame = updates + 1;
  } else {
    ++sFrame;
  }
  return sFrame;
}

// Queues the PortWarp request a warp token names, and logs it where it fired: a warp token shows up
// in the log as the game serves it (`[warp] moved to area 3`), so without this line a log cannot tell
// a token the game ignored from one that never fired - which is the difference wall 3 turned on.
// Text that names no warp token (a button, a stick, a typo) falls through the chain and is ignored.
void FireWarpToken(const InputToken& token, unsigned frame) {
  if (NameIs("NEXTAREA", token.name, token.at)) {
    PortWarp::CycleArea(1);
  } else if (NameIs("PREVAREA", token.name, token.at)) {
    PortWarp::CycleArea(-1);
  } else if (NameIs("ATDOOR", token.name, token.at)) {
    PortWarp::Dock();
  } else if (NameIs("DOOR", token.name, token.at)) {
    PortWarp::Door();
  } else if (NameIs("STATION", token.name, token.at)) {
    PortWarp::Station();
  } else if (NameIs("WORLDPORTAL", token.name, token.at)) {
    PortWarp::WorldPortal();
  } else if (NameIs("KILL", token.name, token.at)) {
    PortWarp::Kill();
  } else if (NameIs("HEAL", token.name, token.at)) {
    PortWarp::Heal();
  } else if (token.at - token.name > 5 && std::strncmp(token.name, "LAYER", 5) == 0) {
    // LAYER<layer>: the number is part of the token, so a sweep can ask for a different layer in
    // each area it visits.
    PortWarp::Layer(std::atoi(token.name + 5));
  } else if (NameIs("NEXTWORLD", token.name, token.at)) {
    PortWarp::CycleWorld(1);
  } else if (NameIs("PREVWORLD", token.name, token.at)) {
    PortWarp::CycleWorld(-1);
  } else {
    return;
  }
  std::fprintf(stderr, "[input] %.*s at frame %u (%.1f s)\n",
               static_cast< int >(token.at - token.name), token.name, frame, Seconds());
}

// Pad 0 as the debug console drives it (`press`/`stick`, see port_input.h): a hold that merges into
// the same virtual pad the MP2_INPUT script writes. The game thread is the only writer - console
// commands run inside `CStateManager::PortWarpTick` - so no locking is needed.
struct SHoldPad {
  u16 buttons = 0;
  s8 stickX = 0;
  s8 stickY = 0;
  unsigned endUpdate = 0; // PortHarness::GameUpdates() at which the hold ends; ignored when forever
  bool forever = false;
  bool used = false;      // a hold was set at some point, so pad 0 must stay connected afterwards
};

SHoldPad sHold;

bool HoldActive() {
  return sHold.used && (sHold.forever || PortHarness::GameUpdates() < sHold.endUpdate);
}

// MP2_INPUT="BUTTON@frame[+frames],...": hold BUTTON on pad 0 from that game update for the given
// number of game updates (default 8), so runs can be driven past menus without a player. See
// ScriptFrame for what a frame is, and platform/README.md's "Input" for the table. Stick names
// (kSticks) deflect fully while held; overlapping entries add up. Warp tokens (NEXTAREA, PREVAREA,
// NEXTWORLD, PREVWORLD, KILL, HEAL) queue a PortWarp request at their frame instead, which is how a
// run sweeps every area on its own.
void RunInputScript(unsigned frame) {
  size_t count = 0;
  InputToken* tokens = InputScript(count);
  // No script and no console hold: pad 0 is left to the real controller, as it always was.
  if (count == 0 && !sHold.used) {
    return;
  }
  Seconds();
  PADStatus status{};
  u16 held = HoldActive() ? sHold.buttons : 0;
  if (HoldActive()) {
    status.stickX += sHold.stickX;
    status.stickY += sHold.stickY;
  }

  for (size_t i = 0; i < count; ++i) {
    InputToken& token = tokens[i];
    // Frame 1 is the first game update and 0 means "the state the game starts with", so a token at
    // 0 is served with the first update too rather than never being reached.
    const unsigned start = token.start == 0 ? 1 : token.start;
    const bool down = frame >= start && frame < start + token.length;
    const bool fire = !token.fired && frame >= start;
    token.fired = token.fired || fire;
    if (fire) {
      FireWarpToken(token, frame);
    }
    for (const PortInput::SButtonName& button : kButtonNames) {
      if (NameIs(button.name, token.name, token.at)) {
        if (down) {
          held |= button.mask;
        }
        if (fire) {
          std::fprintf(stderr, "[input] %s at frame %u (%.1f s)\n", button.name, frame, Seconds());
        }
      }
    }
    for (const StickName& stick : kSticks) {
      if (NameIs(stick.name, token.name, token.at)) {
        if (down) {
          status.stickX += stick.stickX;
          status.stickY += stick.stickY;
          status.substickX += stick.substickX;
          status.substickY += stick.substickY;
        }
        if (fire) {
          std::fprintf(stderr, "[input] %s at frame %u (%.1f s)\n", stick.name, frame, Seconds());
        }
      }
    }
  }
  // Once the script ends, pad 0 stays connected and idle: clearing it would leave no controller,
  // and the game then gets no input messages at all (player dt stays 0, the gun never draws).
  status.err = PAD_ERR_NONE;
  status.button = held;
  PADSetVirtualStatus(0, &status);
}

} // namespace

namespace PortInput {

void Hold(u16 buttons, s8 stickX, s8 stickY, unsigned updates) {
  sHold.buttons = buttons;
  sHold.stickX = stickX;
  sHold.stickY = stickY;
  sHold.forever = updates == 0;
  // The next update reads the pad state this iteration sets, so `updates` counts from the update
  // after this one.
  sHold.endUpdate = PortHarness::GameUpdates() + updates + 1;
  sHold.used = true;
  std::fprintf(stderr, "[input] console holds 0x%x stick %d,%d for %u updates\n", buttons, stickX,
               stickY, updates);
}

void Clear() {
  sHold.buttons = 0;
  sHold.stickX = 0;
  sHold.stickY = 0;
  sHold.forever = false;
  sHold.endUpdate = 0;
}

} // namespace PortInput

// Runs aurora_update (window, input and GX housekeeping); true once the window asks to quit.
bool PortPumpEvents() {
  InstallDefaultKeyBindings();
  PortWarp::ApplyEnvOnce();
  // A run that has reached its MP2_FRAMES limit leaves through the same door the window's close
  // button opens: the game loop ends, Aurora shuts down and the process exits 0. The check is here
  // rather than in the game because the loop asks this every iteration, whatever the game is doing.
  if (PortHarness::ExitRequested()) {
    std::fprintf(stderr, "[harness] the run is over (MP2_FRAMES); shutting down\n");
    return true;
  }
  // A run that has stopped updating is reported here, every iteration, because this poll is the one
  // thing a paused or wedged game still runs.
  PortHarness::IdleCheck();
  PortTouch::BeginFrame();
  PortMouse::BeginFrame();
  RunInputScript(ScriptFrame());
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
    // Anything physical - a key or a mouse button - means the on-screen controls should step
    // aside, which is what the overlay's own poll acts on. A *gamepad* is deliberately not
    // listed here: the overlay's own pad arrives as an added device too, and treating that as
    // physical hid the controls the moment they attached. Physical pads are found instead by
    // the scan in `PortTouch::BeginFrame`, which can tell a virtual device from a real one.
    if (event->type == AURORA_SDL_EVENT &&
        (event->sdl.type == SDL_EVENT_KEY_DOWN ||
         event->sdl.type == SDL_EVENT_MOUSE_BUTTON_DOWN)) {
      PortTouch::NotePhysicalInput();
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
