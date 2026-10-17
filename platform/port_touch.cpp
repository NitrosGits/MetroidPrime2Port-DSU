// The Android on-screen controller's native side. See platform/include/port_touch.h for what
// this is and why it is one module rather than MP1's three.
//
// Two threads meet here and that is the whole design problem. The Java view runs on the
// Android UI thread and calls in through JNI (a button, an axis, a drag delta, a question
// about the layout) whenever a finger moves; the game runs on Aurora's thread and polls the
// same state once per iteration through `BeginFrame`. So: the pad is SDL's virtual joystick,
// which is mutexed inside SDL and is the one thing both threads may touch directly; the aim
// goes straight into `PortMouse`, which MP1 already made safe for it (the same mutex pattern
// is in platform/port_mouse.cpp); and the settings below are the only state this file owns,
// under one mutex, with the per-frame readers taking a copy.

#include "port_touch.h"

#include "port_mouse.h"
#include "port_overlay.h"
#include "touch_pad.h"

#include <SDL3/SDL.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>

#if defined(__ANDROID__)
#include <android/log.h>
#include <jni.h>
#endif

namespace {

// MP1's defaults (platform/include/port_input_settings.h there), minus the settings MP2 has
// no UI for. `classic` is true and `aim` is false here and the other way round in MP1: MP2
// switches beams and visors with the C-stick, and MP2 has no wheel control to take that job
// over, so the C-stick has to be on the screen from the first launch. Drag-to-aim is the
// opt-in alternative (MP2_TOUCH_AIM=1, see the header) exactly because turning it on gives
// the C-stick's beam and visor switching away.
constexpr float kSideMarginDp = 16.f;
constexpr float kStickInsetDp = 32.f;
constexpr float kButtonInsetDp = 0.f;
constexpr float kOpacity = 100.f;
constexpr float kAimSpeed = 2.25f;

struct Settings {
  bool classic = true;
  bool twin = false;
  bool aim = false;
  std::string layout;
};

std::mutex sMutex;
Settings sSettings;
std::string sConfigPath;
bool sLoaded = false; // guarded by sMutex
bool sDirty = false;
bool sPadAttachFailed = false; // guarded by sPadMutex: the failure is logged once, not per event

// The aim's own speed: MP1 keeps it beside the other touch settings and reads it from the F1
// window; MP2 has no such window, so the environment is where it is set for now.
float sAimSpeed = kAimSpeed;

std::atomic< bool > sEditRequested{false};
std::atomic< bool > sPhysicalInput{false};
std::atomic< bool > sRealPad{false};

// Which layout touched last: MP1's prompt module reads this through NoteTouchInput, which
// MP2 does not build yet (see NoteDevice).
std::atomic< bool > sDeviceXbox{false};

std::mutex sPadMutex;
PortTouchPad::Pad sPad;

bool EnvFlag(const char* name) {
  const char* value = std::getenv(name);
  return value != nullptr && *value != '\0' && std::strcmp(value, "0") != 0;
}

// `MP2_DEBUG_PAD=1`: one line per button press and per stick move, so a device log says
// whether a touch ever became a pad write. A phone has no console, and without this a working
// overlay and a dead one look the same from the outside - which is how the first device run
// of M3 was read. Off by default; it is a line per event, not per frame.
bool DebugPad() {
  static const bool on = EnvFlag("MP2_DEBUG_PAD");
  return on;
}

void ReadEnvironment() {
  static bool read = false;
  if (read) {
    return;
  }
  read = true;
  if (EnvFlag("MP2_TOUCH_AIM")) {
    sSettings.aim = true;
  }
  if (const char* speed = std::getenv("MP2_TOUCH_AIM_SPEED")) {
    const float value = static_cast< float >(std::atof(speed));
    if (std::isfinite(value) && value > 0.f) {
      const float clamped = value < 0.25f ? 0.25f : (value > 10.f ? 10.f : value);
      sAimSpeed = clamped;
    }
  }
}

// The settings file lives in the user directory next to the memory card, because that is the
// one directory this app owns on every platform. Lines are `key=value`, one per line; the
// layout string is the only value that may contain anything but letters and digits.
//
// `sLoaded` latches only once a real path has been read. The Java view polls the layout from
// its first draw, which can run before `main` calls `Initialize`; latching on that first call
// made the later `Initialize` skip the file, so the saved layout and the classic/twin choice
// were silently ignored (and the next edit overwrote the file with defaults) on any launch
// where the overlay drew first.
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
    if (key == "layout") {
      sSettings.layout = value;
    } else if (key == "classic") {
      sSettings.classic = value != "0";
      // The two layouts are exclusive; the file is written by this module, but a hand-edited
      // one could say both, and the classic one winning is MP1's rule on load.
      if (sSettings.classic) {
        sSettings.twin = false;
      }
    } else if (key == "twin") {
      sSettings.twin = value != "0";
      if (sSettings.twin) {
        sSettings.classic = false;
      }
    }
  }
  std::fprintf(stderr, "[touch] config %s: layout=\"%s\" classic=%d twin=%d\n",
               sConfigPath.c_str(), sSettings.layout.c_str(), sSettings.classic ? 1 : 0,
               sSettings.twin ? 1 : 0);
}

