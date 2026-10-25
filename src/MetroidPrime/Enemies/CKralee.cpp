#include "MetroidPrime/Enemies/CKralee.hpp"

#include "Kyoto/Animation/CCECharacterInfo.hpp"
#include "Kyoto/Animation/CPOINode.hpp"
#include "Kyoto/Audio/CAudioSys.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CloseEnough.hpp"
#include "Kyoto/Particles/CParticleData.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CParticleDatabase.hpp"
#include "MetroidPrime/CPositionalParticleData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrKralee.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "REL/REL_Setup.h"
#include <stdio.h>

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"ShouldPatrol", static_cast< CPatterned::StateMachine::TriggerFunc >(&CKralee::ShouldPatrol)},
    {"ShouldWarpOut",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CKralee::ShouldWarpOut)},
    {"AnimOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CKralee::AnimOver)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CKralee::Patrol)},
    {"WarpOut", static_cast< CPatterned::StateMachine::StateFunc >(&CKralee::WarpOut)},
};

static CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    {"WarpOutExit", static_cast< CPatterned::StateMachine::CodeFunc >(&CKralee::WarpOutExit)},
};

// @ fn_37_244C
CKralee::CKralee(TUniqueId uid, const rstl::string& name, CEntityInfo& info, const CTransform4f& xf,
                 const CModelData& modelData, const CPatternedInfo& patternedInfo,
                 const CActorParameters& actorParams, float stickyReach, float floorTurnSpeed,
                 float waypointApproachDistance, float visibleDistance,
                 float projectileBoundsMultiplier, float collisionLookAhead, float warpInTime,
                 float warpOutTime, bool initiallyPaused, float warpAttackRadius,
                 float warpAttackKnockback, float warpAttackDamage, float animSpeedScalar,
                 float maxAudibleDistance, CAssetId warpInParticleEffect,
                 CAssetId warpOutParticleEffect, ushort warpInSound, ushort warpOutSound,
                 bool initiallyInvisible, float visibleTime, float visibleTimeRandomOffset,
                 float invisibleTime, float invisibleTimeRandomOffset)
: CWallCrawler(static_cast< EPatternedAI >(0x45), uid, name, kFT_Zero, info, xf, modelData,
               patternedInfo, kMT_Ground, kCT_Zero, kBT_WallWalker, actorParams,
               patternedInfo.GetHalfExtent() * modelData.GetScale().GetX(), stickyReach,
               floorTurnSpeed, waypointApproachDistance, visibleDistance,
               static_cast< CWallCrawler::EType >(6), initiallyPaused, projectileBoundsMultiplier,
               0.167f, 0.6f, 1.5f, 0.6f, 1.5f)
, mWarpState(initiallyInvisible ? kWS_Invisible : kWS_Visible)
, mCollisionLookAhead(collisionLookAhead)
, mCurrentWaypointId(kInvalidUniqueId)
, mWarpInTime(warpInTime)
, mWarpOutTime(warpOutTime)
, mWarpAttackDamage(patternedInfo.GetContactDamage().GetWeaponMode(), warpAttackDamage,
                    warpAttackRadius, warpAttackKnockback, false, false)
, mAnimSpeedScalar(animSpeedScalar)
, mMaxAudibleDistance(maxAudibleDistance)
, mWarpStateTimer(0.f)
, mWarpInParticleEffect(warpInParticleEffect)
, mWarpOutParticleEffect(warpOutParticleEffect)
, mWarpInSound(warpInSound)
, mWarpOutSound(warpOutSound)
, mEffectIndex(0)
, mDamageVulnerability(patternedInfo.GetDamageVulnerability())
, mWantsWarpOut(false)
, mVisibleTime(visibleTime)
, mVisibleTimeRandomOffset(visibleTimeRandomOffset)
, mInvisibleTime(invisibleTime)
, mInvisibleTimeRandomOffset(invisibleTimeRandomOffset)
, mStateElapsedTime(0.f)
, mVisibleDuration(0.f)
, mInvisibleDuration(0.f)
, mWarpEffectTransform(CTransform4f::Identity()) {
  mAlignToFloor = true;
  SetDrawShadow(false);

  rstl::vector< CAssetId > particles;
  particles.reserve(2);
  if (mWarpInParticleEffect != kInvalidAssetId) {
    particles.push_back_unsafe(mWarpInParticleEffect);
  }
  if (mWarpOutParticleEffect != kInvalidAssetId) {
    particles.push_back_unsafe(mWarpOutParticleEffect);
  }
  AnimationData()->GetParticleDB().CacheParticleDesc(CCECharacterInfo::CParticleResData(
      particles, rstl::vector< CAssetId >(), rstl::vector< CAssetId >(), rstl::vector< CAssetId >(),
      rstl::vector< CAssetId >(), rstl::vector< CAssetId >()));
  mSpeed = mAnimSpeedScalar;
}

