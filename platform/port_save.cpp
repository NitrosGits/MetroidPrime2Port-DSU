#include "port_mobile.h"
#include "port_save.h"

#include "port_save_trilogy.h"

#include "port_entry.h"
#include "port_env.h"
#include "port_overlay.h"
#include "port_save_convert.h"

#include "Kyoto/CCrc32.hpp"
#include "MetroidPrime/Player/CGameState.hpp"

#include <aurora/card.h>

#include <algorithm>
#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#endif

namespace PortSave {

namespace {

// The card the port mounts: CMemoryCardSys::Initialize calls CARDInit(<disc game code>, "01"), and
// Aurora resolves that to `<user>/USA/Card A` for G2ME or `<user>/EUR/Card A` for a PAL disc's G2MP
// (GCI folder or raw image). The releases' saves carry different SAVW layouts, so each keeps its own
// card, and a save of the other release is refused by name rather than read with the wrong layout.
const char* GameCode() { return port::entry::CurrentGameCode(); }
// CMemoryCardDriver's own file name (skSaveFileName, src/MetroidPrime/CMemoryCardDriver.cpp), and
// the name Aurora writes it under in a GCI folder.
constexpr const char* kSaveFileName = "MetroidPrime2";

// The .gci layout, from CMemoryCardSys::CCardFileInfo: a 64-byte directory entry, a 0x2000 header
// block (comment, banner, icons, CRC), then two copies of the save data, each mAlignedUserDataSize
// (one 0x2000 block for this game) - CARDRead's offsets 0x2000 and mAlignedUserDataSize + 0x2000.
constexpr size_t kDirEntry = 0x40;
constexpr size_t kBlock = 0x2000;
constexpr int kCopies = 2;

// A copy's payload, from CMemoryCardDriver::BuildSaveBuffer: SSaveHeader::PutTo (signature, index,
// three present flags, the SAVH marker), the 192-byte system options, one 32-byte game-options block
// per slot, the 32-byte global options, then one SGameFileSlot (0xa38) per present slot.
constexpr size_t kSaveHeaderSize = 15;
constexpr size_t kSystemDataSize = 192;
constexpr size_t kGameOptionsSize = 32;
constexpr size_t kGlobalOptionsSize = 32;
constexpr size_t kSlotsStart =
    kSaveHeaderSize + kSystemDataSize + 3 * kGameOptionsSize + kGlobalOptionsSize;
constexpr size_t kSlotStride = 0xa38;
// What a copy holds after its CRC and generation: 0x2000 (mAlignedUserDataSize) - 8.
constexpr size_t kUserDataSize = 0x1ff8;

std::string sUserDirectory;

uint32_t Be32(const uint8_t* p) {
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}

uint32_t Le32(const uint8_t* p) {
  return (uint32_t(p[3]) << 24) | (uint32_t(p[2]) << 16) | (uint32_t(p[1]) << 8) | uint32_t(p[0]);
}

bool ReadAll(const std::string& path, std::vector<uint8_t>& out, std::string& error) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    error = "cannot open " + path;
    return false;
  }
  in.seekg(0, std::ios::end);
  const std::streamoff size = in.tellg();
  in.seekg(0, std::ios::beg);
  if (size < 0) {
    error = "cannot read " + path;
    return false;
  }
  out.resize(size_t(size));
  if (size > 0 && !in.read(reinterpret_cast< char* >(out.data()), size)) {
    error = "cannot read " + path;
    return false;
  }
  return true;
}

// Written beside the target and renamed over it, so a failed write leaves no half file.
bool WriteAll(const std::string& path, const std::vector<uint8_t>& bytes, std::string& error) {
  const std::string temp = path + ".tmp";
  {
    std::ofstream out(temp, std::ios::binary | std::ios::trunc);
    if (!out) {
      error = "cannot write " + path;
      return false;
    }
    if (!bytes.empty()) {
      out.write(reinterpret_cast< const char* >(bytes.data()),
                static_cast< std::streamsize >(bytes.size()));
    }
    out.close();
    if (!out) {
      std::error_code ignored;
      std::filesystem::remove(temp, ignored);
      error = "cannot write " + path;
      return false;
    }
  }
  std::error_code ec;
  std::filesystem::rename(temp, path, ec);
  if (ec) {
    std::filesystem::remove(temp, ec);
    error = "cannot write " + path;
    return false;
  }
  return true;
}

std::string Stamp() {
  const std::time_t now = std::time(nullptr);
  std::tm parts{};
#if defined(_WIN32)
  localtime_s(&parts, &now);
#else
  localtime_r(&now, &parts);
#endif
  char text[32];
  std::strftime(text, sizeof text, "%Y%m%d-%H%M%S", &parts);
  return text;
}

