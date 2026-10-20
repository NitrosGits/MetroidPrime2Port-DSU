// The Ing half of the Boost Guardian's state machine: the triggers, the code functions and the
// shared helpers the states use. The behaviour is the Ing's, ported from the Ing host body (branch
// decomp/ing, src/MetroidPrime/Enemies/PortCIng.cpp).
//
// This module (IngBoostBallGuardian) carries its own copy of the Ing code, so every `fn_29_`
// address in the comments below is the Ing REL's address, not this module's; the module's own
// addresses are in docs/module-analysis/BoostGuardian_fsm.tsv.

#include "MetroidPrime/Enemies/CIngBoostBallGuardian.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Particles/CGenDescription.hpp"
#include "Kyoto/Text/CStringTable.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CAxisAngle.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CGameLight.hpp"
#include "MetroidPrime/Enemies/CCollisionTracker.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/Weapons/CWeapon.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CGenericFSM2.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Enemies/CTeamAiRole.hpp"
#include "MetroidPrime/Enemies/PortBoostGuardianShared.hpp"
#include "MetroidPrime/HUD/CHUDMemoParms.hpp"
#include "MetroidPrime/HUD/CSamusHud.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/Player/CEnvironmentVariable.hpp"
#include "MetroidPrime/Player/CGameState.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrIng.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CHUDBillboardEffect.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIHint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSpecialFunction.hpp"
#include "MetroidPrime/Enemies/PortCBoostBallGuardianBall.hpp"

#include <stdio.h>
#include <stdlib.h>

namespace {

// The Ing's file-local solid filter (the damage filter that only takes solid material) is shared
// with the other files of this module: PortBoostGuardianShared.hpp has SolidFilter().

} // namespace

// ---- the collision actors --------------------------------------------------------------------

// fn_30_4E90: the body manager (guest 0xD60), the seven joint spheres of lbl_30_rodata_0, built
// in kSM_Create (0x0000DF18) before the ing-spot manager; the Ing REL's fn_29_3190 with two more
// materials (0x00005018: kMT_Unknown54 and kMT_ProjectilePassthrough) and two more exclusions in
// the filter (lbl_30_data_9A0..9B0: CollisionActor, AIPassthrough, Player, Projectile, Bomb).
// @ fn_30_4E90
void CIngBoostBallGuardian::CreateCollisionActorManager(CStateManager& mgr) {
  rstl::vector< CJointCollisionDescription > joints;
  joints.reserve(7);
  const CAnimData* anim = GetModelData()->GetAnimationData();
  for (uint i = 0; i < 7; ++i) {
    const SIngJointSphere& sphere = skIngJointSpheres[i];
    const CSegId seg = anim->GetLocatorSegId(rstl::string_l(sphere.mLocator));
    joints.push_back_unsafe(CJointCollisionDescription::SphereCollision(
        seg, CVector3f::Zero(), sphere.mRadius, rstl::string_l(sphere.mLocator), kIngJointMass));
  }
  CCollisionActorManager* manager =
      rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), joints, false);
  delete mCollisionActorManager0;
  mCollisionActorManager0 = manager;
  SetupManagerActors(mgr, mCollisionActorManager0);
  AddMaterial(kMT_Unknown54, kMT_ProjectilePassthrough, mgr);
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid), // lbl_30_data_99C
      CMaterialList(kMT_CollisionActor, kMT_AIPassthrough, kMT_Player, kMT_Projectile, kMT_Bomb)));
}

// fn_30_4A38: the ing-spot manager (guest 0xD64), built in kSM_Create after the body manager. The
// vector helpers are rstl inlines: fn_30_10F9C reserve, fn_30_4BD8 push_back_unsafe, fn_30_4C58
// the description's copy, fn_30_4D74 the vector's destructor.
// @ fn_30_4A38
void CIngBoostBallGuardian::CreateIngSpotManager(CStateManager& mgr) {
  rstl::vector< CJointCollisionDescription > joints;
  joints.reserve(1);
  const CSegId root = GetModelData()->GetAnimationData()->GetLocatorSegId(rstl::string_l("Root"));
  joints.push_back_unsafe(CJointCollisionDescription::AABoxCollision(
      root, CVector3f(5.f, 5.f, 1.5f), rstl::string_l("PuddleCollisionActor"), 1000.f));
  CCollisionActorManager* manager =
      rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), joints, false);
  delete mCollisionActorManager1;
  mCollisionActorManager1 = manager;
  SetupManagerActors(mgr, mCollisionActorManager1);
}

// fn_30_4924: the Ing REL's fn_29_3014 for a given manager; the vulnerability call is the
// non-virtual CPatterned one.
// @ fn_30_4924
void CIngBoostBallGuardian::SetupManagerActors(CStateManager& mgr,
                                               CCollisionActorManager* const& manager) {
  for (uint i = 0; i < manager->GetNumCollisionActors(); ++i) {
    const TUniqueId id = manager->GetCollisionDescFromIndex(i).GetCollisionActorId();
    if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(id))) {
      const CHealthInfo health = *GetHealthInfo();
      actor->SetDamageVulnerability(*CPatterned::GetDamageVulnerability());
      *actor->HealthInfo() = health;
    }
  }
}

// @ fn_30_648C
TUniqueId CIngBoostBallGuardian::CreateBlobTracker(CStateManager& mgr,
                                                   const TLockedToken< CGenDescription >& desc) {
  const TUniqueId id = mgr.AllocateUniqueId();
  if (id != kInvalidUniqueId) {
    CCollisionTracker* tracker = rs_new CCollisionTracker(
        desc, id, GetCurrentAreaId(), true, rstl::string_l("IngBlobEffect"),
        CTransform4f::Translate(GetTranslation()), GetUniqueId(), 0,
        CCollisionTracker::skDefaultExtents);
    tracker->SetParticleEmissionRateScalar(0.f);
    tracker->SetNextDrawNode(GetUniqueId()); // 0x000065A4: CActor 0x12A
    mgr.AddObject(tracker);
  }
  return id;
}

// @ fn_30_67B4
void CIngBoostBallGuardian::CreateHitLight(CStateManager& mgr) {
  const TUniqueId id = mgr.AllocateUniqueId();
  CGameLight* light = rs_new CGameLight(
      id, GetAreaIdForPersistence(), GetActive(), rstl::string_l("Ing Light"),
      CTransform4f::Identity(), GetUniqueId(), CLight::BuildPoint(CVector3f::Zero(), CColor::Black()),
      0, 0, 0.f);
  if (light != nullptr) {
    mHitLightId = id;
    mgr.AddObject(light);
  }
}

// @ fn_30_510C
void CIngBoostBallGuardian::LeaveTeam(CStateManager& mgr) {
  if (mTeamId == kInvalidUniqueId) { // guest 0x1086
    return;
  }
  if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamId))) {
    if (team->IsPartOfTeam(GetUniqueId())) {
      team->QuitTeam(GetUniqueId());
      mTeamId = kInvalidUniqueId;
    }
  }
}

