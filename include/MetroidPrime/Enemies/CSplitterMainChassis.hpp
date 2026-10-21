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

#ifdef TARGET_PC
  // The retail's fn_75_E6E4: the chassis accepts a head (the head's fn_75_3EEC asks). It is
  // public because the command module calls it.
  bool PortAcceptHead(CStateManager& mgr, TUniqueId headId);
#endif

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

  // The chassis's state machine names. The REL's table entry is 16 bytes
  // {const char* name, 0, 0xFFFFFFFF, fn}: the module's static initialiser
  // (fn_75_FED0, in the REL's .ctors) copies the three-word records of .data
  // 0x5D0..0x714 into the three tables, so the retail function of each name is that
  // record's third word (docs/module-analysis/Splitter.md section b). The ported ones are
  // the Start state (fn_75_DFE8), the Scanning state (fn_75_DCF8) and the IsDisabled
  // (fn_75_E620), DeployFromHoldingTube (fn_75_E034), ShotAt (fn_75_E5B8), PatrolPathOver
  // (fn_75_E478), LostHead (fn_75_E5AC), ShouldWaitForSnatch (fn_75_E5D0) and
  // PrepareForDocking (fn_75_DFEC) triggers; the rest stand in for the retail function and
  // log it once when the state machine runs them.
  bool StateOver(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E64C
  bool IsIntact(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E62C
  bool IsDisabled(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E620
  bool ShouldWaitForSnatch(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E5D0
  bool ShotAt(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E5B8
  bool LostHead(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E5AC
  bool HasHead(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E58C
  bool HasRetreatPoint(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E56C
  bool HasTarget(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E54C
  bool CanReachTarget(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E530
  bool HasLineOfSight(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E524
  bool PathShagged(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E4DC
  bool PathOver(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E494
  bool PatrolPathOver(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E478
  bool ShouldLegStab(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E3A4
  bool ShouldMorphballStab(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E2F0
  bool ShouldSpinAttack(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E1D0
  bool ShouldLaserSweep(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E0B0
  bool ShouldDodge(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E098
  bool IsSpinAttackOver(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E084
  bool SpunIntoPlayer(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E078
  bool SpunIntoGeometry(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E06C
  bool BreakOutOfSpin(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E060
  bool TooManyBounces(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E040
  bool DeployFromHoldingTube(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E034
  bool ShouldDeploy(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E028
  bool DeployPathOver(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_E00C
  bool PrepareForDocking(CStateManager& mgr, const CTriggerData& data) const;  // fn_75_DFEC

  void Start(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_DFE8
  void Idle(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_DFE4
  void Inactive(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_DF88
  void Activate(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_DDF8
  void Patrol(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_DCFC
  void Scanning(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_DCF8
  void PathFind(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_DC08
  void LegStabAttack(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_DA38
  void SpinAttack(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_D7D4
  void Dodge(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_D61C
  void LaserSweepAttack(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_D3E0
  void SpinCollisionReaction(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_D2A0
  void SpinBounce(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_D13C
  void SpinTelegraph(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_D0A4
  void SpinToIdle(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_CF44
  void SpawnDrop(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_CCF0
  void HoldingTube(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_CC98
  void WaitForDocking(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_C9E4
  void WaitForSnatch(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_C8F4
  void FollowDeployPath(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_C51C
  void DeploymentLanding(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_C034
  void HeadExplosion(CStateManager& mgr, EStateMsg msg, float dt);  // fn_75_BEEC

  void SelectTarget(CStateManager& mgr, float dt);  // fn_75_BE28
  void SetTargetDest(CStateManager& mgr, float dt);  // fn_75_BC70
  void SetRetreatDest(CStateManager& mgr, float dt);  // fn_75_BBC0
  void SetupSpinAttack(CStateManager& mgr, float dt);  // fn_75_BAA8
  void EndSpinAttack(CStateManager& mgr, float dt);  // fn_75_B9B0
  void SetupLaserSweep(CStateManager& mgr, float dt);  // fn_75_B678
  void SetMorphballStabLeg(CStateManager& mgr, float dt);  // fn_75_B66C
  void FindBestStabLeg(CStateManager& mgr, float dt);  // fn_75_B57C
  void FindBestDodgeDirection(CStateManager& mgr, float dt);  // fn_75_B2A0
  void BounceOffPlayer(CStateManager& mgr, float dt);  // fn_75_B1D8
  void BounceOffGeometry(CStateManager& mgr, float dt);  // fn_75_B104
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
  // guest 0xCC4 / 0xCC6: the ids of the connected objects fn_75_AC68 looks up.
  TUniqueId mConnectedObjectId = kInvalidUniqueId;
  TUniqueId mFollowObjectId = kInvalidUniqueId;
  // guest 0xCBE: the team the chassis is in (fn_75_8D50 leaves it).
  TUniqueId mTeamId = kInvalidUniqueId;
  // The retail's fn_75_AC68 (the connected-object lookups), fn_75_8D50 (the team exit) and
  // fn_75_AAC4 (the player-hint release).
  void PortUpdateConnectedObjects(CStateManager& mgr);
  void PortLeaveTeam(CStateManager& mgr);
  void PortReleasePlayerHint(CStateManager& mgr);
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
  // The retail's fn_75_E6E4 (the chassis accepts a head) and fn_75_8EA4 (it copies four of its
  // data's timers into its derived state).
  void PortCopyTimers();
  // guest 0xC6E: the second link, which fn_75_E6E4 clears when it takes a head.
  TUniqueId mSecondLinkId = kInvalidUniqueId;
  // guest 0xCBC: the id of the CIngSnatchingSwarm that is snatching this chassis. The retail's
  // fn_75_E810 keeps it there when the attack target is one; the port's SetAttackTarget is
  // still a shell, so it stays invalid.
  TUniqueId mSnatcherId = kInvalidUniqueId;
  // guest 0xD10-0xD1C: the four timers fn_75_8EA4 copies.
  float mLegStabTimer = 0.f;     // 0xD10 (the data's leg stab interval, 0x82C)
  float mSpinPenaltyTimer = 0.f;  // 0xD14 (the data's spin attack interval penalty, 0x874)
  float mLaserSweepTimer = 0.f;   // 0xD18 (the data's laser sweep interval, 0x858)
  float mDodgeTimer = 0.f;        // 0xD1C (the data's min dodge interval, 0x848)
  // The retail's fn_75_AFE8 (the chassis's timers) and fn_75_9520 (its fade), the Think's
  // calls at 0x0000F164 and 0x0000F180.
  void PortUpdateTimers(CStateManager& mgr, float dt);
  void PortUpdateFade(float dt);
  // guest 0xD0C: the steering speed range the Patrol state puts on the body controller's command
  // manager (fn_75_DCFC, 0x0000DD50). The retail's constructor leaves 1.0f there
  // (fn_75_F5F8, 0x0000F834) and then divides the walk speed by the run speed
  // (0x0000F93C-0x0000F964).
  float mSteeringSpeed = 1.f;
  // guest 0x6E0, 0x6E4 and 0x6E8: the face vector the Patrol state copies out of the body
  // controller's command manager (fn_75_DCFC, 0x0000DDC0-0x0000DDD8).
  CVector3f mPatrolFacing;
  // guest 0xD20: the timer fn_75_AFE8 counts down; at zero or below the chassis dies.
  float mTimerD20 = 0.f;
  // guest 0xD30 / 0xD34: two timers fn_75_AFE8 counts up.
  float mTimerD30 = 0.f;
  float mTimerD34 = 0.f;
  // guest 0xD3C: the timer fn_75_AFE8 counts up while the id at 0xCC0 is valid and the counter
  // at 0xC48 is not zero, and clears otherwise.
  float mTimerD3C = 0.f;
  // guest 0xC48: the counter fn_75_AFE8 tests. What it counts is not established.
  int mIntC48 = 0;
  // guest 0xCB4: the additive animation's id (fn_75_9520 adds and deletes it).
  uint mAdditiveAnimId = 0;
  // guest 0xCB8: the fade weight (fn_75_9520 counts it up and down).
  float mFadeWeight = 0.f;
  // guest 0xCC8: the locator's segment id - the retail's constructor leaves it at 0
  // (0x0000F780) and fn_75_ACEC asks the locator's transform for the head's placement.
  CSegId mLocatorSegment = CSegId(0);
  // The retail's fn_75_986C (the chassis's own path update, called by the Patrol state) and
  // fn_75_8EC8 (the separation steering it asks first).
  void PortUpdatePath(CStateManager& mgr);
  CVector3f PortSeparationVector(CStateManager& mgr);
  // The retail's fn_75_ACEC: the head's placement (the Think's call at 0x0000F174).
  void PortPlaceHead(CStateManager& mgr, float dt);
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
