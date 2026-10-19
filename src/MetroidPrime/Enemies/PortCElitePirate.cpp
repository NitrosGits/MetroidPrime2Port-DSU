// Host body for the ElitePirate REL (module 15); in game the class is the Ingsmasher (its scan
// text). The analysis is docs/module-analysis/ElitePirate.md. This slice ports the constructor, the
// vtable overrides (Think, AcceptScriptMsg, PreRender, Render, DoUserAnimEvent, the accessors) and
// the helpers they call from the retail pseudocode; the FSM is installed from fn_15_9840's tables,
// but the 54 trigger/state/code bodies at the end of the file are still StandIn shells.
//
// A `// @ fn_15_XXXX` line above a function marks a body checked against that retail function.
// `[?]` marks a literal reading of retail whose purpose is not established.
#include "MetroidPrime/Enemies/CElitePirate.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CSkinnedModel.hpp"
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
#include <math.h>
#include <stdio.h>

namespace {

// Reports a stand-in the first time it runs, so a log lists what is still a shell.
void StandIn(const char* what) {
  static const char* sSeen[64] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == what) {
      return;
    }
  }
  if (sCount < 64) {
    sSeen[sCount++] = what;
  }
  fprintf(stderr, "[elitepirate] %s is a port stand-in (see PortCElitePirate.cpp)\n", what);
}

// One subdivided arm/leg segment (lbl_15_rodata_0 left, lbl_15_rodata_30 right).
struct SJointPair {
  const char* mPivot;
  const char* mNext;
  float mRadius;
  float mMaxSeparation;
};

const SJointPair skLeftArmJoints[3] = {
    {"L_shoulder", "L_elbow", 1.f, 1.5f},
    {"L_elbow", "L_wrist", 1.1f, 1.3f},
    {"L_knee", "L_ankle", 0.9f, 1.2f},
};
const SJointPair skRightArmJoints[3] = {
    {"R_shoulder", "R_elbow", 1.f, 1.5f},
    {"R_elbow", "R_wrist", 1.1f, 1.3f},
    {"R_knee", "R_ankle", 0.9f, 1.2f},
};

// lbl_15_rodata_60: the single-joint spheres.
struct SJointSphere {
  const char* mName;
  float mRadius;
};
const SJointSphere skJointSpheres[8] = {
    {"Head_1", 1.2f},  {"L_Palm_LCTR", 1.5f}, {"R_Palm_LCTR", 1.5f},
    {"Spine_1", 1.8f}, {"Collar", 1.5f},      {"L_ball", 0.8f},
    {"R_ball", 0.8f},  {"Grenade_Collision_LCTR", 1.f},
};

// @ fn_15_2F10
// True for the palm locator itself or any joint that pivots a subdivided segment of that arm.
bool IsArmClawCollider(const rstl::string& name, const char* locator, const SJointPair* joints) {
  if (name == rstl::string_l(locator)) {
    return true;
  }
  for (int i = 0; i < 3; ++i) {
    if (name == rstl::string_l(joints[i].mPivot)) {
      return true;
    }
  }
  return false;
}

CMaterialFilter SolidFilter() {
  return CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList());
}

} // namespace

// @ fn_15_A190
// @ fn_15_2CFC
CElitePirate::CElitePirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                           const CTransform4f& xf, const CModelData& modelData,
                           const CPatternedInfo& pInfo, const CActorParameters& actParms,
                           const CElitePirateData& data)
// EPatternedAI 0xc, flavor 0, movement 0 (ground), collider 1, body 1 (bipedal).
: CPatterned(static_cast< EPatternedAI >(0xc), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, x7c0_(-1)
, mBodyVulnerability(pInfo.GetDamageVulnerability())
, mLeftPalmPos(CVector3f::Zero())
, mRightPalmPos(CVector3f::Zero())
, x810_(CVector3f::Zero())
, x81c_(CVector3f::Zero())
, mData(data)
, mCollidableBox(GetBoundingBox(), GetMaterialList())
, mHeadColliderId(kInvalidUniqueId)
, mLauncherId(kInvalidUniqueId)
, mShieldColliderId(kInvalidUniqueId)
, xa26_(kInvalidUniqueId)
, mBaseSpeed(mSpeed)
, mSteeringSpeed(1.f)
, mLastHP(0.f)
, mTimer(0.f)
, xa38_(0.f)
, mTime(0.f)
, xa40_(0.f)
, xa44_(0.f) // not written by the ctor; the console's heap is zeroed
, mLastPushTime(0.f)
, mClaimedRegion(0) // [?] retail stores 0, though fn_15_1328 tests it against -1
// The 0x200 flag is the patterned info's encounter byte (+0x120); the radii are rodata 1.0f.
, mPathFindSearch(nullptr, pInfo.IsAnEncounter() ? 0x201 : 1, pInfo.GetPathfindingIndex(), 1.f,
                  1.f, 0, CPFRegion::kRP_Center)
, xb3c_(CVector3f::Zero())
, mPositionHistory(5.f)
, mEvent8Active(false)
, mXDamaged(false)
, mAlerted(false)
, xc10_10_(false)
, mAlerted2(false)
, xc10_04_(false)
, mLauncherIngPossessed(false)
, xc10_01_(false)
, xc14_(0)
, xc28_(-1)
, mAttackKind(-1)
, mFsm2(gpSimplePool->GetObj(SObjectTag('FSM2', data.mStateMachine)))
, xc3c_(6)
, mInitialAnimState(1)
, mEvent7(false)
, xc48_(1)
, xc4c_(1)
, mLastMeleeTime(-1000.f)
, mMeleeDamagePending(false)
, mKnockPending(false)
, xc58_(0.f)
, xc5c_(-1000.f)
, mShieldDamage(0.f)
, mShieldHitTime(-1000.f)
, mShieldAlpha(0.f)
, mShieldPolarity(2)
, mShieldPopPending(false)
, mDoubleSpeed(false)
, xcb0_(1)
, xcb4_(7)
, xcb8_(CVector3f::Zero())
, xcc4_(CVector3f::Zero())
, mProjectileInfo(data.mRocket, data.mRocketDamage)
, mRocketsFired(0)
, xcfc_(0)
, mRocketPattern(0)
, xd04_(1)
, mRocketEventSeen(false)
, mAimBlend(0.f)
, mShieldAimPos(CVector3f::Zero())
, xd1c_(0.f)
, mAngryCount(0)
, xd24_80_(false)
, xd24_40_(false)
, xd24_20_(false)
, xd24_10_(false)
, mEnergyGlow(0.f)
, mAbsorbing(false) {
  if (mData.mPartAsset != kInvalidAssetId) {
    mPart = TLockedToken< CGenDescription >(
        gpSimplePool->GetObj(SObjectTag('PART', mData.mPartAsset)));
  }
  mProjectileInfo.Token().Lock();

  CKnockBackMgr& knockBack = KnockBackController();
  knockBack.EnableAllAnimReactions(false);
  knockBack.EnableBurnDeath(false);
  knockBack.EnableExplodeDeath(false);
  knockBack.EnableBurn(false);
  knockBack.EnableSlow(false);
  knockBack.EnableFreeze(false);
  knockBack.EnableAnimReaction(CKnockBackMgr::kAR_Flinch, true);

  // fn_15_2CFC: a 3 x 3 x 6 box scaled by the model, shared with the path search.
  const CVector3f scale = GetModelData()->GetScale();
  const float radius = 1.5f * scale.GetY();
  const float height = 6.f * scale.GetZ();
  SetBoundingBox(CAABox(-radius, -radius, 0.f, radius, radius, height));
  mCollidableBox = CCollidableAABox(GetBoundingBox(), GetMaterialList());
  mPathFindSearch.SetCharacterRadius(radius);
  mPathFindSearch.SetCharacterHeight(height);

  // The idle variant whose animation is the editor's initial one (state 5, parm 10/11/12).
  for (int n = 10; n <= 12; ++n) {
    const rstl::pair< float, int > best = GetAnimationData()->GetPASDatabase().FindBestAnimation(
        CPASAnimParmData(pas::EAnimationState(5), CPASAnimParm::FromEnum(0),
                         CPASAnimParm::FromEnum(n)),
        -1);
    if (best.second == mData.mInitialAnim) {
      mInitialAnimState = n;
      break;
    }
  }

  if (mData.mShieldedModel != kInvalidAssetId) {
    mShieldModel = TLockedToken< CSkinnedModel >(
        rs_new CSkinnedModel(gpSimplePool->GetObj(SObjectTag('CMDL', mData.mShieldedModel)),
                             gpSimplePool->GetObj(SObjectTag('CSKR', mData.mShieldedSkinRules)),
                             AnimationData()->GetModelData()->GetLayoutInfo()));
  }
}

CElitePirate::~CElitePirate() {}

// @ fn_15_1A48
const CShockWaveInfo& CElitePirate::GetShockWaveInfo() const {
  return xcb0_ == 5 ? mData.mDoubleShockWaveInfo : mData.mSingleShockWaveInfo;
}

// @ fn_15_2CE8
bool CElitePirate::IsShieldUp() const { return GetBodyController()->GetLocomotionType() == 0; }

// @ fn_15_9598
void CElitePirate::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  if (!mStateMachine->HasState()) {
    InstallStateMachine(mgr);
    return;
  }
  if (mShieldPopPending) {
    PopShield(mgr);
    mShieldPopPending = false;
  }
  CPatterned::Think(dt, mgr);

  if (CElitePirateGrenadeLauncher* launcher =
          static_cast< CElitePirateGrenadeLauncher* >(mgr.ObjectById(mLauncherId))) {
    launcher->SetDamageAddColor(mColor);
    launcher->SetSlowedSpeed(GetBodyController()->GetTimeScale());
    launcher->SetFollowPlayer(!GetBodyController()->IsFrozen());
  }
  mBodyCollisionManager->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  mShieldCollisionManager->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);

  if (mDoubleSpeed && IsShieldUp()) {
    mSpeed = 2.f * mBaseSpeed;
  } else {
    mSpeed = mBaseSpeed;
  }
  UpdateTimer(dt);
  UpdatePositionHistory();
  UpdateLauncherTransform(mgr);
  UpdateAimBlend(dt);
  UpdateParticleGen(dt);
  mTime += dt;

  if (!mLauncherIngPossessed && mIngPossessionBlend > 0.15f) {
    DeleteGrenadeLauncher(mgr);
    mLauncherId = mgr.AllocateUniqueId();
    CreateGrenadeLauncher(mgr, mLauncherId);
    if (CEntity* launcher = mgr.ObjectById(mLauncherId)) {
      launcher->SetActive(GetActive());
    }
  }
  UpdateShieldAlpha(dt);
  UpdatePushBack(mgr, dt);
}

