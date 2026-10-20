// Host body for the EmperorIngStage1 REL (module 16), milestone 1: the actor loads, stands and
// thinks. Every vtable override that belongs to retail's AI forwards to CPatterned behind a
// once-only StandIn log, and the REL function it stands in for is named on each. The state
// machine's tables are registered (fn_16_8AF4 installs the 6 triggers, 12 states and 3 code
// functions fn_16_BF88 builds), but the 21 functions they point at are stand-ins that log the
// retail function they stand in for and return the neutral value. The tentacles, the five attacks
// (melee, projectile, warp, beam, taunt) and the heart stage are milestone 2 - docs/module-
// analysis/EmperorIngStage1.md has the plan.
//
// Which methods the REL overrides is not guessed: its vtable lbl_16_data_3C8 lists them by
// address, and the slots line up with CPatterned's declaration order (the inherited entries are
// spelled out by name in the same table). The tiny accessors at the head of the module's .text
// (fn_16_B994, fn_16_B9A4, fn_16_B9AC, fn_16_B9B4, fn_16_B9BC, fn_16_B9CC, fn_16_B9C4, fn_16_B9DC,
// fn_16_B9E8, fn_16_B9F4 and fn_16_B994's neighbours in EmperorIngStage1Accessors.s) have
// four-to-twelve-instruction bodies, so they are real rather than stand-ins.
#include "MetroidPrime/Enemies/CEmperorIngStage1.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CParticleGenInfo.hpp"

#include "Kyoto/Animation/CPASAnimParm.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CPASDatabase.hpp"

#include "Kyoto/Text/CStringTable.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrEmperorIngStage1.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

#include <stdio.h>

namespace {

// Reports a stand-in the first time it runs, so a log lists what is still a shell.
void StandIn(const char* what) {
  static const char* sSeen[32] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == what) {
      return;
    }
  }
  if (sCount < 32) {
    sSeen[sCount++] = what;
  }
  fprintf(stderr, "[emp1] %s is a port stand-in (see PortCEmperorIngStage1.cpp)\n", what);
}

// The same, for the state machine's own functions: the message names the retail function the
// entry stands in for, so the log says which of the module's 21 FSM functions the run reached.
void StandInFn(const char* fn) {
  static const char* sSeen[32] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == fn) {
      return;
    }
  }
  if (sCount < 32) {
    sSeen[sCount++] = fn;
  }
  fprintf(stderr, "[emp1] stand-in %s\n", fn);
}

