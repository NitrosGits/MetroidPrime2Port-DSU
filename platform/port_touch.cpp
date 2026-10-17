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
bool sLoaded = false;
bool sDirty = false;

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
void LoadConfig() {
  if (sLoaded) {
    return;
  }
  sLoaded = true;
  std::lock_guard< std::mutex > lock(sMutex);
  if (sConfigPath.empty()) {
    return;
  }
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

void SaveConfigLocked() {
  if (sConfigPath.empty()) {
    return;
  }
  std::ofstream file(sConfigPath, std::ios::trunc);
  if (!file) {
    std::fprintf(stderr, "[touch] cannot write %s\n", sConfigPath.c_str());
    return;
  }
  file << "classic=" << (sSettings.classic ? 1 : 0) << '\n';
  file << "twin=" << (sSettings.twin ? 1 : 0) << '\n';
  file << "layout=" << sSettings.layout << '\n';
}

PortTouchPad::Pad& Pad() {
  std::lock_guard< std::mutex > lock(sPadMutex);
  if (!sPad.ok()) {
    sPad = PortTouchPad::Attach();
    // One line per attach so a device log says whether the virtual pad ever existed; without
    // it a broken overlay and a broken mapping look the same from outside. stderr is the
    // right sink rather than a direct logcat call: on Android the entry repoints fd 2 at
    // logcat (platform/main.cpp), so this reaches the same place and still compiles for the
    // host, which has no android/log.h.
    std::fprintf(stderr, "[touch] pad attached id=%d open=%d gamepad=%d\n",
                 static_cast< int >(sPad.id), sPad.ok() ? 1 : 0, SDL_IsGamepad(sPad.id) ? 1 : 0);
    if (!sPad.ok()) {
      std::fprintf(stderr, "[touch] pad attach failed: %s\n", SDL_GetError());
    }
  }
  return sPad;
}

} // namespace

namespace PortTouch {

void Initialize(const std::string& userDirectory) {
  ReadEnvironment();
  if (!userDirectory.empty()) {
    sConfigPath = userDirectory + "/touch.cfg";
  }
  LoadConfig();
}

void BeginFrame() {
  LoadConfig();

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

  bool dirty = false;
  {
    std::lock_guard< std::mutex > lock(sMutex);
    dirty = sDirty;
    sDirty = false;
  }
  if (dirty) {
    std::lock_guard< std::mutex > lock(sMutex);
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
  }
  SaveConfigLocked();
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
  }
  SaveConfigLocked();
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
// The last group is the honest "MP1 has this, MP2 does not" set. MP1's view drives
// hold-and-slide beam/visor wheels, a minimap tap, map pan/zoom, pointer menus, a Turbo
// button, Remastered beam shift, the F1 overlay button and a texture-update toast, and asks
// the native side about each of them; MP2 has no counterpart for any of those subsystems
// yet, so each answer here is the one that keeps the feature off and out of the way instead
// of a stub that pretends. When one of them is ported, the Java side needs no change.
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

// The "MP2 has no wheels / map / pointer menu / Turbo / beam shift / overlay button" group.
// See the comment above it.

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchWheelsEnabled(JNIEnv*, jclass) {
  return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchVisorTapScan(JNIEnv*, jclass) {
  return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchMapTapEnabled(JNIEnv*, jclass) {
  return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeMinimapRect(JNIEnv*, jclass, jfloatArray) {
  return JNI_FALSE;
}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeMapTap(JNIEnv*, jclass) {
}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeMapPan(
    JNIEnv*, jclass, jfloat, jfloat, jfloat) {}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeMapZoom(JNIEnv*, jclass,
                                                                                 jfloat) {}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeMapRotate(JNIEnv*,
                                                                                   jclass, jfloat) {}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeMapScreenOpen(JNIEnv*, jclass) {
  return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativePauseScreenOpen(JNIEnv*, jclass) {
  return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeMessageBoxOpen(JNIEnv*, jclass) {
  return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativePointerMenuActive(JNIEnv*, jclass) {
  return JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeWheelOwned(JNIEnv*, jclass) {
  return 0;
}

JNIEXPORT jintArray JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeWheelIcon(
    JNIEnv*, jclass, jint, jint) {
  return nullptr;
}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeRequestVisor(JNIEnv*,
                                                                                       jclass,
                                                                                       jint) {}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeRequestBeam(JNIEnv*,
                                                                                      jclass,
                                                                                      jint) {}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTapUpdateToast(JNIEnv*, jclass, jfloat, jfloat) {
  return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchTurbo(JNIEnv*, jclass) {
  return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchBeamShift(JNIEnv*, jclass, jboolean) {}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchTurboFire(JNIEnv*, jclass, jboolean) {}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeToggleDebugOverlay(JNIEnv*, jclass) {}
} // extern "C"
#endif
