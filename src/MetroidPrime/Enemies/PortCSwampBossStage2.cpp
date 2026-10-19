// Host body for REL module SwampBossStage2 (module 79): Chykka's adult form (light flyer with
// wing growths, dark flyer with ovipositor). Ported from the retail REL pseudocode; see
// docs/module-analysis/SwampBoss-stage2.md for the function map and retail offsets.

#include "MetroidPrime/Enemies/CSwampBossStage2.hpp"

#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Collision/CJointCollisionDescription.hpp"
#include "MetroidPrime/Enemies/CAiKnockBackMgr.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Factories/CScannableObjectInfo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWater.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CEnergyProjectile.hpp"
#include "MetroidPrime/Weapons/CShockWave.hpp"

#include <float.h>

namespace {
// rodata: collision actors are kept at this HP and damage is read as the shortfall.
const float kCollisionActorHP = 500.f;
// rodata: wing-growth locators, in growth slot order (fn_79_67A4).
const char* const kGrowthLocators[4] = {
    "L_backWing_1",
    "L_frontWing_1",
    "R_backWing_1",
    "R_frontWing_1",
};
// rodata: wing-target effect names, in growth slot order (fn_79_5BC8).
const char* const kGrowthTargetEffects[4] = {
    "WingTarget_LB",
    "WingTarget_LF",
    "WingTarget_RB",
    "WingTarget_RF",
};
// fn_79_6E68: sphere joints, all with mass 1000.
struct SSphereJoint {
  const char* mName;
  float mRadius;
};
const SSphereJoint kSphereJoints[10] = {
    {"L_backWing_lockon_SDK_LCTR", 3.f}, {"L_frontWing_lockon_SDK_LCTR", 3.f},
    {"R_backWing_lockon_SDK_LCTR", 3.f}, {"R_frontWing_lockon_SDK_LCTR", 3.f},
    {"spine_5_scale2", 5.f},             {"L_mouth", 5.f},
    {"spine6_Lockon_LCTR", 3.f},         {"spine_5", 4.f},
    {"spine_3", 3.f},                    {"spine_1", 3.f},
};
const float kJointMass = 1000.f;
// Collision-actor indices into kSphereJoints.
const int kHeadActor = 5;      // L_mouth -> mHeadPos
const int kTailActor = 4;      // spine_5_scale2 -> mTailPos, ovipositor in head mode
const int kSplashActor = 6;    // spine6_Lockon_LCTR -> mSplashPos

float RandomRange(CStateManager& mgr, float lo, float hi) {
  return mgr.Random()->Float() * (hi - lo) + lo;
}

// fn_79_3E50: retail rebuilds the AABB collidable's vtable and copies its material list and box
// into the stack copy the water splash probe tests against; the host equivalent is a copy.
CCollidableAABox CopyCollidableAABox(const CCollidableAABox& source) { return source; }
} // namespace

// fn_79_7404
CSwampBossStage2::CSwampBossStage2(TUniqueId uid, const rstl::string& name,
                                   const CEntityInfo& info, const CTransform4f& xf,
                                   const CModelData& modelData, const CActorParameters& actParms,
                                   const CPatternedInfo& pInfo,
                                   const SLdrSwampBossStage2Data& data)
: CPatterned(static_cast< EPatternedAI >(0x3c), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Flyer, kCT_One, static_cast< EBodyType >(7), actParms)
, mData(data)
, mHeadPos(CVector3f::Zero())
, mTailPos(CVector3f::Zero())
, mSplashPos(CVector3f::Zero())
, mSwoopNode(kInvalidUniqueId)
, mHeadHealth(0.f)
, mOvipositorHealth(0.f)
, mCenter(kInvalidUniqueId)
, mEdgeMarker(kInvalidUniqueId)
, mReelDir(CVector3f::Zero())
, mDashing(false)
, mSwoopDone(false)
, mSwoopDamage(false)
, mSwoopWindow(false)
, mSurfaced(false)
, mGrowthDestroyed(false)
, mGrowthsVulnerable(false)
, mHeadMode(false)
, mCircleDamaged(false)
, mSwoopRepeat(false)
, mFacePlayer(false)
, mRotateSign(false)
, mClampHealth(true)
, mPlayerInWater(false)
, mAttackReady(false)
, mLightFlyer(true)
, mMoveRequested(false)
, mSwoopSide(false)
, mCircleDone(false)
, mSwooping(false)
, mCircleCounting(false)
, mSwoopSelected(false)
, mBigHit(false)
, mReelEffect(false)
, mSuppressWaterSfx(false)
, mBlowPushed(false)
, mBlowing(false)
, mWindPending(false)
, mBubbleCreated(false)
, mBubbleTelegraphed(false)
, mAdditiveAnim(-1)
, mAdditiveWeight(0.f)
, mAdditiveRate(0.f)
, mAdditiveJitterX(0.f)
, mAdditiveJitterY(0.f)
, mChainDepth(0)
, mGrowthVulnerableTime(0.f)
, mPhase(0)
, mAttackIndex(0)
, mAttackType(8)
, mAttackDelay(0.f)
, mLightChain(kInvalidUniqueId)
, mSpitProjectile(data.spitProjectile, LdrToDamageInfo(data.spitDamage))
, mSpitNode(kInvalidUniqueId)
, mBarrageCount(1)
, mNodeSpacing(20.f)
, mLastHeadSplash(CVector3f::Zero())
, mLastTailSplash(CVector3f::Zero())
, mWaterZ(0.f)
, mWater(kInvalidUniqueId)
, mSwoopDamageInfo(LdrToDamageInfo(data.swoopDamage))
, mSwoopTimer(0.f)
, mLevelTarget(CVector3f::Zero())
, mCollisionActorHP(kCollisionActorHP)
, mEdgeDistance(50.f)
, mFlyersLeft(0)
, mHeadBlend(0.f)
, mGrowthDeadTime(0.f)
, mSwoopCount(0)
, mCircleDamage(0.f)
, mGrappleTime(0.f)
, mDashTimer(0.f)
, mInterceptTime(0.2f)
, mDarkChain(kInvalidUniqueId)
, mSwoopTarget(CVector3f::Zero())
, mCircleCount(0)
, mMoveDir(xf.GetColumn(kDY))
, mMoveTarget(CVector3f::Zero())
, mMoveSpeed(0.f)
, mFlashTime(0.f)
, mLightVulnerability(pInfo.GetDamageVulnerability())
, mDarkVulnerability(pInfo.GetDamageVulnerability())
, mRepeller(data.unknown_0x9347820e.range, data.unknown_0x9347820e.turnRate,
            static_cast< ushort >(data.unknown_0x9347820e.soundEffect),
            data.unknown_0x9347820e.warpScale, data.unknown_0x9347820e.repelOffset)
, mBlowDamageInfo(LdrToDamageInfo(data.blowDamage))
, mWindNode(kInvalidUniqueId)
, mDropFlyerCooldown(0.f)
, mPendingFlinch(0.f)
, mLastTouched(kInvalidUniqueId) {
  // TODO(uncertain): the retail ctor fills mLightVulnerability/mDarkVulnerability from rodata
  // vulnerability tables; both start as the patterned vulnerability here.
  if (data.scanInfoLight != kInvalidAssetId) {
    mScanLight = TLockedToken< CScannableObjectInfo >(
        gpSimplePool->GetObj(SObjectTag('SCAN', data.scanInfoLight)));
  }
  if (data.scanInfoDark != kInvalidAssetId) {
    mScanDark = TLockedToken< CScannableObjectInfo >(
        gpSimplePool->GetObj(SObjectTag('SCAN', data.scanInfoDark)));
  }
  if (data.splash != kInvalidAssetId) {
    mSplashGen = gpSimplePool->GetObj(SObjectTag('PART', data.splash));
  }
  if (data.wingDamageEffect != kInvalidAssetId) {
    mWingDamageGen = gpSimplePool->GetObj(SObjectTag('PART', data.wingDamageEffect));
  }
  if (data.blowEffect != kInvalidAssetId) {
    mBlowGen = gpSimplePool->GetObj(SObjectTag('PART', data.blowEffect));
  }
  if (data.spitVisorEffect != kInvalidAssetId) {
    mSpitVisorGen = TLockedToken< CGenDescription >(
        gpSimplePool->GetObj(SObjectTag('PART', data.spitVisorEffect)));
  }
  // TODO(uncertain): retail also disables part of the knockback controller here.
  InitGrowths();
  for (int i = 0; i < 4; ++i) {
    mGrowthBroken.push_back(false);
  }
}

