#ifndef _CSPLITTERCOMMANDMODULE
#define _CSPLITTERCOMMANDMODULE

class CCollisionActorManager;

#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSplitterCommandModule.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"

#include "REL/REL_Setup.h"

// Host copy of the SPLU parameter block (docs/module-analysis/Splitter.md section c). Retail
// reads the loader's values into a runtime struct (fn_75_791C) and copies them into the object
// (fn_75_72BC, 1:1 from the loader's stack struct at 0x7C0). The host keeps the scalars, the
// *converted* damage infos and vulnerabilities and the beam info; the raw loader copies are not
// kept, because they duplicate the converted ones. Nothing in this milestone reads the values -
// the class keeps them for the state machine the later milestones add.
class CSplitterCommandModuleData {
public:
  CSplitterCommandModuleData()
  : mUnknownBD80FD94(0)
  , mMaxLinearVelocity(0.f)
  , mMaxTurnSpeed(0.f)
  , mScanningTurnSpeed(0.f)
  , mMinLaserPulseAttackTime(0.f)
  , mMinLaserPulseRange(0.f)
  , mMaxLaserPulseRange(0.f)
  , mMaxLaserPulseShots(0)
  , mLaserPulseProjectile(kInvalidAssetId)
  , mLaserPulseDamage()
  , mMinDodges(0)
  , mMaxDodges(0)
  , mDodgeChance(0.f)
  , mResetShieldTime(0.f)
  , mShieldHP(0.f)
  , mLaserSweepTurnSpeed(0.f)
  , mLaserSweepDamage()
  , mLaserSweepBeamInfo()
  , mUnknown9EC51FE4()
  , mSoundLaserSweep(0)
  , mSoundLaserChargeUp(0)
  , mSoundDocking(0)
  , mSoundScanning(0)
  , mSoundLightShield(0)
  , mSoundDarkShield(0)
  , mSoundShieldOn(0)
  , mLightShieldVulnerability(CDamageVulnerability(CWeaponTypeVulnerability::Immune()))
  , mDarkShieldVulnerability(CDamageVulnerability(CWeaponTypeVulnerability::Immune())) {}

  int mUnknownBD80FD94;            // 0x00 unknown_0xbd80fd94
  float mMaxLinearVelocity;        // 0x04
  float mMaxTurnSpeed;             // 0x08
  float mScanningTurnSpeed;        // 0x0C
  float mMinLaserPulseAttackTime;  // 0x10
  float mMinLaserPulseRange;       // 0x14
  float mMaxLaserPulseRange;       // 0x18
  int mMaxLaserPulseShots;         // 0x1C
  CAssetId mLaserPulseProjectile;  // 0x20
  CDamageInfo mLaserPulseDamage;   // 0x24 (converted; guest 0xCD4)
  int mMinDodges;                  // 0x34
  int mMaxDodges;                  // 0x38
  float mDodgeChance;              // 0x3C
  float mResetShieldTime;          // 0x40
  float mShieldHP;                 // 0x44
  float mLaserSweepTurnSpeed;      // 0x48
  CDamageInfo mLaserSweepDamage;   // 0x4C (converted; guest 0xCF0)
  SLdrPlasmaBeamInfo mLaserSweepBeamInfo; // 0x5C (guest 0x81C)
  SLdrUnknownStruct42 mUnknown9EC51FE4;  // 0xA4 (guest 0x864)
  int mSoundLaserSweep;            // 0xC4
  int mSoundLaserChargeUp;         // 0xC8
  int mSoundDocking;               // 0xCC
  int mSoundScanning;              // 0xD0
  int mSoundLightShield;           // 0xD4
  int mSoundDarkShield;            // 0xD8
  int mSoundShieldOn;              // 0xDC
  CDamageVulnerability mLightShieldVulnerability; // 0x25C (converted; guest 0xD0C)
  CDamageVulnerability mDarkShieldVulnerability;  // 0x3B8 (converted; guest 0xD3C)
};

