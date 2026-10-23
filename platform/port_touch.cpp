// The Android on-screen controller's native side. See platform/include/port_touch.h for what
// this is; the JNI entry points the Java view calls are platform/port_touch_jni.cpp.
//
// Two threads meet here and that is the whole design problem. The Java view runs on the
// Android UI thread and calls in whenever a finger moves; the game runs on Aurora's thread and
// polls the same state once per iteration through `BeginFrame`. So: the pad is SDL's virtual
// joystick, which is mutexed inside SDL and is the one thing both threads may touch directly;
// the aim goes straight into `PortMouse`, which is safe for it; the settings are under one
// mutex; and everything that reaches the game (wheel picks, beam shift, Turbo) is an atomic the
// UI thread raises and `BeginFrame` applies, so the player state is only ever touched on the
// game thread.

#include "port_touch.h"

#include "port_harness.h"
#include "port_mouse.h"
#include "port_overlay.h"
#include "port_prompts.h"
#include "touch_pad.h"

#include "MetroidPrime/Player/CGameState.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"

#include <SDL3/SDL.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>

namespace {

// MP1's defaults (platform/include/port_input_settings.h there). The default layout is MP1's
// Default one - the left stick, the face buttons, drag-to-aim and the visor/beam wheels - which
// MP2 now has the wheels for; Classic and Twin stick are one Layout tap away in the editor.
// An older touch.cfg (written when MP2 defaulted to Classic) keeps its `classic=1`.
constexpr float kAimSpeed = 2.25f;
constexpr int kTurboPeriod = 2; // loop iterations per Turbo half-cycle (down 2, up 2)

struct Settings {
  bool classic = false;
  bool twin = false;
  // On only where the overlay exists: an enabled touch source makes PortMouse track the camera
  // every tick, which a desktop run (no overlay, no fingers) has no use for. MP2_TOUCH_AIM=1
  // still turns it on there.
#if defined(__ANDROID__)
  bool aim = true;
#else
  bool aim = false;
#endif
  bool wheels = true;
  bool turbo = false;
  bool colors = false;
  bool labels = true;
  bool floating = true;
  bool visorTapScan = false;
  bool controls = true;
  float sideMargin = 16.f;
  float stickInset = 32.f;
  float buttonInset = 0.f;
  float opacity = 100.f;
  float aimSpeed = kAimSpeed;
  // MP1's "Separate vertical speed##touch": up and down drags use aimSpeedY.
  bool aimSplit = false;
  float aimSpeedY = kAimSpeed;
  bool springSwipe = true;
  std::string layout;
};

std::mutex sMutex;
Settings sSettings;
std::string sConfigPath;
bool sLoaded = false; // guarded by sMutex
bool sDirty = false;
bool sPadAttachFailed = false; // guarded by sPadMutex: the failure is logged once, not per event

std::atomic< bool > sEditRequested{false};
std::atomic< bool > sPhysicalInput{false};
std::atomic< bool > sRealPad{false};
std::atomic< bool > sDeviceXbox{false};

// What the UI thread asks of the game; BeginFrame applies it on the game thread.
std::atomic< int > sWheelState{0};
std::atomic< int > sPendingVisor{-1};
std::atomic< int > sPendingBeam{-1};
std::atomic< bool > sBeamShift{false};
std::atomic< bool > sTurboHeld{false};
// The D-pad buttons the beam shift swallowed on their press, so their release is swallowed too
// (and a press that reached the pad before the shift still releases there). UI thread only.
bool sDpadShifted[4] = {};
// Turbo's own A presses (game thread only).
bool sTurboDown = false;
unsigned sTurboTicks = 0;

std::mutex sPadMutex;
PortTouchPad::Pad sPad;

constexpr int kButtonA = 0; // SDL_GAMEPAD_BUTTON_SOUTH
constexpr int kDpadUp = 11; // SDL_GAMEPAD_BUTTON_DPAD_UP; then down, left, right
constexpr int kInGame = 1;  // CMFGame::kFS_InGame
constexpr int kPaused = 2;  // CMFGame::kFS_Paused

bool EnvFlag(const char* name) {
  const char* value = std::getenv(name);
  return value != nullptr && *value != '\0' && std::strcmp(value, "0") != 0;
}

// `MP2_DEBUG_PAD=1`: one line per button press and per stick move, so a device log says
// whether a touch ever became a pad write. Off by default; it is a line per event.
bool DebugPad() {
  static const bool on = EnvFlag("MP2_DEBUG_PAD");
  return on;
}

// Callers hold sMutex. An `MP2_TOUCH_*` that is set wins over the file: "0" turns a switch off,
// anything else on; the numbers are clamped as MP1's F1 sliders clamp them.
void ApplyEnvironmentLocked() {
  struct Flag {
    const char* name;
    bool Settings::* field;
  };
  static const Flag kFlags[] = {
      {"MP2_TOUCH_CLASSIC", &Settings::classic},
      {"MP2_TOUCH_TWIN", &Settings::twin},
      {"MP2_TOUCH_AIM", &Settings::aim},
      {"MP2_TOUCH_WHEELS", &Settings::wheels},
      {"MP2_TOUCH_TURBO", &Settings::turbo},
      {"MP2_TOUCH_COLORS", &Settings::colors},
      {"MP2_TOUCH_LABELS", &Settings::labels},
      {"MP2_TOUCH_FLOATING", &Settings::floating},
      {"MP2_TOUCH_VISOR_TAP_SCAN", &Settings::visorTapScan},
      {"MP2_TOUCH_CONTROLS", &Settings::controls},
      {"MP2_TOUCH_SPRING_SWIPE", &Settings::springSwipe},
      {"MP2_TOUCH_AIM_SPLIT", &Settings::aimSplit},
  };
  for (const Flag& flag : kFlags) {
    if (const char* value = std::getenv(flag.name)) {
      sSettings.*flag.field = *value != '\0' && std::strcmp(value, "0") != 0;
    }
  }
  if (sSettings.classic && sSettings.twin) {
    sSettings.twin = false;
  }
  struct Number {
    const char* name;
    float Settings::* field;
    float low;
    float high;
  };
  static const Number kNumbers[] = {
      {"MP2_TOUCH_SIDE_MARGIN", &Settings::sideMargin, 0.f, 96.f},
      {"MP2_TOUCH_STICK_INSET", &Settings::stickInset, 0.f, 160.f},
      {"MP2_TOUCH_BUTTON_INSET", &Settings::buttonInset, 0.f, 160.f},
      {"MP2_TOUCH_OPACITY", &Settings::opacity, 10.f, 100.f},
      {"MP2_TOUCH_AIM_SPEED_Y", &Settings::aimSpeedY, 0.25f, 10.f},
  };
  for (const Number& number : kNumbers) {
    if (const char* text = std::getenv(number.name)) {
      const float value = static_cast< float >(std::atof(text));
      if (std::isfinite(value)) {
        sSettings.*number.field =
            value < number.low ? number.low : (value > number.high ? number.high : value);
      }
    }
  }
  if (const char* speed = std::getenv("MP2_TOUCH_AIM_SPEED")) {
    const float value = static_cast< float >(std::atof(speed));
    if (std::isfinite(value) && value > 0.f) {
      sSettings.aimSpeed = value < 0.25f ? 0.25f : (value > 10.f ? 10.f : value);
    }
  }
}

void ReadEnvironment() {
  static std::once_flag once;
  std::call_once(once, [] {
    std::lock_guard< std::mutex > lock(sMutex);
    ApplyEnvironmentLocked();
  });
}

bool ParseBool(const std::string& value) { return value != "0" && value != "false"; }

// The settings file lives in the user directory next to the memory card. Lines are
// `key=value`; the layout string is the only value that may contain anything but digits.
//
// `sLoaded` latches only once a real path has been read: the Java view polls from its first
// draw, which can run before `main` calls `Initialize`, and latching then skipped the file.
void LoadConfig() {
  std::lock_guard< std::mutex > lock(sMutex);
  if (sLoaded || sConfigPath.empty()) {
    return;
  }
  sLoaded = true;
  std::ifstream file(sConfigPath);
  std::string line;
  while (std::getline(file, line)) {
    const size_t first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos || line[first] == '#') {
      continue;
    }
    const size_t last = line.find_last_not_of(" \t\r\n");
    const std::string entry = line.substr(first, last - first + 1);
    const size_t equals = entry.find('=');
    if (equals == std::string::npos) {
      continue;
    }
    const std::string key = entry.substr(0, equals);
    const std::string value = entry.substr(equals + 1);
    Settings& s = sSettings;
    if (key == "layout") {
      s.layout = value;
    } else if (key == "classic") {
      // Exclusive with twin; classic winning is MP1's rule on load.
      s.classic = ParseBool(value);
      if (s.classic) {
        s.twin = false;
      }
    } else if (key == "twin") {
      s.twin = ParseBool(value) && !s.classic;
    } else if (key == "aim") {
      s.aim = ParseBool(value);
    } else if (key == "wheels") {
      s.wheels = ParseBool(value);
    } else if (key == "turbo") {
      s.turbo = ParseBool(value);
    } else if (key == "colors") {
      s.colors = ParseBool(value);
    } else if (key == "labels") {
      s.labels = ParseBool(value);
    } else if (key == "floating") {
      s.floating = ParseBool(value);
    } else if (key == "visor_tap_scan") {
      s.visorTapScan = ParseBool(value);
    } else if (key == "controls") {
      s.controls = ParseBool(value);
    } else if (key == "side_margin") {
      s.sideMargin = static_cast< float >(std::atof(value.c_str()));
    } else if (key == "stick_inset") {
      s.stickInset = static_cast< float >(std::atof(value.c_str()));
    } else if (key == "button_inset") {
      s.buttonInset = static_cast< float >(std::atof(value.c_str()));
    } else if (key == "aim_speed") {
      s.aimSpeed = static_cast< float >(std::atof(value.c_str()));
    } else if (key == "aim_split") {
      s.aimSplit = ParseBool(value);
    } else if (key == "aim_speed_y") {
      s.aimSpeedY = static_cast< float >(std::atof(value.c_str()));
    } else if (key == "spring_swipe") {
      s.springSwipe = ParseBool(value);
    } else if (key == "opacity") {
      s.opacity = static_cast< float >(std::atof(value.c_str()));
    }
  }
  ApplyEnvironmentLocked();
  std::fprintf(stderr,
               "[touch] config %s: layout=\"%s\" classic=%d twin=%d aim=%d wheels=%d turbo=%d\n",
               sConfigPath.c_str(), sSettings.layout.c_str(), sSettings.classic ? 1 : 0,
               sSettings.twin ? 1 : 0, sSettings.aim ? 1 : 0, sSettings.wheels ? 1 : 0,
               sSettings.turbo ? 1 : 0);
}

