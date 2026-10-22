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

#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Animation/CPASAnimParm.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CPASDatabase.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"
#include "Kyoto/Math/CAABox.hpp"
#include "Collision/CCollisionInfoList.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Collision/CMaterialList.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyState.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CValidEntityPredicate.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Collision/CJointCollisionDescription.hpp"
#include "MetroidPrime/Enemies/CTeamAiRole.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAiJumpPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrCommandoPirate.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
#include "MetroidPrime/Weapons/CWeapon.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWater.hpp"
#include "Kyoto/SObjectTag.hpp"

#include <stdio.h>
#include <string.h>
#include "rstl/string.hpp"

namespace {

// The module's bursts: the REL's data table lbl_9_data_7A8 (.data 0x7A8) points at four SBurst
// entries in the module's .rodata - 0x0, 0x134, 0x1E4 and 0x318 - and its fifth entry is null.
// The entries are the retail's SBurst (the weight, the eight shot angles, the time to the next
// shot and its variation); the angles are the 22.5 degree steps and the -1 ends the burst.
static const SBurst sBursts[] = {
    {15, {6, 5, 3, 2, 1, -1, 0, 0}, 0.1f, 0.05f},
    {20, {16, 4, 8, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {26, {16, 5, 8, 11, 14, -1, 0, 0}, 0.1f, 0.05f},
    {40, {7, 9, 13, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
};
static const SBurst* sBurstTable[] = {&sBursts[0], &sBursts[1], &sBursts[2], &sBursts[3], nullptr};

// The module's global at bss_38 counts the live "Commando E-Grenade" script objects: the module's
// object constructor raises it (fn_9_F418 @0x0000F4E8) and its destructor lowers it (fn_9_F290
// @0x0000F2D0); PortCCommandoPirateCore.cpp's E-grenade does the same, so it sits outside the
// anonymous namespace.
}  // namespace
int sEgrenadeObjectCount = 0;
namespace {

// The module's own predicate for the connected-object search in its area-loaded case: the
// module's vtable lbl_9_data_A84 (.data 0xA84) replaces the DOL's CValidEntityPredicate vtable
// with its own two entries - the destructor (fn_9_2CAC) and fn_9_F6DC, which accepts only an
// object that casts to a CScriptWaypoint.
class CCommandoPirateWaypointPredicate : public CValidEntityPredicate {
public:
  ~CCommandoPirateWaypointPredicate() override {}
  bool IsValid(const CStateManager& mgr, TUniqueId id) const override {
    return TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(id)) != nullptr;
  }
};

static const CCommandoPirateWaypointPredicate skWaypointPredicate;

// The module's own collision filter (the guest's lbl_9_bss_20, .bss 0x20, the two words of one
// u64) and the flag that says it is built (the guest's lbl_9_bss_1C, .bss 0x1C). fn_9_BF44 fills
// the filter with the two material bits kMT_Ceiling and kMT_Wall on the first collision that has
// no object id (0x0000C060..0x0000C0E8) and only then sets the flag.
static u64 sShieldFilter = 0;
static bool sShieldFilterBuilt = false;

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
, mCollisionShield()                            // 0xa3c, fn_9_D904 @0x0000D9D0
, mCollisionBlade()                             // 0xa40, fn_9_D904 @0x0000D9D8
, mShieldVulnerability(data.mShield.mChargeVulnerability) // 0xa44, fn_9_D904 @0x0000D9D4
, mBoneTracking(*AnimationData(), rstl::string_l("Head_1"), 1.2217305f, 3.1415927f,
                kBTF_None)                        // 0xa74, fn_9_D904 @0x0000DA08
, xae8_(0)                                        // 0xae8, the module's byte
, mBurstFire(sBurstTable, 0)                      // 0xaf0, fn_9_D904 @0x0000DA78
, xb7c_(0.f)                                      // 0xb7c, the melee cooldown; fn_9_662C advances it
, xb5c_(0.f)                                      // 0xb5c, fn_9_D904 @0x0000DABC
, xb50_(0)                                        // 0xb50, fn_9_D904 @0x0000DA84
, xb54_(0)                                        // 0xb54, fn_9_D904 @0x0000DA94
, xb58_(0)                                        // 0xb58, fn_9_D904 @0x0000DAA4
, xb60_(1.5f)                                     // 0xb60, lbl_9_rodata_47C, fn_9_D904 @0x0000DACC
, xb64_(0.75f)                                    // 0xb64, lbl_9_rodata_4E0, fn_9_D904 @0x0000DAD8
, xb68_(0.f)                                      // 0xb68, a timer; fn_9_662C advances it
, xb6c_(0.f)                                      // 0xb6c, a timer; fn_9_662C advances it
, xb70_(0.f)                                      // 0xb70, a timer; fn_9_662C advances it
, xb74_(0.f)                                      // 0xb74, a timer; fn_9_662C advances it
, xb78_(0.f)                                      // 0xb78, a timer; fn_9_662C advances it
, xb84_(0.f)                                      // 0xb84, a timer; fn_9_662C advances it
, xb88_(0.f)                                      // 0xb88, the time to the waypoint
, xb8c_(0.f)                                      // 0xb8c, fn_9_D904 @0x0000DB08
, xb90_(0.f)                                      // 0xb90, fn_9_D904 @0x0000DB0C
, xb94_(CVector3f::Zero())                         // 0xb94, fn_9_D904 @0x0000DB14
, xbb8_(CVector3f::Zero())                         // 0xbb8, the fourth vector; fn_9_D904 @0x0000DB5C
, xba0_(CVector3f::Zero())                         // 0xba0, the charge direction
, xbac_(CVector3f::Zero())                         // 0xbac, the waypoint's forward
, xc4c_(-1)                                       // 0xc4c, the module's own state word; -1 = unset
, xc44_(0.f)                                      // 0xc44, the module's own scale
, xc48_(0.f)                                      // 0xc48, the scale's target
, xc50_(0.f)                                      // 0xc50, a counter; fn_9_662C lowers it
, xc00_(0.f)                                      // 0xc00, a float; fn_9_3090 sets 0.01f
, xbca_(kInvalidUniqueId)                          // 0xbca, fn_9_D904 @0x0000DBA8
, xbc4_teamId(kInvalidUniqueId)                    // 0xbc4, the team id; fn_9_19D8 sets it
, xbc6_(kInvalidUniqueId)                          // 0xbc6, the shield collision actor's id
, xbc8_(kInvalidUniqueId)                          // 0xbc8, the blade collision actor's id
, xbcc_(kInvalidUniqueId)                          // 0xbcc, fn_9_D904 @0x0000DBB0
, xbce_(kInvalidUniqueId)                          // 0xbce, fn_9_D904 @0x0000DBB8
, xbdc_(-1)                                    // 0xbdc, the dodge id; fn_9_8934 sets it
, xbe0_(0.f)                                    // 0xbe0, the jump-back distance
, xbe4_(0.f)                                    // 0xbe4, a distance
, xbe8_(0.f)                                    // 0xbe8, the jump distance
, xbd0_(kInvalidUniqueId)                          // 0xbd0, fn_9_D904 @0x0000DBC0
, xbd2_(kInvalidUniqueId)                          // 0xbd2, fn_9_D904 @0x0000DBC8
, xbd4_(kInvalidUniqueId)                          // 0xbd4, the cover point the actor holds
, xbd6_(kInvalidUniqueId)                          // 0xbd6, the waypoint the WallHang state finds
, xbd8_(kInvalidUniqueId)                          // 0xbd8, the id the delete case deletes
, xbda_(kInvalidUniqueId)                          // 0xbda, the waypoint the commando is at
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
, xc54_(-1)                                       // 0xc54, fn_9_D904 @0x0000E0CC
, xc3c_armEffect(xF58C_CreateEffect(data.mShield.mArmEffect))        // 0xc3c, fn_9_D904 @0x0000E094
, xc40_chargeEffect(xF58C_CreateEffect(data.mShield.mChargeEffect))  // 0xc40, fn_9_D904 @0x0000E0A4
{
  // The retail constructor sets knock-back physics on CPatterned's own manager (@0x0000E264) to
  // !((byte at +0x91c >> 2) & 1) - that is, to the manager's own default while the flag bit is
  // clear - and sets the base's flag byte at +0x681. The rest - the CVisualKnowledge, the six
  // locked tokens, the burst fire and the three animation distances
  // - belongs to the state machine and the render paths this milestone does not enter; the
  // analysis note lists them.
  KnockBackController().EnableKnockBackPhysics(true);

  // The burst fire's own type starts at 0 (fn_9_D904 @0x0000E420), not at the -1 its constructor
  // leaves: the module's burst-fire helper fn_9_6390 gives up when the type is -1 (0x00006468).
  mBurstFire.SetBurstType(0);

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
    // The jump distance (0x0000E4D4): the jump animation's travel distance (the state id 13 =
    // kAS_Jump with the parms (0, 0, 0), the parm data fn_9_5D74 builds at 0x0000E49C) times the
    // model's scale Y (the model's scale's second float, the load at 0x0000E4B0).
    const CPASAnimParmData jumpParm(pas::kAS_Jump, CPASAnimParm::FromEnum(0),
                                    CPASAnimParm::FromEnum(0), CPASAnimParm::FromEnum(0));
    xbe8_ = scale.GetY() * GetAnimationDistance(jumpParm);
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
// Run: force2, 05_swamp_timber! (2026-10-21): the body has no arm of its own (the rag-doll arm is dead, above).
// @ fn_9_CAC8
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
// Run: cov 2026-10-21 (core drivers, 59 + swamp; force2/force3): every guest arm;
// the hit flag / counter / shield data forced. Open: 0x0000673C's host ternary (the guest is
// branchless).
// @ fn_9_662C
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
    // 0x000066F4..0x000066FC: when the byte at guest this+0xC64 has its 0x40 bit the retail
    // takes r4 = 0 (0x00006730) and writes the flag as 0 (the rlwimi at 0x0000673C) - the flag
    // is cleared, not left alone.
    const bool roll = (xc64_ & 0x40) == 0 &&
                      mData.mDodgeCheck >= mgr.Random()->Range(0.f, 100.f);  // 0x00006710, 0x00006718
    xc63_ = (xc63_ & ~0x04) | (roll ? 0x04 : 0);  // 0x0000673C
    xb68_ = 0.f;                               // 0x00006748
  }
  xb6c_ += dt;                                 // 0x0000674C
  if (xb6c_ >= 5.f) {                          // 0x00006764
    // 0x00006764..0x0000676C: the same for the cover flag - the retail takes r4 = 0 at
    // 0x000067AC and writes the flag as 0 at 0x000067B8.
    const bool roll = (xc64_ & 0x40) == 0 &&
                      mData.mCoverCheck >= mgr.Random()->Range(0.f, 100.f);  // 0x0000678C, 0x00006794
    xc63_ = (xc63_ & ~0x01) | (roll ? 0x01 : 0);  // 0x000067B8
    xb6c_ = 0.f;                               // 0x000067C4
  }
  xb74_ += dt;                                 // 0x000067C8
  if (sEgrenadeObjectCount > 0) {              // 0x000067D8
    xb74_ = 0.f;                               // 0x000067EC
  } else if (xb74_ >= mData.mGrenade.mGrenadeMinAttackInterval) {  // 0x000067F8
    // 0x000067F8..0x00006804: the same for the grenade flag - the retail takes r4 = 0 at
    // 0x00006844 and writes the flag as 0 at 0x00006850.
    const bool roll = (xc64_ & 0x40) == 0 &&
                      mData.mGrenade.mGrenadeAttackChance >=
                          mgr.Random()->Range(0.f, 100.f);  // 0x0000682C
    xc63_ = (xc63_ & ~0x02) | (roll ? 0x02 : 0);  // 0x00006850
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
    mHitByPlayerProjectile = false;            // the base's 0x420 byte, its 0x80 bit, 0x00006988
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
  // The module's two collision actor managers are updated with the object-space option (the
  // guest's r5 = 0 at 0x0000C9B4 and 0x0000C9C8). The guest loads the pointers and calls without
  // a test; the host's test covers the window before the kSM_Create case has built them (the
  // host limit named here).
  if (mCollisionShield.get() != nullptr) {
    mCollisionShield->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);  // 0x0000C9B8
  }
  if (mCollisionBlade.get() != nullptr) {
    mCollisionBlade->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);  // 0x0000C9CC
  }
  if (!BodyController()->IsFrozen()) {
    mBoneTracking.Think(dt);                   // 0x0000C9E8
  }
  StandIn("CVisualKnowledge::Update (guest this+0xab0)");  // 0x0000C9F8
  x6390_UpdateBurstFire(mgr, dt);                     // 0x0000CA08
  x4D84_UpdateAim(mgr);                     // 0x0000CA18
  x662C_UpdateTimers(mgr, dt);                 // 0x0000CA28
  x3090_Drown(mgr);                             // 0x0000CA34
  x1C0C_UpdateRenderables(mgr, dt);              // 0x0000CA88
  x4BC0_UpdateEmitters();                     // 0x0000CA8C
  x3E1C_UpdateRagDoll(mgr, dt);               // 0x0000CAA4
}

// fn_9_4BC0 (0xe4 bytes): the module's two sound emitters. The handle at guest this+0xC04 is
// updated while it is playing or queued (the IsPlaying and IsQueued calls at 0x00004BEC and
// 0x00004C04); when it is neither, the handle is cleared (the store at 0x00004C4C). The
// emitter's position is the actor's own (this+0x54, the argument at 0x00004C1C) and its direction
// the actor's forward column (this+0x28, +0x38 and +0x48, the loads at
// 0x00004C20..0x00004C34); the volume is 0x7f (0x00004C28). The second emitter, the handle at
// guest this+0xC38, is updated the same way (the call at 0x00004C8C) but is never cleared.
void CCommandoPirate::x4BC0_UpdateEmitters() {
  if (xc04_ != CSfxHandle::NullHandle()) {                      // 0x00004BD4..0x00004BDC
    if (CSfxManager::IsPlaying(xc04_) || CSfxManager::IsQueued(xc04_)) {  // 0x00004BEC..0x00004C0C
      CSfxManager::UpdateEmitter(xc04_, GetTranslation(), GetTransform().GetForward(),
                                 0x7f);                          // 0x00004C40
    } else {
      xc04_.Clear();                                            // 0x00004C48..0x00004C4C
    }
  }
  if (xc38_ != CSfxHandle::NullHandle()) {                      // 0x00004C50..0x00004C58
    CSfxManager::UpdateEmitter(xc38_, GetTranslation(), GetTransform().GetForward(),
                               0x7f);                            // 0x00004C8C
  }
}

// @ fn_9_4D84
// fn_9_4D84 (0x228 bytes): the module's aim helper, one of the Think's own (the call at
// 0x0000CA18). While the actor is alive (the byte at guest this+0x420's 0x40 bit, the extrwi at
// 0x00004DA4 - the host's GetAlive()), the module's own helper fn_9_4CA4 (0xd8 bytes - the gate
// x4CA4_CanAim) has to agree; then the target's aim position
// (the module's helper fn_9_3CBC, the call at 0x00004DFC) is delivered as an additive aim
// command (the guest's command id 0x1b = 27 = kBSC_AdditiveAim, with the 0x02 bit of the
// flag byte at guest this+0xC65 as the command's third field - the DeliverCmd call at
// 0x00004E50), and the direction from the gun locator (guest this+0xC5E, the GetLctrTransform
// call at 0x00004E94) to that position is put into the actor's own frame (the TransposeRotate
// call at 0x00004EE0) and given to the body controller's additive target vector (the body
// controller's +0x40, the command manager's sixth CVector3f - the stores at
// 0x00004F18..0x00004F20). The 0x02 bit of the flag byte negates the local x and y (the fnegs at
// 0x00004EFC..0x00004F00), and a local y that is not above zero gets the forward vector instead
// (the sForwardVector stores at 0x00004F34..0x00004F44).
//
// Without the actor (the flag byte at guest this+0xC64 has its 0x10 bit clear) and with the
// block's flag byte at guest this+0x91C without its 0x10 bit (the extrwi at 0x00004F50 and
// 0x00004F5C), the module delivers an additive idle command instead (the guest's command id
// 0x1a = 26 = kBSC_AdditiveIdle, the DeliverCmd call at 0x00004F84).
void CCommandoPirate::x4D84_UpdateAim(CStateManager& mgr) {
  if (GetAlive()) {                                     // 0x00004DA0..0x00004DA8
    if (!x4CA4_CanAim()) {                             // 0x00004DB8..0x00004DC0
      return;
    }
    CVector3f aim;
    if (!x3CBC_(mgr, aim, 0.f)) {                       // 0x00004DFC..0x00004E04
      return;
    }
    CBCAdditiveAimCmd cmd;                              // 0x00004E50
    cmd.SetAimType((xc65_ & 0x02) != 0 ? 1 : 0);        // the command's third field (0x00004E18)
    BodyController()->CommandMgr().DeliverCmd(cmd);
    const CVector3f locator = GetLctrTransform(mLocatorGun).GetTranslation();  // 0x00004E94
    CVector3f dir = GetTransform().TransposeRotate(aim - locator);  // 0x00004EE0
    if ((xc65_ & 0x02) != 0) {                          // 0x00004EEC..0x00004F00
      dir.SetX(-dir.GetX());
      dir.SetY(-dir.GetY());
    }
    if (dir.GetY() > 0.f) {                             // 0x00004F0C..0x00004F10
      BodyController()->CommandMgr().DeliverAdditiveTargetVector(dir);  // 0x00004F18..0x00004F20
    } else {
      BodyController()->CommandMgr().DeliverAdditiveTargetVector(CVector3f::Forward());  // 0x00004F34
    }
  } else {
    if ((xc64_ & 0x10) == 0) {                          // 0x00004F4C..0x00004F54
      return;
    }
    if ((mData.mFlags0 & 0x10) != 0) {                  // the block's flag byte at guest this+0x91C
      return;                                           // 0x00004F58..0x00004F60
    }
    BodyController()->CommandMgr().DeliverCmd(kBSC_AdditiveIdle);  // 0x00004F84
  }
}

// @ fn_9_4CA4
// fn_9_4CA4 (0xd8 bytes): the module's own gate - may the commando aim now? The body state's
// answer comes first: the body controller (guest this+0x48C) holds its state info at +0x36C, whose
// current state is asked through its vtable slot 0x28 (the vcall at 0x00004CC8) - the slot is
// CBodyState::CanShoot. The guest keeps the low byte of the answer (the clrlwi at 0x00004CD4) and
// gives up when it is zero (the beq at 0x00004CD8). Then the module's own flag bytes: the byte at
// guest this+0xC64 must have its 0x10 bit (the extrwi at 0x00004CE0) and must not have its 0x20
// bit (the extrwi at 0x00004CF4), the byte at guest this+0xC65 must not have its 0x01 bit (the
// clrlwi at 0x00004D00), its 0x04 bit (the extrwi at 0x00004D08) or its 0x08 bit (the extrwi at
// 0x00004D10), and the block's flag byte at guest this+0x91C must not have its 0x10 bit (the
// extrwi at 0x00004CEC). The module's own word at guest this+0xC4C must be -1 (the cmpwi at
// 0x00004D1C) and the melee cooldown at guest this+0xB7C must be above 1.0f (rodata_43C, the
// compare at 0x00004D30). Last the body: it must not be frozen (its byte at guest this+0x48C +
// 0x594, the 0x20 bit, the extrwi at 0x00004D40) and not electrocuting (its float at guest
// this+0x48C + 0x5B8 above 0.0f, the compare at 0x00004D54).
bool CCommandoPirate::x4CA4_CanAim() {
  if (!BodyController()->BodyStateInfo().GetCurrentState()->CanShoot()) {  // 0x00004CB8..0x00004CD8
    return false;
  }
  if ((xc64_ & 0x10) == 0) {                       // 0x00004CDC..0x00004CE4
    return false;
  }
  if ((mData.mFlags0 & 0x10) != 0) {               // 0x00004CE8..0x00004CF0 (guest this+0x91c)
    return false;
  }
  if ((xc64_ & 0x20) != 0) {                       // 0x00004CF4..0x00004CF8
    return false;
  }
  if ((xc65_ & 0x01) != 0) {                       // 0x00004CFC..0x00004D04
    return false;
  }
  if ((xc65_ & 0x04) != 0) {                       // 0x00004D08..0x00004D0C
    return false;
  }
  if ((xc65_ & 0x08) != 0) {                       // 0x00004D10..0x00004D14
    return false;
  }
  if (xc4c_ != -1) {                               // 0x00004D18..0x00004D20
    return false;
  }
  if (xb7c_ <= 1.f) {                              // 0x00004D24..0x00004D34 (rodata_43c)
    return false;
  }
  if (BodyController()->IsFrozen()) {               // 0x00004D38..0x00004D44 (guest +0x594 bit 0x20)
    return false;
  }
  if (BodyController()->IsElectrocuting()) {         // 0x00004D48..0x00004D58 (guest +0x5b8 > 0.f)
    return false;
  }
  return true;                                     // 0x00004D5C
}

// @ fn_9_6390
// fn_9_6390 (0x29c bytes): the module's burst-fire helper, one of the Think's own (the call at
// 0x0000CA08). It is the module's shot timer and its burst fire:
//
// 0x000063C0..0x000063E0: the module's timer at guest this+0xB5C (`xb5c_`) falls by the frame
//   time while it is above zero.
// 0x000063E4..0x000063EC: the flag byte at guest this+0xC64 must not have its 0x80 bit ("armed").
// 0x000063F0..0x000063FC: the counter at guest this+0xB50 (`xb50_`) must be above the one at
//   +0xB58 (`xb58_`) - the guest compares them unsigned.
// 0x00006400..0x00006424: the block's shield arm chance (the block's +0x140 = guest this+0x900)
//   is rolled against a random number in [0, 100] (rodata_4CC): when the chance is at least the
//   roll the flag byte takes its 0x80 bit (0x00006428..0x00006434), else the counter at +0xB58
//   takes the counter at +0xB50 plus one (0x0000643C..0x00006444).
// 0x00006448..0x00006460: while the actor is alive the aim gate fn_9_4CA4 has to agree, else the
//   helper ends.
// 0x00006464..0x0000646C: the burst fire's own type (guest this+0xAF0) must not be -1.
// 0x00006470..0x00006480: the target is looked up (the id at guest this+0xBCA).
// 0x00006484..0x00006494: the actor must be alive and the target must be there.
// 0x00006498..0x000064DC: the module's timer at +0xB5C must have run out (the cror at 0x000064D8).
// 0x000064E0..0x000064FC: unless the flag byte at guest this+0xC65 has its 0x02 bit, the delta to
//   the target dotted with the actor's own up column (guest this+0x28, +0x38 and +0x48) must be
//   above zero - the target must be higher than the commando.
// 0x00006500..0x00006548: the same delta the other way round, dotted with the target's own up
//   column (the target's +0x28, +0x38 and +0x48), must be below zero; then the burst fire's type
//   takes two more (0x0000654C..0x0000655C) when it is below two.
// 0x00006560..0x00006594: the burst fire starts, the module's timer at +0xB5C takes the base's
//   average attack time (GetAverageAttackTime) plus a random number times the base's attack time
//   variation (guest this+0x3E8), and the counter at +0xB50 goes up by one.
// 0x00006598..0x000065A4: the burst fire is updated with the frame time.
// 0x000065A8..0x000065B0: when the burst fire wants to fire (its byte at guest this+0xB04's 0x80
//   bit) the module's own helper fn_9_57A0 (0x370 bytes, over the 0x80 limit - a named
//   stand-in) runs, and the burst fire's own timer (guest this+0xAF8) takes the block's
//   intra-burst shot time (the block's +0x60) plus a random number times the block's
//   intra-burst shot variation (the block's +0x64) around its middle (0x000065C4..0x000065F0),
//   but only when that timer is above zero (0x000065E4..0x000065F4).
void CCommandoPirate::x6390_UpdateBurstFire(CStateManager& mgr, float dt) {
  if (xb5c_ > 0.f) {                                     // 0x000063D4..0x000063E0
    xb5c_ -= dt;
  }
  if ((xc64_ & 0x80) == 0 && xb50_ > xb58_) {             // 0x000063E4..0x000063FC
    if (mData.mShield.mArmChance >= mgr.Random()->Range(0.f, 100.f)) {  // 0x00006408..0x00006424
      xc64_ |= 0x80;                                     // 0x00006428..0x00006434
    } else {
      xb58_ = xb50_ + 1;                                 // 0x0000643C..0x00006444
    }
  }
  if (GetAlive() && !x4CA4_CanAim()) {                   // 0x00006448..0x00006460
    return;
  }
  if (mBurstFire.GetBurstType() == -1) {                 // 0x00006464..0x0000646C
    return;
  }
  const CEntity* target = mgr.GetObjectById(xbca_);      // 0x00006470..0x00006480
  if (GetAlive()) {                                      // 0x00006484..0x0000648C
    // The guest reads the target's position and its frame without a cast; the host's CEntity has
    // neither, so the target is cast to an actor - a port limit the guest does not have, and the
    // one the module's other ported helpers (x3CBC_ and the target triggers) guard the same way.
    const CActor* actor = target != nullptr ? TCastToConstPtr< CActor >(target) : nullptr;
    if (actor != nullptr && xb5c_ <= 0.f) {              // 0x00006490..0x000064DC
      const CVector3f delta = actor->GetTranslation() - GetTranslation();
      bool targetIsHigher = true;
      if ((xc65_ & 0x02) == 0) {                         // 0x000064E0..0x000064E8
        targetIsHigher = CVector3f::Dot(GetTransform().GetColumn(kDY), delta) > 0.f;  // 0x000064EC
      }
      if (targetIsHigher) {                              // 0x000064FC..0x00006500
        const CVector3f targetUp = actor->GetTransform().GetColumn(kDY);
        if (CVector3f::Dot(targetUp, GetTranslation() - actor->GetTranslation()) < 0.f &&
            mBurstFire.GetBurstType() < 2) {             // 0x00006544..0x00006554
          mBurstFire.SetBurstType(mBurstFire.GetBurstType() + 2);  // 0x00006558..0x0000655C
        }
        mBurstFire.Start(mgr);                           // 0x00006560..0x00006568
        xb5c_ = GetAverageAttackTime() + mgr.Random()->Float() * mAttackTimeVariation;  // 0x0000656C
        xb50_++;                                         // 0x0000658C..0x00006594
      }
    }
  }
  mBurstFire.Update(mgr, dt);                            // 0x00006598..0x000065A4
  if (mBurstFire.ShouldFire()) {                         // 0x000065A8..0x000065B0 (guest +0xb04 bit 0x80)
    x57A0_FireShot(mgr, dt);                             // 0x000065C0
    const float shotTime = (mgr.Random()->Float() - 0.5f) * mData.mIntraBurstShotVariation +
                           mData.mIntraBurstShotTime;    // 0x000065C4..0x000065F0
    if (mBurstFire.GetTimeToNextShot() > 0.f) {          // 0x000065E4..0x000065F4
      mBurstFire.SetTimeToNextShot(shotTime);            // 0x000065F8
    }
  }
}

// @ fn_9_57A0
// fn_9_57A0 (0x370 bytes): the module's shot - the burst fire's projectile. The guest's helper
// takes the state manager and the frame time and answers whether it fired.
//
// 0x000057DC..0x000057F8: the gun locator's transform (the guest's locator at guest this+0xC5E) is
//   fetched and copied.
// 0x000057FC..0x00005804: while the actor is alive (the byte at guest this+0x420's 0x40 bit - the
//   host's GetAlive()) the shot is aimed; a dead commando fires straight ahead (0x00005B84).
// 0x00005808..0x00005820: the target (the id at guest this+0xBCA) is looked up; without it the
//   helper gives up (0x00005BD4).
// 0x00005824..0x0000583C: the target's position (the guest's this+0x54..0x5C) is the aim point.
// 0x00005840..0x00005874: when the target is the player and the player's own state word (the guest's
//   player+0x12F8) is 3 - the host's morph ball transition state kMS_Unmorphing - the player's own
//   position is the aim point and the line of sight ray is skipped (the test at 0x00005A2C).
// 0x00005878..0x000058EC: otherwise the projectile's intercept position is predicted (the
//   PredictInterceptPos call at 0x000058E8): the gun's position, the player's aim position, the
//   frame time and gravity.
// 0x00005904..0x0000593C: the delta from the gun to the aim point and its length.
// 0x00005940..0x00005998: the gun's own direction - the right wrist locator (guest this+0xC60)
//   minus the right elbow locator (guest this+0xC61) - and the angle between it and the delta (the
//   GetAngleDiff call at 0x00005998).
// 0x0000599C..0x000059D4: the shot needs the angle to be at most 30 degrees (rodata_4C8), or at most
//   45 degrees (rodata_4C0) when the target is within 6 units (rodata_49C).
// 0x000059D8..0x00005A48: the line of sight filter: the include list is the material bit 27 (the
//   shift 0x3B) and the exclude list the bits 0 and 60 (the shifts 0x20 and 0x14), with the filter
//   type 3 (kFT_IncludeExclude).
// 0x00005A4C..0x00005A80: the ray from the gun to the aim point; when it hits nothing the module's
//   line of sight byte (guest this+0xAE8) loses its 0x80 bit (0x00005B70..0x00005B7C) and the shot
//   is given up.
// 0x00005A84..0x00005B08: the burst fire's own error (GetDistanceCompensatedError with the
//   distance and 6.0f) is rotated by the actor's own frame and added to the aim point; the shot's
//   transform looks from the gun to it (the LookAt call at 0x00005B0C).
// 0x00005B1C..0x00005B68: the projectile is launched (the LaunchProjectile call at 0x00005B58)
//   with the guest's type 6, no attributes, no homing and the scale 1.0f (rodata_43C).
// 0x00005BD4..0x00005D18: after a shot the muzzle flash is played: the additive animation's state
//   id is 0x18 and its one parameter is the enum 2, or 8 when the body is in its locomotion state
//   (the body's own state word, the host's GetCurrentStateId) and that state says it is moving (the
//   body state's vtable slot 0x18, the call at 0x00005C04); the animation is found in the body's
//   animation database (the FindBestAnimation call at 0x00005CAC) and added as an additive
//   animation (the AddAdditiveAnimation call at 0x00005CD8).
// 0x00005CDC..0x00005D18: the shot's sound (the block's +0x48 = guest this+0x808) is emitted at the
//   actor's own position.
//
// The guest builds the player's aim position with its own copy of the player (the vcall at
// 0x000058B4); the port asks the player for its aim position instead (GetAimPosition), the host's
// own accessor for it.
bool CCommandoPirate::x57A0_FireShot(CStateManager& mgr, float dt) {
  bool fired = false;
  const CTransform4f gunXf = GetLctrTransform(mLocatorGun);        // 0x000057E4..0x000057F8
  // The guest's visor effect: fn_9_6054 builds the default CImpactVisorEffect (its three optional
  // members empty) and fn_9_5F3C releases it (the call at 0x00005B64) - the host's
  // CImpactVisorEffect::None().
  const CImpactVisorEffect visorEffect = CImpactVisorEffect::None();  // fn_9_6054, fn_9_5F3C
  if (!GetAlive()) {                                                // 0x000057FC..0x00005804
    // A dead commando fires straight ahead (0x00005B84..0x00005BC0).
    LaunchProjectile(gunXf, mgr, 6, CWeapon::kPA_None, false, visorEffect,
                     CVector3f(1.f, 1.f, 1.f));                     // rodata_43C
    fired = true;                                                  // 0x00005BD0
  } else {
    // The guest's GetObjectById (the call at 0x00005818) hands out a mutable pointer and the cast
    // to the player (the call at 0x00005840) needs one; the host's GetObjectById is the const one.
    CEntity* target = const_cast< CEntity* >(mgr.GetObjectById(xbca_));  // 0x00005808..0x00005820
    if (target == nullptr) {
      return false;                                                // 0x00005BD4
    }
    // The guest reads the target's position without a cast; the host's CEntity has none, so the
    // target is cast to an actor - the port limit the module's other helpers guard the same way.
    CActor* actor = TCastToPtr< CActor >(target);
    if (actor == nullptr) {
      return false;
    }
    CVector3f pos = actor->GetTranslation();                       // 0x00005824..0x0000583C
    bool skipRay = false;                                          // the guest's r30
    if (CPlayer* player = TCastToPtr< CPlayer >(target)) {         // 0x00005840
      if (player->GetMorphballTransitionState() == CPlayer::kMS_Unmorphing) {  // guest +0x12f8 == 3
        skipRay = true;                                            // 0x0000585C (r30 = 1)
        pos = player->GetTranslation();                            // 0x00005858..0x00005870
      } else {
        // The guest asks the module's own projectile info (the vcall at 0x000058C0, the vtable slot
        // 0xDC - the port's own member, guest this+0xA14).
        pos = mProjectileInfo.PredictInterceptPos(                 // 0x000058B8..0x000058EC
            gunXf.GetTranslation(), player->GetAimPosition(mgr, 0.f), *player, true, dt);
      }
    }
    const CVector3f delta = pos - gunXf.GetTranslation();          // 0x00005904..0x00005938
    const float distance = delta.Magnitude();                      // 0x0000593C
    const CVector3f gunDir = GetLctrTransform(mLocatorRWrist).GetTranslation() -
                             GetLctrTransform(mLocatorRElbow).GetTranslation();  // 0x00005940..0x00005994
    const float angle = CVector3f::GetAngleDiff(gunDir, delta);    // 0x00005998
    if (!(angle <= 0.5235988f)) {                                   // rodata_4C8, 0x0000599C..0x000059AC
      if (!(distance <= 6.f)) {                                     // rodata_49C, 0x000059B0..0x000059C0
        return false;                                               // 0x00005BD4
      }
      if (!(angle <= 0.7853982f)) {                                 // rodata_4C0, 0x000059C4..0x000059D4
        return false;                                               // 0x00005BD4
      }
    }
    if (!skipRay) {
      // The guest's mask (the shifts 0x3B, 0x20 and 0x14): the include list is the material 59
      // (kMT_Solid) and the exclude list the materials 32 and 20 (kMT_Player and
      // kMT_ProjectilePassthrough) - the solid materials block the ray, the player does not.
      const CMaterialFilter losFilter = CMaterialFilter::MakeIncludeExclude(
          CMaterialList(kMT_Solid), CMaterialList(kMT_Player, kMT_ProjectilePassthrough));
      if (!mgr.RayCollideWorld(gunXf.GetTranslation(), pos, losFilter, this)) {  // 0x00005A4C
        xae8_ &= ~0x80;                                            // 0x00005B70..0x00005B7C
        return false;                                               // 0x00005BD4
      }
    }
    const CVector3f error =
        GetTransform().Rotate(mBurstFire.GetDistanceCompensatedError(distance, 6.f));  // 0x00005A84..0x00005AAC
    pos += error;                                                  // 0x00005AB0..0x00005AFC
    const CTransform4f shotXf =
        CTransform4f::LookAt(gunXf.GetTranslation(), pos, CVector3f::Up());  // 0x00005B0C
    LaunchProjectile(shotXf, mgr, 6, CWeapon::kPA_None, false, visorEffect,
                     CVector3f(1.f, 1.f, 1.f));                     // 0x00005B58, rodata_43C
    fired = true;                                                  // 0x00005B68
  }
  if (fired) {
    // The muzzle flash: the guest's additive animation (the state id 0x18) with its one parameter
    // (the enum 2, or 8 while the body moves).
    int parm = 2;                                                  // 0x00005C4C
    if (BodyController()->GetCurrentStateId() == pas::kAS_Locomotion &&  // the guest's +0x37c == 5
        // The body state's vtable slot 0x18 (the retail's __vt__10CBodyState: IsMoving).
        BodyController()->BodyStateInfo().GetCurrentState()->IsMoving()) {
      parm = 8;                                                    // 0x00005C10
    }
    // The guest's fn_9_5D74 builds the animation's parameter block: the state id 0x18 and the eight
    // parameters - the one enum above and seven NoParameter ones (the calls at 0x00005C18..0x00005C4C).
    const CPASAnimParmData parms(static_cast< pas::EAnimationState >(0x18),
                                 CPASAnimParm::FromEnum(parm),      // fn_9_5D74
                                 CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                                 CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                                 CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                                 CPASAnimParm::NoParameter());
    const rstl::pair< float, int > best =
        BodyController()->GetPASDatabase().FindBestAnimation(parms, *mgr.Random(), -1);  // 0x00005CAC
    if (best.second != -1) {                                       // 0x00005CB0..0x00005CB8
      AnimationData()->AddAdditiveAnimation(best.second, 1.f, false, true);  // 0x00005CD8
    }
    CSfxManager::AddEmitter(mData.mSoundProjectile, GetTranslation(),  // 0x00005CDC..0x00005D18
                             GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
  }
  return fired;
}

// fn_9_3090 (0x2a8 bytes): the module's drowning. It runs from the module's Think (the call at
// 0x0000CA34) and ends the commando when its head is under water.
//
// 0x000030A8..0x000030B8: while the actor is alive (the byte at guest this+0x420's 0x40 bit - the
//   host's GetAlive()).
// 0x000030C4..0x000030D8: the actor must be in a fluid (the host's InFluidId) and the fluid id must
//   not be the invalid one.
// 0x000030E4..0x00003104: the fluid is looked up (the id from the guest's InFluidId, the call at
//   0x000030F8) and cast to the water script object.
// 0x00003108..0x00003110: the water's own byte at +0x20 must have its 0x80 bit (the extrwi at
//   0x0000310C).
// 0x0000311C..0x0000315C: the water's trigger bounds (the GetTriggerBoundsWR call at 0x0000311C)
//   and the head locator's (guest this+0xC5C) position: when the head is not below the bounds'
//   height the commando does not drown.
// 0x00003160..0x0000316C: the module's own float at guest this+0xC00 takes 0.01f (rodata_4A0).
// 0x00003170..0x00003180: the health info's second float (the guest's +0x04) is cleared.
// 0x00003184..0x000031C8: the actor dies with the script object state 0x5241544C ("RATL", the
//   host's kSS_DeathRattle) and the actor's own forward vector as the death direction.
// 0x000031CC..0x000032E4: the drowning explosion: the guest's effect token is at guest this+0xC28
//   and the guest only builds the explosion when that token is loaded (the byte at +0xC34, the
//   test at 0x000031CC); the explosion is a 0x180-byte CExplosion named "Commando Drowning Fx"
//   (the module's .rodata at "StateOver" + 0x37A) at the actor's own transform, with the actor's
//   own scale, the white colour and the player index -1 (the constructor call at 0x000032B4).
// 0x000032F4..0x00003318: the drowning sound (the block's +0x06 = guest this+0x7C6) is emitted at
//   the actor's own position with the volume 0x7f.
void CCommandoPirate::x3090_Drown(CStateManager& mgr) {
  if (!GetAlive()) {                                             // 0x000030A8..0x000030B8
    return;
  }
  const TUniqueId fluidId = InFluidId();                         // 0x000030C4
  if (fluidId == kInvalidUniqueId) {                             // 0x000030C8..0x000030D8
    return;
  }
  // The guest's GetObjectById (the call at 0x000030F8) hands out a mutable pointer and the cast to
  // the water (the call at 0x000030FC) needs one; the host's GetObjectById is the const one.
  CScriptWater* water = TCastToPtr< CScriptWater >(
      const_cast< CEntity* >(mgr.GetObjectById(fluidId)));       // 0x000030E4..0x00003104
  if (water == nullptr) {
    return;
  }
  if ((water->GetTriggerFlags() & 0x80) == 0) {                 // 0x00003108..0x00003110
    return;
  }
  const CAABox bounds = water->GetTriggerBoundsWR();            // 0x0000311C
  const CVector3f head = GetLctrTransform(mLocatorHead).GetTranslation();  // 0x00003128..0x00003140
  if (!(head.GetZ() < bounds.GetMaxPoint().GetZ())) {            // 0x0000314C..0x0000315C
    return;
  }
  xc00_ = 0.01f;                                                // rodata_4A0, 0x0000316C
  HealthInfo()->SetHP(0.f);                                     // the guest's +0x04, 0x0000319C
  Death(mgr, GetTransform().GetForward(), kSS_DeathRattle);     // 0x000031C8 (the FourCC 0x5241544C)
  // The guest's drowning effect: the particle it keeps at guest this+0xC28 (the copy at
  // 0x0000DFF0) comes from the asset id in the block's +0x08 (the guest's own +0x7C8, the load at
  // 0x0000DF08); the guest's "is it loaded" byte is at +0xC34 (the test at 0x000031CC).
  const CAssetId effectId(static_cast< uint >(mData.mAlwaysFF2));
  rstl::optional_object< TCachedToken< CGenDescription > > effect;
  if (effectId != kInvalidAssetId) {
    effect = gpSimplePool->GetObj(SObjectTag('PART', effectId));
  }
  if (effect.valid()) {
    // fn_9_2378 - the guest releases its own CEntityInfo here (the call at 0x000032CC) and its
    // string (the call at 0x000032DC); the host's CEntityInfo and the temporary string are
    // released at the end of the statement.
    CExplosion* explosion = rs_new CExplosion(
        TLockedToken< CGenDescription >(*effect), mgr.AllocateUniqueId(),
        CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId),
        rstl::string_l("Commando Drowning Fx"), GetTransform(), 0, GetModelData()->GetScale(),
        CColor::White(), -1);                                    // 0x000032B4
    if (explosion != nullptr) {
      mgr.AddObject(explosion);                                  // 0x000032F0
    }
  }
  CSfxManager::AddEmitter(mData.mAlwaysFF, GetTranslation(), 0x7f, GetCurrentAreaId().Value(),
                          true, false, CSfxManager::kMedPriority);  // 0x00003318
}

// vtable slot 0x18 = fn_9_CB40 (0x754): the message switch that activates the body and starts the
// module's state machine.
//
// fn_9_CB40 (0x754 bytes) calls the base's AcceptScriptMsg first (0x0000CB84) and then switches
// on the message's four-character code (the binary search at 0x0000CB88..0x0000CCC8). The byte at
// guest this+0x20 is CEntity's flag byte - the DOL's CEntity::SetActive writes its 0x80 bit
// (MetroidPrime2Port/build/G2ME01/asm/MetroidPrime/CEntity.s:678) - so the register r31, which
// the function computes from it at 0x0000CB60..0x0000CB80, is the entity's "active" bit.
//
// The seventeen cases, by four-character code and by the offset the case's body starts at:
//
// | code | message | body |
// |---|---|---|
// | XCRT | kSM_Create | the body's Activate and the module's collision actors (0x0000CCCC) |
// | DCTV | kSM_Deactivate | the collision actors' SetActive(false) and the team (0x0000CD88) |
// | ACTV | kSM_Activate | the colour's alpha and the module's flag byte (0x0000CD3C) |
// | XDEL | kSM_Delete | the collision actors' Destroy, the cover point (0x0000CCEC) |
// | ESCP | kSM_Escape | the module's warp-out helper (0x0000CF2C) |
// | XALD | kSM_AreaLoaded | the path area, or the actor's death (0x0000CE04) |
// | STRT | kSM_Start | the module's flag byte (0x0000D060) |
// | STOP | kSM_Stop | the module's flag byte (0x0000D074) |
// | XAUD | kSM_AIUpdateDisabled | the collision actors' SetPhysicsActive(false) (0x0000CDD0) |
// | XLNN | kSM_Landed | the burst fire's type and the flag byte (0x0000D048) |
// | XDMG | kSM_Damage | the module's knock-back, the damage counter (0x0000D09C) |
// | XHIT | kSM_HitObject | the module's hit-object helper (0x0000D25C) |
// | XLAU | kSM_Launching | the actor's own off-ground message (0x0000CFA4) |
// | XRDG | kSM_ResistedDamage | the module's knock-back (0x0000D164) |
// | XOFF | kSM_OffGround | the momentum and the burst fire's type (0x0000CF54) |
// | ALRT | kSM_Alert | the base's flag byte (0x0000D088) |
// | ZERO | kSM_SetToZero | the module's warp-out helper (0x0000CEF8) |
//
// Two of the module helpers this case calls are not ported yet (fn_9_2D0C, the knock-back, and
// fn_9_3900, the jump-back velocity); they are named stand-ins. The third (fn_9_3A5C, the
// hit-object damage) is ported as x3A5C_HitObject.
void CCommandoPirate::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const EScriptObjectMessage message = msg.GetMessage();
  // r31: the entity's active bit as it was before the base's handler (0x0000CB60..0x0000CB80) -
  // the base's own kSM_Activate/kSM_Deactivate change it, so the cases test the old value.
  const bool wasActive = GetActive();
  CPatterned::AcceptScriptMsg(mgr, msg);  // 0x0000CB84
  switch (message) {
  case kSM_Create: {                                       // "XCRT", 0x0000CCCC
    BodyController()->Activate(mgr, pas::kAS_Invalid);     // 0x0000CCD8
    x45A4_CreateCollisionActors(mgr);                      // 0x0000CCE4
    break;
  }
  case kSM_Deactivate: {                                   // "DCTV", 0x0000CD88
    if (wasActive) {                                       // r31, 0x0000CD88
      if (mCollisionShield.get() != nullptr) {             // 0x0000CD90..0x0000CDA4
        mCollisionShield->SetActive(mgr, false);
      }
      if (mCollisionBlade.get() != nullptr) {              // 0x0000CDA8..0x0000CEBC
        mCollisionBlade->SetActive(mgr, false);
      }
    }
    x1930_LeaveTeam(mgr);                                  // 0x0000CDC8
    break;
  }
  case kSM_Activate: {                                     // "ACTV", 0x0000CD3C
    if ((mData.mFlags1 & 0x80) != 0) {                     // guest this+0x91d, 0x0000CD3C
      if (!wasActive && x920_ == -1) {                       // 0x0000CD48..0x0000CD58
        mColor.SetAlpha(0.f);                              // guest this+0x44f, 0x0000CD6C
        mAlphaDelta = 0.f;                                 // guest this+0x40c, 0x0000CD70
      }
      xc63_ |= 0x20;                                       // 0x0000CD7C
    }
    break;
  }
  case kSM_Delete: {                                       // "XDEL", 0x0000CCEC
    if (mCollisionShield.get() != nullptr) {               // 0x0000CCEC..0x0000CCF4
      mCollisionShield->Destroy(mgr);
    }
    if (mCollisionBlade.get() != nullptr) {                // 0x0000CCF8..0x0000CD00
      mCollisionBlade->Destroy(mgr);
    }
    ReleaseCoverPoint(mgr, xbd2_, true);                   // guest this+0xbd2, 0x0000CD14
    mgr.DeleteObjectRequest(xbd8_);                        // guest this+0xbd8, 0x0000CD28
    x1930_LeaveTeam(mgr);                                  // 0x0000CD34
    break;
  }
  case kSM_Escape: {                                       // "ESCP", 0x0000CF2C
    if (wasActive && (xc63_ & 0x20) == 0) {                // r31, 0x0000CF2C..0x0000CF3C
      x3D64_SetWarpOut(mgr, true);                         // 0x0000CF4C
    }
    break;
  }
  case kSM_AreaLoaded: {                                   // "XALD", 0x0000CE04
    if ((mData.mFlags0 & 0x01) != 0) {                     // guest this+0x91c, 0x0000CE04
      // The branch the guest takes when the block's first flag byte has its 0x01 bit: the actor
      // dies here (0x0000CE28..0x0000CE68).
      xc00_ = 0.01f;                                       // rodata_4A0, 0x0000CE28
      RemoveMaterial(kMT_Character, kMT_Solid, kMT_Target, kMT_Orbit, mgr);  // 0x0000CE34
      mAlive = false;                                       // guest this+0x420, 0x0000CE40
      HealthInfo()->SetHP(-1.f);                            // 0x0000CE5C..0x0000CE64
    } else {
      // The connected waypoint (the guest's own predicate accepts only a CScriptWaypoint:
      // fn_9_F6DC) and the path find search's area.
      xbce_ = FindConnectedObject_if(mgr, kSS_Retreat, kSM_Follow, skWaypointPredicate);
      // 0x0000CEA4..0x0000CEB8
      mPathFindSearch.SetArea(
          mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea);
      // 0x0000CEC8..0x0000CEF0
    }
    break;
  }
  case kSM_Start: {                                        // "STRT", 0x0000D060
    xc64_ &= ~0x20;                                        // 0x0000D068
    break;
  }
  case kSM_Stop: {                                         // "STOP", 0x0000D074
    xc64_ |= 0x20;                                          // 0x0000D07C
    break;
  }
  case kSM_AIUpdateDisabled: {                             // "XAUD", 0x0000CDD0
    if (mCollisionShield.get() != nullptr) {               // 0x0000CDD0..0x0000CDE4
      mCollisionShield->SetPhysicsActive(mgr, false);
    }
    if (mCollisionBlade.get() != nullptr) {                // 0x0000CDE8..0x0000CDFC
      mCollisionBlade->SetPhysicsActive(mgr, false);
    }
    break;
  }
  case kSM_Landed: {                                        // "XLNN", 0x0000D048
    mBurstFire.SetBurstType(0);                            // guest this+0xaf0, 0x0000D04C
    xc63_ &= ~0x08;                                        // 0x0000D054
    break;
  }
  case kSM_Damage: {                                       // "XDMG", 0x0000D09C
    if ((xc65_ & 0x0C) != 0) {                             // 0x0000D0A0..0x0000D0AC
      x2D0C_KnockBack(mgr, msg);                           // 0x0000D0C8
    }
    if ((xc66_ & 0x40) != 0) {                             // 0x0000D0D0..0x0000D0E8
      xb8c_ += 5.f;                                        // rodata_468, guest this+0xb8c
    }
    if (const CWeapon* weapon =
            TCastToPtr< CWeapon >(mgr.ObjectById(msg.GetSenderId()))) {  // 0x0000D100..0x0000D10C
      xc50_ += weapon->GetCurrentDamageInfo().GetDamage(*GetDamageVulnerability());  // 0x0000D12C
    }
    mgr.InformListeners(GetTranslation(), kLNT_PlayerFire);   // 0x0000D148
    if ((xc64_ & 0x10) == 0) {                             // 0x0000D150..0x0000D154
      BodyController()->AbortScriptedAnimations();          // 0x0000D15C
    }
    break;
  }
  case kSM_HitObject: {                                    // "XHIT", 0x0000D25C
    x3A5C_HitObject(mgr, msg);                             // 0x0000D274
    break;
  }
  case kSM_Launching: {                                    // "XLAU", 0x0000CFA4
    if (BodyController()->GetCurrentStateId() != pas::kAS_Hurled) {  // guest body+0x37c, 0x0000CFAC
      // The guest sends itself the off-ground message through the base's own handler (the
      // message it builds at 0x0000CFB4..0x0000D000) and then sets its momentum.
      CScriptMsg offGround(msg.GetSenderId(), GetUniqueId(), kSM_OffGround);
      CPatterned::AcceptScriptMsg(mgr, offGround);         // 0x0000D000
      MomentumWR() = CVector3f(0.f, 0.f, -GetGravityConstant() * GetMass());  // 0x0000D034
      x3900_JumpBackVelocity(mgr);                         // 0x0000D040
    }
    break;
  }
  case kSM_ResistedDamage: {                               // "XRDG", 0x0000D164
    if ((xc65_ & 0x0C) != 0 && msg.GetSenderId() == xbc6_) {  // 0x0000D168..0x0000D184
      if (const CCollisionActor* actor =
              TCastToPtr< CCollisionActor >(mgr.ObjectById(xbc6_))) {  // 0x0000D198..0x0000D1A4
        // The collision actor's own vulnerability (its vtable +0x40, 0x0000D1CC), copied
        // (fn_9_D294, 0x0000D1D8) - not the pirate's.
        const CDamageVulnerability vulnerability = *actor->GetDamageVulnerability();
        if (const CWeapon* weapon = TCastToPtr< CWeapon >(
                mgr.ObjectById(actor->GetLastTouchedObject()))) {  // 0x0000D1B0..0x0000D1EC
          if (vulnerability.WeaponHurts(
                  weapon->GetCurrentDamageInfo().GetWeaponMode())) {  // 0x0000D204
            x2D0C_KnockBack(mgr, msg);                       // 0x0000D224
          }
        }
      }
    }
    mgr.InformListeners(GetTranslation(), kLNT_PlayerFire);    // 0x0000D240
    if ((xc64_ & 0x10) == 0) {                              // 0x0000D248..0x0000D24C
      BodyController()->AbortScriptedAnimations();           // 0x0000D254
    }
    break;
  }
  case kSM_OffGround: {                                    // "XOFF", 0x0000CF54
    if (!BodyController()->IsFrozen()) {                     // guest body+0x594's 0x20 bit, 0x0000CF5C
      MomentumWR() = CVector3f(0.f, 0.f, -GetGravityConstant() * GetMass());  // 0x0000CF88..0x0000CF94
    }
    mBurstFire.SetBurstType(1);                            // guest this+0xaf0, 0x0000CF9C
    break;
  }
  case kSM_Alert: {                                        // "ALRT", 0x0000D088
    SetHitByPlayerProjectile(true);                        // guest this+0x420's 0x80 bit, 0x0000D090
    break;
  }
  case kSM_SetToZero: {                                    // "ZERO", 0x0000CEF8
    if (wasActive && (xc63_ & 0x20) == 0) {                // r31, 0x0000CEF8..0x0000CF08
      xc63_ |= 0x40;                                       // 0x0000CF14
      x3D64_SetWarpOut(mgr, false);                        // 0x0000CF24
    }
    break;
  }
  default:
    break;
  }
}

// fn_9_45A4 (0x304 bytes): the module's own collision actor managers, built by its kSM_Create
// case. The guest makes two managers, each with one sphere joint:
//   - the shield's (guest this+0xA3C): a sphere on the left wrist (guest this+0xC62), radius
//     1.5f (rodata_47C), mass 1000.0f (rodata_4B0), named "Shield_CollisionActor" (the module's
//     rodata at 0x893). Its collision actor gets the shield's charge vulnerability (the block's
//     +0xF4 = guest this+0x8B4) and the response type 0x12 = 18 = kWCR_EnemyShielded
//     (0x000046EC..0x000046F4). The actor's id goes to guest this+0xBC6.
//   - the blade's (guest this+0xA40): a sphere on the second gun locator (guest this+0xC5F),
//     radius 1.0f (rodata_43C) and the same mass, named "Blade_CollisionActor" (the module's
//     rodata at 0x8A9). Its collision actor gets the pass-through vulnerability
//     (0x00004814..0x00004820). The actor's id goes to guest this+0xBC8.
// The guest then adds the bit 0x8000 (the material 15, kMT_Glass) to the actor's own material
// filter's exclude list (0x00004830..0x0000487C).
void CCommandoPirate::x45A4_CreateCollisionActors(CStateManager& mgr) {
  {
    rstl::vector< CJointCollisionDescription > descriptions;
    descriptions.push_back(CJointCollisionDescription::SphereCollision(
        mLocatorLWrist, CVector3f::Zero(), 1.5f, rstl::string_l("Shield_CollisionActor"),
        1000.f));  // 0x00004604 (the radius rodata_47C), 0x0000460C (the mass rodata_4B0)
    mCollisionShield = rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(),
                                                      descriptions, false);  // 0x00004694
    xbc6_ = mCollisionShield->GetCollisionDescFromIndex(0).GetCollisionActorId();  // 0x000046B8
    if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(xbc6_))) {
      actor->SetDamageVulnerability(mData.mShield.mChargeVulnerability);  // 0x000046EC
      actor->SetResponseType(kWCR_EnemyShielded);                          // 0x000046F4
    }
  }
  {
    rstl::vector< CJointCollisionDescription > descriptions;
    descriptions.push_back(CJointCollisionDescription::SphereCollision(
        mLocatorGun2, CVector3f::Zero(), 1.f, rstl::string_l("Blade_CollisionActor"),
        1000.f));  // 0x00004728 (the radius rodata_43C), 0x00004730 (the mass rodata_4B0)
    mCollisionBlade = rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(),
                                                     descriptions, false);  // 0x000047C8
    xbc8_ = mCollisionBlade->GetCollisionDescFromIndex(0).GetCollisionActorId();  // 0x000047E8
    if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(xbc8_))) {
      actor->SetDamageVulnerability(CDamageVulnerability::PassThroughVulnerabilty());
      // 0x00004814..0x00004820
    }
  }
  CMaterialFilter filter = GetMaterialFilter();  // 0x00004834
  filter.ExcludeList().Add(kMT_Glass);           // the bit 0x8000, 0x0000485C..0x0000487C
  SetMaterialFilter(filter);                     // 0x0000487C
}

