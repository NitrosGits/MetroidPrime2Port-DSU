// Ingsmasher (CElitePirate, REL 15): the FSM combat states (shield, shock wave, melee, rockets, angry attack, taunt, power up/down, death).
// Part of the module body in PortCElitePirate.cpp; analysis in docs/module-analysis/ElitePirate.md.
#include "MetroidPrime/Enemies/CElitePirate.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CSkinnedModel.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CPlane.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CAxisAngle.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CRumbleManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Cameras/CFirstPersonCamera.hpp"
#include "MetroidPrime/Collision/CJointCollisionDescription.hpp"
#include "MetroidPrime/Enemies/CElitePirateGrenadeLauncher.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrElitePirate.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIHint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CEnergyProjectile.hpp"
#include "MetroidPrime/Weapons/CGameProjectile.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
#include "MetroidPrime/Weapons/WeaponTypes.hpp"
#include <float.h>
#include <stdio.h>
#include <math.h>

namespace {

// The actor behind the target uid (0xa26) that SelectTarget picks; retail reads its translation.
const CActor* TargetActor(const CStateManager& mgr, TUniqueId id) {
  return TCastToConstPtr< CActor >(mgr.GetObjectById(id));
}

} // namespace

// Retail has one copy of this per command type (fn_15_BAF8 melee, fn_15_BC04 projectile,
// fn_15_BD10 loop attack; the taunt's fn_15_BEA0 is TauntCmd): deliver on Activate, re-deliver on Update while the
// body is not yet in `state`, and clear the animation state (0x6b4) on Deactivate.
template < typename T >
void CElitePirate::TryCommand(EStateMsg msg, pas::EAnimationState state, const T& cmd) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(cmd);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), state)) {
      BodyController()->CommandMgr().DeliverCmd(cmd);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// @ fn_15_1F2C
// After an attack: unless the repeat chance hits, wait an attack time before the next one.
void CElitePirate::ResetAttackTimer(CStateManager& mgr) {
  if (mgr.IsRandomAvailable() && mData.mRepeatedAttackChance < mgr.Random()->Float()) {
    const float average = GetAverageAttackTime();
    mTimer = mgr.Random()->Float() * mAttackTimeVariation + average;
  }
}

// @ fn_15_41A0
// Shield up: keep closing on the target, drive the shield's emitter from the left palm.
void CElitePirate::Shielding(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) fprintf(stderr, "[ep-probe] %s\n", "Shielding");
  if (!ShieldKilled(mgr, CTriggerData(0))) {
    CVector3f target = x810_;
    if (const CActor* actor = TargetActor(mgr, xa26_)) {
      target = actor->GetTranslation();
    }
    PathToward(mgr, msg, dt, target);
  }
  switch (msg) {
  case kStateMsg_Activate:
    mDoubleSpeed = true;
    mAngryCount = 0;
    break;
  case kStateMsg_Update:
    if (mXDamaged) {
      // [?] nothing but the ctor writes 0xa38, so this clears XDamaged on the first update.
      xa38_ -= dt;
      if (xa38_ <= 0.f) {
        mXDamaged = false;
      }
    }
    CSfxManager::UpdateEmitter(mShieldSfx, mLeftPalmPos, CVector3f::Zero(), 0x7f);
    break;
  case kStateMsg_Deactivate:
    mShieldCollisionManager->SetActive(mgr, false);
    SetShieldVulnerability(mgr, false);
    mDoubleSpeed = false;
    mHandGen = nullptr;
    mShieldPopPending = true;
    CSfxManager::RemoveEmitter(mShieldSfx);
    mShieldSfx = CSfxHandle();
    break;
  }
}

// @ fn_15_44A8
// Raise the shield: crouch, switch polarity (dark <-> light, random from none), start the hand
// effect and its sound, and make the body reflect.
void CElitePirate::ShieldUp(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) fprintf(stderr, "[ep-probe] %s\n", "ShieldUp");
  if (msg == kStateMsg_Activate) {
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    mShieldCollisionManager->SetActive(mgr, true);
    KnockBackController().EnableAnimReaction(CKnockBackMgr::kAR_Flinch, false);
    BodyController()->CommandMgr().ClearLocomotionCmds();
    xc5c_ = mTime;
    switch (mShieldPolarity) {
    case 0:
      mShieldPolarity = 1;
      break;
    case 1:
      mShieldPolarity = 0;
      break;
    case 2:
      mShieldPolarity = mgr.Random()->Range(0.f, 1.f) >= 0.5f ? 0 : 1;
      break;
    }
    const CAssetId effect = mShieldPolarity == 1 ? mData.mLightShield : mData.mDarkShield;
    const ushort sfx = mShieldPolarity == 1 ? mData.mLightShieldSound : mData.mDarkShieldSound;
    mHandGen = rs_new CElementGen(
        TToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', effect))),
        CElementGen::kMOT_Normal, CElementGen::kOSF_One);
    mHandGen->SetParticleEmission(true);
    mShieldDamage = 0.f;
    mShieldSfx = CSfxManager::AddEmitter(sfx, GetTranslation(), GetCurrentAreaId().Value(), false, true,
                                         CSfxManager::kMedPriority);
    SetShieldVulnerability(mgr, true);
  } else if (msg == kStateMsg_Update) {
    RotateToPoint(mgr.GetPlayer(0)->GetTranslation(), dt, 1.7453293f);
  }
}