std::string Unique(const std::string& path) {
  std::error_code ec;
  if (!std::filesystem::exists(path, ec)) {
    return path;
  }
  const std::filesystem::path original(path);
  const std::string stem = (original.parent_path() / original.stem()).string();
  const std::string extension = original.extension().string();
  for (int n = 2; n < 100; ++n) {
    const std::string candidate = stem + "-" + std::to_string(n) + extension;
    if (!std::filesystem::exists(candidate, ec)) {
      return candidate;
    }
  }
  return path;
}

std::string CardPathOf(AuroraCardType type) {
  const size_t needed = aurora_card_get_path(GameCode(), type, 0, nullptr, 0);
  if (needed == 0 || needed > 4096) {
    return std::string();
  }
  std::vector<char> buffer(needed + 1, '\0');
  const size_t written =
      aurora_card_get_path(GameCode(), type, 0, buffer.data(), buffer.size());
  if (written == 0 || written > buffer.size()) {
    return std::string();
  }
  buffer[needed] = '\0';
  return std::string(buffer.data());
}

std::string BaseName(const std::string& path) {
  return std::filesystem::path(path).filename().string();
}

// One copy's header and slots. `newest` is the greatest save time among the present slots, which is
// how the newer copy is chosen when both check out (the generations alternate 0/1, so the greater
// number is not the newer one).
struct SCopy {
  bool crc = false;
  bool parsed = false;
  int generation = -1;
  int saveIdx = -1;
  uint32_t newest = 0;
  Slot slots[3];
  std::string why;
};

bool ParseCopy(const uint8_t* user, size_t size, SCopy& out) {
  if (size < kSlotsStart + kSlotStride) {
    out.why = "the save data is too short";
    return false;
  }
  if (std::memcmp(user + 11, "SAVH", 4) != 0) {
    out.why = "the save data has no SAVH header";
    return false;
  }
  out.saveIdx = int(Be32(user + 4));
  const bool present[3] = {user[8] != 0, user[9] != 0, user[10] != 0};
  for (int i = 0; i < 3; ++i) {
    out.slots[i].present = present[i];
    if (!present[i]) {
      continue;
    }
    const uint8_t* slot = user + kSlotsStart + size_t(i) * kSlotStride;
    if (std::memcmp(slot, "GMST", 4) != 0) {
      out.why = "a slot does not start with GMST";
      return false;
    }
    const CGameState::GameFileStateInfo info = CGameState::LoadGameFileState(slot);
    Slot& into = out.slots[i];
    into.worldId = info.mMlvlId;
    into.playTime = info.mPlayTime;
    into.health = info.mHealth;
    into.energyTanks = info.mEnergyTanks;
    into.timestamp = info.mTimestamp;
    into.itemPercent = info.mItemPercent;
    into.scanPercent = info.mScanPercent;
    into.hardMode = info.mHardMode;
    out.newest = std::max(out.newest, info.mTimestamp);
  }
  out.parsed = true;
  return true;
}

std::vector<std::string> BackupsOf(const std::string& prefix) {
  std::vector<std::string> out;
  const std::string dir = BackupsDirectory();
  if (dir.empty()) {
    return out;
  }
  std::error_code ec;
  for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dir, ec)) {
    if (!entry.is_regular_file(ec)) {
      continue;
    }
    const std::string name = entry.path().filename().string();
    if (name.rfind(prefix, 0) == 0 && entry.path().extension() == ".gci") {
      out.push_back(entry.path().string());
    }
  }
  std::sort(out.begin(), out.end());
  return out;
}

void PruneBackups() {
  std::vector<std::string> backups = BackupsOf("card-");
  for (size_t i = 0; i + size_t(kBackupsKept) < backups.size(); ++i) {
    std::error_code ignored;
    std::filesystem::remove(backups[i], ignored);
  }
}

std::mutex sEmulatorMutex;
std::string sEmulatorFolder;
bool sShareCard = false;

// "USA" or "EUR": the region folder of the mounted disc's game code.
const char* CardRegion() { return port::entry::IsPalRegion(port::entry::CurrentDiscVersion()) ? "EUR" : "USA"; }

