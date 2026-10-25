// F1 overlay: System > Memory card import/export and System > Data folder. The Prime 1 port's
// debug_ui/file_dialogs.cpp card section and data folder panel, on MP2's own card tools
// (PortSave: one 01-G2ME-*.gci per card, no region conversion) and without MP1's Wii Trilogy
// save.bin and raw-image Dolphin cards, which MP2 has no counterpart for. The file dialogs answer
// on another thread, so a pick waits in sPicks until the next frame.
#include "debug_ui_internal.h"

#include "port_save.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace PortDebug {
namespace {

namespace fs = std::filesystem;

enum Pick { kPick_Import, kPick_Export };
std::mutex sPickMutex;
std::vector< std::pair< Pick, std::string > > sPicks;
bool sDialogOpen = false;
std::string sStatus;

SDL_Window* FirstWindow() {
  int count = 0;
  SDL_Window** windows = SDL_GetWindows(&count);
  SDL_Window* window = windows != nullptr && count > 0 ? windows[0] : nullptr;
  SDL_free(windows);
  return window;
}

void OnPick(void* userdata, const char* const* files, int) {
  std::lock_guard< std::mutex > lock(sPickMutex);
  if (files != nullptr && files[0] != nullptr) {
    sPicks.emplace_back(static_cast< Pick >(reinterpret_cast< intptr_t >(userdata)), files[0]);
  }
  sDialogOpen = false;
}

void OpenDialog(Pick pick) {
  static const SDL_DialogFileFilter filters[] = {{"Save files (.gci)", "gci"}, {"All files", "*"}};
  sDialogOpen = true;
  void* userdata = reinterpret_cast< void* >(static_cast< intptr_t >(pick));
  if (pick == kPick_Import) {
    SDL_ShowOpenFileDialog(OnPick, userdata, FirstWindow(), filters, 2, nullptr, false);
  } else {
    // A folder: the card's files keep their names, so they import back as they are.
    SDL_ShowOpenFolderDialog(OnPick, userdata, FirstWindow(), nullptr, false);
  }
}

// Dolphin's (or PrimeHack's) GCI-folder card A for the USA release, where this machine has one.
// The roots are the usual user folders; the card folder is <root>/GC/USA/Card A.
fs::path DolphinCardFolder() {
  std::vector< fs::path > roots;
  const char* home = std::getenv("HOME");
  const char* appdata = std::getenv("APPDATA");
  const char* xdg = std::getenv("XDG_DATA_HOME");
  if (appdata != nullptr) {
    roots.push_back(fs::path(appdata) / "Dolphin Emulator");
  }
  if (xdg != nullptr && xdg[0] == '/') {
    roots.push_back(fs::path(xdg) / "dolphin-emu");
  }
  if (home != nullptr) {
    const fs::path h(home);
    roots.push_back(h / ".local/share/dolphin-emu");
    roots.push_back(h / ".var/app/org.DolphinEmu.dolphin-emu/data/dolphin-emu");
    roots.push_back(h / ".dolphin-emu");
    roots.push_back(h / "Library/Application Support/Dolphin");
  }
  for (const fs::path& root : roots) {
    std::error_code ec;
    const fs::path card = root / "GC" / "USA" / "Card A";
    if (fs::is_directory(card, ec)) {
      return card;
    }
  }
  return {};
}

// Dolphin's card files for this game, newest last: 01-G2ME-*.gci.
std::vector< fs::path > DolphinSaves(const fs::path& card) {
  std::vector< fs::path > out;
  std::error_code ec;
  for (const fs::directory_entry& entry : fs::directory_iterator(card, ec)) {
    const std::string name = entry.path().filename().string();
    if (entry.is_regular_file(ec) && name.rfind("01-G2ME-", 0) == 0 && entry.path().extension() == ".gci") {
      out.push_back(entry.path());
    }
  }
  std::sort(out.begin(), out.end(), [](const fs::path& a, const fs::path& b) {
    std::error_code ec;
    return fs::last_write_time(a, ec) < fs::last_write_time(b, ec);
  });
  return out;
}

void OpenFolder(const std::string& path) {
  // "Card A" has a space, which a URL can't carry as is.
  std::string url = !path.empty() && path.front() == '/' ? "file://" : "file:///";
  for (const char c : path) {
    url += c == ' ' ? std::string("%20") : std::string(1, c == '\\' ? '/' : c);
  }
  SDL_OpenURL(url.c_str());
}

void TakePicks() {
  std::vector< std::pair< Pick, std::string > > picks;
  {
    std::lock_guard< std::mutex > lock(sPickMutex);
    picks.swap(sPicks);
  }
  for (const auto& [pick, path] : picks) {
    std::string message;
    if (pick == kPick_Import) {
      const bool ok = PortSave::Import(path, message);
      sStatus = ok ? "Imported: " + message : "Import failed: " + message;
    } else {
      const bool ok = PortSave::Export(path, message);
      sStatus = ok ? "Exported: " + message : "Export failed: " + message;
    }
  }
}

void PathRow(const char* label, const std::string& path) {
  ImGui::TextWrapped("%s: %s", label, path.empty() ? "(not set)" : path.c_str());
}

} // namespace

