// Save states, phase 2: room state, the live script objects of the loaded areas (MP1's
// port_savestate_room.cpp, adapted to MP2). See platform/include/port_savestate.h.
//
// An object is keyed by its editor id, whose area bits name its area. Capture records, for each
// loaded area, every scripted object that still exists (a dead enemy counts as gone) with tagged
// blocks of state. Apply deletes the live objects of a saved area that the state does not have -
// a collected pickup or a killed enemy stays gone - and puts the saved state back on the rest.

#include "port_savestate.h"

#include "Kyoto/Math/CTransform4f.hpp"
#include "Kyoto/Math/CVector3f.hpp"
#include "MetroidPrime/CActor.hpp"
#include "MetroidPrime/CEntity.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CObjectList.hpp"
#include "MetroidPrime/CPhysicsActor.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCounter.hpp"
#include "MetroidPrime/ScriptObjects/CScriptDoor.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTimer.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>

// Friend of CScriptTimer, CScriptCounter and CScriptDoor (their TARGET_PC blocks): their state is
// private.
struct PortRoomState {
  static CScriptDoor::ELockState DoorLock(const CScriptDoor& d) { return d.mLockState; }
  static float TimerTime(const CScriptTimer& t) { return t.mTime; }
  static void SetTimer(CScriptTimer& t, float time, bool timing) {
    t.mTime = time;
    t.mIsTiming = timing;
  }
  static void SetCounter(CScriptCounter& c, int current) { c.mCurrent = current; }
};