// Where Dolphin (and PrimeHack) keep their user folders, in preference order.
std::vector< std::filesystem::path > AutoEmulatorRoots() {
  namespace fs = std::filesystem;
  std::vector< fs::path > roots;
#if defined(PORT_MOBILE)
  // No emulator to find.
#elif defined(_WIN32)
  // Dolphin's own rule: the registry's UserConfigPath, else Documents.
  HKEY key;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Dolphin Emulator", 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS) {
    wchar_t value[MAX_PATH] = {};
    DWORD size = sizeof(value) - sizeof(wchar_t);
    if (RegQueryValueExW(key, L"UserConfigPath", nullptr, nullptr, reinterpret_cast< LPBYTE >(value), &size) ==
            ERROR_SUCCESS &&
        value[0] != 0) {
      roots.emplace_back(value);
    }
    RegCloseKey(key);
  }
  PWSTR documents = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &documents))) {
    roots.push_back(fs::path(documents) / L"Dolphin Emulator");
  }
  CoTaskMemFree(documents);
  if (const char* appdata = std::getenv("APPDATA"); appdata != nullptr) {
    roots.push_back(fs::path(appdata) / "Dolphin Emulator");
  }
#else
  const char* appdata = std::getenv("APPDATA");
  const char* xdg = std::getenv("XDG_DATA_HOME");
  const char* home = port::HomeFolder();
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
    roots.push_back(h / ".var/app/io.github.shiiion.primehack/data/dolphin-emu");
    roots.push_back(h / "Library/Application Support/Dolphin");
  }
#endif
  return roots;
}

// The chosen emulator folder, else the autodetected ones.
std::vector< std::filesystem::path > EmulatorRoots() {
  {
    std::lock_guard< std::mutex > lock(sEmulatorMutex);
    if (!sEmulatorFolder.empty()) {
      return {std::filesystem::path(sEmulatorFolder)};
    }
  }
  return AutoEmulatorRoots();
}

} // namespace

void SetEmulatorFolder(const std::string& path) {
  namespace fs = std::filesystem;
  fs::path picked(path);
  if (!path.empty()) {
    if (!picked.has_filename()) {
      picked = picked.parent_path(); // a trailing slash
    }
    const std::string name = picked.filename().string();
    auto up = [&](int levels) {
      for (int i = 0; i < levels; ++i) {
        picked = picked.parent_path();
      }
    };
    if (name == "Card A" && picked.parent_path().parent_path().filename() == "GC") {
      up(3);
    } else if ((name == "USA" || name == "EUR" || name == "JAP") &&
               picked.parent_path().filename() == "GC") {
      up(2);
    } else if (name == "GC") {
      up(1);
    }
  }
  std::lock_guard< std::mutex > lock(sEmulatorMutex);
  sEmulatorFolder = path.empty() ? std::string() : picked.string();
}

std::string EmulatorFolder() {
  std::lock_guard< std::mutex > lock(sEmulatorMutex);
  return sEmulatorFolder;
}

std::vector< std::string > DetectedEmulatorFolders() {
  std::vector< std::string > found;
  std::error_code ec;
  for (const std::filesystem::path& root : AutoEmulatorRoots()) {
    if (std::filesystem::is_directory(root / "GC", ec)) {
      found.push_back(root.string());
    }
  }
  return found;
}

std::string EmulatorCardFolder() {
  std::error_code ec;
  for (const std::filesystem::path& root : EmulatorRoots()) {
    const std::filesystem::path card = root / "GC" / CardRegion() / "Card A";
    if (std::filesystem::is_directory(card, ec)) {
      return card.string();
    }
  }
  return {};
}

void SetShareEmulatorCard(bool share) {
#if defined(PORT_MOBILE)
  share = false; // no emulator to share with
#endif
  std::lock_guard< std::mutex > lock(sEmulatorMutex);
  sShareCard = share;
}

bool ShareEmulatorCard() {
  std::lock_guard< std::mutex > lock(sEmulatorMutex);
  return sShareCard;
}

std::string SharedCardBase(std::string* why) {
  if (!ShareEmulatorCard()) {
    if (why != nullptr) {
      *why = "Sharing is off.";
    }
    return {};
  }
  std::error_code ec;
  const std::vector< std::filesystem::path > roots = EmulatorRoots();
  for (const std::filesystem::path& root : roots) {
    if (std::filesystem::is_directory(root / "GC" / CardRegion() / "Card A", ec)) {
      return (root / "GC").string();
    }
  }
  if (why != nullptr) {
    *why = roots.empty()
               ? "No Dolphin or PrimeHack folder found: choose it above."
               : std::string("The emulator folder has no GCI-folder card for this region (") +
                     CardRegion() +
                     "): set its Slot A to \"GCI Folder\" and start the game once there.";
  }
  return {};
}

void Initialize(const std::string& userDirectory) { sUserDirectory = userDirectory; }

std::string Directory() { return CardPathOf(AURORA_CARD_GCI_DIRECTORY); }

std::string ImagePath() { return CardPathOf(AURORA_CARD_RAW_IMAGE); }

bool IsImage() { return aurora_card_get_type(0) == AURORA_CARD_RAW_IMAGE; }