// Callers hold sMutex. Writes `<path>.tmp` and renames it over the real file: a write that is
// killed part way must not leave a truncated touch.cfg that a later launch loads (a cut-off
// `layout=` line would read as wrong offsets). The rename is atomic on the same directory.
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
    file << "classic=" << (sSettings.classic ? 1 : 0) << '\n';
    file << "twin=" << (sSettings.twin ? 1 : 0) << '\n';
    file << "layout=" << sSettings.layout << '\n';
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

PortTouchPad::Pad& Pad() {
  std::lock_guard< std::mutex > lock(sPadMutex);
  if (!sPad.ok()) {
    sPad = PortTouchPad::Attach();
    if (!sPad.ok() && sPadAttachFailed) {
      // Failed before and was logged: this runs once per touch event while SDL's joystick
      // subsystem is not up, and a line per event would flood logcat.
      return sPad;
    }
    // One line per attach (success, or the first failure) so a device log says whether the virtual pad ever existed; without
    // it a broken overlay and a broken mapping look the same from outside. stderr is the
    // right sink rather than a direct logcat call: on Android the entry repoints fd 2 at
    // logcat (platform/main.cpp), so this reaches the same place and still compiles for the
    // host, which has no android/log.h.
    if (sPad.ok()) {
      std::fprintf(stderr, "[touch] pad attached id=%d gamepad=%d\n", static_cast< int >(sPad.id),
                   SDL_IsGamepad(sPad.id) ? 1 : 0);
    } else {
      sPadAttachFailed = true;
      std::fprintf(stderr, "[touch] pad attach failed: %s (retried every frame)\n",
                   SDL_GetError());
    }
  }
  return sPad;
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
  LoadConfig();

  // Retry an attach that failed because SDL's joystick subsystem was not up yet (the disc
  // picker, early boot), so the pad appears without needing a touch. Deliberately here on
  // the game thread: the UI thread would otherwise retry per touch event, and the pad is
  // then created on the thread that owns SDL's event loop.
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

  // The aim source: the layout asks for it, and the F1 overlay takes it away while it is up,
  // because the game is paused there and the touches belong to that window.
  const bool aim = AimEnabled() && !OverlayVisible();
  PortMouse::SetTouchEnabled(aim);
  if (!aim) {
    // The finger's release may never arrive (the overlay opened under it), and a stuck
    // "down" would keep the player turning after the overlay closes.
    PortMouse::SetTouchAimDown(false);
  }
  PortMouse::SetTouchAimSpeed(sAimSpeed);

  // SetLayout and CycleLayout save for themselves under the lock; this flush covers a failed
  // write (sDirty stays set), so the next frame tries again.
  std::lock_guard< std::mutex > lock(sMutex);
  if (sDirty) {
    SaveConfigLocked();
  }
}

