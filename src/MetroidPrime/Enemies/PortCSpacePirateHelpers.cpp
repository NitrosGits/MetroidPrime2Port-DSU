// The SpacePirate REL's (module 72) stateless helper half: the bodies that are neither an FSM
// state nor an FSM trigger, ported from the REL's disassembly
// (build/G2ME01/SpacePirate/asm/auto_00_00000000_text.s). The module note with the H/T/B split
// and every offset is docs/module-analysis/SpacePirate_helpers.md; the states and the triggers
// are in PortCSpacePirateStates.cpp and PortCSpacePirateTriggers.cpp, the module's own file is
// PortCSpacePirate.cpp.
//
// Every body below carries the retail offset it came from. A `// @ fn_72_<off>` marker means the
// body is retail's and every live arm of it ran (the arm table is in the comment above it).
#include "MetroidPrime/Enemies/CSpacePirate.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CVector2f.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CValidEntityPredicate.hpp"
#include "MetroidPrime/Enemies/CBouncyGrenade.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "rstl/algorithm.hpp"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>

namespace {
// MP2_DEBUG_SPACEPIRATE=1: one line per entry of the helpers below (arg is a detail value). The
// same gate and line format as PortCSpacePirate.cpp's own copy.
void TraceCall(const CSpacePirate& self, const char* what, int arg) {
  static const bool sOn = getenv("MP2_DEBUG_SPACEPIRATE") != nullptr;
  if (sOn) {
    fprintf(stderr, "[spacepirate] %u %s %d\n", self.GetUniqueId().Value(), what, arg);
  }
}
} // namespace

// Retail fn_72_10CE4 (0x40): the module's own CValidEntityPredicate (its vtable is lbl_72_data_AD4)
// - the object with the id has to be a CScriptWaypoint: GetObjectById (asm 0x10D00) and the cast
// (asm 0x10D04) with the null test (asm 0x10D08-0x10D10). The WallHang state walks its cover
// point's kSS_Arrived/kSM_Next connection with this predicate (fn_72_69AC).
class CSpacePirateWaypointPredicate : public CValidEntityPredicate {
public:
  ~CSpacePirateWaypointPredicate() override {}
  bool IsValid(const CStateManager& mgr, TUniqueId id) const override {
    return TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(id)) != nullptr;
  }
};

// Retail fn_72_2EA8 (0xA4): the Think helper that keeps the pirate's sfx emitter (the handle at
// 0xc00) where the pirate is. The handle is 0 (asm 0x2EBC) -> nothing happens; a handle that is
// playing (asm 0x2ED4) is updated; a handle that is only queued (asm 0x2EEC) is updated as well,
// but one that is neither playing nor queued clears the handle (asm 0x2F30-0x2F34). The emitter is
// moved to the actor's position (0x54, asm 0x2F04) and the world's Y axis of the transform - the
// transform's second column (0x28, 0x38 and 0x48 are the y of its three rows, asm 0x2EF8-0x2F1C)
// - with the volume 0x7f (asm 0x2F10).
// Arms (the 2026-10-21 run, tools/cov with tools/cov/specs/spacepirate_helpers.py, 08_sand_casualpirates
// and the gdb forcing run): the handle is 0 (asm 0x2EC0-0x2EC4) 11468 entries; the handle is playing
// (asm 0x2ED4) 45 entries, the emitter updated; the handle is queued but not playing (asm 0x2EEC)
// 3 entries, the emitter updated; the handle is neither playing nor queued (asm 0x2EF4 -> 0x2F30)
// 1 entry, the handle cleared - forced (gdb: a bogus handle 0x3039 written into 0xc00 of a live
// pirate). The playing / queued entries need the handle the retail's KnockBack stores at 0xc00
// (asm 0xC944 / 0xC9B8), which the port now does - they are forced (gdb: CSfxManager::AddEmitter
// of the pirate's hurled sound, its handle written into 0xc00).
// @ fn_72_2EA8
void CSpacePirate::UpdateSfxEmitter() {
  const CSfxHandle handle = xc00_;
  if (handle == CSfxHandle::NullHandle()) {
    return; // 0x2EC0-0x2EC4
  }
  if (!CSfxManager::IsPlaying(handle) && !CSfxManager::IsQueued(handle)) {
    xc00_ = CSfxHandle::NullHandle(); // 0x2F30-0x2F34
    return;
  }
  // 0x2EF8-0x2F28: UpdateEmitter(handle, position, the transform's Y axis, 0x7f).
  CSfxManager::UpdateEmitter(handle, GetTranslation(), GetTransform().GetColumn(kDY), 0x7f);
}

