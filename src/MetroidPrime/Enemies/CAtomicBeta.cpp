#include "MetroidPrime/Enemies/CAtomicBeta.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CloseEnough.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrAtomicBeta.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CElectricBeamProjectile.hpp"
#include "REL/REL_Setup.h"

#include "rstl/math.hpp"

static const char* skBeamLocators[] = {
    "bomb2_LCTR",
    "bomb3_LCTR",
    "bomb4_LCTR",
};

// Retail `__sinit_CAtomicBeta_cpp` (0x2458-0x24A0) writes exactly this table and nothing else:
// `lis` 0x2468 takes the table address, then 6 word copies (`stw` 0x2484-0x2498) put each entry's
// member pointer at +4/+8/+0xC as {adjust 0, vtoff -1 (non-virtual), fn = address}. Values read
// from the REL's module-5 DOL relocation list: names .rodata 0x44 "Patrol" and 0x4B "Dead", both
// CPatterned from the DOL (0x80152924 Patrol, 0x80152820 Dead) - the same names, order and targets
// as below; the name words are static relocations already in the file, not sinit writes. No call,
// no branch, no other store (skBeamLocators is a constant initialiser in .data, not written).
// @ fn_5_2458
static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CPatterned::Patrol)},
    {"Dead", static_cast< CPatterned::StateMachine::StateFunc >(&CPatterned::Dead)},
};

CAtomicBeta::CAtomicBeta(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                         const CTransform4f& xf, const CModelData& modelData,
                         const CActorParameters& actorParams, const CPatternedInfo& patternedInfo,
                         CAssetId electricId, CAssetId weaponId, const CDamageInfo& beamDamage,
                         CAssetId particleId, float beamFadeSpeed, float beamRadius,
                         float beamDamageInterval, const CDamageVulnerability& frozenVulnerability,
                         float moveSpeed, float minSpeed, float maxSpeed, ushort flySound,
                         ushort chargedFlySound, ushort electricitySound, float speedStep)
: CPatterned(kPAI_AtomicBeta, uid, name, kFT_Zero, info, xf, modelData, patternedInfo, kMT_Flyer,
             kCT_One, kBT_Floater, actorParams)
, mBeamIds()
, mBeamFired(false)
, mMinSpeed(minSpeed)
, mMaxSpeed(maxSpeed)
, mSpeedStep(speedStep)
, mCurrentSpeed(mMinSpeed)
, mFrozenVulnerability(frozenVulnerability)
, mMoveSpeed(moveSpeed)
, mDirection(xf.GetForward())
, mElectricDescription(gpSimplePool->GetObj(SObjectTag('ELSC', electricId)))
, mWeaponDescription(gpSimplePool->GetObj(SObjectTag('WPSC', weaponId)))
, mBeamDamage(beamDamage)
, mBeamParticle(particleId)
, mBeamFadeSpeed(beamFadeSpeed)
, mBeamRadius(beamRadius)
, mBeamDamageInterval(beamDamageInterval)
, mStaticInterferenceStrength(1.f)
, mStaticInterferenceRadius(10.f)
, mFlySound(flySound)
, mChargedFlySound(chargedFlySound)
, mElectricitySound(electricitySound)
, mFlySoundHandle()
, mChargedFlySoundHandle()
, mElectricitySoundHandle()
, mTouchRadius(patternedInfo.GetHalfExtent() * GetModelData()->GetScale().GetX()) {
  KnockBackController().EnableKnockBackPhysics(false);
  KnockBackController().SetHurlVelocityEnabled(false);
}

// @ fn_5_1BE0
CAtomicBeta::~CAtomicBeta() {
  StopLoopedSound(mChargedFlySoundHandle);
  StopLoopedSound(mElectricitySoundHandle);
  StopLoopedSound(mFlySoundHandle);
}

// @ fn_5_1B00
void CAtomicBeta::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_Create:
    BodyController()->Activate(mgr, pas::kAS_Invalid);
    CreateBeams(mgr);
    break;
  case kSM_Deactivate:
    UpdateBeams(mgr, false);
    StopLoopedSound(mChargedFlySoundHandle);
    StopLoopedSound(mElectricitySoundHandle);
    StopLoopedSound(mFlySoundHandle);
    break;
  case kSM_Delete:
    DestroyBeams(mgr);
    break;
  }

  CPatterned::AcceptScriptMsg(mgr, msg);
}

// @ fn_5_1AB4
void CAtomicBeta::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

