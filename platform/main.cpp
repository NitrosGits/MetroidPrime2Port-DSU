// Port entry point for Metroid Prime 2: Echoes.
//
// Aurora owns the real process entry, so this is what it calls. The order below
// is the one the Metroid Prime port proves: bring Aurora up, mount the disc,
// verify it is the disc this port is built for, then hand control to the game
// and shut the platform down afterwards.
//
// The game's entry is `InvokeCMain` (src/MetroidPrime/main.cpp:79), which builds
// `CMain` and calls `RsMain`. It takes the OS context, the memory system and the
// graphics system from its caller, and that caller is not decompiled - so the three
// objects are built here in the shape the caller will need. `COsContext`'s and
// `CMemorySys`'s constructors are host work (`src/Kyoto/Basics/COsContext.cpp` and
// the SDK), and `CGraphicsSys`'s is `src/Kyoto/Graphics/CGraphicsHostStartup.cpp`,
// which carries retail's `CGraphics::Startup` chain - so the VI bring-up this
// translation unit used to leave to a `CMain::OpenWindow` stand-in inside
// `RsMain` now happens here, where retail does it. Everything else here (Aurora
// setup, disc mounting and checking, teardown) is settled. See PORT_NOTES.md.

// Aurora owns the real process entry: this header's `#define main aurora_main` is
// what turns the `main` below into the function Aurora's own `main` (and SDL's
// Android activity, by that name) calls. Android needs it for its shared library
// to have the entry point at all; ../MetroidPrimePort includes it the same way.
#include <aurora/main.h>

#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <dolphin/gx/GXAurora.h>

#include <cstdio>
#include <csignal>
#if defined(__ANDROID__)
#include <signal.h>
#include <android/log.h>
#else
#include <execinfo.h>
#endif
#include <unistd.h>
#include <cstdlib>
#include <filesystem>
#include <string>
#if defined(__ANDROID__)
// SDL owns the app's private directories on Android, and the port's own three are resolved
// beneath them here, so the entry needs SDL's headers on Android. The include directory is
// attached to this target in CMakeLists.txt, in the AURORA_SDL3_TARGET block, and nothing
// outside this `#if` includes SDL.
#include <SDL3/SDL.h>
#include <atomic>
#include <cerrno>
#include <fstream>
#include <thread>
#include <sys/ioctl.h>
#endif

#include "Kyoto/Alloc/CMemorySys.hpp"

extern "C" void* OSGetArenaLo();
extern "C" void* OSGetArenaHi();
extern void* gPortGameHeapLo;
extern void* gPortGameHeapHi;
#include "Kyoto/Basics/COsContext.hpp"
#include "Kyoto/Graphics/CGraphicsSys.hpp"
#include "compiled_modules.h"
#include "port_binds.h"
#include "port_build_info.h"
#include "port_console.h"
#include "port_entry.h"
#include "port_save.h"
#include "port_touch.h"
#include "port_tweaks.h"

// The decompilation's exported entry point; see src/MetroidPrime/main.cpp.
namespace port {
void* LoadDefaultFont();
void FreeDefaultFont(void* token);
} // namespace port

extern "C" void InvokeCMain(int argc, char** argv, COsContext* context, void* unk1,
                            CMemorySys* memorySys, void* unk2);

