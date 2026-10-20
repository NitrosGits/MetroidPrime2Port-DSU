// Host body for the IngBoostBallGuardian REL (module 30), milestone 1: the actor loads, stands
// and renders. Every vtable override that belongs to retail's AI forwards to CPatterned behind a
// once-only StandIn log, and the REL function it stands in for is named on each. The state machine
// (fn_30_C6CC installs the 49 triggers, 28 states and 9 codes), the boost ball, the puddle form,
// the mini portals and the second class (a CPhysicsActor spawned by fn_30_3478) are not written
// yet - docs/module-analysis/BoostGuardian.md section i has the plan.
//
// Which methods the REL overrides is not guessed: its vtable lbl_30_data_9C0 lists them by address
// (/tmp/opencode/ibbg_vtable.md) and the slots line up with CPatterned's declaration order, the
// inherited entries being spelled out by name in the same table. The tiny accessors at the head of
// the module's .text (fn_30_0, fn_30_8, fn_30_10, fn_30_34, fn_30_3C, fn_30_58, fn_30_6C) have
// four-to-twelve-instruction bodies, so they are real rather than stand-ins.
#include "MetroidPrime/Enemies/CIngBoostBallGuardian.hpp"

#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CGenericFSM2.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Enemies/PortBoostGuardianShared.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrIngBoostBallGuardian.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

#include <cstdlib>
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
  fprintf(stderr, "[ibbg] %s is a port stand-in (see PortCIngBoostBallGuardian.cpp)\n", what);
}

// ---------------------------------------------------------------------------------------------
// The module's FSM2 probe (MP2_BG_FSM). The transitions are not in the REL's .data - they come
// from the actor's 'FSM2' asset - and a transition's own mName is empty in this asset, so the
// TARGET node is the information a dump has to carry: which kind of node (state, trigger, code
// or sub-machine), its index in the machine's own list and its name.

// The node a transition points at.
struct SBGFsmNode {
  const char* mKind; // "state", "trigger", "code", "submachine" or "none".
  int mIndex;        // Index in the machine's own list, -1 when the target is not in it.
  const char* mName; // The node's name.
};

// Finds the list the target belongs to (by its type) and then the element the target points at.
SBGFsmNode BGFsmNodeOf(const CGenericFSM2& machine, const CState2* target) {
  SBGFsmNode node = { "none", -1, "" };
  if (target == nullptr) {
    return node;
  }
  switch (target->GetType()) {
  case CState2::kType_State: {
    const rstl::vector< CState2State >& states = machine.GetStates();
    for (int i = 0; i < states.size(); ++i) {
      if (static_cast< const CState2* >( &states[i] ) == target) {
        node.mKind = "state";
        node.mIndex = i;
        node.mName = states[i].GetName().data();
        return node;
      }
    }
    break;
  }
  case CState2::kType_Trigger: {
    const rstl::vector< CState2Trigger >& triggers = machine.GetTriggers();
    for (int i = 0; i < triggers.size(); ++i) {
      if (static_cast< const CState2* >( &triggers[i] ) == target) {
        node.mKind = "trigger";
        node.mIndex = i;
        node.mName = triggers[i].GetName().data();
        return node;
      }
    }
    break;
  }
  case CState2::kType_Code: {
    const rstl::vector< CState2Code >& codes = machine.GetCodes();
    for (int i = 0; i < codes.size(); ++i) {
      if (static_cast< const CState2* >( &codes[i] ) == target) {
        node.mKind = "code";
        node.mIndex = i;
        node.mName = codes[i].GetName().data();
        return node;
      }
    }
    break;
  }
  case CState2::kType_SubMachine: {
    const rstl::vector< CState2SubMachine >& subMachines = machine.GetSubMachines();
    for (int i = 0; i < subMachines.size(); ++i) {
      if (static_cast< const CState2* >( &subMachines[i] ) == target) {
        node.mKind = "submachine";
        node.mIndex = i;
        node.mName = subMachines[i].GetName().data();
        return node;
      }
    }
    break;
  }
  }
  return node;
}

