#include "MetroidPrime/Enemies/CMediumIng.hpp"

#include "Kyoto/Math/CMath.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrMediumIng.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "REL/REL_Setup.h"
#ifdef TARGET_PC
#include "Collision/CCollidableAABox.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CObjectList.hpp"
#include "MetroidPrime/Enemies/CSurfaceParticleEffect.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAiJumpPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#endif

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"PathShagged", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::PathShagged)},
    {"IsOffPath", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::IsOffPath)},
    {"Leash", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::Leash)},
    {"IsMisting", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::IsMisting)},
    {"IsAggressive",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::IsAggressive)},
    {"ShouldGenerate",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::ShouldGenerate)},
    {"ShouldArmAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::ShouldArmAttack)},
    {"ShouldMeleeAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::ShouldMeleeAttack)},
    {"ShouldMistAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::ShouldMistAttack)},
    {"ShouldTaunt", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::ShouldTaunt)},
    {"ShouldDoubleDash",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::ShouldDoubleDash)},
    {"ShouldBackUp",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::ShouldBackUp)},
    {"HasLineOfSight",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::HasLineOfSight)},
    {"HasJumpPoint",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::HasJumpPoint)},
    {"HasAttackPattern",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::HasAttackPattern)},
    {"HasApproachPattern",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::HasApproachPattern)},
    {"PatternOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::PatternOver)},
    {"IsDestInsideCurrentRegion",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::IsDestInsideCurrentRegion)},
    {"IsFrustated", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::IsFrustated)},
    {"ShouldDoSafezoneReaction",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::ShouldDoSafezoneReaction)},
    {"ShouldEvaporate",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::ShouldEvaporate)},
    {"IsLuredBySafeZone",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::IsLuredBySafeZone)},
    {"StillLuredBySafeZone",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMediumIng::StillLuredBySafeZone)},
    {"InPosition", static_cast< CPatterned::StateMachine::TriggerFunc >(&CPatterned::InPosition)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"SubStart", static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::SubStart)},
    {"PathFind", static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::PathFind)},
    {"FollowAttackPattern",
     static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::FollowAttackPattern)},
    {"FollowApproachPattern",
     static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::FollowApproachPattern)},
    {"MistIn", static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::MistIn)},
    {"MistOut", static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::MistOut)},
    {"ArmAttack", static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::ArmAttack)},
    {"MeleeAttack", static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::MeleeAttack)},
    {"MistAttack", static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::MistAttack)},
    {"Jump", static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::Jump)},
    {"Dash", static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::Dash)},
    {"Taunt", static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::Taunt)},
    {"WaitForLocomotion",
     static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::WaitForLocomotion)},
    {"Idle", static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::Idle)},
    {"SafezoneReaction",
     static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::SafezoneReaction)},
    {"MoveToSafeZone",
     static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::MoveToSafeZone)},
    {"BackUp", static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::BackUp)},
    {"Dead", static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::Dead)},
    {"SelectTargetState",
     static_cast< CPatterned::StateMachine::StateFunc >(&CMediumIng::SelectTargetState)},
};

static CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    {"SelectTarget", static_cast< CPatterned::StateMachine::CodeFunc >(&CMediumIng::SelectTarget)},
    {"SetLeashTarget",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CMediumIng::SetLeashTarget)},
    {"SetRetreatDestination",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CMediumIng::SetRetreatDestination)},
    {"SetMeshPathDestination",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CMediumIng::SetMeshPathDestination)},
    {"SetDashDestination",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CMediumIng::SetDashDestination)},
    {"ResetFrustatedCounter",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CMediumIng::ResetFrustatedCounter)},
    {"IncrementFrustatedCounter",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CMediumIng::IncrementFrustatedCounter)},
};

CMediumIngData::CMediumIngData(
    const CDamageVulnerability& mistingVulnerability, CAssetId tentacleModel,
    int tentacleCharacterIndex, const CActorParameters& tentacleActorParameters, int spawnMode,
    float aggressiveness, const CColor& lightColor, float lightAttenuation,
    const CDamageInfo& mistDamage, float maxMistAttackRange, const CDamageInfo& meleeDamage,
    float maxMeleeAttackRange, float minArmAttackRange, float maxArmAttackRange,
    const CMayaSpline& attackMotion, const CCameraShakerData& attackTentacleImpact,
    const CDamageInfo& attackTentacleDamage, float noMistDamageThreshold, float tauntChance,
    const CMayaSpline& unknownSpline, const CMayaSpline& dashSpeed, CAssetId ingSpotBlobFx,
    float doubleDashChance, float minMistAttackInterval, float minArmAttackInterval,
    float minTentacleLength, float maxTentacleLength, float armAttackTime, float unknown8f1d597c,
    float minMeleeAttackInterval, float unknown0e3d3708, ushort ingSpotSound)
: mMistingVulnerability(mistingVulnerability)
, mTentacleModel(tentacleModel)
, mTentacleCharacterIndex(tentacleCharacterIndex)
, mTentacleActorParameters(tentacleActorParameters)
, mSpawnMode(spawnMode)
, mAggressiveness(aggressiveness)
, mLightColor(lightColor)
, mLightAttenuation(lightAttenuation)
, mIngSpotBlobFx(ingSpotBlobFx)
, mIngSpotSound(ingSpotSound)
, mMaxMistAttackRange(maxMistAttackRange)
, mMistDamage(mistDamage)
, mMaxMeleeAttackRange(maxMeleeAttackRange)
, mMeleeDamage(meleeDamage)
, mMinArmAttackRange(minArmAttackRange)
, mMaxArmAttackRange(maxArmAttackRange)
, mAttackMotion(attackMotion)
, mAttackTentacleImpact(attackTentacleImpact)
, mAttackTentacleDamage(attackTentacleDamage)
, mUnknownSpline(unknownSpline)
, mDashSpeed(dashSpeed)
, mNoMistDamageThreshold(noMistDamageThreshold)
, mTauntChance(tauntChance)
, mDoubleDashChance(doubleDashChance)
, mMinMistAttackInterval(minMistAttackInterval)
, mMinArmAttackInterval(minArmAttackInterval)
, mMinTentacleLength(minTentacleLength)
, mMaxTentacleLength(maxTentacleLength)
, mArmAttackTime(armAttackTime)
, mUnknown8f1d597c(unknown8f1d597c)
, mMinMeleeAttackInterval(minMeleeAttackInterval)
, mSafeZoneRangeSq(unknown0e3d3708 * unknown0e3d3708) {}

CMediumIngData::CMediumIngData(const CMediumIngData& other)
: mMistingVulnerability(other.mMistingVulnerability)
, mTentacleModel(other.mTentacleModel)
, mTentacleCharacterIndex(other.mTentacleCharacterIndex)
, mTentacleActorParameters(other.mTentacleActorParameters)
, mSpawnMode(other.mSpawnMode)
, mAggressiveness(other.mAggressiveness)
, mLightColor(other.mLightColor)
, mLightAttenuation(other.mLightAttenuation)
, mIngSpotBlobFx(other.mIngSpotBlobFx)
, mIngSpotSound(other.mIngSpotSound)
, mMaxMistAttackRange(other.mMaxMistAttackRange)
, mMistDamage(other.mMistDamage)
, mMaxMeleeAttackRange(other.mMaxMeleeAttackRange)
, mMeleeDamage(other.mMeleeDamage)
, mMinArmAttackRange(other.mMinArmAttackRange)
, mMaxArmAttackRange(other.mMaxArmAttackRange)
, mAttackMotion(other.mAttackMotion)
, mAttackTentacleImpact(other.mAttackTentacleImpact)
, mAttackTentacleDamage(other.mAttackTentacleDamage)
, mUnknownSpline(other.mUnknownSpline)
, mDashSpeed(other.mDashSpeed)
, mNoMistDamageThreshold(other.mNoMistDamageThreshold)
, mTauntChance(other.mTauntChance)
, mDoubleDashChance(other.mDoubleDashChance)
, mMinMistAttackInterval(other.mMinMistAttackInterval)
, mMinArmAttackInterval(other.mMinArmAttackInterval)
, mMinTentacleLength(other.mMinTentacleLength)
, mMaxTentacleLength(other.mMaxTentacleLength)
, mArmAttackTime(other.mArmAttackTime)
, mUnknown8f1d597c(other.mUnknown8f1d597c)
, mMinMeleeAttackInterval(other.mMinMeleeAttackInterval)
, mSafeZoneRangeSq(other.mSafeZoneRangeSq) {}