// The body's three FSM tables. Retail builds them in a static initialiser (fn_16_BF88, .ctors at
// 0x0000C2A4): the tables are lbl_16_data_19C (6 triggers, .data 0x19C), lbl_16_data_94 (12
// states, .data 0x94) and lbl_16_data_220 (3 codes, .data 0x220), each entry 16 bytes
// {name, 0, 0xFFFFFFFF, fn}, and the function pointers come from the three-word records
// {0, 0xFFFFFFFF, fn} in .data: the states' twelve at 0x4..0x88 (fn_16_BF88's stores at
// 0x0000C0C0..0x0000C124 and 0x0000C214), the triggers' six at 0x154..0x190 (0x0000C234..
// 0x0000C284) and the codes' three at 0x1FC..0x214 (0x0000C284..0x0000C294).
//
// On the host the table addresses are irrelevant: the state machine matches a node by its name,
// so the name strings are retail's and the entry's function is the module's member function.
// The members are cast the way the DigitalGuardian and SpacePirate ports do it.
#define EMP1_TRIGGER(name)                                                                          \
  { #name, reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CEmperorIngStage1::name) }
#define EMP1_STATE(name)                                                                            \
  { #name, reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CEmperorIngStage1::name) }
#define EMP1_CODE(name)                                                                             \
  { #name, reinterpret_cast< CPatterned::StateMachine::CodeFunc >(&CEmperorIngStage1::name) }

CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    EMP1_TRIGGER(AllTentaclesRetracted),
    EMP1_TRIGGER(HeartStageOver),
    EMP1_TRIGGER(IsDead),
    EMP1_TRIGGER(ShouldMeleeAttack),
    EMP1_TRIGGER(ShouldAttack),
    EMP1_TRIGGER(ShouldTaunt),
};

CPatterned::StateMachine::SStateFunction skStates[] = {
    EMP1_STATE(TrackPlayer),
    EMP1_STATE(BeginHeartStage),
    EMP1_STATE(HeartStage),
    EMP1_STATE(EndHeartStage),
    EMP1_STATE(Dead),
    EMP1_STATE(MeleeAttack),
    EMP1_STATE(Attack),
    EMP1_STATE(ProjectileAttack),
    EMP1_STATE(WarpAttack),
    EMP1_STATE(TauntAttack),
    EMP1_STATE(Taunt),
    EMP1_STATE(JumpRope),
};

CPatterned::StateMachine::SCodeFunction skCodes[] = {
    EMP1_CODE(BeginSubStage),
    EMP1_CODE(ResetTentacles),
    EMP1_CODE(ChooseRandomTentacleCount),
};

#undef EMP1_TRIGGER
#undef EMP1_STATE
#undef EMP1_CODE

} // namespace

// fn_16_900C. The base call is the retail one: EPatternedAI 0x0D (`li r4, 0xd` at 0x00009068),
// flavor 0 (`li r7, 0x0` at 0x0000905C), movement 1 (kMT_Flyer), collider 1 (kCT_One) and body 2
// (kBT_Restricted) - the three constants stored to the stack arguments at 0x00009060..0x00009070.
CEmperorIngStage1::CEmperorIngStage1(TUniqueId uid, const rstl::string& name,
                                     const CEntityInfo& info, const CTransform4f& xf,
                                     const CModelData& modelData,
                                     const CActorParameters& actParms,
                                     const CPatternedInfo& pInfo,
                                     const SLdrEmperorIngStage1Data& data)
: CPatterned(static_cast< EPatternedAI >(0x0d), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Flyer, kCT_One, kBT_Restricted, actParms)
, mHealthInfo(LdrToHealthInfo(data.tentacle.health))            // guest 0xAAC, fn_16_9AB0
, mNormalVulnerability(LdrToDamageVulnerability(                  // guest 0xACC, fn_16_98D0
      data.tentacle.normalVulnerability))
, mWarpVulnerability(LdrToDamageVulnerability(                    // guest 0xAFC, fn_16_98D0
      data.tentacle.warpAttackVulnerability))
, mMeleeVulnerability(LdrToDamageVulnerability(                   // guest 0xB2C, fn_16_98D0
      data.tentacle.meleeAttackVulnerability))
, mProjectileVulnerability(LdrToDamageVulnerability(              // guest 0xB5C, fn_16_98D0
      data.tentacle.projectileAttackVulnerability))
, mMeleeDamage(LdrToDamageInfo(data.melee.damage))               // guest 0xB8C
, mBeamDamage(LdrToDamageInfo(data.beamAttack.damage))            // guest 0xBC4
, mWarpStabDamage(LdrToDamageInfo(data.warp.stabDamage))         // guest 0xC10
, mHeartExposedTime(data.heartExposedTime)                        // guest 0xBA8
, mStayRetractedTime(data.tentacle.stayRetractedTime)            // guest 0xBB4
, mMaxTurnSpeedNormal(data.maxTurnSpeedNormal)                    // guest 0xBF0
, mMaxTurnSpeedMelee(data.maxTurnSpeedMelee)                      // guest 0xBF4
, mTurnSpeedAccel(data.turnSpeedAccel)                            // guest 0xBFC
, mTauntFrequency(data.tauntFrequency)                            // guest 0xC34
, mAttackIntervalMin(data.attackIntervalMin)                      // guest 0xCF4
, mAttackIntervalMax(data.attackIntervalMax)                      // guest 0xCF8
, x7c0_(0)                                                        // guest 0x7C0, fn_16_900C clears it
, x7c4_(0)                                                        // guest 0x7C4, cleared
, x7c8_(nullptr)                                                  // guest 0x7C8, cleared
, x7cc_(nullptr)                                                  // guest 0x7CC, cleared
, x7d0_(nullptr)                                                  // guest 0x7D0, cleared
, x7d4_(0)                                                        // guest 0x7D4, cleared
, x7d8_()                                                         // guest 0x7D8, the tentacle slots
, xa18_(kInvalidUniqueId)                                         // guest 0xA18, fn_16_900C clears it
, xa1a_(kInvalidUniqueId)                                         // guest 0xA1A, fn_16_900C clears it
, xa1c_(kInvalidUniqueId)                                         // guest 0xA1C, fn_16_900C clears it
, xa1e_(kInvalidUniqueId)                                         // guest 0xA1E, fn_16_900C clears it
, xa20_(-1)                                                   // guest 0xA20, fn_16_900C clears it
, xa24_(-1)                                                   // guest 0xA24, fn_16_900C clears it
, xa28_(-1)                                                   // guest 0xA28, fn_16_900C clears it
, xa3c_(0.f)                                                      // guest 0xA3C, rodata_380
, xa4c_(4.f)                                                      // guest 0xA4C, rodata_38C
, xa50_(0.f)                                                      // guest 0xA50, rodata_380
, xa58_(0.f)                                                      // guest 0xA58, rodata_380
, xa5c_(0.f)                                                      // guest 0xA5C, rodata_380
, xa78_(0.f)                                                      // guest 0xA78, rodata_380
, xa8c_(CSfxHandle::NullHandle())                                  // guest 0xA8C, cleared
, xa90_(kInvalidUniqueId)                                         // guest 0xA90, fn_16_900C clears it
{
  // The retail constructor stops the actor drawing a shadow (SetDrawShadow(this, 0) at
  // 0x000095E0) and calls the DOL's EnableAllAnimReactions(&this->0x620, 0) at 0x000095EC; the
  // port's CAiKnockBackMgr has no such call yet, so that half is missing.
  // Everything else the retail constructor builds - the shock-wave block at guest 0xC40, the two
  // tokens at guest 0xBB8 and 0xBE0, the projectile info at guest 0xC80 and the beam info at
  // guest 0xCA8 - belongs to the attacks, which this milestone does not enter.
  SetDrawShadow(false);
}

CEmperorIngStage1::~CEmperorIngStage1() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove / Death / KnockBack all dereference the current state. The analysis does not say
// where retail activates it, so the port activates on kSM_Create and, as the other patterned
// actors in this port do, again on demand in every entry point that can run first.
// The null test is a port limit, not retail's: CPatterned's constructor only builds the body
// controller when the patterned info has model data (CPatterned.cpp's HasModelData() test), so an
// actor without a model has no controller to activate.
void CEmperorIngStage1::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// vtable slot 3 = fn_16_56D8 (0x56D8..0x5928): retail's Think. The body only runs while the
// actor is active - the asm's gate on the byte at 0x20 (CEntity::mActive) at 0x5708 - and it
// counts the attack timer down (0x5734) and the taunt timer up (0x5740) around the base's own
// Think (the call at 0x5720).
void CEmperorIngStage1::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  UpdateTentacles(dt, mgr); // fn_16_3668, 0x5710
  // The retail constructor does not activate the body controller; the port does it here, as the
  // other patterned actors in this port do, because CPatterned::Think dereferences the body state.
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr); // 0x5720
  xa4c_ -= dt;                // 0x5734
  xa50_ += dt;                // 0x5740
  UpdateVoice(dt, mgr);       // fn_16_51D4, 0x5748
  UpdateTentacleAnchor(dt, mgr); // fn_16_4EB8, 0x5758
  UpdateCollisionActors(dt, mgr); // fn_16_3AFC, 0x5768
  UpdateTrackPlayer(dt, mgr); // fn_16_4C0C, 0x5778
  xa78_ -= dt;                // 0x578C
  // The effect state's name is the string lbl_16_rodata_340 points at (0x5780), and it is on
  // while the timer is above zero (the compare at 0x57B0).
  AnimationData()->SetEffectState(rstl::string_l("DarkProjectileCharge"), xa78_ > 0.f, mgr); // 0x57CC
  UpdateFacing(dt, mgr);      // fn_16_4C8, 0x57E4
  UpdateTentacleHits(dt, mgr); // fn_16_8C, 0x57F4
  // The emitter that follows the target (0x57F8..0x5900): the handle must be playing, the
  // target must exist, and the emitter sits on the target's momentum, pointing at the player.
  if (xa8c_ && xa90_ != kInvalidUniqueId && CSfxManager::IsPlaying(xa8c_)) {
    const CEntity* target = mgr.GetObjectById(xa90_);
    if (target != nullptr) {
      // The retail reads the target's position (CActor::mPosition) and its momentum
      // (CPhysicsActor::mMomentum) straight out of the object; the port needs the cast, and a
      // null here means the target is not a physics actor.
      const CPhysicsActor* targetActor = TCastToConstPtr<CPhysicsActor>(target);
      if (targetActor != nullptr) {
        const CVector3f& playerPos = mgr.GetPlayer(0)->GetTranslation();
        const CVector3f& targetPos = targetActor->GetTranslation();
        // The direction is the horizontal one: the asm zeroes the third component at 0x5890.
        CVector3f dir(playerPos.GetX() - targetPos.GetX(), playerPos.GetY() - targetPos.GetY(),
                      0.f);
        if (dir.CanBeNormalized()) {
          dir.Normalize();
          // The retail's float at the target's +0x1C0 is CPhysicsActor::mMomentum's X. The
          // retail's CActor is 0x158 bytes (include/MetroidPrime/CActor.hpp's
          // CHECK_SIZEOF(CActor, 0x158)), so the retail's CPhysicsActor starts at 0x158; the
          // port's CActor is 0x1B8 (the port's own presentation members), so the port's starts
          // 0x60 later. The member order in CPhysicsActor is upstream's, and mMomentum is the
          // member at the CPhysicsActor's +0x68 (the port's mMomentum is at 0x220 = 0x1B8 +
          // 0x68), so 0x158 + 0x68 = 0x1C0 is the same member in both.
          const float offset = targetActor->GetMomentumWR().GetX();
          const CVector3f emitterPos(targetPos.GetX() + dir.GetX() * offset,
                                     targetPos.GetY() + dir.GetY() * offset, targetPos.GetZ());
          CSfxManager::UpdateEmitter(xa8c_, emitterPos, dir, 0xFF);
        }
      }
    }
  }
}

