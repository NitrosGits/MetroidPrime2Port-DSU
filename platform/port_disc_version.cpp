// The mounted disc's version, kept apart from entry.cpp so port_entry_tests links without Aurora.
#include "port_entry.h"

namespace port {
namespace entry {

DiscVersion CurrentDiscVersion() {
  const DVDDiskID* id = DVDGetCurrentDiskID();
  return id == nullptr ? DiscVersion::Unknown : IdentifyDisc(*id);
}

const char* CurrentGameCode() { return DiscGameCode(CurrentDiscVersion()); }

} // namespace entry
} // namespace port
