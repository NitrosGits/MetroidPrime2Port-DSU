#ifndef _CELITEPIRATE
#define _CELITEPIRATE

#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "REL/REL_Setup.h"

#ifdef TARGET_PC
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CAnimationParameters.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/ScriptLoader/Structs/SLdrShockWaveInfo.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"
#include "MetroidPrime/Weapons/CShockWave.hpp"

// Host copy of the properties the ElitePirate loader reads and hands to the class (retail:
// CElitePirateData, 0x198 bytes at 0x828, built on the loader's stack by fn_15_B82C and copied in
// by its copy constructor fn_15_B360). Only the retail offsets this milestone could prove are
// noted; the rest are named after the SLdrElitePirate property they come from, and the host layout
// is its own (nothing in this milestone reads them - the class stores them for the FSM the later
// milestones add). The REL's own builder takes them as one long parameter list (8 floats, seven
// ints/pointers, then 24 stack slots); the ordering inside that list could not be recovered, only
// the grouping, so the fields are plain members the loader assigns by name.
class CElitePirateData {
public:
  CElitePirateData()
  : mStateMachine(kInvalidAssetId)
  , mMeleeDamage()
  , mShieldedModel(kInvalidAssetId)
  , mShieldedSkinRules(kInvalidAssetId)
  , mDarkShield(kInvalidAssetId)
  , mDarkShieldSound(0)
  , mDarkShieldPop(kInvalidAssetId)
  , mLightShield(kInvalidAssetId)
  , mLightShieldSound(0)
  , mLightShieldPop(kInvalidAssetId)
  , mSingleShockWaveInfo(SLdrShockWaveInfo())
  , mDoubleShockWaveInfo(SLdrShockWaveInfo())
  , mRocket(kInvalidAssetId)
  , mRocketDamage()
  , mRocketLauncherActorInfo()
  , mRocketLauncherAnimInfo(CAssetId(0), 0, 0)
  , mUnknown7E6E0D38(CAssetId(0), 0, 0)
  , mVisorElectricEffect(kInvalidAssetId)
  , mSoundVisorElectric(0)
  , mPartAsset(kInvalidAssetId) {}

  CAssetId mStateMachine;       // +0x00 [proven] patterned.stateMachine; the ctor loads it as the
                                // 'FSM2' token at 0xc30 (fn_15_B748)
  CDamageInfo mMeleeDamage;     // SLdrElitePirate.meleeDamage (fn_15_B82C's damage argument)
  float mMaxMeleeRange;
  float mMinShockwaveRange;
  float mMaxShockwaveRange;
  float mMinRocketRange;
  float mMaxRocketRange;
  float mUnknown5236C2B6;
  float mUnknown01EAAB17;
  CAssetId mShieldedModel;
  CAssetId mShieldedSkinRules;
  CAssetId mDarkShield;
  ushort mDarkShieldSound;
  CAssetId mDarkShieldPop;
  CAssetId mLightShield;
  ushort mLightShieldSound;
  CAssetId mLightShieldPop;
  float mTauntInterval;
  float mTauntVariance;
  CShockWaveInfo mSingleShockWaveInfo;
  CShockWaveInfo mDoubleShockWaveInfo;
  float mUnknown28B39197;
  float mUnknownE27DE71B;
  float mUnknown665E7ACE;
  float mUnknownACD4D06D;
  CAssetId mRocket;             // +0x168 [proven] the ctor's CProjectileInfo (0xcd0) asset
  CDamageInfo mRocketDamage;    // +0x16c [proven] that CProjectileInfo's damage
  int mUnknown624222F8;
  int mUnknown31E43A1C;
  float mRepeatedAttackChance;
  float mEnergyAbsorbDuration;
  float mEnergyAbsorbVariance;
  float mEnergyAttractionForce;
  int mAlwaysFF;
  int mAlwaysFF23F5E1EE;
  CActorParameters mRocketLauncherActorInfo;
  CAnimationParameters mRocketLauncherAnimInfo;
  CAnimationParameters mUnknown7E6E0D38;
  CAssetId mVisorElectricEffect;
  ushort mSoundVisorElectric;
  CAssetId mPartAsset;          // +0x34 [proven offset] the asset the ctor loads under the 'PART'
                                // tag (a CToken at 0x9e8 with its valid byte at 0x9f4)
};
#endif