// @ fn_15_27DC
void CElitePirate::UpdateTimer(float dt) {
  if (mTimer > 0.f) {
    mTimer -= dt;
  }
}

// @ fn_15_2204
void CElitePirate::UpdatePositionHistory() {
  const CVector3f pos = GetTranslation();
  if (mPathFindSearch.OnPath(pos) == CPathFindSearch::kR_Success) {
    mPositionHistory.Clear();
  }
  mPositionHistory.AddValue(pos);
}

// @ fn_15_2394
void CElitePirate::UpdateLauncherTransform(CStateManager& mgr) {
  if (mLauncherId == kInvalidUniqueId) {
    return;
  }
  CActor* launcher = TCastToPtr< CActor >(mgr.ObjectById(mLauncherId));
  if (launcher == nullptr) {
    mLauncherId = kInvalidUniqueId;
  } else {
    launcher->SetTransform(GetLctrTransform(rstl::string_l("grenadeLauncher_LCTR")));
  }
}

// @ fn_15_8D84
void CElitePirate::UpdateAimBlend(float dt) {
  if (IsShieldUp()) {
    mAimBlend += dt / 0.8f;
    if (mAimBlend > 1.f) {
      mAimBlend = 1.f;
    }
  } else {
    mAimBlend -= dt / 0.4f;
    if (mAimBlend < 0.f) {
      mAimBlend = 0.f;
    }
  }
}

// @ fn_15_8E34
void CElitePirate::UpdateParticleGen(float dt) {
  if (mHandGen.get() != nullptr) {
    mHandGen->SetTranslation(mLeftPalmPos);
    mHandGen->Update(dt);
  }
}

// @ fn_15_8EA0
void CElitePirate::UpdateShieldAlpha(float dt) {
  if (IsShieldUp()) {
    mShieldAlpha += dt / 0.8f;
    if (mShieldAlpha > 1.f) {
      mShieldAlpha = 1.f;
    }
  } else {
    mShieldAlpha -= dt * 0.5f;
    if (mShieldAlpha < 0.f) {
      mShieldAlpha = 0.f;
    }
  }
  if (mAbsorbing) {
    mEnergyGlow += dt / 3.f;
    if (mEnergyGlow > 1.f) {
      mEnergyGlow = 1.f;
    }
  } else {
    mEnergyGlow -= dt * 0.5f;
    if (mEnergyGlow < 0.f) {
      mEnergyGlow = 0.f;
    }
  }
}

// @ fn_15_8FB8
// Steps back when the shield-hand line or the launcher's firing line is blocked.
void CElitePirate::UpdatePushBack(CStateManager& mgr, float dt) {
  if (mAttackKind == 3 || mAttackKind == 5 || mAttackKind == 6) {
    return;
  }
  const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid, kMT_AIBlock),
      CMaterialList(kMT_Floor, kMT_Player, kMT_AIPassthrough, kMT_CollisionActor));
  const CVector3f fwd = GetTransform().GetForward();
  const CVector3f pushBack(0.f, -0.15f, 0.f);

  const CVector3f start = mLeftPalmPos + fwd * 1.8f;
  CVector3f dir = (mRightPalmPos + fwd * 1.8f) - start;
  if (dir.CanBeNormalized()) {
    const float len = dir.Magnitude();
    dir.Normalize();
    if (!CGameCollision::RayStaticLineOfSightTest(
            mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()), start, dir, len, filter)) {
      MoveInOneFrameOR(pushBack, dt);
      mLastPushTime = mTime;
      return;
    }
  }

  if (IsOnJumpHint(mgr)) {
    return;
  }
  const CElitePirateGrenadeLauncher* launcher =
      static_cast< const CElitePirateGrenadeLauncher* >(mgr.GetObjectById(mLauncherId));
  if (launcher == nullptr) {
    return;
  }
  CVector3f r(fwd.GetX(), fwd.GetY(), 0.f);
  r.Normalize();
  const CVector3f up = CVector3f::Up();
  const CVector3f head = GetLctrTransform(rstl::string_l("Head_1")).GetTranslation();
  const CVector3f from =
      head + r * 1.5f + CVector3f(r.GetY(), -r.GetX(), 0.f) * 2.2f - up * 0.6f;
  const CVector3f to = launcher->GetTurretTransform().GetTranslation() + r * 2.f + up * 0.1f;
  CVector3f delta = to - from;
  if (!delta.CanBeNormalized()) {
    return;
  }
  const float len = delta.Magnitude();
  delta.Normalize();
  CAABox box = CAABox::MakeMaxInvertedBox();
  box.AccumulateBounds(from);
  box.AccumulateBounds(to);
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, box, filter, this);
  if (!CGameCollision::RayDynamicLineOfSightTest(mgr, from, delta, len, filter, nearList, this)) {
    MoveInOneFrameOR(pushBack, dt);
    mLastPushTime = mTime;
  }
}

// @ fn_15_1788
// Inside an active same-area AI hint of type 0x1a (XY distance).
bool CElitePirate::IsOnJumpHint(const CStateManager& mgr) const {
  const CObjectList& list = mgr.GetObjectListById(kOL_AiWaypoint);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CScriptAIHint* hint = TCastToConstPtr< CScriptAIHint >(list[i]);
    if (hint != nullptr && hint->GetHintType() == 0x1a &&
        hint->GetCurrentAreaId() == GetCurrentAreaId() && hint->GetActive()) {
      const CVector3f d(hint->GetTranslation().GetX() - GetTranslation().GetX(),
                        hint->GetTranslation().GetY() - GetTranslation().GetY(), 0.f);
      if (d.Magnitude() < hint->GetRadius()) {
        return true;
      }
    }
  }
  return false;
}

// @ fn_15_15F8
// The type of the nearest 0x16/0x17/0x1b hint containing the player; a 0x1b wins at once.
int CElitePirate::GetPlayerHintType(const CStateManager& mgr) const {
  const CVector3f playerPos = mgr.GetPlayer(0)->GetTranslation();
  const CObjectList& list = mgr.GetObjectListById(kOL_AiWaypoint);
  const CScriptAIHint* nearest = nullptr;
  float nearestDist = FLT_MAX;
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CScriptAIHint* hint = TCastToConstPtr< CScriptAIHint >(list[i]);
    if (hint == nullptr || hint->GetCurrentAreaId() != GetCurrentAreaId() || !hint->GetActive()) {
      continue;
    }
    const int type = hint->GetHintType();
    if (type != 0x16 && type != 0x1b && type != 0x17) {
      continue;
    }
    const float dist = (hint->GetTranslation() - playerPos).Magnitude();
    if (dist < hint->GetRadius()) {
      if (type == 0x1b) {
        return 0x1b;
      }
      if (dist < nearestDist) {
        nearest = hint;
        nearestDist = dist;
      }
    }
  }
  return nearest != nullptr ? nearest->GetHintType() : -1;
}

