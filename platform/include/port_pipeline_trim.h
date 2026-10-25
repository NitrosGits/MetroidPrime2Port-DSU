#pragma once

#include <cstdint>

// Lazy pipeline cache under memory pressure (MP1 sync package 20; MP1 issue #187). Built pipelines
// stay until memory runs short: Tick checks the system's available memory every 5 s and, below the
// moderate or severe level, asks Aurora to release built pipelines no recent frame drew with (idle
// for 3600 frames, or for 16). GLOBAL pipelines stay pinned. Linux only (/proc/meminfo); elsewhere
// the check never fires.
namespace PortPipelineTrim {

// Levels in MiB of available memory; 0 turns a level off, both 0 (the default) turns the check off.
void SetPressure(int64_t moderateMib, int64_t severeMib);
// Called once per main-loop iteration; cheap until the 5 s poll is due.
void Tick(uint64_t nowNs);

} // namespace PortPipelineTrim
