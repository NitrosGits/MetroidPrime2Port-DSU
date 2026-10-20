#ifndef _CINGBOOSTBALLGUARDIAN
#define _CINGBOOSTBALLGUARDIAN

#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "REL/REL_Setup.h"

#ifdef TARGET_PC
#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/SObjectTag.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CSurfaceAlignmentHelper.hpp"
#include "MetroidPrime/Enemies/CIng.hpp"
#include "MetroidPrime/Enemies/CIngSpotPathFindNavigation.hpp"
#include "MetroidPrime/PathFinding/CPathFindPointSearch.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/ScriptLoader/Structs/SLdrPlasmaBeamInfo.hpp"

class CCollisionActorManager;

// Host copies of the two parameter blocks the REL's constructor copies into the object
// (guest +0x7C0, 0x320 bytes, by fn_30_1056C; guest +0xDEC, 0x110 bytes, by fn_30_10250). The
// host layout is its own; the guest offsets are in docs/module-analysis/BoostGuardian.md. Every
// field is one property of SLdrIngBoostBallGuardian (include/MetroidPrime/ScriptLoader/
// SLdrIngBoostBallGuardian.hpp), which is why the names carry the property hashes where the game
// gives none: nothing in this milestone reads the values, they are held so the loader can build
// them. The Ing half reuses CIng's own sub-structs (CIngSpotData, CIngMiniPortalData,
// CIngBodyProjectileData, CIngSwarmData) because the REL reads the same set of properties into
// them; the grapple block and the trigger/face-plate properties CIng has are absent here.
struct CIngBoostBallGuardianData {
  CIngBoostBallGuardianData(uint unknownFlags, float hearingRadius, float unknown5D0D,
                            float unknownC620, float frustrationTime, float tauntChance,
                            float aggressiveness, const CColor& lightColor,
                            float lightAttenuation, const CDamageInfo& armSwipeDamage,
                            const CDamageVulnerability& ingSpotVulnerability,
                            const CIngBodyProjectileData& bodyProjectile,
                            const CIngSpotData& ingSpot, const CIngMiniPortalData& miniPortal,
                            const CIngSwarmData& swarm)
  : mUnknownFlags(unknownFlags)
  , mHearingRadius(hearingRadius)
  , mUnknown5D0D(unknown5D0D)
  , mUnknownC620(unknownC620)
  , mFrustrationTime(frustrationTime)
  , mTauntChance(tauntChance)
  , mAggressiveness(aggressiveness)
  , mLightColor(lightColor)
  , mLightAttenuation(lightAttenuation)
  , mArmSwipeDamage(armSwipeDamage)
  , mIngSpotVulnerability(ingSpotVulnerability)
  , mBodyProjectile(bodyProjectile)
  , mIngSpot(ingSpot)
  , mMiniPortal(miniPortal)
  , mSwarm(swarm) {}

  uint mUnknownFlags;                          // SLdrIngBoostBallGuardianData.unknown_0x01789abd
  float mHearingRadius;                        // hearingRadius
  float mUnknown5D0D;                          // unknown_0x5d0d2c40
  float mUnknownC620;                          // unknown_0xc620183a
  float mFrustrationTime;                      // frustrationTime
  float mTauntChance;                          // tauntChance
  float mAggressiveness;                       // aggressiveness
  CColor mLightColor;                          // lightColor
  float mLightAttenuation;                     // lightAttenuation
  CDamageInfo mArmSwipeDamage;                 // armSwipeDamage
  CDamageVulnerability mIngSpotVulnerability;  // ingSpotVulnerability
  CIngBodyProjectileData mBodyProjectile;      // bodyProjectile* properties
  CIngSpotData mIngSpot;                       // ingSpot* properties
  CIngMiniPortalData mMiniPortal;              // miniPortal* properties
  CIngSwarmData mSwarm;                        // pART/sRSC/swarm sound properties
};

