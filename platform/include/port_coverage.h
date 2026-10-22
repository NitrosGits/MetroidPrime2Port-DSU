// The gcov flush hook of a coverage build (CMake option MP_PORT_COVERAGE).
//
// A coverage run must leave its .gcda behind however it ends: the game's clean quit, the
// harness's `timeout` (SIGTERM) or a Ctrl-C (SIGINT). libgcov's own exit handler only runs when
// the process exits normally, so the two signals get a handler that dumps the counters and
// re-raises - the exit status still says "killed by SIGTERM" instead of hiding it.
//
// The file is compiled only into a coverage build (CMakeLists.txt adds it to mp_platform when
// MP_PORT_COVERAGE is on), so the normal build does not see it. See docs/LANE_BRIEF.md,
// "Coverage without probes".

#pragma once

// Installs the signal handlers and the exit hook. Call it before the game starts.
extern "C" void PortCoverageInstall();

// Writes the counters to the .gcda files. Safe to call more than once: the second call does
// nothing, so a dump from a signal and the one from the exit path cannot double-count.
extern "C" void PortCoverageDump();

// Writes the counters now and zeroes them, so the next dump adds only what ran since (libgcov
// merges into the existing .gcda files). The console's `covdump`: a run that hangs in shutdown
// and is killed still leaves a profile behind.
extern "C" void PortCoverageFlush();
