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

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrCommandoPirate.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

#include <stdio.h>
#include "rstl/string.hpp"

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
, xb5c_(0.f)                                      // 0xb5c, fn_9_D904 @0x0000DABC
, xb60_(1.5f)                                     // 0xb60, lbl_9_rodata_47C, fn_9_D904 @0x0000DACC
, xb64_(0.75f)                                    // 0xb64, lbl_9_rodata_4E0, fn_9_D904 @0x0000DAD8
, xb8c_(0.f)                                      // 0xb8c, fn_9_D904 @0x0000DB08
, xb90_(0.f)                                      // 0xb90, fn_9_D904 @0x0000DB0C
, xb94_(CVector3f::Zero())                         // 0xb94, fn_9_D904 @0x0000DB14
, xbca_(kInvalidUniqueId)                          // 0xbca, fn_9_D904 @0x0000DBA8
, xbcc_(kInvalidUniqueId)                          // 0xbcc, fn_9_D904 @0x0000DBB0
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

// vtable slot 0x10 = fn_9_CAC8 (0x78).
void CCommandoPirate::PreThink(float dt, CStateManager& mgr) {
  StandIn("CCommandoPirate::PreThink (vtable slot 0x10, fn_9_CAC8)");
  EnsureBodyActive(mgr);
  CPatterned::PreThink(dt, mgr);
}

// vtable slot 0x14 = fn_9_C944 (0x184).
void CCommandoPirate::Think(float dt, CStateManager& mgr) {
  StandIn("CCommandoPirate::Think (vtable slot 0x14, fn_9_C944)");
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
}

// vtable slot 0x18 = fn_9_CB40 (0x754): the message switch that activates the body and starts the
// module's state machine.
void CCommandoPirate::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CCommandoPirate::AcceptScriptMsg (vtable slot 0x18, fn_9_CB40)");
  CPatterned::AcceptScriptMsg(mgr, msg);
  if (msg.GetMessage() == kSM_Create) {
    EnsureBodyActive(mgr);
  }
}

// vtable slot 0x24 = fn_9_C6D4 (0xc4).
void CCommandoPirate::PreRender(CStateManager& mgr) {
  StandIn("CCommandoPirate::PreRender (vtable slot 0x24, fn_9_C6D4)");
  CPatterned::PreRender(mgr);
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

// vtable slot 0x34 = fn_9_C51C (0x7c).
void CCommandoPirate::PreRenderAllViewports(CStateManager& mgr) {
  StandIn("CCommandoPirate::PreRenderAllViewports (vtable slot 0x34, fn_9_C51C)");
  CPatterned::PreRenderAllViewports(mgr);
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

bool CCommandoPirate::ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_BCB8");
  return false;
}

bool CCommandoPirate::ShouldFireEGrenade(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_BBC8");
  return false;
}

bool CCommandoPirate::ShouldRetreat(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_BB7C");
  return false;
}

bool CCommandoPirate::ShouldJumpBack(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_BB10");
  return false;
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

bool CCommandoPirate::PathShagged(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_B9D8");
  return false;
}

bool CCommandoPirate::PathOver(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_B990");
  return false;
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

bool CCommandoPirate::IsFacingTarget(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_B818");
  return false;
}

// fn_9_B7FC: the base's waypoint navigation destination (guest this+0x6C0, the u16 loaded at
// 0x0000B800) is kInvalidUniqueId. The guest's test is the cntlzw idiom - subf r0, r5, r0
// (0x0000B808), cntlzw (0x0000B80C) and srwi r3, r0, 5 (0x0000B810) - one iff the id is 0xFFFF.
// The host's base carries the same role in its waypoint navigation, as the Grenchler's own
// AttackPatternOver does.
bool CCommandoPirate::AttackPatternOver(CStateManager& mgr, const CTriggerData& data) const {
  return mWaypointNavigation.GetDestination() == kInvalidUniqueId;
}

bool CCommandoPirate::TooClose(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_B76C");
  return false;
}

bool CCommandoPirate::ShouldDodge(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_B690");
  return false;
}

bool CCommandoPirate::ShouldArmShield(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_9_B660");
  return false;
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

void CCommandoPirate::Alert(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_AE04");
}

// fn_9_3CBC: the target's aim position. The guest looks the target up by the id at this+0xBCA
// (the load at 0x00003CD8) and, when it exists, asks it for its aim position - the vtable call
// at 0x00003D1C, CActor's slot 0x54, GetAimPosition - with the state manager and the dt, and
// copies the answer into the out vector (0x00003D28..0x00003D38). It returns false when the
// target is gone (0x00003D40).
bool CCommandoPirate::x3CBC_(CStateManager& mgr, CVector3f& out, float dt) {
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

void CCommandoPirate::SetTargetDest(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_AA20");
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

void CCommandoPirate::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_9AAC");
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
    BodyController()->CommandMgr().SetSteeringSpeedRange(ratio, ratio);
    // 0x00009A1C: the guest's body controller takes a flag at +0x4C, one here and zero in the
    // other branch (0x00009A60); the host's body controller has no such member.
    break;
  }
  case kStateMsg_Deactivate: {
    // 0x00009A3C: the combat locomotion; 0x00009A50: twice the turn speed (lbl_9_rodata_44C is
    // 2.f); 0x00009A60: the same flag at +0x4C, cleared.
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    BodyController()->SetTurnSpeed(2.f * BodyController()->GetTurnSpeed());
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

void CCommandoPirate::Dodge(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_9_8934");
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