// Callers hold sMutex. Writes `<path>.tmp` and renames it over the real file, so a write that
// is killed part way never leaves a truncated touch.cfg for the next launch.
void SaveConfigLocked() {
  if (sConfigPath.empty()) {
    return;
  }
  const std::string temporary = sConfigPath + ".tmp";
  {
    std::ofstream file(temporary, std::ios::trunc);
    if (!file) {
      std::fprintf(stderr, "[touch] cannot write %s\n", temporary.c_str());
      return;
    }
    const Settings& s = sSettings;
    file << "classic=" << (s.classic ? 1 : 0) << '\n';
    file << "twin=" << (s.twin ? 1 : 0) << '\n';
    file << "aim=" << (s.aim ? 1 : 0) << '\n';
    file << "wheels=" << (s.wheels ? 1 : 0) << '\n';
    file << "turbo=" << (s.turbo ? 1 : 0) << '\n';
    file << "colors=" << (s.colors ? 1 : 0) << '\n';
    file << "labels=" << (s.labels ? 1 : 0) << '\n';
    file << "floating=" << (s.floating ? 1 : 0) << '\n';
    file << "visor_tap_scan=" << (s.visorTapScan ? 1 : 0) << '\n';
    file << "controls=" << (s.controls ? 1 : 0) << '\n';
    file << "side_margin=" << s.sideMargin << '\n';
    file << "stick_inset=" << s.stickInset << '\n';
    file << "button_inset=" << s.buttonInset << '\n';
    file << "opacity=" << s.opacity << '\n';
    file << "aim_speed=" << s.aimSpeed << '\n';
    file << "aim_split=" << (s.aimSplit ? 1 : 0) << '\n';
    file << "aim_speed_y=" << s.aimSpeedY << '\n';
    file << "spring_swipe=" << (s.springSwipe ? 1 : 0) << '\n';
    file << "layout=" << s.layout << '\n';
    file.flush();
    if (!file) {
      std::fprintf(stderr, "[touch] cannot write %s\n", temporary.c_str());
      std::remove(temporary.c_str());
      return;
    }
  }
  if (std::rename(temporary.c_str(), sConfigPath.c_str()) != 0) {
    std::fprintf(stderr, "[touch] cannot replace %s\n", sConfigPath.c_str());
    std::remove(temporary.c_str());
  }
  sDirty = false;
}