// fn_79_7010
CSwampBossStage2::~CSwampBossStage2() {}

// --------------------------------------------------------------------------------------------
// Helpers

// fn_79_3524
const SLdrSwampBossStage2Phase& CSwampBossStage2::CurrentPhase() const {
  if (mPhase == 2) {
    return mData.lightFlyer2;
  }
  if (mPhase < 2) {
    return mPhase == 0 ? mData.lightFlyer1 : mData.darkFlyer1;
  }
  if (mPhase < 4) {
    return mData.darkFlyer2;
  }
  // Retail falls back to the first light phase for any phase past the fourth.
  return mData.lightFlyer1;
}

// fn_79_34CC: the attack slot for an index; 8 means "no attack".
int CSwampBossStage2::AttackForIndex(int idx) const {
  const SLdrSwampBossStage2Phase& phase = CurrentPhase();
  if (idx == 2) {
    return phase.thirdAttack;
  }
  if (idx > 1) {
    if (idx == 4) {
      return phase.fifthAttack;
    }
    if (idx > 3) {
      return 8;
    }
    return phase.fourthAttack;
  }
  if (idx == 0) {
    return phase.firstAttack;
  }
  if (idx < 0) {
    return 8;
  }
  return phase.secondAttack;
}

// fn_79_3458: take the current slot's attack, wrap to the first slot on the "no attack" sentinel,
// then advance the index. Lurk raises mAttackReady itself once the attack delay runs out.
void CSwampBossStage2::NextAttack() {
  mAttackType = AttackForIndex(mAttackIndex);
  if (mAttackType == 8) {
    mAttackIndex = 0;
    mAttackType = AttackForIndex(mAttackIndex);
  }
  ++mAttackIndex;
}

// fn_79_6754: retail resolves the actor through
// CCollisionActorManager::GetCollisionDescFromIndex(idx) then ObjectById/TCastToPtr.
CCollisionActor* CSwampBossStage2::CollisionActor(CStateManager& mgr, int idx) const {
  if (mCollisionMgr.null() || idx >= mCollisionMgr->GetNumCollisionActors()) {
    return nullptr;
  }
  return TCastToPtr< CCollisionActor >(
      mgr.ObjectById(mCollisionMgr->GetCollisionDescFromIndex(idx).GetCollisionActorId()));
}

CVector3f CSwampBossStage2::CollisionActorPosition(const CStateManager& mgr, int idx) const {
  if (mCollisionMgr.null() || idx >= mCollisionMgr->GetNumCollisionActors()) {
    return GetTranslation();
  }
  const CActor* act = TCastToConstPtr< CActor >(
      mgr.GetObjectById(mCollisionMgr->GetCollisionDescFromIndex(idx).GetCollisionActorId()));
  return act ? act->GetTranslation() : GetTranslation();
}

// fn_79_6054: the centre actor's position, or the origin when the chain has none.
CVector3f CSwampBossStage2::CenterPosition(const CStateManager& mgr) const {
  const CActor* center = TCastToConstPtr< CActor >(mgr.GetObjectById(mCenter));
  return center ? center->GetTranslation() : CVector3f::Zero();
}

// fn_79_60A0: the point the states steer at. fn_79_2E7C is its mSwoopTarget branch.
CVector3f CSwampBossStage2::TargetPoint(const CStateManager& mgr) const {
  if (mDashing) {
    return mSwoopTarget;
  }
  const CActor* node = TCastToConstPtr< CActor >(mgr.GetObjectById(mSwoopNode));
  return node ? node->GetTranslation() : GetTranslation();
}

// fn_79_6108: remember a move target and raise the move request.
void CSwampBossStage2::RequestMove(const CVector3f& point) {
  mMoveTarget = point;
  mMoveRequested = true;
}

// fn_79_238C: the first chain node on the far side of the centre actor from the current target;
// the last node is taken unconditionally. Returns null for an empty or unresolved chain.
CActor* CSwampBossStage2::NextChainNode(CStateManager& mgr) const {
  const int count = static_cast< int >(mPathNodes.size());
  for (int i = 0; i < count; ++i) {
    CActor* node = TCastToPtr< CActor >(mgr.ObjectById(mPathNodes[i]));
    if (!node) {
      continue;
    }
    if (i == count - 1) {
      return node;
    }
    const CVector3f center = CenterPosition(mgr);
    if (CVector3f::Dot(node->GetTranslation() - center, TargetPoint(mgr) - center) < 0.f) {
      return node;
    }
  }
  return nullptr;
}

// fn_79_20D8: joints 4..9 follow the phase's vulnerability table while the body can be hurt, and
// reflect damage otherwise.
void CSwampBossStage2::SetBodyVulnerable(CStateManager& mgr, bool vulnerable) {
  const CDamageVulnerability& vuln =
      !vulnerable ? CDamageVulnerability::ReflectVulnerabilty()
                  : ((mPhase & 1) != 0 ? mDarkVulnerability : mLightVulnerability);
  for (int i = kTailActor; i < ARRAY_SIZE(kSphereJoints); ++i) {
    CCollisionActor* act = CollisionActor(mgr, i);
    if (act) {
      act->SetDamageVulnerability(vuln);
    }
  }
}

CVector3f CSwampBossStage2::LevelTarget(const CStateManager& mgr) const {
  return mLevelTarget;
}

CVector3f CSwampBossStage2::PlayerAimPoint(const CStateManager& mgr) const {
  return mgr.GetPlayer(0)->GetAimPosition(mgr, 0.f);
}

// fn_79_4220: the chain node nearest the player (retail returns the actor, used as its uid).
TUniqueId CSwampBossStage2::NearestNodeToPlayer(CStateManager& mgr) const {
  const CVector3f playerPos = mgr.GetPlayer(0)->GetTranslation();
  TUniqueId best = kInvalidUniqueId;
  float bestDist = FLT_MAX;
  for (int i = 0; i < mPathNodes.size(); ++i) {
    const CActor* node = TCastToConstPtr< CActor >(mgr.GetObjectById(mPathNodes[i]));
    if (!node) {
      continue;
    }
    const float dist = (node->GetTranslation() - playerPos).MagSquared();
    if (dist < bestDist) {
      bestDist = dist;
      best = mPathNodes[i];
    }
  }
  return best;
}

// fn_79_67A4
void CSwampBossStage2::InitGrowths() {
  // Retail model slots: 0 = LB, 1 = LF, 2 = RB, 3 = RF.
  const CAssetId models[4] = {mData.wingGrowthLB, mData.wingGrowthLF, mData.wingGrowthRB,
                              mData.wingGrowthRF};
  const CVector3f scale = GetModelData()->GetScale();
  for (int i = 0; i < 4; ++i) {
    mGrowthLocators.push_back(GetAnimationData()->GetLocatorSegId(rstl::string_l(kGrowthLocators[i])));
    if (models[i] != kInvalidAssetId) {
      mGrowthModels.push_back(rstl::optional_object< CModelData >(
          CModelData(CStaticRes(models[i], scale))));
    } else {
      mGrowthModels.push_back(rstl::optional_object< CModelData >());
    }
    mGrowthHealth.push_back(mData.wingGrowthHealth);
    mGrowthFlash.push_back(0.f);
  }
}

// fn_79_6E68
void CSwampBossStage2::SetupCollisionManager(CStateManager& mgr) {
  rstl::vector< CJointCollisionDescription > joints;
  joints.reserve(ARRAY_SIZE(kSphereJoints));
  for (int i = 0; i < ARRAY_SIZE(kSphereJoints); ++i) {
    const CSegId seg = GetAnimationData()->GetLocatorSegId(rstl::string_l(kSphereJoints[i].mName));
    joints.push_back(CJointCollisionDescription::SphereCollision(
        seg, CVector3f::Zero(), kSphereJoints[i].mRadius, rstl::string_l(kSphereJoints[i].mName), kJointMass));
  }
  mCollisionMgr = rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), joints, true);
  for (int i = 0; i < mCollisionMgr->GetNumCollisionActors(); ++i) {
    CCollisionActor* act = CollisionActor(mgr, i);
    if (!act) {
      continue;
    }
    act->HealthInfo()->SetHP(kCollisionActorHP);
    // fn_79_6B78: retail data_418 excludes material bit 0x31 from every joint's filter.
    act->SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
        CMaterialList(kMT_Solid), CMaterialList(static_cast< EMaterialTypes >(0x31))));
  }
  SetGrowthsActive(mgr, false);
}

