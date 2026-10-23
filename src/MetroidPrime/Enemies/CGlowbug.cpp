#include "MetroidPrime/Enemies/CGlowbug.hpp"

#include "Kyoto/Animation/CCECharacterInfo.hpp"
#include "Kyoto/Animation/CPOINode.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/SObjectTag.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/CParticleDatabase.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Cameras/CCameraShakerManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrGlowbug.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCameraShaker.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "REL/REL_Setup.h"
#include <float.h>
#include <stdio.h>

static const char* const skGlowEffect = "Glow";           // Guessed name
static const char* const skDeathGlowEffect = "DeathGlow"; // Guessed name

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"ShouldAttack", static_cast< CPatterned::StateMachine::TriggerFunc >(&CGlowbug::ShouldAttack)},
    {"AttackFinished",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CGlowbug::AttackFinished)},
    {"AnimOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CGlowbug::AnimOver)},
    {"AttackTelegraphDelay",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CGlowbug::AttackTelegraphDelay)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CGlowbug::Patrol)},
    {"AttackTelegraph",
     static_cast< CPatterned::StateMachine::StateFunc >(&CGlowbug::AttackTelegraph)},
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CGlowbug::Attack)},
    {"Dead", static_cast< CPatterned::StateMachine::StateFunc >(&CGlowbug::Dead)},
};

// @ fn_26_2140
CGlowbug::CGlowbug(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                   const CTransform4f& xf, const CModelData& modelData,
                   const CPatternedInfo& patternedInfo, const CActorParameters& actorParams,
                   CAssetId deathFlashEffect, CAssetId deathBreakApartEffect, CAssetId attackEffect,
                   CAssetId attackEchoEffect, const CVector3f& attackAimOffset,
                   ushort attackTelegraphSound, ushort attackSound, CAssetId attackTelegraphEffect,
                   CAssetId scanModel, bool isInLightWorld, float attackDuration,
                   float attackTelegraphDuration)
: CPatterned(static_cast< EPatternedAI >(0x46), uid, name, static_cast< EFlavorType >(0), info, xf,
             modelData, patternedInfo, kMT_Flyer, kCT_One, kBT_Flyer, actorParams)
, mDeathFlashEffect(deathFlashEffect)
, mDeathBreakApartEffect(deathBreakApartEffect)
, mAttackElectric(attackEffect == kInvalidAssetId
                      ? rstl::auto_ptr< CParticleElectric >()
                      : rstl::auto_ptr< CParticleElectric >(
                            rs_new CParticleElectric(TCachedToken< CElectricDescription >(
                                gpSimplePool->GetObj(SObjectTag('ELSC', attackEffect)), true))))
, mAttackEchoGen(attackEchoEffect == kInvalidAssetId
                     ? rstl::auto_ptr< CElementGen >()
                     : rstl::auto_ptr< CElementGen >(rs_new CElementGen(
                           TCachedToken< CGenDescription >(
                               gpSimplePool->GetObj(SObjectTag('PART', attackEchoEffect)), true),
                           CElementGen::kMOT_Normal, CElementGen::kOSF_One)))
, mEffectIndex(0)
, mHasBrokenApart(false)
, mDeathTimer(0.f)
, mAttackDamage(patternedInfo.GetContactDamage())
, mAttackDuration(attackDuration)
, mAttackPhase(0)
, mAttackTargetId(kInvalidUniqueId)
, mMinAttackRange(patternedInfo.GetMinAttackRange())
, mMaxAttackRange(patternedInfo.GetMaxAttackRange())
, mDamageApplied(false)
, mAttackAimOffset(attackAimOffset)
, mAttackTelegraphDuration(attackTelegraphDuration)
, mAttackSound(attackSound)
, mAttackTelegraphSound(attackTelegraphSound)
, mAttackTelegraphEffect(attackTelegraphEffect)
, mBeamTargetId(kInvalidUniqueId)
, mIsInDarkWorld(!isInLightWorld)
, mCameraShakerId(kInvalidUniqueId)
, mScanModel(scanModel == kInvalidAssetId
                 ? CModelData::None()
                 : CModelData(CStaticRes(scanModel, CVector3f(1.f, 1.f, 1.f))))