std::string BackupsDirectory() {
  std::string user = sUserDirectory;
  if (user.empty()) {
    // `<user>/USA/Card A` is two levels below the user directory Aurora was configured with.
    const std::string dir = Directory();
    if (!dir.empty()) {
      const std::filesystem::path card(dir);
      if (card.parent_path().has_parent_path()) {
        user = card.parent_path().parent_path().string();
      }
    }
  }
  return user.empty() ? std::string() : user + "/save-backups";
}

bool ReadFile(const std::string& path, std::vector<uint8_t>& bytes, std::string& error) {
  return ReadAll(path, bytes, error);
}

bool Inspect(const std::string& path, File& out, std::string& error) {
  std::vector<uint8_t> bytes;
  if (!ReadAll(path, bytes, error)) {
    return false;
  }
  if (bytes.size() < kDirEntry + 3 * kBlock) {
    error = path + ": not a save file (too short for a directory entry and three blocks)";
    return false;
  }
  out = File();
  out.path = path;
  out.game.assign(reinterpret_cast< const char* >(bytes.data()), 4);
  out.maker.assign(reinterpret_cast< const char* >(bytes.data()) + 4, 2);
  if (out.maker != "01") {
    error = path + ": maker code '" + out.maker + "', not 01";
    return false;
  }
  if (out.game != "G2ME" && out.game != "G2MP" && out.game != "R3ME" && out.game != "R3MP") {
    error = path + ": game code '" + out.game + "', not this game's save (G2ME, G2MP, R3ME or R3MP)";
    return false;
  }
  const size_t body = bytes.size() - kDirEntry;
  if (body % kBlock != 0) {
    error = path + ": not a whole number of 0x2000 blocks";
    return false;
  }
  const size_t blocks = body / kBlock;
  if (blocks < 3 || (blocks - 1) % kCopies != 0) {
    error = path + ": " + std::to_string(blocks) + " blocks, not a header block and two copies";
    return false;
  }
  const size_t copyBlocks = (blocks - 1) / kCopies;
  if (copyBlocks * kBlock < kBlock) {
    error = path + ": a copy is smaller than one block";
    return false;
  }
  out.copies = kCopies;

  SCopy copies[kCopies];
  for (int i = 0; i < kCopies; ++i) {
    const uint8_t* block = bytes.data() + kDirEntry + kBlock + size_t(i) * copyBlocks * kBlock;
    const size_t copyBytes = copyBlocks * kBlock;
    const uint32_t crc = CCRC32::Calculate(block + 4, uint(copyBytes - 4), 0xFFFFFFFFu);
    // The generation and CRC are host-endian in a save this port wrote (BuildSaveSlot assigns them
    // through a uint*) and big-endian in one a console or Dolphin wrote; a match either way is a
    // valid copy, and the byte order that matched says how to read the generation.
    const bool little = crc == Le32(block);
    const bool big = crc == Be32(block);
    copies[i].crc = little || big;
    copies[i].generation = little ? int(Le32(block + 4)) : int(Be32(block + 4));
    if (!copies[i].crc) {
      copies[i].why = "CRC mismatch";
      continue;
    }
    ParseCopy(block + 8, std::min(copyBytes - 8, kUserDataSize), copies[i]);
  }

  int chosen = -1;
  for (int i = 0; i < kCopies; ++i) {
    if (!copies[i].crc || !copies[i].parsed) {
      continue;
    }
    if (chosen < 0 || copies[i].newest > copies[chosen].newest) {
      chosen = i;
    }
  }
  if (chosen < 0) {
    const std::string why = copies[0].why.empty() ? copies[1].why : copies[0].why;
    error = path + ": no readable save copy (" + (why.empty() ? "both copies are bad" : why) + ")";
    return false;
  }

  out.copy = chosen;
  out.copyGeneration = copies[chosen].generation;
  out.saveIdx = copies[chosen].saveIdx;
  for (int i = 0; i < 3; ++i) {
    out.slots[i] = copies[chosen].slots[i];
  }
  out.note = "copy " + std::to_string(chosen + 1) + " of " + std::to_string(kCopies) +
             " (generation " + std::to_string(copies[chosen].generation) + ")";
  const int other = chosen == 0 ? 1 : 0;
  if (!copies[other].crc) {
    out.note += "; the other copy's CRC is bad";
  } else if (!copies[other].parsed) {
    out.note += "; the other copy does not parse (" + copies[other].why + ")";
  }
  return true;
}