// Retail fn_72_1854 (0x68): the actor's own bounding box in the optional that GetTouchBounds
// returns - the box is copied out of CPhysicsActor::GetBoundingBox (asm 0x1868) and the optional's
// valid flag is set (asm 0x1870-0x1874). Branchless, so the run's calls prove all of it.
// @ fn_72_1854
rstl::optional_object< CAABox > CSpacePirate::GetActorTouchBounds() const {
  return GetBoundingBox();
}

// Retail fn_72_CA94 (0x9C): the cinematic camera takes the AI block away - the material is removed
// and the include list drops it (asm 0xCA94-0xCAF0). No branch, so the one forced call of
// 2026-10-21 (gdb: SetCinematicCollision on a live pirate) proves all of it.
// @ fn_72_CA94
void CSpacePirate::SetCinematicCollision(CStateManager& mgr) {
  RemoveMaterial(kMT_AIBlock, mgr);
  CMaterialList include = GetMaterialFilter().GetIncludeList();
  include.Remove(kMT_AIBlock);
  SetMaterialFilter(
      CMaterialFilter::MakeIncludeExclude(include, GetMaterialFilter().GetExcludeList()));
}

// Retail fn_72_C9DC (0xB8): the AI block comes back - the material is added and the include list
// gets it (asm 0xC9DC-0xCA90). No branch, so the one forced call of 2026-10-21 (gdb:
// SetNonCinematicCollision on a live pirate) proves all of it.
// @ fn_72_C9DC
void CSpacePirate::SetNonCinematicCollision(CStateManager& mgr) {
  AddMaterial(kMT_AIBlock, mgr);
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
      GetMaterialFilter().GetIncludeList().Union(CMaterialList(kMT_AIBlock)),
      GetMaterialFilter().GetExcludeList()));
}

TUniqueId CSpacePirate::ChooseTeamTarget(CStateManager& mgr) {
  if (xba0_teamAiMgrId != kInvalidUniqueId) {
    CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(xba0_teamAiMgrId));
    if (team != nullptr && team->IsTeamMemberInRange(mgr, *this, 20.f)) {
      return team->FindBestIndividualAttackTarget(mgr, *this);
    }
  }
  return CScriptTeamAiMgr::ChoosePlayer(mgr, *this);
}

// Keeps the current target unless it has gone inactive, in which case the squad picks one; a distant target is dropped for a player within 10 units.
TUniqueId CSpacePirate::ChooseTarget(CStateManager& mgr) {
  float nearest = 10.f;
  float currentDist = 0.f;
  TUniqueId nearestId = kInvalidUniqueId;
  TUniqueId result = mAttackTarget;
  for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
    const CPlayer* player = mgr.GetPlayer(i);
    if (player == nullptr) {
      continue;
    }
    if (player->GetUniqueId() == mAttackTarget) {
      if (!player->GetActive()) {
        result = ChooseTeamTarget(mgr);
      } else {
        currentDist = (player->GetTranslation() - GetTranslation()).Magnitude();
      }
    } else {
      const float dist = (player->GetTranslation() - GetTranslation()).Magnitude();
      if (dist < nearest) {
        nearest = dist;
        nearestId = player->GetUniqueId();
      }
    }
  }
  if (currentDist > 30.f && nearest < 10.f) {
    result = nearestId;
  }
  return result;
}

// Whether the next shot is a frenzy burst; a roll of 2..4 shots is queued and counted down.
// Retail fn_72_3220 (0x17C): the 0x8f8 byte's bit 24 (the pending frenzy chance, asm 0x3244-0x324C)
// is cleared and a roll below 25 % (asm 0x3284) arms the frenzy; the module's charge-player list
// (bss_40, asm 0x3298-0x32A0) arms it too; a morphed target (asm 0x32C4-0x32D4) arms it; and a
// health below 0.3 (rodata B68) times the initial HP (0xa18) with a roll below 60 % (asm 0x3330)
// and the low-health frenzy timer below 0.5 (rodata B44, asm 0x3340-0x3348) arms it. An armed
// frenzy queues 2..4 shots (asm 0x3364) and the counter runs down (asm 0x336C-0x3378).
bool CSpacePirate::ShouldFrenzy(CStateManager& mgr) {
  bool reset = false;
  if (x8f8_24_pendingFrenzyChance) {
    x8f8_24_pendingFrenzyChance = false;
    if (mgr.Random()->Next() % 100 < 25) {
      reset = true;
    }
  }
  // 0x3298-0x32A4: the module's charge-player list (bss_40) is not empty.
  if (!mChargePlayerList.empty()) {
    reset = true;
  }
  const CPlayer* target = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mAttackTarget));
  if (target != nullptr && target->GetMorphballTransitionState() == CPlayer::kMS_Morphed) {
    reset = true;
  }
  if (GetHealthInfo()->GetHP() < 0.3f * xa18_ && mgr.Random()->Next() % 100 < 60 &&
      xb28_lowHealthFrenzyTimer < 0.5f) {
    reset = true;
  }
  if (reset) {
    x8fc_frenzyFrames = mgr.Random()->Range(2, 4);
  }
  return --x8fc_frenzyFrames >= 0;
}

