// Host body for the CommandoPirate REL (module 9), milestone 1: the actor loads, stands and
// thinks. Every vtable override that belongs to retail's AI forwards to CPatterned behind a
// once-only StandIn log, and the REL function it stands in for is named on each. The state
// machine (the module's trigger, state and code tables), the melee, the E-grenade, the shield and
// the burst fire are milestone 2 and forward - docs/module-analysis/CommandoPirate.md has the plan.
//
// Which methods the REL overrides is not guessed: its vtable lbl_9_data_828 (82 words, two leading
// zeros) lists them by address and the slots line up with CPatterned's declaration order (the
// entries the REL inherits are spelled out by name in the same table). The tiny accessors at the
// head of the module's .text (fn_9_0, fn_9_8, fn_9_10, fn_9_70, fn_9_78, fn_9_80, fn_9_A4) have
// four-to-twelve-instruction bodies, so they are real rather than stand-ins; fn_9_C2CC (the
// damage vulnerability) and fn_9_D474 (the projectile info) are real too.
#include "MetroidPrime/Enemies/CCommandoPirate.hpp"

#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Collision/CMaterialList.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CTeamAiRole.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrCommandoPirate.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

#include <stdio.h>
#include "rstl/string.hpp"

namespace {

// The module's global at bss_38 counts the live "Commando E-Grenade" script objects: the module's
// object constructor raises it (fn_9_F418 @0x0000F4E8) and its destructor lowers it (fn_9_F290
// @0x0000F2D0). The port does not create those objects yet, so the count stays at zero.
static int sEgrenadeObjectCount = 0;

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
  fprintf(stderr, "[cmdopirate] %s is a port stand-in (see PortCCommandoPirate.cpp)\n", what);
}

// The same, for the state machine's own functions: the message names the retail function
// the entry stands in for, so the log says which of the module's 70 FSM functions the run
// reached.
void StandInFn(const char* fn) {
  static const char* sSeen[80] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == fn) {
      return;
    }
  }
  if (sCount < 80) {
    sSeen[sCount++] = fn;
  }
  fprintf(stderr, "[cmdopirate] stand-in %s\n", fn);
}

// The body's two FSM tables. Retail builds them in a static initialiser (fn_9_F9F8,
// .ctors): the tables are lbl_9_data_198 (34 triggers, .data 0x198) and lbl_9_data_568
// (36 states, .data 0x568), each entry 16 bytes {name, 0, 0xFFFFFFFF, fn}, and the
// function pointers come from the three-word records {0, 0xFFFFFFFF, fn} in .data
// 0x0..0x198 (the triggers, fn_9_F9F8's stores at 0x0000FAD4..0x0000FF24) and in .data
// 0x3B8..0x568 (the states, the stores at 0x0000FF34..0x000103C4).
//
// On the host the table addresses are irrelevant: the state machine matches a node by its
// name, so the name strings are retail's and the entry's function is the module's member
// function. The members are cast the way the DigitalGuardian's port does it.
#define CMDO_TRIGGER(name)                                                                         \
  { #name, reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::name) }
#define CMDO_STATE(name)                                                                           \
  { #name, reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::name) }

CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    CMDO_TRIGGER(StateOver), CMDO_TRIGGER(ShouldWarpIn), CMDO_TRIGGER(ShouldMeleeAttack),
    CMDO_TRIGGER(ShouldFireEGrenade), CMDO_TRIGGER(ShouldRetreat), CMDO_TRIGGER(ShouldJumpBack),
    CMDO_TRIGGER(ShouldAmbush), CMDO_TRIGGER(BreakAmbush), CMDO_TRIGGER(HasLineOfSight),
    CMDO_TRIGGER(UnderFire), CMDO_TRIGGER(HeardShot), CMDO_TRIGGER(HasTarget),
    CMDO_TRIGGER(HasNewTarget), CMDO_TRIGGER(PathShagged), CMDO_TRIGGER(PathOver),
    CMDO_TRIGGER(IsOffPath), CMDO_TRIGGER(HasAttackPattern), CMDO_TRIGGER(IsFacingTarget),
    CMDO_TRIGGER(AttackPatternOver), CMDO_TRIGGER(TooClose), CMDO_TRIGGER(ShouldDodge),
    CMDO_TRIGGER(ShouldArmShield), CMDO_TRIGGER(ShouldShieldCharge), CMDO_TRIGGER(ShouldBoost),
    CMDO_TRIGGER(AbortShieldCharge), CMDO_TRIGGER(IsAggressive), CMDO_TRIGGER(FoundJumpPoint),
    CMDO_TRIGGER(ShouldCrouch), CMDO_TRIGGER(ShouldWallHang), CMDO_TRIGGER(ShouldCover),
    CMDO_TRIGGER(FoundCover), CMDO_TRIGGER(ShouldCoverAttack), CMDO_TRIGGER(CoverBlown),
    CMDO_TRIGGER(AbortSeekCover),
};

CPatterned::StateMachine::SStateFunction skStates[] = {
    CMDO_STATE(Lurk), CMDO_STATE(Ambush), CMDO_STATE(Alert),
    CMDO_STATE(FaceTarget), CMDO_STATE(SelectTarget), CMDO_STATE(SetTargetDest),
    CMDO_STATE(SetRetreatDest), CMDO_STATE(SetJumpDest), CMDO_STATE(SetPathMeshDest),
    CMDO_STATE(MeleeAttack), CMDO_STATE(EGrenadeAttack), CMDO_STATE(PostEGrenadeAttack),
    CMDO_STATE(JumpPointFind), CMDO_STATE(JetBoost), CMDO_STATE(PathFind),
    CMDO_STATE(Patrol), CMDO_STATE(FollowAttackPattern), CMDO_STATE(WarpIn),
    CMDO_STATE(WarpOut), CMDO_STATE(PostWarpOut), CMDO_STATE(JumpBack),
    CMDO_STATE(Dodge), CMDO_STATE(ArmShield), CMDO_STATE(ShieldCharge),
    CMDO_STATE(ScriptedShieldCharge), CMDO_STATE(RestoreOrientation), CMDO_STATE(Crouch),
    CMDO_STATE(WallHang), CMDO_STATE(WallDetach), CMDO_STATE(CoverFind),
    CMDO_STATE(SetCoverDest), CMDO_STATE(Cover), CMDO_STATE(CoverAttack),
    CMDO_STATE(BreakCover), CMDO_STATE(GetUp), CMDO_STATE(Dead),
};

#undef CMDO_TRIGGER
#undef CMDO_STATE











} // namespace

// fn_9_D904. The base call is the retail one: EPatternedAI 5 (`li r4, 0x5` at 0x0000DA54), flavor
// 0 (`li r7, 0x0` at 0x0000DA40), movement 0 / collider 1 / body 1 (the words stored to the stack
// arguments at 0x0000DA50..0x0000DA5C). The path search is fn_9_D904's own call at 0x0000D9B4:
// CPFArea null, flags 1, the patterned info's pathfinding index (+0xEC), radii 1.0f (rodata_43C)
// and no avoidance filter, root position 0 (kRP_Center).
CCommandoPirate::CCommandoPirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                                 const CTransform4f& xf, const CModelData& modelData,
                                 const CActorParameters& actParms, const CPatternedInfo& pInfo,
                                 const CCommandoPirateData& data)
: CPatterned(static_cast< EPatternedAI >(0x5), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data)                                     // 0x7c0, copied by fn_9_EA24 @0x0000D980
, x920_(-1)                                       // 0x920, fn_9_D904 @0x0000D994
, x924_(-1)                                       // 0x924, fn_9_D904 @0x0000D9A4
, mPathFindSearch(nullptr, 1, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0,
                  CPFRegion::kRP_Center)           // 0x928, fn_9_D904 @0x0000D990
, mProjectileInfo(data.mProjectile, data.mProjectileDamage) // 0xa14, fn_9_D904 @0x0000D9B8
, xa3c_(0)                                        // 0xa3c, fn_9_D904 @0x0000D9D0
, xa40_(0)                                        // 0xa40, fn_9_D904 @0x0000D9D8
, mShieldVulnerability(data.mShield.mChargeVulnerability) // 0xa44, fn_9_D904 @0x0000D9D4
, mBoneTracking(*AnimationData(), rstl::string_l("Head_1"), 1.2217305f, 3.1415927f,
                kBTF_None)                        // 0xa74, fn_9_D904 @0x0000DA08