std::vector<std::string> Files() {
  std::vector<std::string> out;
  const std::string dir = Directory();
  if (dir.empty()) {
    return out;
  }
  std::error_code ec;
  for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dir, ec)) {
    if (!entry.is_regular_file(ec)) {
      continue;
    }
    const std::string path = entry.path().string();
    // The directory entry, not the file name: a card folder can hold other games' files too.
    std::ifstream in(path, std::ios::binary);
    char head[6];
    if (!in.read(head, sizeof head)) {
      continue;
    }
    const bool game = std::memcmp(head, "G2ME", 4) == 0 || std::memcmp(head, "G2MP", 4) == 0 ||
                      std::memcmp(head, "R3ME", 4) == 0 || std::memcmp(head, "R3MP", 4) == 0;
    if (game && head[4] == '0' && head[5] == '1') {
      out.push_back(path);
    }
  }
  std::sort(out.begin(), out.end());
  return out;
}

bool Export(const std::string& destination, std::string& message) {
  const std::vector<std::string> files = Files();
  if (files.empty()) {
    message = "the card has no save file (is it mounted? `save info` reads a file directly)";
    return false;
  }
  std::error_code ec;
  std::string dir = destination;
  const bool toDirectory =
      dir.empty() || dir.back() == '/' || std::filesystem::is_directory(dir, ec);
  if (!dir.empty() && dir.back() == '/') {
    dir.pop_back();
  }
  if (!toDirectory && files.size() > 1) {
    message = "the card holds " + std::to_string(files.size()) +
              " save files; give a directory to export them into";
    return false;
  }
  if (toDirectory) {
    if (dir.empty()) {
      dir = BackupsDirectory();
      if (dir.empty()) {
        message = "no user directory to export into; give a path";
        return false;
      }
    }
    std::filesystem::create_directories(dir, ec);
  }

  std::string written;
  for (size_t i = 0; i < files.size(); ++i) {
    std::vector<uint8_t> bytes;
    std::string error;
    if (!ReadAll(files[i], bytes, error)) {
      message = error;
      return false;
    }
    std::string target;
    if (toDirectory) {
      // A directory named explicitly keeps the card's own file name, so it can be imported back as
      // it is; the default backups folder gets a stamped name instead.
      target = destination.empty() ? dir + "/export-" + Stamp() + ".gci"
                                   : dir + "/" + BaseName(files[i]);
      target = Unique(target);
    } else {
      target = dir;
    }
    if (!WriteAll(target, bytes, error)) {
      message = error;
      return false;
    }
    written += (written.empty() ? "" : ", ") + target;
  }
  message = "exported " + std::to_string(files.size()) + " file(s): " + written;
  return true;
}

namespace {
std::atomic<bool> sCardChanged{false};
} // namespace

void MarkCardChanged() { sCardChanged = true; }

bool RemountIfChanged() {
  if (!sCardChanged.exchange(false)) {
    return false;
  }
  aurora_card_remount(0);
  return true;
}

bool Import(const std::string& source, std::string& message) {
  std::string path = source;
  if (path == "latest") {
    const std::vector<std::string> backups = BackupsOf("card-");
    if (backups.empty()) {
      message = "no card backup to import";
      return false;
    }
    path = backups.back();
  } else if (path.find('/') == std::string::npos) {
    const std::string backups = BackupsDirectory();
    std::error_code ec;
    if (!backups.empty() && std::filesystem::exists(backups + "/" + path, ec)) {
      path = backups + "/" + path;
    }
  }

  std::vector<uint8_t> bytes;
  std::string error;
  if (!ReadAll(path, bytes, error)) {
    message = error;
    return false;
  }
  File info;
  if (!Inspect(path, info, error)) {
    message = error;
    return false;
  }
  std::string converted;
  if (info.game != GameCode()) {
    // The card is mounted for one game code (CARDInit's), and the save carries its release's SAVW
    // layout. G2ME and G2MP are converted (PortSave::Convert, docs/SAVE_RELEASES.md); Trilogy's
    // saves are a different format and stay refused.
    if (!Convert::CanConvert(info.game, GameCode())) {
      message = path + " is a " + info.game + " save; this card is " + GameCode() +
                ", and only G2ME and G2MP saves convert";
      return false;
    }
    Convert::Report report;
    std::vector<uint8_t> out;
    if (!Convert::Gci(bytes, GameCode(), out, report, error)) {
      message = path + ": cannot convert " + info.game + " to " + GameCode() + ": " + error;
      return false;
    }
    bytes.swap(out);
    converted = " (converted: " + report.Summary() + ")";
  }

  if (IsImage()) {
    const std::string image = ImagePath();
    if (image.empty()) {
      message = "the card's image is not reachable";
      return false;
    }
    if (!aurora_card_raw_insert(image.c_str(), bytes.data(), bytes.size(), true)) {
      message = "Aurora could not write " + image;
      return false;
    }
    MarkCardChanged();
    message = "imported " + path + " into " + image + converted;
    return true;
  }

  const std::string dir = Directory();
  if (dir.empty()) {
    message = "the card folder is not reachable";
    return false;
  }
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  // Whatever is there now is copied into the backups folder first, never deleted: an import that
  // replaces the wrong save is recoverable.
  const std::vector<std::string> existing = Files();
  std::string target = dir + "/01-" + GameCode() + "-" + kSaveFileName + ".gci";
  std::string kept;
  if (!existing.empty()) {
    target = existing.front();
    const std::string backups = BackupsDirectory();
    if (!backups.empty()) {
      std::filesystem::create_directories(backups, ec);
      const std::string keep = Unique(backups + "/replaced-" + Stamp() + ".gci");
      std::vector<uint8_t> old;
      std::string ignored;
      if (ReadAll(target, old, ignored) && WriteAll(keep, old, ignored)) {
        kept = keep;
      }
    }
  }
  if (!WriteAll(target, bytes, error)) {
    message = error;
    return false;
  }
  MarkCardChanged();
  message = "imported " + path + " as " + target + converted;
  if (!kept.empty()) {
    message += " (the file it replaced is " + kept + ")";
  }
  return true;
}

