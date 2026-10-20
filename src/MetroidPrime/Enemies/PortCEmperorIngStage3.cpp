// Host body for the EmperorIngStage3 REL (module 18), milestone 1: the mutated Emperor Ing
// loads, stands and thinks. Every vtable override that belongs to retail's AI forwards to
// CPatterned behind a once-only StandIn log, and the REL function it stands in for is named on
// each. The state machine (fn_18_943C installs the 21 triggers, 21 states and 2 code functions),
// the four attacks (melee, stampede, jump slide, ground pound), the two beams, the portal attack
// and the light swarm are milestone 2 - docs/module-analysis/EmperorIngStage3.md has the plan.
//
// Which methods the REL overrides is not guessed: its vtable lbl_18_data_4F0 lists them by
// address, and the slots line up with CPatterned's declaration order (the inherited entries are
// spelled out by name in the same table). The tiny accessors at the head of the module's .text
// (fn_18_0, fn_18_8, fn_18_44, fn_18_4C, fn_18_54, fn_18_5C, fn_18_64, fn_18_6C, fn_18_7C,
// fn_18_88, fn_18_90 and fn_18_9044) have four-to-twelve-instruction bodies, so they are real
// rather than stand-ins.
#include "MetroidPrime/Enemies/CEmperorIngStage3.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "Kyoto/Animation/CAnimTreeNode.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CParticleGenInfoGeneric.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Collision/CJointCollisionDescription.hpp"
#include "MetroidPrime/Enemies/CEmperorIngLightSwarm.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Enemies/CSwarmBasics.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerKnockBackMgr.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrEmperorIngStage3.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptEffect.hpp"

#include "Collision/CCollisionPrimitive.hpp"
#include "Collision/CRayCastResult.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Math/CAABox.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "Kyoto/Animation/CInt32POINode.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "MetroidPrime/Weapons/CPlasmaProjectile.hpp"
#include "MetroidPrime/Weapons/CBeamInfo.hpp"
#include "MetroidPrime/Weapons/CWeaponAssetInfo.hpp"
#include "MetroidPrime/Enemies/CIngMiniPortalAttack.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Text/CStringTable.hpp"

#include <float.h>
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
  fprintf(stderr, "[emp3] %s is a port stand-in (see PortCEmperorIngStage3.cpp)\n", what);
}

// The same, for the state machine's own functions: the message names the retail function the
// entry stands in for, so the log says which of the module's FSM functions the run reached.
void StandInFn(const char* fn) {
  static const char* sSeen[64] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == fn) {
      return;
    }
  }
  if (sCount < 64) {
    sSeen[sCount++] = fn;
  }
  fprintf(stderr, "[emp3] stand-in %s\n", fn);
}

// The body's three FSM tables. Retail builds them in a static initialiser (fn_18_E180): the
// tables are lbl_18_data_100 (21 states, .data 0x100), lbl_18_data_34C (21 triggers, .data 0x34C)
// and lbl_18_data_4B4 (2 codes, .data 0x4B4), each entry 16 bytes {name, 0, 0xFFFFFFFF, fn}, and
// the function pointers come from the three-word records {0, 0xFFFFFFFF, fn} in .data: the states'
// twenty-one at 0x4..0xF4, the triggers' twenty-one at 0x250..0x344 and the codes' two at
// 0x49C..0x4A8.
//
// On the host the table addresses are irrelevant: the state machine matches a node by its name,
// so the name strings are retail's and the entry's function is the module's member function.
// The members are cast the way the DigitalGuardian, SpacePirate and EmperorIngStage1 ports do it.
#define EMP3_TRIGGER(name)                                                                          \
  { #name, reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CEmperorIngStage3::name) }
#define EMP3_STATE(name)                                                                            \
  { #name, reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CEmperorIngStage3::name) }
#define EMP3_CODE(name)                                                                             \
  { #name, reinterpret_cast< CPatterned::StateMachine::CodeFunc >(&CEmperorIngStage3::name) }

CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    EMP3_TRIGGER(TooClose),
    EMP3_TRIGGER(ShouldAttack),
    EMP3_TRIGGER(ShouldMeleeAttack),
    EMP3_TRIGGER(ShouldTaunt),
    EMP3_TRIGGER(ShouldStampede),
    EMP3_TRIGGER(ShouldDodge),
    EMP3_TRIGGER(ShouldChangeVulnerabilityToVulnerable),
    EMP3_TRIGGER(ShouldChangeVulnerabilityToInvulnerable),
    EMP3_TRIGGER(ShouldDarkBeamAttack),
    EMP3_TRIGGER(ShouldLightBeamAttack),
    EMP3_TRIGGER(ShouldLightSwarmAttack),
    EMP3_TRIGGER(ShouldGroundPound),
    EMP3_TRIGGER(ShouldGenerateDarkFliers),
    EMP3_TRIGGER(ShouldPortalAttack),
    EMP3_TRIGGER(ShouldJumpSlideAttack),
    EMP3_TRIGGER(ShouldGrowHeart),
    EMP3_TRIGGER(JumpSlideAttackOver),
    EMP3_TRIGGER(ShouldEyeShut),
    EMP3_TRIGGER(AbortedJumpSlide),
    EMP3_TRIGGER(ShouldJumpAttack),
    EMP3_TRIGGER(ShouldStampedeReact),
};

CPatterned::StateMachine::SStateFunction skStates[] = {
    EMP3_STATE(Start),
    EMP3_STATE(Idle),
    EMP3_STATE(MeleeAttack),
    EMP3_STATE(GainDistance),
    EMP3_STATE(Stampede),
    EMP3_STATE(Dodge),
    EMP3_STATE(Taunt),
    EMP3_STATE(ChangeVulnerability),
    EMP3_STATE(EyeShutResetVulnerability),
    EMP3_STATE(DarkBeamAttack),
    EMP3_STATE(LightBeamAttack),
    EMP3_STATE(LightSwarmAttack),
    EMP3_STATE(GroundPound),
    EMP3_STATE(GenerateDarkFliers),
    EMP3_STATE(PortalAttack),
    EMP3_STATE(JumpSlideAttack),
    EMP3_STATE(GrowHeart),
    EMP3_STATE(EyeShut),
    EMP3_STATE(JumpAttack),
    EMP3_STATE(StampedeReact),
    EMP3_STATE(Dead),
};

CPatterned::StateMachine::SCodeFunction skCodes[] = {
    EMP3_CODE(ResetAttackTimer),
    EMP3_CODE(ChoseAction),
};

#undef EMP3_TRIGGER
#undef EMP3_STATE
#undef EMP3_CODE

// fn_18_39A0 (0x3C): the cross product of two vectors: the result is (a.y*b.z - a.z*b.y,
// a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x) (0x39CC..0x39D0).
CVector3f Cross3f(const CVector3f& a, const CVector3f& b) { return CVector3f::Cross(a, b); }

// fn_18_3AC (0x20): the copy of the ray's result. The host's CRayCastResult is copyable, so the
// copy is the assignment; the byte the retail reads at +0x28 is the result's mValid.
CRayCastResult CopyRayResult(const CRayCastResult& result) { return result; }

} // namespace

// fn_18_9A8C (0xA044..0xA13C) locks the portal effect only when the asset is set.
static rstl::optional_object< TLockedToken< CGenDescription > > PortalPart(CAssetId id) {
  if (id == kInvalidAssetId) {
    return rstl::optional_object< TLockedToken< CGenDescription > >();
  }
  return rstl::optional_object< TLockedToken< CGenDescription > >(
      TLockedToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', id))));
}

// fn_18_9A8C. The base call is the retail one: EPatternedAI 0x0F (`li r4, 0xf` at 0x9AF0),
// flavor 0 (`li r7, 0x0` at 0x9AC8), movement 0 / collider 1 / body 1 (the constants stored
// to the stack arguments at 0x9AD8..0x9AE8 - kMT_Ground, kCT_One and kBT_BiPedal).
CEmperorIngStage3::CEmperorIngStage3(TUniqueId uid, const rstl::string& name,
                                     const CEntityInfo& info, const CTransform4f& xf,
                                     const CModelData& modelData,
                                     const CActorParameters& actParms,
                                     const CPatternedInfo& pInfo,
                                     const SLdrEmperorIngStage3Data& data)
: CPatterned(static_cast< EPatternedAI >(0x0f), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mVulnerableDamageThreshold(data.vulnerableDamageThreshold) // guest 0x814
, mTauntFrequency(data.tauntFrequency)                       // guest 0x91C
, mYellowHealth(LdrToHealthInfo(data.yellowHealth))           // guest 0x920
, mHealth(LdrToHealthInfo(data.health))                       // guest 0x940
, mRedVulnerability(LdrToDamageVulnerability(                  // guest 0x960
      data.redVulnerability))
, mLightVulnerability(LdrToDamageVulnerability(                // guest 0x990
      data.lightVulnerability))
, mDarkVulnerability(LdrToDamageVulnerability(                 // guest 0x9C0
      data.darkVulnerability))
, mMeleeDamage(LdrToDamageInfo(data.meleeDamage))             // guest 0x9F0
, mStampedeDamage(LdrToDamageInfo(data.stampedeDamage))       // guest 0xA0C
, mJumpSlideDamage(LdrToDamageInfo(data.jumpSlideDamage))     // guest 0xA28
, mGroundPoundDamage(LdrToDamageInfo(data.groundPoundDamage)) // guest 0xA44
, x824_(0)                                                 // guest 0x824
, x825_(0)                                                 // guest 0x825
, x826_(0)                                                 // guest 0x826 (the word at 0x824)
, x7c0_(nullptr)                                           // guest 0x7C0
, mVulnerabilityState(0)                                  // guest 0x7C4
, mX7c8_(0.f)                                             // guest 0x7C8
, mX7cc_(0.f)                                             // guest 0x7CC
, mX7d4_(0.f)                                             // guest 0x7D4
, mX7d0_(0.f)                                             // guest 0x7D0
, mX7e0_(10000.f)                                         // guest 0x7E0
, mAttackTimer(4.f)                                       // guest 0x7E4
, mAction(-1)                                             // guest 0x7E8
, mX810_(0.f)                                             // guest 0x810
, mX86c_(CDamageVulnerability::ImmuneVulnerabilty())     // guest 0x86C
, mX8c0_(0xffff)                                          // guest 0x8C0
, x8c2_(kInvalidUniqueId.value)                            // guest 0x8C2
, x8c4_(kInvalidUniqueId.value)                            // guest 0x8C4
, x8c6_(kInvalidUniqueId.value)                            // guest 0x8C6
, x8c8_(kInvalidUniqueId.value)                            // guest 0x8C8
, mX8ec_(0.f)                                             // guest 0x8EC
, mX8f0_(0.f)                                             // guest 0x8F0
, mX8f4_(0.f)                                             // guest 0x8F4
, mX910_(CVector3f::Zero())                               // guest 0x910
, mX818_(CVector3f::Zero())                               // guest 0x818
, mX864_(0.f)                                             // guest 0x864
, mX868_(-1)                                              // guest 0x868 (0x9CB4)
, x8a4_(-1)                                               // guest 0x8A4
, mX7d8_(0.f)                                             // guest 0x7D8 (0x9B58)
, mX7dc_(0.f)                                             // guest 0x7DC (0x9B5C)
, mX89c_(0)                                               // guest 0x89C (0x9CD0)
, mX8a0_(0)                                               // guest 0x8A0 (0x9CE0)
, x8a8_(-1)                                               // guest 0x8A8 (0x9CF8)
, mX8e0_(0)                                               // guest 0x8E0 (0x9D58)
, mX8e4_(0)                                               // guest 0x8E4 (0x9D5C)
, mX828_(FLT_MAX)                                         // guest 0x828 (rodata 0x2C8)
, mX82c_(0.f)                                             // guest 0x82C (0x9C80)
, mX860_(0.f)                                             // guest 0x860 (0x9CAC)
, mX8e8_(0.f)                                             // guest 0x8E8
, mX8dc_(0.f)                                             // guest 0x8DC
, mBeamId(kInvalidUniqueId.value)                          // guest 0x8AC
, mBeamSfx()                                              // guest 0x8B0
, mX8b4_(CVector3f::Zero())                               // guest 0x8B4 (0x9D0C)
, mShockWaveId(kInvalidUniqueId.value)                     // guest 0x90C
, mShockWaveSfx()                                         // guest 0x908
, mJumpAttackShockWave(data.jumpAttackShockWaveInfo)      // guest 0xE90 (fn_18_A2EC)
, mJumpAttackSound(static_cast< u16 >(data.sound))        // guest 0xECC
, mDarkBeamProjectile(data.darkBeamAttack.beamInfo.weaponSystem,
                      LdrToDamageInfo(data.darkBeamAttack.damage)) // guest 0xA60
, mDarkBeamInfo(data.darkBeamAttack.beamInfo)             // guest 0xA88
, mDarkBeamSound(static_cast< u16 >(data.darkBeamAttack.beamSound)) // guest 0xAD0
, mLightBeamProjectile(data.lightBeamAttack.beamInfo.weaponSystem,
                       LdrToDamageInfo(data.lightBeamAttack.damage)) // guest 0xAD4
, mLightBeamInfo(data.lightBeamAttack.beamInfo)           // guest 0xAFC
, mLightBeamSound(static_cast< u16 >(data.lightBeamAttack.beamSound)) // guest 0xB44
, mPortalEffect(PortalPart(data.portalAttack.effect))     // guest 0xD04, 0xD10
, mPortalSound(static_cast< u16 >(data.portalAttack.portalOpenSound)) // guest 0xD14
, mPortalDamage(LdrToDamageInfo(data.portalAttack.projectileDamage)) // guest 0xD18
, mPortalBeamInfo(data.portalAttack.beamInfo)             // guest 0xD34 (fn_18_A794)
, mX8f8_(0.f)                                             // guest 0x8F8 (0x9D70)
, mX8fc_(CVector3f::Zero())                               // guest 0x8FC (0x9D78..0x9D88)
, mVulnerableTime(data.vulnerableTime)                    // guest 0xB4C
, mVulnerableDamageThresholdReset(data.vulnerableDamageThreshold) // guest 0xB50
, mBaseTurnSpeed(mTurnSpeed)                              // guest 0xE5C (0xA258)
, mLightSwarmEffect(data.lightSwarmEffect)                // guest 0xB48 (0xA008)
, mLightSwarmData(LdrToBasicSwarmData(data.lightSwarmProperties)) // guest 0xD7C (0xA23C)
, mLightSwarmDeathSound(data.lightSwarmDeathSound)        // guest 0xE60 (0xA25C..0xA280)
, mLightSwarmExpireSound(data.audioPlaybackParms)         // guest 0xE78 (0xA284..0xA2B0)

{
  // The rest of the retail constructor belongs to the attacks and the state machine: the two beam
  // infos (0xA88 and 0xAFC), the projectile infos (0xA60 and 0xAD4), the four stage records
  // (0xB54, 0xBC0, 0xC2C, 0xC98), the light swarm (0xD34) and the portal data (0xE90). The
  // constructor ends with SetDrawShadow(this, 0) (0xA2D0).
  // The three lists reserve sixteen entries each (fn_18_ADF8 with 0x10, 0x9C84..0x9C9C).
  mX830_.reserve(0x10);
  mX840_.reserve(0x10);
  mX850_.reserve(0x10);
  // The four stage records (0xB54, 0xBC0, 0xC2C, 0xC98) are copies of the data's stage1..4.
  mStages[0] = data.stage1;
  mStages[1] = data.stage2;
  mStages[2] = data.stage3;
  mStages[3] = data.stage4;
  SetDrawShadow(false);
}

// The collision actor manager at guest 0x7C0 is owned by the actor.
CEmperorIngStage3::~CEmperorIngStage3() { delete x7c0_; }

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove / Death / KnockBack all dereference the current state. The analysis does not say
// where retail activates it, so the port activates on kSM_Create and, as the other patterned
// actors in this port do, again on demand in every entry point that can run first.
// The null test is a port limit, not retail's: CPatterned's constructor only builds the body
// controller when the patterned info has model data (CPatterned.cpp's HasModelData() test), so an
// actor without a model has no controller to activate.
void CEmperorIngStage3::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// @ fn_18_47D4
// vtable slot 3 = fn_18_47D4 (0x3E4): retail's Think. The base class's Think runs first
// (0x4818), then the module's own work with no active test: the stampede-react check
// (0x481C..0x4928), the timers (0x492C..0x4984), the module's per-frame calls (0x4988..0x4A20),
// the player knock-back (0x4A24..0x4A6C) and the shock wave's sound (0x4A70..0x4B78).
void CEmperorIngStage3::Think(float dt, CStateManager& mgr) {
  // Port guard, not retail: kSM_Create activates the body (0x7BE8); this covers a Think first.
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr); // 0x4818

  // The stampede-react check (0x481C): only while the flag at guest 0x825 is clear, and only
  // when the player is below the ing's head: the vector from the ing's "head" locator (rodata
  // 0x704) to the player, normalised, against the ing's down axis (the negated Z column of the
  // transform), has to be past 0.70685834 radians (rodata 0x2E8).
  if ((x825_ & 0x01) == 0) {
    const CVector3f head =
        (GetTransform() * GetScaledLocatorTransform(rstl::string_l("head"))).GetTranslation();
    const CVector3f& theirs = mgr.GetPlayer(0)->GetTranslation();
    CVector3f toPlayer(theirs.GetX() - head.GetX(), theirs.GetY() - head.GetY(),
                       theirs.GetZ() - head.GetZ());
    if (toPlayer.CanBeNormalized()) {
      toPlayer = toPlayer.AsNormalized();
      const CVector3f down = GetTransform().GetColumn(kDZ);
      const float dot = -down.GetZ() * toPlayer.GetZ() +
                        (-down.GetX() * toPlayer.GetX() + -down.GetY() * toPlayer.GetY());
      if (dot > CMath::FastCosR(0.70685834f)) { // rodata 0x2E8
        x825_ |= 0x01;
      }
    }
  }

  // The timers (0x492C): the taunt, eye-shut, attack-cooldown, too-close and jump-attack timers
  // grow by the frame time; the attack timer and the damage taken while vulnerable shrink.
  mX7c8_ += dt;
  mX7cc_ += dt;
  mX7d0_ += dt;
  mX7d4_ += dt;
  mX7e0_ += dt;
  mAttackTimer -= dt;
  mX810_ -= dt;

  // The module's per-frame calls (0x4988..0x4A20), in retail order; the ones still stand-ins
  // carry their retail sizes.
  UpdateList(mX830_, dt); // fn_18_AC44 (0xB4)
  UpdateList(mX840_, dt); // fn_18_AC44 (0xB4)
  UpdateList(mX850_, dt); // fn_18_AC44 (0xB4)
  // Retail has no test: its manager exists from kSM_AreaLoaded on. The port guard covers a Think
  // before that message and logs once if it ever happens.
  if (x7c0_ != nullptr) {
    x7c0_->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace); // fn_18_3B64 (0x28)
    UpdateTargetMaterials(mgr);                                       // fn_18_39E0 (0xA4)
  } else {
    StandIn("Think before kSM_AreaLoaded (no collision actors yet)");
  }
  UpdateHearts(mgr, dt);  // fn_18_3448 (0x1AC)
  MeleeContact(mgr);      // fn_18_27DC (0x38C)
  ChargeContact(mgr);     // fn_18_22E0 (0x4C0)
  CheckList(mgr);         // fn_18_1978 (0xDC)
  SteerBeam(mgr, dt);     // fn_18_1370 (0x1C0)
  WatchObject(mgr);       // fn_18_125C (0x68)
  UpdateLightSwarm(mgr, dt); // fn_18_E40 (0x41C)

  // The player knock-back (0x4A24..0x4A6C): while the timer MeleeContact set (guest 0x8F8) is
  // above zero (rodata 0x258) it runs down by the frame time and the player is pushed along
  // the normalised direction (guest 0x8FC) with power 100 (rodata 0x27C) and zero (rodata
  // 0x258). The DOL's fn_801BF3DC is CPlayerKnockBackMgr::ApplyPlayerKnockBackForce.
  if (mX8f8_ > 0.f) {
    mX8f8_ -= dt;
    CPlayerKnockBackMgr::ApplyPlayerKnockBackForce(*mgr.Player(0), mX8fc_.AsNormalized(), 100.f,
                                                   0.f);
  }

  // The shock wave's sound (0x4A70..0x4B78): while its emitter (guest 0x908) plays and the
  // wave (guest 0x90C) exists, the sound sits on the wave's ring on the player's side: the flat
  // (z zeroed, rodata 0x258) direction from the wave to the player, scaled by the wave's radius
  // (wave+0x1C0), at full volume (0xFF). Retail reads the object without a cast.
  if (mShockWaveSfx && mShockWaveId != kInvalidUniqueId.value &&
      CSfxManager::IsPlaying(mShockWaveSfx)) {
    const CShockWave* wave =
        static_cast< const CShockWave* >(mgr.GetObjectById(TUniqueId(mShockWaveId)));
    if (wave != nullptr) {
      CVector3f dir = mgr.Player(0)->GetTranslation() - wave->GetTranslation();
      dir = CVector3f(dir.GetX(), dir.GetY(), 0.f);
      if (dir.CanBeNormalized()) {
        dir.Normalize();
        const CVector3f pos = wave->GetTranslation() + dir * wave->GetRadius();
        CSfxManager::UpdateEmitter(mShockWaveSfx, pos, dir, 0xff);
      }
    }
  }
}

// fn_18_125C (0x68): the module watches the object its id at guest 0x8C0 names; when the
// object is gone the id is cleared (0x12AC).
void CEmperorIngStage3::WatchObject(CStateManager& mgr) {
  if (mX8c0_ == 0xffff) {
    return;
  }
  if (mgr.GetObjectById(TUniqueId(mX8c0_)) == nullptr) {
    mX8c0_ = 0xffff;
  }
}

// @ fn_18_E40
// fn_18_E40 (0x41C): the object-drop timer (guest 0x8DC, which kUE_ObjectDrop sets to 1.5)
// runs down and the light-swarm timer (guest 0x7DC) up. While the drop timer runs, the
// INTERNAL_STATE_01/PLAY effect (guest 0x8C4) emits and the INTERNAL_STATE_00 swarm (guest
// 0x8C2) is activated and counted up; after it, the effect stops and the swarm counts down.
// While that swarm has no active boid the dark-flier timer (guest 0x7D8) grows. The
// INTERNAL_STATE_02 actors follow the "mainHeart_SDK" locator (rodata 0xBC -> 0x340): the
// DEACTIVATE one (guest 0x8C8) shows once the light swarm has been gone 20 seconds (rodata
// 0x270), the ACTIVATE ones (guest 0x8CC) until then, each at its own offset.
void CEmperorIngStage3::UpdateLightSwarm(CStateManager& mgr, float dt) {
  mX8dc_ -= dt;
  mX7dc_ += dt;
  if (CScriptEffect* effect = TCastToPtr< CScriptEffect >(mgr.ObjectById(TUniqueId(x8c4_)))) {
    if (mX8dc_ > 0.f) {
      if (!effect->IsEmitting()) { // guest effect+0x2C8, 0x80
        effect->AcceptScriptMsg(mgr, CScriptMsg(GetUniqueId(), effect->GetUniqueId(), kSM_Activate));
      }
    } else if (effect->IsEmitting()) {
      effect->AcceptScriptMsg(mgr, CScriptMsg(GetUniqueId(), effect->GetUniqueId(), kSM_Deactivate));
    }
  }
  if (CSwarmBasics* swarm = TCastToPtr< CSwarmBasics >(mgr.ObjectById(TUniqueId(x8c2_)))) {
    if (swarm->EvaluateActiveBoidCount() == 0) { // fn_80_1280
      mX7d8_ += dt;
    }
    if (CEntity* shown = mgr.ObjectById(TUniqueId(x8c8_))) {
      shown->SetActive(mX7dc_ > 20.f);
    }
    if (mX8dc_ > 0.f) {
      if (!swarm->GetActive()) { // guest swarm+0x20, 0x80
        swarm->AcceptScriptMsg(mgr, CScriptMsg(GetUniqueId(), TUniqueId(x8c2_), kSM_Activate));
      }
      swarm->AcceptScriptMsg(mgr, CScriptMsg(GetUniqueId(), TUniqueId(x8c2_), kSM_Increment));
    } else {
      swarm->AcceptScriptMsg(mgr, CScriptMsg(GetUniqueId(), TUniqueId(x8c2_), kSM_Decrement));
    }
  }
  const CTransform4f xf =
      GetTransform() * GetScaledLocatorTransform(rstl::string_l("mainHeart_SDK")); // 0x1138
  if (CActor* actor = TCastToPtr< CActor >(mgr.ObjectById(TUniqueId(x8c8_)))) {
    actor->SetTransform(xf);
  }
  for (int i = 0; i < x8cc_.size(); ++i) { // stride 0x34
    if (CActor* actor = TCastToPtr< CActor >(mgr.ObjectById(x8cc_[i].mId))) {
      actor->SetTransform(xf * x8cc_[i].mXf);
      actor->SetActive(mX7dc_ < 20.f);
    }
  }
}

// @ fn_18_7B14
// vtable slot 4 = fn_18_7B14 (0x668): retail's message switch. The base class's message
// handler runs first (0x7B50) and then the module's own cases, one per message: the constants
// are 0x58444D47 (kSM_Damage, 0x7B60), 0x58435254 (kSM_Create, 0x7B74), 0x58414C44
// (kSM_AreaLoaded, 0x7B88), 0x5844454C (kSM_Delete, 0x7B98), 0x584C4155 (kSM_Launching,
// 0x7BAC), 0x58484954 (kSM_HitObject, 0x7BC0) and 0x584F4646 (kSM_OffGround, 0x7BD4). Each
// case says whether it is ported; the bodies are the module's own work (the collision actor
// list, the id lists, the boss parameters, the launch jump).
void CEmperorIngStage3::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CPatterned::AcceptScriptMsg(mgr, msg); // 0x7B50
  switch (msg.GetMessage()) {
  case kSM_Create: // 0x7BE0, 0x14
    // The module's body controller is activated with the invalid animation state (0x7BE8).
    BodyController()->Activate(mgr, pas::kAS_Invalid);
    break;
  case kSM_Delete: // 0x7BF4, 0x24
    // The collision actors are destroyed (fn_18_3B8C, 0x28: the manager's Destroy) and the
    // object the module watches is deleted (0x7C00). The manager test is a port guard for an
    // actor deleted before its area loaded; retail has none.
    if (x7c0_ != nullptr) {
      x7c0_->Destroy(mgr);
    }
    mgr.DeleteObjectRequest(TUniqueId(mX8c0_));
    break;
  case kSM_AreaLoaded: { // 0x7C18..0x7F48 (0x330)
    // The INTERNAL_STATE_00 connection, any message (`li r7, -0x1`, 0x7C2C).
    x8c2_ = FindConnectedObject(mgr, kSS_InternalState0, kSM_None).value;
    const rstl::vector< SConnection >& conns = GetConnectionList();
    // INTERNAL_STATE_01: the first id of the connection's list - PLAY to guest 0x8C4, ACTIVATE
    // to guest 0x8C6.
    for (rstl::vector< SConnection >::const_iterator it = conns.begin(); it != conns.end(); ++it) {
      if (it->state != kSS_InternalState1) {
        continue;
      }
      const CStateManager::TIdListResult ids = mgr.GetIdListForScript(it->objId);
      if (ids.first == ids.second) {
        continue;
      }
      switch (it->msg) {
      case kSM_Activate:
        x8c6_ = ids.first->second.value;
        break;
      case kSM_Play:
        x8c4_ = ids.first->second.value;
        break;
      default:
        break;
      }
    }
    // INTERNAL_STATE_02: ACTIVATE adds the actor and its transform relative to this actor's
    // area (the area's inverse transform times the actor's, 0x7D84..0x7DBC) to the portal list
    // at guest 0x8CC; DEACTIVATE goes to guest 0x8C8.
    for (rstl::vector< SConnection >::const_iterator it = conns.begin(); it != conns.end(); ++it) {
      if (it->state != kSS_InternalState2) {
        continue;
      }
      const CStateManager::TIdListResult ids = mgr.GetIdListForScript(it->objId);
      if (ids.first == ids.second) {
        continue;
      }
      if (it->msg == kSM_Activate) {
        const TUniqueId id = ids.first->second;
        const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(id));
        if (actor != nullptr) {
          const CTransform4f& areaXf = mgr.World()->GetArea(GetCurrentAreaId())->GetTM();
          x8cc_.push_back(SEmperorIngPortal(id, areaXf.GetInverse() * actor->GetTransform()));
        }
      } else if (it->msg == kSM_Deactivate) {
        x8c8_ = ids.first->second.value;
      }
    }
    SetupCollisionActors(mgr); // fn_18_3BB0
    // The actor's health becomes the data's health (guest 0x944, its current value) with no
    // knockback resistance (0x7E80), and the boss bar gets the id, that health's initial value
    // (the vtable slot 0x3C result's first float, 0x7F10) and the name "BossTempleBossStage3"
    // (rodata 0x709).
    *HealthInfo() = CHealthInfo(mHealth.GetHP(), 0.f);
    mgr.SetBossParams(GetUniqueId(), GetHealthInfo()->GetInitialHP(),
                      static_cast< uint >(gpStringTable->GetStringIndex("BossTempleBossStage3")));
    break;
  }
  case kSM_Damage: // 0x7F4C, 0x20
    // fn_18_36BC (0x88): the damage's source id is compared with the id each heart's collision
    // actor carries (0x36E8 and 0x3708) and a match sets the flag byte 0x824's bit 0 (the mask
    // 0x01, 0x3718).
    OnDamage(mgr, msg.GetSenderId().value);
    break;
  case kSM_HitObject: // 0x7F6C, 0x20
    // fn_18_2D50 (0x1B4): the contact damage to the player the hit collision actor last
    // touched.
    OnHitObject(mgr, msg.GetSenderId().value);
    break;
  case kSM_OffGround: // 0x7F8C, 0x38
    // The off-ground force: the actor's gravity constant (the vtable slot 0x114) times the
    // actor's mass (CPhysicsActor::mMass, guest 0x158), negated (0x7FA4..0x7FC0). The retail
    // writes it to CPhysicsActor::mConstantForce (guest 0x1C0..0x1C8).
    SetConstantForceWR(CVector3f(0.f, 0.f, -(GetGravityConstant() * GetMass())));
    break;
  case kSM_Launching: { // 0x7FC4, 0x184
    // The base class gets an off-ground message from the launcher (the message's sender, this
    // actor's id, 0x7FC4..0x8014) - CPatterned's handler, not this one - and the off-ground
    // force is written as in kSM_OffGround (mass times gravity along the down vector).
    CPatterned::AcceptScriptMsg(mgr, CScriptMsg(msg.GetSenderId(), GetUniqueId(), kSM_OffGround));
    // The force is sDownVector times the gravity constant (slot 0x114), times the mass (0x8030).
    const float mass = GetMass();
    SetConstantForceWR((CVector3f(CVector3f::Down()) * GetGravityConstant()) * mass);
    // The jump to the launch target (guest 0x910): the apex is 7.5 (rodata 0x304) above the
    // higher of the actor and the target; the upward speed reaches it, and the flight time is
    // the rise plus the fall to the target's height (0x80D4..0x811C). The flat speed covers the
    // flat distance in that time (0x8130..0x8138). Retail reads the gravity constant again
    // (slot 0x114, 0x80AC).
    const CVector3f& mine = GetTranslation();
    const float dx = mX910_.GetX() - mine.GetX();
    const float dy = mX910_.GetY() - mine.GetY();
    const float gravity = GetGravityConstant();
    float top = mX910_.GetZ();
    if (mine.GetZ() >= top) {
      top = mine.GetZ();
    }
    const float apex = 7.5f + top; // rodata 0x304
    const float up = CMath::SqrtF(2.f * gravity * (apex - mine.GetZ())); // rodata 0x274
    const float time = up / gravity + CMath::SqrtF(2.f * (apex - mX910_.GetZ()) / gravity); // 0x274
    const float inverse = 1.f / time; // rodata 0x264
    SetVelocityWR(CVector3f(inverse * dx, inverse * dy, up));
    break;
  }
  default:
    break;
  }
}