, mDeathFlashPlayed(false) {
  rstl::vector< CAssetId > particles;
  particles.reserve(3);
  if (mDeathFlashEffect != kInvalidAssetId) {
    particles.push_back_unsafe(mDeathFlashEffect);
  }
  if (mDeathBreakApartEffect != kInvalidAssetId) {
    particles.push_back_unsafe(mDeathBreakApartEffect);
  }
  if (mAttackTelegraphEffect != kInvalidAssetId) {
    particles.push_back_unsafe(mAttackTelegraphEffect);
  }
  AnimationData()->GetParticleDB().CacheParticleDesc(CCECharacterInfo::CParticleResData(
      particles, rstl::vector< CAssetId >(), rstl::vector< CAssetId >(), rstl::vector< CAssetId >(),
      rstl::vector< CAssetId >(), rstl::vector< CAssetId >()));
}

// @ fn_26_0
CGlowbug::~CGlowbug() {}

// @ fn_26_20D4
void CGlowbug::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

// @ fn_26_1F68
void CGlowbug::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_AreaLoaded: {
    rstl::vector< SConnection >::const_iterator it = GetConnectionList().begin();
    for (; it != GetConnectionList().end(); ++it) {
      if (it->state == static_cast< EScriptObjectState >('APRC')) { // Guessed state
        const TUniqueId id = mgr.GetIdForScript(it->objId);
        if (TCastToPtr< CScriptCameraShaker >(mgr.ObjectById(id))) {
          mCameraShakerId = id;
        }
      }
    }
    mIsInDarkWorld = mgr.GetIsDarkWorld();
    break;
  }
  case kSM_Increment:
  case kSM_Decrement:
  case 'XAUD': // Guessed message
    break;
  case 'XCRT': // Guessed message
    if (!BodyController()->GetIsActive()) {
      BodyController()->SetLocomotionType(pas::kLT_Relaxed);
      BodyController()->Activate(mgr, pas::kAS_Invalid);
    }
    break;
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
}

// @ fn_26_1AE4
void CGlowbug::Think(float dt, CStateManager& mgr) {
  CPatterned::Think(dt, mgr);
  mCurDamageRemTime = rstl::max_val(mCurDamageRemTime - dt, 0.f);

  const float hitFraction = rstl::min_val(1.f, rstl::max_val(0.f, mDamageCooldownTimer) / 0.3f);
  const CColor hitColor = CColor::Lerp(CColor::White(), CColor(0.5f, 0.f, 0.f, 1.f), hitFraction);
  AnimationData()->GetParticleDB().SetModulationColorAllActiveEffects(hitColor);

  FindAttackTarget(mgr);
  if (mAttackTargetId != kInvalidUniqueId) {
    mBeamTargetId = mAttackTargetId;
  }

  if (mAttackEchoGen.get() != nullptr && mAttackPhase == 2 && mAlive) {
    mAttackEchoGen->SetGlobalTranslation(GetTranslation());
    mAttackEchoGen->Update(dt);
  }

  if (mAttackElectric.get() != nullptr && mAttackPhase == 2 && mBeamTargetId != kInvalidUniqueId &&
      mAlive) {
    if (const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mBeamTargetId))) {
      mAttackElectric->SetOverrideIPos(GetTranslation());
      mAttackElectric->SetOverrideFPos(target->GetAimPosition(mgr, 0.f) + mAttackAimOffset);
      mAttackElectric->Update(dt);
      if (!mDamageApplied) {
        mgr.ApplyDamage(
            GetUniqueId(), mBeamTargetId, GetUniqueId(), mAttackDamage,
            CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
            CVector3f::Zero());
        mDamageApplied = true;
        ProcessSoundEvent(mAttackSound, 1.f, 0, 0.1f, 100.f, CSegId(0), 0, 0, 0.f, 20, 127,
                          GetDistanceToCamera(mgr), GetTranslation(), mgr.GetNextAreaId().Value(),
                          mgr, true);
        if (mCameraShakerId != kInvalidUniqueId) {
          if (const CScriptCameraShaker* shaker =
                  TCastToConstPtr< CScriptCameraShaker >(mgr.GetObjectById(mCameraShakerId))) {
            mgr.CameraManager(0)->CameraShakerManager()->AddCameraShaker(shaker->GetShakeData(),
                                                                         mgr, false, false);
          }
        }
      }
    }
  }
}

// @ fn_26_19C0
void CGlowbug::PreRender(CStateManager& mgr) { CPatterned::PreRender(mgr); }