// fn_79_25C0
void CSwampBossStage2::Activate(CStateManager& mgr) {
  if (mCollisionMgr.null()) {
    SetupCollisionManager(mgr);
  }
  mCollisionMgr->SetActive(mgr, true);
  BodyController()->Activate(mgr, pas::kAS_Invalid);
  // TODO(uncertain): retail SetBossParams passes a string; the host takes a string index.
  mgr.SetBossParams(GetUniqueId(), HealthInfo()->GetHP(), 0);
}

// fn_79_5D58
void CSwampBossStage2::SetHeadVulnerable(CStateManager& mgr, bool vulnerable) {
  // TODO(uncertain): retail swaps the head collision actor's vulnerability between the light and
  // dark tables; reconstructed from the material/vulnerability writes only.
  CCollisionActor* head = CollisionActor(mgr, kHeadActor);
  if (head) {
    head->SetDamageVulnerability(vulnerable ? mLightVulnerability
                                            : CDamageVulnerability::ImmuneVulnerabilty());
  }
}

// fn_79_5BC8
void CSwampBossStage2::SetGrowthTargetEffects(CStateManager& mgr, bool on) {
  for (int i = 0; i < 4; ++i) {
    AnimationData()->SetEffectState(rstl::string_l(kGrowthTargetEffects[i]),
                                    on && mGrowthHealth[i] > 0.f, mgr);
  }
}

// fn_79_5C74
void CSwampBossStage2::SetGrowthsActive(CStateManager& mgr, bool active) {
  for (int i = 0; i < 4; ++i) {
    SetGrowthActive(mgr, i, active && mGrowthHealth[i] > 0.f);
  }
}

// fn_79_5D00: retail reaches the growth actor's vfunc at vtable+0x1c with the flag.
// TODO(uncertain): the virtual slot is not identified; the port switches the growth's
// Orbit/Target materials instead, which is what the setup (fn_79_6B78) and ProcessDamage imply.
void CSwampBossStage2::SetGrowthActive(CStateManager& mgr, int idx, bool active) {
  CCollisionActor* act = CollisionActor(mgr, idx);
  if (!act) {
    return;
  }
  if (active) {
    act->AddMaterial(kMT_Orbit, kMT_Target, mgr);
  } else {
    act->RemoveMaterial(kMT_Orbit, kMT_Target, mgr);
  }
}

// fn_79_24E0: growths are only targetable while the player is in front of the boss, and the boss
// itself gains or drops its orbit/target/seeker materials with that side. Retail calls this from
// Reel (fn_79_7870) only.
void CSwampBossStage2::UpdatePlayerSideMaterials(CStateManager& mgr) {
  const CPlayer* player = mgr.GetPlayer(0);
  if (!player) {
    return;
  }
  const bool playerBehind = CVector3f::Dot(player->GetTranslation() - GetTranslation(),
                                           GetTransform().GetForward()) < 0.f;
  SetGrowthsActive(mgr, playerBehind);
  if (playerBehind) {
    RemoveMaterial(kMT_Orbit, kMT_Target, kMT_SeekerTarget, mgr);
  } else {
    AddMaterial(kMT_Orbit, kMT_Target, kMT_SeekerTarget, mgr);
  }
}

// fn_79_5EC4: direction from a random chain node of the pair the boss picks between back towards
// the centre actor. The node farthest along the boss's forward axis is never the one picked.
CVector3f CSwampBossStage2::DirectionFromRandomNode(CStateManager& mgr) const {
  const int count = static_cast< int >(mPathNodes.size());
  if (count == 0) {
    return CVector3f::Zero();
  }
  const CVector3f center = CenterPosition(mgr);
  const CVector3f forward = GetTransform().GetForward();
  int farthest = 0;
  float farthestDot = FLT_MAX;
  for (int i = 0; i < count; ++i) {
    const CActor* node = TCastToConstPtr< CActor >(mgr.GetObjectById(mPathNodes[i]));
    if (!node) {
      continue;
    }
    const float dot = CVector3f::Dot(center - node->GetTranslation(), forward);
    if (dot < farthestDot) {
      farthestDot = dot;
      farthest = i;
    }
  }
  int pick = mgr.Random()->Next() & 1;
  if (pick == farthest) {
    ++pick;
  }
  if (pick >= count) {
    pick = 0;
  }
  const CActor* node = TCastToConstPtr< CActor >(mgr.GetObjectById(mPathNodes[pick]));
  return node ? (center - node->GetTranslation()).AsNormalized() : CVector3f::Zero();
}

// fn_79_6134: normalised direction from the body towards the player's aim point.
CVector3f CSwampBossStage2::DirectionToPlayerAim(const CStateManager& mgr) const {
  if (mgr.GetPlayer(0) == nullptr) {
    return GetTransform().GetForward();
  }
  return (PlayerAimPoint(mgr) - GetTranslation()).AsNormalized();
}

void CSwampBossStage2::UpdateFacingMaterial(CStateManager& mgr) {
  // TODO(uncertain): not decoded. Retail's Lurk (fn_79_9A3C) turns the body towards the player
  // here through fn_79_6134, which is not ported yet.
}

void CSwampBossStage2::UpdateHover(CStateManager& mgr, float dt) {
  // TODO(uncertain): not decoded.
}

void CSwampBossStage2::UpdateAdditive(CStateManager& mgr, float dt) {
  // TODO(uncertain): not decoded.
}

// fn_79_3900: head/tail water splash. `pos` is the splash point and `lastSplash` the last one
// emitted, kept just above the water surface.
void CSwampBossStage2::UpdateWater(CStateManager& mgr, const CVector3f& pos,
                                   CVector3f& lastSplash) {
  const float heightAboveWater = mWaterZ - pos.GetZ();
  const bool hasEmitter = static_cast< bool >(mWaterSfx);
  bool aboveSurface = false;
  if (heightAboveWater > 0.f) {
    if (heightAboveWater < 4.f) {
      // Hovering just over the water: start the splash sounds (the second one loops and is kept).
      if (!hasEmitter && !mBlowPushed) {
        const CVector3f forward = GetTransform().GetForward();
        PlayCustomSound(pos, forward, mData.audioPlaybackParms, false);
        mWaterSfx = PlayCustomSound(pos, forward, mData.audioPlaybackParms_0x2b3c923a, true);
      }
      aboveSurface = true;
    }
    // rodata: splash plane is a centimetre above the surface.
    const float splashZ = mWaterZ + 0.01f;
    const CVector3f delta(pos.GetX() - lastSplash.GetX(), pos.GetY() - lastSplash.GetY(),
                          splashZ - lastSplash.GetZ());
    // rodata: only splash again once the point has moved sqrt(8) from the last one.
    if (8.f < delta.MagSquared()) {
      CTransform4f xf(GetTransform());
      xf.SetTranslation(CVector3f(pos.GetX(), pos.GetY(), splashZ));
      const CAABox bounds(CVector3f(pos.GetX() - 1.f, pos.GetY() - 1.f, splashZ - 1.f),
                          CVector3f(pos.GetX() + 1.f, pos.GetY() + 1.f, splashZ + 1.f));
      rstl::reserved_vector< TUniqueId, 1024 > nearList;
      // rodata data_41C: the include filter the probe gathers nearby objects with.
      const CMaterialFilter nearFilter(CMaterialList((u64(1) << 32) | 0x31u), CMaterialList(),
                                       CMaterialFilter::kFT_Include);
      mgr.BuildNearList(nearList, bounds, nearFilter, this);
      bool blocked = false;
      if (nearList.size() != 0) {
        // rodata data_420: the probe's own material list.
        const CCollidableAABox probe(bounds, CMaterialList(0x3bu));
        blocked = CGameCollision::DetectDynamicCollisionBoolean(CopyCollidableAABox(probe),
                                                               CTransform4f::Identity(), nearList,
                                                               mgr);
      }
      if (!blocked) {
        // rodata: the splash grows with the drop height, up to 1.6.
        float scale = heightAboveWater * 0.2f + 0.8f;
        if (scale > 1.6f) {
          scale = 1.6f;
        }
        CExplosion* splash = rs_new CExplosion(
            mSplashGen, mgr.AllocateUniqueId(),
            CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true),
            rstl::string_l("Chykka head/tail splash"), xf, 0,
            CVector3f(scale, scale, scale), CColor::White(), -1);
        // TODO(uncertain): retail links the splash to the water actor (CExplosion +0x12a =
        // mWater); the host CExplosion exposes no such field.
        mgr.AddObject(splash);
      }
      lastSplash = CVector3f(pos.GetX(), pos.GetY(), splashZ);
    }
  }
  if (hasEmitter && !aboveSurface) {
    // Left the surface: stop the loop and splash out.
    CSfxManager::RemoveEmitter(mWaterSfx);
    mWaterSfx = CSfxHandle::NullHandle();
    if (!mBlowPushed) {
      PlayCustomSound(pos, GetTransform().GetForward(), mData.audioPlaybackParms_0xc05d5c7a,
                      false);
    }
  }
  if (mBlowPushed && mWaterSfx) {
    CSfxManager::RemoveEmitter(mWaterSfx);
    mWaterSfx = CSfxHandle::NullHandle();
  }
}