// Retail fn_72_3D7C: the launcher's ballistic solve. Ten candidate angles are tried from 0 (or from
// pi/4 downwards when the target is above the gun); the first one whose required speed lands inside
// [minLaunchSpeed, maxLaunchSpeed] wins, otherwise the closest miss is kept.
static void SolveGrenadeLaunch(const CVector3f& target, const CVector3f& origin, float minLaunchSpeed,
                               float maxLaunchSpeed, float& angle, float& speed) {
  const float heightDelta = target.GetZ() - origin.GetZ();
  const float distance =
      CVector2f(target.GetX() - origin.GetX(), target.GetY() - origin.GetY()).Magnitude();
  const float minSpeedSq = minLaunchSpeed * minLaunchSpeed;
  const float maxSpeedSq = maxLaunchSpeed * maxLaunchSpeed;

  float startAngle = 0.f;
  float angleStep = 0.07853982f; // pi/40
  if (target.GetZ() > origin.GetZ()) {
    startAngle = 0.7853982f; // pi/4
    angleStep = -angleStep;
  }
  const float halfGravityDist = 0.5f * kDefaultGravityAccel * distance;
  const float lhs = distance * halfGravityDist;

  float bestAngle = 0.f;
  float bestSpeed = minLaunchSpeed;
  float bestError = FLT_MAX;
  for (float i = 0.f; i < 10.f; i += 1.f) {
    const float candidate = angleStep * i + startAngle;
    const float cosine = CMath::FastCosR(candidate);
    const float sine = CMath::FastSinR(candidate);
    // Retail's fmsubs leaves the terms in this order: distance*(cos*sin) - height*cos^2.
    const float denominator = distance * (cosine * sine) - heightDelta * (cosine * cosine);
    if (denominator > 1.1920929e-7f) {
      const float speedSq = lhs / denominator;
      if (speedSq >= minSpeedSq && speedSq <= maxSpeedSq) {
        bestAngle = candidate;
        bestSpeed = CMath::SqrtF(speedSq);
        break; // retail branches straight past the loop counter here
      }
      const float error = speedSq > maxSpeedSq ? speedSq - maxSpeedSq : minSpeedSq - speedSq;
      if (error < bestError) {
        bestAngle = candidate;
        bestSpeed = CMath::SqrtF(speedSq);
        bestError = error;
      }
    }
  }
  angle = bestAngle;
  speed = bestSpeed;
}

// Retail fn_72_4030: the point the grenade is thrown at - the target's aim position, lowered to half
// the player's eye height, then (for a bouncing launcher) pulled 70% of the way back towards the gun
// when the aim is at most 2 m above the pirate (asm: cror lt/eq -> <=, not an equality test).
static CVector3f GetGrenadeAimPoint(const CActor& target, const CStateManager& mgr,
                                    const CVector3f& piratePos, uint numBounces) {
  CVector3f aim = target.GetAimPosition(mgr, 0.5f);
  const CPlayer* player = TCastToConstPtr< CPlayer >(target);
  if (player && player->GetMorphballTransitionState() == CPlayer::kMS_Unmorphed) {
    aim -= CVector3f(0.f, 0.f, 0.5f * player->GetEyeHeight());
  }
  if (numBounces != 0 && aim.GetZ() <= piratePos.GetZ() + 2.0f) {
    aim = piratePos + 0.7f * (aim - piratePos);
  }
  return aim;
}