, xae8_(0)                                        // 0xae8, the module's byte
, xb7c_(0.f)                                      // 0xb7c, the melee cooldown; fn_9_662C advances it
, xb5c_(0.f)                                      // 0xb5c, fn_9_D904 @0x0000DABC
, xb60_(1.5f)                                     // 0xb60, lbl_9_rodata_47C, fn_9_D904 @0x0000DACC
, xb64_(0.75f)                                    // 0xb64, lbl_9_rodata_4E0, fn_9_D904 @0x0000DAD8
, xb68_(0.f)                                      // 0xb68, a timer; fn_9_662C advances it
, xb6c_(0.f)                                      // 0xb6c, a timer; fn_9_662C advances it
, xb70_(0.f)                                      // 0xb70, a timer; fn_9_662C advances it
, xb74_(0.f)                                      // 0xb74, a timer; fn_9_662C advances it
, xb78_(0.f)                                      // 0xb78, a timer; fn_9_662C advances it
, xb84_(0.f)                                      // 0xb84, a timer; fn_9_662C advances it
, xb8c_(0.f)                                      // 0xb8c, fn_9_D904 @0x0000DB08
, xb90_(0.f)                                      // 0xb90, fn_9_D904 @0x0000DB0C
, xb94_(CVector3f::Zero())                         // 0xb94, fn_9_D904 @0x0000DB14
, xc50_(0.f)                                      // 0xc50, a counter; fn_9_662C lowers it
, xbca_(kInvalidUniqueId)                          // 0xbca, fn_9_D904 @0x0000DBA8
, xbc4_teamId(kInvalidUniqueId)                    // 0xbc4, the team id; fn_9_19D8 sets it
, xbcc_(kInvalidUniqueId)                          // 0xbcc, fn_9_D904 @0x0000DBB0
, xbce_(kInvalidUniqueId)                          // 0xbce, fn_9_D904 @0x0000DBB8
, xbdc_(-1)                                    // 0xbdc, the dodge id; fn_9_8934 sets it
, xbe0_(0.f)                                    // 0xbe0, the jump-back distance
, xbe4_(0.f)                                    // 0xbe4, a distance
, xbd0_(kInvalidUniqueId)                          // 0xbd0, fn_9_D904 @0x0000DBC0
, xbd2_(kInvalidUniqueId)                          // 0xbd2, fn_9_D904 @0x0000DBC8
, mLocatorHead(CSegId::Invalid())                 // 0xc5c, fn_9_D904 @0x0000E29C
, mLocatorMidLaunch(CSegId::Invalid())            // 0xc5d, fn_9_D904 @0x0000E2D0
, mLocatorGun(CSegId::Invalid())                  // 0xc5e, fn_9_D904 @0x0000E304
, mLocatorGun2(CSegId::Invalid())                 // 0xc5f, fn_9_D904 @0x0000E338
, mLocatorRWrist(CSegId::Invalid())               // 0xc60, fn_9_D904 @0x0000E36C
, mLocatorRElbow(CSegId::Invalid())               // 0xc61, fn_9_D904 @0x0000E3A0
, mLocatorLWrist(CSegId::Invalid())               // 0xc62, fn_9_D904 @0x0000E3D4
, xc63_(0)                                        // 0xc63, fn_9_D904 @0x0000E0F0
, xc64_(0)                                        // 0xc64, fn_9_D904 @0x0000E150
, xc65_(0x80)                                     // 0xc65, fn_9_D904 @0x0000E1B0
, xc66_(0)                                        // 0xc66, fn_9_D904 @0x0000E1E8
{
  // The retail constructor sets knock-back physics on CPatterned's own manager (@0x0000E264) to
  // !((byte at +0x91c >> 2) & 1) - that is, to the manager's own default while the flag bit is
  // clear - and sets the base's flag byte at +0x681. The rest - the CVisualKnowledge, the six
  // locked tokens, the two CElementGen pointers, the burst fire and the three animation distances
  // - belongs to the state machine and the render paths this milestone does not enter; the
  // analysis note lists them.
  KnockBackController().EnableKnockBackPhysics(true);

  // The vulnerability at guest +0xA44 starts as the shield's charge vulnerability (fn_9_D904
  // @0x0000D9D4 copies the block's +0xF4) and ends as the actor's own vulnerability with the
  // ignore radius: fn_9_E6D0 (@0x0000E638) copies GetDamageVulnerability(this)->MakeIgnoreRadius()
  // over it. That is the copy GetDamageVulnerability returns while the shield is up.
  mShieldVulnerability = CPatterned::GetDamageVulnerability()->MakeIgnoreRadius();

  // The seven locator segment ids (fn_9_D904 @0x0000E26C..0x0000E3F0) are read from the model's
  // animation data. The names are the module's own rodata strings, at "StateOver" + 0x3cd and
  // friends: "Head_1", "Mid_Launch_LCTRL", "gun_LCTR" (twice), "R_wrist", "R_elbow", "L_wrist".
  if (const CAnimData* animData = GetAnimationData()) {
    mLocatorHead = animData->GetLocatorSegId(rstl::string_l("Head_1"));
    mLocatorMidLaunch = animData->GetLocatorSegId(rstl::string_l("Mid_Launch_LCTRL"));
    mLocatorGun = animData->GetLocatorSegId(rstl::string_l("gun_LCTR"));
    mLocatorGun2 = animData->GetLocatorSegId(rstl::string_l("gun_LCTR"));
    mLocatorRWrist = animData->GetLocatorSegId(rstl::string_l("R_wrist"));
    mLocatorRElbow = animData->GetLocatorSegId(rstl::string_l("R_elbow"));
    mLocatorLWrist = animData->GetLocatorSegId(rstl::string_l("L_wrist"));

    // Two step distances (fn_9_5D74 is the module's inlined CPASAnimParmData ctor): the step
    // with parms (3, 2) times the scale's X (0x0000E4DC..0x0000E56C) and the step with parms
    // (1, 2) times the scale's Y (0x0000E570..0x0000E61C).
    const CVector3f scale = GetModelData()->GetScale();
    const CPASAnimParmData sideStep(pas::kAS_Step, CPASAnimParm::FromEnum(3),
                                    CPASAnimParm::FromEnum(2));
    xbe4_ = scale.GetX() * GetAnimationDistance(sideStep);
    const CPASAnimParmData backStep(pas::kAS_Step, CPASAnimParm::FromEnum(1),
                                    CPASAnimParm::FromEnum(2));
    xbe0_ = scale.GetY() * GetAnimationDistance(backStep);
  }
}

CCommandoPirate::~CCommandoPirate() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove / Death / KnockBack all dereference the current state. The analysis does not say
// where retail activates it, so the port activates on kSM_Create and, as the other patterned
// actors in this port do, again on demand in every entry point that can run first.
void CCommandoPirate::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// vtable slot 0x10 = fn_9_CAC8 (0x78): the bone tracking's own PreThink and then the base's.
// The guest's rag doll (guest this+0xBFC) is always null - the constructor stores 0 there
// (0x0000DC18) - so the guest's tests (the rag doll is there at 0x0000CAF0, its byte at +0x70
// has its 0x40 bit, the primed flag, at 0x0000CAFC) always take the bone tracking's branch (the
// call at 0x0000CB10); the base's PreThink follows (0x0000CB20).
void CCommandoPirate::PreThink(float dt, CStateManager& mgr) {
  EnsureBodyActive(mgr);
  mBoneTracking.PreThink(*AnimationData());  // 0x0000CB10
  CPatterned::PreThink(dt, mgr);             // 0x0000CB20
}

// fn_9_662C (0x39c bytes): the module's per-frame helper. The module's Think is its only caller
// (the call at 0x0000CA28) - no state calls it. It runs only while the actor is alive: the base's
// flag word at guest this+0x420, the mask 0x40 (the bit 25, CPatterned::mAlive), the test at
// 0x00006660. Then it advances the module's timers and sets its flag bits:
//
// 0x00006668: the timer at guest this+0xB70 grows by the frame time; once it reaches 5.0f
//   (rodata_468, the compare at 0x00006680) the flag byte at +0xC64 takes the bit 0x40 (the
//   rlwimi at 0x000066C0, the mask 1u << (31 - 25) = 0x40) when the block's aggressiveness (the
//   data's +0x4C, the load at 0x0000669C) is at least a random number in [0, 100] (rodata_4CC, the
//   Range call at 0x000066A4), and the timer is cleared (0x000066CC).
// 0x000066D0: the timer at +0xB68 does the same with 2.0f (rodata_44C, the compare at
//   0x000066E8), the block's cover check (the data's +0x50, the load at 0x00006710) and the flag
//   byte at +0xC63's bit 0x04 (the rlwimi at 0x0000673C, the mask 1u << (31 - 29) = 0x04) - and
//   only while the flag byte at +0xC64 does not have its 0x40 bit (the extrwi at 0x000066F8).
// 0x0000674C: the timer at +0xB6C with 5.0f (rodata_468, the compare at 0x00006764), the block's
//   search radius (the data's +0x54, the load at 0x0000678C) and the flag byte at +0xC63's bit
//   0x01 (the rlwimi at 0x000067B8, the mask 1u << (31 - 31) = 0x01) - again only while the flag
//   byte at +0xC64's 0x40 bit is clear (the extrwi at 0x00006774).
// 0x000067C8: the timer at +0xB74 grows; while an E-grenade object is alive it is cleared (the
//   module's global at bss_38, the load at 0x000067D8, the clear at 0x000067EC); otherwise it
//   reaches the grenade's minimum attack interval (the data's +0xC0, the load at 0x000067F8) and
//   then the grenade's attack chance (the data's +0xBC, the load at 0x00006824) takes the flag
//   byte at +0xC63's bit 0x02 (the rlwimi at 0x00006850, the mask 1u << (31 - 30) = 0x02) and
//   the timer is cleared (0x0000685C).
// 0x00006860: while the flag byte at +0xC64 has its 0x02 bit (the extrwi at 0x00006864) the timer
//   at +0xB84 grows (0x0000686C), that bit is cleared (the rlwimi at 0x00006888, the mask
//   1u << (31 - 30) = 0x02) and, once the timer reaches 0.3f (rodata_454, the compare at
//   0x00006898), the flag byte at +0xC64's bit 0x04 is set (the rlwimi at 0x000068A8, the mask
//   1u << (31 - 29) = 0x04).
// 0x000068B0: while the module's flag byte at +0xC66 has its 0x40 bit (the extrwi at 0x000068B4)
//   the timer at +0xB8C grows (0x000068BC).
// 0x000068C8: the timers at +0xB60, +0xB78 and +0xB7C grow - +0xB7C is the melee cooldown.
// 0x000068F4: while the shield's second unknown float (the data's +0x13C) is above zero (the
//   compare at 0x000068F8) the module's own float at +0xC50 falls by the frame time times the
//   shield's first unknown float (the data's +0x138) over it (0x00006900..0x00006918) and is
//   clamped at zero (0x00006920..0x00006930).
// 0x00006934: while the flag byte at +0xC64 does not have its 0x80 bit (the extrwi at
// 0x00006938) and the shield's first unknown float (the data's +0x138) is above zero (the
//   compare at 0x0000694C), that bit takes the compare's "greater" bit when the module's float at
//   +0xC50 is above it (the compare at 0x00006958, the rlwimi at 0x00006960, the mask
//   1u << (31 - 24) = 0x80).
// 0x00006968: while the base's flag word at guest this+0x420 has its 0x80 bit (the mask
//   1u << (31 - 24) = 0x80, the extrwi at 0x0000696C) - the base's mHitByPlayerProjectile - the
//   under-fire timer at +0xB64 is cleared and that bit is cleared (the rlwimi at 0x00006988); else
//   the timer grows (0x0000699C).
void CCommandoPirate::x662C_UpdateTimers(CStateManager& mgr, float dt) {
  if (!GetAlive()) {
    return;                                    // 0x00006660
  }
  xb70_ += dt;                                 // 0x00006668
  if (xb70_ >= 5.f) {                          // 0x00006680
    const bool roll = mData.mAggressiveness >= mgr.Random()->Range(0.f, 100.f);  // 0x000066A4
    xc64_ = (xc64_ & ~0x40) | (roll ? 0x40 : 0);  // 0x000066C0
    xb70_ = 0.f;                               // 0x000066CC
  }
  xb68_ += dt;                                 // 0x000066D0
  if (xb68_ >= 2.f) {                          // 0x000066E8
    if ((xc64_ & 0x40) == 0) {                // 0x000066F8
      const bool roll = mData.mCoverCheck >= mgr.Random()->Range(0.f, 100.f);  // 0x00006718
      xc63_ = (xc63_ & ~0x04) | (roll ? 0x04 : 0);  // 0x0000673C
    }
    xb68_ = 0.f;                               // 0x00006748
  }
  xb6c_ += dt;                                 // 0x0000674C
  if (xb6c_ >= 5.f) {                          // 0x00006764
    if ((xc64_ & 0x40) == 0) {                // 0x00006774
      const bool roll = mData.mSearchRadius >= mgr.Random()->Range(0.f, 100.f);  // 0x00006794
      xc63_ = (xc63_ & ~0x01) | (roll ? 0x01 : 0);  // 0x000067B8
    }
    xb6c_ = 0.f;                               // 0x000067C4
  }
  xb74_ += dt;                                 // 0x000067C8
  if (sEgrenadeObjectCount > 0) {              // 0x000067D8
    xb74_ = 0.f;                               // 0x000067EC
  } else if (xb74_ >= mData.mGrenade.mGrenadeMinAttackInterval) {  // 0x000067F8
    if ((xc64_ & 0x40) == 0) {                // 0x0000680C
      const bool roll = mData.mGrenade.mGrenadeAttackChance >=
                        mgr.Random()->Range(0.f, 100.f);  // 0x0000682C
      xc63_ = (xc63_ & ~0x02) | (roll ? 0x02 : 0);  // 0x00006850
    }
    xb74_ = 0.f;                               // 0x0000685C
  }
  if ((xc64_ & 0x02) != 0) {                   // 0x00006864
    xb84_ += dt;                               // 0x0000686C
    xc64_ &= ~0x02;                            // 0x00006888
    if (xb84_ >= 0.3f) {                       // 0x00006898
      xc64_ |= 0x04;                            // 0x000068A8
    }
  }
  if ((xc66_ & 0x40) != 0) {                   // 0x000068B4
    xb8c_ += dt;                               // 0x000068BC
  }
  xb60_ += dt;                                 // 0x000068C8
  xb78_ += dt;                                 // 0x000068DC
  xb7c_ += dt;                                 // 0x000068E8
  if (mData.mShield.mUnknownC3938663 > 0.f) {   // 0x000068F4
    xc50_ -= dt * mData.mShield.mUnknown6CB0DA5A / mData.mShield.mUnknownC3938663;  // 0x00006900
    if (xc50_ < 0.f) {                         // 0x00006920
      xc50_ = 0.f;                             // 0x0000692C
    }
  }
  if ((xc64_ & 0x80) == 0 && mData.mShield.mUnknown6CB0DA5A > 0.f) {  // 0x00006938, 0x0000694C
    if (xc50_ > mData.mShield.mUnknown6CB0DA5A) {  // 0x00006958
      xc64_ |= 0x80;                            // 0x00006960
    }
  }
  if (mHitByPlayerProjectile) {                // 0x0000696C
    xb64_ = 0.f;                               // 0x00006980
  } else {
    xb64_ += dt;                               // 0x0000699C
  }
}

