#ifndef _CDARKSAMUS
#define _CDARKSAMUS

#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "REL/REL_Setup.h"

#ifdef TARGET_PC
#include "MetroidPrime/CAnimationParameters.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/ScriptLoader/Structs/SLdrAudioPlaybackParms.hpp"

// The fight script's object (the guest's CScriptDarkSamusBattleStage), whose stage info the
// ShouldFadeOut and CanMissileDeflect triggers read.
class CDarkSamusBattleStage;

// Host copy of the properties the DarkSamus loader reads (fn_10_CF44 reads the module's 63
// property ids, which are exactly the ids of the generated SLdrDarkSamus reader). Retail keeps no
// single block of them: the loader hands them to the constructor (fn_10_1FA14) as eight floats
// plus about fifty stack arguments, and the constructor spreads them over the sub-objects it
// builds (0x94C, 0x9A4, 0x9E0, 0xA0C, 0xA4C, 0xA88, 0xAC0, 0xB50, 0xCD0, 0xD74, 0xDF0, 0xE10,
// 0xEF0, 0xF68, 0xF80, 0x1064). The host therefore keeps them in one struct, one member per
// property (60 of the 63 ids: the editor properties, the patterned AI typedef and the actor
// parameters are the three the loader passes on separately), named after the property it holds.
// Nothing in milestone 1 reads the values: the class
// holds them for the state machine the later milestones add. The guest offsets the constructor
// writes them to are in docs/module-analysis/DarkSamus.md.
struct CDarkSamusData {
  CDarkSamusData()
  : mUnknown72EDEB7D(-1.f)
  , mUnknown74FA22F0(-1.f)
  , mGlideSound(0)
  , mMissileRicochetSound(0)
  , mStartsInTheAir(false)
  , mTXTR(kInvalidAssetId)
  , mUnknown2C6A3344(100.f)
  , mMeleeAttackDamage()
  , mMeleeAttackFX(kInvalidAssetId)
  , mPart(kInvalidAssetId)
  , mDiveAttackDamage()
  , mDiveAttackImpulseVertical(40.f)
  , mDiveAttackImpulseHorizontal(500.f)
  , mDiveAttackEffect(kInvalidAssetId)
  , mScatterShotProjectile(kInvalidAssetId)
  , mScatterShotProjectile2(kInvalidAssetId)
  , mScatterShotDamage()
  , mUnknown378285B9(0.f)
  , mScatterShotProjectilesPerSecond(300.f)
  , mNormalMissileProjectile(kInvalidAssetId)
  , mNormalMissileDamage()
  , mUnknown1F1EF7A9(1)
  , mUnknownC4A1A44E(3)
  , mSuperMissileProjectile(kInvalidAssetId)
  , mSuperMissileDamage()
  , mFreezeBeamProjectile(kInvalidAssetId)
  , mFreezeBeamDamage()
  , mFreezeBeamVisorTexture(kInvalidAssetId)
  , mDamageInterruptThreshold(10.f)
  , mUnknownF317F4D5(4.f)
  , mSweepSwoosh(kInvalidAssetId)
  , mSweepBeamDamage()
  , mSweepBeamCollisionEffect(kInvalidAssetId)
  , mSweepBeamSound(0)
  , mUnknown0EF8DC15(-1)
  , mInvulnerableModel(kInvalidAssetId)
  , mInvulnerableSkinRules(kInvalidAssetId)
  , mBoostBallModel(CAssetId(0), 0, 0)
  , mBoostBallDamage()
  , mBoostBallGlow(kInvalidAssetId)
  , mBoostBallTrailSwoosh(kInvalidAssetId)
  , mSWHC(kInvalidAssetId)
  , mBoostBallRollSound(0)
  , mBoostBallHitPlayerSound(0)
  , mBoostBallCollision(kInvalidAssetId)
  , mBoostBallCollisionSoundOneShot()
  , mIceSpreadFX(kInvalidAssetId)
  , mIceSpreadSound(0)
  , mPart908B06E9(kInvalidAssetId)
  , mPart494DE4A4(kInvalidAssetId)
  , mSound(0)
  , mDamageInfo()
  , mPartE701DAEA(kInvalidAssetId)
  , mPhazonProjectile(kInvalidAssetId)
  , mPhazonSuperMissileProjectile(kInvalidAssetId)
  , mPhazonSuperMissileDamage()
  , mPhazonProjectileDamage()
  , mPhazonEnrageSphere(kInvalidAssetId)
  , mPhazonEnrageSphereContactDamage()
  , mAlternateScannableInfo(kInvalidAssetId) {}

