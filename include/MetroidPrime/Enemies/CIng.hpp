#ifndef _CING
#define _CING

#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "REL/REL_Setup.h"

#ifdef TARGET_PC
#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/SObjectTag.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CSurfaceAlignmentHelper.hpp"
#include "MetroidPrime/Enemies/CIngSpotData.hpp"
#include "MetroidPrime/Enemies/CIngSpotPathFindNavigation.hpp"
#include "MetroidPrime/PathFinding/CPathFindPointSearch.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/ScriptLoader/Structs/SLdrPlasmaBeamInfo.hpp"
#include "Kyoto/Audio/CSfxHandle.hpp"
#include "Kyoto/Math/CAABox.hpp"
#include "Kyoto/TToken.hpp"
#include "MetroidPrime/CLineOfSightTracker.hpp"
#include "rstl/optional_object.hpp"

class CGenDescription;
class CScriptAIHint;
class CPFArea;

// Host copies of the sub-structs CIngData embeds (retail: built by the Ing REL's fn_29_F12C,
// fn_29_F090, fn_29_F000 and fn_29_EFC0, copied into CIngData by its constructor fn_29_EE68 - the
// analysis is docs/module-analysis/IngFamily.md section A). The guest offsets in the comments are
// the REL's; the host layout is its own and does not need to match. Every name here is the
// analysis's, i.e. guessed from the properties the loader reads; the state machine in PortCIng.cpp
// reads the attack ranges, odds and sounds.
struct CIngBodyProjectileData {
  CIngBodyProjectileData(const CDamageInfo& contactDamage, float unknownA0D6, float suckTime,
                         float speed, float dropTime, float minAttackDist, float maxAttackDist,
                         float odds, CAssetId splatEffect, ushort soundProjectile,
                         ushort soundSplatWall)
  : mContactDamage(contactDamage)
  , mUnknownA0D6(unknownA0D6)
  , mSplatEffect(splatEffect)
  , mSoundProjectile(soundProjectile)
  , mSoundSplatWall(soundSplatWall)
  , mSuckTime(suckTime)
  , mSpeed(speed)
  , mDropTime(dropTime)
  , mMinAttackDist(minAttackDist)
  , mMaxAttackDist(maxAttackDist)
  , mOdds(odds) {}

  CDamageInfo mContactDamage; // +0x00 (0x1c) SLdrIng.bodyProjectileContactDamage
  float mUnknownA0D6;         // +0x1c SLdrIng.unknown_0xa0d63374
  CAssetId mSplatEffect;      // +0x20 SLdrIng.bodyProjectileSplatEffect
  ushort mSoundProjectile;    // +0x24 SLdrIng.sound_BodyProjectile
  ushort mSoundSplatWall;     // +0x26 SLdrIng.sound_BodyProjectileSplatWall
  // +0x28..0x3c: six floats. The builder's argument order is not the struct's store order, so
  // which slot holds which property is not established; the fields are named by property.
  float mSuckTime;     // SLdrIng.bodyProjectileSuckTime
  float mSpeed;        // SLdrIng.bodyProjectileSpeed
  float mDropTime;     // SLdrIng.bodyProjectileDropTime
  float mMinAttackDist; // SLdrIng.bodyProjectileMinAttackDist
  float mMaxAttackDist; // SLdrIng.bodyProjectileMaxAttackDist
  float mOdds;          // SLdrIng.bodyProjectileOdds
};

// Retail shape from fn_29_F090: +0 f, +4 CDamageInfo (0x1c), +0x20 f, +0x24/+0x26 sounds,
// +0x28..0x30 floats, +0x34 CDamageVulnerability (0x30) = 0x64.
struct CIngGrappleData {
  CIngGrappleData(float unknown67F6, const CDamageInfo& exitDamage, float spitForce,
                  float maxHoldTime, float postWaitTime, float pursuitRange,
                  const CDamageVulnerability& vulnerability, ushort soundExitGrapple,
                  ushort soundGrapple)
  : mUnknown67F6(unknown67F6)
  , mExitGrappleDamage(exitDamage)
  , mSpitForce(spitForce)
  , mSoundExitGrapple(soundExitGrapple)
  , mSoundGrapple(soundGrapple)
  , mMaxHoldTime(maxHoldTime)
  , mPostWaitTime(postWaitTime)
  , mPursuitRange(pursuitRange)
  , mVulnerability(vulnerability) {}