namespace {

const char* kAppName = "Metroid Prime 2: Echoes";

std::string ExecutableDir(const char* argv0) {
  if (argv0 == nullptr || argv0[0] == '\0') {
    return {};
  }
  std::error_code error;
  const std::filesystem::path path = std::filesystem::absolute(argv0, error);
  if (error || !path.has_parent_path()) {
    return {};
  }
  return path.parent_path().string();
}

void ReportNoDisc(const char* argv0) {
  std::fprintf(stderr,
               "metroid_prime2_port: no disc image given.\n"
               "  usage: %s <path to Metroid Prime 2: Echoes (USA) (v1.00).iso>\n"
               "  or set MP2_DISC, or place the image next to the executable.\n",
               argv0 != nullptr ? argv0 : "metroid_prime2_port");
}

// The three directories the port needs: where the game's user data goes, where its cache
// goes, and which disc image to mount. On the desktop all three come from the environment
// (MP2_USER_PATH / MP2_CACHE_PATH / MP2_DISC, or an image beside the executable); an
// installed app has no environment to set, so `ResolveAndroidPaths` fills them in instead
// and the environment only wins over that when it is actually set.
struct PortPaths {
  std::string user;
  std::string cache;
  std::string disc;
};

// An environment variable if it is set and non-empty, otherwise the value resolved for this
// platform (empty on the desktop, where there is nothing to resolve).
const char* EnvOrFallback(const char* name, const std::string& fallback) {
  const char* value = std::getenv(name);
  if (value != nullptr && value[0] != '\0') {
    return value;
  }
  return fallback.empty() ? nullptr : fallback.c_str();
}

#if defined(__ANDROID__)

// ---------------------------------------------------------------------------
// Android: fd 1/2 to logcat, and the port's directories from SDL.
// ---------------------------------------------------------------------------
// Android gives an app no console: fd 1 and fd 2 go nowhere, so every std::fprintf the
// port makes - and the "no disc" message that says where a run stopped - is discarded, and
// a boot that stops at the disc check reads exactly like one that never started. Both
// descriptors are pointed at a pipe here and a thread turns the bytes back into logcat
// lines. This is the small version of ../MetroidPrimePort's PortLog: lines, one tag,
// nothing else (no file, no buffering, no per-frame state). It runs before anything else in
// the entry, so Aurora's own lines arrive too.
constexpr size_t kMaxPendingLine = 8192;

int gLogPipeRead = -1;
std::atomic<bool> gLogPumpIdle{true};

void PortLogLine(const std::string& line) {
  // A whole line at a time, `%.*s` rather than a format string the writer chose.
  __android_log_print(ANDROID_LOG_INFO, "mp2port", "%.*s", static_cast<int>(line.size()),
                      line.c_str());
}

void PortLogPump() {
  std::string pending;
  char buffer[4096];
  for (;;) {
    const ssize_t count = ::read(gLogPipeRead, buffer, sizeof(buffer));
    if (count == 0) {
      break; // the write end is gone: the process is going down
    }
    if (count < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    gLogPumpIdle.store(false);
    pending.append(buffer, static_cast<size_t>(count));
    size_t start = 0;
    for (;;) {
      const size_t newline = pending.find('\n', start);
      if (newline == std::string::npos) {
        break;
      }
      PortLogLine(pending.substr(start, newline - start));
      start = newline + 1;
    }
    pending.erase(0, start);
    // One line is logged as it arrives: a writer that never sends a newline must not make
    // this grow without bound.
    if (pending.size() > kMaxPendingLine) {
      PortLogLine(pending);
      pending.clear();
    }
    gLogPumpIdle.store(true);
  }
  // The pipe is closed and this thread is going away with it: a last line without its
  // newline is still worth logging.
  if (!pending.empty()) {
    PortLogLine(pending);
  }
}

void PortLogBegin() {
  int descriptors[2];
  if (::pipe(descriptors) != 0) {
    return; // no log; a crash still leaves a tombstone
  }
  gLogPipeRead = descriptors[0];
  // What the C library has already buffered belongs in the log rather than in the pipe's
  // future, so both streams are flushed before the descriptors move.
  std::fflush(stdout);
  std::fflush(stderr);
  if (::dup2(descriptors[1], STDOUT_FILENO) < 0 || ::dup2(descriptors[1], STDERR_FILENO) < 0) {
    ::close(descriptors[0]);
    ::close(descriptors[1]);
    gLogPipeRead = -1;
    return;
  }
  ::close(descriptors[1]); // file descriptors 1 and 2 are the writers now
  // stderr unbuffered and stdout line-buffered even when neither is a terminal: a partial
  // line on an abort()ing run is the line worth having.
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  std::setvbuf(stderr, nullptr, _IONBF, 0);
  std::thread(PortLogPump).detach();
}

// Called on every way out of the entry. A pipe cannot be asked how much is still in it, so
// the queue is polled until it is empty and the pump is between reads; bounded, because a
// log line is not worth a hang.
void PortLogDrain() {
  if (gLogPipeRead < 0) {
    return;
  }
  for (int attempt = 0; attempt < 100; ++attempt) {
    int queued = 0;
    if (::ioctl(gLogPipeRead, FIONREAD, &queued) == 0 && queued == 0 && gLogPumpIdle.load()) {
      return;
    }
    ::usleep(2000);
  }
}

// The app's own directories, all three beneath SDL's preference directory - which on Android
// *is* the app's private files directory - as user/, cache/ and disc.iso. That is the shape
// ../MetroidPrimePort settled on (port_paths.h); the difference here is that nothing has
// been migrated yet, so there is only one layout. A `port_paths.txt` beside them overrides
// the three (`user=`, `cache=`, `disc=`), which a debugging push can use to move them, or to
// point at a disc copied into app storage; the Java side writes that file, and the file
// picker that fills in the disc is `EnsureDiscImage` below. A fourth line, `env.NAME=value`,
// sets an environment variable, which is the device equivalent of the desktop harness's
// `MP2_FRAMES=1800 MP2_SKIP_FRONTEND=1 ./metroid_prime2_port` prefix (tools/run_gameplay.sh
// exports MP2_DEBUG_IDS / MP2_SKIP_CINEMATICS / MP2_SKIP_FRONTEND / MP2_DISC; the harness and
// the port read those plus MP2_WORLD and MP2_INPUT from the environment) - an installed app
// has no shell to set them in, so the file is where a device run says them. The assignments
// are made while this function reads the file, which is the top of the entry: before Aurora
// starts and before the game or the harness looks at any of them. SDL is already up when this
// runs: the activity's context is what SDL_GetPrefPath reads, and Aurora itself falls back to
// the same call for its user and cache directories.
PortPaths ResolveAndroidPaths() {
  PortPaths paths;
  char* preference = SDL_GetPrefPath(nullptr, kAppName);
  if (preference == nullptr) {
    std::fprintf(stderr,
                 "metroid_prime2_port: no SDL preference directory; set MP2_USER_PATH, "
                 "MP2_CACHE_PATH and MP2_DISC\n");
    return paths;
  }
  const std::string base = preference;
  SDL_free(preference);

  paths.user = base + "user";
  paths.cache = base + "cache";
  paths.disc = base + "disc.iso";

  std::ifstream overrides(base + "port_paths.txt");
  std::string line;
  while (std::getline(overrides, line)) {
    const size_t first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos || line[first] == '#') {
      continue;
    }
    const size_t last = line.find_last_not_of(" \t\r\n");
    const std::string entry = line.substr(first, last - first + 1);
    const size_t equals = entry.find('=');
    if (equals == std::string::npos) {
      continue;
    }
    const std::string value = entry.substr(equals + 1);
    if (value.empty()) {
      continue;
    }
    const std::string key = entry.substr(0, equals);
    if (key == "user") {
      paths.user = value;
    } else if (key == "cache") {
      paths.cache = value;
    } else if (key == "disc") {
      paths.disc = value;
    } else if (key.starts_with("env.")) {
      // The variables the desktop harness is given on its command line (see the comment
      // above). `NAME` is whatever follows `env.`, and this file is the app's only way to
      // name one, so it overwrites: a value here is meant. An invalid name is left to
      // setenv to reject, and is reported rather than dropped.
      const std::string name = key.substr(4);
      if (::setenv(name.c_str(), value.c_str(), 1) == 0) {
        std::fprintf(stderr, "metroid_prime2_port: port_paths.txt set %s=%s\n", name.c_str(),
                     value.c_str());
      } else {
        std::fprintf(stderr, "metroid_prime2_port: port_paths.txt cannot set %s\n", name.c_str());
      }
    }
  }

  // Created before the game can ask for them, so a first run has somewhere to write.
  std::error_code error;
  std::filesystem::create_directories(paths.user, error);
  error.clear();
  std::filesystem::create_directories(paths.cache, error);

  std::printf("metroid_prime2_port: build %s\n", MP_BUILD_REVISION);
  std::printf("metroid_prime2_port: paths chosen: user=%s cache=%s disc=%s\n", paths.user.c_str(),
              paths.cache.c_str(), paths.disc.c_str());
  return paths;
}

// ---------------------------------------------------------------------------
// Android: the disc image, asked for once and copied into the app's own storage.
// ---------------------------------------------------------------------------
// An Android app has no command line and no image beside the executable, so the only disc it
// can ever have is a file in its own files directory - and a first launch has none. This is
// ../MetroidPrimePort's picker (its `AskForDiscImage` and `CopyDiscFromContentUri`), reduced
// to what this port needs.
//
// The trigger is the disc image the resolution named not being a Metroid Prime 2 image, which
// is where a missing file lands: `ResolveDiscPath` hands back the default `<files>/disc.iso`
// whether or not it exists, so the launch would otherwise reach `aurora_dvd_open` and die
// there with nothing on screen saying what to do about it.
//
// It runs *before* `aurora_initialize`, and that is deliberate: the system picker covers the
// app and destroys its surface, which is why MP1 drops its swapchain by hand while a dialog is
// up (`aurora_release_lost_surface`, which this snapshot of Aurora does not export). Before the
// window exists there is no swapchain to lose, and the copy of a 1.4 GB image - minutes on a
// phone - runs before the renderer is up rather than during a frame.
//
// The pick comes back as a `content://` URI, a permission grant the system can revoke, so the
// image is copied into app storage once and later launches read a plain file with no grant at
// all. The copy goes through a `.part` file and a rename: a copy that is killed part way (the
// app can be killed during a multi-minute copy) must not leave a truncated disc.iso, which the
// next launch would then prefer over asking again.

// The three fields `port::entry::IsSupportedDisc` checks after mounting, read from the head of
// the image instead: "G2ME", "01", version 0, disc 0. Only a plain image (iso, gcm) carries
// them - a container hides them, which is why the picker offers those two and "All files"
// rather than the container formats MP1's loader also reads.
bool SupportedHeader(const char* header, size_t size) {
  return size >= 8 && std::string(header, 6) == "G2ME01" && header[6] == 0 && header[7] == 0;
}

bool DiscImageIsUsable(const std::string& path) {
  if (path.empty()) {
    return false;
  }
  std::ifstream image(path, std::ios::binary);
  if (!image) {
    return false;
  }
  char header[8] = {};
  image.read(header, sizeof(header));
  return SupportedHeader(header, static_cast<size_t>(image.gcount()));
}

// SDL delivers the answer on the activity's thread through JNI, not through the event queue,
// so it is handed back through atomics and this waits for it; nothing pumps events, because
// there is no window yet to deliver a quit. A picker left unanswered ends at the deadline
// rather than hanging a launch with nothing in logcat saying why.
std::string AskForDiscImage() {
  static std::atomic<bool> answered{false};
  static std::atomic<bool> dismissed{false};
  static std::string chosen;
  answered.store(false);
  dismissed.store(false);
  chosen.clear();
  // Static: SDL reads the filters until the dialog closes, which can outlive this call.
  static const SDL_DialogFileFilter filters[] = {
      {"Metroid Prime 2: Echoes disc image (iso, gcm)", "iso;gcm"},
      {"All files", "*"},
  };
  std::fprintf(stderr, "metroid_prime2_port: asking for a disc image\n");
  const SDL_PropertiesID props = SDL_CreateProperties();
  SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_FILTERS_POINTER,
                         const_cast<SDL_DialogFileFilter*>(filters));
  SDL_SetNumberProperty(props, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER, 2);
  SDL_ShowFileDialogWithProperties(
      SDL_FILEDIALOG_OPENFILE,
      [](void*, const char* const* files, int) {
        if (files != nullptr && files[0] != nullptr) {
          chosen = files[0];
        }
        // An empty list is a cancel (Back on Android); a null one is SDL reporting an error.
        dismissed.store(files != nullptr && files[0] == nullptr);
        answered.store(true);
      },
      nullptr, props);
  SDL_DestroyProperties(props);
  const Uint64 deadline = SDL_GetTicks() + 5 * 60 * 1000;
  while (!answered.load()) {
    if (SDL_GetTicks() > deadline) {
      std::fprintf(stderr, "metroid_prime2_port: disc selection timed out\n");
      return {};
    }
    SDL_Delay(10);
  }
  if (chosen.empty()) {
    if (dismissed.load()) {
      std::fprintf(stderr, "metroid_prime2_port: no disc image picked\n");
    } else {
      std::fprintf(stderr, "metroid_prime2_port: the file picker failed: %s\n", SDL_GetError());
    }
  }
  return chosen;
}