// vtable slot 0x14 = fn_9_C944 (0x184): the actor's Think. The guest leaves everything out when
// the entity is not active (the entity's flag byte at guest this+0x20, the mask 0x80, the test at
// 0x0000C96C) - the port's GetActive() check is that test. The guest's rag-doll branches (the rag
// doll at guest this+0xBFC) are dead here, so the body is the base's Think (0x0000C9A4), the two
// collision actor managers' updates (the module's own at guest this+0xA3C and +0xA40, the calls at
// 0x0000C9B8 and 0x0000C9CC - the port has no such managers, they are created by the melee and
// grenade states), the bone tracking's Think (the call at 0x0000C9E8, which the guest makes only
// when the body controller is not frozen - its byte at +0x594, the mask 0x20) and then the module's
// own helpers: the visual knowledge's update (guest this+0xAB0, the call at 0x0000C9F8),
// fn_9_6390 (0x0000CA08), fn_9_4D84 (0x0000CA18), fn_9_662C (0x0000CA28 - the helper that
// advances the module's timers, including the melee cooldown at guest this+0xB7C) and fn_9_3090
// (0x0000CA34), then the tail: fn_9_1C0C (0x0000CA88), fn_9_4BC0 (0x0000CA8C) and fn_9_3E1C
// (0x0000CAA0).
void CCommandoPirate::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;                                    // 0x0000C96C
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);                  // 0x0000C9A4
  StandIn("CCommandoPirate's collision actor managers (guest this+0xa3c, this+0xa40)");  // 0x0000C9B8
  if (!BodyController()->IsFrozen()) {
    mBoneTracking.Think(dt);                   // 0x0000C9E8
  }
  StandIn("CVisualKnowledge::Update (guest this+0xab0)");  // 0x0000C9F8
  StandInFn("fn_9_6390 (0x29c bytes)");        // 0x0000CA08
  StandInFn("fn_9_4D84 (0x228 bytes)");        // 0x0000CA18
  x662C_UpdateTimers(mgr, dt);                 // 0x0000CA28
  StandInFn("fn_9_3090 (0x2a8 bytes)");        // 0x0000CA34
  StandInFn("fn_9_1C0C (0x2ec bytes)");        // 0x0000CA88
  StandInFn("fn_9_4BC0 (0xe4 bytes)");         // 0x0000CA8C
  StandInFn("fn_9_3E1C (0x324 bytes)");        // 0x0000CAA0
}

// vtable slot 0x18 = fn_9_CB40 (0x754): the message switch that activates the body and starts the
// module's state machine.
void CCommandoPirate::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CCommandoPirate::AcceptScriptMsg (vtable slot 0x18, fn_9_CB40)");
  CPatterned::AcceptScriptMsg(mgr, msg);
  if (msg.GetMessage() == kSM_Create) {
    EnsureBodyActive(mgr);
  } else if (msg.GetMessage() == kSM_AreaLoaded) {
    // 0x0000CEC8..0x0000CEF0: the guest's area-loaded case sets the path find search's area
    // pointer (guest this+0x928) to the area's path area: the world's area at the actor's area id
    // (this+0x04, the load at 0x0000CEC8), its post-constructed data (+0x104, the load at
    // 0x0000CEE8) and that data's path area (+0xEC, the load at 0x0000CEEC). Without it the
    // search has no area and every path find fails.
    mPathFindSearch.SetArea(
        mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea);
  }
}

// vtable slot 0x24 = fn_9_C6D4 (0xc4): the base's PreRender and then the bone tracking's own.
// The guest's rag doll (guest this+0xBFC) is always null - the constructor stores 0 there
// (0x0000DC18) - so the guest's rag-doll call (the vtable slot 0x10 call at 0x0000C718) is dead
// here and only the base's PreRender runs (0x0000C728). The bone tracking's own PreRender (the
// call at 0x0000C77C) follows with the model's scale (the three floats at the model data's +0,
// loaded at 0x0000C754..0x0000C770), the actor's transform (guest this+0x24) and the body
// controller (guest this+0x48c).
void CCommandoPirate::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);  // 0x0000C728
  mBoneTracking.PreRender(mgr, *AnimationData(), GetTransform(), GetModelData()->GetScale(),
                          *BodyController());  // 0x0000C77C
}

// vtable slot 0x28 = fn_9_C4FC (0x20): a straight call to CPatterned::AddToRenderer, so the base's
// is what runs.
void CCommandoPirate::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
}

// vtable slot 0x2c = fn_9_C598 (0x13c).
void CCommandoPirate::Render(const CStateManager& mgr) const {
  StandIn("CCommandoPirate::Render (vtable slot 0x2c, fn_9_C598)");
  CPatterned::Render(mgr);
}

// vtable slot 0x34 = fn_9_C51C (0x7c): the base's own. The guest's rag doll (guest this+0xBFC)
// is always null - the constructor stores 0 there (0x0000DC18) - so the guest's rag-doll branch
// (the CRagDoll::PreRenderAllViewports call at 0x0000C560 with lbl_9_rodata_458 = 0.2f, then the
// portal system state) is dead here and the base's PreRenderAllViewports runs (0x0000C57C).
void CCommandoPirate::PreRenderAllViewports(CStateManager& mgr) {
  CPatterned::PreRenderAllViewports(mgr);  // 0x0000C57C
}

// vtable slot 0x40 = fn_9_C2CC (0x34): the shield's damage vulnerability while the flag byte at
// guest +0xc65 has its 0x04 bit set (the guest tests it with extrwi at 0x0000C2DC), the actor's
// own vulnerability otherwise.
const CDamageVulnerability* CCommandoPirate::GetDamageVulnerability() const {
  if (xc65_ & 0x04) {
    return &mShieldVulnerability;
  }
  return CPatterned::GetDamageVulnerability();
}

// vtable slot 0x70 = fn_9_C798 (0x1ac): the user-event switch that starts the melee, the
// E-grenade and the shield charge.
void CCommandoPirate::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                      EUserEventType type, float dt) {
  StandIn("CCommandoPirate::DoUserAnimEvent (vtable slot 0x70, fn_9_C798)");
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

// vtable slot 0x84 = fn_9_BF44 (0x2b0): what the blade and the shield hit.
void CCommandoPirate::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                                    CStateManager& mgr) {
  StandIn("CCommandoPirate::CollidedWith (vtable slot 0x84, fn_9_BF44)");
  CPatterned::CollidedWith(id, list, mgr);
}

// vtable slot 0x9c = fn_9_C300 (0x1fc).
void CCommandoPirate::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  StandIn("CCommandoPirate::KnockBack (vtable slot 0x9c, fn_9_C300)");
  CPatterned::KnockBack(mgr, info);
}

// vtable slot 0xa8 = fn_9_A4 (0x8): `li r3, 0x1`, so the actor can always be shot.
bool CCommandoPirate::CanBeShot(const CStateManager& mgr, int attributes) { return true; }

// vtable slot 0xac = fn_9_8 (0x8): `li r3, 0x1`, so the actor always listens.
bool CCommandoPirate::IsListening() const { return true; }

// vtable slot 0xb0 = fn_9_BE4C (0xf8): what the actor hears.
bool CCommandoPirate::Listen(CStateManager& mgr, const CVector3f& position, EListenNoiseType type) {
  StandIn("CCommandoPirate::Listen (vtable slot 0xb0, fn_9_BE4C)");
  return CPatterned::Listen(mgr, position, type);
}

// vtable slot 0xdc = fn_9_D474 (0x8): `addi r3, r3, 0xa14`, the projectile info at guest +0xa14.
CProjectileInfo* CCommandoPirate::ProjectileInfo() { return &mProjectileInfo; }

// vtable slot 0xe0 = fn_9_0 (0x8): `addi r3, r3, 0x928`, the path search at guest +0x928.
CPathFindSearch* CCommandoPirate::GetSearchPath() { return &mPathFindSearch; }

// vtable slot 0xe4 = fn_9_70 (0x8): `li r3, 0x0` - this actor has no point-based path.
CPathFindPointSearch* CCommandoPirate::GetPointSearchPath() { return nullptr; }

// vtable slot 0xe8 = fn_9_C1F4 (0xd8): the contact damage, which the module's own blade damage
// feeds in place of the patterned info's.
CDamageInfo CCommandoPirate::GetContactDamage() const {
  StandIn("CCommandoPirate::GetContactDamage (vtable slot 0xe8, fn_9_C1F4)");
  return CPatterned::GetContactDamage();
}