template < typename T >
T Read(T Settings::* field) {
  ReadEnvironment();
  LoadConfig();
  std::lock_guard< std::mutex > lock(sMutex);
  return sSettings.*field;
}

PortTouchPad::Pad& Pad() {
  std::lock_guard< std::mutex > lock(sPadMutex);
  if (!sPad.ok()) {
    sPad = PortTouchPad::Attach();
    if (!sPad.ok() && sPadAttachFailed) {
      // Failed before and was logged: this runs once per touch event while SDL's joystick
      // subsystem is not up, and a line per event would flood logcat.
      return sPad;
    }
    // One line per attach (success, or the first failure) so a device log says whether the virtual
    // pad ever existed; without it a broken overlay and a broken mapping look the same from
    // outside. stderr is the right sink rather than a direct logcat call: on Android the entry
    // repoints fd 2 at logcat (platform/main.cpp), so this reaches the same place and still
    // compiles for the host, which has no android/log.h.
    if (sPad.ok()) {
      std::fprintf(stderr, "[touch] pad attached id=%d gamepad=%d\n", static_cast< int >(sPad.id),
                   SDL_IsGamepad(sPad.id) ? 1 : 0);
    } else {
      sPadAttachFailed = true;
      std::fprintf(stderr, "[touch] pad attach failed: %s (retried every frame)\n", SDL_GetError());
    }
  }
  return sPad;
}