// @ fn_37_23DC
CKralee::~CKralee() {}

// @ fn_37_1D24
void CKralee::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  stateMachine->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
}

// @ fn_37_1BA0
void CKralee::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case 'XCRT': // Guessed message
    if (!BodyController()->GetIsActive()) {
#ifdef TARGET_PC
      // fn_37_1BA0: 0x1C40 li r4,0x7 / 0x1C44 bl SetLocomotionType (Patrol's 0x2120 is li r4,0x1).
      BodyController()->SetLocomotionType(pas::kLT_Internal7);
#else
      BodyController()->SetLocomotionType(pas::kLT_Relaxed);
#endif
      BodyController()->Activate(mgr, pas::kAS_Invalid);
    }
    if (mWarpState == kWS_Invisible) {
      mColor.SetAlpha(0.f);
    }
    if (mVisibleTime > 0.f) {
      mVisibleDuration = mVisibleTime + mVisibleTimeRandomOffset * mgr.Random()->Float();
    }
    if (mInvisibleTime > 0.f) {
      mInvisibleDuration = mInvisibleTimeRandomOffset * mgr.Random()->Float() + mInvisibleTime;
    }
    break;
  case kSM_Increment:
    WarpIn(mgr);
    break;
  case kSM_Decrement:
    if (mWarpState == kWS_Visible) {
      mWantsWarpOut = true;
    }
    break;
  }
  CWallCrawler::AcceptScriptMsg(mgr, msg);
}

// @ fn_37_13C8
void CKralee::Think(float dt, CStateManager& mgr) {
  static float skLookAheadTime = 0.02f;
  if (!GetActive()) {
    return;
  }

  ++mThinkCounter;
  mPlayerObstructed = false;
  const CGameArea& area = mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId());
  if (area.GetOcclusionState() != CGameArea::kOS_Visible) {
    mPlayerObstructed = true;
  }
  if (mPlayerObstructed) {
    SetMovable(false);
    return;
  }

  SetMovable(!mAlignToFloor);
  CWallCrawler::Think(dt, mgr);
  if (!mDisableMove && close_enough(mBodyController->GetPercentageFrozen(), 0.f) && mAlignToFloor) {
    AlignToFloor(mgr, mColSphere.GetSphere().GetRadius(),
                 GetTranslation() + GetVelocityWR() * skLookAheadTime, dt);
  }

  float alpha = 1.f;
  if (mWarpState == kWS_WarpIn || mWarpState == kWS_WarpOut) {
    if (mWarpState == kWS_WarpIn) {
      alpha = 0.f;
      if (mWarpStateTimer > mWarpInTime) {
        mWarpState = kWS_Visible;
        mWarpStateTimer = 0.f;
        alpha = 1.f;
      }
    } else if (mWarpState == kWS_WarpOut) {
      const float timeRatio = mWarpStateTimer / mWarpOutTime;
      alpha = 1.f - CMath::Min(1.f, timeRatio);
      if (mWarpStateTimer > mWarpOutTime) {
        mWarpState = kWS_Invisible;
        mWarpStateTimer = 0.f;
      }
    }

    const float radiusSq = mWarpAttackDamage.GetRadius() * mWarpAttackDamage.GetRadius();
    for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
      CPlayer* player = mgr.Player(i);
      const float dz = player->GetTranslation().GetZ() - GetTranslation().GetZ();
      const float dy = player->GetTranslation().GetY() - GetTranslation().GetY();
      const float dx = player->GetTranslation().GetX() - GetTranslation().GetX();
      if (dz * dz + dx * dx + dy * dy < radiusSq) {
        mgr.ApplyDamage(
            GetUniqueId(), player->GetUniqueId(), GetUniqueId(), mWarpAttackDamage,
            CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
            CVector3f::Zero());
        mCurDamageRemTime = mDamageWaitTime;
      }
    }
    mWarpStateTimer += dt;
    SetVolume(CAudioSys::kMaxVolume);
  }

  if (mWarpState == kWS_Invisible) {
    alpha = 0.f;
    for (int i = 0; i < mgr.GetNumPlayers(); ++i) {
      if (mgr.GetPlayerState(i)->GetActiveVisor(mgr) == CPlayerState::kPV_Dark) {
        SetValidTarget(i, true);
      } else {
        SetValidTarget(i, false);
      }
    }
    if (mInvisibleTime > 0.f) {
      if (mStateElapsedTime >= mInvisibleDuration) {
        WarpIn(mgr);
        mStateElapsedTime = 0.f;
        mInvisibleDuration = mInvisibleTime + mInvisibleTimeRandomOffset * mgr.Random()->Float();
      }
      mStateElapsedTime += dt;
    }
    SetVolume(0);
  } else if (mWarpState == kWS_Visible) {
    alpha = 1.f;
    for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
      SetValidTarget(i, true);
    }
    if (mVisibleTime > 0.f) {
      if (mStateElapsedTime >= mVisibleDuration) {
        mWantsWarpOut = true;
        mStateElapsedTime = 0.f;
        mVisibleDuration = mVisibleTime + mVisibleTimeRandomOffset * mgr.Random()->Float();
      }
      mStateElapsedTime += dt;
    }
    SetVolume(CAudioSys::kMaxVolume);
  }

  if (mWarpState == kWS_Invisible) {
    *DamageVulnerability() = CDamageVulnerability::PassThroughVulnerabilty();
    SetupPlayerCollision(false);
    AddMaterial(kMT_ExcludeFromRadar, mgr);
  } else {
    *DamageVulnerability() = mDamageVulnerability;
    SetupPlayerCollision(true);
    RemoveMaterial(kMT_ExcludeFromRadar, mgr);
  }

  if (mAlive) {
    mColor.SetAlpha(alpha);
    if (mWarpState == kWS_Invisible) {
      AnimationData()->GetParticleDB().SetModulationColorAllActiveEffects(
          CColor(1.f, 1.f, 1.f, 0.f));
    } else {
      AnimationData()->GetParticleDB().SetModulationColorAllActiveEffects(
          CColor(1.f, 1.f, 1.f, 1.f));
    }
    SetEnableRender(mWarpState != kWS_WarpOut && mWarpState != kWS_Invisible);
  } else {
    SetEnableRender(true);
  }
}

