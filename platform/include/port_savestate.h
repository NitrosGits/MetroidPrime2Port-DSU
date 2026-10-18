#pragma once

#include <cstdint>
#include <string>

class CStateManager;

// Save states: save anywhere, load back to the same spot (MP1's port_savestate, phase 1).
//
// A state is the retail save data - the same CGameState stream a memory card slot holds (items,
// health, ammo, map, scans, script layers, in-game time) - plus the world and area Samus was in,
// where she stood, which way she faced and whether she was in morph ball. Loading swaps that game
// state in the way CMain::StreamNewGameState does, restarts the world the way the debug world warp
// does (CStateManager::PortWarpToWorld), and once the rebuilt world is running puts Samus back.
//
// Not carried (MP1's optional "room state" section): the live script objects of the loaded areas.
// A loaded room comes back the way a memory card load leaves it - enemies respawn, doors and
// platforms reset - unless the save data's layer and relay state says otherwise.
//
// Files: `<user>/savestates/slotN.mp2ss`. Slot 0 is the undo slot, written before every load.
namespace PortSaveState {

constexpr int kSlotCount = 8; // slots 1..8
constexpr int kUndoSlot = 0;  // written before every load

struct Info {
  bool exists = false;
  uint32_t worldId = 0;
  std::string room;     // the area's internal name, or "Area N"
  double playTime = 0.0; // in-game seconds
  int64_t savedAt = 0;   // Unix seconds
  bool morphed = false;
};

// `<user>/savestates`, or empty when the user directory is unknown (states are then off).
std::string Folder();
Info SlotInfo(int slot);

int SelectedSlot();          // 1..kSlotCount, the slot F5/F9 act on
void SetSelectedSlot(int slot);
void SelectNextSlot();

// Queue a save or load of `slot` (0..kSlotCount); the game thread runs it on its next Tick.
// A load of the undo slot does not overwrite the undo slot first.
void RequestSave(int slot);
void RequestLoad(int slot);

// The outcome of the last request, for the overlay ("" before the first).
std::string LastMessage();

// Called on the game thread at the start of every CStateManager update (from PortConsole::Tick).
void Tick(CStateManager& mgr);

} // namespace PortSaveState