void CSwampBossStage2::UpdateHealthFraction(CStateManager& mgr) {
  // TODO(uncertain): not decoded.
}

void CSwampBossStage2::AddAdditiveFlinch(float amount) {
  mPendingFlinch = rstl::max_val(mPendingFlinch, rstl::min_val(amount / 20.f, 1.f));
}

void CSwampBossStage2::StartBubbleTelegraph(CStateManager& mgr, const CVector3f& pos) {
  // TODO(uncertain): not decoded.
}

void CSwampBossStage2::SpawnShockWave(CStateManager& mgr, const CVector3f& pos) {
  // TODO(uncertain): not decoded.
}

// fn_79_3578
void CSwampBossStage2::SpawnWindEffect(CStateManager& mgr) {
  const CActor* node = TCastToConstPtr< CActor >(mgr.GetObjectById(mWindNode));
  if (!node) {
    return;
  }
  CTransform4f xf(GetTransform());
  const CVector3f nodePos = node->GetTranslation();
  const CVector3f pos = GetTranslation();
  // Flat direction to the wind node; the effect sits 50 units short of it.
  const CVector3f dir =
      CVector3f(nodePos.GetX() - pos.GetX(), nodePos.GetY() - pos.GetY(), 0.f).AsNormalized();
  xf.SetTranslation(CVector3f(nodePos.GetX() - dir.GetX() * 50.f,
                              nodePos.GetY() - dir.GetY() * 50.f,
                              nodePos.GetZ() - dir.GetZ() * 50.f));
  // Scale 3, white, and -1 for "no player owner".
  CExplosion* explosion = rs_new CExplosion(
      mBlowGen, mgr.AllocateUniqueId(),
      CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true),
      rstl::string_l("Chykka wind effect"), xf, 0, CVector3f(3.f, 3.f, 3.f), CColor::White(), -1);
  mgr.AddObject(explosion);
}

void CSwampBossStage2::SpawnWingDamageEffect(CStateManager& mgr, int idx) {
  // TODO(uncertain): not decoded.
}

void CSwampBossStage2::ReadWaterAndNodes(CStateManager& mgr) {
  // TODO(uncertain): not decoded.
}

// fn_79_2E84: pick the chain node to swoop at. Retail crosses the player-to-centre line with up
// (fn_79_3314, the host's CVector3f::Cross) to walk the chain round to the far side, then keeps the
// connected node that is farthest from the player with a random spread.
void CSwampBossStage2::PickSwoopTarget(CStateManager& mgr) {
  const CPlayer* player = mgr.GetPlayer(0);
  if (!player) {
    return;
  }
  CActor* origin = nullptr;
  if (mAttackType == 5) {
    const CActor* nearest = TCastToConstPtr< CActor >(mgr.GetObjectById(NearestNodeToPlayer(mgr)));
    const CVector3f centre = CenterPosition(mgr);
    const CVector3f across =
        CVector3f::Cross(player->GetTranslation() - centre, CVector3f::Up());
    const float bossSide = CVector3f::Dot(across, GetTranslation() - centre);
    for (int i = 0; nearest && i < mPathNodes.size(); ++i) {
      // Skip the node the player is nearest to; take the first node past the halfway line.
      if (mPathNodes[i] == nearest->GetUniqueId()) {
        continue;
      }
      CActor* node = TCastToPtr< CActor >(mgr.ObjectById(mPathNodes[i]));
      if (!node) {
        continue;
      }
      origin = node;
      if (bossSide * CVector3f::Dot(across, node->GetTranslation() - centre) < 0.f) {
        break;
      }
    }
  } else {
    origin = TCastToPtr< CActor >(mgr.ObjectById(NearestNodeToPlayer(mgr)));
  }
  if (!origin) {
    return;
  }
  // The chain continues along the chosen node's ARRV/NEXT connections.
  const rstl::vector< TUniqueId > candidates =
      origin->FindConnectedObjects(mgr, kSS_Arrived, kSM_Next);
  float bestScore = 0.f;
  for (int i = 0; i < candidates.size(); ++i) {
    const CActor* node = TCastToConstPtr< CActor >(mgr.GetObjectById(candidates[i]));
    if (!node) {
      continue;
    }
    // rodata: while diving, nodes too close to the current one are not worth swooping at.
    if (mAttackType == 5 &&
        (node->GetTranslation() - origin->GetTranslation()).Magnitude() <
            mData.unknown_0x9e116385) {
      continue;
    }
    const float score = (player->GetTranslation() - node->GetTranslation()).Magnitude() +
                        mNodeSpacing * 0.5f * mgr.Random()->Float();
    if (score > bestScore) {
      mSwoopTarget = node->GetTranslation();
      bestScore = score;
    }
  }
}

void CSwampBossStage2::PickSpitNode(CStateManager& mgr) {
  // TODO(uncertain): not decoded.
}

void CSwampBossStage2::FireSpit(CStateManager& mgr, const rstl::string& locator) {
  // TODO(uncertain): not decoded.
}

void CSwampBossStage2::LaunchPlatformChain(CStateManager& mgr,
                                           const rstl::vector< TUniqueId >& starts,
                                           const CVector3f& from) {
  // TODO(uncertain): not decoded.
}

void CSwampBossStage2::SendDecrement(CStateManager& mgr) {
  // TODO(uncertain): not decoded.
}

// fn_79_3390: total damage still inside the memory window.
float CSwampBossStage2::RecentDamage() const {
  float sum = 0.f;
  for (rstl::list< SDamageMemory >::const_iterator it = mDamageMemory.begin();
       it != mDamageMemory.end(); ++it) {
    sum += it->mDamage;
  }
  return sum;
}

void CSwampBossStage2::DecayDamageMemory(float dt) {
  rstl::list< SDamageMemory >::iterator it = mDamageMemory.begin();
  while (it != mDamageMemory.end()) {
    it->mTime -= dt;
    if (it->mTime <= 0.f) {
      it = mDamageMemory.erase(it);
    } else {
      ++it;
    }
  }
}

bool CSwampBossStage2::PlayerOnGrapplePoint(const CStateManager& mgr) const {
  // TODO(uncertain): not decoded.
  return false;
}

// fn_79_61C4
void CSwampBossStage2::ProcessDamage(CStateManager& mgr, TUniqueId colActorId) {
  // TODO(uncertain): body not yet ported.
}

// --------------------------------------------------------------------------------------------
// Triggers

// fn_79_6250
bool CSwampBossStage2::StateOver(CStateManager& mgr, const CTriggerData&) const {
  return mAnimationState.IsOver();
}

bool CSwampBossStage2::AllGrowthsDead(CStateManager& mgr, const CTriggerData&) const {
  for (int i = 0; i < mGrowthHealth.size(); ++i) {
    if (mGrowthHealth[i] > 0.f) {
      return false;
    }
  }
  return true;
}

