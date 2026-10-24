#include "MetroidPrime/Enemies/CSporbTop.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CSporbBase.hpp"
#include "MetroidPrime/Enemies/CSporbProjectile.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSporbTop.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "REL/REL_Setup.h"
#include "rstl/math.hpp"

static EMaterialTypes skExcludeCharacter = kMT_Character; // Guessed name
static EMaterialTypes skExcludePlayer = kMT_Player;       // Guessed name

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"AnimOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::AnimOver)},
    {"ShouldPatrol",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::ShouldPatrol)},
    {"ShouldAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::ShouldAttack)},
    {"ShouldFire", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::ShouldFire)},
    {"ShouldReload",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::ShouldReload)},
    {"ShouldClose", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::ShouldClose)},
    {"ShouldSpit", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::ShouldSpit)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Sleep", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Sleep)},
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Patrol)},
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Attack)},
    {"WakeUp", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::WakeUp)},
    {"GoToSleep", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::GoToSleep)},
    {"Fire", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Fire)},
    {"Reload", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Reload)},
    {"Close", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Close)},
    {"Spit", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Spit)},
    {"Flinch", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Flinch)},
};

// @ fn_76_2204
CSporbTop::CSporbTop(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                     const CTransform4f& xf, const CModelData& modelData,
                     const CPatternedInfo& patternedInfo, const CActorParameters& actorParams)
: CPatterned(kPAI_SporbTop, uid, name, kFT_Zero, info, xf, modelData, patternedInfo, kMT_Flyer,
             kCT_One, kBT_Floater, actorParams)
, mState(kS_Invalid)
, mOrbitPosition(GetTranslation())
, mFreezeDuration(0.f)
, mSphere(CSphere(patternedInfo.GetBodyOrigin(),
                  rstl::max_val(patternedInfo.GetHalfExtent(), patternedInfo.GetHeight() * 0.5f)),
          GetMaterialList())
, mFireGenerateType(pas::kGType_Three)
, mBaseId(kInvalidUniqueId)
, mProjectileId(kInvalidUniqueId)
, mImmune(false) {
  SetDrawShadow(false);
  CMaterialList include = GetMaterialFilter().GetIncludeList();
  include.Remove(kMT_Player);
  include.Remove(kMT_Character);
  include.Remove(kMT_ExcludeFromRadar);
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
      include, CMaterialList(skExcludeCharacter, skExcludePlayer)));
}

// @ fn_76_2198
void CSporbTop::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

// @ fn_76_207C
void CSporbTop::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_AreaLoaded: {
    rstl::vector< SConnection >::const_iterator it = GetConnectionList().begin();
    for (; it != GetConnectionList().end(); ++it) {
    }
    break;
  }
  case kSM_Increment:
  case kSM_Decrement:
  case kSM_AIUpdateDisabled:
    break;
  case kSM_Create:
    if (!BodyController()->GetIsActive()) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch);
      BodyController()->Activate(mgr, pas::kAS_Invalid);
    }
    RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    break;
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
}

// @ fn_76_205C
void CSporbTop::Think(float dt, CStateManager& mgr) { CPatterned::Think(dt, mgr); }

// @ fn_76_203C
void CSporbTop::Render(const CStateManager& mgr) const { CPatterned::Render(mgr); }

// @ fn_76_201C
void CSporbTop::PreRender(CStateManager& mgr) { CPatterned::PreRender(mgr); }

// @ fn_76_1FFC
void CSporbTop::AddToRenderer(const CStateManager& mgr) const { CPatterned::AddToRenderer(mgr); }

// @ fn_76_1F94
void CSporbTop::Sleep(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

// @ fn_76_1F2C
void CSporbTop::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    AddMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

// @ fn_76_1EF4
void CSporbTop::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Lurk);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

// @ fn_76_1CC0
void CSporbTop::WakeUp(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Zero, -1));
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Zero, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// @ fn_76_1A6C
void CSporbTop::Flinch(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Six, -1));
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Six, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// @ fn_76_1814
void CSporbTop::GoToSleep(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_One, -1));
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_One, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// @ fn_76_15D8
void CSporbTop::Fire(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(mFireGenerateType, -1));
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(mFireGenerateType, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// @ fn_76_15A0
void CSporbTop::Spit(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

// @ fn_76_1568
void CSporbTop::Close(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Internal6);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

// @ fn_76_1288
void CSporbTop::Reload(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Four, -1));
    BodyController()->SetLocomotionType(pas::kLT_Lurk);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Four, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mState = kS_Attack;
    break;
  }
}

// @ fn_76_126C
bool CSporbTop::ShouldPatrol(CStateManager& mgr, const CTriggerData& data) const {
  return mState >= kS_Patrol;
}

// @ fn_76_1250
bool CSporbTop::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mState >= kS_Attack;
}

// @ fn_76_1230
bool CSporbTop::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::AnimOver(mgr, data);
}

// @ fn_76_1214
bool CSporbTop::ShouldFire(CStateManager& mgr, const CTriggerData& data) const {
  return mState >= kS_Fire;
}

// @ fn_76_11F8
bool CSporbTop::ShouldReload(CStateManager& mgr, const CTriggerData& data) const {
  return mState >= kS_Reload;
}

// @ fn_76_11E8
bool CSporbTop::ShouldClose(CStateManager& mgr, const CTriggerData& data) const {
  return mState == kS_Close;
}

// @ fn_76_11D4
bool CSporbTop::ShouldSpit(CStateManager& mgr, const CTriggerData& data) const {
  return mState == kS_Spit;
}

void CSporbTop::ResetAttack(CStateManager& mgr, bool flag) {}