// @ fn_15_4AE0
// The ground smash. The projectile-attack severity picks the shock wave: weights data+0x144..0x150
// for severities 0/1/2/5, and 5 is the double wave (GetShockWaveInfo).
void CElitePirate::Shockwave(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) fprintf(stderr, "[ep-probe] %s\n", "Shockwave");
  SetAttackKind(7, msg);
  if (msg == kStateMsg_Activate) {
    // [probable] the four weights are the loader's 0x28b39197/0xe27de71b/0x665e7ace/0xacd4d06d in
    // data order (+0x144..+0x150).
    const float w0 = mData.mUnknown28B39197;
    const float w1 = mData.mUnknownE27DE71B;
    const float w2 = mData.mUnknown665E7ACE;
    const float w3 = mData.mUnknownACD4D06D;
    const float r = mgr.Random()->Range(0.f, w3 + w2 + w0 + w1);
    if (r < w0) {
      xcb0_ = 0;
    } else if (r < w0 + w1) {
      xcb0_ = 1;
    } else if (r < w0 + w1 + w2) {
      xcb0_ = 2;
    } else {
      xcb0_ = 5;
    }
  } else if (msg == kStateMsg_Deactivate) {
    ResetAttackTimer(mgr);
    xcb4_ = xcb0_;
  }
  if (const CActor* target = TargetActor(mgr, xa26_)) {
    BodyController()->CommandMgr().SetTargetVector(target->GetTranslation() - GetTranslation());
    TryCommand(msg, pas::kAS_ProjectileAttack,
               CBCProjectileAttackCmd(pas::ESeverity(xcb0_), target->GetTranslation(), false));
  } else {
    mAnimationState.SetState(CAnimationState::kAS_Over);
  }
}

// @ fn_15_48BC
// The fist attack; its severity flips between 1 and 7 with 70% chance after each one.
void CElitePirate::MeleeAttack(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) fprintf(stderr, "[ep-probe] %s\n", "MeleeAttack");
  SetAttackKind(4, msg);
  TryCommand(msg, pas::kAS_MeleeAttack, CBCMeleeAttackCmd(pas::ESeverity(xc48_)));
  if (msg == kStateMsg_Activate) {
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    KnockBackController().EnableAnimReaction(CKnockBackMgr::kAR_Flinch, true);
    if (mgr.Random()->Float() < 0.7f) {
      xc4c_ = xc48_ == 1 ? 7 : 1;
    }
  } else if (msg == kStateMsg_Deactivate) {
    mMeleeDamagePending = false;
    mKnockPending = false;
    xc48_ = xc4c_;
    mAngryCount = 0;
    SetArmTouchBounds(mgr, CVector3f::Zero());
  }
}

// @ fn_15_50BC
// The missile salvo: a loop attack until data+0xb4..0xb8 (random) rockets are out, then exit.
void CElitePirate::ProjectileAttack(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) fprintf(stderr, "[ep-probe] %s\n", "ProjectileAttack");
  if (msg == kStateMsg_Activate) {
    mRocketsFired = 0;
    xcfc_ = mgr.Random()->Range(mData.mUnknown624222F8, mData.mUnknown31E43A1C);
    mRocketEventSeen = false;
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
  }
  SetAttackKind(2, msg);
  if (msg == kStateMsg_Deactivate) {
    ResetAttackTimer(mgr);
  }
  if (mRocketsFired < xcfc_) {
    if (const CActor* target = TargetActor(mgr, xa26_)) {
      xcc4_ = target->GetTranslation();
      BodyController()->CommandMgr().SetTargetVector(target->GetTranslation() - GetTranslation());
    }
    TryCommand(msg, pas::kAS_LoopAttack, CBCLoopAttackCmd(pas::kLAT_Zero, false));
  } else {
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
  }
}