// One line per transition: the transition's own name (when it has one) and its target.
void DumpBGFsmTransitions(const CGenericFSM2& machine,
                           const rstl::vector< SStateMachine2Transition >& transitions,
                           const char* indent) {
  for (int i = 0; i < transitions.size(); ++i) {
    const SStateMachine2Transition& transition = transitions[i];
    const SBGFsmNode node = BGFsmNodeOf(machine, transition.mTarget);
    if (transition.mName.size() > 0) {
      fprintf(stderr, "[bgfsm] %s[%s] -> %s %d %s\n", indent, transition.mName.data(),
              node.mKind, node.mIndex, node.mName);
    } else {
      fprintf(stderr, "[bgfsm] %s-> %s %d %s\n", indent, node.mKind, node.mIndex, node.mName);
    }
  }
}

// One line per node and one line per transition, for every list of the machine. A sub-machine
// that carries its own machine is dumped one level deeper.
void DumpBGFsmMachine(const CGenericFSM2& machine, const char* indent, const char* childIndent) {
  fprintf(stderr, "[bgfsm] %smachine: %d states, %d triggers, %d codes, %d sub-machines\n", indent,
          machine.GetStates().size(), machine.GetTriggers().size(), machine.GetCodes().size(),
          machine.GetSubMachines().size());

  const rstl::vector< CState2State >& states = machine.GetStates();
  for (int i = 0; i < states.size(); ++i) {
    fprintf(stderr, "[bgfsm] %sstate %d %s\n", indent, i, states[i].GetName().data());
    DumpBGFsmTransitions(machine, states[i].GetTransitions(), childIndent);
  }

  const rstl::vector< CState2Trigger >& triggers = machine.GetTriggers();
  for (int i = 0; i < triggers.size(); ++i) {
    fprintf(stderr, "[bgfsm] %strigger %d %s negate=%d arg=%f\n", indent, i,
            triggers[i].GetName().data(), triggers[i].IsNegated() ? 1 : 0,
            triggers[i].GetArgument().GetFloat());
    DumpBGFsmTransitions(machine, triggers[i].GetTransitions(), childIndent);
  }

  const rstl::vector< CState2Code >& codes = machine.GetCodes();
  for (int i = 0; i < codes.size(); ++i) {
    fprintf(stderr, "[bgfsm] %scode %d %s\n", indent, i, codes[i].GetName().data());
    DumpBGFsmTransitions(machine, codes[i].GetTransitions(), childIndent);
  }

  const rstl::vector< CState2SubMachine >& subMachines = machine.GetSubMachines();
  for (int i = 0; i < subMachines.size(); ++i) {
    fprintf(stderr, "[bgfsm] %ssubmachine %d %s\n", indent, i, subMachines[i].GetName().data());
    DumpBGFsmTransitions(machine, subMachines[i].GetTransitions(), childIndent);
    const CGenericFSM2* sub = subMachines[i].GetMachine();
    if (sub != nullptr) {
      fprintf(stderr, "[bgfsm] %ssubmachine %d machine:\n", indent, i);
      DumpBGFsmMachine(*sub, "    ", "      ");
    }
  }
}

// Dumps the machine once per process. The machine is the actor's 'FSM2' asset; its nodes carry
// the names the REL's own tables (fn_30_C6CC) use, so the dump gives the machine's shape and
// the names the module looks its handlers up by.
void DumpBGFsmOnce(CIngBoostBallGuardian* guardian) {
  static bool sDumped = false;
  if (sDumped) {
    return;
  }
  sDumped = true;
  CGenericFSM2* machine = guardian->GetStateMachine2();
  if (machine == nullptr) {
    fprintf(stderr, "[bgfsm] no FSM2 machine\n");
    return;
  }
  DumpBGFsmMachine(*machine, "", "  ");
}

} // namespace

// fn_30_EC6C. The base call is the retail one: EPatternedAI 0x19 (`li r4, 0x19` at 0x0000ECC0),
// flavor 0 (`li r7, 0x0` at 0x0000ECAC), movement 0 / collider 1 / body 1 (the constants stored to
// the stack arguments at 0x0000ECBC..0x0000ECC8). The path searches are fn_30_EC6C's own calls:
// CPFArea null, flags 0x301, the patterned info's pathfinding index (+0xEC), radii 1.0f (rodata_64)
// and region 0 (0x0000ED14..0x0000ED24).
CIngBoostBallGuardian::CIngBoostBallGuardian(TUniqueId uid, const rstl::string& name,
                                             const CEntityInfo& info, const CTransform4f& xf,
                                             const CModelData& modelData,
                                             const CActorParameters& actParms,
                                             const CPatternedInfo& pInfo,
                                             const CIngBoostBallGuardianData& data,
                                             const CBoostBallGuardianData& boost)