CMediumIngTentacle::CMediumIngTentacle(TUniqueId uid, TAreaId areaId, const rstl::string& name,
                                       const CModelData& modelData, const CMediumIngData& data)
: CActor(uid, name, CEntityInfo(areaId, rstl::vector< SConnection >(), false, kInvalidEditorId), 0,
         CTransform4f::Identity(), modelData, CMaterialList(), data.mTentacleActorParameters,
         kInvalidUniqueId)
, mSpline(false, 1.f, CMotionSpline::kST_Linear)
, mAttackMotion(data.mAttackMotion)
, mImpactShake(data.mAttackTentacleImpact)
, mDamage(data.mAttackTentacleDamage)
, mTipBounds()
, mSplinePoints()
#ifdef TARGET_PC
// Port: retail leaves mCanDamage to the heap; the host heap isn't zeroed.
, mCanDamage(false)
#endif
, mExtending(false)
, mRetracting(false)
, mElapsedTime(0.f)
, mRetractTime(0.f)
, mLengthScale(1.f)
, mAttackTime(data.mArmAttackTime) {
  SetDrawShadow(false);
}

CMediumIng::CMediumIng(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                       const CTransform4f& xf, const CModelData& modelData,
                       const CActorParameters& actorParams, const CPatternedInfo& patternedInfo,
                       const CMediumIngData& data)
: CPatterned(kPAI_MediumIng, uid, name, kFT_Zero, info, xf, modelData, patternedInfo, kMT_Flyer,
             kCT_Zero, kBT_AiMovedFlyer, actorParams)
, mPathFindSearch(nullptr, 0x30b, patternedInfo.GetPathfindingIndex(), 1.f, 1.f, 0,
                  CPFRegion::kRP_Center)
, mLineOfSightTracker(GetUniqueId(), CSegId(0x63), 0.1f, 0.05f)
, mSteering()
, mSurfaceAlignment()
, mData(data)
, mJumpPointId(kInvalidUniqueId)
, mTeamAiMgrId(kInvalidUniqueId)
, mSafeZoneId(kInvalidUniqueId)
, mMistState(0)
, mMistAmount(0.f)
, mF60(0.f)
, mF64(0.f)
, mAggressionTimer(0.f)
, mI6C(0)
, mI74(0)
, mV78(CVector3f::Zero())
, mV84(CVector3f::Zero())
, mV90(CVector3f::Zero())
, mDamageSinceMist(0.f)
, mTentacleIndex(0)
, mFA4(0.f)
, mFA8(1.f)
, mFAC(0.f)
, mFB0(0.f)
, mFB4(1.f)
, mFB8(mData.mMinArmAttackInterval)
, mFBC(mData.mMinMeleeAttackInterval)
, mFC0(mData.mMinMistAttackInterval)
, mFC4(1.f)
, mFC8(0.f)
, mFCC(0.f)
, mFD0(1.f)
, mTentacleIds()
, mLightId(kInvalidUniqueId)
, mMistEffectId(kInvalidUniqueId)
, mSfx0(0)
, mSfx1(0)
, mSfx2(0)
, mTauntAnim(4)
, mSafeZoneReactionCount(0)
, mC0(false)
, mC1(true)
, mC2(false)
, mC3(true)
, mC4(false)
, mC5(false)
, mC6(false)
, mC7(false)
, mD0(false)
, mD1(false)
, mD2(false)
, mD3(false)
, mD4(false)
, mD5(false)
, mD6(false)
, mD7(false)
, mE1(false)
, mE2(false) {
  mPathFindSearch.SetCharacterRadius(2.f);
  mPathFindSearch.SetCharacterHeight(5.f);
  mSurfaceAlignment.SetMode(CSurfaceAlignmentHelper::kM_NearbySurface);
  SetDrawShadow(false);
}

void CMediumIng::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  stateMachine->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
}

CEntity* LoadMediumIng(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrMediumIng sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrMediumIng.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  const CMediumIngData data(
      LdrToDamageVulnerability(sldrThis.mistingVulnerability), sldrThis.attackTentacle.ancs,
      sldrThis.attackTentacle.character_index,
      LdrToActorParameters(sldrThis.attackTentacleActorInformation), sldrThis.spawnMode,
      sldrThis.aggressiveness, sldrThis.lightColor, sldrThis.lightAttenuation,
      LdrToDamageInfo(sldrThis.mistDamage), sldrThis.maxMistAttackRange,
      LdrToDamageInfo(sldrThis.meleeDamage), sldrThis.maxMeleeAttackRange,
      sldrThis.minArmAttackRange, sldrThis.maxArmAttackRange, sldrThis.attackMotion,
      LdrToCameraShakerData(sldrThis.attackTentacleImpact, CVector3f::Zero()),
      LdrToDamageInfo(sldrThis.attackTentacleDamage), sldrThis.noMistDamageThreshold,
      sldrThis.tauntChance, sldrThis.unknown_0xb459c3e9, sldrThis.dashSpeed, sldrThis.ingSpotBlobFx,
      sldrThis.doubleDashChance, sldrThis.minMistAttackInterval, sldrThis.minArmAttackInterval,
      sldrThis.minTentacleLength, sldrThis.maxTentacleLength, sldrThis.armAttackTime,
      sldrThis.unknown_0x8f1d597c, sldrThis.minMeleeAttackInterval, sldrThis.unknown_0x0e3d3708,
      static_cast< ushort >(sldrThis.ingSpotSound));

  return rs_new CMediumIng(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                           LdrToEntityInfo(info, sldrThis.editorProperties),
                           LdrToTransform4f(sldrThis.editorProperties), *modelData,
                           LdrToActorParameters(sldrThis.actorInformation),
                           LdrToPatternedInfo(sldrThis.patterned, nullptr), data);
}

#ifndef MONOLITHIC
static void SetFuncPtrs() {
  static SMediumIng_FuncPtrs funcPtrs;
  funcPtrs.mLoader = &LoadMediumIng;
  SetSMediumIng_FuncPtrs(&funcPtrs);
}

extern "C" void RELMain() { SetFuncPtrs(); }

extern "C" void RELExit() { SetSMediumIng_FuncPtrs(nullptr); }
#endif

void CMediumIng::QuitTeam(CStateManager& mgr) {
  if (mTeamAiMgrId != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiMgrId))) {
      if (team->IsPartOfTeam(GetUniqueId())) {
        team->QuitTeam(GetUniqueId());
        mTeamAiMgrId = kInvalidUniqueId;
      }
    }
  }
}

void CMediumIng::JoinTeam(CStateManager& mgr) {
  if (mTeamAiMgrId == kInvalidUniqueId) {
    mTeamAiMgrId = CScriptTeamAiMgr::GetAssociatedTeamId(*this, mgr);
    if (mTeamAiMgrId != kInvalidUniqueId) {
      if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiMgrId))) {
        team->JoinTeam(*this, CTeamAiRole::kTAR_Melee, CTeamAiRole::kTAR_Projectile,
                       CTeamAiRole::kTAR_Unknown);
      }
    }
  }
}