// fn_30_30E4: every collision actor of the guest's second manager (0xD64) takes the boost-ball
// vulnerability (0x1178) or the ing spot's (0xA5C) (0x0000310C).
void CIngBoostBallGuardian::SetIngSpotManagerVulnerability(CStateManager& mgr, bool boost) {
  const CDamageVulnerability& vuln = boost ? mVulnerability : mData.mIngSpotVulnerability;
  for (uint i = 0; i < mCollisionActorManager1->GetNumCollisionActors(); ++i) {
    const TUniqueId id =
        mCollisionActorManager1->GetCollisionDescFromIndex(i).GetCollisionActorId();
    if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(id))) {
      actor->SetDamageVulnerability(vuln); // 0x00003158
    }
  }
}

// fn_30_3190: records the reflect flag, then every collision actor of the body manager (guest
// 0xD60) takes the reflect vulnerability or CPatterned's own
// (0x000031C8 / 0x000031D0, the non-virtual call).
void CIngBoostBallGuardian::SetBodyManagerReflect(CStateManager& mgr, bool reflect) {
  mReflectVulnerability = reflect; // 0x000031BC: 0x11EE 0x02
  const CDamageVulnerability& vuln = reflect ? CDamageVulnerability::ReflectVulnerabilty()
                                             : *CPatterned::GetDamageVulnerability();
  for (uint i = 0; i < mCollisionActorManager0->GetNumCollisionActors(); ++i) {
    const TUniqueId id =
        mCollisionActorManager0->GetCollisionDescFromIndex(i).GetCollisionActorId();
    if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(id))) {
      actor->SetDamageVulnerability(vuln); // 0x00003214
    }
  }
}

// fn_30_3068: entering (when allowed) raises the boost, the puddle form and the puddle generate
// bits and zeroes 0x11D4; anything else leaves the puddle form, zeroes 0x11D8/0x11DC and reloads
// the puddle mode time from the current phase.
void CIngBoostBallGuardian::SetBoostPuddleMode(bool enter) {
  if (enter && mBoostPuddleAllowed) { // 0x00003068; 0x00003074: 0x11EF 0x20
    mShouldBoost = true;              // 0x00003084: 0x11EE 0x10 = 1
    mBoostPuddleForm = true;          // 0x00003098: 0x11EE 0x08 = 1
    mBoostPuddleGenerate = true;      // 0x000030A4: 0x11EE 0x04 = 1
    mBoost11D4 = 0.f;                 // 0x000030AC
    return;
  }
  mBoostPuddleForm = false; // 0x000030C0: 0x11EE 0x08 = 0
  mBoost11D8 = 0.f;         // 0x000030CC
  mBoostTimer = 0.f;        // 0x000030D0: 0x11DC
  mBoostPuddleModeTimer = CurrentPhase().mBoostPuddleModeTime; // 0x000030DC: *(0x11D0)+0
}

// Ing body fn_29_2ED8 (kSM_HitObject): when a collision actor that last touched the player reports a hit
// and the contact damage is off cooldown, the player takes the Ing's contact damage.
void CIngBoostBallGuardian::CollisionActorHitPlayer(CStateManager& mgr, TUniqueId sender) {
  const CCollisionActor* actor = TCastToConstPtr< CCollisionActor >(mgr.ObjectById(sender));
  if (actor == nullptr) {
    return;
  }
  const CPlayer* player = mgr.GetPlayer(0);
  if (player == nullptr || actor->GetLastTouchedObject() != player->GetUniqueId()) {
    return;
  }
  // xE5C_1 (0xe5c bit 1) suppresses the contact damage; fn_29_6A88 owns that bit.
  if (mCurDamageRemTime > 0.f || xE5C_1) {
    return;
  }
  mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(), GetContactDamage(),
                  SolidFilter(), CVector3f::Zero());
  mCurDamageRemTime = mDamageWaitTime;
}

// @ fn_30_3CB0
void CIngBoostBallGuardian::SpawnSpotHitEffect(CStateManager& mgr, float damage) {
  // The ing-spot effects while the ing blob tracker (0x1074) has live particles, else the boost's.
  bool emitting = false;
  CEntity* ent = mgr.ObjectById(mIngBlobTrackerId);
  if (ent != nullptr) {
    CCollisionTracker* tracker = static_cast< CCollisionTracker* >(ent);
    if (tracker->GetParticleSystem() != nullptr &&
        tracker->GetParticleSystem()->GetParticleCount() > 0) {
      emitting = true;
    }
  }
  CEntity* explosion = nullptr;
  const bool heavy = !(damage < 25.f); // 0x00003D40: rodata_BC
  const rstl::optional_object< TLockedToken< CGenDescription > >& effect =
      heavy ? (emitting ? mHeavyHitEffect : mBoostHeavyHitEffect)
            : (emitting ? mNormalHitEffect : mBoostNormalHitEffect);
  if (effect) {
    explosion = rs_new CExplosion(*effect, mgr.AllocateUniqueId(),
                                  CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true,
                                              kInvalidEditorId),
                                  rstl::string_l("Ing Damage Fx"), GetTransform(), 0,
                                  GetModelData()->GetScale(), CColor::White(), -1);
  }
  CSfxManager::AddEmitter(heavy ? mData.mIngSpot.GetHeavyHitSound()
                                : mData.mIngSpot.GetNormalHitSound(), // guest 0x7F8 / 0x7F4
                          GetTranslation(), 0x7f, GetCurrentAreaId().Value(), true, false,
                          CSfxManager::kMedPriority);
  if (explosion != nullptr) {
    mgr.AddObject(explosion);
  }
}

// @ fn_30_4030
void CIngBoostBallGuardian::AccumulatePuddleDamage(CStateManager& mgr, float hp, float damage) {
  mBoost11D4 += damage;
  if (mBoost11D4 >= hp * mBoost.mUnknownB0E8) { // guest 0xEC0
    SetBoostPuddleMode(true);
  }
}