// @ fn_18_4650
// fn_18_4650 (vtable slot 7, 0x164): the base's pre-render, then the draw flags (this+0xFC).
// While the base's damage cooldown (guest 0x448) or the heart hit timer (0x860) is above zero
// (rodata 0x258) the model draws with blend mode 2 and depth test/update in a colour lerped from
// black to CPatterned::skDamageColor by the cooldown (clamped at zero) or, failing that, to
// skHitsWithoutDamageColor by the hit timer, either over skDamageHitTime; otherwise it is opaque
// white (rodata 0x264).
void CEmperorIngStage3::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);
  if (mDamageCooldownTimer > 0.f || mX860_ > 0.f) {
    CColor color;
    if (mDamageCooldownTimer > 0.f) {
      color = CColor::Lerp(CColor::Black(), skDamageColor,
                           rstl::max_val(mDamageCooldownTimer, 0.f) / skDamageHitTime);
    } else {
      color = CColor::Lerp(CColor::Black(), skHitsWithoutDamageColor, mX860_ / skDamageHitTime);
    }
    SetModelFlags(CModelFlags(CModelFlags::kT_Two, color)); // 0x4714
  } else {
    SetModelFlags(CModelFlags(CModelFlags::kT_Opaque, 1.f)); // 0x473C
  }
}

// @ fn_18_47B4
// fn_18_47B4 (vtable slot 9, 0x20): the base's render and nothing else.
void CEmperorIngStage3::Render(const CStateManager& mgr) const { CPatterned::Render(mgr); }

// vtable slot 15 = fn_18_B4: a call through the vtable slot 0x38, i.e. CActor::HealthInfo().
const CHealthInfo* CEmperorIngStage3::GetHealthInfo() const {
  return const_cast< CEmperorIngStage3* >(this)->HealthInfo();
}

// vtable slot 18 = fn_18_8: the actor's own bounding box, in an optional.
rstl::optional_object< CAABox > CEmperorIngStage3::GetTouchBounds() const {
  return GetBoundingBox();
}

// fn_18_8E84 (vtable slot 21, 0x1C0): the active heart's pivot joint - the second heart's when its
// collision actor (fn_18_3A84) is active (the byte at +0x20, bit 7), else the first's (the
// description's CSegId at +0x8, fn_18_3B14) - at the current animation's elapsed time (the tree's
// steady-state duration, vtable slot 0x18, less its time remaining, slot 0x14), through the
// unscaled CAnimData locator and the actor's transform. With neither heart active it is the zero
// vector (the CSegId 0xFF test at 0x8F4C).
CVector3f CEmperorIngStage3::GetAimPosition(const CStateManager& mgr, float param) const {
  if (x7c0_ == nullptr) {
    return CVector3f::Zero(); // port guard: no collision actors before kSM_AreaLoaded
  }
  const CEntity* first = mgr.GetObjectById(HeartRecord(0)->GetCollisionActorId());  // fn_18_3A84
  const CEntity* second = mgr.GetObjectById(HeartRecord(1)->GetCollisionActorId()); // fn_18_3A84
  CSegId pivot(static_cast< uchar >(0xFF));
  if (second->GetActive()) {
    pivot = HeartRecord(1)->GetPivotId(); // 0x8F18
  } else if (first->GetActive()) {
    pivot = HeartRecord(0)->GetPivotId(); // 0x8F3C
  }
  if (pivot == CSegId(static_cast< uchar >(0xFF))) {
    return CVector3f::Zero();
  }
  const CAnimData* animData = GetModelData()->GetAnimationData();
  const rstl::ncrc_ptr< CAnimTreeNode >& tree = animData->GetAnimationTree();
  const CCharAnimTime elapsed(tree->VGetSteadyStateAnimInfo().GetDuration().GetSeconds() -
                              tree->VGetTimeRemaining().GetSeconds());
  return (GetTransform() * animData->GetLocatorTransform(pivot, &elapsed)).GetTranslation();
}

// @ fn_18_45B4
// fn_18_45B4 (vtable slot 27, 0x9C): a box one unit (sOneVector) either side of the centre of
// CActor's sorting bounds (0x45CC).
CAABox CEmperorIngStage3::GetSortingBounds(const CStateManager& mgr) const {
  const CVector3f center = CActor::GetSortingBounds(mgr).GetCenterPoint();
  return CAABox(center - CVector3f(1.f, 1.f, 1.f), center + CVector3f(1.f, 1.f, 1.f));
}

// @ fn_18_9048
// fn_18_9048 (vtable slot 28, 0x3F0): the user events, a 40-entry jump table (lbl_18_data_670;
// any other type, and the table's default entries, go to the base). kUE_EggLay runs fn_18_4D0;
// kUE_ChangeMaterial sets the grow-heart flag (0x02 of 0x825); kUE_DamageOn sets 0x40 of 0x824
// and kUE_TakeOff clears it and sets the no-contact-damage flag (0x10 of 0x825);
// kUE_AlignTargetPosStart puts the INTERNAL_STATE_01 ACTIVATE then PLAY actors (guest 0x8C6,
// 0x8C4) at the event locator's point, 3.22 (rodata 0x31C) above it; kUE_Landing sets the
// room-check flag (0x08 of 0x825); kUE_ScreenShake sends INTERNAL_STATE_04 ('IS04') with no
// message; kUE_BeginAction runs fn_18_BB8; kUE_ObjectDrop sets guest 0x8DC to 1.5 (rodata
// 0x29C); kUE_EventStart starts the beam (fn_18_1530) with 0x01 of 0x824 or 0x80 of 0x825;
// kUE_EventStop stops it (fn_18_12C4); kUE_Activate and kUE_Deactivate switch the hearts
// (fn_18_1BB0, fn_18_1B88); kUE_EffectOn/Off set and clear the slide-steer flag (0x20 of 0x825);
// event 37 sets 0x10 of 0x824, 38 makes the hearts immune (fn_18_1C34) and 39 spawns the jump
// attack's shock wave (0x9274) with its looped sound.
void CEmperorIngStage3::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                        EUserEventType type, float dt) {
  switch (type) {
  case kUE_EggLay:
    EggLayEvent(mgr, dt); // fn_18_4D0
    break;
  case kUE_ChangeMaterial:
    x825_ |= 0x02;
    break;
  case kUE_DamageOn:
    x824_ |= 0x40;
    break;
  case kUE_TakeOff:
    x824_ &= ~0x40;
    x825_ |= 0x10;
    break;
  case kUE_AlignTargetPosStart: {
    const CVector3f lift = CVector3f::Up() * 3.22f; // rodata 0x31C
    const CVector3f pos =
        GetTransform() * GetScaledLocatorTransform(node.GetLocatorName()).GetTranslation() + lift;
    if (CActor* actor = TCastToPtr< CActor >(mgr.ObjectById(TUniqueId(x8c6_)))) {
      actor->SetTranslation(pos);
    }
    if (CActor* actor = TCastToPtr< CActor >(mgr.ObjectById(TUniqueId(x8c4_)))) {
      actor->SetTranslation(pos);
    }
    break;
  }
  case kUE_Landing:
    x825_ |= 0x08;
    break;
  case kUE_ScreenShake:
    SendScriptMsgs(kSS_InternalState4, mgr, kInvalidUniqueId, kSM_None); // 0x93D8
    break;
  case kUE_BeginAction:
    BeginActionEvent(mgr); // fn_18_BB8
    break;
  case kUE_ObjectDrop:
    mX8dc_ = 1.5f; // rodata 0x29C
    break;
  case kUE_EventStart:
    if ((x824_ & 0x01) != 0 || (x825_ & 0x80) != 0) {
      StartBeam(mgr); // fn_18_1530
    }
    break;
  case kUE_EventStop:
    StopBeam(mgr); // fn_18_12C4
    break;
  case kUE_Activate:
    SetCurrentHeart(mgr); // fn_18_1BB0
    break;
  case kUE_Deactivate:
    HeartsOff(mgr); // fn_18_1B88
    break;
  case kUE_EffectOn:
    x825_ |= 0x20;
    break;
  case kUE_EffectOff:
    x825_ &= ~0x20;
    break;
  case kUE_Unknown37:
    x824_ |= 0x10;
    break;
  case kUE_Unknown38:
    SetHeartsImmune(mgr); // fn_18_1C34
    break;
  case kUE_Unknown39: {
    mShockWaveId = mgr.AllocateUniqueId().value; // 0x9288
    mgr.AddObject(rs_new CShockWave(TUniqueId(mShockWaveId), rstl::string_l("Shockwave"),
                                    CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true),
                                    GetTransform(), GetUniqueId(), mJumpAttackShockWave,
                                    4.f, 0.4f)); // fn_80171970, rodata 0x2AC/0x320
    mShockWaveSfx = CSfxManager::AddEmitter(mJumpAttackSound, GetTranslation(),
                                            GetCurrentAreaId().Value(), true, false,
                                            CSfxManager::kMedPriority);
    break;
  }
  default:
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);
    break;
  }
}

