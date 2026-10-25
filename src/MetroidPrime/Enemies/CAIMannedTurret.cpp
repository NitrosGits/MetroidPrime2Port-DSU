#include "MetroidPrime/Enemies/CAIMannedTurret.hpp"

#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CPASDatabase.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CResFactory.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CAnimPlaybackParms.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/SFX/MannedTurret2.h"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrAIMannedTurret.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CEnergyProjectile.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"
#include "REL/REL_Setup.h"
#include <math.h>

static rstl::string skMuzzleLocator = rstl::string_l("muzzleend_LCTR");
static rstl::string skTelegraphLocator1 = rstl::string_l("can01_LCTR");
static rstl::string skTelegraphLocator2 = rstl::string_l("can02_LCTR");
static rstl::string skTelegraphLocator3 = rstl::string_l("can03_LCTR");
static rstl::string skTelegraphLocator4 = rstl::string_l("can04_LCTR");

static CColor skDamageFlashColor(0.5f, 0.f, 0.f, 1.f);

// Retail `__sinit_CAIMannedTurret_cpp` (0x4E08-0x5130) accounts for exactly three tables, five
// rstl::string statics and one CColor. Tables: 30 word copies (`stw` 0x504C-0x5118, the only .data
// writes) put each entry's member pointer at +4/+8/+0xC as {adjust 0, vtoff -1 (non-virtual),
// fn = address}; `lis` 0x4FE8/0x4FD4/0x4FEC take .data 0x30 (4 states), 0xAC (5 triggers), 0x108
// (1 code func). Targets .text 0x2FA0 Start, 0x2AA8 Patrol, 0x2C6C Attack, 0x2AA4 Dead;
// 0x2A98/0x298C/0x28A4/0x2A8C/0x2A78 CheckReady..CheckAlive; 0x2890 TookDamage (names .rodata
// 0x68/0x6E/0x75/0x7C, 0x81/0x8C/0x98/0xA4/0xAE, 0xB9) - same names, order, targets as below.
// Strings, .bss 0x0C/0x28/0x44/0x60/0x7C, each built by the same four calls - rstl::string_l
// (0x4E28/0x4E68/0x4EA8/0x4EE8/0x4F28), string copy ctor (0x4E38/.../0x4F38),
// __register_global_object (0x4E4C/.../0x4F4C), internal_dereference (0x4E54/.../0x4F54) - over
// .rodata 0xDF "muzzleend_LCTR", 0xEE "can01_LCTR", 0xF9 "can02_LCTR", 0x104 "can03_LCTR",
// 0x10F "can04_LCTR": the host's five skMuzzle/skTelegraphLocator statics with the same literals.
// CColor, retail .bss 0x8C (4 B, the one non-.data store): `lfs` 0x4F64 reads .rodata 0x2C
// (0x3F000000 = 0.5f), 0x4F68 .rodata 0x10 (0.0f), 0x4F70 .rodata 0x0C (0x3F800000 = 1.0f), f3 = f2,
// then __ct__6CColorFffff (DOL 0x803207FC) at 0x4F7C -> CColor(0.5f, 0.f, 0.f, 1.f), the host's
// `skDamageFlashColor(0.5f, 0.f, 0.f, 1.f)` (same four values; host ctor call passes 0.5/0/0/1).
// No other store, no branch; frame stmw r14 0x4E20 (0xC0, lmw 0x511C). `Attack` (0x2C6C) is still
// without a `// @` marker yet; the other nine targets carry theirs.
// @ fn_1_4E08
static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Start", reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CAIMannedTurret::Start)},
    {"Patrol", reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CAIMannedTurret::Patrol)},
    {"Attack", reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CAIMannedTurret::Attack)},
    {"Dead", reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CAIMannedTurret::Dead)},
};

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"CheckReady",
     reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CAIMannedTurret::CheckReady)},
    {"CheckPatrol",
     reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CAIMannedTurret::CheckPatrol)},
    {"CheckAttack",
     reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CAIMannedTurret::CheckAttack)},
    {"CheckDead",
     reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CAIMannedTurret::CheckDead)},
    {"CheckAlive",
     reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CAIMannedTurret::CheckAlive)},
};

static CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    {"TookDamage",
     reinterpret_cast< CPatterned::StateMachine::CodeFunc >(&CAIMannedTurret::TookDamage)},
};

// @ fn_1_3F00
CAIMannedTurret::CAIMannedTurret(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                                 const CTransform4f& xf, const CModelData& modelData,
                                 const CMaterialList& materials, const CHealthInfo& health,
                                 const CDamageVulnerability& vulnerability,
                                 const SLdrAIMannedTurretData& data, const CDamageInfo& damage,
                                 const SLdrSpline& horizontalSpline,
                                 const SLdrSpline& verticalSpline, const CAssetId& telegraphEffect)
: CAi(uid, name, info, 0, xf, modelData, CAABox::MakeNullBox(), 1.f, health, vulnerability,
      materials, kInvalidAssetId, data.stateMachine, CActorParameters::None(), 0.3f, 0.8f)
, mData(data)
, mTurretId(kInvalidUniqueId)
, mRiderId(kInvalidUniqueId)
, mAimUpAnim(0)
, mAimDownAnim(0)
, mFireAnim(0)
, mDeathAnim(0)
, mTargetElevation(0.f)
, mElevation(0.f)
, mFireTimer(0.f)
, mLeashTimer(0.f)
, mTurretModelFlags(CModelFlags::Normal())
, mWeaponToken(nullptr)
, mDamageInfo(damage)
, mDamageFlashTimer(0.f)
, mInitialPosition(xf.GetTranslation())
, x54c_(0)
, mAimDirection(xf.GetForward())
, mStateMachineState(rs_new CGenericFSM2State< CPatterned >)
, mHorizontalSpline(horizontalSpline)
, mVerticalSpline(verticalSpline)
, mPatrolTime(0.f)
, mPatrolDuration(0.f)
, mTelegraphGen1(nullptr)
, mTelegraphGen2(nullptr)
, mTelegraphGen3(nullptr)
, mTelegraphGen4(nullptr)
, mTelegraphTimer(0.f) {
  mReady = false;
  mDead = false;
  mTookDamage = false;
  SetDrawShadow(false);
  mPatrolDuration = mHorizontalSpline.GetMaxTime();
  if (mVerticalSpline.GetMaxTime() > mPatrolDuration) {
    mPatrolDuration = mVerticalSpline.GetMaxTime();
  }
  if (telegraphEffect != kInvalidAssetId) {
    mTelegraphEffect = TCachedToken< CGenDescription >(
        gpSimplePool->GetObj(SObjectTag('PART', telegraphEffect)), true);
  }
}

// @ fn_1_3D70
CAIMannedTurret::~CAIMannedTurret() {}

// @ fn_1_3C80
void CAIMannedTurret::SetupStateMachine(CStateManager& mgr) {
  CGenericFSM2* machine = GetStateMachine2();
  if (machine != nullptr) {
    CGenericFSM2State< CPatterned >* state = mStateMachineState.get();
    state->Setup(*machine);
    state->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
    state->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
    state->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
    mStateMachineState->SetState(mgr, reinterpret_cast< CPatterned& >(*this),
                                 rstl::string_l("Start"));
  }
}

// @ fn_1_3694
void CAIMannedTurret::AttachToActors(CStateManager& mgr) {
  mTurretId = FindConnectedObject(mgr, kSS_AttachedAnimatedObject, kSM_Attach);
  if (CActor* turret = TCastToPtr< CActor >(mgr.ObjectById(mTurretId))) {
    if (turret->ModelData()->AnimationData() != nullptr) {
      const CPASDatabase& pasDatabase = turret->AnimationData()->GetPASDatabase();
      mAimUpAnim =
          pasDatabase
              .FindBestAnimation(CPASAnimParmData(pas::kAS_Getup, CPASAnimParm::FromEnum(0)), -1)
              .second;
      mAimDownAnim =
          pasDatabase
              .FindBestAnimation(CPASAnimParmData(pas::kAS_Getup, CPASAnimParm::FromEnum(1)), -1)
              .second;
      mFireAnim = pasDatabase.FindBestAnimation(CPASAnimParmData(pas::kAS_Step), -1).second;
      mDeathAnim = pasDatabase.FindBestAnimation(CPASAnimParmData(pas::kAS_Death), -1).second;
      mIdleAnim = pasDatabase.FindBestAnimation(CPASAnimParmData(pas::kAS_Locomotion), -1).second;
      mTurretModelFlags = turret->GetModelFlags();
    }
  }
  mRiderId = FindConnectedObject(mgr, kSS_AttachedCollisionObject, kSM_Attach);
  if (CActor* rider = TCastToPtr< CActor >(mgr.ObjectById(mRiderId))) {
    rider->AddMaterial(kMT_ScanPassthrough, mgr);
  }
}