  float mUnknown72EDEB7D;      // unknown_0x72edeb7d
  float mUnknown74FA22F0;      // unknown_0x74fa22f0
  int mGlideSound;             // glideSound
  int mMissileRicochetSound;   // missileRicochetSound
  bool mStartsInTheAir;        // startsInTheAir
  CAssetId mTXTR;              // tXTR
  float mUnknown2C6A3344;      // unknown_0x2c6a3344
  CDamageInfo mMeleeAttackDamage;    // meleeAttackDamage
  CAssetId mMeleeAttackFX;     // meleeAttackFX
  CAssetId mPart;              // pART
  CDamageInfo mDiveAttackDamage;      // diveAttackDamage
  float mDiveAttackImpulseVertical;   // diveAttackImpulseVertical
  float mDiveAttackImpulseHorizontal; // diveAttackImpulseHorizontal
  CAssetId mDiveAttackEffect;  // diveAttackEffect
  CAssetId mScatterShotProjectile;    // scatterShotProjectile
  CAssetId mScatterShotProjectile2;   // scatterShotProjectile2
  CDamageInfo mScatterShotDamage;     // scatterShotDamage
  float mUnknown378285B9;      // unknown_0x378285b9
  float mScatterShotProjectilesPerSecond; // scatterShotProjectilesPerSecond
  CAssetId mNormalMissileProjectile;   // normalMissileProjectile
  CDamageInfo mNormalMissileDamage;    // normalMissileDamage
  int mUnknown1F1EF7A9;        // unknown_0x1f1ef7a9
  int mUnknownC4A1A44E;        // unknown_0xc4a1a44e
  CAssetId mSuperMissileProjectile;    // superMissileProjectile
  CDamageInfo mSuperMissileDamage;     // superMissileDamage
  CAssetId mFreezeBeamProjectile;      // freezeBeamProjectile
  CDamageInfo mFreezeBeamDamage;       // freezeBeamDamage
  CAssetId mFreezeBeamVisorTexture;    // freezeBeamVisorTexture
  float mDamageInterruptThreshold;     // damageInterruptThreshold
  float mUnknownF317F4D5;      // unknown_0xf317f4d5
  CAssetId mSweepSwoosh;       // sweepSwoosh
  CDamageInfo mSweepBeamDamage;        // sweepBeamDamage
  CAssetId mSweepBeamCollisionEffect;  // sweepBeamCollisionEffect
  int mSweepBeamSound;         // sweepBeamSound
  int mUnknown0EF8DC15;        // unknown_0x0ef8dc15
  CAssetId mInvulnerableModel; // invulnerableModel
  CAssetId mInvulnerableSkinRules; // invulnerableSkinRules
  CAnimationParameters mBoostBallModel; // boostBallModel (an SLdrAnimationSet)
  CDamageInfo mBoostBallDamage;        // boostBallDamage
  CAssetId mBoostBallGlow;      // boostBallGlow
  CAssetId mBoostBallTrailSwoosh; // boostBallTrailSwoosh
  CAssetId mSWHC;              // sWHC
  int mBoostBallRollSound;     // boostBallRollSound
  int mBoostBallHitPlayerSound; // boostBallHitPlayerSound
  CAssetId mBoostBallCollision; // boostBallCollision
  SLdrAudioPlaybackParms mBoostBallCollisionSoundOneShot; // boostBallCollisionSound_OneShot
  CAssetId mIceSpreadFX;       // iceSpreadFX
  int mIceSpreadSound;         // iceSpreadSound
  CAssetId mPart908B06E9;      // pART_0x908b06e9
  CAssetId mPart494DE4A4;      // pART_0x494de4a4
  int mSound;                  // sound
  CDamageInfo mDamageInfo;     // damageInfo
  CAssetId mPartE701DAEA;      // pART_0xe701daea
  CAssetId mPhazonProjectile;  // phazonProjectile
  CAssetId mPhazonSuperMissileProjectile; // phazonSuperMissileProjectile
  CDamageInfo mPhazonSuperMissileDamage;  // phazonSuperMissileDamage
  CDamageInfo mPhazonProjectileDamage;    // phazonProjectileDamage
  CAssetId mPhazonEnrageSphere;          // phazonEnrageSphere
  CDamageInfo mPhazonEnrageSphereContactDamage; // phazonEnrageSphereContactDamage
  CAssetId mAlternateScannableInfo;       // alternateScannableInfo
};
#endif