namespace {
struct Release {
  const char* code;
  const char* label;
  const char* region;
};
constexpr Release kReleases[] = {
    {"G2ME", "Metroid Prime 2: Echoes (USA GameCube)", "USA"},
    {"G2MP", "Metroid Prime 2: Echoes (PAL GameCube)", "EUR"},
};

// The .gci files of `folder` whose directory entry is `code`, with their write times.
std::vector<std::pair<std::string, int64_t>> FilesOfCode(const std::filesystem::path& folder,
                                                          const char* code) {
  std::vector<std::pair<std::string, int64_t>> out;
  std::error_code ec;
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(folder, ec)) {
    if (!entry.is_regular_file(ec)) {
      continue;
    }
    std::ifstream in(entry.path(), std::ios::binary);
    char head[6];
    if (!in.read(head, sizeof head) || std::memcmp(head, code, 4) != 0 || head[4] != '0' ||
        head[5] != '1') {
      continue;
    }
    const auto time = std::filesystem::last_write_time(entry.path(), ec);
    out.emplace_back(entry.path().string(),
                     std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count());
  }
  return out;
}

std::string RestoreMarker(const std::string& folder) {
  return folder + "/.restore-offered-" + GameCode();
}
} // namespace

std::vector<OtherRelease> OtherReleaseSaves() {
  std::vector<OtherRelease> found;
  if (IsImage()) {
    return found;
  }
  std::filesystem::path folder = Directory();
  if (folder.empty()) {
    return found;
  }
  if (!folder.has_filename()) {
    folder = folder.parent_path(); // a trailing separator
  }
  // <base>/<region>/Card A: the other region's card sits beside this one.
  const std::filesystem::path base = folder.parent_path().parent_path();
  for (const Release& release : kReleases) {
    if (std::strncmp(release.code, GameCode(), 4) == 0) {
      continue;
    }
    const std::filesystem::path source = base / release.region / folder.filename();
    OtherRelease other{source.string(), release.code, release.label, std::string(), 0};
    for (const auto& file : FilesOfCode(source, release.code)) {
      if (other.file.empty() || file.second > other.written) {
        other.file = file.first;
        other.written = file.second;
      }
    }
    if (!other.file.empty()) {
      found.push_back(std::move(other));
    }
  }
  std::sort(found.begin(), found.end(),
            [](const OtherRelease& a, const OtherRelease& b) { return a.written > b.written; });
  return found;
}

bool RestoreFrom(const OtherRelease& source, std::string& message) {
  if (source.file.empty()) {
    message = "nothing to restore";
    return false;
  }
  return Import(source.file, message);
}

std::vector<OtherRelease> RestoreToOffer() {
  const std::string folder = IsImage() ? std::string() : Directory();
  std::error_code ec;
  if (folder.empty() || !FilesOfCode(folder, GameCode()).empty() ||
      std::filesystem::exists(RestoreMarker(folder), ec)) {
    return {};
  }
  return OtherReleaseSaves();
}

void MarkRestoreOffered() {
  const std::string folder = IsImage() ? std::string() : Directory();
  if (!folder.empty()) {
    std::ofstream(RestoreMarker(folder)) << "asked\n";
  }
}

bool Backup(std::string* written, std::string& error) {
  const std::vector<std::string> files = Files();
  if (files.empty()) {
    error = "the card has no save file to copy";
    return false;
  }
  const std::string dir = BackupsDirectory();
  if (dir.empty()) {
    error = "no user directory for the backups folder";
    return false;
  }
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  std::vector<uint8_t> bytes;
  const std::string target = Unique(dir + "/card-" + Stamp() + ".gci");
  if (!ReadAll(files.front(), bytes, error) || !WriteAll(target, bytes, error)) {
    return false;
  }
  PruneBackups();
  if (written != nullptr) {
    *written = target;
  }
  return true;
}