// @ fn_5_1890
void CAtomicBeta::CreateBeams(CStateManager& mgr) {
  const CElectricBeamInfo beamInfo(mElectricDescription, 50.f, mBeamRadius, 10.f, mBeamParticle,
                                   mBeamFadeSpeed, mBeamDamageInterval);

  for (int i = 0; i < ARRAY_SIZE(skBeamLocators); ++i) {
    const TUniqueId beamId = mgr.AllocateUniqueId();
    mBeamIds.push_back(beamId);

    mgr.AddObject(rs_new CElectricBeamProjectile(
        mWeaponDescription, kWT_AI, beamInfo, CTransform4f::Identity(), kMT_Character, mBeamDamage,
        beamId, GetCurrentAreaId(), GetUniqueId(), CWeapon::kPA_None));
  }
}

// @ fn_5_16D4
void CAtomicBeta::UpdateBeams(CStateManager& mgr, bool fire) {
  if (mBeamFired == fire) {
    return;
  }

  for (int i = 0; i < ARRAY_SIZE(skBeamLocators); ++i) {
    const CTransform4f locatorXf = GetTransform() * GetScaledLocatorTransform(skBeamLocators[i]);
    const CTransform4f lookXf = CTransform4f::LookAt(
        locatorXf.GetTranslation(), locatorXf.GetTranslation() + locatorXf.GetForward());

    if (CElectricBeamProjectile* beam =
            static_cast< CElectricBeamProjectile* >(mgr.ObjectById(mBeamIds[i]))) {
      if (fire) {
        beam->Fire(GetTransform() * GetScaledLocatorTransform(skBeamLocators[i]), mgr, false);
      } else {
        beam->ResetBeam(mgr, false);
      }
    }
  }

  mBeamFired = fire;
}

void CAtomicBeta::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);

  const float damageLerp = rstl::min_val(1.f, mDamageCooldownTimer / skDamageHitTime);
  if (damageLerp > 0.f) {
    CModelFlags flags = GetModelFlags();
    if (flags.GetTransSigned() == CModelFlags::kT_Two) {
      SetModelFlags(CModelFlags(flags, CModelFlags::kT_Blend,
                                CColor::Lerp(CColor::White(), skDamageColor, damageLerp)));
    }
  }
}

// @ fn_5_155C
void CAtomicBeta::DestroyBeams(CStateManager& mgr) {
  for (int i = 0; i < mBeamIds.size(); ++i) {
    mgr.DeleteObjectRequest(mBeamIds[i]);
  }

  mBeamIds.clear();
}

// @ fn_5_10AC
void CAtomicBeta::Think(float dt, CStateManager& mgr) {
  CPatterned::Think(dt, mgr);

  const CVector3f moveVec = BodyController()->CommandMgr().GetMoveVector();
  BodyController()->CommandMgr().ClearLocomotionCmds();
  if (moveVec.IsNonZero()) {
    BodyController()->CommandMgr().DeliverCmd(CBCLocomotionCmd(moveVec, mDirection, 1.f));
  }

  for (int i = 0; i < mgr.GetNumPlayers(); ++i) {
    CPlayer* player = mgr.GetPlayer(i);
    const CVector3f diff = player->GetTranslation() - GetTranslation();
    const float interference =
        mStaticInterferenceStrength *
        rstl::max_val(
            1.f - diff.MagSquared() / (mStaticInterferenceRadius * mStaticInterferenceRadius), 0.f);
    if (!close_enough(interference, 0.f)) {
      player->GetPlayerState()->StaticInterference().AddSource(GetUniqueId(), interference, 0.5f);
    }
  }

  if (InMaxRange(mgr, CTriggerData(0.f))) {
    UpdateBeams(mgr, true);
    PlayLoopedSound(mChargedFlySoundHandle, mChargedFlySound, GetTranslation(), 96);
    PlayLoopedSound(mElectricitySoundHandle, mElectricitySound, GetTranslation(), 96);
    StopLoopedSound(mFlySoundHandle);
  } else {
    UpdateBeams(mgr, false);
    StopLoopedSound(mChargedFlySoundHandle);
    StopLoopedSound(mElectricitySoundHandle);
    PlayLoopedSound(mFlySoundHandle, mFlySound, GetTranslation(), 96);
  }

  for (int i = 0; i < ARRAY_SIZE(skBeamLocators); ++i) {
    CElectricBeamProjectile* beam =
        static_cast< CElectricBeamProjectile* >(mgr.ObjectById(mBeamIds[i]));
    if (beam && beam->GetActive()) {
      const CTransform4f locatorXf = GetTransform() * GetScaledLocatorTransform(skBeamLocators[i]);
      const CTransform4f lookXf = CTransform4f::LookAt(
          locatorXf.GetTranslation(), locatorXf.GetTranslation() + locatorXf.GetForward());
      beam->UpdateFx(lookXf, dt, mgr);
    }
  }

  mCurrentSpeed = CMath::Clamp(
      mMinSpeed, mSpeedStep * (dt * (IsCharging(mgr) ? 1.f : -1.f)) + mCurrentSpeed, mMaxSpeed);
  mSpeed = mCurrentSpeed;
  BodyController()->SetRestrictedFlyerMoveSpeed(mMoveSpeed * mCurrentSpeed);
}