bool CSwampBossStage2::OvipositorDead(CStateManager& mgr, const CTriggerData&) const {
  return mOvipositorHealth <= 0.f;
}

bool CSwampBossStage2::ZeroHealth(CStateManager& mgr, const CTriggerData&) const {
  // TODO(uncertain): retail compares the phase index against unknown_0x96ce7897.
  return mPhase >= mData.unknown_0x96ce7897;
}

bool CSwampBossStage2::PlayerInWater(CStateManager& mgr, const CTriggerData&) const {
  return mAttackReady && mPlayerInWater;
}

bool CSwampBossStage2::ShouldSpit(CStateManager& mgr, const CTriggerData&) const {
  return mAttackReady && mAttackType >= 0 && mAttackType <= 2;
}

bool CSwampBossStage2::ShouldBarrage(CStateManager& mgr, const CTriggerData&) const {
  return mAttackReady && mAttackType == 3;
}

bool CSwampBossStage2::ShouldTaunt(CStateManager& mgr, const CTriggerData&) const {
  return mAttackReady && mAttackType == 7;
}

bool CSwampBossStage2::ShouldSwoop(CStateManager& mgr, const CTriggerData&) const {
  return mAttackReady && mAttackType == 4;
}

bool CSwampBossStage2::SwoopOver(CStateManager& mgr, const CTriggerData&) const {
  // TODO(uncertain): retail reads bit 5 of 0xc76; mapped to mCircleCounting.
  return mCircleCounting;
}

bool CSwampBossStage2::CircleDamage(CStateManager& mgr, const CTriggerData&) const {
  return mCircleDamaged;
}

bool CSwampBossStage2::CircleOver(CStateManager& mgr, const CTriggerData&) const {
  return GetBodyController()->GetCurrentStateId() == pas::kAS_Locomotion || mCircleDone;
}

bool CSwampBossStage2::ShouldBlow(CStateManager& mgr, const CTriggerData&) const {
  return mAttackReady && mAttackType == 6;
}

bool CSwampBossStage2::IsLevel(CStateManager& mgr, const CTriggerData&) const {
  if (GetBodyController()->GetLocomotionType() != 5 || mMoveSpeed >= 0.2f) {
    return false;
  }
  const CVector3f pos = GetTranslation();
  const CVector3f target = LevelTarget(mgr);
  if (CMath::AbsF(target.GetZ() - pos.GetZ()) >= 1.f || (target - pos).Magnitude() >= 2.f) {
    return false;
  }
  CVector2f toLevel(mLevelTarget.GetX() - pos.GetX(), mLevelTarget.GetY() - pos.GetY());
  toLevel.Normalize();
  const CVector3f fwd = GetTransform().GetForward();
  // rodata: 0x3fa1df46a0000000 (~2 degrees).
  return toLevel.GetX() * fwd.GetX() + toLevel.GetY() * fwd.GetY() >
         CMath::FastCosR(0.034906585f);
}

bool CSwampBossStage2::SwitchedPlatform(CStateManager& mgr, const CTriggerData&) const {
  return mWindNode != NearestNodeToPlayer(mgr);
}

bool CSwampBossStage2::Stunned(CStateManager& mgr, const CTriggerData&) const {
  return mHeadHealth <= 0.f;
}

bool CSwampBossStage2::StunTimeOut(CStateManager& mgr, const CTriggerData&) const {
  return const_cast< CSwampBossStage2* >(this)->StateMachineState().GetTime() > mData.stunTime && mGrowthDeadTime == 0.f;
}

bool CSwampBossStage2::OneGrowthDead(CStateManager& mgr, const CTriggerData&) const {
  return mGrowthDestroyed && mGrowthDeadTime > 0.3f;
}

bool CSwampBossStage2::ShouldDropFlyer(CStateManager& mgr, const CTriggerData&) const {
  return mAttackReady && mAttackType == 5;
}

// --------------------------------------------------------------------------------------------
// States

// fn_79_9F4C
void CSwampBossStage2::Start(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {
    return;
  }
  // The radar radius override lasts the whole fight; DeathSequence (fn_79_8FAC) clears it.
  mgr.GetPlayer(0)->SetRadarRadiusOverride(mData.radarRange, mData.unknown_0xfe97e835);
}

// fn_79_9534
void CSwampBossStage2::BecomeLightFlyer(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_KnockBack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCKnockBackCmd(CVector3f::Forward(), pas::kS_Two, -1));
    }
  } else if (msg == kStateMsg_Activate) {
    // A new light flyer: wings back at full health, next phase in, at rest.
    for (int i = 0; i < mGrowthHealth.size(); ++i) {
      mGrowthHealth[i] = mData.wingGrowthHealth;
    }
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    ++mPhase;
    mAttackIndex = 0;
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mFlyersLeft = 0x14;
    mLightFlyer = true;
    if (mPhase == 4) {
      SendScriptMsgs(kSS_Dead, mgr, kInvalidUniqueId, kSM_None);
    } else {
      SendScriptMsgs(kSS_InternalState13, mgr, kInvalidUniqueId, kSM_None);
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mHeadHealth = CurrentPhase().health;
    mLightFlyer = false;
  }
}

// fn_79_91A4
void CSwampBossStage2::BecomeDarkFlyer(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_KnockBack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCKnockBackCmd(CVector3f::Forward(), pas::kS_One, -1));
    }
    if (mSurfaced) {
      if (!mFacePlayer) {
        // rodata: face the player plus or minus 70 degrees, by mRotateSign.
        BodyController()->FaceDirection(
            CTransform4f::RotateZ(CRelAngle::FromDegrees(mRotateSign ? -70.f : 70.f)) *
                DirectionToPlayerAim(mgr),
            dt);
      } else {
        BodyController()->FaceDirection(DirectionToPlayerAim(mgr), dt);
      }
    }
    // rodata: per-frame additive jitter of 0.2 either side of zero.
    mAdditiveJitterX = (mgr.Random()->Float() - 0.5f) * 0.2f;
    mAdditiveJitterY = (mgr.Random()->Float() - 0.5f) * 0.2f;
    UpdateWater(mgr, mHeadPos, mLastTailSplash);
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mSurfaced = false;
    mRotateSign = mgr.Random()->Float() > 0.5f;
    mFacePlayer = false;
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    ++mPhase;
    mAttackIndex = 0;
    mBubbleTelegraphed = false;
    mOvipositorHealth = CurrentPhase().health;
    mPendingFlinch = 0.f;
    mBlowPushed = true;
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mBlowPushed = false;
  }
}

// fn_79_8FAC
void CSwampBossStage2::DeathSequence(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Fall)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCKnockDownCmd(CVector3f::Forward(), pas::kS_One));
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    if (CPlayer* player = mgr.GetPlayer(0)) {
      player->ClearRadarRadiusOverride();
    }
    // Clears the boss bar (invalid id, no energy) and tells the script the boss is dead.
    mgr.SetBossParams(kInvalidUniqueId, 0.f, 0);
    SendScriptMsgs(kSS_Dead, mgr, kInvalidUniqueId, kSM_None);
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// fn_79_9A3C
void CSwampBossStage2::Lurk(CStateManager& mgr, EStateMsg msg, float dt) {
  const SLdrSwampBossStage2Phase& phase = CurrentPhase();
  if (msg == kStateMsg_Activate) {
    mAttackDelay = RandomRange(mgr, phase.minTimeBetweenAttacks, phase.maxTimeBetweenAttacks);
    mAttackReady = false;
    if (mCircleDamaged) {
      if (mPhase == 0 || mSwoopSelected) {
        mAttackType = 3;
        mAttackReady = true;
      } else {
        mAttackType = 4;
        mSwoopSelected = true;
      }
      mCircleDamaged = false;
    } else if (mPhase >= 1 && mSwoopCount <= 2 && !mHeadMode && mAttackType == 4 &&
               !mSwoopDone && !mSwoopRepeat) {
      mAttackReady = true;
      mSwoopRepeat = true;
    } else if (mgr.Random()->Float() >= phase.tauntChance) {
      NextAttack();
      mSwoopRepeat = false;
      mSwoopSelected = false;
    } else {
      mAttackType = 7;
    }
    mSwoopCount = mAttackType == 4 ? mSwoopCount + 1 : 0;
    mDashing = mgr.Random()->Float() < phase.dashChance;
    mDashTimer = RandomRange(mgr, phase.unknown_0x29e6ead6, phase.unknown_0x1753225e);
    SetHeadVulnerable(mgr, true);
    PickSpitNode(mgr);
  } else if (msg == kStateMsg_Update) {
    // fn_79_6134: face the player.
    UpdateFacingMaterial(mgr);
    if (mDashing) {
      mDashing = false;
      BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(pas::kSD_Up, pas::kStep_Dodge));
    }
    if (GetBodyController()->GetCurrentStateId() == pas::kAS_Locomotion) {
      // TODO(uncertain): fn_79_6108(LevelTarget) is a move request toward the level target.
      mDashTimer -= dt;
      if (mDashTimer <= 0.f) {
        BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(pas::kSD_Up, pas::kStep_Dodge));
        mDashTimer = RandomRange(mgr, phase.unknown_0x29e6ead6, phase.unknown_0x1753225e);
      }
      if (const_cast< CSwampBossStage2* >(this)->StateMachineState().GetTime() > mAttackDelay) {
        mAttackReady = true;
      }
    }
    if (!mHeadMode && mAttackType != 4 && RecentDamage() > mData.unknown_0x7fc50ac2) {
      mAttackType = 4;
    }
  }
}

