#include "port_pipeline_trim.h"

#include "port_logfile.h"

#include <aurora/gfx.h>

#include <cstdio>
#include <string>

namespace PortPipelineTrim {
namespace {
constexpr uint64_t kSecondNs = 1000000000ull;
constexpr uint64_t kPollNs = 5 * kSecondNs;
// A level that holds trims again this often, so a moderate trim catches what went idle since.
constexpr uint64_t kRepeatNs = 30 * kSecondNs;
constexpr uint32_t kModerateIdleFrames = 3600; // a minute at 60 fps: areas left a while ago
constexpr uint32_t kSevereIdleFrames = 16;     // everything this frame and the last few don't draw
int64_t sModerateKb = 0;
int64_t sSevereKb = 0;

#if defined(__linux__)
// MemAvailable from /proc/meminfo in kB, or -1.
int64_t AvailableKb() {
  std::FILE* f = std::fopen("/proc/meminfo", "r");
  if (f == nullptr) {
    return -1;
  }
  char line[160];
  long long kb = -1;
  while (std::fgets(line, sizeof(line), f) != nullptr) {
    if (std::sscanf(line, "MemAvailable: %lld kB", &kb) == 1) {
      break;
    }
  }
  std::fclose(f);
  return kb;
}
#endif
} // namespace

void SetPressure(int64_t moderateMib, int64_t severeMib) {
  sModerateKb = moderateMib > 0 ? moderateMib * 1024 : 0;
  sSevereKb = severeMib > 0 ? severeMib * 1024 : 0;
}

void Tick(uint64_t nowNs) {
#if defined(__linux__)
  static uint64_t sLastPollNs = 0;
  static uint64_t sLastTrimNs = 0;
  static int sLastLevel = 0;
  if ((sModerateKb <= 0 && sSevereKb <= 0) || (sLastPollNs != 0 && nowNs - sLastPollNs < kPollNs)) {
    return;
  }
  sLastPollNs = nowNs;
  const int64_t availableKb = AvailableKb();
  if (availableKb < 0) {
    return;
  }
  const int level = sSevereKb > 0 && availableKb < sSevereKb       ? 2
                    : sModerateKb > 0 && availableKb < sModerateKb ? 1
                                                                   : 0;
  if (level == 0) {
    sLastLevel = 0;
    return;
  }
  if (level <= sLastLevel && nowNs - sLastTrimNs < kRepeatNs) {
    return;
  }
  sLastLevel = level;
  sLastTrimNs = nowNs;
  const uint32_t idle = level == 2 ? kSevereIdleFrames : kModerateIdleFrames;
  // Aurora's "pipeline cache: evicted N" line at the next frame start gives the count.
  char line[200];
  std::snprintf(line, sizeof(line),
                "port: memory pressure: %s, system %lld MiB available (under %lld): trimming pipelines "
                "unused for %u frames",
                level == 2 ? "severe" : "moderate", static_cast< long long >(availableKb / 1024),
                static_cast< long long >((level == 2 ? sSevereKb : sModerateKb) / 1024), idle);
  PortLogFile::Write(line);
  aurora_trim_pipelines(idle);
#else
  (void)nowNs;
#endif
}

} // namespace PortPipelineTrim
