// F1 overlay: The Game and System pages' sections (game, cutscenes, audio, speedrun, overlay,
// settings, memory card, log). State and helpers shared with the other parts are in
// debug_ui_internal.h.

#include "debug_ui_internal.h"

#include "port_config_reset.h"
#include "port_disc_verify.h"
#include "port_discord.h"
#include "port_freecam.h"
#include "port_input_remap_ui.h"
#include "port_livesplit.h"
#include "port_mods.h"
#include "port_logfile.h"
#include "port_overlay.h"
#include "port_settings.h"

#include "MetroidPrime/Player/CGameOptions.hpp"
#include "MetroidPrime/Player/CGameState.hpp"

#include <aurora/aurora.h>
#include <SDL3/SDL_events.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <filesystem>
#include <string>

namespace aurora {
void request_screenshot() noexcept;
}

namespace PortDebug {

// Screenshot, photo mode and exit; the top of the Game page (settings are on System).
void DrawGameSection() {
  ImGui::SeparatorText("Game");
  const std::string shotKeys = PortInputRemap::BindingsText(PortInput::Action::Screenshot, " / ");
  const std::string shotLabel =
      (shotKeys.empty() ? std::string("Screenshot") : "Screenshot (" + shotKeys + ")") + "###screenshot";
  if (ImGui::Button(shotLabel.c_str())) {
    aurora::request_screenshot();
  }
  ImGui::SameLine();
  const std::string photoKeys = PortInputRemap::BindingsText(PortInput::Action::PhotoMode, " / ");
  const std::string photoLabel =
      (photoKeys.empty() ? std::string("Photo mode") : "Photo mode (" + photoKeys + ")") + "###photomode";
  if (ImGui::Button(photoLabel.c_str())) {
    // Photo mode here is the free camera (Debug > Camera has its rows); the overlay would cover
    // the picture.
    PortFreeCam::SetActive(true, nullptr);
    sVisible = false;
  }
  ImGui::SameLine();
  if (ImGui::Button("Exit game")) {
    ImGui::OpenPopup("Exit game?");
  }
  if (ImGui::BeginPopupModal("Exit game?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted("Progress since the last save station is lost.");
    if (ImGui::Button("Exit")) {
      // The same path as closing the window: the main loop sees the quit and shuts down cleanly.
      PortOverlaySettings::SaveSettings();
      SDL_Event quit{};
      quit.type = SDL_EVENT_QUIT;
      SDL_PushEvent(&quit);
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void DrawCutscenesSection() {
  ImGui::SeparatorText("Cutscenes");
  // The game reads this every frame in its cinematic flow state.
  Changed(ImGui::Checkbox("Skippable cutscenes", &sSkipCinematics));
  ItemHelp("START or A skips any cutscene, seen or not. Off: START skips only the ones already seen.");
  // Read by CSplashScreen, so a change applies from the next launch.
  int bootLogos = sBootLogos;
  if (ImGui::Combo("Boot logos", &bootLogos, "Show\0" "Skip with A or Start\0" "Off\0")) {
    sBootLogos = std::clamp(bootLogos, PortOverlay::kScreen_Show, PortOverlay::kScreen_Off);
    Changed(true);
  }
  ItemHelp("The health warning and the Nintendo, Retro and Dolby logos at launch. Applies at the next "
           "launch.");
  // Read by CMFGameLoader, so a change applies from the next load.
  int worldName = sWorldNameCard;
  if (ImGui::Combo("World name on load", &worldName, "Show\0" "Skip with A or Start\0" "Off\0")) {
    sWorldNameCard = std::clamp(worldName, PortOverlay::kScreen_Show, PortOverlay::kScreen_Off);
    Changed(true);
  }
  ItemHelp("The world's name typed out while a save loads. The first boot's intro text is not "
           "affected.");
  int walkSpeed = sWalkSpeed;
  if (ImGui::SliderInt("Walk speed", &walkSpeed, PortOverlay::kWalkSpeedMin,
                       PortOverlay::kWalkSpeedMax, "%d%%", ImGuiSliderFlags_AlwaysClamp)) {
    sWalkSpeed = walkSpeed;
    Changed(true);
  }
  ItemHelp("How fast Samus moves on foot, walking, strafing and circle-strafing, in percent of "
           "the original speed. Jumps, dashes, turning and the Morph Ball are unchanged.");
  int hintDelay = sHintDelay;
  if (ImGui::SliderInt("Hint delay", &hintDelay, PortOverlay::kHintDelayMin,
                       PortOverlay::kHintDelayMax, hintDelay == 0 ? "Game default" : "%d min",
                       ImGuiSliderFlags_AlwaysClamp)) {
    sHintDelay = hintDelay;
    Changed(true);
  }
  ItemHelp("The longest a hint waits before it first shows. Game default keeps the game's own "
           "timing; a shorter wait also applies to hints already waiting in your save. Needs the "
           "in-game hint system on.");
  ImGui::BeginDisabled(gpGameState == nullptr ||
                       !gpGameState->GameOptions().GetIsHintSystemEnabled());
  if (ImGui::Button("Show current hint")) {
    PortOverlay::RequestShowHint();
  }
  ImGui::EndDisabled();
  ItemHelp("Shows the hint the game would show next, now: the one on screen again, or the one "
           "due soonest. Needs the in-game hint system on.");
}

// The game's own volumes, the same as the pause menu's two sliders.
void DrawGameAudio() {
  ImGui::SeparatorText("Audio");
  ImGui::BeginDisabled(gpGameState == nullptr);
  int music = gpGameState != nullptr ? static_cast< int >(gpGameState->GameOptions().GetMusicVolume()) : 0;
  int musicPct = (music * 100 + 63) / 127;
  if (ImGui::SliderInt("Music volume", &musicPct, 0, 100, "%d%%") && gpGameState != nullptr) {
    gpGameState->GameOptions().SetMusicVolume((musicPct * 127 + 50) / 100, true);
  }
  int sfx = gpGameState != nullptr ? static_cast< int >(gpGameState->GameOptions().GetSfxVolume()) : 0;
  int sfxPct = (sfx * 100 + 63) / 127;
  if (ImGui::SliderInt("Sound effects volume", &sfxPct, 0, 100, "%d%%") && gpGameState != nullptr) {
    gpGameState->GameOptions().SetSfxVolume((sfxPct * 127 + 50) / 100, true);
  }
  // Sound mode: Mono / Stereo / Dolby Surround
  int soundMode = gpGameState != nullptr ? static_cast< int >(gpGameState->GameOptions().GetSurroundMode())
                                         : CAudioSys::kSM_Stereo;
  constexpr const char* kSoundModeLabels[] = {"Mono", "Stereo", "Dolby Surround"};
  if (ImGui::Combo("Sound mode", &soundMode, kSoundModeLabels, IM_ARRAYSIZE(kSoundModeLabels)) &&
      gpGameState != nullptr) {
    gpGameState->GameOptions().SetSurroundMode(static_cast< CAudioSys::ESurroundModes >(soundMode), true);
  }
  ImGui::EndDisabled();
  ImGui::SetItemTooltip("The game's own options: the next save writes them, as the pause menu's do.");
}

void DrawSpeedrunSection() {
  ImGui::SeparatorText("Speedrun");
  bool liveSplit = PortLiveSplit::Enabled();
  if (ImGui::Checkbox("LiveSplit", &liveSplit)) {
    PortLiveSplit::SetEnabled(liveSplit);
    Changed(true);
  }
  ItemHelp("In LiveSplit: right-click, Control > Start TCP Server (port 16834), and compare "
           "against Game Time. A new file starts the timer; it splits on the final blow.");
  SameLineAfterHelp();
  switch (PortLiveSplit::Status()) {
  case PortLiveSplit::kStatus_Off:
    ImGui::TextDisabled("off");
    break;
  case PortLiveSplit::kStatus_Connecting:
    ImGui::TextUnformatted("connecting...");
    break;
  case PortLiveSplit::kStatus_Connected:
    ImGui::TextColored(ThemeGoodColor(), "connected");
    break;
  case PortLiveSplit::kStatus_Failed:
    ImGui::TextColored(ThemeBadColor(), "%s", PortLiveSplit::LastError().c_str());
    break;
  }
  // Applied when the field loses focus; until then the text is left alone.
  static char address[128];
  static bool editingAddress = false;
  if (!editingAddress) {
    std::snprintf(address, sizeof(address), "%s", PortLiveSplit::Address().c_str());
  }
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.f);
  ImGui::InputText("Server (host:port)", address, sizeof(address));
  editingAddress = ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    PortLiveSplit::SetAddress(address);
    Changed(true);
  }
  ImGui::SameLine();
  bool splitUpgrades = PortLiveSplit::SplitUpgrades();
  if (ImGui::Checkbox("Split on upgrades", &splitUpgrades)) {
    PortLiveSplit::SetSplitUpgrades(splitUpgrades);
    Changed(true);
  }
  ImGui::SetItemTooltip("Also splits on each new suit, beam, visor, ball part, key or other upgrade.");
}

// The mods folder (port_mods.h): the list is what this launch loaded; changes apply on restart.
void DrawModsSection() {
  ImGui::SeparatorText("Mods");
  const PortMods::Status& status = PortMods::CurrentStatus();
  bool enabled = PortMods::Enabled();
  if (ImGui::Checkbox("Load mods", &enabled)) {
    PortMods::SetEnabled(enabled);
    Changed(true);
  }
  ItemHelp("Layers the folders in the mods folder over the disc. Mods load at startup, so a "
           "change here applies after a restart.");
  std::vector< std::string > disabled = PortMods::SplitDisabled(PortMods::Disabled());
  bool changed = enabled != status.active;
  if (status.mods.empty()) {
    ImGui::TextDisabled("No mods in the folder.");
  }
  for (const PortMods::ModInfo& mod : status.mods) {
    const auto found = std::find(disabled.begin(), disabled.end(), mod.name);
    bool on = found == disabled.end();
    ImGui::BeginDisabled(!enabled);
    if (ImGui::Checkbox((mod.name + "##mod").c_str(), &on)) {
      if (on) {
        disabled.erase(std::remove(disabled.begin(), disabled.end(), mod.name), disabled.end());
      } else {
        disabled.push_back(mod.name);
      }
      PortMods::SetDisabled(PortMods::JoinDisabled(disabled));
      Changed(true);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (mod.enabled) {
      ImGui::TextDisabled("%d file(s), %d resource(s)", mod.files, mod.resources);
    } else {
      ImGui::TextDisabled("not loaded");
    }
    changed = changed || (enabled && on) != mod.enabled;
  }
  if (changed) {
    ImGui::TextColored(ThemeBadColor(), "Restart the game to apply.");
  }
  for (const std::string& message : status.messages) {
    ImGui::TextColored(ThemeBadColor(), "%s", message.c_str());
  }
  if (!status.folder.empty()) {
    ImGui::TextWrapped("Folder: %s", status.folder.c_str());
  }
  ImGui::TextWrapped("Each folder in the mods folder is a mod; later names win. A file at a disc "
                     "path (Metroid5.pak, Audio/..., Video/...) replaces that file, and a "
                     "resource named by id and type (1A2B3C4D.TXTR) replaces it in every PAK.");
}

// The look of this overlay: the floating window, the theme and the size.
void DrawOverlaySection() {
  ImGui::SeparatorText("Overlay");
#if !defined(__ANDROID__)
  Changed(ImGui::Checkbox("Overlay as a floating window", &sOverlayWindowed));
#endif
  int theme = sUiTheme;
  if (ImGui::Combo("Menu theme", &theme, "Auto\0Metroid Prime\0Plain\0")) {
    sUiTheme = theme;
    Changed(true);
  }
  ItemHelp("The overlay's look. Auto is the Metroid Prime pause-screen style; Plain is stock Dear "
           "ImGui.");
  // MP1's ui_sounds row: the game's own UI SFX for open, close, choose, focus move and back.
  Changed(ImGui::Checkbox("Menu sounds", &sUiSounds));
  ItemHelp("Plays the game's own menu sounds for opening, closing and moving through this window. "
           "Not with the Plain theme.");
  // Multiplies the display's own scale (UpdateUiScale); the product is clamped to 1..4, so on a
  // desktop at scale 1 only values above 100% make the window larger.
  Changed(ImGui::SliderInt("UI scale", &sUiScale, PortOverlay::kUiScaleMin, PortOverlay::kUiScaleMax, "%d%%"));
  ItemHelp("The size of this window's text and controls.");
}

void DrawSettingsSection() {
  ImGui::SeparatorText("Settings");
  if (ImGui::Button("Save settings now")) {
    PortOverlaySettings::MarkDirty();
    PortOverlaySettings::SaveSettings();
  }
  ImGui::SameLine();
  ImGui::TextUnformatted(PortOverlaySettings::Dirty() ? "Unsaved changes" : "Saved");
  ImGui::SetItemTooltip("Settings are saved automatically when changed. An MP2_<KEY> environment\n"
                        "variable overrides the file for one run.");
  const std::string& path = PortSettings::FilePath();
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
  ImGui::TextWrapped("File: %s", path.empty() ? "(no user folder)" : path.c_str());
  ImGui::PopStyleColor();

  static std::string sResetError;
  ImGui::BeginDisabled(path.empty());
  if (ImGui::Button("Reset configuration...")) {
    sResetError.clear();
    ImGui::OpenPopup("Reset configuration?");
  }
  ImGui::EndDisabled();
  ImGui::SetItemTooltip("Back up and clear every port setting and control mapping.");
  if (ImGui::BeginPopupModal("Reset configuration?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
    ImGui::TextUnformatted("This resets ALL port settings and control mappings (keyboard, mouse, touch and "
                           "controllers) to their defaults on the next launch. The current files are moved to a "
                           "backup folder inside the user folder, not deleted.");
    ImGui::TextUnformatted("Save files, save states, the disc image and the data folder location are not "
                           "touched. Game options (volumes, brightness...) stored inside a save file come back "
                           "when that save loads.");
    ImGui::TextUnformatted("The game closes now: progress since the last save station is lost. Start it again to "
                           "get the defaults.");
    if (!sResetError.empty()) {
      ImGui::PushStyleColor(ImGuiCol_Text, ThemeBadColor());
      ImGui::TextWrapped("Reset failed: %s", sResetError.c_str());
      ImGui::PopStyleColor();
    }
    ImGui::PopTextWrapPos();
    if (ImGui::Button("Reset and exit")) {
      // Back up what is current, not what was last saved; a failed write stops here.
      PortOverlaySettings::MarkDirty();
      PortOverlaySettings::SaveSettings();
      if (PortOverlaySettings::Dirty()) {
        sResetError = "could not save the current settings first, so nothing was moved.";
      } else {
        const PortConfigReset::Result result = PortConfigReset::Reset(
            std::filesystem::path(path).parent_path().empty() ? std::filesystem::path("./")
                                                              : std::filesystem::path(path).parent_path());
        if (!result.ok) {
          sResetError = result.error;
          if (!result.backupDir.empty()) {
            sResetError += " Files that could not be restored are in " + result.backupDir + ".";
          } else {
            sResetError += " Your configuration files were not moved.";
          }
          std::fprintf(stderr, "port: reset configuration failed: %s\n", result.error.c_str());
        } else {
          std::fprintf(stderr, "port: reset configuration: %s\n",
                       result.moved ? ("backup in " + result.backupDir).c_str() : "no files to move");
          // From here no code path may write the configuration again.
          PortOverlaySettings::FreezeAfterReset();
          aurora_disable_config_persistence();
          if (ImGui::GetCurrentContext() != nullptr) {
            ImGui::GetIO().IniFilename = nullptr;
          }
          SDL_Event quit{};
          quit.type = SDL_EVENT_QUIT;
          SDL_PushEvent(&quit);
          ImGui::CloseCurrentPopup();
        }
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

// The whole disc image check (port_disc_verify.h): MP1's System > Disc image section, without its
// "Replace disc image" (that picks a new file to copy, which this port's mount loop already does
// on a failed boot).
void DrawDiscImageSection() {
  using PortDiscVerify::State;
  ImGui::SeparatorText("Disc image");
  const PortDiscVerify::Result r = PortDiscVerify::Snapshot();
  const bool running = r.state == State::Running;
  if (const std::string image = PortDiscVerify::ImagePath(); !image.empty()) {
    ImGui::TextWrapped("Disc image: %s", image.c_str());
  }
  ImGui::BeginDisabled(running);
  if (ImGui::Button("Check disc image")) {
    PortDiscVerify::Start();
  }
  ImGui::EndDisabled();
  if (running) {
    ImGui::SameLine();
    if (ImGui::Button("Cancel##disc")) {
      PortDiscVerify::Cancel();
    }
    ImGui::ProgressBar(static_cast< float >(r.Progress()), ImVec2(-FLT_MIN, 0.0f));
  }
  switch (r.state) {
  case State::Idle:
    ImGui::TextWrapped("Compares the whole disc image with the known dump of this release to find "
                       "damage. Runs in the background and takes a minute or two; the result goes "
                       "to the log.");
    break;
  case State::Running:
    ImGui::TextWrapped("Checking the disc image (%.0f%%)...", r.Progress() * 100.0);
    break;
  case State::Cancelled:
    ImGui::TextWrapped("The check was cancelled.");
    break;
  case State::Error:
    ImGui::TextWrapped("The disc image couldn't be checked: %s.", r.error.c_str());
    break;
  case State::Done:
    if (r.Ok()) {
      ImGui::TextWrapped("This disc image is complete and unaltered (G2ME01, %s).",
                         r.revision.c_str());
      ImGui::TextDisabled("SHA-1 %s", r.sha1.c_str());
    } else if (!r.known) {
      ImGui::TextWrapped("This disc image is not a release the port has a record of.");
      ImGui::TextDisabled("SHA-1 %s", r.sha1.c_str());
    } else if (r.bad.empty()) {
      ImGui::TextWrapped("Every part of this %s image matches, but the file does not end where the "
                         "original does.",
                         r.revision.c_str());
    } else {
      ImGui::TextWrapped("This disc image is damaged: %zu of %u MiB differ from %s (%u of them are "
                         "blank).",
                         r.bad.size(), r.chunks, r.revision.c_str(), r.zeroed);
      if (!r.files.empty()) {
        std::string files;
        for (size_t i = 0; i < r.files.size() && i < 8; ++i) {
          files += (i ? ", " : "") + r.files[i];
        }
        if (r.files.size() > 8) {
          files += ", ...";
        }
        ImGui::TextWrapped("Damaged: %s", files.c_str());
      }
      ImGui::TextWrapped("Copy the disc image again; this part of it is damaged.");
    }
    break;
  }
}

// Discord Rich Presence (port_discord.h): MP1's Game > Discord section. The presence itself is
// fed from CStateManager::Update; this row only turns it on and reports what the background
// thread is doing.
void DrawDiscordSection() {
  if (!PortDiscord::Supported()) {
    return;
  }
  ImGui::SeparatorText("Discord");
  bool discord = sDiscord;
  if (ImGui::Checkbox("Rich Presence", &discord)) {
    sDiscord = discord;
    PortDiscord::Configure(discord);
    Changed(true);
  }
  ItemHelp("Shows the area, energy, missiles and item percentage on your Discord profile while the "
           "desktop Discord client runs. Nothing is sent when it is not running.");
  switch (PortDiscord::Status()) {
  case PortDiscord::kStatus_Off:
    ImGui::TextDisabled("off");
    break;
  case PortDiscord::kStatus_Connecting:
    ImGui::TextUnformatted("connecting...");
    break;
  case PortDiscord::kStatus_Connected:
    ImGui::TextColored(ThemeGoodColor(), "connected");
    break;
  case PortDiscord::kStatus_Failed:
    ImGui::TextColored(ThemeBadColor(), "%s", PortDiscord::LastError().c_str());
    break;
  }
  if (sDiscord) {
    ImGui::TextDisabled("Showing: %s", PortDiscord::CurrentText().c_str());
  }
}

void DrawMemoryCard() {
  ImGui::SeparatorText("Memory card");
  Changed(ImGui::Checkbox("Back up the card", &sCardBackup));
  ItemHelp("Copies the save file into save-backups before each card write; `save import latest` on "
           "the console puts one back.");
  DrawCardTransfer();
}

void DrawLogSection() {
  ImGui::SeparatorText("Log");
  // PortLogFile::Start reads the setting before Aurora comes up, so the row sets the next launch.
  Changed(ImGui::Checkbox("Write the log to a file", &sLogFile));
  const std::string logPath = PortLogFile::Path();
  if (!logPath.empty()) {
    ImGui::SameLine();
    if (ImGui::Button("Copy log path")) {
      ImGui::SetClipboardText(logPath.c_str());
    }
  }
  if (PortLogFile::Enabled()) {
    ImGui::TextWrapped("Writing to %s.", logPath.c_str());
    if (!sLogFile) {
      ImGui::TextDisabled("Stops at the next start.");
    }
  } else {
    ImGui::TextWrapped("Everything the game logs, including the reason for a crash, goes to %s from the "
                       "next start.",
                       logPath.empty() ? "metroid_prime2_port.log in the user folder" : logPath.c_str());
  }
}

} // namespace PortDebug