bool CMediumIng::ShouldBackUp(CStateManager& mgr, const CTriggerData& data) const {
  return (mgr.GetPlayer(0)->GetTranslation() - GetTranslation()).MagSquared() < 25.f;
}

bool CMediumIng::ShouldDoubleDash(CStateManager& mgr, const CTriggerData& data) const {
  return mData.mDoubleDashChance >= mgr.Random()->Range(0.f, 100.f);
}

bool CMediumIng::ShouldTaunt(CStateManager& mgr, const CTriggerData& data) const {
  return mData.mTauntChance >= mgr.Random()->Range(0.f, 100.f);
}

bool CMediumIng::PatternOver(CStateManager& mgr, const CTriggerData& data) const {
  return mWaypointNavigation.GetDestination() == kInvalidUniqueId;
}

bool CMediumIng::HasApproachPattern(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  if (!mD0) {
    if (GetConnectedObject(mgr, kSS_Approach, kSM_Follow) != kInvalidUniqueId) {
      result = true;
    }
  }
  return result;
}

bool CMediumIng::HasAttackPattern(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  if (!mD0) {
    if (GetConnectedObject(mgr, kSS_GeneratorConnection, kSM_Follow) != kInvalidUniqueId) {
      result = true;
    }
  }
  return result;
}

bool CMediumIng::CanBeShot(const CStateManager& mgr, int type) { return !mC1; }

void CMediumIng::IncrementFrustatedCounter(CStateManager& mgr, float dt) { ++mFrustratedCounter; }

void CMediumIng::ResetFrustatedCounter(CStateManager& mgr, float dt) { mFrustratedCounter = 0; }

bool CMediumIng::StillLuredBySafeZone(CStateManager& mgr, const CTriggerData& data) const {
  return mSafeZoneId != kInvalidUniqueId;
}

bool CMediumIng::IsLuredBySafeZone(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  if (!mC1 && !mC0 && mSafeZoneId != kInvalidUniqueId) {
    result = true;
  }
  return result;
}

bool CMediumIng::ShouldEvaporate(CStateManager& mgr, const CTriggerData& data) const { return mE1; }

bool CMediumIng::ShouldDoSafezoneReaction(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  if (!HasMist() && mSafeZoneReactionCount > 0) {
    result = true;
  }
  return result;
}

bool CMediumIng::IsFrustated(CStateManager& mgr, const CTriggerData& data) const {
  return static_cast< float >(mFrustratedCounter) > data.GetFloat();
}

bool CMediumIng::IsOffPath(CStateManager& mgr, const CTriggerData& data) const {
  return GetSearchPath()->OnPath(GetTranslation()) != CPathFindSearch::kR_Success;
}

bool CMediumIng::HasJumpPoint(CStateManager& mgr, const CTriggerData& data) const {
  return mJumpPointId != kInvalidUniqueId;
}

bool CMediumIng::HasLineOfSight(CStateManager& mgr, const CTriggerData& data) const {
  return mLineOfSightTracker.HasLineOfSight();
}

void CMediumIng::TakeDamage(const CVector3f& direction, float magnitude) {
  mDamageCooldownTimer = skDamageHitTime;
  mDamageSinceMist += magnitude;
}

bool CMediumIng::ShouldGenerate(CStateManager& mgr, const CTriggerData& data) const {
  if (CPatterned::InDetectionRange(mgr, data) || mD4) {
    return true;
  }
  return false;
}

const CDamageVulnerability* CMediumIng::GetDamageVulnerability() const {
  return HasMist() ? &mData.mMistingVulnerability : CPatterned::GetDamageVulnerability();
}

bool CMediumIng::IsAggressive(CStateManager& mgr, const CTriggerData& data) const { return mC2; }

bool CMediumIng::HasMist() const { return !(CMath::AbsF(mMistAmount - 0.f) < 0.00001f); }

bool CMediumIng::IsMisting(CStateManager& mgr, const CTriggerData& data) const { return HasMist(); }

bool CMediumIng::Leash(CStateManager& mgr, const CTriggerData& data) const {
  if (mE2) {
    return true;
  }
  const CPlayer* player = mgr.GetPlayer(0);
  if (player->GetCurrentAreaId() != GetCurrentAreaId()) {
    return true;
  }
  if ((player->GetTranslation() - GetTranslation()).MagSquared() > mLeashRadius * mLeashRadius &&
      mFC8 > 5.f) {
    return true;
  }
  return false;
}

bool CMediumIng::PathShagged(CStateManager& mgr, const CTriggerData& data) const {
  if (CPatterned::PathShagged(mgr, data) || mC4) {
    return true;
  }
  return false;
}

bool CMediumIng::IsPointInSafeZone(const CStateManager& mgr, const CVector3f& pos) const {
  return mgr.GetSafeZoneManager()->PointIsInSafeZone(mgr, pos);
}

bool CMediumIng::IsInSafeZone(const CStateManager& mgr) const {
  return mgr.GetSafeZoneManager()->IsObjectInSafeZone(*mgr.GetPlayer(0), mgr);
}

void CMediumIng::ValidateSafeZone(const CStateManager& mgr) {
  const CScriptSafeZone* safeZone =
      TCastToConstPtr< CScriptSafeZone >(mgr.GetObjectById(mSafeZoneId));
  if (!safeZone || !safeZone->GetActive() || safeZone->GetZoneType() != CScriptSafeZone::kZT_Echo) {
    mSafeZoneId = kInvalidUniqueId;
  }
}

CPFArea* CMediumIng::GetPathArea(CStateManager& mgr) const {
  return mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea;
}

bool CMediumIng::IsDestInsideCurrentRegion(CStateManager& mgr, const CTriggerData& data) const {
  const CVector3f position = GetTranslation();
  const CVector3f dest = mDestPos;
  if ((dest - position).MagSquared() > 0.0625f) {
    return false;
  }
  CPFArea* area = GetPathArea(mgr);
  const CPFRegion* region = area->FindClosestRegion(position, GetSearchPath()->GetRegionFlags(),
                                                    GetSearchPath()->GetCreatureMask(), 10.f);
  const CPFRegion* destRegion = area->FindClosestRegion(dest, GetSearchPath()->GetRegionFlags(),
                                                        GetSearchPath()->GetCreatureMask(), 10.f);
  bool result = false;
  if (region && region == destRegion) {
    result = true;
  }
  return result;
}

bool CMediumIng::ShouldMistAttack(CStateManager& mgr, const CTriggerData& data) const {
  if (mLineOfSightTracker.HasLineOfSight()) {
    bool mistReady = false;
    if (HasMist() && mFB4 > 1.f) {
      mistReady = true;
    }
    if (mistReady || mFB4 > 1.f) {
      if (mFC0 > mData.mMinMistAttackInterval && (mistReady || !mC2)) {
        const CPlayer* player = mgr.GetPlayer(0);
        CVector3f toPlayer = player->GetTranslation() - GetTranslation();
        const float distance = toPlayer.Magnitude();
        toPlayer *= 1.f / distance;
        if (CVector3f::GetAngleDiff(player->GetTransform().GetForward(), -toPlayer) < 0.7853982f &&
            distance * distance < mData.mMaxMistAttackRange * mData.mMaxMistAttackRange) {
          return true;
        }
      }
    }
  }
  return false;
}

bool CMediumIng::ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const {
  if (!IsInSafeZone(mgr) && !HasMist() && mFB4 > 1.f && mLineOfSightTracker.HasLineOfSight() &&
      (mTeamAiMgrId == kInvalidUniqueId ||
       CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Melee, mgr, mTeamAiMgrId,
                                        GetUniqueId()))) {
    const float distanceSquared =
        (mgr.GetPlayer(0)->GetTranslation() - GetTranslation()).MagSquared();
    if (mFBC > mData.mMinMeleeAttackInterval) {
      return distanceSquared < mData.mMaxMeleeAttackRange * mData.mMaxMeleeAttackRange;
    }
  }
  return false;
}