// vtable slot 41 = fn_18_9044: the retail body is a single `blr` - the boss takes its damage in
// the state machine, not here.
void CEmperorIngStage3::TakeDamage(const CVector3f& direction, float magnitude) {}

// vtable slot 42 = fn_18_90: `li r3, 0x1` - the actor can be shot.
bool CEmperorIngStage3::CanBeShot(const CStateManager&, int) { return true; }

// vtable slot 43 = fn_18_0: `li r3, 0x1` - the actor listens for noises.
bool CEmperorIngStage3::IsListening() const { return true; }

// vtable slot 44 = fn_18_8E50: a player fire (the noise type 0) sets the flag byte at guest
// 0x824 (its bit 30, the mask 0x02) and the actor says it heard it; every other noise is
// ignored.
bool CEmperorIngStage3::Listen(CStateManager& mgr, const CVector3f& position,
                                EListenNoiseType type) {
  if (type != kLNT_PlayerFire) {
    return false;
  }
  x824_ |= 0x02;
  return true;
}

// fn_18_98 (vtable slot 45, 0x1C): the actor's own translation (this+0x54..0x5C, r4 is `this`
// after the return slot).
CVector3f CEmperorIngStage3::GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                                       const CVector3f& aimPos) const {
  return GetTranslation();
}

// vtable slot 50 = fn_18_44: the model alpha byte at guest 0x44F, CPatterned::mColor's alpha
// (CColor at 0x44C; the module never writes it).
uchar CEmperorIngStage3::GetModelAlphau8(const CStateManager& mgr) const { return mColor.GetAlphau8(); }

// vtable slot 55 = fn_18_4C: `li r3, 0x0` - the actor has no projectile info.
CProjectileInfo* CEmperorIngStage3::ProjectileInfo() { return nullptr; }

// vtable slot 56 = fn_18_54: `li r3, 0x0` - the actor has no path search.
CPathFindSearch* CEmperorIngStage3::GetSearchPath() { return nullptr; }

// vtable slot 57 = fn_18_5C: `li r3, 0x0` - the actor has no point path search.
CPathFindPointSearch* CEmperorIngStage3::GetPointSearchPath() { return nullptr; }

// vtable slot 61 = fn_18_64: `li r3, 0x0` - the actor has no rag doll.
CRagDoll* CEmperorIngStage3::GetRagDoll() const { return nullptr; }

// vtable slot 66 = fn_18_6C: stores kInvalidUniqueId, i.e. the actor never reports an attack
// target through this accessor.
TUniqueId CEmperorIngStage3::GetAttackTarget() const { return kInvalidUniqueId; }

// vtable slot 68 = fn_18_7C: the on-ground bit of the byte at guest 0x34C, which is what the base
// class keeps in mOnGround.
bool CEmperorIngStage3::IsOnGround() const { return CPatterned::IsOnGround(); }

// vtable slot 69 = fn_18_8E78: the gravity constant, rodata 0x2A4 = 200.0f.
float CEmperorIngStage3::GetGravityConstant() const { return 200.f; }

// vtable slot 60 = fn_18_943C: CPatterned's own setup first (the call at 0x9450), then the
// module's three name tables are handed to the state machine - the triggers (lbl_18_data_34C, 21
// entries) through the machine's vtable slot +0x18 (0x9470), the states (lbl_18_data_100, 21
// entries) through +0x14 (0x9490) and the codes (lbl_18_data_4B4, 2 entries) through +0x1C
// (0x94B0). That FSM is milestone 2, so only the base setup runs here.
void CEmperorIngStage3::SetupStateMachine(CStateManager& mgr) {
  CPatterned::SetupStateMachine(mgr);
  StateMachine* stateMachine = mStateMachine.get();
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  // The host's FSM1 does not dispatch code nodes yet (TStateMachineState::SetCodeFunctions is
  // empty), so this only records the table retail registers.
  stateMachine->SetCodeFunctions(skCodes, ARRAY_SIZE(skCodes));
}

// fn_18_F8 (0x11C): the module's look check. The parameter is the angle in degrees the player
// may be off the actor's facing by. Retail: the vector from the actor's "DarkBeam_LCTR_SDK"
// locator (fn_18_DBC) to the player, normalised (0x170) and flattened to the XY plane (the
// zero at rodata 0x258, 0x198), has to be normalisable (0x260); the angle between it and the
// actor's facing (the transform's Y column, guest 0x28/0x38/0x48) is turned into degrees
// (rodata 0x25C = 57.295776, 0x1D8) and has to be below the parameter (0x1E0).
bool CEmperorIngStage3::RandomCheck(CStateManager& mgr, float maxAngleDeg) const {
  const CVector3f& mine = DarkBeamLocatorPosition();
  const CVector3f& theirs = mgr.GetPlayer(0)->GetTranslation();
  CVector3f direction(theirs.GetX() - mine.GetX(), theirs.GetY() - mine.GetY(),
                       theirs.GetZ() - mine.GetZ());
  direction.Normalize();
  direction.SetZ(0.f); // rodata 0x258
  if (!direction.CanBeNormalized()) {
    return false;
  }
  direction.Normalize();
  const float degrees =
      57.295776f * CVector3f::GetAngleDiff(direction, GetTransform().GetColumn(kDY));
  return degrees < maxAngleDeg;
}

// fn_18_DBC (0x40): the actor's "DarkBeam_LCTR_SDK" locator (rodata 0xA8) in world space: the
// actor's transform times the model's scaled locator transform, and the translation of the
// product (0xE08..0xE20).
CVector3f CEmperorIngStage3::DarkBeamLocatorPosition() const {
  const CTransform4f locator = GetScaledLocatorTransform(rstl::string_l("DarkBeam_LCTR_SDK"));
  return (GetTransform() * locator).GetTranslation();
}

// fn_18_2298 (0x24): the average (fn_18_AB9C) over the taunt list at guest 0x840 (`addi r3,
// r3, 0x840` at 0x22A0), the list AddTauntEntry (fn_18_2B68) fills.
float CEmperorIngStage3::TauntAverage() const { return ListAverage(mX840_); }

// @ fn_18_27DC
// fn_18_27DC (0x38C): the melee contact. While the melee flag (0x40 at guest 0x824) is set and
// the player has touch bounds, the three leg locator pairs (rodata 0x90: F_knee/F_end,
// L_F_knee/L_F_end, R_F_knee/R_F_end) each make a box - both locators' positions in world
// space, grown by 2.5 (rodata 0x298) each way. The first box that meets the player's deals the
// melee damage (guest 0x9F0) with the solid-only filter (data 0x0: 1 << 59, the include-exclude
// type 3), puts the player in kMS_ApplyJump (2), starts the knock-back (the flat direction to
// the player, its Z set to 1.5 - rodata 0x29C - and normalised again, for 0.5 seconds -
// rodata 0x2A0) and clears the melee flag.
void CEmperorIngStage3::MeleeContact(CStateManager& mgr) {
  if ((x824_ & 0x40) == 0) { // 0x2800: extrwi 1,25 is the byte's 0x40
    return;
  }
  CPlayer& player = *mgr.Player(0);
  const rstl::optional_object< CAABox > touch = player.GetTouchBounds(); // vtable slot 0x48
  if (!touch) {
    return;
  }
  static const char* const kLocators[6] = {"F_knee",   "F_end",   "L_F_knee",
                                           "L_F_end",  "R_F_knee", "R_F_end"};
  const CAnimData* animData = GetModelData()->GetAnimationData();
  for (int i = 0; i < 3; ++i) {
    const CSegId first = animData->GetLocatorSegId(rstl::string_l(kLocators[i * 2]));
    const CSegId second = animData->GetLocatorSegId(rstl::string_l(kLocators[i * 2 + 1]));
    CAABox box = CAABox::MakeMaxInvertedBox();
    box.AccumulateBounds(GetTransform() * GetScaledLocatorTransform(first).GetTranslation());
    box.AccumulateBounds(GetTransform() * GetScaledLocatorTransform(second).GetTranslation());
    const CVector3f grownMin(box.GetMinPoint().GetX() - 2.5f, box.GetMinPoint().GetY() - 2.5f,
                             box.GetMinPoint().GetZ() - 2.5f);
    const CVector3f grownMax(box.GetMaxPoint().GetX() + 2.5f, box.GetMaxPoint().GetY() + 2.5f,
                             box.GetMaxPoint().GetZ() + 2.5f);
    box.AccumulateBounds(grownMin);
    box.AccumulateBounds(grownMax);
    if (touch->DoBoundsOverlap(box)) {
      mgr.ApplyDamage(GetUniqueId(), player.GetUniqueId(), GetUniqueId(), mMeleeDamage,
                      CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid),
                                                          CMaterialList()),
                      CVector3f::Zero()); // the DOL's fn_8003E25C
      player.SetMoveState(NPlayer::kMS_ApplyJump, mgr);
      mX8fc_ = PlayerDirection(mgr); // fn_18_2D1C
      mX8fc_.Normalize();
      mX8fc_.SetZ(1.5f);
      mX8fc_.Normalize();
      mX8f8_ = 0.5f;
      x824_ &= ~0x40; // 0x2B30 (rlwimi 6,25,25)
      return;
    }
  }
}

// @ fn_18_22E0
// fn_18_22E0 (0x4C0): the charge contact. While the charge flag (0x20 at guest 0x824) is set and
// the player has touch bounds, the three leg locator pairs (rodata 0x90, as in fn_18_27DC) each
// make a box from both locators' world positions; the body box gathers every leg box, and each
// leg box grown by 0.1 (rodata 0x28C) that meets the player's counts as a leg hit. The body box
// grows by 3 (rodata 0x284). A morphed player (0x38C == 1) is hit by a leg hit, any other by the
// body box. A hit deals the jump-slide damage (guest 0xA28) with the no-contact-damage flag
// (0x10 of 0x825) set, the stampede damage (guest 0xA0C) without, with the solid-only filter
// (data 0x0: 1 << 59, type 3) and no direction, and clears the charge flag.
void CEmperorIngStage3::ChargeContact(CStateManager& mgr) {
  if ((x824_ & 0x20) == 0) { // 0x2304: extrwi 1,26
    return;
  }
  CPlayer& player = *mgr.Player(0);
  const rstl::optional_object< CAABox > touch = player.GetTouchBounds(); // vtable slot 0x48
  if (!touch) {
    return;
  }
  static const char* const kLocators[6] = {"F_knee",   "F_end",   "L_F_knee",
                                           "L_F_end",  "R_F_knee", "R_F_end"};
  const CAnimData* animData = GetModelData()->GetAnimationData();
  CAABox body = CAABox::MakeMaxInvertedBox();
  bool legHit = false;
  for (int i = 0; i < 3; ++i) {
    const CSegId first = animData->GetLocatorSegId(rstl::string_l(kLocators[i * 2]));
    const CSegId second = animData->GetLocatorSegId(rstl::string_l(kLocators[i * 2 + 1]));
    CAABox box = CAABox::MakeMaxInvertedBox();
    box.AccumulateBounds(GetTransform() * GetScaledLocatorTransform(first).GetTranslation());
    box.AccumulateBounds(GetTransform() * GetScaledLocatorTransform(second).GetTranslation());
    body.AccumulateBounds(box.GetMinPoint());
    body.AccumulateBounds(box.GetMaxPoint());
    const CVector3f grownMin(box.GetMinPoint().GetX() - 0.1f, box.GetMinPoint().GetY() - 0.1f,
                             box.GetMinPoint().GetZ() - 0.1f);
    const CVector3f grownMax(box.GetMaxPoint().GetX() + 0.1f, box.GetMaxPoint().GetY() + 0.1f,
                             box.GetMaxPoint().GetZ() + 0.1f);
    box.AccumulateBounds(grownMin);
    box.AccumulateBounds(grownMax);
    if (touch->DoBoundsOverlap(box)) {
      legHit = true;
    }
  }
  const CVector3f bodyMin(body.GetMinPoint().GetX() - 3.f, body.GetMinPoint().GetY() - 3.f,
                          body.GetMinPoint().GetZ() - 3.f);
  body.AccumulateBounds(bodyMin);
  const CVector3f bodyMax(body.GetMaxPoint().GetX() + 3.f, body.GetMaxPoint().GetY() + 3.f,
                          body.GetMaxPoint().GetZ() + 3.f);
  body.AccumulateBounds(bodyMax);
  bool hit = false;
  if (player.GetMorphballTransitionState() == CPlayer::kMS_Morphed) { // 0x2628
    if (legHit) {
      hit = true;
    }
  } else if (touch->DoBoundsOverlap(body)) {
    hit = true;
  }
  if (!hit) {
    return;
  }
  if ((x825_ & 0x10) != 0) { // 0x2668: extrwi 1,27
    mgr.ApplyDamage(GetUniqueId(), player.GetUniqueId(), GetUniqueId(), mJumpSlideDamage,
                    CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
                    CVector3f::Zero()); // fn_8003E25C
  } else {
    mgr.ApplyDamage(GetUniqueId(), player.GetUniqueId(), GetUniqueId(), mStampedeDamage,
                    CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
                    CVector3f::Zero()); // fn_8003E25C
  }
  x824_ &= ~0x20; // 0x277C
}

// fn_18_1A54 (0xE0): the look along the actor's facing. The ray starts at the furthest point of
// the actor's collision box along the facing (the actor's own transform, 0x1AE0) and runs the
// length the caller gives; the answer is "the way is clear", true when the ray hit nothing:
// the retail reads the result's byte at +0x28 and answers true when it is zero (0x1B5C).
bool CEmperorIngStage3::LineOfSight(CStateManager& mgr, const CVector3f& facing,
                                     float length) const {
  CVector3f direction(facing.GetX(), facing.GetY(), facing.GetZ());
  if (!direction.CanBeNormalized()) {
    return false; // 0x1AB0
  }
  direction.Normalize(); // 0x1AB8
  const CAABox bounds = GetCollisionPrimitive()->CalculateAABox(GetTransform()); // 0x1AE0
  const CVector3f start = bounds.FurthestPointAlongVector(direction);           // 0x1AFC
  // The retail's material filter is the type zero (always) with every material in the include
  // list (0x1B38..0x1B48), which is the host's default filter.
  const CRayCastResult result =
      mgr.RayStaticIntersection(start, direction, length, CMaterialFilter()); // 0x1B4C
  const CRayCastResult copy = CopyRayResult(result); // fn_18_3AC (0x20)
  return !copy.IsValid();              // 0x1B5C: the byte the copy has at +0x28
}

// fn_18_3804 (0x25C): the turn towards a position. The facing at guest 0x818 is cleared first
// (0x382C, 0x3840, 0x3848) and only written at the end (0x3970..0x3980): the angle between
// the actor's facing and the direction to the target (0x38A4), snapped to the nearest 45
// degrees (fn_18_3744 at 0x38AC), signed by the up vector's dot with the cross product of the
// two (fn_18_39A0 at 0x38E8, the dot at 0x3908..0x3920), and rotated around Z (0x3958).
void CEmperorIngStage3::FaceTarget(const CVector3f& target) {
  mX818_ = CVector3f::Zero(); // 0x382C, 0x3840, 0x3848
  const CVector3f& mine = GetTranslation();
  CVector3f toTarget(target.GetX() - mine.GetX(), target.GetY() - mine.GetY(),
                     target.GetZ() - mine.GetZ());
  if (!toTarget.CanBeNormalized()) {
    return; // 0x3878
  }
  toTarget.Normalize(); // 0x3880
  const CVector3f facing = GetTransform().GetColumn(kDY);
  const float angle = SnapAngleTo45(CVector3f::GetAngleDiff(facing, toTarget)); // fn_18_3744
  if (angle <= 0.f) {                                                          // rodata 0x258
    return;                                                                    // 0x38C0
  }
  const CVector3f cross = Cross3f(facing, toTarget); // fn_18_39A0
  const CVector3f& up = CVector3f::Up();
  const float turn = up.GetX() * cross.GetX() + up.GetY() * cross.GetY() +
                     up.GetZ() * cross.GetZ(); // 0x3908..0x3920
  const float signedAngle = turn <= 0.f ? -angle : angle; // 0x3924..0x3930
  const CTransform4f turn4f = CTransform4f::RotateZ(CRelAngle::FromRadians(signedAngle)); // 0x3958
  mX818_ = turn4f * facing;                                                            // 0x3968
}

// fn_18_3744 (0xC0): the angle snapped to the nearest 45 degrees. The table at rodata 0x244
// holds 0, pi/4, pi/2, 3pi/4 and pi, and the entry nearest the angle is the answer.
float CEmperorIngStage3::SnapAngleTo45(float angle) const {
  static const float kAngles[] = {0.f, 0.7853982f, 1.5707964f, 2.3561945f,
                                  3.1415927f}; // rodata 0x244
  float best = FLT_MAX; // rodata 0x2C8
  u32 index = 0;
  for (u32 i = 0; i < 5; ++i) {
    const float diff = CMath::AbsF(angle - kAngles[i]);
    if (diff < best) {
      best = diff;
      index = i;
    }
  }
  return kAngles[index];
}

// fn_18_36BC (0x88): the damage's source id against the two hearts' collision actor ids; a
// match sets the flag byte 0x824's bit 0 (the mask 0x01).
void CEmperorIngStage3::OnDamage(CStateManager& mgr, u16 sourceId) {
  const CCollisionActor* first = HeartCollisionActor(mgr, 0);
  if (first != nullptr && first->GetUniqueId().value == sourceId) {
    x824_ |= 0x80; // 0x3718, the bit 24
    return;
  }
  const CCollisionActor* second = HeartCollisionActor(mgr, 1);
  if (second != nullptr && second->GetUniqueId().value == sourceId) {
    x824_ |= 0x80; // 0x3718, the bit 24
  }
}