: CPatterned(static_cast< EPatternedAI >(0x19), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data)      // guest 0x7C0, copied by fn_30_1056C
, mForm(2)         // 0x0000ED00
, mDesiredForm(-1) // 0x0000ED10
, mPathFindSearch(nullptr, 0x301, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0,
                  CPFRegion::kRP_Center) // 0x0000ED24
, mPointPathFindSearch(nullptr)          // 0x0000ED30, the DOL's CPathFindPointSearch ctor
, mCollisionActorManager0(nullptr)       // 0x0000ED3C
, mCollisionActorManager1(nullptr)       // 0x0000ED40
, mSurfaceAlignment()                    // 0x0000ED44
, mIngSpotPathFindNavigation()           // 0x0000ED4C
, mBoost(boost)                          // guest 0xDEC, copied by fn_30_10250
, mVulnerability(mBoost.mVulnerability)  // 0x0000FD48 builds a CWeaponTypeVulnerability-derived
                                         // CDamageVulnerability; its source is still open, this
                                         // uses the boost ball's until the asm says otherwise
, mHostId(kInvalidUniqueId)
, mPossessedEffectId(kInvalidUniqueId)
, mSwarmEffect(BgOptionalPart(data.mSwarm.mPART))
, mSwarmEffect2(BgOptionalPart(data.mSwarm.mPART081E))
, mBlobId(kInvalidUniqueId)
, mTeamId(kInvalidUniqueId)
, mTargetId(kInvalidUniqueId)
, mPrevTargetId(kInvalidUniqueId)
, mAttackPathResumeId(kInvalidUniqueId)
, mCoverHintId(kInvalidUniqueId)
, mLastCoverHintId(kInvalidUniqueId)
, mD76Id(kInvalidUniqueId)
, mNormalHitEffect(BgOptionalPart(data.mIngSpot.GetNormalHitEffect()))
, mHeavyHitEffect(BgOptionalPart(data.mIngSpot.GetHeavyHitEffect()))
, mDeathEffect(BgOptionalPart(data.mIngSpot.GetDeathEffect()))
, mMiniPortalEffect(BgOptionalPart(data.mMiniPortal.mEffect))
, mSplatEffect(BgOptionalPart(data.mBodyProjectile.mSplatEffect))
, mEyeSegId(BgHeadSegId(modelData))
, mRightShoulderSegId(BgArmSegId(modelData, "R_shoulder"))
, mRightElbowSegId(BgArmSegId(modelData, "R_elbow"))
, mRightForearmSegId(BgArmSegId(modelData, "R_forearm"))
, mRightWristSegId(BgArmSegId(modelData, "R_wrist_end"))
, mLeftShoulderSegId(BgArmSegId(modelData, "L_shoulder"))
, mLeftElbowSegId(BgArmSegId(modelData, "L_elbow"))
, mLeftForearmSegId(BgArmSegId(modelData, "L_forearm"))
, mLeftWristSegId(BgArmSegId(modelData, "L_wrist_end"))
, mLineOfSight(uid, mEyeSegId, 0.1f, 0.05f)
, mTargetPos(CVector3f::Zero())
, mSurfaceMoveDir(CVector3f::Zero())
, mPortalPlaneNormal(CVector3f::Zero())
, mPortalPlaneConstant(0.f)
, mSwipeArm(-1)
, mModelScale(modelData.GetScale())
, mMiniPortalCount(0)
, mMiniPortalPositions()
, mMiniPortalIndex(-1)
, mHeardShotTime(0.f)
, mUnderFireTime(0.f)
, mArmSwipeTime(0.f)
, mFormTime(0.f)
, mBodyState5Time(0.f)
, mFrustrationTimer(0.f)
, mBallCooldown(0.f)
, mHitReactionTime(0.f)
, mLightBlend(0.f)
, mDeadFade(0.f)
, mBodyProjectileTime(0.f)
, mCollidedWithCrystal(false)
, mBoostPuddleForm(false)
, mShouldBoost(false)
, mBoostOver(true) // ctor 0x0000FECC: 0x11EE 0x20 = 1
, mBoostModeRetreatActive(false)
, mCurrentlyInBoostPuddleForm(false)
, mBoostPuddleGenerate(false) // ctor 0x0000FEF0: 0x11EE 0x04 = 0
, mReflectVulnerability(false) // ctor 0x0000FEFC: 0x11EE 0x02 = 0
, mBoostPuddleAllowed(true)    // ctor 0x0000FF2C: 0x11EF 0x20 = 1
, mBoostChildId(kInvalidUniqueId)
, mBoostNavPointId(kInvalidUniqueId)
, mBoostNavPointId2(kInvalidUniqueId)
, mBoostTimer(0.f)
, mBoost11D4(0.f)                                       // ctor 0x0000FDC4
, mBoost11D8(0.f)                                       // ctor 0x0000FDC8
, mBoostPuddleModeTimer(mBoost.mPhase0.mBoostPuddleModeTime) // ctor 0x0000FDDC: *(0x11D0)+0
, mBoostRandomFloat(0.f)
, mAggressive(false)
, mFoundMovementPos(true) // ctor 0x0000FE0C: 0x11EC 0x10 = 1
, mRecalcPath(false)
, mTakeOff(false)
, mStartAsIngSpot((mData.mFlags210 & kFlag_StartAsIngSpot) != 0) // ctor 0x0000FE60: data 0x80
, mMoving(false)
, mGrappled(false)
, mProjectileFSMEntry(false)
, mSplat(false)
, mHurtfulZone(false)
, mPortalPlane(false)
, mRenderModel(true) // ctor 0x0000FE3C: 0x11EC 0x01 = 1 (Render's 0x0000D408 test)
, mForm9Render(false)
, mFollowingAttackPath(false)
, mAlert(false)
, mTaunt(false)
, mBodyProjectileAllowed(false)
, mLastLoggedState(nullptr)
, mStateTime(0.f)
, mSuckTickLogged(false)
, mPathArea(nullptr)
, mDebugPoll(0)
{
  // The retail constructor turns knock-back physics off on CPatterned's own manager
  // (@0x0000FF40) and leaves the shadow off; everything else it builds - the locator segment ids
  // at 0x10AC, the nine locked tokens at 0x0FDC and the line-of-sight tracker at 0x10B8 - belongs
  // to the state machine and the render paths this milestone does not enter.
  KnockBackController().EnableKnockBackPhysics(false);
  SetDrawShadow(false);
  // The path searches' radii come from the model's scale, and the line-of-sight tracker only sees
  // solid material (the Ing's own settings, fn_29_D540).
  mPathFindSearch.SetCharacterHeight(mModelScale.GetY() * 3.f);
  mPathFindSearch.SetCharacterRadius(mModelScale.GetZ() * 5.f);
  mLineOfSight.SetRayFilter(
      CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid),
                                          CMaterialList(kMT_Player, kMT_CollisionActor)));
  if (std::getenv("MP2_BG_FSM") != nullptr) {
    // The 'FSM2' asset is loaded with the actor, so the machine is already here. The state
    // machine itself is only set up when the actor is active (CPatterned::Think reaches
    // InitializeStateMachine), which the boss of this area is not yet, so the dump is taken
    // here as well as in SetupStateMachine.
    fprintf(stderr, "[bgfsm] ctor: the FSM2 machine is %s\n",
            GetStateMachine2() != nullptr ? "loaded" : "missing");
    DumpBGFsmOnce(this);
  }
}