// One of the three health phases (SLdrIngBoostBallGuardianStruct). Retail keeps three of them and
// picks the first whose threshold the current health is under - the same shape as Sandworm's
// SLdrSandwormStruct. The six ints are unnamed in the script properties; their meaning is not
// established, so they keep their hashes.
struct CIngBoostBallGuardianPhase {
  CIngBoostBallGuardianPhase(float boostPuddleModeTime, float unknownABE9,
                             float breakBoostPuddleModeDamage, float breakBoostModeDamage,
                             float nextBoostTimeMin, float nextBoostTimeMax, int unknown285D,
                             int unknown6D5F, int unknownE3FF, int unknownA6FD, int unknownECB3,
                             int unknownA9B1, float locomotionSpeedScale,
                             float ingSpotSpeedScale)
  : mBoostPuddleModeTime(boostPuddleModeTime)
  , mUnknownABE9(unknownABE9)
  , mBreakBoostPuddleModeDamage(breakBoostPuddleModeDamage)
  , mBreakBoostModeDamage(breakBoostModeDamage)
  , mNextBoostTimeMin(nextBoostTimeMin)
  , mNextBoostTimeMax(nextBoostTimeMax)
  , mUnknown285D(unknown285D)
  , mUnknown6D5F(unknown6D5F)
  , mUnknownE3FF(unknownE3FF)
  , mUnknownA6FD(unknownA6FD)
  , mUnknownECB3(unknownECB3)
  , mUnknownA9B1(unknownA9B1)
  , mLocomotionSpeedScale(locomotionSpeedScale)
  , mIngSpotSpeedScale(ingSpotSpeedScale) {}

  float mBoostPuddleModeTime;        // boostPuddleModeTime
  float mUnknownABE9;                // unknown_0xabe99de0
  float mBreakBoostPuddleModeDamage; // breakBoostPuddleModeDamage
  float mBreakBoostModeDamage;       // breakBoostModeDamage
  float mNextBoostTimeMin;           // nextBoostTimeMin
  float mNextBoostTimeMax;           // nextBoostTimeMax
  int mUnknown285D;                  // unknown_0x285d67ad
  int mUnknown6D5F;                  // unknown_0x6d5f242f
  int mUnknownE3FF;                  // unknown_0xe3ff2ed6
  int mUnknownA6FD;                  // unknown_0xa6fd6d54
  int mUnknownECB3;                  // unknown_0xecb314b6
  int mUnknownA9B1;                  // unknown_0xa9b15734
  float mLocomotionSpeedScale;       // locomotionSpeedScale
  float mIngSpotSpeedScale;          // ingSpotSpeedScale
};