bool CMediumIng::ShouldArmAttack(CStateManager& mgr, const CTriggerData& data) const {
  const bool armReady = mFC4 < 1.f;
  if ((armReady || (mFB4 > 1.f && !HasMist())) &&
      (armReady || mFB8 > mData.mMinArmAttackInterval) &&
      (mTeamAiMgrId == kInvalidUniqueId ||
       CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamAiMgrId,
                                        GetUniqueId()))) {
    const float distanceSquared =
        (mgr.GetPlayer(0)->GetTranslation() - GetTranslation()).MagSquared();
    const bool farEnough = distanceSquared >= mData.mMinArmAttackRange * mData.mMinArmAttackRange;
    bool result = false;
    if (distanceSquared < mData.mMaxArmAttackRange * mData.mMaxArmAttackRange) {
      if (farEnough || IsInSafeZone(mgr)) {
        result = true;
      }
    }
    return result;
  }
  return false;
}

#ifndef TARGET_PC
void CMediumIng::SubStart(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::FollowApproachPattern(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::MistIn(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::MistOut(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::ArmAttack(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::MistAttack(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::Jump(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::Dash(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::Taunt(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::WaitForLocomotion(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::Idle(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::SafezoneReaction(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::MoveToSafeZone(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::BackUp(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::Dead(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::SelectTargetState(CStateManager& mgr, EStateMsg msg, float dt) {}
void CMediumIng::SelectTarget(CStateManager& mgr, float dt) {}
void CMediumIng::SetLeashTarget(CStateManager& mgr, float dt) {}
void CMediumIng::SetRetreatDestination(CStateManager& mgr, float dt) {}
void CMediumIng::SetMeshPathDestination(CStateManager& mgr, float dt) {}
void CMediumIng::SetDashDestination(CStateManager& mgr, float dt) {}
#endif
#ifdef TARGET_PC
// Port bodies for the MediumIng states and the code functions they use (REL module 41).

static const char* const skTentacleLocators[] = {
    "R_TopArm_LCTR", "L_TopArm_LCTR", "R_BotArm_LCTR", "L_BotArm_LCTR",
};

// fn_41_6F18: the summed squared segment lengths of a waypoint list (no square roots).
static float PathCostSquared(const rstl::reserved_vector< CVector3f, 16 >& waypoints) {
  float cost = 0.f; // 0x6F2C
  for (int i = 0; i < waypoints.size() - 1; ++i) {
    cost += (waypoints[i + 1] - waypoints[i]).MagSquared();
  }
  return cost;
}

struct SJumpRoute {
  bool mValid;
  float mCost;
};

// fn_41_6D08: the straight-line cost of walking to a jump point, jumping to its landing point and
// walking on to the target.
static float JumpRouteEstimate(const CStateManager& mgr, const CVector3f& pos, TUniqueId jumpId,
                               const CVector3f& target) {
  const CScriptAiJumpPoint* jump =
      TCastToConstPtr< CScriptAiJumpPoint >(mgr.GetObjectById(jumpId));
  if (!jump) {
    return FLT_MAX; // 0x6EC4
  }
  const CActor* land = TCastToConstPtr< CActor >(mgr.GetObjectById(jump->GetJumpPoint())); // 0x6D7C
  if (!land) {
    return 0.f; // 0x6EB8
  }
  return (jump->GetTranslation() - pos).MagSquared() +
         (land->GetTranslation() - jump->GetTranslation()).MagSquared() +
         (target - land->GetTranslation()).MagSquared();
}

// fn_41_6BFC: searches the two walks around a jump; the cost is the second walk's path.
static SJumpRoute SearchJumpRoute(CPathFindSearch& search, const CStateManager& mgr,
                                  const CVector3f& pos, TUniqueId jumpId,
                                  const CVector3f& target) {
  SJumpRoute route;
  route.mValid = false;
  route.mCost = 0.f;
  const CScriptAiJumpPoint* jump =
      TCastToConstPtr< CScriptAiJumpPoint >(mgr.GetObjectById(jumpId));
  if (jump && search.Search(pos, jump->GetTranslation()) == CPathFindSearch::kR_Success) { // 0x6C58
    const CActor* land =
        TCastToConstPtr< CActor >(mgr.GetObjectById(jump->GetJumpPoint())); // 0x6C60
    if (land &&
        search.Search(land->GetTranslation(), target) == CPathFindSearch::kR_Success) { // 0x6CA4
      route.mValid = true;
      route.mCost = PathCostSquared(search.GetWaypoints()); // 0x6CC4
    }
  }
  return route;
}

void CMediumIng::FaceTarget(CStateManager& mgr, float dt) {
  BodyController()->FaceDirection(mgr.GetPlayer(0)->GetTranslation() - GetTranslation(), dt);
}

void CMediumIng::ExtendTentacle(CStateManager& mgr, int index) {
  if (index < 0 || index >= mTentacleIds.size()) { // 0x4330, 0x433C
    return;
  }
  CEntity* entity = mgr.ObjectById(mTentacleIds[index]);
  if (!entity) { // 0x4360
    return;
  }
  if (index % 2 == 0) { // 0x4364..0x4374
    mV90 = mgr.GetPlayer(0)->GetHomingPosition(mgr, 0.25f); // 0x4390 rodata_78
  }
  static_cast< CMediumIngTentacle* >(entity)->Extend(mgr, mV90);
}

void CMediumIng::RetractTentacles(CStateManager& mgr) {
  for (int i = 0; i < mTentacleIds.size(); ++i) {
    if (CEntity* entity = mgr.ObjectById(mTentacleIds[i])) { // 0x442C
      static_cast< CMediumIngTentacle* >(entity)->Retract(mgr);
    }
  }
}

void CMediumIng::PositionTentacles(CStateManager& mgr) {
  for (int i = 0; i < mTentacleIds.size(); ++i) {
    CActor* tentacle = TCastToPtr< CActor >(mgr.ObjectById(mTentacleIds[i]));
    if (!tentacle) { // 0x44B8
      continue;
    }
    // 0x44D8 rodata_80 = -1.078, 0x44D4 rodata_40 = 0.
    const CTransform4f offset = CTransform4f::Translate(CVector3f(-1.078f, 0.f, 0.f));
    tentacle->SetTransform(GetTransform() *
                           GetScaledLocatorTransform(rstl::string_l(skTentacleLocators[i])) *
                           offset);
    tentacle->SetModelFlags(GetModelFlags().UseShaderSet(0)); // 0x4560 stb 0 at +0x101
  }
}

void CMediumIng::SetTentaclesActive(CStateManager& mgr, bool active) {
  mC3 = !active; // 0x45B4 cntlzw, 0x45C4 rlwimi 0x10
  for (int i = 0; i < mTentacleIds.size(); ++i) {
    if (CEntity* entity = mgr.ObjectById(mTentacleIds[i])) { // 0x45E4
      entity->SetActive(active);
    }
  }
}

void CMediumIng::SubStart(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mC1 = true;         // 0x73DC 0xCFC |= 0x40
    mE2 = false;        // 0x73F0 0xCFE &= ~0x20
    SetMovable(false);  // 0x73FC 0x168 &= ~0x80
    SetMistCollision(false);
    RemoveMaterial(kMT_Orbit, kMT_Target, kMT_Solid, mgr); // 0x7410 0x29 0x28 0x3B
    SetMuted(true);
    break;
  case kStateMsg_Deactivate:
    mC0 = true;        // 0x7438 0xCFC |= 0x80
    SetMovable(true);  // 0x7448
    SetMuted(false);
    BodyController()->SetLocomotionType(pas::kLT_Relaxed); // 0x7458 li 1
    break;
  default:
    break;
  }
}

void CMediumIng::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  mPathFindNavigation.PathFind(mgr, msg, dt, *this);
  switch (msg) {
  case kStateMsg_Activate:
    mC4 = false; // 0x7314 0xCFC &= ~0x08
    if (IsMistDecayed()) {
      GetSearchPath()->SetFlags(0x30b); // 0x7340
    } else {
      GetSearchPath()->SetFlags(0x303); // 0x7360
    }
    break;
  case kStateMsg_Update:
    CalculateSeparation(mgr);
    break;
  default:
    break;
  }
}

void CMediumIng::FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float dt) {
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
  if (msg != kStateMsg_Activate) {
    return;
  }
  // 'GRNT' / 'FOLW'
  const TUniqueId dest = GetConnectedObject(mgr, kSS_GeneratorConnection, kSM_Follow);
  mWaypointNavigation.SetDestination(dest);
  if (const CScriptWaypoint* wp = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(dest))) {
    // fcmpo + cror eq,lt,eq: forward . (wp - pos) <= 0 (rodata_40).
    if (CVector3f::Dot(GetTransform().GetForward(), wp->GetTranslation() - GetTranslation()) <=
        0.f) {
      mWaypointNavigation.SetInPosition(true);
    }
  }
}

void CMediumIng::FollowApproachPattern(CStateManager& mgr, EStateMsg msg, float dt) {
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
  if (msg != kStateMsg_Activate) {
    return;
  }
  // 0x70F4..0x710C: 'APRC' / 'FOLW'
  const TUniqueId dest = GetConnectedObject(mgr, kSS_Approach, kSM_Follow);
  mWaypointNavigation.SetDestination(dest); // 0x7120 sth 0x6C0
  if (const CScriptWaypoint* wp = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(dest))) {
    // 0x7184 fcmpo + 0x7188 cror eq,lt,eq: forward . (wp - pos) <= 0 (rodata_40).
    if (CVector3f::Dot(GetTransform().GetForward(), wp->GetTranslation() - GetTranslation()) <=
        0.f) {
      mWaypointNavigation.SetInPosition(true); // 0x7198 0x6F0 |= 0x80
    }
  }
}

void CMediumIng::MistIn(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    AddMaterial(kMT_Orbit, kMT_Target, kMT_Solid, mgr); // 0x58C8 0x29 0x28 0x3B
    SetMistCollision(true);
    mC0 = true; // 0x58EC 0xCFC |= 0x80
    mV78 = GetTranslation();
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x590C
    break;
  case kStateMsg_Update: {
    float amount = mFCC + dt;
    if (amount >= 1.f) { // 0x5924 fcmpo + bge, rodata_14
      amount = 1.f;
    }
    mFCC = amount;
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) { // 0x593C li 0xC
      bool generate = false;
      if (mD0) { // 0x5958 extrwi 0x80 of 0xCFD
        generate = true;
      } else if (mData.mSpawnMode == 0) { // 0x5960 lwz 0x9E8
        generate = true;
      } else if (mData.mSpawnMode == 1) {
        BodyController()->CommandMgr().DeliverCmd(
            CBCGenerateCmd(pas::kGType_Four, CVector3f::Zero())); // 0x59B8 li 4
      }
      if (generate) {
        BodyController()->CommandMgr().DeliverCmd(
            CBCGenerateCmd(pas::kGType_Three, CVector3f::Zero())); // 0x5A98 li 3
      }
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) { // 0x5B40 cmpwi 0xC
      // 0x5B64 rodata_10 = 2, 0x5B6C ble.
      if (StateMachineState().GetTime() > 2.f * dt) {
        mC1 = false; // 0x5B78 0xCFC &= ~0x40
      }
    }
    break;
  }
  case kStateMsg_Deactivate:
    mC0 = false;                                               // 0x5B8C
    mSurfaceAlignment.SetMode(CSurfaceAlignmentHelper::kM_WorldUp); // 0x5B9C stw 2, 0x944
    mAnimationState.SetState(CAnimationState::kAS_NotReady);   // 0x5BA0
    mD0 = true;                                                // 0x5BA8 0xCFD |= 0x80
    mI74 = 0;                                                  // 0x5BB0
    break;
  default:
    break;
  }
}

void CMediumIng::MistOut(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x4BD4
    break;
  case kStateMsg_Update: {
    if (StateMachineState().GetTime() > 0.9f) { // 0x4BF8 rodata_88
      mC0 = true;                                  // 0x4C08
    }
    float amount = mFCC - dt;
    if (0.f >= amount) { // 0x4C20 fcmpo rodata_40
      amount = 0.f;
    }
    mFCC = amount; // 0x4C30
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) { // 0x4C38 li 0xC
      BodyController()->CommandMgr().DeliverCmd(
          CBCGenerateCmd(pas::kGType_One, CVector3f::Zero())); // 0x4C80 li 1
    }
    break;
  }
  case kStateMsg_Deactivate:
    if (mSfx1) { // 0x4D24
      CSfxManager::RemoveEmitter(mSfx1);
      mSfx1 = CSfxHandle(); // 0x4D40 0xCE8
    }
    if (mSfx2) { // 0x4D48
      CSfxManager::RemoveEmitter(mSfx2);
      mSfx2 = CSfxHandle(); // 0x4D64 0xCEC
    }
    mC0 = false;                                             // 0x4D70
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x4D7C
    mC1 = true;                                              // 0x4D84 0xCFC |= 0x40
    mD4 = false;                                             // 0x4D90 0xCFD &= ~0x08
    mE2 = false;                                             // 0x4D9C 0xCFE &= ~0x20
    break;
  default:
    break;
  }
}

void CMediumIng::ArmAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (mTeamAiMgrId == kInvalidUniqueId ||
        CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamAiMgrId,
                                      GetUniqueId())) { // 0x3FA4 li 1
      mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x3FCC
      mE0 = false;                                          // 0x3FDC
      mTentacleIndex = 0;
      mFA4 = 0.f; // 0x3FE8
      mC6 = true; // 0x3FF0 0xCFC |= 0x02
    } else {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x4000
    }
    break;
  case kStateMsg_Update:
    PositionTentacles(mgr); // 0x4008
    if (HasMist()) {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x4244
    } else if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_One)); // 0x4054 li 1
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_LoopAttack) { // 0x40C8 cmpwi 9
      FaceTarget(mgr, dt);
      if (StateMachineState().GetTime() > 1.f) { // 0x412C fcmpo rodata_14 + ble
        if (!mE0) {                                 // 0x4138 0xCFE & 0x80
          RetractTentacles(mgr);
          SetTentaclesActive(mgr, true);
          mE0 = true; // 0x4164
        }
        mFA4 -= dt;          // 0x417C
        if (mFA4 < 0.f) {    // 0x4184 fcmpo rodata_40 + bge
          if (mTentacleIndex > 4) { // 0x4190 cmplwi 4 + ble
            bool allRetracted = true;
            for (int i = 0; i < mTentacleIds.size(); ++i) {
              if (CEntity* entity = mgr.ObjectById(mTentacleIds[i])) { // 0x41C0
                CMediumIngTentacle* tentacle = static_cast< CMediumIngTentacle* >(entity);
                tentacle->SetRetracting(true); // 0x41CC 0x31C |= 0x20
                if (!(tentacle->GetLengthScale() < 0.f)) { // 0x41D8 fcmpo 0x328 + blt
                  allRetracted = false;
                }
              }
            }
            if (allRetracted) {
              mAnimationState.SetState(CAnimationState::kAS_Over); // 0x4204
            }
          }
          mFA4 = mData.mUnknown8f1d597c; // 0x4208 lfs 0xC44
          ExtendTentacle(mgr, mTentacleIndex);
          ++mTentacleIndex; // 0x4228
        }
      }
      if (mD6) { // 0x4230 0xCFD & 0x02
        mAnimationState.SetState(CAnimationState::kAS_Over); // 0x423C
      }
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x4258
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState)); // 0x4260 li 0xA
    SetTentaclesActive(mgr, false);
    mFB8 = 0.f; // 0x42B8
    mFB4 = 0.f; // 0x42C4
    mC6 = false; // 0x42CC
    CScriptTeamAiMgr::EndAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamAiMgrId,
                                GetUniqueId(), false); // 0x42BC li 1, 0x42C0 li 0
    break;
  default:
    break;
  }
}