// fn_16_3668 (0x3668..0x36E8): every live tentacle gets its per-frame update.
void CEmperorIngStage1::UpdateTentacles(float dt, CStateManager& mgr) {
  for (int i = 0; i < x7d4_; ++i) {
    UpdateTentacle(i, dt, mgr);
  }
}

// fn_16_2B0C (0x2B0C..0x2B68): the per-tentacle update - fn_16_26B8 (the tentacle's own work),
// fn_16_2524 (its motion) and fn_16_24AC (its vulnerability effect states), in that order.
void CEmperorIngStage1::UpdateTentacle(int index, float dt, CStateManager& mgr) {
  UpdateTentacleCore(index);
  UpdateTentacleMotion(index, dt, mgr);
  SetVulnerabilityEffects(index);
}

// fn_16_26B8 (0x26B8..0x2A68): the tentacle's own update. Too big for the slice's rule, so it
// stays a stand-in.
void CEmperorIngStage1::UpdateTentacleCore(int index) { StandInFn("fn_16_26B8"); }

// fn_16_2524 (0x2524..0x26B8): the tentacle's motion update. Too big for the slice's rule, so it
// stays a stand-in.
void CEmperorIngStage1::UpdateTentacleMotion(int index, float dt, CStateManager& mgr) {
  StandInFn("fn_16_2524");
}

// fn_16_24AC (0x24AC..0x2524): the tentacle's two vulnerability effect states - the strings
// lbl_16_rodata_344 and lbl_16_rodata_348 point at ("DarkVulnerability" and
// "LightVulnerability"), each handed to fn_16_2070.
void CEmperorIngStage1::SetVulnerabilityEffects(int index) {
  SetTentacleEffectState(index, rstl::string_l("DarkVulnerability"));
  SetTentacleEffectState(index, rstl::string_l("LightVulnerability"));
}

// fn_16_2070 (0x2070..0x2204): the effect state a tentacle carries. Too big for the slice's rule,
// so it stays a stand-in.
void CEmperorIngStage1::SetTentacleEffectState(int index, const rstl::string& name) {
  StandInFn("fn_16_2070");
}

// fn_16_302C (0x302C..0x3060): the actor the module tracks - the id at guest 0xA1A, looked up in
// the state manager and cast to an actor.
CActor* CEmperorIngStage1::GetTargetActor(CStateManager& mgr) const {
  return TCastToPtr< CActor >(mgr.ObjectById(xa1a_));
}

// fn_16_2FA8 (0x2FA8..0x2FF8): the collision actor of the second tentacle's collision actor
// manager (guest 0x7CC): its first joint description's collision actor id, looked up in the state
// manager.
CCollisionActor* CEmperorIngStage1::GetCollisionActor(CStateManager& mgr) const {
  if (x7cc_ == nullptr) {
    // The retail has no test here: the manager is set by the tentacle spawn code, which the port
    // does not have yet.
    return nullptr;
  }
  const TUniqueId id = x7cc_->GetCollisionDescFromIndex(0).GetCollisionActorId();
  return TCastToPtr< CCollisionActor >(mgr.ObjectById(id));
}

// fn_16_4E44 (0x4E44..0x4E68): takes the particle effect's reference - the two pointers at the
// effect's +0x78 and +0x7C and the counter at the second one's +0x00. The counter lives in the
// retail particle effect, which the port does not have, so it stays a stand-in.
void CEmperorIngStage1::AcquireParticleEffect(CParticleGenInfo* effect) {
  StandInFn("fn_16_4E44");
}

// fn_16_BDD0 (0xBDD0..0xBE34): releases the reference the effect update took (it drops the
// counter and frees the reference when it reaches zero). The counter lives in the retail particle
// effect, which the port does not have, so it stays a stand-in.
void CEmperorIngStage1::ReleaseParticleEffect(CParticleGenInfo* effect) {
  StandInFn("fn_16_BDD0");
}

// fn_16_6D10 (0x6D10..0x6D94): the effect's own update, with the scale the caller computed.
// Too big for the slice's rule, so it stays a stand-in.
void CEmperorIngStage1::UpdateParticleEffect(CParticleGenInfo* effect, float scale) {
  StandInFn("fn_16_6D10");
}