// The boost-ball half (SLdrBoostBallGuardianData): what the rolling form needs, plus the three
// phases.
struct CBoostBallGuardianData {
  CBoostBallGuardianData(const CVector3f& scale, float mass, float unknownBEA9,
                         float minDamageSpeed, float unknown7B21, CAssetId model,
                         CAssetId chargeUpEffect, CAssetId shieldEffect, CAssetId trailEffect,
                         ushort soundBounce, ushort soundIntoBall, ushort soundOutOfBall,
                         ushort soundLoop, ushort soundBoost, ushort soundRolling,
                         const CDamageVulnerability& vulnerability, int unknownEE69,
                         int unknown4B2D, float searchConeAngle, float unknownB0E8,
                         const CDamageInfo& damageToMorphBall,
                         const CDamageInfo& damageToFirstPerson,
                         const CDamageInfo& ingSpotDamageToMorphBall,
                         const CDamageInfo& ingSpotDamageToFirstPerson, CAssetId blobEffect,
                         CAssetId hitNormalDamage, CAssetId hitHeavyDamage,
                         const CIngBoostBallGuardianPhase& phase0,
                         const CIngBoostBallGuardianPhase& phase1,
                         const CIngBoostBallGuardianPhase& phase2)
  : mScale(scale)
  , mMass(mass)
  , mUnknownBEA9(unknownBEA9)
  , mMinDamageSpeed(minDamageSpeed)
  , mUnknown7B21(unknown7B21)
  , mModel(model)
  , mChargeUpEffect(chargeUpEffect)
  , mShieldEffect(shieldEffect)
  , mTrailEffect(trailEffect)
  , mSoundBounce(soundBounce)
  , mSoundIntoBall(soundIntoBall)
  , mSoundOutOfBall(soundOutOfBall)
  , mSoundLoop(soundLoop)
  , mSoundBoost(soundBoost)
  , mSoundRolling(soundRolling)
  , mVulnerability(vulnerability)
  , mUnknownEE69(unknownEE69)
  , mUnknown4B2D(unknown4B2D)
  , mSearchConeAngle(searchConeAngle)
  , mUnknownB0E8(unknownB0E8)
  , mDamageToMorphBall(damageToMorphBall)
  , mDamageToFirstPerson(damageToFirstPerson)
  , mIngSpotDamageToMorphBall(ingSpotDamageToMorphBall)
  , mIngSpotDamageToFirstPerson(ingSpotDamageToFirstPerson)
  , mBlobEffect(blobEffect)
  , mHitNormalDamage(hitNormalDamage)
  , mHitHeavyDamage(hitHeavyDamage)
  , mPhase0(phase0)
  , mPhase1(phase1)
  , mPhase2(phase2) {}

  CVector3f mScale;                     // boostBallScale
  float mMass;                          // boostBallMass
  float mUnknownBEA9;                   // unknown_0xbea96fb7
  float mMinDamageSpeed;                // boostBallMinDamageSpeed
  float mUnknown7B21;                   // unknown_0x7b21e31a
  CAssetId mModel;                      // boostBallModel
  CAssetId mChargeUpEffect;             // boostBallChargeUpEffect
  CAssetId mShieldEffect;               // boostBallShieldEffect
  CAssetId mTrailEffect;                // boostBallTrailEffect
  ushort mSoundBounce;                  // sound_Bounce
  ushort mSoundIntoBall;                // sound_IntoBall
  ushort mSoundOutOfBall;               // sound_OutofBall
  ushort mSoundLoop;                    // sound (0x9f7372b3)
  ushort mSoundBoost;                   // sound_Boost
  ushort mSoundRolling;                 // sound_Rolling
  CDamageVulnerability mVulnerability;  // boostBallVulnerability
  int mUnknownEE69;                     // unknown_0xee69b993
  int mUnknown4B2D;                     // unknown_0x4b2de673
  float mSearchConeAngle;               // searchConeAngle
  float mUnknownB0E8;                   // unknown_0xb0e85d53
  CDamageInfo mDamageToMorphBall;       // boostBallDamageToMorphBall
  CDamageInfo mDamageToFirstPerson;     // boostBallDamageToFirstPerson
  CDamageInfo mIngSpotDamageToMorphBall;    // ingSpotDamageToMorphBall
  CDamageInfo mIngSpotDamageToFirstPerson;  // ingSpotDamageToFirstPerson
  CAssetId mBlobEffect;                 // boostIngSpotBlobEffect
  CAssetId mHitNormalDamage;            // boostIngSpotHitNormalDamage
  CAssetId mHitHeavyDamage;             // boostIngSpotHitHeavyDamage
  CIngBoostBallGuardianPhase mPhase0;   // ingBoostBallGuardianStruct
  CIngBoostBallGuardianPhase mPhase1;   // ingBoostBallGuardianStruct_0xfe18a18f
  CIngBoostBallGuardianPhase mPhase2;   // ingBoostBallGuardianStruct_0xc2784287
};
#endif

