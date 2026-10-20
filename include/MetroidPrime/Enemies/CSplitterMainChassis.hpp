#ifndef _CSPLITTERMAINCHASSIS
#define _CSPLITTERMAINCHASSIS

#include "REL/REL_Setup.h"
#ifdef TARGET_PC
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CLineOfSightTracker.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
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

  // The command module reads these two ids (fn_75_31A8).
  TUniqueId GetCommandModuleId() const { return mCommandModuleId; }
  TUniqueId GetSyncId() const { return mSyncId; }

  // guest 0xD28: the scan timer, which the command module's fn_75_58E8 tests (the chassis must
  // have it above zero for the head to spawn its beam effect). The retail's constructor leaves
  // it at zero (fn_75_F5F8 stores lbl_75_rodata_390, 0.0f) and its script-message handler sets
  // it to the scan duration when that is above zero and to FLT_MAX otherwise (fn_75_F1C0,
  // 0x0000F40C-0x0000F42C).
  float GetScanTimer() const { return mScanTimer; }

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
  // guest 0xC68: the collision actor manager. The retail's Think reads it at 0xF11C and calls
  // CCollisionActorManager::Update when it is there (0xF144, the DOL's fn_80135D84); when it is
  // 0 the retail builds it instead (0xF130, fn_75_A4E4). The port's manager is empty: the
  // joint collision descriptions belong to the state machine.
  CCollisionActorManager* mCollisionActorManager = nullptr;
  TUniqueId mCommandModuleId;
  // guest 0xCC0: the id the command module copies into its own 0xEFE (fn_75_31A8, 0x320C).
  // The retail writes it at 0xBE54 and in its constructor (0xF764); what it names is not
  // established, so the port keeps it invalid.
  TUniqueId mSyncId = kInvalidUniqueId;
  // guest 0xC70: the retail's CVisualKnowledge, whose Update the Think calls (the DOL's
  // fn_801F55F0). The host's CLineOfSightTracker is the same thing.
  CLineOfSightTracker mVisualKnowledge;
  // guest 0xD28: the scan timer (see GetScanTimer).
  float mScanTimer = 0.f;
  // guest 0xD00: the counter the XLND script message sets to 5 (fn_75_F1C0, 0x0000F4F0).
  int mLandedCount = 0;
  // guest 0xD2C: the timer the IM00 script message advances by the data's spin attack max time
  // (fn_75_F1C0, 0x0000F518-0x0000F524).
  float mSpinTimer = 0.f;
  // guest 0xCFC: the counter the IM00 script message advances by the data's
  // unknown_0xd5f34476 (fn_75_F1C0, 0x0000F528-0x0000F534).
  int mSpinCounter = 0;
  // guest 0xD40 / 0xD41: the flag bytes (the IM00 and IM01 script messages set 0xD40's 0x80
  // bit, the STRT message sets 0xD41's 0x04 bit and IM00 tests 0xD41's 0x80 bit).
  uchar mFlagsD40 = 0;
  uchar mFlagsD41 = 0;
  // The retail's fn_75_AA68: the locomotion type the data's flag word at 0x828 decides.
  void PortSetLocomotionType();
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