// fn_16_51D4 (0x51D4..0x52FC): the module's own sound update. It reads a float at the damage
// vulnerability's +4 (0x5214) - the retail's CDamageVulnerability is 0x54 bytes and the port's is
// 0x38 (upstream's layout), so the port has no float there and the value's meaning is not
// established. It stays a stand-in until that is.
void CEmperorIngStage1::UpdateVoice(float dt, CStateManager& mgr) { StandInFn("fn_16_51D4"); }

// fn_16_4EB8 (0x4EB8..0x51D4): moves the actor onto the target's locator. Too big for the
// slice's rule, so it stays a stand-in.
void CEmperorIngStage1::UpdateTentacleAnchor(float dt, CStateManager& mgr) {
  StandInFn("fn_16_4EB8");
}

// fn_16_3AFC (0x3AFC..0x3B70): the three collision actor managers get their update (the calls
// at 0x3B28, 0x3B3C and 0x3B50). The retail has no null test: the managers are set by the
// tentacle spawn code, which the port does not have yet, so on the host they are still null and
// the update is skipped.
void CEmperorIngStage1::UpdateCollisionActors(float dt, CStateManager& mgr) {
  if (x7c8_ != nullptr) {
    x7c8_->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  }
  if (x7cc_ != nullptr) {
    x7cc_->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  }
  if (x7d0_ != nullptr) {
    x7d0_->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  }
}

// fn_16_4C0C (0x4C0C..0x4DF0): the module's track-player update. It looks the tracked actor up
// (fn_16_302C), and for the "Damage" and "Telegraph" effect states it finds the actor's first
// particle effect of that name, hands it to the effect update (fn_16_4E44, fn_16_BDD0 and
// fn_16_6D10) and turns the effect state on (SetEffectState at 0x4CF0 and 0x4DB0). The two
// timers (guest 0xA58 and 0xA5C) count down between and after the blocks (0x4D0C and 0x4DC0).
void CEmperorIngStage1::UpdateTrackPlayer(float dt, CStateManager& mgr) {
  CActor* target = GetTargetActor(mgr);
  if (target == nullptr) {
    return;
  }
  CParticleGenInfo* damage = target->AnimationData()->GetFirstParticleEffect(
      rstl::string_l("Damage"));
  if (damage != nullptr) {
    AcquireParticleEffect(damage);
    ReleaseParticleEffect(damage);
    UpdateParticleEffect(damage, xa58_ > 0.f ? 1.f : 0.f);
  }
  target->AnimationData()->SetEffectState(rstl::string_l("Damage"), true, mgr);
  xa58_ -= dt;
  CParticleGenInfo* telegraph = target->AnimationData()->GetFirstParticleEffect(
      rstl::string_l("Telegraph"));
  if (telegraph != nullptr) {
    AcquireParticleEffect(telegraph);
    ReleaseParticleEffect(telegraph);
    UpdateParticleEffect(telegraph, xa5c_ > 0.f ? 1.f : 0.f);
  }
  target->AnimationData()->SetEffectState(rstl::string_l("Telegraph"), true, mgr);
  xa5c_ -= dt;
}

// fn_16_4C8 (0x4C8..0x830): aims the actor at the player. Too big for the slice's rule, so it
// stays a stand-in.
void CEmperorIngStage1::UpdateFacing(float dt, CStateManager& mgr) { StandInFn("fn_16_4C8"); }

// fn_16_8C (0x8C..0x1A50): the tentacles' hit test and damage. Too big for the slice's rule, so
// it stays a stand-in.
void CEmperorIngStage1::UpdateTentacleHits(float dt, CStateManager& mgr) {
  StandInFn("fn_16_8C");
}

// vtable slot 4 = fn_16_7CE8 (0x7CE8..0x7EA8): retail's message switch. The base's own handler
// runs first (the call at 0x7D14) and then the message decides:
//   kSM_Create (0x58435254, compared at 0x7D24) activates the body controller (0x7D78);
//   kSM_Delete (0x5844454C, at 0x7D64) destroys the collision actors (0x7D88);
//   kSM_AreaLoaded (0x58414C44, at 0x7D3C) looks the three connected objects up and sets the
//     tentacle work up (0x7DAC..0x7E28);
//   kSM_Activate (0x41435456, at 0x7D50) hands the boss bar its id, health and name (0x7E80).
void CEmperorIngStage1::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CPatterned::AcceptScriptMsg(mgr, msg);
  switch (msg.GetMessage()) {
    case kSM_Create: {
      // The body controller is the member at guest 0x48C.
      BodyController()->Activate(mgr, pas::kAS_Invalid);
      break;
    }
    case kSM_Delete: {
      DestroyCollisionActors(mgr);
      break;
    }
    case kSM_AreaLoaded: {
      xa18_ = FindConnectedObject(mgr, kSS_InternalState3, kSM_Activate);
      // The retail's second lookup asks for the message 0x41574348, which the port's
      // EScriptObjectMessage has no name for (its kSM_Attach is 0x41544348), so the value is
      // used as it is.
      xa1a_ = FindConnectedObject(mgr, kSS_Connect,
                                 static_cast< EScriptObjectMessage >(0x41574348u));
      xa1c_ = FindConnectedObject(mgr, kSS_Connect, kSM_Activate);
      SetupTentacleAnimations(mgr);
      SetupTentacleCollisions(mgr);
      SetupTentacleJoints(mgr);
      break;
    }
    case kSM_Activate: {
      // The boss bar: the health info's initial health (the vtable slot 13 call at 0x7E40 and the
      // float at +0x00 of the result) and the string table index of the boss's name - the string
      // lbl_16_rodata_41C+0x670 points at, "BossTempleBossStage1".
      const float health = GetHealthInfo()->GetInitialHP();
      const int index = gpStringTable->GetStringIndex("BossTempleBossStage1");
      mgr.SetBossParams(GetUniqueId(), health, static_cast< uint >(index));
      break;
    }
  }
}

