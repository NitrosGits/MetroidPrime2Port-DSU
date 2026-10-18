#pragma once

#include <cstdint>
#include <string>
#include <vector>

class CStateManager;

// Save states: save anywhere, load back to the same spot (MP1's port_savestate, phase 1).
//
// A state is the retail save data - the same CGameState stream a memory card slot holds (items,
// health, ammo, map, scans, script layers, in-game time) - plus the world and area Samus was in,
// where she stood, which way she faced and whether she was in morph ball. Loading swaps that game
// state in the way CMain::StreamNewGameState does, restarts the world the way the debug world warp
// does (CStateManager::PortWarpToWorld), and once the rebuilt world is running puts Samus back.
//
// Room state (phase 2, MP1's "room state" section): the live script objects of the areas loaded
// at save time - which of them still exist (collected pickups and killed enemies stay gone),
// whether each is active, enemy health and pose, timers, counters and whether doors stand open.
// Once Samus is back it is applied to each saved area as that area finishes loading. Platforms
// are not carried (MP2's waypoint tracker is not ported yet): they come back as a card load
// leaves them.
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

// --- room state (platform/port_savestate_room.cpp), game thread only -------------------------

// One tagged piece of an object's state; an unknown tag is skipped on load.
struct RoomBlock {
  uint32_t tag = 0;
  std::string data;
};
struct RoomObject {
  uint32_t editorId = 0; // TEditorId::Value(); its AreaNum() is the object's area
  bool active = false;
  std::vector< RoomBlock > blocks;
};
struct RoomArea {
  int32_t area = -1;
  uint32_t assetId = 0; // a restore skips the area when another one sits at that index
};
struct RoomState {
  std::vector< RoomArea > areas;   // the areas that were loaded, so captured
  std::vector< RoomObject > objects;
};
struct RoomApplyStats {
  int applied = 0;   // saved objects found and set
  int removed = 0;   // live objects the state does not have (collected, killed)
  int changed = 0;   // applied objects whose active flag, door or health differed
  int unmatched = 0; // saved objects not in the rebuilt area
  int otherLayer = 0; // live objects of a layer the state has none of, left alone
  int skipped = 0;   // areas left alone: another area asset at the index
};

// A restore in progress: areas already applied, and areas seen loaded on an earlier tick (an
// area is applied one tick after it finishes loading, once its objects took their first messages).
struct RoomApplyProgress {
  std::vector< int32_t > done;
  std::vector< int32_t > seen;
  RoomApplyStats stats;
};

void CaptureRoomState(CStateManager& mgr, RoomState& out);
// Applies the saved areas that are ready now and not yet done; returns how many saved areas are
// still waiting to load.
int ApplyRoomState(CStateManager& mgr, const RoomState& room, RoomApplyProgress& progress);
void EncodeRoomState(const RoomState& room, std::string& out);
bool DecodeRoomState(const std::string& in, RoomState& room);

} // namespace PortSaveState