// @ fn_37_1080
void CKralee::PreRender(CStateManager& mgr) {
  if (mAlive) {
    if ((mWarpState == kWS_WarpOut || mWarpState == kWS_Invisible) &&
        mgr.GetPlayerState()->GetActiveVisor(mgr) == CPlayerState::kPV_Dark) {
      SetEnableRender(true);
      mColor.SetAlpha(1.f);
      AnimationData()->GetParticleDB().SetModulationColorAllActiveEffects(
          CColor(1.f, 1.f, 1.f, 1.f));
    }
  } else {
    SetEnableRender(true);
  }
  CPatterned::PreRender(mgr);
}

// @ fn_37_1018
void CKralee::AddToRenderer(const CStateManager& mgr) const {
  if (mAlive && mWarpState == kWS_Invisible &&
      mgr.GetPlayerState()->GetActiveVisor(mgr) == CPlayerState::kPV_Echo) {
    return;
  }
  CPatterned::AddToRenderer(mgr);
}

// @ fn_37_13A8
void CKralee::Render(const CStateManager& mgr) const { CWallCrawler::Render(mgr); }

// @ fn_37_117C
void CKralee::WarpIn(CStateManager& mgr) {
  if (mWarpState != kWS_Invisible) {
    return;
  }

  mWarpState = kWS_WarpIn;
  char name[100];
  sprintf(name, "KRALEE_EFFECT%d-%d", mWarpInParticleEffect, mEffectIndex++);
  const CSegId locator = AnimationData()->GetLocatorSegId(rstl::string_l("warp_effect_LCTR"));
  AnimationData()->GetParticleDB().AddParticleEffect(
      CPOINode::GetHashForString(name), 0x4040,
      CParticleData(0, SObjectTag('PART', mWarpInParticleEffect),
                    locator == CSegId::Invalid() ? CSegId(0) : locator, 1.f,
                    CParticleData::kPM_ContinuousSystem),
      GetModelData()->GetScale(), &mgr, GetCurrentAreaId(), false, 0);
  ProcessSoundEvent(mWarpInSound, 1.f, 0, 0.1f, mMaxAudibleDistance, CSegId(0), 0, 0, 0.f, 20, 127,
                    GetDistanceToCamera(mgr), GetTranslation(), mgr.GetNextAreaId().Value(), mgr,
                    true);
}