std::vector<std::string> Backups() { return BackupsOf("card-"); }

void BeforeCardWrite(const char* fileName) {
  if (!PortOverlay::CardBackup()) {
    return;
  }
  std::string written;
  std::string error;
  if (Backup(&written, error)) {
    std::fprintf(stderr, "[save] card backup before the write to '%s': %s\n",
                 fileName != nullptr ? fileName : "?", written.c_str());
  } else {
    std::fprintf(stderr, "[save] card backup failed: %s\n", error.c_str());
  }
}

// ---- Metroid Prime Trilogy's save.bin (port_save_trilogy.h) ----------------------------------------
// MP1's PortGci::ExportTrilogy / SyncTrilogy / SyncTrilogyShare. The container is port_save_trilogy.cpp and the
// slot stream conversion (G2ME .gci <-> the bin's parts A/B, by asset ID) is port_save_trilogy_payload.cpp.
// An import needs a card save to take the framing from; a G2MP (PAL Echoes) card file is refused.

namespace {

bool sMirrorTrilogy = false;

// The Trilogy's title ids: R3ME (USA) and R3MP (PAL).
std::string FindTrilogyBin(bool pal) {
  std::error_code ec;
  for (const std::filesystem::path& root : EmulatorRoots()) {
    const std::filesystem::path bin =
        root / "Wii/title/00010000" / (pal ? "52334d50" : "52334d45") / "data/save.bin";
    if (std::filesystem::is_regular_file(bin, ec)) {
      return bin.string();
    }
  }
  return {};
}

bool IsPal() { return port::entry::IsPalRegion(port::entry::CurrentDiscVersion()); }

// The card's own G2ME/G2MP files, newest first by modification time.
std::vector<std::string> GameFilesNewest() {
  std::vector<std::string> out;
  for (const std::string& path : Files()) {
    std::ifstream in(path, std::ios::binary);
    char head[3];
    if (in.read(head, sizeof head) && std::memcmp(head, "G2M", 3) == 0) {
      out.push_back(path);
    }
  }
  std::sort(out.begin(), out.end(), [](const std::string& a, const std::string& b) {
    std::error_code ec;
    return std::filesystem::last_write_time(a, ec) > std::filesystem::last_write_time(b, ec);
  });
  return out;
}

} // namespace

void SetMirrorTrilogy(bool mirror) {
#if defined(PORT_MOBILE)
  mirror = false;
#endif
  std::lock_guard<std::mutex> lock(sEmulatorMutex);
  sMirrorTrilogy = mirror;
}

bool MirrorTrilogy() {
  std::lock_guard<std::mutex> lock(sEmulatorMutex);
  return sMirrorTrilogy;
}

std::string FindEmulatorTrilogy() { return FindTrilogyBin(IsPal()); }

bool ExportTrilogy(const std::string& saveBin, std::string& message) {
  const std::vector<std::string> files = GameFilesNewest();
  if (files.empty()) {
    message = "the card has no save file to export";
    return false;
  }
  std::vector<uint8_t> base;
  if (!ReadAll(saveBin, base, message)) {
    message += " (start the Trilogy once so that it makes its save)";
    return false;
  }
  if (!Trilogy::Validate(base.data(), base.size(), message)) {
    return false;
  }
  // The Trilogy shows one Echoes file per Mii, the first whose summary carries the Mii's id, which
  // only the Trilogy can write: the newest slot goes into a file a Mii owns.
  const int target = Trilogy::TargetFile(base.data());
  if (target < 0) {
    message = "the Trilogy save has no Metroid Prime file yet: start Metroid Prime in the Trilogy once first";
    return false;
  }
  // The newest of the card's slots, by the save time the game stored.
  File best;
  int slot = -1;
  std::vector<uint8_t> gci;
  for (const std::string& path : files) {
    File info;
    std::string error;
    if (!Inspect(path, info, error)) {
      continue;
    }
    for (int i = 0; i < 3; ++i) {
      if (info.slots[i].present && (slot < 0 || info.slots[i].timestamp > best.slots[slot].timestamp)) {
        best = info;
        slot = i;
      }
    }
  }
  if (slot < 0) {
    message = "the card's save has no slots";
    return false;
  }
  if (!ReadAll(best.path, gci, message)) {
    return false;
  }
  Trilogy::Payload payload;
  if (!Trilogy::PayloadFromGci(gci, slot, payload.partA, payload.partB, message)) {
    return false;
  }
  const Slot& s = best.slots[slot];
  payload.summary.difficulty = s.hardMode ? 1 : 0;
  payload.summary.percent = s.itemPercent;
  payload.summary.playTime = s.playTime;
  payload.summary.tanks = s.energyTanks;
  payload.summary.mlvl = s.worldId;
  std::vector<uint8_t> out;
  if (!Trilogy::Write(base, target, payload, out, message)) {
    return false;
  }
  const std::string backup = saveBin + ".bak";
  std::error_code ec;
  std::filesystem::copy_file(saveBin, backup, std::filesystem::copy_options::overwrite_existing, ec);
  if (ec) {
    message = "cannot back up " + saveBin + ": " + ec.message();
    return false;
  }
  if (!WriteAll(saveBin, out, message)) {
    return false;
  }
  message = "slot " + std::to_string(slot + 1) + " of " + best.path + " written into Trilogy file " +
            std::to_string(target + 1) + " of " + saveBin + "; the previous save is kept as " +
            std::filesystem::path(backup).filename().string();
  return true;
}