// Copies the picked image to `target` through `target.part` and a rename, and returns `target`
// on success, an empty string after a line saying which step failed. The header is checked
// before anything is written, so a wrong pick costs a read rather than a 1.4 GB copy.
std::string CopyDiscImage(const std::string& uri, const std::string& target) {
  if (target.empty()) {
    std::fprintf(stderr, "metroid_prime2_port: nowhere to copy the picked disc image\n");
    return {};
  }
  const std::filesystem::path partial = target + ".part";
  std::error_code error;
  std::filesystem::create_directories(std::filesystem::path(target).parent_path(), error);

  SDL_IOStream* in = SDL_IOFromFile(uri.c_str(), "rb");
  if (in == nullptr) {
    std::fprintf(stderr, "metroid_prime2_port: could not read the picked image: %s: %s\n", uri.c_str(),
                 SDL_GetError());
    return {};
  }
  char header[8] = {};
  const bool headerRead = SDL_ReadIO(in, header, sizeof(header)) == sizeof(header);
  if (headerRead && !SupportedHeader(header, sizeof(header))) {
    std::fprintf(stderr, "metroid_prime2_port: not copying the picked image: it is not a "
                         "Metroid Prime 2: Echoes disc (expected G2ME01 revision 0)\n");
    SDL_CloseIO(in);
    return {};
  }
  // Some providers hand out a pipe, which cannot be rewound: start over rather than copy the
  // bytes the header check consumed.
  if (SDL_SeekIO(in, 0, SDL_IO_SEEK_SET) != 0) {
    SDL_CloseIO(in);
    in = SDL_IOFromFile(uri.c_str(), "rb");
    if (in == nullptr) {
      std::fprintf(stderr, "metroid_prime2_port: could not reopen the picked image: %s\n",
                   SDL_GetError());
      return {};
    }
  }
  const Sint64 total = SDL_GetIOSize(in);
  SDL_IOStream* out = SDL_IOFromFile(partial.string().c_str(), "wb");
  if (out == nullptr) {
    std::fprintf(stderr, "metroid_prime2_port: could not create %s: %s\n", partial.string().c_str(),
                 SDL_GetError());
    SDL_CloseIO(in);
    return {};
  }

  char buffer[1 << 16];
  Sint64 copied = 0;
  int lastPercent = -1;
  bool ok = true;
  for (;;) {
    const size_t got = SDL_ReadIO(in, buffer, sizeof(buffer));
    if (got == 0) {
      // Zero is also what a failed read returns; only the stream status tells end of file from
      // an error that would leave a truncated copy.
      if (SDL_GetIOStatus(in) != SDL_IO_STATUS_EOF) {
        std::fprintf(stderr, "metroid_prime2_port: reading the picked image failed: %s\n",
                     SDL_GetError());
        ok = false;
      }
      break;
    }
    if (SDL_WriteIO(out, buffer, got) != got) {
      std::fprintf(stderr, "metroid_prime2_port: writing %s failed: %s\n", partial.string().c_str(),
                   SDL_GetError());
      ok = false;
      break;
    }
    copied += static_cast<Sint64>(got);
    if (total > 0) {
      const int percent = static_cast<int>(copied * 100 / total);
      // Every 5%: a 1.4 GB image would otherwise put thousands of lines in logcat.
      if (percent / 5 != lastPercent / 5) {
        lastPercent = percent;
        std::fprintf(stderr, "metroid_prime2_port: copying the disc image, %d%%\n", percent);
      }
    }
  }
  if (!SDL_FlushIO(out)) {
    std::fprintf(stderr, "metroid_prime2_port: flushing %s failed: %s\n", partial.string().c_str(),
                 SDL_GetError());
    ok = false;
  }
  if (!SDL_CloseIO(out)) {
    std::fprintf(stderr, "metroid_prime2_port: closing %s failed: %s\n", partial.string().c_str(),
                 SDL_GetError());
    ok = false;
  }
  if (!SDL_CloseIO(in)) {
    std::fprintf(stderr, "metroid_prime2_port: closing the picked image failed: %s\n", SDL_GetError());
  }
  if (!ok) {
    std::filesystem::remove(partial, error);
    return {};
  }
  // What landed on disk, not what was handed to the writer: a short write that only fails at
  // close looks identical to a good copy otherwise, and a truncated image fails to mount with
  // no further clue.
  const auto written = std::filesystem::file_size(partial, error);
  if (error || static_cast<Sint64>(written) != copied || (total > 0 && copied != total)) {
    std::fprintf(stderr, "metroid_prime2_port: %s holds %lld bytes, copied %lld of %lld\n",
                 partial.string().c_str(), static_cast<long long>(written),
                 static_cast<long long>(copied), static_cast<long long>(total));
    std::filesystem::remove(partial, error);
    return {};
  }
  std::filesystem::rename(partial, target, error);
  if (error) {
    std::fprintf(stderr, "metroid_prime2_port: could not rename %s to %s: %s\n", partial.string().c_str(),
                 target.c_str(), error.message().c_str());
    std::filesystem::remove(partial, error);
    return {};
  }
  return target;
}

