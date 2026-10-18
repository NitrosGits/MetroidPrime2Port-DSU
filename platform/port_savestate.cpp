// Save states (room state: port_savestate_room.cpp). See platform/include/port_savestate.h.
//
// The hard part is the load, because nothing in the platform layer can stop the game between two
// frames: Tick runs at the start of a CStateManager update, so the old world is still live for the
// rest of that frame and until CMainFlow restarts it. The load therefore
//  - swaps the game state the way CMain::StreamNewGameState does, but keeps the old CGameState
//    alive (the old CStateManager holds raw CPlayerState pointers into it) until the rebuilt world
//    runs, and only then deletes it;
//  - restarts through CStateManager::PortWarpToWorld, which the debug warp uses;
//  - keeps the outgoing world's pak readable, as PortWarpToWorld does for its own warps, because the
//    new state's constructor and the warp both stash every pak but the destination's.
#include "port_savestate.h"

#include "port_save.h"
#include "port_warp.h"

#include "Kyoto/Alloc/CMemory.hpp"
#include "Kyoto/CPakFile.hpp"
#include "Kyoto/CResFactory.hpp"
#include "Kyoto/CResLoader.hpp"
#include "Kyoto/Math/CTransform4f.hpp"
#include "Kyoto/Math/CVector3f.hpp"
#include "Kyoto/Streams/CBitStreamReader.hpp"
#include "Kyoto/Streams/CBitStreamWriter.hpp"
#include "Kyoto/Streams/CMemoryInStream.hpp"
#include "Kyoto/Streams/CMemoryStreamOut.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CGameGlobalObjects.hpp"
#include "MetroidPrime/CMain.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Player/CGameOptions.hpp"
#include "MetroidPrime/Player/CGameState.hpp"
#include "MetroidPrime/Player/CPersistentOptions.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CWorldState.hpp"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <vector>