void PadButton(int button, bool down) {
  PortTouchPad::Pad& pad = Pad();
  if (pad.handle == nullptr) {
    return;
  }
  if (DebugPad()) {
    std::fprintf(stderr, "[touch] button %d %s\n", button, down ? "down" : "up");
  }
  SDL_SetJoystickVirtualButton(pad.handle, button, down);
}

// The player the wheels act on: player 0's state while the game is in play (MP1's wheels are
// disabled outside play too). Game thread only.
CPlayerState* PlayerInPlay() {
  if (gpGameState == nullptr || PortHarness::FlowState() != kInGame) {
    return nullptr;
  }
  return gpGameState->PlayerState(0).GetPtr();
}

// The wheels' item indices are CPlayerState's enum values (visors Combat/Echo/Scan/Dark, beams
// Power/Dark/Light/Annihilator) and the power-up item of a beam is its beam id.
CPlayerState::EBeamId BeamOf(int item) { return static_cast< CPlayerState::EBeamId >(item); }
CPlayerState::EItemType BeamItemOf(int item) {
  return static_cast< CPlayerState::EItemType >(item);
}

// A requested visor or beam, on the game thread. The console's path (`visor`/`beam` in
// platform/port_console.cpp), except that the scan visor's gun holster is not done: that needs
// the CStateManager, which nothing outside src/ holds, and CPlayerGun skips drawing in the scan
// visor regardless.
void ApplyRequests(CPlayerState* state) {
  const int visor = sPendingVisor.exchange(-1, std::memory_order_acq_rel);
  const int beam = sPendingBeam.exchange(-1, std::memory_order_acq_rel);
  if (state == nullptr) {
    return;
  }
  if (visor >= 0 && visor < 4) {
    const CPlayerState::EPlayerVisor wanted = static_cast< CPlayerState::EPlayerVisor >(visor);
    if (state->HasVisor(wanted) && state->GetCurrentVisor() != wanted) {
      state->StartTransitionToVisor(wanted);
    }
  }
  if (beam >= 0 && beam < 4 && state->HasPowerUp(BeamItemOf(beam)) &&
      state->GetCurrentBeam() != BeamOf(beam)) {
    // CPlayerGun::HandleWeaponChange reads the beam from the state.
    state->SetCurrentBeam(BeamOf(beam));
  }
}