// @ fn_15_1598
CPFArea* CElitePirate::GetPathArea(CStateManager& mgr) const {
  return mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea;
}

// @ fn_15_1328
void CElitePirate::ReleaseClaimedRegion(CStateManager& mgr) {
  if (mClaimedRegion != -1 && GetPathArea(mgr) != nullptr) {
    CPFRegionData* region = GetPathArea(mgr)->GetRegion(mClaimedRegion).Data();
    if (region != nullptr) {
      region->SetAvoidanceFlags(region->GetAvoidanceFlags() & ~4u);
    }
    mClaimedRegion = -1;
  }
}

// @ fn_15_36CC
void CElitePirate::SetupCollisionManagers(CStateManager& mgr) {
  const CAnimData* animData = GetAnimationData();
  rstl::vector< CJointCollisionDescription > descs;
  const SJointPair* tables[2] = {skLeftArmJoints, skRightArmJoints};
  for (int t = 0; t < 2; ++t) {
    for (int i = 0; i < 3; ++i) {
      const SJointPair& joint = tables[t][i];
      const CSegId pivot = animData->GetLocatorSegId(rstl::string_l(joint.mPivot));
      const CSegId next = animData->GetLocatorSegId(rstl::string_l(joint.mNext));
      if (pivot != CSegId::Invalid() && next != CSegId::Invalid()) {
        descs.push_back(CJointCollisionDescription::SphereSubdivideCollision(
            pivot, next, joint.mRadius, joint.mMaxSeparation,
            CJointCollisionDescription::kOT_BetweenJoints, rstl::string_l(joint.mPivot), 5.f));
      }
    }
  }
  for (int i = 0; i < 8; ++i) {
    const CSegId seg = animData->GetLocatorSegId(rstl::string_l(skJointSpheres[i].mName));
    if (seg != CSegId::Invalid()) {
      descs.push_back(CJointCollisionDescription::SphereCollision(
          seg, CVector3f::Zero(), skJointSpheres[i].mRadius,
          rstl::string_l(skJointSpheres[i].mName), 5.f));
    }
  }
  mBodyCollisionManager =
      rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), descs, true);
  mBodyCollisionManager->SetActive(mgr, GetActive());
  mRightArmColliders.clear();
  mLeftArmColliders.clear();

  rstl::vector< CJointCollisionDescription > shieldDescs;
  const CSegId palm = animData->GetLocatorSegId(rstl::string_l("L_Palm_LCTR"));
  shieldDescs.push_back(CJointCollisionDescription::OBBCollision(
      palm, CVector3f(4.f, 4.f, 2.f), CVector3f::Zero(), rstl::string_l("Shield"), 5.f));
  mShieldCollisionManager =
      rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), shieldDescs, false);

  ClassifyCollisionActors(mgr);
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid), CMaterialList(kMT_ProjectilePassthrough, kMT_CollisionActor,
                                              kMT_AIPassthrough, kMT_Player, kMT_Platform)));
  AddMaterial(kMT_ProjectilePassthrough, mgr);

  mShieldColliderId = mShieldCollisionManager->GetCollisionDescFromIndex(0).GetCollisionActorId();
  if (CCollisionActor* shield = TCastToPtr< CCollisionActor >(mgr.ObjectById(mShieldColliderId))) {
    shield->SetResponseType(EWeaponCollisionResponseTypes(0));
  }
  SetShieldVulnerability(mgr, false);
  mShieldCollisionManager->AddMaterialList(mgr, CMaterialList(kMT_AIJoint, kMT_CameraPassthrough));
}

// @ fn_15_3574
void CElitePirate::ClassifyCollisionActors(CStateManager& mgr) {
  for (uint i = 0; i < mBodyCollisionManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc = mBodyCollisionManager->GetCollisionDescFromIndex(i);
    if (TCastToPtr< CCollisionActor >(mgr.ObjectById(desc.GetCollisionActorId())) == nullptr) {
      continue;
    }
    if (desc.GetName() == rstl::string_l("Head_1")) {
      mHeadColliderId = desc.GetCollisionActorId();
    } else if (IsArmClawCollider(desc.GetName(), "R_Palm_LCTR", skRightArmJoints)) {
      mRightArmColliders.push_back(desc.GetCollisionActorId());
    } else if (IsArmClawCollider(desc.GetName(), "L_Palm_LCTR", skLeftArmJoints)) {
      mLeftArmColliders.push_back(desc.GetCollisionActorId());
    }
  }
}

// @ fn_15_27FC
// The body colliders share the Ingsmasher's health; with the shield up they and the shield
// reflect, except that the shield lets the opposite beam (and the Annihilator) through.
void CElitePirate::SetShieldVulnerability(CStateManager& mgr, bool shielded) {
  mLastHP = HealthInfo()->GetHP();
  const CSegId palm = GetAnimationData()->GetLocatorSegId(rstl::string_l("L_Palm_LCTR"));
  for (uint i = 0; i < mBodyCollisionManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc = mBodyCollisionManager->GetCollisionDescFromIndex(i);
    CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(desc.GetCollisionActorId()));
    if (actor == nullptr) {
      continue;
    }
    *actor->HealthInfo() = *HealthInfo();
    actor->SetDamageVulnerability(shielded ? CDamageVulnerability::ReflectVulnerabilty()
                                           : mBodyVulnerability.MakeIgnoreRadius());
    if (desc.GetPivotId() == palm) {
      if (shielded) {
        actor->AddMaterial(kMT_ProjectilePassthrough, mgr);
      } else {
        actor->RemoveMaterial(kMT_ProjectilePassthrough, mgr);
      }
    }
  }

  CCollisionActor* shield = TCastToPtr< CCollisionActor >(mgr.ObjectById(mShieldColliderId));
  if (shield == nullptr) {
    return;
  }
  if (!shielded) {
    shield->SetDamageVulnerability(mBodyVulnerability.MakeIgnoreRadius());
    return;
  }
  CDamageVulnerability vulnerability = CDamageVulnerability::ReflectVulnerabilty();
  const CWeaponTypeVulnerability normal(1.f, CWeaponTypeVulnerability::kE_Normal, false);
  for (int type = 1; type <= 3; ++type) {
    if ((mShieldPolarity == 0 && type == 1) || (mShieldPolarity == 1 && type == 2)) {
      continue;
    }
    vulnerability.SetVulnerability(type, normal);
    vulnerability.SetChargedVulnerability(type, normal);
    vulnerability.SetComboVulnerability(type, normal);
  }
  shield->SetDamageVulnerability(vulnerability);
}

// @ fn_15_2564
void CElitePirate::CreateGrenadeLauncher(CStateManager& mgr, TUniqueId uid) {
  const CAnimationParameters& anim =
      IsIngPossessed() ? mData.mUnknown7E6E0D38 : mData.mRocketLauncherAnimInfo;
  if (anim.GetACSFile() == kInvalidAssetId) {
    return;
  }
  const CModelData modelData(CAnimRes(anim.GetACSFile(), anim.GetCharacter(),
                                      GetModelData()->GetScale(), anim.GetInitialAnimation(),
                                      true));
  mgr.AddObject(rs_new CElitePirateGrenadeLauncher(
      uid, rstl::string_l("Rocket Launcher"),
      CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true), GetTransform(),
      modelData, mData.mRocketLauncherActorInfo, GetUniqueId(), 0.f));
  mLauncherIngPossessed = IsIngPossessed();
}

// @ fn_15_251C
void CElitePirate::DeleteGrenadeLauncher(CStateManager& mgr) {
  mgr.DeleteObjectRequest(mLauncherId);
  mLauncherId = kInvalidUniqueId;
}

// @ fn_15_24F0
// @ fn_15_2448
void CElitePirate::SendLauncherMsg(CStateManager& mgr, bool start) {
  if (mLauncherId == kInvalidUniqueId) {
    return;
  }
  if (CEntity* launcher = mgr.ObjectById(mLauncherId)) {
    mgr.SendScriptMsg(launcher, GetUniqueId(), start ? kSM_Start : kSM_Stop, kInvalidUniqueId);
  }
}