// The REL's CDarkSamus (module 10): the dark Samus boss. 0x1128 bytes on the GameCube (the
// loader's __nw__FUlPCcPCc at 0x0000DA78), built by fn_10_1FA14; its vtable lbl_10_data_CBC
// (174 words, 32 local overrides) and the analysis are docs/module-analysis/DarkSamus.md.
//
// Milestone 1 (this file's first slice) is load-and-think: the loader, the constructor and the
// accessors the vtable gives one-line bodies, so the actor registers, takes kSM_Create and thinks.
// The state machine, the melee / dive / scatter-shot / missile attacks, the boost ball, the
// freeze beam, the sweep beam and the phazon enrage are later milestones, and every override that
// belongs to them forwards to CPatterned behind a once-only stand-in log.
class REL_EXPORT CDarkSamus : public CPatterned {
public:
#ifdef TARGET_PC
  // Argument order is the REL loader's call at 0x0000DD84 (fn_10_CF44): (uid, name, entity info,
  // transform, model, patterned info, then the properties).
  CDarkSamus(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
             const CTransform4f& xf, const CModelData& modelData, const CPatternedInfo& pInfo,
             const CActorParameters& actParms, const CDarkSamusData& data);
  ~CDarkSamus() override;

  // CEntity / CActor. PreThink is not overridden: the REL's vtable keeps the base-class symbol in
  // slot 4 (CPatterned::PreThink).
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  const CHealthInfo* GetHealthInfo() const override;
  const CDamageVulnerability* GetDamageVulnerability() const override;
  rstl::optional_object< CAABox > GetTouchBounds() const override;
  CVector3f GetOrbitPosition(const CStateManager& mgr) const override;
  CVector3f GetAimPosition(const CStateManager& mgr, float dt) const override;
  EWeaponCollisionResponseTypes GetCollisionResponseType(const CVector3f& position,
                                                         const CVector3f& direction,
                                                         const CWeaponMode& mode,
                                                         int attributes) const override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;
  CScannableObjectInfo* GetScannableObjectInfo() const override;

  // CPhysicsActor
  const CCollisionPrimitive* GetCollisionPrimitive() const override;
  void CollidedWith(const TUniqueId& id, const CCollisionInfoList& list, CStateManager& mgr) override;

  // CAi
  CDamageVulnerability* DamageVulnerability() override;
  void TakeDamage(const CVector3f& direction, float magnitude) override;
  // The three one-line accessors below are the REL's own bodies, so they are real, not stand-ins.
  bool CanBeShot(const CStateManager&, int) override { return true; }   // slot 42, fn_10_CE9C
  bool IsListening() const override { return false; }                  // slot 43, fn_10_CEA4
  bool Listen(CStateManager&, const CVector3f&, EListenNoiseType) override { // slot 44, fn_10_CEAC
    return false;
  }
  CVector3f GetOrigin(const CStateManager&, const CTeamAiRole&,
                      const CVector3f&) const override; // slot 45, fn_10_CEB4