void SetButton(int button, bool down) {
  PortTouchPad::Pad& pad = Pad();
  if (pad.handle == nullptr) {
    return;
  }
  if (DebugPad()) {
    std::fprintf(stderr, "[touch] button %d %s\n", button, down ? "down" : "up");
  }
  SDL_SetJoystickVirtualButton(pad.handle, button, down);
}

void SetAxis(int axis, float value) {
  PortTouchPad::Pad& pad = Pad();
  if (pad.handle == nullptr) {
    return;
  }
  // A stick arrived: reported once per move, not once per pixel, so the log stays readable.
  static float lastLogged[SDL_GAMEPAD_AXIS_COUNT] = {};
  if (DebugPad() && axis >= 0 && axis < SDL_GAMEPAD_AXIS_COUNT &&
      std::fabs(value - lastLogged[axis]) > 0.02f) {
    lastLogged[axis] = value;
    std::fprintf(stderr, "[touch] axis %d %.2f\n", axis, value);
  }
  // The overlay sends a normalised -1..1 deflection; the virtual joystick takes a Sint16.
  // Passing the float through truncated every partial deflection to 0, so the stick read as
  // centred and the player could not move - MP1's touch_pad.cpp has the conversion, and
  // `PortTouchPad::AxisValue` is it.
  SDL_SetJoystickVirtualAxis(pad.handle, axis, PortTouchPad::AxisValue(value));
}

void AddAim(float dxDp, float dyDp) {
  if (DebugPad()) {
    std::fprintf(stderr, "[touch] aim %.1f %.1f dp\n", dxDp, dyDp);
  }
  PortMouse::AddTouchAim(dxDp, dyDp);
}

void SetAimDown(bool down) { PortMouse::SetTouchAimDown(down); }

void SpringSwipe() { PortMouse::SpringSwipe(); }

bool AimEnabled() {
  ReadEnvironment();
  std::lock_guard< std::mutex > lock(sMutex);
  return sSettings.aim;
}

float AimSpeed() {
  ReadEnvironment();
  return sAimSpeed;
}

bool Classic() {
  std::lock_guard< std::mutex > lock(sMutex);
  return sSettings.classic;
}

bool TwinStick() {
  std::lock_guard< std::mutex > lock(sMutex);
  return sSettings.twin;
}

bool Labels() { return true; }
bool Colors() { return false; }
bool FloatingStick() { return true; }
float SideMarginDp() { return kSideMarginDp; }
float StickInsetDp() { return kStickInsetDp; }
float ButtonInsetDp() { return kButtonInsetDp; }
float Opacity() { return kOpacity; }

std::string Layout() {
  LoadConfig();
  std::lock_guard< std::mutex > lock(sMutex);
  return sSettings.layout;
}

void SetLayout(const std::string& layout) {
  {
    std::lock_guard< std::mutex > lock(sMutex);
    if (sSettings.layout == layout) {
      return;
    }
    sSettings.layout = layout;
    sDirty = true;
    // Under the same lock the mutation took: the game thread's flush and the UI thread's
    // readers must never see a half-updated Settings or write the file at the same time.
    SaveConfigLocked();
  }
}

void RequestEdit() { sEditRequested.store(true, std::memory_order_release); }

bool TakeEditRequested() { return sEditRequested.exchange(false, std::memory_order_acq_rel); }

void CycleLayout() {
  {
    std::lock_guard< std::mutex > lock(sMutex);
    const bool classic = sSettings.classic;
    sSettings.classic = !classic;
    sSettings.twin = classic;
    // The offsets are the Java view's per-control moves and they belong to the layout they
    // were made in; carrying them into the other one puts buttons under the wrong fingers.
    sSettings.layout.clear();
    sDirty = true;
    SaveConfigLocked();
  }
}

bool OverlayVisible() { return PortOverlay::Visible(); }

void NoteDevice(bool xboxLayout) {
  // MP1's equivalent reaches `PortPrompts::NoteTouchInput`, which re-registers the in-game
  // prompt icons for the input actually in use. MP2 has that module's source
  // (platform/port_prompts.cpp) but does not build it yet - it is one of the port files whose
  // game-side hooks are still stand-ins - so the layout is recorded here instead and the
  // prompt module can read it the day it is wired in. The JNI entry point stays either way,
  // so the Java view needs no change then.
  sDeviceXbox.store(xboxLayout, std::memory_order_release);
}