CIngBoostBallGuardian::~CIngBoostBallGuardian() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove / Death / KnockBack all dereference the current state. The analysis does not say
// where retail activates it, so the port activates on kSM_Create and, as the other patterned
// actors in this port do, again on demand in every entry point that can run first.
void CIngBoostBallGuardian::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}
// vtable slot 31 = fn_30_C850 (0x1c8): what the boost ball hits.
void CIngBoostBallGuardian::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                                         CStateManager& mgr) {
  StandIn("CIngBoostBallGuardian::CollidedWith (vtable slot 31, fn_30_C850)");
  CPatterned::CollidedWith(id, list, mgr);
}

// vtable slot 58 = fn_30_C6CC (0x8c): CPatterned's own setup, then the three name tables are
// handed to the machine - lbl_30_data_250 (49 triggers), lbl_30_data_6B0 (28 states) and
// lbl_30_data_8DC (9 codes). That is the module's FSM, and it is installed here.
void CIngBoostBallGuardian::SetupStateMachine(CStateManager& mgr) {
  CPatterned::SetupStateMachine(mgr);
  // The three tables the REL hands the machine (the 49 triggers, the 28 states and the 9 code
  // functions) go in under the REL's own names; the machine finds a node's handler by name.
  InstallStateMachineFunctions();

  // The transitions are only in the actor's 'FSM2' asset, so the probe (MP2_BG_FSM) dumps the
  // machine the asset holds: every state, trigger, code and sub-machine with the target of each
  // of their transitions. Once per process.
  if (std::getenv("MP2_BG_FSM") != nullptr) {
    DumpBGFsmOnce(this);
  }
}