void CMediumIng::MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (mTeamAiMgrId == kInvalidUniqueId ||
        CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Melee, mgr, mTeamAiMgrId,
                                      GetUniqueId())) { // 0x3B50 li 0
      mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x3B78
      mC6 = true;                                           // 0x3B8C
      mI6C = 0;                                             // 0x3B9C
      mF70 = 0.25f;                                         // 0x3BA0 rodata_78
      KnockBackController().EnableAnimReaction(CKnockBackMgr::kAR_KnockBack, false); // 0x3B90
    } else {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x3BB0
    }
    break;
  case kStateMsg_Update:
    if (HasMist()) {
      mAnimationState.SetState(CAnimationState::kAS_Over);
    } else if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_Four)); // 0x3BFC li 4
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_LoopAttack) { // 0x3C70 cmpwi 9
      if (StateMachineState().GetTime() < 0.5f) { // 0x3C94 rodata_38 + bge
        FaceTarget(mgr, dt);
      }
      if (StateMachineState().GetTime() < 2.f) { // 0x3CFC rodata_10 + bge
        if (mI6C < 4) {                             // 0x3D08 cmpwi 4 + bge
          mF70 += dt;
          if (mF70 > 0.25f) { // 0x3D28 rodata_78 + ble
            const CPlayer* player = mgr.GetPlayer(0);
            const float radius = mData.mMeleeDamage.GetRadius(); // 0x3D34 0xA30
            // 0x3D80 fcmpo + bge
            if ((player->GetTranslation() - GetTranslation()).MagSquared() < radius * radius) {
              ++mI6C;      // 0x3D9C
              mF70 = 0.f;  // 0x3DAC
              CDamageInfo damage = mData.mMeleeDamage;
              damage.SetRadius(0.f);                                  // 0x3E08
              damage.SetKnockBackPower(2.f * damage.GetKnockBackPower()); // 0x3DE0 rodata_10
              // 0x3DB8 data_0 = 0x3B (Solid); 0x3E14 li 3 = kFT_IncludeExclude.
              mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(), damage,
                              CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid),
                                                                  CMaterialList()),
                              CVector3f::Zero());
            }
          }
        }
      } else {
        BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState)); // 0x3E80
      }
    }
    break;
  case kStateMsg_Deactivate:
    KnockBackController().EnableAnimReaction(CKnockBackMgr::kAR_KnockBack, true); // 0x3EC4
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x3ED4
    mC6 = false;  // 0x3EE8
    mFBC = 0.f;   // 0x3EFC
    mFB4 = 0.f;   // 0x3F00
    CScriptTeamAiMgr::EndAttack(CScriptTeamAiMgr::kAT_Melee, mgr, mTeamAiMgrId, GetUniqueId(),
                                false); // 0x3EF0 li 0, 0x3EF8 li 0
    break;
  default:
    break;
  }
}

