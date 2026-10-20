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
#endif
  // CEntity
  CEntity* TypesMatch(int typeId) const override;

private:
#ifdef TARGET_PC
  // Host layout (retail's object is 0x1128, CHECK_SIZEOF below; CPatterned is 0x7c0). The guest
  // offsets are the REL's, read from the constructor fn_10_1FA14 and the module's one-line
  // accessors; the host layout is its own.
  CPathFindSearch mPathFindSearch; // guest 0x7C0 (0xEC): fn_10_CE4C returns this+0x7C0
  CDarkSamusData mData;            // host only: the loader's 60 data properties (see above)
  uchar mGuestCD0;                 // guest 0xCD0, the first byte of the sub-object fn_10_E560
                                   // builds; fn_10_F84C reads it to pick the collision response
#else
  uchar x7c0_[0x1128 - 0x7c0];
#endif
};
CHECK_SIZEOF(CDarkSamus, 0x1128)

#endif
