#pragma once

#include <cstdint>

// The F1 debug overlay: a small ImGui window with the port's live settings and the warp
// buttons, so the knobs do not need environment variables and the game can be driven around
// without walking. Aurora creates the ImGui context and feeds it the SDL events; the overlay
// only has to build its windows between `aurora_begin_frame` and `aurora_end_frame`.

namespace PortOverlay {

// Published by the game every update, so the window can say where the player is. The ids are
// the game's own (a world asset id and an area index); resolving them to a name would mean
// loading the world's string table, which is not worth it for a debug label.
void SetGameState(uint32_t worldId, int areaId);
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
// Scan and X-ray swap (`MP2_SWAP_SCAN_XRAY=1`, F1 > Controls): the two visor commands trade their
// mappings while the Power Beam is equipped (CControlMapper::GetMapping, TARGET_PC only).
bool SwapScanXray();
void SetSwapScanXray(bool enabled);
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
// console `uiscale`); the product is clamped to 1..4 by PortUi::UpdateUiScale.
const int kUiScaleMin = 50;
const int kUiScaleMax = 200;
int UiScale();
void SetUiScale(int percent);
// Vertical sync (`MP2_VSYNC=1`, F1 > Render, console `vsync`); off by default. Applied through
// aurora_enable_vsync from Draw, even with the window closed.
bool Vsync();
void SetVsync(bool enabled);
bool Visible();
// Builds the window. Call once per presented frame, inside the open Aurora frame.
void Draw();

} // namespace PortOverlay
