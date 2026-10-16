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
#include "port_entry.h"
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
// picker that will fill it in is the next milestone. SDL is already up when this runs: the
// activity's context is what SDL_GetPrefPath reads, and Aurora itself falls back to the same
// call for its user and cache directories.
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
    }
  }

  // Created before the game can ask for them, so a first run has somewhere to write.
  std::error_code error;
  std::filesystem::create_directories(paths.user, error);
  error.clear();
  std::filesystem::create_directories(paths.cache, error);

  std::printf("metroid_prime2_port: paths chosen: user=%s cache=%s disc=%s\n", paths.user.c_str(),
              paths.cache.c_str(), paths.disc.c_str());
  return paths;
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
#endif

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
      port::entry::ResolveDiscPath(argc, argv, EnvOrFallback("MP2_DISC", appPaths.disc),
                                   ExecutableDir(argc > 0 ? argv[0] : nullptr));
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