namespace PortSaveState {
namespace {

constexpr uint32_t kMagic = 0x5332504d; // "MP2S" read little-endian
// 2: a room state section follows the blob. Version 1 states still load, without room state.
constexpr uint32_t kVersion = 2;
constexpr uint32_t kVersionNoRoom = 1;
// The card slot holds 0xa38 bytes of CGameState stream (SGameFileSlot::mSaveBuffer); the state
// keeps more room than that so a port-only addition to the stream cannot be cut off silently.
constexpr size_t kBlobSize = 0x2000;
// Frames the rebuilt world runs before Samus is put back, and how long a load may take at most.
constexpr int kSettleFrames = 3;
constexpr int kPlaceTimeoutFrames = 60 * 90;
// How long after Samus is placed the saved areas still loading may take to come back.
constexpr int kRoomTimeoutFrames = 60 * 20;

struct State {
  uint32_t worldId = 0;
  int32_t areaIndex = -1;
  uint32_t areaAssetId = 0;
  float pos[3] = {};
  float fwd[3] = {0.f, 1.f, 0.f};
  bool morphed = false;
  double playTime = 0.0;
  int64_t savedAt = 0;
  std::string room;
  std::vector< uint8_t > blob;
  RoomState roomState; // empty for a version 1 state
};

std::atomic< int > sSaveRequest{-1};
std::atomic< int > sLoadRequest{-1};
std::atomic< int > sSelected{1};
std::mutex sMessageMutex;
std::string sMessage;

// A load that has restarted the world and waits to put Samus back.
struct Placement {
  bool active = false;
  State state;
  const CStateManager* oldMgr = nullptr;
  const CWorld* oldWorld = nullptr;
  int settled = 0;
  int waited = 0;
};
Placement sPlace;

// A load that has put Samus back and applies the room state as the saved areas load.
struct RoomRestore {
  bool active = false;
  RoomState state;
  RoomApplyProgress progress;
  int waited = 0;
};
RoomRestore sRoom;
CGameState* sRetired = nullptr; // the game state a load replaced, deleted once the world restarted

void SetMessage(const std::string& message) {
  std::fprintf(stderr, "[savestate] %s\n", message.c_str());
  std::lock_guard< std::mutex > lock(sMessageMutex);
  sMessage = message;
}

std::string SlotPath(int slot) {
  const std::string folder = Folder();
  return folder.empty() ? std::string() : folder + "/slot" + std::to_string(slot) + ".mp2ss";
}

// --- file format: little-endian header, then the CGameState stream ---------------------------

void Put32(std::string& out, uint32_t v) {
  for (int i = 0; i < 4; ++i) {
    out.push_back(static_cast< char >(v >> (8 * i)));
  }
}
void Put64(std::string& out, uint64_t v) {
  Put32(out, static_cast< uint32_t >(v));
  Put32(out, static_cast< uint32_t >(v >> 32));
}
void PutF(std::string& out, float f) {
  uint32_t v;
  std::memcpy(&v, &f, 4);
  Put32(out, v);
}
void PutBytes(std::string& out, const void* data, size_t len) {
  Put32(out, static_cast< uint32_t >(len));
  out.append(static_cast< const char* >(data), len);
}

struct Reader {
  const std::string& in;
  size_t at = 0;
  bool ok = true;
  uint32_t U32() {
    if (at + 4 > in.size()) {
      ok = false;
      return 0;
    }
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
      v |= static_cast< uint32_t >(static_cast< uint8_t >(in[at + i])) << (8 * i);
    }
    at += 4;
    return v;
  }
  uint64_t U64() {
    const uint64_t lo = U32();
    return lo | (static_cast< uint64_t >(U32()) << 32);
  }
  float F() {
    const uint32_t v = U32();
    float f;
    std::memcpy(&f, &v, 4);
    return f;
  }
  std::string Bytes() {
    const uint32_t len = U32();
    if (!ok || at + len > in.size()) {
      ok = false;
      return std::string();
    }
    std::string s = in.substr(at, len);
    at += len;
    return s;
  }
};

std::string Encode(const State& s) {
  std::string out;
  Put32(out, kMagic);
  Put32(out, kVersion);
  Put32(out, s.worldId);
  Put32(out, static_cast< uint32_t >(s.areaIndex));
  Put32(out, s.areaAssetId);
  for (float f : s.pos) {
    PutF(out, f);
  }
  for (float f : s.fwd) {
    PutF(out, f);
  }
  Put32(out, s.morphed ? 1 : 0);
  uint64_t time;
  std::memcpy(&time, &s.playTime, 8);
  Put64(out, time);
  Put64(out, static_cast< uint64_t >(s.savedAt));
  PutBytes(out, s.room.data(), s.room.size());
  PutBytes(out, s.blob.data(), s.blob.size());
  std::string room;
  EncodeRoomState(s.roomState, room);
  PutBytes(out, room.data(), room.size());
  return out;
}

bool Decode(const std::string& in, State& s, std::string& error) {
  Reader r{in};
  if (r.U32() != kMagic) {
    error = "not a save state";
    return false;
  }
  const uint32_t version = r.U32();
  if (version != kVersion && version != kVersionNoRoom) {
    error = "save state version " + std::to_string(version) + " is not supported";
    return false;
  }
  s.worldId = r.U32();
  s.areaIndex = static_cast< int32_t >(r.U32());
  s.areaAssetId = r.U32();
  for (float& f : s.pos) {
    f = r.F();
  }
  for (float& f : s.fwd) {
    f = r.F();
  }
  s.morphed = r.U32() != 0;
  const uint64_t time = r.U64();
  std::memcpy(&s.playTime, &time, 8);
  s.savedAt = static_cast< int64_t >(r.U64());
  s.room = r.Bytes();
  const std::string blob = r.Bytes();
  if (!r.ok || blob.empty()) {
    error = "save state is truncated";
    return false;
  }
  s.blob.assign(blob.begin(), blob.end());
  if (version >= kVersion) {
    const std::string room = r.Bytes();
    if (!r.ok || !DecodeRoomState(room, s.roomState)) {
      error = "save state's room state is damaged";
      return false;
    }
  }
  return true;
}

bool ReadSlot(int slot, State& s, std::string& error) {
  const std::string path = SlotPath(slot);
  if (path.empty()) {
    error = "no user directory, so no save state folder";
    return false;
  }
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    error = "slot " + std::to_string(slot) + " is empty";
    return false;
  }
  const std::string bytes((std::istreambuf_iterator< char >(file)), std::istreambuf_iterator< char >());
  if (!Decode(bytes, s, error)) {
    error = path + ": " + error;
    return false;
  }
  return true;
}