// @ fn_30_4078
void CIngBoostBallGuardian::BodyColliderDamage(CStateManager& mgr, TUniqueId sender) {
  if (!mAlive) { // CPatterned 0x420 bit 6
    return;
  }
  CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(sender));
  if (actor == nullptr) {
    return;
  }
  if (mCollisionActorManager0->GetCollisionDescIndexFromUniqueId(sender) == -1) { // guest 0xD60
    return;
  }
  const TUniqueId touched = actor->GetLastTouchedObject();
  CHealthInfo* actorHealth = actor->HealthInfo();
  CHealthInfo* health = HealthInfo();
  const float initial = health->GetInitialHP();
  if (const CWeapon* weapon = TCastToConstPtr< CWeapon >(mgr.GetObjectById(touched))) {
    const float damage = initial - actorHealth->GetHP();
    const float hp = health->GetHP();
    if (!mBoostPuddleForm) {
      AccumulatePuddleDamage(mgr, initial, damage);
    }
    // A hit that would kill leaves 1 HP and ends puddle mode for good, except while hits are
    // lethal (outside the body-projectile form).
    bool survive = false;
    if ((mForm == kForm_BodyProjectile || !mBodyHitsLethal) && hp >= 0.f && hp <= damage) {
      survive = true;
    }
    if (survive) {
      health->SetHP(1.f);
      SetBoostPuddleMode(false);
      mBoostPuddleAllowed = false; // 0x00004204: 0x11EF 0x20 = 0
    } else {
      health->SetHP(hp - damage);
    }
    const CVector3f direction = weapon->GetTransform().GetForward();
    TakeDamage(direction, damage); // 0xa4
    if (hp <= damage && !survive) {
      Death(mgr, direction, kSS_DeathRattle); // 0x98, 0x5241544C
      mgr.RecordDamageSource(*this, weapon->GetUniqueId(), weapon->GetCurrentDamageInfo(), true,
                             false);
    }
    KnockBack(mgr, CKnockBackInfo(direction, touched, weapon->GetOwnerId(),
                                  weapon->GetCurrentDamageInfo(), true)); // 0x9c
  }
  actorHealth->SetHP(initial);
}

// @ fn_30_4324
void CIngBoostBallGuardian::PuddleColliderResistedDamage(CStateManager& mgr, TUniqueId sender) {
  if (!mAlive) {
    return;
  }
  CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(sender));
  if (actor == nullptr) {
    return;
  }
  if (mCollisionActorManager1->GetCollisionDescIndexFromUniqueId(sender) == -1) { // guest 0xD64
    return;
  }
  const CWeapon* weapon = TCastToConstPtr< CWeapon >(mgr.GetObjectById(actor->GetLastTouchedObject()));
  if (weapon == nullptr || weapon->GetType() != kWT_Bomb) {
    return;
  }
  mBoostHitTimer = CPatterned::skDamageHitTime; // 0x000043E4: 0x1080
  mHitByPlayerProjectile = true;                // 0x000043F0: 0x420 bit 7
  if (DebugBg()) { // host trace
    fprintf(stderr, "[ibbg] %04x puddle resisted bomb %.1f: puddle form %d boost over %d 11D8 %.1f\n",
            GetUniqueId().Value(), weapon->GetCurrentDamageInfo().GetDamage(),
            int(mCurrentlyInBoostPuddleForm), int(mBoostOver), mBoost11D8);
  }
  if (!mCurrentlyInBoostPuddleForm || !mBoostOver) {
    return;
  }
  mBoost11D8 += weapon->GetCurrentDamageInfo().GetDamage();
  mBoostTimer += weapon->GetCurrentDamageInfo().GetDamage(); // guest 0x11DC
  const CIngBoostBallGuardianPhase& phase = CurrentPhase();
  if (mBoost11D8 >= phase.mBreakBoostPuddleModeDamage) {
    SetBoostPuddleMode(false);
  } else if (mBoostTimer > phase.mUnknownABE9 * phase.mBreakBoostPuddleModeDamage) {
    mShouldBoost = true; // 0x0000447C: 0x11EE 0x10 = 1
    mBoostTimer = 0.f;
  }
}

// @ fn_30_44A4
void CIngBoostBallGuardian::PuddleColliderDamage(CStateManager& mgr, TUniqueId sender) {
  if (!mAlive) {
    return;
  }
  CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(sender));
  if (actor != nullptr &&
      mCollisionActorManager1->GetCollisionDescIndexFromUniqueId(sender) != -1) { // guest 0xD64
    const TUniqueId touched = actor->GetLastTouchedObject();
    CHealthInfo* actorHealth = actor->HealthInfo();
    CHealthInfo* health = HealthInfo();
    const float initial = health->GetInitialHP();
    if (const CWeapon* weapon = TCastToConstPtr< CWeapon >(mgr.GetObjectById(touched))) {
      const float damage = initial - actorHealth->GetHP();
      if (DebugBg()) { // host trace
        const CWeaponTypeVulnerability rate =
            actor->GetDamageVulnerability()->GetVulnerability(weapon->GetCurrentDamageInfo().GetWeaponMode());
        fprintf(stderr,
                "[ibbg] %04x puddle damage %.1f (weapon type %d, x%.2f effect %d), hp %.1f, boost "
                "puddle form %d\n",
                GetUniqueId().Value(), damage, int(weapon->GetType()), rate.mDamageMultiplier,
                int(rate.mEffect), health->GetHP(), int(mCurrentlyInBoostPuddleForm));
      }
      const float hp = health->GetHP();
      AccumulatePuddleDamage(mgr, GetHealthInfo()->GetHP(), damage);
      // The puddle never kills: a lethal hit leaves 1 HP and ends puddle mode for good.
      if (hp >= 0.f && hp <= damage) {
        health->SetHP(1.f);
        SetBoostPuddleMode(false);
        mBoostPuddleAllowed = false; // 0x00004610: 0x11EF 0x20 = 0
      } else {
        health->SetHP(hp - damage);
      }
      const CVector3f direction = weapon->GetTransform().GetForward();
      TakeDamage(direction, damage); // 0xa4
      KnockBack(mgr, CKnockBackInfo(direction, touched, weapon->GetOwnerId(),
                                    weapon->GetCurrentDamageInfo(), true)); // 0x9c
      SpawnSpotHitEffect(mgr, damage); // fn_30_3CB0
    }
    actorHealth->SetHP(initial);
    return;
  }
  // 0x000046B8: the boost ball relays what hit it (its 0x4F6, fn_30_13B7C).
  CEntity* ent = mgr.ObjectById(sender);
  CBoostBallGuardianBall* ball =
      ent != nullptr
          ? static_cast< CBoostBallGuardianBall* >(ent->TypesMatch(CBoostBallGuardianBall::kTypeId))
          : nullptr; // fn_800979F0
  if (ball == nullptr) {
    return;
  }
  const CEntity* hitter = mgr.GetObjectById(ball->GetLastSender());
  if (TCastToConstPtr< CScriptSpecialFunction >(hitter)) {
    mCollidedWithCrystal = true; // 0x00004714: 0x11EE 0x01
  } else if (const CWeapon* weapon = TCastToConstPtr< CWeapon >(hitter)) {
    if (weapon->GetType() == kWT_Bomb) {
      mBoost11E0 += weapon->GetCurrentDamageInfo().GetDamage();
      if (mBoost11E0 >= CurrentPhase().mBreakBoostModeDamage) {
        mCollidedWithCrystal = true; // 0x0000476C
      }
    }
  }
}