// ---------------------------------------------------------------------------------------------
// The body's FSM functions. Retail's entries are the module's own functions; their bodies are
// not ported in this slice, so each one logs the retail function it stands in for and returns
// the neutral value (false for a trigger, nothing for a state).
//
// The 34 triggers of lbl_9_data_198 (.data 0x198; fn_9_F9F8's stores at 0x0000FAD4..0x0000FF24).

// fn_9_BDCC: the base's animation state id (guest this+0x6B4) with 3. The asm tests it with
// "subfic r0, r0, 3; cntlzw r0, r0; srwi r3, r0, 5" (0x0000BDD0..0x0000BDD8), i.e. the answer is
// true when the id is 3 - the host enum's kAS_Over (the ctor's state id 0 is "not ready").
bool CCommandoPirate::StateOver(CStateManager& mgr, const CTriggerData& data) const {
  return mAnimationState.GetState() == CAnimationState::kAS_Over;
}

// fn_9_BD98: the actor is warped in when the state machine's owner is unset (the word at guest
// +0x920 is -1, cmpwi r0, -0x1 at 0x0000BDA0), the block's flag byte at +0x15D (guest this+0x91D)
// has its 0x80 bit set (extrwi. r0, r0, 1, 24 at 0x0000BDAC) and the module's flag byte at +0xC63
// has its 0x20 bit set (extrwi. r0, r0, 1, 26 at 0x0000BDB8).
// The block's +0x15D is the loader's mFlags1 - the "sound" property's bits 6, 3, 0 and 29 (the
// data ctor fn_9_EDC0 packs them at 0x0000EF34..0x0000EF60) - so the flag is script-supplied.
bool CCommandoPirate::ShouldWarpIn(CStateManager& mgr, const CTriggerData& data) const {
  return x920_ == -1 && (mData.mFlags1 & 0x80) != 0 && (xc63_ & 0x20) != 0;
}

// fn_9_BCB8: the commando melees when the target is inside its attack band. The block's flag
// byte at +0x15C (guest this+0x91C) must not have its 0x02 bit (extrwi at 0x0000BCCC), and the
// module's own cooldown timer at guest this+0xB7C (the time since the last melee ended, which
// the module's per-frame helper fn_9_662C advances at 0x000068E8 and fn_9_A60C clears at
// 0x0000A700) must be above 1.0f (rodata_43C, the compare at 0x0000BCE4). Then the target (the id
// at guest this+0xBCA, the lookup at 0x0000BCFC) must be inside the base's attack band: the
// squared distance must not be below the minimum attack range (guest this+0x3DC, the compare at
// 0x0000BD48) nor above the maximum attack range (guest this+0x3E0, the compare at
// 0x0000BD54), and the height difference must be under 3.0f (rodata_448, the compare at
// 0x0000BD70).
//
// The guest reads the target's position at the actor's +0x54 without a cast; the host's CEntity
// has no position, so the port casts to CActor and keeps a null test of its own.
bool CCommandoPirate::ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const {
  if ((mData.mFlags0 & 0x02) != 0) {
    return false;                                   // 0x0000BCCC
  }
  if (xb7c_ <= 1.f) {
    return false;                                   // 0x0000BCE4
  }
  const CEntity* target = mgr.GetObjectById(xbca_);  // 0x0000BCFC
  if (target == nullptr) {
    return false;                                   // 0x0000BD00
  }
  const CActor* actor = TCastToConstPtr< CActor >(target);
  if (actor == nullptr) {
    return false;
  }
  const CVector3f delta = actor->GetTranslation() - GetTranslation();
  const float distSq = delta.MagSquared();
  if (distSq < mMinAttackRange * mMinAttackRange) {
    return false;                                   // 0x0000BD48
  }
  if (distSq > mMaxAttackRange * mMaxAttackRange) {
    return false;                                   // 0x0000BD54
  }
  return fabsf(delta.GetZ()) < 3.f;                  // 0x0000BD70
}

// fn_9_BBC8: the commando fires its E-grenade when the target is inside the grenade's band.
// The module's flag byte at guest this+0xC63 must have its 0x02 bit (extrwi at 0x0000BBDC), no
// E-grenade script object may be alive (the module's global at bss_38, the load at
// 0x0000BBEC) and the block's flag byte at +0x15D (guest this+0x91D) must not have its 0x10 bit
// (extrwi at 0x0000BBF8). Then the target's aim position (the module's helper fn_9_3CBC with the
// dt 0.f, the call at 0x0000BC30) must be inside the grenade's band: the squared distance must
// not be below the grenade's minimum attack distance (the block's +0x60, the compare at
// 0x0000BC80) nor above its maximum (the block's +0x64, the compare at 0x0000BC8C).
bool CCommandoPirate::ShouldFireEGrenade(CStateManager& mgr, const CTriggerData& data) const {
  if ((xc63_ & 0x02) == 0) {
    return false;                                   // 0x0000BBDC
  }
  if (sEgrenadeObjectCount != 0) {
    return false;                                   // 0x0000BBEC
  }
  if ((mData.mFlags1 & 0x10) != 0) {
    return false;                                   // 0x0000BBF8
  }
  CVector3f aim;
  if (!x3CBC_(mgr, aim, 0.f)) {
    return false;                                   // 0x0000BC30
  }
  const CVector3f delta = aim - GetTranslation();
  const float distSq = delta.MagSquared();
  if (distSq < mData.mGrenade.mGrenadeMinAttackDist * mData.mGrenade.mGrenadeMinAttackDist) {
    return false;                                   // 0x0000BC80
  }
  if (distSq > mData.mGrenade.mGrenadeMaxAttackDist * mData.mGrenade.mGrenadeMaxAttackDist) {
    return false;                                   // 0x0000BC8C
  }
  return true;
}

// fn_9_BB7C: the commando retreats when its flag byte at guest this+0xC63 has its 0x40 bit set
// (extrwi. r0, r0, 1, 25 at 0x0000BB80), the block's flag byte at +0x15D does not have its 0x80
// bit (extrwi. r0, r0, 1, 24 at 0x0000BB8C), the block's flag byte at +0x15C does not have its
// 0x08 bit (extrwi. r0, r0, 1, 28 at 0x0000BB98) and the id at guest this+0xBCE is not
// kInvalidUniqueId (the compare at 0x0000BBA0..0x0000BBB8).
bool CCommandoPirate::ShouldRetreat(CStateManager& mgr, const CTriggerData& data) const {
  return (xc63_ & 0x40) != 0 && (mData.mFlags1 & 0x80) == 0 &&
         (mData.mFlags0 & 0x08) == 0 && xbce_ != kInvalidUniqueId;
}

// fn_9_BB10: the commando jumps back. The block's flag byte at +0x15C (guest this+0x91C) must not
// have its 0x08 bit (extrwi at 0x0000BB20); then the module's own helper fn_9_3338 (0x148 bytes,
// over the 0x80 limit) decides - the guest calls it with the actor's forward column negated (the
// three loads at 0x0000BB28..0x0000BB54) and the module's float at guest this+0xBE0 (the load at
// 0x0000BB58).
bool CCommandoPirate::ShouldJumpBack(CStateManager& mgr, const CTriggerData& data) const {
  if ((mData.mFlags0 & 0x08) != 0) {
    return false;                                   // 0x0000BB20
  }
  // 0x0000BB28..0x0000BB54: the actor's forward column, negated, and the module's float at
  // guest this+0xBE0 as the distance (the load at 0x0000BB58).
  return x3338_IsJumpBackClear(mgr, -GetTransform().GetForward(), xbe0_);  // 0x0000BB60
}

// fn_9_3338 (0x148 bytes): the module's jump-back helper, which both the jump-back trigger
// (fn_9_BB10) and the dodge picker (fn_9_3678) call. The ray starts at the actor's bounding
// box centre (the GetBoundingBox call at 0x00003374 and the GetCenterPoint call at
// 0x00003380) and ends at the point that distance along the given vector (the adds at
// 0x000033B0..0x000033D8); the end point must not hit the world (the RayCollideWorld call at
// 0x0000342C - the answer is false when it does) and must be on the path (the OnPath call at
// 0x00003440 - the answer is true when it returns 0).
bool CCommandoPirate::x3338_IsJumpBackClear(CStateManager& mgr, const CVector3f& dir,
                                           float distance) const {
  const CVector3f center = GetBoundingBox().GetCenterPoint();  // 0x00003374, 0x00003380
  const CVector3f end = center + distance * dir;                // 0x000033B0..0x000033D8
  // The guest's material filter comes from the module's own static lists (the .data words at
  // lbl_9_data_810 and lbl_9_data_80C, the two __shl2i calls at 0x000033DC and 0x000033F8);
  // the port uses the solid material list, the filter a collision ray needs.
  const CMaterialFilter filter = CMaterialFilter::MakeInclude(CMaterialList(kMT_Solid));
  if (mgr.RayCollideWorld(center, end, filter, this)) {
    return false;                                                // 0x00003430
  }
  return mPathFindSearch.OnPath(end) == CPathFindSearch::kR_Success;  // 0x00003440
}

// fn_9_BB04: the block's flag byte at +0x15C (guest this+0x91C) has its 0x80 bit set (extrwi
// r3, r0, 1, 24 at 0x0000BB08). That byte is the loader's mFlags0 - the "sound" property's bits
// 14, 11, 8, 5, 2, 31, 28 and 25 (the data ctor fn_9_EDC0 packs them at 0x0000EED4..0x0000EF30)
// - so the flag is script-supplied.
bool CCommandoPirate::ShouldAmbush(CStateManager& mgr, const CTriggerData& data) const {
  return (mData.mFlags0 & 0x80) != 0;
}

// fn_9_BAF8: the block's flag byte at +0x15C (guest this+0x91C) has its 0x40 bit set (extrwi
// r3, r0, 1, 25 at 0x0000BAFC). That byte is the loader's mFlags0 - the "sound" property's bits
// 14, 11, 8, 5, 2, 31, 28 and 25 (the data ctor fn_9_EDC0 packs them at 0x0000EED4..0x0000EF30)
// - so the flag is script-supplied.
bool CCommandoPirate::BreakAmbush(CStateManager& mgr, const CTriggerData& data) const {
  return (mData.mFlags0 & 0x40) != 0;
}

// fn_9_BAEC: the module's byte at guest this+0xAE8 has its 0x80 bit set (extrwi r3, r0, 1, 24
// at 0x0000BAF0). Nothing in the module writes that byte, so the bit is clear.
bool CCommandoPirate::HasLineOfSight(CStateManager& mgr, const CTriggerData& data) const {
  return (xae8_ & 0x80) != 0;
}

// fn_9_BAD0: the actor is under fire while the float at guest +0xB64 is below 0.75f
// (lbl_9_rodata_4E0, loaded at 0x0000BAD8). The compare is fcmpo cr0, f1, f0 (0x0000BADC) and the
// return takes cr0.LT (mfcr r0 at 0x0000BAE0, srwi r3, r0, 31 at 0x0000BAE4). The constructor
// sets that member to 0.75f, so the test is false until something lowers it.
bool CCommandoPirate::UnderFire(CStateManager& mgr, const CTriggerData& data) const {
  return xb64_ < 0.75f;
}

