// The JNI entry points of the on-screen controller: every native that
// android/app/src/main/java/org/metroidprime2/port/TouchControlsView.java declares, and nothing
// else. The view is a copy of MetroidPrimePort's, so this is that tree's
// platform/debug_ui/android_jni.cpp touch block, pointed at PortTouch (platform/port_touch.cpp)
// and the F1 overlay. Every one is a static method there, hence the unused `jclass`.

#if defined(__ANDROID__)

#include "port_overlay.h"
#include "port_touch.h"

#include <jni.h>

extern "C" {

// --- Settings and layout ---

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchClassic(JNIEnv*, jclass) {
  return PortTouch::Classic() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchTwinStick(JNIEnv*, jclass) {
  return PortTouch::TwinStick() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeTouchColors(JNIEnv*,
                                                                                           jclass) {
  return PortTouch::Colors() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeTouchLabels(JNIEnv*,
                                                                                           jclass) {
  return PortTouch::Labels() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchFloatingStick(JNIEnv*, jclass) {
  return PortTouch::FloatingStick() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeTouchTurbo(JNIEnv*,
                                                                                          jclass) {
  return PortTouch::Turbo() ? JNI_TRUE : JNI_FALSE;
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

JNIEXPORT jfloat JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeTouchOpacity(JNIEnv*,
                                                                                          jclass) {
  return PortTouch::Opacity();
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchControlsEnabled(JNIEnv*, jclass) {
  return PortTouch::ControlsEnabled() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchWheelsEnabled(JNIEnv*, jclass) {
  return PortTouch::WheelsEnabled() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchVisorTapScan(JNIEnv*, jclass) {
  return PortTouch::VisorTapScan() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchAimEnabled(JNIEnv*, jclass) {
  return PortTouch::AimEnabled() ? JNI_TRUE : JNI_FALSE;
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

// --- Pad, aim and the held buttons ---

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeVirtualButton(
    JNIEnv*, jclass, jint button, jboolean down) {
  PortTouch::SetButton(static_cast< int >(button), down == JNI_TRUE);
}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeVirtualAxis(
    JNIEnv*, jclass, jint axis, jfloat value) {
  PortTouch::SetAxis(static_cast< int >(axis), static_cast< float >(value));
}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeTouchAim(JNIEnv*, jclass,
                                                                                    jfloat dxDp,
                                                                                    jfloat dyDp) {
  PortTouch::AddAim(static_cast< float >(dxDp), static_cast< float >(dyDp));
}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchAimDown(JNIEnv*, jclass, jboolean down) {
  PortTouch::SetAimDown(down == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchSpringSwipe(JNIEnv*, jclass) {
  PortTouch::SpringSwipe();
}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchBeamShift(JNIEnv*, jclass, jboolean held) {
  PortTouch::SetBeamShift(held == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchTurboFire(JNIEnv*, jclass, jboolean held) {
  PortTouch::SetTurboFire(held == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeSetTouchDevice(JNIEnv*, jclass, jboolean xbox) {
  PortTouch::NoteDevice(xbox == JNI_TRUE);
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTakePhysicalInput(JNIEnv*, jclass) {
  return PortTouch::TakePhysicalInput() ? JNI_TRUE : JNI_FALSE;
}

// --- Wheels and the game state ---

JNIEXPORT jint JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeWheelOwned(JNIEnv*,
                                                                                      jclass) {
  return static_cast< jint >(PortTouch::WheelState());
}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeRequestVisor(JNIEnv*, jclass, jint visor) {
  PortTouch::RequestVisor(static_cast< int >(visor));
}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeRequestBeam(JNIEnv*,
                                                                                       jclass,
                                                                                       jint beam) {
  PortTouch::RequestBeam(static_cast< int >(beam));
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativePauseScreenOpen(JNIEnv*, jclass) {
  return PortTouch::PauseScreenOpen() ? JNI_TRUE : JNI_FALSE;
}

// --- F1 overlay ---

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeDebugOverlayVisible(JNIEnv*, jclass) {
  return PortTouch::OverlayVisible() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeToggleDebugOverlay(JNIEnv*, jclass) {
  PortOverlay::Toggle();
}

JNIEXPORT void JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeDebugWheel(JNIEnv*, jclass, jfloat steps) {
  PortOverlay::Wheel(steps);
}

// --- Stubs: MP2 has no backend for these (see port_touch.h) ---

JNIEXPORT jintArray JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeWheelIcon(JNIEnv*,
                                                                                          jclass,
                                                                                          jint,
                                                                                          jint) {
  return nullptr; // no HUD icon capture: the wheels draw their text labels
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeMessageBoxOpen(JNIEnv*, jclass) {
  return JNI_FALSE; // kFS_Paused covers it; see nativePauseScreenOpen
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

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeMapPan(JNIEnv*, jclass,
                                                                                  jfloat, jfloat,
                                                                                  jfloat) {}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeMapScreenOpen(JNIEnv*, jclass) {
  return JNI_FALSE;
}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeMapZoom(JNIEnv*, jclass,
                                                                                   jfloat) {}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeMapRotate(JNIEnv*,
                                                                                     jclass,
                                                                                     jfloat) {}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativePointerMenuActive(JNIEnv*, jclass) {
  return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeTapUpdateToast(
    JNIEnv*, jclass, jfloat, jfloat) {
  return JNI_FALSE;
}

JNIEXPORT void JNICALL Java_org_metroidprime2_port_TouchControlsView_nativeTouchControl(JNIEnv*,
                                                                                        jclass,
                                                                                        jint,
                                                                                        jboolean) {
  // no touch: control bindings
}

JNIEXPORT jboolean JNICALL
Java_org_metroidprime2_port_TouchControlsView_nativeTouchControlBound(JNIEnv*, jclass, jint) {
  return JNI_FALSE;
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

} // extern "C"

#endif
