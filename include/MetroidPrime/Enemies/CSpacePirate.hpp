#ifndef _CSPACEPIRATE
#define _CSPACEPIRATE

#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "REL/REL_Setup.h"

#ifdef TARGET_PC
#include "MetroidPrime/CBoneTracking.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "Kyoto/Math/CPlane.hpp"

// Host copy of the properties the SpacePirate loader reads (retail: CSpacePirateData, 0x134 bytes
// at 0x7c0, built by fn_72_10A10). Only the fields milestone 1a keeps are held; the retail offset
// inside the 0x134 block is noted on each.
struct CSpacePirateData {
  float mAggressiveness;       // +0x00
  float mCoverCheck;           // +0x04
  float mSearchRadius;         // +0x08
  float mFallBackCheck;        // +0x0c
  float mFallBackRadius;       // +0x10
  float mHearingRadius;        // +0x14
  uint mFlags;                 // +0x18
  bool mUnknownCe670970;       // +0x1c
  CAssetId mProjectile;        // +0x20
  CDamageInfo mProjectileDamage; // +0x24
  ushort mProjectileSfx;       // +0x40
  CDamageInfo mBladeDamage;    // +0x44
  float mKneelAttackChance;    // +0x60
  CAssetId mKneelAttackShot;   // +0x64
  CDamageInfo mKneelAttackDamage; // +0x68
  float mDodgeCheck;           // +0x84
  ushort mImpactSfx;           // +0x88
  float mIntraBurstTime;       // +0x8c
  float mIntraBurstVariation;  // +0x90
  float mGunTrackDelay;        // +0xa0
  uint mFirstBurstCount;       // +0xa4
  float mCloakOpacity;         // +0xa8
  float mMaxCloakOpacity;      // +0xac
  float mBreakDodgeMinTime;    // +0xb0
  float mBreakDodgeMaxTime;    // +0xb4
  ushort mHurledSfx;           // +0xb8
  ushort mDeathSfx;            // +0xba
  float mAvoidDistance;        // +0xc0
  CAssetId mGrenadeLauncher;   // +0xcc (launcher block +4, i.e. +0xc8 + 4)
  uint mEquippedWeapon;        // +0xc8
};
#endif

class REL_EXPORT CSpacePirate : public CPatterned {
public:
#ifdef TARGET_PC
  CSpacePirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
               const CTransform4f& xf, const CModelData& modelData,
               const CActorParameters& actParms, const CPatternedInfo& pInfo,
               const CSpacePirateData& data);
  ~CSpacePirate() override;

  // The module's table (SSpacePirate_FuncPtrs) points at these; AttachActorToPirate and
  // DetachActorFromPirate are the proxies that call through the table, so the bodies live here.
  bool PortAttachActor(TUniqueId id);
  void PortDetachActor();

  void PreThink(float dt, CStateManager& mgr) override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;
  void PreRender(CStateManager& mgr) override;
  void Render(const CStateManager& mgr) const override;
  void Touch(CActor& actor, CStateManager& mgr) override;
  void Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) override;
  void KnockBack(CStateManager& mgr, const CKnockBackInfo& info) override;
  void SetupStateMachine(CStateManager& mgr) override;
  TUniqueId GetAttackTarget() const override { return mAttackTarget; }
#endif
  // CEntity
  CEntity* TypesMatch(int typeId) const override;

  bool AttachActorToPirate(TUniqueId id);
  void DetachActorFromPirate();
  TUniqueId GetAttachedActor() const { return mAttachedActor; }

  bool GetEnableAim() const { return mEnableAim; } // Guessed Prime name.
  bool AllEnergyDrained() const { return mAllEnergyDrained; }
  void SetPortalPlane(const CPlane& plane) { mPortalPlane = plane; } // Guessed name.

private:
#ifdef TARGET_PC
  void EnsureBodyActive(CStateManager& mgr);
  // Host layout (retail's is 0xc70 bytes, CHECK_SIZEOF below). Guest offsets kept as comments.
  CSpacePirateData mData;       // 0x7c0 (0x134)
  CSegId mHeadSegId;            // 0xa20 "Head_1"
  CBoneTracking mBoneTracking;  // 0xa2c (0x3c)
  // 0x8f4..0x8fa flag bits are not unpacked yet (milestone 2); the two the other modules read:
  bool mEnableAim;              // 0x8f7 bit 25
  bool mAllEnergyDrained;       // 0x8f8 bit 30
  TUniqueId mAttachedActor;     // 0xa84
  TUniqueId mAttackTarget;      // 0xa94
  float mAttackRemTime;         // 0xa90 (1.0)
  void* xb30_;                  // 0xb30 (null; GetRagDoll's slot, unidentified)
  CPlane mPortalPlane;          // 0xc5c (0x10); written by the WispTentacle while it drags the pirate
#else
  uchar x7c0_[0x137];
  bool x8f7_24_ : 1;
  bool mEnableAim : 1; // Guessed Prime name; read by flying pirate taunts.
  bool x8f7_26_ : 1;
  bool x8f7_27_ : 1;
  bool x8f7_28_ : 1;
  bool x8f7_29_ : 1;
  bool x8f7_30_ : 1;
  bool x8f7_31_ : 1;
  bool x8f8_24_ : 1;
  bool x8f8_25_ : 1;
  bool x8f8_26_ : 1;
  bool x8f8_27_ : 1;
  bool x8f8_28_ : 1;
  bool x8f8_29_ : 1;
  bool mAllEnergyDrained : 1; // Guessed Prime name; read by the Metroid energy drain.
  bool x8f8_31_ : 1;
  uchar x8f9_[0x18b];
  TUniqueId mAttachedActor; // Guessed member name.
  uchar xa86_[0xaa];
  void* xb30_;
  uchar xb34_[0x128];
  CPlane mPortalPlane; // Guessed name; written by the WispTentacle while it drags the pirate.
  uchar xc6c_[4];
#endif
};
CHECK_SIZEOF(CSpacePirate, 0xc70)

#endif