bool WriteSlot(int slot, const State& s, std::string& error) {
  const std::string path = SlotPath(slot);
  if (path.empty()) {
    error = "no user directory, so no save state folder";
    return false;
  }
  std::error_code ec;
  std::filesystem::create_directories(Folder(), ec);
  const std::string temp = path + ".tmp";
  {
    std::ofstream file(temp, std::ios::binary | std::ios::trunc);
    const std::string bytes = Encode(s);
    file.write(bytes.data(), static_cast< std::streamsize >(bytes.size()));
    if (!file) {
      error = "cannot write " + temp;
      return false;
    }
  }
  std::filesystem::rename(temp, path, ec);
  if (ec) {
    error = "cannot replace " + path + ": " + ec.message();
    return false;
  }
  return true;
}

// --- game side -------------------------------------------------------------------------------

// Why the game cannot be saved or loaded from right now, or nullptr when it can.
const char* NotReady(CStateManager& mgr) {
  if (gpGameState == nullptr || gpMain == nullptr || mgr.World() == nullptr ||
      mgr.GetPlayer(0) == nullptr) {
    return "no game running";
  }
  if (!mgr.IsFullyInitialized() || mgr.mQuitGame) {
    return "the world is loading";
  }
  if (sPlace.active || sRoom.active || PortWarp::Pending() ||
      mgr.mPortPendingWarpArea != kInvalidAreaId ||
      mgr.mPortSpawnAtArea != kInvalidAreaId || mgr.mPortWarpQueuedArea != kInvalidAreaId) {
    return "a warp or load is in progress";
  }
  if (mgr.GetGameState() != CStateManager::kGS_Running) {
    return "the game is paused";
  }
  return nullptr;
}

bool Capture(CStateManager& mgr, State& s, std::string& error) {
  if (const char* why = NotReady(mgr)) {
    error = why;
    return false;
  }
  CWorld& world = *mgr.World();
  const TAreaId area = world.GetCurrentAreaId();
  if (!world.DoesAreaExist(area) || world.Area(area) == nullptr) {
    error = "Samus is in no area";
    return false;
  }
  const CGameArea& gameArea = *world.Area(area);
  s.worldId = world.IGetWorldAssetId();
  s.areaIndex = area.Value();
  s.areaAssetId = gameArea.GetAreaAssetId();
  s.room = gameArea.IGetInternalAreaName().c_str();
  if (s.room.empty()) {
    s.room = "Area " + std::to_string(area.Value());
  }

  const CPlayer& player = *mgr.GetPlayer(0);
  const CVector3f pos = player.GetTranslation();
  const CVector3f fwd = player.GetTransform().GetForward();
  s.pos[0] = pos.GetX();
  s.pos[1] = pos.GetY();
  s.pos[2] = pos.GetZ();
  s.fwd[0] = fwd.GetX();
  s.fwd[1] = fwd.GetY();
  s.fwd[2] = fwd.GetZ();
  const CPlayer::EPlayerMorphBallState morph = player.GetMorphballTransitionState();
  s.morphed = morph == CPlayer::kMS_Morphed || morph == CPlayer::kMS_Morphing;
  s.playTime = gpGameState->GetTotalPlayTime();
  s.savedAt = static_cast< int64_t >(std::time(nullptr));

  // A card save records the area it was made in; do the same for the stream, and leave the
  // desired area as it was (the load sets it from the header).
  CWorldState& worldState = gpGameState->CurrentWorldState();
  const CAssetId desired = worldState.GetDesiredAreaAssetId();
  worldState.SetAreaId(area);
  s.blob.assign(kBlobSize, 0);
  {
    CMemoryStreamOut stream(s.blob.data(), s.blob.size());
    CBitStreamWriter writer(stream);
    gpGameState->PutTo(writer);
  }
  worldState.SetDesiredAreaAssetId(desired);
  CaptureRoomState(mgr, s.roomState);
  return true;
}