// fn_9_1930 (0xa8 bytes): the module's team helper - it leaves the team the commando joined
// (fn_9_19D8). The team id is the module's own member at guest this+0xBC4.
void CCommandoPirate::x1930_LeaveTeam(CStateManager& mgr) {
  if (xbc4_teamId == kInvalidUniqueId) {  // 0x00001950..0x0000195C
    return;
  }
  CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(xbc4_teamId));
  if (team == nullptr) {  // 0x00001970..0x0000197C
    return;
  }
  if (!team->IsPartOfTeam(GetUniqueId())) {  // 0x00001990..0x0000199C
    return;
  }
  team->QuitTeam(GetUniqueId());  // 0x000019B0
  xbc4_teamId = kInvalidUniqueId;  // 0x000019BC
}

// fn_9_3D64 (0xb8 bytes): the module's warp-out helper. It runs only while the actor is alive
// (the byte at guest this+0x420's 0x40 bit), the module's flag byte at guest this+0xC65 has no
// 0x10 bit, the module's word at guest this+0x920 is -1 and the block's flag byte at guest
// this+0x91D has its 0x80 bit. Then it enters the WarpOut state of the state machine (the guest's
// own object at guest this+0x350, the vtable slot 0x20 - the module's rodata string "WarpOut" at
// 0x7A5) and sets the module's flag byte's 0x10 bit to the argument (0x00003DF4).
void CCommandoPirate::x3D64_SetWarpOut(CStateManager& mgr, bool warpOut) {
  if (!GetAlive()) {           // 0x00003D88..0x00003D90
    return;
  }
  if ((xc65_ & 0x10) != 0) {  // 0x00003D94..0x00003D9C
    return;
  }
  if (x920_ != -1) {          // 0x00003DA0..0x00003DA8
    return;
  }
  if ((mData.mFlags1 & 0x80) == 0) {  // 0x00003DAC..0x00003DB4
    return;
  }
  StateMachineState().SetState(mgr, *this, rstl::string_l("WarpOut"));  // 0x00003DEC
  if (warpOut) {
    xc63_ |= 0x10;  // 0x00003DF4
  } else {
    xc63_ &= ~0x10;  // 0x00003DF4
  }
}

