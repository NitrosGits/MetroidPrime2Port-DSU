#include "port_utf8_path.h"
#include "port_disc_replace.h"

#include "port_entry.h"
#include "port_overlay_internal.h"
#include "port_settings.h"

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <thread>

namespace PortDiscReplace {

namespace {

std::mutex sMutex;
std::mutex sThreadMutex; // Start and Shutdown
Status sStatus;
bool sApplied = true;
std::thread sThread;
std::atomic< bool > sCancel{false};
std::atomic< uint64_t > sDone{0};
std::atomic< uint64_t > sTotal{0};

bool IsContentUri(const std::string& s) { return s.compare(0, 10, "content://") == 0; }

// Says what a wrong plain image is, from its first six bytes (the game id).
std::string DescribeDisc(const char* id6) {
  char id[7] = {};
  for (int i = 0; i < 6; ++i) {
    const char c = id6[i];
    id[i] = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ? c : '?';
  }
  if (std::memcmp(id, "GM8", 3) == 0) {
    return std::string("This is Metroid Prime (") + id + "), not Metroid Prime 2: Echoes.";
  }
  if (std::memcmp(id, "G2M", 3) == 0 || std::memcmp(id, "R3M", 3) == 0) {
    return std::string("This release of Metroid Prime 2: Echoes (") + id +
           ") is not one the port runs (USA 1.00, PAL or Metroid Prime Trilogy, revision 0, disc 0).";
  }
  return std::string("This disc image is not Metroid Prime 2: Echoes (game id ") + id + ").";
}

const char* PlainName(const char* id6) {
  if (std::memcmp(id6, "G2ME01", 6) == 0) {
    return "USA 1.00";
  }
  if (std::memcmp(id6, "G2MP01", 6) == 0) {
    return "PAL";
  }
  return std::memcmp(id6, "R3ME01", 6) == 0 ? "Metroid Prime Trilogy (USA)" : "Metroid Prime Trilogy (PAL)";
}

void Finish(State state, const std::string& message, const std::string& path = {}, const std::string& revision = {}) {
  std::lock_guard lock(sMutex);
  sStatus.state = state;
  sStatus.copying = false;
  sStatus.message = message;
  sStatus.path = path;
  sStatus.revision = revision;
  sApplied = state != State::Done;
}

void Run(std::string picked) {
  std::string revision;
  if (const std::string bad = CheckImage(picked, &revision); !bad.empty()) {
    std::fprintf(stderr, "port: not using the picked disc image: %s\n", bad.c_str());
    return Finish(State::Failed, bad);
  }
  std::string path = picked;
  // Only a content:// address needs the copy; a path is remembered as it is.
  if (IsContentUri(picked)) {
    {
      std::lock_guard lock(sMutex);
      sStatus.copying = true;
    }
    if (const std::string bad = CopyToUserFolder(picked, &path, &sDone, &sTotal, &sCancel); !bad.empty()) {
      return Finish(State::Failed, sCancel ? "Cancelled; the disc image is unchanged." : bad);
    }
  }
  Finish(State::Done, "Restart the game to use the new disc image.", path, revision);
}

std::filesystem::path UserFolder() {
  const std::string& file = PortSettings::FilePath();
  return file.empty() ? std::filesystem::path() : PortUtf8::FromUtf8(file).parent_path();
}

} // namespace

std::string CheckImage(const std::string& path, std::string* revision) {
  uint8_t header[8] = {};
  SDL_IOStream* io = SDL_IOFromFile(path.c_str(), "rb");
  if (io == nullptr) {
    return "The file can't be opened.";
  }
  const size_t got = SDL_ReadIO(io, header, sizeof(header));
  SDL_CloseIO(io);
  if (got != sizeof(header)) {
    return "The file is too short to be a disc image.";
  }
  const char* h = reinterpret_cast< const char* >(header);
  if (port::entry::IsPlainDiscHeader(h, sizeof(header))) {
    if (revision != nullptr) {
      *revision = PlainName(h);
    }
    return {};
  }
  if (port::entry::IsDiscContainerMagic(h, sizeof(header))) {
    // The disc ID of an RVZ, WIA, GCZ, CISO or WBFS file is only visible once nod opens it; the
    // mount at the next start verifies it and asks for another image if it is wrong.
    if (revision != nullptr) {
      *revision = "compressed image, checked when mounted";
    }
    return {};
  }
  return DescribeDisc(h);
}

std::string CopyToUserFolder(const std::string& source, std::string* target, std::atomic< uint64_t >* doneOut,
                             std::atomic< uint64_t >* totalOut, const std::atomic< bool >* cancel) {
  const std::filesystem::path folder = UserFolder();
  if (folder.empty()) {
    std::fprintf(stderr, "port: no data folder to copy the disc into\n");
    return "There is no data folder to copy the disc image into.";
  }
  const std::filesystem::path destination = folder / "disc.iso";
  // Copied under another name and renamed when complete: a copy killed part way must not leave a
  // truncated disc.iso that the next start would then prefer.
  const std::filesystem::path partial = folder / "disc.iso.part";
  const std::string partialName = PortUtf8::ToUtf8(partial);

  SDL_IOStream* in = SDL_IOFromFile(source.c_str(), "rb");
  if (in == nullptr) {
    std::fprintf(stderr, "port: could not read the picked image: %s: %s\n", source.c_str(), SDL_GetError());
    return "The picked image can't be read.";
  }
  // A plain image names its game in the first bytes: refuse a wrong one before minutes of copying.
  Uint8 header[8] = {};
  if (SDL_ReadIO(in, header, sizeof(header)) == sizeof(header)) {
    const char* h = reinterpret_cast< const char* >(header);
    if (!port::entry::IsPickableDiscHeader(h, sizeof(header))) {
      const std::string why = DescribeDisc(h);
      std::fprintf(stderr, "port: not copying the picked image: %s\n", why.c_str());
      SDL_CloseIO(in);
      return why;
    }
  }
  if (SDL_SeekIO(in, 0, SDL_IO_SEEK_SET) != 0) {
    // Some providers hand out a pipe, which cannot seek: start over.
    SDL_CloseIO(in);
    in = SDL_IOFromFile(source.c_str(), "rb");
    if (in == nullptr) {
      std::fprintf(stderr, "port: could not reopen the picked image: %s\n", SDL_GetError());
      return "The picked image can't be read.";
    }
  }
  const Sint64 total = SDL_GetIOSize(in);
  if (totalOut != nullptr) {
    totalOut->store(total > 0 ? static_cast< uint64_t >(total) : 0);
  }
  if (doneOut != nullptr) {
    doneOut->store(0);
  }
  SDL_IOStream* out = SDL_IOFromFile(partialName.c_str(), "wb");
  if (out == nullptr) {
    std::fprintf(stderr, "port: could not create %s: %s\n", partialName.c_str(), SDL_GetError());
    SDL_CloseIO(in);
    return "The copy can't be created in the data folder.";
  }

  std::string failure;
  char buffer[1 << 16];
  Sint64 done = 0;
  int lastPercent = -1;
  for (;;) {
    if (cancel != nullptr && cancel->load()) {
      failure = "cancelled";
      break;
    }
    const size_t got = SDL_ReadIO(in, buffer, sizeof(buffer));
    if (got == 0) {
      // Zero is also what a failed read returns; only the stream status tells end of file from an
      // error that would leave a truncated copy.
      if (SDL_GetIOStatus(in) != SDL_IO_STATUS_EOF) {
        std::fprintf(stderr, "port: reading the picked image failed: %s\n", SDL_GetError());
        failure = "Reading the picked image failed.";
      }
      break;
    }
    if (SDL_WriteIO(out, buffer, got) != got) {
      std::fprintf(stderr, "port: writing %s failed: %s\n", partialName.c_str(), SDL_GetError());
      failure = "Writing the copy failed; the device may be full.";
      break;
    }
    done += static_cast< Sint64 >(got);
    if (doneOut != nullptr) {
      doneOut->store(static_cast< uint64_t >(done));
    }
    if (total > 0) {
      const int percent = static_cast< int >(done * 100 / total);
      // Every 5% rather than every chunk: a 1.5 GB image would otherwise put 3000 lines in the log.
      if (percent / 5 != lastPercent / 5) {
        lastPercent = percent;
        std::fprintf(stderr, "port: copying the disc image, %d%%\n", percent);
      }
    }
  }
  if (!SDL_FlushIO(out) && failure.empty()) {
    std::fprintf(stderr, "port: flushing %s failed: %s\n", partialName.c_str(), SDL_GetError());
    failure = "Writing the copy failed.";
  }
  if (!SDL_CloseIO(in)) {
    std::fprintf(stderr, "port: closing the picked image failed: %s\n", SDL_GetError());
  }
  if (!SDL_CloseIO(out) && failure.empty()) {
    std::fprintf(stderr, "port: closing %s failed: %s\n", partialName.c_str(), SDL_GetError());
    failure = "Writing the copy failed.";
  }
  std::error_code ec;
  if (!failure.empty()) {
    std::filesystem::remove(partial, ec);
    return failure;
  }
  // What landed on disk, not what was handed to the writer: a short write that only fails at close
  // looks identical to a good copy otherwise.
  const auto written = std::filesystem::file_size(partial, ec);
  if (ec || static_cast< Sint64 >(written) != done || (total > 0 && done != total)) {
    std::fprintf(stderr, "port: %s is %lld bytes on disk, copied %lld of %lld\n", partialName.c_str(),
                 static_cast< long long >(written), static_cast< long long >(done), static_cast< long long >(total));
    std::filesystem::remove(partial, ec);
    return "The copy is incomplete.";
  }
  std::filesystem::rename(partial, destination, ec);
  if (ec) {
    std::fprintf(stderr, "port: could not rename %s to %s: %s\n", partialName.c_str(), PortUtf8::ToUtf8(destination).c_str(),
                 ec.message().c_str());
    std::filesystem::remove(partial, ec);
    return "The copy can't be put in place.";
  }
  std::fprintf(stderr, "port: copied the disc image to %s (%lld bytes)\n", PortUtf8::ToUtf8(destination).c_str(),
               static_cast< long long >(done));
  if (target != nullptr) {
    *target = PortUtf8::ToUtf8(destination);
  }
  return {};
}

void Adopt(const std::string& path, const std::string& revision) {
  PortOverlaySettings::SetDiscPath(path);
  // Persist at once: the settings are otherwise written from the overlay's draw path.
  PortOverlaySettings::SaveSettings();
  std::fprintf(stderr, "port: disc image replaced: %s (%s)\n", path.c_str(), revision.c_str());
}

bool Start(const std::string& picked) {
  std::lock_guard threadLock(sThreadMutex);
  {
    std::lock_guard lock(sMutex);
    if (sStatus.state == State::Running) {
      return false;
    }
  }
  if (sThread.joinable()) {
    sThread.join();
  }
  sCancel = false;
  sDone = 0;
  sTotal = 0;
  {
    std::lock_guard lock(sMutex);
    sStatus = Status{};
    sStatus.state = State::Running;
    sApplied = true;
  }
  sThread = std::thread([picked] { Run(picked); });
  return true;
}

void Cancel() { sCancel = true; }

void Shutdown() {
  Cancel();
  std::lock_guard threadLock(sThreadMutex);
  if (sThread.joinable()) {
    sThread.join();
  }
  // A replacement that finished while the menu was closed is still remembered.
  Poll();
}

Status Snapshot() {
  std::lock_guard lock(sMutex);
  Status s = sStatus;
  s.bytesDone = sDone;
  s.bytesTotal = sTotal;
  return s;
}

bool Poll() {
  Status s;
  {
    std::lock_guard lock(sMutex);
    if (sApplied) {
      return false;
    }
    sApplied = true;
    s = sStatus;
  }
  Adopt(s.path, s.revision);
  return true;
}

void PickAsync() {
  {
    std::lock_guard lock(sMutex);
    if (sStatus.state == State::Running) {
      return;
    }
  }
  int windowCount = 0;
  SDL_Window** windows = SDL_GetWindows(&windowCount);
  SDL_Window* window = windows != nullptr && windowCount > 0 ? windows[0] : nullptr;
  SDL_free(windows);
  // Static: SDL reads the filters until the dialog closes.
  static const SDL_DialogFileFilter filters[] = {
      {"Metroid Prime 2: Echoes disc image (iso, gcm, rvz, wia, gcz, wbfs, ciso, nkit)",
       "iso;gcm;rvz;wia;gcz;wbfs;ciso;nkit"},
      {"All files", "*"},
  };
  const SDL_PropertiesID props = SDL_CreateProperties();
  SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_FILTERS_POINTER, const_cast< SDL_DialogFileFilter* >(filters));
  SDL_SetNumberProperty(props, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER, 2);
  SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_WINDOW_POINTER, window);
  SDL_SetStringProperty(props, SDL_PROP_FILE_DIALOG_TITLE_STRING,
                        "Select your Metroid Prime 2: Echoes disc image (GameCube USA or PAL, or Metroid Prime Trilogy)");
  SDL_ShowFileDialogWithProperties(
      SDL_FILEDIALOG_OPENFILE,
      [](void*, const char* const* files, int) {
        if (files == nullptr) {
          Finish(State::Failed, std::string("The file dialog failed: ") + SDL_GetError());
        } else if (files[0] != nullptr) {
          Start(files[0]);
        } // an empty list is a cancel
      },
      nullptr, props);
  SDL_DestroyProperties(props);
}

} // namespace PortDiscReplace