void CAtomicBeta::PlayLoopedSound(CSfxHandle& handle, ushort sfxId, const CVector3f position,
                                  uchar volume) const {
  if (!handle) {
    handle = CSfxManager::AddEmitter(sfxId, position, volume, GetCurrentAreaId().Value(), true,
                                     true, CSfxManager::kMedPriority);
  } else {
    CSfxManager::UpdateEmitter(handle, position, CVector3f::Zero(), volume);
  }
}

// @ fn_5_FD0
void CAtomicBeta::StopLoopedSound(CSfxHandle& handle) const {
  if (handle) {
    CSfxManager::RemoveEmitter(handle);
    handle = CSfxHandle();
  }
}

// @ fn_5_F8C
bool CAtomicBeta::IsCharging(const CStateManager& mgr) {
  for (int i = 0; i < mgr.GetNumPlayers(); ++i) {
    if (mgr.GetPlayer(i)->GetPlayerState()->GetChargeBeamFactor() > 0.1f) {
      return true;
    }
  }
  return false;
}

// @ fn_5_F28
const CDamageVulnerability* CAtomicBeta::GetDamageVulnerability() const {
  if (close_enough(GetBodyController()->GetPercentageFrozen(), 0.f)) {
    return CPatterned::GetDamageVulnerability();
  }
  return &mFrozenVulnerability;
}

// @ fn_5_E58
rstl::optional_object< CAABox > CAtomicBeta::GetTouchBounds() const {
  const CVector3f extent = mTouchRadius * CVector3f::One();
  return rstl::optional_object< CAABox >(
      CAABox(GetTranslation() - extent, GetTranslation() + extent));
}

void CAtomicBeta::Touch(CActor& other, CStateManager& mgr) {
  if (GetAlive()) {
    const CGameProjectile* projectile = TCastToConstPtr< CGameProjectile >(other);
    if (projectile && TCastToConstPtr< CPlayer >(mgr.GetObjectById(projectile->GetOwnerId())) &&
        (projectile->GetAttribField() & CWeapon::kPA_Annihilator) == CWeapon::kPA_Annihilator &&
        GetBodyController()->GetPercentageFrozen() == 0.f) {
      const CKnockBackInfo info(CVector3f::Forward(), projectile->GetUniqueId(),
                                projectile->GetOwnerId(), projectile->GetCurrentDamageInfo(), true);
      KnockBack(mgr, info);
    }
  }
  CPatterned::Touch(other, mgr);
}

// @ fn_5_CF0
EWeaponCollisionResponseTypes CAtomicBeta::GetCollisionResponseType(const CVector3f& position,
                                                                    const CVector3f& direction,
                                                                    const CWeaponMode& mode,
                                                                    int attributes) const {
  return GetDamageVulnerability()->WeaponHurts(mode)
             ? kWCR_AtomicBeta
             : static_cast< EWeaponCollisionResponseTypes >(0x66);
}

// @ fn_5_C64
void CAtomicBeta::Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) {
  UpdateBeams(mgr, false);
  StopLoopedSound(mChargedFlySoundHandle);
  StopLoopedSound(mElectricitySoundHandle);
  StopLoopedSound(mFlySoundHandle);
  CPatterned::Death(mgr, direction, state);
}

CEntity* LoadAtomicBeta(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrAtomicBeta sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrAtomicBeta.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  return rs_new CAtomicBeta(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      *modelData, LdrToActorParameters(sldrThis.actorInformation),
      LdrToPatternedInfo(sldrThis.patterned, nullptr), sldrThis.beamEffect, sldrThis.beam,
      LdrToDamageInfo(sldrThis.beamDamage), sldrThis.contactFx, sldrThis.beamFadeTime,
      sldrThis.beamRadius, sldrThis.damageDelay,
      LdrToDamageVulnerability(sldrThis.frozenVulnerability), sldrThis.hoverSpeed,
      sldrThis.normalRotateSpeed, sldrThis.chargingRotateSpeed, sldrThis.sound_FlyLoop,
      sldrThis.sound_FlyLoopActivated, sldrThis.sound_ElectricityLoop, sldrThis.speedChangeRate);
}

#ifndef MONOLITHIC
// @ fn_5_10C
static void SetFuncPtrs() {
  static SAtomicBeta_FuncPtrs funcPtrs;
  funcPtrs.mLoader = &LoadAtomicBeta;
  SetSAtomicBeta_FuncPtrs(&funcPtrs);
}

void RELMain() { SetFuncPtrs(); }

void RELExit() { SetSAtomicBeta_FuncPtrs(nullptr); }
#endif