// fn_9_3A5C (0x260 bytes): the module's hit-object helper - the shield's damage on the player
// that touched it, and the blade's damage when the blade is out. With the shield up (the byte
// at guest this+0xC65's 0x04 bit) the sender has to be the shield's collision actor (guest
// this+0xBC6) and the damage is the shield's charge damage (the block's +0xD8 = guest this+0x898);
// a sender that is not the shield's actor falls into the blade's path (0x00003A90), so the blade's
// damage runs with the shield up too. With the blade out (the 0x20 bit) the sender has to be the
// blade's collision actor (guest this+0xBC8) and the damage is the blade's damage (the block's
// +0x0C = guest this+0x7CC). The
// touched object has to be the player, the module's damage timer (guest this+0x440) has to be at
// or below zero, and the damage is applied with the module's own material filter (its data word
// 0x3B = kMT_Solid, the type 3 = kFT_IncludeExclude) and the actor's forward direction.
void CCommandoPirate::x3A5C_HitObject(CStateManager& mgr, const CScriptMsg& msg) {
  TUniqueId actorId = kInvalidUniqueId;
  const CDamageInfo* damage = nullptr;
  // The guest's first path (the shield, the 0x04 bit of the byte at guest this+0xC65) falls into
  // the second one (the blade) when the sender is not the shield's collision actor (0x00003A90),
  // so the blade's damage still runs with the shield up.
  if ((xc65_ & 0x04) != 0 && msg.GetSenderId() == xbc6_) {  // 0x00003A78..0x00003A90
    actorId = xbc6_;
    damage = &mData.mShield.mChargeDamage;  // guest this+0x898
  } else if ((xc65_ & 0x20) != 0 && msg.GetSenderId() == xbc8_) {  // 0x00003B90..0x00003BA8
    actorId = xbc8_;
    damage = &mData.mBladeDamage;  // guest this+0x7CC
  } else {
    return;
  }
  CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(actorId));
  if (actor == nullptr) {  // 0x00003AA8..0x00003AAC
    return;
  }
  const TUniqueId touched = actor->GetLastTouchedObject();  // 0x00003AB8
  if (TCastToPtr< CPlayer >(mgr.ObjectById(touched)) == nullptr) {  // 0x00003AD0..0x00003ADC
    return;
  }
  if (mCurDamageRemTime > 0.f) {  // guest this+0x440, 0x00003AE0..0x00003AF4
    return;
  }
  mgr.ApplyDamage(GetUniqueId(), touched, GetUniqueId(), *damage,
                  CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
                  GetTransform().GetForward());  // 0x00003B80
  mCurDamageRemTime = mDamageWaitTime;              // guest this+0x444, 0x00003C9C
}

// fn_9_2048 (0x330 bytes): the module's shield effect - the explosion and the sound that play
// when the shield takes a hit.
//
// The byte at guest this+0xC64's 0x01 bit stops it (the clrlwi at 0x00002064); otherwise the bit
// is set (0x00002074) and the effect is built at the module's left-wrist locator (the
// GetLctrTransform call at 0x00002088, the locator id at guest this+0xC62).
//
// Which effect it is depends on the module's state word at guest this+0xC4C: 1 is the charge (the
// compare at 0x0000209C), which needs the effect the guest's token at guest this+0xC08 holds to
// be loaded (the test at 0x000020A8) and builds an explosion it clears again (the call at
// 0x000021A4), so the charge's effect never reaches the world; 0 is the armed shield (the compare
// at 0x000021BC), which needs the effect the guest's token at guest this+0xC0C holds to be loaded
// (the test at 0x000021C8) and puts its explosion into the world (the AddObject call at
// 0x000022E8), starts the block's explode sound (the SfxStart call at 0x00002324, the sound id at
// guest this+0x8F4 = the block's +0x134) and removes the module's own second sound emitter (the
// RemoveEmitter call at 0x00002340, the handle at guest this+0xC38) before it resets the module's
// state word and its two scale floats (0x0000234C..0x00002360).
//
// The guest's two tokens are the block's two effect asset ids: the port reads the charge effect
// (the block's +0x78) for the charge and the explode effect (the block's +0x58, the one the
// explode sound at the block's +0x5C belongs to) for the armed shield.
void CCommandoPirate::x2048_ShieldEffect(CStateManager& mgr) {
  if ((xc64_ & 0x01) != 0) {  // 0x00002064
    return;
  }
  xc64_ |= 0x01;              // 0x00002074
  const CTransform4f xf = GetLctrTransform(mLocatorLWrist);  // 0x00002088
  // 0x00002098..0x000020AC: the state word at guest this+0x0C4C picks the effect - 1 is the
  // charge (the guest's own token at guest this+0x0C08, used when the byte at guest this+0x0C14
  // says it is loaded) and 0 is the armed shield (the token at guest this+0x18, used when the
  // byte at guest this+0x0C24 says it is loaded); every other value does nothing (0x000021BC).
  // Both effects reach the world (the AddObject at 0x000022E8) and the sound plays
  // (0x00002324); the guest destroys its temporary copy of the token on the way (0x000021A4).
  const CAssetId effectId = xc4c_ == 1 ? mData.mShield.mChargeEffect    // 0x0000216C (this+0x0C08)
                          : xc4c_ == 0 ? mData.mShield.mExplodeEffect   // 0x0000228C (this+0x18)
                                       : kInvalidAssetId;
  // 0x000021BC..0x000021C0: a state word that is neither 0 nor 1 reaches 0x000022D8 with no
  // explosion at all - the port takes the invalid id, so no token is loaded and the three fields
  // at the end are reset like the guest's.
  // 0x000020A8 (the charge) and 0x000021C8 (the armed shield): the guest's own test of the
  // token's "loaded" flag (the bytes at guest this+0xC14 and this+0xC24); the port's token's
  // validity stands in for it.  When the token is not there the guest reaches 0x000022D8 with no
  // explosion, so nothing is added and no sound plays - but the three fields at the end are
  // reset either way (0x0000234C..0x00002360).
  CExplosion* explosion = nullptr;
  rstl::optional_object< TCachedToken< CGenDescription > > effect;
  if (effectId != kInvalidAssetId) {
    effect = gpSimplePool->GetObj(SObjectTag('PART', effectId));
  }
  if (effect.valid()) {  // 0x000021C8 (the guest's test of the flag at guest this+0xC24)
    explosion = rs_new CExplosion(
        TLockedToken< CGenDescription >(*effect), mgr.AllocateUniqueId(),
        CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId),
        rstl::string_l("CommandExplosion F"), xf, 0, GetModelData()->GetScale(), CColor::White(),
        -1);
    // 0x000021A4: the guest destroys its temporary copy of the token here - nothing is cleared
    // from the explosion itself.
  }
  if (explosion != nullptr) {  // 0x000022D8..0x000022DC
    mgr.AddObject(explosion);  // 0x000022E8
    CSfxManager::SfxStart(mData.mShield.mSoundExplode, 0x40, 0, CSfxManager::kAllAreas, false, false,
                          CSfxManager::kMedPriority);  // 0x00002324
    if (xc38_ != CSfxHandle::NullHandle()) {  // 0x00002328..0x00002344
      CSfxManager::RemoveEmitter(xc38_);
      xc38_ = CSfxHandle::NullHandle();
    }
  }
  xc4c_ = -1;   // 0x00002354
  xc48_ = 0.f;  // 0x0000235C
  xc44_ = 0.f;  // 0x00002360
}

// fn_9_2A98 (0x214 bytes): the module's waypoint picker - it takes the AI waypoint the module
// follows (the id at guest this+0x6C0) and, when the third argument is set, walks to the
// waypoint's own "arrived" connection.
//
// The charge speed (the block's +0x12C = guest this+0x8EC) has to be above zero
// (0x00002AD0..0x00002AD4), the AI waypoint has to be there (0x00002AD8..0x00002AF8) and it has
// to allow the boost (its flags, the word at its +0x164, have to have their 0x20 bit - the
// rlwinm at 0x00002B00). Then the module's own connection search finds the waypoint's "arrived"
// connection (0x00002B44: the state kSS_Arrived and the message kSM_Next, with the module's own
// waypoint predicate), the found waypoint becomes the module's waypoint (the store at
// 0x00002B8C) and its position (the loads at 0x00002B90..0x00002BA8) and its forward column
// (0x00002BAC..0x00002BC4) become the module's own vectors. Without the third argument the
// module keeps the waypoint it has (0x00002C0C..0x00002C3C). The module's time to the waypoint
// (the float at guest this+0xB88) is the distance to it over the charge speed
// (0x00002BF8..0x00002C04).
bool CCommandoPirate::x2A98_PickWaypoint(CStateManager& mgr, bool useNext) {
  const float speed = mData.mShield.mChargeSpeed;  // guest this+0x8EC
  if (speed <= 0.f) {  // 0x00002AD0..0x00002AD4
    return false;
  }
  const CScriptAIWaypoint* waypoint =
      TCastToPtr< CScriptAIWaypoint >(
          mgr.ObjectById(mWaypointNavigation.GetDestination()));  // 0x00002AD8..0x00002AF8
  if (waypoint == nullptr) {
    return false;
  }
  if ((waypoint->GetFlags() & 0x20) == 0) {  // 0x00002B00..0x00002B04
    return false;
  }
  CVector3f position;
  CVector3f forward;
  if (useNext) {  // 0x00002B08
    // The guest searches the *waypoint's* connections (its r4 is the waypoint the flags were
    // read from, 0x00002AF4..0x00002B44), not the commando's own.
    const TUniqueId id =
        waypoint->FindConnectedObject_if(mgr, kSS_Arrived, kSM_Next, skWaypointPredicate);
    const CScriptWaypoint* next = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(id));
    if (next == nullptr) {  // 0x00002B80..0x00002B84
      return false;
    }
    mWaypointNavigation.SetDestination(id);           // 0x00002B8C (guest this+0x6C0)
    position = next->GetTranslation();                // 0x00002B90..0x00002BA8
    forward = next->GetTransform().GetForward();      // 0x00002BAC..0x00002BC4
  } else {
    position = waypoint->GetTranslation();             // 0x00002C0C..0x00002C24
    forward = waypoint->GetTransform().GetForward();   // 0x00002C28..0x00002C3C
  }
  xb94_ = position;  // guest this+0xB94
  xbac_ = forward;   // guest this+0xBAC
  const CVector3f delta = xb94_ - GetTranslation();  // 0x00002C40..0x00002C6C
  xb88_ = delta.Magnitude() / speed;                // 0x00002BF8..0x00002C04
  return true;
}

// fn_9_2474 (0x580 bytes): the module's turn helper - it turns the commando towards the waypoint
// it follows and charges at it.
//
// The charge speed (the block's +0x12C = guest this+0x8EC) has to be above zero
// (0x000024B4..0x000024B8) or the helper does nothing. The direction to the waypoint (the vector
// at guest this+0xB94, the delta at 0x00002534..0x0000253C) is normalized when it is magnitude
// safe (0x0000254C..0x00002564) and the actor's own forward column otherwise
// (0x0000256C..0x00002584). The time to the waypoint is the *raw* delta's length over the charge
// speed (0x000025A4): the guest's Magnitude call takes the delta at its own stack slot (the
// `addi r3, r1, 0xd4` at 0x0000258C), not the copy of the direction it made at sp+0xC8.
//
// The new rotation: the actor's position and the direction give a look-at transform (with the
// waypoint's forward column as the up vector, 0x000025EC), the target's quaternion comes from it
// (0x00002604) and the actor's own quaternion (0x000024DC) is slerped towards it with the factor
// clamp(1 - the time / the module's own time to the waypoint (the float at guest this+0xB88), 0, 1)
// (0x00002634..0x00002678). The result is normalized (0x000026A4), turned into a transform
// (0x000026B4) and the actor takes it (0x000026C0).
//
// The charge command: when the actor's facing and the direction differ by more than 10 degrees
// (rodata_498 = 0.17453292, 0x000026D8) the commando is told to loop-attack, and the loop attack
// type depends on the sign of the dot product between the direction and the actor's own right
// column (guest this+0x24, 0x34 and 0x44, 0x000026E0..0x00002718): 6 when it is positive
// (0x00002720..0x0000276C) and 5 when it is not (0x000027B0..0x000027FC). When the actor faces
// the waypoint the type is 4 (0x00002840..0x00002890). The guest also sets the command's flag
// byte's 0x80 bit (0x00002740, 0x000027D0 and 0x00002860) - the retail's skip-into flag - and
// leaves the "wait for the animation" word at 0 (0x0000274C and 0x000027DC).
//
// The movement: the direction in the actor's own frame (TransposeRotate, 0x000028CC) times the
// charge speed and the frame time (0x00002918) moves the actor, and the module's own push vector
// (guest this+0xBB8) does the same when it is not zero (0x0000291C..0x000029A4), after which the
// push vector is cleared (0x000029A8..0x000029C0).
void CCommandoPirate::x2474_TurnToWaypoint(CStateManager& mgr, float dt) {
  const float speed = mData.mShield.mChargeSpeed;  // guest this+0x8EC
  if (speed <= 0.f) {  // 0x000024B4..0x000024B8
    return;
  }
  const CVector3f pos = GetTranslation();  // guest this+0x54
  const CVector3f delta = xb94_ - pos;    // guest this+0xB94
  CVector3f dir;
  if (delta.IsMagnitudeSafe()) {  // 0x0000254C
    dir = delta.AsNormalized();   // 0x00002560
  } else {
    dir = GetTransform().GetForward();  // 0x0000256C (guest this+0x28, 0x38 and 0x48)
  }
  // 0x000025A4: the guest's Magnitude call takes the raw delta (sp+0xD4, the `addi r3, r1, 0xd4`
  // at 0x0000258C) - the normalized copy at sp+0xC8 is the direction the look-at uses, not the
  // one the time comes from.
  const float time = delta.Magnitude() / speed;  // 0x000025A4
  const CTransform4f lookAt = CTransform4f::LookAt(pos, pos + dir, xbac_);  // 0x000025EC
  const CQuaternion current = CQuaternion::FromMatrix(GetTransform());  // 0x000024DC
  const CQuaternion target = CQuaternion::FromMatrix(lookAt);  // 0x00002604
  // the factor: clamp(1 - the time / the module's time to the waypoint, 0, 1) (0x00002634..0x00002678)
  const float factor = CMath::Clamp(0.f, 1.f - time / xb88_, 1.f);
  const CQuaternion turn = CQuaternion::SlerpLocal(current, target, factor);
  SetTransform(turn.BuildNormalized().BuildTransform4f(pos));  // 0x000026A4..0x000026C0
  const float angle = CVector3f::GetAngleDiff(GetTransform().GetForward(), dir);  // 0x000026CC
  pas::ELoopAttackType type = pas::kLAT_Four;  // 0x00002840
  if (angle > 0.17453292f) {  // rodata_498 = 10 degrees, 0x000026D8
    const float dot = CVector3f::Dot(dir, GetTransform().GetRight());  // 0x000026E0..0x00002718
    type = (dot > 0.f) ? pas::kLAT_Six : pas::kLAT_Five;  // 0x0000271C, 0x000027D8
  }
  CBCLoopAttackCmd cmd(type, false);  // 0x00002744..0x0000276C, the "wait" word stays 0
  cmd.SetSkipInto(true);  // the guest's flag byte's 0x80 bit (0x00002740)
  BodyController()->CommandMgr().DeliverCmd(cmd);
  // The movement: the direction in the actor's own frame (0x000028CC) times the charge speed and
  // the frame time (0x00002918).
  const CVector3f local = GetTransform().TransposeRotate(dir);
  MoveInOneFrameOR(local * (speed * dt), dt);
  if (xbb8_.GetX() != 0.f || xbb8_.GetY() != 0.f || xbb8_.GetZ() != 0.f) {  // 0x0000291C..0x00002948
    const CVector3f push = GetTransform().TransposeRotate(xbb8_);  // 0x00002964
    // 0x000029A4: the push is scaled by the charge speed and the frame time, like the movement
    // above (the two fmuls chains at 0x0000297C..0x00002994).
    MoveInOneFrameOR(push * (speed * dt), dt);  // 0x000029A4
    xbb8_ = CVector3f::Zero();  // 0x000029A8..0x000029C0
  }
}

// fn_9_2D0C (0x384 bytes): the module's knock-back helper - the shield's and the blade's damage
// and the knock-back they cause.
//
// The weapon: when the sender is the shield's own collision actor (guest this+0xBC6) the weapon is
// the one that touched that collision actor (0x00002D38..0x00002D74) and the module's own effect
// helper fn_9_2048 runs when the byte at guest this+0xC66 has its 0x20 bit (0x00002D78..0x00002D90);
// otherwise the weapon is the sender itself (0x00002D98..0x00002DA8).
//
// The knock-back: the info (the weapon's own forward column, its id, its owner id, its current
// damage info and the direct flag - 0x00002DB8..0x00002E00), the knock-back manager's anim
// reaction range when the flag byte at guest this+0xC64 has its 0x01 bit (0x00002E04..0x00002E1C),
// the knock-back itself (the vtable slot 0x9C, 0x00002E38), the reaction range the guest restores
// right after it (0x00002E3C..0x00002E48) and the module's own flag byte (0x00002E4C..0x00002E8C).
//
// Then the commando looks for the ground under itself: a ray from its own position plus the up
// vector six units down (rodata_49C) with the module's own material filter - the include list
// kMT_Solid (the data word 0x3B at lbl_9_data_814) and the exclude list kMT_Player,
// kMT_Character, kMT_CollisionActor and kMT_Platform (the data words 0x20, 0x21, 0x2F and 0x31
// at lbl_9_data_81C, lbl_9_data_818, lbl_9_data_820 and lbl_9_data_824), the type 3 =
// kFT_IncludeExclude.
//
// When the ray is clear, the module's own float at guest this+0xC00 takes
// 0.01f (rodata_4A0), the commando dies with the script object state "RATL" (kSS_DeathRattle)
// and its own forward direction (the vtable slot 0x98, 0x00003018), its health is zeroed (the
// vtable slot 0x38, 0x0000302C), the damage source is recorded (0x00003044..0x00003068) and the
// module's flag byte takes its 0x04 bit (0x0000306C..0x00003078).
void CCommandoPirate::x2D0C_KnockBack(CStateManager& mgr, const CScriptMsg& msg) {
  const TUniqueId senderId = msg.GetSenderId();
  const CWeapon* weapon = nullptr;
  if (senderId == xbc6_) {
    if (const CCollisionActor* actor =
            TCastToPtr< CCollisionActor >(mgr.ObjectById(xbc6_))) {  // 0x00002D3C..0x00002D4C
      weapon = TCastToPtr< CWeapon >(mgr.ObjectById(actor->GetLastTouchedObject()));
    }
    if ((xc66_ & 0x20) != 0) {  // 0x00002D78..0x00002D84
      x2048_ShieldEffect(mgr);  // 0x00002D90
    }
  } else {
    weapon = TCastToPtr< CWeapon >(mgr.ObjectById(senderId));  // 0x00002D9C..0x00002DA8
  }
  if (weapon != nullptr) {
    const CKnockBackInfo info(weapon->GetTransform().GetForward(), weapon->GetUniqueId(),
                              weapon->GetOwnerId(), weapon->GetCurrentDamageInfo(),
                              true);  // 0x00002DB8..0x00002E00
    if ((xc64_ & 0x01) != 0) {  // 0x00002E04..0x00002E1C
      KnockBackController().SetAnimReactionRange(CKnockBackMgr::kAR_Hurled, CKnockBackMgr::kAR_Fall);
    }
    KnockBack(mgr, info);  // 0x00002E38 (the vtable slot 0x9C, the module's own override, fn_9_C300)
    // The guest restores the reaction range right after the knock-back (0x00002E3C..0x00002E48).
    KnockBackController().SetAnimReactionRange(CKnockBackMgr::kAR_None, CKnockBackMgr::kAR_Fall);
  }
  // The module's flag byte: the 0x04 bit is set when the knock-back manager's reaction is between
  // 2 and 4 (the guest's word at guest this+0x630, the manager's own mReaction at its +0x10, the
  // compares at 0x00002E50 and 0x00002E58); otherwise the 0x04 bit becomes 0x04 | 0x01 (the extrwi
  // 1,29 = 0x04 and clrlwi 31 = 0x01 or-ed and put back by rlwimi 2,29,29, 0x00002E78..0x00002E8C).
  const uint reaction = KnockBackController().GetActiveReaction();
  if (reaction >= 2 && reaction < 5) {
    xc64_ |= 0x04;                                         // 0x00002E64..0x00002E70
  } else {
    xc64_ |= (xc64_ & 0x01) << 2;                          // 0x00002E78..0x00002E8C
  }
  // The ray under the commando: from its own position plus the up vector six units down.
  const CVector3f start = GetTranslation() + CVector3f::Up();
  const CVector3f end = start + CVector3f::Down() * 6.f;
  const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid),
      CMaterialList(kMT_Player, kMT_Character, kMT_CollisionActor, kMT_Platform));
  if (mgr.RayCollideWorld(start, end, filter, this)) {  // 0x00002FC4
    xc00_ = 0.01f;                                        // rodata_4A0, 0x00002FF0
    Death(mgr, GetTransform().GetForward(), kSS_DeathRattle);  // 0x00003018
    HealthInfo()->SetHP(0.f);                              // 0x0000302C..0x0000303C
    // The guest records the damage source when there is a weapon (the call at 0x00003068).
    if (weapon != nullptr) {
      mgr.RecordDamageSource(*this, weapon->GetUniqueId(), weapon->GetCurrentDamageInfo(), true,
                             false);                       // 0x00003044..0x00003068
    }
    xc64_ |= 0x04;                                         // 0x0000306C..0x00003078
  }
}