// @ fn_30_479C
void CIngBoostBallGuardian::ColliderDamage(CStateManager& mgr, TUniqueId sender) {
  if (mForm == kForm_IngSpot) {
    PuddleColliderDamage(mgr, sender);
  } else {
    BodyColliderDamage(mgr, sender);
  }
}

// ---- helpers ----------------------------------------------------------------------------------

void CIngBoostBallGuardian::LogState(const char* name) {
  mStateTime = 0.f;
  if (DebugBg() && mLastLoggedState != name) {
    mLastLoggedState = name;
    fprintf(stderr, "[ibbg] %04x state %s form %d\n", GetUniqueId().Value(), name, mForm);
  }
}

void CIngBoostBallGuardian::JoinTeam(CStateManager& mgr) {
  if (mTeamId == kInvalidUniqueId) {
    mTeamId = CScriptTeamAiMgr::GetAssociatedTeamId(*this, mgr);
  }
  if (mTeamId != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamId))) {
      // [?] the roles are MinorIng's (fn_44_593C).
      team->JoinTeam(*this, CTeamAiRole::kTAR_Projectile, CTeamAiRole::kTAR_Melee,
                     CTeamAiRole::kTAR_Invalid);
    }
  }
}

// The DOL's surface move shared with MinorIng (fn_44_5384): drop the part of the move along the
// surface normal, step, and keep the step on the path mesh unless following a point path.
void CIngBoostBallGuardian::MoveOnSurface(const CVector3f& moveIn, float speed, float dt) {
  const CVector3f pos = GetTranslation();
  const CVector3f normal = mSurfaceAlignment.GetSurface().GetNormal();
  CVector3f move = moveIn;
  const float along = CVector3f::Dot(move, normal);
  if (along * along < move.MagSquared() * 0.6f) {
    move = move - normal * along;
  }
  if (!move.IsMagnitudeSafe()) {
    mSurfaceMoveDir = CVector3f::Zero();
  } else {
    const CVector3f dir = move.AsNormalized();
    SetTranslation(pos + dir * (speed * dt));
    mSurfaceMoveDir = dir;
  }
  Stop();
}

void CIngBoostBallGuardian::ApplySeparation(CStateManager& mgr) {
  CVector3f total = CVector3f::Zero();
  const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CPatterned* other = TCastToConstPtr< CPatterned >(list[i]);
    if (other != nullptr && other != this && other->GetCurrentAreaId() == GetCurrentAreaId()) {
      const CVector2f separation = mSteeringBehaviors.Separation2D(
          *this, CVector2f(other->GetTranslation().GetX(), other->GetTranslation().GetY()),
          mModelScale.GetX() * 15.f);
      const CVector3f separation3(separation.GetX(), separation.GetY(), 0.f);
      if (separation3.IsMagnitudeSafe()) {
        total += separation3;
      }
    }
  }
  if (total != CVector3f::Zero()) {
    BodyController()->CommandMgr().DeliverCmd(CBCLocomotionCmd(total, CVector3f::Zero(), 0.5f));
  }
}

float CIngBoostBallGuardian::DistSqToTarget(const CStateManager& mgr) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return FLT_MAX;
  }
  return (target->GetTranslation() - GetTranslation()).MagSquared();
}

CScriptAIHint* CIngBoostBallGuardian::GetCoverHint(CStateManager& mgr, TUniqueId id) const {
  if (id == kInvalidUniqueId) {
    return nullptr;
  }
  return TCastToPtr< CScriptAIHint >(mgr.ObjectById(id));
}

void CIngBoostBallGuardian::ReleaseCoverHint(CStateManager& mgr, TUniqueId& id) {
  if (CScriptAIHint* hint = GetCoverHint(mgr, id)) {
    hint->SetInUse(false);
    hint->SetTimeRemaining(0.f);
  }
  id = kInvalidUniqueId;
}

// @ fn_30_6064
void CIngBoostBallGuardian::ReleaseOwnCoverHint(CStateManager& mgr) {
  if (CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId)) { // fn_30_60C0
    hint->SetInUse(false);
    hint->SetTimeRemaining(0.f); // 0x0000609C: hint 0x170
    mCoverHintId = kInvalidUniqueId;
  }
}

// @ fn_30_5D90: the module's own timers. The Ing's ball cooldown and body-projectile time are not
// in it; the boost countdowns are: in the boost puddle form (0x11EF 0x80, 0x11EE 0x08 and 0x20,
// ing spot) 0x11E8 runs down to ShouldBoost; otherwise, outside the ing spot form, 0x11E4 runs down
// (outside cinematics) to the boost puddle mode (fn_30_3068).
void CIngBoostBallGuardian::UpdateTimers(float dt, CStateManager& mgr) {
  mHeardShotTime += dt;
  mArmSwipeTime += dt;
  mFormTime += dt;
  mStateTime += dt; // [port] the host-only state clock
  if (mForm == kForm_Corporeal && PathShagged(mgr, CTriggerData(0.f))) {
    mFrustrationTimer += dt;
  } else {
    mFrustrationTimer = 0.f;
  }
  if (mForm == kForm_IngSpot && mCurDamageRemTime > 0.f) {
    mCurDamageRemTime -= dt;
  }
  if (mHitByPlayerProjectile) {
    mUnderFireTime = 0.f;
    mHitByPlayerProjectile = false;
  } else {
    mUnderFireTime += dt;
  }
  if (mCurrentlyInBoostPuddleForm && mBoostPuddleForm && mBoostOver && mForm == kForm_IngSpot) {
    if (mBoostRandomFloat > 0.f) {
      mBoostRandomFloat -= dt;
      if (mBoostRandomFloat <= 0.f) {
        mShouldBoost = true;
      }
    }
  } else if (!mBoostPuddleForm && mForm != kForm_IngSpot &&
             !mgr.GetCameraManager(0)->IsInCinematicCamera()) {
    mBoostPuddleModeTimer -= dt;
    if (mBoostPuddleModeTimer <= 0.f) {
      SetBoostPuddleMode(true);
    }
  }
}

void CIngBoostBallGuardian::UpdateValidTarget(CStateManager& mgr) {
  switch (mForm) {
  case 0:
  case 1:
    SetValidTarget(0, false);
    break;
  case kForm_IngSpot:
  case kForm_Evaporate:
    SetValidTarget(0, mgr.GetPlayerState(0)->GetActiveVisor(mgr) == CPlayerState::kPV_Dark ||
                          (mData.mFlags210 & kFlag_AlwaysTargetable) != 0); // 0x00005D58
    break;
  default:
    SetValidTarget(0, true);
    break;
  }
}