// vtable slot 67 = fn_30_3C: stores kInvalidUniqueId, i.e. the actor never reports an attack
// target through this accessor.
TUniqueId CIngBoostBallGuardian::GetAttackTarget() const { return kInvalidUniqueId; }

CEntity* CIngBoostBallGuardian::TypesMatch(int typeId) const {
  return typeId == kET_IngBoostBallGuardian ? const_cast< CIngBoostBallGuardian* >(this) : nullptr;
}

// ---------------------------------------------------------------------------------------------
// The module's loader (fn_30_130) and entry points.

static CEntity* PortLoadIngBoostBallGuardian(CStateManager& mgr, CInputStream& input,
                                             CEntityInfo& info) {
  SLdrIngBoostBallGuardian sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrIngBoostBallGuardian.inc"

  rstl::optional_object< CModelData > modelData(LdrToModelData(
      sldrThis.editorProperties.transform.scale, kInvalidAssetId,
      sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL checks the copy's valid byte (sp+0x36c at 0x000002A4) and returns null when the
    // model is empty.
    return nullptr;
  }

  // The Ing half first (loader fn_30_10C24 builds it into sp+0x8bc), then the boost half
  // (fn_30_1988 into sp+0x41c).
  const SLdrIngBoostBallGuardianData& ing = sldrThis.guardianProperties;
  const SLdrBoostBallGuardianData& boostSldr = sldrThis.boostBallGuardian;

  if (std::getenv("MP2_BG_FSM") != nullptr) {
    fprintf(stderr, "[bgfsm] loader: stateMachine=%08x stateMachine2=%08x\n",
            sldrThis.patterned.stateMachine, sldrThis.patterned.stateMachine2);
  }

  const CIngSpotData ingSpot(
      ing.ingSpotBlobEffect, ing.ingSpotHitNormalDamage, ing.ingSpotHitHeavyDamage, ing.ingSpotDeath,
      ing.ingSpotMaxSpeed, ing.ingSpotMaxWallSpeed, ing.ingSpotBallPursuitSpeed,
      ing.unknown_0x50398a06, ing.ingSpotTurnSpeed,
      LdrToDamageVulnerability(ing.ingSpotVulnerability),
      static_cast< ushort >(ing.sound_IngSpotIdle),
      static_cast< ushort >(ing.sound_IngSpotMove),
      static_cast< ushort >(ing.sound_HitNormalDamage),
      static_cast< ushort >(ing.sound_HitHeavyDamage),
      static_cast< ushort >(ing.sound_IngSpotDeath));

  const CIngBodyProjectileData bodyProjectile(
      LdrToDamageInfo(ing.bodyProjectileContactDamage), ing.unknown_0xa0d63374,
      ing.bodyProjectileSuckTime, ing.bodyProjectileSpeed, ing.bodyProjectileDropTime,
      ing.bodyProjectileMinAttackDist, ing.bodyProjectileMaxAttackDist, ing.bodyProjectileOdds,
      ing.bodyProjectileSplatEffect, static_cast< ushort >(ing.sound_BodyProjectile),
      static_cast< ushort >(ing.sound_BodyProjectileSplatWall));

  const CIngMiniPortalData miniPortal(ing.miniPortalMinAttackDist, ing.miniPortalMaxAttackDist,
                                      ing.miniPortalEffect,
                                      static_cast< ushort >(ing.sound_MiniPortal),
                                      LdrToDamageInfo(ing.miniPortalProjectileDamage),
                                      ing.miniPortalBeamInfo);

  const CIngSwarmData swarm(ing.pART, ing.sRSC, ing.pART_0x3da219c7, ing.unknown_0x23271976,
                            ing.pART_0x081e9e6c, ing.unknown_0xcb39eccb, ing.unknown_0x587ca175,
                            ing.unknown_0x0bd7d5a9, static_cast< ushort >(ing.sound_SwarmMove),
                            static_cast< ushort >(ing.sound_ExitHost),
                            static_cast< ushort >(ing.sound_ExitHostSafeZone),
                            static_cast< ushort >(ing.sound_InsideHost),
                            static_cast< ushort >(ing.sound));

  const CIngBoostBallGuardianData data(
      PackBgDataFlags(ing.unknown_0x01789abd), ing.hearingRadius, ing.unknown_0x5d0d2c40, ing.unknown_0xc620183a,
      ing.frustrationTime, ing.tauntChance, ing.aggressiveness, ing.lightColor,
      ing.lightAttenuation, LdrToDamageInfo(ing.armSwipeDamage),
      LdrToDamageVulnerability(ing.ingSpotVulnerability), bodyProjectile, ingSpot, miniPortal,
      swarm);

  const auto phase = [](const SLdrIngBoostBallGuardianStruct& s) {
    return CIngBoostBallGuardianPhase(s.boostPuddleModeTime, s.unknown_0xabe99de0,
                                      s.breakBoostPuddleModeDamage, s.breakBoostModeDamage,
                                      s.nextBoostTimeMin, s.nextBoostTimeMax, s.unknown_0x285d67ad,
                                      s.unknown_0x6d5f242f, s.unknown_0xe3ff2ed6,
                                      s.unknown_0xa6fd6d54, s.unknown_0xecb314b6,
                                      s.unknown_0xa9b15734, s.locomotionSpeedScale,
                                      s.ingSpotSpeedScale);
  };

  const CBoostBallGuardianData boost(
      boostSldr.boostBallScale, boostSldr.boostBallMass, boostSldr.unknown_0xbea96fb7,
      boostSldr.boostBallMinDamageSpeed, boostSldr.unknown_0x7b21e31a, boostSldr.boostBallModel,
      boostSldr.boostBallChargeUpEffect, boostSldr.boostBallShieldEffect,
      boostSldr.boostBallTrailEffect, static_cast< ushort >(boostSldr.sound_Bounce),
      static_cast< ushort >(boostSldr.sound_IntoBall),
      static_cast< ushort >(boostSldr.sound_OutofBall),
      static_cast< ushort >(boostSldr.sound), static_cast< ushort >(boostSldr.sound_Boost),
      static_cast< ushort >(boostSldr.sound_Rolling),
      LdrToDamageVulnerability(boostSldr.boostBallVulnerability), boostSldr.unknown_0xee69b993,
      boostSldr.unknown_0x4b2de673, boostSldr.searchConeAngle, boostSldr.unknown_0xb0e85d53,
      LdrToDamageInfo(boostSldr.boostBallDamageToMorphBall),
      LdrToDamageInfo(boostSldr.boostBallDamageToFirstPerson),
      LdrToDamageInfo(boostSldr.ingSpotDamageToMorphBall),
      LdrToDamageInfo(boostSldr.ingSpotDamageToFirstPerson), boostSldr.boostIngSpotBlobEffect,
      boostSldr.boostIngSpotHitNormalDamage, boostSldr.boostIngSpotHitHeavyDamage,
      phase(boostSldr.ingBoostBallGuardianStruct),
      phase(boostSldr.ingBoostBallGuardianStruct_0xfe18a18f),
      phase(boostSldr.ingBoostBallGuardianStruct_0xc2784287));

  return rs_new CIngBoostBallGuardian(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                      LdrToEntityInfo(info, sldrThis.editorProperties),
                                      LdrToTransform4f(sldrThis.editorProperties), *modelData,
                                      LdrToActorParameters(sldrThis.actorInformation),
                                      LdrToPatternedInfo(sldrThis.patterned, nullptr), data, boost);
}

static SIngBoostBallGuardian_FuncPtrs REL_loader_IngBoostBallGuardian;

// Retail's RELMain calls fn_30_100, which stores the loader fn_30_130 into lbl_30_bss_6C (the
// module's SIngBoostBallGuardian_FuncPtrs) and calls the DOL's SetSIngBoostBallGuardian_FuncPtrs
// (fn_8022FFC4). The proxy in SIngBoostBallGuardian_FuncPtrs.cpp calls through it.
void RELMain() {
  REL_loader_IngBoostBallGuardian.mLoader = PortLoadIngBoostBallGuardian;
  SetSIngBoostBallGuardian_FuncPtrs(&REL_loader_IngBoostBallGuardian);
}

void RELExit() { SetSIngBoostBallGuardian_FuncPtrs(nullptr); }