// @ fn_76_11B4
CVector3f CSporbTop::GetOrbitPosition(const CStateManager& mgr) const { return mOrbitPosition; }

// @ fn_76_1164
void CSporbTop::Freeze(CStateManager& mgr, const CVector3f& position, CUnitVector3f direction,
                       float duration, float intoFreezeDuration) {
  if (!BodyController()->IsFrozen()) {
    mFreezeDuration = duration;
    CPatterned::Freeze(mgr, position, direction, duration, intoFreezeDuration);
  }
}

// @ fn_76_115C
const CCollisionPrimitive* CSporbTop::GetCollisionPrimitive() const { return &mSphere; }

// @ fn_76_10C0
rstl::optional_object< CAABox > CSporbTop::GetTouchBounds() const {
  return GetCollisionPrimitive()->CalculateAABox(GetTransform());
}

// @ fn_76_1048
void CSporbTop::StartFlinch(CStateManager& mgr) {
  mState = kS_Flinch;
  mStateMachine->SetState(mgr, *this, rstl::string_l("Flinch"));
}

void CSporbTop::ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                                const CModelFlags& flags) const {
  const CTransform4f savedXf = GetTransform();
  CModelFlags savedFlags = GetModelFlags();
  const_cast< CTransform4f& >(GetTransform()) = xf;
  const_cast< CSporbTop* >(this)->SetModelFlags(flags);
  CPatterned::Render(mgr);
  if (const CSporbProjectile* projectile =
          TCastToConstPtr< CSporbProjectile >(mgr.GetObjectById(mProjectileId))) {
    projectile->GetModelData()->Render(mgr, xf, projectile->GetActorLights(), flags);
  }
  const_cast< CTransform4f& >(GetTransform()) = savedXf;
  const_cast< CSporbTop* >(this)->SetModelFlags(savedFlags);

  const TUniqueId scanningId = mgr.GetPlayer(0)->GetScanningObject();
  if (scanningId == GetUniqueId() ||
      TCastToConstPtr< CSporbProjectile >(mgr.GetObjectById(scanningId))) {
    if (const CSporbBase* base = TCastToConstPtr< CSporbBase >(mgr.GetObjectById(mBaseId))) {
      const CSegId seg =
          base->GetAnimationData()->GetLocatorSegId(rstl::string_l(CSporbBase::skConnectLocator));
      const CTransform4f locXf = base->GetScaledLocatorTransform(seg);
      base->ScanVisorRender(mgr, xf * locXf.GetInverse(), flags);
    }
  }
}

// @ fn_76_C24
CAABox CSporbTop::GetScanVisorRenderBounds(const CStateManager& mgr) const {
  CAABox bounds = GetModelBounds();
  if (const CSporbBase* base = TCastToConstPtr< CSporbBase >(mgr.GetObjectById(mBaseId))) {
    const CTransform4f locXf =
        base->GetScaledLocatorTransform(rstl::string_l(CSporbBase::skConnectLocator));
    const CTransform4f xf =
        CTransform4f::Translate(-base->GetTranslation()) * base->GetTransform() * locXf;
    const CAABox baseBounds = base->GetModelBounds();
    bounds.Include(baseBounds.GetTransformedAABox(CTransform4f::Translate(-xf.GetTranslation())));
    return bounds;
  }
  return bounds;
}

// @ fn_76_AC8
CAABox CSporbTop::GetModelBounds() const {
  CAABox box = GetAnimationData()->CalcBoundingBoxFromModelVerts();
  box = box.GetTransformedAABox(CTransform4f::Translate(-GetTranslation()) * GetTransform() *
                                CTransform4f::Scale(GetModelData()->GetScale()));
  return box;
}

// @ fn_76_838
void CSporbTop::Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) {
  if (mAlive) {
    BodyController()->SetTimeScale(1.f);
    const CWeaponMode deathWeapon = GetHealthInfo()->GetCauseOfDeathWeapon();
    bool massiveDeath = true;
    if (deathWeapon.GetType() == kWT_Light || deathWeapon.GetType() == kWT_PowerBomb ||
        (deathWeapon.IsComboed() &&
         (deathWeapon.GetType() == kWT_Dark || deathWeapon.GetType() == kWT_Annihilator))) {
      massiveDeath = false;
    }

    if (massiveDeath) {
      mPendingMassiveDeath = true;
      if (mLookAtDeathDir && mXDamageDelay <= 0.f && direction.IsNonZero()) {
        const CVector3f pos = GetTranslation();
        const CVector3f target = pos - direction;
        const CTransform4f deathXf = CTransform4f::LookAt(pos, target) *
                                     CTransform4f::RotateX(CRelAngle::FromRadians(0.7853982f));
        SetTransform(deathXf);
      }
    } else {
      if (mStateMachine->HasState()) {
        mStateMachine->SetState(mgr, *this, rstl::string_l("Dead"));
      }
      RemoveMaterial(kMT_GroundCollider, mgr);
    }

    mAlive = false;
    SetHighlightedInDarkVisor(false);
    IssueDeathBodyCommand(mgr, direction);
    if (state != kSS_InvalidState) {
      SendScriptMsgs(state, mgr, GetUniqueId(), kSM_None);
    }
  }
}

CEntity* LoadSporbTop(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrSporbTop sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrSporbTop.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  return rs_new CSporbTop(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                          LdrToEntityInfo(info, sldrThis.editorProperties),
                          LdrToTransform4f(sldrThis.editorProperties), *modelData,
                          LdrToPatternedInfo(sldrThis.patterned, nullptr),
                          LdrToActorParameters(sldrThis.actorInformation));
}

// @ fn_76_0
CSporbTop::~CSporbTop() {}