// The REL's CElitePirate (module 15). 0xd30 bytes on the GameCube (the loader's rs_new size), built
// by fn_15_A190; its vtable lbl_15_data_634 is the map of what it overrides - the slots it inherits
// keep the DOL's own symbols (e.g. slot 4 PreThink is CPatterned::PreThink, so the class does not
// override it). Only the passive half is implemented here: the loader, the constructor and the
// methods that let the actor load, think, stand and render. The state machine (fn_15_C0D0, 0x808),
// the shields, the shock waves, the rockets and the melee attack are later milestones.
class REL_EXPORT CElitePirate : public CPatterned {
public:
#ifdef TARGET_PC
  // Argument order is the REL loader's call at 0x00000BE0: (uid, name, entityInfo, transform,
  // modelData, patternedInfo, actorParameters, data).
  CElitePirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
               const CTransform4f& xf, const CModelData& modelData, const CPatternedInfo& pInfo,
               const CActorParameters& actParms, const CElitePirateData& data);
  ~CElitePirate() override;

  // CEntity / CActor
  // PreThink, Touch, Death, KnockBack and SetupStateMachine are not overridden: the REL's vtable
  // keeps the base-class symbols in those slots (4, 19, 38, 39, 60).
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;

  // CAi
  void TakeDamage(const CVector3f& direction, float magnitude) override;
  // The three one-line accessors below are the REL's own bodies (vtable slot -> function), so they
  // are real rather than stand-ins.
  bool CanBeShot(const CStateManager&, int) override { return true; } // slot 42, fn_15_A4
  bool IsListening() const override { return false; }                 // slot 43, fn_15_AC
  bool Listen(CStateManager&, const CVector3f&, EListenNoiseType) override { // slot 44, fn_15_B4
    return false;
  }

  // CPatterned
  CProjectileInfo* ProjectileInfo() override { return &mProjectileInfo; } // slot 55, fn_15_52C8
  using CPatterned::GetSearchPath;
  CPathFindSearch* GetSearchPath() override { return &mPathFindSearch; } // slot 56, fn_15_0
  CPathFindPointSearch* GetPointSearchPath() override { return nullptr; } // slot 57, fn_15_64
  CRagDoll* GetRagDoll() const override { return nullptr; }              // slot 61, fn_15_6C
  TUniqueId GetAttackTarget() const override { return kInvalidUniqueId; } // slot 67, fn_15_74
  // The REL also overrides slots 15 (fn_15_D8, GetHealthInfo - it calls the base class's), 18
  // (fn_15_10, GetTouchBounds - bounding box then fn_15_C094), 21 (fn_15_7EEC, GetAimPosition), 50
  // (fn_15_5C), 68 (fn_15_84, IsOnGround - the base class's own bit) and 73 (fn_15_9C,
  // GetDeathExplosionParticle - returns the base member at 0x754). None is needed for this
  // milestone: 68 and 73 have the inherited bodies' behaviour and the rest are accessors the
  // milestone's code paths do not call.
#endif
  // CEntity
  CEntity* TypesMatch(int typeId) const override;

private:
#ifdef TARGET_PC
  void EnsureBodyActive(CStateManager& mgr);

  // Host layout (retail's is 0xd30, CHECK_SIZEOF below; CPatterned is 0x7c0). Guest offsets are
  // the REL's, read from the constructor fn_15_A190 and the module's one-line accessors. The
  // members this milestone needs:
  CElitePirateData mData;          // 0x828 (0x198; the next member starts at 0x9c0)
  CPathFindSearch mPathFindSearch; // 0xa50 (0xec; fn_15_0 returns this+0xa50)
  CProjectileInfo mProjectileInfo; // 0xcd0 (0x28; fn_15_52C8 returns this+0xcd0)
  // Guest members this milestone does not hold, because nothing in it reads them: the int at 0x7c0
  // (-1), the 0x64-byte sub-object copied from pInfo+0x74 at 0x7c4 (fn_15_2B08), the four zeroed
  // CVector3f at 0x7f8/0x804/0x810/0x81c, the CCollidableAABox at 0x9c0 built from GetBoundingBox,
  // the 'PART' CToken at 0x9e8 with its valid byte at 0x9f4, the uid/flag/float blocks at
  // 0x9f8..0xa4c, the CVector3f at 0xb3c, the 0x18-byte object built by DOL fn_801931E0 at 0xb48,
  // the bit block at 0xc10, the FSM2 token object at 0xc30 (fn_15_B748), the initial-state ints at
  // 0xc3c/0xc40, the 0x58-byte object built by fn_15_B080 at 0xc58, and the 0xcf8..0xd2c block.
#else
  uchar x7c0_[0x570]; // CElitePirateData, the path search, the projectile info and everything above
#endif
};
CHECK_SIZEOF(CElitePirate, 0xd30)

#endif