int BuildWheelState(CPlayerState* state) {
  if (state == nullptr) {
    return 0;
  }
  int bits = 1 << 12;
  for (int item = 0; item < 4; ++item) {
    if (state->HasVisor(static_cast< CPlayerState::EPlayerVisor >(item))) {
      bits |= 1 << item;
    }
    if (state->HasPowerUp(BeamItemOf(item))) {
      bits |= 1 << (4 + item);
    }
  }
  const int visor = static_cast< int >(state->GetCurrentVisor());
  const int beam = static_cast< int >(state->GetCurrentBeam());
  bits |= (visor >= 0 && visor < 4 ? visor : 0) << 8;
  bits |= (beam >= 0 && beam < 4 ? beam : 0) << 10;
  // Bit 13 (morphed) stays clear: the morph state is CPlayer's, out of reach without the
  // CStateManager, so the overlay keeps the on-foot labels in the ball.
  return bits;
}

// Turbo: while the button is held (and the setting is on), A goes down for two iterations and
// up for two, as MP1's turbo fire mashes it.
void UpdateTurbo(bool enabled) {
  const bool held =
      enabled && sTurboHeld.load(std::memory_order_acquire) && PortHarness::FlowState() == kInGame;
  if (!held) {
    if (sTurboDown) {
      PadButton(kButtonA, false);
      sTurboDown = false;
    }
    sTurboTicks = 0;
    return;
  }
  const bool down = (sTurboTicks++ / kTurboPeriod) % 2 == 0;
  if (down != sTurboDown) {
    PadButton(kButtonA, down);
    sTurboDown = down;
  }
}

} // namespace