// Run: cov 2026-10-21 (area 59, 05_temple_federationsite_dark, the launching driver): every arm -
// the gcov report gives 15/15 lines and 2/2 branches.  The driver sends the module's own
// launching message (0x584C4155) four times in a row, so the helper's early out (the 0x08 bit of
// the flag byte at guest this+0xC63, 0x0000393C) runs after the first call set it (0x00003A24).
// @ fn_9_3900
// fn_9_3900 (0x15c bytes): the module's jump-back velocity helper - the ballistic velocity that
// carries the commando to its own jump target (the vector at guest this+0xBF0, the float at guest
// this+0xBEC). It runs only while the flag byte at guest this+0xC63 has no 0x08 bit
// (0x00003930..0x0000393C) and sets that bit when it is done (0x00003A1C..0x00003A28).
void CCommandoPirate::x3900_JumpBackVelocity(CStateManager& mgr) {
  if ((xc63_ & 0x08) != 0) {  // 0x00003930..0x0000393C
    return;
  }
  const float gravity = GetGravityConstant();  // the vtable slot 0x114, 0x00003974..0x00003980
  const CVector3f target = xbf0_;             // guest this+0xBF0
  const CVector3f pos = GetTranslation();      // guest this+0x54..0x5C
  const float dx = target.GetX() - pos.GetX();
  const float dy = target.GetY() - pos.GetY();
  // The apex: the float at guest this+0xBEC over the higher of the two heights.
  const float apex = xbec_ + (pos.GetZ() >= target.GetZ() ? pos.GetZ() : target.GetZ());
  const float zVelocity = CMath::SqrtF(2.f * gravity * (apex - pos.GetZ()));  // 0x000039A8..0x000039BC
  const float timeUp = zVelocity / gravity;                                 // 0x000039D8
  const float timeDown = CMath::SqrtF(2.f * (apex - target.GetZ()) / gravity);  // 0x000039DC
  const float time = timeUp + timeDown;                                      // 0x000039EC
  SetVelocityWR(CVector3f(dx / time, dy / time, zVelocity));  // 0x00003A04..0x00003A18
  xc63_ |= 0x08;                                                  // 0x00003A24
}

// vtable slot 0x24 = fn_9_C6D4 (0xc4): the base's PreRender and then the bone tracking's own.
// The guest's rag doll (guest this+0xBFC) is always null - the constructor stores 0 there
// (0x0000DC18) - so the guest's rag-doll call (the vtable slot 0x10 call at 0x0000C718) is dead
// here and only the base's PreRender runs (0x0000C728). The bone tracking's own PreRender (the
// call at 0x0000C77C) follows with the model's scale (the three floats at the model data's +0,
// loaded at 0x0000C754..0x0000C770), the actor's transform (guest this+0x24) and the body
// controller (guest this+0x48c).
// Run: force2, 05_swamp_timber! (2026-10-21): the body has no arm of its own (the rag-doll arm is dead, above).
// @ fn_9_C6D4
void CCommandoPirate::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);  // 0x0000C728
  mBoneTracking.PreRender(mgr, *AnimationData(), GetTransform(), GetModelData()->GetScale(),
                          *BodyController());  // 0x0000C77C
}

// @ fn_9_C4FC
// vtable slot 0x28 = fn_9_C4FC (0x20): a straight call to CPatterned::AddToRenderer, so the base's
// is what runs.
// One arm. Run: tools/cov/run.sh with tools/cov/specs/commandopirate.py and
// tools/drivers/commandopirate_core.py, area 59 (05_temple_federationsite_dark) and 05_swamp_timber!:
// entry 50/50 (capped), the base's call 50.
void CCommandoPirate::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
}

// vtable slot 0x2c = fn_9_C598 (0x13c).
// fn_9_C598 (0x13c bytes): the actor's render (the vtable slot 0x2c). The guest fades the actor
// with its colour's alpha (the psq_l at 0x0000C5C4 reads the alpha byte at guest this+0x44F and
// rodata_4F0 is 1/255) and, while the actor is alive (the byte at guest this+0x420's 0x40 bit)
// and its own word at guest this+0x920 is 1 (the port's `x920_`, the compare at 0x0000C5E4), it
// draws the space warp at the bounding box's centre point with the sine of pi times the alpha
// (rodata_4F4, the FastSinR call at 0x0000C5F8 and the DrawSpaceWarp call at 0x0000C634). With an
// alpha above zero (the compare at 0x0000C640) the base's own render runs (the call at
// 0x0000C650) and the module's own renderable follows - the one at guest this+0xC3C when the
// module's state word at guest this+0xC4C (the port's `xc4c_`) is 0 (the compare at 0x0000C664)
// and the one at guest this+0xC40 when it is 1 (the compare at 0x0000C658). The port has no such
// renderables, so those two are named stand-ins.
// Run: cov 2026-10-21 (core drivers, 59 + swamp; force2/force3): every arm; the null
// effects from F58C forced to kInvalidAssetId.
// @ fn_9_C598
void CCommandoPirate::Render(const CStateManager& mgr) const {
  const float alpha = mColor.GetAlpha();                        // 0x0000C5C4..0x0000C5D8
  if (GetAlive() && x920_ == 1) {                                // 0x0000C5CC..0x0000C5E8
    const float warp = CMath::FastSinR(3.1415927f * alpha);      // rodata_4F4, 0x0000C5F8
    if (warp > 0.f) {                                            // 0x0000C608..0x0000C60C
      mgr.DrawSpaceWarp(GetBoundingBox().GetCenterPoint(), warp);  // 0x0000C618..0x0000C634
    }
  }
  if (alpha > 0.f) {                                            // 0x0000C640..0x0000C644
    CPatterned::Render(mgr);                                    // 0x0000C650
    if (xc4c_ == 0) {                                           // 0x0000C664..0x0000C668
      if (xc3c_armEffect.get() != nullptr) {                    // 0x0000C670..0x0000C678
        xc3c_armEffect->Render();                               // vtable +0x10, 0x0000C688
      }
    } else if (xc4c_ == 1) {                                    // 0x0000C658..0x0000C65C
      if (xc40_chargeEffect.get() != nullptr) {                 // 0x0000C690..0x0000C698
        xc40_chargeEffect->Render();                            // vtable +0x10, 0x0000C6A8
      }
    }
  }
}

// vtable slot 0x34 = fn_9_C51C (0x7c): the base's own. The guest's rag doll (guest this+0xBFC)
// is always null - the constructor stores 0 there (0x0000DC18) - so the guest's rag-doll branch
// (the CRagDoll::PreRenderAllViewports call at 0x0000C560 with lbl_9_rodata_458 = 0.2f, then the
// portal system state) is dead here and the base's PreRenderAllViewports runs (0x0000C57C).
// Run: force2, 05_swamp_timber! (2026-10-21): the body has no arm of its own (the rag-doll arm is dead, above).
// @ fn_9_C51C
void CCommandoPirate::PreRenderAllViewports(CStateManager& mgr) {
  CPatterned::PreRenderAllViewports(mgr);  // 0x0000C57C
}

// vtable slot 0x40 = fn_9_C2CC (0x34): the shield's damage vulnerability while the flag byte at
// guest +0xc65 has its 0x04 bit set (the guest tests it with extrwi at 0x0000C2DC), the actor's
// own vulnerability otherwise.
// Run: force2, 05_swamp_timber! (2026-10-21): both arms; the shield arm forced (tools/cov/force.sh,
// FORCE "C2CC shield bit": xc65_ |= 0x04 from the 301st call).
// @ fn_9_C2CC
const CDamageVulnerability* CCommandoPirate::GetDamageVulnerability() const {
  if (xc65_ & 0x04) {
    return &mShieldVulnerability;
  }
  return CPatterned::GetDamageVulnerability();
}

// vtable slot 0x70 = fn_9_C798 (0x1ac): the module's user-event switch.
//
// fn_9_C798 (0x1ac bytes) switches on the event type (the jump table lbl_9_data_970, .data 0x970,
// the 31 entries for the types 0..30; the types above 30 branch to the base at 0x0000C8FC). Each
// of the module's own cases sets the "handled" register (r6, the test at 0x0000C8FC) and the
// base's own DoUserAnimEvent (the call at 0x0000C918) is then skipped; the default case leaves
// the register at zero and the base's call runs.
// Run: cov 2026-10-21 (core drivers, 59 + swamp; force2/force3): every arm; types
// 13/22/29/30/11 forced on alternate calls.
// @ fn_9_C798
void CCommandoPirate::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                      EUserEventType type, float dt) {
  bool handled = false;                                              // r6, 0x0000C8D4
  switch (type) {
  case kUE_Projectile: {                                             // the entry 0 (+0xb8)
    x5410_CreateEGrenade(mgr);                                        // 0x0000C850
    handled = true;                                                  // 0x0000C854
    break;
  }
  case kUE_DamageOn: {                                               // the entry 8 (+0x154)
    mCurDamageRemTime = 0.f;                                         // 0x0000C8F8 (guest this+0x440)
    handled = true;                                                  // 0x0000C8F0
    break;
  }
  case kUE_DeGenerate:                                               // the entry 11 (+0x64)
  case kUE_BecomeRagDoll: {                                          // the entry 19 (+0x64)
    if (GetHealthInfo()->GetHP() <= 0.f) {                           // 0x0000C810..0x0000C824
      xc00_ = 0.05f * mgr.Random()->Float() + 0.001f;                // 0x0000C828..0x0000C844
    }
    handled = true;                                                  // 0x0000C848
    break;
  }
  case kUE_Landing: {                                                // the entry 12 (+0xe8)
    if ((xc65_ & 0x04) != 0) {                                       // 0x0000C880..0x0000C888
      xc64_ &= ~0x08;                                                // 0x0000C88C..0x0000C898
    }
    handled = true;                                                  // 0x0000C89C
    break;
  }
  case kUE_TakeOff: {                                                // the entry 13 (+0xc4)
    if ((xc65_ & 0x04) != 0) {                                       // 0x0000C85C..0x0000C864
      xc64_ |= 0x08;                                                 // 0x0000C868..0x0000C874
    }
    handled = true;                                                  // 0x0000C878
    break;
  }
  case kUE_BreakLockOn:                                              // the entry 22 (+0x5c)
    handled = true;                                                  // 0x0000C7F4
    break;
  case kUE_Activate: {                                               // the entry 29 (+0x10c)
    if ((xc65_ & 0x04) != 0 || (xc65_ & 0x08) != 0) {               // 0x0000C8A4..0x0000C8B4
      x1F88_EventStart();                                            // 0x0000C8B8..0x0000C8BC
    }
    handled = true;                                                  // 0x0000C8C0
    break;
  }
  case kUE_Deactivate: {                                             // the entry 30 (+0x130)
    if ((xc65_ & 0x04) != 0 || (xc65_ & 0x08) != 0) {               // 0x0000C8C8..0x0000C8D8
      x1EF8_Activate();                                              // 0x0000C8DC..0x0000C8E0
    }
    handled = true;                                                  // 0x0000C8E4
    break;
  }
  default:
    break;                                                           // 0x0000C8FC (the base's call)
  }
  if (!handled) {
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);                // 0x0000C904..0x0000C918
  }
}

// fn_9_1C0C (0x2ec bytes): the module's renderable driver, called from the actor's Think (the
// call at 0x0000CA88). The run in area 59 (2026-10-21) reaches only the early return: the
// module's state word at guest this+0xC4C stays -1, because only the shield's own animation
// events (fn_9_1F88 and fn_9_1EF8) set it, and those need the shield's bits at guest this+0xC65
// that the module's ArmShield state (fn_9_86B4) sets - until that state is ported, this body is
// not marked. While the module's state word at guest this+0xC4C is -1 it does nothing
// (the compare at 0x00001C34); otherwise, while the byte at guest this+0xC66 has its 0x20 bit
// (the test at 0x00001C40) and the body controller's byte at guest this+0x48C + 0x594 has its
// 0x20 bit (the test at 0x00001C50), it sets the 0x04 bit of the byte at guest this+0xC64 and
// deactivates the module's own collision actor manager (the guest's this+0xA3C). Then it
// turns off the emission of the shield's two particle systems (the guest's this+0xC40 and
// this+0xC3C) and
// advances the module's own scale (the float at guest this+0xC44) towards its target (the float
// at guest this+0xC48): with a frame time below 0.5f (rodata_440) the scale moves by the frame
// time times the difference over 0.5f (0x00001CC4..0x00001CDC), otherwise it takes the target
// (0x00001CE4..0x00001CE8); when both the scale and its target are below 0.1f (rodata_42C) the
// state word goes back to -1 (0x00001CEC..0x00001D10). Finally the state word picks the
// renderable to drive: 1 is the one at guest this+0xC40 and 0 the one at guest this+0xC3C
// (0x00001D14..0x00001ED4) - both are positioned at the locator segment id at guest this+0xC62
// and scaled by the module's own scale.
//
// The body controller's byte at +0x594 is the retail CBodyController's flag byte: the class
// layout (the actor pointer, the command manager 0x368, the state info 0x204, the rotation and
// five words) puts the flags there, and MWCC allocates the bitfields from the MSB, so the bits
// are mAnimationOver 0x80, mActive 0x40, mFrozen 0x20, mHasBeenFrozen 0x10 and mPlayDeathAnims
// 0x08. The 0x20 bit is therefore the frozen flag: the Dark Commando (module 3) reads the same
// byte with the same mask (the extrwi at 0x00006920 and 0x00006978) and its own source guards
// the same calls with mBodyController->IsFrozen()
// (src/MetroidPrime/Enemies/CDarkCommando.cpp:451 and :459).
// Run: cov 2026-10-21 (core drivers, 59 + swamp; force2/force3): every guest arm;
// frozen, dt 0.6 and the state word forced. Open: the host-only shield-manager null test.
// @ fn_9_1C0C
void CCommandoPirate::x1C0C_UpdateRenderables(CStateManager& mgr, float dt) {
  if (xc4c_ == -1) {                                                // 0x00001C30..0x00001C38
    return;
  }
  if ((xc66_ & 0x20) != 0 && BodyController()->IsFrozen()) {        // 0x00001C3C..0x00001C54
    xc64_ |= 0x04;                                                  // 0x00001C58..0x00001C68
    // The guest calls through the pointer without a test; the host's test covers the window
    // before the kSM_Create case has built the manager.
    if (mCollisionShield.get() != nullptr) {
      mCollisionShield->SetActive(mgr, false);                      // 0x00001C70
    }
  }
  if (xc40_chargeEffect.get() != nullptr) {                         // 0x00001C74..0x00001C7C
    xc40_chargeEffect->SetParticleEmission(false);                  // vtable +0x2c, 0x00001C90
  }
  if (xc3c_armEffect.get() != nullptr) {                            // 0x00001C94..0x00001C9C
    xc3c_armEffect->SetParticleEmission(false);                     // vtable +0x2c, 0x00001CB0
  }
  if (dt < 0.5f) {                                                  // rodata_440, 0x00001CB4..0x00001CC0
    xc44_ = xc44_ + dt * (xc48_ - xc44_) / 0.5f;                     // 0x00001CC4..0x00001CDC
  } else {
    xc44_ = xc48_;                                                  // 0x00001CE4..0x00001CE8
  }
  if (xc44_ < 0.1f && xc48_ < 0.1f) {                               // rodata_42C, 0x00001CEC..0x00001D08
    xc4c_ = -1;                                                     // 0x00001D0C..0x00001D10
  }
  // The state word picks the effects to drive (0x00001D14..0x00001D28): 1 drives the charge
  // effect and then falls through to the arm effect (there is no branch after 0x00001E00), 0
  // drives the arm effect alone, anything else neither.
  if (xc4c_ != 0 && xc4c_ != 1) {                                   // 0x00001D20, 0x00001D28
    return;
  }
  if (xc4c_ == 1 && xc40_chargeEffect.get() != nullptr) {           // 0x00001D18, 0x00001D30..0x00001D38
    x1C0C_DriveEffect(*xc40_chargeEffect, dt);                      // 0x00001D3C..0x00001E00
  }
  if (xc3c_armEffect.get() != nullptr) {                            // 0x00001E04..0x00001E0C
    x1C0C_DriveEffect(*xc3c_armEffect, dt);                         // 0x00001E10..0x00001ED4
  }
}

// One effect's block of fn_9_1C0C (the two blocks at 0x00001D3C and 0x00001E10 are the same):
// emission on (vtable +0x2c), the orientation (+0x14) and the global translation (+0x20) from
// the locator at guest this+0xC62's transform, the global scale (+0x24) from the module's own
// scale at guest this+0xC44 on all three axes, and the update (+0x0c) by the frame time.
void CCommandoPirate::x1C0C_DriveEffect(CElementGen& effect, float dt) {
  effect.SetParticleEmission(true);                                 // 0x00001E20
  const CTransform4f xf = GetLctrTransform(mLocatorLWrist);         // 0x00001E30
  effect.SetOrientation(xf.GetRotation());                          // 0x00001E64
  effect.SetGlobalTranslation(xf.GetTranslation());                 // 0x00001E68..0x00001E94
  effect.SetGlobalScale(CVector3f(xc44_, xc44_, xc44_));            // 0x00001E98..0x00001EBC
  effect.Update(dt);                                                // 0x00001ED4
}

// fn_9_3E1C (0x324 bytes): the module's rag-doll driver, called from the actor's Think (the
// call at 0x0000CAA4). Its third argument says whether the rag doll is gone: the guest's Think
// computes it from the rag doll pointer at guest this+0xBFC (the cntlzw/srwi pair at
// 0x0000C97C..0x0000C984 gives one when the pointer is null) and passes it at 0x0000CAA0. That
// is the guest's null test for the rag doll here (0x00003E3C..0x00003E4C); the same pointer is
// tested for null in the guest's PreThink (0x0000CAEC..0x0000CAF4).
//
// With a rag doll the guest drives it: the rag doll's own flag byte at +0x70 decides between the
// water check (its 0x40 bit at 0x00003E58, the InFluidId calls at 0x00003ED4 and 0x00003EF4, the
// water script object's own flag byte at 0x00003F1C and its trigger bounds at 0x00003F2C) and
// the "rigid" path (the rag doll's own update at 0x00003E6C, the identity transform at
// 0x00003E9C and the body's playback rate at 0x00003EB8). The retail's own GetRagDoll (fn_9_78)
// answers null and the port keeps that, so the host has no rag doll for this actor and that whole
// part is a stand-in.
//
// Without a rag doll the guest runs the drowning timer: the module's float at guest this+0xC00
// falls by the frame time while it is above zero (0x00003FF8..0x0000401C); when it reaches zero
// the guest creates the rag doll (0x00004020..0x0000410C) and clears the timer
// (0x00004110..0x00004118). The host cannot create the retail's rag doll, so that is a
// stand-in too.
void CCommandoPirate::x3E1C_UpdateRagDoll(CStateManager& mgr, float dt) {
  if (GetRagDoll() != nullptr) {                                   // 0x00003E3C..0x00003E4C
    StandIn("the module's rag doll (fn_9_3E1C)");                  // 0x00003E50..0x00003FF0
    return;
  }
  if (xc00_ <= 0.f) {                                              // 0x00003FF8..0x00004000
    return;
  }
  xc00_ -= dt;                                                     // 0x00004008..0x0000400C
  if (xc00_ > 0.f) {                                               // 0x00004010..0x0000401C
    return;
  }
  StandIn("the module's rag doll (fn_9_3E1C)");                    // 0x00004020..0x0000410C
  xc00_ = 0.f;                                                     // 0x00004110..0x00004118
}

// fn_9_1F88 (0xd4 bytes): the kUE_Activate event's helper (jump-table entry 29; the name predates
// that) - the shield's sound and scale.
// While the flag byte at guest this+0xC64 has its 0x01 bit it does nothing (the clrlwi at
// 0x00001FA0 clears the byte's low bit); otherwise it sets the 0x20 bit of the byte at guest this+0xC66 (the mask
// 1u << (31 - 26) = 0x20, 0x00001FB0), adds the sound emitter whose id is the module's shield
// "turn on" sound (the block's +0x158, guest this+0x918) when the handle at guest this+0xC38 is
// null (0x00001FB8..0x00001FF0), writes the module's state word at guest this+0xC4C (1 when the
// byte at guest this+0xC65 has its 0x04 bit, 0 when it has its 0x08 bit and -1 otherwise - the
// tests at 0x00001FF4..0x00002018), the constant rodata_42C (0.1f) into guest this+0xC44 and the
// model's scale (the model data's first float, the object at guest this+0x60) into guest
// this+0xC48 (0x00002018..0x00002030).
void CCommandoPirate::x1F88_EventStart() {
  if ((xc64_ & 0x01) != 0) {                                           // 0x00001F9C..0x00001FA4
    return;
  }
  xc66_ |= 0x20;                                                       // 0x00001FA8..0x00001FB4
  if (xc38_ == CSfxHandle::NullHandle()) {                             // 0x00001FB8..0x00001FC0
    xc38_ = CSfxManager::AddEmitter(mData.mShield.mSoundTurnOn, GetTranslation(), 0x7f,
                                    GetCurrentAreaId().Value(), true, true,  // 0x00001FC4..0x00001FE8
                                    CSfxManager::kMedPriority);
  }
  xc4c_ = (xc65_ & 0x04) != 0 ? 1 : ((xc65_ & 0x08) != 0 ? 0 : -1);    // 0x00001FF4..0x00002018
  xc44_ = 0.1f;                                                        // rodata_42C, 0x00002018..0x00002024
  xc48_ = GetModelData()->GetScale().GetX();                           // 0x00002028..0x00002030
}

// fn_9_1EF8 (0xd4 bytes): the kUE_Deactivate event's helper (entry 30) - the counterpart of
// fn_9_1F88. While the flag byte at guest this+0xC64 has its 0x01 bit it does nothing (the clrlwi at 0x00001F10
// clears the byte's low bit); otherwise it removes the sound emitter whose handle is at guest this+0xC38, clears that handle
// and adds the emitter whose id is the module's shield "turn off" sound (the block's +0x15A,
// guest this+0x91A) without keeping its handle (0x00001F18..0x00001F60), and zeroes the floats at
// guest this+0xC48 and guest this+0xC50 (0x00001F64..0x00001F70).
void CCommandoPirate::x1EF8_Activate() {
  if ((xc64_ & 0x01) != 0) {                                           // 0x00001F0C..0x00001F14
    return;
  }
  if (xc38_ != CSfxHandle::NullHandle()) {                             // 0x00001F18..0x00001F20
    CSfxManager::RemoveEmitter(xc38_);                                  // 0x00001F24..0x00001F30
    xc38_ = CSfxHandle::NullHandle();                                   // 0x00001F34..0x00001F3C
    CSfxManager::AddEmitter(mData.mShield.mSoundTurnOff, GetTranslation(), 0x7f,
                            GetCurrentAreaId().Value(), true, false,    // 0x00001F40..0x00001F60
                            CSfxManager::kMedPriority);
  }
  xc48_ = 0.f;                                                         // 0x00001F64..0x00001F6C
  xc50_ = 0.f;                                                         // 0x00001F70
}

