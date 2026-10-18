// The settings file's location and bytes: `<user>/settings.cfg`. The keys, their text format and
// the self-test are in platform/port_overlay_settings.cpp (a port of MP1's debug_ui/settings.cpp);
// this only knows where the file is and how to read and replace it.
//
// Built into `mp_platform`; `mp_port_audio` (the F1 overlay and the console) only includes this.
#pragma once

#include <string>

namespace PortSettings {

// Remembers `<userDirectory>/settings.cfg`. Call once, before Aurora. An empty directory disables
// the file (the environment still applies).
void Initialize(const std::string& userDirectory);

// The file's path, or empty when there is none.
const std::string& FilePath();

// The whole file into `text`; false when there is no file or it cannot be read.
bool ReadFile(std::string& text);

// Replaces the file (through a temporary file and a rename); false when it cannot be written.
bool WriteFile(const std::string& text);

// Whether stdout and stderr also go to a file: `MP2_LOG_FILE`, else `log_file` in settings.cfg,
// else on. Read before the overlay (and so the key table) exists, by a hand parse of the file as
// MP1's RoomGeoResidentAtStartup does.
bool LogFileEnabled();

} // namespace PortSettings