// fn_79_9754
void CSwampBossStage2::DarkLurk(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAttackType == 5) {
      BodyController()->FaceDirection(CenterPosition(mgr) - GetTranslation(), dt);
    } else {
      BodyController()->FaceDirection(DirectionToPlayerAim(mgr), dt);
    }
    RequestMove(TargetPoint(mgr));
    if (GetBodyController()->GetCurrentStateId() == pas::kAS_Locomotion) {
      mDashTimer -= dt;
      if (mDashTimer <= 0.f) {
        PickSwoopTarget(mgr);
        const SLdrSwampBossStage2Phase& phase = CurrentPhase();
        mDashTimer = RandomRange(mgr, phase.unknown_0x29e6ead6, phase.unknown_0x1753225e);
      }
    }
    if (StateMachineState().GetTime() > mAttackDelay &&
        GetBodyController()->GetCurrentStateId() == pas::kAS_Locomotion &&
        mDropFlyerCooldown <= 0.f) {
      mAttackReady = true;
      if (const CPlayer* player = mgr.GetPlayer(0)) {
        // Close enough while diving: switch the queued attack to the flyer drop.
        if (mAttackType == 3 &&
            (player->GetTranslation() - GetTranslation()).Magnitude() <
                mData.unknown_0x9e116385) {
          mAttackType = 5;
        }
      }
    }
  } else if (msg == kStateMsg_Activate) {
    const SLdrSwampBossStage2Phase& phase = CurrentPhase();
    mAttackDelay = RandomRange(mgr, phase.minTimeBetweenAttacks, phase.maxTimeBetweenAttacks);
    mAttackReady = false;
    if (mgr.Random()->Float() >= phase.tauntChance) {
      NextAttack();
    } else {
      mAttackType = 7;
    }
    mDashTimer = RandomRange(mgr, phase.unknown_0x29e6ead6, phase.unknown_0x1753225e);
    PickSwoopTarget(mgr);
  }
}

// fn_79_8D90
void CSwampBossStage2::SpitWater(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) {
      const CPlayer* player = mgr.GetPlayer(0);
      const CVector3f target = player ? player->GetTranslation() : GetTranslation();
      const pas::ESeverity severity =
          mAttackType == 0 ? pas::kS_Zero : (mAttackType == 1 ? pas::kS_One : pas::kS_Two);
      BodyController()->CommandMgr().DeliverCmd(CBCProjectileAttackCmd(severity, target, false));
    }
    // Retail stores the direction to the player's aim point as the steering target vector.
    BodyController()->CommandMgr().SetTargetVector(DirectionToPlayerAim(mgr));
    RequestMove(TargetPoint(mgr));
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mSpitNode = kInvalidUniqueId;
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// fn_79_8BB4
void CSwampBossStage2::SpitBarrage(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCLoopAttackCmd(mHeadMode ? pas::kLAT_Two : pas::kLAT_Zero, true));
    } else if (mBarrageCount == CurrentPhase().unknown_0x2b0bfd51) {
      BodyController()->CommandMgr().DeliverCmd(kBSC_ExitState);
    }
    BodyController()->CommandMgr().SetTargetVector(DirectionToPlayerAim(mgr));
    RequestMove(TargetPoint(mgr));
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mBarrageCount = 0;
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// fn_79_8A3C
void CSwampBossStage2::Taunt(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
      BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(pas::kTT_Two));
    }
    RequestMove(TargetPoint(mgr));
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// fn_79_8958
void CSwampBossStage2::Level(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    const CVector3f toTarget = mLevelTarget - GetTranslation();
    BodyController()->FaceDirection(toTarget, dt);
    RequestMove(TargetPoint(mgr));
  } else if (msg == kStateMsg_Activate) {
    const CActor* node = NextChainNode(mgr);
    if (node) {
      mWindNode = node->GetUniqueId();
      mLevelTarget = node->GetTranslation();
    }
  }
}

// fn_79_82B8
void CSwampBossStage2::Swoop(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_MeleeAttack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCMeleeAttackCmd(mSwoopSide ? pas::kS_Zero : pas::kS_Two));
    }
    // The swoop rides just above the water surface.
    const CVector3f pos = GetTranslation();
    if (pos.GetZ() < mWaterZ + 5.f) {
      SetTranslation(CVector3f(pos.GetX(), pos.GetY(), mWaterZ + 5.f));
    }
    CPlayer* player = mgr.GetPlayer(0);
    if (!player) {
      return;
    }
    const CVector3f rel = player->GetAimPosition(mgr, 0.f) - mHeadPos;
    const CVector3f forward = GetTransform().GetForward();
    const CVector3f right = GetTransform().GetRight();
    const float alongForward = CVector3f::Dot(rel, forward);
    const float alongRight = CVector3f::Dot(rel, right);
    // rodata: the hit window is 30 units ahead and 18 model scales to either side.
    if (mSwoopWindow && !mSwoopDamage && !mSwoopDone && mGrappleTime == 0.f && alongForward > 0.f &&
        alongForward < 30.f &&
        CMath::AbsF(alongRight) < GetModelData()->GetScale().GetX() * 18.f) {
      player->PushSustainedDamage();
      player->DisableControls(mgr, 0xe2, GetUniqueId(), mData.swoopDamageTime,
                              CGameHint::kBHT_None);
      mSwoopTimer = mData.swoopDamageTime;
      mSwoopDamage = true;
    }
    if (mSwoopTimer > 0.f) {
      const CMaterialFilter playerFilter(CMaterialList(kMT_Player), CMaterialList(),
                                         CMaterialFilter::kFT_IncludeExclude);
      if (player->GetMorphballTransitionState() == CPlayer::kMS_Morphed) {
        // Morph Ball takes half the swoop damage.
        CDamageInfo halfSwoop(mSwoopDamageInfo);
        halfSwoop.SetDamage(mSwoopDamageInfo.GetDamage() * 0.5f);
        mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(),
                        CDamageInfo(halfSwoop, dt), playerFilter, CVector3f::Zero());
      } else {
        const CVector3f impulse =
            (forward + right * (alongRight > 0.f ? 0.5f : -0.5f)) *
            (mData.swoopPush * player->GetMass());
        player->ApplyImpulseWR(impulse, CAxisAngle::Identity());
        mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(),
                        CDamageInfo(mSwoopDamageInfo, dt), playerFilter, CVector3f::Zero());
      }
      player->SetMoveState(NPlayer::kMS_ApplyJump, mgr);
      mSwoopTimer -= dt;
      if (mSwoopTimer <= 0.f) {
        player->PopSustainedDamage();
        mSwoopDamage = false;
        mSwoopDone = true;
      }
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mSwoopWindow = false;
    mSwoopDamage = false;
    mSwoopDone = false;
    mSwoopTimer = 0.f;
    mCircleCounting = false;
    mSwooping = true;
    if (const CPlayer* player = mgr.GetPlayer(0)) {
      mSwoopSide =
          CVector3f::Dot(GetTransform().GetRight(), player->GetTranslation() - GetTranslation()) >
          0.f;
    }
  } else if (msg == kStateMsg_Deactivate) {
    mSwoopWindow = false;
    if (mSwoopDamage) {
      if (CPlayer* player = mgr.GetPlayer(0)) {
        player->PopSustainedDamage();
      }
      mSwoopDamage = false;
    }
  }
}