// Retail fn_72_4174: launches one bouncy grenade from the gun locator, aimed by fn_72_4030 and
// fn_72_3D7C, with retail's default model / actor parameters and 5 m player-explode distance.
void CSpacePirate::SpawnGrenade(CStateManager& mgr) {
  --xc58_;
  const CTransform4f gunXf = GetLctrTransform(xa86_gunSeg);
  const CVector3f origin = gunXf.GetTranslation();
  float angle = 0.f;
  float speed = mData.mGrenadeMinLaunchSpeed;
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mAttackTarget));
  if (!target) {
    return;
  }
  const CVector3f aim =
      GetGrenadeAimPoint(*target, mgr, GetTranslation(), mData.mGrenadeNumBounces);
  SolveGrenadeLaunch(aim, origin, mData.mGrenadeMinLaunchSpeed, mData.mGrenadeMaxLaunchSpeed,
                     angle, speed);

  CVector3f flat = aim - origin;
  flat.SetZ(0.f);
  const CVector3f forward = gunXf.GetForward();
  CVector3f horizontal = flat.CanBeNormalized() ? flat.AsNormalized() : forward;
  if (CVector3f::GetAngleDiff(forward, horizontal) > 0.7853982f) {
    horizontal = CVector3f::Slerp(forward, horizontal, CRelAngle::FromRadians(0.7853982f));
  }
  const CVector3f launchDir =
      CVector3f::Slerp(horizontal, CVector3f::Up(), CRelAngle::FromRadians(angle));
  const CTransform4f xf = CTransform4f::LookAt(origin, origin + launchDir, CVector3f::Up());

  CBouncyGrenade* grenade = rs_new CBouncyGrenade(
      mgr.AllocateUniqueId(), rstl::string_l("Bouncy Grenade"),
      CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId), xf,
      CModelData(), CActorParameters(), GetUniqueId(), speed, mData.BuildGrenadeData(), 5.f,
      CAABox::MakeMaxInvertedBox(), kInvalidUniqueId, 0.f, 0, nullptr, nullptr);
  if (grenade) {
    mgr.AddObject(*grenade);
  }
}

// Retail fn_72_51C8: a live trooper with no warp point and no pending warp enters "WarpOut".
void CSpacePirate::TrooperWarpOut(CStateManager& mgr, bool flag) {
  if (mAlive && !x8fa_25_ && xc04_warpPoint == -1 && x8f6_24_trooper) {
    mStateMachine->SetState(mgr, *this, rstl::string_l("WarpOut"));
    x8fa_27_ = flag;
  }
}

// Retail fn_72_E3D8 adds a force-field trigger's push to the ragdoll's torso impulse once the
// ragdoll is primed; with no ragdoll (xb30_ is never set) it is CPatterned::Touch.
// Retail fn_72_E3D8 (0xC0): the base class's touch first, then a force-field trigger's push
// onto the ragdoll's torso impulse. The ragdoll pointer at 0xb30 is never set in this port, so
// that whole branch stays a comment (asm 0xE3F8-0xE47C): the ragdoll's own flag byte 0x70 has
// to carry bit 0x40 (asm 0xE408), the other actor has to be a CScriptTrigger (asm 0xE414) whose
// flag byte 0x20 carries bit 0x80 (asm 0xE424) and whose word 0x1a0 carries bit 0x10000 (asm
// 0xE430), and the trigger's float 0x19c has to be above zero (asm 0xE444); the ragdoll's
// 0xb8, 0xbc and 0xc0 then take the trigger's 0x190, 0x194 and 0x198.
void CSpacePirate::Touch(CActor& actor, CStateManager& mgr) {
  TraceCall(*this, "Touch", 0);
  CPatterned::Touch(actor, mgr);
}

// Retail fn_72_18BC (TryToBeCaptured): the pirate switches to the "Captured" state and says so.
// Arms: the body has no branch (asm 0x18BC-0x1918: the state name "Captured", rodata 0xF42, set
// through the state machine's virtual slot 0x20, then `li r3, 1`), so the one forced call of
// 2026-10-21 (gdb: TryToBeCaptured on a live pirate) proves all of it.
// @ fn_72_18BC
bool CSpacePirate::TryToBeCaptured(CStateManager& mgr) {
  // 0x18D8: the state name "Captured" (rodata 0xF42), set through the state machine's virtual
  // slot 0x20 (SetState).
  mStateMachine->SetState(mgr, *this, rstl::string_l("Captured"));
  return true; // 0x1918 li r3, 1
}