// @ fn_18_3164
// fn_18_3164 (0x288): one heart's reaction. The heart's collision actor (fn_18_3ACC) has to be
// active (CEntity's flag byte at guest 0x20, the mask 0x80); its health (vtable slot 0x38) at or
// under zero (rodata 0x258) is the answer. With the hit flag (0x80 at guest 0x824, OnDamage) set
// or the heart dead: while invulnerable (state 0) the ing flinches (body command 0x1C,
// CBCAdditiveFlinchCmd at weight 1.0, rodata 0x264), the hit timer (guest 0x860) restarts at
// CPatterned::skDamageHitTime and the attack list gets kind 2 (fn_18_2BF0); while vulnerable
// (state 2) the base's damage cooldown (guest 0x448) restarts at skDamageHitTime, the ing
// flinches, and the ing's health past the heart's - at most the vulnerable damage threshold
// left (guest 0x814) - comes off the threshold and the ing's health and adds to guest 0x8EC.
// Either way the heart-reacted flag (0x80 at guest 0x826) is set.
bool CEmperorIngStage3::HeartDamageReaction(CStateManager& mgr, int heart) {
  CCollisionActor* actor = HeartCollisionActor(mgr, heart); // fn_18_3ACC
  if (actor == nullptr) {
    return false; // port guard: retail reads the actor without a test
  }
  if (!actor->GetActive()) { // 0x319C: extrwi 1,24 is the byte's 0x80
    return false;
  }
  const float heartHP = actor->HealthInfo()->GetHP();
  const bool dead = heartHP <= 0.f;
  if ((x824_ & 0x80) == 0 && !dead) { // 0x31D4: extrwi 1,24
    return dead;
  }
  switch (mVulnerabilityState) {
  case 0:
    BodyController()->CommandMgr().DeliverCmd(CBCAdditiveFlinchCmd(1.f)); // 0x3248
    mX860_ = skDamageHitTime;                                              // 0x3290
    AddListEntry(2);                                                       // fn_18_2BF0
    x826_ |= 0x80;                                                         // 0x32A0
    break;
  case 2: {
    mDamageCooldownTimer = skDamageHitTime;                                // 0x32CC
    BodyController()->CommandMgr().DeliverCmd(CBCAdditiveFlinchCmd(1.f)); // 0x3300
    float damage = HealthInfo()->GetHP() - heartHP;                       // 0x3350
    if (damage >= mVulnerableDamageThreshold) {
      damage = mVulnerableDamageThreshold;
    }
    mVulnerableDamageThreshold -= damage; // 0x336C
    mX8ec_ += damage;                     // 0x3378
    const float hp = HealthInfo()->GetHP();
    HealthInfo()->SetHP(hp - damage); // 0x33AC
    x826_ |= 0x80;                    // 0x33B4
    break;
  }
  default:
    break;
  }
  return dead;
}

// @ fn_18_2F04
// fn_18_2F04 (0x1E0): the hit timer (guest 0x860) runs down by the frame time to zero, then both
// hearts react (fn_18_3164, the second only when the first is alive) and the hit flag (0x80 at
// guest 0x824) clears. A dead heart moves the invulnerable state 0 to 1 - with a knock-back
// (body command 4: no direction, kS_Two, no animation, no restart) when the base's knock-back
// manager (guest 0x620) enables the knock-back reaction (2) - and the vulnerable state 2 to the
// base's pending death (0x10 at guest 0x421, mPendingDeath).
void CEmperorIngStage3::UpdateHeartDamage(CStateManager& mgr, float dt) {
  float timer = mX860_ - dt;
  if (0.f >= timer) {
    timer = 0.f;
  }
  mX860_ = timer;
  bool dead = false;
  if (HeartDamageReaction(mgr, 0) || HeartDamageReaction(mgr, 1)) {
    dead = true;
  }
  x824_ &= ~0x80; // 0x2F88 (rlwimi 7,24,24)
  if (!dead) {
    return;
  }
  switch (mVulnerabilityState) {
  case 0:
    mVulnerabilityState = 1; // 0x2FC0
    if (GetKnockBackController().IsAnimReactionEnabled(CKnockBackMgr::kAR_KnockBack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCKnockBackCmd(CVector3f::Zero(), pas::kS_Two, -1, false)); // 0x303C
    }
    break;
  case 2:
    mPendingDeath = true; // 0x309C (rlwimi 4,27,27 on guest 0x421)
    break;
  default:
    break;
  }
}

// @ fn_18_4BB8
// fn_18_4BB8 (0x84): sets an external param on an element generator and, recursively, on every
// system it spawned.
static void SetExternalParamTree(CElementGen* gen, uint index, float value) {
  if (gen == nullptr) {
    return;
  }
  gen->SetExternalParam(index, value);
  for (int i = 0; i < gen->GetNumSpawnedParticleSystems(); ++i) {
    SetExternalParamTree(static_cast< CElementGen* >(gen->SpawnedParticleSystem(i)), index, value);
  }
}

// @ fn_18_3448
// fn_18_3448 (0x1AC): the heart damage update (fn_18_2F04), then the heart effect of the heart
// that is on (guest 0x868: 0 "EnragedHeart", 1 "DarkHeart", 2 "LightHeart"; none past two) gets
// the growth (guest 0x82C) as param 0, and params 2 and 3 at one past 0.49 / 0.74 (rodata
// 0x2C0, 0x2C4), else zero. The generator is the effect's system (fn_18_3648 copies its
// reference at +0x78).
void CEmperorIngStage3::UpdateHearts(CStateManager& mgr, float dt) {
  UpdateHeartDamage(mgr, dt); // fn_18_2F04
  rstl::string name;
  switch (mX868_) {
  case 0:
    name = rstl::string_l("EnragedHeart");
    break;
  case 1:
    name = rstl::string_l("DarkHeart");
    break;
  case 2:
    name = rstl::string_l("LightHeart");
    break;
  default:
    break;
  }
  CParticleGenInfo* info = ModelData()->AnimationData()->GetFirstParticleEffect(name);
  if (info == nullptr) {
    return;
  }
  CElementGen* gen = static_cast< CElementGen* >(
      static_cast< CParticleGenInfoGeneric* >(info)->GetParticleSystem().GetPtr());
  SetExternalParamTree(gen, 0, mX82c_);
  SetExternalParamTree(gen, 2, mX82c_ > 0.49f ? 1.f : 0.f);
  SetExternalParamTree(gen, 3, mX82c_ > 0.74f ? 1.f : 0.f);
}

// @ fn_18_2D50
// fn_18_2D50 (0x1B4): the contact damage. Not while either flag 0x40/0x20 of guest 0x824 or 0x10
// of guest 0x825 is set (0x2D7C..0x2D98). The hit object's collision actor (the walk below) is
// asked for the object it last touched (the DOL's fn_80137718 is
// CCollisionActor::GetLastTouchedObject); when that is the player and the base's damage time
// (guest 0x440) is not above zero (rodata 0x258), the contact damage (the vtable slot 0xE8)
// goes from the ing to the player with a solid-only filter (lbl_18_data_4EC = 59, kMT_Solid)
// and the ing's facing (the transform's Y column, guest 0x28/0x38/0x48), and the damage time
// restarts at the base's wait (guest 0x444).
void CEmperorIngStage3::OnHitObject(CStateManager& mgr, u16 sourceId) {
  if ((x824_ & 0x40) != 0 || (x824_ & 0x20) != 0 || (x825_ & 0x10) != 0) {
    return; // 0x2D7C..0x2D98
  }
  const CCollisionActor* heart = HeartCollisionActorById(mgr, sourceId); // 0x2DA4
  if (heart == nullptr) {
    return;
  }
  const TUniqueId touched = heart->GetLastTouchedObject(); // fn_80137718
  const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(touched));
  if (player == nullptr || mCurDamageRemTime > 0.f) {
    return;
  }
  mgr.ApplyDamage(GetUniqueId(), touched, GetUniqueId(), GetContactDamage(),
                  CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
                  GetTransform().GetColumn(kDY)); // fn_8003E25C
  mCurDamageRemTime = mDamageWaitTime; // 0x2EC8
}

// fn_18_2D50's walk (0x2D9C..0x2DE0): every description of the manager at guest 0x7C0 whose
// collision actor id is the hit object's, cast to a collision actor.
const CCollisionActor* CEmperorIngStage3::HeartCollisionActorById(CStateManager& mgr,
                                                                u16 id) const {
  if (x7c0_ == nullptr) {
    return nullptr; // port guard: no collision actors before kSM_AreaLoaded
  }
  for (uint i = 0; i < x7c0_->GetNumCollisionActors(); ++i) {
    if (x7c0_->GetCollisionDescFromIndex(i).GetCollisionActorId() != TUniqueId(id)) {
      continue;
    }
    return TCastToPtr< CCollisionActor >(mgr.ObjectById(TUniqueId(id)));
  }
  return nullptr;
}

// fn_18_3B14 (0x4C): the heart's description: the manager's description 0xA (the first heart,
// "mainHeart_SDK", the index at 0x3B2C) or 0xB (the second, "tentacle_6_1", 0x3B44); any other
// heart is no heart (0x3B50). Retail reads the manager without a test; the port's guard covers
// a lookup before kSM_AreaLoaded made it.
const CJointCollisionDescription* CEmperorIngStage3::HeartRecord(int heart) const {
  if (x7c0_ == nullptr) {
    StandIn("heart lookup before kSM_AreaLoaded (no collision actors yet)");
    return nullptr;
  }
  if (heart == 0) {
    return &x7c0_->GetCollisionDescFromIndex(0xA);
  }
  if (heart == 1) {
    return &x7c0_->GetCollisionDescFromIndex(0xB);
  }
  return nullptr; // 0x3B50: any other heart is no heart
}

// fn_18_3ACC (0x48): the heart's collision actor: the description's collision actor id (+0x3C,
// read at 0x3AE8) looked up in the state manager (0x3AFC).
CCollisionActor* CEmperorIngStage3::HeartCollisionActor(CStateManager& mgr, int heart) const {
  const CJointCollisionDescription* record = HeartRecord(heart);
  if (record == nullptr) {
    return nullptr; // the port guard in HeartRecord
  }
  return static_cast< CCollisionActor* >(mgr.ObjectById(record->GetCollisionActorId()));
}

namespace {

// fn_18_3BB0's two tables. rodata 0x18: the ten leg boxes {from, to, extent} - each leg's hip to
// knee at 1.5 and knee to end at 1.0. rodata 0x0: the three spheres {locator, radius}.
struct SEmperorIng3Box {
  const char* from;
  const char* to;
  float extent;
};
const SEmperorIng3Box skLegBoxes[10] = {
    {"F_hip", "F_knee", 1.5f},     {"F_knee", "F_end", 1.f},
    {"L_F_hip", "L_F_knee", 1.5f}, {"L_F_knee", "L_F_end", 1.f},
    {"R_F_hip", "R_F_knee", 1.5f}, {"R_F_knee", "R_F_end", 1.f},
    {"L_B_hip", "L_B_knee", 1.5f}, {"L_B_knee", "L_B_end", 1.f},
    {"R_B_hip", "R_B_knee", 1.5f}, {"R_B_knee", "R_B_end", 1.f},
};
struct SEmperorIng3Sphere {
  const char* locator;
  float radius;
};
const SEmperorIng3Sphere skSpheres[3] = {
    {"mainHeart_SDK", 2.f}, // description 0xA, the first heart
    {"tentacle_6_1", 1.f},  // description 0xB, the second heart
    {"Skeleton_Root", 3.9f},
};

} // namespace

// fn_18_3BB0 (0x450): the collision actors. The vector reserves 0xD (0x3BDC); the ten leg boxes
// (OBBAutoSizeCollision, fn_80137338: the two locators' segments on the actor's own animation
// data, a cube of the extent, kOT_BetweenJoints (`li r7, 0x1`, 0x3CE8), the name from + "->"
// (rodata 0x701) + to, mass 10.0 (rodata 0x294)) and then the three spheres (SphereCollision,
// fn_80137460: the locator's segment, no offset, the radius, the locator's name, mass 10.0). The
// manager (fn_80136434, 0x3E9C) belongs to this actor, in its area, active as the actor is (the
// byte at +0x20, extrwi bit 24, 0x3E70); the old one is deleted first (fn_801362C4, 0x3EAC).
void CEmperorIngStage3::SetupCollisionActors(CStateManager& mgr) {
  rstl::vector< CJointCollisionDescription > descs;
  descs.reserve(0xD);
  const CAnimData* animData = GetModelData()->GetAnimationData();
  for (int i = 0; i < 10; ++i) {
    const SEmperorIng3Box& box = skLegBoxes[i];
    const CSegId from = animData->GetLocatorSegId(rstl::string_l(box.from));
    const CSegId to = animData->GetLocatorSegId(rstl::string_l(box.to));
    descs.push_back(CJointCollisionDescription::OBBAutoSizeCollision(
        from, to, CVector3f(box.extent, box.extent, box.extent),
        CJointCollisionDescription::kOT_BetweenJoints,
        rstl::string_l(box.from) + rstl::string("->") + rstl::string_l(box.to), 10.f));
  }
  for (int i = 0; i < 3; ++i) {
    const SEmperorIng3Sphere& sphere = skSpheres[i];
    const CSegId seg = animData->GetLocatorSegId(rstl::string_l(sphere.locator));
    descs.push_back(CJointCollisionDescription::SphereCollision(
        seg, CVector3f::Zero(), sphere.radius, rstl::string_l(sphere.locator), 10.f));
  }
  CCollisionActorManager* manager =
      rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), descs, GetActive());
  delete x7c0_;
  x7c0_ = manager;

  // This actor's own filter (0x3EB4..0x3F78): include kMT_Solid (0x3B); exclude
  // kMT_CollisionActor (0x2F), kMT_Character (0x21), kMT_AIPassthrough (0x1C), kMT_Player (0x20)
  // and kMT_Projectile (0x23); type 3, kFT_IncludeExclude.
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid), CMaterialList(kMT_CollisionActor, kMT_Character,
                                              kMT_AIPassthrough, kMT_Player, kMT_Projectile)));
  // Both hearts take the normal vulnerability (fn_80137724, 0x3F9C and 0x3FC0) and the second
  // heart starts inactive (fn_18_214 with 1 and false, 0x3FCC).
  HeartCollisionActor(mgr, 0)->SetDamageVulnerability(CDamageVulnerability::NormalVulnerabilty());
  HeartCollisionActor(mgr, 1)->SetDamageVulnerability(CDamageVulnerability::NormalVulnerabilty());
  SetHeartActive(mgr, 1, false);
}

// fn_18_214 (0x48): a heart's collision actor (fn_18_3ACC), when there is one, is switched on or
// off through its vtable slot 0x1C (SetActive).
// @ fn_18_214
void CEmperorIngStage3::SetHeartActive(CStateManager& mgr, int heart, bool active) {
  CCollisionActor* actor = HeartCollisionActor(mgr, heart);
  if (actor != nullptr) {
    actor->SetActive(active);
  }
}

// fn_18_39E0 (0xA4): while either heart's collision actor (fn_18_3A84, the const lookup) is
// active (the byte at +0x20, bit 7) the actor is a target - kMT_Target (0x28), kMT_Orbit (0x29)
// and kMT_SeekerTarget (0x3F) are added; otherwise they are removed.
void CEmperorIngStage3::UpdateTargetMaterials(CStateManager& mgr) {
  const CEntity* first = mgr.GetObjectById(HeartRecord(0)->GetCollisionActorId());
  const CEntity* second = mgr.GetObjectById(HeartRecord(1)->GetCollisionActorId());
  if (first->GetActive() || second->GetActive()) {
    AddMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    return;
  }
  RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
}

// fn_18_22BC (0x24): the module's average (fn_18_AB9C) over the list at guest 0x830 (`addi r3,
// r3, 0x830` at 0x22C4).
float CEmperorIngStage3::BoneAverage() const { return ListAverage(mX830_); }

// fn_18_AB9C (0x64): the weighted average over a list: every entry counts its weight times
// what is left of its threshold (one minus the timer over the threshold, capped at one).
float CEmperorIngStage3::ListAverage(const SEmperorIngList& list) const {
  float total = 0.f; // rodata 0x258
  for (int i = 0; i < list.size(); ++i) {
    const SEmperorIngListEntry& entry = list[i];
    float ratio = entry.mTimer / entry.mThreshold;
    if (ratio >= 1.f) { // rodata 0x264
      ratio = 1.f;
    }
    total += entry.mWeight * (1.f - ratio);
  }
  return total;
}

// fn_18_AC44 (0xB4): the list walk: every entry's timer grows by the given time, and an entry
// whose timer is past its threshold is erased (fn_18_ACF8 / fn_18_AD44, the vector's erase).
void CEmperorIngStage3::UpdateList(SEmperorIngList& list, float dt) {
  SEmperorIngList::iterator it = list.begin();
  while (it != list.end()) {
    it->mTimer += dt;
    if (it->mTimer > it->mThreshold) {
      it = list.erase(it);
    } else {
      ++it;
    }
  }
}

// fn_18_AC00 (0x44): an entry {weight, threshold, timer 0} is added only while the list is
// under its capacity (the count at +0x4 against +0x8).
// @ fn_18_AC00
void CEmperorIngStage3::PushListEntry(SEmperorIngList& list, float weight, float threshold) {
  if (list.size() < list.capacity()) {
    SEmperorIngListEntry entry;
    entry.mWeight = weight;
    entry.mThreshold = threshold;
    entry.mTimer = 0.f; // rodata 0x258
    list.push_back(entry);
  }
}

// fn_18_2BF0 (0xC4): the entry kinds for the list at guest 0x830 - 0 adds {75, 15} (rodata
// 0x2B0, 0x2B4), 1 {200, 10} (0x2A4, 0x294), 2 {25, 5} (0x2B8, 0x2BC); 3 ages the list by 5.0
// (0x2BC).
// @ fn_18_2BF0
void CEmperorIngStage3::AddListEntry(int kind) {
  switch (kind) {
  case 0:
    PushListEntry(mX830_, 75.f, 15.f);
    break;
  case 1:
    PushListEntry(mX830_, 200.f, 10.f);
    break;
  case 2:
    PushListEntry(mX830_, 25.f, 5.f);
    break;
  case 3:
    UpdateList(mX830_, 5.f);
    break;
  default:
    break;
  }
}

// fn_18_1978 (0xDC): the module's flag check over the list at guest 0x850. While the list's
// average is below one (rodata 0x264) the player far away (over fifty units, rodata 0x290)
// sets the flag byte 0x824's bit 29 (the mask 0x04), and otherwise a random draw (0.0 to
// 100.0, rodata 0x258 and 0x27C) under ten (rodata 0x294) sets that bit and the bit 28
// (the mask 0x08) otherwise - unless one of the two bits is already set.
void CEmperorIngStage3::CheckList(CStateManager& mgr) {
  if (ListAverage(mX850_) >= 1.f) { // rodata 0x264
    return;
  }
  if (PlayerDistance(mgr) > 50.f) { // fn_18_2CB8, rodata 0x290
    x824_ |= 0x04;
  }
  if ((x824_ & 0x04) != 0) {
    return;
  }
  if ((x824_ & 0x08) != 0) {
    return;
  }
  if (mgr.Random()->Range(0.f, 100.f) < 10.f) { // rodata 0x258, 0x27C, 0x294
    x824_ |= 0x04;
  } else {
    x824_ |= 0x08;
  }
}

// fn_18_2D1C (0x34): the vector from the actor to the first player, flattened to the XY plane
// (the zero at rodata 0x258).
CVector3f CEmperorIngStage3::PlayerDirection(CStateManager& mgr) const {
  const CVector3f& mine = GetTranslation();
  const CVector3f& theirs = mgr.GetPlayer(0)->GetTranslation();
  return CVector3f(theirs.GetX() - mine.GetX(), theirs.GetY() - mine.GetY(), 0.f);
}

// fn_18_2CB8 (0x64): the distance from the actor to the player (the vector flattened to the
// XY plane, the zero at rodata 0x258).
float CEmperorIngStage3::PlayerDistance(CStateManager& mgr) const {
  return PlayerDirection(mgr).Magnitude();
}

