// Port-only: the DigitalGuardian REL's head module (module 14, CDigitalGuardianHead, script type
// DGHD, docs/module-analysis/DigitalGuardian.md sections b.5-b.7, c.3 and f.2). The retail class is
// 0x10F8 bytes, built by fn_14_18264. This slice is the loader, the constructor, the link to the
// body ("XALD", fn_14_13830), Think, AcceptScriptMsg, the state machine's tables, the 20 triggers
// and the two smallest states; the other FSM functions and the module callees they reach are
// once-logging stand-ins named after the retail function. The comments cite the REL's .text
// offsets.

#include "MetroidPrime/Enemies/CDigitalGuardianHead.hpp"

#include "Kyoto/Math/CRelAngle.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CObjectList.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CValidEntityPredicate.hpp"
#include "MetroidPrime/Enemies/CDigitalGuardian.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Enemies/CSplitterCommandModule.hpp"
#include "MetroidPrime/Enemies/CSplitterMainChassis.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptPlatform.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"

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
, xDA0_(kInvalidUniqueId)
, xDB0_(0.f)
, xDB8_(0)
, xFE0_(3)
, xFE8_(1)
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
, x10E0_(data.gravityRepulsorHP) // data +0x18
, x10E4_(2)
, x10EC_(0.f)
, x10F0_(0.f)
, x10F4_(0x08) // only bit 0x08 is set (the rlwimi stores at the ctor's tail)
, x10F5_(0)    // bit 0x80 cleared
{}

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
  StandInFn("fn_14_12818 (0x2EC, head Think)");
  StandInFn("fn_14_1223C (0xD8, head Think)");
}

// fn_14_13AE0: the projectile attack timer counts down, the two timers at 0x10EC and 0x10F0 up.
// @ fn_14_13AE0
void CDigitalGuardianHead::x13AE0_(float dt) {
  x10D4_ -= dt;
  x10EC_ += dt;
  x10F0_ += dt;
}

// fn_14_17D38: the active flag is read before the base's call (0x17D64..0x17D78).
void CDigitalGuardianHead::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const bool active = GetActive();
  CPatterned::AcceptScriptMsg(mgr, msg);
  switch (static_cast< uint >(msg.GetMessage())) {
  case kSM_Create: // "XCRT", 0x17E18
    BodyController()->Activate(mgr, pas::kAS_Invalid);
    StandInFn("fn_14_13318 (0x490, head collision actors)");
    SetVisorOrbitableFlags(CVisorParameters::kVOF_All, false);
    RemoveMaterial(kMT_SeekerTarget, mgr); // li r4, 0x3F
    StandInFn("fn_14_12314 (0x3B4, head echo targets)");
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
    StandInFn("fn_14_1303C (head XDMG)");
    break;
  case kSM_ResistedDamage: // "XRDG", 0x17F2C
    StandInFn("fn_14_12D98 (head XRDG)");
    break;
  default:
    break;
  }
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

// fn_14_17A44: the base's PreRender.
// @ fn_14_17A44
void CDigitalGuardianHead::PreRender(CStateManager& mgr) { CPatterned::PreRender(mgr); }

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
DGHD_STATE_STANDIN(StunnedReaction, fn_14_15F3C)
DGHD_STATE_STANDIN(DeploySplitters, fn_14_15E18)
DGHD_STATE_STANDIN(BombedReaction, fn_14_15CE8)
DGHD_STATE_STANDIN(SeekAttackPosition, fn_14_15B94)
DGHD_STATE_STANDIN(SeekMotionPath, fn_14_158F8)
DGHD_STATE_STANDIN(FollowRetreatPath, fn_14_1569C)
DGHD_STATE_STANDIN(Dead, fn_14_1561C)
#undef DGHD_STATE_STANDIN

// Codes not ported yet: once-logging stand-ins (the FSM2 state machine runs codes by name).
// fn_14_1560C: the player (state manager +0x14FC) becomes the face target (0x1036).
// @ fn_14_1560C
void CDigitalGuardianHead::SetPlayerFaceTarget(CStateManager& mgr, float dt) {
  x1036_ = mgr.GetPlayer(0)->GetUniqueId();
}

#define DGHD_CODE_STANDIN(name, fn)                                                                \
  void CDigitalGuardianHead::name(CStateManager& mgr, float dt) {                                  \
    StandInFn(#fn " (head code " #name ")");                                                       \
  }
DGHD_CODE_STANDIN(ResetProjectileAttackTimes, fn_14_15598)
DGHD_CODE_STANDIN(SelectProjectileAttack, fn_14_15448)
DGHD_CODE_STANDIN(SetPlatformPosition, fn_14_153D0)
DGHD_CODE_STANDIN(SetStunnedPosition, fn_14_15284)
DGHD_CODE_STANDIN(SeekLineOfSight, fn_14_1524C)
DGHD_CODE_STANDIN(DropShields, fn_14_15054)
DGHD_CODE_STANDIN(RaiseShields, fn_14_14F74)
DGHD_CODE_STANDIN(ExtractAntenna, fn_14_14E78)
DGHD_CODE_STANDIN(ExtractEchoTargets, fn_14_14CF4)
DGHD_CODE_STANDIN(RetractEchoTargets, fn_14_14DA0)
DGHD_CODE_STANDIN(DestroyActiveSplitters, fn_14_14C24)
DGHD_CODE_STANDIN(StartSplitterDeployment, fn_14_14B98)
DGHD_CODE_STANDIN(EndSplitterDeployment, fn_14_14B60)
DGHD_CODE_STANDIN(Stage3Start, fn_14_149F8)
DGHD_CODE_STANDIN(EnableDynamicCollision, fn_14_14828)
DGHD_CODE_STANDIN(DisableDynamicCollision, fn_14_14664)
DGHD_CODE_STANDIN(EnableValidTarget, fn_14_14604)
DGHD_CODE_STANDIN(DisableValidTarget, fn_14_145A4)
DGHD_CODE_STANDIN(SelectDeathHeadAttack, fn_14_14470)
DGHD_CODE_STANDIN(SetInitialAttackPosition, fn_14_143D8)
DGHD_CODE_STANDIN(SetAttackPosition, fn_14_14254)
DGHD_CODE_STANDIN(ResetGravityRepulsorHP, fn_14_14248)
DGHD_CODE_STANDIN(EjectPlayerFromBombPit, fn_14_14058)
DGHD_CODE_STANDIN(GrabPlayerHintControl, fn_14_13F70)
DGHD_CODE_STANDIN(SetVulnerableToDamage, fn_14_13F0C)
DGHD_CODE_STANDIN(SetImmuneToDamage, fn_14_13EB8)
DGHD_CODE_STANDIN(SelectRetreatPlatform, fn_14_13C6C)
DGHD_CODE_STANDIN(StartVocalRecoverReaction, fn_14_13C1C)
#undef DGHD_CODE_STANDIN

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