// fn_9_BAB4: the float at guest this+0xB60 is below 1.5f (lbl_9_rodata_47C, loaded at
// 0x0000BAB8). The compare is fcmpo cr0, f1, f0 (0x0000BAC0) and the return takes cr0.LT
// (mfcr r0 at 0x0000BAC4, srwi r3, r0, 31 at 0x0000BAC8). The constructor sets that float to
// 1.5f (0x0000DACC), the same constant, so the test is false until something lowers it.
bool CCommandoPirate::HeardShot(CStateManager& mgr, const CTriggerData& data) const {
  return xb60_ < 1.5f;
}

// fn_9_BA78: the target the commando holds (the id at guest this+0xBCA) is still in the world.
// The guest looks it up (the GetObjectById call at 0x0000BA94) and answers whether the lookup
// found something (the neg/or/srwi pair at 0x0000BA98..0x0000BAA0).
bool CCommandoPirate::HasTarget(CStateManager& mgr, const CTriggerData& data) const {
  return mgr.GetObjectById(xbca_) != nullptr;
}

// fn_9_BA20: there is a target and it is a new one. The guest asks its own HasTarget trigger
// first (the call at 0x0000BA3C, fn_9_BA78) and then compares the id at guest this+0xBCC with
// the target id at +0xBCA (0x0000BA48..0x0000BA54).
bool CCommandoPirate::HasNewTarget(CStateManager& mgr, const CTriggerData& data) const {
  return HasTarget(mgr, data) && xbcc_ != xbca_;
}

// fn_9_B9D8: the commando's own flag byte at guest this+0xC63 has its 0x80 bit set (extrwi.
// r0, r0, 1, 24 at 0x0000B9F0), or the base's own answer says the path is shagged (the call at
// 0x0000B9F8).
bool CCommandoPirate::PathShagged(CStateManager& mgr, const CTriggerData& data) const {
  return (xc63_ & 0x80) != 0 || CPatterned::PathShagged(mgr, data);
}

// fn_9_B990: the commando's own flag byte at guest this+0xC65 does not have its 0x80 bit set
// (extrwi. r0, r0, 1, 24 at 0x0000B9A8 - the beq at 0x0000B9AC takes the "true" branch), or the
// base's own answer says the path is over (the call at 0x0000B9B0).
bool CCommandoPirate::PathOver(CStateManager& mgr, const CTriggerData& data) const {
  return (xc65_ & 0x80) == 0 || CPatterned::PathOver(mgr, data);
}

bool CCommandoPirate::IsOffPath(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_B958");
  return false;
}

// fn_9_B900: the state machine's asset has an "attack" connection that is being followed -
// GetConnectedObject(mgr, kSS_Attack, kSM_Follow) (the call at 0x0000B928, with the two FourCCs
// built at 0x0000B908..0x0000B924) - and the actor's compare against kInvalidUniqueId
// (0x0000B930..0x0000B944) is the != test.
bool CCommandoPirate::HasAttackPattern(CStateManager& mgr, const CTriggerData& data) const {
  return GetConnectedObject(mgr, kSS_Attack, kSM_Follow) != kInvalidUniqueId;
}

// fn_9_B818: the commando is facing the target when the angle between its forward and the
// direction to the target's aim position (the module's helper fn_9_3CBC with the dt 0.f, the
// call at 0x0000B85C) is at most 20 degrees (rodata_4B4 = 0.34906584, the compare at
// 0x0000B8D4). The guest flattens the direction (the 0.f stored over the z difference at
// 0x0000B8AC) and a target that is gone answers true (the test at 0x0000B860).
bool CCommandoPirate::IsFacingTarget(CStateManager& mgr, const CTriggerData& data) const {
  CVector3f aim;
  if (!x3CBC_(mgr, aim, 0.f)) {
    return true;                                    // 0x0000B860
  }
  CVector3f delta = aim - GetTranslation();
  delta.SetZ(0.f);                                  // 0x0000B8AC
  return CVector3f::GetAngleDiff(GetTransform().GetForward(), delta) <= 0.34906584f;  // 0x0000B8D4
}

// fn_9_B7FC: the base's waypoint navigation destination (guest this+0x6C0, the u16 loaded at
// 0x0000B800) is kInvalidUniqueId. The guest's test is the cntlzw idiom - subf r0, r5, r0
// (0x0000B808), cntlzw (0x0000B80C) and srwi r3, r0, 5 (0x0000B810) - one iff the id is 0xFFFF.
// The host's base carries the same role in its waypoint navigation, as the Grenchler's own
// AttackPatternOver does.
bool CCommandoPirate::AttackPatternOver(CStateManager& mgr, const CTriggerData& data) const {
  return mWaypointNavigation.GetDestination() == kInvalidUniqueId;
}

// fn_9_B76C: the target is closer than the actor's minimum attack range. The guest looks the
// target up by the id at guest this+0xBCA (the GetObjectById call at 0x0000B790) and, when it is
// there, compares the squared distance between the two positions (the loads at
// 0x0000B79C..0x0000B7D0) with the squared radius at guest this+0x3DC (the load at
// 0x0000B7A4). That radius is the base's minimum attack range - the host's own
// CPatterned::TooClose compares against mMinAttackRange (src/MetroidPrime/Enemies/
// CPatternedAiFunctions.cpp:107).
bool CCommandoPirate::TooClose(CStateManager& mgr, const CTriggerData& data) const {
  const CEntity* target = mgr.GetObjectById(xbca_);
  if (target == nullptr) {
    return false;
  }
  const CActor* actor = TCastToConstPtr< CActor >(target);
  if (actor == nullptr) {
    return false;
  }
  return (actor->GetTranslation() - GetTranslation()).MagSquared() <
         mMinAttackRange * mMinAttackRange;
}

// fn_9_B690: the commando dodges when the target is in front of it and it has just heard a
// shot or is under fire. The block's flag byte at +0x15C (guest this+0x91C) must not have its
// 0x20 bit (extrwi at 0x0000B6A4) and the module's flag byte at guest this+0xC63 must have its
// 0x04 bit (extrwi at 0x0000B6B0). The target (the id at guest this+0xBCA, the lookup at
// 0x0000B6CC) must be in front of the actor: the dot product of the direction to it with the
// actor's forward column (guest this+0x28/0x38/0x48) must be above zero (the compare at
// 0x0000B71C). Then either the under-fire distance at guest this+0xB64 is under 0.75f
// (rodata_4E0, the compare at 0x0000B730) or the heard-shot distance at guest this+0xB60 is
// under 1.5f (rodata_47C, the compare at 0x0000B744).
bool CCommandoPirate::ShouldDodge(CStateManager& mgr, const CTriggerData& data) const {
  if ((mData.mFlags0 & 0x20) != 0) {
    return false;                                   // 0x0000B6A4
  }
  if ((xc63_ & 0x04) == 0) {
    return false;                                   // 0x0000B6B0
  }
  const CEntity* target = mgr.GetObjectById(xbca_);  // 0x0000B6CC
  if (target == nullptr) {
    return false;                                   // 0x0000B6D0
  }
  const CActor* actor = TCastToConstPtr< CActor >(target);
  if (actor == nullptr) {
    return false;
  }
  const CVector3f delta = actor->GetTranslation() - GetTranslation();
  if (CVector3f::Dot(delta, GetTransform().GetForward()) <= 0.f) {
    return false;                                   // 0x0000B71C
  }
  return xb64_ < 0.75f || xb60_ < 1.5f;             // 0x0000B730, 0x0000B744
}

// fn_9_B660: the commando arms its shield when its flag byte at guest this+0xC64 has its 0x80
// bit set (extrwi. r0, r4, 1, 24 at 0x0000B668), the block's flag byte at +0x15D does not have
// its 0x20 bit (extrwi. r0, r0, 1, 26 at 0x0000B674) and that same byte's 0x01 bit is clear
// (clrlwi. r0, r4, 31 at 0x0000B67C).
bool CCommandoPirate::ShouldArmShield(CStateManager& mgr, const CTriggerData& data) const {
  return (xc64_ & 0x80) != 0 && (mData.mFlags1 & 0x20) == 0 && (xc64_ & 0x01) == 0;
}

bool CCommandoPirate::ShouldShieldCharge(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_B250");
  return false;
}

bool CCommandoPirate::ShouldBoost(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_B1E0");
  return false;
}

// fn_9_B1D4: the module's flag byte at guest this+0xC64 has its 0x04 bit set (extrwi r3, r0,
// 1, 29 at 0x0000B1D8). The constructor clears that byte (0x0000E150); the module's states set
// its bits.
bool CCommandoPirate::AbortShieldCharge(CStateManager& mgr, const CTriggerData& data) const {
  return (xc64_ & 0x04) != 0;
}

// fn_9_B1C8: the module's flag byte at guest this+0xC64 has its 0x40 bit set (extrwi r3, r0,
// 1, 25 at 0x0000B1CC).
bool CCommandoPirate::IsAggressive(CStateManager& mgr, const CTriggerData& data) const {
  return (xc64_ & 0x40) != 0;
}

// fn_9_B1A8: the module's jump point id at guest this+0xBD0 (the u16 loaded at 0x0000B1AC) is
// not kInvalidUniqueId. The guest's test is "subf r3, r5, r0; subf r0, r0, r5; or r0, r3, r0"
// (0x0000B1B4..0x0000B1BC) and keeps the top bit (srwi r3, r0, 31 at 0x0000B1C0): one iff the
// two differ. The constructor leaves the id kInvalidUniqueId (0x0000DBC0).
bool CCommandoPirate::FoundJumpPoint(CStateManager& mgr, const CTriggerData& data) const {
  return xbd0_ != kInvalidUniqueId;
}

bool CCommandoPirate::ShouldCrouch(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_B168");
  return false;
}

bool CCommandoPirate::ShouldWallHang(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_B114");
  return false;
}

// fn_9_B0F4: the block's flag byte at +0x15C (guest this+0x91C) must have its 0x08 bit clear
// (extrwi. r0, r0, 1, 28 at 0x0000B0F8, and the beq at 0x0000B0FC takes the "clear" branch) and
// the module's flag byte at this+0xC63 must have its 0x01 bit set (clrlwi r3, r0, 31 at
// 0x0000B10C keeps only that bit).
bool CCommandoPirate::ShouldCover(CStateManager& mgr, const CTriggerData& data) const {
  return (mData.mFlags0 & 0x08) == 0 && (xc63_ & 0x01) != 0;
}