// @ fn_37_20DC
void CKralee::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mAlignToFloor = true;
    SetMomentumWR(CVector3f::Zero());
    mHasAlignSurface = false;
    SetMovable(false);
    TUniqueId connected = kInvalidUniqueId;
    const TUniqueId* waypoint = &mCurrentWaypointId;
    if (mCurrentWaypointId == kInvalidUniqueId) {
      connected = GetConnectedObject(mgr, kSS_Patrol, kSM_Follow);
      waypoint = &connected;
    }
    if (*waypoint != kInvalidUniqueId) {
      mDestObj = *waypoint;
    }
    break;
  }
  case kStateMsg_Update: {
    UpdateWPDestination(mgr);
    const CVector3f up = GetTransform().GetUp();
    CVector3f toDest = mDestPos - GetTranslation();
    toDest.Normalize();
    BodyController()->CommandMgr().DeliverCmd(
        CBCLocomotionCmd(ProjectVectorToPlane(toDest, up), CVector3f::Zero(), 0.f));
    const CVector3f seek = ProjectVectorToPlane(mSteeringBehaviors.Seek(*this, mDestPos), up);
    BodyController()->CommandMgr().DeliverCmd(
        CBCLocomotionCmd(ProjectVectorToPlane(1.f * seek, up), CVector3f::Zero(), 1.f));
    BodyController()->CommandMgr().DeliverCmd(
        CBCLocomotionCmd(GetTransform().GetForward(), CVector3f::Zero(), 0.f));
    break;
  }
  case kStateMsg_Deactivate:
    mCurrentWaypointId = mDestObj;
    break;
  }
}

// @ fn_37_1DB8
void CKralee::WarpOut(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mWantsWarpOut = false;
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::EGenerateType(15), -1));
    mWarpEffectTransform =
        GetTransform() * GetScaledLocatorTransform(rstl::string_l("warp_effect_LCTR"));
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::EGenerateType(15), -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mWarpState = kWS_WarpOut;
    mPatrolPauseRemTime = mWarpOutTime;
    break;
  }
}

// @ fn_37_E18
void CKralee::WarpOutExit(CStateManager& mgr, float dt) {
  char name[100];
  sprintf(name, "KRALEE_EFFECT%d-%d", mWarpOutParticleEffect, mEffectIndex++);
  const CSegId locator = AnimationData()->GetLocatorSegId(rstl::string_l("warp_effect_LCTR"));
  AnimationData()->GetParticleDB().AddParticleEffect(
      CPOINode::GetHashForString(name), 0x4040,
      CPositionalParticleData(0, SObjectTag('PART', mWarpOutParticleEffect), mWarpEffectTransform,
                              1.f),
      GetModelData()->GetScale(), &mgr, GetCurrentAreaId(), 0);
  ProcessSoundEvent(mWarpOutSound, 1.f, 0, 0.1f, mMaxAudibleDistance, CSegId(0), 0, 0, 0.f, 20, 127,
                    GetDistanceToCamera(mgr), GetTranslation(), mgr.GetNextAreaId().Value(), mgr,
                    true);
}

// @ fn_37_1DB0
bool CKralee::ShouldPatrol(CStateManager& mgr, const CTriggerData& data) const { return true; }

// @ fn_37_13A0
bool CKralee::ShouldWarpOut(CStateManager& mgr, const CTriggerData& data) const {
  return mWantsWarpOut;
}

// @ fn_37_1378
bool CKralee::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::AnimOver(mgr, data);
}

// @ fn_37_13C
CEntity* LoadKralee(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrKralee sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrKralee.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  return rs_new CKralee(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      *modelData, LdrToPatternedInfo(sldrThis.patterned, nullptr),
      LdrToActorParameters(sldrThis.actorInformation), sldrThis.stickyReach,
      sldrThis.floorTurnSpeed, sldrThis.waypointApproachDistance, sldrThis.visibleDistance,
      sldrThis.projectileBoundsMultiplier, sldrThis.collisionLookAhead, sldrThis.warpInTime,
      sldrThis.warpOutTime, sldrThis.initiallyPaused, sldrThis.warpAttackRadius,
      sldrThis.warpAttackKnockback, sldrThis.warpAttackDamage, sldrThis.animSpeedScalar,
      sldrThis.maxAudibleDistance, sldrThis.warpInParticleEffect, sldrThis.warpOutParticleEffect,
      sldrThis.warpInSound, sldrThis.warpOutSound, sldrThis.initiallyInvisible,
      sldrThis.visibleTime, sldrThis.visibleTimeRandomOffset, sldrThis.invisibleTime,
      sldrThis.invisibleTimeRandomOffset);
}

#ifndef MONOLITHIC
// @ fn_37_10C
static void SetFuncPtrs() {
  static SKralee_FuncPtrs funcPtrs;
  funcPtrs.mLoadKralee = &LoadKralee;
  SetSKralee_FuncPtrs(&funcPtrs);
}

// @ fn_37_EC
extern "C" void RELMain() { SetFuncPtrs(); }

// @ fn_37_C8
extern "C" void RELExit() { SetSKralee_FuncPtrs(nullptr); }
#endif
