// Port-only: the DigitalGuardian REL's head module (module 14, CDigitalGuardianHead, script type
// DGHD, docs/module-analysis/DigitalGuardian.md sections b.5-b.7, c.3 and f.2). The retail class is
// 0x10F8 bytes, built by fn_14_18264. This slice is the loader, the constructor, the link to the
// body ("XALD", fn_14_13830), Think, AcceptScriptMsg, the state machine's tables, the 20 triggers
// and the two smallest states; the other FSM functions and the module callees they reach are
// once-logging stand-ins named after the retail function. The comments cite the REL's .text
// offsets.

#include "MetroidPrime/Enemies/CDigitalGuardianHead.hpp"

#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CEchoEmitter.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CObjectList.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Weapons/CBomb.hpp"
#include "MetroidPrime/Weapons/CWeapon.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CValidEntityPredicate.hpp"
#include "MetroidPrime/Enemies/CDigitalGuardian.hpp"
#include "MetroidPrime/Enemies/CDigitalGuardianEchoTarget.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Enemies/CSplitterCommandModule.hpp"
#include "MetroidPrime/Enemies/CSplitterMainChassis.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptPlatform.hpp"
#include "MetroidPrime/ScriptObjects/CScriptPlayerHint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include <float.h>
#include <stdio.h>

namespace {

// Reports a stand-in the first time it runs, so a log lists what the head still lacks. The
// message names the retail function the entry stands in for.
void StandInFn(const char* fn) {
  static const char* sSeen[128] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == fn) {
      return;
    }
  }
  if (sCount < 128) {
    sSeen[sCount++] = fn;
  }
  fprintf(stderr, "[dghd] stand-in %s\n", fn);
}

// lbl_14_data_1024: the waypoint predicate fn_14_13830 and fn_14_119E4 pass (its IsValid is a
// TCastToPtr<CScriptWaypoint> of the object).
class CWaypointPredicate : public CValidEntityPredicate {
public:
  ~CWaypointPredicate() override {}
  bool IsValid(const CStateManager& mgr, TUniqueId id) const override {
    return TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(id)) != nullptr;
  }
};

// lbl_14_data_1008: the platform predicate fn_14_13830 passes (TCastToPtr<CScriptPlatform>).
class CPlatformPredicate : public CValidEntityPredicate {
public:
  ~CPlatformPredicate() override {}
  bool IsValid(const CStateManager& mgr, TUniqueId id) const override {
    return TCastToConstPtr< CScriptPlatform >(mgr.GetObjectById(id)) != nullptr;
  }
};

#define DGHD_TRIGGER(name)                                                                         \
  { #name, reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CDigitalGuardianHead::name) }
#define DGHD_STATE(name)                                                                           \
  { #name, reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CDigitalGuardianHead::name) }
#define DGHD_CODE(name)                                                                            \
  { #name, reinterpret_cast< CPatterned::StateMachine::CodeFunc >(&CDigitalGuardianHead::name) }

// The head's three FSM tables, in retail order: lbl_14_data_808 (20 triggers), lbl_14_data_A14
// (17 states) and lbl_14_data_C80 (29 codes). The host state machine matches a node by its name.
CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    DGHD_TRIGGER(StateOver),           DGHD_TRIGGER(MainBodyDestroyed),
    DGHD_TRIGGER(ShouldProjectileAttack), DGHD_TRIGGER(IsBurstOver),
    DGHD_TRIGGER(IsIdle),              DGHD_TRIGGER(IsStunned),
    DGHD_TRIGGER(IsGuarding),          DGHD_TRIGGER(UseMachineGun),
    DGHD_TRIGGER(UseLockOnMissiles),   DGHD_TRIGGER(UseAnnihilatorPulse),
    DGHD_TRIGGER(UseAnnihilatorCharge), DGHD_TRIGGER(AllSplittersDead),
    DGHD_TRIGGER(HeadArmorDestroyed),  DGHD_TRIGGER(IsAntennaTransmitting),
    DGHD_TRIGGER(HasLineOfSight),      DGHD_TRIGGER(EchoTargetDestroyed),
    DGHD_TRIGGER(AllEchoTargetsDestroyed), DGHD_TRIGGER(ShouldRetreat),
    DGHD_TRIGGER(BombPitDestroyed),    DGHD_TRIGGER(IsDead),
};

CPatterned::StateMachine::SStateFunction skStates[] = {
    DGHD_STATE(Start),            DGHD_STATE(Idle),
    DGHD_STATE(FaceTarget),       DGHD_STATE(ProjectileTelegraph),
    DGHD_STATE(PulseAttack),      DGHD_STATE(ChargeAttack),
    DGHD_STATE(MissileAttack),    DGHD_STATE(MachineGunAttack),
    DGHD_STATE(AttachedToBody),   DGHD_STATE(DetachFromBody),
    DGHD_STATE(StunnedReaction),  DGHD_STATE(DeploySplitters),
    DGHD_STATE(BombedReaction),   DGHD_STATE(SeekAttackPosition),
    DGHD_STATE(SeekMotionPath),   DGHD_STATE(FollowRetreatPath),
    DGHD_STATE(Dead),
};

CPatterned::StateMachine::SCodeFunction skCodes[] = {
    DGHD_CODE(SetPlayerFaceTarget),      DGHD_CODE(ResetProjectileAttackTimes),
    DGHD_CODE(SelectProjectileAttack),   DGHD_CODE(SetPlatformPosition),
    DGHD_CODE(SetStunnedPosition),       DGHD_CODE(SeekLineOfSight),
    DGHD_CODE(DropShields),              DGHD_CODE(RaiseShields),
    DGHD_CODE(ExtractAntenna),           DGHD_CODE(ExtractEchoTargets),
    DGHD_CODE(RetractEchoTargets),       DGHD_CODE(DestroyActiveSplitters),
    DGHD_CODE(StartSplitterDeployment),  DGHD_CODE(EndSplitterDeployment),
    DGHD_CODE(Stage3Start),              DGHD_CODE(EnableDynamicCollision),
    DGHD_CODE(DisableDynamicCollision),  DGHD_CODE(EnableValidTarget),
    DGHD_CODE(DisableValidTarget),       DGHD_CODE(SelectDeathHeadAttack),
    DGHD_CODE(SetInitialAttackPosition), DGHD_CODE(SetAttackPosition),
    DGHD_CODE(ResetGravityRepulsorHP),   DGHD_CODE(EjectPlayerFromBombPit),
    DGHD_CODE(GrabPlayerHintControl),    DGHD_CODE(SetVulnerableToDamage),
    DGHD_CODE(SetImmuneToDamage),        DGHD_CODE(SelectRetreatPlatform),
    DGHD_CODE(StartVocalRecoverReaction),
};

#undef DGHD_TRIGGER
#undef DGHD_STATE
#undef DGHD_CODE

} // namespace

// fn_14_18264. The base call is the retail one: EPatternedAI 0xA (li r4 at 0x182B8), flavor 0
// (li r7 at 0x182A4), movement 1 (flyer), collider 1 and body 7 (kBT_AiMovedFlyer) in the stack
// words at 0x182B0..0x182BC. The ctor's other members (the visual knowledge at 0xD80, the four
// projectile infos at 0xE04..0xE7C, the 'PART' and 'SCAN' tokens, the head armour model and the
// locator ids) belong to the states this slice does not port, so the host keeps only what the
// ported functions read, with the values the retail stores.
CDigitalGuardianHead::CDigitalGuardianHead(TUniqueId uid, const rstl::string& name,
                                           const CEntityInfo& info, const CTransform4f& xf,
                                           const CModelData& modelData,
                                           const CActorParameters& actParms,
                                           const CPatternedInfo& pInfo,
                                           const SLdrDigitalGuardianHeadData& data)
: CPatterned(static_cast< EPatternedAI >(0xA), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Flyer, kCT_One, kBT_AiMovedFlyer, actParms)
, mData(data)
, xD70_(nullptr)
, xDC4_{kInvalidUniqueId, kInvalidUniqueId} // 0x18358
, xDD0_(LdrToDamageVulnerability(data.bombPitVulnerability)) // 0x18370
, xE00_(kInvalidUniqueId)
, xDA0_(kInvalidUniqueId)
, xDB0_(0.f)
, xDB8_(0)
, xEF4_()
, xF04_()
, xF14_()
, xF34_(3, kInvalidUniqueId) // 0x188A8..0x188C8
, xF40_{CTransform4f::Identity(), CTransform4f::Identity(), CTransform4f::Identity()} // 0x188CC
, xFD0_()
, xFE0_(3)
, xFE8_(1)
, xFEC_(CVector3f::Zero())
, xFF8_(CVector3f::Zero())
// 0x1004: the translation, then the transform's column 1 and the negated column 0 (fn_14_190E0).
, x1004_(GetTranslation())
, x1010_(GetTransform().GetColumn(kDY))
, x101C_(-GetTransform().GetColumn(kDX))
, x1028_(CVector3f::Zero())
, x1034_(kInvalidUniqueId)
, x1036_(kInvalidUniqueId)
, x1038_(kInvalidUniqueId)
, x103A_(kInvalidUniqueId)
, x103C_(kInvalidUniqueId)
, x103E_(kInvalidUniqueId)
, x1040_(kInvalidUniqueId)
, x1044_()
, x10D0_(-1)
, x10D4_(0.f)
, x10D8_(0)
, x10DC_(-1)
, x10E0_(data.gravityRepulsorHP) // data +0x18
, x10E4_(2)
, x10E8_(0)
, x10EC_(0.f)
, x10F0_(0.f)
, x10F4_(0x08) // only bit 0x08 is set (the rlwimi stores at the ctor's tail)
, x10F5_(0)    // bit 0x80 cleared
{
  // The drop-shields effect (data +0x48, 0x184D8): locked when set.
  if (mData.dropShieldsFx != kInvalidAssetId) {
    xEF4_ = TLockedToken< CGenDescription >(
        gpSimplePool->GetObj(SObjectTag('PART', mData.dropShieldsFx)));
  }
  // The 'PART' token (0x185A0..0x18654): locked when data +0x94 is set.
  if (mData.pART != kInvalidAssetId) {
    xF04_ = TLockedToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', mData.pART)));
  }
  // The bomb-slot-destroyed effect (data +0x138, 0x18708) and the echo-target-destroyed one
  // (data +0xA4, 0x18900), each locked when set.
  if (mData.bombSlotDestroyedFx != kInvalidAssetId) {
    xF14_ = TLockedToken< CGenDescription >(
        gpSimplePool->GetObj(SObjectTag('PART', mData.bombSlotDestroyedFx)));
  }
  if (mData.echoTargetDestroyedFx != kInvalidAssetId) {
    xFD0_ = TLockedToken< CGenDescription >(
        gpSimplePool->GetObj(SObjectTag('PART', mData.echoTargetDestroyedFx)));
  }
  // 0x18D28 / 0x18D5C: the balls' locators (.rodata 0x954's names) at 0xDCC / 0xDCD.
  const CAnimData* anim = GetModelData()->GetAnimationData();
  xDCC_[0] = anim->GetLocatorSegId(rstl::string_l("L_ball_LCTR"));
  xDCC_[1] = anim->GetLocatorSegId(rstl::string_l("R_ball_LCTR"));
}

CDigitalGuardianHead::~CDigitalGuardianHead() {}

// The body controller is activated by "XCRT" in retail; the port also activates it before the
// first Think, which can run first (docs/GOTCHAS.md, patterned actors).
void CDigitalGuardianHead::EnsureBodyActive(CStateManager& mgr) {
  if (!BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// fn_14_17C98: only while active. The base's Think, then the collision actor manager's update
// (object space, the li r5, 0 at 0x17CD8), the visual knowledge's update (0xD80), the timers
// (fn_14_13AE0), fn_14_12818 and fn_14_1223C.
void CDigitalGuardianHead::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
  if (xD70_ != nullptr) { // retail calls it unchecked; the port builds no manager yet
    xD70_->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  }
  StandInFn("CVisualKnowledge::Update (head 0xD80)");
  x13AE0_(dt);
  x12818_(mgr, dt);
  x1223C_(mgr);
}

// fn_14_13AE0: the projectile attack timer counts down, the two timers at 0x10EC and 0x10F0 up.
// @ fn_14_13AE0
void CDigitalGuardianHead::x13AE0_(float dt) {
  x10D4_ -= dt;
  x10EC_ += dt;
  x10F0_ += dt;
}

namespace {
// .rodata 0x928: the three spheres (name, radius); .rodata 0x9D4 the mass (1000).
struct SHeadSphereJoint {
  const char* name;
  float radius;
};
const SHeadSphereJoint skHeadSphereJoints[3] = {
    {"L_ball_LCTR", 1.5f},
    {"R_ball_LCTR", 1.5f},
    {"Skeleton_Root", 6.f},
};
const float kHeadJointMass = 1000.f;
// .rodata 0x954: the ball names fn_14_13284 compares.
const char* const skHeadBallJoints[2] = {"L_ball_LCTR", "R_ball_LCTR"};
} // namespace

// @ fn_14_13284
// The loop at 0x132AC: the name against the two ball names (fn_14_21A0, operator==); the id goes
// to 0xDC4 + 2 * index (0x132E8).
bool CDigitalGuardianHead::x13284_(const rstl::string& name, const TUniqueId& id) {
  for (int i = 0; i < 2; ++i) {
    if (name == rstl::string_l(skHeadBallJoints[i])) {
      xDC4_[i] = id;
      return true;
    }
  }
  return false;
}

// @ fn_14_13318
// The three spheres (the loop at 0x13378, room for 3 reserved at 0x13348), built active (li r8, 1
// at 0x1345C); every collider (the loop at 0x13490) gets the include list's high word | 0x4000000
// (material 58) first, then a ball the pass-through vulnerability and the exclude bits of .data
// 0xE64..0xE74 (33, 47, 49, 60, 32), the root the immune-ricochet one and the bits of 0xE78..0xE84
// (33, 47, 49, 60) and its id at 0xE00; then the knockback resistance .rodata 0x96C (1, the
// HealthInfo slot at 0x13690). The head's own filter (0x136C0): include .data 0xE88 (59), exclude
// 0xE8C..0xE9C (33, 32, 47, 49, 60), type 3. __shl2i(0, 1, n) is the 64-bit 1 << n.
void CDigitalGuardianHead::x13318_(CStateManager& mgr) {
  rstl::vector< CJointCollisionDescription > joints;
  joints.reserve(3); // fn_14_D7DC
  const CAnimData* anim = GetModelData()->GetAnimationData();
  for (uint i = 0; i < 3; ++i) {
    const SHeadSphereJoint& joint = skHeadSphereJoints[i];
    const CSegId seg = anim->GetLocatorSegId(rstl::string_l(joint.name));
    // fn_14_5FA4 (the description's copy)
    // fn_14_5F24 (push_back)
    joints.push_back(CJointCollisionDescription::SphereCollision(
        seg, CVector3f::Zero(), joint.radius, rstl::string_l(joint.name), kHeadJointMass));
  }
  CCollisionActorManager* manager =
      rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), joints, true);
  delete xD70_;
  xD70_ = manager;

  for (uint i = 0; i < xD70_->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc = xD70_->GetCollisionDescFromIndex(i);
    const TUniqueId id = desc.GetCollisionActorId();
    CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(id));
    if (actor == nullptr) { // the TCastToPtr test at 0x134BC
      continue;
    }
    CMaterialFilter filter = actor->GetMaterialFilter();
    filter.IncludeList().Add(kMT_NoPlayerCollision);
    if (x13284_(desc.GetName(), id)) {
      actor->SetDamageVulnerability(CDamageVulnerability::PassThroughVulnerabilty());
      filter.ExcludeList().Add(kMT_Character);
      filter.ExcludeList().Add(kMT_CollisionActor);
      filter.ExcludeList().Add(kMT_Platform);
      filter.ExcludeList().Add(kMT_NoPlatformCollision);
      filter.ExcludeList().Add(kMT_Player);
    } else {
      actor->SetDamageVulnerability(CDamageVulnerability::ReflectVulnerabilty());
      filter.ExcludeList().Add(kMT_Character);
      filter.ExcludeList().Add(kMT_CollisionActor);
      filter.ExcludeList().Add(kMT_Platform);
      filter.ExcludeList().Add(kMT_NoPlatformCollision);
      xE00_ = id;
    }
    actor->SetMaterialFilter(filter);
    actor->HealthInfo()->SetKnockbackResistance(1.f);
  }
  CMaterialList exclude;
  exclude.Add(kMT_Character);
  exclude.Add(kMT_Player);
  exclude.Add(kMT_CollisionActor);
  exclude.Add(kMT_Platform);
  exclude.Add(kMT_NoPlatformCollision);
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), exclude));
  // fn_14_60C0 (the vector's destructor, 0x13790)
}