// @ fn_1_328C
void CAIMannedTurret::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CAi::AcceptScriptMsg(mgr, msg);
  if (GetActive()) {
    switch (msg.GetMessage()) {
    case kSM_Activate:
      mDead = false;
      mReady = false;
      mTookDamage = false;
      break;
    case kSM_AreaLoaded:
      SetupStateMachine(mgr);
      AttachToActors(mgr);
      break;
    case kSM_Create:
      mWeaponToken = gpSimplePool->GetObj(SObjectTag(
          gpResourceFactory->GetResourceTypeById(mData.weaponEffect), mData.weaponEffect));
      break;
    case kSM_Start:
      mReady = true;
      mDead = false;
      mTookDamage = false;
      AddMaterial(kMT_Orbit, kMT_SeekerTarget, kMT_Target, mgr);
      break;
    case kSM_Stop:
      if (GetActive()) {
        RemoveMaterial(kMT_Orbit, kMT_SeekerTarget, mgr);
        CActor* turret = TCastToPtr< CActor >(mgr.ObjectById(mTurretId));
        CActor* rider = TCastToPtr< CActor >(mgr.ObjectById(mRiderId));
        mElevation = 0.f;
        mTargetElevation = 0.f;
        if (turret != nullptr && rider != nullptr) {
          const CVector3f position = turret->GetTranslation();
          turret->SetTransform(GetTransform());
          turret->SetTranslation(position);
          turret->AnimationData()->AddAdditiveAnimation(mAimUpAnim, 0.f, false, false);
          turret->AnimationData()->AddAdditiveAnimation(mAimDownAnim, 0.f, false, false);
          turret->AnimationData()->EnableLooping(false);
          turret->AnimationData()->SetAnimation(CAnimPlaybackParms(mIdleAnim, -1, 1.f, true),
                                                false);
        }
        mDead = true;
        mReady = false;
        RemoveMaterial(kMT_Target, mgr);
      }
      break;
    case kSM_Damage:
      mDamageFlashTimer = 0.33f;
      CSfxManager::AddEmitter(SFXman_r_impact_00_oneshot, mInitialPosition,
                              GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
      mTookDamage = true;
      break;
    }
  }
}

// @ fn_1_31F4
CVector3f CAIMannedTurret::GetAimPosition(const CStateManager& mgr, float dt) const {
  if (const CActor* rider = TCastToConstPtr< CActor >(mgr.GetObjectById(mRiderId))) {
    return rider->GetTranslation();
  }
  return CPhysicsActor::GetAimPosition(mgr, 0.f);
}

// @ fn_1_3168
CVector3f CAIMannedTurret::GetOrbitPosition(const CStateManager& mgr) const {
  if (const CActor* rider = TCastToConstPtr< CActor >(mgr.GetObjectById(mRiderId))) {
    return rider->GetTranslation();
  }
  return CPhysicsActor::GetOrbitPosition(mgr);
}

