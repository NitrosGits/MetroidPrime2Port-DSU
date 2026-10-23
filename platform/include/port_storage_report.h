#ifndef METROID_PRIME2_PORT_PORT_STORAGE_REPORT_H
#define METROID_PRIME2_PORT_PORT_STORAGE_REPORT_H

// MP1's startup log of where the app's disk space goes: one line per top-level entry of each
// folder the port writes to (the user folder, the cache, Android's internal storage), so a full
// device can be diagnosed from the log alone.

#include <string>
#include <vector>

namespace PortStorageReport {

// Walks `roots` on a detached background thread and logs
// "port: storage: <path> <MiB> MiB (<files> files)" per entry and a "total" line. A root inside an
// earlier one is listed with it, not again. Never blocks startup, never follows symlinks, never
// throws.
void StartLogging(std::vector< std::string > roots);

} // namespace PortStorageReport

#endif