// fn_14_12818: bit 0x80 of 0x10F5 (a ball held) is cleared, then only with HeadArmorDestroyed
// (0x10F4 bit 0x10) each ball (.rodata 0x928's radius): a destroyed slot's looping sound (0xF28)
// follows its locator (translation, column 1, volume 0x7F) and skips the rest; otherwise a fully
// morphed player (0x38C when 0x390 is 0) on an active ball collider within the radius
// (|player + (0, 0, ball radius) - ball|^2 <= r^2, the cror at 0x129E8) is stopped and moved onto
// it, the 0x1038 platform excludes the player (material 32, the ori at 0x12A38), the ball gets
// 0xDD0, bit 0x80 is set and the bombs there are pulled in (fn_14_126C8 with r^2); out of range
// the ball is pass-through again. .rodata 0x960 is 0.
// Unmarked: the area-53 fight ran the unmorphed, 0xF28 and in-range platform arms; the inactive
// ball arm never ran and the out-of-range arm was not traced.
void CDigitalGuardianHead::x12818_(CStateManager& mgr, float dt) {
  x10F5_ &= ~0x80;
  if ((x10F4_ & 0x10) == 0) {
    return;
  }
  CPlayer* player = mgr.Player(0);
  for (int i = 0; i < 2; ++i) {
    if (xF28_[i]) {
      const CTransform4f xf = GetLctrTransform(xDCC_[i]);
      CSfxManager::UpdateEmitter(xF28_[i], xf.GetTranslation(), xf.GetColumn(kDY), 0x7F);
      continue;
    }
    const CPlayer::EPlayerMorphBallState state =
        player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed
            ? player->GetMorphballTransitionState()
            : CPlayer::kMS_Unmorphed;
    if (state != CPlayer::kMS_Morphed) {
      continue;
    }
    CCollisionActor* ball = TCastToPtr< CCollisionActor >(mgr.ObjectById(xDC4_[i]));
    if (ball == nullptr || !ball->GetActive()) {
      continue;
    }
    const CVector3f ballPos = ball->GetTranslation();
    const float ballRadius = player->GetMorphBall()->GetBallRadius();
    const float r2 = skHeadSphereJoints[i].radius * skHeadSphereJoints[i].radius;
    const CVector3f playerPos = player->GetTranslation();
    const float dx = (playerPos.GetX() + 0.f) - ballPos.GetX();
    const float dy = (playerPos.GetY() + 0.f) - ballPos.GetY();
    const float dz = (playerPos.GetZ() + ballRadius) - ballPos.GetZ();
    if (dz * dz + (dx * dx + dy * dy) <= r2) {
      player->Stop();
      player->MoveToWR(
          CVector3f(ballPos.GetX() - 0.f, ballPos.GetY() - 0.f, ballPos.GetZ() - ballRadius), dt);
      if (CScriptPlatform* plat = TCastToPtr< CScriptPlatform >(mgr.ObjectById(x1038_))) {
        CMaterialFilter filter = plat->GetMaterialFilter();
        filter.ExcludeList().Add(kMT_Player);
        plat->SetMaterialFilter(filter);
      }
      ball->SetDamageVulnerability(xDD0_);
      x10F5_ |= 0x80;
      x126C8_(mgr, ball->GetTranslation(), r2);
    } else {
      ball->SetDamageVulnerability(CDamageVulnerability::PassThroughVulnerabilty());
    }
  }
}