// Brings the Trilogy's file into the card: the converter updates the card's newest file (or makes
// one), the card file is backed up first, and Aurora re-reads the card.
static bool ImportTrilogy(const std::string& saveBin, std::string& message) {
  std::vector<uint8_t> bin;
  if (!ReadAll(saveBin, bin, message) || !Trilogy::Validate(bin.data(), bin.size(), message)) {
    return false;
  }
  const int file = Trilogy::TargetFile(bin.data());
  Trilogy::Payload payload;
  if (file < 0 || !Trilogy::Read(bin.data(), file, payload)) {
    message = "the Trilogy save has no Echoes progress";
    return false;
  }
  const std::string dir = Directory();
  if (dir.empty() || IsImage()) {
    message = "the card is not a GCI folder";
    return false;
  }
  const std::vector<std::string> existing = GameFilesNewest();
  std::vector<uint8_t> gci;
  if (!existing.empty() && !ReadAll(existing.front(), gci, message)) {
    return false;
  }
  if (!Trilogy::GciFromPayload(payload, gci, message)) {
    return false;
  }
  std::string backup;
  if (!existing.empty()) {
    Backup(&backup, message);
  }
  const std::string target = existing.empty()
                                 ? dir + "/01-" + port::entry::CurrentGameCode() + "-" + kSaveFileName + ".gci"
                                 : existing.front();
  if (!WriteAll(target, gci, message)) {
    return false;
  }
  aurora_card_remount(0);
  message = "Trilogy file " + std::to_string(file + 1) + " imported as " + target;
  return true;
}

std::string TrilogyShareBin() {
  if (!ShareEmulatorCard() || !MirrorTrilogy()) {
    return {};
  }
  return FindTrilogyBin(IsPal());
}

// MP1's SyncTrilogy: the newer of the card and the bin wins, and an equal modification time means
// in step. What a sync failed on is remembered, so a save the converter cannot take is not retried
// every check.
bool SyncTrilogyShare(bool pull) {
  static std::chrono::steady_clock::time_point sNext{};
  static std::filesystem::file_time_type sFailedCard{};
  static std::filesystem::file_time_type sFailedBin{};
  const auto now = std::chrono::steady_clock::now();
  if (now < sNext) {
    return false;
  }
  sNext = now + std::chrono::milliseconds(500);
  const std::string bin = TrilogyShareBin();
  if (bin.empty()) {
    return false;
  }
  std::error_code ec;
  const std::filesystem::file_time_type binTime = std::filesystem::last_write_time(bin, ec);
  if (ec) {
    return false;
  }
  const std::vector<std::string> files = GameFilesNewest();
  std::filesystem::file_time_type cardTime = std::filesystem::file_time_type::min();
  for (const std::string& file : files) {
    cardTime = std::max(cardTime, std::filesystem::last_write_time(file, ec));
  }
  std::string message;
  if (!files.empty() && cardTime > binTime && cardTime != sFailedCard) {
    if (!ExportTrilogy(bin, message)) {
      std::fprintf(stderr, "[save] Trilogy mirror: export failed: %s\n", message.c_str());
      sFailedCard = cardTime;
      return false;
    }
    std::filesystem::last_write_time(bin, cardTime, ec); // equal times mean in step
    return false;
  }
  if (!pull || binTime <= cardTime || binTime == sFailedBin) {
    return false;
  }
  if (!ImportTrilogy(bin, message)) {
    std::fprintf(stderr, "[save] Trilogy mirror: import failed: %s\n", message.c_str());
    sFailedBin = binTime;
    return false;
  }
  for (const std::string& file : GameFilesNewest()) {
    std::filesystem::last_write_time(file, binTime, ec);
  }
  return true;
}

} // namespace PortSave