namespace PortSaveState {
namespace {

enum : uint32_t {
  kTagTransform = 1,
  kTagMotion = 2,
  kTagHealth = 3,
  kTagTimer = 5,
  kTagCounter = 6,
  kTagDoor = 7,
};

// Per-object lines a restore logs, at most; the summary line always follows.
constexpr int kMaxChangeLines = 24;
constexpr uint32_t kMaxCount = 1u << 20; // sanity bound for decoded counts

struct BlockWriter {
  std::string bytes;
  void U32(uint32_t v) {
    for (int i = 0; i < 4; ++i) {
      bytes.push_back(static_cast< char >(v >> (8 * i)));
    }
  }
  void F32(float f) {
    uint32_t v;
    std::memcpy(&v, &f, 4);
    U32(v);
  }
  void Vec(const CVector3f& v) {
    F32(v.GetX());
    F32(v.GetY());
    F32(v.GetZ());
  }
  void Bytes(const std::string& s) {
    U32(static_cast< uint32_t >(s.size()));
    bytes.append(s);
  }
};

struct BlockReader {
  const std::string& in;
  size_t at = 0;
  bool ok = true;
  explicit BlockReader(const std::string& s) : in(s) {}
  uint32_t U32() {
    if (!ok || at + 4 > in.size()) {
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
  float F32() {
    const uint32_t v = U32();
    float f;
    std::memcpy(&f, &v, 4);
    return f;
  }
  // A finite vector, or false.
  bool Vec(CVector3f& out) {
    const float x = F32();
    const float y = F32();
    const float z = F32();
    if (!ok || !(x == x) || !(y == y) || !(z == z) || x > 1e7f || x < -1e7f || y > 1e7f ||
        y < -1e7f || z > 1e7f || z < -1e7f) {
      return false;
    }
    out = CVector3f(x, y, z);
    return true;
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

const RoomBlock* FindBlock(const RoomObject& obj, uint32_t tag) {
  for (const RoomBlock& b : obj.blocks) {
    if (b.tag == tag) {
      return &b;
    }
  }
  return nullptr;
}

void PutTransform(BlockWriter& w, const CTransform4f& xf) {
  const float m[12] = {xf.Get00(), xf.Get01(), xf.Get02(), xf.Get03(), xf.Get10(), xf.Get11(),
                       xf.Get12(), xf.Get13(), xf.Get20(), xf.Get21(), xf.Get22(), xf.Get23()};
  for (float f : m) {
    w.F32(f);
  }
}

bool GetTransform(BlockReader& r, CTransform4f& out) {
  float m[12];
  for (float& f : m) {
    f = r.F32();
  }
  if (!r.ok) {
    return false;
  }
  for (float f : m) {
    if (!(f == f) || f > 1e7f || f < -1e7f) {
      return false;
    }
  }
  out = CTransform4f(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11]);
  return true;
}

template < typename F >
void ForEachObject(CStateManager& mgr, F&& fn) {
  CObjectList& list = mgr.ObjectListById(kOL_All);
  for (int idx = list.GetFirstObjectIndex(); idx != -1; idx = list.GetNextObjectIndex(idx)) {
    if (CEntity* ent = list[idx]) {
      fn(*ent);
    }
  }
}

const CGameArea* ReadyArea(const CWorld& world, int area) {
  if (area < 0 || area >= world.GetNumAreas()) {
    return nullptr;
  }
  const CGameArea* a = world.GetArea(TAreaId(area));
  return a != nullptr && a->IsLoaded() ? a : nullptr;
}

// An editor id's area and layer (its top six bits), the unit an area spawns or skips.
uint32_t LayerKey(uint32_t editorId) {
  return (TEditorId(editorId).AreaNum() << 8) | (editorId >> 26);
}

bool IsScripted(const CEntity& ent) { return ent.GetEditorId().value != kInvalidEditorId.value; }

// A dead enemy is gone for the room even while its corpse still runs.
bool IsGone(CEntity& ent) {
  const CPatterned* patterned = TCastToPtr< CPatterned >(&ent);
  return patterned != nullptr && !patterned->GetAlive();
}

void CaptureObject(CEntity& ent, RoomObject& obj) {
  obj.editorId = ent.GetEditorId().value;
  obj.active = ent.GetActive();

  if (CActor* actor = TCastToPtr< CActor >(&ent)) {
    if (const CHealthInfo* health = actor->HealthInfo()) {
      BlockWriter w;
      w.F32(health->GetHP());
      obj.blocks.push_back({kTagHealth, w.bytes});
    }
  }
  CPhysicsActor* physics = TCastToPtr< CPhysicsActor >(&ent);
  if (physics != nullptr && TCastToPtr< CScriptDoor >(&ent) == nullptr) {
    // Inert props keep the card load's pose; enemies and anything moving are saved.
    if (TCastToPtr< CPatterned >(&ent) != nullptr || physics->GetVelocityWR().IsNonZero() ||
        physics->GetMomentumWR().IsNonZero()) {
      BlockWriter xf;
      PutTransform(xf, physics->GetTransform());
      obj.blocks.push_back({kTagTransform, xf.bytes});
      BlockWriter motion;
      motion.Vec(physics->GetVelocityWR());
      motion.Vec(physics->GetMomentumWR());
      obj.blocks.push_back({kTagMotion, motion.bytes});
    }
  }
  if (CScriptTimer* timer = TCastToPtr< CScriptTimer >(&ent)) {
    BlockWriter w;
    w.F32(PortRoomState::TimerTime(*timer));
    w.U32(timer->IsTiming() ? 1 : 0);
    obj.blocks.push_back({kTagTimer, w.bytes});
  }
  if (CScriptCounter* counter = TCastToPtr< CScriptCounter >(&ent)) {
    BlockWriter w;
    w.U32(static_cast< uint32_t >(counter->GetCurrent()));
    obj.blocks.push_back({kTagCounter, w.bytes});
  }
  if (CScriptDoor* door = TCastToPtr< CScriptDoor >(&ent)) {
    // Open, then the lock (MP1's door block carries open plus close flags; Echoes' doors have a
    // lock machine instead, and a state written before it was saved has only the first word).
    BlockWriter w;
    w.U32(door->IsOpen() ? 1 : 0);
    w.U32(static_cast< uint32_t >(PortRoomState::DoorLock(*door)));
    obj.blocks.push_back({kTagDoor, w.bytes});
  }
}

struct ChangeLog {
  int lines = 0;
  void Note(uint32_t editorId, const char* what) {
    if (lines++ < kMaxChangeLines) {
      std::fprintf(stderr, "[savestate] room: object %08x %s\n", editorId, what);
    }
  }
};

// Returns whether a visible part of the object's state (active flag, health, door) differed.
bool ApplyObject(CStateManager& mgr, CEntity& ent, const RoomObject& obj, ChangeLog& log) {
  bool changed = false;
  if (ent.GetActive() != obj.active) {
    ent.SetActive(obj.active);
    log.Note(obj.editorId, obj.active ? "activated" : "deactivated");
    changed = true;
  }
  if (CActor* actor = TCastToPtr< CActor >(&ent)) {
    if (const RoomBlock* b = FindBlock(obj, kTagHealth)) {
      BlockReader r(b->data);
      const float hp = r.F32();
      CHealthInfo* health = actor->HealthInfo();
      if (r.ok && hp == hp && health != nullptr && health->GetHP() != hp) {
        health->SetHP(hp);
        log.Note(obj.editorId, "health restored");
        changed = true;
      }
    }
  }
  if (CPhysicsActor* physics = TCastToPtr< CPhysicsActor >(&ent)) {
    if (const RoomBlock* b = FindBlock(obj, kTagTransform)) {
      BlockReader r(b->data);
      CTransform4f xf = CTransform4f::Identity();
      if (GetTransform(r, xf)) {
        physics->SetTransform(xf);
      }
    }
    if (const RoomBlock* b = FindBlock(obj, kTagMotion)) {
      BlockReader r(b->data);
      CVector3f velocity(0.f, 0.f, 0.f);
      CVector3f momentum(0.f, 0.f, 0.f);
      if (r.Vec(velocity) && r.Vec(momentum)) {
        physics->SetVelocityWR(velocity);
        physics->SetMomentumWR(momentum);
      }
    }
  }
  if (CScriptTimer* timer = TCastToPtr< CScriptTimer >(&ent)) {
    if (const RoomBlock* b = FindBlock(obj, kTagTimer)) {
      BlockReader r(b->data);
      const float time = r.F32();
      const bool timing = r.U32() != 0;
      if (r.ok && time == time) {
        PortRoomState::SetTimer(*timer, time, timing);
      }
    }
  }
  if (CScriptCounter* counter = TCastToPtr< CScriptCounter >(&ent)) {
    if (const RoomBlock* b = FindBlock(obj, kTagCounter)) {
      BlockReader r(b->data);
      const int current = static_cast< int >(r.U32());
      if (r.ok && current != counter->GetCurrent()) {
        PortRoomState::SetCounter(*counter, current);
        log.Note(obj.editorId, "counter restored");
      }
    }
  }
  if (CScriptDoor* door = TCastToPtr< CScriptDoor >(&ent)) {
    if (const RoomBlock* b = FindBlock(obj, kTagDoor)) {
      BlockReader r(b->data);
      const bool open = r.U32() != 0;
      // The door's own messages run its state machine: an open request waits for the area
      // behind it the way a shot door does, and a close swings it shut.
      if (r.ok && open != door->IsOpen()) {
        mgr.SendScriptMsg(door, kInvalidUniqueId, open ? kSM_Open : kSM_Close);
        log.Note(obj.editorId, open ? "door opened" : "door closed");
        changed = true;
      }
      // The lock, through the same messages a script sends: kSM_Lock locks once Samus is in the
      // door's area and it is shut (closing it first), kSM_Unlock plays the unlock. A lock that
      // was mid-animation counts as where it was heading.
      const CScriptDoor::ELockState saved = static_cast< CScriptDoor::ELockState >(r.U32());
      if (r.ok) {
        const CScriptDoor::ELockState live = PortRoomState::DoorLock(*door);
        const bool wantLocked = saved == CScriptDoor::kLS_Pending ||
                                saved == CScriptDoor::kLS_Locking || saved == CScriptDoor::kLS_Locked;
        const bool isLocked = live == CScriptDoor::kLS_Pending || live == CScriptDoor::kLS_Locking ||
                              live == CScriptDoor::kLS_Locked;
        if (wantLocked != isLocked) {
          mgr.SendScriptMsg(door, kInvalidUniqueId, wantLocked ? kSM_Lock : kSM_Unlock);
          log.Note(obj.editorId, wantLocked ? "door locked" : "door unlocked");
          changed = true;
        }
      }
    }
  }
  return changed;
}

bool Contains(const std::vector< int32_t >& v, int32_t x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

} // namespace

void CaptureRoomState(CStateManager& mgr, RoomState& out) {
  out = RoomState();
  const CWorld* world = mgr.World();
  if (world == nullptr) {
    return;
  }
  std::set< int > areas;
  for (int i = 0; i < world->GetNumAreas(); ++i) {
    if (const CGameArea* a = ReadyArea(*world, i)) {
      areas.insert(i);
      out.areas.push_back({i, static_cast< uint32_t >(a->GetAreaAssetId())});
    }
  }
  ForEachObject(mgr, [&](CEntity& ent) {
    if (!IsScripted(ent) || areas.count(ent.GetEditorId().AreaNum()) == 0 || IsGone(ent)) {
      return;
    }
    RoomObject obj;
    CaptureObject(ent, obj);
    out.objects.push_back(std::move(obj));
  });
  std::fprintf(stderr, "[savestate] room: captured %zu objects in %zu areas\n", out.objects.size(),
               out.areas.size());
}

int ApplyRoomState(CStateManager& mgr, const RoomState& room, RoomApplyProgress& progress) {
  const CWorld* world = mgr.World();
  if (world == nullptr) {
    return static_cast< int >(room.areas.size());
  }
  RoomApplyStats& stats = progress.stats;
  std::set< int > now;
  int waiting = 0;
  for (const RoomArea& ra : room.areas) {
    if (Contains(progress.done, ra.area)) {
      continue;
    }
    const CGameArea* a = ReadyArea(*world, ra.area);
    if (a == nullptr) {
      ++waiting;
      continue;
    }
    if (static_cast< uint32_t >(a->GetAreaAssetId()) != ra.assetId) {
      std::fprintf(stderr, "[savestate] room: area %d is another area now; left as loaded\n",
                   ra.area);
      ++stats.skipped;
      progress.done.push_back(ra.area);
      continue;
    }
    if (!Contains(progress.seen, ra.area)) {
      progress.seen.push_back(ra.area);
      ++waiting;
      continue;
    }
    now.insert(ra.area);
  }
  if (now.empty()) {
    return waiting;
  }

  ChangeLog log;
  std::set< uint32_t > present;
  std::set< uint32_t > savedLayers;
  for (const RoomObject& obj : room.objects) {
    present.insert(TEditorId(obj.editorId).Value());
    savedLayers.insert(LayerKey(obj.editorId));
  }
  std::map< uint32_t, int > otherLayers;
  std::map< uint32_t, int > unmatchedLayers;
  std::vector< std::pair< TUniqueId, uint32_t > > doomed;
  ForEachObject(mgr, [&](CEntity& ent) {
    if (!IsScripted(ent) || now.count(ent.GetEditorId().AreaNum()) == 0) {
      return;
    }
    // A layer the state has no object of was switched on after its area spawned (the reload
    // follows the saved layer flags, the live room did not yet): leave it as the reload made it.
    if (savedLayers.count(LayerKey(ent.GetEditorId().value)) == 0) {
      ++stats.otherLayer;
      ++otherLayers[LayerKey(ent.GetEditorId().value)];
      return;
    }
    if (present.count(ent.GetEditorId().Value()) == 0 || IsGone(ent)) {
      doomed.push_back({ent.GetUniqueId(), ent.GetEditorId().value});
    }
  });
  for (const std::pair< TUniqueId, uint32_t >& d : doomed) {
    log.Note(d.second, "removed (gone in the state)");
    mgr.DeleteObjectRequest(d.first);
    ++stats.removed;
  }
  for (const RoomObject& obj : room.objects) {
    if (now.count(TEditorId(obj.editorId).AreaNum()) == 0) {
      continue;
    }
    CEntity* ent = mgr.ObjectById(mgr.GetIdForScript(TEditorId(obj.editorId)));
    if (ent == nullptr) {
      ++stats.unmatched;
      ++unmatchedLayers[LayerKey(obj.editorId)];
      continue;
    }
    if (ApplyObject(mgr, *ent, obj, log)) {
      ++stats.changed;
    }
    ++stats.applied;
  }
  if (log.lines > kMaxChangeLines) {
    std::fprintf(stderr, "[savestate] room: ... %d more object lines\n",
                 log.lines - kMaxChangeLines);
  }
  for (const std::pair< const uint32_t, int >& l : unmatchedLayers) {
    std::fprintf(stderr,
                 "[savestate] room: area %u layer %u: %d saved objects not in the reloaded room\n",
                 l.first >> 8, l.first & 0xff, l.second);
  }
  for (const std::pair< const uint32_t, int >& l : otherLayers) {
    std::fprintf(stderr,
                 "[savestate] room: area %u layer %u: %d live objects left as loaded (no saved "
                 "object)\n",
                 l.first >> 8, l.first & 0xff, l.second);
  }
  for (int area : now) {
    progress.done.push_back(area);
    std::fprintf(stderr, "[savestate] room: area %d restored\n", area);
  }
  return waiting;
}

void EncodeRoomState(const RoomState& room, std::string& out) {
  BlockWriter w;
  w.U32(static_cast< uint32_t >(room.areas.size()));
  for (const RoomArea& a : room.areas) {
    w.U32(static_cast< uint32_t >(a.area));
    w.U32(a.assetId);
  }
  w.U32(static_cast< uint32_t >(room.objects.size()));
  for (const RoomObject& obj : room.objects) {
    w.U32(obj.editorId);
    w.U32(obj.active ? 1 : 0);
    w.U32(static_cast< uint32_t >(obj.blocks.size()));
    for (const RoomBlock& b : obj.blocks) {
      w.U32(b.tag);
      w.Bytes(b.data);
    }
  }
  out = std::move(w.bytes);
}

bool DecodeRoomState(const std::string& in, RoomState& room) {
  room = RoomState();
  BlockReader r(in);
  const uint32_t areas = r.U32();
  if (!r.ok || areas > kMaxCount) {
    return false;
  }
  for (uint32_t i = 0; i < areas && r.ok; ++i) {
    RoomArea a;
    a.area = static_cast< int32_t >(r.U32());
    a.assetId = r.U32();
    room.areas.push_back(a);
  }
  const uint32_t objects = r.U32();
  if (!r.ok || objects > kMaxCount) {
    room = RoomState();
    return false;
  }
  room.objects.reserve(objects);
  for (uint32_t i = 0; i < objects && r.ok; ++i) {
    RoomObject obj;
    obj.editorId = r.U32();
    obj.active = r.U32() != 0;
    const uint32_t blocks = r.U32();
    if (blocks > 64) {
      r.ok = false;
      break;
    }
    for (uint32_t b = 0; b < blocks && r.ok; ++b) {
      RoomBlock block;
      block.tag = r.U32();
      block.data = r.Bytes();
      obj.blocks.push_back(std::move(block));
    }
    room.objects.push_back(std::move(obj));
  }
  if (!r.ok) {
    room = RoomState();
    return false;
  }
  return true;
}

} // namespace PortSaveState