// fn_14_126C8: the near list in the box pos -/+ half, filtered to include material 36 (.data
// 0xEA0, the bombs) only; every CBomb in it is moved to pos.
// @ fn_14_126C8
void CDigitalGuardianHead::x126C8_(CStateManager& mgr, const CVector3f& pos, float half) {
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  const CVector3f center = pos;
  const CAABox box(CVector3f(center.GetX() - half, center.GetY() - half, center.GetZ() - half),
                   CVector3f(center.GetX() + half, center.GetY() + half, center.GetZ() + half));
  mgr.BuildNearList(nearList, box, CMaterialFilter::MakeInclude(CMaterialList(kMT_Bomb)), this);
  for (rstl::reserved_vector< TUniqueId, 1024 >::iterator it = nearList.begin();
       it != nearList.end(); ++it) {
    if (CBomb* bomb = TCastToPtr< CBomb >(mgr.ObjectById(*it))) {
      bomb->SetTranslation(center);
    }
  }
}

// fn_14_17D38: the active flag is read before the base's call (0x17D64..0x17D78).
void CDigitalGuardianHead::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const bool active = GetActive();
  CPatterned::AcceptScriptMsg(mgr, msg);
  switch (static_cast< uint >(msg.GetMessage())) {
  case kSM_Create: // "XCRT", 0x17E18
    BodyController()->Activate(mgr, pas::kAS_Invalid);
    x13318_(mgr);
    SetVisorOrbitableFlags(CVisorParameters::kVOF_All, false);
    RemoveMaterial(kMT_SeekerTarget, mgr); // li r4, 0x3F
    x12314_(mgr);
    break;
  case kSM_Delete: // "XDEL", 0x17E64
    if (xD70_ != nullptr) {
      xD70_->Destroy(mgr);
    }
    StandInFn("fn_14_11EA8 (head XDEL)");
    StandInFn("fn_14_FAEC (head XDEL)");
    StandInFn("fn_14_11F7C (head XDEL)");
    break;
  case kSM_Activate: // "ACTV", 0x17E9C
    if (!active && xD70_ != nullptr) {
      xD70_->SetActive(mgr, true);
    }
    break;
  case kSM_Deactivate: // "DCTV", 0x17EC0
    if (active && xD70_ != nullptr) {
      xD70_->SetActive(mgr, false);
    }
    break;
  case kSM_AIUpdateDisabled: // "XAUD", 0x17EE4
    if (xD70_ != nullptr) {
      xD70_->SetPhysicsActive(mgr, false);
    }
    break;
  case kSM_AreaLoaded: // "XALD", 0x17F00
    x13830_(mgr);
    break;
  case kSM_Damage: // "XDMG", 0x17F10
    x1303C_(mgr, msg.GetSenderId());
    break;
  case kSM_ResistedDamage: // "XRDG", 0x17F2C
    x12D98_(mgr, msg.GetSenderId());
    break;
  default:
    break;
  }
}

// fn_14_12D98: "XRDG". With the armour gone (0x10F4 bit 0x10) and the sender the root collider
// (0xE00), the weapon that last touched it (not the one already counted, 0x1040) and that its
// vulnerability (vtable 0x40) lets hit: the vulnerable damage, if above .rodata 0x960 (0), hurts
// the head along its column 1 (vtable 0xA4), flashes the echo emitter (0xC8) for
// skDamageHitTime and, above .rodata 0x9B0 (25) with the weapon still there, flinches the body
// (state cmd 0x1C, weight .rodata 0x96C = 1). Then the gravity repulsor's health (0x10E0) drops
// by it (even at 0), the weapon is remembered and "HeadModuleHit" (data +0x2C) plays at the
// collider, downward.
// Unmarked: the area-53 fight ran the not-root, same-weapon, miss, damage and flinch arms; the
// armour-on, no-weapon and zero-damage arms never ran.
void CDigitalGuardianHead::x12D98_(CStateManager& mgr, TUniqueId sender) {
  if ((x10F4_ & 0x10) == 0 || sender != xE00_) {
    return;
  }
  const CCollisionActor* col = TCastToConstPtr< CCollisionActor >(mgr.GetObjectById(sender));
  if (col == nullptr) {
    return;
  }
  const TUniqueId touched = col->GetLastTouchedObject();
  const CWeapon* weapon = TCastToConstPtr< CWeapon >(mgr.GetObjectById(touched));
  if (weapon == nullptr || touched == x1040_) {
    return;
  }
  // Retail's one-argument WeaponHits (0x800DB834) is the host's WeaponHurts: an effect-3 entry
  // (every damaging entry of the head's table) counts, and its multiplier still applies.
  if (!col->GetDamageVulnerability()->WeaponHurts(weapon->GetCurrentDamageInfo().GetWeaponMode())) {
    return;
  }
  const float damage =
      weapon->GetCurrentDamageInfo().GetVulnerableDamage(*col->GetDamageVulnerability());
  if (damage > 0.f) {
    TakeDamage(GetTransform().GetColumn(kDY), damage);
    if (EchoEmitter() != nullptr) {
      EchoEmitter()->SetAutoReducingYellowDamage(CPatterned::skDamageHitTime);
    }
    if (TCastToConstPtr< CWeapon >(mgr.GetObjectById(touched)) && damage > 25.f) {
      BodyController()->CommandMgr().DeliverCmd(CBCAdditiveFlinchCmd(1.f));
    }
  }
  x10E0_ -= damage;
  x1040_ = touched;
  PlayCustomSound(col->GetTranslation(), CVector3f::Down(), mData.sound_HeadModuleHit, false);
}

// fn_14_13830: the body is the CONN/ATCH object; if it is a guardian it is linked to this head
// (fn_14_7168 with this head's id, 0x13968) and kept at 0x1034 (0x13970). Then the IS04/ACTV
// object (0x103E), the MDFY/FOLW waypoint (0x103C), the MDFY/PLAY platforms (0x1044) and the
// attack area (fn_14_119E4).
// @ fn_14_13830
void CDigitalGuardianHead::x13830_(CStateManager& mgr) {
  const TUniqueId bodyId = CheckConnectedObject(mgr, kSS_Connect, kSM_Attach);
  if (CDigitalGuardian* body = TCastToPtr< CDigitalGuardian >(mgr.ObjectById(bodyId))) {
    body->x7168_(mgr, GetUniqueId());
    x1034_ = bodyId;
  }
  x103E_ = FindConnectedObject(mgr, kSS_InternalState4, kSM_Activate);
  x103C_ = CheckConnectedObject_if(mgr, kSS_Modify, kSM_Follow, CWaypointPredicate());
  x1044_ = FindConnectedObjects_if(mgr, kSS_Modify, kSM_Play, CPlatformPredicate());
  x119E4_(mgr);
}

// fn_14_119E4: the IS03/FOLW waypoints' positions, raised by 15 (.rodata 0x988). With exactly two
// the area's centre is their midpoint (.rodata 0x9A0, 0.5), its first axis the horizontal step
// from the first to the second and its second axis that step turned a quarter (z is .rodata
// 0x960, 0). Retail keeps the positions in a three-slot stack array it does not bound; the host
// stops storing at three but keeps counting, so the "exactly two" test is the same.
// @ fn_14_119E4
void CDigitalGuardianHead::x119E4_(CStateManager& mgr) {
  const rstl::vector< TUniqueId > ids =
      FindConnectedObjects_if(mgr, kSS_InternalState3, kSM_Follow, CWaypointPredicate());
  CVector3f pos[3];
  int count = 0;
  for (rstl::vector< TUniqueId >::const_iterator it = ids.begin(); it != ids.end(); ++it) {
    const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(*it));
    if (actor == nullptr) { // retail's null test at 0x11AA0 (the predicate admits waypoints only)
      continue;
    }
    if (count < 3) {
      const CVector3f& t = actor->GetTranslation();
      pos[count] = CVector3f(t.GetX(), t.GetY(), t.GetZ() + 15.f);
    }
    ++count;
  }
  if (count == 2) {
    const float dx = pos[1].GetX() - pos[0].GetX();
    const float dy = pos[1].GetY() - pos[0].GetY();
    x1004_ = CVector3f(0.5f * (pos[0].GetX() + pos[1].GetX()),
                       0.5f * (pos[0].GetY() + pos[1].GetY()),
                       0.5f * (pos[0].GetZ() + pos[1].GetZ()));
    x1010_ = CVector3f(dx, dy, 0.f);
    x101C_ = CVector3f(-dy, dx, 0.f);
  }
}

// fn_14_13B38: the transform's forward column (0x28/0x38/0x48) flattened (z is .rodata 0x960, 0)
// and normalised; when that is not safe (the head looks straight up or down) the up column
// (0x2C/0x3C/0x4C), negated if forward's z is above 0.
// @ fn_14_13B38
CVector3f CDigitalGuardianHead::x13B38_() const {
  const CVector3f forward = GetTransform().GetColumn(kDY);
  const CVector3f flat(forward.GetX(), forward.GetY(), 0.f);
  if (flat.IsMagnitudeSafe()) {
    return flat.AsNormalized();
  }
  const CVector3f up = GetTransform().GetColumn(kDZ);
  if (forward.GetZ() > 0.f) {
    return -up;
  }
  return up;
}