namespace PortTouch {

void Initialize(const std::string& userDirectory) {
  ReadEnvironment();
  if (!userDirectory.empty()) {
    // The Java view's first draw can call Layout() on the UI thread at any moment.
    std::lock_guard< std::mutex > lock(sMutex);
    sConfigPath = userDirectory + "/touch.cfg";
  }
  LoadConfig();
}

void BeginFrame() {
  ReadEnvironment();
  LoadConfig();

  // Retry an attach that failed because SDL's joystick subsystem was not up yet, here on the
  // game thread rather than per touch event on the UI thread.
  {
    bool missing;
    {
      std::lock_guard< std::mutex > lock(sPadMutex);
      missing = !sPad.ok();
    }
    if (missing) {
      Pad();
    }
  }

  // A pad that is not this overlay's own is a real one; while one is connected the overlay
  // steps aside (MP1 does the same), which is what `TakePhysicalInput` reports.
  {
    int count = 0;
    bool real = false;
    if (SDL_JoystickID* ids = SDL_GetJoysticks(&count)) {
      for (int i = 0; i < count; ++i) {
        if (!SDL_IsJoystickVirtual(ids[i])) {
          real = true;
          break;
        }
      }
      SDL_free(ids);
    }
    sRealPad.store(real, std::memory_order_release);
  }

  // The aim source: the setting asks for it, and the F1 overlay takes it away while it is up.
  const bool aim = AimEnabled() && !OverlayVisible();
  PortMouse::SetTouchEnabled(aim);
  if (!aim) {
    // The finger's release may never arrive (the overlay opened under it).
    PortMouse::SetTouchAimDown(false);
  }
  PortMouse::SetTouchAimSpeed(AimSpeed());
  PortMouse::SetTouchAimSpeedY(AimSplit() ? AimSpeedY() : AimSpeed());

  CPlayerState* const player = PlayerInPlay();
  ApplyRequests(player);
  sWheelState.store(BuildWheelState(player), std::memory_order_release);
  UpdateTurbo(Turbo());

  // SetLayout and CycleLayout save for themselves; this flush retries a failed write.
  std::lock_guard< std::mutex > lock(sMutex);
  if (sDirty) {
    SaveConfigLocked();
  }
}

void SetButton(int button, bool down) {
  // The twin layout's Beam button turns the D-pad into the beam picker (MP1's Remastered beam
  // shift): up Power, right Light, down Annihilator, left Dark.
  if (button >= kDpadUp && button < kDpadUp + 4) {
    const int index = button - kDpadUp; // up, down, left, right
    if (down && sBeamShift.load(std::memory_order_acquire)) {
      static const int kBeams[4] = {0, 3, 1, 2};
      RequestBeam(kBeams[index]);
      sDpadShifted[index] = true;
      return;
    }
    if (!down && sDpadShifted[index]) {
      sDpadShifted[index] = false;
      return;
    }
  }
  PadButton(button, down);
}

void SetAxis(int axis, float value) {
  PortTouchPad::Pad& pad = Pad();
  if (pad.handle == nullptr) {
    return;
  }
  // Reported once per move, not once per pixel, so the log stays readable.
  static float lastLogged[SDL_GAMEPAD_AXIS_COUNT] = {};
  if (DebugPad() && axis >= 0 && axis < SDL_GAMEPAD_AXIS_COUNT &&
      std::fabs(value - lastLogged[axis]) > 0.02f) {
    lastLogged[axis] = value;
    std::fprintf(stderr, "[touch] axis %d %.2f\n", axis, value);
  }
  // The overlay sends a normalised -1..1 deflection; the virtual joystick takes a Sint16.
  SDL_SetJoystickVirtualAxis(pad.handle, axis, PortTouchPad::AxisValue(value));
}

void AddAim(float dxDp, float dyDp) {
  if (DebugPad()) {
    std::fprintf(stderr, "[touch] aim %.1f %.1f dp\n", dxDp, dyDp);
  }
  PortMouse::AddTouchAim(dxDp, dyDp);
}

void SetAimDown(bool down) { PortMouse::SetTouchAimDown(down); }

// MP1's TouchSpringSwipe: the swipe both finishes the aim flick and asks CMorphBall for a Spring
// Ball, which it does only on the ground in the ball with Spring Ball on, so elsewhere it expires.
std::atomic< uint64_t > sSpringRequestUntilNs{0};
constexpr uint64_t kSpringRequestNs = 120'000'000;

void SpringSwipe() {
  if (!SpringSwipeEnabled()) return;
  PortMouse::SpringSwipe();
  sSpringRequestUntilNs.store(SDL_GetTicksNS() + kSpringRequestNs);
}

bool SpringBallRequested() { return SDL_GetTicksNS() < sSpringRequestUntilNs.load(); }

bool AimEnabled() { return Read(&Settings::aim); }

float AimSpeed() { return Read(&Settings::aimSpeed); }
bool AimSplit() { return Read(&Settings::aimSplit); }
float AimSpeedY() { return Read(&Settings::aimSpeedY); }

bool Classic() { return Read(&Settings::classic); }
bool TwinStick() { return Read(&Settings::twin); }
bool Labels() { return Read(&Settings::labels); }
bool Colors() { return Read(&Settings::colors); }
bool FloatingStick() { return Read(&Settings::floating); }
bool Turbo() { return Read(&Settings::turbo); }
bool WheelsEnabled() { return Read(&Settings::wheels); }
bool VisorTapScan() { return Read(&Settings::visorTapScan); }
bool ControlsEnabled() { return Read(&Settings::controls); }
float SideMarginDp() { return Read(&Settings::sideMargin); }
float StickInsetDp() { return Read(&Settings::stickInset); }
float ButtonInsetDp() { return Read(&Settings::buttonInset); }
float Opacity() { return Read(&Settings::opacity); }
bool SpringSwipeEnabled() { return Read(&Settings::springSwipe); }

std::string Layout() { return Read(&Settings::layout); }

void SetLayout(const std::string& layout) {
  LoadConfig();
  std::lock_guard< std::mutex > lock(sMutex);
  if (sSettings.layout == layout) {
    return;
  }
  sSettings.layout = layout;
  sDirty = true;
  // Under the same lock the mutation took: never a half-updated Settings in the file.
  SaveConfigLocked();
}

namespace {
void SetSetting(bool Settings::* field, bool value) {
  LoadConfig();
  std::lock_guard< std::mutex > lock(sMutex);
  if (sSettings.*field == value) {
    return;
  }
  sSettings.*field = value;
  sDirty = true;
  SaveConfigLocked();
}
} // namespace

void SetControlsEnabled(bool enabled) { SetSetting(&Settings::controls, enabled); }

void SetAimEnabled(bool enabled) { SetSetting(&Settings::aim, enabled); }
void SetColors(bool enabled) { SetSetting(&Settings::colors, enabled); }
void SetLabels(bool enabled) { SetSetting(&Settings::labels, enabled); }
void SetTurbo(bool enabled) { SetSetting(&Settings::turbo, enabled); }
void SetFloatingStick(bool enabled) { SetSetting(&Settings::floating, enabled); }
void SetWheelsEnabled(bool enabled) { SetSetting(&Settings::wheels, enabled); }
void SetVisorTapScan(bool enabled) { SetSetting(&Settings::visorTapScan, enabled); }
void SetSpringSwipe(bool enabled) { SetSetting(&Settings::springSwipe, enabled); }
void SetAimSplit(bool enabled) { SetSetting(&Settings::aimSplit, enabled); }

namespace {
void SetNumber(float Settings::* field, float value, float low, float high) {
  LoadConfig();
  std::lock_guard< std::mutex > lock(sMutex);
  value = value < low ? low : (value > high ? high : value);
  if (sSettings.*field == value) {
    return;
  }
  sSettings.*field = value;
  sDirty = true;
  SaveConfigLocked();
}
} // namespace

void SetSideMarginDp(float dp) { SetNumber(&Settings::sideMargin, dp, 0.f, 300.f); }
void SetStickInsetDp(float dp) { SetNumber(&Settings::stickInset, dp, 0.f, 300.f); }
void SetButtonInsetDp(float dp) { SetNumber(&Settings::buttonInset, dp, 0.f, 300.f); }
void SetOpacity(float percent) { SetNumber(&Settings::opacity, percent, 10.f, 100.f); }
void SetAimSpeed(float pxPerDp) { SetNumber(&Settings::aimSpeed, pxPerDp, 0.5f, 6.f); }
void SetAimSpeedY(float pxPerDp) { SetNumber(&Settings::aimSpeedY, pxPerDp, 0.5f, 6.f); }

void RequestEdit() { sEditRequested.store(true, std::memory_order_release); }

bool TakeEditRequested() { return sEditRequested.exchange(false, std::memory_order_acq_rel); }

void CycleLayout() {
  LoadConfig();
  std::lock_guard< std::mutex > lock(sMutex);
  Settings& s = sSettings;
  if (s.classic) {
    s.classic = false;
    s.twin = true;
  } else if (s.twin) {
    s.twin = false;
  } else {
    s.classic = true;
  }
  std::fprintf(stderr, "[touch] layout -> %s\n",
               s.classic ? "classic" : (s.twin ? "twin stick" : "default"));
  sDirty = true;
  SaveConfigLocked();
}

int WheelState() { return sWheelState.load(std::memory_order_acquire); }

void RequestVisor(int visor) { sPendingVisor.store(visor, std::memory_order_release); }

void RequestBeam(int beam) { sPendingBeam.store(beam, std::memory_order_release); }

void SetBeamShift(bool held) { sBeamShift.store(held, std::memory_order_release); }

void SetTurboFire(bool held) { sTurboHeld.store(held, std::memory_order_release); }

bool PauseScreenOpen() { return PortHarness::FlowState() == kPaused; }

bool OverlayVisible() { return PortOverlay::Visible(); }

void NoteDevice(bool xboxLayout) {
  // The prompt icons follow the on-screen layout while touch is the active input.
  sDeviceXbox.store(xboxLayout, std::memory_order_release);
  PortPrompts::NoteTouchInput(xboxLayout);
}

bool DeviceIsXbox() { return sDeviceXbox.load(std::memory_order_acquire); }

void NotePhysicalInput() { sPhysicalInput.store(true, std::memory_order_release); }

bool TakePhysicalInput() {
  const bool event = sPhysicalInput.exchange(false, std::memory_order_acq_rel);
  return event || sRealPad.load(std::memory_order_acquire);
}

} // namespace PortTouch
