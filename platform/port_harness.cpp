// See platform/include/port_harness.h. The sibling of platform/port_warp.cpp: the warp slot is
// what a test *does*, this is how long it does it for.

#include "port_harness.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>

namespace {

bool sReadEnv = false;
int sLimit = 0;   // 0 = no limit
int sUpdates = 0;
bool sExitRequested = false;

void ReadEnvOnce() {
  if (sReadEnv) {
    return;
  }
  sReadEnv = true;
  const char* value = std::getenv("MP2_FRAMES");
  if (value == nullptr) {
    return;
  }
  sLimit = std::atoi(value);
  if (sLimit < 0) {
    sLimit = 0;
  }
  if (sLimit > 0) {
    std::fprintf(stderr, "[harness] MP2_FRAMES=%d: the run ends itself after %d game updates\n",
                 sLimit, sLimit);
  } else {    std::fprintf(stderr, "[harness] MP2_FRAMES=%s is not a frame count; the run has no limit\n",
                 value);
  }
}

} // namespace

namespace PortHarness {

bool GameUpdate() {
  ReadEnvOnce();
  if (++sUpdates == 1) {
    // The first game update is the moment the game is up and running (the world the flow asked for
    // has been loaded), which is the number most test timings are compared against.
    std::fprintf(stderr, "[harness] the first game update is at %.1f s\n", Seconds());
  }
  if (sLimit == 0) {
    return false;
  }
  if (sUpdates < sLimit) {
    return false;
  }
  sExitRequested = true;
  return true;
}

bool ExitRequested() { return sExitRequested; }

double Seconds() {
  // Anchored on the first call into this file, which is the game loop's first iteration: the loop's
  // own poll asks `ExitRequested` before anything else runs, and the game's first update asks
  // `GameUpdate`. Every second a `[skip]`/`[harness]` line prints is measured from that moment, so a
  // log's phases are comparable across runs.
  static const auto start = std::chrono::steady_clock::now();
  return std::chrono::duration< double >(std::chrono::steady_clock::now() - start).count();
}

} // namespace PortHarness