// Retail fn_72_D608 is MP1's CPatterned::Random2f, which this tree does not have. Two random
// components in the XY plane - the *first* CRandom16 draw is y and the second is x, both minus
// rodata 0xB44 = 0.5, while z is rodata 0xB9C = 0 (asm 0xD644-0xD688: the first draw lands at
// sp+0xc and the second at sp+0x8, which is the vector base) - then the x term is forced to
// rodata 0xBC8 = 0.001 when |x| is under 0.001 (0xD68C-0xD69C) so the normalize cannot collapse
// the direction, the vector is normalized in place (0xD6A0), and a third draw lerps the length
// minLen + (maxLen - minLen) * rand (0xD6C0 fmadds) which then scales the unit vector
// (0xD6C4-0xD6E8). Retail leaves that scaled length in the return register unused.
CVector3f CSpacePirate::ShuffleRandomVector(CStateManager& mgr, float minLen, float maxLen) {
  const float y = mgr.Random()->Float() - 0.5f;
  float x = mgr.Random()->Float() - 0.5f;
  if (x > -0.001f && x < 0.001f) {
    x = 0.001f;
  }
  CVector3f vec(x, y, 0.f);
  vec.Normalize();
  return (minLen + (maxLen - minLen) * mgr.Random()->Float()) * vec;
}

// fn_72_CE30. Unmarked: ShouldStrafe never enters the branch that calls it in the 208 cycle
// (the 0.66 s shot windows and the lost-LOS branch never open there).
// Retail's step-direction picker for the strafe trigger: a side is dropped when another pirate of
// the same area is that way (dot against the transform's right axis, in the squared-distance frame
// MP1 uses), then the survivors must be on the path find mesh and have a clear dynamic ray. Both
// sides clear is a coin flip on CRandom16 bit 0x4000. Returns pas::EStepDirection.
int CSpacePirate::GetStrafeDir(CStateManager& mgr, float dist) {
  const float distSquared = dist * dist;
  bool canStrafeLeft = true;
  bool canStrafeRight = true;
  CObjectList& list = mgr.ObjectListById(kOL_ListeningAi);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CSpacePirate* other = TCastToConstPtr< CSpacePirate >(list[i]);
    if (other == nullptr || other == this || other->GetCurrentAreaId() != GetCurrentAreaId()) {
      continue;
    }
    const CVector3f delta = other->GetTranslation() - GetTranslation();
    const float magSquared = delta.MagSquared();
    if (magSquared >= distSquared) {
      continue;
    }
    const float rightDot = CVector3f::Dot(delta, GetTransform().GetRight());
    if (rightDot > 0.866f * magSquared || (rightDot > 0.f && magSquared < 3.f)) {
      canStrafeRight = false;
    } else if (rightDot < 0.866f * -magSquared || (rightDot < 0.f && magSquared < 3.f)) {
      canStrafeLeft = false;
    }
  }

  const CVector3f center = GetBoundingBox().GetCenterPoint();
  const CVector3f right = GetTransform().GetRight();
  if (canStrafeRight) {
    canStrafeRight =
        mPathFindSearch.OnPath(center + dist * right) == CPathFindSearch::kR_Success;
  }
  if (canStrafeLeft) {
    canStrafeLeft = mPathFindSearch.OnPath(center - dist * right) == CPathFindSearch::kR_Success;
  }
  if (!canStrafeLeft && !canStrafeRight) {
    TraceCall(*this, "GetStrafeDir", pas::kSD_Invalid);
    return pas::kSD_Invalid;
  }

  // The near list is anchored on the centre when the left side is open and on the left end point
  // otherwise; its length doubles when both sides are still in play (0xB60 = 2).
  const CVector3f startPoint = canStrafeLeft ? center : center - dist * right;
  const float length = canStrafeLeft && canStrafeRight ? 2.f * dist : dist;
  const CMaterialFilter filter = CMaterialFilter::MakeInclude(CMaterialList(kMT_Solid));
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, startPoint, right, length, filter, this);
  if (canStrafeLeft) {
    canStrafeLeft = CGameCollision::RayDynamicLineOfSightTest(mgr, startPoint, right, dist, filter,
                                                             nearList, nullptr);
  }
  if (canStrafeRight) {
    canStrafeRight = CGameCollision::RayDynamicLineOfSightTest(mgr, center, right, dist, filter,
                                                               nearList, nullptr);
  }
  if (canStrafeLeft && canStrafeRight) {
    if ((mgr.Random()->Next() & 0x4000) != 0) {
      canStrafeLeft = false;
    } else {
      canStrafeRight = false;
    }
  }

  int straightDir = pas::kSD_Invalid;
  if (canStrafeLeft) {
    straightDir = pas::kSD_Left;
  } else if (canStrafeRight) {
    straightDir = pas::kSD_Right;
  }
  TraceCall(*this, "GetStrafeDir", straightDir);
  return straightDir;
}

