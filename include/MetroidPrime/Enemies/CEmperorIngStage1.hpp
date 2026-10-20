#ifndef _CEMPERORINGSTAGE1
#define _CEMPERORINGSTAGE1

#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "REL/REL_Setup.h"

#ifdef TARGET_PC
#include "Kyoto/Audio/CSfxHandle.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/ScriptLoader/SLdrEmperorIngStage1.hpp"

class CCollisionActor;
class CCollisionActorManager;
class CParticleGenInfo;
#endif

// The REL's CEmperorIngStage1 (module 16): the Emperor Ing's first stage, the boss the player
// fights in the Great Temple. 0xD18 bytes on the GameCube (the loader's rs_new at 0x0000A4B8),
// built by fn_16_900C, its vtable lbl_16_data_3C8 (80 entries, 22 of them local) and the analysis
// is docs/module-analysis/EmperorIngStage1.md.
//
// Milestone 1 (this file's first slice) is load-and-stand: the loader, the constructor and the
// accessors the vtable gives one-line bodies to. The state machine's tables are registered
// (fn_16_8AF4 installs the 6 triggers, 12 states and 3 code functions fn_16_BF88 builds), but the
// 21 functions they point at are stand-ins that log the retail function they stand in for. The
// tentacles, the five attacks (melee, projectile, warp, beam, taunt) and the heart stage are
// milestone 2 and forward to CPatterned behind a once-only StandIn log.
//
// The host layout is its own: the retail object is 0xD18 bytes, CPatterned is 0x7C0, and the
// guest offsets of every member the constructor touches are in the comment above each one. Host
// objects outgrow retail, so the class is allocated with sizeof, not with 0xD18.
#ifdef TARGET_PC
// One tentacle slot. The retail element is 0x48 bytes and starts with the tentacle actor's
// pointer (+0x00) and its id (+0x08), which the FSM's tentacle code (fn_16_1F34) uses; the
// tentacles themselves are separate actors the state machine spawns, so only the fields the
// ported functions read are modelled here, named from their retail offsets.
struct CEmperorIngStage1Tentacle {
  float x4_;  // guest +0x04: the tentacle's timer, cleared by fn_16_1DFC
  float x18_; // guest +0x18: the retraction timer, set to stayRetractedTime by fn_16_4A04
  uchar x24_; // guest +0x24: the flags; bit 0x80 is the one fn_16_2EE0 sets
  int x28_;   // guest +0x28: 0 = retracted, 1 or 2 = out (fn_16_1DFC, fn_16_4BD4)
};

class CEmperorIngStage1;

// The module's own joint description (the retail element is 0x48 bytes; the member names are
// guesses from what the code does with them).
struct CEmperorIngStage1Joint {
  CEmperorIngStage1Joint()
  : mOwner(nullptr)
  , mRadius(0.f)
  , mIndex(0)
  , mAnimId1(0)
  , mAnimId2(0)
  , mAnimId3(0)
  , mTime(0.f)
  , mDamageHitTime(0.f)
  , mSpeed(0.f)
  , mFlags(0)
  , mState(0)
  , mId(kInvalidUniqueId)
  , mPosition(CVector3f::Zero())
  , mUp(CVector3f::Up()) {}

  CEmperorIngStage1* mOwner;  // +0x00
  float mRadius;              // +0x04
  int mIndex;                 // +0x08
  int mAnimId1;               // +0x0C
  int mAnimId2;               // +0x10
  int mAnimId3;               // +0x14
  float mTime;                // +0x18
  float mDamageHitTime;       // +0x1C
  float mSpeed;               // +0x20
  uchar mFlags;               // +0x24
  int mState;                 // +0x28
  TUniqueId mId;              // +0x2C
  CVector3f mPosition;        // +0x30
  CVector3f mUp;              // +0x3C
};

// The module's own joint vector (the retail one is a count at +0x00 and the elements at +0x04,
// 0x48 bytes each). The port's holds the eight tentacle slots.
struct CEmperorIngStage1JointVector {
  CEmperorIngStage1JointVector() : mCount(0) {}

  int mCount;                          // +0x00
  CEmperorIngStage1Joint mJoints[8];   // +0x04
};