// @ fn_18_25C
// fn_18_25C (0x150): the room ahead. A ray from the point of the collision primitive's box
// (the vtable slot 0x7C, the box at the transform) furthest along the facing (the transform's Y
// column) runs along the facing for 1000 units (rodata 0x260, the default filter); a hit
// answers its distance (fn_18_3AC copies the result), a miss answers nothing.
bool CEmperorIngStage3::ForwardClearance(CStateManager& mgr, float& distance) const {
  const CVector3f facing = GetTransform().GetColumn(kDY);
  const CAABox box = GetCollisionPrimitive()->CalculateAABox(GetTransform());
  const CRayCastResult result = mgr.RayStaticIntersection(box.FurthestPointAlongVector(facing),
                                                          facing, 1000.f, CMaterialFilter());
  const CRayCastResult copy = CopyRayResult(result); // fn_18_3AC
  if (!copy.IsValid()) {
    return false;
  }
  distance = copy.GetTime();
  return true;
}

// @ fn_18_12C4
// fn_18_12C4 (0xAC): the beam (guest 0x8AC, the CPlasmaProjectile fn_18_1530 makes) is reset
// (the vtable slot 0x90, ResetBeam(mgr, false)) and deleted, its id cleared, and its looped
// sound (guest 0x8B0) removed.
void CEmperorIngStage3::StopBeam(CStateManager& mgr) {
  CPlasmaProjectile* beam = static_cast< CPlasmaProjectile* >(mgr.ObjectById(TUniqueId(mBeamId)));
  if (beam == nullptr) {
    return;
  }
  beam->ResetBeam(mgr, false);
  mgr.DeleteObjectRequest(TUniqueId(mBeamId));
  mBeamId = kInvalidUniqueId.value;
  if (mBeamSfx) {
    CSfxManager::RemoveEmitter(mBeamSfx);
    mBeamSfx = CSfxHandle();
  }
}

// @ fn_18_1370
// fn_18_1370 (0x1C0): the beam (guest 0x8AC), while there is one, aims from the "DarkBeam_LCTR_SDK"
// locator (fn_18_DBC) at a point (guest 0x8B4) that moves 23 units a second (rodata 0x280) towards
// the player's X and Y and the actor's own Z; the beam takes the look-at transform (the vtable
// slot 0x8C, UpdateFx). In the release state (0x5E8 == 3) its sound (guest 0x8B0) is removed;
// otherwise the sound follows the beam's end (beam+0x444, GetCurrentPos).
void CEmperorIngStage3::SteerBeam(CStateManager& mgr, float dt) {
  CPlasmaProjectile* beam = static_cast< CPlasmaProjectile* >(mgr.ObjectById(TUniqueId(mBeamId)));
  if (beam == nullptr) {
    return;
  }
  const CVector3f origin = DarkBeamLocatorPosition(); // fn_18_DBC
  const CVector3f& playerPos = mgr.Player(0)->GetTranslation();
  CVector3f toPlayer(playerPos.GetX() - mX8b4_.GetX(), playerPos.GetY() - mX8b4_.GetY(),
                     GetTranslation().GetZ() - mX8b4_.GetZ());
  toPlayer.Normalize();
  mX8b4_ += toPlayer * 23.f * dt; // rodata 0x280
  beam->UpdateFx(CTransform4f::LookAt(origin, mX8b4_, CVector3f::Up()), dt, mgr);
  if (beam->GetExpansionState() == CPlasmaProjectile::kES_Release) {
    if (mBeamSfx) {
      CSfxManager::RemoveEmitter(mBeamSfx);
      mBeamSfx = CSfxHandle();
    }
  } else {
    CSfxManager::UpdateEmitter(mBeamSfx, beam->GetCurrentPos(), CVector3f::Up(), 0x7f);
  }
}

// @ fn_18_1B88
// fn_18_1B88 (0x28): the heart switch (fn_18_1DF4) with heart 3, no heart, and no vulnerability.
void CEmperorIngStage3::HeartsOff(CStateManager& mgr) { SetHeartState(mgr, 3, false); }

// fn_18_4D0's tables. rodata 0xC0: per portal attack (guest 0x8E4) eight X/Y/Z angles in degrees,
// one per portal of the attack (guest 0x8E0). bss 0x0 (written by the static init fn_18_E180,
// rodata 0x258/0x33C/0x2AC/0x2BC): the attack's offset added to each portal's position.
static const float skPortalAngles[4][8][3] = {
    {{10.f, 0.f, 90.f}, {170.f, 0.f, 90.f}, {30.f, 0.f, 90.f}, {150.f, 0.f, 90.f},
     {50.f, 0.f, 90.f}, {130.f, 0.f, 90.f}, {70.f, 0.f, 90.f}, {110.f, 0.f, 90.f}},
    {{0.f, 0.f, -70.f}, {0.f, 0.f, 70.f}, {0.f, 0.f, -50.f}, {0.f, 0.f, 50.f},
     {0.f, 0.f, -30.f}, {0.f, 0.f, 30.f}, {0.f, 0.f, -10.f}, {0.f, 0.f, 10.f}},
    {{0.f, 0.f, 180.f}, {0.f, 0.f, 135.f}, {0.f, 0.f, 225.f}, {0.f, 0.f, 90.f},
     {0.f, 0.f, 270.f}, {0.f, 0.f, 45.f}, {0.f, 0.f, 315.f}, {0.f, 0.f, 0.f}},
    {{3.5f, 0.f, -2.5f}, {0.f, 0.f, 1.5f}, {-6.f, 0.f, 7.f}, {7.f, 0.f, -4.5f},
     {-7.f, 0.f, 4.f}, {8.f, 0.f, 10.f}, {-3.f, 0.f, -6.f}, {1.5f, 0.f, 0.f}},
};
static const float skPortalOffsets[4][3] = {
    {0.f, 0.f, 0.f}, {0.f, 0.f, 2.85f}, {0.f, 0.f, 4.f}, {0.f, 0.f, 5.f}};

// @ fn_18_4D0
// fn_18_4D0 (0x3C0): one Ing mini portal (module 29's CIngMiniPortalAttack, fn_29_11E8C) per egg-lay
// event. The portal's angles (rodata 0xC0, degrees by rodata 0x26C) are taken from the attack
// (guest 0x8E4) and the portal count (guest 0x8E0, which steps on by one, the signed remainder
// of eight); the Z*Y*X rotation applied to the actor's transform carries 20 units forward
// (rodata 0x270), and the attack's offset (bss 0x0) is added. The portal's info (fn_29_12100)
// aims at the player for 1 s (rodata 0x264) with a 2 s life (0x274), the effect (guest 0xD04),
// the open sound (0xD14), 500 / 1 (0x278, 0x264) and the beam info (0xD34); its damage (0xD18)
// is scaled by dt and made immunity-proof (0x1A bit 7). The 0x238-byte "Portal" gets a fresh
// id, the actor's area and the null connection list, unit scale and the actor as its owner.
void CEmperorIngStage3::EggLayEvent(CStateManager& mgr, float dt) {
  const int portal = mX8e0_;
  const int attack = mX8e4_;
  mX8e0_ = (portal + 1) % 8; // 0x518..0x548
  if (!mPortalEffect) {
    return; // host guard: retail passes the empty token storage when the asset is unset
  }
  const float* angles = skPortalAngles[attack & 3][portal & 7];
  CTransform4f xf = GetTransform();
  const float deg = 0.017453292f; // rodata 0x26C
  const CQuaternion rot = CQuaternion::ZRotation(CRelAngle::FromRadians(deg * angles[2])) *
                          CQuaternion::YRotation(CRelAngle::FromRadians(deg * angles[1])) *
                          CQuaternion::XRotation(CRelAngle::FromRadians(deg * angles[0]));
  const CVector3f pos = (GetTransform() * rot.BuildTransform4f()) * (CVector3f::Forward() * 20.f);
  const float* offset = skPortalOffsets[attack & 3];
  xf.SetTranslation(CVector3f(offset[0], offset[1], offset[2]) + pos); // 0x194/0x1A4/0x1B4
  CDamageInfo damage(mPortalDamage);
  damage.MultiplyDamage(dt);  // 0x6E0
  damage.SetNoImmunity(true); // 0x71C
  const CIngMiniPortalInfo info(mgr.Player(0)->GetUniqueId(), 1.f, 2.f, *mPortalEffect,
                                mPortalSound, 500.f, 1.f, damage, mPortalBeamInfo);
  CIngMiniPortalAttack* attackObj = rs_new CIngMiniPortalAttack(
      mgr.AllocateUniqueId(), rstl::string_l("Portal"),
      CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true), xf, GetUniqueId(),
      CVector3f(1.f, 1.f, 1.f), info);
  mgr.AddObject(attackObj);
}

// @ fn_18_BB8
// fn_18_BB8 (0x204): the light swarm (PortCEmperorIngLightSwarm.cpp). While none is out (guest
// 0x8C0 invalid) one is spawned at the "Swarm_LCTR_SDK" locator (fn_18_817C), drawn after the
// Ing (its next draw node, 0xD84). Its only connection is a copy of the Ing's first
// INTERNAL_STATE_03 connection with the state changed to DEAD (0xC28..0xC9C), so the swarm's
// death sends the Ing's IS03 message.
void CEmperorIngStage3::BeginActionEvent(CStateManager& mgr) {
  if (mX8c0_ != kInvalidUniqueId.value) {
    return;
  }
  mX8c0_ = mgr.AllocateUniqueId().value;
  rstl::vector< SConnection > conns;
  conns.reserve(1); // fn_18_DB5C
  const rstl::vector< SConnection >& own = GetConnectionList();
  for (rstl::vector< SConnection >::const_iterator it = own.begin(); it != own.end(); ++it) {
    if (it->state == kSS_InternalState3) {
      SConnection conn = *it;
      conn.state = kSS_Dead;
      conns.push_back(conn);
      break;
    }
  }
  CEmperorIngLightSwarm* swarm = rs_new CEmperorIngLightSwarm(
      TUniqueId(mX8c0_), CEntityInfo(GetCurrentAreaId(), conns, true, kInvalidEditorId),
      CActorParameters(), mLightSwarmData, mLightSwarmEffect, GetUniqueId(),
      EmperorIngSwarmLocator(*this), mLightSwarmDeathSound, mLightSwarmExpireSound);
  if (swarm != nullptr) {
    swarm->SetNextDrawNode(GetUniqueId());
    mgr.AddObject(swarm);
  }
}

// @ fn_18_1530
// fn_18_1530 (0x3B4): the beam start. The light beam with 0x80 of guest 0x825, else the dark one:
// its beam info (guest 0xAFC / 0xA88, TLdrToBeamInfo with 0x301 and the copy fn_18_18E4) and its
// projectile info's damage (0xAE0 / 0xA6C) with the amount zeroed - the amount becomes the beam's
// initial damage - make the 0x6A8-byte "EmperorIngStage3Beam" CPlasmaProjectile: the projectile
// info's weapon (0xAD4 / 0xA60), the light or dark weapon type (2 / 1), the identity transform,
// kMT_ProjectilePassthrough (0x14), a fresh id, the default asset info and the attributes 0xA0000
// / 0x21000. The aim point (guest 0x8B4) starts 20 units (rodata 0x270) along the facing from the
// actor; the beam's damage duration is 3 (rodata 0x284: 0x158 |= 0x1000, 0x1C0) and it fires
// (the vtable slot 0x94) along the look-at from the DarkBeam locator (fn_18_DBC) before it is
// added. Its looped sound (0xB44 / 0xAD0, emitter 150/0.1, rodata 0x288/0x28C) starts at the
// locator; the handle goes to guest 0x8B0 and the id to 0x8AC.
void CEmperorIngStage3::StartBeam(CStateManager& mgr) {
  const bool light = (x825_ & 0x80) != 0; // 0x1554: extrwi 1,24
  const CBeamInfo beamInfo =
      TLdrToBeamInfo(light ? mLightBeamInfo : mDarkBeamInfo, 0x301); // fn_80239B64, fn_18_18E4
  const TUniqueId uid = mgr.AllocateUniqueId();
  CProjectileInfo& projectile = light ? mLightBeamProjectile : mDarkBeamProjectile;
  CDamageInfo damage = projectile.GetDamage();
  const float initialDamage = damage.GetDamage();
  damage.SetDamage(0.f); // rodata 0x258
  CPlasmaProjectile* beam = rs_new CPlasmaProjectile(
      projectile.Token(), rstl::string_l("EmperorIngStage3Beam"), light ? kWT_Light : kWT_Dark,
      beamInfo, CTransform4f::Identity(), kMT_ProjectilePassthrough, damage, uid,
      GetCurrentAreaId(), GetUniqueId(), CWeaponAssetInfo(), false, light ? 0xA0000 : 0x21000);
  if (beam == nullptr) {
    return;
  }
  beam->SetInitialDamage(initialDamage);
  const CVector3f origin = DarkBeamLocatorPosition(); // fn_18_DBC
  mX8b4_ = GetTranslation() + GetTransform().GetColumn(kDY) * 20.f; // rodata 0x270
  const CTransform4f xf = CTransform4f::LookAt(origin, mX8b4_, CVector3f::Up());
  beam->SetDamageDuration(3.f); // rodata 0x284
  beam->Fire(xf, mgr, false);   // vtable slot 0x94
  mgr.AddObject(*beam);         // fn_80041E60
  CAudioSys::C3DEmitterParmData parms(150.f, 0.1f, 1, 0x7f, 0x14); // rodata 0x288, 0x28C
  parms.mPos = origin;
  parms.mDir = CVector3f::Up();
  parms.mSfxId = light ? mLightBeamSound : mDarkBeamSound;
  mBeamSfx = CSfxManager::AddEmitter(parms, GetCurrentAreaId().Value(), true, true,
                                     CSfxManager::kMedPriority);
  mBeamId = uid.value;
}

// fn_18_2C98 (0x20): the actor's facing (the transform's Y column), flattened to the XY plane.
CVector3f CEmperorIngStage3::FlatFacing() const {
  const CVector3f facing = GetTransform().GetColumn(kDY);
  return CVector3f(facing.GetX(), facing.GetY(), 0.f);
}

// fn_18_1BD8 (0x5C): both hearts get the module's vulnerability (guest 0x86C). Retail has no
// tests; the port's only cover the guard in HeartRecord (no collision actors before
// kSM_AreaLoaded).
void CEmperorIngStage3::SetHeartVulnerability(CStateManager& mgr) const {
  CCollisionActor* first = HeartCollisionActor(mgr, 0); // fn_18_3ACC
  if (first != nullptr) {
    first->SetDamageVulnerability(mX86c_); // 0x1C00
  }
  CCollisionActor* second = HeartCollisionActor(mgr, 1); // fn_18_3ACC
  if (second != nullptr) {
    second->SetDamageVulnerability(mX86c_); // 0x1C18
  }
}

// fn_18_1C34 (0x98): the first heart's vulnerability is saved at guest 0x86C (fn_18_1CCC, the
// CDamageVulnerability copy) and both hearts turn immune (fn_800DBB60). Retail has no tests;
// the port's match fn_18_1BD8's (no collision actors before kSM_AreaLoaded).
void CEmperorIngStage3::SetHeartsImmune(CStateManager& mgr) {
  CCollisionActor* first = HeartCollisionActor(mgr, 0); // fn_18_3ACC
  if (first != nullptr) {
    mX86c_ = *first->GetDamageVulnerability();
    first->SetDamageVulnerability(CDamageVulnerability::ImmuneVulnerabilty());
  }
  CCollisionActor* second = HeartCollisionActor(mgr, 1); // fn_18_3ACC
  if (second != nullptr) {
    second->SetDamageVulnerability(CDamageVulnerability::ImmuneVulnerabilty());
  }
}

// fn_18_2B68 (0x88): the entry kinds for the list at guest 0x840 - 0 adds {200, 8} (rodata
// 0x2A4, 0x2A8), 1 {100, 4} (0x27C, 0x2AC); 2 ages the list by 10.0 (0x294).
// @ fn_18_2B68
void CEmperorIngStage3::AddTauntEntry(int kind) {
  switch (kind) {
  case 0:
    PushListEntry(mX840_, 200.f, 8.f);
    break;
  case 1:
    PushListEntry(mX840_, 100.f, 4.f);
    break;
  case 2:
    UpdateList(mX840_, 10.f);
    break;
  default:
    break;
  }
}

// fn_18_1BB0 (0x28): the heart switch with the heart that is on (guest 0x868) and no
// vulnerability change.
void CEmperorIngStage3::SetCurrentHeart(CStateManager& mgr) { SetHeartState(mgr, mX868_, false); }

// fn_18_8C4C (0x13C): "ShouldMeleeAttack". The module's time since the last attack (guest
// 0x7D0) has to be at least 2.0 (rodata 0x274), the module's average over the bone list runs
// (fn_18_22BC) and the time has to be at least 10.0 (rodata 0x294). The vector to the player
// (fn_18_2D1C) has to be normalisable and shorter than 30.0 (rodata 0x2FC), and the player has
// to be in front of the actor: the cosine of the angle between the actor's flattened facing
// (fn_18_2C98) and the direction to the player has to be above 1.3089969 (rodata 0x318).
bool CEmperorIngStage3::ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const {
  if (mX7d0_ < 2.f) { // rodata 0x274
    return false;
  }
  if (BoneAverage() < 10.f) { // fn_18_22BC, rodata 0x294
    return false;
  }
  const CVector3f toPlayer = PlayerDirection(mgr); // fn_18_2D1C
  if (!toPlayer.CanBeNormalized()) {
    return false;
  }
  if (toPlayer.Magnitude() >= 30.f) { // rodata 0x2FC
    return false;
  }
  const CVector3f facing = FlatFacing(); // fn_18_2C98
  // The retail takes the cosine of the constant (rodata 0x318) and compares the dot with it
  // (0x8D40); the direction is not normalised here, only the facing is a unit vector.
  return CVector3f::Dot(facing, toPlayer) > CMath::FastCosR(1.3089969f); // rodata 0x318
}

// fn_18_8ACC (0x180): "ShouldGroundPound". The module's time since the last attack (guest
// 0x7D0) has to be at least 2.0 (rodata 0x274). The vector from the ing's "head" locator
// (rodata 0x704) to the player has to be normalisable, and the player has to be below the ing:
// the cosine of the angle between the direction to the player and the ing's down axis (the
// negated Z column of the transform) has to be above 0.7853982 (rodata 0x314).
bool CEmperorIngStage3::ShouldGroundPound(CStateManager& mgr, const CTriggerData& data) const {
  if (mX7d0_ < 2.f) { // rodata 0x274
    return false;
  }
  const CVector3f head =
      (GetTransform() * GetScaledLocatorTransform(rstl::string_l("head"))).GetTranslation();
  const CVector3f& theirs = mgr.GetPlayer(0)->GetTranslation();
  CVector3f toPlayer(theirs.GetX() - head.GetX(), theirs.GetY() - head.GetY(),
                     theirs.GetZ() - head.GetZ());
  if (!toPlayer.CanBeNormalized()) {
    return false;
  }
  toPlayer = toPlayer.AsNormalized();
  const CVector3f down = GetTransform().GetColumn(kDZ);
  // The retail takes the cosine of the constant (rodata 0x314) and compares the dot with it
  // (0x8C04).
  const float dot = -down.GetZ() * toPlayer.GetZ() +
                    (-down.GetX() * toPlayer.GetX() + -down.GetY() * toPlayer.GetY());
  return dot > CMath::FastCosR(0.7853982f); // rodata 0x314
}