void DrawCardTransfer() {
  TakePicks();
  const std::string folder = PortSave::Directory();
  const size_t saves = PortSave::Files().size();
  if (PortSave::IsImage() || folder.empty()) {
    ImGui::TextDisabled("The card is not a GCI folder (yet); import and export need one.");
    return;
  }
  ImGui::TextWrapped("Card: %s (%zu save file%s)", folder.c_str(), saves, saves == 1 ? "" : "s");
  const bool inGame = sWorldId != 0;
  ImGui::BeginDisabled(inGame || sDialogOpen);
  if (ImGui::Button("Import file...")) {
    OpenDialog(kPick_Import);
  }
  ItemHelp("Loads a .gci save of this release. The card's current save is copied to save-backups "
           "first.");
  const fs::path dolphin = DolphinCardFolder();
  ImGui::SameLine();
  ImGui::BeginDisabled(dolphin.empty());
  if (ImGui::Button("Import from Dolphin/PrimeHack")) {
    const std::vector< fs::path > saved = DolphinSaves(dolphin);
    std::string message;
    if (saved.empty()) {
      sStatus = "Dolphin's card has no Metroid Prime 2 save.";
    } else {
      const bool ok = PortSave::Import(saved.back().string(), message);
      sStatus = ok ? "Imported: " + message : "Import failed: " + message;
    }
  }
  ImGui::EndDisabled();
  ItemHelp(dolphin.empty() ? "No Dolphin card folder (GC/USA/Card A) was found in the usual user folders."
                           : "Imports the newest save in Dolphin's USA card folder.");
  ImGui::EndDisabled();
  ImGui::BeginDisabled(sDialogOpen || saves == 0);
  if (ImGui::Button("Export to folder...")) {
    OpenDialog(kPick_Export);
  }
  ItemHelp("Copies the card's save files into the folder you pick, with their own names so they can "
           "be imported again.");
  ImGui::SameLine();
  ImGui::BeginDisabled(dolphin.empty());
  if (ImGui::Button("Export to Dolphin/PrimeHack")) {
    std::string message;
    const bool ok = PortSave::Export(dolphin.string(), message);
    sStatus = ok ? "Exported: " + message : "Export failed: " + message;
  }
  ImGui::EndDisabled();
  ItemHelp("Close the emulator first. Writes the save into its USA card folder; a file of the same "
           "name there gets a numbered copy, not replaced.");
  ImGui::EndDisabled();
  ImGui::SameLine();
  if (ImGui::Button("Open card folder")) {
    OpenFolder(folder);
  }
  if (inGame) {
    ImGui::TextColored(ThemeWarnColor(), "Return to the title screen to import.");
  }
  if (!sStatus.empty()) {
    ImGui::TextWrapped("%s", sStatus.c_str());
  }
}

// MP1's Data folder panel is Android's (moving the data to shared storage); on the desktop the
// useful part is where everything is and a way to open it.
void DrawDataFolder() {
  ImGui::SeparatorText("Data folder");
  const std::string backups = PortSave::BackupsDirectory();
  const std::string user =
      backups.empty() ? std::string() : fs::path(backups).parent_path().string();
  PathRow("User folder", user);
  PathRow("Memory card", PortSave::Directory());
  PathRow("Save backups", backups);
  if (!user.empty() && ImGui::Button("Open data folder")) {
    OpenFolder(user);
  }
  ItemHelp("Settings, memory card, save backups, save states and mods live in the user folder "
           "(`MP2_USER_PATH` overrides it).");
}

} // namespace PortDebug