// PortKeepWorldPakReadable (CStateManager.cpp) for the platform layer: un-stash the pak that owns
// `worldId`, because that world stays live until the restart replaces it.
void KeepWorldPakReadable(CAssetId worldId) {
  if (worldId == kInvalidAssetId || gpResourceFactory == nullptr) {
    return;
  }
  CResLoader& loader = gpResourceFactory->GetResLoader();
  for (int i = 0; i < loader.GetPakCount(); ++i) {
    CPakFile* pak = loader.GetPakFile(i);
    if (pak == nullptr || !pak->IsWorldPak() || !pak->IsCompletelyLoaded()) {
      continue;
    }
    const rstl::vector< rstl::pair< rstl::string, SObjectTag > >& names = pak->GetStringToObjectList();
    for (rstl::vector< rstl::pair< rstl::string, SObjectTag > >::const_iterator it = names.begin();
         it != names.end(); ++it) {
      if (it->second.GetId() == worldId) {
        pak->EnsureWorldPakReady();
        return;
      }
    }
  }
}

// CMain::StreamNewGameState with the state's own stream, keeping the old game state alive.
void SwapGameState(const State& s) {
  const CPersistentOptions systemOptions = gpGameState->SystemOptions();
  const u64 cardSerial = gpGameState->GetCardSerial();
  const rstl::reserved_vector< rstl::vector< uchar >, 3 > states = gpGameState->GetCompressedGameStates();
  const rstl::reserved_vector< rstl::vector< uchar >, 3 > options = gpGameState->GetCompressedGameOptions();
  const rstl::vector< uchar > multiplayerOptions = gpGameState->GetCompressedMultiplayerOptions();
  const CGameOptions gameOptions = gpGameState->GameOptions();

  rstl::single_ptr< CGameState >& owner = gpMain->GetGameGlobalObjects()->GameState();
  delete sRetired;
  sRetired = owner.release();
  {
    CMemoryInStream stream(s.blob.data(), s.blob.size());
    CBitStreamReader reader(stream);
    owner = rs_new CGameState(reader);
  }
  gpGameState = owner.get();
  gpGameState->SystemOptions() = systemOptions;
  gpGameState->SetCompressedGameStates(states);
  gpGameState->SetCompressedGameOptions(options);
  gpGameState->SetCompressedMultiplayerOptions(multiplayerOptions);
  gpGameState->GameOptions() = gameOptions;
  gpGameState->GameOptions().EnsureOptions();
  gpGameState->SetCardSerial(cardSerial);
}

void DoSave(CStateManager& mgr, int slot) {
  State s;
  std::string error;
  if (!Capture(mgr, s, error) || !WriteSlot(slot, s, error)) {
    SetMessage("save to slot " + std::to_string(slot) + " failed: " + error);
    return;
  }
  SetMessage("saved slot " + std::to_string(slot) + " (" + s.room + ")");
}

