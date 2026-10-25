#pragma once

// Quick resume, the pure parts (MP1's port_resume): the container around a save state, which
// launches it may resume, and when a new snapshot is due. Header-only so a test can link it alone.

#include <cstdint>
#include <cstring>
#include <string>

namespace PortResume {

constexpr uint32_t kContainerMagic = 0x5352504d; // "MPRS" read little-endian
constexpr uint32_t kContainerVersion = 1;
constexpr int kMaxFileIdx = 2; // the card has three save slots

// Seconds in a room before the first snapshot of it, and between refreshes within one room.
constexpr double kFirstSnapshotDelay = 2.0;
constexpr double kRefreshInterval = 30.0;

struct Header {
  int32_t fileIdx = 0;     // the card slot the game was played from
  uint64_t cardSerial = 0; // the card the save came from; a resume of another one is refused
  int64_t savedAt = 0;     // Unix seconds
};

inline void Put(std::string& out, const void* p, size_t n) { out.append(static_cast< const char* >(p), n); }

// magic, version, fileIdx, cardSerial, savedAt, then the state bytes.
inline std::string WrapContainer(const Header& h, const std::string& state) {
  std::string out;
  const uint32_t magic = kContainerMagic;
  const uint32_t version = kContainerVersion;
  Put(out, &magic, 4);
  Put(out, &version, 4);
  Put(out, &h.fileIdx, 4);
  Put(out, &h.cardSerial, 8);
  Put(out, &h.savedAt, 8);
  out += state;
  return out;
}

constexpr size_t kContainerHeaderSize = 4 + 4 + 4 + 8 + 8;

inline bool UnwrapContainer(const std::string& in, Header& h, std::string& state) {
  if (in.size() < kContainerHeaderSize) {
    return false;
  }
  uint32_t magic = 0;
  uint32_t version = 0;
  Header parsed;
  const char* p = in.data();
  std::memcpy(&magic, p, 4);
  std::memcpy(&version, p + 4, 4);
  std::memcpy(&parsed.fileIdx, p + 8, 4);
  std::memcpy(&parsed.cardSerial, p + 12, 8);
  std::memcpy(&parsed.savedAt, p + 20, 8);
  if (magic != kContainerMagic || version != kContainerVersion || parsed.fileIdx < 0 ||
      parsed.fileIdx > kMaxFileIdx) {
    return false;
  }
  h = parsed;
  state.assign(in, kContainerHeaderSize, std::string::npos);
  return true;
}

// A resume may load only into the card it came from.
inline bool Matches(const Header& h, uint64_t cardSerial) { return h.cardSerial == cardSerial; }

// What to write when the game closes: a fresh capture when the game is in a safe moment, else the
// snapshot already held, else nothing.
enum class Write { Nothing, Snapshot, Fresh };
inline Write ChooseWrite(bool safeNow, bool haveSnapshot) {
  if (safeNow) {
    return Write::Fresh;
  }
  return haveSnapshot ? Write::Snapshot : Write::Nothing;
}

// Whether a new snapshot is due on this tick. `sameRoom` is whether the held snapshot is of the
// room Samus is in now.
inline bool RefreshDue(bool safe, bool haveSnapshot, bool sameRoom, double secondsInRoom,
                       double secondsSinceSnapshot) {
  if (!safe) {
    return false;
  }
  if (!haveSnapshot || !sameRoom) {
    return secondsInRoom >= kFirstSnapshotDelay;
  }
  return secondsSinceSnapshot >= kRefreshInterval;
}

// The running marker's text after a resume, so the next launch can tell "crashed again right
// after resuming this snapshot" from "crashed later".
inline std::string ResumedMarker(int64_t savedAt) { return "resumed " + std::to_string(savedAt); }

inline bool CrashedAgain(bool lastRunCrashed, const std::string& marker, int64_t savedAt) {
  return lastRunCrashed && marker == ResumedMarker(savedAt);
}

} // namespace PortResume