void CMediumIng::MistAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (HasMist()) {
      mC7 = true; // 0x3810
    }
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x3820
    mD2 = true;   // 0x3830 0xCFD |= 0x20
    mD3 = false;  // 0x383C 0xCFD &= ~0x10
    mFB0 = 0.f;   // 0x3844
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_Two)); // 0x3884 li 2
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_LoopAttack) { // 0x38F8 cmpwi 9
      if (StateMachineState().GetTime() > 0.75f) { // 0x391C rodata_74
        mC7 = true;                                   // 0x392C
      }
      // 0x3938 0xCFD & 0x10; 0x395C rodata_14
      if (!mD3 && StateMachineState().GetTime() < 1.f) {
        FaceTarget(mgr, dt);
      }
      if (mD3) { // 0x39AC
        mFB0 += dt; // 0x39BC
      }
      // 0x39CC rodata_78, 0x39F0 rodata_7C
      if (mFB0 > 0.25f || StateMachineState().GetTime() > 1.5f) {
        BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState)); // 0x3A18
        mC7 = false; // 0x3A38
      }
    }
    break;
  case kStateMsg_Deactivate:
    mD2 = false;  // 0x3A50
    mFC0 = 0.f;   // 0x3A5C
    mFB4 = 0.f;   // 0x3A60
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x3A64
    break;
  default:
    break;
  }
}

void CMediumIng::Jump(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (mJumpPointId != kInvalidUniqueId) { // 0x5774
      SetDestPos(mV84);
    }
    mDestObj = kInvalidUniqueId;          // 0x5790
    mReflectedDestPos = GetTranslation(); // 0x5798
    mInPosition = false;                  // 0x57B4
    break;
  case kStateMsg_Update:
    // 0x57CC rodata_78 = 0.25
    mC7 = IsBlocked(mgr, PredictLinearMotion(0.25f).GetTranslation()); // 0x5818/0x582C
    if (!mInPosition) { // 0x5838
      ApproachDest(mgr);
    }
    break;
  case kStateMsg_Deactivate:
    mC7 = false; // 0x5858
    break;
  default:
    break;
  }
}

void CMediumIng::Dash(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mDestObj = kInvalidUniqueId;
    mReflectedDestPos = GetTranslation(); // 0x5064
    mInPosition = false;                  // 0x507C
    mFA8 = 1.f;                           // 0x5084 rodata_14
    mFAC = 0.f;                           // 0x5088 rodata_40
    break;
  case kStateMsg_Update: {
    const CVector3f total = mDestPos - mReflectedDestPos;
    if (total.CanBeNormalized()) { // 0x50C4
      const float totalDist = total.Magnitude();
      const CVector3f remaining = mDestPos - GetTranslation();
      if (remaining.CanBeNormalized()) { // 0x5110
        float t = 1.f - remaining.Magnitude() / totalDist; // 0x512C rodata_14
        // 0x513C fcmpo 0 (rodata_40), t; 0x514C fcmpo 1, t: NaN passes through.
        if (0.f > t) {
          t = 0.f;
        } else if (1.f < t) {
          t = 1.f;
        }
        mFA8 = mData.mDashSpeed.EvaluateAt(t); // 0x5168
        const float height = mReflectedDestPos.GetZ() +
                             t * (mDestPos.GetZ() - mReflectedDestPos.GetZ()) +
                             mData.mUnknownSpline.EvaluateAt(t);
        mFAC = (height - GetTranslation().GetZ()) / dt; // 0x5198
      }
    }
    if (!mInPosition) { // 0x51A0
      ApproachDest(mgr);
    }
    FaceTarget(mgr, dt); // 0x51F4
    break;
  }
  case kStateMsg_Deactivate:
    mFA8 = 1.f; // 0x520C
    mFAC = 0.f; // 0x5210
    mFC4 = 0.f; // 0x5214
    break;
  default:
    break;
  }
}