// fn_16_3B70 (0x3B70..0x3BC4): the three collision actor managers are destroyed. The retail has
// no null test: the managers are set by the tentacle spawn code, which the port does not have
// yet, so on the host they are still null.
void CEmperorIngStage1::DestroyCollisionActors(CStateManager& mgr) {
  if (x7c8_ != nullptr) {
    x7c8_->Destroy(mgr);
  }
  if (x7cc_ != nullptr) {
    x7cc_->Destroy(mgr);
  }
  if (x7d0_ != nullptr) {
    x7d0_->Destroy(mgr);
  }
}

// fn_16_30C8 (0x30C8..0x3340): the tentacle animation ids. The tracked actor is the one
// connected on CONNECT/'PLAY' (0x504C4159, looked up at 0x3100 and stored at guest 0xA1E at
// 0x3110); its animation database is the model's animation data at +0x3C (0x3118..0x3124). Three
// parameter sets are searched for the best animation and the ids land at guest 0xA20 (0x31CC, the
// set with FromEnum(0) and FromEnum(0)), 0xA24 (0x3278, FromEnum(0) and FromEnum(7)) and 0xA28
// (0x3324, FromEnum(0) and FromEnum(6)). An id is only taken when the search found a time above
// 1.1920929e-07f (lbl_16_rodata_3A8).
void CEmperorIngStage1::SetupTentacleAnimations(CStateManager& mgr) {
  // The port's EScriptObjectMessage has no name for the retail's 0x504C4159, so the raw value is
  // used.
  xa1e_ = FindConnectedObject(mgr, kSS_Connect,
                              static_cast< EScriptObjectMessage >(0x504C4159u));
  // fn_16_3094 (0x3094..0x30C8): that actor. The retail has no test here - the actor connected
  // on CONNECT/'PLAY' exists by the time the area is loaded - but on the host the connection may
  // not be there yet.
  const CActor* actor = GetPlayActor(mgr);
  if (actor == nullptr) {
    return;
  }
  const CPASDatabase& db = actor->GetModelData()->GetAnimationData()->GetPASDatabase();
  // The first set (0x3128..0x319C): six NoParameter, then FromEnum(0) and FromEnum(0).
  const CPASAnimParmData first(pas::EAnimationState(5), CPASAnimParm::FromEnum(0),
                               CPASAnimParm::FromEnum(0), CPASAnimParm::NoParameter(),
                               CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                               CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                               CPASAnimParm::NoParameter());
  const rstl::pair< float, int > found = db.FindBestAnimation(first, -1);
  if (found.first > 1.1920929e-07f) {
    xa20_ = found.second;
  }
  // The second set (0x31D0..0x3258): six NoParameter, then FromEnum(0) and FromEnum(7).
  const CPASAnimParmData second(pas::EAnimationState(5), CPASAnimParm::FromEnum(7),
                                CPASAnimParm::FromEnum(0), CPASAnimParm::NoParameter(),
                                CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                                CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                                CPASAnimParm::NoParameter());
  const rstl::pair< float, int > found2 = db.FindBestAnimation(second, -1);
  if (found2.first > 1.1920929e-07f) {
    xa24_ = found2.second;
  }
  // The third set (0x3280..0x3308): six NoParameter, then FromEnum(0) and FromEnum(6).
  const CPASAnimParmData third(pas::EAnimationState(5), CPASAnimParm::FromEnum(6),
                               CPASAnimParm::FromEnum(0), CPASAnimParm::NoParameter(),
                               CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                               CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                               CPASAnimParm::NoParameter());
  const rstl::pair< float, int > found3 = db.FindBestAnimation(third, -1);
  if (found3.first > 1.1920929e-07f) {
    xa28_ = found3.second;
  }
}

// fn_16_3094 (0x3094..0x30C8): the actor connected on CONNECT/'PLAY' (the id at guest 0xA1E).
CActor* CEmperorIngStage1::GetPlayActor(CStateManager& mgr) const {
  return TCastToPtr< CActor >(mgr.ObjectById(xa1e_));
}

// fn_16_3EAC (0x794): creates the three CCollisionActorManagers (rs_new + the manager's
// constructor with a joint description vector), gives their collision actors the damage
// vulnerability and adds the materials. Too big for the slice's rule, so it stays a stand-in.
void CEmperorIngStage1::SetupTentacleCollisions(CStateManager& mgr) {
  StandInFn("fn_16_3EAC");
}

// fn_16_36E8 (0x36E8..0x3984): the tentacle joint collision descriptions. For each of the eight
// tentacle slots (the loop at 0x395C..0x3964) three parameter sets are searched for the best
// animation - the first with FromEnum(0) and FromInt32(the index), the second with FromEnum(2)
// and FromInt32(2), the third with FromEnum(1) and FromInt32(the index) - and when the first two
// found an animation (the tests at 0x3918 and 0x3920) the joint is built and added to the vector at
// guest 0x7D4 (0x3928..0x3958).
void CEmperorIngStage1::SetupTentacleJoints(CStateManager& mgr) {
  // The database is this actor's own model's animation data at +0x3C (0x3708..0x3728).
  const CPASDatabase& db = GetModelData()->GetAnimationData()->GetPASDatabase();
  for (int index = 0; index < 8; ++index) {
    const CPASAnimParmData first(pas::EAnimationState(27), CPASAnimParm::FromEnum(0),
                                 CPASAnimParm::FromInt32(index), CPASAnimParm::NoParameter(),
                                 CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                                 CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                                 CPASAnimParm::NoParameter());
    const rstl::pair< float, int > found = db.FindBestAnimation(first, -1);
    int animId1 = found.first > 1.1920929e-07f ? found.second : -1;
    const CPASAnimParmData second(pas::EAnimationState(27), CPASAnimParm::FromEnum(2),
                                 CPASAnimParm::FromInt32(2), CPASAnimParm::NoParameter(),
                                 CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                                 CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                                 CPASAnimParm::NoParameter());
    const rstl::pair< float, int > found2 = db.FindBestAnimation(second, -1);
    int animId2 = found2.first > 1.1920929e-07f ? found2.second : -1;
    const CPASAnimParmData third(pas::EAnimationState(27), CPASAnimParm::FromEnum(1),
                                 CPASAnimParm::FromInt32(index), CPASAnimParm::NoParameter(),
                                 CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                                 CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                                 CPASAnimParm::NoParameter());
    const rstl::pair< float, int > found3 = db.FindBestAnimation(third, -1);
    int animId3 = found3.first > 1.1920929e-07f ? found3.second : -1;
    if (animId1 != -1 && animId2 != -1) {
      BuildTentacleJoint(index, animId1, animId2, animId3);
      AddTentacleJoint(mgr);
      AddTentacleJointToVector();
    }
  }
}