// The disc this launch will use. The path the resolution chose is used when it is a usable
// image - an image pushed into app storage, or MP2_DISC - and otherwise the user is asked for
// one, which is copied over that same path (the app's own port_paths.txt makes it
// `<files>/disc.iso`, and naming the path the resolution gave keeps a pushed `disc=` override
// meaningful instead of silently writing somewhere else). An empty return is a user who
// cancelled, a picker that failed or a pick that could not be copied, each of which is logged.
std::string EnsureDiscImage(const std::string& resolved, const std::string& defaultDisc) {
  std::string disc = resolved.empty() ? defaultDisc : resolved;
  for (;;) {
    if (DiscImageIsUsable(disc)) {
      return disc;
    }
    std::fprintf(stderr,
                 "metroid_prime2_port: %s is not a Metroid Prime 2: Echoes disc image; "
                 "asking for one\n",
                 disc.c_str());
    const std::string picked = AskForDiscImage();
    if (picked.empty()) {
      return {};
    }
    const std::string copied = CopyDiscImage(picked, disc);
    if (copied.empty()) {
      continue; // the failure is logged; ask again rather than give up on one bad pick
    }
    std::printf("metroid_prime2_port: disc image picked: %s, copied to %s\n", picked.c_str(),
                copied.c_str());
    disc = copied; // re-checked on the way round, so a bad copy is asked for again
  }
}