// fn_9_B0D4: the module's cover point id at guest this+0xBD2 (the u16 loaded at 0x0000B0DC)
// is not kInvalidUniqueId - the same "the two differ" idiom as FoundJumpPoint (0x0000B0E0..).
// The constructor leaves the id kInvalidUniqueId (0x0000DBC8).
bool CCommandoPirate::FoundCover(CStateManager& mgr, const CTriggerData& data) const {
  return xbd2_ != kInvalidUniqueId;
}

// fn_9_B0B4: the float at guest this+0xB5C is at most 0.f (lbl_9_rodata_494, loaded at
// 0x0000B0BC). The compare is fcmpo cr0, f1, f0 (0x0000B0C0) and the cror at 0x0000B0C4 makes
// cr0's eq bit "lt or eq", so the answer is "the first is at most the second".
bool CCommandoPirate::ShouldCoverAttack(CStateManager& mgr, const CTriggerData& data) const {
  return xb5c_ <= 0.f;
}

bool CCommandoPirate::CoverBlown(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_AFD4");
  return false;
}

// fn_9_AFB8: the float at guest this+0xB8C against the float at this+0xB90 (both loaded at
// 0x0000AFB8 and 0x0000AFBC). The compare is fcmpo cr0, f1, f0 (0x0000AFC0) and the cror at
// 0x0000AFC4 makes cr0's eq bit "gt or eq", so the answer is "the first is at least the second".
bool CCommandoPirate::AbortSeekCover(CStateManager& mgr, const CTriggerData& data) const {
  return xb8c_ >= xb90_;
}

// The 36 states of lbl_9_data_568 (.data 0x568; fn_9_F9F8's stores at 0x0000FF34..0x000103C4).

// fn_9_ADD4: the lurk state only does something on activate (the guest's test of the message
// against 0 at 0x0000ADD4): when the commando has a target (the id at guest this+0xBCA is not
// kInvalidUniqueId, 0x0000ADE0..0x0000ADEC) it sets the 0x10 bit of the flag byte at +0xC64
// (the rlwimi at 0x0000ADF8 keeps the bit at position 27, the mask 1u << (31 - 27) = 0x10).
void CCommandoPirate::Lurk(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {
    return;
  }
  if (xbca_ == kInvalidUniqueId) {
    return;
  }
  xc64_ |= 0x10;
}

void CCommandoPirate::Ambush(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_ADA0");
}

// fn_9_AE04: the alert state. On activate (the guest's message 0, 0x0000AE24) it remembers the
// target id in the field at guest this+0xBCC (the store at 0x0000AE44) and marks the animation
// state "ready" (the store at 0x0000AE48). On update (the guest's message 1, 0x0000AE0C) it asks
// the animation state whether the alert command may be issued - the guest's ShouldRetry with the
// command id 0x10 (0x0000AE58), which is the host's pas::kAS_Taunt - and, when it may, makes the
// commando taunt (the DeliverCmd calls at 0x0000AEAC and 0x0000AF10). On deactivate (the guest's
// message 2, 0x0000AF40) the animation state goes back to "not ready".
void CCommandoPirate::Alert(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    xbcc_ = xbca_;                                        // 0x0000AE44
    mAnimationState.SetState(CAnimationState::kAS_Ready);  // 0x0000AE48
    break;
  }
  case kStateMsg_Update: {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
      // 0x0000AE68..0x0000AE74: the body controller's field at +0x580 picks the taunt: the
      // relaxed taunt (kTT_Two, the li r0, 2 at 0x0000AE90) while the actor is relaxed, the
      // other one (kTT_Four, the li r0, 4 at 0x0000AEF4) otherwise. That field is the body
      // controller's locomotion type: the DOL's SetLocomotionType (fn_800F73EC) is the single
      // "stw r4, 0x580(r3)" that writes it, and 1 is the relaxed locomotion.
      const pas::ETauntType type = BodyController()->GetLocomotionType() == pas::kLT_Relaxed
                                       ? pas::kTT_Two
                                       : pas::kTT_Four;
      // 0x0000AEAC / 0x0000AF10: the taunt command itself; the guest's stores at the command
      // manager's +0x2B0 and +0x2B4 (0x0000AEC0 and 0x0000AED0) are the command id and the
      // taunt type, which is what the host's DeliverCmd does with the command.
      BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(type));
    }
    break;
  }
  case kStateMsg_Deactivate: {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);  // 0x0000AF40
    break;
  }
  default:
    break;
  }
}

// fn_9_3CBC: the target's aim position. The guest looks the target up by the id at this+0xBCA
// (the load at 0x00003CD8) and, when it exists, asks it for its aim position - the vtable call
// at 0x00003D1C, CActor's slot 0x54, GetAimPosition - with the state manager and the dt, and
// copies the answer into the out vector (0x00003D28..0x00003D38). It returns false when the
// target is gone (0x00003D40).
bool CCommandoPirate::x3CBC_(CStateManager& mgr, CVector3f& out, float dt) const {
  const CEntity* target = mgr.GetObjectById(xbca_);
  if (target == nullptr) {
    return false;
  }
  const CActor* actor = TCastToConstPtr< CActor >(target);
  if (actor == nullptr) {
    return false;
  }
  out = actor->GetAimPosition(mgr, dt);
  return true;
}

// fn_9_ABDC: the state faces the target. On activate (the guest's message 0, 0x0000ABFC) the
// module's own helper x3CBC_ (fn_9_3CBC) asks the target for its aim position and stores it at
// guest this+0xB94; the state then measures the angle between the actor's facing and the
// direction to the target (the GetAngleDiff call at 0x0000AC84) against 20 degrees
// (lbl_9_rodata_4B4 is 0.34906584): over when the target is in front (0x0000ACA4) and ready when
// it is not (0x0000AC98).
// On update (the guest's message 1, 0x0000ABE4) the guest retries its own try command - that
// word is the base's animation state, so the port asks the animation state whether the command
// may be issued - and, when it succeeds, checks the distance to the target (IsMagnitudeSafe at
// 0x0000AD08) and delivers the locomotion command that walks there (the CBCLocomotionCmd at
// 0x0000AD70).
// On deactivate (the guest's message 2, 0x0000AD84) the animation state goes back to "not ready".
void CCommandoPirate::FaceTarget(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    // 0x0000AC1C: the guest's own helper asks the target for its aim position (fn_9_3CBC) with
    // 0.5f (lbl_9_rodata_440) and stores it at guest this+0xB94.
    if (x3CBC_(mgr, xb94_, 0.5f)) {
      // 0x0000AC2C..0x0000AC84: the direction to the target and the angle between it and the
      // actor's facing (the guest's three-argument GetAngleDiff has no host counterpart; the
      // host's takes the facing and the direction).
      const CVector3f delta = xb94_ - GetTranslation();
      // 0x0000AC6C..0x0000AC80: the actor's facing is the transform's forward column
      // (GetTransform().GetForward()); the guest's three loads are its 0x28, 0x38 and 0x48.
      const float angle = CVector3f::GetAngleDiff(GetTransform().GetForward(), delta);
      mAnimationState.SetState(angle > 0.34906584f ? CAnimationState::kAS_Ready
                                                   : CAnimationState::kAS_Over);
    } else {
      // 0x0000ACB0: no target, so the animation is over.
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
    break;
  }
  case kStateMsg_Update: {
    // 0x0000ACC8: the guest asks its own try command (guest this+0x6B4) whether the turn command
    // may be issued; that word is the base's animation state, so the host's animation state
    // answers it (CAnimationState::CanIssueCommand), with the guest's command id 8
    // (pas::kAS_Turn).
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Turn)) {
      // 0x0000ACD4..0x0000AD08: the direction to the target and its magnitude test.
      const CVector3f delta = xb94_ - GetTranslation();
      if (delta.IsMagnitudeSafe()) {
        // 0x0000AD1C..0x0000AD70: the normalized direction, delivered as a locomotion command
        // with the move vector zero and the weight 1.
        const CBCLocomotionCmd cmd(CVector3f::Zero(), delta.AsNormalized(), 1.f);
        BodyController()->CommandMgr().DeliverCmd(cmd);
      } else {
        // 0x0000AD78: too far, so the animation is over.
        mAnimationState.SetState(CAnimationState::kAS_Over);
      }
    }
    break;
  }
  case kStateMsg_Deactivate: {
    // 0x0000AD84: the animation state back to "not ready".
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
  default:
    break;
  }
}

// fn_9_AB08: on the activate message (the guest's test of 0 at 0x0000AB10) the commando takes the
// player as its target - but only when the target it had is gone (the object lookup at
// 0x0000AB40..0x0000AB48). It then points the bone tracking at that target (0x0000AB74 and
// 0x0000AB88), tells the script that the attack connection changed (0x0000ABB4, the message is
// the host's kSM_None, the guest's -1) and switches to the combat locomotion (0x0000ABC0).
void CCommandoPirate::SelectTarget(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {
    return;
  }
  if (mgr.GetObjectById(xbca_) != nullptr) {
    return;
  }
  // 0x0000AB4C: the guest takes the id the state manager holds for the player (its +0x14FC, the
  // id at +8 of it); the host's state manager has no member at that offset, so the port uses the
  // player actor's id, which is what the bone tracking and the script message want.
  const TUniqueId target = mgr.GetPlayer(0)->GetUniqueId(); // mgr+0x14FC, +0x8 at 0x0000AB50
  xbca_ = target;                                    // 0x0000AB5C
  // 0x0000AB68: the guest tells its visual knowledge the new target (guest this+0xAB0). The host
  // has no CVisualKnowledge class yet, so the call is a stand-in.
  StandInFn("CVisualKnowledge::SetVisualTarget (guest this+0xab0)");
  mBoneTracking.SetActive(true);                     // 0x0000AB74
  mBoneTracking.SetTarget(target);                   // 0x0000AB88
  SendScriptMsgs(kSS_Attack, mgr, kInvalidUniqueId, kSM_None); // 0x0000ABB4
  BodyController()->SetLocomotionType(pas::kLT_Combat);   // 0x0000ABC0
}