// [?] which loop plays in which form, and the emitter parameters: the idle loop in the ing-spot
// form while still, the move loop while moving on the surface.
void CIngBoostBallGuardian::UpdateLoopSfx() {
  const bool spot = mForm == kForm_IngSpot && GetActive();
  const bool moving = spot && mSurfaceMoveDir != CVector3f::Zero();
  const ushort ids[2] = {mData.mIngSpot.GetIdleSound(), mData.mIngSpot.GetMoveSound()};
  const bool want[2] = {spot && !moving, moving};
  for (int i = 0; i < 2; ++i) {
    if (want[i] && !mLoopSfx[i] && ids[i] != CSfxManager::kInternalInvalidSfxId) {
      mLoopSfx[i] = CSfxManager::AddEmitter(ids[i], GetTranslation(), GetCurrentAreaId().Value(),
                                            true, true);
    } else if (!want[i] && mLoopSfx[i]) {
      CSfxManager::SfxStop(mLoopSfx[i]);
      mLoopSfx[i] = CSfxHandle();
    }
  }
}

void CIngBoostBallGuardian::StopSfx() {
  for (int i = 0; i < 2; ++i) {
    if (mLoopSfx[i]) {
      CSfxManager::SfxStop(mLoopSfx[i]);
      mLoopSfx[i] = CSfxHandle();
    }
  }
}

// ---- triggers --------------------------------------------------------------------------------

// Ing body fn_29_B1F4
bool CIngBoostBallGuardian::StateOver(CStateManager&, const CTriggerData&) const {
  return mAnimationState.IsOver();
}

// fn_29_B1E8 (not yet exercised in a run)
bool CIngBoostBallGuardian::IsAlert(CStateManager&, const CTriggerData&) const { return mAlert; }

// fn_29_B1C8 (not yet exercised in a run). The asm tests mHostId (0xcdc) against kInvalidUniqueId -
// the lhz/subf/or/srwi-31 idiom - and nothing else; it does not look at the form. That is what lets
// SuckEnergy, which sets form 0 and then clears the host once its suck time runs out, take the
// !IsControllingHost trigger into SetExitHostDest and on into ExitHost.
bool CIngBoostBallGuardian::IsControllingHost(CStateManager&, const CTriggerData&) const {
  return mHostId != kInvalidUniqueId; // 0xcdc
}

// Ing body fn_29_B0E0
bool CIngBoostBallGuardian::IsFacingTarget(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return true;
  }
  CVector3f delta = target->GetTranslation() - GetTranslation();
  delta = CVector3f(delta.GetX(), delta.GetY(), 0.f);
  return CVector3f::GetAngleDiff(GetTransform().GetForward(), delta) <= kFacingAngle;
}

// Ing body fn_29_B0A4
bool CIngBoostBallGuardian::HasTarget(CStateManager& mgr, const CTriggerData&) const {
  return mgr.GetObjectById(mTargetId) != nullptr;
}

// Ing body fn_29_B04C
bool CIngBoostBallGuardian::HasNewTarget(CStateManager& mgr, const CTriggerData& data) const {
  return HasTarget(mgr, data) && mPrevTargetId != mTargetId;
}

// Ing body fn_29_B01C
bool CIngBoostBallGuardian::HasLineOfSight(CStateManager&, const CTriggerData&) const {
  return mLineOfSight.HasLineOfSight() && mLineOfSight.GetClearTime() > 0.5f;
}

// Ing body fn_29_B000
bool CIngBoostBallGuardian::UnderFire(CStateManager&, const CTriggerData&) const {
  return mUnderFireTime < 0.75f;
}

// Ing body fn_29_AFE4
bool CIngBoostBallGuardian::HeardShot(CStateManager&, const CTriggerData&) const {
  return mHeardShotTime < 1.5f;
}

// fn_29_AE68 (not yet exercised in a run)
bool CIngBoostBallGuardian::AreaClear(CStateManager& mgr, const CTriggerData&) const {
  const CVector3f half = mModelScale * 10.f;
  const CVector3f pos = GetTranslation();
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, CAABox(pos - half, pos + half),
                    CMaterialFilter::MakeInclude(CMaterialList(kMT_Player, kMT_Character)), this);
  return nearList.empty();
}

// Ing body fn_29_ADE8
bool CIngBoostBallGuardian::CoverLeash(CStateManager& mgr, const CTriggerData&) const {
  const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId);
  return hint == nullptr || (hint->GetTranslation() - GetTranslation()).MagSquared() >
                                mData.mUnknown5D0D * mData.mUnknown5D0D;
}

// Ing body fn_29_ADBC
bool CIngBoostBallGuardian::HasCoverPoint(CStateManager& mgr, const CTriggerData&) const {
  return GetCoverHint(mgr, mCoverHintId) != nullptr;
}

// Ing body fn_29_AD7C
bool CIngBoostBallGuardian::HasWallCoverPoint(CStateManager& mgr, const CTriggerData&) const {
  const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId);
  return hint != nullptr && hint->GetHintType() == static_cast< CScriptAIHint::EHintType >(4);
}

// fn_29_AD68 (not yet exercised in a run)
bool CIngBoostBallGuardian::HasMiniPortals(CStateManager&, const CTriggerData&) const {
  return mMiniPortalCount != 0;
}

// Ing body fn_29_AD54
bool CIngBoostBallGuardian::IsCorporeal(CStateManager&, const CTriggerData&) const {
  return mForm == kForm_Corporeal;
}

// Ing body fn_29_AD40
bool CIngBoostBallGuardian::IsIngSpot(CStateManager&, const CTriggerData&) const {
  return mForm == kForm_IngSpot;
}

// Ing body fn_29_AD2C
bool CIngBoostBallGuardian::IsBodyProjectile(CStateManager&, const CTriggerData&) const {
  return mForm == kForm_BodyProjectile;
}

// fn_29_AD14 (not yet exercised in a run)
bool CIngBoostBallGuardian::IsFrustrated(CStateManager&, const CTriggerData&) const {
  return mData.mFrustrationTime < mFrustrationTimer;
}

// @ fn_30_C1B0 (the Ing's fn_29_ACEC)
bool CIngBoostBallGuardian::IsAggressive(CStateManager&, const CTriggerData&) const {
  // 0x11EC 0x20, or the XENZ/XEXZ count at 0x115C.
  return mAggressive || mSafeZoneCount > 0;
}

// Ing body fn_29_ACE0
bool CIngBoostBallGuardian::UseProjectileFSMEntry(CStateManager&, const CTriggerData&) const {
  return mProjectileFSMEntry;
}

// fn_29_AC20 (not yet exercised in a run)
bool CIngBoostBallGuardian::TargetInSafeZone(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  return target != nullptr && mgr.GetSafeZoneManager()->IsObjectInSafeZone(*target, mgr);
}

// Ing body fn_29_ABC8
bool CIngBoostBallGuardian::TargetInLightSafeZone(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  return target != nullptr && mgr.GetSafeZoneManager()->IsObjectInHurtfulSafeZone(*target, mgr);
}

