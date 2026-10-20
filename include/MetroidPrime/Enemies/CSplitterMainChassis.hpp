#ifndef _CSPLITTERMAINCHASSIS
#define _CSPLITTERMAINCHASSIS

#include "REL/REL_Setup.h"
#ifdef TARGET_PC
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"

// Host copy of the SPLL parameter block (docs/module-analysis/Splitter.md section c). Retail
// reads the loader's values into a runtime struct (fn_75_FD58) and copies them into the object
// (fn_75_FB30 for the converted damage info and vulnerability at 0x7C0, fn_75_FC10 for the scalars
// at 0x828). The host keeps the scalars and the *converted* damage info and vulnerability; the
// raw loader copies are not kept, because they duplicate the converted ones. Nothing in this
// milestone reads the values - the class keeps them for the state machine the later milestones
// add - so the fields are a faithful copy of what the loader read.
class CSplitterMainChassisData {
public:
  CSplitterMainChassisData()
  : mUnknownCEF5C2FE(0)
  , mLegStabAttackInterval(0.f)
  , mLegStabMinAttackRange(0.f)
  , mLegStabMaxAttackRange(0.f)
  , mLegStabDamage()
  , mMinDodgeInterval(0.f)
  , mDodgeChance(0.f)
  , mDeploymentSpeed(0.f)
  , mScanDuration(0.f)
  , mLaserSweepInterval(0.f)
  , mLaserSweepMinAttackRange(0.f)
  , mLaserSweepMaxAttackRange(0.f)
  , mSpinAttackLinearVelocity(0.f)
  , mSpinAttackLinearAcceleration(0.f)
  , mSpinAttackLinearDeceleration(0.f)
  , mSpinAttackTurnSpeed(0.f)
  , mSpinAttackInterval(0.f)
  , mSpinAttackIntervalPenalty(0.f)
  , mSpinAttackTelegraphTime(0.f)
  , mSpinAttackMinAttackRange(0.f)
  , mSpinAttackMaxAttackRange(0.f)
  , mUnknownD5F34476(0)
  , mSpinAttackMaxTime(0.f)
  , mSpinAttackDamage()
  , mSoundAlerted(0)
  , mSpinAttackVulnerability(CDamageVulnerability(CWeaponTypeVulnerability::Immune())) {}

  int mUnknownCEF5C2FE;                 // 0x00 unknown_0xcef5c2fe (bit flags; bits 0, 4 and 6)
  float mLegStabAttackInterval;         // 0x04
  float mLegStabMinAttackRange;         // 0x08
  float mLegStabMaxAttackRange;         // 0x0C
  CDamageInfo mLegStabDamage;           // 0x10 (converted; guest 0x7C0)
  float mMinDodgeInterval;              // 0x20
  float mDodgeChance;                   // 0x24
  float mDeploymentSpeed;               // 0x28
  float mScanDuration;                  // 0x2C
  float mLaserSweepInterval;            // 0x30
  float mLaserSweepMinAttackRange;      // 0x34
  float mLaserSweepMaxAttackRange;      // 0x38
  float mSpinAttackLinearVelocity;      // 0x3C
  float mSpinAttackLinearAcceleration;  // 0x40
  float mSpinAttackLinearDeceleration;  // 0x44
  float mSpinAttackTurnSpeed;           // 0x48
  float mSpinAttackInterval;            // 0x4C
  float mSpinAttackIntervalPenalty;     // 0x50
  float mSpinAttackTelegraphTime;       // 0x54
  float mSpinAttackMinAttackRange;      // 0x58
  float mSpinAttackMaxAttackRange;      // 0x5C
  int mUnknownD5F34476;                 // 0x60
  float mSpinAttackMaxTime;             // 0x64
  CDamageInfo mSpinAttackDamage;        // 0x68 (converted; guest 0x7DC)
  int mSoundAlerted;                    // 0x78
  CDamageVulnerability mSpinAttackVulnerability; // 0x1F8 (converted; guest 0x7F8)
};

