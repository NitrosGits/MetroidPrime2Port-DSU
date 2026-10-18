#include "port_save.h"

#include "port_overlay.h"

#include "Kyoto/CCrc32.hpp"
#include "MetroidPrime/Player/CGameState.hpp"

#include <aurora/card.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>

namespace PortSave {

namespace {

// The card the port mounts: CMemoryCardSys::Initialize calls CARDInit("G2ME", "01"), and Aurora
// resolves that to `<user>/USA/Card A` (GCI folder) or its raw image. A PAL disc's save is G2MP;
// this tree builds one card for the code it was compiled with, so the lookup stays G2ME - a card of
// the other release is refused by name in Inspect rather than read with the wrong world layout.
constexpr const char* kGameCode = "G2ME";
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
  const size_t needed = aurora_card_get_path(kGameCode, type, 0, nullptr, 0);
  if (needed == 0 || needed > 4096) {
    return std::string();
  }
  std::vector<char> buffer(needed + 1, '\0');
  const size_t written =
      aurora_card_get_path(kGameCode, type, 0, buffer.data(), buffer.size());
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

} // namespace

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
  if (out.game != "G2ME" && out.game != "G2MP") {
    error = path + ": game code '" + out.game + "', not this game's save (G2ME or G2MP)";
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
    const bool game = std::memcmp(head, "G2ME", 4) == 0 || std::memcmp(head, "G2MP", 4) == 0;
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
  if (info.game != kGameCode) {
    // The card is mounted for one game code (CARDInit's), and MP2's save carries the world table's
    // SAVW hashes and area data of its own release: a converted save is not something to guess at.
    message = path + " is a " + info.game + " save; this card is " + kGameCode +
              ", and region conversion is not supported";
    return false;
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
    aurora_card_remount(0);
    message = "imported " + path + " into " + image;
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
  std::string target = dir + "/01-" + info.game + "-" + kSaveFileName + ".gci";
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
  aurora_card_remount(0);
  message = "imported " + path + " as " + target;
  if (!kept.empty()) {
    message += " (the file it replaced is " + kept + ")";
  }
  return true;
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

} // namespace PortSave