// fn_18_5264 (0x2F8): the "EyeShut" state.
//  - Activate (0x52B0): the eye-shut time. The module's time (guest 0x8EC) times two percent
//    (rodata 0x274) over a hundred (rodata 0x27C), at most 2.5 (rodata 0x298), goes to guest
//    0x8F0 (at least 2.5, rodata 0x298) and the module's time is cleared; a random time between one (rodata 0x264) and 1.2
//    (rodata 0x2EC) times that, at most 5.0 (rodata 0x2BC), goes to guest 0x8F4; the eye-shut
//    time then is at least 2.5 (rodata 0x298) plus that random time; the animation state at
//    set.
//  - Update (0x5370): the body's eye-shut commands. The module's command classes (the vtables
//    at lbl_18_data_744, _750 and _78C) are weak copies of the host's CBCAdditiveReactionCmd,
//    CBCLoopAttackCmd and CBodyStateCmd; the DOL's fn_801B73E4 is CAnimationState's
//    CanIssueCommand.
//  - Deactivate (0x5518): the eye-shut timer (guest 0x7CC) is cleared, both hearts get the
//    module's vulnerability (fn_18_1BD8) and the heart switch runs with the heart that is on
//    (fn_18_1BB0); the animation state at guest 0x6B4 is set to `kAS_NotReady` (0).
// @ fn_18_5264
void CEmperorIngStage3::EyeShut(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    float time = mX8ec_ * 2.f / 100.f; // rodata 0x274, 0x27C
    if (2.5f >= time) {              // rodata 0x298: at least 2.5 (fcmpo/bge, 0x52D8)
      time = 2.5f;
    }
    mX8f0_ = time;
    mX8ec_ = 0.f;
    float range = 1.2f * mX8f0_; // rodata 0x2EC
    if (range > 5.f) {            // rodata 0x2BC
      range = 5.f;
    }
    mX8f4_ = mgr.Random()->Range(1.f, range); // rodata 0x264
    if (mX8f0_ < 2.5f + mX8f4_) { // rodata 0x298
      mX8f0_ = 2.5f + mX8f4_;
    }
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x5368, the value one
    break;
  }
  case kStateMsg_Update:
    // The loop attack while the body can take it (0x5370..0x540C); otherwise the random timer
    // runs down and, once below zero (rodata 0x258), plays the additive reaction 4 at full
    // weight (rodata 0x264) and is parked at FLT_MAX (rodata 0x2C8); the state then ends once
    // the state machine's time is past the eye-shut time (0x54C4..0x5514).
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_Zero, false));
      break;
    }
    mX8f4_ -= dt;
    if (mX8f4_ < 0.f) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCAdditiveReactionCmd(pas::kART_Four, 1.f, false));
      mX8f4_ = FLT_MAX;
    }
    if (mStateMachine->GetTime() > mX8f0_) {
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    }
    break;
  case kStateMsg_Deactivate:
    mX7cc_ = 0.f;
    SetHeartVulnerability(mgr); // fn_18_1BD8
    SetCurrentHeart(mgr);      // fn_18_1BB0
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x5538, the value zero
    break;
  default:
    break;
  }
}

// fn_18_1DF4 (0x4A0): the heart switch. Heart 0 is the enraged heart: the two hearts get the
// yellow health (guest 0x920, 0x1E8C) and the "EnragedHeart" effect; heart 1 the dark
// heart: the actor's own health (the vtable slot 0x3C, 0x1F80) and the "DarkHeart" effect;
// heart 2 the light heart: the same health (0x208C) and the "LightHeart" effect. The heart
// that is on is remembered at guest 0x868 (0x2164) and, when the caller asks for it, the two
// hearts get that heart's vulnerability (0x960 red, 0x9C0 dark, 0x990 light, 0x219C..0x21E4).
// The lookup of the two hearts finds none while the module's holder is not created, and
// retail returns early then (0x1E34), so the body below does not run on the host.
void CEmperorIngStage3::SetHeartState(CStateManager& mgr, int heart, bool applyVulnerability) {
  CCollisionActor* first = HeartCollisionActor(mgr, 0);  // fn_18_3ACC
  CCollisionActor* second = HeartCollisionActor(mgr, 1); // fn_18_3ACC
  if (first == nullptr) {
    return; // 0x1E34: the retail's own test of the first heart
  }
  bool enraged = false;
  bool light = false;
  bool dark = false;
  switch (heart) {
  case 0:
    if (heart != mX868_) {
      *first->HealthInfo() = mYellowHealth;  // 0x1E8C: the yellow health at guest 0x920
      *second->HealthInfo() = mYellowHealth; // 0x1FDC
    }
    enraged = true; // 0x1F40
    break;
  case 1:
    if (heart != mX868_) {
      *first->HealthInfo() = *GetHealthInfo();  // 0x1F80: the actor's own health
      *second->HealthInfo() = *GetHealthInfo(); // 0x2000
    }
    dark = true; // 0x204C
    break;
  case 2:
    if (heart != mX868_) {
      *first->HealthInfo() = *GetHealthInfo();  // 0x208C
      *second->HealthInfo() = *GetHealthInfo(); // 0x2108
    }
    light = true; // 0x2158
    break;
  default:
    break;
  }
  if (heart != 3) {
    mX868_ = heart; // 0x2164
  }
  if (applyVulnerability) {
    switch (mX868_) {
    case 0:
      first->SetDamageVulnerability(mRedVulnerability);  // 0x219C: guest 0x960
      second->SetDamageVulnerability(mRedVulnerability); // 0x21A8
      break;
    case 1:
      first->SetDamageVulnerability(mDarkVulnerability);  // 0x21B4: guest 0x9C0
      second->SetDamageVulnerability(mDarkVulnerability); // 0x21C0
      break;
    case 2:
      first->SetDamageVulnerability(mLightVulnerability);  // 0x21D0: guest 0x990
      second->SetDamageVulnerability(mLightVulnerability); // 0x21DC
      break;
    default:
      break;
    }
  }
  AnimationData()->SetEffectState(rstl::string_l("EnragedHeart"), enraged, mgr); // 0x2210
  AnimationData()->SetEffectState(rstl::string_l("LightHeart"), light, mgr);     // 0x2244
  AnimationData()->SetEffectState(rstl::string_l("DarkHeart"), dark, mgr);       // 0x2278
}

// The module's state machine functions. The eleven triggers whose retail body is small enough
// are ported; the rest are stand-ins that log the retail function they stand in for and return the
// neutral value (false for a trigger, nothing for a state or a code).
// fn_18_8DA4 (0x8DA4): "TooClose". The module's own distance (guest 0x7D4) has to be at least
// 2.0 (rodata 0x274) and the player then has to be nearer than 20 units (rodata 0x270). A vector
// that cannot be normalized counts as too close.
// fn_18_2D1C (0x34) is the vector from the actor to the first player, flattened to the XY plane;
// fn_18_2C98 (0x20) copies a vector the retail then never reads, so it is left out.
bool CEmperorIngStage3::TooClose(CStateManager& mgr, const CTriggerData& data) const {
  if (mX7d4_ < 2.f) { // rodata 0x274
    return false;
  }
  const CVector3f toPlayer = PlayerDirection(mgr); // fn_18_2D1C
  if (!toPlayer.CanBeNormalized()) {
    return true;
  }
  return toPlayer.Magnitude() < 20.f; // rodata 0x270
}

// fn_18_8D88 (0x8D88): the attack timer (guest 0x7E4) against zero (rodata 0x258).
bool CEmperorIngStage3::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mAttackTimer < 0.f;
}

// fn_18_8A4C (0x8A4C): "ShouldTaunt". The look check has to pass (rodata 0x2E4 = 60.0f),
// the module's taunt average runs and has to be above 50.0 (rodata 0x290), and the taunt
// timer (guest 0x7C8) has to be past the taunt frequency (guest 0x91C).
bool CEmperorIngStage3::ShouldTaunt(CStateManager& mgr, const CTriggerData& data) const {
  if (!RandomCheck(mgr, 60.f)) { // rodata 0x2E4
    return false;
  }
  if (TauntAverage() <= 50.f) { // fn_18_2298, rodata 0x290
    return false;
  }
  return mX7c8_ > mTauntFrequency;
}

// fn_18_897C (0x897C): the action (guest 0x7E8) is 0.
bool CEmperorIngStage3::ShouldStampede(CStateManager& mgr, const CTriggerData& data) const {
  return mAction == 0;
}

// fn_18_8A34 (0x8A34): the flag byte at guest 0x824, the bit 28 (the mask 0x08).
bool CEmperorIngStage3::ShouldDodge(CStateManager& mgr, const CTriggerData& data) const {
  return (x824_ & 0x08) != 0;
}

// fn_18_8A20 (0x8A20): the vulnerability state (guest 0x7C4) is 1.
bool CEmperorIngStage3::ShouldChangeVulnerabilityToVulnerable(CStateManager& mgr, const CTriggerData& data) const {
  return mVulnerabilityState == 1;
}

// fn_18_898C (0x898C): "ShouldChangeVulnerabilityToInvulnerable". The look check has to pass
// (rodata 0x2CC = 90.0f); in the vulnerable state (3) the trigger is true, in the invulnerable
// state (2) it needs the damage taken while vulnerable (guest 0x810) or the vulnerable damage
// threshold (guest 0x814).
bool CEmperorIngStage3::ShouldChangeVulnerabilityToInvulnerable(CStateManager& mgr,
                                                               const CTriggerData& data) const {
  if (!RandomCheck(mgr, 90.f)) { // rodata 0x2CC
    return false;
  }
  if (mVulnerabilityState == 3) {
    return true;
  }
  if (mVulnerabilityState != 2) {
    return false;
  }
  if (mX810_ < 0.f) { // rodata 0x258
    return true;
  }
  return mVulnerableDamageThreshold < 1.1920929e-07f; // rodata 0x310
}

// fn_18_8968 (0x8968): the action (guest 0x7E8) is 3.
bool CEmperorIngStage3::ShouldDarkBeamAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mAction == 3;
}

// fn_18_8954 (0x8954): the action (guest 0x7E8) is 4.
bool CEmperorIngStage3::ShouldLightBeamAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mAction == 4;
}

// @ fn_18_87F0
// fn_18_87F0 (0x164): the light swarm when the player is low on beam ammo and the light swarm is
// possible (fn_18_4468), else when ChoseAction picked it (the action at guest 0x7E8 is 2).
bool CEmperorIngStage3::ShouldLightSwarmAttack(CStateManager& mgr, const CTriggerData& data) const {
  // The player's light ammo (item 0x2E) or dark ammo (item 0x2D) under a tenth (rodata 0x28C)
  // of its capacity, the amount read with the query flag set (`li r5, 0x1`).
  const CPlayerState& state = *mgr.GetPlayerState(0);
  const float light =
      0.1f * static_cast< float >(state.GetItemCapacity(CPlayerState::kIT_LightAmmo));
  bool low = static_cast< float >(state.GetItemAmount(CPlayerState::kIT_LightAmmo, true)) < light;
  if (!low) {
    const float dark =
        0.1f * static_cast< float >(state.GetItemCapacity(CPlayerState::kIT_DarkAmmo));
    low = static_cast< float >(state.GetItemAmount(CPlayerState::kIT_DarkAmmo, true)) < dark;
  }
  if (low && CanLightSwarm(mgr)) {
    return true;
  }
  return mAction == 2;
}

// @ fn_18_868C
// fn_18_868C (0x164): the dark fliers when the player is low on beam ammo and the fliers are
// possible (fn_18_436C), else when ChoseAction picked them (the action is 6).
bool CEmperorIngStage3::ShouldGenerateDarkFliers(CStateManager& mgr, const CTriggerData& data) const {
  // The player's light ammo (item 0x2E) or dark ammo (item 0x2D) under a tenth (rodata 0x28C)
  // of its capacity, the amount read with the query flag set (`li r5, 0x1`).
  const CPlayerState& state = *mgr.GetPlayerState(0);
  const float light =
      0.1f * static_cast< float >(state.GetItemCapacity(CPlayerState::kIT_LightAmmo));
  bool low = static_cast< float >(state.GetItemAmount(CPlayerState::kIT_LightAmmo, true)) < light;
  if (!low) {
    const float dark =
        0.1f * static_cast< float >(state.GetItemCapacity(CPlayerState::kIT_DarkAmmo));
    low = static_cast< float >(state.GetItemAmount(CPlayerState::kIT_DarkAmmo, true)) < dark;
  }
  if (low && CanDarkFlier(mgr)) {
    return true;
  }
  return mAction == 6;
}

// fn_18_8678 (0x8678): the action (guest 0x7E8) is 5.
bool CEmperorIngStage3::ShouldPortalAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mAction == 5;
}

// fn_18_8664 (0x8664): the action (guest 0x7E8) is 1.
bool CEmperorIngStage3::ShouldJumpSlideAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mAction == 1;
}

// fn_18_8648 (0x8648): the vulnerable damage threshold (guest 0x814) against rodata
// 0x310 (1.1920929e-07f).
bool CEmperorIngStage3::ShouldGrowHeart(CStateManager& mgr, const CTriggerData& data) const {
  return mVulnerableDamageThreshold < 1.1920929e-07f;
}

// @ fn_18_8600
// fn_18_8600 (0x48): "JumpSlideAttackOver": the slide's done flag (0x40 of guest 0x825, 0x8618:
// extrwi 1,25) and the base's AnimOver.
bool CEmperorIngStage3::JumpSlideAttackOver(CStateManager& mgr, const CTriggerData& data) const {
  return (x825_ & 0x40) != 0 && AnimOver(mgr, data);
}

// fn_18_85A4 (0x85A4): "ShouldEyeShut". The look check has to pass (rodata 0x2CC = 90.0f) and
// the eye-shut timer (guest 0x7CC) has to be above 8.0 (rodata 0x2A8): the retail takes the
// compare's GT bit (the mfcr and the extrwi at 0x85E0), so the timer has to have grown past
// eight seconds.
bool CEmperorIngStage3::ShouldEyeShut(CStateManager& mgr, const CTriggerData& data) const {
  if (!RandomCheck(mgr, 90.f)) { // rodata 0x2CC
    return false;
  }
  return mX7cc_ > 8.f; // rodata 0x2A8
}

// fn_18_858C (0x858C): the flag byte at guest 0x825, the bit 29 (the mask 0x04).
bool CEmperorIngStage3::AbortedJumpSlide(CStateManager& mgr, const CTriggerData& data) const {
  return (x825_ & 0x04) != 0;
}

// fn_18_8518 (0x8518): "ShouldJumpAttack". The look check has to pass (rodata 0x30C = 35.0f),
// the jump-attack timer (guest 0x7E0) has to be at least 2.5 (rodata 0x298) and the flag byte at
// guest 0x824 has to have its bit 29 (the mask 0x04) set.
bool CEmperorIngStage3::ShouldJumpAttack(CStateManager& mgr, const CTriggerData& data) const {
  if (!RandomCheck(mgr, 35.f)) { // rodata 0x30C
    return false;
  }
  if (mX7e0_ < 2.5f) { // rodata 0x298
    return false;
  }
  return (x824_ & 0x04) != 0;
}

// fn_18_8500 (0x8500): the flag byte at guest 0x825, the bit 0 (the mask 0x01).
bool CEmperorIngStage3::ShouldStampedeReact(CStateManager& mgr, const CTriggerData& data) const {
  return (x825_ & 0x01) != 0;
}

// fn_18_7ADC (0x7ADC): the "Start" state. On the deactivation message it hands over to
// fn_18_1DF4 with the enraged heart (0) and the flag set (0x7AF8..0x7B00).
void CEmperorIngStage3::Start(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Deactivate) {
    return;
  }
  SetHeartState(mgr, 0, true); // fn_18_1DF4
}

// fn_18_788C (0x250): the "Idle" state.
//  - Activate (0x78D0): the timer at guest 0x864 is set to the biggest float (rodata 0x2C8)
//    and the body's steering is set up: the walk speed over the run speed (the two
//    GetLocomotionSpeed calls, pas::kLA_Walk over pas::kLA_Run, 0x78E4..0x78FC), the steering
//    blend mode (the write of one at 0x7908) and the steering speed range (0x7918).
//  - Update (0x7920): the vector to the player; fn_18_1A54 (0xE0, a stand-in) looks along the
//    actor's facing for something to hit and fn_18_3804 (0x25C, a stand-in) turns the actor
//    towards the player; over fifty units (rodata 0x290) the timer is cleared (rodata 0x258);
//    the locomotion command carries the normalised direction to the player when the look hit
//    and the timer is below half a second (rodata 0x2A0), the zero vector otherwise; then the
//    timer grows by the frame time (0x7AB0).
void CEmperorIngStage3::Idle(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    mX864_ = FLT_MAX; // rodata 0x2C8
    const float run = BodyController()->GetBodyStateInfo().GetLocomotionSpeed(pas::kLA_Run);
    const float walk = BodyController()->GetBodyStateInfo().GetLocomotionSpeed(pas::kLA_Walk);
    const float ratio = walk / run; // 0x78FC
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed); // 0x7908
    BodyController()->CommandMgr().SetSteeringSpeedRange(ratio, ratio);  // 0x7918
    break;
  }
  case kStateMsg_Update: {
    const CVector3f& theirs = mgr.GetPlayer(0)->GetTranslation();
    const CVector3f& mine = GetTranslation();
    CVector3f toPlayer(theirs.GetX() - mine.GetX(), theirs.GetY() - mine.GetY(),
                       theirs.GetZ() - mine.GetZ());
    if (!toPlayer.CanBeNormalized()) {
      return; // 0x796C: the retail leaves the state without the timer
    }
    const bool hit = LineOfSight(mgr, GetTransform().GetColumn(kDY), 15.f); // fn_18_1A54
    FaceTarget(theirs);                                                   // fn_18_3804
    const float distance = toPlayer.Magnitude();
    const float scale = 1.f / distance;
    const CVector3f direction(toPlayer.GetX() * scale, toPlayer.GetY() * scale,
                              toPlayer.GetZ() * scale);
    if (distance > 50.f) { // rodata 0x290
      mX864_ = 0.f;        // rodata 0x258
    }
    if (hit && mX864_ < 0.5f) { // rodata 0x2A0
      BodyController()->CommandMgr().DeliverCmd(CBCLocomotionCmd(direction, mX818_, 1.f));
    } else {
      BodyController()->CommandMgr().DeliverCmd(CBCLocomotionCmd(CVector3f::Zero(), mX818_, 1.f));
    }
    mX864_ += dt; // 0x7AB0
    break;
  }
  default:
    break;
  }
}

// fn_18_76D0 (0x1B8): the "MeleeAttack" state. Its activate path (0x7710) sets the animation
// state to `kAS_Ready`. The update path (0x771C) issues a severity-zero melee attack while the
// body can take one, else aims the body at the player during the state's first 0.9 seconds
// (rodata 0x300, the state machine's time). The deactivate path (0x7830) clears the animation
// state, adds the entry 0 to the list at guest 0x840 (fn_18_2B68) unless the 0x40 flag at guest
// 0x824 is set, ages the list at guest 0x830 (fn_18_2BF0 kind 3), clears the time since the
// last attack (guest 0x7D0) and the 0x40 flag.
// @ fn_18_76D0
void CEmperorIngStage3::MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x7714, the value one
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_MeleeAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCMeleeAttackCmd(pas::kS_Zero));
    } else if (mStateMachine->GetTime() < 0.9f) { // rodata 0x300
      BodyController()->CommandMgr().SetTargetVector(PlayerDirection(mgr)); // fn_18_2D1C
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x7834, the value zero
    if ((x824_ & 0x40) == 0) {
      AddTauntEntry(0); // fn_18_2B68
    }
    AddListEntry(3); // fn_18_2BF0
    mX7d0_ = 0.f;    // rodata 0x258
    x824_ &= ~0x40;
    break;
  default:
    break;
  }
}

