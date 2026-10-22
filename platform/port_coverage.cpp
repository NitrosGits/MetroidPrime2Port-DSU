// The gcov flush hook of a coverage build (CMake option MP_PORT_COVERAGE).
//
// Why this file exists: gcov's counters live in the process, and they reach the disk only when
// the process exits normally (libgcov's own exit handler) or when something asks libgcov to
// write them out (__gcov_dump()). A coverage run ends in three ways, and all three have to
// leave a profile behind:
//
//   * the game's clean quit (MP2_FRAMES reached): the exit handler does this, and the
//     shutdown path asks for it explicitly as well;
//   * the harness's backstop: `timeout` sends SIGTERM;
//   * a Ctrl-C or a killed run: SIGINT.
//
// The two signals get a handler that dumps the counters and re-raises with the default action,
// so the run still ends "on SIGTERM" (the harness reads the exit status) instead of pretending
// to have exited cleanly.
//
// The dump is not async-signal-safe: it opens and writes files, and malloc. That is a known
// trade-off and the right one here - a run that ends in a signal is ending anyway, and the
// alternative is a run whose profile is lost. The counters are atomic (-fprofile-update=atomic),
// so a thread that is mid-update cannot tear a counter.
//
// GCOV_PREFIX and GCOV_PREFIX_STRIP decide where the .gcda files land; the run scripts set them
// so that concurrent runs do not write over each other. See docs/RUNNING.md.

#include "port_coverage.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>

// libgcov's dump. Declared here instead of including <gcov.h> so that the file also compiles
// where the gcc headers are not on the include path (the header is a C header of the compiler's
// own include directory, which is not a default include directory of a C++ compile).
extern "C" void __gcov_dump(void);
extern "C" void __gcov_reset(void);

namespace {

// Set once the counters are out, so that the signal handler and the exit path cannot write the
// same profile twice.
volatile sig_atomic_t gPortCoverageDumped = 0;

void PortCoverageDumpOnce() {
  if (gPortCoverageDumped != 0) {
    return;
  }
  gPortCoverageDumped = 1;
  __gcov_dump();
}

void PortCoverageSignalHandler(int sig) {
  PortCoverageDumpOnce();
  // Back to the default action, then raise: the process still dies of the signal it was given,
  // so the exit status says SIGTERM (143) or SIGINT (130) and not "exited normally".
  std::signal(sig, SIG_DFL);
  std::raise(sig);
}

} // namespace

extern "C" void PortCoverageDump() { PortCoverageDumpOnce(); }

extern "C" void PortCoverageFlush() {
  if (gPortCoverageDumped != 0) {
    return;
  }
  __gcov_dump();
  __gcov_reset();
}

extern "C" void PortCoverageInstall() {
  std::signal(SIGTERM, PortCoverageSignalHandler);
  std::signal(SIGINT, PortCoverageSignalHandler);
  // The clean quit: the exit hook writes the counters when main returns.
  std::atexit(PortCoverageDumpOnce);
  std::fprintf(stderr, "[port] coverage: gcov counters will be written on exit\n");
}