// @ fn_15_7004
void CElitePirate::ApplyMeleeDamage(CStateManager& mgr, TUniqueId target) {
  mgr.ApplyDamage(GetUniqueId(), target, GetUniqueId(), mData.mMeleeDamage, SolidFilter(),
                  CVector3f::Zero());
  mMeleeDamagePending = false;
  mLastMeleeTime = mTime;
}

// @ fn_15_1B60
void CElitePirate::PushPlayer(CStateManager& mgr, float horizontal, float vertical) {
  CPlayer* player = mgr.GetPlayer(0);
  CVector3f dir(player->GetTranslation().GetX() - GetTranslation().GetX(),
                player->GetTranslation().GetY() - GetTranslation().GetY(), 0.f);
  dir.Normalize();
  dir = CVector3f(dir.GetX() * horizontal, dir.GetY() * horizontal, vertical);
  player->Stop();
  player->ApplyImpulseWR(dir * player->GetMass(), CAxisAngle::Identity());
  player->SetMoveState(NPlayer::EPlayerMovementState(2), mgr);
}

// @ fn_15_1C7C
// The stomp: nearby players in morph ball are bounced, others get a camera-shake rumble.
void CElitePirate::ShakePlayers(CStateManager& mgr) {
  const bool notShockwave = mAttackKind != 7;
  for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
    CPlayer* player = mgr.GetPlayer(i);
    const float dist = (GetTranslation() - player->GetTranslation()).Magnitude();
    const float strength = notShockwave ? 0.25f : 1.f;
    if (!(strength * GetModelData()->GetScale().Magnitude() - 0.05f * dist > 0.f) ||
        player->GetSurfaceRestraint() == 1 || player->GetFluidCount() != 0) {
      continue;
    }
    const int morph = player->GetSpawnedMorphballState() == 0
                          ? static_cast< int >(player->GetMorphballTransitionState())
                          : 0;
    if (morph == 1) {
      const float impulse = notShockwave ? 10.f : 20.f;
      player->Stop();
      player->ApplyImpulseWR(CVector3f::Up() * impulse * player->GetMass(), CAxisAngle::Identity());
      player->SetMoveState(NPlayer::EPlayerMovementState(2), mgr);
    } else if (mgr.CameraManager(i)->GetCurrentCameraId(true) ==
               mgr.CameraManager(i)->GetFirstPersonCamera()->GetUniqueId()) {
      mgr.RumbleManager(mgr.MaskUIdNumPlayers(player->GetUniqueId()))
          ->Rumble(mgr, kRFX_CameraShake, 1.f, kRP_Two);
    }
  }
}

// @ fn_15_3C5C
void CElitePirate::PopShield(CStateManager& mgr) {
  rstl::optional_object< TLockedToken< CGenDescription > >& pop =
      mShieldPolarity == 1 ? mLightShieldPop : mDarkShieldPop;
  const CAssetId asset = mShieldPolarity == 1 ? mData.mLightShieldPop : mData.mDarkShieldPop;
  if (!pop && asset != kInvalidAssetId) {
    pop = TLockedToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', asset)));
  }
  if (!pop) {
    return;
  }
  CTransform4f xf = GetTransform();
  xf.SetTranslation(mLeftPalmPos);
  mgr.AddObject(rs_new CExplosion(*pop, mgr.AllocateUniqueId(),
                                  CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true),
                                  rstl::string_l("IngSmasher shield pop"), xf, 0,
                                  CVector3f(1.f, 1.f, 1.f), CColor::White(), -1));
}

// @ fn_15_70D4
void CElitePirate::CreateShockWave(CStateManager& mgr, const CInt32POINode& node) {
  CTransform4f xf = GetTransform();
  const CVector3f& palm =
      node.GetLocatorName() == rstl::string_l("R_ball") ? mRightPalmPos : mLeftPalmPos;
  xf.SetTranslation(CVector3f(palm.GetX(), palm.GetY(), GetTranslation().GetZ()));
  mgr.AddObject(rs_new CShockWave(mgr.AllocateUniqueId(), rstl::string_l("Shock Wave"),
                                  CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true),
                                  xf, GetUniqueId(), GetShockWaveInfo(), 2.f, 0.4f));
}

// @ fn_15_728C
// Pattern 1 spreads four rockets across the player (offsets indexed by the rocket count, 1..4);
// pattern 0 fires one homing rocket.
void CElitePirate::FireRocket(CStateManager& mgr) {
  ++mRocketsFired;
  const CElitePirateGrenadeLauncher* launcher =
      static_cast< const CElitePirateGrenadeLauncher* >(mgr.GetObjectById(mLauncherId));
  if (launcher == nullptr || !mProjectileInfo.Token().TryCache()) {
    return;
  }
  CTransform4f xf = launcher->GetTurretTransform();
  const CVector3f origin = xf.GetTranslation();
  TUniqueId homing = kInvalidUniqueId;
  CPlayer* player = mgr.GetPlayer(0);
  if (mRocketPattern == 1) {
    const CVector3f aim = player->GetAimPosition(mgr, 0.f);
    CVector3f d(aim.GetX() - origin.GetX(), aim.GetY() - origin.GetY(), 0.f);
    if (d.CanBeNormalized()) {
      d.Normalize();
      const int hint = GetPlayerHintType(mgr);
      const float s = hint == 0x17 ? 1.5f : 1.f;
      const CVector3f offsets[4] = {
          CVector3f(-d.GetY() * 3.2f * s, d.GetX() * 3.2f * s, 0.f),
          CVector3f(-d.GetY() * 1.2f * s, d.GetX() * 1.2f * s, 0.f),
          CVector3f(d.GetY() * 1.2f * s, -d.GetX() * 1.2f * s, 0.f),
          CVector3f(d.GetY() * 3.2f * s, -d.GetX() * 3.2f * s, 0.f),
      };
      // [?] retail indexes with no bound; the states fire at most four per volley.
      CVector3f target = player->GetAimPosition(mgr, 0.f) + offsets[mRocketsFired - 1];
      if (hint == 0x17) {
        target = CVector3f(target.GetX(), target.GetY(),
                           target.GetZ() + mgr.Random()->Range(6.f, 12.f));
        homing = player->GetUniqueId();
      }
      xf = CTransform4f::LookAt(origin, target, CVector3f::Up());
    }
  } else if (mRocketPattern == 0) {
    homing = player->GetUniqueId();
  }
  mgr.AddObject(rs_new CEnergyProjectile(
      true, mProjectileInfo.Token(), kWT_AI, xf, kMT_Character, mProjectileInfo.GetDamage(),
      mgr.AllocateUniqueId(), GetCurrentAreaId(), GetUniqueId(), homing, 0, false,
      CVector3f(1.f, 1.f, 1.f), CImpactVisorEffect::None(), false, true, false, 1.f, 4.f, 4.f));
}