// fn_18_7504 (0x1CC): the "GainDistance" state. Its activate path (0x7548) clears the module's
// too-close distance (guest 0x7D4), sets the animation state at guest 0x6B4 to `kAS_Ready` and
// looks thirty units along the actor's facing (fn_18_1A54): a blocked look adds the list entry
// 1 (fn_18_2BF0) and sets the animation state to 3, a clear one adds the entry 0. The update
// path (0x75B4) steps backwards while the body can take a step (CanIssueCommand, the DOL's
// fn_801B73E4) and aims the body at the player (the command manager's target vector). The
// deactivate path (0x76B4) clears the animation state.
// @ fn_18_7504
void CEmperorIngStage3::GainDistance(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mX7d4_ = 0.f; // rodata 0x258
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x7564, the value one
    if (!LineOfSight(mgr, GetTransform().GetColumn(kDY), 30.f)) { // rodata 0x2FC
      AddListEntry(1);                                              // fn_18_2BF0
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x759C, the value three
    } else {
      AddListEntry(0); // fn_18_2BF0
    }
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {
      BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(pas::kSD_Backward, pas::kStep_Normal));
      BodyController()->CommandMgr().SetTargetVector(PlayerDirection(mgr)); // fn_18_2D1C
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x76B0, the value zero
    break;
  default:
    break;
  }
}

// @ fn_18_71D8
// fn_18_71D8 (0x32C): the "Stampede" state. Activate (0x7218) sets the animation state to
// `kAS_Ready`, sets the stampede flag (0x20 at guest 0x824), clears the stampede-react flag
// (0x01 at guest 0x825) and halves the turn speed (rodata 0x2A0 times guest 0xE5C). Update
// (0x7258) issues the loop attack while the body can take one (type 6 when 0x04 at guest 0x825
// is set, else 4); otherwise, while 0x20 at guest 0x825 is set and the player is over 15 units
// away (fn_18_2CB8, rodata 0x2B4), it steers at the player (the command manager's target vector,
// fn_18_2D1C). It then looks 15 units along the flat facing (fn_18_2C98) from the actor's
// position with the pass-everything filter, and exits the state when the state time is over 5
// (rodata 0x2BC) or the ray hit. Deactivate (0x74A4) restores the turn speed, clears the three
// flags and the attack-cooldown timer (guest 0x7D0).
void CEmperorIngStage3::Stampede(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x7220, the value one
    x824_ |= 0x20;                                        // 0x7230 (rlwimi 5,26,26)
    x825_ &= ~0x01;                                       // 0x723C (rlwimi 0,31,31)
    BodyController()->SetTurnSpeed(0.5f * mBaseTurnSpeed); // rodata 0x2A0
    break;
  case kStateMsg_Update: {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      // 0x7274: extrwi 1,29 is the byte's 0x04.
      const pas::ELoopAttackType type =
          static_cast< pas::ELoopAttackType >((x825_ & 0x04) != 0 ? 6 : 4);
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(type, false));
      break;
    }
    // 0x7398: extrwi 1,26 is the byte's 0x20.
    if ((x825_ & 0x20) != 0 && PlayerDistance(mgr) > 15.f) { // fn_18_2CB8, rodata 0x2B4
      BodyController()->CommandMgr().SetTargetVector(PlayerDirection(mgr)); // fn_18_2D1C
    }
    const CRayCastResult result = mgr.RayStaticIntersection(
        GetTranslation(), FlatFacing(), 15.f, CMaterialFilter()); // fn_18_2C98, 0x7430
    const CRayCastResult copy = CopyRayResult(result);             // fn_18_3AC
    if (mStateMachine->GetTime() > 5.f || copy.IsValid()) {        // rodata 0x2BC, sp+0xC0
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState)); // 0x7490
    }
    break;
  }
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x74A8, the value zero
    BodyController()->SetTurnSpeed(mBaseTurnSpeed);           // guest 0xE5C
    x824_ &= ~0x20;                                           // 0x74C0 (rlwimi 5,26,26)
    x825_ &= ~0x04;                                           // 0x74D4 (rlwimi 2,29,29)
    x825_ &= ~0x20;                                           // 0x74E0 (rlwimi 5,26,26)
    mX7d0_ = 0.f;                                             // rodata 0x258
    break;
  default:
    break;
  }
}

// fn_18_6E80 (0x358): the "Dodge" state. Its activate path (0x6EC0) clears the dodge flag
// (0x08 at guest 0x824), sets the animation state to `kAS_Ready` and the step direction (guest
// 0x8A4) to -1. When the player is within 60 degrees (fn_18_F8) it looks fifteen units to the
// actor's left and right (fn_18_1A54): a clear left picks `kSD_Left`, a clear right picks
// `kSD_Right` (over a picked left only on Float() > 0.5), and neither picks `kSD_Down`. No
// direction sets the animation state to 3. A roll of Range(0, 100) over 45 adds {100, 5} to the
// list at guest 0x850 (fn_18_AC00), and the hearts turn immune (fn_18_1C34). The update path
// (0x7040) steps in the picked direction while the body can take a step, else faces the player
// unless the step is down. The deactivate path (0x714C) clears the animation state, restores the
// hearts' vulnerability (fn_18_1BD8) and adds the entry 1 to the list at guest 0x840
// (fn_18_2B68).
// @ fn_18_6E80
void CEmperorIngStage3::Dodge(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    x824_ &= ~0x08;
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6EE0, the value one
    x8a4_ = -1;
    if (RandomCheck(mgr, 60.f)) { // rodata 0x2E4
      const CVector3f right = GetTransform().GetColumn(kDX);
      if (LineOfSight(mgr, -right, 15.f)) { // rodata 0x2B4
        x8a4_ = pas::kSD_Left;
      }
      if (LineOfSight(mgr, right, 15.f)) {
        if (x8a4_ == -1) {
          x8a4_ = pas::kSD_Right;
        } else if (mgr.Random()->Float() > 0.5f) { // rodata 0x2A0
          x8a4_ = pas::kSD_Right;
        }
      }
      if (x8a4_ == -1) {
        x8a4_ = pas::kSD_Down;
      }
    }
    if (x8a4_ == -1) {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6FE4, the value three
    }
    if (mgr.Random()->Range(0.f, 100.f) > 45.f) { // rodata 0x258, 0x27C, 0x2D0
      PushListEntry(mX850_, 100.f, 5.f);           // fn_18_AC00, rodata 0x27C, 0x2BC
    }
    SetHeartsImmune(mgr); // fn_18_1C34
    break;
  }
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCStepCmd(static_cast< pas::EStepDirection >(x8a4_), pas::kStep_Normal));
    } else if (x8a4_ != pas::kSD_Down) {
      BodyController()->CommandMgr().SetTargetVector(PlayerDirection(mgr)); // fn_18_2D1C
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x7150, the value zero
    SetHeartVulnerability(mgr);                              // fn_18_1BD8
    AddTauntEntry(1);                                        // fn_18_2B68
    break;
  default:
    break;
  }
}

// @ fn_18_6D84
// fn_18_6D84 (0xFC): the "Taunt" state. Activate (0x6DBC) sets the animation state to
// `kAS_Ready`, adds a taunt entry of kind 2 (fn_18_2B68) and sets the taunt type (guest 0x8A8)
// to 1. Update (0x6DD8) issues the taunt while the body can take one (state 0x10). Deactivate
// (0x6E58) clears the animation state and the taunt timer (guest 0x7C8).
void CEmperorIngStage3::Taunt(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6DC4, the value one
    AddTauntEntry(2);                                     // fn_18_2B68
    x8a8_ = 1;                                            // 0x6DD0
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCTauntCmd(static_cast< pas::ETauntType >(x8a8_))); // 0x6E28
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6E60, the value zero
    mX7c8_ = 0.f;                                            // rodata 0x258
    break;
  default:
    break;
  }
}

// @ fn_18_408
// fn_18_408 (0xC8): the heart ChangeVulnerability turns on. With a roll (Range(0, 1), rodata
// 0x258/0x264) under 0.3 (rodata 0x268) and the two heart counts (guest 0x89C, 0x8A0) one
// apart, the heart counted less is picked again (1 when 0x89C is the smaller, else 2);
// otherwise the other one (2 when 0x89C is the smaller, else 1). Heart 2 counts at 0x89C, heart
// 1 at 0x8A0.
int CEmperorIngStage3::PickHeart(CStateManager& mgr) {
  int heart;
  if (mgr.Random()->Range(0.f, 1.f) < 0.3f && abs(mX89c_ - mX8a0_) == 1) {
    heart = mX89c_ < mX8a0_ ? 1 : 2;
  } else {
    heart = mX89c_ < mX8a0_ ? 2 : 1;
  }
  if (heart == 2) {
    ++mX89c_;
  } else {
    ++mX8a0_;
  }
  return heart;
}

// @ fn_18_6A40
// fn_18_6A40 (0x140): the "ChangeVulnerability" state. Activate (0x6A80) sets the animation state
// to `kAS_Ready` and clears the action history (guest 0x7EC); from the vulnerability states 0
// and 1 it goes to 2 and turns a heart on (fn_18_408, fn_18_1DF4 with the flag set), from 2 and
// 3 back to 0 with the enraged heart (heart 0). Update (0x6B4C) sets the animation state to
// `kAS_Over` every frame. Deactivate (0x6B58) restarts the vulnerable-damage timer (guest 0x810)
// at the data's vulnerableTime (guest 0xB4C).
void CEmperorIngStage3::ChangeVulnerability(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6A88, the value one
    mActionHistory.clear();                               // 0x6AD8
    switch (mVulnerabilityState) {
    case 0:
    case 1:
      mVulnerabilityState = 2;                    // 0x6B08
      SetHeartState(mgr, PickHeart(mgr), true);   // fn_18_408, fn_18_1DF4
      break;
    case 2:
    case 3:
      mVulnerabilityState = 0;                    // 0x6B30
      SetHeartState(mgr, 0, true);                // fn_18_1DF4
      break;
    default:
      break;
    }
    break;
  case kStateMsg_Update:
    mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6B50, the value three
    break;
  case kStateMsg_Deactivate:
    mX810_ = mVulnerableTime;                                // 0x6B60
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B64, the value zero
    break;
  default:
    break;
  }
}

// @ fn_18_6B80
// fn_18_6B80 (0x204): the "EyeShutResetVulnerability" state. Activate (0x6BC0) sets the animation
// state to `kAS_Ready`, clears the action history (guest 0x7EC) and makes the hearts immune
// (fn_18_1C34). Update (0x6C2C) issues loop attack type 0 while the body can take one (state 9);
// otherwise, past 2 seconds in the state (rodata 0x274) it exits the body state and, from the
// vulnerability states 2 and 3, goes back to 0 with the enraged heart (fn_18_1DF4, heart 0, the
// flag set). Deactivate (0x6D60) switches the heart that is on (fn_18_1BB0) and clears the
// animation state.
void CEmperorIngStage3::EyeShutResetVulnerability(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6BC8, the value one
    mActionHistory.clear();                               // 0x6C1C
    SetHeartsImmune(mgr);                                 // fn_18_1C34
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCLoopAttackCmd(static_cast< pas::ELoopAttackType >(0), false)); // 0x6C8C
      break;
    }
    if (mStateMachine->GetTime() > 2.f) { // rodata 0x274
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState)); // 0x6D10
      if (mVulnerabilityState >= 2 && mVulnerabilityState < 4) {
        mVulnerabilityState = 0;      // 0x6D48
        SetHeartState(mgr, 0, true); // fn_18_1DF4
      }
    }
    break;
  case kStateMsg_Deactivate:
    SetCurrentHeart(mgr);                                    // fn_18_1BB0
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6D68, the value zero
    break;
  default:
    break;
  }
}

// @ fn_18_6904
// fn_18_6904 (0x13C): the "DarkBeamAttack" state. Activate (0x693C) sets the
// animation state to `kAS_Ready` and the dark-beam flag (0x01 of 0x824); Update (0x6954) issues the
// projectile attack (severity 0, a zero target, no blend) while the body can take one (state
// 0x12) - the beam starts on the animation's event (fn_18_1530); Deactivate (0x6A14) stops the
// beam (fn_18_12C4), clears the flag and resets the animation state.
void CEmperorIngStage3::DarkBeamAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // the value one
    x824_ |= 0x01; // 0x6948: the dark-beam flag
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCProjectileAttackCmd(pas::kS_Zero, CVector3f::Zero(), false)); // 0x69C4
    }
    break;
  case kStateMsg_Deactivate:
    StopBeam(mgr); // fn_18_12C4
    x824_ &= ~0x01; // 0x6A20
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // the value zero
    break;
  default:
    break;
  }
}

// @ fn_18_67C0
// fn_18_67C0 (0x144): the "LightBeamAttack" state, DarkBeamAttack with the
// light-beam flag (0x80 of 0x825, rlwimi 7,24) and severity 3 (0x6850).
void CEmperorIngStage3::LightBeamAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // the value one
    x825_ |= 0x80; // 0x6804: the light-beam flag, which StartBeam reads
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCProjectileAttackCmd(pas::kS_Three, CVector3f::Zero(), false)); // 0x6888
    }
    break;
  case kStateMsg_Deactivate:
    StopBeam(mgr); // fn_18_12C4
    x825_ &= ~0x80; // 0x68E4
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // the value zero
    break;
  default:
    break;
  }
}

// @ fn_18_6698
// fn_18_6698 (0x128): the "LightSwarmAttack" state, PortalAttack's shape with severity 11. The
// attack animation's kUE_BeginAction event spawns the light swarm (BeginActionEvent, fn_18_BB8).
// Activate (0x66D0) readies the animation state; Update (0x66DC) issues the projectile attack
// (severity 11, a zero target, no blend) while the body can take one (state 0x12); Deactivate
// (0x67A4) un-readies it.
void CEmperorIngStage3::LightSwarmAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x66D4, the value one
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCProjectileAttackCmd(pas::kS_Eleven, CVector3f::Zero(), false)); // 0x6750
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x67A8, the value zero
    break;
  default:
    break;
  }
}

// @ fn_18_6390
// fn_18_6390 (0x2AC): the "GroundPound" state. Activate (0x63D0) readies the animation state.
// Update (0x63DC) issues the melee attack (severity 2) while the body can take one (state 7);
// otherwise, once the pound's animation event has set 0x10 of 0x824 (extrwi 1,27) and the player
// has touch bounds (vtable slot 0x48), a box ten units (rodata 0x294) around the actor's
// translation that overlaps them deals the ground-pound damage (guest 0xA44, a solid-only
// filter) and clears the flag. Deactivate (0x6610) clears the flag and un-readies the state.
void CEmperorIngStage3::GroundPound(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x63D4, the value one
    break;
  case kStateMsg_Update: {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_MeleeAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCMeleeAttackCmd(pas::kS_Two)); // 0x6454
      break;
    }
    CPlayer& player = *mgr.Player(0);
    const rstl::optional_object< CAABox > touch = player.GetTouchBounds(); // vtable slot 0x48
    if ((x824_ & 0x10) == 0 || !touch) {                                   // 0x64FC, 0x6508
      break;
    }
    const CVector3f& pos = GetTranslation();
    const CAABox box(CVector3f(pos.GetX() - 10.f, pos.GetY() - 10.f, pos.GetZ() - 10.f),
                     CVector3f(pos.GetX() + 10.f, pos.GetY() + 10.f, pos.GetZ() + 10.f));
    if (box.DoBoundsOverlap(*touch)) { // 0x6570
      mgr.ApplyDamage(GetUniqueId(), player.GetUniqueId(), GetUniqueId(), mGroundPoundDamage,
                      CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid),
                                                          CMaterialList()),
                      CVector3f::Zero()); // fn_8003E25C
      x824_ &= ~0x10;                     // 0x6604
    }
    break;
  }
  case kStateMsg_Deactivate:
    x824_ &= ~0x10;                                           // 0x6618
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6620, the value zero
    break;
  default:
    break;
  }
}

// @ fn_18_6258
// fn_18_6258 (0x138): the "GenerateDarkFliers" state. Activate (0x6290) readies the animation
// state; Update (0x629C) issues the projectile attack (severity 1, a zero target, no blend) while
// the body can take one (state 0x12); Deactivate (0x6364) clears the dark-flier timer (guest
// 0x7D8) and the light-swarm time (0x7DC, both rodata 0x258) and un-readies the animation state.
void CEmperorIngStage3::GenerateDarkFliers(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6294, the value one
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCProjectileAttackCmd(pas::kS_One, CVector3f::Zero(), false)); // 0x6314
    }
    break;
  case kStateMsg_Deactivate:
    mX7d8_ = 0.f;                                             // 0x6370
    mX7dc_ = 0.f;                                             // 0x6374
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6378, the value zero
    break;
  default:
    break;
  }
}

// @ fn_18_60AC
// fn_18_60AC (0x14C): the "PortalAttack" state. Activate (0x60E4) clears the portal count (guest
// 0x8E0) and sets the animation state to `kAS_Ready`. Update (0x60F8) issues the projectile
// attack (severity 2, a zero target, no blend) while the body can take one (state 0x12); the
// shot itself is the animation event's. Deactivate (0x61C0) moves on to the next of the four
// portals (guest 0x8E4, the signed remainder of the increment by four).
void CEmperorIngStage3::PortalAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mX8e0_ = 0;                                           // 0x60EC
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x60F0, the value one
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCProjectileAttackCmd(pas::kS_Two, CVector3f::Zero(), false)); // 0x6170
    }
    break;
  case kStateMsg_Deactivate:
    mX8e4_ = (mX8e4_ + 1) % 4;                               // 0x61C0..0x61E0
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x61E4, the value zero
    break;
  default:
    break;
  }
}