  float mUnknown67F6;               // +0x00 SLdrIng.unknown_0x67f6c10e
  CDamageInfo mExitGrappleDamage;   // +0x04 (0x1c) SLdrIng.exitGrappleDamage
  float mSpitForce;                 // +0x20 SLdrIng.exitGrappleSpitForce
  ushort mSoundExitGrapple;         // +0x24 SLdrIng.sound_ExitGrapple
  ushort mSoundGrapple;             // +0x26 SLdrIng.sound_Grapple
  float mMaxHoldTime;               // +0x28..0x30, slot [?]: maxGrappleBallHoldTime,
  float mPostWaitTime;              // postGrappleBallWaitTime and grappleBallPursuitRange
  float mPursuitRange;
  CDamageVulnerability mVulnerability; // +0x34 (0x30) SLdrIng.grappleBallVulnerability
};

// Retail shape from fn_29_F000: +0 min, +4 max, +8 effect, +0xc sound, +0x10 CDamageInfo (0x1c),
// +0x2c SLdrPlasmaBeamInfo (0x48) = 0x74.
struct CIngMiniPortalData {
  CIngMiniPortalData(float minAttackDist, float maxAttackDist, CAssetId effect, ushort sound,
                     const CDamageInfo& damage, const SLdrPlasmaBeamInfo& beamInfo)
  : mMinAttackDist(minAttackDist)
  , mMaxAttackDist(maxAttackDist)
  , mEffect(effect)
  , mSound(sound)
  , mDamage(damage)
  , mBeamInfo(beamInfo) {}

  float mMinAttackDist;              // +0x00 SLdrIng.miniPortalMinAttackDist
  float mMaxAttackDist;              // +0x04 SLdrIng.miniPortalMaxAttackDist
  CAssetId mEffect;                  // +0x08 SLdrIng.miniPortalEffect
  ushort mSound;                     // +0x0c SLdrIng.sound_MiniPortal
  CDamageInfo mDamage;               // +0x10 (0x1c) SLdrIng.miniPortalProjectileDamage
  SLdrPlasmaBeamInfo mBeamInfo;      // +0x2c (0x48) SLdrIng.miniPortalBeamInfo
};

// Retail shape from fn_29_EFC0: +0/+4/+8 assets, +0xc f, +0x10 asset, +0x14..0x1c floats,
// +0x20..0x28 five u16 sounds = 0x2a (padded to 0x2c). This is the swarm/exit-host block; the
// ctor's two optional GEN tokens at 0xce0/0xcf0 come from +0x00 and +0x10.
struct CIngSwarmData {
  CIngSwarmData(CAssetId pART, CAssetId sRSC, CAssetId pART_3DA2, float unknown2327,
                CAssetId pART_081E, float unknownCB39, float unknown587C, float unknown0BD7,
                ushort soundSwarmMove, ushort soundExitHost, ushort soundExitHostSafeZone,
                ushort soundInsideHost, ushort sound)
  : mPART(pART)
  , mSRSC(sRSC)
  , mPART3DA2(pART_3DA2)
  , mUnknown2327(unknown2327)
  , mPART081E(pART_081E)
  , mUnknownCB39(unknownCB39)
  , mUnknown587C(unknown587C)
  , mUnknown0BD7(unknown0BD7)
  , mSoundSwarmMove(soundSwarmMove)
  , mSoundExitHost(soundExitHost)
  , mSoundExitHostSafeZone(soundExitHostSafeZone)
  , mSoundInsideHost(soundInsideHost)
  , mSound(sound) {}