// fn_14_11C08: with dir and its horizontal part both safe, dir normalised, slerped from the
// horizontal toward it by 55 degrees (.rodata 0x9B8, 0.9599311) when its pitch is larger;
// otherwise fn_14_13B38.
// @ fn_14_11C08
CVector3f CDigitalGuardianHead::x11C08_(const CVector3f& dir) const {
  const CVector3f fallback = x13B38_();
  const CVector3f flat(dir.GetX(), dir.GetY(), 0.f);
  if (dir.IsMagnitudeSafe() && flat.IsMagnitudeSafe()) {
    const CVector3f dirN = dir.AsNormalized();
    const CVector3f flatN = flat.AsNormalized();
    if (CVector3f::GetAngleDiff(flatN, dirN) > 0.9599311f) {
      return CVector3f::Slerp(flatN, dirN, CRelAngle::FromRadians(0.9599311f));
    }
    return dirN;
  }
  return fallback;
}

// fn_14_11D64: only for dt > 0 (.rodata 0x960). 0x1028 becomes the direction to the face target
// (0x1036; its +0x54 minus this one's, through fn_14_11C08), or fn_14_13B38 with no target. Unless
// bit 0x08 of 0x10F4 is set (extrwi. 1, 28 at 0x11E54), the body controller faces it.
// @ fn_14_11D64
void CDigitalGuardianHead::x11D64_(CStateManager& mgr, float dt) {
  if (dt <= 0.f) {
    return;
  }
  if (const CEntity* ent = mgr.GetObjectById(x1036_)) {
    // Retail reads the entity's translation unchecked (lfs 0x54(r3) at 0x11DA8); the face target is
    // the player.
    const CActor* target = static_cast< const CActor* >(ent);
    x1028_ = x11C08_(target->GetTranslation() - GetTranslation());
  } else {
    x1028_ = x13B38_();
  }
  if ((x10F4_ & 0x08) == 0 && x1028_.IsMagnitudeSafe()) {
    BodyController()->FaceDirection(x1028_.AsNormalized(), dt);
  }
}

// fn_14_11698: with pos off the attack area's centre (0x1004), the angle from the first axis
// (0x1010) to it, negative when it lies behind the second axis (0x101C; the dot product below
// .rodata 0x960, 0). Returns false, leaving angle alone, when pos is the centre.
// @ fn_14_11698
bool CDigitalGuardianHead::x11698_(const CVector3f& pos, float& angle) const {
  const CVector3f d = pos - x1004_;
  if (d.GetX() == 0.f && d.GetY() == 0.f && d.GetZ() == 0.f) {
    return false;
  }
  angle = CVector3f::GetAngleDiff(x1010_, d);
  if (CVector3f::Dot(x101C_, d) < 0.f) {
    angle = -angle;
  }
  return true;
}

// fn_14_11788: the point (50 cos a, 40 sin a, 0) (.rodata 0x9C4, 0x9C0) in the frame looking from
// the centre (0x1004) along the second axis (0x101C, up is CVector3f::Up()), plus the centre.
// @ fn_14_11788
CVector3f CDigitalGuardianHead::x11788_(float angle) const {
  const float y = 40.f * CMath::FastSinR(angle);
  const float x = 50.f * CMath::FastCosR(angle);
  const CTransform4f frame = CTransform4f::LookAt(x1004_, x1004_ + x101C_, CVector3f::Up());
  return x1004_ + frame.Rotate(CVector3f(x, y, 0.f));
}

// fn_14_118B0: none when the head is at the attack area's centre (fn_14_11698); else the area's
// points at its angle, that plus 8 degrees and that minus 8 degrees (.rodata 0x9C8, 0.13962634).
// @ fn_14_118B0
int CDigitalGuardianHead::x118B0_(CVector3f* points) const {
  int count = 0;
  float angle = 0.f;
  if (x11698_(GetTranslation(), angle)) {
    points[count++] = x11788_(angle);
    points[count++] = x11788_(0.13962634f + angle);
    points[count++] = x11788_(angle - 0.13962634f);
  }
  return count;
}

// fn_14_114C8 needs the visual knowledge's line of sight, which the host never reports yet.
CVector3f CDigitalGuardianHead::x114C8_(const CVector3f& player, const CVector3f* points,
                                         int count) {
  StandInFn("fn_14_114C8 (0x1D0, attack point in sight of the player)");
  return xFEC_;
}

// fn_14_F390: the echo targets (0xF30/0xF34) that are gone or inactive (+0x20 bit 0x80), plus
// one, capped at 2.
// @ fn_14_F390
int CDigitalGuardianHead::xF390_(CStateManager& mgr) const {
  int active = 0;
  for (int i = 0; i < xF34_.size(); ++i) {
    const CEntity* ent = mgr.GetObjectById(xF34_[i]);
    if (ent != nullptr && ent->GetActive()) {
      ++active;
    }
  }
  const int count = xF34_.size() - active + 1;
  return count > 2 ? 2 : count;
}

// fn_14_F424: with the MDFY/FOLW waypoint (0x103C) and 4 s (.rodata 0x998) since the last one
// (0x10EC), a splitter is spent (0x10E8), the waypoint moves to the "B_ball_LCTR" locator
// (.rodata 0x940), IS01 goes out from this head, 0x10EC restarts, the 'PART' token (0xF04) becomes
// an "Eject Splitter Fx" explosion there (the model's scale, white) and sound_EjectSplitter (data
// +0x7C) plays along the forward column. With no waypoint none are left.
// @ fn_14_F424
void CDigitalGuardianHead::xF424_(CStateManager& mgr) {
  CScriptWaypoint* wp = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(x103C_));
  if (wp == nullptr) {
    x10E8_ = 0;
    return;
  }
  if (x10EC_ < 4.f) {
    return;
  }
  --x10E8_;
  const CTransform4f xf = GetLctrTransform(rstl::string_l("B_ball_LCTR"));
  wp->SetTranslation(xf.GetTranslation());
  SendScriptMsgs(kSS_InternalState1, mgr, GetUniqueId(), kSM_None);
  x10EC_ = 0.f;
  if (xF04_.valid()) {
    const CVector3f scale = GetModelData()->GetScale();
    const rstl::string name = rstl::string_l("Eject Splitter Fx");
    const CEntityInfo info(GetCurrentAreaId(), CEntity::NullConnectionList, true,
                           kInvalidEditorId);
    const TUniqueId uid = mgr.AllocateUniqueId();
    CExplosion* explosion = rs_new CExplosion(xF04_.data(), uid, info, name, xf, 0, scale,
                                              CColor::White(), -1);
    if (explosion != nullptr) {
      mgr.AddObject(explosion);
    }
  }
  PlayCustomSound(xf.GetTranslation(), GetTransform().GetColumn(kDY), mData.sound_EjectSplitter,
                  false);
}

// fn_14_12314: the offsets (0xF40): RotateX(pi/2), RotateX(-pi/2) * RotateY(pi/2) and
// RotateX(-pi/2) * RotateY(-pi/2) (.rodata 0x9CC/0x9D0). With the echo-target model (data +0x9C)
// at this head's scale, each target ("EchoTarget", .rodata 0xF2E) is built inactive-info
// (li r6, 0) at locator 1 times its offset, solid, orbit, target and seeker-target (.data
// 0xEA4..0xEB0: 59, 41, 40, 63), with the echo-target vulnerability (data +0x454), this head as
// its owner and the parameter block; its id goes to 0xF34.
void CDigitalGuardianHead::x12314_(CStateManager& mgr) {
  xF40_[0] = CTransform4f::RotateX(CRelAngle::FromRadians(1.5707964f));
  xF40_[1] = CTransform4f::RotateX(CRelAngle::FromRadians(-1.5707964f)) *
             CTransform4f::RotateY(CRelAngle::FromRadians(1.5707964f));
  xF40_[2] = CTransform4f::RotateX(CRelAngle::FromRadians(-1.5707964f)) *
             CTransform4f::RotateY(CRelAngle::FromRadians(-1.5707964f));
  if (mData.echoTargets == kInvalidAssetId) {
    return;
  }
  const CModelData modelData(CStaticRes(mData.echoTargets, GetModelData()->GetScale()));
  const CTransform4f lctr = GetLctrTransform(CSegId(1));
  for (int i = 0; i < xF34_.size(); ++i) {
    const TUniqueId uid = mgr.AllocateUniqueId();
    const CTransform4f xf = lctr * xF40_[i];
    CDigitalGuardianEchoTarget* target = rs_new CDigitalGuardianEchoTarget(
        uid, rstl::string_l("EchoTarget"),
        CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, false, kInvalidEditorId), 0,
        xf, modelData, CMaterialList(kMT_Solid, kMT_Orbit, kMT_Target, kMT_SeekerTarget),
        LdrToDamageVulnerability(mData.echoTargetVulnerability), GetUniqueId(), mData);
    if (target != nullptr) {
      mgr.AddObject(target);
      xF34_[i] = uid;
    }
  }
}

// fn_14_1223C: unless the armour is gone (0x10F4 bit 0x10), each echo target is placed at
// locator 1 times its offset, pushed out by its extension (fn_14_19F18); a gone one's id is
// cleared.
// @ fn_14_1223C
void CDigitalGuardianHead::x1223C_(CStateManager& mgr) {
  if ((x10F4_ & 0x10) != 0) {
    return;
  }
  const CTransform4f lctr = GetLctrTransform(CSegId(1));
  for (int i = 0; i < xF34_.size(); ++i) {
    CDigitalGuardianEchoTarget* target =
        static_cast< CDigitalGuardianEchoTarget* >(mgr.ObjectById(xF34_[i]));
    if (target != nullptr) {
      target->SetExtendedTransform(lctr * xF40_[i]);
    } else {
      xF34_[i] = kInvalidUniqueId;
    }
  }
}