void DoLoad(CStateManager& mgr, int slot) {
  const std::string what = "load of slot " + std::to_string(slot);
  if (const char* why = NotReady(mgr)) {
    SetMessage(what + " failed: " + why);
    return;
  }
  State s;
  std::string error;
  if (!ReadSlot(slot, s, error)) {
    SetMessage(what + " failed: " + error);
    return;
  }
  // PortWarpToWorld refuses an invalid world; check here, before the game state is swapped.
  if (s.worldId == kInvalidAssetId || s.areaAssetId == kInvalidAssetId) {
    SetMessage(what + " failed: the state names no world or area");
    return;
  }
  std::string undo;
  if (slot != kUndoSlot) {
    State current;
    if (!Capture(mgr, current, error) || !WriteSlot(kUndoSlot, current, error)) {
      undo = " (no undo state: " + error + ")";
    }
  }

  const CAssetId liveWorld = gpGameState->CurrentWorldAssetId();
  SwapGameState(s);
  mgr.PortWarpToWorld(s.worldId, s.areaAssetId);
  if (liveWorld != s.worldId) {
    KeepWorldPakReadable(liveWorld);
  }

  sPlace = Placement();
  sPlace.active = true;
  sPlace.state = s;
  sPlace.oldMgr = &mgr;
  sPlace.oldWorld = mgr.World();
  SetMessage("loading slot " + std::to_string(slot) + " (" + s.room + ")" + undo);
}

// Put Samus back once the rebuilt world runs.
void TickPlacement(CStateManager& mgr) {
  if (++sPlace.waited > kPlaceTimeoutFrames) {
    sPlace.active = false;
    SetMessage("load timed out before the world came back; Samus stays at the area's spawn");
    return;
  }
  // The load quit the old manager, so one that is not quitting is the rebuilt one even if the
  // allocator handed it the old one's address.
  const bool restarted =
      &mgr != sPlace.oldMgr || mgr.World() != sPlace.oldWorld || !mgr.mQuitGame;
  if (!restarted || mgr.mQuitGame || !mgr.IsFullyInitialized() || mgr.World() == nullptr ||
      mgr.GetPlayer(0) == nullptr || mgr.GetGameState() != CStateManager::kGS_Running ||
      PortWarp::Pending() || mgr.mPortPendingWarpArea != kInvalidAreaId ||
      mgr.mPortSpawnAtArea != kInvalidAreaId || mgr.mPortWarpQueuedArea != kInvalidAreaId) {
    sPlace.settled = 0;
    return;
  }
  // The old world, and with it every pointer into the replaced game state, is gone.
  delete sRetired;
  sRetired = nullptr;
  if (mgr.World()->IGetWorldAssetId() != sPlace.state.worldId) {
    sPlace.active = false;
    SetMessage("load landed in another world; Samus stays at its spawn");
    return;
  }
  if (CCameraManager* cameras = mgr.CameraManager(0);
      cameras != nullptr && cameras->IsInCinematicCamera()) {
    sPlace.settled = 0;
    return;
  }
  if (++sPlace.settled < kSettleFrames) {
    return;
  }
  sPlace.active = false;

  const State& s = sPlace.state;
  CPlayer& player = *mgr.GetPlayer(0);
  const CVector3f pos(s.pos[0], s.pos[1], s.pos[2]);
  // Face along the saved forward in the horizontal plane, as the console's `face` does.
  float lx = s.fwd[0];
  float ly = s.fwd[1];
  const float len = std::sqrt(lx * lx + ly * ly);
  if (len < 1e-4f || !std::isfinite(len)) {
    lx = 0.f;
    ly = 1.f;
  } else {
    lx /= len;
    ly /= len;
  }
  const CVector3f look(pos.GetX() + lx, pos.GetY() + ly, pos.GetZ());
  player.Teleport(CTransform4f::LookAt(pos, look, CVector3f::Up()), mgr, true);
  player.SetVelocityWR(CVector3f(0.f, 0.f, 0.f));
  const CPlayer::EPlayerMorphBallState morph = player.GetMorphballTransitionState();
  const bool morphedNow = morph == CPlayer::kMS_Morphed || morph == CPlayer::kMS_Morphing;
  if (morphedNow != s.morphed) {
    player.SetSpawnedMorphBallState(s.morphed ? CPlayer::kMS_Morphed : CPlayer::kMS_Unmorphed, mgr);
  }
  if (CCameraManager* cameras = mgr.CameraManager(0)) {
    cameras->ResetCameras(mgr);
  }
  SetMessage("loaded " + s.room);
  if (!s.roomState.areas.empty()) {
    sRoom = RoomRestore();
    sRoom.active = true;
    sRoom.state = s.roomState;
  }
}