  // CPatterned. The accessors below are the REL's own one-line bodies (the module's .text starts
  // with them), so they are real, not stand-ins.
  // slot 50, fn_10_CE54: the byte at guest this+0x44F, the base's model colour's alpha.
  uchar GetModelAlphau8(const CStateManager&) const override { return mColor.GetAlphau8(); }
  CProjectileInfo* ProjectileInfo() override;                                             // 55
  using CPatterned::GetSearchPath;
  CPathFindSearch* GetSearchPath() override { return &mPathFindSearch; }                    // 56
  using CPatterned::GetPointSearchPath;
  CPathFindPointSearch* GetPointSearchPath() override { return nullptr; }                   // 57
  CRagDoll* GetRagDoll() const override { return nullptr; }                                 // 61
  TUniqueId GetAttackTarget() const override { return kInvalidUniqueId; }                    // 67
  bool IsOnGround() const override { return CPatterned::IsOnGround(); }                     // 68
  float GetGravityConstant() const override { return kDefaultGravityAccel; }                // 69
  const rstl::optional_object< TCachedToken< CGenDescription > >&
  GetDeathExplosionParticle() const override;                                              // 73

  // ---------------------------------------------------------------------------------------------
  // The state machine. fn_10_18D78 (the module's own setup, called from the first Think) hands the
  // machine two tables: lbl_10_data_450 with 49 entries (the triggers) and lbl_10_data_964 with 43
  // entries (the states). The names are the REL's own strings and the order is the table order;
  // the machine finds a node's handler by name. The comment on each name is the REL's function that
  // implements it (the .data record beside the entry holds its offset in lbl_10_data_CBC).
  void SetupStateMachine(CStateManager& mgr) override;
  void InstallStateMachineFunctions();

  // The 49 triggers (lbl_10_data_450's order).