// fn_14_14CF4: every retracted or retracting echo target starts extending (fn_14_19DF8 with 3) and
// takes damage again (+0x78C bit 0x80 cleared); bit 0x02 (EchoTargetDestroyed) is cleared.
// @ fn_14_14CF4
void CDigitalGuardianHead::ExtractEchoTargets(CStateManager& mgr, float) {
  for (int i = 0; i < xF34_.size(); ++i) {
    CDigitalGuardianEchoTarget* target =
        static_cast< CDigitalGuardianEchoTarget* >(mgr.ObjectById(xF34_[i]));
    if (target != nullptr && (target->GetState() == CDigitalGuardianEchoTarget::kS_Retracted ||
                              target->GetState() == CDigitalGuardianEchoTarget::kS_Retracting)) {
      target->SetState(mgr, CDigitalGuardianEchoTarget::kS_Extending);
      target->SetDamageIgnored(false);
    }
  }
  x10F4_ &= ~0x02;
}

// fn_14_14E78: the body (0x1034) gets "OPEN" from this head; the head leaves every visor's
// orbit list (0xF) but the echo visor's (2), and every inactive echo target (+0x20 bit 0x80, the
// extrwi 1,24) is activated (vtable 0x1C).
// @ fn_14_14E78
void CDigitalGuardianHead::ExtractAntenna(CStateManager& mgr, float) {
  mgr.SendScriptMsg(x1034_, GetUniqueId(), kSM_Open, GetUniqueId());
  SetVisorOrbitableFlags(CVisorParameters::kVOF_All, false);
  SetVisorOrbitableFlags(CVisorParameters::kVOF_Echo, true);
  for (int i = 0; i < xF34_.size(); ++i) {
    CEntity* target = mgr.ObjectById(xF34_[i]);
    if (target != nullptr && !target->GetActive()) {
      target->SetActive(true);
    }
  }
}

// fn_14_14DA0: every extended or extending echo target starts retracting (fn_14_19DF8 with 2) and
// ignores damage (+0x78C bit 0x80); bits 0x40 and 0x02 are cleared, the deployment time (0x10F0)
// becomes data +0x20 and bit 0x04 is set.
void CDigitalGuardianHead::RetractEchoTargets(CStateManager& mgr, float) {
  for (int i = 0; i < xF34_.size(); ++i) {
    CDigitalGuardianEchoTarget* target =
        static_cast< CDigitalGuardianEchoTarget* >(mgr.ObjectById(xF34_[i]));
    if (target != nullptr && (target->GetState() == CDigitalGuardianEchoTarget::kS_Extended ||
                              target->GetState() == CDigitalGuardianEchoTarget::kS_Extending)) {
      target->SetState(mgr, CDigitalGuardianEchoTarget::kS_Retracting);
      target->SetDamageIgnored(true);
    }
  }
  x10F4_ &= ~0x40;
  x10F4_ &= ~0x02;
  x10F0_ = mData.maxStage2StunnedTime;
  x10F4_ |= 0x04;
}

// fn_14_17A44: the base's PreRender.
// @ fn_14_17A44
void CDigitalGuardianHead::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);
}

// fn_14_17A00: the base's AddToRenderer, then the base's render (the relocations name both).
// @ fn_14_17A00
void CDigitalGuardianHead::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
  CPatterned::Render(mgr);
}

// fn_14_179E0: the pass-through vulnerability.
const CDamageVulnerability* CDigitalGuardianHead::GetDamageVulnerability() const {
  return &CDamageVulnerability::PassThroughVulnerabilty();
}

// fn_14_177C8: while bit 0x08 of 0x10F4 is clear, one of the three 'SCAN' tokens (0x10 set: the
// final one at 0xD7C, else 0x40 set: the stunned one at 0xD78, else the shield one at 0xD74);
// otherwise, or with no token, the base's. The tokens are not built yet, so the head's answer is
// always the base's; the ctor sets 0x08, so that is also retail's answer until the head moves.
CScannableObjectInfo* CDigitalGuardianHead::GetScannableObjectInfo() const {
  if ((x10F4_ & 0x08) == 0) {
    StandInFn("fn_14_177C8 'SCAN' tokens (0xD74..0xD7C)");
  }
  return CPatterned::GetScannableObjectInfo();
}

// fn_14_17468: the base's setup, then the triggers (20 = 0x14), the states (17 = 0x11) and the
// codes (29 = 0x1D) on the state machine at 0x350 (the calls at 0x17498, 0x174B8, 0x174D8).
// @ fn_14_17468
void CDigitalGuardianHead::SetupStateMachine(CStateManager& mgr) {
  CPatterned::SetupStateMachine(mgr);
  StateMachine* stateMachine = mStateMachine.get();
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  stateMachine->SetCodeFunctions(skCodes, ARRAY_SIZE(skCodes));
}

// ---------------------------------------------------------------------------------------------
// Triggers. The masks come from the rlwinm/extrwi bit numbers, MSB first.

// fn_14_17454: the animation state (0x6B4) is 3 (kAS_Over).
// @ fn_14_17454
bool CDigitalGuardianHead::StateOver(CStateManager& mgr, const CTriggerData& data) const {
  return mAnimationState.GetState() == CAnimationState::kAS_Over;
}

// fn_14_17408: the body's bit 0x40 of 0x128C (extrwi 1, 25; set by its CrippledReaction); with no
// body the answer is true (li r3, 1 at 0x17440).
// @ fn_14_17408
bool CDigitalGuardianHead::MainBodyDestroyed(CStateManager& mgr, const CTriggerData& data) const {
  if (const CDigitalGuardian* body = TCastToConstPtr< CDigitalGuardian >(mgr.GetObjectById(x1034_))) {
    return (body->x128C_ & 0x40) != 0;
  }
  return true;
}

// fn_14_173CC: attack mode 0 or 1 and the projectile timer (0x10D4) at most 0 (.rodata 0x960).
bool CDigitalGuardianHead::ShouldProjectileAttack(CStateManager& mgr,
                                                  const CTriggerData& data) const {
  if (xFE8_ == 0 || xFE8_ == 1) {
    return x10D4_ <= 0.f;
  }
  return false;
}

// fn_14_173B8: the burst count (0x10D8, a word: cntlzw/rlwnm) at most 0.
bool CDigitalGuardianHead::IsBurstOver(CStateManager& mgr, const CTriggerData& data) const {
  return x10D8_ <= 0;
}

// fn_14_173AC: bit 0x80 of 0x10F4 (extrwi 1, 24).
bool CDigitalGuardianHead::IsIdle(CStateManager& mgr, const CTriggerData& data) const {
  return (x10F4_ & 0x80) != 0;
}

// fn_14_17398: the attack mode (0xFE8) is 4.
// @ fn_14_17398
bool CDigitalGuardianHead::IsStunned(CStateManager& mgr, const CTriggerData& data) const {
  return xFE8_ == 4;
}

// fn_14_17384: the attack mode (0xFE8) is 3.
// @ fn_14_17384
bool CDigitalGuardianHead::IsGuarding(CStateManager& mgr, const CTriggerData& data) const {
  return xFE8_ == 3;
}

// fn_14_17370: the projectile attack (0x10D0) is 3.
bool CDigitalGuardianHead::UseMachineGun(CStateManager& mgr, const CTriggerData& data) const {
  return x10D0_ == 3;
}

// fn_14_1735C: the projectile attack (0x10D0) is 2.
bool CDigitalGuardianHead::UseLockOnMissiles(CStateManager& mgr, const CTriggerData& data) const {
  return x10D0_ == 2;
}

// fn_14_1734C: the projectile attack (0x10D0) is 0.
bool CDigitalGuardianHead::UseAnnihilatorPulse(CStateManager& mgr,
                                               const CTriggerData& data) const {
  return x10D0_ == 0;
}

// fn_14_17338: the projectile attack (0x10D0) is 1.
bool CDigitalGuardianHead::UseAnnihilatorCharge(CStateManager& mgr,
                                                const CTriggerData& data) const {
  return x10D0_ == 1;
}

// fn_14_17298: no splitter command module or main chassis in the object list (retail walks the
// list at state manager +0x830, its first dynamic list, which holds every object).
bool CDigitalGuardianHead::AllSplittersDead(CStateManager& mgr, const CTriggerData& data) const {
  CObjectList& list = mgr.ObjectListById(kOL_All);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    if (TCastToPtr< CSplitterCommandModule >(list[i]) != nullptr) {
      return false;
    }
    if (TCastToPtr< CSplitterMainChassis >(list[i]) != nullptr) {
      return false;
    }
  }
  return true;
}

// fn_14_1728C: bit 0x10 of 0x10F4 (extrwi 1, 27).
// @ fn_14_1728C
bool CDigitalGuardianHead::HeadArmorDestroyed(CStateManager& mgr, const CTriggerData& data) const {
  return (x10F4_ & 0x10) != 0;
}

// fn_14_17244: the body's fn_14_70B4; with no body the answer is false (li r3, 0 at 0x17278).
bool CDigitalGuardianHead::IsAntennaTransmitting(CStateManager& mgr,
                                                 const CTriggerData& data) const {
  if (const CDigitalGuardian* body = TCastToConstPtr< CDigitalGuardian >(mgr.GetObjectById(x1034_))) {
    return body->x70B4_();
  }
  return false;
}

// fn_14_171F8: with no visual target (0xDA0 invalid) the answer is true; else the target must be
// seen (bit 0x80 of 0xDB8, extrwi. 1, 24) for longer than 2.25 (.rodata 0x9DC, 0xDB0). The visual
// knowledge is not ported, so its target stays invalid.
bool CDigitalGuardianHead::HasLineOfSight(CStateManager& mgr, const CTriggerData& data) const {
  if (xDA0_ == kInvalidUniqueId) {
    return true;
  }
  return (xDB8_ & 0x80) != 0 && xDB0_ > 2.25f;
}