// fn_79_7E24
void CSwampBossStage2::Blow(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_MeleeAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCMeleeAttackCmd(pas::kS_One));
    }
    if (mWindPending) {
      if (CPlayer* player = mgr.GetPlayer(0)) {
        const CVector3f rel = player->GetAimPosition(mgr, 0.f) - mHeadPos;
        const CVector3f forward = GetTransform().GetForward();
        // rodata: the blow window is in front of the head and 18 model scales wide.
        if (CVector3f::Dot(rel, forward) > 0.f &&
            CMath::AbsF(CVector3f::Dot(rel, GetTransform().GetRight())) <
                GetModelData()->GetScale().GetX() * 18.f &&
            mGrappleTime == 0.f) {
          if (!mBlowing) {
            player->PushSustainedDamage();
            mBlowing = true;
            player->DisableControls(mgr, 0xe2, GetUniqueId(), 1.f, CGameHint::kBHT_None);
          }
          // Retail tests the pending flag again here, so the push is always the blow push.
          const float push = mWindPending ? mData.blowPush : mData.breakStunDamage;
          player->ApplyImpulseWR(forward * (push * player->GetMass()), CAxisAngle::Identity());
        }
        if (mBlowing) {
          mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(),
                          CDamageInfo(mBlowDamageInfo, dt),
                          CMaterialFilter(CMaterialList(kMT_Player), CMaterialList(),
                                          CMaterialFilter::kFT_IncludeExclude),
                          CVector3f::Zero());
          player->SetMoveState(NPlayer::kMS_ApplyJump, mgr);
        }
      }
    }
    if (mBlowing && !mWindPending) {
      if (CPlayer* player = mgr.GetPlayer(0)) {
        player->PopSustainedDamage();
      }
      mBlowing = false;
    }
    if (mBubbleCreated) {
      SpawnWindEffect(mgr);
      mBubbleCreated = false;
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mBubbleCreated = false;
    mWindPending = false;
    mBlowing = false;
  } else if (msg == kStateMsg_Deactivate) {
    if (mBlowing) {
      if (CPlayer* player = mgr.GetPlayer(0)) {
        player->PopSustainedDamage();
      }
      mBlowing = false;
    }
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// fn_79_7BE0
void CSwampBossStage2::Circle(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    // Light phases only, and only while the circle is being counted: enough damage in one circuit
    // ends it.
    const bool lightPhase = mPhase == 0 || mPhase == 2;
    const float threshold = mPhase == 0 ? mData.unknown_0xf55924da : mData.unknown_0x83bc1de7;
    if (lightPhase && threshold < mCircleDamage && mCircleCounting) {
      mCircleDamaged = true;
      mCircleDone = true;
    }
  } else if (msg == kStateMsg_Activate) {
    mClampHealth = false;
    mCircleDone = false;
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    if (mCircleDone) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCGenerateCmd(mSwoopSide ? pas::kGType_Three : pas::kGType_Four, CVector3f::Zero(),
                         false, false));
    }
    mClampHealth = true;
    mCircleCounting = false;
  }
}

// fn_79_7870
void CSwampBossStage2::Reel(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopReaction)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopHitReactionCmd(pas::kRT_Three));
    }
    BodyController()->FaceDirection(mReelDir, dt);
    RequestMove(CenterPosition(mgr));
    UpdatePlayerSideMaterials(mgr);
    // The same bit also holds the target effects on; they come up a second into the reel.
    if (!mSuppressWaterSfx && StateMachineState().GetTime() > 1.f) {
      SetGrowthTargetEffects(mgr, true);
      mSuppressWaterSfx = true;
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mReelDir = DirectionFromRandomNode(mgr);
    SetGrowthsActive(mgr, true);
    SetBodyVulnerable(mgr, false);
    mGrowthDestroyed = false;
    mGrowthsVulnerable = true;
    mPendingFlinch = 0.f;
    BodyController()->SetTurnSpeed(BodyController()->GetTurnSpeed() * 0.125f);
    SendScriptMsgs(kSS_InternalState10, mgr, kInvalidUniqueId, kSM_None);
    mRepeller.SetActive(false);
    mSuppressWaterSfx = false;
    for (int i = 0; i < mGrowthBroken.size(); ++i) {
      mGrowthBroken[i] = false;
    }
  } else if (msg == kStateMsg_Deactivate) {
    BodyController()->CommandMgr().DeliverCmd(kBSC_ExitState);
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mGrowthVulnerableTime = 0.5f;
    mHeadHealth = CurrentPhase().health;
    BodyController()->SetTurnSpeed(BodyController()->GetTurnSpeed() * 8.f);
    mAttackIndex = 0;
    AddMaterial(kMT_Orbit, kMT_Target, kMT_SeekerTarget, mgr);
    mRepeller.SetActive(true);
    SetGrowthTargetEffects(mgr, false);
  }
}

// fn_79_76EC
void CSwampBossStage2::Recover(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {
      BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(pas::kSD_Forward, pas::kStep_Dodge));
    } else if (mAnimationState.GetState() == CAnimationState::kAS_Over) {
      // Recovered: tell the chain the reaction is over and take damage again.
      SendScriptMsgs(kSS_InternalState11, mgr, kInvalidUniqueId, kSM_None);
      SetBodyVulnerable(mgr, true);
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// fn_79_755C
void CSwampBossStage2::Flinch(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {
      BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(pas::kSD_Backward, pas::kStep_Dodge));
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    SetBodyVulnerable(mgr, true);
  }
}