// @ fn_15_8580
void CElitePirate::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const TUniqueId sender = msg.GetSenderId();
  switch (msg.GetMessage()) {
  case kSM_Delete:
    ReleaseClaimedRegion(mgr);
    if (mBodyCollisionManager.get() != nullptr) {
      mBodyCollisionManager->Destroy(mgr);
    }
    if (mShieldCollisionManager.get() != nullptr) {
      mShieldCollisionManager->Destroy(mgr);
    }
    mgr.DeleteObjectRequest(mLauncherId);
    mLauncherId = kInvalidUniqueId;
    break;
  case kSM_Deactivate:
    mBodyCollisionManager->SetActive(mgr, false);
    mShieldCollisionManager->SetActive(mgr, false);
    break;
  case kSM_Alert:
    mAlerted = true;
    mAlerted2 = true;
    break;
  case kSM_Activate:
    mBodyCollisionManager->SetActive(mgr, true);
    if (CEntity* launcher = mgr.ObjectById(mLauncherId)) {
      launcher->SetActive(true);
    }
    break;
  case kSM_Create: {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
    SetupCollisionManagers(mgr);
    mLauncherId = mgr.AllocateUniqueId();
    CreateGrenadeLauncher(mgr, mLauncherId);
    if (CEntity* launcher = mgr.ObjectById(mLauncherId)) {
      launcher->SetActive(GetActive());
    }
    const CBodyStateInfo& info = GetBodyController()->GetBodyStateInfo();
    if (info.GetMaxSpeed() > 0.f) {
      mSteeringSpeed = info.GetLocomotionSpeed(pas::ELocomotionAnim(1)) * 0.99f / info.GetMaxSpeed();
    }
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed);
    BodyController()->CommandMgr().SetSteeringSpeedRange(mSteeringSpeed, mSteeringSpeed);
    break;
  }
  case kSM_AreaLoaded:
    mPathFindSearch.SetArea(GetPathArea(mgr));
    break;
  case kSM_ResistedDamage:
  case kSM_ReflectedDamage:
    SetXDamaged(true);
    break;
  case kSM_HitObject: {
    if (!mAlive) {
      break;
    }
    const CCollisionActor* collider = TCastToConstPtr< CCollisionActor >(mgr.GetObjectById(sender));
    if (HealthInfo()->GetHP() > 0.f && mAttackKind == 4 && collider != nullptr) {
      if (const CPlayer* player =
              TCastToConstPtr< CPlayer >(mgr.GetObjectById(collider->GetLastTouchedObject()))) {
        if (mMeleeDamagePending) {
          ApplyMeleeDamage(mgr, player->GetUniqueId());
        }
        if (mKnockPending) {
          PushPlayer(mgr, mgr.GetPlayer(0)->GetMorphballTransitionState() == 1 ? 60.f : 25.f,
                     15.f);
        }
      }
    }
    if (collider != nullptr && collider->GetLastTouchedObject() != GetUniqueId()) {
      const CPlayer* player =
          TCastToConstPtr< CPlayer >(mgr.GetObjectById(collider->GetLastTouchedObject()));
      if (player != nullptr && mCurDamageRemTime <= 0.f && mLastMeleeTime + 1.5f < mTime) {
        mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(), GetContactDamage(),
                        SolidFilter(), CVector3f::Zero());
        mCurDamageRemTime = mDamageWaitTime;
      }
    }
    break;
  }
  case kSM_Damage: {
    const CCollisionActor* collider = TCastToConstPtr< CCollisionActor >(mgr.GetObjectById(sender));
    const CGameProjectile* projectile =
        collider != nullptr
            ? TCastToConstPtr< CGameProjectile >(mgr.GetObjectById(collider->GetLastTouchedObject()))
            : nullptr;
    if (IsShieldUp()) {
      if (projectile != nullptr) {
        const int type = projectile->GetCurrentDamageInfo().GetWeaponMode().GetType();
        if ((mShieldPolarity == 1 && (type == 1 || type == 3)) ||
            (mShieldPolarity == 0 && (type == 2 || type == 3))) {
          mShieldDamage += projectile->GetCurrentDamageInfo().GetDamage();
          mShieldHitTime = mTime;
        }
      }
    } else if (!mAbsorbing && projectile != nullptr) {
      CDamageInfo damage = projectile->GetCurrentDamageInfo();
      damage.SetRadius(0.f);
      mgr.ApplyDamage(projectile->GetUniqueId(), GetUniqueId(), projectile->GetUniqueId(), damage,
                      SolidFilter(), CVector3f::Zero());
      SetXDamaged(true);
    }
    const float pct = HealthInfo()->GetHP() / HealthInfo()->GetInitialHP() * 100.f;
    KnockBackController().EnableFreeze(pct <= 25.f);
    KnockBackController().EnableSlow(pct <= 25.f);
    return;
  }
  default:
    break;
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
}

// @ fn_15_83C0
void CElitePirate::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);
  mLeftPalmPos = GetLctrTransform(rstl::string_l("L_Palm_LCTR")).GetTranslation();
  mRightPalmPos = GetLctrTransform(rstl::string_l("R_Palm_LCTR")).GetTranslation();
  if (CActor* launcher = TCastToPtr< CActor >(mgr.ObjectById(mLauncherId))) {
    launcher->SetModelFlags(GetModelFlags());
  }
  SetModelFlags(GetModelFlags().UseShaderSet(0));
  UpdateParticleGen(0.f);
}

// @ fn_15_84EC
void CElitePirate::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
}

// @ fn_15_8294
// Register 1 flashes white when the shield is hit and fades to black over a second.
void CElitePirate::Render(const CStateManager& mgr) const {
  if (mShieldHitTime + 1.f <= mTime) {
    gpRender->SetGXRegister1Color(CColor::Black());
  } else {
    gpRender->SetGXRegister1Color(
        CColor::Lerp(CColor::White(), CColor::Black(), mTime - mShieldHitTime));
  }
  CPatterned::Render(mgr);
  RenderShield(mgr);
  if (mAlive && mHandGen.get() != nullptr) {
    mHandGen->Render();
  }
}

// @ fn_15_8080
void CElitePirate::RenderShield(const CStateManager& mgr) const {
  if ((mShieldAlpha == 0.f && mEnergyGlow == 0.f) || !mShieldModel) {
    return;
  }
  gpRender->SetModelMatrix(GetTransform() *
                           CTransform4f::Scale(GetModelData()->GetScale()));
  CColor color = CColor::White();
  float alpha;
  if (mEnergyGlow > 0.f) {
    alpha = mEnergyGlow * ((sinf(8.f * mTime) + 1.f) * 0.5f * 0.15f + 0.25f);
  } else {
    float freq;
    if (mShieldPolarity == 1) {
      freq = 10.f;
    } else if (mShieldPolarity == 0) {
      color = CColor::Purple();
      freq = 16.f;
    } else {
      return;
    }
    alpha = mShieldAlpha;
    if (alpha == 1.f) {
      alpha = (sinf(freq * mTime) + 1.f) * 0.5f * 0.4f + 0.6f;
    }
  }
  color.SetAlpha(alpha);
  GetAnimationData()->Render(*(**mShieldModel),
                             CModelFlags(CModelFlags::kT_Blend, 0, CModelFlags::EFlags(3), color));
}

// @ fn_15_800C
void CElitePirate::RenderIngSnatchingTransition(const CStateManager& mgr) const {
  CPatterned::RenderIngSnatchingTransition(mgr);
  const CUnitVector3f up = CVector3f::Up();
  GetModelData()->SetupWorldSpacePortalPlane(
      GetTransform(), CPlane(CVector3f::Dot(up, GetTranslation()), up));
}

// @ fn_15_7EEC
// Blends towards the shield collider while the shield is up.
CVector3f CElitePirate::GetAimPosition(const CStateManager& mgr, float dt) const {
  const CVector3f base = CPatterned::GetAimPosition(mgr, dt);
  if (mAimBlend == 0.f) {
    return base;
  }
  if (const CActor* shield = TCastToConstPtr< CActor >(mgr.GetObjectById(mShieldColliderId))) {
    mShieldAimPos = shield->GetTranslation();
  }
  return base * (1.f - mAimBlend) + mShieldAimPos * mAimBlend;
}

// @ fn_15_7BFC
void CElitePirate::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                   EUserEventType type, float dt) {
  bool handled = false;
  switch (static_cast< int >(type)) {
  case 0:
    if (mAttackKind == 7) {
      CreateShockWave(mgr, node);
    } else if (mAttackKind == 2 || mAttackKind == 8) {
      FireRocket(mgr);
    }
    handled = true;
    break;
  case 7:
    mEvent7 = true;
    break;
  case 8:
    mEvent8Active = true;
    handled = true;
    break;
  case 9:
    mEvent8Active = false;
    handled = true;
    break;
  case 16:
    ApplyScreenShake(mgr, GetTranslation(),
                     GetConnectedObject(mgr, kSS_InternalState1, kSM_Attach));
    ShakePlayers(mgr);
    handled = true;
    break;
  case 17:
    if (mAttackKind == 4) {
      mMeleeDamagePending = true;
      mKnockPending = true;
      SetArmTouchBounds(mgr, CVector3f(1.5f, 1.5f, 1.5f));
    }
    break;
  case 18:
    if (mAttackKind == 2) {
      mRocketEventSeen = true;
    } else if (mAttackKind == 4) {
      mMeleeDamagePending = false;
      mKnockPending = false;
      SetArmTouchBounds(mgr, CVector3f::Zero());
    }
    break;
  case 23:
    for (uint i = 0; i < mBodyCollisionManager->GetNumCollisionActors(); ++i) {
      if (CActor* actor = TCastToPtr< CActor >(mgr.ObjectById(
              mBodyCollisionManager->GetCollisionDescFromIndex(i).GetCollisionActorId()))) {
        actor->AddMaterial(kMT_ProjectilePassthrough, mgr);
      }
    }
    handled = true;
    break;
  case 24:
    mBodyCollisionManager->SetActive(mgr, false);
    RemoveMaterial(kMT_Solid, mgr);
    break;
  case 26:
    ApplyScreenShake(mgr, GetTranslation(), GetConnectedObject(mgr, kSS_Footstep, kSM_Attach));
    handled = true;
    break;
  default:
    break;
  }
  if (!handled) {
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);
  }
}

