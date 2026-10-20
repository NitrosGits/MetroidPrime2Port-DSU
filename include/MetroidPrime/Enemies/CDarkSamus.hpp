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
  uchar mTauntPending;             // guest 0x6B4, the pending flag of the taunt command that
                                   // fn_10_8698 (the TurnToFacePlayer state) delivers
#else
  uchar x7c0_[0x1128 - 0x7c0];
#endif
};
CHECK_SIZEOF(CDarkSamus, 0x1128)

#endif