// fn_9_AA20: the state sets the path find destination to the target. Only the activate message
// (the guest's test of 0 at 0x0000AA28) does anything: the destination starts as the actor's own
// position (guest this+0x54, the three loads at 0x0000AA44..0x0000AA64), the 0x80 bit of the
// module's flag byte at guest this+0xC65 is cleared (the rlwimi at 0x0000AA6C) and the target is
// looked up (the id at guest this+0xBCA, the load at 0x0000AA74, the GetObjectById call at
// 0x0000AA7C). When the target is there and either the module's flag byte at +0xC64 has its 0x40
// bit (extrwi at 0x0000AA8C) or the module's byte at +0xAE8 does not have its 0x80 bit (extrwi at
// 0x0000AA98), the destination becomes the target's position (the loads at
// 0x0000AAA0..0x0000AAB8) and the 0x80 bit of +0xC65 is set (the rlwimi at 0x0000AAC0).
// Then the path find navigation's destination is set (the SetDestination call at 0x0000AAD0) and
// its face target id (guest this+0x718 - the navigation's own member at +0x24) is cleared (the
// store at 0x0000AAE8). The module's own helper fn_9_19D8 (0xc4 bytes, over the 0x80 limit) is
// called last (0x0000AAEC) and stands in for now.
//
// The guest reads the target's position at the actor's +0x54 without a cast; the host's CEntity
// has no position, so the port casts to CActor and keeps a null test of its own (the guest's
// only null test is the lookup's, at 0x0000AA80).
void CCommandoPirate::SetTargetDest(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {
    return;
  }
  CVector3f destination = GetTranslation();  // 0x0000AA44..0x0000AA64
  xc65_ &= ~0x80;                            // 0x0000AA6C
  const CEntity* target = mgr.GetObjectById(xbca_);  // 0x0000AA7C
  const CActor* actor = target != nullptr ? TCastToConstPtr< CActor >(target) : nullptr;
  if (actor != nullptr && ((xc64_ & 0x40) != 0 || (xae8_ & 0x80) == 0)) {  // 0x0000AA88..0x0000AA9C
    destination = actor->GetTranslation();  // 0x0000AAA0..0x0000AAB8
    xc65_ |= 0x80;                          // 0x0000AAC0
  }
  mPathFindNavigation.SetDestination(destination);         // 0x0000AAD0
  mPathFindNavigation.SetFaceTarget(kInvalidUniqueId);     // 0x0000AAE8
  x19D8_JoinTeam(mgr);                                      // 0x0000AAEC
}

void CCommandoPirate::SetRetreatDest(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_A97C");
}

void CCommandoPirate::SetJumpDest(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_A8E8");
}

void CCommandoPirate::SetPathMeshDest(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_A838");
}

void CCommandoPirate::MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_A4BC");
}

void CCommandoPirate::EGrenadeAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_A2EC");
}

void CCommandoPirate::PostEGrenadeAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_A110");
}

void CCommandoPirate::JumpPointFind(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_9F24");
}

void CCommandoPirate::JetBoost(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_9BB4");
}

// fn_9_19D8: the module's team helper, called by the destination states. The commando takes the
// team of its area when it has none yet: the id at guest this+0xBC4 must be kInvalidUniqueId
// (the compare at 0x00001A04), the block's flag byte at +0x15C (guest this+0x91C) must not have
// its 0x02 bit (extrwi at 0x00001A10) and the one at +0x15D (guest this+0x91D) must not have its
// 0x40 bit (extrwi at 0x00001A1C). The team id comes from the team AI manager (the call at
// 0x00001A30) and is stored (0x00001A3C); then that manager is looked up (the ObjectById call at
// 0x00001A60) and the commando joins it with the melee role (the JoinTeam call at 0x00001A80,
// the roles 1, -1 and -1 - the host's kTAR_Melee, kTAR_Invalid and kTAR_Invalid).
void CCommandoPirate::x19D8_JoinTeam(CStateManager& mgr) {
  if (xbc4_teamId != kInvalidUniqueId) {
    return;                                   // 0x00001A04
  }
  if ((mData.mFlags0 & 0x02) != 0) {
    return;                                   // 0x00001A10
  }
  if ((mData.mFlags1 & 0x40) != 0) {
    return;                                   // 0x00001A1C
  }
  xbc4_teamId = CScriptTeamAiMgr::GetAssociatedTeamId(*this, mgr);  // 0x00001A30, 0x00001A3C
  if (xbc4_teamId == kInvalidUniqueId) {
    return;                                   // 0x00001A48
  }
  if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(xbc4_teamId))) {
    team->JoinTeam(*this, CTeamAiRole::kTAR_Melee, CTeamAiRole::kTAR_Invalid,
                    CTeamAiRole::kTAR_Invalid);  // 0x00001A80
  }
}

// fn_9_9AAC: the state path-finds. On activate (the guest's message 0, 0x00009AEC) the body
// controller's field at +0x4C is set to 1 (the store at 0x00009BE4) - that field is the command
// manager's steering blend mode (the cmd mgr sits at body controller + 4 and its six CVector3f
// put mSteeringMode at +0x48) - the steering speed range is set to 1.0f twice (the
// SetSteeringSpeedRange call at 0x00009BF4, lbl_9_rodata_43C is 1.f) and the 0x80 bit of the
// module's flag byte at guest this+0xC63 is cleared (the rlwimi at 0x00009C00 keeps the bit at
// position 24, the mask 1u << (31 - 24) = 0x80). On deactivate (the guest's message 2,
// 0x00009AF4) that field is set to 0 (0x00009C18) and the 0x80 bit of the byte at +0xC65 is set
// (the rlwimi at 0x00009C20).
// Then, while the block's flag byte at +0x15C does not have its 0x08 bit (extrwi. r0, r0, 1, 28 at
// 0x00009B58) and the byte at +0xC65 has its 0x80 bit (extrwi. r0, r0, 1, 24 at 0x00009B64), the
// path find navigation runs (the call at 0x00009B80) and the module's own helper follows.
void CCommandoPirate::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    // 0x00009BE4: the guest writes 1 into the body controller's field at +0x4C. The host's
    // enumerator with the value 1 is kSBM_FullSpeed.
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed);  // 0x00009BE4
    BodyController()->CommandMgr().SetSteeringSpeedRange(1.f, 1.f);        // 0x00009BF4
    xc63_ &= ~0x80;                                    // 0x00009C00
    break;
  }
  case kStateMsg_Deactivate: {
    // 0x00009C18: the same field, cleared - the host's enumerator with the value 0 is
    // kSBM_Normal.
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_Normal);  // 0x00009C18
    xc65_ |= 0x80;                                     // 0x00009C20
    break;
  }
  default:
    break;
  }
  if ((mData.mFlags0 & 0x08) == 0 && (xc65_ & 0x80) != 0) {
    mPathFindNavigation.PathFind(mgr, msg, dt, *this);  // 0x00009B80
    // 0x00009B8C: the module's own helper, fn_9_1A9C.
    x1A9C_AvoidActors(mgr);                             // 0x00009B8C
  }
}

// fn_9_1A9C (0x170 bytes): the module's own helper, called by the PathFind state after the path
// find navigation has run. It walks the listening-AI object list (the state manager's +0x830, the
// load at 0x00001ABC; the list's first index is the word at +0x2008, the load at 0x00001AC8) and
// for every other CPatterned of the same area (the entities' +0x4 area id, compared at
// 0x00001AF0..0x00001AFC; the null and the self tests are at 0x00001AE0 and 0x00001AE8) asks the
// steering behaviors for the separation from that actor (the behaviors at this+0x61c, the other
// actor's position at its +0x54; the call at 0x00001B3C). The distance is 5.0f times the model's
// X scale - the model data sits at this+0x60 (the load at 0x00001B00) and its scale's first
// float is the x, so the multiply is 5.f (lbl_9_rodata_468) times it (the fmuls at 0x00001B2C).
// A separation that is magnitude-safe (the test at 0x00001B5C) becomes a locomotion command: the
// normalized separation as the move vector, a zero look direction and the weight 0.5f
// (lbl_9_rodata_440) - the command's fields are laid out at 0x00001BA0..0x00001BB8 and delivered
// at 0x00001BC4.
void CCommandoPirate::x1A9C_AvoidActors(CStateManager& mgr) {
  CObjectList& list = mgr.ObjectListById(kOL_ListeningAi);  // 0x00001ABC
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    CPatterned* other = TCastToPtr< CPatterned >(list[i]);  // 0x00001AD0..0x00001ADC
    if (other == nullptr || other == this ||
        other->GetCurrentAreaId() != GetCurrentAreaId()) {  // 0x00001AE0..0x00001AFC
      continue;
    }
    const float separationDistance =
        5.f * GetModelData()->GetScale().GetX();  // 0x00001B00..0x00001B2C
    const CVector3f separation =
        mSteeringBehaviors.Separation(*this, other->GetTranslation(), separationDistance);
    if (!separation.IsMagnitudeSafe()) {  // 0x00001B5C
      continue;
    }
    BodyController()->CommandMgr().DeliverCmd(
        CBCLocomotionCmd(separation.AsNormalized(), CVector3f::Zero(), 0.5f));  // 0x00001BC4
  }
}

// fn_9_996C: the message picks the locomotion the patrol uses and the waypoint navigation then
// patrols (the call at 0x00009A78). The guest's own enum has more messages than the host's, so
// the two the body tests are the host's kStateMsg_Activate (0) and kStateMsg_Deactivate (2);
// every other message skips the setup (0x00009A9C0..0x00009A9C8 and 0x00009A9BC).
void CCommandoPirate::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    // 0x000099D4: the relaxed locomotion; 0x000099EC: half the turn speed (lbl_9_rodata_440 is
    // 0.5f); 0x00009A10: the ratio of the two locomotion speeds (the walk's over the run's);
    // 0x00009A2C: the steering speed range, that ratio twice.
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    BodyController()->SetTurnSpeed(0.5f * BodyController()->GetTurnSpeed());
    const float ratio = BodyController()->BodyStateInfo().GetLocomotionSpeed(pas::kLA_Walk) /
                         BodyController()->BodyStateInfo().GetLocomotionSpeed(pas::kLA_Run);
    // 0x00009A1C: the guest's body controller takes a flag at +0x4C, one here and zero in the
    // other branch (0x00009B60). That field is the command manager's steering blend mode (the
    // cmd mgr sits at body controller + 4 and its six CVector3f put mSteeringMode at +0x48), so
    // the host's enumerator with the value 1 is kSBM_FullSpeed.
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed);  // 0x00009A1C
    BodyController()->CommandMgr().SetSteeringSpeedRange(ratio, ratio);
    break;
  }
  case kStateMsg_Deactivate: {
    // 0x00009A3C: the combat locomotion; 0x00009A50: twice the turn speed (lbl_9_rodata_44C is
    // 2.f); 0x00009B60: the same flag at +0x4C, cleared - the host's value 0 is kSBM_Normal.
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    BodyController()->SetTurnSpeed(2.f * BodyController()->GetTurnSpeed());
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_Normal);  // 0x00009B60
    break;
  }
  default:
    break;
  }
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
}

void CCommandoPirate::FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_961C");
}

void CCommandoPirate::WarpIn(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_9064");
}

void CCommandoPirate::WarpOut(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_8DF0");
}

void CCommandoPirate::PostWarpOut(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_8D28");
}

void CCommandoPirate::JumpBack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_8B70");
}