// Apply the room state to each saved area once it is back, Samus's own first.
void TickRoom(CStateManager& mgr) {
  if (mgr.mQuitGame || !mgr.IsFullyInitialized() || mgr.World() == nullptr ||
      mgr.World()->IGetWorldAssetId() != sPlace.state.worldId) {
    sRoom.active = false;
    SetMessage("room state dropped: the world changed before it was applied");
    return;
  }
  const int waiting = ApplyRoomState(mgr, sRoom.state, sRoom.progress);
  if (waiting > 0 && ++sRoom.waited <= kRoomTimeoutFrames) {
    return;
  }
  sRoom.active = false;
  const RoomApplyStats& st = sRoom.progress.stats;
  SetMessage("room state restored: " + std::to_string(sRoom.progress.done.size()) + " of " +
             std::to_string(sRoom.state.areas.size()) + " areas, " + std::to_string(st.applied) +
             " objects applied, " + std::to_string(st.changed) + " changed, " +
             std::to_string(st.removed) + " removed, " + std::to_string(st.unmatched) +
             " unmatched, " + std::to_string(st.otherLayer) + " on other layers" +
             (waiting > 0 ? ", " + std::to_string(waiting) + " areas never loaded" : ""));
}

} // namespace

std::string Folder() {
  const std::string backups = PortSave::BackupsDirectory();
  if (backups.empty()) {
    return std::string();
  }
  return std::filesystem::path(backups).parent_path().string() + "/savestates";
}

Info SlotInfo(int slot) {
  Info info;
  State s;
  std::string error;
  if (!ReadSlot(slot, s, error)) {
    return info;
  }
  info.exists = true;
  info.worldId = s.worldId;
  info.room = s.room;
  info.playTime = s.playTime;
  info.savedAt = s.savedAt;
  info.morphed = s.morphed;
  return info;
}

int SelectedSlot() { return sSelected.load(); }

void SetSelectedSlot(int slot) {
  if (slot >= 1 && slot <= kSlotCount) {
    sSelected.store(slot);
  }
}

void SelectNextSlot() { sSelected.store(sSelected.load() % kSlotCount + 1); }

void RequestSave(int slot) {
  if (slot >= 0 && slot <= kSlotCount) {
    sSaveRequest.store(slot);
  }
}

void RequestLoad(int slot) {
  if (slot >= 0 && slot <= kSlotCount) {
    sLoadRequest.store(slot);
  }
}

std::string LastMessage() {
  std::lock_guard< std::mutex > lock(sMessageMutex);
  return sMessage;
}

// Test hook: MP2_SAVESTATE_AT=<save frame>,<load frame> queues a save of the selected slot and
// later a load of it, counted in Tick calls (game updates). Lets a headless run exercise the
// round trip, since the harness cannot press F5/F9.
void TestHook() {
  static const char* env = std::getenv("MP2_SAVESTATE_AT");
  static long saveAt = -1;
  static long loadAt = -1;
  static bool parsed = false;
  static long frame = 0;
  if (!parsed) {
    parsed = true;
    if (env != nullptr) {
      std::sscanf(env, "%ld,%ld", &saveAt, &loadAt);
    }
  }
  ++frame;
  if (frame == saveAt) {
    RequestSave(SelectedSlot());
  }
  if (frame == loadAt) {
    RequestLoad(SelectedSlot());
  }
}

void Tick(CStateManager& mgr) {
  TestHook();
  if (sPlace.active) {
    TickPlacement(mgr);
  } else if (sRoom.active) {
    TickRoom(mgr);
  }
  const int save = sSaveRequest.exchange(-1);
  if (save >= 0) {
    DoSave(mgr, save);
  }
  const int load = sLoadRequest.exchange(-1);
  if (load >= 0) {
    DoLoad(mgr, load);
  }
}

} // namespace PortSaveState