// Ing body fn_29_AAF8
bool CIngBoostBallGuardian::CanChangeForm(CStateManager&, const CTriggerData&) const {
  // 0x0000C0A8 (corporeal) / 0x0000C0D0 (ing spot): extrwi 1,28 = data 0x08.
  const bool locked = (mData.mFlags210 & kFlag_NoFormChange) != 0;
  switch (mForm) {
  case kForm_IngSpot:
    return !locked;
  case kForm_Corporeal:
    return !locked && mData.mUnknownC620 < mFormTime;
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
    return false;
  default:
    return true;
  }
}

// @ fn_30_C05C (the Ing's fn_29_AA94)
bool CIngBoostBallGuardian::ShouldFleeSafeZone(CStateManager&, const CTriggerData&) const {
  return mSafeZoneCount > 0; // fn_30_C05C: 0x115C
}

// fn_29_AA4C (not yet exercised in a run)
bool CIngBoostBallGuardian::ShouldBecomeCorporeal(CStateManager&, const CTriggerData&) const {
  return mDesiredForm == kForm_Corporeal && mForm != kForm_Corporeal;
}

// fn_29_AA04 (not yet exercised in a run)
bool CIngBoostBallGuardian::ShouldBecomeIngSpot(CStateManager&, const CTriggerData&) const {
  return mDesiredForm == kForm_IngSpot && mForm != kForm_IngSpot;
}

// Ing body fn_29_A8CC
bool CIngBoostBallGuardian::ShouldArmSwipe(CStateManager& mgr, const CTriggerData&) const {
  // 0x0000BEB4 form == 2, 0x0000BEC0 data 0x40.
  if (mForm != kForm_Corporeal || (mData.mFlags210 & kFlag_NoArmSwipe) != 0 ||
      !(mArmSwipeTime > 1.f)) {
    return false;
  }
  // The team gate only applies inside a team (retail skips it when mTeamId is kInvalid, as
  // fn_29_A8CC's branch at 0x0000A924 does).
  if (mTeamId != kInvalidUniqueId &&
      !CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Melee, mgr, mTeamId,
                                        GetUniqueId())) {
    return false;
  }
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return false;
  }
  const CVector3f delta = target->GetTranslation() - GetTranslation();
  const float distSq = delta.MagSquared();
  return mMinAttackRange * mMinAttackRange <= distSq &&
         distSq <= mMaxAttackRange * mMaxAttackRange && fabsf(delta.GetZ()) < 3.f;
}

// Ing body fn_29_A8C0
bool CIngBoostBallGuardian::ShouldTaunt(CStateManager&, const CTriggerData&) const {
  return mTaunt;
}

// Ing body fn_29_A780
bool CIngBoostBallGuardian::ShouldBodyProjectile(CStateManager& mgr, const CTriggerData&) const {
  // The team gate only applies inside a team: retail compares mTeamId with kInvalidUniqueId
  // and skips CanStartAttack when they are equal (asm 0x0000A8A4, the branch that lands at
  // .L_0000A7FC). An unconditional gate keeps the state off for every Ing placed without a team.
  // 0x0000BD68 (data 0x20), 0x0000BD70 (data 0x08), 0x0000BD7C (0x11EC 0x40).
  if ((mData.mFlags210 & (kFlag_NoBodyProjectile | kFlag_NoFormChange)) != 0 ||
      !mBodyProjectileAllowed ||
      (mTeamId != kInvalidUniqueId &&
       !CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId,
                                         GetUniqueId()))) {
    return false;
  }
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  const CVector3f delta = target != nullptr ? target->GetTranslation() - GetTranslation()
                                            : CVector3f::Zero();
  const float distSq = delta.MagSquared();
  const float minDist = mData.mBodyProjectile.mMinAttackDist; // 0x85c
  const float maxDist = mData.mBodyProjectile.mMaxAttackDist; // 0x860
  const bool inRange = target != nullptr && minDist * minDist <= distSq &&
                       distSq <= maxDist * maxDist && -15.f <= delta.GetZ() &&
                       delta.GetZ() <= 3.f;
  static int sTick = 0;
  if (BgLogTick(sTick)) {
    fprintf(stderr,
            "[ibbg] %04x body proj trigger: target %04x dist %.1f [%.1f..%.1f] dz %.1f -> %d\n",
            GetUniqueId().Value(), mTargetId.Value(), CMath::SqrtF(distSq), minDist, maxDist,
            delta.GetZ(), inRange ? 1 : 0);
  }
  return inRange;
}

// Ing body fn_29_A664
bool CIngBoostBallGuardian::ShouldOpenMiniPortal(CStateManager& mgr, const CTriggerData&) const {
  // Same team gate as fn_29_A780/A8CC: retail asks CanStartAttack only when mTeamId is set
  // (asm 0x0000A788, the compare that skips to .L_0000A6E0).
  // 0x0000BC4C: data 0x10.
  if ((mData.mFlags210 & kFlag_NoMiniPortal) != 0 || mFormTime < mData.mUnknownC620 ||
      (mTeamId != kInvalidUniqueId &&
       !CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId,
                                         GetUniqueId()))) {
    return false;
  }
  const float distSq = DistSqToTarget(mgr);
  const float minDist = mData.mMiniPortal.mMinAttackDist;
  const float maxDist = mData.mMiniPortal.mMaxAttackDist;
  return minDist * minDist <= distSq && distSq <= maxDist * maxDist;
}

// @ fn_30_BC20 (the Ing's fn_29_A658): 0x11ED 0x01, raised by CollidedWith
bool CIngBoostBallGuardian::ProjectileSplat(CStateManager&, const CTriggerData&) const {
  return mSplat;
}

// fn_29_A4EC (not yet exercised in a run)
bool CIngBoostBallGuardian::ProjectileHitTarget(CStateManager& mgr, const CTriggerData&) const {
  const CEntity* entity = mgr.GetObjectById(mTargetId);
  const CPhysicsActor* target = TCastToConstPtr< CPhysicsActor >(entity);
  if (target == nullptr) {
    return false;
  }
  if (const CPlayer* player = TCastToConstPtr< CPlayer >(entity)) {
    const CPlayer::EPlayerMorphBallState state = player->GetMorphballTransitionState();
    if (state != CPlayer::kMS_Unmorphed && state != CPlayer::kMS_Unmorphing) {
      return false;
    }
  }
  const CVector3f half = mModelScale * 2.5f;
  const CVector3f pos = GetTranslation();
  return CAABox(pos - half, pos + half).DoBoundsOverlap(target->GetBoundingBox());
}

// fn_29_A4E0 (not yet exercised in a run)
bool CIngBoostBallGuardian::FoundMovementPos(CStateManager&, const CTriggerData&) const {
  return mFoundMovementPos;
}