// @ fn_15_4D18
// The four-rocket spread (FireRocket's pattern 1) as projectile attack severity 9.
void CElitePirate::SpreadShot(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) fprintf(stderr, "[ep-probe] %s\n", "SpreadShot");
  if (msg == kStateMsg_Activate) {
    mRocketsFired = 0;
    xcfc_ = 4;
    mRocketEventSeen = false;
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
  }
  SetAttackKind(8, msg);
  if (msg == kStateMsg_Deactivate) {
    ResetAttackTimer(mgr);
  }
  if (const CActor* target = TargetActor(mgr, xa26_)) {
    xcc4_ = target->GetTranslation();
    BodyController()->CommandMgr().SetTargetVector(target->GetTranslation() - GetTranslation());
  }
  TryCommand(msg, pas::kAS_ProjectileAttack,
             CBCProjectileAttackCmd(pas::ESeverity(9), xcc4_, false));
}

// @ fn_15_1900
// Chooses the angry attack: shock wave (xd24_80_, read by ShockwaveIsNext) by the player's hint
// (0x17 no, 0x16 yes), else 70% the first time and alternating after that.
void CElitePirate::AngryAttackBegin(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) fprintf(stderr, "[ep-probe] %s\n", "AngryAttackBegin");
  if (msg != kStateMsg_Activate) {
    return;
  }
  xd24_40_ = false;
  const int hint = GetPlayerHintType(mgr);
  if (hint == 0x17) {
    xd24_80_ = false;
  } else if (hint == 0x16) {
    xd24_80_ = true;
  } else if (!xd24_10_) {
    xd24_80_ = mgr.Random()->Range(0.f, 1.f) < 0.7f;
  } else {
    xd24_80_ = !xd24_20_;
  }
  xd24_10_ = true;
  xd24_20_ = xd24_80_;
}

// @ fn_15_18D8
void CElitePirate::AngryAttackEnd(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) fprintf(stderr, "[ep-probe] %s\n", "AngryAttackEnd");
  if (msg != kStateMsg_Activate) {
    return;
  }
  --mAngryCount;
  xd24_40_ = true; // AngryAttackOver
}

// @ fn_15_5A60
// Taunts cycle 0 -> 6 -> 1 -> 0; each one leaves 1..3 angry attacks to make.
void CElitePirate::Taunt(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) fprintf(stderr, "[ep-probe] %s\n", "Taunt");
  TauntCmd(msg, xd1c_);
  if (msg == kStateMsg_Activate) {
    xc10_01_ = true;
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
  } else if (msg == kStateMsg_Deactivate) {
    static const int skNextTaunt[4] = {0, 6, 1, 0};
    for (int i = 0; i < 3; ++i) {
      if (xd1c_ == skNextTaunt[i]) {
        xd1c_ = skNextTaunt[i + 1];
        break;
      }
    }
    mAngryCount = mgr.Random()->Range(1, 3);
    xd24_40_ = false;
    xd24_10_ = false;
  }
}

// @ fn_15_5F08
// Energy absorb: reflect while absorbing, remember where it powered up (x81c_, ShouldAlert),
// and send ATTK; the launcher restarts when it ends.
void CElitePirate::PowerUp(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) fprintf(stderr, "[ep-probe] %s\n", "PowerUp");
  SetAttackKind(6, msg);
  UpdateAbsorb(mgr, msg);
  BodyController()->SetLocomotionType(pas::kLT_Relaxed);
  if (msg == kStateMsg_Activate) {
    x81c_ = GetTranslation();
    SendScriptMsgs(kSS_Attack, mgr, kInvalidUniqueId, kSM_None);
  } else if (msg == kStateMsg_Deactivate) {
    KnockBackController().EnableAnimReaction(CKnockBackMgr::kAR_Flinch, true);
    SendLauncherMsg(mgr, true);
  }
}

// @ fn_15_5FF0
void CElitePirate::PowerDown(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) fprintf(stderr, "[ep-probe] %s\n", "PowerDown");
  SetAttackKind(5, msg);
  UpdateAbsorb(mgr, msg);
  BodyController()->SetLocomotionType(pas::ELocomotionType(mInitialAnimState));
  KnockBackController().EnableAnimReaction(CKnockBackMgr::kAR_Flinch, false);
}

// @ fn_15_7B00
void CElitePirate::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) fprintf(stderr, "[ep-probe] %s\n", "Dead");
  if (msg == kStateMsg_Activate) {
    ReleaseClaimedRegion(mgr);
    RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    if (CEntity* launcher = mgr.ObjectById(mLauncherId)) {
      launcher->SetActive(false);
    }
    if (mShieldSfx) {
      CSfxManager::RemoveEmitter(mShieldSfx);
    }
    mgr.DeleteObjectRequest(mLauncherId);
    mLauncherId = kInvalidUniqueId;
    KnockBackController().EnableKnockBackPhysics(false);
  }
  CPatterned::Dead(mgr, msg, dt);
}