// @ fn_1_2FA0
void CAIMannedTurret::Start(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    mElevation = 0.f;
    mTargetElevation = 0.f;
    mReady = false;
    mDead = false;
    CActor* turret = TCastToPtr< CActor >(mgr.ObjectById(mTurretId));
    CActor* rider = TCastToPtr< CActor >(mgr.ObjectById(mRiderId));
    if (turret != nullptr && turret->ModelData()->AnimationData() != nullptr && rider != nullptr) {
      const CVector3f position = turret->GetTranslation();
      turret->SetTransform(GetTransform());
      turret->SetTranslation(position);
      turret->AnimationData()->AddAdditiveAnimation(mAimUpAnim, 0.f, false, false);
      turret->AnimationData()->AddAdditiveAnimation(mAimDownAnim, 0.f, false, false);
      rider->SetTransform(turret->GetTransform() * turret->GetModelData()->GetLocatorTransform(
                                                       rstl::string_l("upwardballsnap_LCTR")));
      SetTransform(turret->GetTransform());
    }
  }
  UpdateRider(mgr, dt);
}

void CAIMannedTurret::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mLeashTimer = mData.attackLeashTimer;
    mTelegraphTimer = 0.f;
    break;
  case kStateMsg_Update:
    if (mTelegraphTimer > 0.f) {
      mTelegraphTimer -= dt;
      if (mTelegraphTimer < 0.f) {
        Fire(mgr);
        mFireTimer = mData.fireRate + mData.fireRateRandomFactor * mgr.Random()->Float();
      }
    } else if (CActor* rider = TCastToPtr< CActor >(mgr.ObjectById(mRiderId))) {
      CPlayer* player = mgr.GetPlayer(0);
      if ((player->GetTranslation() - GetTranslation()).ToVec2f().Magnitude() >=
          mData.maxAttackRange) {
        mLeashTimer -= dt;
      } else {
        mLeashTimer = mData.attackLeashTimer;
      }
      const CVector3f aimPosition = player->GetAimPosition(mgr, 0.f);
      CProjectileInfo projectileInfo(mData.weaponEffect, mDamageInfo);
      mAimDirection = aimPosition - rider->GetTranslation();
      mAimDirection.Normalize();
      mFireTimer -= dt;
      if (mFireTimer <= 0.f) {
        const float angle = CMath::Rad2Deg(acos(
            CMath::Limit(CVector3f::Dot(mAimDirection, rider->GetTransform().GetForward()), 1.f)));
        if (angle < 5.f) {
          SpawnTelegraph(mgr);
        }
      }
    }
    break;
  }
  UpdateAim(mgr, dt, false);
}

// @ fn_1_2AA8
void CAIMannedTurret::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    const CVector3f forward = GetTransform().GetForward();
    mAimDirection = CQuaternion::ZRotation(CRelAngle::FromDegrees(-mData.maxHorizRotationRight))
                        .Transform(forward);
    break;
  }
  case kStateMsg_Update: {
    mPatrolTime += dt;
    if (mPatrolTime > mPatrolDuration) {
      mPatrolTime -= mPatrolDuration;
    }
    const CVector3f forward = GetTransform().GetForward();
    mAimDirection =
        CQuaternion::ZRotation(CRelAngle::FromDegrees(mHorizontalSpline.EvaluateAt(mPatrolTime)))
            .Transform(forward);
    mAimDirection =
        CQuaternion::XRotation(CRelAngle::FromDegrees(mVerticalSpline.EvaluateAt(mPatrolTime)))
            .Transform(mAimDirection);
    UpdateAim(mgr, dt, false);
    break;
  }
  }
}

// @ fn_1_2AA4
void CAIMannedTurret::Dead(CStateManager& mgr, EStateMsg msg, float dt) {}

// @ fn_1_2A98
bool CAIMannedTurret::CheckReady(CStateManager& mgr, const CTriggerData& data) const {
  return mReady;
}

// @ fn_1_2A8C
bool CAIMannedTurret::CheckDead(CStateManager& mgr, const CTriggerData& data) const {
  return mDead;
}

// @ fn_1_2A78
bool CAIMannedTurret::CheckAlive(CStateManager& mgr, const CTriggerData& data) const {
  return !mDead;
}

// @ fn_1_298C
bool CAIMannedTurret::CheckPatrol(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  if (TCastToPtr< CActor >(mgr.ObjectById(mRiderId))) {
    CVector2f toPlayer = (mgr.GetPlayer(0)->GetTranslation() - GetTranslation()).ToVec2f();
    if (CVector3f(toPlayer, 0.f).Magnitude() >= mData.maxAttackRange && mLeashTimer < 0.f) {
      result = true;
    }
  }
  return result;
}