// vtable slot 0x84 = fn_9_BF44 (0x2b0): what the blade and the shield hit.
// The run in area 59 (2026-10-21) reaches only the early return: the shield's own arms need the
// 0x04 bit of the byte at guest this+0xC65, and the module's ArmShield state (fn_9_86B4) and its
// shield-charge states are what set it - until those are ported, this body is not marked.
//
// fn_9_BF44 (0x2b0 bytes) runs the base's own CollidedWith first (the call at 0x0000BF70) and then
// the module's own: while the flag byte at guest this+0xC65 has its 0x04 bit (the test at
// 0x0000BF78) the actor is the shield, and a collision with an object (an id that is not
// kInvalidUniqueId, the compare at 0x0000BF8C) applies the shield's charge damage (the damage
// info at guest this+0x898, the block's +0xD8) with the material filter that includes kMT_Solid
// (the filter's words at 0x0000C0B0..0x0000C0F0 and the type 3 at 0x0000C100) and the actor's own
// direction (the transform's second column, the loads at 0x0000BFBC..0x0000BFC4) - but only when
// the object is NOT a player: the guest casts the object to a CPlayer (the call at 0x0000BFA4)
// and the compare at 0x0000BFA8 branches over the damage when the cast gave a player. The flag
// byte at guest this+0xC64 then gets its 0x02 bit (the mask 1u << (31 - 30) = 0x02, the store at
// 0x0000C048) and the function returns.
//
// A collision with no object (the kInvalidUniqueId case at 0x0000C050) is the module's own
// collision actor's: the module's material filter (the guest's lbl_9_bss_20, built once, the flag
// lbl_9_bss_1C) is the two material bits kMT_Ceiling and kMT_Wall (the guest's __shl2i shifts 1
// by the material id, 0x1D and 0x1E, 0x0000C060..0x0000C0E8) and the collision list is walked
// (0x0000C120): an entry whose left material list shares a bit with that filter is the shield's
// own hit (the flag bit is set and the entry's left normal is stored, 0x0000C178..0x0000C19C), an
// entry whose left normal is steeper than the actor's up axis (the dot product with the
// transform's third column at 0x0000C158..0x0000C16C, against rodata_4E8 = 0.707) is a hit too,
// and an entry whose left material list has kMT_Floor (bit 31, the mask the load at 0x0000C108
// builds, against the list's low word at +0x34, the test at 0x0000C1A8..0x0000C1C8) only stores
// the normal.
void CCommandoPirate::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                                   CStateManager& mgr) {
  CPatterned::CollidedWith(id, list, mgr);                       // 0x0000BF70
  if ((xc65_ & 0x04) == 0) {                                     // 0x0000BF74..0x0000BF7C
    return;
  }
  if (id != kInvalidUniqueId) {                                  // 0x0000BF80..0x0000BF90
    // The guest's CStateManager::GetObjectById is the host's CStateManager::ObjectById (the host
    // kept the name for the const overload); both look the object up in the all-objects list.
    CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(id));  // 0x0000BF94..0x0000BFA8
    if (player == nullptr) {                                      // 0x0000BFA8..0x0000BFAC
      mgr.ApplyDamage(GetUniqueId(), id, GetUniqueId(), mData.mShield.mChargeDamage,
                      CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
                      GetTransform().GetColumn(kDY));             // 0x0000BFB0..0x0000C038
    }
    xc64_ |= 0x02;                                               // 0x0000C03C..0x0000C048
    return;                                                      // 0x0000C04C
  }
  if (!sShieldFilterBuilt) {                                     // 0x0000C050..0x0000C05C
    sShieldFilter = (u64(1) << kMT_Ceiling) | (u64(1) << kMT_Wall);  // 0x0000C060..0x0000C0E8
    sShieldFilterBuilt = true;                                   // 0x0000C0DC
  }
  const int count = list.GetCount();                             // 0x0000C0EC..0x0000C118
  for (int i = 0; i < count; ++i) {                              // 0x0000C120
    const CCollisionInfo& info = list[i];
    const u64 mats = info.GetMaterialLeft().GetValue();           // 0x0000C128..0x0000C130
    if ((sShieldFilter & mats) != 0) {                            // 0x0000C134..0x0000C154
      xc64_ |= 0x02;                                              // 0x0000C178..0x0000C184
      xbb8_ = info.GetNormalLeft();                               // 0x0000C184..0x0000C19C
      return;
    }
    const float steep =
        CVector3f::Dot(GetTransform().GetColumn(kDZ), info.GetNormalLeft());  // 0x0000C158..0x0000C16C
    if (steep < 0.707f) {                                         // rodata_4E8, 0x0000C170..0x0000C174
      xc64_ |= 0x02;                                              // 0x0000C178..0x0000C184
      xbb8_ = info.GetNormalLeft();                               // 0x0000C184..0x0000C19C
      return;
    }
    if ((mats & (u64(1) << kMT_Floor)) != 0) {                     // 0x0000C1A4..0x0000C1BC
      xbb8_ = info.GetNormalLeft();                               // 0x0000C1C0..0x0000C1C8
    }
  }
}

// vtable slot 0x9c = fn_9_C300 (0x1fc).
// fn_9_C300 (0x1fc bytes): the KnockBack override (the vtable slot 0x9C).
//
// The guest's first branch (0x0000C324..0x0000C3AC) is the rag-doll one: it wants the block's
// flag byte at guest this+0x91C to have its 0x01 bit and the module's own rag doll at guest
// this+0xBFC to be there. The rag doll is always null here (the constructor stores 0 there and
// the module's GetRagDoll returns null), so the guest always takes the path below and the impulse
// it would give the rag doll never happens.
//
// The knock-back itself (0x0000C3C0..0x0000C3EC): the module's own IsOnGround (the vtable slot
// 0x110, fn_9_90 - the bit 3 of the byte at guest this+0x34C) enables the knock-back reaction
// (the reaction 2 = kAR_KnockBack) and the base's own KnockBack runs.
//
// Then the sounds (0x0000C3F0..0x0000C4D4): while the actor is alive (the byte at guest
// this+0x420's 0x40 bit) and the knock-back manager's active reaction flags (the word at guest
// this+0x630, the manager's +0x10) are 3 (kAR_Hurled), the state machine goes to the "GetUp"
// state (the guest's string at rodata_504 + 0x352) and the block's "hurled" sound (the halfword
// at guest this+0x7C2) is played. When the actor is dead the block's "death" sound (the halfword
// at guest this+0x7C4) is played instead, unless the knock-back manager's follow-up (the word at
// guest this+0x634, the manager's +0x14) is one of the four death follow-ups (7, 8, 9 and 10).
void CCommandoPirate::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  KnockBackController().EnableAnimReaction(CKnockBackMgr::kAR_KnockBack, IsOnGround());  // 0x0000C3D4..0x0000C3DC
  CPatterned::KnockBack(mgr, info);                                                        // 0x0000C3EC
  if (GetAlive()) {                                                                        // 0x0000C3F0..0x0000C3F8
    if (KnockBackController().GetActiveReactionFlags() == 3) {                              // 0x0000C3FC..0x0000C404
      StateMachineState().SetState(mgr, *this, rstl::string_l("GetUp"));                 // 0x0000C40C..0x0000C43C
      xc04_ = CSfxManager::AddEmitter(mData.mSoundHurled, GetTranslation(), 0x7f,           // 0x0000C448..0x0000C46C
                                      GetCurrentAreaId().Value(), true, false,
                                      CSfxManager::kMedPriority);
    }
  } else if (KnockBackController().GetActiveReactionFlags() == 3) {                         // 0x0000C47C..0x0000C484
    const CKnockBackMgr::EFollowUp followUp = KnockBackController().GetFollowUp();          // 0x0000C48C
    if (followUp != CKnockBackMgr::kFU_ExplodeDeath &&                                      // 0x0000C490..0x0000C4AC
        followUp != CKnockBackMgr::kFU_IceDeath && followUp != CKnockBackMgr::kFU_BurnDeath &&
        followUp != CKnockBackMgr::kFU_LaggedBurnDeath) {
      xc04_ = CSfxManager::AddEmitter(mData.mSoundDeath, GetTranslation(), 0x7f,             // 0x0000C4B0..0x0000C4D4
                                      GetCurrentAreaId().Value(), true, false,
                                      CSfxManager::kMedPriority);
    }
  }
}

// vtable slot 0xa8 = fn_9_A4 (0x8): `li r3, 0x1`, so the actor can always be shot.
bool CCommandoPirate::CanBeShot(const CStateManager& mgr, int attributes) { return true; }

// @ fn_9_8
// vtable slot 0xac = fn_9_8 (0x8): `li r3, 0x1`, so the actor always listens.
// One arm. Run: tools/cov/run.sh with tools/cov/specs/commandopirate.py and
// tools/drivers/commandopirate_core.py, area 59 (05_temple_federationsite_dark) and 05_swamp_timber!:
// 50 hits (capped) in each.
bool CCommandoPirate::IsListening() const { return true; }

// vtable slot 0xb0 = fn_9_BE4C (Listen): PortCCommandoPirateCore.cpp.

// @ fn_9_D474
// vtable slot 0xdc = fn_9_D474 (0x8): `addi r3, r3, 0xa14`, the projectile info at guest +0xa14.
// One arm. Run: tools/cov/run.sh with tools/cov/specs/commandopirate.py and
// tools/drivers/commandopirate_core.py, area 59 (05_temple_federationsite_dark) and 05_swamp_timber!:
// 50 hits (capped) in each.
CProjectileInfo* CCommandoPirate::ProjectileInfo() { return &mProjectileInfo; }

// @ fn_9_0
// vtable slot 0xe0 = fn_9_0 (0x8): `addi r3, r3, 0x928`, the path search at guest +0x928.
// One arm. Run: tools/cov/run.sh with tools/cov/specs/commandopirate.py and
// tools/drivers/commandopirate_core.py, area 59 (05_temple_federationsite_dark) and 05_swamp_timber!:
// 50 hits (capped) in area 59.
CPathFindSearch* CCommandoPirate::GetSearchPath() { return &mPathFindSearch; }

// vtable slot 0xe4 = fn_9_70 (0x8): `li r3, 0x0` - this actor has no point-based path.
CPathFindPointSearch* CCommandoPirate::GetPointSearchPath() { return nullptr; }

// @ fn_9_C1F4
// fn_9_C1F4 (0xd8 bytes): the contact damage (vtable slot 0xe8). The guest's flag byte at guest
// this+0xC65 decides which damage the actor gives: its 0x04 bit (the extrwi at 0x0000C20C, the
// mask 1u << (31 - 29) = 0x04) takes the shield's charge damage (the block's +0xD8 = guest
// this+0x898), its 0x20 bit (the extrwi at 0x0000C260, the mask 1u << (31 - 26) = 0x20) takes the
// blade damage (the block's +0x0C = guest this+0x7CC), and without either of them the base's own
// contact damage stands (the call at 0x0000C2B4). The guest copies the damage into the base's
// member and returns it; the port returns it directly (the host's base getter is a const one).
CDamageInfo CCommandoPirate::GetContactDamage() const {
  if ((xc65_ & 0x04) != 0) {                            // 0x0000C20C..0x0000C210
    return mData.mShield.mChargeDamage;                // the block's +0xD8, guest this+0x898
  }
  if ((xc65_ & 0x20) != 0) {                            // 0x0000C260..0x0000C264
    return mData.mBladeDamage;                          // the block's +0x0C, guest this+0x7CC
  }
  return CPatterned::GetContactDamage();                // 0x0000C2B4
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
// run: area 59 (05_temple_federationsite_dark), tools/cov/run.sh with
// tools/cov/specs/commandopirate_triggers.py, 2026-10-21
// @ fn_9_BDCC
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

// Run: cov 2026-10-21 (area 59, 05_temple_federationsite_dark, the warpout driver): every arm -
// the gcov report gives 7/7 lines and 2/2 branches (the ray clear and the path finder).
// @ fn_9_3338
// fn_9_3338 (0x148 bytes): the module's jump-back helper, which both the jump-back trigger
// (fn_9_BB10) and the dodge picker (fn_9_3678) call. The ray starts at the actor's bounding
// box centre (the GetBoundingBox call at 0x00003374 and the GetCenterPoint call at
// 0x00003380) and ends at the point that distance along the given vector (the adds at
// 0x000033B0..0x000033D8); the end point must be clear of the world (the RayCollideWorld call at
// 0x0000342C - the answer is false when the ray is blocked) and must be on the path (the OnPath
// call at 0x00003440 - the answer is true when it returns 0).
bool CCommandoPirate::x3338_IsJumpBackClear(CStateManager& mgr, const CVector3f& dir,
                                           float distance) const {
  const CVector3f center = GetBoundingBox().GetCenterPoint();  // 0x00003374, 0x00003380
  const CVector3f end = center + distance * dir;                // 0x000033B0..0x000033D8
  // The guest's material filter (the two __shl2i calls at 0x000033DC and 0x000033F8) includes
  // the material 59 (kMT_Solid - the module's .data word at lbl_9_data_80C) and excludes the
  // material 47 (kMT_CollisionActor - the .data word at lbl_9_data_810); its type is
  // kFT_IncludeExclude (the word 3 at 0x0000341C).
  const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid), CMaterialList(kMT_CollisionActor));  // 0x000033DC, 0x000033F8
  // The guest's answer is false when the ray is blocked (the branch at 0x00003434 takes the false
  // path when the ray's answer is false); the path check runs only when the ray is clear
  // (0x00003438..0x00003440).
  if (!mgr.RayCollideWorld(center, end, filter, this)) {
    return false;                                                // 0x00003430..0x00003434
  }
  return mPathFindSearch.OnPath(end) == CPathFindSearch::kR_Success;  // 0x00003440
}

// fn_9_BB04: the block's flag byte at +0x15C (guest this+0x91C) has its 0x80 bit set (extrwi
// r3, r0, 1, 24 at 0x0000BB08). That byte is the loader's mFlags0 - the "sound" property's bits
// 14, 11, 8, 5, 2, 31, 28 and 25 (the data ctor fn_9_EDC0 packs them at 0x0000EED4..0x0000EF30)
// - so the flag is script-supplied.
// run: area 59 (05_temple_federationsite_dark), the coverage build (build-port-cov) with
// tools/cov/run.sh, 2026-10-21: the trigger answers false in the run, and its true arm is dead -
// the block's flag byte at guest this+0x91C (the loader's mFlags0, the "sound" property's bit 14)
// has its 0x80 bit clear in every shipped CommandoPirate - the scan of the nine placements gives
// the sound words 0x80, 0x300, 0x114 and 0x104, and none of them has bit 14 set.  The byte comes
// only from the loader (the module never writes it), so the true arm is dead.
// @ fn_9_BB04
bool CCommandoPirate::ShouldAmbush(CStateManager& mgr, const CTriggerData& data) const {
  return (mData.mFlags0 & 0x80) != 0;
}

// fn_9_BAF8: the block's flag byte at +0x15C (guest this+0x91C) has its 0x40 bit set (extrwi
// r3, r0, 1, 25 at 0x0000BAFC). That byte is the loader's mFlags0 - the "sound" property's bits
// 14, 11, 8, 5, 2, 31, 28 and 25 (the data ctor fn_9_EDC0 packs them at 0x0000EED4..0x0000EF30)
// - so the flag is script-supplied.
// run: area 59 (05_temple_federationsite_dark), the coverage build (build-port-cov) with
// tools/cov/run.sh, 2026-10-21: the trigger answers false in the run, and its true arm is dead -
// the block's flag byte at guest this+0x91C (the loader's mFlags0, the "sound" property's bit 11)
// has its 0x40 bit clear in every shipped CommandoPirate - the scan of the nine placements gives
// the sound words 0x80, 0x300, 0x114 and 0x104, and none of them has bit 11 set.  The byte comes
// only from the loader (the module never writes it), so the true arm is dead.
// @ fn_9_BAF8
bool CCommandoPirate::BreakAmbush(CStateManager& mgr, const CTriggerData& data) const {
  return (mData.mFlags0 & 0x40) != 0;
}

// fn_9_BAEC: the module's byte at guest this+0xAE8 has its 0x80 bit set (extrwi r3, r0, 1, 24
// at 0x0000BAF0).  The byte has three accesses in the REL and only one of them is a store: the
// read-modify-write at 0x00005B70..0x00005B7C (the module clears the 0x80 bit there) - the other
// two are reads (0x0000AA94 and the trigger's own load at 0x0000BAEC).  Nothing sets the bit, and
// the constructor leaves the byte at 0 (the port's xae8_(0)), so the true arm is dead.
// run: area 59 (05_temple_federationsite_dark), the coverage build (build-port-cov) with
// tools/cov/run.sh, 2026-10-21: the trigger answers false in the run, and its true arm is dead -
// nothing in the module writes the byte at guest this+0xAE8, so its 0x80 bit is clear.
// @ fn_9_BAEC
bool CCommandoPirate::HasLineOfSight(CStateManager& mgr, const CTriggerData& data) const {
  return (xae8_ & 0x80) != 0;
}

// fn_9_BAD0: the actor is under fire while the float at guest +0xB64 is below 0.75f
// (lbl_9_rodata_4E0, loaded at 0x0000BAD8). The compare is fcmpo cr0, f1, f0 (0x0000BADC) and the
// return takes cr0.LT (mfcr r0 at 0x0000BAE0, srwi r3, r0, 31 at 0x0000BAE4). The constructor
// sets that member to 0.75f, so the test is false until something lowers it.
// run: area 59 (05_temple_federationsite_dark), the coverage build (build-port-cov) with
// tools/cov/run.sh (the false answer) and tools/cov/force.sh with
// tools/cov/specs/commandopirate_forced.py (the true answer, the float at guest this+0xB64 is
// written below 0.75), 2026-10-21.
// @ fn_9_BAD0
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
// run: area 59 (05_temple_federationsite_dark) and the Cliff's entrance, the coverage build
// (build-port-cov) with tools/cov/run.sh and the gdb spec tools/cov/specs/commandopirate_triggers.py,
// 2026-10-21: the gcov report has no branch in the body and the trigger answers true and false
// (the gdb spec's ret counts: true 269 times in area 59, false 295 times in the Cliff).
// @ fn_9_BA78
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

// @ fn_9_B958
// fn_9_B958 (0x38 bytes): the commando is off its path when its own position is not on the path
// search's path. The guest asks its own CPathFindSearch (the one at guest this+0x928) about its
// own position (the translation at this+0x54, the call at 0x0000B970) and answers true when the
// answer is not kR_Success - the neg/or/srwi pair at 0x0000B974..0x0000B97C is the != 0 idiom.
bool CCommandoPirate::IsOffPath(CStateManager& mgr, const CTriggerData& data) const {
  return mPathFindSearch.OnPath(GetTranslation()) != CPathFindSearch::kR_Success;  // 0x0000B970
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
// run: area 59 (05_temple_federationsite_dark), tools/cov/run.sh (the angle arms) and
// tools/cov/force.sh (the early return) with tools/cov/specs/commandopirate_triggers.py and
// tools/cov/specs/commandopirate_isfacing.py, 2026-10-21
// @ fn_9_B818
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
// run: area 59 (05_temple_federationsite_dark) and the Cliff's entrance, the coverage build
// (build-port-cov) with tools/cov/run.sh and the gdb spec tools/cov/specs/commandopirate_triggers.py,
// 2026-10-21: the gcov report has no branch in the body and the trigger answers true and false
// (the gdb spec's ret counts: true twice in the Cliff, false 121 times in area 59).
// @ fn_9_B7FC
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

// fn_9_B250 (0x410 bytes): the commando charges with its shield when it is aggressive, the
// The run in area 59 (2026-10-21) reaches only the first gate: the 0x40 bit of the byte at guest
// this+0xC64 is set by the module's own timer helper (fn_9_662C) when the aggressiveness roll
// wins, and the counter at guest this+0xB54 that the gate also asks about is raised only by the
// module's shield-charge states (fn_9_7A98 and fn_9_8028) - until those are ported, this body
// is not marked.
// script does not forbid the charge, the target is inside the charge band and the way to it is
// clear.
//
// The flag byte at guest this+0xC64 must have its 0x40 bit (the extrwi at 0x0000B2B8), the
// block's flag byte at +0x15D (guest this+0x91D, mData.mFlags1) must not have its 0x40 bit
// (0x0000B2C0) and that same byte's 0x01 bit must be clear (the clrlwi at 0x0000B2CC). The
// module's counter at guest this+0xB50 (the number of shots the burst fire has fired - fn_9_6390
// raises it at 0x00006594) must be above the module's counter at guest this+0xB54 (the number of
// shield charges - the two shield charge states raise it at 0x00007B78 and 0x000080E4); the
// guest compares them unsigned (the cmplw at 0x0000B2DC).
//
// The target (the id at guest this+0xBCA) must be there (the GetObjectById call at 0x0000B2F4
// and its own null test at 0x0000B2F8), the height difference must be at most 5.0f
// (lbl_9_rodata_468, the fabs and the compare at 0x0000B324..0x0000B348) and the squared
// distance must be inside the charge band: the block's +0x124 and +0x128 (guest this+0x8E4 and
// this+0x8E8) are the shield's minimum and maximum charge distance - the loader's shield half
// starts at the block's +0xD8 and its own minimum and maximum charge distance are at +0x4C and
// +0x50 (fn_9_EF80 stores them at 0x0000F064 and 0x0000F06C). The delta must be magnitude-safe
// (0x0000B38C..0x0000B394).
//
// Then the team: when the module's team id at guest this+0xBC4 is not kInvalidUniqueId the team
// AI manager must let the commando start a melee attack (the CanStartAttack call at 0x0000B3D0:
// the attack type 0 - the Dark Commando passes the same type for its melee ask at 0x000060D4 -
// the team id and the actor's own id at guest this+0x8).
//
// Finally the way to the target must be clear: three rays from the actor's own bounding box (the
// centre, and the centre plus and minus the box's x extent times the transform's right column -
// the guest's this+0x24, +0x34 and +0x44, the transform's m00, m10 and m20) along the direction
// to the target, each as long as the distance to it, must all hit the world (the calls at
// 0x0000B548, 0x0000B598 and 0x0000B5E8). The guest's material filter includes the material 59
// (kMT_Solid) and excludes the materials 32 (kMT_Player) and 49 (kMT_Platform) - the module's
// data words at lbl_9_data_7D0, lbl_9_data_7D4 and lbl_9_data_7D8, the two __shl2i calls at
// 0x0000B3F0 and 0x0000B40C, the filter type 3 (kFT_IncludeExclude).
bool CCommandoPirate::ShouldShieldCharge(CStateManager& mgr, const CTriggerData& data) const {
  if ((xc64_ & 0x40) == 0) {                                       // 0x0000B2B8..0x0000B2BC
    return false;
  }
  if ((mData.mFlags1 & 0x40) != 0) {                               // 0x0000B2C0..0x0000B2C8
    return false;
  }
  if ((xc64_ & 0x01) != 0) {                                       // 0x0000B2CC..0x0000B2D0
    return false;
  }
  if (xb50_ <= xb54_) {                                            // 0x0000B2D4..0x0000B2E0
    return false;
  }
  const CEntity* target = mgr.GetObjectById(xbca_);                // 0x0000B2F4
  if (target == nullptr) {                                         // 0x0000B2F8..0x0000B2FC
    return false;
  }
  // The guest reads the target's position (the guest's this+0x54) without a cast; the host's
  // CEntity has no position, so the port casts to CActor.
  const CActor* actor = TCastToConstPtr< CActor >(target);
  if (actor == nullptr) {
    return false;
  }
  const CVector3f delta = actor->GetTranslation() - GetTranslation();
  if (CMath::AbsF(delta.GetZ()) > 5.f) {                           // 0x0000B324..0x0000B348
    return false;
  }
  const float distSq = delta.MagSquared();                         // 0x0000B34C..0x0000B36C
  const float minSq =
      mData.mShield.mChargeMinAttackDist * mData.mShield.mChargeMinAttackDist;  // guest +0x8e4
  const float maxSq =
      mData.mShield.mChargeMaxAttackDist * mData.mShield.mChargeMaxAttackDist;  // guest +0x8e8
  if (distSq < minSq || distSq > maxSq) {                          // 0x0000B370..0x0000B384
    return false;
  }
  if (!delta.IsMagnitudeSafe()) {                                  // 0x0000B38C..0x0000B394
    return false;
  }
  if (xbc4_teamId != kInvalidUniqueId &&
      !CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Melee, mgr, xbc4_teamId,
                                         GetUniqueId())) {           // 0x0000B398..0x0000B3D8
    return false;
  }
  const CVector3f dir = delta.AsNormalized() * CMath::SqrtF(distSq);  // 0x0000B450..0x0000B4F4
  const CAABox box = GetBoundingBox();                             // 0x0000B474
  const CVector3f center = box.GetCenterPoint();                   // 0x0000B498
  const float extent = box.GetMaxPoint().GetX() - box.GetMinPoint().GetX();  // 0x0000B478..0x0000B488
  const CVector3f offset = extent * GetTransform().GetRight();     // 0x0000B49C..0x0000B4B0
  const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid), CMaterialList(kMT_Player, kMT_Platform));
  if (!mgr.RayCollideWorld(center, center + dir, filter, this)) {  // 0x0000B548
    return false;
  }
  if (!mgr.RayCollideWorld(center + offset, center + offset + dir, filter, this)) {  // 0x0000B598
    return false;
  }
  if (!mgr.RayCollideWorld(center - offset, center - offset + dir, filter, this)) {  // 0x0000B5E8
    return false;
  }
  return true;                                                     // 0x0000B5F4
}