// @ fn_26_19A0
void CGlowbug::AddToRenderer(const CStateManager& mgr) const { CPatterned::AddToRenderer(mgr); }

// @ fn_26_19E0
void CGlowbug::Render(const CStateManager& mgr) const {
  const int alpha = GetRenderAlphaBufferAlpha(mgr);
  if (alpha != -1) {
    gpRender->SetDestinationAlpha(alpha);
  }
  CPatterned::Render(mgr);
  if (mAttackElectric.get() != nullptr && mAttackPhase == 2 && mAlive) {
    mAttackElectric->Render();
  }
  if (mAttackEchoGen.get() != nullptr && mAttackPhase == 2 && mAlive) {
    mAttackEchoGen->Render();
  }
  if (alpha != -1) {
    gpRender->DisableDestinationAlpha();
  }
}

// @ fn_26_151C
void CGlowbug::Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) {
  BodyController()->SetLocomotionType(pas::kLT_Internal9);
  AnimationData()->SetEffectState(rstl::string_l(skGlowEffect), false, mgr);
  if (mIsInDarkWorld) {
    AnimationData()->SetEffectState(rstl::string_l(skDeathGlowEffect), true, mgr);
  }
  AddMaterial(kMT_ProjectilePassthrough, mgr);
  CPatterned::Death(mgr, direction, state);
}

// @ fn_26_F14
bool CGlowbug::IsScanVisorSelfRender() const { return true; }

// @ fn_26_EAC
CAABox CGlowbug::GetScanVisorRenderBounds(const CStateManager& mgr) const {
  if (mScanModel.IsNull()) {
    return CPatterned::GetScanVisorRenderBounds(mgr);
  }
  return mScanModel.GetBounds();
}

// @ fn_26_E50
void CGlowbug::ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                               const CModelFlags& flags) const {
  if (!mScanModel.IsNull()) {
    mScanModel.Render(mgr, xf, nullptr, flags);
  } else {
    CPatterned::ScanVisorRender(mgr, xf, flags);
  }
}

// @ fn_26_10D4
void CGlowbug::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    StopLoopedSounds();
    if (!mIsInDarkWorld) {
      RemoveMaterial(kMT_Character, kMT_Solid, kMT_Target, kMT_Orbit, mgr);
    } else {
      RemoveMaterial(kMT_Character, kMT_Target, kMT_Orbit, mgr);
    }
    break;
  case kStateMsg_Update:
    if (!mDeathFlashPlayed) {
      char name[100];
      sprintf(name, "GLOWBUG_EFFECT%d-%d", mDeathFlashEffect, mEffectIndex++);
      AnimationData()->GetParticleDB().AddParticleEffect(
          CPOINode::GetHashForString(name), 0,
          CParticleData(0, SObjectTag('PART', mDeathFlashEffect), CSegId(1), 1.f,
                        CParticleData::kPM_Initial),
          GetModelData()->GetScale(), &mgr, GetCurrentAreaId(), false, 0);
      mDeathFlashPlayed = true;
    }
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_Die));
    if (mIsInDarkWorld) {
      if (IsOnGround() && !mHasBrokenApart) {
        mHasBrokenApart = true;
        RemoveMaterial(kMT_Solid, mgr);
        AnimationData()->SetEffectState(rstl::string_l(skDeathGlowEffect), false, mgr);
        char name[100];
        sprintf(name, "GLOWBUG_EFFECT%d-%d", mDeathBreakApartEffect, mEffectIndex++);
        AnimationData()->GetParticleDB().AddParticleEffect(
            CPOINode::GetHashForString(name), 0,
            CParticleData(0, SObjectTag('PART', mDeathBreakApartEffect), CSegId(1), 1.f,
                          CParticleData::kPM_Initial),
            GetModelData()->GetScale(), &mgr, GetCurrentAreaId(), false, 0);
        SendScriptMsgs(static_cast< EScriptObjectState >('DGNR'), mgr, kSM_None); // Guessed state
      } else if (mHasBrokenApart) {
        mDeathTimer += dt;
        if (mDeathTimer > 1.f) {
          DeathDelete(mgr);
        }
      }
    } else {
      mDeathTimer += dt;
      if (mDeathTimer > 1.f) {
        DeathDelete(mgr);
      }
    }
    break;
  }
}