// Part of fn_15_7BFC (events 17/18): both arms' colliders grow or lose their touch margin.
void CElitePirate::SetArmTouchBounds(CStateManager& mgr, const CVector3f& bounds) {
  for (int i = 0; i < mRightArmColliders.size(); ++i) {
    if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(mRightArmColliders[i]))) {
      actor->SetExtendedTouchBounds(bounds);
    }
  }
  for (int i = 0; i < mLeftArmColliders.size(); ++i) {
    if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(mLeftArmColliders[i]))) {
      actor->SetExtendedTouchBounds(bounds);
    }
  }
}

namespace {

// fn_15_9840's tables (lbl_15_data_178, _464, _5CC), in their order. Own members are cast to the
// base-class pointer-to-member types, as CSpacePirate's tables do.
#define EP_TRIGGER(name)                                                                           \
  { #name, reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CElitePirate::name) }
#define EP_STATE(name)                                                                             \
  { #name, reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CElitePirate::name) }
#define EP_CODE(name)                                                                              \
  { #name, reinterpret_cast< CPatterned::StateMachine::CodeFunc >(&CElitePirate::name) }

CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    EP_TRIGGER(Alerted), EP_TRIGGER(AngryAttackOver), EP_TRIGGER(AttackPatternOver),
    EP_TRIGGER(BreakProjectileAttack), EP_TRIGGER(CanShockwave), EP_TRIGGER(ClearLineOfSight),
    EP_TRIGGER(DonePursuing), EP_TRIGGER(DoneTurning), EP_TRIGGER(HasAttackPattern),
    EP_TRIGGER(InDetectionRange), EP_TRIGGER(InPosition), EP_TRIGGER(NotReachedTarget),
    EP_TRIGGER(PickedSpreadShot), EP_TRIGGER(PlayerInNoAttack), EP_TRIGGER(PoweredDown),
    EP_TRIGGER(PoweredUp), EP_TRIGGER(ReadyToCharge), EP_TRIGGER(ReturnedToPatrol),
    EP_TRIGGER(ShieldKilled), EP_TRIGGER(ShockwaveIsNext), EP_TRIGGER(ShotAt),
    EP_TRIGGER(ShouldAlert), EP_TRIGGER(ShouldFire), EP_TRIGGER(ShouldMeleeAttack),
    EP_TRIGGER(ShouldShockwave), EP_TRIGGER(ShouldTurn), EP_TRIGGER(SpotPlayer),
    EP_TRIGGER(StillAngry), EP_TRIGGER(TargetNotOnMesh), EP_TRIGGER(TargetUnreachable),
    EP_TRIGGER(TooClose),
};

CPatterned::StateMachine::SStateFunction skStates[] = {
    EP_STATE(Alert), EP_STATE(AngryAttackBegin), EP_STATE(AngryAttackEnd),
    EP_STATE(Dead), EP_STATE(FollowAttackPattern), EP_STATE(InvulnAlert),
    EP_STATE(MeleeAttack), EP_STATE(Patrol), EP_STATE(PowerDown),
    EP_STATE(PowerUp), EP_STATE(ProjectileAttack), EP_STATE(Pursue),
    EP_STATE(Shielding), EP_STATE(ShieldUp), EP_STATE(Shockwave),
    EP_STATE(SpreadShot), EP_STATE(Stunned), EP_STATE(TargetPatrol),
    EP_STATE(Taunt), EP_STATE(Turn), EP_STATE(Wait),
};

CPatterned::StateMachine::SCodeFunction skCodes[] = {
    EP_CODE(SelectTarget), EP_CODE(PickAttackType),
};

#undef EP_TRIGGER
#undef EP_STATE
#undef EP_CODE

} // namespace

// @ fn_15_9840
// CPatterned's tables come from InitializeStateMachine -> SetupStateMachine; the Ingsmasher's own
// (31 triggers, 21 states, 2 code functions) are installed after it, as retail does.
void CElitePirate::InstallStateMachine(CStateManager& mgr) {
  InitializeStateMachine(mgr);
  mStateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  mStateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  mStateMachine->SetCodeFunctions(skCodes, ARRAY_SIZE(skCodes));
}

// Retail fn_15_178. Property ids and order are the generated SLdrElitePirate reader's, which the
// analysis checked against the REL's switch id by id: the loader's dispatch tree reads all 43.
static CEntity* PortLoadElitePirate(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrElitePirate sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrElitePirate.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL checks the optional's valid byte (r1+0x1fc) and returns null when it is empty.
    return nullptr;
  }

  CElitePirateData data;
  // data+0x00 is the loader's first integer argument (r4 = patterned+0x1e4), which the class's
  // constructor loads as the 'FSM2' token at 0xc30 - i.e. the patterned info's second state
  // machine asset. data+0x08 is a CDamageInfo copied out of the builder's damage argument, i.e.
  // the melee damage. The rest of the REL builder's parameter list could not be mapped back to
  // properties (see docs/module-analysis/ElitePirate.md), so the remaining fields are assigned by
  // name and nothing in this milestone reads them.
  data.mStateMachine = sldrThis.patterned.stateMachine2;
  data.mInitialAnim = sldrThis.patterned.animationInformation.initial_anim;
  data.mMeleeDamage = LdrToDamageInfo(sldrThis.meleeDamage);
  data.mMaxMeleeRange = sldrThis.maxMeleeRange;
  data.mMinShockwaveRange = sldrThis.minShockwaveRange;
  data.mMaxShockwaveRange = sldrThis.maxShockwaveRange;
  data.mMinRocketRange = sldrThis.minRocketRange;
  data.mMaxRocketRange = sldrThis.maxRocketRange;
  data.mUnknown5236C2B6 = sldrThis.unknown_0x5236c2b6;
  data.mUnknown01EAAB17 = sldrThis.unknown_0x01eaab17;
  data.mShieldedModel = sldrThis.shieldedModel;
  data.mShieldedSkinRules = sldrThis.shieldedSkinRules;
  data.mDarkShield = sldrThis.darkShield;
  data.mDarkShieldSound = static_cast< ushort >(sldrThis.darkShieldSound);
  data.mDarkShieldPop = sldrThis.darkShieldPop;
  data.mLightShield = sldrThis.lightShield;
  data.mLightShieldSound = static_cast< ushort >(sldrThis.lightShieldSound);
  data.mLightShieldPop = sldrThis.lightShieldPop;
  data.mTauntInterval = sldrThis.tauntInterval;
  data.mTauntVariance = sldrThis.tauntVariance;
  data.mSingleShockWaveInfo = CShockWaveInfo(sldrThis.singleShockWaveInfo);
  data.mDoubleShockWaveInfo = CShockWaveInfo(sldrThis.doubleShockWaveInfo);
  data.mUnknown28B39197 = sldrThis.unknown_0x28b39197;
  data.mUnknownE27DE71B = sldrThis.unknown_0xe27de71b;
  data.mUnknown665E7ACE = sldrThis.unknown_0x665e7ace;
  data.mUnknownACD4D06D = sldrThis.unknown_0xacd4d06d;
  data.mRocket = sldrThis.rocket;
  data.mRocketDamage = LdrToDamageInfo(sldrThis.rocketDamage);
  data.mUnknown624222F8 = sldrThis.unknown_0x624222f8;
  data.mUnknown31E43A1C = sldrThis.unknown_0x31e43a1c;
  data.mRepeatedAttackChance = sldrThis.repeatedAttackChance;
  data.mEnergyAbsorbDuration = sldrThis.energyAbsorbDuration;
  data.mEnergyAbsorbVariance = sldrThis.energyAbsorbVariance;
  data.mEnergyAttractionForce = sldrThis.energyAttractionForce;
  data.mPartAsset = sldrThis.alwaysFF;
  data.mAlwaysFF23F5E1EE = sldrThis.alwaysFF_0x23f5e1ee;
  data.mRocketLauncherActorInfo = LdrToActorParameters(sldrThis.rocketLauncherActorInfo);
  data.mRocketLauncherAnimInfo = LdrToAnimationParameters(sldrThis.rocketLauncherAnimInfo);
  data.mUnknown7E6E0D38 = LdrToAnimationParameters(sldrThis.unknown_0x7e6e0d38);
  data.mVisorElectricEffect = sldrThis.visorElectricEffect;
  data.mSoundVisorElectric = static_cast< ushort >(sldrThis.sound_VisorElectric);

  return rs_new CElitePirate(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                             LdrToEntityInfo(info, sldrThis.editorProperties),
                             LdrToTransform4f(sldrThis.editorProperties), *modelData,
                             LdrToPatternedInfo(sldrThis.patterned, &sldrThis.ingPossessionData),
                             LdrToActorParameters(sldrThis.actorInformation), data);
}

static SElitePirate_FuncPtrs REL_loader_ElitePirate;

// Retail's RELMain calls fn_15_148, which stores the loader fn_15_178 into lbl_15_bss_0 (the
// module's SElitePirate_FuncPtrs, a single loader slot) and calls the DOL's SetSElitePirate_FuncPtrs
// (fn_80218AD4). The proxy in SElitePirate_FuncPtrs.cpp calls through it.
void RELMain() {
  REL_loader_ElitePirate.mLoader = PortLoadElitePirate;
  SetSElitePirate_FuncPtrs(&REL_loader_ElitePirate);
}

void RELExit() { SetSElitePirate_FuncPtrs(nullptr); }

// ---- FSM triggers/states (later slices)

namespace {

// Retail's 0x17/0x1b hint types: a jump-down zone / a "no attack" zone around the player.
const int kHintJumpDown = 0x17;
const int kHintNoAttack = 0x1b;
// Angle (rad) past which the Ingsmasher must turn before it attacks (lbl_804... 0.5497787).
const float kTurnAngle = 0.5497787f;

// The 2D angle between this actor's forward vector and the direction to `target`.
float AngleToTarget(const CActor& self, const CActor& target) {
  const CVector3f d = target.GetTranslation() - self.GetTranslation();
  const CVector3f fwd = self.GetTransform().GetForward();
  return CVector2f::GetAngleDiff(CVector2f(fwd.GetX(), fwd.GetY()), CVector2f(d.GetX(), d.GetY()));
}

float DistSq(const CVector3f& a, const CVector3f& b) {
  const CVector3f d = a - b;
  return d.GetX() * d.GetX() + d.GetY() * d.GetY() + d.GetZ() * d.GetZ();
}

} // namespace

// @ fn_15_60C8
bool CElitePirate::Alerted(CStateManager&, const CTriggerData&) const { return mAlerted2; }

// @ fn_15_18CC
bool CElitePirate::AngryAttackOver(CStateManager&, const CTriggerData&) const {
  return xd24_40_;
}

// @ fn_15_6F28
bool CElitePirate::AttackPatternOver(CStateManager&, const CTriggerData&) const {
  return mWaypointNavigation.GetDestination() == kInvalidUniqueId;
}

// @ fn_15_50B4
bool CElitePirate::BreakProjectileAttack(CStateManager&, const CTriggerData&) const {
  return mRocketEventSeen;
}

// @ fn_15_5084
bool CElitePirate::CanShockwave(CStateManager& mgr, const CTriggerData&) const {
  return GetPlayerHintType(mgr) != kHintJumpDown;
}

// @ fn_15_4E90
// A static line of sight from the launcher's turret to the player, when the player is within
// 1/sqrt(2) of the shock wave range in the XY plane.
bool CElitePirate::ClearLineOfSight(CStateManager& mgr, const CTriggerData&) const {
  const CElitePirateGrenadeLauncher* launcher =
      static_cast< const CElitePirateGrenadeLauncher* >(mgr.GetObjectById(mLauncherId));
  if (launcher == nullptr) {
    return false;
  }
  const CVector3f start = launcher->GetTurretTransform().GetTranslation();
  CVector3f d = mgr.GetPlayer(0)->GetAimPosition(mgr, 0.f) - start;
  if (!d.CanBeNormalized()) {
    return false;
  }
  if (d.GetX() * d.GetX() + d.GetY() * d.GetY() >
      mData.mMaxShockwaveRange * mData.mMaxShockwaveRange * 0.5f) {
    return false;
  }
  const CMaterialFilter filter =
      CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList(kMT_Floor));
  const float length = d.Magnitude();
  d.Normalize();
  return CGameCollision::RayStaticLineOfSightTest(
      mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()), start, d, length, filter);
}