#else

// The desktop keeps its console and its environment: fd 1 and fd 2 already arrive where the
// harness reads them, and MP2_USER_PATH / MP2_CACHE_PATH / MP2_DISC are set by the caller,
// so neither of these has anything to do.
void PortLogBegin() {}
void PortLogDrain() {}

#endif

} // namespace

// A SIGSEGV that prints where it happened. `tools/boot_probe.sh` reports "died on signal 11"
// and nothing else, which is enough to know the boot died and not enough to fix it: the last
// `printf` before the marker says which *step*, and everything inside that step is a guess.
// A backtrace turns the guess into a measurement, and it costs one signal handler.
//
// Deliberately not a substitute for a debugger: this has to work in the exact binary
// `boot_probe.sh` builds, with the reach stubs linked and no debugger attached, because that
// is the configuration that reproduces the fault. `-rdynamic` is already in the link line, so
// `backtrace_symbols` can name our own functions rather than only addresses.
//
// bionic has no `execinfo.h`, so Android has no backtrace to print, and it discards stderr:
// there the logcat line below is what says the port caught anything at all, and the fault
// address is the one piece of information a tombstone alone makes awkward to pin down. The
// re-raise is unchanged, so bionic still writes its own tombstone.
#if defined(__ANDROID__)
static void PortFaultHandler(int sig, siginfo_t* info, void* /*context*/) {
  const char* name = (sig == SIGSEGV) ? "SIGSEGV" : (sig == SIGBUS ? "SIGBUS" : "signal");
  const void* faultAddress = info != nullptr ? info->si_addr : nullptr;
  // Not "backtrace follows" as the desktop path says: bionic prints none, and the
  // tombstone the re-raise produces is the stack this platform gets.
  std::fprintf(stderr, "\n[port] caught %s (%d) - tombstone follows\n", name, sig);
  std::fflush(stderr);
  __android_log_print(ANDROID_LOG_FATAL, "mp2port", "[port] caught %s (%d) at %p", name, sig,
                      faultAddress);
  // Re-raise with the default handler so the exit status still says "died on signal 11",
  // which is what boot_probe.sh greps for. A handler that swallows it would make the probe
  // report a clean exit, and a clean exit from a segfaulting boot is the one lie worth
  // being unable to tell.
  std::signal(sig, SIG_DFL);
  std::raise(sig);
}
#else
static void PortFaultHandler(int sig) {
  const char* name = (sig == SIGSEGV) ? "SIGSEGV" : (sig == SIGBUS ? "SIGBUS" : "signal");
  std::fprintf(stderr, "\n[port] caught %s (%d) - backtrace follows\n", name, sig);
  std::fflush(stderr);
  void* frames[64];
  const int n = ::backtrace(frames, 64);
  ::backtrace_symbols_fd(frames, n, STDERR_FILENO);
  std::fflush(stderr);
  // Re-raise with the default handler so the exit status still says "died on signal 11",
  // which is what boot_probe.sh greps for. A handler that swallows it would make the probe
  // report a clean exit, and a clean exit from a segfaulting boot is the one lie worth
  // being unable to tell.
  std::signal(sig, SIG_DFL);
  std::raise(sig);
}
#endif