// Ing body fn_29_A498
bool CIngBoostBallGuardian::PathShagged(CStateManager& mgr, const CTriggerData& data) const {
  return mRecalcPath || CPatterned::PathShagged(mgr, data);
}

// Ing body fn_29_A450
bool CIngBoostBallGuardian::PathOver(CStateManager& mgr, const CTriggerData& data) const {
  return !mFoundMovementPos || CPatterned::PathOver(mgr, data);
}

// fn_29_A428 (not yet exercised in a run)
bool CIngBoostBallGuardian::PointPathOver(CStateManager&, const CTriggerData&) const {
  return mIngSpotPathFindNavigation.IsPathOver(*this);
}

// fn_29_A400 (not yet exercised in a run)
bool CIngBoostBallGuardian::FoundPointPath(CStateManager&, const CTriggerData&) const {
  return mIngSpotPathFindNavigation.HasPath(*this);
}

// Ing body fn_29_A3C8
bool CIngBoostBallGuardian::IsOffPath(CStateManager&, const CTriggerData&) const {
  return mPathFindSearch.OnPath(GetTranslation()) != CPathFindSearch::kR_Success;
}

// Ing body fn_29_A370
bool CIngBoostBallGuardian::HasAttackPattern(CStateManager& mgr, const CTriggerData&) const {
  return const_cast< CIngBoostBallGuardian* >(this)->GetConnectedObject(mgr, kSS_Attack,
                                                                        kSM_Follow) !=
         kInvalidUniqueId;
}

// fn_29_A340 (not yet exercised in a run)
bool CIngBoostBallGuardian::AttackPatternOver(CStateManager&, const CTriggerData&) const {
  if (mFollowingAttackPath) {
    const_cast< CIngBoostBallGuardian* >(this)->mFollowingAttackPath =
        mWaypointNavigation.GetDestination() != kInvalidUniqueId;
  }
  return !mFollowingAttackPath;
}

// Ing body fn_29_A2D8: the target is a ball that was not spawned as one - asm 0x0000A2E0 tests
// 0x390 (spawned state) first and only reads 0x38c (transition state) when it is 0.
bool CIngBoostBallGuardian::TargetIsBall(CStateManager& mgr, const CTriggerData&) const {
  const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId));
  const bool isBall = player != nullptr &&
                      player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed &&
                      player->GetMorphballTransitionState() == CPlayer::kMS_Morphed;
  static int sTick = 0;
  if (BgLogTick(sTick)) {
    fprintf(stderr, "[ibbg] %04x target is ball: target %04x player %d morph %d -> %d\n",
            GetUniqueId().Value(), mTargetId.Value(), player != nullptr ? 1 : 0,
            player != nullptr ? int(player->GetMorphballTransitionState()) : -1,
            isBall ? 1 : 0);
  }
  return isBall;
}

// fn_29_A1C0 (not yet exercised in a run)
bool CIngBoostBallGuardian::HasPathToTarget(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  return target != nullptr &&
         const_cast< CPathFindSearch& >(mPathFindSearch)
                 .PathExists(GetTranslation(), target->GetTranslation()) ==
             CPathFindSearch::kR_Success;
}

// fn_30_B7E0: a target player with nothing attached to it (0x0000B818 compares the player's
// 0x2e4 with kInvalidUniqueId). Unlike the Ing's fn_29_A098 it reads no data flag.
bool CIngBoostBallGuardian::CanGrappleTarget(CStateManager& mgr, const CTriggerData&) const {
  const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId));
  return player != nullptr && player->GetAttachedActorId() == kInvalidUniqueId;
}

// ---- code functions (the FSM2 runs them: CGenericFSM2State::SetCodeFunctions) -------------------

// Ing body fn_29_5B60
void CIngBoostBallGuardian::SetTargetDest(CStateManager& mgr, float) {
  LogState("SetTargetDest");
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return;
  }
  mTargetPos = target->GetTranslation();
  mFoundMovementPos = true;
  mPathFindNavigation.SetDestination(mTargetPos);
  JoinTeam(mgr);
}

// Ing body fn_29_5AC4: destination = the cover hint's translation when there is one, else the Ing's own
// position; mFoundMovementPos only in the first case; then SetDestination and the face target at
// kInvalidUniqueId (0x718), asm 0x00005B00-0x00005B48.
void CIngBoostBallGuardian::SetCoverDest(CStateManager& mgr, float) {
  LogState("SetCoverDest");
  CVector3f dest = GetTranslation();
  mFoundMovementPos = false;
  if (const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId)) {
    dest = hint->GetTranslation();
    mFoundMovementPos = true;
  }
  mPathFindNavigation.SetDestination(dest);
  mPathFindNavigation.SetFaceTarget(kInvalidUniqueId);
}

// Ing body fn_29_59D0: two closest-point queries from the Ing's own position - filter (20, 0, -1) then
// (20, 1, from), asm 0x000059E0 and 0x00005A44 - and a point-graph search between the two results
// (0x00005A94). The second filter's connected point is the *first* query's index (the asm keeps it
// in r5 from 0x00005A2C), so the second point is one the graph connects to the first - without
// that the pair is unrelated and the search below fails. It sets no destination:
// mFoundMovementPos is what IngSpotPathFind reads.
void CIngBoostBallGuardian::SetRecoverDest(CStateManager&, float) {
  LogState("SetRecoverDest");
  mFoundMovementPos = false;
  mPointPathFindSearch.ClearWaypoints(); // 0xacc = 0, asm 0x00005A08
  int from = -1;
  if (mPointPathFindSearch.FindClosestPhysicalPoint(GetTranslation(), from,
                                                    CPathFindPointSearchFilter(20.f, 0, -1)) !=
          CPathFindPointSearch::kCPR_Success ||
      from == -1) {
    return;
  }
  int to = -1;
  if (mPointPathFindSearch.FindClosestPhysicalPoint(GetTranslation(), to,
                                                    CPathFindPointSearchFilter(20.f, 1, from)) !=
          CPathFindPointSearch::kCPR_Success ||
      to == -1) {
    return;
  }
  if (mPathArea != nullptr && BgPointSearch(mPointPathFindSearch, *mPathArea, from, to)) {
    mFoundMovementPos = true;
  }
}