class REL_EXPORT CEmperorIngStage1 : public CPatterned {
public:
  // Argument order is the REL loader's call at 0x0000A51C (fn_16_A2A0 -> fn_16_900C): uid,
  // editor-property name, entity info, transform, model, actor parameters, patterned info, then
  // the script's data block (the REL passes it as the constructor's ninth argument, the stack
  // slot 0x8 at 0x0000A51C).
  CEmperorIngStage1(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                    const CTransform4f& xf, const CModelData& modelData,
                    const CActorParameters& actParms, const CPatternedInfo& pInfo,
                    const SLdrEmperorIngStage1Data& data);
  ~CEmperorIngStage1() override;

  // CEntity
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void Render(const CStateManager& mgr) const override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;
  void ProcessSoundEvent(int sfxId, float weight, int flags, float fallOff, float maxDist,
                         const CSegId& locator, ushort pitchStart, ushort pitchEnd,
                         float pitchDuration, uchar minVol, uchar maxVol, float distanceSquared,
                         const CVector3f& position, int aid, CStateManager& mgr,
                         bool translateId) override;

  // CAi
  bool CanBeShot(const CStateManager&, int) override;
  bool IsListening() const override;

  // CPatterned
  void SetupStateMachine(CStateManager& mgr) override;

  // The body's state machine. Retail fn_16_8AF4 (vtable slot 58) calls the base's setup and
  // then registers the three tables fn_16_BF88 builds: 6 triggers (lbl_16_data_19C), 12 states
  // (lbl_16_data_94) and 3 codes (lbl_16_data_220). The names are retail's - the state machine
  // asset's nodes are matched by name - and each comment is the retail function the entry points
  // at. The bodies of the four triggers and the two codes that the run reaches are ported; the
  // rest are stand-ins that log the retail function they stand in for.
  //
  // Triggers (lbl_16_data_19C, 6 entries).
  bool AllTentaclesRetracted(CStateManager& mgr, const CTriggerData& data) const;  // fn_16_4BD4
  bool HeartStageOver(CStateManager& mgr, const CTriggerData& data) const;        // fn_16_4BB4
  bool IsDead(CStateManager& mgr, const CTriggerData& data) const;                 // fn_16_4B6C
  bool ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const;      // fn_16_4B20
  bool ShouldAttack(CStateManager& mgr, const CTriggerData& data) const;           // fn_16_4B04
  bool ShouldTaunt(CStateManager& mgr, const CTriggerData& data) const;           // fn_16_4AEC

  // States (lbl_16_data_94, 12 entries). Dead hides CPatterned::Dead, which is what retail
  // does too: the module's table replaces the base's "Dead" entry.
  void TrackPlayer(CStateManager& mgr, EStateMsg msg, float dt);     // fn_16_7C28
  void BeginHeartStage(CStateManager& mgr, EStateMsg msg, float dt); // fn_16_78AC
  void HeartStage(CStateManager& mgr, EStateMsg msg, float dt);       // fn_16_774C
  void EndHeartStage(CStateManager& mgr, EStateMsg msg, float dt);   // fn_16_71BC
  void Dead(CStateManager& mgr, EStateMsg msg, float dt);             // fn_16_716C
  void MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt);      // fn_16_6F28
  void Attack(CStateManager& mgr, EStateMsg msg, float dt);           // fn_16_6D94
  void ProjectileAttack(CStateManager& mgr, EStateMsg msg, float dt); // fn_16_68F4
  void WarpAttack(CStateManager& mgr, EStateMsg msg, float dt);       // fn_16_601C
  void TauntAttack(CStateManager& mgr, EStateMsg msg, float dt);      // fn_16_5DBC
  void Taunt(CStateManager& mgr, EStateMsg msg, float dt);            // fn_16_5C4C
  void JumpRope(CStateManager& mgr, EStateMsg msg, float dt);         // fn_16_5928

  // Codes (lbl_16_data_220, 3 entries).
  void BeginSubStage(CStateManager& mgr, float dt);             // fn_16_4AE8
  void ResetTentacles(CStateManager& mgr, float dt);            // fn_16_4A04
  void ChooseRandomTentacleCount(CStateManager& mgr, float dt); // fn_16_4954

