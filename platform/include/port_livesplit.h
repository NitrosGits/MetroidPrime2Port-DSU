#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

// Speedrun support: a LiveSplit Server client (LiveSplit > Control > Start TCP
// Server, port 16834 by default) that starts the timer on a new file, feeds it
// the game's in-game time and splits on upgrades and at the end of the game.
//
// The Tracker below turns per-tick game observations into LiveSplit Server
// commands. It is pure so it can be tested without the game; the glue in
// port_livesplit.cpp feeds it from CStateManager and hands the commands to a
// background socket thread.

namespace PortLiveSplit {

// CPlayerState::kIT_Max; the tracker only needs the count.
constexpr int kItemCount = 0x6d;

// "H:MM:SS.fff", what LiveSplit's setgametime parses.
inline std::string FormatTime(double seconds) {
  if (!(seconds > 0.0))
    seconds = 0.0;
  const long long ms = static_cast<long long>(std::floor(seconds * 1000.0 + 0.5));
  char text[32];
  std::snprintf(text, sizeof(text), "%lld:%02lld:%02lld.%03lld", ms / 3600000, ms / 60000 % 60,
                ms / 1000 % 60, ms % 1000);
  return text;
}

// Whether gaining an item (its capacity going from 0 to more) is an upgrade
// split: beams, combos, visors, suits, morph ball parts, the other gear,
// translators and Sky Temple / temple keys (CPlayerState::EItemType, 0x01-0x28
// and 0x61-0x69). Not the starting equipment (power beam, combat and scan
// visors), the translator upgrade placeholder, energy tanks, refills, dark and
// light ammo (they come with their beam), counters and status flags. Missiles
// and power bombs split once, on the launcher / main power bomb, since
// expansions only raise a capacity that is already above 0.
inline bool CountsAsUpgrade(int item) {
  if (item >= 0x01 && item <= 0x28)
    return item != 0x09 && item != 0x08 && item != 0x1c;
  if (item >= 0x61 && item <= 0x69)
    return true;
  return item == 0x2b || item == 0x2c; // power bomb, missile
}

class Tracker {
public:
  // How far the in-game time may run ahead of what LiveSplit was last told.
  static constexpr double kGameTimeStep = 0.1;

  // Called every game tick while a CStateManager runs. The first call of a
  // session takes a snapshot of the capacities, so what the file already has
  // never splits; an in-game time under a second there means a new file, which
  // resets and starts the timer.
  void Tick(double igt, const int* capacities, bool splitUpgrades, std::vector<std::string>& out) {
    if (!mSession) {
      mSession = true;
      for (int i = 0; i < kItemCount; ++i)
        mCapacities[i] = capacities[i];
      if (igt < 1.0) {
        mRunning = true;
        out.push_back("reset");
        out.push_back("starttimer");
        // LiveSplit's game time only moves when told, and the first split
        // records no game time unless it was set after the start.
        out.push_back("pausegametime");
        SendTime(igt, out);
      }
      return;
    }
    bool split = false;
    for (int i = 0; i < kItemCount; ++i) {
      if (mCapacities[i] <= 0 && capacities[i] > 0 && CountsAsUpgrade(i))
        split = true;
      mCapacities[i] = capacities[i];
    }
    if (!mRunning)
      return;
    if (split && splitUpgrades) {
      SendTime(igt, out);
      out.push_back("split");
    } else if (std::fabs(igt - mSentTime) >= kGameTimeStep) {
      SendTime(igt, out);
    }
  }

  // The final blow (the game's EndGame special function): the last split.
  void EndGame(double igt, std::vector<std::string>& out) {
    if (!mRunning)
      return;
    SendTime(igt, out);
    out.push_back("split");
    mRunning = false;
  }

  // The CStateManager went away (quit, world change, death). The next Tick
  // starts a new session; a run in progress carries on across it.
  void EndSession(double igt, std::vector<std::string>& out) {
    if (mRunning && igt != mSentTime)
      SendTime(igt, out);
    mSession = false;
  }

  bool Running() const { return mRunning; }

private:
  void SendTime(double igt, std::vector<std::string>& out) {
    out.push_back("setgametime " + FormatTime(igt));
    mSentTime = igt;
  }

  bool mSession = false;
  bool mRunning = false;
  double mSentTime = 0.0;
  int mCapacities[kItemCount] = {};
};

// Port glue (port_livesplit.cpp).

// The client's state, for the overlay.
enum EStatus { kStatus_Off, kStatus_Connecting, kStatus_Connected, kStatus_Failed };
EStatus Status();
// The last connection error, or an empty string.
std::string LastError();
// Connects to host:port while enabled (reconnecting every few seconds) and
// disconnects when not. Commands made while not connected are dropped.
void Configure(bool enabled, const std::string& address, bool splitUpgrades);
// Called from CStateManager: every tick after the play time advanced
// (`capacities` holds kItemCount entries), when the game ends, and when the
// manager is destroyed.
void GameTick(double igt, const int* capacities);
void GameEnd(double igt);
void GameSessionEnd(double igt);
// The effective settings: an MP2_LIVESPLIT / MP2_LIVESPLIT_ADDR /
// MP2_LIVESPLIT_SPLIT_UPGRADES environment variable wins over what the overlay
// set. Not saved; Refresh applies them (GameTick does it every tick).
bool Enabled();
void SetEnabled(bool enabled);
std::string Address();
void SetAddress(const std::string& address);
bool SplitUpgrades();
void SetSplitUpgrades(bool enabled);
void Refresh();
// Sends a raw command line (console testing).
void SendRaw(const std::string& line);

} // namespace PortLiveSplit