  CAssetId mPART;                 // +0x00 SLdrIng.pART (a GEN token in the ctor)
  CAssetId mSRSC;                 // +0x04 SLdrIng.sRSC
  CAssetId mPART3DA2;             // +0x08 SLdrIng.pART_0x3da219c7
  float mUnknown2327;             // +0x0c SLdrIng.unknown_0x23271976
  CAssetId mPART081E;             // +0x10 SLdrIng.pART_0x081e9e6c (a GEN token in the ctor)
  float mUnknownCB39;             // +0x14 SLdrIng.unknown_0xcb39eccb
  float mUnknown587C;             // +0x18 SLdrIng.unknown_0x587ca175
  float mUnknown0BD7;             // +0x1c SLdrIng.unknown_0x0bd7d5a9
  ushort mSoundSwarmMove;         // +0x20 SLdrIng.sound_SwarmMove
  ushort mSoundExitHost;          // +0x22 SLdrIng.sound_ExitHost
  ushort mSoundExitHostSafeZone;  // +0x24 SLdrIng.sound_ExitHostSafeZone
  ushort mSoundInsideHost;        // +0x26 SLdrIng.sound_InsideHost
  ushort mSound;                  // +0x28 SLdrIng.sound (0x148b81e4)
};

// Host copy of the properties the Ing loader reads (retail: CIngData, 0x214 bytes at 0x7c0, built
// by fn_29_EE68 - the constructor's stores and the loader's call fixed the argument order). The
// +0x210 flags are eight bits of SLdrIng.ingFlagsIng (fn_29_EE68 stores ingFlags' low byte there
// bit for bit). The triggers' tests give the bits their meaning (the kFlag_* enum in PortCIng.cpp; the
// table is in IngFamily.md section A.5).
class CIngData {
public:
  CIngData(uint ingFlags, float hearingRadius, const CDamageInfo& armSwipeDamage,
           const CDamageVulnerability& triggerVulnerability, const CColor& lightColor,
           const CIngBodyProjectileData& bodyProjectile, const CIngSpotData& ingSpot,
           const CIngGrappleData& grapple, const CIngMiniPortalData& miniPortal,
           const CIngSwarmData& swarm, float unknown5D0D, float unknownC620,
           float frustrationTime, float tauntChance, float aggressiveness,
           float lightAttenuation)
  : mHearingRadius(hearingRadius)
  , mUnknown5D0D(unknown5D0D)
  , mUnknownC620(unknownC620)
  , mFrustrationTime(frustrationTime)
  , mTauntChance(tauntChance)
  , mAggressiveness(aggressiveness)
  , mLightColor(lightColor)
  , mLightAttenuation(lightAttenuation)
  , mArmSwipeDamage(armSwipeDamage)
  , mTriggerVulnerability(triggerVulnerability)
  , mBodyProjectile(bodyProjectile)
  , mIngSpot(ingSpot)
  , mGrapple(grapple)
  , mMiniPortal(miniPortal)
  , mSwarm(swarm)
  , mFlags210(static_cast< uchar >(ingFlags)) {}

  float mHearingRadius;      // +0x00 SLdrIng.hearingRadius (fn_29_EE68's first float argument)
  float mUnknown5D0D;        // +0x04 SLdrIng.unknown_0x5d0d2c40
  float mUnknownC620;        // +0x08 SLdrIng.unknown_0xc620183a
  float mFrustrationTime;    // +0x0c SLdrIng.frustrationTime
  float mTauntChance;        // +0x10 SLdrIng.tauntChance
  float mAggressiveness;     // +0x14 SLdrIng.aggressiveness
  CColor mLightColor;        // +0x18 SLdrIng.lightColor
  float mLightAttenuation;   // +0x1c SLdrIng.lightAttenuation
  CDamageInfo mArmSwipeDamage; // +0x20 (0x1c) SLdrIng.armSwipeDamage
  CDamageVulnerability mTriggerVulnerability; // +0x3c (0x30) SLdrIng.triggerVulnerability
  CIngBodyProjectileData mBodyProjectile;     // +0x6c (0x40)
  CIngSpotData mIngSpot;                      // +0xac (0x60)
  CIngGrappleData mGrapple;                   // +0x10c (0x64)
  CIngMiniPortalData mMiniPortal;             // +0x170 (0x74)
  CIngSwarmData mSwarm;                       // +0x1e4 (0x2c)
  uchar mFlags210;                            // +0x210, eight bits of SLdrIng.ingFlagsIng
};
#endif