// @ fn_9_B1E0
// fn_9_B1E0 (0x70 bytes): the trigger that enters the module's scripted shield charge.
//
// The byte at guest this+0xC66's 0x80 bit (the extrwi at 0x0000B1F0) and the byte at guest
// this+0xC64's 0x01 bit (the clrlwi at 0x0000B1FC) both have to be right, and the AI waypoint the
// module follows (the id at guest this+0x6C0 - the waypoint navigation's destination field) has to
// allow the boost: the waypoint's flags (the word at the waypoint's +0x164) have to have their
// 0x20 bit (the rlwinm at 0x0000B22C).
//
// The three arms the run does not reach are unreachable in the shipped data: the FollowAttackPattern
// state (fn_9_961C) sets the 0x80 bit of guest this+0xC66 and the waypoint on its activate
// (0x000096CC and 0x00009754), and this trigger is only evaluated while that state is current;
// nothing in the module ever sets the 0x01 bit of guest this+0xC64 (the constructor clears it at
// 0x0000E1AC and every store to that byte sets another bit - the 0x02, 0x04, 0x08, 0x10, 0x20,
// 0x40 or 0x80 one).
bool CCommandoPirate::ShouldBoost(CStateManager& mgr, const CTriggerData& data) const {
  if ((xc66_ & 0x80) == 0) {  // 0x0000B1F0
    return false;
  }
  if ((xc64_ & 0x01) != 0) {  // 0x0000B1FC
    return false;
  }
  const CScriptAIWaypoint* waypoint =
      TCastToPtr< CScriptAIWaypoint >(
          mgr.ObjectById(mWaypointNavigation.GetDestination()));  // 0x0000B218..0x0000B224
  if (waypoint == nullptr) {
    return false;
  }
  if ((waypoint->GetFlags() & 0x20) == 0) {  // 0x0000B22C
    return false;
  }
  return true;
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
// run: area 59 (05_temple_federationsite_dark), the coverage build (build-port-cov) with
// tools/cov/run.sh (the false answer) and tools/cov/force.sh with
// tools/cov/specs/commandopirate_forced.py (the true answer, the jump point id at guest
// this+0xBD0 is written valid), 2026-10-21.
// @ fn_9_B1A8
bool CCommandoPirate::FoundJumpPoint(CStateManager& mgr, const CTriggerData& data) const {
  return xbd0_ != kInvalidUniqueId;
}

// fn_9_B168 (0x40 bytes): the ShouldCrouch trigger. The cover point the module holds (the id at
// guest this+0xBD2, 0x0000B178) is looked up (the GetCoverPoint call at 0x0000B180); when there
// is none the trigger answers false (0x0000B184..0x0000B194), otherwise the cover point's own
// answer (the ShouldCrouch call at 0x0000B18C).
bool CCommandoPirate::ShouldCrouch(CStateManager& mgr, const CTriggerData& data) const {
  const CScriptCoverPoint* cover = GetCoverPoint(mgr, xbd2_);  // 0x0000B178..0x0000B180
  if (cover == nullptr) {                                     // 0x0000B184
    return false;                                            // 0x0000B194
  }
  return cover->ShouldCrouch();                              // 0x0000B18C
}

// fn_9_B114 (0x54 bytes): the ShouldWallHang trigger. The cover point the module holds (the id at
// guest this+0xBD2, 0x0000B124) is looked up (the GetCoverPoint call at 0x0000B130); when there is
// none the trigger answers false (0x0000B134..0x0000B138), otherwise the cover point's own answer
// (the ShouldWallHang call at 0x0000B140) - the guest keeps only the non-zero case
// (0x0000B144..0x0000B14C).
bool CCommandoPirate::ShouldWallHang(CStateManager& mgr, const CTriggerData& data) const {
  const CScriptCoverPoint* cover = GetCoverPoint(mgr, xbd2_);  // 0x0000B124..0x0000B130
  if (cover == nullptr) {                                     // 0x0000B134
    return false;                                            // 0x0000B138
  }
  return cover->ShouldWallHang();                            // 0x0000B140..0x0000B14C
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

// fn_9_AFD4 (CoverBlown): PortCCommandoPirateCore.cpp.

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

// run: area 59 (05_temple_federationsite_dark), the gdb spec tools/cov/specs/commandopirate_triggers.py,
// 2026-10-21: the state is forced with the console's `fsm` and both its arms run (the activate
// 202 times, the update 2 times).
// @ fn_9_ADA0
// fn_9_ADA0 (0x34 bytes): the Ambush state. Only the activate message (the guest's test of 0 at
// 0x0000ADA8) does anything: the body controller's locomotion type becomes the crouch one (the
// type 0, 0x0000ADB8..0x0000ADC0).
void CCommandoPirate::Ambush(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {                     // 0x0000ADA8..0x0000ADB4
    BodyController()->SetLocomotionType(pas::kLT_Crouch);  // 0x0000ADB8..0x0000ADC0
  }
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

// fn_9_A97C (0xa4 bytes): the SetRetreatDest state. Only the activate message (the guest's test
// of 0 at 0x0000A984) does anything: the retreat point (the id at guest this+0xBCE, 0x0000A998) is
// looked up (the GetObjectById call at 0x0000A9A8) and the path find navigation's destination
// becomes that point's position (0x0000A9C4..0x0000A9CC) or the actor's own position when there
// is no such point (0x0000A9E4..0x0000A9EC); the flag byte at guest this+0xC65 loses its 0x80 bit
// (0x0000A9B0..0x0000A9BC) and gets it back when there is a point (0x0000A9D0..0x0000A9DC). The
// navigation's face target id (guest this+0x718) goes to kInvalidUniqueId
// (0x0000A9F0..0x0000A9FC) and the byte at guest this+0xC63 gets its 0x01 bit (0x0000AA00..0x0000AA08).
void CCommandoPirate::SetRetreatDest(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {  // 0x0000A984..0x0000A994
    return;
  }
  const CActor* point = TCastToConstPtr< CActor >(mgr.GetObjectById(xbce_));  // 0x0000A998..0x0000A9A8
  xc65_ &= ~0x80;                                                        // 0x0000A9B0..0x0000A9BC
  if (point != nullptr) {                                               // 0x0000A9C0..0x0000A9C4
    mPathFindNavigation.SetDestination(point->GetTranslation());        // 0x0000A9C4..0x0000A9CC
    xc65_ |= 0x80;                                                      // 0x0000A9D0..0x0000A9DC
  } else {
    mPathFindNavigation.SetDestination(GetTranslation());               // 0x0000A9E4..0x0000A9EC
  }
  mPathFindNavigation.SetFaceTarget(kInvalidUniqueId);                  // 0x0000A9F0..0x0000A9FC
  xc63_ |= 0x01;                                                       // 0x0000AA00..0x0000AA08
}

// fn_9_A8E8 (0x94 bytes): the SetJumpDest state. Only the activate message (the guest's test of
// 0 at 0x0000A8F0) does anything: the jump point (the id at guest this+0xBD0, 0x0000A904) is
// looked up (the GetObjectById call at 0x0000A914) and the path find navigation's destination
// becomes that point's position (0x0000A930..0x0000A938) or the actor's own position when there
// is no such point (0x0000A950..0x0000A958); the flag byte at guest this+0xC65 loses its 0x80 bit
// (0x0000A91C..0x0000A928) and gets it back when there is a point (0x0000A93C..0x0000A948). The
// navigation's face target id (guest this+0x718) goes to kInvalidUniqueId (0x0000A95C..0x0000A964).
void CCommandoPirate::SetJumpDest(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {  // 0x0000A8F0..0x0000A900
    return;
  }
  const CActor* point = TCastToConstPtr< CActor >(mgr.GetObjectById(xbd0_));  // 0x0000A904..0x0000A914
  xc65_ &= ~0x80;                                                       // 0x0000A91C..0x0000A928
  if (point != nullptr) {                                              // 0x0000A92C
    mPathFindNavigation.SetDestination(point->GetTranslation());       // 0x0000A930..0x0000A938
    xc65_ |= 0x80;                                                     // 0x0000A93C..0x0000A948
  } else {
    mPathFindNavigation.SetDestination(GetTranslation());              // 0x0000A950..0x0000A958
  }
  mPathFindNavigation.SetFaceTarget(kInvalidUniqueId);                 // 0x0000A95C..0x0000A964
}

// @ fn_9_A838
// fn_9_A838 (0xb0 bytes): the SetPathMeshDest state. Only the activate message (the guest's test
// of 0 at 0x0000A840) does anything: the module's flag byte at guest this+0xC65 loses its 0x80
// bit (the rlwimi at 0x0000A888), the path find search (the CPathFindSearch at this+0x928) looks
// for the closest reachable point (the call at 0x0000A890 - the guest's out point starts as the
// zero vector, the sZeroVector copy at 0x0000A878..0x0000A880) and the path find navigation (the
// CPathFindNavigation at this+0x6F4) takes that point as its destination (the call at
// 0x0000A8A4). When the search finds no point, the navigation takes the actor's own position
// instead (the call at 0x0000A8C4) and the flag byte's 0x80 bit stays clear. Either way the
// navigation's face target id (guest this+0x718, the navigation's own member at +0x24) is
// cleared (the store at 0x0000A8D0).
void CCommandoPirate::SetPathMeshDest(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {
    return;                                     // 0x0000A840
  }
  xc65_ &= ~0x80;                               // 0x0000A884..0x0000A88C
  CVector3f point = CVector3f::Zero();           // 0x0000A878..0x0000A880
  if (mPathFindSearch.FindClosestReachablePoint(GetTranslation(), point) ==
      CPathFindSearch::kR_Success) {             // 0x0000A890..0x0000A898
    mPathFindNavigation.SetDestination(point);   // 0x0000A89C..0x0000A8A4
    xc65_ |= 0x80;                              // 0x0000A8A8..0x0000A8B4
  } else {
    mPathFindNavigation.SetDestination(GetTranslation());  // 0x0000A8BC..0x0000A8C4
  }
  mPathFindNavigation.SetFaceTarget(kInvalidUniqueId);     // 0x0000A8C8..0x0000A8D0
}

// fn_9_A4BC (MeleeAttack): PortCCommandoPirateCore.cpp.

// fn_9_A2EC (EGrenadeAttack): PortCCommandoPirateCore.cpp.

// fn_9_A110 (PostEGrenadeAttack): PortCCommandoPirateCore.cpp.

// fn_9_9F24 (0x1ec bytes): the JumpPointFind state. Only the activate message (the guest's test
// of 0 at 0x00009F44) does anything: the jump point id (guest this+0xBD0) is cleared (the store
// at 0x00009F64), the timer at guest this+0xB78 has to be at least 2.0f (lbl_9_rodata_44C, the
// compare at 0x00009F6C) and the module's own helper fn_9_3CBC has to answer with the target's
// aim position (the call at 0x00009F84), which it stores at guest this+0xB94 (the argument at
// 0x00009F7C). The timer is then cleared (the store at 0x00009FA0).
//
// The AiWaypoint list (the state manager's +0x838, the load at 0x00009FA4) is scanned for the
// jump point to use: it has to be active (the byte at +0x20's 0x80 bit, the extrwi at
// 0x00009FE4), free (GetInUse with this actor's id, the call at 0x00009FFC), of type 0 (the word
// at +0x15C, the compare at 0x0000A00C), have no jump target (+0x182, the compare at
// 0x0000A020) and be in this area (+0x04, the compare at 0x0000A030). Its current waypoint
// (+0x180, the lookup at 0x0000A04C) has to resolve, and that waypoint has to be nearer to the
// target's aim position than the best candidate so far (the squares at 0x0000A064..0x0000A090,
// the limit lbl_9_rodata_4B8 = FLT_MAX). The path find search (the guest's GetSearchPath, the
// vtable call at 0x0000A0A4) has to connect the actor to the jump point (the PathExists call at
// 0x0000A0B0). The winner's id goes into guest this+0xBD0 (the store at 0x0000A0C4).
void CCommandoPirate::JumpPointFind(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {
    return;                                     // 0x00009F44..0x00009F50
  }
  xbd0_ = kInvalidUniqueId;                     // 0x00009F64
  if (xb78_ < 2.f) {                            // 0x00009F6C..0x00009F74
    return;
  }
  if (!x3CBC_(mgr, xb94_, 0.f)) {                // 0x00009F84..0x00009F8C
    return;
  }
  xb78_ = 0.f;                                  // 0x00009FA0
  float best = 3.4028235e+38f;                   // lbl_9_rodata_4B8, the load at 0x00009F9C
  CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint);          // 0x00009FA4
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    CScriptAiJumpPoint* jumpPoint = TCastToPtr< CScriptAiJumpPoint >(list[i]);
    if (jumpPoint == nullptr || !jumpPoint->GetActive() ||             // 0x00009FD8..0x00009FE8
        jumpPoint->GetInUse(GetUniqueId()) ||                            // 0x00009FFC..0x0000A004
        jumpPoint->GetType() != 0 ||                                    // 0x0000A008..0x0000A010
        jumpPoint->GetJumpTarget() != kInvalidUniqueId ||               // 0x0000A014..0x0000A024
        jumpPoint->GetCurrentAreaId() != GetCurrentAreaId()) {          // 0x0000A028..0x0000A034
      continue;
    }
    const CScriptWaypoint* waypoint = TCastToConstPtr< CScriptWaypoint >(
        mgr.GetObjectById(jumpPoint->GetJumpPoint()));  // 0x0000A038..0x0000A058
    if (waypoint == nullptr) {
      continue;
    }
    const float distSq = (waypoint->GetTranslation() - xb94_).MagSquared();  // 0x0000A05C..0x0000A088
    if (distSq >= best) {                       // 0x0000A08C..0x0000A090
      continue;
    }
    if (mPathFindSearch.PathExists(GetTranslation(), jumpPoint->GetTranslation()) !=
        CPathFindSearch::kR_Success) {          // 0x0000A094..0x0000A0B8
      continue;
    }
    best = distSq;                              // 0x0000A0C0
    xbd0_ = jumpPoint->GetUniqueId();           // 0x0000A0BC..0x0000A0C4
  }
}

// fn_9_9BB4 (JetBoost): PortCCommandoPirateCore.cpp.

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

// fn_9_961C (0x350 bytes): the FollowAttackPattern state - the commando walks the attack
// pattern's waypoints and, at the end of the chain, goes for the target.
//
// The guest's own waypoint navigation patrols first (the call at 0x00009658, the navigation at
// guest this+0x6B8) and the state's own work follows.
//
// On activate (the guest's message 0, 0x00009680) the module's waypoint is the one it is at (the
// id at guest this+0xBDA, the load at 0x00009684) or, when that is kInvalidUniqueId, the object
// connected in the "ATTK"/"FOLW" state (the GetConnectedObject call at 0x000096B8); the id goes to
// guest this+0x6C0 (0x000096CC). Then the waypoint (a CScriptWaypoint, the cast at 0x000096E0)
// is looked up: when it is behind the commando (the delta's projection on the actor's forward
// column - the loads at 0x000096EC..0x0000972C - is zero or less, the compare with rodata_494 =
// 0 at 0x00009730) the navigation's "in position" flag (the byte at guest this+0x6F0, the
// bitfield byte of the navigation at guest this+0x6B8) gets its 0x80 bit (0x0000973C..0x00009748).
// The byte at guest this+0xC66 gets its 0x80 bit too (0x0000974C..0x00009758) - the bit the
// ShouldBoost trigger reads (0x0000B1F0).
//
// On update (the guest's message 1, 0x00009760) the AI waypoint the module follows (the id at
// guest this+0x6C0) is looked up. When it is there and its flags (the word at its +0x164) have
// their 0x02 bit (the rlwinm at 0x00009788) or their 0x04 bit (the rlwinm at 0x00009790), the
// commando's reach is the body's max speed (the GetMaxSpeed call at 0x000097A0) times
// (0.1f + 1.5f * dt) (rodata_42C and rodata_47C) times the model's scale Y (the model data at
// guest this+0x60, the load at 0x000097C0) plus the module's jump distance (the float at guest
// this+0xBE8, the load at 0x000097CC); when the commando is nearer to the waypoint than that (the
// squared distance at 0x000097D0..0x00009828) the navigation's in-position flag is set
// (0x00009830..0x0000983C) and the float at guest this+0xBEC takes 3 (rodata_448) when the
// waypoint's flags have their 0x02 bit and 0 (rodata_494) when they do not
// (0x00009840..0x00009860).
//
// Then, when the body is in the jump state (the body's state id 13, the compare at 0x0000986C)
// and the waypoint has no successor (its "ARRV"/"NEXT" connection, the CheckConnectedObject call
// at 0x0000989C, is kInvalidUniqueId - the same chain the host's waypoint navigation walks), the
// target's aim position (the module's helper fn_9_3CBC, the call at 0x000098D8) becomes the
// body's target vector (the delta to the position, the stores at 0x0000990C..0x00009914).
//
// Finally the navigation's destination position (guest this+0x6D4..0x6DC) is copied to the
// module's jump-back target (guest this+0xBF0, 0x00009918..0x0000992C).
//
// On deactivate (the guest's message 2, 0x00009934) the byte at guest this+0xC66 loses its 0x80
// bit (0x0000993C..0x00009940).
void CCommandoPirate::FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float dt) {
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);  // 0x00009654..0x00009658
  switch (msg) {
  case kStateMsg_Activate: {  // the guest's message 0
    TUniqueId id = xbda_;  // 0x00009684
    if (xbda_ == kInvalidUniqueId) {  // 0x0000968C..0x00009690
      id = GetConnectedObject(mgr, kSS_Attack, kSM_Follow);  // 0x000096B8
    }
    mWaypointNavigation.SetDestination(id);  // 0x000096CC (guest this+0x6C0)
    // The guest's guest this+0x6C0 is the waypoint navigation's own destination field (the
    // navigation sits at guest this+0x6B8 and its destination is at +0x8), so the host's
    // navigation gets it too: the AttackPatternOver trigger reads the destination there
    // (0x0000B800) and the state has to stay when it is set.
    mWaypointNavigation.SetDestination(id);
    const CScriptWaypoint* waypoint =
        TCastToPtr< CScriptWaypoint >(mgr.ObjectById(id));  // 0x000096DC..0x000096E8
    if (waypoint != nullptr) {
      // the delta to the waypoint and the actor's forward column (guest this+0x28, 0x38, 0x48)
      const CVector3f delta = waypoint->GetTranslation() - GetTranslation();  // 0x000096EC..0x00009720
      if (CVector3f::Dot(GetTransform().GetForward(), delta) <= 0.f) {  // rodata_494, 0x00009730
        mWaypointNavigation.SetInPosition(true);  // 0x0000973C..0x00009748
      }
    }
    xc66_ |= 0x80;  // 0x0000974C..0x00009758
    break;
  }
  case kStateMsg_Update: {  // the guest's message 1
    const CScriptAIWaypoint* waypoint =
        TCastToPtr< CScriptAIWaypoint >(
            mgr.ObjectById(mWaypointNavigation.GetDestination()));  // 0x00009774..0x00009780
    if (waypoint != nullptr) {
      const uint flags = waypoint->GetFlags();  // the word at the waypoint's +0x164
      if ((flags & 0x02) != 0 || (flags & 0x04) != 0) {  // 0x00009788, 0x00009790
        const float maxSpeed = BodyController()->BodyStateInfo().GetMaxSpeed();  // 0x000097A0
        const float scaleY = GetModelData()->GetScale().GetY();  // guest this+0x60, 0x000097C0
        // rodata_42C = 0.1f, rodata_47C = 1.5f, the module's jump distance at guest this+0xBE8
        const float reach = maxSpeed * ((0.1f + 1.5f * dt) * scaleY) + xbe8_;  // 0x000097AC..0x000097DC
        const CVector3f delta =
            GetTranslation() - waypoint->GetTranslation();  // 0x000097D0..0x000097F4
        if (delta.MagSquared() < reach * reach) {  // 0x00009824..0x0000982C
          mWaypointNavigation.SetInPosition(true);  // 0x00009830..0x0000983C
          xbec_ = (flags & 0x02) != 0 ? 3.f : 0.f;  // rodata_448 / rodata_494, 0x00009840..0x00009860
        }
      }
    }
    // the body's state: the jump state (the id 13, the compare at 0x0000986C)
    if (BodyController()->GetCurrentStateId() == pas::kAS_Jump) {
      // the chain: the waypoint's "ARRV"/"NEXT" connection (0x0000989C); a waypoint without one
      // is the end of the pattern.
      bool endOfPattern = (waypoint == nullptr);
      if (waypoint != nullptr) {
        endOfPattern =
            waypoint->CheckConnectedObject(mgr, kSS_Arrived, kSM_Next) == kInvalidUniqueId;
      }
      if (endOfPattern) {
        if (x3CBC_(mgr, xb94_, 0.f)) {  // 0x000098D8 (fn_9_3CBC with rodata_494 = 0.f)
          // the body's target vector (the command manager's fifth CVector3f, the body
          // controller's +0x34, the stores at 0x0000990C..0x00009914)
          BodyController()->CommandMgr().SetTargetVector(xb94_ - GetTranslation());
        }
      }
    }
    // the navigation's destination position becomes the module's jump-back target
    xbf0_ = mWaypointNavigation.GetDestinationPosition();  // 0x00009918..0x0000992C
    break;
  }
  case kStateMsg_Deactivate: {  // the guest's message 2
    xc66_ &= ~0x80;  // 0x0000993C..0x00009940
    break;
  }
  default:
    break;
  }
}

// fn_9_9064 (WarpIn): PortCCommandoPirateCore.cpp.

// run: area 59 (05_temple_federationsite_dark), tools/drivers/commandopirate_warpout.py, 2026-10-21
// @ fn_9_8DF0
// fn_9_8DF0 (0x218 bytes): the WarpOut state - the commando plays the generate animation and
// fades out while it does.
//
// The message switch (0x00008E08..0x00008E2C) picks the activate (0), the update (1) and the
// deactivate (2); every other message leaves the state alone.
//
// Activate (0x00008E30): the animation state becomes "ready" (the store at 0x00008E38), the
// module's state word (guest this+0x920) becomes 1 (0x00008E44), the 0x40 bit of the flag byte
// at guest this+0xC65 is cleared (the guest's rlwimi at 0x00008E4C takes the bit 25 from the
// MSB, which is the LSB bit 6) and the cover point the module holds (the id at guest this+0xBD2)
// is released (0x00008E54).
//
// Update (0x00008E5C): the animation state is asked whether the generate command may be issued
// (0x00008E68, the guest's command id 0xc = 12 = kAS_Generate). When it may, the generate
// command goes to the body controller (0x00008EF0): the guest's command carries the module's
// own animation object (lbl_9_data_A2C), the type 1 (pas::kGType_One), the zero target vector,
// the animation id -1 and the three flags clear. The host's command has no room for the
// animation object, so the port delivers the host's CBCGenerateCmd with the type 1 and the id
// -1 (the host's own defaults fill the rest). The guest also copies the command into the base's
// field at guest this+0x20C (0x00008F04..0x00008F3C), which the host has no member for; the
// host's command manager keeps the command instead.
//
// When the command may not be issued the guest looks at the body's own state (0x00008F4C): only
// while it is the generate state (12) does the fade run. The first time (the 0x40 bit of the
// flag byte at guest this+0xC65 is clear - the guest's extrwi at 0x00008F5C takes the bit 25
// from the MSB) the flag is set and the body's remaining animation time is kept (guest
// this+0xC58, 0x00008F74..0x00008F78). Afterwards, while the kept time is above the epsilon
// (rodata_4C4 = 1.1920929e-07, 0x00008F8C), the ratio of the remaining time to the kept one
// (0x00008F94..0x00008FA4) is clamped to 1.0 (rodata_43C) and scaled by 255.0 (rodata_4DC); the
// guest puts that float on the stack and reads its low byte (0x00008FC4..0x00008FC8), so the
// byte it writes into the alpha byte at guest this+0x44F is the low byte of the float's bits -
// the retail's own bit pun, which the port reproduces.
//
// Deactivate (0x00008FD4): the alpha byte is 0 (the guest writes the low byte of rodata_494 =
// 0.0f) and the base's alpha delta (guest this+0x40C) is 0.0.
void CCommandoPirate::WarpOut(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
    case kStateMsg_Activate: {                                   // 0x00008E30
      mAnimationState.SetState(CAnimationState::kAS_Ready);      // 0x00008E38
      x920_ = 1;                                                  // 0x00008E44
      xc65_ &= ~0x40;                                             // 0x00008E48..0x00008E50
      ReleaseCoverPoint(mgr, xbd2_, true);                         // 0x00008E54
      break;
    }
    case kStateMsg_Update: {                                      // 0x00008E5C
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {  // 0x00008E68
        BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_One, -1));  // 0x00008EF0
      } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {  // 0x00008F4C..0x00008F54
        if ((xc65_ & 0x40) == 0) {                                // 0x00008F5C..0x00008F60
          xc65_ |= 0x40;                                          // 0x00008F68..0x00008F6C
          xc58_ = BodyController()->GetAnimTimeRemaining();        // 0x00008F74..0x00008F78
        } else if (xc58_ > 1.1920929e-07f) {                      // rodata_4C4, 0x00008F8C
          const float ratio =
              BodyController()->GetAnimTimeRemaining() / xc58_;    // 0x00008F94..0x00008FA4
          const float scaled = 255.0f * (ratio < 1.0f ? ratio : 1.0f);  // rodata_43C, rodata_4DC
          // The retail casts the float to a byte (the guest's psq_st + lbz at 0x00008FC4..
          // 0x00008FC8 is its own float-to-byte cast, the one the host's CCast::ToUint8 is the
          // counterpart of); the host's cast is the value conversion, as it is everywhere else
          // the host's colour alpha is set.
          mColor.SetAlpha(CCast::ToUint8(scaled));                  // 0x00008FC4..0x00008FCC
        } else {
        }
      } else {
      }
      break;
    }
    case kStateMsg_Deactivate: {                                 // 0x00008FD4
      mColor.SetAlpha(0.f);                                       // 0x00008FD4..0x00008FE4
      mAlphaDelta = 0.f;                                          // 0x00008FE8
      break;
    }
    default:
      break;
  }
}

// run: area 59 (05_temple_federationsite_dark), tools/drivers/commandopirate_warpout.py, 2026-10-21
// @ fn_9_8D28
// fn_9_8D28 (0xc8 bytes): the PostWarpOut state. Only the activate message (the guest's test of 0
// at 0x00008D30) does anything: when the byte at guest this+0xC63 has no 0x20 bit (the test at
// 0x00008D54) the commando tells the world it leaves with the deactivate message (the delivery at
// 0x00008DA8 - the sender is the actor's own id, 0x00008D60, and the message is kSM_Deactivate,
// 0x44435456, 0x00008D70). Then the module's state word (guest this+0x920) goes back to -1
// (0x00008DAC..0x00008DB0) and, when the byte at guest this+0xC63 has its 0x10 bit (the test at
// 0x00008DB8), the actor asks to be deleted (0x00008DC0..0x00008DD4).
void CCommandoPirate::PostWarpOut(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {  // 0x00008D30..0x00008D4C
    return;
  }
  // the guest's extrwi. r0, r0, 1, 26 (0x00008D54) takes the bit 26 from the MSB, which is the
  // LSB bit 5, so the mask is 0x20; the branch at 0x00008D58 skips the message when it is set.
  if ((xc63_ & 0x20) == 0) {  // 0x00008D50..0x00008D58
    mgr.DeliverScriptMsg(CScriptMsg(GetUniqueId(), kInvalidUniqueId, kSM_Deactivate));  // 0x00008DA8
  }
  x920_ = -1;                    // 0x00008DAC..0x00008DB0
  // the guest's extrwi. r0, r0, 1, 27 (0x00008DB8) takes the bit 27 from the MSB, which is the
  // LSB bit 4, so the mask is 0x10; the branch at 0x00008DBC skips the delete when it is clear.
  if ((xc63_ & 0x10) != 0) {     // 0x00008DB4..0x00008DBC
    mgr.DeleteObjectRequest(GetUniqueId());  // 0x00008DC0..0x00008DD4
  }
}