// The REL's CSplitterCommandModule (module 75): the "Quad" robot's detachable head, script type
// SPLU ("Mechanism: Quad CM"). 0xF70 bytes on the GameCube (the loader's __nw__ at 0x00000340),
// built by fn_75_6B08; its vtable lbl_75_data_448 (82 words) and the analysis are
// docs/module-analysis/Splitter.md.
//
// Milestone 1 is load-and-think, like the main chassis: the loader, the constructor and the
// vtable's accessors. The state machine (fn_75_5E8C installs the 17 triggers, 14 states and 7
// codes), the scanning, the laser pulse and sweep, the shields, the docking and the beam effect
// actor are later milestones, and every override that belongs to them forwards to CPatterned
// behind a once-only StandIn log.
class REL_EXPORT CSplitterCommandModule : public CPatterned {
public:
  // Argument order is the port's own (the REL loader's call is
  // (uid, editor properties, entity info, transform, model, actor parameters, patterned info)
  // plus the parameter block).
  CSplitterCommandModule(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                          const CTransform4f& xf, const CModelData& modelData,
                          const CActorParameters& actParms, const CPatternedInfo& pInfo,
                          const CSplitterCommandModuleData& data);
  ~CSplitterCommandModule() override;

  // CEntity / CActor
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  const CHealthInfo* GetHealthInfo() const override;
  const CDamageVulnerability* GetDamageVulnerability() const override;
  rstl::optional_object< CAABox > GetTouchBounds() const override;
  CVector3f GetAimPosition(const CStateManager& mgr, float dt) const override;
  CAABox GetSortingBounds(const CStateManager& mgr) const override;

  // CAi
  void KnockBack(CStateManager& mgr, const CKnockBackInfo& info) override;
  bool CanBeShot(const CStateManager&, int) override { return true; }
  bool IsListening() const override { return true; }
  bool Listen(CStateManager&, const CVector3f&, EListenNoiseType) override;

  // CPhysicsActor
  void CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                    CStateManager& mgr) override;

  // CPatterned. The accessors below are the REL's own one-line bodies (the module's .text starts
  // with them), so they are real, not stand-ins.
  CProjectileInfo* ProjectileInfo() override { return &mProjectileInfo; }
  using CPatterned::GetSearchPath;
  CPathFindSearch* GetSearchPath() override { return &mPathFindSearch; }
  using CPatterned::GetPointSearchPath;
  CPathFindPointSearch* GetPointSearchPath() override { return nullptr; }
  uchar GetModelAlphau8(const CStateManager&) const override { return mColor.GetAlphau8(); }
  CRagDoll* GetRagDoll() const override { return nullptr; }
  void SetupStateMachine(CStateManager& mgr) override;
  bool CanBeIngPossessed(CStateManager& mgr) const override;

  // CEntity
  CEntity* TypesMatch(int typeId) const override;

private:
  void EnsureBodyActive(CStateManager& mgr);

  // Host layout (retail's object is 0xF70, CPatterned is 0x7C0; the guest offsets are the REL's
  // and are in docs/module-analysis/Splitter.md section e). The host class has its own layout and
  // only keeps what this milestone needs.
  CSplitterCommandModuleData mData; // guest 0x7C0 (the parameter block fn_75_72BC copies in)
  CPathFindSearch mPathFindSearch;  // guest 0xD6C (fn_75_0 returns this+0xD6C)
  CProjectileInfo mProjectileInfo;  // guest 0xE5C (fn_75_8 returns this+0xE5C)
  // guest 0xE58: the retail's collision actor manager (the command module's Think updates it
  // at 0x64C8). It holds the head's joint collision descriptions, which belong to the state
  // machine, so the port starts it empty.
  CCollisionActorManager* mCollisionActorManager = nullptr;
};

#endif // _CSPLITTERCOMMANDMODULE