// @ fn_15_691C
bool CElitePirate::DonePursuing(CStateManager& mgr, const CTriggerData& data) const {
  if (xa40_ > 0.5f) {
    return true;
  }
  const int hint = GetPlayerHintType(mgr);
  if (hint == kHintNoAttack) {
    return true;
  }
  if (hint == kHintJumpDown) {
    if (PathShagged(mgr, data) || mPathFindSearch.IsOver()) {
      return true;
    }
  }
  const CPlayer* player = mgr.GetPlayer(0);
  if (mLastPushTime <= xa44_) {
    const CVector3f d = player->GetAimPosition(mgr, 0.f) - GetTranslation();
    if (mData.mMaxMeleeRange * mData.mMaxMeleeRange * 4.f <= DistSq(d, CVector3f::Zero())) {
      if (mTime < xa44_ + 2.5f) {
        if (ClearLineOfSight(mgr, data) || IsOnJumpHint(mgr)) {
          return true;
        }
      }
      return mPredictedLeashTime > 3.f;
    }
    return true;
  }
  if (GetCurrentAreaId() != player->GetCurrentAreaId()) {
    return true;
  }
  if (hint != kHintJumpDown) {
    const CVector3f d = player->GetAimPosition(mgr, 0.f) - GetTranslation();
    if (DistSq(d, CVector3f::Zero()) < mData.mMaxShockwaveRange * mData.mMaxShockwaveRange) {
      return true;
    }
  }
  if (IsOnJumpHint(mgr)) {
    return true;
  }
  return ClearLineOfSight(mgr, data);
}

// @ fn_15_6C64
bool CElitePirate::DoneTurning(CStateManager& mgr, const CTriggerData& data) const {
  return AnimOver(mgr, data);
}

// @ fn_15_6F44
bool CElitePirate::HasAttackPattern(CStateManager& mgr, const CTriggerData&) const {
  return GetConnectedObject(mgr, kSS_Attack, kSM_Follow) != kInvalidUniqueId;
}

// @ fn_15_6F9C
bool CElitePirate::InDetectionRange(CStateManager& mgr, const CTriggerData& data) const {
  return mAlerted ? true : CPatterned::InDetectionRange(mgr, data);
}

// @ fn_15_6368
bool CElitePirate::InPosition(CStateManager&, const CTriggerData&) const {
  return DistSq(xb3c_, GetTranslation()) < 25.f;
}

// @ fn_15_6D94
bool CElitePirate::NotReachedTarget(CStateManager&, const CTriggerData&) const {
  if ((xc5c_ + mTime) * 0.5f < mLastPushTime) {
    return true;
  }
  return xc5c_ + 15.f < mTime;
}

// @ fn_15_4E7C
bool CElitePirate::PickedSpreadShot(CStateManager&, const CTriggerData&) const {
  return mRocketPattern == 1;
}

// @ fn_15_15CC
bool CElitePirate::PlayerInNoAttack(CStateManager& mgr, const CTriggerData&) const {
  return GetPlayerHintType(mgr) == kHintNoAttack;
}

// @ fn_15_61CC
bool CElitePirate::PoweredDown(CStateManager&, const CTriggerData&) const {
  return mInitialAnimState != 1;
}

// @ fn_15_61C0
bool CElitePirate::PoweredUp(CStateManager&, const CTriggerData&) const { return mEvent7; }

// @ fn_15_60D4
// [?] retail's virtual call at vt+0x194 is the ShieldKilled slot (fn_15_6D68); the HP/knock-back
// ratio is GetHealthInfo()'s two floats read as {first, second}.
bool CElitePirate::ReadyToCharge(CStateManager& mgr, const CTriggerData& data) const {
  if (GetPlayerHintType(mgr) == kHintNoAttack) {
    if (!ShieldKilled(mgr, data)) {
      return false;
    }
  }
  const CHealthInfo* health = GetHealthInfo();
  return xc5c_ + (health->GetKnockBackResistance() / health->GetHP()) * 1.7f + 0.3f < mTime;
}

// @ fn_15_60BC
bool CElitePirate::ReturnedToPatrol(CStateManager&, const CTriggerData&) const {
  return xc10_04_;
}

// @ fn_15_6D68
bool CElitePirate::ShieldKilled(CStateManager&, const CTriggerData&) const {
  const float limit =
      mShieldPolarity == 1 ? mData.mUnknown5236C2B6 : mData.mUnknown01EAAB17;
  return limit < mShieldDamage;
}

// @ fn_15_18AC
bool CElitePirate::ShockwaveIsNext(CStateManager&, const CTriggerData&) const {
  return xd24_80_;
}

// @ fn_15_63B0
bool CElitePirate::ShotAt(CStateManager&, const CTriggerData&) const { return mXDamaged; }