// Ing body fn_29_5820: the point-graph route to the cover hint. Filter (20, 0, -1) from the Ing's own
// position, then the nearest flag-1 point that the graph connects to it, sits under 400
// (rodata_F0) from the hint and has a mesh path to it (asm 0x00005844-0x00005998), then the point
// search between the two.
void CIngBoostBallGuardian::SetPointCoverDest(CStateManager& mgr, float) {
  LogState("SetPointCoverDest");
  mFoundMovementPos = false;
  mPointPathFindSearch.ClearWaypoints();
  const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId);
  if (hint == nullptr || mPathArea == nullptr) {
    return;
  }
  int from = -1;
  mPointPathFindSearch.FindClosestPhysicalPoint(GetTranslation(), from,
                                                CPathFindPointSearchFilter(20.f, 0, -1));
  if (from == -1) {
    return;
  }
  const CVector3f cover = hint->GetTranslation();
  float bestDistSq = 400.f; // rodata_F0
  int best = -1;
  for (int i = 0; i < mPathArea->GetNumPoints(); ++i) {
    const CPFPoint& point = mPathArea->GetPoint(i);
    // asm 0x000058E0: only points whose flags word has bit 0 set are cover points.
    if ((point.GetFlags() & 1) == 0 || !mPathArea->PointPathExists(from, i)) {
      continue;
    }
    const float distSq = (cover - point.GetPosition()).MagSquared();
    if (distSq < bestDistSq &&
        mPathFindSearch.PathExists(point.GetPosition(), cover) == CPathFindSearch::kR_Success) {
      best = i;
      bestDistSq = distSq;
    }
  }
  if (best == -1) {
    return;
  }
  if (BgPointSearch(mPointPathFindSearch, *mPathArea, from, best)) {
    mFoundMovementPos = true;
  }
}

// fn_29_5714 (not yet exercised in a run)
void CIngBoostBallGuardian::SetWallPointCoverDest(CStateManager& mgr, float) {
  LogState("SetWallPointCoverDest");
  mFoundMovementPos = false;
  mPointPathFindSearch.ClearWaypoints(); // 0xacc = 0, asm 0x0000573C
  const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId); // fn_29_45BC, id at 0xd72
  // Retail tests the hint only (fn_29_45BC returns null when the id is not a CScriptAIHint); the
  // host can run without a path area, so the point queries are skipped when mPathArea is null.
  if (hint == nullptr || mPathArea == nullptr) {
    return;
  }
  int from = -1;
  mPointPathFindSearch.FindClosestPhysicalPoint(GetTranslation(), from,
                                                CPathFindPointSearchFilter(20.f, 0, -1));
  if (from == -1) {
    return;
  }
  int to = -1;
  // asm 0x00005778: the second filter's connected point is the index the first query found, so only
  // points the graph connects to the Ing's own point are candidates.
  mPointPathFindSearch.FindClosestPhysicalPoint(hint->GetTranslation(), to,
                                                CPathFindPointSearchFilter(20.f, 0, from));
  if (to != -1 && BgPointSearch(mPointPathFindSearch, *mPathArea, from, to)) {
    mFoundMovementPos = true;
  }
}

// Ing body fn_29_54E8. Where the exit host lands: the AI hint that best matches
// the way the player is facing. Retail starts from the player's own position (uid +8, transform
// +0x24) and takes mgr+0x14fc as the player with no null test, which the port keeps as a guard.
// The score is (GetAngleDiff(forward, hint - Ing) + 1) * |hint - Ing|^2, and the comparison pair
// keeps a 400.0f (rodata_F0, i.e. 20 units squared) petal around zero: under it the *smaller*
// non-negative score wins, over it the larger one does. Retail writes mTargetPos (0xdc8) and
// nothing else - in particular it does not touch mFoundMovementPos, so neither does this.
void CIngBoostBallGuardian::SetExitHostDest(CStateManager& mgr, float) {
  LogState("SetExitHostDest");
  const CPlayer* player = mgr.GetPlayer(0); // mgr+0x14fc
  if (player == nullptr) {
    return;
  }
  const CVector3f forward = player->GetTransform().GetForward();
  float best = 0.f; // rodata_6C
  CVector3f bestPos = player->GetTranslation();
  CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    CScriptAIHint* hint = TCastToPtr< CScriptAIHint >(list[i]);
    // Retail compares the object's own area word against the Ing's (there is no active/in-use test)
    // and skips hint types 4, 14 and 15 (asm 0x00006D20..0x00006D30), types with no name in
    // CScriptAIHint::EHintType.
    const int hintType = hint != nullptr ? static_cast< int >(hint->GetHintType()) : 0;
    if (hint == nullptr || hint->GetCurrentAreaId() != GetCurrentAreaId() || hintType == 4 ||
        hintType == 14 || hintType == 15) {
      continue;
    }
    const CVector3f delta = hint->GetTranslation() - GetTranslation();
    const float score = (CVector3f::GetAngleDiff(forward, delta) + 1.f) * delta.MagSquared();
    if ((score < best && score >= 400.f) || (score > best && best <= 400.f)) {
      best = score;
      bestPos = hint->GetTranslation();
    }
  }
  mTargetPos = bestPos; // 0xdc8
}

// fn_29_5190 (not yet exercised in a run)
void CIngBoostBallGuardian::StartRangedAttack(CStateManager& mgr, float) {
  CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId, GetUniqueId());
}

// fn_29_514C (not yet exercised in a run)
void CIngBoostBallGuardian::EndRangedAttack(CStateManager& mgr, float) {
  CScriptTeamAiMgr::EndAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId, GetUniqueId(),
                              false);
}

// @ fn_30_6980 (the Ing's fn_29_4F14): the body projectile hit the mesh, so the Ing splats back
// into its ing-spot form: form 3, the scripted body state aborted, the splat-wall sound once at the
// Ing's own position (its handle is discarded), then - when the splat token at 0xd48 is engaged - an
// explosion on the Ing's transform at the model's scale ("Ing Splat Wall Fx").
void CIngBoostBallGuardian::SplatOntoMesh(CStateManager& mgr, float) {
  LogState("SplatOntoMesh");
  mForm = kForm_IngSpot; // 0x9d4 = 3
  BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_AbortScripted)); // id word 13
  // 0x000069D8: the contact normal CollidedWith kept (0x1104); rodata_64 is 1.0f.
  mSurfaceAlignment.OrientToSurfaceNormal(*this, mSplatNormal, 1.f);
  // 0x852: the body projectile's splat-wall sound. Retail stores no handle, so neither do we.
  CSfxManager::AddEmitter(mData.mBodyProjectile.mSoundSplatWall, GetTranslation(), 0x7f,
                          GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
  const bool splatFx = static_cast< bool >(mSplatEffect); // 0xd54, the token's engaged byte
  if (DebugBg()) {
    fprintf(stderr, "[ibbg] %04x splat: normal (%.2f, %.2f, %.2f), splat fx %s\n",
            GetUniqueId().Value(), mSplatNormal.GetX(), mSplatNormal.GetY(), mSplatNormal.GetZ(),
            splatFx ? "explosion" : "token not loaded");
  }
  if (splatFx) {
    mgr.AddObject(rs_new CExplosion(*mSplatEffect, mgr.AllocateUniqueId(),
                                    CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList,
                                                true, kInvalidEditorId),
                                    rstl::string_l("Ing Splat Wall Fx"), GetTransform(), 0,
                                    GetModelData()->GetScale(), CColor::White(), -1));
  }
}