int main(int argc, char** argv) {
  // Android first, before anything can print: from here on fd 1 and fd 2 reach logcat, which
  // is the only place a device run leaves a record. A no-op on the desktop.
  PortLogBegin();

#if defined(__ANDROID__)
  // `signal(2)` never hands the handler the fault address, so Android installs the
  // handler that can take one. Nothing else about the registration differs.
  struct sigaction faultAction = {};
  faultAction.sa_sigaction = PortFaultHandler;
  faultAction.sa_flags = SA_SIGINFO;
  sigemptyset(&faultAction.sa_mask);
  sigaction(SIGSEGV, &faultAction, nullptr);
  sigaction(SIGBUS, &faultAction, nullptr);
  sigaction(SIGILL, &faultAction, nullptr);
  sigaction(SIGFPE, &faultAction, nullptr);
#else
  std::signal(SIGSEGV, PortFaultHandler);
  std::signal(SIGBUS, PortFaultHandler);
  std::signal(SIGILL, PortFaultHandler);
  std::signal(SIGFPE, PortFaultHandler);
#endif

  // Android has no environment to set, so the app's own directories stand in where the
  // variables are unset (and print which they are); on the desktop this stays empty and
  // everything below resolves exactly as it did before.
  PortPaths appPaths;
#if defined(__ANDROID__)
  appPaths = ResolveAndroidPaths();

  // The disc, resolved and - if the image is missing or is not this game - picked from the
  // user's storage, all before Aurora comes up: `EnsureDiscImage` says why the trigger is the
  // image not being usable rather than the disc check after the renderer starts. The desktop
  // keeps the resolution it always had, below.
  const std::string androidDiscImage =
      EnsureDiscImage(port::entry::ResolveDiscPath(argc, argv, EnvOrFallback("MP2_DISC", appPaths.disc),
                                                   ExecutableDir(argc > 0 ? argv[0] : nullptr)),
                      appPaths.disc);
  if (androidDiscImage.empty()) {
    // Nothing was picked, so there is nothing to boot: exiting here, before Aurora is up,
    // rather than at the disc check below, which would bring the renderer up only to close it.
    std::fprintf(stderr, "metroid_prime2_port: no disc image to boot\n");
    PortLogDrain();
    return 1;
  }
#endif

  // The touch overlay's own settings live in the user directory, next to the memory card:
  // the layout the player moved, and which of the two layouts is active. On the desktop this
  // is MP2_USER_PATH or nothing at all - there is no overlay there, and an empty directory
  // just leaves the defaults in place. Called before Aurora because the Java view polls the
  // layout as soon as the surface exists.
  const char* touchUser = EnvOrFallback("MP2_USER_PATH", appPaths.user);
  PortTouch::Initialize(touchUser != nullptr ? touchUser : std::string());

  // The keyboard bindings (platform/port_binds.cpp) are saved beside touch.cfg, in the same
  // directory. The file is read on the game thread's first frame rather than here, because its
  // key names come from SDL, which is only up after Aurora is.
  PortBinds::Initialize(touchUser != nullptr ? touchUser : std::string());

  // The save tools (platform/port_save.cpp) keep their backups beside keybinds.cfg; the card's own
  // path comes from Aurora when a command needs it.
  PortSave::Initialize(touchUser != nullptr ? touchUser : std::string());

  // The debug command console (platform/port_console.cpp): with MP2_CONSOLE=<port> it listens on
  // 127.0.0.1 before the game starts, so a test can drive it from the first update. Commands are
  // queued here and run inside the state manager's update, where the world is live.
  PortConsole::Start();

  // 16:9 at the game's logical height. The render mode is widened to match when
  // the widescreen work lands; Aurora upscales to the window either way.
  const AuroraConfig config = {
      .appName = kAppName,
      .userPath = EnvOrFallback("MP2_USER_PATH", appPaths.user),
      .cachePath = EnvOrFallback("MP2_CACHE_PATH", appPaths.cache),
      .resourcesPath = nullptr,
      .desiredBackend = BACKEND_AUTO,
      .vsync = false,
      .allowTextureDumps = false,
      .windowWidth = 854u,
      .windowHeight = 480u,
      .mem1Size = MEM1_DEFAULT_SIZE,
      .mem2Size = ARAM_DEFAULT_SIZE,
  };

  aurora_initialize(argc, argv, &config);
  // Keep the internal framebuffer at the game's aspect rather than the window's,
  // so a mismatched window shape letterboxes instead of stretching.
  AuroraSetViewportPolicy(AURORA_VIEWPORT_FIT);

  const std::string discImage =
#if defined(__ANDROID__)
      // Resolved and, if need be, picked from the user's storage before Aurora started.
      androidDiscImage;
#else
      port::entry::ResolveDiscPath(argc, argv, EnvOrFallback("MP2_DISC", appPaths.disc),
                                   ExecutableDir(argc > 0 ? argv[0] : nullptr));
#endif
  if (discImage.empty()) {
    ReportNoDisc(argc > 0 ? argv[0] : nullptr);
    aurora_shutdown();
    PortLogDrain();
    return 1;
  }

  if (!aurora_dvd_open(discImage.c_str())) {
    std::fprintf(stderr, "metroid_prime2_port: failed to open disc image: %s\n", discImage.c_str());
    aurora_shutdown();
    PortLogDrain();
    return 1;
  }

  const DVDDiskID* discId = DVDGetCurrentDiskID();
  if (discId == nullptr || !port::entry::IsSupportedDisc(*discId)) {
    std::fprintf(stderr,
                 "metroid_prime2_port: unsupported disc; expected Metroid Prime 2: Echoes "
                 "(USA) (v1.00), G2ME01 revision 0.\n");
    aurora_dvd_close();
    aurora_shutdown();
    PortLogDrain();
    return 1;
  }
  std::printf("metroid_prime2_port: disc mounted: %s\n", discImage.c_str());

  // Prime the window and event state so the game's first frame can succeed: the
  // console build submits GX during early initialization, before its main loop.
  aurora_update();

  // The game's entry owns the OS context and memory system. It returns void, so
  // the exit code is the platform's: reaching the end means the game returned.
  COsContext osContext(true, true);
  // The game heap is carved out of the arena by the line below; `PortMwccNew.cpp`'s `operator
  // delete` routes pointers inside this range to `CMemory::Free`.
  gPortGameHeapLo = OSGetArenaLo();
  gPortGameHeapHi = OSGetArenaHi();
  CMemorySys memorySys(osContext, CMemorySys::GetGameAllocator());

  // Retail builds a `CGraphicsSys` here too, in `main` (0x801EFB00), and passes it to
  // `InvokeCMain` as the sixth argument - an 8-byte object built with `fn_802BE85C`,
  // which is `CGraphicsSys::CGraphicsSys` and whose whole body is
  // `CGraphics::Startup(osContext, progressive)`. That constructor is where retail's VI
  // bring-up happens (`fn_802C329C` -> `fn_802C2FD4` = `ConfigureVideo`:
  // `VIGetTvFormat` -> `GXAdjustForOverscan` -> two framebuffers -> `VIConfigure` ->
  // `VIFlush` -> `GXInit` -> `GXSetCopyFilter`), and it has to run *before* the game
  // starts: without `GXInit` there is no GX, and without `mRenderModeObj` filled there is
  // no EFB shape for Aurora's presenter.
  //
  // The `false` is the progressive flag. Retail reads it from the console's saved region -
  // `fn_801EFC68` / `fn_801EFE6C` read the region `lbl_80419304` points at, and the byte
  // they return is the flag. The host has no saved region: there is no console to have
  // set it, and `OSGetSavedRegion` in platform/sdk_stubs.cpp answers null/null, so
  // progressive is unconditionally off. That is also the value retail's own PAL build
  // uses, and the one `ConfigureVideo`'s NTSC branch takes `GXNtsc480IntDf` for.
  //
  // This is a real object with a real destructor, not a leaked temporary: `~CGraphicsSys`
  // calls `CGraphics::Shutdown`, which restores the texture-region callback and stalls the
  // frame-delayed allocator, and both are retail's.
  //
  // It lives in a scope of its own so that it is destroyed before the platform goes down.
  // `CGraphics::Shutdown` stalls the GPU (`StallAndFlushAllAllocations` -> `GXDrawDone` ->
  // `aurora::gx::fifo::drain`), and in Aurora's threaded mode that waits for the FIFO worker
  // thread, which `aurora_shutdown` joins. As a plain local of `main` it was destroyed after
  // `aurora_shutdown` and waited forever for a thread that was gone (measured 2026-10-01: six
  // threads at the hang, no "Aurora FIFO processor"). On the console `main` returns with the
  // video hardware still up, so retail's order is this one.
  {
    CGraphicsSys graphicsSys(osContext, memorySys, false);

    // Three REL modules are compiled into the game library instead of being read
    // off the disc - Tweaks, CannonBall and ForgottenObject. On the cube each is a
    // separate module whose prolog and epilog run when it loads, which is what
    // publishes their function-pointer tables. Nothing on the host loads them, so
    // the port runs their entry points here, before the game's own entry, or those
    // tables stay null and the loaders behind them are never reached.
    port::modules::InitAll();

    // Tweaks.rel's `REL_CreateTweakGlobals` is the only writer of `gpTweakPlayerA`
    // (0x80418F44), and `CGameArchitectureSupport`'s constructor dereferences it at
    // 0x80007F38 with no null test - so this is the first of the two globals the
    // boot path needs and the only one that can be stood in for. The stand-in
    // allocates real 4-byte cells over a zeroed `SLdrTweakPlayer`, which makes the
    // five `CTweakPlayer` accessors answer 0.0f; it carries no tweak data, because
    // the data lives in `Standard.NTWK` inside a pak and the paks are boot-path
    // step 13. `STweaks_FuncPtrs::CreateGlobals` is the retail route and it is
    // assigned by `TweaksInit` and called by nothing. See
    // src/MetroidPrime/PortTweakGlobals.cpp and docs/research/tweak_globals.md.
    port::tweaks::CreateStandInTweakPlayers();

    // Retail `main` loads the embedded default font right after `CGraphicsSys` and points
    // `gpDefaultFont` at it; `CSplashScreen::Draw` dereferences it with no null test.
    // src/MetroidPrime/PortDefaultFont.cpp.
    void* defaultFont = port::LoadDefaultFont();

    InvokeCMain(argc, argv, &osContext, nullptr, &memorySys, nullptr);

    port::FreeDefaultFont(defaultFont);
    port::modules::ShutdownAll();
  }

  aurora_dvd_close();
  aurora_shutdown();
  // The game has returned; nothing else will write to fd 1 or 2, so the last lines are
  // handed to logcat before this thread ends.
  PortLogDrain();
  return 0;
}