// @ fn_18_5844
// fn_18_5844 (0x630): the "JumpSlideAttack" state. Activate (0x5898) sets the animation state to
// `kAS_Ready`, clears 0x10/0x40/0x01/0x04/0x08 of guest 0x825 and 0x80/0x40 of 0x826, and the
// slide's end time (guest 0x828) to FLT_MAX (rodata 0x2C8). Update (0x5908): a set 0x10 of 0x825
// moves to 0x20 of 0x824; a set 0x08 of 0x825 (the room check) measures the room ahead
// (fn_18_25C) - none, or under 40 (rodata 0x2E0), aborts the slide (0x04 of 0x825) - and the
// extra slide time (guest 0x8E8) is the room past 20 over 90 (rodata 0x270, 0x2CC). While the
// body can take one (state 9) it issues loop attack type 5. Otherwise the slide runs the loop
// attack animation's duration (state 9, parms enum 0 and 5) plus the extra time, plus 6.5 with
// 0x01 of 0x825 or 2 without (rodata 0x2F4, 0x274); the first frame 0x80 of 0x826 (a heart hit)
// is seen with 0x01 set the end time becomes the state time plus 4 (rodata 0x2AC, 0x40 of 0x826
// marks it). An abort (body command 0xD), a knock-back (fn_18_E0: the body state 6), the time
// past that total (unless the end time is set) or past the end time sets the done flag (0x40 of
// 0x825, `fn_18_8600`); past the duration plus the extra time the body exits, the hearts swap
// (fn_18_214: 1 on, 0 off) and 0x80 of 0x826 clears; before that, past 9 s (rodata 0x2F8) the
// slide steers at the player (fn_18_2D1C) with 0x20 of 0x825 set and the player over 15 units
// away (fn_18_2CB8, rodata 0x2B4) - before 9 s with 0x20 alone - and the locomotion type is 5.
// Deactivate (0x5D98) swaps the hearts back (1 off, 0 on), switches the knock-back reactions on,
// clears 0x20 of 0x824, the animation state and the attack timer (guest 0x7D0), exits the body
// state, sets the locomotion type to 0 and clears 0x20/0x10 of 0x825.
void CEmperorIngStage3::JumpSlideAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x58A0, the value one
    x825_ &= ~0x10;
    x825_ &= ~0x40;
    x825_ &= ~0x01;
    x826_ &= ~0x80;
    x826_ &= ~0x40;
    mX828_ = FLT_MAX; // rodata 0x2C8
    x825_ &= ~0x04;
    x825_ &= ~0x08;
    break;
  case kStateMsg_Update: {
    if ((x825_ & 0x10) != 0) { // 0x590C: extrwi 1,27
      x825_ &= ~0x10;
      x824_ |= 0x20;
    }
    bool abort = false;
    if ((x825_ & 0x08) != 0) { // 0x5938: extrwi 1,28
      float room = 0.f;
      const bool hit = ForwardClearance(mgr, room); // fn_18_25C
      if (!hit || room < 40.f) {                    // rodata 0x2E0
        x825_ |= 0x04;
        abort = true;
      }
      if (hit) {
        float extra = room - 20.f; // rodata 0x270
        if (0.f >= extra) {
          extra = 0.f;
        }
        mX8e8_ = extra / 90.f; // rodata 0x2CC
      }
      x825_ &= ~0x08;
    }
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCLoopAttackCmd(static_cast< pas::ELoopAttackType >(5), false)); // 0x5A54
      break;
    }
    const float slide =
        GetAnimationDuration(CPASAnimParmData(pas::kAS_LoopAttack, CPASAnimParm::FromEnum(0),
                                              CPASAnimParm::FromEnum(5))) +
        mX8e8_; // fn_18_5EB0, 0x5B18
    const float tail = (x825_ & 0x01) != 0 ? 6.5f : 2.f; // rodata 0x2F4, 0x274
    if ((x826_ & 0x40) == 0 && (x826_ & 0x80) != 0 && (x825_ & 0x01) != 0) {
      mX828_ = 4.f + mStateMachine->GetTime(); // rodata 0x2AC
      x826_ |= 0x40;
    }
    if (abort || BodyController()->GetCurrentStateId() == pas::kAS_KnockBack || // fn_18_E0
        ((x826_ & 0x40) == 0 && mStateMachine->GetTime() > slide + tail) ||
        mStateMachine->GetTime() > mX828_) {
      if (abort) {
        BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_AbortScripted)); // 0xD
      }
      x825_ |= 0x40; // 0x5C38
      break;
    }
    if (mStateMachine->GetTime() > slide) {
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState)); // 0x5C80
      SetHeartActive(mgr, 1, true);                                             // fn_18_214
      SetHeartActive(mgr, 0, false);                                            // fn_18_214
      x826_ &= ~0x80;
      break;
    }
    if (mStateMachine->GetTime() > 9.f) { // rodata 0x2F8
      if ((x825_ & 0x20) != 0 && PlayerDistance(mgr) > 15.f) { // fn_18_2CB8, rodata 0x2B4
        BodyController()->CommandMgr().SetTargetVector(PlayerDirection(mgr)); // fn_18_2D1C
      }
    } else if ((x825_ & 0x20) != 0) {
      BodyController()->CommandMgr().SetTargetVector(PlayerDirection(mgr)); // fn_18_2D1C
    }
    BodyController()->SetLocomotionType(static_cast< pas::ELocomotionType >(5)); // 0x5D90
    break;
  }
  case kStateMsg_Deactivate:
    SetHeartActive(mgr, 1, false); // fn_18_214
    SetHeartActive(mgr, 0, true);  // fn_18_214
    mKnockBackController.EnableAllAnimReactions(true);
    x824_ &= ~0x20;
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x5DE4, the value zero
    mX7d0_ = 0.f;                                            // 0x5DF4
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState)); // 0x5E08
    BodyController()->SetLocomotionType(static_cast< pas::ELocomotionType >(0));
    x825_ &= ~0x20;
    x825_ &= ~0x10;
    break;
  default:
    break;
  }
}

// @ fn_18_A00
// fn_18_A00 (0x94): the share of the ing's health lost: one less the health over the initial
// health (the vtable slot 0x3C), clamped to 0..1 (rodata 0x258, 0x264).
static float HealthLost(const CHealthInfo& health) {
  float ratio = health.GetHP() / health.GetInitialHP();
  if (ratio >= 1.f) {
    ratio = 1.f;
  }
  if (0.f > ratio) {
    ratio = 0.f;
  }
  return 1.f - ratio;
}

// @ fn_18_5614
// fn_18_5614 (0x230): the "GrowHeart" state; the health lost (fn_18_A00) is the heart effect's
// target growth. Activate (0x5678) sets the animation state to `kAS_Ready`, switches off every
// knock-back reaction (guest 0x620), makes the hearts immune (fn_18_1C34) and clears guest
// 0x825's 0x02. Update (0x56AC) issues a knock-back (body command 4: no direction, kS_Zero, no
// animation, no restart) after the locomotion type goes to 0 while the body can take one
// (state 6); otherwise, with 0x02 of guest 0x825 set, the vulnerability goes back to 0 with the
// enraged heart (fn_18_1DF4) and the growth (guest 0x82C) rises by the frame time up to the
// target. Deactivate (0x57DC) sets the growth to the target, switches the knock-back reactions
// back on, restores the vulnerable damage threshold (guest 0x814 from 0xB50), goes back to the
// vulnerability state 0 with the enraged heart and clears the animation state.
void CEmperorIngStage3::GrowHeart(CStateManager& mgr, EStateMsg msg, float dt) {
  const float target = HealthLost(*HealthInfo()); // fn_18_A00
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x5680, the value one
    mKnockBackController.EnableAllAnimReactions(false);
    SetHeartsImmune(mgr); // fn_18_1C34
    x825_ &= ~0x02;       // 0x56A0 (rlwimi 1,30,30)
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_KnockBack)) {
      BodyController()->SetLocomotionType(static_cast< pas::ELocomotionType >(0));
      BodyController()->CommandMgr().DeliverCmd(
          CBCKnockBackCmd(CVector3f::Zero(), pas::kS_Zero, -1, false)); // 0x5730
      break;
    }
    if ((x825_ & 0x02) != 0) {
      mVulnerabilityState = 0;      // 0x579C
      SetHeartState(mgr, 0, true); // fn_18_1DF4
    }
    if ((x825_ & 0x02) != 0) {
      if (mX82c_ < target) {
        mX82c_ += dt; // 0x57C8
      } else {
        mX82c_ = target;
      }
    }
    break;
  case kStateMsg_Deactivate:
    mX82c_ = target; // 0x57DC
    mKnockBackController.EnableAllAnimReactions(true);
    mVulnerableDamageThreshold = mVulnerableDamageThresholdReset; // 0x57EC
    mVulnerabilityState = 0;                                      // 0x57FC
    SetHeartState(mgr, 0, true);                                  // fn_18_1DF4
    mAnimationState.SetState(CAnimationState::kAS_NotReady);      // 0x5814, the value zero
    break;
  default:
    break;
  }
}

// fn_18_4EB4 (0x23C): the "JumpAttack" state. Its activate path (0x4F0C) sets the flag at guest
// 0x6B4 and picks the jump target: the player's position, or - when the player is over fifty
// units away (rodata 0x290) - a point thirty-five percent (rodata 0x2F0) of the way along the
// direction to the player, starting at the actor's own position. The update path (0x500C) jumps
// at the player while the body can take a jump (fn_18_51E8 builds the CBCJumpCmd: the player's
// position, kJT_Normal, kJS_IntoJump, variant 0, facing flags 2; fn_18_50F0 delivers it) and
// otherwise aims the body at the player. The deactivate path (0x50A0) clears the flag, the
// jump-attack timer (guest 0x7E0) and the flag byte 0x824's bit 29 (the mask 0x04).
void CEmperorIngStage3::JumpAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x4F14, the value one
    const CVector3f& theirs = mgr.GetPlayer(0)->GetTranslation();
    const CVector3f toPlayer = PlayerDirection(mgr); // fn_18_2D1C
    const float distance = toPlayer.Magnitude();
    if (distance <= 0.f) { // rodata 0x258
      return;
    }
    if (distance > 50.f) { // rodata 0x290
      // The direction is normalised first; the target is 35 percent (rodata 0x2F0) of the
      // distance along it.
      const CVector3f dir = toPlayer * (1.f / distance);
      const float along = 0.35f * distance;
      const CVector3f& mine = GetTranslation();
      mX910_ = CVector3f(mine.GetX() + dir.GetX() * along, mine.GetY() + dir.GetY() * along,
                         mine.GetZ() + dir.GetZ() * along);
    } else {
      mX910_ = theirs;
    }
    break;
  }
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Jump)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCJumpCmd(mgr.GetPlayer(0)->GetTranslation(), pas::kJT_Normal, pas::kJS_IntoJump, 0,
                     CBCJumpCmd::kFF_AmbushJump)); // fn_18_51E8, fn_18_50F0
    } else {
      BodyController()->CommandMgr().SetTargetVector(PlayerDirection(mgr)); // fn_18_2D1C
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x50A8, the value zero
    mX7e0_ = 0.f;
    x824_ &= ~0x04; // the bit 29 (the mask 0x04) is cleared
    break;
  default:
    break;
  }
}

// @ fn_18_4CC8
// fn_18_4CC8 (0x190): the "StampedeReact" state. Activate (0x4D08) readies the animation state.
// Update (0x4D14): for the state's first 1.2 seconds (the state machine's time, rodata 0x2EC) the
// actor turns towards the player (fn_18_3804) and stands, facing guest 0x818 at weight one
// (rodata 0x264); after that it taunts (type 3) while the body can take one (state 0x10).
// Deactivate (0x4E38) un-readies the animation state.
void CEmperorIngStage3::StampedeReact(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x4D0C, the value one
    break;
  case kStateMsg_Update:
    if (mStateMachine->GetTime() < 1.2f) { // rodata 0x2EC
      const CVector3f theirs = mgr.GetPlayer(0)->GetTranslation();
      FaceTarget(theirs); // fn_18_3804
      BodyController()->CommandMgr().DeliverCmd(
          CBCLocomotionCmd(CVector3f::Zero(), mX818_, 1.f)); // 0x4DB0
    } else if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
      BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(pas::kTT_Three)); // 0x4E08
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x4E3C, the value zero
    break;
  default:
    break;
  }
}

// @ fn_18_4C3C
// fn_18_4C3C (0x8C): the "Dead" state. Activate sends DEAD with no message; every message then
// goes to the base's Dead.
void CEmperorIngStage3::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    SendScriptMsgs(kSS_Dead, mgr, kInvalidUniqueId, kSM_None); // 0x4C90
  }
  CPatterned::Dead(mgr, msg, dt);
}

// @ fn_18_B04
// fn_18_B04 (0xB4): the stage record for the actor's health: the percentage (100, rodata 0x27C,
// times the health info's hp over its initial hp, the vtable slot 0x3C) is checked against each
// stage's minHealthPercentage in turn; past the third the fourth stage applies.
const SLdrEmperorIngStage3Stage& CEmperorIngStage3::CurrentStage() {
  const float initial = HealthInfo()->GetInitialHP(); // 0xB30
  const float percent = 100.f * HealthInfo()->GetHP() / initial; // 0xB4C..0xB5C
  for (int i = 0; i < 3; ++i) {
    if (percent > mStages[i].minHealthPercentage) {
      return mStages[i];
    }
  }
  return mStages[3];
}

// @ fn_18_AA4
// fn_18_AA4 (0x60): the stage's action record: 0 stampede (+0xC), 1 jump slide, 2 light swarm,
// 3 dark beam, 4 light beam, 5 portal attack, 6 dark flier (+0x54); null past six.
const SLdrEmperorIngStage3Action* CEmperorIngStage3::StageAction(
    const SLdrEmperorIngStage3Stage& stage, int action) {
  switch (action) {
  case 0: return &stage.stampede;
  case 1: return &stage.jumpSlide;
  case 2: return &stage.lightSwarm;
  case 3: return &stage.darkBeam;
  case 4: return &stage.lightBeam;
  case 5: return &stage.portalAttack;
  case 6: return &stage.darkFlier;
  default: return nullptr;
  }
}

// @ fn_18_458C
// fn_18_458C (0x28): the stampede's look check at 60 degrees (rodata 0x2E4).
bool CEmperorIngStage3::CanStampede(CStateManager& mgr) const { return RandomCheck(mgr, 60.f); }

// @ fn_18_431C
// fn_18_431C (0x28): the jump slide's look check at 90 degrees (rodata 0x2CC).
bool CEmperorIngStage3::CanJumpSlide(CStateManager& mgr) const { return RandomCheck(mgr, 90.f); }

// @ fn_18_4344
// fn_18_4344 (0x28): the portal attack's look check at 45 degrees (rodata 0x2D0).
bool CEmperorIngStage3::CanPortalAttack(CStateManager& mgr) const {
  return RandomCheck(mgr, 45.f);
}

// @ fn_18_4468
// fn_18_4468 (0x5C): the light swarm needs heart 2 (guest 0x868), no watched object (guest
// 0x8C0) and the look check at 90 degrees (rodata 0x2CC).
bool CEmperorIngStage3::CanLightSwarm(CStateManager& mgr) const {
  return mX868_ == 2 && mX8c0_ == kInvalidUniqueId.value && RandomCheck(mgr, 90.f);
}

// @ fn_18_44C4
// fn_18_44C4 (0x64): the light beam needs no watched object, heart 2 and the look check at 40
// degrees (rodata 0x2E0).
bool CEmperorIngStage3::CanLightBeam(CStateManager& mgr) const {
  if (mX8c0_ != kInvalidUniqueId.value) {
    return false;
  }
  return mX868_ == 2 && RandomCheck(mgr, 40.f);
}

// @ fn_18_4528
// fn_18_4528 (0x64): the dark beam needs no watched object, heart 1 and the look check at 40
// degrees (rodata 0x2E0).
bool CEmperorIngStage3::CanDarkBeam(CStateManager& mgr) const {
  if (mX8c0_ != kInvalidUniqueId.value) {
    return false;
  }
  return mX868_ == 1 && RandomCheck(mgr, 40.f);
}

// @ fn_18_436C
// fn_18_436C (0xFC): the dark fliers need heart 1 and the look check at 90 degrees (rodata
// 0x2CC); a swarm still alive (the id at guest 0x8C2, more than a tenth - rodata 0x28C - of its
// boids active, CSwarmBasics::EvaluateActiveBoidCount against the boid vector's size) blocks
// them, and the dark-flier timer (guest 0x7D8) has to be at least 10 (rodata 0x294).
bool CEmperorIngStage3::CanDarkFlier(CStateManager& mgr) const {
  if (mX868_ != 1) {
    return false;
  }
  if (!RandomCheck(mgr, 90.f)) {
    return false;
  }
  if (const CSwarmBasics* swarm = TCastToPtr< CSwarmBasics >(mgr.ObjectById(TUniqueId(x8c2_)))) {
    // fn_80_1280 is CSwarmBasics::EvaluateActiveBoidCount (SwarmBasics symbols); +0x17C is the
    // boid vector's size.
    if (static_cast< float >(swarm->EvaluateActiveBoidCount()) >
        0.1f * static_cast< float >(swarm->GetBoidCount())) {
      return false;
    }
  }
  return mX7d8_ >= 10.f;
}

// @ fn_18_84A8
// fn_18_84A8 (0x58): the attack timer (guest 0x7E4) restarts at a random time between the
// current stage's min and max time between attacks, and the action (guest 0x7E8) is cleared.
void CEmperorIngStage3::ResetAttackTimer(CStateManager& mgr, float dt) {
  const SLdrEmperorIngStage3Stage& stage = CurrentStage(); // fn_18_B04
  mAttackTimer = mgr.Random()->Range(stage.minTimeBetweenAttacks, stage.maxTimeBetweenAttacks);
  mAction = -1;
}

// @ fn_18_8200
// fn_18_8200 (0x1F8): the action pick. Each of the seven actions (fn_18_AA4) scores its chance,
// less its modifier once for every time it is in the action history (guest 0x7EC/0x7F0); a
// disabled action, or one whose check fails (the jump table at data 0x654), scores -FLT_MAX
// (rodata 0x308). The best score above -FLT_MAX wins (the first on a tie; -1 when none), and is
// pushed onto the eight-entry history, dropping the oldest when it is full (fn_18_83F8).
void CEmperorIngStage3::ChoseAction(CStateManager& mgr, float dt) {
  const SLdrEmperorIngStage3Stage& stage = CurrentStage(); // fn_18_B04
  float scores[7];
  for (int i = 0; i < 7; ++i) {
    float score = -FLT_MAX; // rodata 0x308
    const SLdrEmperorIngStage3Action* action = StageAction(stage, i);
    if (action->enabled) {
      score = action->chance;
      for (int j = 0; j < mActionHistory.size(); ++j) {
        if (mActionHistory[j] == i) {
          score -= action->modifier;
        }
      }
    }
    bool possible;
    switch (i) {
    case 0: possible = CanStampede(mgr); break;     // fn_18_458C
    case 1: possible = CanJumpSlide(mgr); break;    // fn_18_431C
    case 2: possible = CanLightSwarm(mgr); break;   // fn_18_4468
    case 3: possible = CanDarkBeam(mgr); break;     // fn_18_4528
    case 4: possible = CanLightBeam(mgr); break;    // fn_18_44C4
    case 5: possible = CanPortalAttack(mgr); break; // fn_18_4344
    case 6: possible = CanDarkFlier(mgr); break;    // fn_18_436C
    default: possible = true; break;
    }
    if (!possible) {
      score = -FLT_MAX;
    }
    scores[i] = score;
  }
  int best = -1;
  float bestScore = -FLT_MAX;
  for (int i = 0; i < 7; ++i) {
    if (scores[i] > bestScore) {
      best = i;
      bestScore = scores[i];
    }
  }
  mAction = best;
  if (mActionHistory.size() == 8) {
    mActionHistory.erase(mActionHistory.begin()); // fn_18_83F8
  }
  mActionHistory.push_back(mAction);
}

// ---------------------------------------------------------------------------------------------
// The module's loader (fn_18_C380) and entry points.

static CEntity* PortLoadEmperorIngStage3(CStateManager& mgr, CInputStream& input,
                                          CEntityInfo& info) {
  SLdrEmperorIngStage3 sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrEmperorIngStage3.inc"

  // The REL builds the model with LdrToModelData(scale, the asset id, the patterned info's
  // animation set, true) at 0xC510 and moves the result into the object at 0xC51C.
  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL reads the copy's valid byte (sp+0x148 at 0xC538) and branches to its null
    // return when the model is empty.
    return nullptr;
  }

  // The REL's own allocation is rs_new(0xED0, "CEmperorIngStage3") at 0xC598; the port
  // allocates sizeof(CEmperorIngStage3), which is larger because the host layout is its own.
  return rs_new CEmperorIngStage3(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                  LdrToEntityInfo(info, sldrThis.editorProperties),
                                  LdrToTransform4f(sldrThis.editorProperties), *modelData,
                                  LdrToActorParameters(sldrThis.actorInformation),
                                  LdrToPatternedInfo(sldrThis.patterned, nullptr), sldrThis.data);
}

static SEmperorIngStage3_FuncPtrs REL_loader_EmperorIngStage3;

// Retail's RELMain calls fn_18_C350, which stores the loader fn_18_C380 into lbl_18_bss_30 (the
// module's SEmperorIngStage3_FuncPtrs) and calls the DOL's SetSEmperorIngStage3_FuncPtrs. The
// proxy in SEmperorIngStage3_FuncPtrs.cpp calls through it.
void RELMain() {
  REL_loader_EmperorIngStage3.mLoader = PortLoadEmperorIngStage3;
  SetSEmperorIngStage3_FuncPtrs(&REL_loader_EmperorIngStage3);
}

void RELExit() { SetSEmperorIngStage3_FuncPtrs(nullptr); }