bool DeviceIsXbox() { return sDeviceXbox.load(std::memory_order_acquire); }

void NotePhysicalInput() { sPhysicalInput.store(true, std::memory_order_release); }

bool TakePhysicalInput() {
  const bool event = sPhysicalInput.exchange(false, std::memory_order_acq_rel);
  return event || sRealPad.load(std::memory_order_acquire);
}

} // namespace PortTouch

#if defined(__ANDROID__)
// The JNI entry points the Java view calls. Package org.metroidprime2.port, class
// TouchControlsView; every one of them is a static method there, hence the unused `jclass`.
//
// Only what the Java view declares is here. MP1's wheels, map, pointer menus, Turbo and beam
// shift are not ported, so no native exists for them: porting one adds its Java declaration
// and its native together.
extern "C" {

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeVirtualButton(
    JNIEnv*, jclass, jint button, jboolean down) {
  PortTouch::SetButton(static_cast< int >(button), down == JNI_TRUE);
}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeVirtualAxis(
    JNIEnv*, jclass, jint axis, jfloat value) {
  PortTouch::SetAxis(static_cast< int >(axis), static_cast< float >(value));
}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeTouchAim(
    JNIEnv*, jclass, jfloat dxDp, jfloat dyDp) {
  PortTouch::AddAim(static_cast< float >(dxDp), static_cast< float >(dyDp));
}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeTouchAimDown(
    JNIEnv*, jclass, jboolean down) {
  PortTouch::SetAimDown(down == JNI_TRUE);
}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeTouchSpringSwipe(
    JNIEnv*, jclass) {
  PortTouch::SpringSwipe();
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchAimEnabled(JNIEnv*, jclass) {
  return PortTouch::AimEnabled() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchClassic(JNIEnv*, jclass) {
  return PortTouch::Classic() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchTwinStick(JNIEnv*, jclass) {
  return PortTouch::TwinStick() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchLabels(JNIEnv*, jclass) {
  return PortTouch::Labels() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchColors(JNIEnv*, jclass) {
  return PortTouch::Colors() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchFloatingStick(JNIEnv*, jclass) {
  return PortTouch::FloatingStick() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jfloat JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchSideMarginDp(JNIEnv*, jclass) {
  return PortTouch::SideMarginDp();
}

JNIEXPORT jfloat JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchStickInsetDp(JNIEnv*, jclass) {
  return PortTouch::StickInsetDp();
}

JNIEXPORT jfloat JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchButtonInsetDp(JNIEnv*, jclass) {
  return PortTouch::ButtonInsetDp();
}

JNIEXPORT jfloat JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchOpacity(JNIEnv*, jclass) {
  return PortTouch::Opacity();
}

JNIEXPORT jstring JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchLayout(JNIEnv* env, jclass) {
  return env->NewStringUTF(PortTouch::Layout().c_str());
}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeSetTouchLayout(
    JNIEnv* env, jclass, jstring layout) {
  if (layout == nullptr) {
    return;
  }
  const char* chars = env->GetStringUTFChars(layout, nullptr);
  if (chars != nullptr) {
    PortTouch::SetLayout(chars);
    env->ReleaseStringUTFChars(layout, chars);
  }
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchEditRequested(JNIEnv*, jclass) {
  return PortTouch::TakeEditRequested() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeRequestTouchEdit(JNIEnv*, jclass) {
  PortTouch::RequestEdit();
}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeCycleTouchLayout(JNIEnv*, jclass) {
  PortTouch::CycleLayout();
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeDebugOverlayVisible(JNIEnv*, jclass) {
  return PortTouch::OverlayVisible() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeSetTouchDevice(JNIEnv*, jclass,
                                                                  jboolean xbox) {
  PortTouch::NoteDevice(xbox == JNI_TRUE);
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTakePhysicalInput(JNIEnv*, jclass) {
  return PortTouch::TakePhysicalInput() ? JNI_TRUE : JNI_FALSE;
}
} // extern "C"
#endif