void CMediumIng::Taunt(CStateManager& mgr, EStateMsg msg, float dt) {
  // rodata_0: the two taunt types.
  static const pas::ETauntType skTauntTypes[] = {pas::kTT_One, pas::kTT_Four};
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);         // 0x4E74
    mTauntAnim = skTauntTypes[mgr.Random()->Range(0, 1)];          // 0x4E80, 0x4E94
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) { // 0x4EA4 li 0x10
      BodyController()->CommandMgr().DeliverCmd(
          CBCTauntCmd(static_cast< pas::ETauntType >(mTauntAnim))); // 0x4ED0 li 0x15
    }
    FaceTarget(mgr, dt); // 0x4F58
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x4F64
    break;
  default:
    break;
  }
}

void CMediumIng::WaitForLocomotion(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    break;
  case kStateMsg_Update:
    if (BodyController()->GetCurrentStateId() == pas::kAS_Locomotion) { // 0x1EB0 cmpwi 5
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  default:
    break;
  }
}

void CMediumIng::Idle(CStateManager& mgr, EStateMsg msg, float dt) {}

void CMediumIng::SafezoneReaction(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mC6 = true;                                                                // 0x5484
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState)); // 0x54AC
    mAnimationState.SetState(CAnimationState::kAS_Ready);                      // 0x54C8
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) { // 0x54D8 li 0xC
      if (mD1) { // 0x54EC 0xCFD & 0x40
        BodyController()->CommandMgr().DeliverCmd(
            CBCGenerateCmd(pas::kGType_Six, CVector3f::Zero())); // 0x5528 li 6
      } else {
        BodyController()->CommandMgr().DeliverCmd(
            CBCGenerateCmd(pas::kGType_Five, CVector3f::Zero())); // 0x55FC li 5
      }
    }
    FaceTarget(mgr, dt); // 0x56D8
    break;
  case kStateMsg_Deactivate:
    mC6 = false;                                             // 0x56E8
    mD1 = false;                                             // 0x56F4
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x56FC
    break;
  default:
    break;
  }
}

void CMediumIng::MoveToSafeZone(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    CVector3f point = CVector3f::Zero();
    if (FindSafeZoneRetreatPoint(mgr, point)) { // 0x52A0
      SetDestPos(point);
      mDestObj = kInvalidUniqueId;
      mReflectedDestPos = GetTranslation(); // 0x52CC
      mInPosition = false;                  // 0x52E4
    } else {
      mInPosition = true; // 0x52F8
    }
    break;
  }
  case kStateMsg_Update:
    mC7 = IsBlocked(mgr, PredictLinearMotion(0.25f).GetTranslation()); // 0x530C rodata_78
    if (!mInPosition) { // 0x5380
      ApproachDest(mgr);
    }
    break;
  case kStateMsg_Deactivate:
    mC7 = false; // 0x53A0
    break;
  default:
    break;
  }
}

void CMediumIng::BackUp(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x2EC0
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) { // 0x2ED4 li 3
      BodyController()->CommandMgr().DeliverCmd(
          CBCStepCmd(pas::kSD_Backward, pas::kStep_Normal)); // 0x2F10 li 1, 0x2F00 li 0
    }
    FaceTarget(mgr, dt); // 0x2FD4
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x2FDC
    break;
  default:
    break;
  }
}

void CMediumIng::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    RemoveMaterial(kMT_Orbit, kMT_Target, mgr); // 0x30F4 0x29 0x28
    SetMistCollision(true);
    break;
  case kStateMsg_Update: {
    Stop();
    SetMomentumWR(CVector3f::Zero()); // 0x3124
    FaceTarget(mgr, dt);
    float amount = mFCC - dt;
    if (0.f >= amount) { // 0x3184 fcmpo rodata_40, bge
      amount = 0.f;
    }
    mFCC = amount;
    amount = mFD0 - 0.33f * dt; // 0x31B0 rodata_6C
    if (0.f >= amount) {        // 0x31B4
      amount = 0.f;
    }
    mFD0 = amount;
    if (BodyController()->GetCurrentStateId() == pas::kAS_Death) { // 0x31D0 cmpwi 2
      DeathDelete(mgr);
    }
    if (mE1) { // 0x31E8 0xCFE & 0x40
      if (CEntity* entity = mgr.ObjectById(mMistEffectId)) { // 0x3204
        if (static_cast< CSurfaceParticleEffect* >(entity)->GetParticleGen()->GetParticleCount() ==
            0) { // 0x3214 vslot 0x74
          mgr.DeleteObjectRequest(GetUniqueId());
        }
      }
    }
    break;
  }
  default:
    break;
  }
}

void CMediumIng::SelectTargetState(CStateManager& mgr, EStateMsg msg, float dt) {}

void CMediumIng::SelectTarget(CStateManager& mgr, float dt) {
  JoinTeam(mgr);
  mHitByPlayerProjectile = false; // 0x69C0 0x420 &= ~0x80
  mJumpPointId = kInvalidUniqueId; // 0x69DC
  const TUniqueId playerId = mgr.GetPlayer(0)->GetUniqueId();
  mLineOfSightTracker.SetTarget(playerId);
  CVector3f target = mgr.GetPlayer(0)->GetTranslation();
  if (GetSearchPath()->Search(GetTranslation(), target) == CPathFindSearch::kR_Success) { // 0x6A2C
    const float direct = PathCostSquared(GetSearchPath()->GetWaypoints()); // 0x6A4C
    float best = FLT_MAX;                                                  // 0x6A5C rodata_50
    CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint); // mgr+0x838
    for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
      CScriptAiJumpPoint* jump = TCastToPtr< CScriptAiJumpPoint >(list[i]);
      if (!jump || !jump->GetActive() || jump->GetType() != 1 || // 0x6A84, 0x6A90
          jump->GetCurrentAreaId() != GetCurrentAreaId()) {      // 0x6AA0
        continue;
      }
      const CActor* land =
          TCastToConstPtr< CActor >(mgr.GetObjectById(jump->GetJumpPoint())); // 0x6AA8 +0x180
      if (!land || IsPointInSafeZone(mgr, jump->GetTranslation()) ||
          IsPointInSafeZone(mgr, land->GetTranslation())) { // 0x6AD8, 0x6AF0
        continue;
      }
      const float estimate =
          JumpRouteEstimate(mgr, GetTranslation(), jump->GetUniqueId(), target);
      // 0x6B18/0x6B20 fcmpo + bge
      if (estimate < direct && estimate < best) {
        const SJumpRoute route = SearchJumpRoute(*GetSearchPath(), mgr, GetTranslation(),
                                                 jump->GetUniqueId(), target); // 0x6B44
        if (route.mValid && route.mCost < best) { // 0x6B50, 0x6B58 fcmpo + bge
          best = route.mCost;
          mJumpPointId = jump->GetUniqueId(); // 0x6B68
          target = jump->GetTranslation();
          mV84 = land->GetTranslation(); // 0x6B88
        }
      }
    }
  }
  mPathFindNavigation.SetDestination(target); // 0x6BC8
  mPathFindNavigation.SetFaceTarget(playerId); // 0x6BD4 sth 0x718
}

void CMediumIng::SetLeashTarget(CStateManager& mgr, float dt) {
  const TUniqueId danger = FindClosestDangerActor(mgr, GetTranslation()); // 0x68D4
  if (danger != kInvalidUniqueId) {
    mJumpPointId = danger; // 0x6920
    if (const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(danger))) { // 0x6908
      mV84 = actor->GetTranslation();
    } else {
      mV84 = GetTranslation();
    }
  } else {
    mJumpPointId = kInvalidUniqueId;
    mV84 = mV78; // 0x6944
    SetDestPos(mV84);
  }
}

void CMediumIng::SetRetreatDestination(CStateManager& mgr, float dt) {
  const TUniqueId retreat = FindClosestRetreatActor(mgr, GetTranslation()); // 0x25A8
  if (const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(retreat))) { // 0x25D8
    SetDestPos(actor->GetTranslation());
  } else {
    SetDestPos(GetTranslation());
  }
}