// fn_9_8B70 (0x1b8 bytes): the JumpBack state - the commando steps back away from its target.
//
// On activate (the guest's message 0, 0x00008BA8) the animation state goes "ready" (guest
// this+0x6B4 = 1, 0x00008BB0) and the module's helper fn_9_3CBC (the call at 0x00008BBC) has to
// find the target: when it does not, the state ends right there (0x00008BC0..0x00008BC4). The
// step's own target is then the actor's position plus the transform's second column (guest
// this+0x28, 0x38 and 0x48 - the loads at 0x00008BC8..0x00008BF4), stored at guest this+0xB94.
//
// On update (the guest's message 1, 0x00008BFC) the step command is issued when the animation
// state may issue it (the CanIssueCommand call at 0x00008C08 asks for the animation state 3 =
// kAS_Step): the command the guest delivers is the id 1 = kBSC_Step, with the direction 1
// (0x00008C2C = kSD_Backward) and the type 0 (0x00008C30 = kStep_Normal); the guest also builds
// the command's data on the stack (0x00008C18..0x00008C6C) and copies it into the command
// manager's slot (0x00008C74..0x00008CBC). When the command may not be issued and the body is in
// the state 3 (the compare at 0x00008CD0), the body's target vector (the body controller's +0x34,
// the command manager's fifth CVector3f) takes the step target minus the position
// (0x00008CD8..0x00008D04) - the host's step state faces that vector (CBSStep.cpp:36).
//
// On deactivate (the guest's message 2, 0x00008D0C) the animation state goes "not ready".
void CCommandoPirate::JumpBack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {  // the guest's message 0
    mAnimationState.SetState(CAnimationState::kAS_Ready);  // 0x00008BB0
    if (!x3CBC_(mgr, xb94_, 0.f)) {  // 0x00008BBC (the target has to exist)
      break;
    }
    // 0x00008BC8..0x00008BF4: the jump target is the position plus the transform's second column
    // (guest this+0x28, 0x38 and 0x48).
    xb94_ = GetTranslation() + GetTransform().GetForward();
    break;
  }
  case kStateMsg_Update: {  // the guest's message 1
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {  // 0x00008C08
      // 0x00008C14..0x00008C70: the command the guest delivers is the id 1 - kBSC_Step - with
      // the direction 1 (0x00008C2C) and the type 0 (0x00008C30); the guest also builds the
      // command's data on the stack (0x00008C18..0x00008C6C) and copies it into the command
      // manager's slot (0x00008C74..0x00008CBC), but the id it delivers is the step's. The
      // animation state it asks for is the step's too (3 = kAS_Step).
      BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(pas::kSD_Backward, pas::kStep_Normal));
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_Step) {  // 0x00008CD0
      // the body's target vector: the jump target minus the position (0x00008CD8..0x00008D04)
      BodyController()->CommandMgr().SetTargetVector(xb94_ - GetTranslation());
    }
    break;
  }
  case kStateMsg_Deactivate: {  // the guest's message 2
    mAnimationState.SetState(CAnimationState::kAS_NotReady);  // 0x00008D0C
    break;
  }
  default:
    break;
  }
}

// @ fn_9_3678
// fn_9_3678 (0x288 bytes): the module's dodge picker, called by the Dodge state's activate (the
// call at 0x00008974). It answers the dodge direction: 2 (the host's pas::kSD_Left) or 3 (its
// pas::kSD_Right), or -1 when neither side is free.
//
// The two candidates are the actor's right column and its negation. The guest's transform is a
// CTransform4f at this+0x24 (the FromMatrix call's argument at 0x000024C4), so its kDX column -
// the right one - is this+0x24, +0x34 and +0x44 (the three loads at 0x000036A8..0x000036E4).
//
// The listening-AI list (the state manager's +0x830, the load at 0x000036E8) is walked. Every
// other AI of the same area (the area ids at +0x04, compared at 0x00003710..0x0000371C) that is
// nearer than the module's float at guest this+0xBE4 (its square at 0x000036B8, the compare at
// 0x00003758) blocks the side it stands on: the dot product with the right column (the fmadds
// chain at 0x0000376C..0x0000377C) says which side, and the angle to it must be under 60 degrees
// (lbl_9_rodata_4A4 = 1.0471976f, the load at 0x000037A4; the GetAngleDiff calls at 0x0000379C
// and 0x000037E0, the right column and its negation).
//
// Each side that is still free is then checked by the module's own helper fn_9_3338 (the calls
// at 0x00003834 and 0x00003878): the point that distance along the side has to be clear. When
// both sides are free, the state manager's random (its CRandom16 at +0x16E4, the call at
// 0x00003894) picks one: its bit 14 (the rlwinm at 0x00003898) keeps the right, the clear bit
// keeps the left.
int CCommandoPirate::x3678_PickDodgeDirection(CStateManager& mgr) const {
  const float distance = xbe4_;                                   // 0x000036A8
  const float distanceSq = distance * distance;                   // 0x000036B8
  const CVector3f right = GetTransform().GetRight();              // 0x000036A8..0x000036E4
  const CVector3f position = GetTranslation();                    // 0x000036B0..0x000036C0
  bool rightFree = true;                                          // r29
  bool leftFree = true;                                           // r30

  CObjectList& list = mgr.ObjectListById(kOL_ListeningAi);        // 0x000036E8
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    // The guest tests the object for null (0x00003700), then compares it with itself
    // (0x00003708) and reads the area id at +0x04 (the compare at 0x00003718). The host's cast
    // carries the null test and can also fail, where the guest's raw read of the area id could
    // not.
    CPatterned* other = TCastToPtr< CPatterned >(list[i]);       // 0x000036FC
    if (other == nullptr || other == this) {                      // 0x00003700..0x0000370C
      continue;
    }
    if (other->GetCurrentAreaId() != GetCurrentAreaId()) {        // 0x00003710..0x0000371C
      continue;
    }
    const CVector3f delta = other->GetTranslation() - position;   // 0x00003720..0x00003734
    if (delta.MagSquared() >= distanceSq) {                       // 0x00003738..0x0000375C
      continue;
    }
    if (CVector3f::Dot(delta, right) >= 0.f) {                    // 0x0000376C..0x00003788
      if (!rightFree) {                                           // 0x0000378C
        continue;
      }
      if (CVector3f::GetAngleDiff(right, delta) >= 1.0471976f) {  // 0x0000379C..0x000037AC
        continue;
      }
      rightFree = false;                                         // 0x000037B0
    } else {
      if (!leftFree) {                                            // 0x000037B8
        continue;
      }
      if (CVector3f::GetAngleDiff(-right, delta) >= 1.0471976f) {  // 0x000037E0..0x000037F0
        continue;
      }
      leftFree = false;                                          // 0x000037F4
    }
  }

  if (rightFree) {                                                // 0x0000381C..0x00003824
    rightFree = x3338_IsJumpBackClear(mgr, right, distance);      // 0x00003834
  }
  if (leftFree) {                                                 // 0x0000383C
    leftFree = x3338_IsJumpBackClear(mgr, -right, distance);       // 0x00003878
  }
  if (rightFree && leftFree) {                                    // 0x00003880..0x0000388C
    if ((mgr.Random()->Next() & 0x4000) != 0) {                   // 0x00003894..0x00003898
      leftFree = false;                                           // 0x000038A0
    } else {
      rightFree = false;                                          // 0x000038A8
    }
  }
  if (leftFree) {                                                 // 0x000038AC..0x000038B8
    return pas::kSD_Left;                                         // 2
  }
  if (rightFree) {                                                // 0x000038BC..0x000038C4
    return pas::kSD_Right;                                        // 3
  }
  return -1;                                                      // 0x000038C8
}

// fn_9_8934 (0x1e0 bytes): the Dodge state, in the guest's three parts.
//
// Activate (the guest's message 0, the test at 0x0000895C): the module's helper fn_9_3678
// picks the dodge, the result goes into the module's field at guest this+0xBDC (the store at
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
    xbdc_ = x3678_PickDodgeDirection(mgr);            // 0x00008974
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

// @ fn_9_86B4
// fn_9_86B4 (0x280 bytes): the shield's arming state.
//
// On activate (the guest's message 0, 0x00008704) the module's flag byte at guest this+0xC65 gets
// its 0x08 bit, the byte at guest this+0xC66 loses its 0x20 bit, the animation state goes "ready"
// (guest this+0x6B4 = 1), the body's locomotion type becomes 2 (pas::kLT_Lurk, the guest's
// li r4,0x2 at 0x00008718), the shield's
// arm timer at guest this+0xB80 takes the block's arm time (the block's +0x144 = guest this+0x904)
// plus a random fraction of the arm time variation (the block's +0x148 = guest this+0x908), the
// flag byte at guest this+0xC64 loses its 0x04 bit and the module's shield collision actor manager
// (guest this+0xA3C) becomes active.
//
// On update (the guest's message 1, 0x00008770) the timer falls by the frame time; when it is over
// the animation state goes "over" (guest this+0x6B4 = 3), and so does it when the flag byte at
// guest this+0xC64 has its 0x04 bit. Then the byte at guest this+0xC66's 0x20 bit stops the state,
// and the module's own helper fn_9_3CBC asks the target for its aim position (the module's member
// at guest this+0xB94, with the frame time 0.0f): the commando turns towards it when the angle
// between its facing and the direction to the target is over 20 degrees (rodata_4B4).
//
// On deactivate (the guest's message 2, 0x000088B4) the animation state goes "not ready" (guest
// this+0x6B4 = 0), the body's locomotion type becomes 3 (pas::kLT_Combat), the flag byte at guest
// this+0xC64 loses its 0x80 bit, the counter at guest this+0xB58 takes the counter at guest
// this+0xB50 plus one, the shield's collision actor manager goes inactive, the module's own
// activate helper (fn_9_1EF8) runs and the flag byte at guest this+0xC65 loses its 0x08 bit.
void CCommandoPirate::ArmShield(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    xc65_ |= 0x08;                                   // 0x0000870C
    xc66_ &= ~0x20;                                  // 0x00008720
    mAnimationState.SetState(CAnimationState::kAS_Ready);  // 0x00008728
    // 0x00008718: the guest's locomotion type is 2 - pas::kLT_Lurk - not the relaxed one.
    BodyController()->SetLocomotionType(pas::kLT_Lurk);  // 0x00008718, 0x00008730
    xb80_ = mData.mShield.mArmTime +
            mgr.Random()->Float() * mData.mShield.mArmTimeVariation;  // 0x00008744
    xc64_ &= ~0x04;                                  // 0x0000875C
    // The guest calls the manager without a test; the host's test covers the window before the
    // kSM_Create case has built it (the host limit named here).
    if (mCollisionShield.get() != nullptr) {
      mCollisionShield->SetActive(mgr, true);        // 0x00008768
    }
    break;
  }
  case kStateMsg_Update: {
    xb80_ -= dt;                                      // 0x00008778
    if (xb80_ <= 0.f) {                               // 0x00008788
      mAnimationState.SetState(CAnimationState::kAS_Over);  // 0x000087A4
      break;
    }
    if ((xc64_ & 0x04) != 0) {                       // 0x00008798
      mAnimationState.SetState(CAnimationState::kAS_Over);  // 0x000087A4
      break;
    }
    if ((xc66_ & 0x20) != 0) {                       // 0x000087B0
      break;
    }
    if (!x3CBC_(mgr, xb94_, 0.f)) {                   // 0x000087C0 (the guest's own helper)
      break;
    }
    CVector3f delta = xb94_ - GetTranslation();  // 0x000087CC..0x00008808
    delta.SetZ(0.f);                            // 0x0000880C (rodata_494 = 0.0f)
    if (!delta.IsMagnitudeSafe()) {                    // 0x00008810
      break;
    }
    // 0x00008820..0x00008848: the angle between the actor's facing (the transform's forward
    // column) and the direction to the target, against 20 degrees (rodata_4B4).
    if (CVector3f::GetAngleDiff(GetTransform().GetForward(), delta) <= 0.34906584f) {
      break;
    }
    const CBCLocomotionCmd cmd(CVector3f::Zero(), delta.AsNormalized(), 1.f);  // 0x00008850
    BodyController()->CommandMgr().DeliverCmd(cmd);                        // 0x000088AC
    break;
  }
  case kStateMsg_Deactivate: {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);  // 0x000088B4
    BodyController()->SetLocomotionType(pas::kLT_Combat);     // 0x000088C4 (the guest's 3)
    xc64_ &= ~0x80;                                          // 0x000088D0
    xb58_ = xb50_ + 1;                                       // 0x000088E4
    // The guest calls the manager without a test; the host's test covers the window before the
    // kSM_Create case has built it (the host limit named here).
    if (mCollisionShield.get() != nullptr) {
      mCollisionShield->SetActive(mgr, false);               // 0x000088F0
    }
    x1EF8_Activate();                                        // 0x000088F8
    xc65_ &= ~0x08;                                          // 0x00008908
    break;
  }
  default:
    break;
  }
}

// run: the Swamp (05_swamp_timber!), the attack pattern with the area's own waypoints, 2026-10-21:
// the state runs in the normal flow (the ShouldBoost trigger answers true on the waypoint's 0x20
// flag) and all its arms are taken.
// @ fn_9_8028
// fn_9_8028 (0x68c bytes): the shield-charge state - the commando charges at its target with
// the shield up.
//
// On activate (the guest's message 0, 0x00008088) the byte at guest this+0xC65 gets its 0x04 bit
// (the shield is up), the byte at guest this+0xC66 loses its 0x20 bit, the byte at guest this+0xC64
// loses its 0x08 and its 0x04 bit, the timer at guest this+0xB84 is zeroed, the word at guest
// this+0x924 is zeroed, the animation state goes "ready" (guest this+0x6B4 = 1) and the counter
// at guest this+0xB54 takes the counter at guest this+0xB50 plus one (0x000080DC..0x000080E4).
// Then the commando asks its team for the attack (0x000080E8..0x00008120) and, when the team lets
// it, it takes the direction to the target: the horizontal delta from the target (the id at guest
// this+0xBCA) is normalized, or the actor's own forward column when the delta is not magnitude
// safe (0x00008124..0x000081B8). That direction goes to guest this+0xBA0, the charge target - the
// actor's own position plus the block's maximum charge distance (the block's +0x128 = guest
// this+0x8E8) times that direction - goes to guest this+0xB94, and the charge velocity vector at
// guest this+0xBB8 is zeroed (0x000081BC..0x00008224).
//
// On update (the guest's message 1, 0x00008240) the word at guest this+0x924 picks the part: 0
// turns towards the charge direction (the angle against 10 degrees, rodata_498, 0x0000825C) and 1
// charges - the loop-attack command may be issued (the guest's ShouldRetry with the command id 9,
// 0x000082F4) and the shield's collision actor manager goes active; the charge itself moves the
// actor along the charge direction at the block's charge speed (the block's +0x12C = guest
// this+0x8EC) - the direction rotated into the actor's own frame (TransposeRotate) and scaled by
// the speed and the frame time - until the state machine's own time (its vtable slot 0x30,
// 0x00008430) passes 3.0f (rodata_448) plus the time the charge takes (0x00008404..0x00008578),
// and only while the byte at guest this+0xC64 has its 0x08 bit (the charge animation took off,
// 0x0000848C..0x00008494).
//
// On deactivate (the guest's message 2, 0x0000859C) the animation state goes "not ready", the body
// is told to leave the state (the command id 0xa = kBSC_ExitState, 0x000085B4) - and, when the
// byte at guest this+0xC64 has its 0x04 bit, the abort command (the id 0xd = kBSC_AbortScripted)
// instead - the word at guest this+0x924 goes -1, the shield's collision actor manager goes
// inactive, the module's own activate helper (fn_9_1EF8) runs, the team is told the attack is
// over (0x00008618..0x00008660) and the bytes at guest this+0xC64 and this+0xC65 lose their 0x08
// and their 0x04 bit.
void CCommandoPirate::ShieldCharge(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    xc65_ |= 0x04;                    // 0x00008090 (the shield is up)
    xc66_ &= ~0x20;                   // 0x000080AC
    xc64_ &= ~0x08;                   // 0x000080BC
    xc64_ &= ~0x04;                   // 0x000080C8
    xb84_ = 0.f;                      // rodata_494, 0x000080D0
    x924_ = 0;                        // 0x000080D4
    mAnimationState.SetState(CAnimationState::kAS_Ready);  // 0x000080D8
    xb54_ = xb50_ + 1;                // 0x000080DC..0x000080E4
    if (xbc4_teamId != kInvalidUniqueId &&
        !CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Melee, mgr, xbc4_teamId,
                                       GetUniqueId())) {  // 0x00008118
      mAnimationState.SetState(CAnimationState::kAS_Over);  // 0x00008234
      break;
    }
    const CEntity* target = mgr.GetObjectById(xbca_);  // 0x00008134
    // The guest reads the target's position without a cast; the host's CEntity has no position,
    // so the port casts to CActor (the host limit named here).
    const CActor* targetActor = TCastToConstPtr< CActor >(target);
    if (targetActor == nullptr) {
      mAnimationState.SetState(CAnimationState::kAS_Over);  // 0x00008228
      break;
    }
    // The horizontal delta to the target (the guest zeroes the height, 0x0000817C).
    const CVector3f delta(targetActor->GetTranslation().GetX() - GetTranslation().GetX(),
                          targetActor->GetTranslation().GetY() - GetTranslation().GetY(), 0.f);
    if (delta.IsMagnitudeSafe()) {
      xba0_ = delta.AsNormalized();            // 0x0000818C..0x00008198
    } else {
      xba0_ = GetTransform().GetForward();     // 0x000081A0..0x000081B8
    }
    // The charge target: the actor's own position plus the maximum charge distance times the
    // charge direction (0x000081D8..0x00008208).
    xb94_ = GetTranslation() + mData.mShield.mChargeMaxAttackDist * xba0_;
    xbb8_ = CVector3f::Zero();                 // 0x0000820C..0x00008220
    break;
  }
  case kStateMsg_Update: {
    if (x924_ == 1) {
      // The charge (the guest's message 1 with the word at guest this+0x924 = 1).
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
        // 0x0000830C: the loop-attack command. The guest's payload is the four words
        // {7, 4, 0, the flag byte}, which is the host's CBCLoopAttackCmd: the loop attack type
        // 4 (pas::kLAT_Four), no wait for the animation and no skip.
        BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_Four, false));
        // The guest calls the manager without a test; the host's test covers the window before
        // the kSM_Create case has built it (the host limit named here).
        if (mCollisionShield.get() != nullptr) {
          mCollisionShield->SetActive(mgr, true);  // 0x000083A0
        }
      } else if (BodyController()->GetCurrentStateId() == pas::kAS_LoopAttack) {
        // 0x000083A8: the charge movement.
        const CVector3f toTarget = xb94_ - GetTranslation();
        const float speed = mData.mShield.mChargeSpeed;  // the block's +0x12C = guest this+0x8EC
        float time = 0.f;
        if (speed > 0.f) {
          time = mData.mShield.mChargeMaxAttackDist / speed;
        }
        const float along = CVector3f::Dot(GetTransform().GetForward(), toTarget);
        if (along <= 0.f || (xc64_ & 0x04) != 0) {  // 0x00008418..0x0000842C
          // The dot product is not over 0 (0x00008418..0x00008420) or the byte at guest this+0xC64
          // has its 0x04 bit (0x00008424..0x0000842C): the charge ends.
          BodyController()->CommandMgr().DeliverCmd(kBSC_ExitState);  // 0x00008458 (the id 0xa)
        } else if (StateMachineState().GetTime() > 3.f + time) {
          // 0x00008430..0x00008454: the state machine's own time (the vtable slot 0x30 of the
          // machine, GetTime) against 3.0f (rodata_448) plus the charge time.
          BodyController()->CommandMgr().DeliverCmd(kBSC_ExitState);  // 0x00008458 (the id 0xa)
        } else if ((xc64_ & 0x08) != 0) {         // 0x0000848C..0x00008494
          // The movement only runs while the byte at guest this+0xC64 has its 0x08 bit - the
          // charge animation has taken off (the kUE_TakeOff event sets it).
          const CVector3f local = GetTransform().TransposeRotate(xba0_);  // 0x000084A8
          MoveInOneFrameOR(local * (speed * dt), dt);                     // 0x000084E8
          if (xbb8_.GetX() != 0.f || xbb8_.GetY() != 0.f || xbb8_.GetZ() != 0.f) {
            const CVector3f push = GetTransform().TransposeRotate(xbb8_);  // 0x00008538
            MoveInOneFrameOR(push * (speed * dt), dt);                     // 0x00008578
            xbb8_ = CVector3f::Zero();                                    // 0x0000857C..0x00008594
          }
        }
      }
    } else if (x924_ == 0) {
      // 0x0000825C: turn towards the charge direction.
      if (CVector3f::GetAngleDiff(GetTransform().GetForward(), xba0_) <= 0.17453292f) {
        x924_ = 1;  // rodata_498, 0x000082E8
      } else {
        const CBCLocomotionCmd cmd(CVector3f::Zero(), xba0_, 1.f);  // 0x00008290..0x000082E4
        BodyController()->CommandMgr().DeliverCmd(cmd);
      }
    }
    break;
  }
  case kStateMsg_Deactivate: {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);  // 0x000085A4
    BodyController()->CommandMgr().DeliverCmd(kBSC_ExitState);  // 0x000085B4 (the id 0xa)
    if ((xc64_ & 0x04) != 0) {                                  // 0x000085DC..0x000085E4
      BodyController()->CommandMgr().DeliverCmd(kBSC_AbortScripted);  // 0x000085EC (the id 0xd)
    }
    x924_ = -1;                                        // 0x00008620
    // The guest calls the manager without a test; the host's test covers the window before the
    // kSM_Create case has built it (the host limit named here).
    if (mCollisionShield.get() != nullptr) {
      mCollisionShield->SetActive(mgr, false);         // 0x0000862C
    }
    x1EF8_Activate();                                  // 0x00008634
    CScriptTeamAiMgr::EndAttack(CScriptTeamAiMgr::kAT_Melee, mgr, xbc4_teamId, GetUniqueId(),
                                false);                // 0x00008660
    xc64_ &= ~0x08;                                    // 0x0000866C
    xc65_ &= ~0x04;                                    // 0x00008678
    break;
  }
  default:
    break;
  }
}