// fn_16_2B68 (0x2B68..0x2C3C): the joint description is filled in - the owner (0x2B6C), the
// radius (0.2B88, 0.0f from lbl_16_rodata_380), the index and the three animation ids
// (0x2B98..0x2BA4), the time and the damage hit time (0x2BA8, 0x2BAC) and the speed (0x2BB0),
// the flag byte with its six bits cleared (0x2BB4..0x2BF8), the state (0x2BFC), the id
// (0x2C04, kInvalidUniqueId) and the zero and up vectors (0x2C08..0x2C34).
void CEmperorIngStage1::BuildTentacleJoint(int index, int animId1, int animId2, int animId3) {
  mJoint.mOwner = this;
  mJoint.mRadius = 0.f;
  mJoint.mIndex = index;
  mJoint.mAnimId1 = animId1;
  mJoint.mAnimId2 = animId2;
  mJoint.mAnimId3 = animId3;
  mJoint.mTime = 0.f;
  mJoint.mDamageHitTime = CPatterned::skDamageHitTime;
  mJoint.mSpeed = 0.f;
  // The flag byte keeps its low two bits: the retail clears 0x80 (0x2BB4), 0x40 (0x2BC0), 0x20
  // (0x2BCC), 0x10 (0x2BD8), 0x08 (0x2BE4) and 0x04 (0x2BF0) one bit at a time.
  mJoint.mFlags &= 0x03;
  mJoint.mState = 0;
  mJoint.mId = kInvalidUniqueId;
  mJoint.mPosition = CVector3f::Zero();
  mJoint.mUp = CVector3f::Up();
}

// fn_16_1E6C (0x1E6C..0x1F34): the tentacle actor gets the module's health info (the copy at
// 0x1EB8..0x1F04 takes the module's guest 0xAAC..0xAC8) and then its materials (fn_16_1F34 at
// 0x1F14).
void CEmperorIngStage1::AddTentacleJoint(CStateManager& mgr) {
  CCollisionActor* actor = GetTentacleActor(mgr, mJoint.mIndex);
  if (actor != nullptr) {
    *actor->HealthInfo() = mHealthInfo;
  }
  UpdateTentacleMaterials(mgr, 1);
}

// fn_16_1F34 (0x1F34..0x1FC0): the tentacle actor's materials - kMT_Target (0x28), kMT_Orbit
// (0x29), kMT_SeekerTarget (0x3F) and kMT_ExcludeFromRadar (0x39) are added when the flag's low
// byte is not zero (the asm's clrlwi at 0x1F6C keeps the low byte of the flag) - 0x1F74..0x1F8C -
// and removed when it is (0x1F90).
void CEmperorIngStage1::UpdateTentacleMaterials(CStateManager& mgr, uint flag) {
  CCollisionActor* actor = GetTentacleActor(mgr, mJoint.mIndex);
  if (actor == nullptr) {
    return;
  }
  if ((flag & 0xff) != 0) {
    actor->AddMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, kMT_ExcludeFromRadar, mgr);
  } else {
    actor->RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, kMT_ExcludeFromRadar, mgr);
  }
}

// fn_16_3984 (0x3984..0x39CC): the joint is added to the vector at guest 0x7D4 - the element is
// written (0x39A8) and the count is raised (0x39B4). The retail's vector grows (fn_16_39CC,
// 0xDC); the port's holds the eight tentacle slots, so it never has to.
void CEmperorIngStage1::AddTentacleJointToVector() {
  GrowTentacleJointVector();
  mJoints.mJoints[mJoints.mCount] = mJoint;
  mJoints.mCount += 1;
}

// fn_16_39CC (0x39CC..0x3AA8): the vector's growth. Too big for the slice's rule, so it stays a
// stand-in - the port's vector holds the eight tentacle slots, so it never has to grow.
void CEmperorIngStage1::GrowTentacleJointVector() { StandInFn("fn_16_39CC"); }

// fn_16_3AA8 (0x3AA8..0x3AFC): the tentacle's collision actor - the owner's first collision actor
// manager (guest 0x7C8) holds five joint descriptions per tentacle (the index is scaled by five
// at 0x3AB4), the description's collision actor id (at +0x3C) is looked up in the state manager
// and cast to a collision actor. Too big for the slice's rule, so it stays a stand-in.
CCollisionActor* CEmperorIngStage1::GetTentacleActor(CStateManager& mgr, int index) {
  StandInFn("fn_16_3AA8");
  return nullptr;
}

// vtable slot 7 = fn_16_52FC (0x20): the retail override is the base class's own PreRender.
void CEmperorIngStage1::PreRender(CStateManager& mgr) { CPatterned::PreRender(mgr); }

// vtable slot 9 = fn_16_531C (0x2B8): retail's render path, which sets up the actor's own
// model state before the base draws it.
void CEmperorIngStage1::Render(const CStateManager& mgr) const {
  StandIn("CEmperorIngStage1::Render (vtable slot 9, fn_16_531C)");
  CPatterned::Render(mgr);
}