void CMediumIng::SetMeshPathDestination(CStateManager& mgr, float dt) {
  static const CVector3f skLift(0.f, 0.f, 3.f); // 0x2AC0 rodata_54
  mJumpPointId = kInvalidUniqueId;
  CVector3f dest = mgr.GetPlayer(0)->GetTranslation();
  if (GetSearchPath()->FindClosestReachablePoint(dest + skLift, dest) ==
      CPathFindSearch::kR_Success) { // 0x2B3C
    SetDestPos(dest);
  } else {
    SetDestPos(GetTranslation());
  }
}

void CMediumIng::SetDashDestination(CStateManager& mgr, float dt) {
  mJumpPointId = kInvalidUniqueId; // 0x2694
  const CVector3f playerPos = mgr.GetPlayer(0)->GetTranslation();
  // 0x2690 rodata_38 = 0.5
  const float radius = 0.5f * (mData.mMinArmAttackRange + mData.mMaxArmAttackRange);
  // 0x26D4 Range(0, 7), 0x26D8 cmpwi 3: > 3 -> 1 (rodata_14), else -1 (rodata_5C).
  float side = mgr.Random()->Range(0, 7) > 3 ? 1.f : -1.f;
  float best = 0.f; // 0x269C rodata_40
  for (int i = 0; i < 2; ++i) { // 0x2A08 cmpwi 2
    side *= -1.f;
    const float dist = mgr.Random()->Range(5.f, 20.f); // rodata_30, rodata_24
    CVector3f cand = GetTranslation() + dist * (side * GetTransform().GetRight());
    cand = playerPos + radius * (cand - playerPos).AsNormalized(); // 0x27B8
    CVector3f dir = cand - GetTranslation();
    float len = dir.Magnitude(); // 0x2814
    dir = dir * (1.f / len);     // 0x2830 rodata_14
    const CAABox box = GetBoundingBox();
    const float height = box.GetMaxPoint().GetZ() - box.GetMinPoint().GetZ(); // 0x2884
    const CVector3f start =
        GetTranslation() + 0.5f * (height * CVector3f(CVector3f::Up())); // 0x28AC rodata_38
    const CRayCastResult result =
        mgr.RayStaticIntersection(start, dir, len, CMaterialFilter()); // 0x28F0
    if (result.IsValid()) { // 0x2904
      float back = result.GetTime() - 5.f; // 0x2920 rodata_30
      if (0.f >= back) {                   // 0x2924 fcmpo rodata_40 + bge
        back = 0.f;
      }
      cand = GetTranslation() + back * dir;
      len = result.GetTime(); // 0x297C
    }
    if (IsPointInSafeZone(mgr, cand)) { // 0x298C
      len = 0.f;
    }
    if (best < len) { // 0x29A0 fcmpo + bge
      CVector3f reachable = cand;
      if (GetSearchPath()->FindClosestReachablePoint(GetTranslation(), reachable) ==
          CPathFindSearch::kR_Success) { // 0x29E0
        best = len;
        SetDestPos(cand);
      } else {
        SetDestPos(GetTranslation());
      }
    }
  }
}

bool CMediumIng::FindSafeZoneRetreatPoint(const CStateManager& mgr, CVector3f& outPoint) {
  const CScriptSafeZone* safeZone =
      TCastToConstPtr< CScriptSafeZone >(mgr.GetObjectById(mSafeZoneId));
  if (!safeZone) { // 0x1424
    return false;
  }
  bool found = false;
  // 0x1438 rodata_10 = 2
  const CVector3f point = safeZone->GetTranslation() + 2.f * CVector3f(CVector3f::Up());
  float height = 0.f;
  const uint flags = GetSearchPath()->GetFlags(); // 0x1498 lwz 0xDC
  GetSearchPath()->SetFlags(3);                   // 0x14B8 stw 3
  if (mPathFindSearch.GetHeightOfPointAboveMesh(point, height, 0.f) ==
      CPathFindSearch::kR_Success) { // 0x14D0
    outPoint = point + height * CVector3f(CVector3f::Down());
    found = true;
  }
  GetSearchPath()->SetFlags(flags);
  return found;
}

static TUniqueId FindClosestConnectedActor(const CEntity& self, const CStateManager& mgr,
                                           const CVector3f& pos, EScriptObjectState state) {
  TUniqueId closest = kInvalidUniqueId;
  float closestDistSq = FLT_MAX; // rodata_50
  rstl::vector< SConnection >::const_iterator it = self.GetConnectionList().begin();
  for (; it != self.GetConnectionList().end(); ++it) {
    if (it->state != state) {
      continue;
    }
    const TUniqueId id = mgr.GetIdForScript(it->objId);
    const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(id));
    if (!actor || !actor->GetActive()) {
      continue;
    }
    const float distSq = (actor->GetTranslation() - pos).MagSquared();
    if (distSq < closestDistSq) {
      closestDistSq = distSq;
      closest = id;
    }
  }
  return closest;
}

TUniqueId CMediumIng::FindClosestRetreatActor(const CStateManager& mgr,
                                              const CVector3f& pos) const {
  return FindClosestConnectedActor(*this, mgr, pos, kSS_Retreat); // 0x1BF0 'RTRT'
}

TUniqueId CMediumIng::FindClosestDangerActor(const CStateManager& mgr, const CVector3f& pos) const {
  return FindClosestConnectedActor(*this, mgr, pos, kSS_DeGenerate); // 0x1D30 'DGNR'
}
#endif
CMediumIng::~CMediumIng() {}
#ifndef TARGET_PC // the real bodies are in PortCMediumIng.cpp
void CMediumIng::Think(float dt, CStateManager& mgr) {}
void CMediumIng::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {}
void CMediumIng::PreRender(CStateManager& mgr) {}
void CMediumIng::Render(const CStateManager& mgr) const {}
void CMediumIng::Touch(CActor& actor, CStateManager& mgr) {}
void CMediumIng::Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) {}
#endif
bool CMediumIng::IsListening() const { return true; }
#ifndef TARGET_PC
bool CMediumIng::Listen(CStateManager& mgr, const CVector3f& pos, EListenNoiseType type) {
  return false;
}
#endif
CMediumIngTentacle::~CMediumIngTentacle() {}
#ifndef TARGET_PC
void CMediumIngTentacle::Think(float dt, CStateManager& mgr) {}
void CMediumIngTentacle::PreRender(CStateManager& mgr) {}
rstl::optional_object< CAABox > CMediumIngTentacle::GetTouchBounds() const {
  return rstl::optional_object< CAABox >();
}
void CMediumIngTentacle::Touch(CActor& actor, CStateManager& mgr) {}
void CMediumIngTentacle::Extend(CStateManager& mgr, const CVector3f& target) {}
void CMediumIngTentacle::Retract(CStateManager& mgr) {}
float CMediumIngTentacle::GetProgress() const { return 0.f; }
void CMediumIngTentacle::UpdateShake(CStateManager& mgr, float dt) {}
void CMediumIng::UpdateCurrentSafeZone(const CStateManager& mgr, const CVector3f& point) {}
void CMediumIng::UpdateMistEffect(CStateManager& mgr) {}
void CMediumIng::CreateMistEffect(CStateManager& mgr, const TLockedToken< CGenDescription >& desc) {
}
void CMediumIng::SetFlagE1() {}
TUniqueId CMediumIng::FindClosestRetreatActor(const CStateManager& mgr,
                                              const CVector3f& pos) const {
  return kInvalidUniqueId;
}
TUniqueId CMediumIng::FindClosestDangerActor(const CStateManager& mgr, const CVector3f& pos) const {
  return kInvalidUniqueId;
}
void CMediumIng::CalculateSeparation(CStateManager& mgr) {}
#endif