  bool Alerted(CStateManager& mgr, const CTriggerData& data) const; // fn_10_F840
  bool Arrived(CStateManager& mgr, const CTriggerData& data) const; // fn_10_62B4
  bool AnyIncomingMissiles(CStateManager& mgr, const CTriggerData& data) const; // fn_10_1BB08
  bool BreakFreezeBeam(CStateManager& mgr, const CTriggerData& data) const; // fn_10_16B48
  bool BreakDash(CStateManager& mgr, const CTriggerData& data) const; // fn_10_149B4
  bool BreakDiveAttack(CStateManager& mgr, const CTriggerData& data) const; // fn_10_11C54
  bool BreakNormalMissile(CStateManager& mgr, const CTriggerData& data) const; // fn_10_16B5C
  bool BreakPause(CStateManager& mgr, const CTriggerData& data) const; // fn_10_106F8
  bool BreakPhazonAttack(CStateManager& mgr, const CTriggerData& data) const; // fn_10_4ABC
  bool BreakSuperMissile(CStateManager& mgr, const CTriggerData& data) const; // fn_10_16B54
  bool CanMeleeAttack(CStateManager& mgr, const CTriggerData& data) const; // fn_10_10634
  bool CanMissileDeflect(CStateManager& mgr, const CTriggerData& data) const; // fn_10_14284
  bool DiveAfterBoost(CStateManager& mgr, const CTriggerData& data) const; // fn_10_95A8
  bool DoneRolling(CStateManager& mgr, const CTriggerData& data) const; // fn_10_9314
  bool DoneTurning(CStateManager& mgr, const CTriggerData& data) const; // fn_10_F864
  bool FacingPlayer(CStateManager& mgr, const CTriggerData& data) const; // fn_10_142CC
  bool HitDuringEnrage(CStateManager& mgr, const CTriggerData& data) const; // fn_10_7B68
  bool OutOfMeleeRange(CStateManager& mgr, const CTriggerData& data) const; // fn_10_17CB8
  bool PickedBoostBall(CStateManager& mgr, const CTriggerData& data) const; // fn_10_116F8
  bool PickedFreezeBeam(CStateManager& mgr, const CTriggerData& data) const; // fn_10_11760
  bool PickedDiveAttack(CStateManager& mgr, const CTriggerData& data) const; // fn_10_1174C
  bool PickedMelee(CStateManager& mgr, const CTriggerData& data) const; // fn_10_117D8
  bool PickedMeleeDash(CStateManager& mgr, const CTriggerData& data) const; // fn_10_11710
  bool PickedMissileJump(CStateManager& mgr, const CTriggerData& data) const; // fn_10_1178C
  bool PickedMovingSShot(CStateManager& mgr, const CTriggerData& data) const; // fn_10_11724
  bool PickedNormalMissile(CStateManager& mgr, const CTriggerData& data) const; // fn_10_117A0
  bool PickedPause(CStateManager& mgr, const CTriggerData& data) const; // fn_10_11800
  bool PickedPhazonAttack(CStateManager& mgr, const CTriggerData& data) const; // fn_10_8680
  bool PickedPhazonEnrage(CStateManager& mgr, const CTriggerData& data) const; // fn_10_8114
  bool PickedScatterShot(CStateManager& mgr, const CTriggerData& data) const; // fn_10_11738
  bool PickedSuperMissile(CStateManager& mgr, const CTriggerData& data) const; // fn_10_11778
  bool PickedSweepBeam(CStateManager& mgr, const CTriggerData& data) const; // fn_10_CDA0
  bool PickedTaunt(CStateManager& mgr, const CTriggerData& data) const; // fn_10_117EC
  bool PlayerIsMorphball(CStateManager& mgr, const CTriggerData& data) const; // fn_10_112E8
  bool PlayerTooClose(CStateManager& mgr, const CTriggerData& data) const; // fn_10_10658
  bool ReachedPissedOff(CStateManager& mgr, const CTriggerData& data) const; // fn_10_FAF4
  bool ReadyForMissiles(CStateManager& mgr, const CTriggerData& data) const; // fn_10_6A40
  bool ShouldEnragedMelee(CStateManager& mgr, const CTriggerData& data) const; // fn_10_6B78
  bool ShouldFadeIn(CStateManager& mgr, const CTriggerData& data) const; // fn_10_BB78
  bool ShouldFadeOut(CStateManager& mgr, const CTriggerData& data) const; // fn_10_BBE4
  bool ShouldGetPissedOff(CStateManager& mgr, const CTriggerData& data) const; // fn_10_FD30
  bool ShouldLeaveSphere(CStateManager& mgr, const CTriggerData& data) const; // fn_10_6A60
  bool ShouldReact(CStateManager& mgr, const CTriggerData& data) const; // fn_10_11158
  bool ShouldRiseAgain(CStateManager& mgr, const CTriggerData& data) const; // fn_10_6AE8
  bool SkipPhazonAttack(CStateManager& mgr, const CTriggerData& data) const; // fn_10_110E0
  bool StartsInTheAir(CStateManager& mgr, const CTriggerData& data) const; // fn_10_F924
  bool TooMuchDamage(CStateManager& mgr, const CTriggerData& data) const; // fn_10_10698
  bool WallCollision(CStateManager& mgr, const CTriggerData& data) const; // fn_10_F800
  bool WasHeavyHit(CStateManager& mgr, const CTriggerData& data) const; // fn_10_112AC