// vtable slot 26 = fn_16_7F8C (0x2C4): retail's user-event switch, which drives the tentacles
// and the heart.
void CEmperorIngStage1::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                        EUserEventType type, float dt) {
  StandIn("CEmperorIngStage1::DoUserAnimEvent (vtable slot 26, fn_16_7F8C)");
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

// vtable slot 28 = fn_16_7EA8 (0xC4): retail's own sound handling for the boss.
void CEmperorIngStage1::ProcessSoundEvent(int sfxId, float weight, int flags, float fallOff,
                                          float maxDist, const CSegId& locator, ushort pitchStart,
                                          ushort pitchEnd, float pitchDuration, uchar minVol,
                                          uchar maxVol, float distanceSquared,
                                          const CVector3f& position, int aid, CStateManager& mgr,
                                          bool translateId) {
  StandIn("CEmperorIngStage1::ProcessSoundEvent (vtable slot 28, fn_16_7EA8)");
  CPatterned::ProcessSoundEvent(sfxId, weight, flags, fallOff, maxDist, locator, pitchStart,
                                pitchEnd, pitchDuration, minVol, maxVol, distanceSquared, position,
                                aid, mgr, translateId);
}

// vtable slot 40 = fn_16_B9FC: `li r3, 0x1` - the actor can be shot.
bool CEmperorIngStage1::CanBeShot(const CStateManager&, int) { return true; }

// vtable slot 41 = fn_16_BA04: `li r3, 0x0` - the actor does not listen for noises.
bool CEmperorIngStage1::IsListening() const { return false; }

// vtable slot 58 = fn_16_8AF4 (0x8AF4..0x8B80): CPatterned's own setup first (the call at
// 0x00008B08), then the module's three name tables are handed to the state machine at this+0x350
// - the triggers (lbl_16_data_19C, 6 entries) through the state manager's vtable slot +0x18 (the
// call at 0x00008B20), the states (lbl_16_data_94, 12 entries) through +0x14 (0x00008B40) and
// the codes (lbl_16_data_220, 3 entries) through +0x1C (0x00008B60).
void CEmperorIngStage1::SetupStateMachine(CStateManager& mgr) {
  CPatterned::SetupStateMachine(mgr);
  StateMachine* stateMachine = mStateMachine.get();
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  // The host's FSM1 does not dispatch code nodes yet (TStateMachineState::SetCodeFunctions is
  // empty), so this only records the table retail registers.
  stateMachine->SetCodeFunctions(skCodes, ARRAY_SIZE(skCodes));
}

// ---------------------------------------------------------------------------------------------
// The body's FSM functions. The four triggers and the two codes the run reaches are retail's
// bodies; the rest are stand-ins that log the retail function they stand in for and return the
// neutral value (false for a trigger, nothing for a state or a code).

// fn_16_4BD4 (the trigger "AllTentaclesRetracted", lbl_16_data_19C entry 0): true while every
// live tentacle's state is 0, i.e. while they are all retracted. The loop runs over the live
// tentacles (the count at guest 0x7D4) and the state is the element's int at +0x28.
bool CEmperorIngStage1::AllTentaclesRetracted(CStateManager& mgr, const CTriggerData& data) const {
  for (int i = 0; i < x7d4_; ++i) {
    if (x7d8_[i].x28_ != 0) {
      return false;
    }
  }
  return true;
}

// fn_16_4BB4 (the trigger "HeartStageOver", lbl_16_data_19C entry 1): the heart-stage timer
// (guest 0xA3C, 0.0f in the constructor) has run out. The asm compares it with 0.0f
// (lbl_16_rodata_380) and folds "less than" and "equal" into one bit (cror eq,lt,eq at 0x4BC4),
// so the test is "<= 0.0f".
bool CEmperorIngStage1::HeartStageOver(CStateManager& mgr, const CTriggerData& data) const {
  return xa3c_ <= 0.f;
}

// fn_16_4B6C (the trigger "IsDead", lbl_16_data_19C entry 2): the actor's health has run out.
// The asm calls the vtable slot at +0x3C (slot 13, GetHealthInfo) and reads the health info's
// second float (CHealthInfo::mHealth at +0x04), then compares it with 0.0f the same way as
// fn_16_4BB4 (cror eq,lt,eq at 0x4B98).
bool CEmperorIngStage1::IsDead(CStateManager& mgr, const CTriggerData& data) const {
  return GetHealthInfo()->GetHP() <= 0.f;
}

// fn_16_4B20 (the trigger "ShouldMeleeAttack", lbl_16_data_19C entry 3): the squared
// horizontal distance to the player against 900 (30 units). The player is the state manager's
// first player actor (retail CStateManager+0x14FC, mPlayers[0]) and the position is the
// actor's own (CActor::mPosition at +0x54, so the two floats are X and Y).
bool CEmperorIngStage1::ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const {
  const CVector3f& mine = GetTranslation();
  const CVector3f& theirs = mgr.GetPlayer(0)->GetTranslation();
  const float dx = mine.GetX() - theirs.GetX();
  const float dy = mine.GetY() - theirs.GetY();
  return dx * dx + dy * dy < 900.f;
}

// fn_16_4B04 (the trigger "ShouldAttack", lbl_16_data_19C entry 4): the attack timer (guest
// 0xA4C, 4.0f in the constructor) has run below zero.
bool CEmperorIngStage1::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  return xa4c_ < 0.f;
}

// fn_16_4AEC (the trigger "ShouldTaunt", lbl_16_data_19C entry 5): the taunt timer (guest
// 0xA50, 0.0f in the constructor) is past the taunt frequency (guest 0xC34).
bool CEmperorIngStage1::ShouldTaunt(CStateManager& mgr, const CTriggerData& data) const {
  return xa50_ > mTauntFrequency;
}

// The 12 states of lbl_16_data_94 (.data 0x94; fn_16_BF88's stores at 0x0000C0C0..0x0000C124 and
// 0x0000C214).

// fn_16_7C28 (the state "TrackPlayer", lbl_16_data_94 entry 0): retail's body calls three
// functions that are too big for this slice - fn_16_830 (0xB4, the IS0/IS1/IS2 message),
// fn_16_353C (0x12C, the attack vulnerability and the tentacle effect states, which itself
// calls fn_16_1E20 at 0x214) and fn_16_7B40 (0xE8, the look-at-the-player helper) - so it
// stays a stand-in here.
void CEmperorIngStage1::TrackPlayer(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_7C28");
}

void CEmperorIngStage1::BeginHeartStage(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_78AC");
}

void CEmperorIngStage1::HeartStage(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_774C");
}

void CEmperorIngStage1::EndHeartStage(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_71BC");
}

// fn_16_716C (the state "Dead", lbl_16_data_94 entry 4): on the activate message - the asm's
// `cmpwi r5, 0` at 0x7174, the message is the second argument - the script is told that the
// actor died (the state tag 0x44454144, 'DEAD', at 0x7198).
void CEmperorIngStage1::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    SendScriptMsgs(kSS_Dead, mgr, kInvalidUniqueId, kSM_None);
  }
}

void CEmperorIngStage1::MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_6F28");
}