// @ fn_1_28A4
bool CAIMannedTurret::CheckAttack(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  if (TCastToPtr< CActor >(mgr.ObjectById(mRiderId))) {
    CVector2f toPlayer = (mgr.GetPlayer(0)->GetTranslation() - GetTranslation()).ToVec2f();
    if (CVector3f(toPlayer, 0.f).Magnitude() <= mData.startAttackRange) {
      result = true;
    }
    if (mTookDamage) {
      result = true;
    }
  }
  return result;
}

// @ fn_1_2890
void CAIMannedTurret::TookDamage(CStateManager& mgr, float dt) { mTookDamage = false; }

// @ fn_1_2870
void CAIMannedTurret::AddToRenderer(const CStateManager& mgr) const { EnsureRendered(mgr); }

// @ fn_1_2730
void CAIMannedTurret::PreRender(CStateManager& mgr) {
  if (CActor* turret = TCastToPtr< CActor >(mgr.ObjectById(mTurretId))) {
    CModelFlags flags(mTurretModelFlags);
    if (mDamageFlashTimer > 0.f) {
      const float t = CMath::Clamp(0.f, mDamageFlashTimer / 0.33f, 1.f);
      flags =
          CModelFlags(CModelFlags::kT_Two, CColor::Lerp(CColor::Black(), skDamageFlashColor, t));
    }
    turret->SetModelFlags(flags);
  }
}

// @ fn_1_2698
void CAIMannedTurret::Render(const CStateManager& mgr) const {
  if (mTelegraphGen1.get() != nullptr) {
    mTelegraphGen1->Render();
  }
  if (mTelegraphGen2.get() != nullptr) {
    mTelegraphGen2->Render();
  }
  if (mTelegraphGen3.get() != nullptr) {
    mTelegraphGen3->Render();
  }
  if (mTelegraphGen4.get() != nullptr) {
    mTelegraphGen4->Render();
  }
}

