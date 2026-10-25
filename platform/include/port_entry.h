#pragma once

// Port entry-point helpers: what the process entry needs before it can hand
// control to the game. Kept free of game types so they can be tested without
// linking the game (see tests/port_entry_tests.cpp).

#include <string>

#include <dolphin/dvd.h>

namespace port {
namespace entry {

// The discs the port runs: Metroid Prime 2: Echoes, disc 0, revision 0, either USA v1.00
// (`G2ME01`, the version the decompilation targets) or PAL (`G2MP01`, experimental). The code
// is always 1.00's; a PAL disc only supplies its own data (docs/PAL.md).
enum class DiscVersion { Unknown, Usa100, Pal };
DiscVersion IdentifyDisc(const DVDDiskID& id);
bool IsSupportedDisc(const DVDDiskID& id);
// "USA 1.00", "PAL" or "unknown".
const char* DiscVersionName(DiscVersion version);
// The disc's four-letter game code, which also names its memory card ("G2ME" -> USA, "G2MP" -> EUR).
const char* DiscGameCode(DiscVersion version);

// The mounted disc's version (Unknown before a disc is mounted) and its game code (G2ME when unknown).
DiscVersion CurrentDiscVersion();
const char* CurrentGameCode();

// Disc-image file names the port will pick up beside the executable.
bool IsDiscImageName(const std::string& name);

// Disc image path, in order: an explicit non-flag argument, `envDiscPath`, then
// the first disc image in `executableDir`. `envDiscPath` may be null. Returns an
// empty string when nothing is found. The caller supplies the environment and
// the directory so this stays testable.
std::string ResolveDiscPath(int argc, char** argv, const char* envDiscPath,
                            const std::string& executableDir);

} // namespace entry
} // namespace port