// The REL's CIngBoostBallGuardian (module 30): the Boost Guardian. 0x11F0 bytes on the GameCube,
// built by fn_30_EC6C, its vtable lbl_30_data_9C0 (82 words, 34 local overrides) is
// /tmp/opencode/ibbg_vtable.md and the analysis is docs/module-analysis/BoostGuardian.md.
//
// Milestone 1 (this file's first slice) is load-and-stand: the loader, the constructor, the path
// searches and the accessors the vtable declares. The state machine (fn_30_C6CC installs the 49
// triggers, 28 states and 9 codes listed in the analysis), the boost ball, the puddle form, the
// mini portals and the second class (a CPhysicsActor spawned by fn_30_3478) are not written yet,
// and every override that belongs to them forwards to CPatterned behind a once-only StandIn log.
class REL_EXPORT CIngBoostBallGuardian : public CPatterned {
public:
#ifdef TARGET_PC
  // Argument order is the REL loader's call at 0x0000051C (fn_30_130): uid, editor-property name,
  // entity info, transform, model, actor parameters, patterned info, then the two parameter blocks
  // on the stack. Retail's constructor takes one model only (loader arg r8 = the CModelData at
  // stack 0x320); the boost ball's model is an asset in the boost block, not a second CModelData.
  CIngBoostBallGuardian(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                        const CTransform4f& xf, const CModelData& modelData,
                        const CActorParameters& actParms, const CPatternedInfo& pInfo,
                        const CIngBoostBallGuardianData& data,
                        const CBoostBallGuardianData& boost);
  ~CIngBoostBallGuardian() override;

  // CPatterned accessors this REL overrides with a one-line body, so they are real, not stand-ins.
  CPathFindSearch* GetSearchPath() override { return &mPathFindSearch; }              // fn_30_0
  CPathFindPointSearch* GetPointSearchPath() override { return &mPointPathFindSearch; } // fn_30_8
  float GetGravityConstant() const override { return kDefaultGravityAccel; }          // fn_30_58
  bool IsListening() const override { return true; }                                 // fn_30_6C
  bool CanBeShot(const CStateManager&, int) override { return true; }                // fn_30_10
  CRagDoll* GetRagDoll() const override { return nullptr; }                          // fn_30_34
  TUniqueId GetAttackTarget() const override;                                        // fn_30_3C

  // CAi
  void Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) override;
  void KnockBack(CStateManager& mgr, const CKnockBackInfo& info) override;

  // CEntity / CActor
  void PreThink(float dt, CStateManager& mgr) override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;
  void PreRender(CStateManager& mgr) override;
  void Render(const CStateManager& mgr) const override;
  void CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                    CStateManager& mgr) override;
  void SetupStateMachine(CStateManager& mgr) override;
  void Touch(CActor& actor, CStateManager& mgr) override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;

private:
  void EnsureBodyActive(CStateManager& mgr);

  // Host layout (retail's object is 0x11F0; CPatterned is 0x7C0). Guest offsets are the REL's,
  // from the constructor fn_30_EC6C - the full table is in the analysis note.
  CIngBoostBallGuardianData mData;                       // guest 0x7C0 (0x320)
  int mGuestAE4;                                         // guest 0xAE4, 2
  int mGuestAE8;                                         // guest 0xAE8, -1
  CPathFindSearch mPathFindSearch;                       // guest 0xAEC
  CPathFindPointSearch mPointPathFindSearch;             // guest 0xBD8
  CCollisionActorManager* mCollisionActorManager0;       // guest 0xD60
  CCollisionActorManager* mCollisionActorManager1;       // guest 0xD64
  CSurfaceAlignmentHelper mSurfaceAlignment;             // guest 0xD68
  CIngSpotPathFindNavigation mIngSpotPathFindNavigation; // guest 0xDC0
  CBoostBallGuardianData mBoost;                         // guest 0xDEC (0x110)
  CDamageVulnerability mVulnerability;                   // guest 0x1178

#else
  uchar x7c0_[0x11f0 - 0x7c0];
#endif
public:
  // CEntity
  CEntity* TypesMatch(int typeId) const override;
};
CHECK_SIZEOF(CIngBoostBallGuardian, 0x11f0)

#endif
