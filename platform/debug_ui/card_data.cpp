// F1 overlay: System > Memory card import/export and System > Data folder. The Prime 1 port's
// debug_ui/file_dialogs.cpp card section and data folder panel, on MP2's own card tools
// (PortSave: one 01-G2ME-*.gci, or 01-G2MP-*.gci on PAL, per card, no region conversion), including
// MP1's "Export as" Wii Trilogy (save.bin) choice and the Trilogy mirror; MP1's raw-image Dolphin
// cards have no counterpart here. The file dialogs answer on another thread, so a pick waits in
// sPicks until the next frame.
#include "debug_ui_internal.h"

#include "port_data_folder.h"
#include "port_overlay.h"
#include "port_entry.h"
#include "port_save.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace PortDebug {
namespace {

namespace fs = std::filesystem;

enum Pick { kPick_Import, kPick_Export, kPick_Emulator, kPick_Trilogy };
// The overlay's "Export as" choice: 0 the card's files as they are, 1 the Wii Trilogy's save.bin.
int sExportChoice = 0;
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
  } else if (pick == kPick_Trilogy) {
    static const SDL_DialogFileFilter bin[] = {{"Trilogy save (save.bin)", "bin"}, {"All files", "*"}};
    SDL_ShowOpenFileDialog(OnPick, userdata, FirstWindow(), bin, 2, nullptr, false);
  } else {
    // A folder: the card's files keep their names, so they import back as they are; the emulator
    // pick is its user folder.
    SDL_ShowOpenFolderDialog(OnPick, userdata, FirstWindow(), nullptr, false);
  }
}

// Dolphin's region folder for the mounted disc's card, as Aurora picks its own.
const char* DolphinRegion() {
  return port::entry::IsPalRegion(port::entry::CurrentDiscVersion()) ? "EUR" : "USA";
}

// Dolphin's (or PrimeHack's) GCI-folder card A for the mounted disc's release, where this machine
// has one: the chosen emulator folder, else the usual user folders (port_save.cpp).
fs::path DolphinCardFolder() { return fs::path(PortSave::EmulatorCardFolder()); }