// The REL's CIng (module 29). 0xe60 bytes on the GameCube, built by fn_29_D540; its vtable
// lbl_29_data_A24 (82 words) is the map of what it overrides, and the analysis is
// docs/module-analysis/IngFamily.md section A (A.5 is the behaviour port). The state machine
// (fn_29_F2C8: 49 triggers, 29 states, 12 code functions) is registered in full; the form changes,
// the attacks and the mini-portal/exit-host states are minimal bodies marked [?].
class REL_EXPORT CIng : public CPatterned {
public:
#ifdef TARGET_PC
  // Argument order is the REL loader's call at 0x00000FF8: (uid, name, entityInfo, transform,
  // modelData, actorParameters, patternedInfo, data).
  CIng(TUniqueId uid, const rstl::string& name, const CEntityInfo& info, const CTransform4f& xf,
       const CModelData& modelData, const CActorParameters& actParms, const CPatternedInfo& pInfo,
       const CIngData& data);
  ~CIng() override;

  // CPatterned
  // The seven accessors below are the REL's own one-line bodies (vtable slot -> function), so they
  // are real rather than stand-ins.
  using CPatterned::GetSearchPath;
  CProjectileInfo* ProjectileInfo() override { return nullptr; }        // slot 55, fn_29_2C
  CPathFindSearch* GetSearchPath() override { return &mPathFindSearch; } // slot 56, fn_29_0
  CPathFindPointSearch* GetPointSearchPath() override {                  // slot 57, fn_29_8
    return &mPointPathFindSearch;
  }
  CRagDoll* GetRagDoll() const override { return nullptr; }             // slot 61, fn_29_34
  TUniqueId GetAttackTarget() const override { return kInvalidUniqueId; } // slot 67, fn_29_3C