// fn_9_7A98 (0x590 bytes): the scripted shield charge - the commando walks its waypoint path
// with the shield up and charges at the waypoints.
//
// On activate (the guest's message 0, 0x00007B24) the byte at guest this+0xC65 gets its 0x04
// bit (the shield is up, 0x00007B2C), the byte at guest this+0xC66 loses its 0x20 bit
// (0x00007B48), the byte at guest this+0xC64 loses its 0x08 and its 0x04 bit (0x00007B54 and
// 0x00007B60), the timer at guest this+0xB84 is zeroed (0x00007B68), the animation state goes
// "ready" (guest this+0x6B4 = 1, 0x00007B6C) and the counter at guest this+0xB54 takes the counter
// at guest this+0xB50 plus one (0x00007B70..0x00007B78). Then the module's waypoint picker
// (fn_9_2A98) takes the waypoint (0x00007B7C); when it fails the animation state goes "over"
// (0x00007C34). The actor's own material filter gets kMT_Ceiling and kMT_Wall in its exclude list
// (the data words 0x1D and 0x1E at lbl_9_data_7E8 and lbl_9_data_7EC, 0x00007B94..0x00007C10) and the
// module's push vector (guest this+0xBB8) is cleared (0x00007C14..0x00007C2C).
//
// On update (the guest's message 1, 0x00007C40) the loop-attack command may be issued (the
// guest's ShouldRetry with the command id 9, 0x00007C40..0x00007C4C): then the actor's vertical
// movement flag (the byte at guest this+0x34C's 0x40 bit, 0x00007C58..0x00007C68) is set, the
// ground collider material is removed (0x00007C74), the loop-attack command is delivered
// (0x00007CC4) and the shield's collision actor manager goes active (0x00007D0C). When the
// command may not be issued and the body is in the loop attack (0x00007D18..0x00007D20) the timer
// at guest this+0xB84 is zeroed (0x00007D2C) and, while the byte at guest this+0xC64 has no
// 0x04 bit, the commando either has reached the waypoint (the waypoint is behind it or inside
// the actor's own bounding box - 0x00007E70..0x00007DF0) and takes the next one (fn_9_2A98 with
// the third argument set, 0x00007E00; when that fails the body is told to leave the state, the
// command id 0xa, 0x00007E2C) or it turns towards the waypoint (fn_9_2474, 0x00007E54).
//
// On deactivate (the guest's message 2, 0x00007E5C) the animation state goes "not ready"
// (0x00007E60), the body is told to leave the state (the command id 0xa, 0x00007E84) - and, when
// the byte at guest this+0xC64 has its 0x04 bit, the abort command (the id 0xd) instead
// (0x00007EC8) - the waypoint the commando is at goes to guest this+0xBDA (0x00007ED8), the actor's
// vertical movement flag is cleared (0x00007EF0..0x00007EF8), the ground collider material is
// added (0x00007EFC), the actor's own material filter loses kMT_Ceiling and kMT_Wall again (the
// data words 0x1D and 0x1E at lbl_9_data_7F0 and lbl_9_data_7F4, 0x00007F04..0x00007F90), the shield's
// collision actor manager goes inactive (0x00007FA0), the module's own activate helper (fn_9_1EF8)
// runs (0x00007FA8) and the bytes at guest this+0xC64 and this+0xC65 lose their 0x08 and their
// 0x04 bit (0x00007FB4 and 0x00007FC0).
void CCommandoPirate::ScriptedShieldCharge(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    xc65_ |= 0x04;                    // 0x00007B2C (the shield is up)
    xc66_ &= ~0x20;                   // 0x00007B48
    xc64_ &= ~0x08;                   // 0x00007B54
    xc64_ &= ~0x04;                   // 0x00007B60
    xb84_ = 0.f;                      // 0x00007B68
    mAnimationState.SetState(CAnimationState::kAS_Ready);  // 0x00007B6C
    xb54_ = xb50_ + 1;                // 0x00007B70..0x00007B78
    if (!x2A98_PickWaypoint(mgr, false)) {  // 0x00007B7C
      mAnimationState.SetState(CAnimationState::kAS_Over);  // 0x00007C34
      break;
    }
    CMaterialFilter filter = GetMaterialFilter();
    filter.ExcludeList().Add(kMT_Ceiling);  // the data word 0x1D at lbl_9_data_7E8
    filter.ExcludeList().Add(kMT_Wall);     // the data word 0x1E at lbl_9_data_7EC
    SetMaterialFilter(filter);              // 0x00007C10
    xbb8_ = CVector3f::Zero();               // 0x00007C14..0x00007C2C
    break;
  }
  case kStateMsg_Update: {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      // 0x00007C58: the vertical movement flag (the byte at guest this+0x34C's 0x40 bit).
      mVerticalMovement = true;
      RemoveMaterial(kMT_GroundCollider, mgr);  // 0x00007C74
      // 0x00007CC4: the loop-attack command; the guest's payload is the loop attack type 4, no
      // wait for the animation and the flag byte's 0x80 bit (0x00007C90) - the retail's skip-into.
      CBCLoopAttackCmd loopCmd(pas::kLAT_Four, false);
      loopCmd.SetSkipInto(true);
      BodyController()->CommandMgr().DeliverCmd(loopCmd);
      // The guest calls the manager without a test; the host's test covers the window before
      // the kSM_Create case has built it (the host limit named here).
      if (mCollisionShield.get() != nullptr) {
        mCollisionShield->SetActive(mgr, true);  // 0x00007D0C
      }
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_LoopAttack) {
      xb84_ = 0.f;  // 0x00007D2C
      if ((xc64_ & 0x04) != 0) {  // 0x00007D34
        BodyController()->CommandMgr().DeliverCmd(kBSC_ExitState);  // 0x00007E3C (the id 0xa)
      } else {
        // 0x00007E70: the commando has reached the waypoint when the waypoint is behind it or
        // inside the actor's own bounding box.
        const CVector3f delta = xb94_ - GetTranslation();
        const CAABox box = GetBoundingBox();
        const float height = box.GetMaxPoint().GetZ() - box.GetMinPoint().GetZ();
        const float along = CVector3f::Dot(GetTransform().GetForward(), delta);
        if (along <= 0.f || delta.MagSquared() <= height * height) {  // 0x00007DDC..0x00007DF0
          if (!x2A98_PickWaypoint(mgr, true)) {  // 0x00007E00
            BodyController()->CommandMgr().DeliverCmd(kBSC_ExitState);  // 0x00007E2C (the id 0xa)
          }
        } else if ((xc64_ & 0x08) != 0) {  // 0x00007E44..0x00007E48
          // 0x00007E40..0x00007E54: the turn helper runs while the byte at guest this+0xC64 has
          // its 0x08 bit - the module's user-event handler sets it when the shield's charge
          // animation takes off (fn_9_C798, 0x0000C868) and clears it when it lands
          // (0x0000C88C). The guest's test is the extrwi at 0x00007E44 and the beq at 0x00007E48:
          // the turn is the fall-through, so the bit has to be set.
          x2474_TurnToWaypoint(mgr, dt);  // 0x00007E54
        }
      }
    }
    break;
  }
  case kStateMsg_Deactivate: {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);  // 0x00007E60
    BodyController()->CommandMgr().DeliverCmd(kBSC_ExitState);  // 0x00007E84 (the id 0xa)
    if ((xc64_ & 0x04) != 0) {  // 0x00007EA0..0x00007EA4
      BodyController()->CommandMgr().DeliverCmd(kBSC_AbortScripted);  // 0x00007EC8 (the id 0xd)
    }
    xbda_ = mWaypointNavigation.GetDestination();  // 0x00007ED8..0x00007EE8 (guest this+0x6C0)
    mVerticalMovement = false;  // 0x00007EF0..0x00007EF8
    AddMaterial(kMT_GroundCollider, mgr);  // 0x00007EFC
    CMaterialFilter filter = GetMaterialFilter();
    filter.ExcludeList().Remove(kMT_Ceiling);  // the data word 0x1D at lbl_9_data_7F0
    filter.ExcludeList().Remove(kMT_Wall);     // the data word 0x1E at lbl_9_data_7F4
    SetMaterialFilter(filter);                 // 0x00007F90
    // The guest calls the manager without a test; the host's test covers the window before the
    // kSM_Create case has built it (the host limit named here).
    if (mCollisionShield.get() != nullptr) {
      mCollisionShield->SetActive(mgr, false);  // 0x00007FA0
    }
    x1EF8_Activate();  // 0x00007FA8
    xc64_ &= ~0x08;    // 0x00007FB4
    xc65_ &= ~0x04;    // 0x00007FC0
    break;
  }
  default:
    break;
  }
}

// fn_9_786C (0x22c bytes): the RestoreOrientation state - the commando turns itself upright
// again after it was tipped over.
//
// On activate (the guest's message 0, 0x000078B0) the animation state goes "ready" (guest
// this+0x6B4 = 1).
//
// On update (the guest's message 1, 0x000078BC) the actor's up column (the transform's third
// column, guest this+0x2C, 0x3C and 0x4C - the loads at 0x000078D4..0x000078E4) decides: when its
// z is 1 within 0.00001f (rodata_4D0, the compare at 0x000078F0) the commando stands upright
// already and the animation state goes "over" (0x000079F4). Otherwise the angle between the up
// column and the world up (the GetAngleDiff call at 0x00007904) is measured: when it is at most
// the turn speed for this frame (the angle in degrees - rodata_4D4 = 57.295776 - against the
// frame time times the body controller's turn speed, the float at its +0x590, the compare at
// 0x00007920) the animation state goes "over" (0x000079E8) as well. Otherwise the commando is
// turned towards the world up: the up column becomes a unit vector (0x00007944) and the
// quaternion that turns it there (the CQuaternion::LookAt call at 0x0000795C) is applied to the
// actor's own rotation (0x00007988..0x00007998) - at most the angle of one frame's turn
// (rodata_4D8 = 0.017453292 times the frame time times the turn speed, 0x00007938..0x00007940) -
// and the actor takes the new transform (0x000079C4..0x000079E0).
//
// On deactivate (the guest's message 2, 0x00007A00) the animation state goes "not ready" and the
// actor is laid down: the look-at from its position towards the position plus the transform's
// second column's x and y (the loads at 0x00007A20..0x00007A4C) with the world up
// (0x00007A60..0x00007A78).
void CCommandoPirate::RestoreOrientation(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {  // the guest's message 0
    mAnimationState.SetState(CAnimationState::kAS_Ready);  // 0x000078B0
    break;
  }
  case kStateMsg_Update: {  // the guest's message 1
    const CVector3f up = GetTransform().GetUp();  // guest this+0x2C, 0x3C and 0x4C
    if (CMath::AbsF(up.GetZ() - 1.f) < 0.00001f) {  // rodata_4D0, 0x000078F0..0x000078F4
      mAnimationState.SetState(CAnimationState::kAS_Over);  // 0x000079F4
      break;
    }
    const float turnSpeed = BodyController()->GetTurnSpeed();  // the body controller's +0x590
    const float angle = CVector3f::GetAngleDiff(up, CVector3f::Up());  // 0x00007904
    if (angle * 57.295776f <= dt * turnSpeed) {  // rodata_4D4, 0x00007918..0x00007924
      mAnimationState.SetState(CAnimationState::kAS_Over);  // 0x000079E8
      break;
    }
    // the turn towards the world up, at most one frame's worth (rodata_4D8 = 0.017453292)
    const CUnitVector3f unitUp(up, CUnitVector3f::kN_Yes);  // 0x00007944
    const CQuaternion look = CQuaternion::LookAt(unitUp, CUnitVector3f(CVector3f::Up()),
                                                 CRelAngle::FromRadians(0.017453292f *
                                                                         (dt * turnSpeed)));
    const CQuaternion current = CQuaternion::FromMatrix(GetTransform());  // 0x00007988
    const CQuaternion turn = current * look;  // 0x00007998
    SetTransform(turn.BuildNormalized().BuildTransform4f(GetTranslation()));  // 0x000079C4..0x000079E0
    break;
  }
  case kStateMsg_Deactivate: {  // the guest's message 2
    mAnimationState.SetState(CAnimationState::kAS_NotReady);  // 0x00007A00
    // 0x00007A20..0x00007A78: the look-at from the position towards the position plus the
    // transform's second column's x and y (guest this+0x28 and 0x38), with the world up.
    const CVector3f pos = GetTranslation();
    const CVector3f forward = GetTransform().GetForward();
    SetTransform(CTransform4f::LookAt(pos, pos + CVector3f(forward.GetX(), forward.GetY(), 0.f),
                                      CVector3f::Up()));
    break;
  }
  default:
    break;
  }
}

// fn_9_7724 (Crouch): PortCCommandoPirateCore.cpp.

// fn_9_7358 (0x310 bytes): the WallHang state - the commando hangs on the wall the cover point
// it holds points at.
//
// The message switch (0x00007360..0x00007398) picks the activate (0), the update (1) and the
// deactivate (2); every other message leaves the state alone.
//
// Activate (0x0000739C): the 0x02 bit of the flag byte at guest this+0xC65 is set (the guest's
// rlwimi at 0x000073A4 takes the bit 30 from the MSB, which is the LSB bit 1), the 0x40 bit of
// the byte at guest this+0xC66 is cleared (0x000073B8, the bit 25 from the MSB), the module's
// word at guest this+0x924 becomes 0 (0x000073C0) and the animation state becomes "ready"
// (0x000073C4). Then the cover point the module holds (the id at guest this+0xBD2) is looked up
// (0x000073D0); when there is none the state does nothing more (0x000073D4..0x000073D8).
// Otherwise the connected waypoint is looked for (the guest's "ARRV" state on the "NEXT"
// message, 0x000073E8..0x00007414) and, when the connection is a waypoint, its id goes to guest
// this+0xBD6 (0x00007454..0x00007458); the cover point's forward vector (its transform's 0x28,
// 0x38 and 0x48) goes to guest this+0xBA0 (0x0000745C..0x00007470).
//
// Update (0x00007478): the module's word at guest this+0xAF0 is cleared (0x0000747C - the burst
// fire's own type) and the word at guest this+0x924 picks the block:
//   - 0 (0x0000749C): the angle between the actor's facing and the cover point's forward (the
//     GetAngleDiff call at 0x000074BC) is compared with 10 degrees (rodata_498 = 0.17453292,
//     0x000074C8): when the commando faces the cover point the word becomes 1 (0x00007528),
//     otherwise the locomotion command that looks at the cover point goes to the body controller
//     (the zero move vector, the cover point's forward as the look direction and the weight 1.0,
//     0x000074D0..0x00007520);
//   - 1 (0x00007534): the animation state is asked whether the wall-hang command may be issued
//     (0x0000753C, the guest's command id 0x14 = 20 = kAS_WallHang). When it may, the wall-hang
//     command goes to the body controller (0x00007588, the guest's command id 0x18 = 24 =
//     kBSC_WallHang) and the guest keeps the command id and the waypoint id in the command
//     manager's wall-hang command (body controller +4, then +0x2F4 and +0x2F8,
//     0x0000759C..0x000075AC). When it may not, the body must be in the wall-hang state (20, 0x000075C0): then the
//     module's helper asks the target for its aim position (0x000075E0, the dt 0.0) and the
//     direction from the actor to that aim position goes into the body controller's own vector
//     at +0x34 (0x00007620..0x00007628), the command manager's target vector (+0x30).
//     The aim position's z is cleared (0x00007618) after the direction is worked out.
//
// Deactivate (0x00007630): the animation state goes back to "not ready" (0x00007638), the 0x02
// bit of the flag byte at guest this+0xC65 is cleared (0x00007640) and the module's word at
// guest this+0x924 goes back to -1 (0x00007648).
void CCommandoPirate::WallHang(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
    case kStateMsg_Activate: {                                       // 0x0000739C
      xc65_ |= 0x02;                                                 // 0x000073A4..0x000073AC
      xc66_ &= ~0x40;                                                // 0x000073B8..0x000073BC
      x924_ = 0;                                                     // 0x000073C0
      mAnimationState.SetState(CAnimationState::kAS_Ready);          // 0x000073C4
      const CScriptCoverPoint* cover = GetCoverPoint(mgr, xbd2_);     // 0x000073C8..0x000073D0
      if (cover == nullptr) {                                        // 0x000073D4..0x000073D8
        break;
      }
      // 0x000073E8..0x00007414: the connected waypoint ("ARRV" on "NEXT").
      const TUniqueId waypointId =
          FindConnectedObject_if(mgr, kSS_Arrived, kSM_Next, skWaypointPredicate);
      const CScriptWaypoint* waypoint =
          TCastToConstPtr< CScriptWaypoint >(mgr.ObjectById(waypointId));  // 0x00007428..0x0000742C
      if (waypoint != nullptr) {                                     // 0x0000744C..0x00007458
        xbd6_ = waypoint->GetUniqueId();
      }
      xba0_ = cover->GetTransform().GetForward();                    // 0x0000745C..0x00007470
      break;
    }
    case kStateMsg_Update: {                                         // 0x00007478
      mBurstFire.SetBurstType(0);                                    // 0x0000747C
      if (x924_ == 0) {                                              // 0x00007480..0x00007498
        // 0x0000749C..0x000074BC: the actor's facing (the transform's 0x28, 0x38 and 0x48) and
        // the cover point's forward; the guest's three-argument GetAngleDiff has no host
        // counterpart, the host's takes the facing and the direction.
        const float angle = CVector3f::GetAngleDiff(GetTransform().GetForward(), xba0_);
        if (angle <= 0.17453292f) {                                  // rodata_498, 0x000074C8
          x924_ = 1;                                                 // 0x00007528
        } else {
          BodyController()->CommandMgr().DeliverCmd(                 // 0x00007520
              CBCLocomotionCmd(CVector3f::Zero(), xba0_, 1.0f));     // rodata_43C, 0x000074D0
        }
      } else if (x924_ == 1) {                                       // 0x00007488..0x00007534
        if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_WallHang)) {  // 0x0000753C
          // 0x0000754C..0x000075AC: the guest's inlined DeliverCmd(CBCWallHangCmd(xbd6_)) - the
          // command id, then the waypoint id into the command manager's wall-hang command
          // (cmd mgr +0x2F4/+0x2F8), which CBSWallHang reads.
          BodyController()->CommandMgr().DeliverCmd(CBCWallHangCmd(xbd6_));  // 0x00007588
        } else if (BodyController()->GetCurrentStateId() == pas::kAS_WallHang) {  // 0x000075C0
          if (x3CBC_(mgr, xb94_, 0.0f)) {                            // 0x000075E0
            // 0x000075EC..0x00007614: the direction from the actor to the aim position.
            const CVector3f direction = xb94_ - GetTranslation();
            xb94_.SetZ(0.0f);                                         // 0x00007618
            // 0x00007620..0x00007628: the direction goes into the body controller's own vector
            // at +0x34 - the host's command manager's target vector.
            BodyController()->CommandMgr().SetTargetVector(direction);
          }
        }
      }
      break;
    }
    case kStateMsg_Deactivate: {                                    // 0x00007630
      mAnimationState.SetState(CAnimationState::kAS_NotReady);       // 0x00007638
      xc65_ &= ~0x02;                                                // 0x00007640..0x00007644
      x924_ = -1;                                                    // 0x00007648
      break;
    }
    default:
      break;
  }
}

// fn_9_728C (0xcc bytes): the WallDetach state.
//
// On activate (the guest's message 0, 0x000072C4) the flag byte at guest this+0xC65 gets its
// 0x02 bit (0x000072C4..0x000072D0) and the animation state goes "ready" (0x000072D4).
//
// On update (the guest's message 1, 0x000072DC) the body is told to leave the state (the
// exit-state command, the id 0xa, 0x000072DC..0x000072FC) and, unless the body is in the wall
// hang state (the body's state id 20, the compare at 0x0000731C), the animation state goes
// "over" (0x00007324..0x00007328).
//
// On deactivate (the guest's message 2, 0x00007330) the animation state goes "not ready"
// (0x00007330..0x00007334) and the flag byte at guest this+0xC65 loses its 0x02 bit
// (0x00007338..0x00007340).
// run: the Swamp (05_swamp_timber!), tools/drivers/commandopirate_wallhang.py, 2026-10-21
// @ fn_9_728C
void CCommandoPirate::WallDetach(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {  // 0x000072C4
    xc65_ |= 0x02;                                         // 0x000072C4..0x000072D0
    mAnimationState.SetState(CAnimationState::kAS_Ready);  // 0x000072D4
    break;
  }
  case kStateMsg_Update: {  // 0x000072DC
    BodyController()->CommandMgr().DeliverCmd(kBSC_ExitState);  // 0x000072DC..0x000072FC (the id 0xa)
    if (BodyController()->GetCurrentStateId() != pas::kAS_WallHang) {  // 0x00007318..0x00007320
      mAnimationState.SetState(CAnimationState::kAS_Over);  // 0x00007324..0x00007328
    } else {
    }
    break;
  }
  case kStateMsg_Deactivate: {  // 0x00007330
    mAnimationState.SetState(CAnimationState::kAS_NotReady);  // 0x00007330..0x00007334
    xc65_ &= ~0x02;                                          // 0x00007338..0x00007340
    break;
  }
  default:
    break;
  }
}

// @ fn_9_709C
// fn_9_709C (0x1f0 bytes): the CoverFind state. Only the activate message (the guest's test of
// 0 at 0x000070BC) does anything: the module's flag byte at guest this+0xC63 loses its 0x40 bit
// (the rlwimi at 0x000070D8) and then its 0x01 bit (the rlwimi at 0x000070F0), and the module's
// own helper fn_9_3CBC has to answer with the target's aim position (the call at 0x000070F8),
// which it stores at guest this+0xB94 (the argument at 0x000070E8).
//
// The AiWaypoint list (the state manager's +0x838, the load at 0x0000710C) is scanned for the
// cover point: it has to be active (the byte at +0x20's 0x80 bit, the extrwi at 0x00007150),
// refuse a landing (ShouldLandHere, the call at 0x00007158), be free (GetInUse with this
// actor's id, the call at 0x00007178), be in this area (+0x04, the compare at 0x0000718C) and not
// be the cover point the actor already holds (+0xBD4, the compare at 0x0000719C). It has to be
// nearer than the block's search radius (the guest's this+0x814, the block's +0x54, squared at
// 0x00007114) or nearer than the best candidate so far (the compare at 0x000071D4), must not be
// blown (the Blown call at 0x000071E4) and the path find search (the guest's GetSearchPath, the
// vtable call at 0x000071FC) has to connect the actor to it (the PathExists call at
// 0x0000720C). The winner is handed to the base's SetCoverPoint (the call at 0x0000725C), which
// writes the id at guest this+0xBD2, and that id is copied to +0xBD4 (the store at 0x00007264).
void CCommandoPirate::CoverFind(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {
    return;                                     // 0x000070BC..0x000070C8
  }
  xc63_ &= ~0x40;                               // 0x000070D0..0x000070E0
  xc63_ &= ~0x01;                               // 0x000070EC..0x000070F4
  if (!x3CBC_(mgr, xb94_, 0.f)) {                // 0x000070F8..0x00007100
    return;
  }
  CScriptCoverPoint* best = nullptr;            // r31
  float bestDistSq = mData.mSearchRadius * mData.mSearchRadius;  // 0x00007104..0x00007114
  CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint);  // 0x0000710C
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    CScriptCoverPoint* cover = TCastToPtr< CScriptCoverPoint >(list[i]);
    if (cover == nullptr || !cover->GetActive() ||            // 0x00007144..0x00007154
        cover->ShouldLandHere() ||                            // 0x00007158..0x00007160
        cover->GetInUse(GetUniqueId()) ||                      // 0x00007178..0x00007180
        cover->GetCurrentAreaId() != GetCurrentAreaId() ||    // 0x00007184..0x00007190
        cover->GetUniqueId() == xbd4_) {                       // 0x00007194..0x000071A0
      continue;
    }
    const float distSq = (cover->GetTranslation() - GetTranslation()).MagSquared();  // 0x000071A4..0x000071D0
    if (distSq >= bestDistSq) {                // 0x000071D4..0x000071D8
      continue;
    }
    if (cover->Blown(xb94_)) {                 // 0x000071DC..0x000071EC
      continue;
    }
    if (mPathFindSearch.PathExists(GetTranslation(), cover->GetTranslation()) !=
        CPathFindSearch::kR_Success) {          // 0x000071F0..0x00007214
      continue;
    }
    bestDistSq = distSq;                        // 0x00007218
    best = cover;                               // 0x0000721C
  }
  if (best != nullptr) {                        // 0x00007244..0x0000724C
    CPatterned::SetCoverPoint(best, xbd2_);     // 0x00007250..0x0000725C
    xbd4_ = xbd2_;                              // 0x00007260..0x00007264
  }
}

// fn_9_A7A8 (0x90 bytes): the SetCoverDest state. Only the activate message (the guest's test of
// 0 at 0x0000A7B0) does anything: the cover point (the id at guest this+0xBD2, 0x0000A7C4) is
// looked up (the GetCoverPoint call at 0x0000A7D0) and the path find navigation's destination
// becomes that point's position (0x0000A7DC..0x0000A7E4) or the actor's own position when there
// is no cover point (0x0000A7EC..0x0000A7F4). The navigation's face target id (guest this+0x718)
// takes the module's target (the id at guest this+0xBCA, 0x0000A7F8..0x0000A7FC) and, when the
// byte at guest this+0xC66 has no 0x40 bit (the test at 0x0000A804), that bit is set
// (0x0000A80C..0x0000A81C) and the module's float at guest this+0xB8C goes to zero
// (0x0000A820).
void CCommandoPirate::SetCoverDest(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {  // 0x0000A7B0..0x0000A7C0
    return;
  }
  const CScriptCoverPoint* cover = GetCoverPoint(mgr, xbd2_);  // 0x0000A7C4..0x0000A7D0
  if (cover != nullptr) {                                     // 0x0000A7D4..0x0000A7D8
    mPathFindNavigation.SetDestination(cover->GetTranslation());  // 0x0000A7DC..0x0000A7E4
  } else {
    mPathFindNavigation.SetDestination(GetTranslation());  // 0x0000A7EC..0x0000A7F4
  }
  mPathFindNavigation.SetFaceTarget(xbca_);  // 0x0000A7F8..0x0000A7FC
  if ((xc66_ & 0x40) == 0) {              // 0x0000A800..0x0000A808
    xc66_ |= 0x40;                        // 0x0000A80C..0x0000A81C
    xb8c_ = 0.f;                          // 0x0000A820
  }
}

// fn_9_6DBC (Cover): PortCCommandoPirateCore.cpp.

// fn_9_6C68 (CoverAttack): PortCCommandoPirateCore.cpp.

// @ fn_9_6BCC
// fn_9_6BCC (0x9c bytes): the BreakCover state. Only the activate message (the guest's test of 0
// at 0x00006BEC) does anything: the body is told to leave the state (the exit-state command, the
// id 0xa, 0x00006BF4..0x00006C14), the cover point (the id at guest this+0xBD2) is released
// with the cooldown kept (0x00006C2C..0x00006C3C) and the byte at guest this+0xC66 loses its
// 0x40 bit (0x00006C40..0x00006C4C).
void CCommandoPirate::BreakCover(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {  // 0x00006BEC..0x00006BF0
    return;
  }
  BodyController()->CommandMgr().DeliverCmd(kBSC_ExitState);  // 0x00006BF4..0x00006C14 (the id 0xa)
  ReleaseCoverPoint(mgr, xbd2_, true);  // 0x00006C2C..0x00006C3C
  xc66_ &= ~0x40;                     // 0x00006C40..0x00006C4C
}

// fn_9_6A40 (GetUp): PortCCommandoPirateCore.cpp.

// fn_9_69C8 (0x78 bytes): the Dead state. The base's own dead handler runs first (the call at
// 0x000069EC); the activate message (the guest's test of 0 at 0x000069F0) then takes the byte at
// guest this+0xC66's 0x40 bit away (0x000069FC..0x00006A08) and, when that byte still has its
// 0x20 bit (the test at 0x00006A10), the module's shield effect runs (0x00006A18..0x00006A20).
void CCommandoPirate::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  CPatterned::Dead(mgr, msg, dt);  // 0x000069EC
  if (msg != kStateMsg_Activate) {  // 0x000069F0..0x000069F8
    return;
  }
  xc66_ &= ~0x40;                        // 0x000069FC..0x00006A08
  if ((xc66_ & 0x20) != 0) {             // 0x00006A0C..0x00006A14
    x2048_ShieldEffect(mgr);             // 0x00006A18..0x00006A20
  }
}

// @ fn_9_BDE0
// vtable slot 60 = fn_9_BDE0 (0xBDE0..0xBEB4): the base's setup first (the call at 0x0000BDF4),
// then the module's two tables on the state manager - the triggers (lbl_9_data_198, 34 = 0x22)
// through the vtable slot at +0x18 (the call at 0x0000BE14) and the states (lbl_9_data_568,
// 36 = 0x24) through +0x14 (the call at 0x0000BE34). The module has no third table: the
// records between the two tables' stores run out at the state's last entry.
// One arm. Run: tools/cov/run.sh with tools/cov/specs/commandopirate.py and
// tools/drivers/commandopirate_core.py, area 59 (05_temple_federationsite_dark) and 05_swamp_timber!:
// once per commando (1 in area 59, 2 in the Swamp), the base's call each time.
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
      static_cast< ushort >(grenade.sound_GrenadeExplode),
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