  // The 43 states (lbl_10_data_964's order). Start, Patrol and Dead hide CPatterned's own.
  void BoostBallFire(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_9724
  void BoostBallInto(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_B164
  void BoostBallRoll(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_A3B0
  void FreezeBeam(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_16CDC
  void Dash(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_150DC
  void DiveAttack(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_11FA4
  void DiveFromBoost(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_9424
  void EnrageHitReact(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_7A14
  void EnragedTravel(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_6E38
  void FadeIn(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_BD40
  void FadeOut(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_BC54
  void FallFromTheAir(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_F86C
  void GetUp(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_18EAC
  void GoToPhazonAttack(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_652C
  void GoToPhazonEnrage(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_6460
  void HeavyHit(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_10C9C
  void Interrupt(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_10FF4
  void MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_17BC0
  void MeleeDash(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_17ABC
  void MissileDeflect(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_178E4
  void MissileJump(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_177BC
  void MovingScatterShot(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_11AD8
  void NormalMissileAttack(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_17448
  void Null(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_14448
  void Dead(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_143A8
  void Patrol(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_1142C
  void Pause(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_10710
  void PhazonAttack(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_81D4
  void PhazonEnraged(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_7BC0
  void PhazonFall(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_6BA0
  void PhazonMissiles(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_67B8
  void PhazonRise(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_7054
  void PhazonSuck(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_4AC8
  void PissedOff(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_F930
  void ScatterShot(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_175AC
  void SideDash(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_153EC
  void Start(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_F920
  void SuperMissileAttack(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_16ECC
  void SweepBeam(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_CA0C
  void Taunt(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_11544
  void TravelToCoverPoints(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_1DF74
  void TravelToPissedOff(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_FB8C
  void TurnToFacePlayer(CStateManager& mgr, EStateMsg msg, float dt); // fn_10_FE18
#endif
  // CEntity
  CEntity* TypesMatch(int typeId) const override;

private:
#ifdef TARGET_PC
  // fn_10_142F4: is the player within `angle` of the actor's facing axis? The direction to the
  // player and the forward axis (the guest's 0x28/0x38/0x48) are flattened into the XY plane,
  // normalized and compared (the fcmpo at 0x00014384).
  bool IsPlayerInFront(CStateManager& mgr, float angle) const;

  // fn_10_17E14: the fight script's object - the one with the uid at the guest's 0x974 - and
  // module 11's accessor on it (fn_11_B70, which returns the object's 0x24).
  const CDarkSamusBattleStage* GetBattleStage(CStateManager& mgr) const;

  // fn_10_1D794: the still timer (the guest's 0x964) and the position it measures from (the
  // guest's 0x958). `reset` is the guest's argument r4: it clears the timer and moves the
  // reference position to the actor's own position.
  void UpdateStillTimer(bool reset, float dt);

  // fn_10_1E178: the damage the actor has taken (the guest's 0xCC0). `count` is the guest's
  // argument r4: the health that is gone is only added to the damage when it is set.
  void AccumulateDamage(bool count, float dt);

  // fn_10_1C778: the destination of the path find (the guest's 0x8B4/0x8B8/0x8BC) and the path
  // find itself.
  void SetPathFindDestination(CStateManager& mgr, const CVector3f& destination, float dt);

  // fn_10_1E26C: the uid of the last attacker (the guest's 0xC24) and the two uids before it
  // (the count at 0xC28, the uids at 0xC2C).
  void PushAttacker(TUniqueId uid);

  // fn_10_1E2E8: the oldest uid is dropped and the rest move one down.
  void EraseFirstAttacker();

  // fn_10_1E740: should the actor forget the uid of the attacker that hit it last (the guest's
  // 0xC24)? True when the path to the attacker is shagged, when the path find is over, when the
  // attacker has no cover point, or when the attacker's cover point is within 2 units.
  bool ShouldForgetLastAttacker(CStateManager& mgr) const;

  // fn_10_1D9A4: the point the actor is pushed away from - an active repulsor of the actor's
  // connections in the actor's area that holds the actor inside its radius (`strength`, when not
  // null, gets 1 - distance / radius), else the centroid of an obstructed path find region the
  // actor stands in, else the last contact point (mContactPoint) for 0.3 seconds after the
  // contact, else the zero vector.
  CVector3f GetRepulsionPoint(CStateManager& mgr, float* strength) const;

  // fn_10_1DD14: the separation - the actor steers away from GetRepulsionPoint's point
  // (mSeparationDir is the direction it steers along, or zero).
  void Separate(CStateManager& mgr);

  // fn_10_1D860: the additive animation of the state's message - kStateMsg_Activate adds the
  // best kAS_AdditiveReaction animation of the type 5 (its parms built by fn_10_13FFC, the
  // CPASAnimParmData constructor), kStateMsg_Deactivate removes it.
  void UpdateAdditiveAnimation(CStateManager& mgr, EStateMsg msg);

  // Host layout (retail's object is 0x1128, CHECK_SIZEOF below; CPatterned is 0x7c0). The guest
  // offsets are the REL's, read from the constructor fn_10_1FA14 and the module's one-line
  // accessors; the host layout is its own.
  CPathFindSearch mPathFindSearch; // guest 0x7C0 (0xEC): fn_10_CE4C returns this+0x7C0
  CDarkSamusData mData;            // host only: the loader's 60 data properties (see above)
  uchar mGuestCD0;                 // guest 0xCD0, the first byte of the sub-object fn_10_E560
                                   // builds; fn_10_F84C reads it to pick the collision response
  uchar mFlags90C;                 // guest 0x90C, the flag byte: bit 6 (0x40) is the "starts in
                                   // the air" flag (fn_10_F924) and bit 7 (0x80) the "alerted" one
                                   // (fn_10_F840); the constructor's rlwimi chain (0x0001FBF8..)
                                   // only clears them, so they start at 0
  uchar mTurnDone;                 // guest 0xD4C, the "done turning" flag: fn_10_FE18 sets it and
                                   // fn_10_F864 (the DoneTurning trigger) reads it
  uchar mPissedOff;                // guest 0xD58, the flag the PissedOff state clears (fn_10_F930)
                                   // and the fight script sets; fn_10_FD30 (ShouldGetPissedOff)
                                   // reads it
  uchar mFadeOut;                  // guest 0xDEC, the fade flag: fn_10_117A0 (PickedNormalMissile)
                                   // and fn_10_BBE4 (ShouldFadeOut) read it
  TUniqueId mBattleStageUid;        // guest 0x974, the uid of the fight script's object: the
                                   // helper fn_10_17DE0 looks it up in the state manager and
                                   // casts it to CScriptDarkSamusBattleStage. The constructor
                                   // writes kInvalidUniqueId to it (the lhz/sth at 0x0001FCC0)
  TUniqueId mLastAttackerUid;       // guest 0xC24, the uid of the attacker that hit the actor
                                   // last: fn_10_1DF74 clears it and fn_10_1E740 sets it (neither
                                   // is in the port yet); the constructor writes
                                   // kInvalidUniqueId to it (0x0001FF08)
  uint mAttackerCount;              // guest 0xC28, the number of uids in mAttackerUids; the
                                   // constructor writes 0 to it (0x0001FF14)
  TUniqueId mAttackerUids[2];       // guest 0xC2C, the uids of the attackers that hit the actor
                                   // (at most two): fn_10_1E26C pushes the last attacker's uid
                                   // here and drops the oldest when there are two
  int mPickedAttack;               // guest 0x984, the word the fight script writes and the
                                   // Picked* triggers compare: the picked attack's id (1 melee,
                                   // 4 melee dash, 8 scatter shot, 0x10 moving scatter shot,
                                   // 0x20 dive attack, 0x40 normal missile, 0x80 missile jump,
                                   // 0x100 super missile, 0x200 taunt, 0x400 pause, 0x10000 sweep
                                   // beam, 0x20000 boost ball, 0x100000 freeze beam, 0x200000 the
                                   // fade variant, 0x400000 phazon attack, 0x800000 phazon
                                   // enrage). The constructor writes 0xFFFF to it (0x0001FCE8),
                                   // which is "no attack picked"
  uint mCurrentAttack;             // guest 0x980, the word the module writes when an attack starts
                                   // (fn_10_1B478, which the port does not have yet) and reads in
                                   // ShouldReact and TooMuchDamage: the id of the attack the actor
                                   // is performing (the same id space as mPickedAttack; 0xFFFF =
                                   // none). The constructor writes 0xFFFF to it (0x0001FCE4)
  uint mAttackHistory[3];          // guest 0x998, the ids of the attacks the actor has finished:
                                   // fn_10_1B478 pushes the current attack here and trims the
                                   // oldest entry when there are three (that function is not in
                                   // the port yet)
  uint mAttackHistoryCount;        // guest 0x994, the number of ids in mAttackHistory; the
                                   // constructor writes 0 to it (0x0001FCF8)
  float mAccumulatedDamage;        // guest 0xCC0, the damage the actor has taken: fn_10_1E178 adds
                                   // the health that is gone and decays it; the constructor writes
                                   // 0.0f to it (0x0001FF58)
  float mLastHealth;               // guest 0xCC4, the health at the last damage check: fn_10_1E178
                                   // compares it with the health info's current health; the
                                   // constructor writes lbl_10_rodata_3EC = -1.0f to it (0x0001FF5C)
  float mDamageDecay;              // guest 0xCCC, the rate the accumulated damage decays at:
                                   // fn_10_1E178 subtracts it times dt; the constructor writes
                                   // lbl_10_rodata_3E8 = 1.0f to it (0x0001FF64)
  float mDamageThreshold;          // guest 0xCC8, the threshold TooMuchDamage compares the
                                   // accumulated damage with; the constructor writes its 8th float
                                   // argument to it (0x0001FF60), the loader's
                                   // damageInterruptThreshold (the property id 0x8f70d3f2)
  float mStillTime;                // guest 0x964, how long the actor has stood still: fn_10_1D794
                                   // adds dt to it while the actor stays within 0.6 of
                                   // mStillPosition; the constructor writes 0.0f to it (0x0001FCB0)
  CVector3f mStillPosition;        // guest 0x958/0x95C/0x960, the position the still timer
                                   // measures from: fn_10_1D794 moves it to the actor's position
                                   // when the actor walks away; the constructor writes the zero
                                   // vector to it (0x0001FC64..0x0001FC74)
  float mLastWallHitTime;          // guest 0xC44 (inside the 0xB50 sub-object), the time of the
                                   // last wall collision: fn_10_1CF84 and fn_10_1DF74 write it
                                   // (neither is in the port yet); the constructor writes 0.0f to
                                   // it (0x0001FF38)
  float mElapsedTime;              // guest 0x8B0, the time since the actor was created: the Think
                                   // (fn_10_1B1FC) adds dt to it (0x0001B2DC); the constructor
                                   // writes 0.0f to it (0x0001FB88)
  CVector3f mPathFindDestination;  // guest 0x8B4/0x8B8/0x8BC, the destination of the path find:
                                   // fn_10_1C778 writes it (0x0001C7A4..0x0001C7BC); the
                                   // constructor writes the zero vector to it (0x0001FB94..)
  int mAdditiveAnim;               // guest 0x970, the additive animation fn_10_1D860 added (-1 =
                                   // none); the constructor writes -1 to it (0x0001FCBC)
  CVector3f mSeparationDir;        // guest 0x910/0x914/0x918, the direction fn_10_1DD14 steers
                                   // the actor along to get away from a repulsor (zero when it
                                   // does not); the constructor writes the zero vector to it
                                   // (0x0001FC20..)
  float mContactTime;              // guest 0xC34, the elapsed time (mElapsedTime) of the last
                                   // contact fn_10_1D0A0 records (0x0001D154; not in the port
                                   // yet, so it stays at the constructor's 0.0f, 0x0001FF1C)
  CVector3f mContactPoint;         // guest 0xC38/0xC3C/0xC40, the point of that contact; the
                                   // constructor writes the zero vector to it (0x0001FF24..)
  // guest 0x6B4 is the base's animation state (the byte fn_10_8698 stores 1 to on Activate and 0
  // to on Deactivate, and the DOL's ShouldRetry advances): it is the base's own member, the
  // host's CAnimationState mAnimationState, which CPatterned::GetAnimOver reads.
#else
  uchar x7c0_[0x1128 - 0x7c0];
#endif
};
CHECK_SIZEOF(CDarkSamus, 0x1128)

#endif