  // CAi
  bool CanBeShot(const CStateManager&, int) override { return true; }   // slot 42, fn_29_6C
  bool IsListening() const override { return true; }                    // slot 43, fn_29_10
  bool Listen(CStateManager& mgr, const CVector3f& position, EListenNoiseType type) override;
  void Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) override;
  void KnockBack(CStateManager& mgr, const CKnockBackInfo& info) override;
  void TakeDamage(const CVector3f& direction, float magnitude) override;

  // CActor
  using CPatterned::GetDamageVulnerability;
  const CDamageVulnerability* GetDamageVulnerability() const override; // fn_29_BF20
  rstl::optional_object< CAABox > GetTouchBounds() const override;   // fn_29_B970

  // CEntity / CActor
  void PreThink(float dt, CStateManager& mgr) override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;
  void PreRender(CStateManager& mgr) override;
  void Render(const CStateManager& mgr) const override;
  void Touch(CActor& actor, CStateManager& mgr) override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;
  void SetupStateMachine(CStateManager& mgr) override;

  // CEntity
  CEntity* TypesMatch(int typeId) const override;

  // Triggers (fn_29_F2C8's table order).
  bool StateOver(CStateManager& mgr, const CTriggerData& data) const;
  bool IsAlert(CStateManager& mgr, const CTriggerData& data) const;
  bool IsControllingHost(CStateManager& mgr, const CTriggerData& data) const;
  bool IsFacingTarget(CStateManager& mgr, const CTriggerData& data) const;
  bool HasTarget(CStateManager& mgr, const CTriggerData& data) const;
  bool HasNewTarget(CStateManager& mgr, const CTriggerData& data) const;
  bool HasLineOfSight(CStateManager& mgr, const CTriggerData& data) const;
  bool UnderFire(CStateManager& mgr, const CTriggerData& data) const;
  bool HeardShot(CStateManager& mgr, const CTriggerData& data) const;
  bool AreaClear(CStateManager& mgr, const CTriggerData& data) const;
  bool CoverLeash(CStateManager& mgr, const CTriggerData& data) const;
  bool HasCoverPoint(CStateManager& mgr, const CTriggerData& data) const;
  bool HasWallCoverPoint(CStateManager& mgr, const CTriggerData& data) const;
  bool HasMiniPortals(CStateManager& mgr, const CTriggerData& data) const;
  bool IsCorporeal(CStateManager& mgr, const CTriggerData& data) const;
  bool IsIngSpot(CStateManager& mgr, const CTriggerData& data) const;
  bool IsBodyProjectile(CStateManager& mgr, const CTriggerData& data) const;
  bool IsFrustrated(CStateManager& mgr, const CTriggerData& data) const;
  bool IsAggressive(CStateManager& mgr, const CTriggerData& data) const;
  bool IsLuredBySafeZone(CStateManager& mgr, const CTriggerData& data) const;
  bool StillLuredBySafeZone(CStateManager& mgr, const CTriggerData& data) const;
  bool UseProjectileFSMEntry(CStateManager& mgr, const CTriggerData& data) const;
  bool TargetInSafeZone(CStateManager& mgr, const CTriggerData& data) const;
  bool TargetInLightSafeZone(CStateManager& mgr, const CTriggerData& data) const;
  bool EmergePtInSafeZone(CStateManager& mgr, const CTriggerData& data) const;
  bool CanChangeForm(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldEvaporate(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldFleeSafeZone(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldBecomeCorporeal(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldBecomeIngSpot(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldArmSwipe(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldTaunt(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldBodyProjectile(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldOpenMiniPortal(CStateManager& mgr, const CTriggerData& data) const;
  bool ProjectileSplat(CStateManager& mgr, const CTriggerData& data) const;
  bool ProjectileHitTarget(CStateManager& mgr, const CTriggerData& data) const;
  bool FoundMovementPos(CStateManager& mgr, const CTriggerData& data) const;
  bool PathShagged(CStateManager& mgr, const CTriggerData& data) const;
  bool PathOver(CStateManager& mgr, const CTriggerData& data) const;
  bool PointPathOver(CStateManager& mgr, const CTriggerData& data) const;
  bool FoundPointPath(CStateManager& mgr, const CTriggerData& data) const;
  bool IsOffPath(CStateManager& mgr, const CTriggerData& data) const;
  bool HasAttackPattern(CStateManager& mgr, const CTriggerData& data) const;
  bool AttackPatternOver(CStateManager& mgr, const CTriggerData& data) const;
  bool TargetIsBall(CStateManager& mgr, const CTriggerData& data) const;
  bool InBallPursuitRange(CStateManager& mgr, const CTriggerData& data) const;
  bool HasPathToTarget(CStateManager& mgr, const CTriggerData& data) const;
  bool InGrappleRange(CStateManager& mgr, const CTriggerData& data) const;
  bool CanGrappleTarget(CStateManager& mgr, const CTriggerData& data) const;

  // States.
  void Start(CStateManager& mgr, EStateMsg msg, float dt);
  void WaitForFSMTransition(CStateManager& mgr, EStateMsg msg, float dt);
  void ExitHost(CStateManager& mgr, EStateMsg msg, float dt);
  void BecomeCorporeal(CStateManager& mgr, EStateMsg msg, float dt);
  void BecomeIngSpot(CStateManager& mgr, EStateMsg msg, float dt);
  void BecomeBodyProjectile(CStateManager& mgr, EStateMsg msg, float dt);
  void BecomeWallProjectile(CStateManager& mgr, EStateMsg msg, float dt);
  void Alert(CStateManager& mgr, EStateMsg msg, float dt);
  void SafeZoneReaction(CStateManager& mgr, EStateMsg msg, float dt);
  void Taunt(CStateManager& mgr, EStateMsg msg, float dt);
  void Lurk(CStateManager& mgr, EStateMsg msg, float dt);
  void PathFind(CStateManager& mgr, EStateMsg msg, float dt);
  void IngSpotPathFind(CStateManager& mgr, EStateMsg msg, float dt);
  void IngSpotPointPathFind(CStateManager& mgr, EStateMsg msg, float dt);
  void Patrol(CStateManager& mgr, EStateMsg msg, float dt);
  void FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float dt);
  void BodyProjectileFlight(CStateManager& mgr, EStateMsg msg, float dt);
  void SuckEnergy(CStateManager& mgr, EStateMsg msg, float dt);
  void SeekWallPoint(CStateManager& mgr, EStateMsg msg, float dt);
  void SelectTarget(CStateManager& mgr, EStateMsg msg, float dt);
  void FaceTarget(CStateManager& mgr, EStateMsg msg, float dt);
  void FindCoverPoint(CStateManager& mgr, EStateMsg msg, float dt);
  void ArmSwipe(CStateManager& mgr, EStateMsg msg, float dt);
  void FailSafeMode(CStateManager& mgr, EStateMsg msg, float dt);
  void GrappleBall(CStateManager& mgr, EStateMsg msg, float dt);
  void FindMiniPortals(CStateManager& mgr, EStateMsg msg, float dt);
  void MiniPortalAttack(CStateManager& mgr, EStateMsg msg, float dt);
  void Dead(CStateManager& mgr, EStateMsg msg, float dt);
  void Evaporate(CStateManager& mgr, EStateMsg msg, float dt);

  // Code functions.
  void SetTargetDest(CStateManager& mgr, float dt);
  void SetCoverDest(CStateManager& mgr, float dt);
  void SetRecoverDest(CStateManager& mgr, float dt);
  void SetPointCoverDest(CStateManager& mgr, float dt);
  void SetWallPointCoverDest(CStateManager& mgr, float dt);
  void SetExitHostDest(CStateManager& mgr, float dt);
  void SetLuredDest(CStateManager& mgr, float dt);
  void SetLuredPointDest(CStateManager& mgr, float dt);
  void StartRangedAttack(CStateManager& mgr, float dt);
  void EndRangedAttack(CStateManager& mgr, float dt);
  void SplatOntoMesh(CStateManager& mgr, float dt);
  void ExpelGrappledBall(CStateManager& mgr, float dt);

private:
  void EnsureBodyActive(CStateManager& mgr);
  void LogState(const char* name);
  void JoinTeam(CStateManager& mgr);
  void MoveOnSurface(const CVector3f& move, float speed, float dt);
  void ApplySeparation(CStateManager& mgr);
  float DistSqToTarget(const CStateManager& mgr) const;
  CScriptAIHint* GetCoverHint(CStateManager& mgr, TUniqueId id) const;
  void ReleaseCoverHint(CStateManager& mgr, TUniqueId& id);
  void UpdateTimers(float dt, CStateManager& mgr);
  void UpdateValidTarget(CStateManager& mgr);
  void UpdateLoopSfx();
  void StopSfx();
  // fn_29_1A80: turn horizontally to the lure safe zone when one is set, else to the target.
  void FaceLureOrTarget(CStateManager& mgr);

  // Host layout (retail's CIng is 0xe60, CHECK_SIZEOF below; CPatterned is 0x7c0). Guest offsets
  // are the REL's, from the constructor fn_29_D540 and the tiny accessors fn_29_0 (this+0x9dc),
  // fn_29_8 (this+0xac8), the DOL's fn_801F8258 (this+0xcb0) and fn_801F7CA0 (this+0xc58).
  CIngData mData;                            // 0x7c0 (0x214)
  CPathFindSearch mPathFindSearch;           // 0x9dc (0xec)
  CPathFindPointSearch mPointPathFindSearch; // 0xac8 (0x188)
  CSurfaceAlignmentHelper mSurfaceAlignment; // 0xc58 (0x58)
  CIngSpotPathFindNavigation mIngSpotPathFindNavigation; // 0xcb0 (0x10)
  int mForm;                                 // 0x9d4 (2): 2 corporeal, 3 ing spot, 4 body proj, 5 evaporating, 6/7 the form changes' own forms
  int mDesiredForm;                          // 0x9d8 (-1)
  rstl::optional_object< CAABox > mTouchBox; // 0xcc0 (0xcd8 = engaged)
  TUniqueId mHostId;                         // 0xcdc
  TUniqueId mBlobId;                         // 0xd5c [?] never spawned yet
  TUniqueId mTeamId;                         // 0xd6a
  TUniqueId mTargetId;                       // 0xd6c
  TUniqueId mPrevTargetId;                   // 0xd6e
  TUniqueId mAttackPathResumeId;             // 0xd70
  TUniqueId mCoverHintId;                    // 0xd72
  TUniqueId mLastCoverHintId;                // 0xd74
  TUniqueId mD76Id;                          // 0xd76, cleared once its object is gone
  TUniqueId mLureSafeZoneId;                 // 0xd78
  rstl::optional_object< TLockedToken< CGenDescription > > mDeathEffect; // 0xd28
  CSfxHandle mLoopSfx[2];                    // 0xd60/0xd64: idle, move
  CSegId mEyeSegId;                          // 0xd7b: the "head" locator, the ctor copies it to 0xd8a
  CLineOfSightTracker mLineOfSight;          // 0xd88 (its segment is the byte at 0xd8a)
  CVector3f mTargetPos;                      // 0xdc8
  CVector3f mSurfaceMoveDir;                 // 0xde0
  CVector3f mPortalPlaneNormal;              // 0xdf0: the portal plane's normal, Up() = (0,0,1)
  float mPortalPlaneConstant;                // 0xdfc: dot(translation, normal) = translation.z
  CVector3f mModelScale;                     // host copy of the model data's scale
  int mMiniPortalCount;                      // 0xe00
  int mSafeZoneCount;                        // 0xe2c
  float mHeardShotTime;                      // 0xe30
  float mUnderFireTime;                      // 0xe34
  float mArmSwipeTime;                       // 0xe38
  float mFormTime;                           // 0xe3c
  float mBodyState5Time;                     // 0xe40
  float mFrustrationTimer;                   // 0xe44
  float mBallCooldown;                       // 0xe48
  float mLightBlend;                         // 0xe50
  float mDeadFade;                           // 0xe54
  float mBodyProjectileTime;                 // 0xe58
  bool mEvaporate : 1;                       // 0xe5c bit 7
  bool mTaunt : 1;                           // 0xe5c bit 6
  bool mBodyProjectileAllowed : 1;           // 0xe5c bit 5
  bool mAggressive : 1;                      // 0xe5c bit 4
  bool mFoundMovementPos : 1;                // 0xe5c bit 3
  bool mRecalcPath : 1;                      // 0xe5c bit 2
  bool mRenderModel : 1;                     // 0xe5d bit 7
  bool mPortalPlane : 1;                     // 0xe5d bit 6
  bool mStartAsIngSpot : 1;                  // 0xe5d bit 5 (data bit 7)
  bool mForm9Render : 1;                     // 0xe5d bit 4
  bool mFollowingAttackPath : 1;             // 0xe5d bit 3
  bool mAlert : 1;                           // 0xe5d bit 2
  bool mMoving : 1;                          // 0xe5d bit 1
  bool mGrappled : 1;                        // 0xe5d bit 0
  bool mProjectileFSMEntry : 1;              // 0xe5e bit 7
  bool mSplat : 1;                           // 0xe5e bit 6
  bool mSafeZoneIngSpot : 1;                 // 0xe5e bit 5
  bool mHurtfulZone : 1;                     // 0xe5e bit 4
  const char* mLastLoggedState;              // host: MP2_DEBUG_ING
  float mStateTime;                          // host: time in the current state [?] retail's source
  CPFArea* mPathArea;                        // host: the area both searches were given on XALD
  // Not held: the CCollisionActorManager* at 0xc50 (fn_29_3190 builds it from seven joint spheres;
  // not ported), the other optional PART/GEN tokens at 0xce0..0xd58, the nine CSegId locators at
  // 0xd7a/0xd7c..0xd83 ("Collar_SDK", the shoulders/forearms/wrists/elbows) and the light at 0xd68.
#else
  uchar x7c0_[0x6a0]; // CIngData, the path searches, the seg ids, the tokens and the flags
#endif
};
CHECK_SIZEOF(CIng, 0xe60)

#endif