// fn_14_171EC: bit 0x02 of 0x10F4 (extrwi 1, 30).
bool CDigitalGuardianHead::EchoTargetDestroyed(CStateManager& mgr,
                                               const CTriggerData& data) const {
  return (x10F4_ & 0x02) != 0;
}

// fn_14_171D8: the echo targets left (0xFE0, a word) at most 0.
bool CDigitalGuardianHead::AllEchoTargetsDestroyed(CStateManager& mgr,
                                                   const CTriggerData& data) const {
  return xFE0_ <= 0;
}

// fn_14_171B8: the gravity repulsor's health (0x10E0) at most 0 (.rodata 0x960).
bool CDigitalGuardianHead::ShouldRetreat(CStateManager& mgr, const CTriggerData& data) const {
  return x10E0_ <= 0.f;
}

// fn_14_17198: the id at 0x103A is set.
bool CDigitalGuardianHead::BombPitDestroyed(CStateManager& mgr, const CTriggerData& data) const {
  return x103A_ != kInvalidUniqueId;
}

// fn_14_17184: the lives left (0x10E4, a word) at most 0.
bool CDigitalGuardianHead::IsDead(CStateManager& mgr, const CTriggerData& data) const {
  return x10E4_ <= 0;
}

// ---------------------------------------------------------------------------------------------
// States.

// fn_14_17180: empty.
// @ fn_14_17180
void CDigitalGuardianHead::Start(CStateManager& mgr, EStateMsg msg, float dt) {}

// fn_14_17134: bit 0x80 of 0x10F4 is set on activate (0) and cleared on deactivate (2); the
// update (1) does nothing (rlwimi r0, r4, 7, 24, 24 at 0x17160 / 0x17174).
// @ fn_14_17134
void CDigitalGuardianHead::Idle(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    x10F4_ |= 0x80;
    break;
  case kStateMsg_Deactivate:
    x10F4_ &= ~0x80;
    break;
  default:
    break;
  }
}

// fn_14_1709C: on activate the head faces its horizontal forward (fn_14_13B38, stored at
// 0x1028); each update turns it toward the face target (fn_14_11D64); on deactivate 0x1028 is
// cleared.
// @ fn_14_1709C
void CDigitalGuardianHead::FaceTarget(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    x1028_ = x13B38_();
    break;
  case kStateMsg_Update:
    x11D64_(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    x1028_ = CVector3f::Zero();
    break;
  default:
    break;
  }
}

// fn_14_16160. Activate: the animation state becomes 1 (ready) and 0x1028 the horizontal facing
// (fn_14_13B38). Update: while the body (0x1034) still holds this head (its 0x1082), the body lets
// go (fn_14_70E4), bit 0x08 of 0x10F4 is cleared, the head moves to the IS05/FOLW waypoint and
// the locomotion type becomes 3 (combat); then once the state's time reaches data +0x44 (the
// field after sound_HeadModuleHit) the animation state is 3 (over). Deactivate: animation state 0,
// locomotion 3, 0x1028 cleared, bit 0x08 cleared and the attack mode (0xFE8) 1.
// @ fn_14_16160
void CDigitalGuardianHead::DetachFromBody(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    x1028_ = x13B38_();
    break;
  case kStateMsg_Update: {
    CDigitalGuardian* body = TCastToPtr< CDigitalGuardian >(mgr.ObjectById(x1034_));
    if (body != nullptr && body->x1082_ == GetUniqueId()) {
      body->x70E4_(mgr);
      x10F4_ &= ~0x08;
      const TUniqueId id =
          FindConnectedObject_if(mgr, kSS_InternalState5, kSM_Follow, CWaypointPredicate());
      if (const CScriptWaypoint* wp = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(id))) {
        SetTranslation(wp->GetTranslation());
      }
      BodyController()->SetLocomotionType(pas::kLT_Combat);
    }
    if (mStateMachine->GetTime() >= mData.unknown_0x1cc6d870) {
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
    break;
  }
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    x1028_ = CVector3f::Zero();
    x10F4_ &= ~0x08;
    xFE8_ = 1;
    break;
  default:
    break;
  }
}

// fn_14_15B94. Activate: animation state 1. Update: the head flies toward the attack position
// (0xFEC) at data +0x14 (maxLinearVelocity) per second, in its own frame (TransposeRotate, then
// MoveInOneFrameOR); once a step would reach it, it is placed there and the animation state is 3.
// Then it turns toward the face target (fn_14_11D64). Deactivate: animation state 0.
// @ fn_14_15B94
void CDigitalGuardianHead::SeekAttackPosition(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    break;
  case kStateMsg_Update: {
    const CVector3f delta = xFEC_ - GetTranslation();
    const float step = mData.maxLinearVelocity * dt;
    if (step < delta.Magnitude()) {
      MoveInOneFrameOR(GetTransform().TransposeRotate(delta.AsNormalized() * step), dt);
    } else {
      SetTranslation(xFEC_);
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
    x11D64_(mgr, dt);
    break;
  }
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  default:
    break;
  }
}

// fn_14_15F3C. Activate: animation state 1. Update: while a knock-back may be issued
// (CanIssueCommand with 6 = kAS_KnockBack, 0x15F94) one is sent along the forward column
// (0x28/0x38/0x48) with severity 1 (kS_One); otherwise, once the body is in the knock-back state
// (its current state id 6, 0x16070), the locomotion type becomes 5 and the head drifts toward the
// attack position (0xFEC) at data +0x1C (maxStunnedLinearVelocity) per second, only while a step
// falls short of it. Deactivate: animation state 0, locomotion type 5.
// @ fn_14_15F3C
void CDigitalGuardianHead::StunnedReaction(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_KnockBack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCKnockBackCmd(GetTransform().GetColumn(kDY), pas::kS_One));
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_KnockBack) {
      BodyController()->SetLocomotionType(pas::kLT_Internal5);
      const CVector3f delta = xFEC_ - GetTranslation();
      if (delta.IsMagnitudeSafe()) {
        const float step = mData.maxStunnedLinearVelocity * dt;
        if (step < delta.Magnitude()) {
          MoveInOneFrameOR(GetTransform().TransposeRotate(delta.AsNormalized() * step), dt);
        }
      }
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    BodyController()->SetLocomotionType(pas::kLT_Internal5);
    break;
  default:
    break;
  }
}

// fn_14_15E18. Activate: locomotion type 5, animation state 1. Update: with splitters left
// (0x10E8 > 0) and the deployment time (0x10F0) at data +0x20 (maxStage2StunnedTime), one is
// ejected (fn_14_F424, after setting bit 0x04 of 0x10F4) when 0x04 is already set or every
// splitter is dead (fn_14_17298). Otherwise, with none left, that time reached and 4 s since the
// last (0x10EC, .rodata 0x998), the animation state is 3. Deactivate: animation state 0.
// @ fn_14_15E18
void CDigitalGuardianHead::DeploySplitters(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Internal5);
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    break;
  case kStateMsg_Update:
    if (x10E8_ > 0 && x10F0_ >= mData.maxStage2StunnedTime &&
        ((x10F4_ & 0x04) != 0 || AllSplittersDead(mgr, CTriggerData(0.f)))) {
      x10F4_ |= 0x04;
      xF424_(mgr);
    } else if (x10E8_ <= 0 && x10F0_ >= mData.maxStage2StunnedTime && x10EC_ >= 4.f) {
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  default:
    break;
  }
}

// fn_14_1561C: on activate only, the boss bar is cleared (no boss, .rodata 0x960 = 0, string 0),
// then MassiveDeath (vtable 0xD4).
// @ fn_14_1561C
void CDigitalGuardianHead::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {
    return;
  }
  mgr.SetBossParams(kInvalidUniqueId, 0.f, 0);
  MassiveDeath(mgr);
}

