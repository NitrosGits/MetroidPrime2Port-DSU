#pragma once

#include <cstdint>
#include <string>

// The F1 debug overlay: a small ImGui window with the port's live settings and the warp
// buttons, so the knobs do not need environment variables and the game can be driven around
// without walking. Aurora creates the ImGui context and feeds it the SDL events; the overlay
// only has to build its windows between `aurora_begin_frame` and `aurora_end_frame`.

class CGameOptions;
class CWorld;
class CWorldLayerState;

namespace PortOverlay {

// The game's own options (rumble, invert Y, brightness, volumes...), kept in settings.cfg as the
// Prime 1 port keeps them (`game_options`). Retail keeps them per save file, so a change the card
// never saved was lost; here the file holds the last options set anywhere and they are global.
// RestoreGameOptions applies the file to the live options at boot and after a reset, and starts
// SyncGameOptions recording; ApplySavedGameOptions overrides a save file's options as it loads
// (false when the file has none); SyncGameOptions, once a frame, records any change.
void RestoreGameOptions();
bool ApplySavedGameOptions(CGameOptions& options);
void SyncGameOptions();

// Published by the game every update, so the window can say where the player is and offer the
// warp and item pages: the ids are the game's own (a world asset id and an area index), and
// `world` is the live world, from which the room list is copied then and there - the list is a
// snapshot, because a world teleport frees the world while the overlay still draws the frames
// before the restart replaces it. `world` is null when there is none; the pages then say so
// instead of drawing an empty list.
void SetGameState(uint32_t worldId, int areaId, CWorld* world);
// The current area's script layers (name, active in the world's layer state), copied every update
// for the Cheats page's layer toggles; `layers` null or an area it lacks clears the list.
void SetAreaLayers(const CWorldLayerState* layers, int areaId);
// "Temple Grounds - Landing Site": the English world and room names, as far as they are known
// (rooms only for the current world, once their name tables have loaded); empty when the room's is not.
std::string RoomName(uint32_t worldId, int areaIndex);
// Opens the window on the first page whose name starts with `name` (case-insensitive: "save"
// is Save states), or closes it for "off"; false when no page matches (console `overlay`).
bool ShowPage(const std::string& name);
// Toggles the window; safe to call from the event loop (F1).
void Toggle();
// Scrolls the open window by `steps` rows: the phone's two-finger drag, since a finger cannot turn
// a mouse wheel and the window is taller than the screen. Queued in ImGui's input queue.
void Wheel(float steps);
// Debug setting (`MP2_SKIP_CINEMATICS=1`): cinematics end as soon as they start, including ones
// the player has never seen, which the game otherwise refuses to skip. Testing only - it skips
// story beats along with the intro.
bool SkipCinematics();
void SetSkipCinematics(bool skip);
// Show / Skip with A or Start / Off, for the two settings below (F1 > Game > Cutscenes).
const int kScreen_Show = 0;
const int kScreen_Skippable = 1;
const int kScreen_Off = 2;
// The health warning and the Nintendo, Retro and Dolby logos at launch (CSplashScreen,
// `MP2_BOOT_LOGOS=show|skip|off`): Skippable ends the current one with its 0.5 s fade on A or
// Start; Off goes straight from the progressive check to the front end.
int BootLogos();
void SetBootLogos(int mode);
// The world-name card while a game loads (CMFGameLoader/CWorldTransManager,
// `MP2_WORLD_NAME=show|skip|off`): Skippable types it out at once on A or Start, so it fades as
// soon as the load is done; Off loads behind the plain black transition. The first boot's intro
// text keeps the game's own rule (Start skips it once seen).
int WorldNameCard();
void SetWorldNameCard(int mode);
// Cheat (`MP2_GODMODE=1`, F1 > Cheats): the player takes no damage, so a hazard cannot end a
// test run in the lose-game screen (CStateManager::ApplyLocalDamage). Never saved.
bool GodMode();
void SetGodMode(bool enabled);
// Gameplay tweaks (F1 > Gameplay, the pause menu's Options > Controller, and one console command
// each), off by default. Never saved - there is no settings file, and these are preference rather
// than state, so a restart goes back to the untouched game.
//
// Fast Morph (`MP2_FAST_MORPH=1`): the morph and unmorph transitions take kFastMorphDuration
// seconds instead of the game's one, so rolling is not a second of animation each way
// (CPlayer::UpdateMorphBallState).
const float kFastMorphDuration = 0.2f;
bool FastMorph();
void SetFastMorph(bool enabled);
// Spring Ball (`MP2_SPRING_BALL=1`): C-stick up on the ground in Morph Ball is a bomb jump from the
// ball's own position, as Trilogy's nunchuk flick is, so the ball can leave the ground without a
// bomb to spend (needs Morph Ball Bombs; CMorphBall::ComputeBallMovement).
bool SpringBall();
void SetSpringBall(bool enabled);
// Toggle Lock-On (`MP2_LOCK_ON_TOGGLE=1`): an L press that locks on latches L until the next press,
// and a lock that ends (target killed, out of range) lets go by itself. Sticky Charge
// (`MP2_STICKY_CHARGE=1`): letting go of an A hold of kStickyChargeAfter or more keeps the beam
// charging until the next press fires it; taps shoot as usual. Both are the state machines in
// port_hold_toggle.h, applied to the player's own CFinalInput in CStateManager::ProcessInput.
const float kStickyChargeAfter = 0.35f;
bool LockOnToggle();
void SetLockOnToggle(bool enabled);
bool StickyCharge();
// The mouse buttons drive the pad's gameplay rows under mouse aim (`MP2_MOUSE_BUTTONS=0` turns off).
bool MouseButtons();
void SetStickyCharge(bool enabled);
// Auto-aim (`MP2_AUTO_AIM=0` turns it off, default on): off, the arm cannon aims only at a
// locked or orbited target, as MP1's auto-aim switch does (CPlayer::UpdateAimTarget).
bool AutoAim();
void SetAutoAim(bool enabled);
// Pointer aim, Trilogy style (`MP2_POINTER_AIM=1`, default off; MP1's pointer aim): the mouse,
// twin stick or touch aim moves a pointer instead of the view. Inside the dead box
// (`MP2_POINTER_AIM_BOX`, percent of the half-screen) the view holds still; past it the view
// turns toward that edge, up to `MP2_POINTER_AIM_TURN` degrees per second at the edge. The arm
// cannon aims along the pointer's ray (CPlayer::UpdateGunTransform). Needs a direct aim source.
const int kPointerAimBoxMin = 20;
const int kPointerAimBoxMax = 95;
const int kPointerAimBoxDefault = 70;
const int kPointerAimTurnMin = 30;
const int kPointerAimTurnMax = 720;
const int kPointerAimTurnDefault = 180;
bool PointerAim();
void SetPointerAim(bool enabled);
int PointerAimBox();
int PointerAimTurn();
// Gyro aim (`MP2_GYRO_AIM=1`, default off; MP1's gyro aim in "always aim" mode, pointer only): a
// controller's gyro (or the phone's, `MP2_GYRO_SOURCE` 0 auto, 1 controller, 2 phone) moves the
// pointer while pointer aim is on. `MP2_GYRO_CURSOR_ANGLE` degrees of pad turn sweep the pointer
// across the screen. Space (`MP2_GYRO_SPACE`, PortGyro::Space), auto calibrate
// (`MP2_GYRO_AUTO_CALIBRATE` 0 off, 1 always, 2 only in menus), smoothing and tightening
// (`MP2_GYRO_SMOOTHING`/`MP2_GYRO_TIGHTENING`, deg/s, 0 = off) are MP1's, ranges and defaults.
const int kGyroSourceDefault = 0;
const int kGyroSpaceDefault = 1;
const int kGyroAutoCalibrateDefault = 1;
const float kGyroThresholdMax = 50.f;
const float kGyroCursorAngleMin = 5.f;
const float kGyroCursorAngleMax = 180.f;
const float kGyroCursorAngleDefault = 40.f;
// The "Gyro aim" binding's mode (`MP2_GYRO_MODE`, MP1's numbers): 1 hold to aim, 2 always aim,
// 3 hold to pause, 4 toggle to aim. "Gyro also turns the camera" (`MP2_GYRO_CURSOR_CAMERA=1`)
// turns the view by `MP2_GYRO_SENSITIVITY` (0.1..10, x the pad's turn) while the gyro moves the
// pointer.
const int kGyroModeMin = 1;
const int kGyroModeMax = 4;
const int kGyroModeDefault = 2;
const float kGyroSensitivityMin = 0.1f;
const float kGyroSensitivityMax = 10.f;
const float kGyroSensitivityDefault = 2.f;
bool GyroAim();
int GyroMode();
float GyroSensitivity();
bool GyroCursorCamera();
void SetGyroAim(bool enabled);
int GyroSource();
int GyroSpace();
int GyroAutoCalibrate();
float GyroSmoothing();
float GyroTightening();
float GyroCursorAngle();
// Remastered charge (`MP2_RAPID_CHARGE=1`), Power Beam only: holding fire after the press shot
// fires two quick shots, then charges faster (CPlayerGun::PortRapidChargeShot).
bool RapidCharge();
void SetRapidCharge(bool enabled);
// Back up the card before the game writes it (`MP2_CARD_BACKUP=1`, F1 > Saves): each write to the
// memory card copies the file as it is on disk first, into `<user>/save-backups`, so a save that
// goes wrong can be imported back (console `save import latest`). Off by default; never saved.
// Hooked in CMemoryCardSys::CCardFileInfo::WriteFile through PortSave::BeforeCardWrite.
bool CardBackup();
void SetCardBackup(bool enabled);
// Whether stdout and stderr also go to a file (`MP2_LOG_FILE=1`, F1 > Testing, console `logfile`).
// On unless the variable is `0`; PortLogFile::Start reads it at startup, so a change here only
// reports what the next launch will do.
bool LogFile();
void SetLogFile(bool enabled);
// First-person field of view in degrees (`MP_FOV=<deg>`, F1 > Render), or 0 for the game's own 55.
// Applied by CCameraManager::UpdateCameras, because the first-person camera takes its FOV when it is
// built; CCameraManager::mFirstPersonFov is what a rebuilt one reads.
float FirstPersonFov();
// Arm cannon offset from its retail place, in metres along the camera's right, forward and up axes
// (`MP2_GUN_OFFSET="<right>,<forward>,<up>"`, F1 > Render), each clamped to +-kGunOffsetMax. It moves
// the real gun, so shots leave from the moved muzzle (CPlayer::UpdateArmAndGunTransforms).
const float kGunOffsetMax = 1.f;
void GunOffset(float out[3]);
void SetGunOffset(const float offset[3]);
// Multisample anti-aliasing (1 = off, 4 = 4x, the only counts WebGPU guarantees) and the max texture
// anisotropy (1-16; retail-style mipmapped textures ask for the max). `MP2_MSAA` / `MP2_ANISO`, F1 >
// Render; applied by aurora_set_graphics_quality at the next frame start, so a change rebuilds the
// framebuffers and costs a frame.
int Msaa();
void SetMsaa(int samples);
int Anisotropy();
void SetAnisotropy(int maxAnisotropy);
// HUD scale in percent (`MP2_HUD_SCALE=<50-100>`, F1 > HUD), 100 = the layout the game authored. It
// scales the HUD's own projection about the screen centre, so every element shrinks toward the
// middle; the helmet, menus and memo screens are not scaled.
const int kHudScaleMin = 50;
const int kHudScaleMax = 100;
int HudScale();
void SetHudScale(int percent);
// True while CSamusHud::Draw is drawing the HUD: CGuiCamera::Draw scales its projection while it is
// set, so nothing else (pause screen, inventory, memo) is affected.
bool HudScalingActive();
void SetHudScalingActive(bool active);
// Draw without the HUD's view overlays (`MP2_NO_HELMET=1`, `MP2_NO_VISOR_FX=1`, F1 > HUD). Read by
// CInGameGuiManager::Draw (the helmet, through its own mHelmetVisMode gate) and CPlayerVisor::Draw
// (the per-visor effect: on this port that is the scan visor's window and edges).
bool HideHelmet();
bool HideVisorEffects();
// The view's aim crosshair while the port's aim owns the body heading (`aim_crosshair`, on by
// default; F1 > HUD, console `crosshair`). The game's own crosshair is a digital input retail
// presses with the look control, which the port never presses - without this the reticle stays
// faded out whenever the mouse, the twin stick or a touch drag is aiming, which is always on the
// phone. The Prime port sets its crosshair bit from its `AimCrosshair` setting the same way.
bool AimCrosshair();
void SetAimCrosshair(bool enabled);
// The aim crosshair, with the Prime port's settings and its meanings (PortDebug's
// ECrosshairStyle/kCrosshairSizeMin..Max/kCrosshairOpacity): the classic style is the game's own
// reticle model with the port's size, opacity and colour applied, and `remastered` replaces it
// with the cursor drawn procedurally in CTargetReticles.cpp (that is the one this port can show:
// the classic model's token does not resolve here). Size is a percent of the model's own scale
// (25..100) and applies only while the port's aim owns the view; opacity is 0..100.
const int kCrosshairSizeMin = 25;
const int kCrosshairSizeMax = 100;
const int kCrosshairSizeDefault = 50;
const int kCrosshairOpacityMin = 0;
const int kCrosshairOpacityMax = 100;
const int kCrosshairStyle_Classic = 0;
const int kCrosshairStyle_Remastered = 1;
int CrosshairStyle();
void SetCrosshairStyle(int style);
int CrosshairSize();
void SetCrosshairSize(int percent);
int CrosshairOpacity();
void SetCrosshairOpacity(int percent);
// A custom colour for both styles, as the Prime port's "custom colour" option; false keeps the
// tweak's own (the visor's colour).
bool CrosshairColor(float rgb[3]);
void SetCrosshairColor(float r, float g, float b);
void ClearCrosshairColor();
// The F1 window's size in percent of the display's own scale (`MP2_UI_SCALE=50..200`, F1 > Render,
// console `uiscale`); the product is clamped to 1..4 by PortDebug::UpdateUiScale.
const int kUiScaleMin = 50;
const int kUiScaleMax = 200;
int UiScale();
void SetUiScale(int percent);
// Vertical sync (`MP2_VSYNC=1`, F1 > Render, console `vsync`); off by default. Applied through
// aurora_enable_vsync from Draw, even with the window closed.
bool Vsync();
void SetVsync(bool enabled);
// Whether the window is fullscreen (on Android: the status and navigation bars hidden).
bool Fullscreen();
void SetFullscreen(bool enabled);
// MP1's 60 FPS cap (`MP2_FRAME_LIMIT=0|1`, F1 > Video > Frame rate, F10); on by default. The main
// loop sleeps to a 60 Hz deadline in PortPaceFrame (platform/port_events.cpp).
bool FrameLimit();
void SetFrameLimit(bool enabled);
// MP1's smooth frames (`MP2_SMOOTH_FRAMES=0|1`, F1 > Video > Frame rate, console `interp`); on by
// default. With the cap off, the camera is drawn between the last two 60 Hz ticks.
bool SmoothFrames();
void SetSmoothFrames(bool enabled);
// MP1's pose interpolation (`MP2_POSE_INTERPOLATION=0|1`, F1 > Video > Frame rate); off by default.
// With smooth frames on and the cap off, skinned models draw a blend of their last two tick poses
// (CAnimData::PortPresentedPose).
bool PoseInterpolation();
void SetPoseInterpolation(bool enabled);
// MP1's particle interpolation (`MP2_PARTICLE_INTERPOLATION=0|1`, F1 > Video > Frame rate); off by
// default. With smooth frames on and the cap off, particle systems draw between their last two
// tick states (CElementGen::PortBeginPresent) and rain/underwater flakes one step back.
bool ParticleInterpolation();
void SetParticleInterpolation(bool enabled);
// MP1's look input per frame (`MP2_LOOK_INTERPOLATION=0|1`, F1 > Video > Frame rate); on by
// default, as MP1's phase 1 is. With smooth frames on, the cap off and mouse/stick/touch aim
// owning the view, the camera turns between ticks by the pending look travel
// (CCameraManager::GetCurrentCameraTransform, PortMouse::PresentedAimDelta).
bool LookInterpolation();
void SetLookInterpolation(bool enabled);
// Widescreen HUD (`MP2_HUD_WIDE`, MP1's HudWide): on spreads the GUI over the whole framebuffer, off
// keeps the game's 4:3 proportions centred. Only matters while the framebuffer is wider than 4:3.
bool HudWide();
void SetHudWide(bool enabled);
// MP1's static render scale (`MP2_RENDER_SCALE`, F1 > Video > Quality): 0 = the window's own size
// (default), else kRenderScaleMin-kRenderScaleMax x the render mode's base, through
// VISetFrameBufferScale from Draw. Photo mode's capture overrides it while it runs.
const float kRenderScaleMin = 0.5f;
const float kRenderScaleMax = 4.f;
float RenderScale();
void SetRenderScale(float scale);
// Rendering aspect ratio, as the Prime 1 port's (`MP2_ASPECT=4:3|16:9|window`, `MP2_WIDESCREEN=1`
// for 16:9, F1 > Video > Display, console `aspect`); Window by default. 4:3 keeps the game's own
// 640x448 picture; 16:9 widens the render mode to 16:9 at the same height, and Window follows the
// window's shape as it is resized. Applied by ApplyAspect.
const int kAspect_4_3 = 0;
const int kAspect_16_9 = 1;
const int kAspect_Window = 2;
int AspectMode();
void SetAspectMode(int mode);
// "4:3", "16:9", "window"; ParseAspect takes the same names (and "16x9"/"4x3") and leaves `mode`
// alone on anything else.
const char* AspectName(int mode);
bool ParseAspect(const char* name, int& mode);
// Widens (or narrows) the render mode's fbWidth to the aspect setting. Called every iteration of
// the game loop by PortPumpEvents, so a window resize and a setting change apply on the next frame.
void ApplyAspect();
// The render mode's width over the game's own (1 at 4:3): CGuiCamera::Draw widens its projection by
// it, so the GUI keeps its proportions on a wide framebuffer.
float AspectWiden();
bool Visible();
// Builds the window. Call once per presented frame, inside the open Aurora frame.
void Draw();

} // namespace PortOverlay
