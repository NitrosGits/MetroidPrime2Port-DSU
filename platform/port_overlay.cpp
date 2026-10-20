// The F1 debug overlay. It is deliberately small: every control here is wired to something that
// exists, so nothing in the window lies about what it does. The teleport buttons drive the same
// PortWarp requests as the Game page's buttons and the MP2_INPUT tokens, and the aim controls drive
// PortMouse, which the game reads every tick.
//
// Built by `mp_port_audio` (it needs ImGui, which Aurora brings in with SDL).

#include "port_overlay.h"

#include "port_controls.h"
#include "port_freecam.h"
#include "port_gyro_motion.h"
#include "port_livesplit.h"
#include "port_photo.h"
#include "port_env.h"
#include "port_overlay_internal.h"
#include "input/port_input_devices.h"
#include "port_input_remap_ui.h"
#include "port_logfile.h"
#include "port_harness.h"
#include "port_mouse.h"
#include "port_randomizer.h"
#include "port_touch.h"
#include "port_savestate.h"
#include "port_settings.h"
#include "port_ui_common.h"
#include "port_warp.h"

#include "Kyoto/Graphics/CGraphics.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CMemoryCard.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/IGameArea.hpp"
#include "MetroidPrime/Player/CGameOptions.hpp"
#include "MetroidPrime/Player/CGameState.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"

#include <SDL3/SDL.h>
#include <aurora/aurora.h>
#include <aurora/gfx.h>
#include <dolphin/pad.h>
#include <dolphin/vi.h>
#include <imgui.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>
#include <utility>
#include <vector>


namespace PortOverlayState {

bool sSkipCinematics = false;
// `MP2_LOG_FILE`: whether stdout and stderr also go to a file (F1 > Testing, console `logfile`).
// PortLogFile::Start reads it before Aurora comes up, so the row only reports the state the next
// launch will take.
bool sLogFile = true;
// The gameplay tweaks (`MP2_FAST_MORPH`, `MP2_SPRING_BALL`, `MP2_LOCK_ON_TOGGLE`,
// `MP2_STICKY_CHARGE`), saved in settings.cfg.
bool sFastMorph = false;
bool sSpringBall = false;
bool sLockOnToggle = false;
bool sStickyCharge = false;
bool sAutoAim = true;
bool sPointerAim = false;
int sPointerAimBox = PortOverlay::kPointerAimBoxDefault;
int sPointerAimTurn = PortOverlay::kPointerAimTurnDefault;
bool sGyroAim = false;
int sGyroSource = PortOverlay::kGyroSourceDefault;
int sGyroSpace = PortOverlay::kGyroSpaceDefault;
int sGyroAutoCalibrate = PortOverlay::kGyroAutoCalibrateDefault;
float sGyroSmoothing = 0.f;
float sGyroTightening = 0.f;
float sGyroCursorAngle = PortOverlay::kGyroCursorAngleDefault;
int sGyroMode = PortOverlay::kGyroModeDefault;
float sGyroSensitivity = PortOverlay::kGyroSensitivityDefault;
bool sGyroCursorCamera = false;
bool sRapidCharge = false;
bool sMouseButtons = true;
// `MP2_CARD_BACKUP`: copy the card's save file into save-backups before every card write (F1 >
// Saves, `cardbackup`).
bool sCardBackup = false;
// First-person field of view in degrees, or 0 for the game's own 55 (CCameraManager::sFirstPersonFOV).
float sFirstPersonFov = 0.f;
// Draw the world without the HUD's view overlays (`MP2_NO_HELMET`, `MP2_NO_VISOR_FX`): for
// screenshots of an area, and for looking at what a visor effect covers.
bool sHideHelmet = false;
bool sHideVisorEffects = false;
// `aim_crosshair`: draw the reticle while the port's own aim owns the body heading (see
// PortOverlay::AimCrosshair). On by default: without it the reticle never appears under mouse,
// twin-stick or touch aim, which is the only way the phone plays.
bool sAimCrosshair = true;
// The Prime port's crosshair settings: style (classic = the game's reticle model, remastered = the
// cursor CTargetReticles draws itself), size in percent, opacity, and an optional custom colour.
// MP2's classic model does not resolve in this port, so `remastered` is the default here.
int sCrosshairStyle = PortOverlay::kCrosshairStyle_Remastered;
int sCrosshairSize = PortOverlay::kCrosshairSizeDefault;
int sCrosshairOpacity = PortOverlay::kCrosshairOpacityMax;
bool sCrosshairCustomColor = false;
float sCrosshairColor[3] = {1.f, 1.f, 1.f};
// Arm cannon offset in metres along the camera's right, forward and up axes (`MP2_GUN_OFFSET`).
float sGunOffset[3] = {0.f, 0.f, 0.f};
// Anti-aliasing and filtering (`MP2_MSAA`, `MP2_ANISO`). The defaults are Aurora's own, so an
// untouched port applies nothing.
int sMsaa = 1;
int sAnisotropy = 16;
// HUD scale in percent (`MP2_HUD_SCALE`), and whether the HUD's own draw is the one being scaled.
int sHudScale = PortOverlay::kHudScaleMax;
// Interface scale in percent (`MP2_UI_SCALE`) and vsync (`MP2_VSYNC`).
int sUiScale = 100;
bool sVsync = false;
// MP1's 60 FPS cap (`MP2_FRAME_LIMIT`), on by default as there: PortPaceFrame holds the loop to 60 Hz.
bool sFrameLimit = true;
// MP1's smooth frames (`MP2_SMOOTH_FRAMES`, on by default): with the cap off, the camera is drawn
// between the last two ticks (CCameraManager::SetPresentationInterpolation, src/MetroidPrime/main.cpp).
bool sSmoothFrames = true;
// MP1's pose interpolation (`MP2_POSE_INTERPOLATION`, off by default as there): with smooth frames
// on, skinned poses are blended between the last two ticks (CAnimData::PortPresentedPose).
bool sPoseInterpolation = false;
// MP1's particle interpolation (`MP2_PARTICLE_INTERPOLATION`, off by default as there): with smooth
// frames on, particle systems and weather draw between the last two ticks (CElementGen::Render).
bool sParticleInterpolation = false;
// MP1's look input per frame (`MP2_LOOK_INTERPOLATION`, on by default as there): with smooth
// frames on, pointer aim turns the camera between ticks (CCameraManager, PortMouse::PresentedAimDelta).
bool sLookInterpolation = true;
// MP1's HudWide (`MP2_HUD_WIDE`, on by default as there): on a wide framebuffer the GUI spreads to the
// screen edges; off keeps the game's 4:3 proportions, centred.
bool sHudWide = true;
// MP1's static render scale (`MP2_RENDER_SCALE`): 0 draws the EFB at the window's own size (MP1's
// "Auto"), 1-4 at that multiple of the render mode's 480-line base. MP2 drew at the window's size
// before this setting, so that stays the default here (MP1's is 1).
float sRenderScale = 0.f;
// Aspect ratio (`MP2_ASPECT`): follows the window by default, as the Prime 1 port.
int sAspectMode = PortOverlay::kAspect_Window;
// MP1's sOverlayWindowed (desktop: the old floating window instead of the page window) and sUiTheme
// (0 auto, 1 Prime, 2 Plain; MP1's 2 is Remastered, which MP2 has no theme for).
bool sOverlayWindowed = false;
int sUiTheme = 0;
} // namespace PortOverlayState

using namespace PortOverlayState;