// Dolphin's card files for the mounted release, newest last: 01-G2ME-*.gci (01-G2MP-*.gci on PAL).
std::vector< fs::path > DolphinSaves(const fs::path& card) {
  std::vector< fs::path > out;
  const std::string prefix = std::string("01-") + port::entry::CurrentGameCode() + "-";
  std::error_code ec;
  for (const fs::directory_entry& entry : fs::directory_iterator(card, ec)) {
    const std::string name = entry.path().filename().string();
    if (entry.is_regular_file(ec) && name.rfind(prefix, 0) == 0 && entry.path().extension() == ".gci") {
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
    if (pick == kPick_Emulator) {
      PortSave::SetEmulatorFolder(path);
      PortOverlaySettings::MarkDirty();
      sStatus = "Emulator folder: " + PortSave::EmulatorFolder();
    } else if (pick == kPick_Trilogy) {
      const bool ok = PortSave::ExportTrilogy(path, message);
      sStatus = ok ? "Exported: " + message : "Export failed: " + message;
    } else if (pick == kPick_Import) {
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

// MP1's emulator card block (file_dialogs.cpp DrawEmulatorCard): the emulator folder, the installed
// emulators, and sharing its card. Sharing mounts <folder>/GC as the card base at launch
// (CMemoryCardSys::Initialize), so it takes effect on the next start.
void DrawEmulatorCard() {
#if !defined(__ANDROID__)
  const bool inGame = sWorldId != 0;
  const std::string chosen = PortSave::EmulatorFolder();
  std::string found = "none found";
  if (chosen.empty()) {
    const fs::path card = DolphinCardFolder();
    if (!card.empty()) {
      found = card.parent_path().parent_path().parent_path().string();
    }
  }
  ImGui::TextUnformatted("Emulator folder");
  ImGui::SameLine(0.0f, 0.0f);
  ImGui::TextWrapped(": %s", chosen.empty() ? ("Auto (" + found + ")").c_str() : chosen.c_str());
  ImGui::BeginDisabled(sDialogOpen);
  if (ImGui::Button("Choose...")) {
    OpenDialog(kPick_Emulator);
  }
  ImGui::EndDisabled();
  ItemHelp("The Dolphin or PrimeHack user folder (the one containing GC/). Only needed for "
           "portable installs.");
  ImGui::SameLine();
  ImGui::BeginDisabled(chosen.empty());
  if (ImGui::Button("Auto")) {
    PortSave::SetEmulatorFolder("");
    PortOverlaySettings::MarkDirty();
  }
  ImGui::EndDisabled();
  // With Dolphin and PrimeHack both installed, Auto takes the first; pick the other here.
  const std::vector< std::string > detected = PortSave::DetectedEmulatorFolders();
  if (!detected.empty()) {
    const std::string preview = chosen.empty() ? "Auto" : chosen;
    if (ImGui::BeginCombo("Installed emulators", preview.c_str())) {
      for (const std::string& root : detected) {
        const std::string label =
            (root.find("primehack") != std::string::npos ? "PrimeHack: " : "Dolphin: ") + root;
        if (ImGui::Selectable(label.c_str(), root == chosen)) {
          PortSave::SetEmulatorFolder(root);
          PortOverlaySettings::MarkDirty();
        }
      }
      ImGui::EndCombo();
    }
    ItemHelp("Installs found on this computer. Pick one instead of the automatic choice.");
  }
  bool share = PortSave::ShareEmulatorCard();
  ImGui::BeginDisabled(inGame);
  if (ImGui::Checkbox("Share memory card with emulator", &share)) {
    PortSave::SetShareEmulatorCard(share);
    PortOverlaySettings::MarkDirty();
  }
  ImGui::EndDisabled();
  ItemHelp("The port and the emulator share the same saves: never run both at once. Takes effect "
           "the next time the game starts.");
  if (!share) {
    return;
  }
  bool mirror = PortSave::MirrorTrilogy();
  if (ImGui::Checkbox("Mirror with the Trilogy save.bin", &mirror)) {
    PortSave::SetMirrorTrilogy(mirror);
    PortOverlaySettings::MarkDirty();
  }
  ItemHelp("Keeps the card and Dolphin's Metroid Prime Trilogy save.bin (Wii/title/00010000/"
           "52334d45/data/save.bin) in step: a save made here goes out once written, and a newer "
           "one made in the Trilogy comes in at the front end. The Trilogy file must have been "
           "started once.");
  if (mirror) {
    const std::string bin = PortSave::FindEmulatorTrilogy();
    if (bin.empty()) {
      ImGui::TextColored(ThemeWarnColor(), "No Trilogy save.bin found in the emulator folder.");
    } else {
      ImGui::TextWrapped("Mirroring: %s", bin.c_str());
    }
  }
  std::string why;
  const std::string base = PortSave::SharedCardBase(&why);
  if (base.empty()) {
    ImGui::TextColored(ThemeWarnColor(), "%s", why.c_str());
    return;
  }
  const std::string active = PortSave::Directory();
  const fs::path shared = fs::path(base) / DolphinRegion() / "Card A";
  std::error_code ec;
  if (!active.empty() && fs::equivalent(fs::path(active), shared, ec)) {
    ImGui::TextWrapped("Sharing: %s", shared.string().c_str());
    return;
  }
  ImGui::TextWrapped("Will share %s from the next start.", shared.string().c_str());
  ImGui::BeginDisabled(inGame || sDialogOpen || active.empty());
  if (ImGui::Button("Copy port saves into shared card")) {
    std::string message;
    const bool ok = PortSave::Export(shared.string(), message);
    sStatus = ok ? "Copied: " + message : "Copy failed: " + message;
  }
  ImGui::EndDisabled();
  ItemHelp("Copies the port's current card saves into the shared card; a file of the same name "
           "there gets a numbered copy, not replaced.");
#endif
}

} // namespace

void DrawCardTransfer() {
  TakePicks();
  DrawEmulatorCard();
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
  ItemHelp(dolphin.empty() ? (std::string("No Dolphin card folder (GC/") + DolphinRegion() +
                            "/Card A) was found in the usual user folders.").c_str()
                         : (std::string("Imports the newest save in Dolphin's ") + DolphinRegion() +
                            " card folder.").c_str());
  ImGui::EndDisabled();
  ImGui::BeginDisabled(sDialogOpen || saves == 0);
  const char* choices[] = {"As on the card (.gci)", "Wii Trilogy (save.bin)"};
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0f);
  ImGui::Combo("Export as", &sExportChoice, choices, IM_ARRAYSIZE(choices));
  ItemHelp("The format to export in. For the Trilogy, start Metroid Prime 2 in an existing save.bin "
           "and save once first: that file's Echoes game is replaced. A Wii's signed data.bin "
           "can't be made here.");
  const bool trilogy = sExportChoice == 1;
  if (ImGui::Button(trilogy ? "Export into save.bin..." : "Export to folder...")) {
    OpenDialog(trilogy ? kPick_Trilogy : kPick_Export);
  }
  ItemHelp(trilogy ? "Pick the Trilogy save.bin to write into; the old one is kept as save.bin.bak."
                   : "Copies the card's save files into the folder you pick, with their own names so "
                     "they can be imported again.");
  ImGui::SameLine();
  const std::string trilogyBin = trilogy ? PortSave::FindEmulatorTrilogy() : std::string();
  ImGui::BeginDisabled(trilogy ? trilogyBin.empty() : dolphin.empty());
  if (ImGui::Button("Export to Dolphin/PrimeHack")) {
    std::string message;
    const bool ok = trilogy ? PortSave::ExportTrilogy(trilogyBin, message)
                            : PortSave::Export(dolphin.string(), message);
    sStatus = ok ? "Exported: " + message : "Export failed: " + message;
  }
  ImGui::EndDisabled();
  ItemHelp(trilogy ? "Close the emulator first. Goes to its Trilogy save (Wii/title/00010000/52334d45/"
                     "data/save.bin); the old one is kept as save.bin.bak."
                   : "Close the emulator first. Writes the save into its card folder for this release; a file "
                     "of the same name there gets a numbered copy, not replaced.");
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

// Where everything is; on Android also the move to shared storage (port_data_folder.cpp), on the
// desktop a way to open the folder.
void DrawDataFolder() {
  ImGui::SeparatorText("Data folder");
  const std::string backups = PortSave::BackupsDirectory();
  const std::string user =
      backups.empty() ? std::string() : fs::path(backups).parent_path().string();
  PathRow("User folder", user);
  PathRow("Memory card", PortSave::Directory());
  PathRow("Save backups", backups);
#if defined(__ANDROID__)
  PortDataFolder::DrawPanel();
#else
  if (!user.empty() && ImGui::Button("Open data folder")) {
    OpenFolder(user);
  }
#endif
  ItemHelp("Settings, memory card, save backups, save states and mods live in the user folder "
           "(`MP2_USER_PATH` overrides it).");
}

} // namespace PortDebug