// The REL's CSplitterMainChassis (module 75): the "Quad" robot's body, script type SPLL ("Mechanism:
// Quad MB"). 0xD48 bytes on the GameCube (the loader's __nw__ at 0x000084F4), built by fn_75_F5F8;
// its vtable lbl_75_data_CB8 (82 words) and the analysis are
// docs/module-analysis/Splitter.md.
//
// Milestone 1 (this file's first slice) is load-and-think: the loader, the constructor and the
// vtable's accessors, so the actor registers and thinks. The state machine (fn_75_E9E4 installs
// the 28 triggers, 22 states and 11 codes), the spin attack, the leg stab, the laser sweep and the
// head module (CSplitterCommandModule, guest 0xF70) are later milestones, and every override that
// belongs to them forwards to CPatterned behind a once-only StandIn log.
class REL_EXPORT CSplitterMainChassis : public CPatterned {
public:
#ifdef TARGET_PC
  // Argument order is the port's own (the REL loader's call at 0x00008580 is
  // (uid, editor properties, entity info, transform, model, actor parameters, patterned info)
  // plus the parameter block).
  CSplitterMainChassis(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                       const CTransform4f& xf, const CModelData& modelData,
                       const CActorParameters& actParms, const CPatternedInfo& pInfo,
                       const CSplitterMainChassisData& data);
  ~CSplitterMainChassis() override;

  // The DOL facade: src/MetroidPrime/Enemies/SSplitterMainChassis_FuncPtrs.cpp defines it and
  // calls the module's table through SSplitterMainChassis_FuncPtrs::mAutoDestruct.
  void AutoDestruct(float time);

  // CEntity / CActor
  void PreThink(float dt, CStateManager& mgr) override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  const CHealthInfo* GetHealthInfo() const override;
  const CDamageVulnerability* GetDamageVulnerability() const override;
  rstl::optional_object< CAABox > GetTouchBounds() const override;
  CAABox GetSortingBounds(const CStateManager& mgr) const override;

  // CAi
  void Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) override;
  void KnockBack(CStateManager& mgr, const CKnockBackInfo& info) override;
  bool CanBeShot(const CStateManager&, int) override { return true; }
  bool IsListening() const override { return true; }
  bool Listen(CStateManager&, const CVector3f&, EListenNoiseType) override;

  // CPhysicsActor
  void CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                    CStateManager& mgr) override;

  // CPatterned. The accessors below are the REL's own one-line bodies (the module's .text starts
  // with them), so they are real, not stand-ins.
  CProjectileInfo* ProjectileInfo() override { return nullptr; }
  using CPatterned::GetSearchPath;
  CPathFindSearch* GetSearchPath() override { return &mPathFindSearch; }
  using CPatterned::GetPointSearchPath;
  CPathFindPointSearch* GetPointSearchPath() override { return nullptr; }
  uchar GetModelAlphau8(const CStateManager&) const override { return mColor.GetAlphau8(); }
  void SetupStateMachine(CStateManager& mgr) override;
  bool CanBeIngPossessed(CStateManager& mgr) const override;
  void SetIngPossessed(bool possessed, CStateManager& mgr) override;
  void SetIngPossessed(bool possessed, float duration, CStateManager& mgr) override;
  void SetAttackTarget(CStateManager& mgr, TUniqueId target) override;
#endif
  // The module's auto-destruct starter (retail fn_75_5A54). What that function does is not
  // understood yet (docs/module-analysis/Splitter.md section j), so the table's slot is filled
  // with a documented stand-in (PortCSplitterMainChassis.cpp).
  void PortAutoDestruct(float time);

  // CEntity
  CEntity* TypesMatch(int typeId) const override;

private:
#ifdef TARGET_PC
  void EnsureBodyActive(CStateManager& mgr);

  // Host layout (retail's object is 0xD48, CPatterned is 0x7C0; the guest offsets are the REL's
  // and are in docs/module-analysis/Splitter.md section e). The host class has its own layout and
  // only keeps what this milestone needs.
  CSplitterMainChassisData mData; // guest 0x7C0 (the parameter block fn_75_FB30 copies in)
  CPathFindSearch mPathFindSearch; // guest 0xB7C (fn_75_81F8 returns this+0xB7C)
  // 0xC6C: the command module's id (the retail keeps a u16 here; the port's ids fit in 16
  // bits, so a TUniqueId carries the same value). The retail's ctor writes kInvalidUniqueId
  // here (fn_75_F5F8 @0x0000F6E4) and the state machine's own code (0x0000AF74,
  // 0x0000E720, 0x0000EBD0) replaces it; those writers are missing from the port.
  // guest 0xC68: the docking partner's id (the retail's Think reads it at 0xF11C and looks
  // for a partner when it is 0; the retail's ctor writes 0 here at 0x0000F6BC).
  uint mDockingPartnerId = 0;
  TUniqueId mCommandModuleId;
#endif
};
#else
// Pointer-only interface to the REL-backed enemy; its layout is not yet recovered.
class REL_EXPORT CSplitterMainChassis {
public:
  // The Wii export corroborates this interface, not the DOL facade's original spelling.
  void AutoDestruct(float time);
};
#endif

#endif // _CSPLITTERMAINCHASSIS