void CEmperorIngStage1::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_6D94");
}

void CEmperorIngStage1::ProjectileAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_68F4");
}

void CEmperorIngStage1::WarpAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_601C");
}

void CEmperorIngStage1::TauntAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_5DBC");
}

void CEmperorIngStage1::Taunt(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_5C4C");
}

void CEmperorIngStage1::JumpRope(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_5928");
}

// fn_16_4AE8 (the code "BeginSubStage", lbl_16_data_220 entry 0): retail's body is a single
// `blr` - the sub-stage itself is the state machine's business.
void CEmperorIngStage1::BeginSubStage(CStateManager& mgr, float dt) {}

// fn_16_1DFC: the tentacle's timer is cleared and its state becomes 1, unless it is already 2.
static void ResetTentacle(CEmperorIngStage1Tentacle& tentacle) {
  tentacle.x4_ = 0.f;
  if (tentacle.x28_ != 2) {
    tentacle.x28_ = 1;
  }
}

// fn_16_4A04 (the code "ResetTentacles", lbl_16_data_220 entry 1): every tentacle slot of the
// current sub-stage is reset (fn_16_1DFC clears its timer and sets its state to 1 unless it is
// already 2) and gets the retraction timer, then the live tentacles' flags are set (fn_16_2EE0)
// and the script is told which sub-stage this is.
void CEmperorIngStage1::ResetTentacles(CStateManager& mgr, float dt) {
  const int count = TentacleCount();
  for (int i = 0; i < count; ++i) {
    ResetTentacle(x7d8_[i]);
    x7d8_[i].x18_ = mStayRetractedTime;
  }
  SetTentacleFlags(1);
  // The switch at 0x4A60..0x4A74: 2 sends IS07, 1 sends IS06, every other value sends nothing.
  if (x7c0_ == 2) {
    SendScriptMsgs(kSS_InternalState7, mgr, kInvalidUniqueId, kSM_None);
  } else if (x7c0_ == 1) {
    SendScriptMsgs(kSS_InternalState6, mgr, kInvalidUniqueId, kSM_None);
  }
}

// fn_16_4954 (the code "ChooseRandomTentacleCount", lbl_16_data_220 entry 2): the sub-stage
// (guest 0x7C4) is picked from a roll of 0..100 - the state manager's random (retail
// CStateManager+0x16E4) asked for Range(0.0f, 100.0f) at 0x497C, with 0.0f from
// lbl_16_rodata_380 and 100.0f from lbl_16_rodata_3D4 - against 50.0f (lbl_16_rodata_3BC): in
// mode 0 the roll picks 0 or 1, in mode 1 it picks 1 or 2, and every other mode takes 2.
void CEmperorIngStage1::ChooseRandomTentacleCount(CStateManager& mgr, float dt) {
  const float roll = mgr.Random()->Range(0.f, 100.f);
  if (x7c0_ == 1) {
    x7c4_ = roll >= 50.f ? 2 : 1;
  } else if (x7c0_ >= 1) {
    // The asm's `bge` at 0x498C: the mode is 2 or more here, because 1 was taken above.
    x7c4_ = 2;
  } else if (x7c0_ >= 0) {
    x7c4_ = roll >= 50.f ? 1 : 0;
  } else {
    x7c4_ = 2;
  }
}

// fn_16_2F70: how many tentacle slots the current sub-stage uses - four, six or eight. The
// sub-stage is the int at guest 0x7C4.
int CEmperorIngStage1::TentacleCount() const {
  if (x7c4_ == 1) {
    return 6;
  }
  if (x7c4_ >= 1) {
    return 8;
  }
  if (x7c4_ >= 0) {
    return 4;
  }
  return 8;
}

// fn_16_2EE0: every live tentacle's flag byte (guest +0x24) gets its 0x80 bit set when the
// argument is non-zero and the tentacle is one of the current sub-stage's slots; the others get
// it cleared.
void CEmperorIngStage1::SetTentacleFlags(uint flag) {
  const bool set = (flag & 0xff) != 0;
  const int count = TentacleCount();
  for (int i = 0; i < x7d4_; ++i) {
    if (set && i < count) {
      x7d8_[i].x24_ |= 0x80;
    } else {
      x7d8_[i].x24_ &= ~0x80;
    }
  }
}

// ---------------------------------------------------------------------------------------------
// The module's loader (fn_16_A2A0) and entry points.

static CEntity* PortLoadEmperorIngStage1(CStateManager& mgr, CInputStream& input,
                                         CEntityInfo& info) {
  SLdrEmperorIngStage1 sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrEmperorIngStage1.inc"

  // The REL builds the model with LdrToModelData(scale, kInvalidAssetId, the patterned info's
  // animation set, true) at 0x0000A41C and moves the result into the object at 0x0000A434.
  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL reads the copy's valid byte (sp+0xF8 at 0x0000A440) and branches to its null
    // return at 0x0000A448 when the model is empty.
    return nullptr;
  }

  // The REL's own allocation is rs_new(0xD18, "CEmperorIngStage1") at 0x0000A4B8; the port
  // allocates sizeof(CEmperorIngStage1), which is larger because the host layout is its own.
  return rs_new CEmperorIngStage1(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                  LdrToEntityInfo(info, sldrThis.editorProperties),
                                  LdrToTransform4f(sldrThis.editorProperties), *modelData,
                                  LdrToActorParameters(sldrThis.actorInformation),
                                  LdrToPatternedInfo(sldrThis.patterned, nullptr), sldrThis.data);
}

static SEmperorIngStage1_FuncPtrs REL_loader_EmperorIngStage1;

// Retail's RELMain calls fn_16_A270, which stores the loader fn_16_A2A0 into lbl_16_bss_0 (the
// module's SEmperorIngStage1_FuncPtrs) and calls the DOL's SetSEmperorIngStage1_FuncPtrs. The
// proxy in SEmperorIngStage1_FuncPtrs.cpp calls through it.
void RELMain() {
  REL_loader_EmperorIngStage1.mLoader = PortLoadEmperorIngStage1;
  SetSEmperorIngStage1_FuncPtrs(&REL_loader_EmperorIngStage1);
}

void RELExit() { SetSEmperorIngStage1_FuncPtrs(nullptr); }