// fn_9_8934 (0x1e0 bytes): the Dodge state, in the guest's three parts.
//
// Activate (the guest's message 0, the test at 0x0000895C): the module's helper fn_9_3678
// (0x288 bytes, over the 0x80 limit, so a named helper that returns the neutral value) picks
// the dodge, the result goes into the module's field at guest this+0xBDC (the store at
// 0x00008978) and, when it is not -1 (the compare at 0x00008980), the animation state takes the
// command id 1 (the store at 0x0000898C).
//
// Update (the guest's message 1, the test at 0x0000893C): the animation state is asked whether
// the dodge command may be issued (the guest's command id 3 - the call at 0x000089A0). When it
// may, the dodge command is delivered (the DeliverCmd call at 0x00008A10): the command's fields
// are the direction - the dodge id at guest this+0xBDC (the load at 0x000089BC) - and the type 2
// (kStep_BreakDodge), with a zero target position. When the command may
// not be issued, and the body controller's body state is 3 (the load at 0x00008A6C, the compare
// at 0x00008A70), the module's helper fn_9_3CBC (the call at 0x00008A90) gives the target's aim
// position and the direction to it (the module's fields at guest this+0xB94..0xB9C) goes into
// the body controller's vector at +0x34..+0x3C (the stores at 0x00008AC4..0x00008ACC).
//
// Deactivate (the guest's message 2, the test at 0x00008968): the animation state goes back to 0
// (the store at 0x00008ADC), the dodge id at guest this+0xBDC goes back to -1 (0x00008AE8), the
// module's flag byte at +0xC63 loses its 0x04 bit (the rlwimi at 0x00008AF0, the mask
// 1u << (31 - 29) = 0x04) and the timer at guest this+0xB68 is cleared (0x00008AF8).
void CCommandoPirate::Dodge(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    StandInFn("fn_9_3678 (0x288 bytes)");            // 0x00008974
    xbdc_ = -1;                                       // 0x00008978 (the helper's neutral value)
    if (xbdc_ != -1) {                               // 0x00008980
      mAnimationState.SetState(CAnimationState::kAS_Ready);  // 0x0000898C
    }
    break;
  }
  case kStateMsg_Update: {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {  // 0x000089A0
      BodyController()->CommandMgr().DeliverCmd(
          CBCStepCmd(static_cast< pas::EStepDirection >(xbdc_), pas::kStep_BreakDodge));  // 0x00008A10
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_Step) {  // 0x00008A6C
      CVector3f aim;
      if (x3CBC_(mgr, aim, 0.f)) {                     // 0x00008A90
        const CVector3f delta = aim - GetTranslation();
        BodyController()->CommandMgr().SetTargetVector(delta);  // 0x00008AC4..0x00008ACC
      }
    }
    break;
  }
  case kStateMsg_Deactivate: {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);  // 0x00008ADC
    xbdc_ = -1;                                        // 0x00008AE8
    xc63_ &= ~0x04;                                    // 0x00008AF0
    xb68_ = 0.f;                                       // 0x00008AF8
    break;
  }
  default:
    break;
  }
}

void CCommandoPirate::ArmShield(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_86B4");
}

void CCommandoPirate::ShieldCharge(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_8028");
}

void CCommandoPirate::ScriptedShieldCharge(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_7A98");
}

void CCommandoPirate::RestoreOrientation(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_786C");
}

void CCommandoPirate::Crouch(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_7724");
}

void CCommandoPirate::WallHang(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_7358");
}

void CCommandoPirate::WallDetach(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_728C");
}

void CCommandoPirate::CoverFind(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_709C");
}

void CCommandoPirate::SetCoverDest(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_A7A8");
}

void CCommandoPirate::Cover(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_6DBC");
}

void CCommandoPirate::CoverAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_6C68");
}

void CCommandoPirate::BreakCover(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_6BCC");
}

void CCommandoPirate::GetUp(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_6A40");
}

void CCommandoPirate::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_69C8");
}

// vtable slot 60 = fn_9_BDE0 (0xBDE0..0xBEB4): the base's setup first (the call at 0x0000BDF4),
// then the module's two tables on the state manager - the triggers (lbl_9_data_198, 34 = 0x22)
// through the vtable slot at +0x18 (the call at 0x0000BE14) and the states (lbl_9_data_568,
// 36 = 0x24) through +0x14 (the call at 0x0000BE34). The module has no third table: the
// records between the two tables' stores run out at the state's last entry.
void CCommandoPirate::SetupStateMachine(CStateManager& mgr) {
  CPatterned::SetupStateMachine(mgr);
  StateMachine* stateMachine = mStateMachine.get();
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

// vtable slot 0xf4 = fn_9_78 (0x8): `li r3, 0x0` - this actor has no rag doll.
CRagDoll* CCommandoPirate::GetRagDoll() const { return nullptr; }

// vtable slot 0x10c = fn_9_80 (0x10): stores kInvalidUniqueId, so the actor never reports an
// attack target through this accessor.
TUniqueId CCommandoPirate::GetAttackTarget() const { return kInvalidUniqueId; }

// vtable slot 0x114 = fn_9_10 (0xc): the module's own rodata_400, the float 50.
float CCommandoPirate::GetGravityConstant() const { return 50.f; }

// ---------------------------------------------------------------------------------------------
// The module's loader (fn_9_168) and entry points.

// fn_9_168. The property ids and their order are the generated SLdrCommandoPirate reader's, which
// the REL's own switch matches id by id (fn_9_168 @0x000001C8..0x00000568).
static CEntity* PortLoadCommandoPirate(CStateManager& mgr, CInputStream& input,
                                        CEntityInfo& info) {
  SLdrCommandoPirate sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrCommandoPirate.inc"

  rstl::optional_object< CModelData > modelData(LdrToModelData(
      sldrThis.editorProperties.transform.scale, kInvalidAssetId,
      sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL checks the copy's valid byte (sp+0x2d0 at 0x000006B0) and returns null when the
    // model is empty.
    return nullptr;
  }

  // The two damage infos the loader converts (fn_9_168 @0x000005E0 and @0x00000738): the blade's
  // and the projectile's.
  const CDamageInfo bladeDamage = LdrToDamageInfo(sldrThis.bladeDamage);
  const CDamageInfo projectileDamage = LdrToDamageInfo(sldrThis.projectileDamage);

  // The E-grenade half (fn_9_98C then fn_9_F50C, out of the 0xfb435257 property block).
  const SLdrUnknownStruct10& grenade = sldrThis.unknown_0xfb435257;
  const CCommandoPirateGrenadeData grenadeData(
      grenade.grenadeMass, grenade.unknown_0xed086ce0, bladeDamage, grenade.grenadeNumBounces,
      grenade.grenadeExplosion, grenade.grenadeExplosion, grenade.grenadeTrail,
      grenade.grenadeEffect, static_cast< ushort >(grenade.sound_GrenadeBounce),
      grenade.eMPDuration, grenade.grenadeAttackChance, grenade.grenadeMinAttackInterval,
      grenade.grenadePostAttackPause, grenade.grenadeMinAttackDist, grenade.grenadeMaxAttackDist,
      grenade.grenadeMinLaunchSpeed, grenade.grenadeMaxLaunchSpeed);

  // The shield half (fn_9_EF80, out of the 0x388e16c9 property block read by fn_9_1308).
  const SLdrCommandoShield& shield = sldrThis.shieldInfo;
  const CCommandoPirateShieldData shieldData(
      LdrToDamageInfo(shield.shieldChargeDamage),
      LdrToDamageVulnerability(shield.shieldVulnerability), shield.shieldChargeMinAttackDist,
      shield.shieldChargeMaxAttackDist, shield.shieldChargeSpeed, shield.shieldExplodeEffect,
      static_cast< ushort >(shield.sound_ShieldExplode), shield.unknown_0x6cb0da5a,
      shield.unknown_0xc3938663, shield.armShieldChance, shield.armShieldTime,
      shield.armShieldTimeVariation, shield.armShieldExplodeEffect, shield.shieldChargeEffect,
      shield.armShieldEffect, static_cast< ushort >(shield.sound_ShieldTurnOn),
      static_cast< ushort >(shield.sound_ShieldTurnOff));

  // The block's last two bytes (fn_9_EDC0 @0x0000EED4..0x0000EF60) hold eight and four bits of
  // the "sound" property, taken in the guest's order: 0x80 <- bit 14, 0x40 <- bit 11, 0x20 <- bit
  // 8, 0x10 <- bit 5, 0x08 <- bit 2, 0x04 <- bit 31, 0x02 <- bit 28, 0x01 <- bit 25; the second
  // byte 0x80 <- bit 6, 0x40 <- bit 3, 0x20 <- bit 0, 0x10 <- bit 29.
  const uint sound = static_cast< uint >(sldrThis.sound);
  const uchar flags0 = static_cast< uchar >(
      (((sound >> 14) & 1) << 7) | (((sound >> 11) & 1) << 6) | (((sound >> 8) & 1) << 5) |
      (((sound >> 5) & 1) << 4) | (((sound >> 2) & 1) << 3) | (((sound >> 31) & 1) << 2) |
      (((sound >> 28) & 1) << 1) | ((sound >> 25) & 1));
  const uchar flags1 = static_cast< uchar >((((sound >> 6) & 1) << 7) | (((sound >> 3) & 1) << 6) |
                                             ((sound & 1) << 5) | (((sound >> 29) & 1) << 4));

  const CCommandoPirateData data(
      static_cast< ushort >(sldrThis.sound_Impact), static_cast< ushort >(sldrThis.sound_Hurled),
      static_cast< ushort >(sldrThis.sound_Death), static_cast< ushort >(sldrThis.alwaysFF),
      sldrThis.alwaysFF_0x467c3d94, bladeDamage, sldrThis.projectile, projectileDamage,
      static_cast< ushort >(sldrThis.sound_Projectile), sldrThis.aggressiveness,
      sldrThis.coverCheck, sldrThis.searchRadius, sldrThis.dodgeCheck, sldrThis.hearingRadius,
      sldrThis.intraBurstShotTime, sldrThis.intraBurstShotVariation, grenadeData, shieldData,
      flags0, flags1);

  return rs_new CCommandoPirate(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                LdrToEntityInfo(info, sldrThis.editorProperties),
                                LdrToTransform4f(sldrThis.editorProperties), *modelData,
                                LdrToActorParameters(sldrThis.actorInformation),
                                LdrToPatternedInfo(sldrThis.patterned, &sldrThis.ingPossessionData),
                                data);
}

static SCommandoPirate_FuncPtrs REL_loader_CommandoPirate;

// Retail's RELMain calls fn_9_138, which stores the loader fn_9_168 into lbl_9_bss_28 (the
// module's SCommandoPirate_FuncPtrs) and calls the DOL's SetSCommandoPirate_FuncPtrs. The proxy in
// SCommandoPirate_FuncPtrs.cpp calls through it.
void RELMain() {
  REL_loader_CommandoPirate.mLoader = PortLoadCommandoPirate;
  SetSCommandoPirate_FuncPtrs(&REL_loader_CommandoPirate);
}

void RELExit() { SetSCommandoPirate_FuncPtrs(nullptr); }