// Every arm ran or was forced (tools/cov amt3, 06_sand_alamofront, 2026-10-25). Forced three
// times each by amt3_force.py (frame-owned writes, readback, restored at return): |dot| < 1e-4 at
// both tests (xmm0/xmm1 := 0 at UpdateAim+353/+543), angle > limit on both sides (horizontal
// limits := 0), the three elevation clamp extremes (mElevation := -1000/1000, 170 with
// vertSpeed 1e6) and snap (rdx := 1 at entry). Snap is code-dead in retail: both callers pass
// li r5,0 (Patrol 0x2C34, Attack 0x2EFC). Unrun arcs are landing pads only (callees don't throw).
// @ fn_1_1F28
void CAIMannedTurret::UpdateAim(CStateManager& mgr, float dt, bool snap) {
  if (CActor* turret = TCastToPtr< CActor >(mgr.ObjectById(mTurretId))) {
    const CVector2f turretForward2d = turret->GetTransform().GetForward().ToVec2f().AsNormalized();
    const CVector3f flatTurretForward(turretForward2d, 0.f);
    CVector3f heading = flatTurretForward;
    const CVector2f aim2d = mAimDirection.ToVec2f().AsNormalized();
    const CVector3f flatAim(aim2d, 0.f);
    float dot = CVector3f::Dot(flatAim, flatTurretForward);
    if (CMath::AbsF(dot) >= 0.0001f) {
      const float maxStep = dt * mData.horizSpeed;
      const float angle = CMath::Rad2Deg(acos(dot));
      CQuaternion rotation =
          CQuaternion::LookAt(CUnitVector3f(flatTurretForward), CUnitVector3f(flatAim),
                              CRelAngle::FromDegrees(maxStep * CMath::Limit(angle / 15.f, 1.f)));
      if (snap) {
        rotation = CQuaternion::LookAt(CUnitVector3f(flatTurretForward), CUnitVector3f(flatAim),
                                       CRelAngle::FromRadians(6.2831855f));
      }
      heading = rotation.Transform(flatTurretForward);
    }
    const CVector2f baseForward2d = GetTransform().GetForward().ToVec2f().AsNormalized();
    const CVector3f flatBaseForward(baseForward2d, 0.f);
    dot = CVector3f::Dot(heading, flatBaseForward);
    if (CMath::AbsF(dot) >= 0.0001f) {
      const float angle = CMath::Rad2Deg(acos(dot));
      if (CVector3f::Cross(heading, flatBaseForward).GetZ() >= 0.f) {
        if (angle > mData.maxHorizRotationRight) {
          heading = turret->GetTransform().GetForward();
        }
      } else {
        if (angle > mData.maxHorizRotationLeft) {
          heading = turret->GetTransform().GetForward();
        }
      }
      const CVector3f position = turret->GetTranslation();
      const CVector2f heading2d = heading.ToVec2f().AsNormalized();
      heading = CVector3f(heading2d, 0.f);
      turret->SetTransform(CTransform4f::LookAt(position, position + heading, CVector3f::Up()));
    }
    mTargetElevation =
        CMath::Rad2Deg(acos(CMath::Limit(CVector3f::Dot(mAimDirection, flatAim), 1.f)));
    if (mAimDirection.GetZ() < 0.f) {
      mTargetElevation = -mTargetElevation;
    }
    const float diff = mTargetElevation - mElevation;
    const float step = dt * mData.vertSpeed;
#ifdef TARGET_PC
    // 0x23EC fcmpo |diff|,0.1 / 0x23F0 blt (NaN falls into the step arm, not the snap arm)
    if (!(CMath::AbsF(diff) < 0.1f) && !snap) {
#else
    if (CMath::AbsF(diff) >= 0.1f && !snap) {
#endif
      const float scaledStep =
          step * (1.f - CMath::PhongBlob(CMath::Clamp(0.f, CMath::AbsF(diff / 20.f), 1.f), 8.f));
      if (diff > 0.f) {
        mElevation = CMath::Clamp(-180.f, mElevation + scaledStep, mTargetElevation);
      } else {
        mElevation = CMath::Clamp(mTargetElevation, mElevation - scaledStep, 180.f);
      }
    } else {
      mElevation = mTargetElevation;
    }
    mElevation = CMath::Clamp(-mData.maxVertElevationDown, mElevation, mData.maxVertElevationUp);
    if (mElevation >= 0.f) {
      turret->AnimationData()->AddAdditiveAnimation(
          mAimUpAnim, CMath::Clamp(0.f, mElevation / 90.f, 1.f), false, false);
      turret->AnimationData()->AddAdditiveAnimation(mAimDownAnim, 0.f, false, false);
    } else {
      turret->AnimationData()->AddAdditiveAnimation(
          mAimDownAnim, CMath::Clamp(0.f, -mElevation / 90.f, 1.f), false, false);
      turret->AnimationData()->AddAdditiveAnimation(mAimUpAnim, 0.f, false, false);
    }
  }
  UpdateRider(mgr, dt);
}

// @ fn_1_1E14
void CAIMannedTurret::UpdateRider(CStateManager& mgr, float dt) {
  CActor* turret = TCastToPtr< CActor >(mgr.ObjectById(mTurretId));
  CActor* rider = TCastToPtr< CActor >(mgr.ObjectById(mRiderId));
  if (turret != nullptr && rider != nullptr) {
    rider->SetTransform(turret->GetTransform() * turret->GetModelData()->GetLocatorTransform(
                                                     rstl::string_l("upwardballsnap_LCTR")));
  }
}

// @ fn_1_19C4
void CAIMannedTurret::Think(float dt, CStateManager& mgr) {
  if (CActor* turret = TCastToPtr< CActor >(mgr.ObjectById(mTurretId))) {
    if (mTelegraphGen1.get() != nullptr) {
      const CTransform4f locatorXf = turret->GetLocatorTransform(skTelegraphLocator1);
      const CVector3f position =
          turret->GetTranslation() + turret->GetTransform().Rotate(locatorXf.GetTranslation());
      mTelegraphGen1->SetGlobalTranslation(position);
      mTelegraphGen1->SetOrientation(turret->GetTransform().GetRotation());
      mTelegraphGen1->Update(dt);
    }
    if (mTelegraphGen2.get() != nullptr) {
      const CTransform4f locatorXf = turret->GetLocatorTransform(skTelegraphLocator2);
      const CVector3f position =
          turret->GetTranslation() + turret->GetTransform().Rotate(locatorXf.GetTranslation());
      mTelegraphGen2->SetGlobalTranslation(position);
      mTelegraphGen2->SetOrientation(turret->GetTransform().GetRotation());
      mTelegraphGen2->Update(dt);
    }
    if (mTelegraphGen3.get() != nullptr) {
      const CTransform4f locatorXf = turret->GetLocatorTransform(skTelegraphLocator3);
      const CVector3f position =
          turret->GetTranslation() + turret->GetTransform().Rotate(locatorXf.GetTranslation());
      mTelegraphGen3->SetGlobalTranslation(position);
      mTelegraphGen3->SetOrientation(turret->GetTransform().GetRotation());
      mTelegraphGen3->Update(dt);
    }
    if (mTelegraphGen4.get() != nullptr) {
      const CTransform4f locatorXf = turret->GetLocatorTransform(skTelegraphLocator4);
      const CVector3f position =
          turret->GetTranslation() + turret->GetTransform().Rotate(locatorXf.GetTranslation());
      mTelegraphGen4->SetGlobalTranslation(position);
      mTelegraphGen4->SetOrientation(turret->GetTransform().GetRotation());
      mTelegraphGen4->Update(dt);
    }
  }
  if (mStateMachineState->HasState()) {
    mStateMachineState->Update(mgr, reinterpret_cast< CPatterned& >(*this), dt);
  }
  if (mDamageFlashTimer > 0.f) {
    mDamageFlashTimer -= dt;
  }
  CAi::Think(dt, mgr);
}

// @ fn_1_19C0
void CAIMannedTurret::Death(CStateManager& mgr, const CVector3f& direction,
                            EScriptObjectState state) {}

// @ fn_1_19BC
void CAIMannedTurret::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {}

// @ fn_1_1494
void CAIMannedTurret::SpawnTelegraph(CStateManager& mgr) {
  mTelegraphTimer = 0.5f;
  CActor* turret = TCastToPtr< CActor >(mgr.ObjectById(mTurretId));
  if (turret != nullptr && mTelegraphEffect) {
    CTransform4f xf = turret->GetTransform() *
                      turret->GetModelData()->GetScaledLocatorTransform(skTelegraphLocator1);
    mTelegraphGen1 =
        rs_new CElementGen(*mTelegraphEffect, CElementGen::kMOT_Normal, CElementGen::kOSF_One);
    mTelegraphGen1->SetGlobalTranslation(xf.GetTranslation());
    mTelegraphGen1->SetOrientation(xf.GetRotation());
    xf = turret->GetTransform() *
         turret->GetModelData()->GetScaledLocatorTransform(skTelegraphLocator2);
    mTelegraphGen2 =
        rs_new CElementGen(*mTelegraphEffect, CElementGen::kMOT_Normal, CElementGen::kOSF_One);
    mTelegraphGen2->SetGlobalTranslation(xf.GetTranslation());
    mTelegraphGen2->SetOrientation(xf.GetRotation());
    xf = turret->GetTransform() *
         turret->GetModelData()->GetScaledLocatorTransform(skTelegraphLocator3);
    mTelegraphGen3 =
        rs_new CElementGen(*mTelegraphEffect, CElementGen::kMOT_Normal, CElementGen::kOSF_One);
    mTelegraphGen3->SetGlobalTranslation(xf.GetTranslation());
    mTelegraphGen3->SetOrientation(xf.GetRotation());
    xf = turret->GetTransform() *
         turret->GetModelData()->GetScaledLocatorTransform(skTelegraphLocator4);
    mTelegraphGen4 =
        rs_new CElementGen(*mTelegraphEffect, CElementGen::kMOT_Normal, CElementGen::kOSF_One);
    mTelegraphGen4->SetGlobalTranslation(xf.GetTranslation());
    mTelegraphGen4->SetOrientation(xf.GetRotation());
    if (mgr.IsMultiplayer()) {
      CSfxManager::AddEmitter(SFXma2_a_mislcharge_00_oneshot, xf.GetTranslation(),
                              GetCurrentAreaId().Value(), false, false, CSfxManager::kMedPriority);
    } else {
      CSfxManager::AddEmitter(SFXman_a_mislcharge_00_oneshot, xf.GetTranslation(),
                              GetCurrentAreaId().Value(), false, false, CSfxManager::kMedPriority);
    }
  }
}

// @ fn_1_13EC
CTransform4f CAIMannedTurret::GetMuzzleTransform(CStateManager& mgr) const {
  CTransform4f xf = GetTransform();
  if (const CActor* turret = TCastToConstPtr< CActor >(mgr.GetObjectById(mTurretId))) {
    xf =
        turret->GetTransform() * turret->GetModelData()->GetScaledLocatorTransform(skMuzzleLocator);
  }
  return xf;
}

// Every arm ran or was forced (tools/cov amt3, 06_sand_alamofront, 2026-10-25): the loaded
// path with anim data ran natural (29 calls); not-loaded (retail 0xD38 cmplwi; host Fire+82 ZF) and
// no-anim-data (retail 0xEC0 cmplwi; host Fire+480 ZF) forced twice each (amt3_force.py, frame-owned,
// eflags readback, restored at return). The null-projectile arm (retail 0xE48 cmplwi r28) runs
// only on allocation failure; the host's throwing new lets gcc fold it. Unrun arcs are landing
// pads only (callees don't throw).
// @ fn_1_CF0
void CAIMannedTurret::Fire(CStateManager& mgr) {
  const CTransform4f xf = GetMuzzleTransform(mgr);
  if (mWeaponToken.IsLoaded()) {
    CEnergyProjectile* projectile = rs_new CEnergyProjectile(
        true, mWeaponToken, static_cast< EWeaponType >(mDamageInfo.GetWeaponMode1()), xf,
        kMT_ProjectilePassthrough, mDamageInfo, mgr.AllocateUniqueId(), GetCurrentAreaId(),
        GetUniqueId(), kInvalidUniqueId, 0, false, CVector3f::One(), CImpactVisorEffect::None(),
        false, true, false, 1.f, 4.f, 4.f);
    if (projectile != nullptr) {
      projectile->AddCollisionCooldown(mRiderId, 3.4028235e38f);
      projectile->SetDamageDuration(1.f);
      mgr.AddObject(projectile);
      if (CActor* turret = TCastToPtr< CActor >(mgr.ObjectById(mTurretId))) {
        if (turret->ModelData()->AnimationData() != nullptr) {
          turret->AnimationData()->EnableLooping(false);
          turret->AnimationData()->SetAnimation(CAnimPlaybackParms(mFireAnim, -1, 1.f, true),
                                                false);
        }
      }
      CSfxManager::AddEmitter(SFXma2_a_mislfire_00_oneshot, xf.GetTranslation(),
                              GetCurrentAreaId().Value(), false, false, CSfxManager::kMedPriority);
    }
  }
}

// @ fn_1_D4
CEntity* LoadAIMannedTurret(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrAIMannedTurret sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrAIMannedTurret.inc"

  return rs_new CAIMannedTurret(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      CModelData::CModelDataNull(), CMaterialList(kMT_Solid, kMT_Orbit, kMT_Target),
      LdrToHealthInfo(sldrThis.data.health), LdrToDamageVulnerability(sldrThis.data.vulnerability),
      sldrThis.data, LdrToDamageInfo(sldrThis.data.weaponDamage), sldrThis.patrolHorizSpline,
      sldrThis.patrolVerticalSpline, sldrThis.data.telegraphEffect);
}

#ifndef MONOLITHIC
// @ fn_1_A4
static void SetFuncPtrs() {
  static SAIMannedTurret_FuncPtrs funcPtrs;
  funcPtrs.mLoader = &LoadAIMannedTurret;
  SetSAIMannedTurret_FuncPtrs(&funcPtrs);
}

extern "C" void RELMain() { SetFuncPtrs(); }

extern "C" void RELExit() { SetSAIMannedTurret_FuncPtrs(nullptr); }
#endif