// @ fn_26_16FC
void CGlowbug::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAttackPhase = 2;
    mDamageApplied = false;
    break;
  case kStateMsg_Update:
    break;
  case kStateMsg_Deactivate:
    mCurDamageRemTime = mDamageWaitTime;
    break;
  }
}

// @ fn_26_1740
void CGlowbug::AttackTelegraph(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    char name[100];
    sprintf(name, "GLOWBUG_EFFECT%d-%d", mAttackTelegraphEffect, mEffectIndex++);
    AnimationData()->GetParticleDB().AddParticleEffect(
        CPOINode::GetHashForString(name), 0,
        CParticleData(0, SObjectTag('PART', mAttackTelegraphEffect), CSegId(1), 1.f,
                      CParticleData::kPM_ContinuousSystem),
        GetModelData()->GetScale(), &mgr, GetCurrentAreaId(), false, 0);
    ProcessSoundEvent(mAttackTelegraphSound, 1.f, 0, 0.1f, 100.f, CSegId(0), 0, 0, 0.f, 20, 127,
                      GetDistanceToCamera(mgr), GetTranslation(), mgr.GetNextAreaId().Value(), mgr,
                      true);
    mAttackPhase = 1;
    break;
  }
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

// @ fn_26_18F8
void CGlowbug::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    AnimationData()->SetEffectState(rstl::string_l(skGlowEffect), true, mgr);
    mAttackPhase = 0;
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
  CPatterned::Patrol(mgr, msg, dt);
}

// @ fn_26_160C
bool CGlowbug::AttackTelegraphDelay(CStateManager& mgr, const CTriggerData& data) const {
  return mStateMachine->GetTime() > mAttackTelegraphDuration;
}

// @ fn_26_1658
bool CGlowbug::AttackFinished(CStateManager& mgr, const CTriggerData& data) const {
  return mStateMachine->GetTime() > mAttackDuration;
}

// @ fn_26_16A4
bool CGlowbug::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mAttackTargetId != kInvalidUniqueId && mCurDamageRemTime == 0.f;
}

bool CGlowbug::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::AnimOver(mgr, data);
}

// @ fn_26_F7C
void CGlowbug::FindAttackTarget(CStateManager& mgr) {
  mAttackTargetId = kInvalidUniqueId;
  float bestScore = FLT_MAX;
  CVector3f forward = GetTransform().GetColumn(kDY);
  const float angleWeight = mMaxAttackRange * mMaxAttackRange / M_PIF;
  for (int i = 0; i < mgr.GetNumPlayers(); ++i) {
    CPlayer* player = mgr.GetPlayer(i);
    if (IsInRange(*player, mMaxAttackRange) && !IsInRange(*player, mMinAttackRange)) {
      CVector3f toPlayer = player->GetTranslation() - GetTranslation();
      const float angle = CVector3f::GetAngleDiff(toPlayer, forward);
      const float score = angle * angleWeight + toPlayer.MagSquared();
      if (score < bestScore) {
        bestScore = score;
        mAttackTargetId = player->GetUniqueId();
      }
    }
  }
}

// @ fn_26_F1C
bool CGlowbug::IsInRange(const CActor& other, float range) const {
  return (other.GetTranslation() - GetTranslation()).MagSquared() < range * range;
}

// @ fn_26_4C8
CEntity* LoadGlowbug(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrGlowbug sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrGlowbug.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  return rs_new CGlowbug(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      *modelData, LdrToPatternedInfo(sldrThis.patterned, nullptr),
      LdrToActorParameters(sldrThis.actorInformation), sldrThis.deathFlashEffect,
      sldrThis.deathBreakApartEffect, sldrThis.attackEffect, sldrThis.attackEchoEffect,
      sldrThis.attackAimOffset, sldrThis.attackTelegraphSound, sldrThis.attackSound,
      sldrThis.attackTelegraphEffect, sldrThis.scanModel, sldrThis.isInLightWorld,
      sldrThis.attackDuration, sldrThis.attackTelegraphDuration);
}

#ifndef MONOLITHIC
// @ fn_26_498
static void SetFuncPtrs() {
  static SGlowbug_FuncPtrs funcPtrs;
  funcPtrs.mLoader = &LoadGlowbug;
  SetSGlowbug_FuncPtrs(&funcPtrs);
}

// @ fn_26_478
extern "C" void RELMain() { SetFuncPtrs(); }

// @ fn_26_454
extern "C" void RELExit() { SetSGlowbug_FuncPtrs(nullptr); }
#endif