namespace {

bool sVisible = false;
bool sGodMode = false; // never saved: a cheat must not persist, so it is not a setting
bool sHudScaling = false;
bool sReadEnv = false;
uint32_t sWorldId = 0;
int sAreaId = -1;
// The room list, taken from the live world at SetGameState time (on the game thread, while the
// world is alive) and never a pointer kept across frames: a world teleport frees the outgoing
// world, and the overlay still draws the frames before the restart replaces it.
struct SRoomEntry {
  std::string name;
  bool real;
};
std::vector< SRoomEntry > sRooms;
uint32_t sRoomsWorld = 0; // the asset id the entries were taken from, 0 before any world

// Frames per second, averaged over a second, from the presented-frame clock.
float sFps = 0.f;
uint64_t sFpsWindowStart = 0;
int sFpsFrames = 0;

// Handed to aurora_set_graphics_quality only when they change; 0 means "never applied yet".
int sAppliedMsaa = 0;
int sAppliedAnisotropy = 0;
// Handed to aurora_enable_vsync only when it changes; -1 means "never applied yet".
int sAppliedVsync = -1;
// Handed to VISetFrameBufferScale only when it changes; -1 means "never applied yet".
float sAppliedRenderScale = -1.f;
// The render mode's fbWidth as CGraphics::ConfigureVideo set it, before ApplyAspect first widened it.
int sBaseFbWidth = 0;

// Applies <user>/settings.cfg once per run. An MP2_<KEY> environment variable is not applied here:
// the accessors below read it each time, so it wins without ever reaching the file.
void ReadEnv() {
  if (sReadEnv) {
    return;
  }
  sReadEnv = true;
  sGodMode = port::EnvFlag("MP2_GODMODE");
  PortOverlaySettings::LoadSettings();
}

// A setter's change is saved at once (a no-op when nothing differs from the file).
void Persist() {
  PortOverlaySettings::MarkDirty();
  PortOverlaySettings::SaveSettings();
}

// A UI row's change is saved by Draw once the mouse is up, not on every frame of a slider drag.
bool Changed(bool changed) {
  if (changed) {
    PortOverlaySettings::MarkDirty();
  }
  return changed;
}

// The effective values: the file's/UI's variable, unless the environment names it.
bool EnvBool(const char* name, bool value) { return port::EnvFlag(name, value); }

int EnvIntIn(const char* name, int value, int min, int max) {
  const int env = port::EnvInt(name, value);
  return env >= min && env <= max ? env : value;
}

float EnvFloatIn(const char* name, float value, float min, float max) {
  const float env = port::EnvFloat(name, value);
  return std::isfinite(env) && env >= min && env <= max ? env : value;
}

bool EnvFloat3(const char* name, float out[3], float min, float max) {
  const char* text = port::EnvString(name);
  float parsed[3];
  char extra = 0;
  if (text == nullptr || std::sscanf(text, "%f,%f,%f%c", &parsed[0], &parsed[1], &parsed[2], &extra) != 3) {
    return false;
  }
  for (int i = 0; i < 3; ++i) {
    if (!std::isfinite(parsed[i])) {
      return false;
    }
  }
  for (int i = 0; i < 3; ++i) {
    out[i] = std::clamp(parsed[i], min, max);
  }
  return true;
}

void StoreGunOffset(const float offset[3]) {
  for (int i = 0; i < 3; ++i) {
    // A NaN slider value or a hand-written env var must not reach the gun's transform.
    sGunOffset[i] = std::isfinite(offset[i])
                        ? std::clamp(offset[i], -PortOverlay::kGunOffsetMax, PortOverlay::kGunOffsetMax)
                        : 0.f;
  }
}

void ApplyGraphicsQuality() {
  ReadEnv();
  const int msaa = PortOverlay::Msaa();
  const int anisotropy = PortOverlay::Anisotropy();
  if (sAppliedMsaa == msaa && sAppliedAnisotropy == anisotropy) {
    return;
  }
  sAppliedMsaa = msaa;
  sAppliedAnisotropy = anisotropy;
  // A request: Aurora rebuilds the framebuffers at the next frame start (a rebuild inside a frame
  // would invalidate the binds that frame already recorded).
  aurora_set_graphics_quality(static_cast< uint32_t >(msaa),
                              static_cast< uint16_t >(anisotropy));
}

void ApplyVsync() {
  ReadEnv();
  const bool vsync = PortOverlay::Vsync();
  if (sAppliedVsync == (vsync ? 1 : 0)) {
    return;
  }
  sAppliedVsync = vsync ? 1 : 0;
  aurora_enable_vsync(vsync);
}

// Photo mode's capture holds its own scale until it is done, and puts this one back itself.
void ApplyRenderScale() {
  ReadEnv();
  if (PortPhoto::Capturing()) {
    return;
  }
  const float scale = PortOverlay::RenderScale();
  if (sAppliedRenderScale == scale) {
    return;
  }
  sAppliedRenderScale = scale;
  // Each change reallocates the EFB targets at the next frame start.
  VISetFrameBufferScale(scale);
  if (scale > 0.f) {
    std::fprintf(stderr, "[video] render scale %.2fx\n", static_cast< double >(scale));
  } else {
    std::fprintf(stderr, "[video] render scale native\n");
  }
}

void TickFps() {
  const uint64_t now = SDL_GetTicksNS();
  if (sFpsWindowStart == 0) {
    sFpsWindowStart = now;
  }
  ++sFpsFrames;
  const uint64_t elapsed = now - sFpsWindowStart;
  if (elapsed >= 1000000000ull) {
    sFps = static_cast< float >(sFpsFrames) / (static_cast< float >(elapsed) / 1e9f);
    sFpsFrames = 0;
    sFpsWindowStart = now;
  }
}

// MP1's save state rows: the selected slot is the one F5 saves and F9 loads.
void DrawSaveStates() {
  ImGui::SeparatorText("Save states");
  if (PortSaveState::Folder().empty()) {
    ImGui::TextDisabled("No user directory, so save states are off");
    return;
  }
  const std::string save = PortInputRemap::BindingsText(PortInput::Action::SaveState, " / ");
  const std::string load = PortInputRemap::BindingsText(PortInput::Action::LoadState, " / ");
  ImGui::TextDisabled("%s: save the selected slot   %s: load it", save.empty() ? "unbound" : save.c_str(),
                      load.empty() ? "unbound" : load.c_str());
  for (int slot = PortSaveState::kUndoSlot; slot <= PortSaveState::kSlotCount; ++slot) {
    ImGui::PushID(slot);
    const PortSaveState::Info info = PortSaveState::SlotInfo(slot);
    if (slot == PortSaveState::kUndoSlot) {
      ImGui::BeginDisabled(!info.exists);
      if (ImGui::Button("Undo last load")) {
        PortSaveState::RequestLoad(slot);
      }
      ImGui::EndDisabled();
    } else {
      if (ImGui::RadioButton("##sel", PortSaveState::SelectedSlot() == slot)) {
        PortSaveState::SetSelectedSlot(slot);
      }
      ImGui::SameLine();
      if (ImGui::Button("Save")) {
        PortSaveState::RequestSave(slot);
      }
      ImGui::SameLine();
      ImGui::BeginDisabled(!info.exists);
      if (ImGui::Button("Load")) {
        PortSaveState::RequestLoad(slot);
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      ImGui::Text("%d", slot);
    }
    if (info.exists) {
      const int t = static_cast< int >(info.playTime);
      ImGui::SameLine();
      ImGui::Text("%s  %d:%02d:%02d%s", info.room.c_str(), t / 3600, t / 60 % 60, t % 60,
                  info.morphed ? "  (ball)" : "");
    } else if (slot != PortSaveState::kUndoSlot) {
      ImGui::SameLine();
      ImGui::TextDisabled("empty");
    }
    ImGui::PopID();
  }
  const std::string message = PortSaveState::LastMessage();
  if (!message.empty()) {
    ImGui::TextWrapped("%s", message.c_str());
  }
  PortUi::Help("A state is the save data plus Samus's place; loading rebuilds the room as a card load "
               "does (enemies and doors reset). Loading first writes the undo state.");
}

// The card's world names are a STRG table that the game's own text parser draws, so an entry
// carries its markup (`&line-spacing=75;Temple Grounds`); the overlay shows the text alone.
std::string PlainWorldName(const wchar_t* text) {
  std::string out;
  bool inMarkup = false;
  for (const wchar_t* p = text; p != nullptr && *p != 0; ++p) {
    if (*p == L'&') {
      inMarkup = true;
    } else if (inMarkup) {
      inMarkup = *p != L';';
    } else if (*p == L'\n' || *p == L'\r' || *p == L'\t') {
      // The entries break lines by hand (that is what the line-spacing markup above is for);
      // a one-row label wants a space instead.
      out.push_back(' ');
    } else {
      out.push_back(*p < 0x80 ? static_cast< char >(*p) : '?');
    }
  }
  const size_t first = out.find_first_not_of(' ');
  const size_t last = out.find_last_not_of(' ');
  return first == std::string::npos ? std::string() : out.substr(first, last - first + 1);
}

// The Prime 1 port's Save states page (debug_ui/savestates.cpp): one page of slots, as there.
void DrawSaveStatesSection() {
  ImGui::TextDisabled("Save anywhere and load back to the same spot.");
  DrawSaveStates();
}

// The Prime 1 port's Debug page (debug_ui/debug.cpp) with the sections this port has: the free
// camera and photo, the live state a test wants to read, and the console that drives the rest.
void DrawDebugSection() {
  ImGui::SeparatorText("Game");
  ImGui::Text("world %08X  area %d", sWorldId, sAreaId);
  ImGui::Text("game updates: %u", PortHarness::GameUpdates());
  PortUi::Help("MP2_CONSOLE=<port> serves the debug console over TCP (tools/mp2con.py -p <port> "
               "help); MP2_INPUT drives the pad from a script");

  ImGui::SeparatorText("Camera");
  {
    bool freeCam = PortFreeCam::Active();
    if (PortUi::Toggle("Free camera", &freeCam, nullptr,
                       "fly a camera of your own with the pad (left stick, C stick, L/R, Z); HUD and arm "
                       "cannon are hidden. The Photo mode key (F8) toggles it too")) {
      PortFreeCam::SetActive(freeCam, nullptr);
    }
  }

  if (PortFreeCam::Active()) {
    ImGui::SeparatorText("Photo");
    // 0 keeps the game's own FOV (55 in first person); the slider shows that until it is moved.
    const bool gameFov = PortPhoto::Fov() <= 0.f;
    float fov = gameFov ? 55.f : PortPhoto::Fov();
    if (PortUi::SliderFloat("Field of view", &fov, 30.f, 100.f, gameFov ? "game (%.0f)" : "%.0f deg",
                            nullptr, "the free camera's vertical field of view; untouched, it keeps "
                                     "the game's own")) {
      PortPhoto::SetFov(fov);
    }
    // The slider stops at 30, so the game's own FOV needs its own button once the slider is moved.
    if (!gameFov) {
      ImGui::SameLine();
      if (ImGui::SmallButton("Game FOV")) {
        PortPhoto::SetFov(0.f);
      }
    }
    float roll = PortPhoto::Roll();
    if (PortUi::SliderFloat("Roll", &roll, -180.f, 180.f, "%.0f deg", nullptr,
                            "tilts the picture about the view axis; movement ignores it")) {
      PortPhoto::SetRoll(roll);
    }
    float speed = PortPhoto::Speed();
    if (PortUi::SliderFloat("Camera speed", &speed, 0.5f, 50.f, "%.1f m/s", nullptr,
                            "metres per second at a full stick; the right trigger multiplies it by four")) {
      PortPhoto::SetSpeed(speed);
    }
    int ss = PortPhoto::Supersample() >= 4 ? 2 : PortPhoto::Supersample() >= 2 ? 1 : 0;
    if (PortUi::Combo("Supersample", &ss, "1x\0" "2x\0" "4x\0", nullptr,
                      "renders the photo at 2x or 4x the window's size; a size the GPU can't hold "
                      "steps down")) {
      PortPhoto::SetSupersample(ss == 2 ? 4 : ss == 1 ? 2 : 1);
    }
    bool showSamus = PortPhoto::ShowSamus();
    if (PortUi::Toggle("Show Samus", &showSamus, nullptr,
                       "draws Samus's body where the player stands")) {
      PortPhoto::SetShowSamus(showSamus);
    }
    ImGui::BeginDisabled(PortPhoto::Capturing());
    if (ImGui::Button("Take photo")) {
      PortPhoto::RequestCapture(0);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
      ImGui::SetTooltip("saves a PNG of the picture without the HUD to screenshots/");
    }
    const PortPhoto::CaptureResult last = PortPhoto::LastCapture();
    if (PortPhoto::Capturing()) {
      PortUi::Help("Capturing...");
    } else if (last.state > 0) {
      char line[512];
      std::snprintf(line, sizeof(line), "Saved %ux%u: %s", last.width, last.height, last.path.c_str());
      PortUi::Help(line);
    } else if (last.state < 0) {
      char line[512];
      std::snprintf(line, sizeof(line), "Capture failed; the log says why");
      PortUi::Help(line);
    }
  }


  ImGui::SeparatorText("Console");
  ImGui::TextWrapped("The console runs on the game thread and answers one command per update: "
                     "`areas`, `warp`, `world`, `items`, `give`, `god`, `objs`, `send`, `press`.");
}

// The Randomizer page: this port's randomizer is the dump-then-shuffle one (docs/RANDOMIZER.md), so
// the page reports what the running game loaded and can write a seed from the dump log.
std::string sGenerateResult;

void DrawRandomizerSection() {
  ImGui::TextUnformatted(PortRandomizer::StatusText());
  const std::string seedPath = PortRandomizer::SeedFilePath();
  const std::string dumpPath = PortRandomizer::DumpLogFilePath();
  ImGui::TextWrapped("seed: %s", seedPath.c_str());
  ImGui::TextWrapped("dump log: %s", dumpPath.c_str());
  static char sSeedName[64] = "echoes1";
  ImGui::SetNextItemWidth(220.f);
  ImGui::InputText("Seed name", sSeedName, sizeof(sSeedName));
  if (ImGui::Button("Generate from the dump log")) {
    std::string error;
    sGenerateResult = PortRandomizer::GenerateSeedFromDumpLog(sSeedName, error)
                          ? "Seed written. It loads on the next launch."
                          : error;
  }
  if (!sGenerateResult.empty()) {
    PortUi::Help(sGenerateResult.c_str());
  }
  PortUi::Help("MP_RANDO_DUMP=1 logs every pickup the areas you visit hold; generate here, then "
               "restart without MP_RANDO_DUMP to play the seed. A seed only covers what was "
               "dumped, and the shuffle has no logic, so a seed may not be beatable.");
}

void DrawGameSection() {
  ImGui::SeparatorText("Game");
  ImGui::Text("world %08X  area %d", sWorldId, sAreaId);

  if (ImGui::Button("Previous area")) {
    PortWarp::CycleArea(-1);
  }
  ImGui::SameLine();
  if (ImGui::Button("Next area")) {
    PortWarp::CycleArea(1);
  }
  if (ImGui::Button("Previous world")) {
    PortWarp::CycleWorld(-1);
  }
  ImGui::SameLine();
  if (ImGui::Button("Next world")) {
    PortWarp::CycleWorld(1);
  }
  PortUi::Help("MP2_WORLD=\"<world>[,<area>]\" and MP2_TELEPORT=\"<area>\" at start-up; the "
               "room and world pickers are on the Cheats page, as in the Prime 1 port");

  ImGui::SeparatorText("Speedrun");
  {
    bool liveSplit = PortLiveSplit::Enabled();
    if (PortUi::Toggle("LiveSplit", &liveSplit, "MP2_LIVESPLIT=1",
                       "In LiveSplit: right-click, Control > Start TCP Server (port 16834), and compare "
                       "against Game Time. A new file starts the timer; the final blow splits")) {
      PortLiveSplit::SetEnabled(liveSplit);
    }
    switch (PortLiveSplit::Status()) {
    case PortLiveSplit::kStatus_Off:
      ImGui::TextDisabled("off");
      break;
    case PortLiveSplit::kStatus_Connecting:
      ImGui::TextUnformatted("connecting...");
      break;
    case PortLiveSplit::kStatus_Connected:
      ImGui::TextUnformatted("connected");
      break;
    case PortLiveSplit::kStatus_Failed:
      ImGui::TextWrapped("%s", PortLiveSplit::LastError().c_str());
      break;
    }
    static char address[128];
    static bool addressInit = false;
    if (!addressInit) {
      std::snprintf(address, sizeof(address), "%s", PortLiveSplit::Address().c_str());
      addressInit = true;
    }
    if (ImGui::InputText("Server (host:port)", address, sizeof(address),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
      PortLiveSplit::SetAddress(address);
    }
    PortUi::Note("MP2_LIVESPLIT_ADDR=<host:port>", "LiveSplit's TCP server; Enter applies it");
    bool splitUpgrades = PortLiveSplit::SplitUpgrades();
    if (PortUi::Toggle("Split on upgrades", &splitUpgrades, "MP2_LIVESPLIT_SPLIT_UPGRADES=0",
                       "a split for every new suit, beam, visor, ball part, key or other upgrade")) {
      PortLiveSplit::SetSplitUpgrades(splitUpgrades);
    }
  }

  ImGui::SeparatorText("Cutscenes");
  // The game reads this every frame in its cinematic flow state.
  Changed(PortUi::Toggle("Skip cinematics", &sSkipCinematics, "MP2_SKIP_CINEMATICS=1",
               "START or A skips any cinematic, seen or not; off, START skips only seen ones"));
}

// The Prime 1 port's Video page (debug_ui/tabs.cpp's DrawVideoTab) with its four sub-tabs. Display
// holds the view (FOV, arm cannon, helmet, HUD); Quality the render settings; Frame rate the vsync.
void DrawVideoDisplay() {
  ImGui::SeparatorText("HUD and view");
  // Applied by CCameraManager::UpdateCameras: the first-person camera takes its FOV when it is
  // built, so the live change goes through CCameraManager::mFirstPersonFov.
  Changed(PortUi::SliderFloat("Field of view (1st person)", &sFirstPersonFov, 0.f, 120.f,
                              sFirstPersonFov <= 0.f ? "game (%.0f)" : "%.0f deg",
                              "MP2_FOV=<deg>; 0 keeps the game's 55"));
  float offset[3] = {sGunOffset[0], sGunOffset[1], sGunOffset[2]};
  bool gunChanged = false;
  static const char* const kGunLabels[3] = {"Arm cannon right", "Arm cannon forward",
                                            "Arm cannon up"};
  for (int i = 0; i < 3; ++i) {
    gunChanged |= ImGui::SliderFloat(kGunLabels[i], &offset[i], -PortOverlay::kGunOffsetMax,
                                     PortOverlay::kGunOffsetMax, "%.2f m");
  }
  if (gunChanged) {
    // Read every frame by CPlayer::UpdateArmAndGunTransforms, so the gun moves on the next update
    // and the shots leave from the muzzle that is drawn.
    StoreGunOffset(offset);
    Changed(true);
  }
  if (ImGui::Button("Reset arm cannon")) {
    const float zero[3] = {0.f, 0.f, 0.f};
    StoreGunOffset(zero);
    Changed(true);
  }
  PortUi::Note("MP2_GUN_OFFSET=\"right,forward,up\" in metres, +-1", nullptr, true);
  Changed(PortUi::Toggle("Hide helmet", &sHideHelmet, "MP2_NO_HELMET=1"));
  Changed(PortUi::Toggle("Hide visor effects", &sHideVisorEffects, "MP2_NO_VISOR_FX=1",
               "the scan visor's window and edges are what a visor effect draws here"));
  if (sAimCrosshair) {
    int style = sCrosshairStyle;
    if (PortUi::Combo("Crosshair style", &style, "Classic\0" "Remastered\0",
                      "MP2_CROSSHAIR_STYLE=classic|remastered",
                      "Remastered is the one this port can draw: the classic reticle's model does "
                      "not load here")) {
      sCrosshairStyle = style == PortOverlay::kCrosshairStyle_Remastered
                            ? PortOverlay::kCrosshairStyle_Remastered
                            : PortOverlay::kCrosshairStyle_Classic;
      Changed(true);
    }
    Changed(PortUi::SliderInt("Crosshair opacity", &sCrosshairOpacity,
                              PortOverlay::kCrosshairOpacityMin, PortOverlay::kCrosshairOpacityMax,
                              "%d%%", "MP2_CROSSHAIR_OPACITY=0..100"));
  }
  // Read by CGuiCamera::Draw while CSamusHud::Draw is drawing the HUD
  // (PortOverlay::HudScalingActive): the HUD's projection is scaled about the screen centre.
  Changed(PortUi::SliderInt("HUD scale", &sHudScale, PortOverlay::kHudScaleMin,
                            PortOverlay::kHudScaleMax, "%d%%", "MP2_HUD_SCALE=50..100",
                            "shrinks the HUD toward the middle; the helmet and menus keep "
                            "their size"));
}

void DrawVideoQuality() {
  ImGui::SeparatorText("Quality");
  int msaa = sMsaa >= 4 ? 1 : 0;
  // Applied by PortOverlay::Draw through aurora_set_graphics_quality: the framebuffers are rebuilt
  // at the next frame start.
  if (PortUi::Combo("Anti-aliasing", &msaa, "Off\0" "4x MSAA\0", "MP2_MSAA=1|4")) {
    sMsaa = msaa == 1 ? 4 : 1;
    Changed(true);
  }
  // A max, so the combo shows the smallest offered step that covers it (a hand-written MP2_ANISO=3
  // reads as 4x); picking a row sets that step.
  static const int kAnisoSteps[] = {1, 2, 4, 8, 16};
  int aniso = 4;
  for (int i = 0; i < 5; ++i) {
    if (kAnisoSteps[i] >= sAnisotropy) {
      aniso = i;
      break;
    }
  }
  if (PortUi::Combo("Anisotropic filtering", &aniso, "1x\0" "2x\0" "4x\0" "8x\0" "16x\0",
                    "MP2_ANISO=1..16")) {
    sAnisotropy = kAnisoSteps[aniso];
    Changed(true);
  }
  // MP1's render scale, without its dynamic resolution. Applied by ApplyRenderScale from Draw.
  bool autoScale = sRenderScale <= 0.f;
  if (PortUi::Toggle("Auto render scale (native)", &autoScale, "MP2_RENDER_SCALE=0",
                     "draws at the window's own size")) {
    sRenderScale = autoScale ? 0.f : 1.f;
    Changed(true);
  }
  if (!autoScale) {
    // Applied when the slider is let go: each new scale reallocates the EFB targets, which a drag
    // would otherwise do every frame.
    static float sPendingScale = 0.f;
    float scale = sPendingScale > 0.f ? sPendingScale : sRenderScale;
    if (PortUi::SliderFloat("EFB scale", &scale, 1.f, 4.f, "%.2fx", "MP2_RENDER_SCALE=1..4",
                            "scales the internal EFB from the 480-line base; above 2x it "
                            "supersamples, and each step costs much more GPU time")) {
      sPendingScale = scale;
    }
    if (sPendingScale > 0.f && !ImGui::IsAnyItemActive()) {
      sRenderScale = sPendingScale;
      sPendingScale = 0.f;
      Changed(true);
    }
  }
}

void DrawVideoFrameRate() {
  ImGui::SeparatorText("Frame rate");
  // Applied by PortOverlay::Draw through aurora_enable_vsync, every frame it changes.
  Changed(PortUi::Toggle("VSync", &sVsync, "MP2_VSYNC=1",
                         "waits for the display's refresh before presenting a frame"));
  // Paced by PortPaceFrame at the top of each main-loop iteration; F10 toggles it, as in MP1.
  bool frameLimit = sFrameLimit;
  if (PortUi::Toggle("60 FPS cap (target)", &frameLimit, "MP2_FRAME_LIMIT=0|1",
                     "holds the game to 60 frames a second, the rate it was written for (F10)")) {
    PortOverlay::SetFrameLimit(frameLimit);
  }
  // Read by the main loop's draw: with the cap on it changes nothing.
  ImGui::BeginDisabled(sFrameLimit);
  bool smoothFrames = sSmoothFrames;
  if (PortUi::Toggle("Smooth frames", &smoothFrames, "MP2_SMOOTH_FRAMES=0|1",
                     "with the cap off, draws the camera between the game's 60 Hz ticks")) {
    PortOverlay::SetSmoothFrames(smoothFrames);
  }
  // Read by CAnimData::PortPresentedPose: needs smooth frames, so it changes nothing without them.
  ImGui::BeginDisabled(!sSmoothFrames);
  bool poseInterpolation = sPoseInterpolation;
  if (PortUi::Toggle("Smooth animation", &poseInterpolation, "MP2_POSE_INTERPOLATION=0|1",
                     "with smooth frames, also draws animated models between the game's ticks")) {
    PortOverlay::SetPoseInterpolation(poseInterpolation);
  }
  // Read by CElementGen::PortBeginPresent and CEnvFxManager::Render, under the same condition.
  bool particleInterpolation = sParticleInterpolation;
  if (PortUi::Toggle("Smooth effects", &particleInterpolation, "MP2_PARTICLE_INTERPOLATION=0|1",
                     "with smooth frames, also draws particles and weather between the game's ticks")) {
    PortOverlay::SetParticleInterpolation(particleInterpolation);
  }
  // Read by CCameraManager::GetCurrentCameraTransform, under the same condition.
  bool lookInterpolation = sLookInterpolation;
  if (PortUi::Toggle("Smooth look", &lookInterpolation, "MP2_LOOK_INTERPOLATION=0|1",
                     "with smooth frames, also turns the view between the game's ticks as the mouse, stick or touch aims")) {
    PortOverlay::SetLookInterpolation(lookInterpolation);
  }
  ImGui::EndDisabled();
  ImGui::EndDisabled();
}

void DrawVideoAspect() {
  ImGui::SeparatorText("Aspect ratio");
  // Applied by ApplyAspect on the next loop iteration: the render mode is widened and the world and
  // GUI cameras follow its viewport.
  int aspect = sAspectMode;
  if (PortUi::Combo("Aspect ratio", &aspect, "4:3 (original)\0" "16:9\0" "Follow window\0",
                    "MP2_ASPECT=4:3|16:9|window",
                    "4:3 keeps the game's own picture; Follow window fills the window's shape")) {
    sAspectMode = aspect;
    Changed(true);
  }
  bool hudWide = sHudWide;
  if (PortUi::Toggle("Widescreen HUD", &hudWide, "MP2_HUD_WIDE=0|1",
                     "on spreads the HUD to the screen edges; off keeps its 4:3 proportions, centred")) {
    PortOverlay::SetHudWide(hudWide);
  }
}

// MP1 files its renderer workarounds (texture copy paths, backend and API choices) here; this port
// has no such knob on a page yet, so the tab says so rather than inventing one.
void DrawVideoCompatibility() {
  ImGui::BeginDisabled();
  ImGui::TextWrapped("No compatibility options in this port yet.");
  ImGui::EndDisabled();
}

void DrawVideoSection() {
  if (ImGui::BeginTabBar("videoPages")) {
    if (PortOverlaySearch::SubTab("Video", "Display")) {
      DrawVideoAspect();
      DrawVideoDisplay();
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Video", "Quality")) {
      DrawVideoQuality();
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Video", "Frame rate")) {
      DrawVideoFrameRate();
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Video", "Compatibility")) {
      DrawVideoCompatibility();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

// The Prime 1 port's System page (DrawSystemTab) with the sections this port has rows for, in its
// order: Overlay, Settings, MemoryCard, Log, About. Discord and Update have no MP2 backend.
void DrawSystemSection() {
  ImGui::SeparatorText("Overlay");
#if !defined(__ANDROID__)
  Changed(PortUi::Toggle("Overlay as a floating window", &sOverlayWindowed, nullptr));
#endif
  Changed(PortUi::Combo("Menu theme", &sUiTheme, "Auto\0Metroid Prime\0Plain\0", nullptr,
                        "The look of this overlay. Auto is the Metroid Prime pause-screen style; Plain is "
                        "stock Dear ImGui."));
  // Multiplies the display's own scale (PortUi::UpdateUiScale); the product is clamped to 1..4, so on a
  // desktop at scale 1 only values above 100% make the window larger.
  Changed(PortUi::SliderInt("UI scale", &sUiScale, PortOverlay::kUiScaleMin, PortOverlay::kUiScaleMax, "%d%%",
                            "MP2_UI_SCALE=50..200",
                            "the size of this window's text and controls"));

  ImGui::SeparatorText("Settings");
  const bool dirty = PortOverlaySettings::Dirty();
  ImGui::TextUnformatted(dirty ? "Unsaved changes" : "Saved");
  ImGui::SameLine();
  ImGui::BeginDisabled(!dirty);
  if (ImGui::Button("Save settings now")) {
    PortOverlaySettings::SaveSettings();
  }
  ImGui::EndDisabled();
  if (const std::string& path = PortSettings::FilePath(); !path.empty()) {
    ImGui::TextWrapped("%s", path.c_str());
  }
  PortUi::Help("Changes are written when no control is being edited; MP2_<KEY> environment "
               "variables override the file for one run");

  ImGui::SeparatorText("MemoryCard");
  Changed(PortUi::Toggle("Back up the card", &sCardBackup, "MP2_CARD_BACKUP=1",
               "copies the save file into save-backups before each card write; `save import "
               "latest` on the console puts one back"));

  ImGui::SeparatorText("Log");
  Changed(PortUi::Toggle("Log file", &sLogFile, "MP2_LOG_FILE=1",
               "stdout and stderr go to metroid_prime2_port.log in the user folder; takes "
               "effect next launch"));

  ImGui::SeparatorText("About");
  ImGui::TextUnformatted("Metroid Prime 2: Echoes native port (G2ME01)");
  ImGui::TextUnformatted("Aurora (SDL3 / WebGPU), Dear ImGui");
}

// The Prime 1 port's Cheats page (debug_ui/cheats.cpp) for this game: the item list is Echoes'
// own and the grant path is the console's `give`/`take` (platform/port_console.cpp, which is
// CScriptPickup's own path): capacity and amount move together, and the step is a delta, so a
// slider sets a value rather than adding one. Zero removes the item, which is all it takes -
// every ability check here reads the capacity (HasPowerUp is `mCapacity > 0`).
void GrantItem(CPlayerState& ps, CPlayerState::EItemType type, int capacity, int amount) {
  // The Prime 1 port's GrantItem in this game's terms: `capacity` is what MP1 calls the power-up
  // and `amount` what it calls the pickup, and the two fields are named that way here. The
  // capacity is moved by the difference, because that is how AddPowerUp takes an expansion, and
  // the amount is then set outright - sliding a counter down must lower the amount even when the
  // capacity has room for it (which AddPowerUp alone would not do).
  ps.AddPowerUp(type, capacity - ps.GetItemCapacity(type));
  ps.SetItemAmount(type, amount);
}

struct SItemCheck {
  const char* name;
  CPlayerState::EItemType type;
};

constexpr SItemCheck kAbilityItems[] = {
    {"Power Beam", CPlayerState::kIT_PowerBeam},
    {"Dark Beam", CPlayerState::kIT_DarkBeam},
    {"Light Beam", CPlayerState::kIT_LightBeam},
    {"Annihilator Beam", CPlayerState::kIT_AnnihilatorBeam},
    {"Charge Beam", CPlayerState::kIT_ChargeBeam},
    {"Super Missile", CPlayerState::kIT_SuperMissile},
    {"Darkburst", CPlayerState::kIT_Darkburst},
    {"Sunburst", CPlayerState::kIT_Sunburst},
    {"Sonic Boom", CPlayerState::kIT_SonicBoom},
    {"Combat Visor", CPlayerState::kIT_CombatVisor},
    {"Scan Visor", CPlayerState::kIT_ScanVisor},
    {"Dark Visor", CPlayerState::kIT_DarkVisor},
    {"Echo Visor", CPlayerState::kIT_EchoVisor},
    {"Varia Suit", CPlayerState::kIT_VariaSuit},
    {"Dark Suit", CPlayerState::kIT_DarkSuit},
    {"Light Suit", CPlayerState::kIT_LightSuit},
    {"Morph Ball", CPlayerState::kIT_MorphBall},
    {"Boost Ball", CPlayerState::kIT_BoostBall},
    {"Spider Ball", CPlayerState::kIT_SpiderBall},
    {"Morph Ball Bombs", CPlayerState::kIT_MorphBallBombs},
    {"Dark Bomb", CPlayerState::kIT_DarkBomb},
    {"Light Bomb", CPlayerState::kIT_LightBomb},
    {"Annihilator Bomb", CPlayerState::kIT_AnnihilatorBomb},
    {"Grapple Beam", CPlayerState::kIT_GrappleBeam},
    {"Space Jump Boots", CPlayerState::kIT_SpaceJumpBoots},
    {"Gravity Boost", CPlayerState::kIT_GravityBoost},
    {"Seeker Launcher", CPlayerState::kIT_SeekerLauncher},
    {"Screw Attack", CPlayerState::kIT_ScrewAttack},
    {"Translator", CPlayerState::kIT_TranslatorUpgrade_TempETM},
};

constexpr SItemCheck kKeyItems[] = {
    {"Temple Key 1", CPlayerState::kIT_TempleKey1},
    {"Temple Key 2", CPlayerState::kIT_TempleKey2},
    {"Temple Key 3", CPlayerState::kIT_TempleKey3},
    {"Agon Key 1", CPlayerState::kIT_AgonKey1},
    {"Agon Key 2", CPlayerState::kIT_AgonKey2},
    {"Agon Key 3", CPlayerState::kIT_AgonKey3},
    {"Torvus Key 1", CPlayerState::kIT_TorvusKey1},
    {"Torvus Key 2", CPlayerState::kIT_TorvusKey2},
    {"Torvus Key 3", CPlayerState::kIT_TorvusKey3},
    {"Hive Key 1", CPlayerState::kIT_HiveKey1},
    {"Hive Key 2", CPlayerState::kIT_HiveKey2},
    {"Hive Key 3", CPlayerState::kIT_HiveKey3},
};

// The toggles as a grid, as many columns as the widest name leaves room for.
void DrawItemChecks(CPlayerState& ps, const char* tableId, const SItemCheck* items, int count) {
  float nameWidth = 0.f;
  for (int i = 0; i < count; ++i) {
    nameWidth = std::max(nameWidth, ImGui::CalcTextSize(items[i].name).x);
  }
  nameWidth += ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x +
               ImGui::GetStyle().ItemSpacing.x * 2.f;
  const int columns = std::clamp(int(ImGui::GetContentRegionAvail().x / nameWidth), 1, 4);
  if (ImGui::BeginTable(tableId, columns)) {
    for (int i = 0; i < count; ++i) {
      ImGui::TableNextColumn();
      bool owned = ps.HasPowerUp(items[i].type);
      if (ImGui::Checkbox(items[i].name, &owned)) {
        GrantItem(ps, items[i].type, owned ? 1 : 0, owned ? 1 : 0);
      }
    }
    ImGui::EndTable();
  }
}

void DrawCheatsSection() {
  PortUi::Toggle("God mode", &sGodMode, "MP2_GODMODE=1",
                 "the player takes no damage; other actors still do. Not saved");

  CPlayerState* ps = gpGameState != nullptr ? gpGameState->GetPlayerState().GetPtr() : nullptr;
  // The front end has its own player state and its own Samus; the page is about the game's.
  if (ps == nullptr ||
      (gpGameState != nullptr && gpGameState->GetGameMode().GetGameModeType() == 'FRND')) {
    ImGui::TextDisabled("Waiting for gameplay...");
    return;
  }
  CHealthInfo* health = ps->HealthInfo();
  ImGui::Text("Health: %.0f / %.0f", health->GetHP(), ps->CalculateHealth());
  if (ImGui::Button("Full health")) {
    health->SetHP(ps->CalculateHealth());
  }
  ImGui::SameLine();
  if (ImGui::Button("Grant everything")) {
    for (const SItemCheck& item : kAbilityItems) {
      GrantItem(*ps, item.type, 1, 1);
    }
    for (const SItemCheck& item : kKeyItems) {
      GrantItem(*ps, item.type, 1, 1);
    }
    for (const CPlayerState::EItemType counter :
         {CPlayerState::kIT_Missile, CPlayerState::kIT_Powerbomb, CPlayerState::kIT_EnergyTanks,
          CPlayerState::kIT_DarkAmmo, CPlayerState::kIT_LightAmmo}) {
      const int max = CPlayerState::GetPowerUpMaxValue(counter);
      GrantItem(*ps, counter, max, max);
    }
    health->SetHP(ps->CalculateHealth());
  }

  ImGui::SeparatorText("Abilities");
  DrawItemChecks(*ps, "##abilitychecks", kAbilityItems, int(std::size(kAbilityItems)));
  if (ImGui::CollapsingHeader("Keys")) {
    DrawItemChecks(*ps, "##keychecks", kKeyItems, int(std::size(kKeyItems)));
  }

  const auto counter = [ps](const char* label, CPlayerState::EItemType type) {
    const int max = CPlayerState::GetPowerUpMaxValue(type);
    int amount = std::min(ps->GetItemAmount(type, false), max);
    // A fixed width, so the label sits next to its slider instead of at the page's right edge.
    ImGui::SetNextItemWidth(220.f);
    if (ImGui::SliderInt(label, &amount, 0, max)) {
      // The capacity stays at its maximum, as with the game's own expansions, and only the
      // amount moves: a slide down must take ammo away even though the capacity has room.
      GrantItem(*ps, type, max, amount);
    }
  };
  counter("Missiles", CPlayerState::kIT_Missile);
  counter("Power Bombs", CPlayerState::kIT_Powerbomb);
  counter("Dark Ammo", CPlayerState::kIT_DarkAmmo);
  counter("Light Ammo", CPlayerState::kIT_LightAmmo);
  // Energy tanks are the one counter that also moves the health bar.
  {
    const int max = CPlayerState::GetPowerUpMaxValue(CPlayerState::kIT_EnergyTanks);
    int tanks = std::min(ps->GetItemAmount(CPlayerState::kIT_EnergyTanks, false), max);
    ImGui::SetNextItemWidth(220.f);
    if (ImGui::SliderInt("Energy Tanks", &tanks, 0, max)) {
      GrantItem(*ps, CPlayerState::kIT_EnergyTanks, max, tanks);
      health->SetHP(ps->CalculateHealth());
    }
  }
  PortUi::Help("The items go into the save in memory: the next save on the memory card writes them, "
               "and nothing here is kept across a restart on its own.");
  ImGui::SeparatorText("Teleport");
  // The Prime 1 port's Teleport block (debug_ui/cheats.cpp): a filtered list of the current
  // world's rooms and the save's world table, both by name. The requests are the same PortWarp
  // ones the console's `warp`/`world` verbs and MP2_TELEPORT use, so nothing here walks around
  // what the game already does.
  if (sRooms.empty()) {
    ImGui::TextDisabled("(no world yet)");
  } else {
    const int areaCount = static_cast< int >(sRooms.size());
    const int current = sAreaId;
    // Between two updates the published ids are the same frame's, so this only guards a world
    // that is arriving: an area index outside the table is not looked up.
    const bool currentValid = current >= 0 && current < areaCount;
    const auto roomLabel = [](int index) { return sRooms[index].name; };
    const auto roomExists = [](int index) { return sRooms[index].real; };
    const auto goToRoom = [](int index) {
      char text[16];
      std::snprintf(text, sizeof(text), "%d", index);
      PortWarp::Area(text);
    };

    static char sRoomFilter[64] = {};
    const float goWidth = ImGui::CalcTextSize("Reload").x + ImGui::GetStyle().FramePadding.x * 2.f;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - goWidth -
                            ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::BeginCombo("##room", currentValid ? roomLabel(current).c_str() : "(no area)",
                          ImGuiComboFlags_HeightLarge)) {
#if !defined(__ANDROID__) // it would raise the on-screen keyboard every time
      if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
      }
#endif
      ImGui::SetNextItemWidth(-FLT_MIN);
      ImGui::InputTextWithHint("##roomfilter", "Filter rooms", sRoomFilter, sizeof(sRoomFilter));
      for (int i = 0; i < areaCount; ++i) {
        if (!roomExists(i)) {
          continue;
        }
        const std::string label = roomLabel(i);
        if (sRoomFilter[0] != '\0' && SDL_strcasestr(label.c_str(), sRoomFilter) == nullptr) {
          continue;
        }
        ImGui::PushID(i);
        if (ImGui::Selectable(label.c_str(), i == current)) {
          goToRoom(i);
        }
        ImGui::PopID();
      }
      ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Pick a room to go there. A room that is not loaded streams in first.");
    ImGui::SameLine();
    ImGui::BeginDisabled(!currentValid);
    if (ImGui::Button("Reload")) {
      // The area warp ignores a request for the room it is already in, so a reload is the
      // world warp the elevators use, to the same world and room.
      char worldText[16];
      char areaText[16];
      std::snprintf(worldText, sizeof(worldText), "%08x", sWorldId);
      std::snprintf(areaText, sizeof(areaText), "%d", current);
      PortWarp::World(worldText, areaText);
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Rebuild the room from the disc, as leaving and coming back does.");

    // Rebuilt when the card's table changes size, so loading another save does not leave the
    // combo showing the previous one's worlds.
    static size_t sWorldListCount = 0;
    static std::vector< std::pair< uint32_t, std::string > > sWorldList;
    const int worldCount =
        gpMemoryCard != nullptr ? static_cast< int >(gpMemoryCard->GetMemoryWorlds().size()) : 0;
    if (worldCount > 0 && static_cast< size_t >(worldCount) != sWorldListCount) {
      sWorldListCount = static_cast< size_t >(worldCount);
      sWorldList.clear();
      const rstl::vector< CMemoryCard::MemoryWorld >& worlds = gpMemoryCard->GetMemoryWorlds();
      for (int i = 0; i < worlds.size(); ++i) {
        const uint32_t id = static_cast< uint32_t >(worlds[i].first);
        std::string name = PlainWorldName(worlds[i].second.GetFrontEndName());
        if (name.empty()) {
          char buf[32];
          std::snprintf(buf, sizeof(buf), "MLVL %08X", static_cast< unsigned >(id));
          name = buf;
        }
        sWorldList.emplace_back(id, name);
      }
    }
    if (sWorldList.empty()) {
      ImGui::TextDisabled("(the memory card's world table is not loaded yet)");
    } else {
      const char* currentName = "(another world)";
      for (const std::pair< uint32_t, std::string >& entry : sWorldList) {
        if (entry.first == sWorldId) {
          currentName = entry.second.c_str();
        }
      }
      ImGui::SetNextItemWidth(-FLT_MIN);
      if (ImGui::BeginCombo("##world", currentName)) {
        for (const std::pair< uint32_t, std::string >& entry : sWorldList) {
          ImGui::PushID(static_cast< int >(entry.first));
          if (ImGui::Selectable(entry.second.c_str(), entry.first == sWorldId)) {
            char text[16];
            std::snprintf(text, sizeof(text), "%08x", entry.first);
            PortWarp::World(text, "");
          }
          ImGui::PopID();
        }
        ImGui::EndCombo();
      }
      ImGui::SetItemTooltip("Pick a world to reload into its first room.");
    }
  }

}

void DrawControlsMouse() {
  ImGui::SeparatorText("Mouse aim");
  bool mouseAim = PortMouse::Enabled();
  if (Changed(PortUi::Toggle("Mouse aim", &mouseAim, "MP2_MOUSE_AIM=1"))) {
    PortMouse::SetEnabled(mouseAim);
  }
  bool invertX = PortMouse::InvertX();
  if (Changed(ImGui::Checkbox("Invert X", &invertX))) {
    PortMouse::SetInvertX(invertX);
  }
  ImGui::SameLine();
  bool invertY = PortMouse::InvertY();
  if (Changed(ImGui::Checkbox("Invert Y", &invertY))) {
    PortMouse::SetInvertY(invertY);
  }
  // Radians per pixel is a poor thing to read; degrees per pixel is not much better, so the
  // slider shows a 1x-100x multiplier over the default instead.
  float scale = PortMouse::Sensitivity() / 0.0035f;
  char sensText[64];
  std::snprintf(sensText, sizeof(sensText), "MP2_MOUSE_SENS=%.5f (radians per pixel)",
                static_cast< double >(PortMouse::Sensitivity()));
  if (Changed(PortUi::SliderFloat("Sensitivity", &scale, 0.1f, 5.f, "%.2fx", nullptr, sensText))) {
    PortMouse::SetSensitivity(0.0035f * scale);
  }
  Changed(PortUi::Toggle("Weapon buttons", &sMouseButtons, "MP2_MOUSE_BUTTONS=0",
                         "the mouse buttons drive the pad's gameplay rows (fire, missile, jump...) "
                         "while mouse aim holds the cursor; bind them on the Remap tab"));
  ImGui::SeparatorText("Crosshair");
  Changed(PortUi::Toggle(
      "Aim crosshair", &sAimCrosshair, "MP2_AIM_CROSSHAIR=0",
      "the game's crosshair input is a look control the port never presses, so the port holds "
      "the reticle on while its own aim owns the view"));
  if (sAimCrosshair) {
    Changed(PortUi::SliderInt("Crosshair size", &sCrosshairSize, PortOverlay::kCrosshairSizeMin,
                              PortOverlay::kCrosshairSizeMax, "%d%%",
                              "MP2_CROSSHAIR_SIZE=25..100",
                              "a percentage of the reticle's own scale, while the port is aiming"));
  }
}

void DrawControlsStickAim() {
  ImGui::SeparatorText("Stick aim");
  bool twinStick = PortMouse::StickEnabled();
  if (Changed(PortUi::Toggle("Twin stick", &twinStick, "MP2_TWIN_STICK=1"))) {
    PortMouse::SetStickEnabled(twinStick);
  }
  float rate = PortMouse::StickAimRate();
  if (Changed(PortUi::SliderFloat("Stick aim rate", &rate, 200.f, 2000.f, "%.0f px/s", nullptr,
                                  "the right stick aims through the same path as the mouse"))) {
    PortMouse::SetStickAimRate(rate);
  }
  bool split = PortMouse::StickAimSplit();
  if (Changed(PortUi::Toggle("Separate vertical speed", &split, "MP2_STICK_AIM_SPLIT=1",
                             "gives looking up and down its own speed; Stick aim rate then sets only left and right"))) {
    PortMouse::SetStickAimSplit(split);
  }
  if (split) {
    float rateY = PortMouse::StickAimRateY();
    if (Changed(PortUi::SliderFloat("Vertical stick aim rate", &rateY, 200.f, 2000.f, "%.0f px/s", nullptr,
                                    "how fast the right stick turns the camera up and down at full tilt"))) {
      PortMouse::SetStickAimRateY(rateY);
    }
  }
  // The game's own option, saved with its settings (Reverse Y Axis in its pause menu).
  ImGui::BeginDisabled(gpGameState == nullptr);
  bool invertStickY = gpGameState != nullptr && gpGameState->GameOptions().GetInvertYAxis();
  if (PortUi::Toggle("Invert stick aim Y", &invertStickY, nullptr,
                     "the game's Reverse Y Axis option; the stick and free look use it") &&
      gpGameState != nullptr) {
    gpGameState->GameOptions().SetInvertYAxis(invertStickY);
  }
  ImGui::EndDisabled();
}

void DrawControlsOptions() {
  ImGui::SeparatorText("Buttons");
  Changed(PortUi::Toggle("Toggle lock-on", &sLockOnToggle, "MP2_LOCK_ON_TOGGLE=1",
               "an L press latches the lock until the next; one that ends lets go"));
  Changed(PortUi::Toggle("Sticky charge", &sStickyCharge, "MP2_STICKY_CHARGE=1",
               "letting go of a long A hold keeps charging until the next press fires it"));
  Changed(PortUi::Toggle("Remastered charge", &sRapidCharge, "MP2_RAPID_CHARGE=1",
               "holding fire after the press shot fires a few quick shots, Power Beam only"));
  ImGui::SeparatorText("Aiming");
  Changed(PortUi::Toggle("Auto-aim", &sAutoAim, "MP2_AUTO_AIM=0",
               "on, as the original: without Lock-On, the arm cannon turns toward the enemy nearest the "
               "middle of the screen and shots home in on it. Off, shots go where you aim; Lock-On still "
               "aims at its target"));
  Changed(PortUi::Toggle("Pointer aim (Trilogy style)", &sPointerAim, "MP2_POINTER_AIM=1",
               "the mouse, stick or touch moves a pointer over the screen; past the dead box the view "
               "turns toward that edge, and the arm cannon aims at the pointer. Needs mouse, twin-stick "
               "or touch aim"));
  if (sPointerAim) {
    Changed(PortUi::SliderInt("Dead box", &sPointerAimBox, PortOverlay::kPointerAimBoxMin,
                              PortOverlay::kPointerAimBoxMax, "%d%%", "MP2_POINTER_AIM_BOX=20..95",
                              "how far out, as a share of the half-screen, the pointer moves before "
                              "the view turns"));
    Changed(PortUi::SliderInt("Edge turn speed", &sPointerAimTurn, PortOverlay::kPointerAimTurnMin,
                              PortOverlay::kPointerAimTurnMax, "%d deg/s",
                              "MP2_POINTER_AIM_TURN=30..720",
                              "the view's turn speed with the pointer at the screen edge"));
    Changed(PortUi::Toggle("Gyro aim", &sGyroAim, "MP2_GYRO_AIM=1",
                 "tilt the controller (or the phone) to move the pointer. Needs a pad with a gyro"));
    if (sGyroAim) {
      Changed(PortUi::Combo("Gyro source", &sGyroSource, "Auto\0" "Controller\0" "Phone\0",
                            "MP2_GYRO_SOURCE=0..2", "auto takes the controller's gyro, else the phone's"));
      // The combo lists MP1's modes 1..4; mode 0 (off) is the Gyro aim switch above.
      int modeRow = std::clamp(sGyroMode, PortOverlay::kGyroModeMin, PortOverlay::kGyroModeMax) - 1;
      if (PortUi::Combo("Mode##gyro", &modeRow,
                        "Hold to aim\0" "Always aim\0" "Hold to pause\0" "Toggle to aim\0",
                        "MP2_GYRO_MODE=1..4",
                        "when the gyro moves the pointer. The hold and toggle modes use the Gyro aim "
                        "button (R3 or left Ctrl by default, Controls > Remap)")) {
        sGyroMode = std::clamp(modeRow + 1, PortOverlay::kGyroModeMin, PortOverlay::kGyroModeMax);
        Changed(true);
      }
      Changed(PortUi::Toggle("Gyro also turns the camera", &sGyroCursorCamera,
                             "MP2_GYRO_CURSOR_CAMERA=1",
                             "turns the camera with the gyro (at the Sensitivity below) while it also "
                             "moves the pointer, so the view follows the pad and the pointer "
                             "fine-tunes the aim"));
      ImGui::BeginDisabled(!sGyroCursorCamera);
      if (PortUi::SliderFloat("Sensitivity##gyro", &sGyroSensitivity, PortOverlay::kGyroSensitivityMin,
                              PortOverlay::kGyroSensitivityMax, "%.2fx", "MP2_GYRO_SENSITIVITY=0.1..10",
                              "how far the camera turns per turn of the pad: at 1.00x turning the pad "
                              "90 degrees turns the camera 90 degrees. Applies only if the gyro also "
                              "turns the camera")) {
        sGyroSensitivity = std::clamp(sGyroSensitivity, PortOverlay::kGyroSensitivityMin,
                                      PortOverlay::kGyroSensitivityMax);
        Changed(true);
      }
      ImGui::EndDisabled();
      if (PortUi::SliderFloat("Pointer speed##gyro", &sGyroCursorAngle, PortOverlay::kGyroCursorAngleMin,
                              PortOverlay::kGyroCursorAngleMax, "%.0f deg across the screen",
                              "MP2_GYRO_CURSOR_ANGLE=5..180",
                              "how far the pad turns to sweep the pointer from one side of the screen "
                              "to the other. Smaller is faster. Independent of the mouse sensitivity")) {
        sGyroCursorAngle = std::clamp(sGyroCursorAngle, PortOverlay::kGyroCursorAngleMin,
                                      PortOverlay::kGyroCursorAngleMax);
        Changed(true);
      }
      // Listed by how common they are (MP1's order); the setting keeps PortGyro::Space's numbers.
      static constexpr int kSpaceOrder[] = {PortGyro::kPlayer, PortGyro::kWorld, PortGyro::kLocal,
                                            PortGyro::kLocalRoll, PortGyro::kLocalBoth};
      int spaceRow = 0;
      for (int i = 0; i < PortGyro::kSpaceCount; ++i) {
        if (kSpaceOrder[i] == sGyroSpace) {
          spaceRow = i;
        }
      }
      if (PortUi::Combo("Gyro space", &spaceRow,
                        "Player (turn)\0" "World (turn)\0" "Local yaw\0" "Local roll\0"
                        "Local yaw and roll\0",
                        "MP2_GYRO_SPACE=0..4",
                        "which turn of the pad moves the pointer left and right. Player: turning the "
                        "pad left and right, held flat or upright. World: about the real vertical "
                        "only. Local yaw: the pad's own flat turn. Local roll: a pad held upright. "
                        "Player and world need an accelerometer; without one they act as local yaw")) {
        sGyroSpace = kSpaceOrder[std::clamp(spaceRow, 0, PortGyro::kSpaceCount - 1)];
        Changed(true);
      }
      Changed(PortUi::Combo("Auto calibrate", &sGyroAutoCalibrate, "Off\0" "Always\0" "Only in menus\0",
                            "MP2_GYRO_AUTO_CALIBRATE=0..2",
                            "measures the gyro's drift while the pad is still and removes it. Needs "
                            "an accelerometer. Only in menus learns with F1 open or outside free aim, "
                            "and keeps what it learned while you play"));
      if (PortUi::SliderFloat("Smoothing##gyro", &sGyroSmoothing, 0.f, PortOverlay::kGyroThresholdMax,
                              sGyroSmoothing > 0.f ? "below %.0f deg/s" : "off",
                              "MP2_GYRO_SMOOTHING=0..50",
                              "averages slow turns over a few frames to hide sensor jitter; turns "
                              "above twice this speed stay direct")) {
        sGyroSmoothing = std::clamp(sGyroSmoothing, 0.f, PortOverlay::kGyroThresholdMax);
        Changed(true);
      }
      if (PortUi::SliderFloat("Tightening##gyro", &sGyroTightening, 0.f, PortOverlay::kGyroThresholdMax,
                              sGyroTightening > 0.f ? "below %.0f deg/s" : "off",
                              "MP2_GYRO_TIGHTENING=0..50",
                              "turns slower than this are scaled down, so a pad held still does not "
                              "wobble the pointer. Keep it low")) {
        sGyroTightening = std::clamp(sGyroTightening, 0.f, PortOverlay::kGyroThresholdMax);
        Changed(true);
      }
      const float countdown = PortInputDevices::GyroRecalibrationCountdown();
      if (countdown >= 0.f) {
        ImGui::BeginDisabled();
        char label[48];
        std::snprintf(label, sizeof(label), "Calibrating in %d...", int(countdown) + 1);
        ImGui::Button(label);
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel##gyrorecal")) {
          PortInputDevices::CancelGyroRecalibration();
        }
      } else if (ImGui::Button("Recalibrate##gyro")) {
        PortInputDevices::StartGyroRecalibration();
      }
      PortUi::Note(nullptr,
                   "waits 5 seconds so you can put the pad down, then clears the gyro's drift "
                   "measurement and learns it again",
                   true);
      ImGui::Text("Gyro: %s", PortInputDevices::GyroStatus());
      const float confidence = PortInputDevices::GyroCalibrationConfidence();
      if (confidence >= 0.f) {
        ImGui::Text("Calibration: %d%%%s", int(confidence * 100.f + 0.5f),
                    PortInputDevices::GyroCalibrating() ? " (calibrating)"
                    : confidence >= 0.99f              ? " (done)"
                                                       : "");
      } else {
        ImGui::TextDisabled("Calibration: no accelerometer");
      }
    }
  }
  ImGui::SeparatorText("Rumble");
  ImGui::BeginDisabled(gpGameState == nullptr);
  bool rumble = gpGameState != nullptr && gpGameState->GameOptions().GetIsRumbleEnabled();
  if (PortUi::Toggle("Rumble", &rumble, nullptr,
                     "controller vibration, the same switch as Rumble in the pause menu's options") &&
      gpGameState != nullptr) {
    gpGameState->GameOptions().SetIsRumbleEnabled(rumble);
  }
  ImGui::EndDisabled();
  ImGui::SeparatorText("Morph ball");
  Changed(PortUi::Toggle("Fast morph", &sFastMorph, "MP2_FAST_MORPH=1",
               "the morph and unmorph transitions take 0.2 s instead of the game's second"));
  Changed(PortUi::Toggle("Spring Ball", &sSpringBall, "MP2_SPRING_BALL=1",
               "C-stick up on the ground bomb-jumps the ball (needs Morph Ball Bombs)"));
}

void DrawControlsTouchGyro() {
  ImGui::SeparatorText("Touch controls");
  bool controls = PortTouch::ControlsEnabled();
  if (PortUi::Toggle("On-screen controls", &controls, "MP2_TOUCH_CONTROLS=0",
                     "the phone's touch overlay; touch.cfg in the user folder")) {
    PortTouch::SetControlsEnabled(controls);
  }
  bool aim = PortTouch::AimEnabled();
  if (PortUi::Toggle("Touch aim", &aim, "MP2_TOUCH_AIM=1",
                     "drag on the free screen to aim")) {
    PortTouch::SetAimEnabled(aim);
  }
  const bool classic = PortTouch::Classic();
  const bool twin = PortTouch::TwinStick();
  int layoutIndex = classic ? 1 : (twin ? 2 : 0);
  if (PortUi::Combo("Layout", &layoutIndex, "Default\0" "Classic GameCube\0" "Twin stick (Remastered)\0",
                    "MP2_TOUCH_CLASSIC=1|MP2_TOUCH_TWIN=1",
                    "Default: free-area drag aims, wheels pick beams and visors. Classic: the "
                    "C-stick and D-pad. Twin stick: a right stick that aims")) {
    // CycleLayout steps Default -> Classic -> Twin -> Default.
    for (int i = 0; i < 3 && (PortTouch::Classic() ? 1 : (PortTouch::TwinStick() ? 2 : 0)) != layoutIndex; ++i) {
      PortTouch::CycleLayout();
    }
    if (layoutIndex == 2) PortTouch::SetWheelsEnabled(false);
  }
  ImGui::SameLine();
  if (ImGui::Button("Edit layout")) {
    PortTouch::RequestEdit();
  }
  ImGui::BeginDisabled(!controls);
  bool b = PortTouch::Colors();
  if (PortUi::Toggle("Coloured buttons", &b, "MP2_TOUCH_COLORS=1",
                     "the on-screen buttons in the GameCube pad's colours")) PortTouch::SetColors(b);
  b = PortTouch::Labels();
  if (PortUi::Toggle("Button descriptions", &b, "MP2_TOUCH_LABELS=0",
                     "what each on-screen button does, next to its letter")) PortTouch::SetLabels(b);
  b = PortTouch::Turbo();
  if (PortUi::Toggle("Turbo fire button", &b, "MP2_TOUCH_TURBO=1",
                     "a Turbo button next to Fire that fires as fast as the game accepts")) PortTouch::SetTurbo(b);
  b = PortTouch::FloatingStick();
  if (PortUi::Toggle("Floating left stick", &b, "MP2_TOUCH_FLOATING=0",
                     "the left stick appears under the finger on the left half of the screen")) PortTouch::SetFloatingStick(b);
  ImGui::BeginDisabled(!classic && !twin);
  b = PortTouch::WheelsEnabled();
  if (PortUi::Toggle("Beam and visor wheels", &b, "MP2_TOUCH_WHEELS=0",
                     "replaces the D-pad with a Visor and a Beam button")) PortTouch::SetWheelsEnabled(b);
  ImGui::EndDisabled();
  ImGui::BeginDisabled(!((!classic && !twin) || PortTouch::WheelsEnabled()));
  b = PortTouch::VisorTapScan();
  if (PortUi::Toggle("Tap Visor for Scan Visor", &b, "MP2_TOUCH_VISOR_TAP_SCAN=1",
                     "a quick tap on the Visor button selects the Scan Visor")) PortTouch::SetVisorTapScan(b);
  ImGui::EndDisabled();
  float speed = PortTouch::AimSpeed();
  if (PortUi::SliderFloat("Touch aim speed", &speed, 0.5f, 6.f, "%.2f px/dp", nullptr,
                          "MP2_TOUCH_AIM_SPEED=0.25..10")) PortTouch::SetAimSpeed(speed);
  b = PortTouch::SpringSwipeEnabled();
  if (PortUi::Toggle("Spring Ball on swipe up", &b, "MP2_TOUCH_SPRING_SWIPE=0",
                     "in morph ball, a quick swipe up on the free area springs; needs the Spring Ball option")) PortTouch::SetSpringSwipe(b);
  float v = PortTouch::Opacity();
  if (PortUi::SliderFloat("Opacity", &v, 10.f, 100.f, "%.0f%%", "MP2_TOUCH_OPACITY=10..100")) PortTouch::SetOpacity(v);
  v = PortTouch::SideMarginDp();
  if (PortUi::SliderFloat("Side margin", &v, 0.f, 300.f, "%.0f dp", "MP2_TOUCH_SIDE_MARGIN")) PortTouch::SetSideMarginDp(v);
  v = PortTouch::StickInsetDp();
  if (PortUi::SliderFloat("Stick inset", &v, 0.f, 300.f, "%.0f dp", "MP2_TOUCH_STICK_INSET")) PortTouch::SetStickInsetDp(v);
  v = PortTouch::ButtonInsetDp();
  if (PortUi::SliderFloat("Button inset", &v, 0.f, 300.f, "%.0f dp", "MP2_TOUCH_BUTTON_INSET")) PortTouch::SetButtonInsetDp(v);
  if (ImGui::Button("Reset margins")) {
    PortTouch::SetSideMarginDp(16.f);
    PortTouch::SetStickInsetDp(32.f);
    PortTouch::SetButtonInsetDp(0.f);
  }
  ImGui::EndDisabled();
  ImGui::SeparatorText("Gyro aim");
  ImGui::BeginDisabled();
  ImGui::TextUnformatted("Gyro aim (not in this port yet)");
  ImGui::EndDisabled();
}

// The Prime 1 port's Controls page (debug_ui/tabs.cpp's DrawControlsTab) with its five sub-tabs in
// its order. Each draws this port's rows for what MP1 files there; the rows MP2 has no backend
// for (button glyphs, pointer aim, rumble, mouse buttons, gyro) are not drawn, gyro as a note.
void DrawControlsSection() {
  if (ImGui::BeginTabBar("controlsPages")) {
    if (PortOverlaySearch::SubTab("Controls", "Options")) {
      DrawControlsOptions();
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Controls", "Mouse settings")) {
      DrawControlsMouse();
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Controls", "Controller settings")) {
      DrawControlsStickAim();
      const s32 controllerIndex = PADGetIndexForPort(PAD_CHAN0);
      if (SDL_Gamepad* pad =
              controllerIndex >= 0 ? PADGetSDLGamepadForIndex(static_cast< u32 >(controllerIndex)) : nullptr;
          pad != nullptr && SDL_IsJoystickVirtual(SDL_GetGamepadID(pad))) {
        // Aurora ignores mappings on the virtual touch pad.
        ImGui::TextWrapped("The touch controls always use the GameCube layout; presets and remaps apply to "
                           "real controllers only.");
      } else {
        PortControls::DrawDeadZones();
      }
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Controls", "Touch & gyro")) {
      DrawControlsTouchGyro();
      ImGui::EndTabItem();
    }
    if (PortOverlaySearch::SubTab("Controls", "Remap")) {
      PortInputRemap::Draw();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  // MP1's HotkeyHint (debug_ui/overlays.cpp): the port's own hotkeys, named by what is bound.
  const std::string menu = PortInputRemap::BindingsText(PortInput::Action::PortMenu, " / ");
  const std::string shot = PortInputRemap::BindingsText(PortInput::Action::Screenshot, " / ");
  ImGui::TextDisabled("%s: this window   %s: screenshot   F5/F9: save/load state   F8: photo mode",
                      menu.empty() ? "unbound" : menu.c_str(), shot.empty() ? "unbound" : shot.c_str());
}

// The phone's window, as the Prime 1 port draws it (debug_ui/window.cpp's DrawPageWindow): full
// screen inside the display's safe area, no title bar, a Close button a thumb can hit, the settings
// search, and a page list down the side in place of a tab strip too narrow to tap. Returns false
// once Close is pressed.
[[maybe_unused]] bool DrawPageWindow() {
  using namespace PortOverlaySearch;
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImVec2 pos = viewport->WorkPos;
  ImVec2 size = viewport->WorkSize;
  SDL_Window* window = PortUi::MainWindow();
  SDL_Rect safe;
  int windowWidth = 0, windowHeight = 0;
  if (window != nullptr && SDL_GetWindowSafeArea(window, &safe) &&
      SDL_GetWindowSize(window, &windowWidth, &windowHeight) && windowWidth > 0 &&
      windowHeight > 0 && safe.w > 0 && safe.h > 0) {
    // ImGui's display size need not be in window coordinates.
    const float sx = size.x / static_cast< float >(windowWidth);
    const float sy = size.y / static_cast< float >(windowHeight);
    pos = ImVec2(pos.x + safe.x * sx, pos.y + safe.y * sy);
    size = ImVec2(safe.w * sx, safe.h * sy);
  }
  ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(size, ImGuiCond_Always);
  constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                      ImGuiWindowFlags_NoSavedSettings |
                                      ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoScrollWithMouse;
  bool open = true;
  if (ImGui::Begin("Metroid Prime 2 port##touch", nullptr, kFlags)) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const char* const kClose = "Close";
    const float closeWidth = ImGui::CalcTextSize(kClose).x + style.FramePadding.x * 4.f;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Metroid Prime 2 Port");
    ImGui::SameLine(0.f, style.ItemSpacing.x * 2.f);
    ImGui::TextDisabled("%.1f fps", static_cast< double >(sFps));
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - closeWidth);
    if (ImGui::Button(kClose, ImVec2(closeWidth, 0.f))) {
      open = false;
    }
    ImGui::Separator();
    DrawSearchField();

    static int sPage = 0;
    if (const int jumpPage = TakeJumpPage(); jumpPage >= 0) {
      sPage = jumpPage;
    }
    float listWidth = 0.f;
    for (int i = 0; i < kPageCount; ++i) {
      listWidth = std::max(listWidth, ImGui::CalcTextSize(kPages[i].name).x);
    }
    listWidth += style.FramePadding.x * 2.f + style.WindowPadding.x * 2.f;
    const float rowHeight = ImGui::GetFrameHeight() * 1.2f;
    if (ImGui::BeginChild("##pages", ImVec2(listWidth, 0.f), ImGuiChildFlags_Borders)) {
      for (int i = 0; i < kPageCount; ++i) {
        if (ImGui::Selectable(kPages[i].name, sPage == i && !Searching(), ImGuiSelectableFlags_None,
                              ImVec2(0.f, rowHeight))) {
          sPage = i;
          sSearch[0] = '\0';
        }
      }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    // Keyed by page, so each page keeps its own scroll position.
    const bool searching = Searching();
    ImGui::PushID(searching ? -1 : sPage);
    if (ImGui::BeginChild("##page", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders)) {
      // A drag that no item took scrolls the page, like a wheel (a finger has no wheel).
      if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
          !ImGui::IsAnyItemActive()) {
        const float dy = ImGui::GetIO().MouseDelta.y;
        if (dy > 0.f || dy < 0.f) {
          ImGui::SetScrollY(ImGui::GetScrollY() - dy);
        }
      }
      if (searching) {
        DrawSearchResults();
      } else {
        DrawPageBody(kPages[sPage]);
      }
    }
    ImGui::EndChild();
    ImGui::PopID();
  }
  ImGui::End();
  return open;
}

// The desktop's window, as the Prime 1 port draws it (debug_ui/window.cpp's DrawDesktopWindow): the
// settings search over a tab per page, its results in place of the tabs while there is a query.
[[maybe_unused]] void DrawDesktopWindow() {
  using namespace PortOverlaySearch;
  // Not movable: on the phone a drag on the window's empty space is how the list is scrolled (a
  // finger cannot turn a mouse wheel), and a window-drag would move it instead.
  if (ImGui::Begin("Metroid Prime 2 port", &sVisible, ImGuiWindowFlags_NoMove)) {
    ImGui::Text("%.1f fps", static_cast< double >(sFps));
    DrawSearchField();
    // A drag on the window that no item took: scroll by the drag, like a wheel would. An item
    // that holds the mouse (a slider, a checkbox press) keeps its own drag.
    if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        !ImGui::IsAnyItemActive()) {
      const float dy = ImGui::GetIO().MouseDelta.y;
      if (dy > 0.f || dy < 0.f) {
        ImGui::SetScrollY(ImGui::GetScrollY() - dy);
      }
    }
    if (Searching()) {
      if (ImGui::BeginChild("##search-results")) {
        DrawSearchResults();
      }
      ImGui::EndChild();
    } else {
      if (ImGui::BeginTabBar("##overlay_tabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
        const int jumpPage = TakeJumpPage();
        for (int i = 0; i < kPageCount; ++i) {
          if (ImGui::BeginTabItem(kPages[i].name, nullptr,
                                  i == jumpPage ? ImGuiTabItemFlags_SetSelected : 0)) {
            DrawPageBody(kPages[i]);
            ImGui::EndTabItem();
          }
        }
        ImGui::EndTabBar();
      }
      ImGui::SeparatorText("Hotkeys");
      ImGui::TextDisabled("F1 overlay, F5/F9 save/load state, F8 photo mode");
    }
  }
  ImGui::End();
}

} // namespace

namespace PortOverlaySearch {

const Page kPages[] = {
    {"Game", DrawGameSection},           {"Controls", DrawControlsSection},
    {"Video", DrawVideoSection},         {"Cheats", DrawCheatsSection},
    {"Save states", DrawSaveStatesSection}, {"Randomizer", DrawRandomizerSection},
    {"System", DrawSystemSection},       {"Debug", DrawDebugSection},
};
const int kPageCount = static_cast< int >(std::size(kPages));

} // namespace PortOverlaySearch

namespace PortOverlay {

void SetGameState(uint32_t worldId, int areaId, CWorld* world) {
  sWorldId = worldId;
  sAreaId = areaId;
  if (world == nullptr) {
    sRooms.clear();
    sRoomsWorld = 0;
    return;
  }
  // The world object's own id, not the game state's: on the frame a world teleport is served the
  // outgoing world is still the live one while the game state already names the destination, and
  // keying on the state's id would cache the outgoing rooms under the new world's name.
  const uint32_t roomWorld = static_cast< uint32_t >(world->IGetWorldAssetId());
  if (roomWorld == sRoomsWorld && !sRooms.empty()) {
    return; // the same world: its area table cannot have changed under it
  }
  sRoomsWorld = roomWorld;
  sRooms.clear();
  const int count = world->IGetAreaCount();
  sRooms.reserve(count);
  for (int i = 0; i < count; ++i) {
    const IGameArea* area = world->IGetAreaAlways(TAreaId(i));
    const char* name = area != nullptr ? area->PortGetMlvlAreaName().data() : nullptr;
    char label[160];
    if (name != nullptr && name[0] != '\0') {
      std::snprintf(label, sizeof(label), "%d  %s", i, name);
    } else {
      std::snprintf(label, sizeof(label), "Area %d", i);
    }
    // A slot with no MREA is not a room the game can travel to; the list hides it instead of
    // offering a warp that would print "does not exist" and do nothing.
    sRooms.push_back(SRoomEntry{label, area != nullptr && area->IGetAreaAssetId() != kInvalidAssetId});
  }
}

void Toggle() {
  sVisible = !sVisible;
  // MP1's Toggle: the cursor goes back to the window (releases the mouse capture), and opening
  // drops any button or key ImGui still holds (a press whose release never came would make it
  // ignore every later click), as a focus loss would.
  PortMouse::SetUiActive(sVisible);
  if (sVisible && ImGui::GetCurrentContext() != nullptr) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(false);
    io.AddFocusEvent(true);
  }
}

// Called on the UI thread (the two-finger scroll), but ImGui's input queue is only safe on the game
// thread, so the steps wait here until Draw() hands them over.
static std::atomic< int > sPendingWheelMilli{0};

void Wheel(float steps) {
  sPendingWheelMilli.fetch_add(static_cast< int >(std::lround(steps * 1000.f)),
                               std::memory_order_relaxed);
}

bool Visible() {
  ReadEnv();
  return sVisible;
}

bool SkipCinematics() {
  ReadEnv();
  return EnvBool("MP2_SKIP_CINEMATICS", sSkipCinematics);
}

void SetSkipCinematics(bool skip) {
  ReadEnv();
  sSkipCinematics = skip;
  Persist();
}

bool HideHelmet() {
  ReadEnv();
  return EnvBool("MP2_NO_HELMET", sHideHelmet);
}

bool HideVisorEffects() {
  ReadEnv();
  return EnvBool("MP2_NO_VISOR_FX", sHideVisorEffects);
}

bool AimCrosshair() {
  ReadEnv();
  return EnvBool("MP2_AIM_CROSSHAIR", sAimCrosshair);
}

void SetAimCrosshair(bool enabled) {
  ReadEnv();
  sAimCrosshair = enabled;
  Persist();
}

int CrosshairStyle() {
  ReadEnv();
  const char* env = port::EnvString("MP2_CROSSHAIR_STYLE");
  if (env != nullptr && std::strcmp(env, "classic") == 0) {
    return kCrosshairStyle_Classic;
  }
  if (env != nullptr && std::strcmp(env, "remastered") == 0) {
    return kCrosshairStyle_Remastered;
  }
  return EnvIntIn("MP2_CROSSHAIR_STYLE", sCrosshairStyle, kCrosshairStyle_Classic, kCrosshairStyle_Remastered);
}

void SetCrosshairStyle(int style) {
  ReadEnv();
  sCrosshairStyle = std::clamp(style, kCrosshairStyle_Classic, kCrosshairStyle_Remastered);
  Persist();
}

int CrosshairSize() {
  ReadEnv();
  return EnvIntIn("MP2_CROSSHAIR_SIZE", sCrosshairSize, kCrosshairSizeMin, kCrosshairSizeMax);
}

void SetCrosshairSize(int percent) {
  ReadEnv();
  sCrosshairSize = std::clamp(percent, kCrosshairSizeMin, kCrosshairSizeMax);
  Persist();
}

int CrosshairOpacity() {
  ReadEnv();
  return EnvIntIn("MP2_CROSSHAIR_OPACITY", sCrosshairOpacity, kCrosshairOpacityMin, kCrosshairOpacityMax);
}

void SetCrosshairOpacity(int percent) {
  ReadEnv();
  sCrosshairOpacity = std::clamp(percent, kCrosshairOpacityMin, kCrosshairOpacityMax);
  Persist();
}

bool CrosshairColor(float rgb[3]) {
  ReadEnv();
  if (!EnvBool("MP2_CROSSHAIR_CUSTOM_COLOR", sCrosshairCustomColor)) {
    return false;
  }
  if (!EnvFloat3("MP2_CROSSHAIR_COLOR", rgb, 0.f, 1.f)) {
    std::copy_n(sCrosshairColor, 3, rgb);
  }
  return true;
}

void SetCrosshairColor(float r, float g, float b) {
  ReadEnv();
  sCrosshairColor[0] = std::clamp(r, 0.f, 1.f);
  sCrosshairColor[1] = std::clamp(g, 0.f, 1.f);
  sCrosshairColor[2] = std::clamp(b, 0.f, 1.f);
  sCrosshairCustomColor = true;
  Persist();
}

void ClearCrosshairColor() {
  ReadEnv();
  sCrosshairCustomColor = false;
  Persist();
}

float FirstPersonFov() {
  ReadEnv();
  const float env = port::EnvFloat("MP2_FOV", sFirstPersonFov);
  return env == 0.f || (env >= 30.f && env <= 120.f) ? env : sFirstPersonFov;
}

void GunOffset(float out[3]) {
  ReadEnv();
  if (EnvFloat3("MP2_GUN_OFFSET", out, -kGunOffsetMax, kGunOffsetMax)) {
    return;
  }
  out[0] = sGunOffset[0];
  out[1] = sGunOffset[1];
  out[2] = sGunOffset[2];
}

void SetGunOffset(const float offset[3]) {
  ReadEnv();
  StoreGunOffset(offset);
  Persist();
}

int Msaa() {
  ReadEnv();
  const int env = port::EnvInt("MP2_MSAA", sMsaa);
  return env == 1 || env == 4 ? env : sMsaa;
}

void SetMsaa(int samples) {
  ReadEnv();
  sMsaa = samples >= 4 ? 4 : 1;
  Persist();
}

int Anisotropy() {
  ReadEnv();
  return EnvIntIn("MP2_ANISO", sAnisotropy, 1, 16);
}

void SetAnisotropy(int maxAnisotropy) {
  ReadEnv();
  sAnisotropy = std::clamp(maxAnisotropy, 1, 16);
  Persist();
}

int HudScale() {
  ReadEnv();
  // 0 means the full-size HUD.
  const int env = port::EnvInt("MP2_HUD_SCALE", sHudScale);
  return env == 0 ? kHudScaleMax : (env >= kHudScaleMin && env <= kHudScaleMax ? env : sHudScale);
}

void SetHudScale(int percent) {
  ReadEnv();
  sHudScale = std::clamp(percent, kHudScaleMin, kHudScaleMax);
  Persist();
}

bool HudScalingActive() { return sHudScaling; }

void SetHudScalingActive(bool active) { sHudScaling = active; }

bool GodMode() {
  ReadEnv();
  return sGodMode;
}

void SetGodMode(bool enabled) {
  ReadEnv();
  sGodMode = enabled;
}

bool FastMorph() {
  ReadEnv();
  return EnvBool("MP2_FAST_MORPH", sFastMorph);
}

void SetFastMorph(bool enabled) {
  ReadEnv();
  sFastMorph = enabled;
  Persist();
}

bool SpringBall() {
  ReadEnv();
  return EnvBool("MP2_SPRING_BALL", sSpringBall);
}

void SetSpringBall(bool enabled) {
  ReadEnv();
  sSpringBall = enabled;
  Persist();
}

bool LockOnToggle() {
  ReadEnv();
  return EnvBool("MP2_LOCK_ON_TOGGLE", EnvBool("MP2_LOCKON_TOGGLE", sLockOnToggle));
}

void SetLockOnToggle(bool enabled) {
  ReadEnv();
  sLockOnToggle = enabled;
  Persist();
}

bool StickyCharge() {
  ReadEnv();
  return EnvBool("MP2_STICKY_CHARGE", sStickyCharge);
}

void SetStickyCharge(bool enabled) {
  ReadEnv();
  sStickyCharge = enabled;
  Persist();
}

bool AutoAim() {
  ReadEnv();
  return EnvBool("MP2_AUTO_AIM", sAutoAim);
}

void SetAutoAim(bool enabled) {
  ReadEnv();
  sAutoAim = enabled;
  Persist();
}

bool PointerAim() {
  ReadEnv();
  return EnvBool("MP2_POINTER_AIM", sPointerAim);
}

void SetPointerAim(bool enabled) {
  ReadEnv();
  sPointerAim = enabled;
  Persist();
}

int PointerAimBox() {
  ReadEnv();
  return EnvIntIn("MP2_POINTER_AIM_BOX", sPointerAimBox, kPointerAimBoxMin, kPointerAimBoxMax);
}

int PointerAimTurn() {
  ReadEnv();
  return EnvIntIn("MP2_POINTER_AIM_TURN", sPointerAimTurn, kPointerAimTurnMin, kPointerAimTurnMax);
}

bool GyroAim() {
  ReadEnv();
  return EnvBool("MP2_GYRO_AIM", sGyroAim);
}

void SetGyroAim(bool enabled) {
  ReadEnv();
  sGyroAim = enabled;
  Persist();
}

int GyroSource() {
  ReadEnv();
  return EnvIntIn("MP2_GYRO_SOURCE", sGyroSource, 0, 2);
}

int GyroSpace() {
  ReadEnv();
  return EnvIntIn("MP2_GYRO_SPACE", sGyroSpace, 0, PortGyro::kSpaceCount - 1);
}

int GyroAutoCalibrate() {
  ReadEnv();
  return EnvIntIn("MP2_GYRO_AUTO_CALIBRATE", sGyroAutoCalibrate, 0, 2);
}

float GyroSmoothing() {
  ReadEnv();
  return EnvFloatIn("MP2_GYRO_SMOOTHING", sGyroSmoothing, 0.f, kGyroThresholdMax);
}

float GyroTightening() {
  ReadEnv();
  return EnvFloatIn("MP2_GYRO_TIGHTENING", sGyroTightening, 0.f, kGyroThresholdMax);
}

float GyroCursorAngle() {
  ReadEnv();
  return EnvFloatIn("MP2_GYRO_CURSOR_ANGLE", sGyroCursorAngle, kGyroCursorAngleMin, kGyroCursorAngleMax);
}

int GyroMode() {
  ReadEnv();
  return EnvIntIn("MP2_GYRO_MODE", sGyroMode, kGyroModeMin, kGyroModeMax);
}

float GyroSensitivity() {
  ReadEnv();
  return EnvFloatIn("MP2_GYRO_SENSITIVITY", sGyroSensitivity, kGyroSensitivityMin, kGyroSensitivityMax);
}

bool GyroCursorCamera() {
  ReadEnv();
  return EnvBool("MP2_GYRO_CURSOR_CAMERA", sGyroCursorCamera);
}

bool RapidCharge() {
  ReadEnv();
  return EnvBool("MP2_RAPID_CHARGE", sRapidCharge);
}

void SetRapidCharge(bool enabled) {
  ReadEnv();
  sRapidCharge = enabled;
  Persist();
}

bool MouseButtons() {
  ReadEnv();
  return EnvBool("MP2_MOUSE_BUTTONS", sMouseButtons);
}

bool CardBackup() {
  ReadEnv();
  return EnvBool("MP2_CARD_BACKUP", sCardBackup);
}

void SetCardBackup(bool enabled) {
  ReadEnv();
  sCardBackup = enabled;
  Persist();
}

bool LogFile() {
  ReadEnv();
  return EnvBool("MP2_LOG_FILE", sLogFile);
}

void SetLogFile(bool enabled) {
  ReadEnv();
  sLogFile = enabled;
  Persist();
}

int UiScale() {
  ReadEnv();
  return EnvIntIn("MP2_UI_SCALE", sUiScale, kUiScaleMin, kUiScaleMax);
}

void SetUiScale(int percent) {
  ReadEnv();
  sUiScale = std::clamp(percent, kUiScaleMin, kUiScaleMax);
  Persist();
}

bool Vsync() {
  ReadEnv();
  return EnvBool("MP2_VSYNC", sVsync);
}

void SetVsync(bool enabled) {
  ReadEnv();
  sVsync = enabled;
  Persist();
}

bool FrameLimit() {
  ReadEnv();
  return EnvBool("MP2_FRAME_LIMIT", sFrameLimit);
}

void SetFrameLimit(bool enabled) {
  ReadEnv();
  if (sFrameLimit != enabled) {
    sFrameLimit = enabled;
    Persist();
  }
}

bool SmoothFrames() {
  ReadEnv();
  return EnvBool("MP2_SMOOTH_FRAMES", sSmoothFrames);
}

void SetSmoothFrames(bool enabled) {
  ReadEnv();
  if (sSmoothFrames != enabled) {
    sSmoothFrames = enabled;
    Persist();
  }
}

bool PoseInterpolation() {
  ReadEnv();
  return EnvBool("MP2_POSE_INTERPOLATION", sPoseInterpolation);
}

void SetPoseInterpolation(bool enabled) {
  ReadEnv();
  if (sPoseInterpolation != enabled) {
    sPoseInterpolation = enabled;
    Persist();
  }
}

bool ParticleInterpolation() {
  ReadEnv();
  return EnvBool("MP2_PARTICLE_INTERPOLATION", sParticleInterpolation);
}

void SetParticleInterpolation(bool enabled) {
  ReadEnv();
  if (sParticleInterpolation != enabled) {
    sParticleInterpolation = enabled;
    Persist();
  }
}

bool HudWide() {
  ReadEnv();
  return EnvBool("MP2_HUD_WIDE", sHudWide);
}

void SetHudWide(bool enabled) {
  ReadEnv();
  if (sHudWide != enabled) {
    sHudWide = enabled;
    Persist();
  }
}

bool LookInterpolation() {
  ReadEnv();
  return EnvBool("MP2_LOOK_INTERPOLATION", sLookInterpolation);
}

void SetLookInterpolation(bool enabled) {
  ReadEnv();
  if (sLookInterpolation != enabled) {
    sLookInterpolation = enabled;
    Persist();
  }
}

float RenderScale() {
  ReadEnv();
  const float env = EnvFloatIn("MP2_RENDER_SCALE", sRenderScale, 0.f, kRenderScaleMax);
  return env > 0.f && env < kRenderScaleMin ? kRenderScaleMin : env;
}

void SetRenderScale(float scale) {
  ReadEnv();
  if (std::isfinite(scale)) {
    sRenderScale = scale <= 0.f ? 0.f : std::clamp(scale, kRenderScaleMin, kRenderScaleMax);
    Persist();
  }
}

const char* AspectName(int mode) {
  return mode == kAspect_4_3 ? "4:3" : mode == kAspect_16_9 ? "16:9" : "window";
}

bool ParseAspect(const char* name, int& mode) {
  if (name == nullptr) {
    return false;
  }
  if (std::strcmp(name, "4:3") == 0 || std::strcmp(name, "4x3") == 0) {
    mode = kAspect_4_3;
  } else if (std::strcmp(name, "16:9") == 0 || std::strcmp(name, "16x9") == 0) {
    mode = kAspect_16_9;
  } else if (std::strcmp(name, "window") == 0) {
    mode = kAspect_Window;
  } else {
    return false;
  }
  return true;
}

int AspectMode() {
  ReadEnv();
  int mode = sAspectMode;
  if (ParseAspect(port::EnvString("MP2_ASPECT"), mode)) {
    return mode;
  }
  if (port::EnvFlag("MP2_WIDESCREEN")) {
    return kAspect_16_9;
  }
  return mode;
}

void SetAspectMode(int mode) {
  ReadEnv();
  sAspectMode = std::clamp(mode, kAspect_4_3, kAspect_Window);
  Persist();
}

// The Prime 1 port's ApplyAspectMode (src/MetroidPrime/main.cpp there), with one difference: 4:3
// is the game's own width (640 at 448 lines, the 1.42 CCameraManager was built for), not
// efbHeight * 4 / 3.
void ApplyAspect() {
  const GXRenderModeObj& renderMode = CGraphics::GetRenderMode();
  const int efbHeight = renderMode.efbHeight;
  if (efbHeight <= 0) {
    return; // CGraphics::ConfigureVideo has not run yet
  }
  if (sBaseFbWidth == 0) {
    sBaseFbWidth = renderMode.fbWidth;
  }
  int fbWidth = sBaseFbWidth;
  switch (AspectMode()) {
  case kAspect_16_9:
    fbWidth = (efbHeight * 16 + 8) / 9;
    break;
  case kAspect_Window: {
    // Ask the window rather than track resize events, as the Prime 1 port does: the loading loops
    // that run before the main loop drain the startup resize.
    int windowWidth = 0;
    int windowHeight = 0;
    SDL_Window* window = PortUi::MainWindow();
    if (window == nullptr || !SDL_GetWindowSize(window, &windowWidth, &windowHeight) ||
        windowWidth <= 0 || windowHeight <= 0) {
      windowWidth = 854;
      windowHeight = 480;
    }
    fbWidth = static_cast< int >(static_cast< double >(efbHeight) *
                                     static_cast< double >(windowWidth) /
                                     static_cast< double >(windowHeight) +
                                 0.5);
    break;
  }
  default:
    break;
  }
  fbWidth &= ~1;
  // The GX scissor is a 12-bit field biased by 342, so a framebuffer wider than 3754 wraps it to zero
  // width and clips every draw (an all-black picture).
  const int maxWidth = 3754;
  if (fbWidth > maxWidth) {
    fbWidth = maxWidth;
  }
  if (fbWidth <= 0 || fbWidth == renderMode.fbWidth) {
    return;
  }
  CGraphics::PortResizeFrameBuffer(static_cast< u16 >(fbWidth));
}

float AspectWiden() {
  const int fbWidth = CGraphics::GetRenderMode().fbWidth;
  if (sBaseFbWidth <= 0 || fbWidth <= 0) {
    return 1.f;
  }
  return static_cast< float >(fbWidth) / static_cast< float >(sBaseFbWidth);
}

void Draw() {
  TickFps();
  if (const int milli = sPendingWheelMilli.exchange(0, std::memory_order_relaxed); milli != 0) {
    ImGui::GetIO().AddMouseWheelEvent(0.f, static_cast< float >(milli) / 1000.f);
  }
  PortPhoto::Frame();
  // Even with the window closed: the env vars are settings, and a headless run never opens it.
  ApplyGraphicsQuality();
  ApplyVsync();
  ApplyRenderScale();
  // A row's change is written once no item is active (MP1's window.cpp), so a slider drag is one write.
  if (PortOverlaySettings::Dirty() && ImGui::GetCurrentContext() != nullptr && !ImGui::IsAnyItemActive()) {
    PortOverlaySettings::SaveSettings();
  }
  PortUi::UpdateControllerNav();
  // While the window is open the cursor belongs to it, not to the game's aim.
  PortMouse::SetUiActive(sVisible);
  if (!sVisible) {
    return;
  }
  PortUi::UpdateUiScale(static_cast< float >(PortOverlay::UiScale()) / 100.f);
  PortUi::ApplyThemeIfChanged();

  PortOverlaySearch::BeginSearchJump();
  if (PortUi::PageLayout()) {
    if (!DrawPageWindow()) {
      sVisible = false;
    }
  } else {
    DrawDesktopWindow();
  }
  PortOverlaySearch::EndSearchJump();
}

} // namespace PortOverlay
