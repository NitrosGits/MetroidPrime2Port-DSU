#pragma once
// Replacing the disc image from the running game (F1 > System > Disc image > Replace disc image):
// the picked file is checked to be a supported Metroid Prime 2: Echoes disc before anything changes,
// then remembered (settings.cfg `disc_path`) for the next start. A content:// URI (Android's picker)
// is first copied into the data folder on a background thread (disc.iso.part, renamed when complete).
// The mounted disc is never swapped live, so nothing is reloaded: the REL modules and every pak
// stay those of the disc the game started with. Port of MP1's port_disc_replace.
#include <atomic>
#include <cstdint>
#include <string>

namespace PortDiscReplace {

// The picked file is a supported disc: "" and its revision ("USA 1.00"), else the reason.
std::string CheckImage(const std::string& path, std::string* revision = nullptr);

// Copies a picked file (a path, or a content:// URI) to <user folder>/disc.iso through disc.iso.part.
// Reports bytes done and total to progress; returns early if cancel turns true. "" on success, else why not.
std::string CopyToUserFolder(const std::string& source, std::string* target, std::atomic<uint64_t>* done,
                             std::atomic<uint64_t>* total, const std::atomic<bool>* cancel);

// Remembers a checked image for the next start, saves the settings and logs it.
void Adopt(const std::string& path, const std::string& revision);

enum class State { Idle, Running, Done, Failed };

struct Status {
  State state = State::Idle;
  uint64_t bytesDone = 0;
  uint64_t bytesTotal = 0;
  bool copying = false;
  std::string message; // the failure, or what to do after a success
  std::string path;
  std::string revision;
};

// Checks (and for a content:// URI, copies) a picked file on its own thread. False if one is running.
bool Start(const std::string& picked);
void Cancel();
// Cancels and joins; call before the process ends.
void Shutdown();
Status Snapshot();
// Call from the main thread: remembers the image once a run has finished well. True the one time it does.
bool Poll();
// Asks for a disc image with the platform's file dialog, without blocking; the pick goes to Start.
void PickAsync();

} // namespace PortDiscReplace