#define DGHD_STATE_STANDIN(name, fn)                                                               \
  void CDigitalGuardianHead::name(CStateManager& mgr, EStateMsg msg, float dt) {                   \
    StandInFn(#fn " (head state " #name ")");                                                      \
  }
DGHD_STATE_STANDIN(ProjectileTelegraph, fn_14_16F74)
DGHD_STATE_STANDIN(PulseAttack, fn_14_16D64)
DGHD_STATE_STANDIN(ChargeAttack, fn_14_16A50)
DGHD_STATE_STANDIN(MissileAttack, fn_14_167EC)
DGHD_STATE_STANDIN(MachineGunAttack, fn_14_16408)
DGHD_STATE_STANDIN(AttachedToBody, fn_14_1633C)
DGHD_STATE_STANDIN(BombedReaction, fn_14_15CE8)
DGHD_STATE_STANDIN(SeekMotionPath, fn_14_158F8)
DGHD_STATE_STANDIN(FollowRetreatPath, fn_14_1569C)
#undef DGHD_STATE_STANDIN

// fn_14_13F0C: the root collider (0xE00) takes the head's own vulnerability (CPatterned's,
// called directly).
// @ fn_14_13F0C
void CDigitalGuardianHead::SetVulnerableToDamage(CStateManager& mgr, float dt) {
  if (CCollisionActor* root = TCastToPtr< CCollisionActor >(mgr.ObjectById(xE00_))) {
    root->SetDamageVulnerability(*CPatterned::GetDamageVulnerability());
  }
}

// fn_14_13EB8: the root collider (0xE00) is immune again (ImmuneRicochetVulnerability, the host's
// ReflectVulnerabilty, as fn_14_13318 gives it).
// @ fn_14_13EB8
void CDigitalGuardianHead::SetImmuneToDamage(CStateManager& mgr, float dt) {
  if (CCollisionActor* root = TCastToPtr< CCollisionActor >(mgr.ObjectById(xE00_))) {
    root->SetDamageVulnerability(CDamageVulnerability::ReflectVulnerabilty());
  }
}

// Codes not ported yet: once-logging stand-ins (the FSM2 state machine runs codes by name).
// fn_14_1560C: the player (state manager +0x14FC) becomes the face target (0x1036).
// @ fn_14_1560C
void CDigitalGuardianHead::SetPlayerFaceTarget(CStateManager& mgr, float dt) {
  x1036_ = mgr.GetPlayer(0)->GetUniqueId();
}

// fn_14_143D8: the attack position is the attack area's point 45 degrees (.rodata 0x9E0,
// 0.7853982) on from the player's angle (fn_14_11698, 0 when the player is at the centre).
// @ fn_14_143D8
void CDigitalGuardianHead::SetInitialAttackPosition(CStateManager& mgr, float dt) {
  float angle = 0.f;
  x11698_(mgr.GetPlayer(0)->GetTranslation(), angle);
  xFEC_ = x11788_(angle + 0.7853982f);
}

// fn_14_15284: the attack position becomes the head's translation. With bit 0x10 of 0x10F4 set
// (extrwi. 1, 27; the head armour is gone) and 0x1038 a platform, it is instead the attack area's
// point at the angle (fn_14_11698, fn_14_11788) of the centre plus 50 (.rodata 0x9C4) along the
// direction to the platform.
// @ fn_14_15284
void CDigitalGuardianHead::SetStunnedPosition(CStateManager& mgr, float dt) {
  xFEC_ = GetTranslation();
  if ((x10F4_ & 0x10) == 0) {
    return;
  }
  if (CScriptPlatform* plat = TCastToPtr< CScriptPlatform >(mgr.ObjectById(x1038_))) {
    const CVector3f d = plat->GetTranslation() - x1004_;
    if (d.IsMagnitudeSafe()) {
      float angle = 0.f;
      if (x11698_(x1004_ + d.AsNormalized() * 50.f, angle)) {
        xFEC_ = x11788_(angle);
      }
    }
  }
}

// fn_14_14254: the attack position starts as the head's translation and the candidates come
// from fn_14_118B0. When the visual knowledge sees the player (flag 0x80 of 0xDB8) for over 2.25 s
// (.rodata 0x9DC; 0xDB0) fn_14_114C8 picks one; otherwise the candidate furthest along the
// previous move (translation minus 0xFF8, dot product >= the best so far, from 0) wins. 0xFF8
// then keeps the result.
// @ fn_14_14254
void CDigitalGuardianHead::SetAttackPosition(CStateManager& mgr, float dt) {
  xFEC_ = GetTranslation();
  CVector3f points[3];
  const int count = x118B0_(points);
  if ((xDB8_ & 0x80) != 0 && xDB0_ > 2.25f) {
    xFEC_ = x114C8_(mgr.GetPlayer(0)->GetTranslation(), points, count);
  } else {
    const CVector3f move = GetTranslation() - xFF8_;
    float best = 0.f;
    for (int i = 0; i < count; ++i) {
      const float dot = CVector3f::Dot(move, points[i] - xFF8_);
      if (dot >= best) {
        xFEC_ = points[i];
        best = dot;
      }
    }
  }
  xFF8_ = xFEC_;
}

// fn_14_14B98: bit 0x04 of 0x10F4 cleared, bit 0x40 set (rlwimi 6, 25, 25), the deployment
// time (0x10F0) restarted; the splitters to deploy (0x10E8) are fn_14_F390's count when every
// splitter is dead (fn_14_17298), else none.
// @ fn_14_14B98
void CDigitalGuardianHead::StartSplitterDeployment(CStateManager& mgr, float dt) {
  x10F4_ &= ~0x04;
  x10F4_ |= 0x40;
  x10F0_ = 0.f;
  x10E8_ = AllSplittersDead(mgr, CTriggerData(0.f)) ? xF390_(mgr) : 0;
}

#define DGHD_CODE_STANDIN(name, fn)                                                                \
  void CDigitalGuardianHead::name(CStateManager& mgr, float dt) {                                  \
    StandInFn(#fn " (head code " #name ")");                                                       \
  }
DGHD_CODE_STANDIN(ResetProjectileAttackTimes, fn_14_15598)
DGHD_CODE_STANDIN(SelectProjectileAttack, fn_14_15448)
DGHD_CODE_STANDIN(SetPlatformPosition, fn_14_153D0)
// fn_14_1524C makes the player the visual knowledge's target (CVisualKnowledge::SetVisualTarget
// at 0xD80: 0xDA0 = the player, bit 0x80 of 0xDB8 and the times 0xDAC..0xDB4 cleared). Its
// Update is not ported, so a set target would never be seen and HasLineOfSight would stay false.
DGHD_CODE_STANDIN(SeekLineOfSight, fn_14_1524C)
DGHD_CODE_STANDIN(DestroyActiveSplitters, fn_14_14C24)
DGHD_CODE_STANDIN(Stage3Start, fn_14_149F8)
DGHD_CODE_STANDIN(EnableDynamicCollision, fn_14_14828)
DGHD_CODE_STANDIN(DisableDynamicCollision, fn_14_14664)
DGHD_CODE_STANDIN(EnableValidTarget, fn_14_14604)
DGHD_CODE_STANDIN(DisableValidTarget, fn_14_145A4)
DGHD_CODE_STANDIN(ResetGravityRepulsorHP, fn_14_14248)
DGHD_CODE_STANDIN(EjectPlayerFromBombPit, fn_14_14058)
DGHD_CODE_STANDIN(StartVocalRecoverReaction, fn_14_13C1C)
#undef DGHD_CODE_STANDIN

// fn_14_15054: the "RaisedShields" effect state (.rodata 0x950) off; at locator 1 the drop-shields
// effect (0xEF4, "Drop Shields Fx", .rodata 0xA00) at this head's scale, then the drop-shields
// sound (data +0x64) facing the locator's column 1.
// @ fn_14_15054
void CDigitalGuardianHead::DropShields(CStateManager& mgr, float dt) {
  ModelData()->AnimationData()->SetEffectState(rstl::string_l("RaisedShields"), false, mgr);
  const CTransform4f xf = GetLctrTransform(CSegId(1));
  if (xEF4_.valid()) {
    const CVector3f scale = GetModelData()->GetScale();
    const rstl::string name = rstl::string_l("Drop Shields Fx");
    const CEntityInfo info(GetCurrentAreaId(), CEntity::NullConnectionList, true,
                           kInvalidEditorId);
    const TUniqueId uid = mgr.AllocateUniqueId();
    CExplosion* explosion = rs_new CExplosion(xEF4_.data(), uid, info, name, xf, 0, scale,
                                              CColor::White(), -1);
    if (explosion != nullptr) {
      mgr.AddObject(explosion);
    }
  }
  PlayCustomSound(xf.GetTranslation(), xf.GetColumn(kDY), mData.sound_DropShields, false);
}

// fn_14_14F74: the "RaisedShields" effect state on, then the raise-shields sound (data +0x4C) at
// locator 1, facing its column 1.
// @ fn_14_14F74
void CDigitalGuardianHead::RaiseShields(CStateManager& mgr, float dt) {
  ModelData()->AnimationData()->SetEffectState(rstl::string_l("RaisedShields"), true, mgr);
  const CTransform4f xf = GetLctrTransform(CSegId(1));
  PlayCustomSound(xf.GetTranslation(), xf.GetColumn(kDY), mData.sound_RaiseShields, false);
}

// fn_14_14B60: bit 0x40 of 0x10F4 cleared, the body back to combat locomotion.
// @ fn_14_14B60
void CDigitalGuardianHead::EndSplitterDeployment(CStateManager& mgr, float dt) {
  x10F4_ &= ~0x40;
  BodyController()->SetLocomotionType(pas::kLT_Combat);
}

// fn_14_14470: with splitters alive the gun (3). Else the first shot (block 6's firstShotType,
// data +0x2DC) once while 0x10DC is set and the type is not 4; else a weighted pick over block
// 6's odds (Range(0, sum), each bound inclusive): pulse 0, charge 1, missiles 2, else the gun 3.
// A pulse fires a burst of 3 (0x10D8), the rest none.
void CDigitalGuardianHead::SelectDeathHeadAttack(CStateManager& mgr, float dt) {
  const SLdrDigitalGuardianHeadStruct& odds = mData.digitalGuardianHeadStruct_0x48b46e55;
  if (!AllSplittersDead(mgr, CTriggerData(0.f))) {
    x10D0_ = 3;
  } else if (x10DC_ != 0 && odds.firstShotType != 4) {
    x10D0_ = odds.firstShotType;
    x10DC_ = 0;
  } else {
    const float r = mgr.Random()->Range(0.f, odds.annihilatorPulseOdds +
                                                  odds.annihilatorChargeOdds +
                                                  odds.machineGunOdds + odds.lockOnMissilesOdds);
    float bound = odds.annihilatorPulseOdds;
    if (r <= bound) {
      x10D0_ = 0;
    } else if (r <= (bound += odds.annihilatorChargeOdds)) {
      x10D0_ = 1;
    } else if (r <= bound + odds.lockOnMissilesOdds) {
      x10D0_ = 2;
    } else {
      x10D0_ = 3;
    }
  }
  x10D8_ = x10D0_ == 0 ? 3 : 0;
}

// fn_14_13F70: the IS02/ATCH player hint, unless already this head's, takes this head as its
// actor (0x1AC) and gets INCR from this head on the player's behalf.
// @ fn_14_13F70
void CDigitalGuardianHead::GrabPlayerHintControl(CStateManager& mgr, float dt) {
  CScriptPlayerHint* hint = TCastToPtr< CScriptPlayerHint >(
      mgr.ObjectById(CheckConnectedObject(mgr, kSS_InternalState2, kSM_Attach)));
  if (hint == nullptr || hint->GetActorId() == GetUniqueId()) {
    return;
  }
  hint->SetActorId(GetUniqueId());
  mgr.DeliverScriptMsg(CScriptMsg(GetUniqueId(), hint->GetUniqueId(), kSM_Increment,
                                  mgr.GetPlayer(0)->GetUniqueId()));
}

// fn_14_13C6C: 0x1038 is cleared (0x13CAC), then with any MDFY/PLAY platforms (0x1048) a random
// one (the state manager's CRandom16, Range(0, count - 1)) is picked. Along its CONN/ATCH
// waypoint's ARRV/NEXT chain (at most 30 waypoints: the counter starts at 1 and stops at 0x20)
// the one nearest the head (from .rodata 0x9D8, FLT_MAX; a strictly smaller squared distance
// wins) is where the platform teleports, and the platform becomes 0x1038 (0x13E80).
// @ fn_14_13C6C
void CDigitalGuardianHead::SelectRetreatPlatform(CStateManager& mgr, float dt) {
  x1038_ = kInvalidUniqueId;
  if (x1044_.size() == 0) {
    return;
  }
  const TUniqueId platId = x1044_[mgr.Random()->Range(0, x1044_.size() - 1)];
  CScriptPlatform* plat = TCastToPtr< CScriptPlatform >(mgr.ObjectById(platId));
  if (plat == nullptr) {
    return;
  }
  const TUniqueId firstId = plat->CheckConnectedObject_if(mgr, kSS_Connect, kSM_Attach,
                                                          CWaypointPredicate());
  const CVector3f headPos = GetTranslation();
  int count = 1;
  TUniqueId bestId = kInvalidUniqueId;
  float best = FLT_MAX;
  const CScriptWaypoint* wp = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(firstId));
  while (wp != nullptr && ++count < 0x20) {
    const CVector3f& pos = wp->GetTranslation();
    const float dx = pos.GetX() - headPos.GetX();
    const float dy = pos.GetY() - headPos.GetY();
    const float dz = pos.GetZ() - headPos.GetZ();
    const float dist = dz * dz + (dx * dx + dy * dy);
    if (dist < best) {
      best = dist;
      bestId = wp->GetUniqueId();
    }
    const TUniqueId nextId = wp->CheckConnectedObject_if(mgr, kSS_Arrived, kSM_Next,
                                                         CWaypointPredicate());
    wp = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(nextId));
  }
  if (bestId != kInvalidUniqueId) {
    plat->TeleportToWaypoint(bestId, mgr);
    x1038_ = platId;
  }
}

