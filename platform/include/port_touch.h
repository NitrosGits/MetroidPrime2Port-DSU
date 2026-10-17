// The Android on-screen controller's native side: milestone M3 of docs/ANDROID.md.
//
// It is the MP2 counterpart of `MetroidPrimePort`'s touch handling, which is spread across
// that tree's `platform/touch_pad.cpp` (the SDL virtual gamepad), `platform/debug_ui/*.inc`
// (the settings and the touch aim) and `platform/debug_ui/android_jni.inc` (the JNI entry
// points the Java view calls). MP2 has none of those three homes - no input-settings module
// and no JNI bridge - so they collapse into this one module, which is why the JNI entry
// points live here rather than in a header of their own. The virtual gamepad itself is
// `platform/touch_pad.cpp`, ported from MP1 unchanged because it is host-testable there.
//
// Three things reach the game, and nothing else:
//
//   - a *real* SDL virtual gamepad (see touch_pad.h), so the overlay is a pad rather than
//     four keys pretending to be a stick: the game's own control mapper, its prompts and its
//     C-stick handling all apply to touch exactly as they do to a Bluetooth pad;
//   - the touch *aim*, through `PortMouse`, because the port's aim state is where the
//     player's yaw and the camera's pitch are owned for a non-stick pointer;
//   - the overlay's own settings, which is the small settings block MP1 keeps in its input
//     settings module and MP2 keeps here, persisted in `<user>/touch.cfg`.
//
// The Java view (`android/app/src/main/java/org/metroidprime2/port/TouchControlsView.java`)
// polls most of this once per draw, on the Android UI thread, while the game thread reads the
// same state each iteration: every getter here is safe to call from either thread, which is
// what the mutex and the atomics in port_touch.cpp are for.
//
// WHAT THIS MODULE DOES NOT DO. MP1's overlay drives a lot more than a pad: hold-and-slide
// beam/visor wheels, a minimap tap, map pan/zoom, pointer menus, a Turbo button, Remastered
// beam shift, the port's F1 overlay and an update toast. None of that has a counterpart in
// MP2 yet, so the Java view keeps MP1's code for it and this module answers each of those
// questions with the value that keeps the feature *off* and out of the way. The natives that
// exist for that reason are marked below; they are the honest "not ported" answer rather than
// a stub that pretends.

#pragma once

#include <string>

namespace PortTouch {

// Called once from the entry, before Aurora and before the game exists, with the user
// directory from `main.cpp`'s path resolution (the same one the memory card lives under).
// An empty path leaves the settings at their defaults and writes nothing.
void Initialize(const std::string& userDirectory);

// One per game-loop iteration, from `PortPumpEvents`: applies the pending touch aim to
// `PortMouse` and flushes a settings change to `touch.cfg`. The Java side may call any
// setter at any time (it is a different thread); this is where that becomes visible to the
// game.
void BeginFrame();

// --- the virtual gamepad (the Java view's `nativeVirtualButton` / `nativeVirtualAxis`) ---

// SDL_GamepadButton index and a normalised -1..1 axis value, exactly as the overlay sends
// them. The pad attaches on first use: SDL is up by the time Java can call this, and the
// attach has to be lazy so that a build without the overlay never attaches a device.
void SetButton(int button, bool down);
void SetAxis(int axis, float value);

// --- aim (the Java view's `nativeTouchAim` / `nativeTouchAimDown`) ---

// A drag on the free part of the screen, in density-independent pixels, positive right and
// down. Converted to aim pixels (see AimSpeed) and handed to `PortMouse::AddTouchAim`.
void AddAim(float dxDp, float dyDp);
// A finger is down on the aim area: the aim owns the view until it lifts. This is what makes
// `PortMouse` aim at all - without it the aim state would only track the camera.
void SetAimDown(bool down);
// A quick swipe that lifted: hold the aim open for a moment afterwards, so a flick turns the
// view by its whole travel instead of stopping where the finger left the glass (MP1's
// `spring_swipe`). Called by the Java side only for a swipe it measured as quick.
void SpringSwipe();

// True when the active layout aims by dragging. Off by default in neither layout (both use
// the C-stick as the game does); `MP2_TOUCH_AIM=1` turns it on for testing. MP1 has the same
// switch, wired to its F1 overlay; MP2 has no touch settings UI yet, so it is the env for now.
bool AimEnabled();
// Aim pixels per dp of drag (MP1's `touch_aim_speed`, 2.25 by default); `MP2_TOUCH_AIM_SPEED`.
float AimSpeed();

// --- layouts (the Java view polls all of these once per draw) ---

// The classic GameCube shape: main stick, C-stick, D-pad, face buttons, Z, L, R, START. This
// is the default, unlike MP1, because MP2's beams and visors are switched with the C-stick
// and MP2 has no wheel control to switch them with instead (see the module comment).
bool Classic();
// The Remastered/Xbox arrangement: the same controls, placed as an Xbox pad, with the right
// stick on the right. Exclusive with Classic; the last one switched wins.
bool TwinStick();
// Cosmetics and margins, MP1's names and defaults: `touch_labels`, `touch_colors`,
// `touch_floating_stick`, `touch_side_margin`, `touch_stick_inset`, `touch_button_inset`,
// `touch_opacity`. The layout editor writes the margins back through these.
bool Labels();
bool Colors();
bool FloatingStick();
float SideMarginDp();
float StickInsetDp();
float ButtonInsetDp();
float Opacity();

// The per-control layout the Java editor edits: `<id>:<dx>,<dy>,<scale>;...`, empty for a
// layout nothing has moved. Opaque to this module - it is the Java view's format and this
// only has to keep it across runs (MP1 stores it as `touch_layout` in its settings file;
// here it is the `layout=` line of `touch.cfg`).
std::string Layout();
void SetLayout(const std::string& layout);

// The editor's two entry points. A phone has no F1 key, so the overlay's own edit pill is
// what asks (MP1 opens the editor from the F1 window's "Edit layout"): `RequestEdit` raises
// the flag and `TakeEditRequested` clears it and reports it once.
void RequestEdit();
bool TakeEditRequested();
// Steps Classic -> Twin -> Classic, persists it, and clears the per-control offsets, which
// belong to the layout they were made in (the Java side reloads from `Layout`).
void CycleLayout();

// --- the rest of the overlay's questions ---

// True while the port's F1 overlay is up: the game is paused and the touches belong to that
// window, so the overlay hides and stops claiming them.
bool OverlayVisible();

// The overlay has just been used, and in which layout, so the game's button prompts can
// follow the input the player actually reached for. MP2 does not build its prompt-icons
// module yet (platform/port_prompts.cpp), so this only records the layout; `DeviceIsXbox` is
// what that module will read.
void NoteDevice(bool xboxLayout);
bool DeviceIsXbox();

// Whether a real pad, keyboard or mouse has been used since the last call - the overlay
// hides itself once a physical pad takes over, as it does in MP1. Polls the pad list on the
// game thread (SDL's joystick API is not documented as callable from another thread) and
// remembers what the event loop saw.
void NotePhysicalInput();
bool TakePhysicalInput();

} // namespace PortTouch