// fn_79_7308
void CSwampBossStage2::DropFlyer(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    BodyController()->FaceDirection(CenterPosition(mgr) - GetTranslation(), dt);
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_One, true));
    } else if (mFlyersLeft < 1) {
      BodyController()->CommandMgr().DeliverCmd(kBSC_ExitState);
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mFlyersLeft = CurrentPhase().unknown_0x2e7e55f2;
    mDropFlyerCooldown = mData.unknown_0x8fe0bf01;
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// fn_79_72FC
void CSwampBossStage2::ResetAttackPhase(CStateManager& mgr, float dt) { mAttackIndex = 0; }

// --------------------------------------------------------------------------------------------
// State machine

#define SB2_TRIGGER(name)                                                                       \
  { #name, reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CSwampBossStage2::name) }
#define SB2_STATE(name)                                                                         \
  { #name, reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CSwampBossStage2::name) }
#define SB2_CODE(name)                                                                          \
  { #name, reinterpret_cast< CPatterned::StateMachine::CodeFunc >(&CSwampBossStage2::name) }

namespace {
CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    SB2_TRIGGER(StateOver),       SB2_TRIGGER(AllGrowthsDead),   SB2_TRIGGER(OvipositorDead),
    SB2_TRIGGER(ZeroHealth),      SB2_TRIGGER(PlayerInWater),    SB2_TRIGGER(ShouldSpit),
    SB2_TRIGGER(ShouldBarrage),   SB2_TRIGGER(ShouldTaunt),      SB2_TRIGGER(ShouldSwoop),
    SB2_TRIGGER(SwoopOver),       SB2_TRIGGER(CircleDamage),     SB2_TRIGGER(CircleOver),
    SB2_TRIGGER(ShouldBlow),      SB2_TRIGGER(IsLevel),          SB2_TRIGGER(SwitchedPlatform),
    SB2_TRIGGER(Stunned),         SB2_TRIGGER(StunTimeOut),      SB2_TRIGGER(OneGrowthDead),
    SB2_TRIGGER(ShouldDropFlyer),
};
CPatterned::StateMachine::SStateFunction skStates[] = {
    SB2_STATE(Start),       SB2_STATE(BecomeLightFlyer), SB2_STATE(BecomeDarkFlyer),
    SB2_STATE(DeathSequence), SB2_STATE(Lurk),           SB2_STATE(DarkLurk),
    SB2_STATE(SpitWater),   SB2_STATE(SpitBarrage),      SB2_STATE(Taunt),
    SB2_STATE(Level),       SB2_STATE(Swoop),            SB2_STATE(Blow),
    SB2_STATE(Circle),      SB2_STATE(Reel),             SB2_STATE(Recover),
    SB2_STATE(Flinch),      SB2_STATE(DropFlyer),
};
CPatterned::StateMachine::SCodeFunction skCodes[] = {
    SB2_CODE(ResetAttackPhase),
};
} // namespace

// fn_79_A4B4
void CSwampBossStage2::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  stateMachine->SetCodeFunctions(skCodes, ARRAY_SIZE(skCodes));
}

// --------------------------------------------------------------------------------------------
// Entity / actor overrides

// fn_79_6A9C
void CSwampBossStage2::Think(float dt, CStateManager& mgr) {
  CPatterned::Think(dt, mgr);
  if (!GetActive()) {
    return;
  }
  if (!mCollisionMgr.null()) {
    mCollisionMgr->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  }
  mHeadPos = CollisionActorPosition(mgr, kHeadActor);
  mTailPos = CollisionActorPosition(mgr, kTailActor);
  mSplashPos = CollisionActorPosition(mgr, kSplashActor);
  UpdateAdditive(mgr, dt);
  UpdateHover(mgr, dt);
  if (mGrowthVulnerableTime > 0.f) {
    mGrowthVulnerableTime -= dt;
    if (mGrowthVulnerableTime <= 0.f) {
      mGrowthsVulnerable = false;
      SetGrowthsActive(mgr, false);
    }
  }
  UpdateWater(mgr, mSplashPos, mLastHeadSplash);
  // TODO(uncertain): retail also clears the player's riding-platform state while in water.
  DecayDamageMemory(dt);
  if (mHeadMode) {
    mHeadBlend = rstl::min_val(mHeadBlend + dt, 1.f);
  } else {
    mHeadBlend = rstl::max_val(mHeadBlend - dt, 0.f);
  }
  mGrowthDeadTime = mGrowthDestroyed ? mGrowthDeadTime + dt : 0.f;
  mGrappleTime = PlayerOnGrapplePoint(mgr) ? mGrappleTime + dt : 0.f;
  UpdateHealthFraction(mgr);
  for (int i = 0; i < mGrowthFlash.size(); ++i) {
    mGrowthFlash[i] = rstl::max_val(mGrowthFlash[i] - dt, 0.f);
  }
  mFlashTime = rstl::max_val(mFlashTime - dt, 0.f);
  mInterceptTime = dt;
  if (!mBubbleGen.null()) {
    mBubbleGen->Update(dt);
  }
  mRepeller.Update(mgr, *this, dt);
  if (mDropFlyerCooldown > 0.f) {
    mDropFlyerCooldown -= dt;
  }
  if (mPendingFlinch > 0.f) {
    BodyController()->CommandMgr().DeliverCmd(CBCAdditiveFlinchCmd(mPendingFlinch));
    mPendingFlinch = 0.f;
  }
}

// fn_79_6849
void CSwampBossStage2::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const bool wasActive = GetActive();
  CPatterned::AcceptScriptMsg(mgr, msg);
  switch (msg.GetMessage()) {
  case kSM_AIUpdateDisabled:
    if (!mCollisionMgr.null()) {
      mCollisionMgr->SetPhysicsActive(mgr, false);
    }
    break;
  case kSM_InternalMessage0:
    mPlayerInWater = true;
    break;
  case kSM_Deactivate:
    if (!mCollisionMgr.null()) {
      mCollisionMgr->SetActive(mgr, false);
    }
    break;
  case kSM_Activate:
    if (!wasActive) {
      Activate(mgr);
    }
    break;
  case kSM_AreaLoaded:
    mSwoopStarts = FindConnectedObjects(mgr, kSS_InternalState0, kSM_None);
    mPathNodes = FindConnectedObjects(mgr, kSS_InternalState5, kSM_None);
    mCenter = FindConnectedObject(mgr, kSS_InternalState1, kSM_None);
    mEdgeMarker = FindConnectedObject(mgr, kSS_InternalState6, kSM_None);
    mPlatformsA = FindConnectedObjects(mgr, kSS_InternalState3, kSM_None);
    mPlatformsB = FindConnectedObjects(mgr, kSS_InternalState4, kSM_None);
    mWater = FindConnectedObject(mgr, kSS_InternalState12, kSM_None);
    mLightChain = FindConnectedObject(mgr, kSS_InternalState2, kSM_None);
    // TODO(uncertain): fn_79_5830 walks each chain into its node/position lists.
    mDarkChain = FindConnectedObject(mgr, kSS_InternalState7, kSM_None);
    ReadWaterAndNodes(mgr);
    break;
  case kSM_Delete:
    if (mWaterSfx) {
      CSfxManager::RemoveEmitter(mWaterSfx);
    }
    if (!mCollisionMgr.null()) {
      mCollisionMgr->Destroy(mgr);
    }
    if (!mBubbleGen.null()) {
      mBubbleGen->SetParticleEmission(false);
      mBubbleGen = nullptr;
    }
    break;
  case kSM_Create:
    if (GetActive()) {
      Activate(mgr);
    }
    break;
  case kSM_Damage:
    ProcessDamage(mgr, msg.GetSenderId());
    break;
  default:
    break;
  }
}

// fn_79_6406
void CSwampBossStage2::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                       EUserEventType type, float dt) {
  // TODO(uncertain): body not yet ported.
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

// fn_79_6380
CVector3f CSwampBossStage2::GetAimPosition(const CStateManager& mgr, float dt) const {
  return mTailPos * mHeadBlend + mHeadPos * (1.f - mHeadBlend);
}

CScannableObjectInfo* CSwampBossStage2::GetScannableObjectInfo() const {
  // TODO(uncertain): retail picks the light or dark scan by flyer form.
  const rstl::optional_object< TLockedToken< CScannableObjectInfo > >& scan =
      mLightFlyer ? mScanLight : mScanDark;
  if (scan) {
    return const_cast< CScannableObjectInfo* >(*scan.data());
  }
  return CPatterned::GetScannableObjectInfo();
}

void CSwampBossStage2::TakeDamage(const CVector3f& dir, float amount) {
  CPatterned::TakeDamage(dir, amount);
}

// fn_79_6370
void CSwampBossStage2::PreRender(CStateManager& mgr) { CPatterned::PreRender(mgr); }

// fn_79_62E0
void CSwampBossStage2::AddToRenderer(const CStateManager& mgr) const {
  // TODO(uncertain): body not yet ported.
  CPatterned::AddToRenderer(mgr);
}

// fn_79_6267
void CSwampBossStage2::Render(const CStateManager& mgr) const {
  uint mask = 0;
  uint target = 0;
  // TODO(uncertain): retail only queries the mask when mDrawParticles is set.
  mgr.GetCharacterRenderMaskAndTarget(mask, target);
  RenderSystemsToBeDrawnFirst(mgr, mask, target);
  RenderSystemsToBeDrawnLast(mgr, mask, target);
  mRepeller.Render(mgr, *this);
}

CVector3f CSwampBossStage2::GetIngSnatchingNormal(float t) const {
  // TODO(uncertain): not yet ported.
  return CPatterned::GetIngSnatchingNormal(t);
}

CVector3f CSwampBossStage2::GetIngSnatchingPoint(float t) const {
  // TODO(uncertain): not yet ported.
  return CPatterned::GetIngSnatchingPoint(t);
}

// --------------------------------------------------------------------------------------------
// Loader

// fn_79_170
static CEntity* PortLoadSwampBossStage2(CStateManager& mgr, CInputStream& input,
                                        CEntityInfo& info) {
  SLdrSwampBossStage2 sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrSwampBossStage2.inc"
  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }
  return rs_new CSwampBossStage2(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      *modelData, LdrToActorParameters(sldrThis.actorInformation),
      LdrToPatternedInfo(sldrThis.patterned, &sldrThis.ingPossessionData),
      sldrThis.swampBossStage2Properties);
}

static SSwampBossStage2_FuncPtrs REL_loader_SwampBossStage2;

void RELMain() {
  REL_loader_SwampBossStage2.mLoader = PortLoadSwampBossStage2;
  SetSSwampBossStage2_FuncPtrs(&REL_loader_SwampBossStage2);
}

void RELExit() { SetSSwampBossStage2_FuncPtrs(nullptr); }