// ---------------------------------------------------------------------------------------------
// fn_14_DF7C: the DGHD loader. The generated reader fills the four property blocks; an empty model
// is skipped (null), as the body's loader does.
CEntity* PortLoadDigitalGuardianHead(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrDigitalGuardianHead sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrDigitalGuardianHead.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  return rs_new CDigitalGuardianHead(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      *modelData, LdrToActorParameters(sldrThis.actorInformation),
      LdrToPatternedInfo(sldrThis.patterned, nullptr), sldrThis.digitalGuardianHeadProperties);
}

// fn_14_1303C: with the armour gone (0x10F4 bit 0x10, extrwi 1,27) a ball collider at no health
// (the cror at 0x13084: HP <= .rodata 0x960) hurts the head along its column 1 by the collider's
// full health (vtable 0xA4), becomes the destroyed bomb pit (0x103A), costs a life (0x10E4), moves
// the waypoint to it with IS08 and destroys its bomb slot, then is deactivated (vtable 0x1C).
// Before, an echo target in state 4 moves the waypoint to its aim position with IS09, is
// destroyed (fn_14_11FEC) and deleted, and sets bit 0x02 (EchoTargetDestroyed).
// @ fn_14_1303C
void CDigitalGuardianHead::x1303C_(CStateManager& mgr, TUniqueId sender) {
  if ((x10F4_ & 0x10) != 0) {
    CCollisionActor* col = TCastToPtr< CCollisionActor >(mgr.ObjectById(sender));
    if (col != nullptr && col->GetHealthInfo()->GetHP() <= 0.f) {
      TakeDamage(GetTransform().GetColumn(kDY), col->GetHealthInfo()->GetInitialHP());
      x103A_ = col->GetUniqueId();
      --x10E4_;
      xF2B8_(mgr, col->GetTranslation(), kSS_InternalState8);
      x12B04_(mgr, x13248_(sender));
      col->SetActive(false);
    }
  } else if (x11F30_(sender)) {
    CDigitalGuardianEchoTarget* target =
        static_cast< CDigitalGuardianEchoTarget* >(mgr.ObjectById(sender));
    if (target != nullptr && target->GetState() == CDigitalGuardianEchoTarget::kS_Destroyed) {
      xF2B8_(mgr, target->GetAimPosition(mgr, 0.f), kSS_InternalState9);
      x11FEC_(mgr, target->GetTransform());
      mgr.DeleteObjectRequest(target->GetUniqueId());
      x10F4_ |= 0x02;
    }
  }
}

// fn_14_F2B8: the IS07/FOLW object (0xF2C0..0xF2D8), if a waypoint, goes to pos; then state is
// sent with no message (-1).
void CDigitalGuardianHead::xF2B8_(CStateManager& mgr, const CVector3f& pos,
                                  EScriptObjectState state) {
  const TUniqueId wpId = CheckConnectedObject(mgr, kSS_InternalState7, kSM_Follow);
  if (CScriptWaypoint* wp = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(wpId))) {
    wp->SetTranslation(pos);
    SendScriptMsgs(state, mgr, GetUniqueId(), kSM_None);
  }
}

// fn_14_13248: the index of id among the ball colliders (count 0xDC0, 2), or -1.
int CDigitalGuardianHead::x13248_(TUniqueId id) const {
  for (int i = 0; i < 2; ++i) {
    if (xDC4_[i] == id) {
      return i;
    }
  }
  return -1;
}

// fn_14_11F30: a valid id among the echo targets (0xF30 count, 0xF34 ids).
// @ fn_14_11F30
bool CDigitalGuardianHead::x11F30_(TUniqueId id) const {
  if (id != kInvalidUniqueId) {
    for (int i = 0; i < xF34_.size(); ++i) {
      if (id == xF34_[i]) {
        return true;
      }
    }
  }
  return false;
}

// fn_14_11FEC: the echo-target-destroyed effect ("EchoTargetExplosion", .rodata 0xF1A) at xf at
// this head's scale, the IS04/ACTV object (0x103E) loses one health (.rodata 0x96C), one echo
// target fewer (0xFE0); at none the armour is gone (bit 0x10) and IS00 is sent. The survivors
// then ignore damage (0x78C bit 0x80).
void CDigitalGuardianHead::x11FEC_(CStateManager& mgr, const CTransform4f& xf) {
  if (xFD0_.valid()) {
    const CVector3f scale = GetModelData()->GetScale();
    const rstl::string name = rstl::string_l("EchoTargetExplosion");
    const CEntityInfo info(GetCurrentAreaId(), CEntity::NullConnectionList, true,
                           kInvalidEditorId);
    const TUniqueId uid = mgr.AllocateUniqueId();
    CExplosion* explosion = rs_new CExplosion(xFD0_.data(), uid, info, name, xf, 0, scale,
                                              CColor::White(), -1);
    if (explosion != nullptr) {
      mgr.AddObject(explosion);
    }
  }
  // Retail calls the object's HealthInfo slots (vtable 0x3C, 0x38) unchecked; the host casts.
  if (CActor* act = TCastToPtr< CActor >(mgr.ObjectById(x103E_))) {
    act->HealthInfo()->SetHP(act->GetHealthInfo()->GetHP() - 1.f);
  }
  if (--xFE0_ <= 0) {
    x10F4_ |= 0x10;
    SendScriptMsgs(kSS_InternalState0, mgr, GetUniqueId(), kSM_None);
  }
  for (int i = 0; i < xF34_.size(); ++i) {
    CDigitalGuardianEchoTarget* target =
        static_cast< CDigitalGuardianEchoTarget* >(mgr.ObjectById(xF34_[i]));
    if (target != nullptr) {
      target->SetDamageIgnored(true);
    }
  }
}

// fn_14_12B04: at ball idx's locator (0xDCC) the bomb-pit effect ("BombPitExplosionFx", .rodata
// 0xF39) when set, the explosion sound (data +0x13C), the slot's destroyed effect state (.data
// 0xE50) and, once, its looping sound (data +0x154) at 0xF28; the IS04/ACTV object loses one
// health.
void CDigitalGuardianHead::x12B04_(CStateManager& mgr, int idx) {
  if (idx == -1) {
    return;
  }
  const CTransform4f xf = GetLctrTransform(xDCC_[idx]);
  const CVector3f pos = xf.GetTranslation();
  if (xF14_.valid()) {
    const CVector3f scale = GetModelData()->GetScale();
    const rstl::string name = rstl::string_l("BombPitExplosionFx");
    const CEntityInfo info(GetCurrentAreaId(), CEntity::NullConnectionList, true,
                           kInvalidEditorId);
    const TUniqueId uid = mgr.AllocateUniqueId();
    CExplosion* explosion = rs_new CExplosion(xF14_.data(), uid, info, name, xf, 0, scale,
                                              CColor::White(), -1);
    if (explosion != nullptr) {
      mgr.AddObject(explosion);
    }
  }
  PlayCustomSound(pos, CVector3f::Up(), mData.sound_BombSlotExplosion, false);
  static const char* const skSlotDestroyed[2] = {"LeftBombSlotDestroyed",
                                                  "RightBombSlotDestroyed"};
  ModelData()->AnimationData()->SetEffectState(rstl::string_l(skSlotDestroyed[idx]), true,
                                               mgr);
  if (!xF28_[idx]) {
    xF28_[idx] = PlayCustomSound(pos, CVector3f::Up(), mData.sound_BombSlotDestroyed, true);
  }
  // Retail calls the object's HealthInfo slots (vtable 0x3C, 0x38) unchecked; the host casts.
  if (CActor* act = TCastToPtr< CActor >(mgr.ObjectById(x103E_))) {
    act->HealthInfo()->SetHP(act->GetHealthInfo()->GetHP() - 1.f);
  }
}