// @ fn_15_684C
// [?] x81c_ (zeroed by the ctor, never written in this slice) reads as an "alert here" position.
bool CElitePirate::ShouldAlert(CStateManager&, const CTriggerData&) const {
  if (xc10_01_) {
    return false;
  }
  if (x81c_ != CVector3f::Zero() && DistSq(x81c_, GetTranslation()) <= 25.f) {
    return false;
  }
  return true;
}

// @ fn_15_6820
// @ fn_15_6618
// The launcher can fire: no cooldown, a target in the rocket range band, the body already turned
// to it, and (retail's RayCollideWorld result) a clear shot from the launcher's lock-on locator.
bool CElitePirate::ShouldFire(CStateManager& mgr, const CTriggerData& data) const {
  if (mTimer > 0.f || mLauncherId == kInvalidUniqueId) {
    return false;
  }
  const CElitePirateGrenadeLauncher* launcher =
      static_cast< const CElitePirateGrenadeLauncher* >(mgr.GetObjectById(mLauncherId));
  if (launcher == nullptr) {
    return false;
  }
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(xa26_));
  if (target == nullptr) {
    return false;
  }
  const CVector3f aim = target->GetAimPosition(mgr, 0.f);
  const float distSq = DistSq(aim, GetTranslation());
  if (!(mData.mMinRocketRange * mData.mMinRocketRange < distSq &&
        distSq < mData.mMaxRocketRange * mData.mMaxRocketRange)) {
    return false;
  }
  if (ShouldTurn(mgr, data)) {
    return false;
  }
  // fn_15_2284: the launcher's "lockon_target_LCTR" locator in world space.
  const CVector3f lockon = launcher->GetTransform().Rotate(
                               launcher->GetLocatorTransform("lockon_target_LCTR").GetTranslation()) +
                           launcher->GetTranslation();
  const CMaterialFilter filter =
      CMaterialFilter::MakeInclude(CMaterialList(kMT_Solid, kMT_Character));
  return mgr.RayCollideWorld(lockon, aim, filter, this);
}

// @ fn_15_6500
bool CElitePirate::ShouldMeleeAttack(CStateManager& mgr, const CTriggerData&) const {
  if (GetPlayerHintType(mgr) != -1) {
    return false;
  }
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(xa26_));
  if (target == nullptr ||
      DistSq(target->GetTranslation(), GetTranslation()) >
          mData.mMaxMeleeRange * mData.mMaxMeleeRange) {
    return false;
  }
  return AngleToTarget(*this, *target) < kTurnAngle;
}

// @ fn_15_63BC
bool CElitePirate::ShouldShockwave(CStateManager& mgr, const CTriggerData&) const {
  if (mTimer > 0.f) {
    return false;
  }
  for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
    const CPlayer* player = mgr.GetPlayer(i);
    if (GetCurrentAreaId() != player->GetCurrentAreaId()) {
      continue;
    }
    const CVector3f aim = player->GetAimPosition(mgr, 0.f);
    const float distSq = DistSq(aim, GetTranslation());
    if (distSq > mData.mMaxShockwaveRange * mData.mMaxShockwaveRange) {
      continue;
    }
    const bool morphed =
        player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed &&
        player->GetMorphballTransitionState() == CPlayer::kMS_Morphed;
    if (morphed) {
      return true;
    }
    if (mData.mMinShockwaveRange * mData.mMinShockwaveRange <= distSq) {
      return fabsf(aim.GetZ() - GetTranslation().GetZ()) < 3.f;
    }
  }
  return false;
}

// @ fn_15_6C84
bool CElitePirate::ShouldTurn(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(xa26_));
  return target != nullptr && AngleToTarget(*this, *target) > kTurnAngle;
}

// @ fn_15_6FD0
bool CElitePirate::SpotPlayer(CStateManager& mgr, const CTriggerData& data) const {
  return mAlerted ? true : CPatterned::SpotPlayer(mgr, data);
}

// @ fn_15_18B8
bool CElitePirate::StillAngry(CStateManager&, const CTriggerData&) const {
  return mAngryCount > 0;
}

// @ fn_15_6240
bool CElitePirate::TargetNotOnMesh(CStateManager& mgr, const CTriggerData&) const {
  const int hint = GetPlayerHintType(mgr);
  if (hint == kHintNoAttack) {
    return false;
  }
  if (hint == kHintJumpDown) {
    return true;
  }
  return mPathFindSearch.NearlyOnPath(mgr.GetPlayer(0)->GetTranslation(), 4.f) !=
         CPathFindSearch::kR_Success;
}

// @ fn_15_61E4
bool CElitePirate::TargetUnreachable(CStateManager& mgr, const CTriggerData&) const {
  const int hint = GetPlayerHintType(mgr);
  if (hint == kHintJumpDown || hint == kHintNoAttack) {
    return true;
  }
  return xa40_ > 1.f;
}

// @ fn_15_62CC
bool CElitePirate::TooClose(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(xa26_));
  return target != nullptr &&
         DistSq(GetTranslation(), target->GetTranslation()) <
             mData.mMaxMeleeRange * mData.mMaxMeleeRange * 0.5f;
}

void CElitePirate::Alert(CStateManager&, EStateMsg, float) { StandIn("Alert (fn_15_5340)"); }

void CElitePirate::AngryAttackBegin(CStateManager&, EStateMsg, float) { StandIn("AngryAttackBegin (fn_15_1900)"); }

void CElitePirate::AngryAttackEnd(CStateManager&, EStateMsg, float) { StandIn("AngryAttackEnd (fn_15_18D8)"); }

void CElitePirate::Dead(CStateManager&, EStateMsg, float) { StandIn("Dead (fn_15_7B00)"); }

void CElitePirate::FollowAttackPattern(CStateManager&, EStateMsg, float) { StandIn("FollowAttackPattern (fn_15_6DDC)"); }

void CElitePirate::InvulnAlert(CStateManager&, EStateMsg, float) { StandIn("InvulnAlert (fn_15_52D0)"); }

void CElitePirate::MeleeAttack(CStateManager&, EStateMsg, float) { StandIn("MeleeAttack (fn_15_48BC)"); }

void CElitePirate::Patrol(CStateManager&, EStateMsg, float) { StandIn("Patrol (fn_15_5708)"); }

void CElitePirate::PowerDown(CStateManager&, EStateMsg, float) { StandIn("PowerDown (fn_15_5FF0)"); }

void CElitePirate::PowerUp(CStateManager&, EStateMsg, float) { StandIn("PowerUp (fn_15_5F08)"); }

void CElitePirate::ProjectileAttack(CStateManager&, EStateMsg, float) { StandIn("ProjectileAttack (fn_15_50BC)"); }

void CElitePirate::Pursue(CStateManager&, EStateMsg, float) { StandIn("Pursue (fn_15_5BB4)"); }

void CElitePirate::Shielding(CStateManager&, EStateMsg, float) { StandIn("Shielding (fn_15_41A0)"); }

void CElitePirate::ShieldUp(CStateManager&, EStateMsg, float) { StandIn("ShieldUp (fn_15_44A8)"); }

void CElitePirate::Shockwave(CStateManager&, EStateMsg, float) { StandIn("Shockwave (fn_15_4AE0)"); }

void CElitePirate::SpreadShot(CStateManager&, EStateMsg, float) { StandIn("SpreadShot (fn_15_4D18)"); }

void CElitePirate::Stunned(CStateManager&, EStateMsg, float) { StandIn("Stunned (fn_15_59BC)"); }

void CElitePirate::TargetPatrol(CStateManager&, EStateMsg, float) { StandIn("TargetPatrol (fn_15_578C)"); }

void CElitePirate::Taunt(CStateManager&, EStateMsg, float) { StandIn("Taunt (fn_15_5A60)"); }

void CElitePirate::Turn(CStateManager&, EStateMsg, float) { StandIn("Turn (fn_15_5598)"); }

void CElitePirate::Wait(CStateManager&, EStateMsg, float) { StandIn("Wait (fn_15_6064)"); }

// @ fn_15_555C
void CElitePirate::SelectTarget(CStateManager& mgr, float) {
  xa26_ = CScriptTeamAiMgr::ChoosePlayer(mgr, *this);
}

// @ fn_15_54AC
// [?] xd04_ (= 1 from the ctor) is the pattern repeated 20% of the time, otherwise the other one.
void CElitePirate::PickAttackType(CStateManager& mgr, float) {
  if (!mgr.IsRandomAvailable()) {
    return;
  }
  if (GetPlayerHintType(mgr) == kHintJumpDown) {
    mRocketPattern = 1;
  } else if (mgr.Random()->Range(0.f, 1.f) >= 0.8f) {
    mRocketPattern = xd04_;
  } else {
    mRocketPattern = xd04_ == 0 ? 1 : 0;
  }
}
