// The settings file: `<user>/settings.cfg`, `key=value` lines with `#` comments.
//
// A setting is registered once with its default (`AddBool`, ...). Registering resolves it: the
// built-in default, then the file, then the environment (`MP2_` + the key upper-cased), the
// environment winning for any key it names and parses. A value that is out of range or does not
// parse is ignored and the next layer down stands.
//
// The file is written only when a value was changed during the run, and then only the lines that
// changed: comments, unknown keys and values the run did not touch (an invalid line, a key an
// environment variable overrode) are left as they were. A run that changes nothing never writes,
// and never creates the file. Cheats are not settings and are never registered.
//
// Built into `mp_platform`; `mp_port_audio` (the F1 overlay and the console) only includes this.
#pragma once

#include <climits>
#include <string>
#include <vector>

namespace PortSettings {

// Reads `<userDirectory>/settings.cfg` if it exists. Call once, before Aurora and before the first
// Add*. An empty directory disables the file (the environment still applies).
void Initialize(const std::string& userDirectory);

// Registration: `value` is set to the resolved value, and is the live variable from then on.
const int kNoZeroAlias = INT_MIN;
void AddBool(const char* key, bool* value, bool defaultValue);
// `valid` (may be null) rejects values inside [min, max] that are still not allowed (MSAA is 1 or
// 4); `zeroAs` (unless kNoZeroAlias) is what a 0 means (the HUD scale's 0 is "max").
void AddInt(const char* key, int* value, int defaultValue, int min, int max,
            bool (*valid)(int) = nullptr, int zeroAs = kNoZeroAlias);
// `allowZero` accepts 0 although it is outside [min, max] (the FOV's 0 is "the game's own").
void AddFloat(const char* key, float* value, float defaultValue, float min, float max,
              bool allowZero = false);
// Three comma-separated floats, each clamped to [min, max]; non-finite is rejected.
void AddFloat3(const char* key, float value[3], float min, float max);

// For a setter that changed a registered variable. NoteChanged marks the settings dirty; Save writes
// them if (and only if) the serialised text now differs from what was read. Returns whether a file
// was written.
void NoteChanged();
bool Dirty();
bool Save();

// A boolean before the registry exists (the log file starts before the overlay is up): the
// environment, then the file, then `defaultValue`. Needs Initialize first.
bool BoolValue(const char* key, bool defaultValue);

struct Info {
  std::string key;
  std::string value;
  const char* source; // "file", "env" or "default"
};
std::vector<Info> List();

// Sets one registered key from text (strictly parsed), then saves. `error` says why not.
bool Set(const std::string& key, const std::string& value, std::string& error);

} // namespace PortSettings