private:
  void EnsureBodyActive(CStateManager& mgr);

  // fn_16_3668: every live tentacle gets its per-frame update.
  void UpdateTentacles(float dt, CStateManager& mgr);
  // fn_16_2B0C: the per-tentacle update.
  void UpdateTentacle(int index, float dt, CStateManager& mgr);
  // fn_16_26B8: the tentacle's own update - too big for the slice's rule, so a stand-in.
  void UpdateTentacleCore(int index);
  // fn_16_2524: the tentacle's motion update - too big for the slice's rule, so a stand-in.
  void UpdateTentacleMotion(int index, float dt, CStateManager& mgr);
  // fn_16_24AC: the tentacle's dark and light vulnerability effect states.
  void SetVulnerabilityEffects(int index);
  // fn_16_2070: the effect state a tentacle carries - too big for the slice's rule, so a stand-in.
  void SetTentacleEffectState(int index, const rstl::string& name);
  // fn_16_3B70: the three collision actor managers are destroyed.
  void DestroyCollisionActors(CStateManager& mgr);
  // fn_16_30C8: the tentacle animation ids.
  void SetupTentacleAnimations(CStateManager& mgr);
  // fn_16_3094: the actor connected on CONNECT/'PLAY'.
  CActor* GetPlayActor(CStateManager& mgr) const;
  // fn_16_3EAC: creates the three collision actor managers and their tentacle actors. Too big for
  // the slice's rule, so it stays a stand-in.
  void SetupTentacleCollisions(CStateManager& mgr);
  // fn_16_36E8: the tentacle joint collision descriptions.
  void SetupTentacleJoints(CStateManager& mgr);
  // fn_16_2B68: the joint description is filled in.
  void BuildTentacleJoint(int index, int animId1, int animId2, int animId3);
  // fn_16_1E6C: the tentacle actor gets the module's health info and its materials.
  void AddTentacleJoint(CStateManager& mgr);
  // fn_16_1F34: the tentacle actor's materials.
  void UpdateTentacleMaterials(CStateManager& mgr, uint flag);
  // fn_16_3984: the joint is added to the vector at guest 0x7D4.
  void AddTentacleJointToVector();
  // fn_16_39CC: the vector's growth. Too big for the slice's rule, so it stays a stand-in.
  void GrowTentacleJointVector();
  // fn_16_3AA8: the tentacle's collision actor. Too big for the slice's rule, so it stays a
  // stand-in.
  CCollisionActor* GetTentacleActor(CStateManager& mgr, int index);
  // fn_16_302C: the actor the module tracks (the id at guest 0xA1A).
  CActor* GetTargetActor(CStateManager& mgr) const;
  // fn_16_2FA8: the collision actor of the second tentacle's collision actor manager.
  CCollisionActor* GetCollisionActor(CStateManager& mgr) const;
  // fn_16_4E44 / fn_16_BDD0: the particle effect's reference and its release. The reference
  // counter lives in the retail particle effect, which the port does not have, so these stay
  // stand-ins.
  void AcquireParticleEffect(CParticleGenInfo* effect);
  void ReleaseParticleEffect(CParticleGenInfo* effect);
  // fn_16_6D10: the effect's own update.
  void UpdateParticleEffect(CParticleGenInfo* effect, float scale);
  // fn_16_51D4: the module's own sound update.
  void UpdateVoice(float dt, CStateManager& mgr);
  // fn_16_4EB8: moves the actor onto the target's locator.
  void UpdateTentacleAnchor(float dt, CStateManager& mgr);
  // fn_16_3AFC: the three collision actor managers get their update.
  void UpdateCollisionActors(float dt, CStateManager& mgr);
  // fn_16_4C0C: the module's track-player update.
  void UpdateTrackPlayer(float dt, CStateManager& mgr);
  // fn_16_4C8: aims the actor at the player.
  void UpdateFacing(float dt, CStateManager& mgr);
  // fn_16_8C: the tentacles' hit test and damage.
  void UpdateTentacleHits(float dt, CStateManager& mgr);

  // fn_16_2F70: how many tentacle slots the current sub-stage uses (4, 6 or 8).
  int TentacleCount() const;
  // fn_16_2EE0: set (or clear) the 0x80 flag of every live tentacle.
  void SetTentacleFlags(uint flag);

  // Host layout. The guest offsets are the REL's (fn_16_900C); the full member map, including
  // the parts this milestone does not build, is in docs/module-analysis/EmperorIngStage1.md.
  CHealthInfo mHealthInfo;                        // guest 0xAAC, fn_16_9AB0
  CDamageVulnerability mNormalVulnerability;      // guest 0xACC, fn_16_98D0
  CDamageVulnerability mWarpVulnerability;        // guest 0xAFC, fn_16_98D0
  CDamageVulnerability mMeleeVulnerability;       // guest 0xB2C, fn_16_98D0
  CDamageVulnerability mProjectileVulnerability;  // guest 0xB5C, fn_16_98D0
  CDamageInfo mMeleeDamage;                      // guest 0xB8C
  CDamageInfo mBeamDamage;                       // guest 0xBC4
  CDamageInfo mWarpStabDamage;                   // guest 0xC10
  float mHeartExposedTime;                       // guest 0xBA8
  float mStayRetractedTime;                      // guest 0xBB4
  float mMaxTurnSpeedNormal;                     // guest 0xBF0
  float mMaxTurnSpeedMelee;                      // guest 0xBF4
  float mTurnSpeedAccel;                         // guest 0xBFC
  float mTauntFrequency;                         // guest 0xC34
  float mAttackIntervalMin;                      // guest 0xCF4
  float mAttackIntervalMax;                      // guest 0xCF8

  // The module's own state, named from the retail offsets; the constructor's values are
  // fn_16_900C's.
  int x7c0_;                          // guest 0x7C0, 0: the mode fn_16_4A04 turns into IS6/IS7
  int x7c4_;                          // guest 0x7C4, 0: the sub-stage fn_16_2F70 reads
  CCollisionActorManager* x7c8_;      // guest 0x7C8, 0: the first tentacle's collision actors
  CCollisionActorManager* x7cc_;      // guest 0x7CC, 0: the second one's
  CCollisionActorManager* x7d0_;      // guest 0x7D0, 0: the third one's
  int x7d4_;                          // guest 0x7D4, 0: the number of live tentacles
  CEmperorIngStage1Tentacle x7d8_[8]; // guest 0x7D8: the eight tentacle slots
  TUniqueId xa18_;                    // guest 0xA18: the actor connected on IS3/ACTIVATE
  TUniqueId xa1a_;                    // guest 0xA1A: the tracked actor's id (fn_16_302C)
  TUniqueId xa1c_;                    // guest 0xA1C: the actor connected on CONNECT/ACTIVATE
  TUniqueId xa1e_;                    // guest 0xA1E: the actor connected on CONNECT/'PLAY'
  int xa20_;                          // guest 0xA20, -1: the first tentacle animation id
  int xa24_;                          // guest 0xA24, -1: the second one's
  int xa28_;                          // guest 0xA28, -1: the third one's
  float xa3c_;                        // guest 0xA3C, 0.0f: the heart-stage timer (fn_16_4BB4)
  float xa4c_;                        // guest 0xA4C, 4.0f: the attack timer (fn_16_4B04)
  float xa50_;                        // guest 0xA50, 0.0f: the taunt timer (fn_16_4AEC)
  float xa58_;                        // guest 0xA58, 0.0f: the "Damage" effect timer
  float xa5c_;                        // guest 0xA5C, 0.0f: the "Telegraph" effect timer
  float xa78_;                        // guest 0xA78, 0.0f: the effect-state timer (fn_16_56D8)
  CEmperorIngStage1Joint mJoint;      // the joint being built (fn_16_2B68)
  CEmperorIngStage1JointVector mJoints; // the joint vector at guest 0x7D4 (fn_16_3984)
  CSfxHandle xa8c_;                   // guest 0xA8C, 0: the emitter that follows the target
  TUniqueId xa90_;                    // guest 0xA90, kInvalidUniqueId: that emitter's target
};
#endif

#endif
