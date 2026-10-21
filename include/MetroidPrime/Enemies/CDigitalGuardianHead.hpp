#ifndef _CDIGITALGUARDIANHEAD
#define _CDIGITALGUARDIANHEAD

// Port-only: the DigitalGuardian REL's head module (module 14, script type DGHD). Upstream has no
// source for it; the retail class is 0x10F8 bytes, built by fn_14_18264 (the loader fn_14_DF7C's
// __nw__). Its tables are docs/module-analysis/DigitalGuardian.md sections b.5-b.7 and its vtable
// section f.2. The guest offsets are in the member names; the host class has its own layout and
// only keeps the members the ported functions read.

#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "REL/REL_Setup.h"

#ifdef TARGET_PC
#include "Kyoto/Animation/CSegId.hpp"
#include "Kyoto/Audio/CSfxHandle.hpp"
#include "Kyoto/TToken.hpp"
#include "MetroidPrime/ScriptLoader/SLdrDigitalGuardianHead.hpp"

#include "rstl/optional_object.hpp"
#include "rstl/reserved_vector.hpp"
#include "rstl/vector.hpp"
#endif

class CCollisionActorManager;
class CGenDescription;
class CInputStream;

class REL_EXPORT CDigitalGuardianHead : public CPatterned {
public:
#ifdef TARGET_PC
  // fn_14_18264. The argument order is the port's own (the loader's call passes the editor
  // properties, the entity info, the transform, the model, the actor and patterned parameters,
  // then the parameter block).
  CDigitalGuardianHead(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                       const CTransform4f& xf, const CModelData& modelData,
                       const CActorParameters& actParms, const CPatternedInfo& pInfo,
                       const SLdrDigitalGuardianHeadData& data);
  ~CDigitalGuardianHead() override;

  // CEntity
  void Think(float dt, CStateManager& mgr) override;                        // fn_14_17C98
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override; // fn_14_17D38

  // CActor
  void PreRender(CStateManager& mgr) override;                 // fn_14_17A44
  void AddToRenderer(const CStateManager& mgr) const override; // fn_14_17A00
  const CDamageVulnerability* GetDamageVulnerability() const override; // fn_14_179E0
  CScannableObjectInfo* GetScannableObjectInfo() const override;      // fn_14_177C8

  // CAi / CPatterned
  bool IsListening() const override { return true; }          // fn_14_DF6C
  bool IsScanVisorSelfRender() const override { return true; } // fn_14_DF74
  void SetupStateMachine(CStateManager& mgr) override;        // fn_14_17468

  // fn_14_13B30: the attack mode word at guest 0xFE8 (the body's fn_14_7168 sets 0, its detach
  // fn_14_70E4 sets 1).
  void SetAttackMode(int mode) { xFE8_ = mode; }
  // fn_14_13B08 (0x28 bytes): ResetProjectileAttackTimes with 0 (.rodata 0x960); the body's
  // fn_14_7B24 calls it.
  void x13B08_(CStateManager& mgr) { ResetProjectileAttackTimes(mgr, 0.f); }

  // Triggers (lbl_14_data_808, 20 entries).
  bool StateOver(CStateManager& mgr, const CTriggerData& data) const;               // fn_14_17454
  bool MainBodyDestroyed(CStateManager& mgr, const CTriggerData& data) const;       // fn_14_17408
  bool ShouldProjectileAttack(CStateManager& mgr, const CTriggerData& data) const;  // fn_14_173CC
  bool IsBurstOver(CStateManager& mgr, const CTriggerData& data) const;             // fn_14_173B8
  bool IsIdle(CStateManager& mgr, const CTriggerData& data) const;                  // fn_14_173AC
  bool IsStunned(CStateManager& mgr, const CTriggerData& data) const;               // fn_14_17398
  bool IsGuarding(CStateManager& mgr, const CTriggerData& data) const;              // fn_14_17384
  bool UseMachineGun(CStateManager& mgr, const CTriggerData& data) const;           // fn_14_17370
  bool UseLockOnMissiles(CStateManager& mgr, const CTriggerData& data) const;       // fn_14_1735C
  bool UseAnnihilatorPulse(CStateManager& mgr, const CTriggerData& data) const;     // fn_14_1734C
  bool UseAnnihilatorCharge(CStateManager& mgr, const CTriggerData& data) const;    // fn_14_17338
  bool AllSplittersDead(CStateManager& mgr, const CTriggerData& data) const;        // fn_14_17298
  bool HeadArmorDestroyed(CStateManager& mgr, const CTriggerData& data) const;      // fn_14_1728C
  bool IsAntennaTransmitting(CStateManager& mgr, const CTriggerData& data) const;   // fn_14_17244
  bool HasLineOfSight(CStateManager& mgr, const CTriggerData& data) const;          // fn_14_171F8
  bool EchoTargetDestroyed(CStateManager& mgr, const CTriggerData& data) const;     // fn_14_171EC
  bool AllEchoTargetsDestroyed(CStateManager& mgr, const CTriggerData& data) const; // fn_14_171D8
  bool ShouldRetreat(CStateManager& mgr, const CTriggerData& data) const;           // fn_14_171B8
  bool BombPitDestroyed(CStateManager& mgr, const CTriggerData& data) const;        // fn_14_17198
  bool IsDead(CStateManager& mgr, const CTriggerData& data) const;                  // fn_14_17184

  // States (lbl_14_data_A14, 17 entries). Start hides CPatterned::Start, as the retail table
  // replaces the base's entry.
  void Start(CStateManager& mgr, EStateMsg msg, float dt);               // fn_14_17180
  void Idle(CStateManager& mgr, EStateMsg msg, float dt);                // fn_14_17134
  void FaceTarget(CStateManager& mgr, EStateMsg msg, float dt);          // fn_14_1709C
  void ProjectileTelegraph(CStateManager& mgr, EStateMsg msg, float dt); // fn_14_16F74
  void PulseAttack(CStateManager& mgr, EStateMsg msg, float dt);         // fn_14_16D64
  void ChargeAttack(CStateManager& mgr, EStateMsg msg, float dt);        // fn_14_16A50
  void MissileAttack(CStateManager& mgr, EStateMsg msg, float dt);       // fn_14_167EC
  void MachineGunAttack(CStateManager& mgr, EStateMsg msg, float dt);    // fn_14_16408
  void AttachedToBody(CStateManager& mgr, EStateMsg msg, float dt);      // fn_14_1633C
  void DetachFromBody(CStateManager& mgr, EStateMsg msg, float dt);      // fn_14_16160
  void StunnedReaction(CStateManager& mgr, EStateMsg msg, float dt);     // fn_14_15F3C
  void DeploySplitters(CStateManager& mgr, EStateMsg msg, float dt);     // fn_14_15E18
  void BombedReaction(CStateManager& mgr, EStateMsg msg, float dt);      // fn_14_15CE8
  void SeekAttackPosition(CStateManager& mgr, EStateMsg msg, float dt);  // fn_14_15B94
  void SeekMotionPath(CStateManager& mgr, EStateMsg msg, float dt);      // fn_14_158F8
  void FollowRetreatPath(CStateManager& mgr, EStateMsg msg, float dt);   // fn_14_1569C
  void Dead(CStateManager& mgr, EStateMsg msg, float dt);                // fn_14_1561C

  // Codes (lbl_14_data_C80, 29 entries); the head's FSM2 state machine runs them by name.
  void SetPlayerFaceTarget(CStateManager& mgr, float dt);        // fn_14_1560C
  void ResetProjectileAttackTimes(CStateManager& mgr, float dt); // fn_14_15598
  void SelectProjectileAttack(CStateManager& mgr, float dt);     // fn_14_15448
  void SetPlatformPosition(CStateManager& mgr, float dt);        // fn_14_153D0
  void SetStunnedPosition(CStateManager& mgr, float dt);         // fn_14_15284
  void SeekLineOfSight(CStateManager& mgr, float dt);            // fn_14_1524C
  void DropShields(CStateManager& mgr, float dt);                // fn_14_15054
  void RaiseShields(CStateManager& mgr, float dt);               // fn_14_14F74
  void ExtractAntenna(CStateManager& mgr, float dt);             // fn_14_14E78
  void ExtractEchoTargets(CStateManager& mgr, float dt);         // fn_14_14CF4
  void RetractEchoTargets(CStateManager& mgr, float dt);         // fn_14_14DA0
  void DestroyActiveSplitters(CStateManager& mgr, float dt);     // fn_14_14C24
  void StartSplitterDeployment(CStateManager& mgr, float dt);    // fn_14_14B98
  void EndSplitterDeployment(CStateManager& mgr, float dt);      // fn_14_14B60
  void Stage3Start(CStateManager& mgr, float dt);                // fn_14_149F8
  void EnableDynamicCollision(CStateManager& mgr, float dt);     // fn_14_14828
  void DisableDynamicCollision(CStateManager& mgr, float dt);    // fn_14_14664
  void EnableValidTarget(CStateManager& mgr, float dt);          // fn_14_14604
  void DisableValidTarget(CStateManager& mgr, float dt);         // fn_14_145A4
  void SelectDeathHeadAttack(CStateManager& mgr, float dt);      // fn_14_14470
  void SetInitialAttackPosition(CStateManager& mgr, float dt);   // fn_14_143D8
  void SetAttackPosition(CStateManager& mgr, float dt);          // fn_14_14254
  void ResetGravityRepulsorHP(CStateManager& mgr, float dt);     // fn_14_14248
  void EjectPlayerFromBombPit(CStateManager& mgr, float dt);     // fn_14_14058
  void GrabPlayerHintControl(CStateManager& mgr, float dt);      // fn_14_13F70
  void SetVulnerableToDamage(CStateManager& mgr, float dt);      // fn_14_13F0C
  void SetImmuneToDamage(CStateManager& mgr, float dt);          // fn_14_13EB8
  void SelectRetreatPlatform(CStateManager& mgr, float dt);      // fn_14_13C6C
  void StartVocalRecoverReaction(CStateManager& mgr, float dt);  // fn_14_13C1C
#endif
  // CEntity
  CEntity* TypesMatch(int typeId) const override;

private:
#ifdef TARGET_PC
  // The body's triggers read the flag byte (head+0x10F4 bit 0x20).
  friend class CDigitalGuardian;

  void EnsureBodyActive(CStateManager& mgr);

  // fn_14_13830 (0x194 bytes): the "XALD" case - the body (CONN/ATCH, linked through its
  // fn_14_7168), the IS04/ACTV object, the MDFY/FOLW waypoint and the MDFY/PLAY platforms, then
  // fn_14_119E4.
  void x13830_(CStateManager& mgr);
  // fn_14_119E4 (0x1C4 bytes): the IS03/FOLW waypoint pair's midpoint and axes (0x1004..0x1024).
  void x119E4_(CStateManager& mgr);
  // fn_14_13AE0 (0x28 bytes): the three timers.
  void x13AE0_(float dt);
  // fn_14_12818 (0x2EC bytes): with the armour gone, a morph ball sitting on an intact ball's
  // bomb slot is held there and makes the ball damageable (0xDD0); a destroyed slot's sound
  // follows its locator.
  void x12818_(CStateManager& mgr, float dt);
  // fn_14_126C8 (0x150 bytes): the bombs in the box of half-size half round pos move to pos.
  void x126C8_(CStateManager& mgr, const CVector3f& pos, float half);
  // fn_14_13318 (0x490 bytes): the collision actor manager (two ball spheres and the root),
  // the colliders' vulnerabilities and material bits, then the head's own material filter.
  void x13318_(CStateManager& mgr);
  // fn_14_13284 (0x94 bytes): a ball collider ("L_ball_LCTR"/"R_ball_LCTR") keeps its id at
  // 0xDC4 + 2 * index; true for a ball.
  bool x13284_(const rstl::string& name, const TUniqueId& id);
  // fn_14_13B38 (0xE4 bytes): the horizontal facing direction.
  CVector3f x13B38_() const;
  // fn_14_11C08 (0x15C bytes): a direction with its pitch limited to 55 degrees.
  CVector3f x11C08_(const CVector3f& dir) const;
  // fn_14_11D64 (0x144 bytes): turn toward the face target (0x1036).
  void x11D64_(CStateManager& mgr, float dt);
  // fn_14_11698 (0xF0 bytes): pos's signed angle around the attack area's centre.
  bool x11698_(const CVector3f& pos, float& angle) const;
  // fn_14_11788 (0x168 bytes): the point at that angle on the attack area's ellipse.
  CVector3f x11788_(float angle) const;
  // fn_14_118B0 (0x134 bytes): the attack area's points at the head's angle and 8 degrees either
  // side; returns the count (0 or 3).
  int x118B0_(CVector3f* points) const;
  // fn_14_114C8 (0x1D0 bytes): the attack point with a line of sight to the player (stand-in).
  CVector3f x114C8_(const CVector3f& player, const CVector3f* points, int count);
  // fn_14_F390 (0x94 bytes): the splitters to deploy, one more than the echo targets lost, at
  // most 2.
  int xF390_(CStateManager& mgr) const;
  // fn_14_F424 (0x280 bytes): eject one splitter from "B_ball_LCTR".
  void xF424_(CStateManager& mgr);
  // fn_14_12314 (0x3B4 bytes): the three echo targets round locator 1, their offsets at 0xF40.
  void x12314_(CStateManager& mgr);
  // fn_14_1223C (0xD8 bytes): the echo targets follow locator 1 while the armour stands.
  void x1223C_(CStateManager& mgr);
  // fn_14_1303C (0x20C bytes): "XDMG". With the armour gone a dead ball collider costs a life;
  // before, a destroyed echo target (state 4) is removed.
  void x1303C_(CStateManager& mgr, TUniqueId sender);
  // fn_14_12D98 (0x248 bytes): "XRDG". A weapon resisted by the root collider wears down the
  // gravity repulsor (0x10E0).
  void x12D98_(CStateManager& mgr, TUniqueId sender);
  // fn_14_F2B8 (0xC4 bytes): move the IS07/FOLW waypoint to pos and send state.
  void xF2B8_(CStateManager& mgr, const CVector3f& pos, EScriptObjectState state);
  // fn_14_13248 (0x3C bytes): a ball collider's index (0xDC4), or -1.
  int x13248_(TUniqueId id) const;
  // fn_14_11F30 (0x5C bytes): whether id is one of the echo targets (0xF34).
  bool x11F30_(TUniqueId id) const;
  // fn_14_11FEC (0x1D4 bytes): an echo target was destroyed at xf.
  void x11FEC_(CStateManager& mgr, const CTransform4f& xf);
  // fn_14_12B04 (0x234 bytes): ball idx's bomb slot was destroyed.
  void x12B04_(CStateManager& mgr, int idx);

  SLdrDigitalGuardianHeadData mData; // guest 0x7C0: the parameter block (fn_14_1912C copies it)
  CCollisionActorManager* xD70_;     // 0xD70: the collision actor manager (null until built)
  TUniqueId xDC4_[2]; // 0xDC4: the ball colliders' ids (fn_14_13284; count 2 at 0xDC0)
  CSegId xDCC_[2];    // 0xDCC: the balls' locators ("L_ball_LCTR", "R_ball_LCTR")
  CDamageVulnerability xDD0_; // 0xDD0: a ball's vulnerability with a bomb slot in use (data +0x2F8)
  TUniqueId xE00_;    // 0xE00: the root collider's id (fn_14_13318)
  // 0xDA0 / 0xDB0 / 0xDB8: the visual knowledge's target id, its sight time and its flag byte
  // (CVisualKnowledge at 0xD80; HasLineOfSight reads them). The class is not ported, so they keep
  // the "no target" value.
  TUniqueId xDA0_;
  float xDB0_;
  uchar xDB8_;
  // 0xEF4: the drop-shields effect (data +0x48), its valid flag at 0xF00.
  rstl::optional_object< TLockedToken< CGenDescription > > xEF4_;
  // 0xF04: the 'PART' token (data +0x94), its valid flag at 0xF10.
  rstl::optional_object< TLockedToken< CGenDescription > > xF04_;
  // 0xF14: the bomb-slot-destroyed effect (data +0x138), its valid flag at 0xF20.
  rstl::optional_object< TLockedToken< CGenDescription > > xF14_;
  CSfxHandle xF28_[2]; // 0xF28: the destroyed bomb slots' looping sounds
  // 0xF30 / 0xF34: the echo targets' ids (count 3, then the ids); fn_14_12314 fills them.
  rstl::reserved_vector< TUniqueId, 3 > xF34_;
  CTransform4f xF40_[3]; // 0xF40: the echo targets' offsets from locator 1 (fn_14_12314)
  // 0xFD0: the echo-target-destroyed effect (data +0xA4), its valid flag at 0xFDC.
  rstl::optional_object< TLockedToken< CGenDescription > > xFD0_;
  int xFE0_;        // 0xFE0: the echo targets left (3)
  int xFE8_;        // 0xFE8: the attack mode (1 in the ctor; fn_14_13B30)
  CVector3f xFEC_;  // 0xFEC: the attack position (SetInitialAttackPosition, SeekAttackPosition)
  CVector3f xFF8_;  // 0xFF8: the previous attack position (SetAttackPosition)
  CVector3f x1004_; // 0x1004: the attack area's centre (fn_14_119E4)
  CVector3f x1010_; // 0x1010: the attack area's first axis
  CVector3f x101C_; // 0x101C: the attack area's second axis
  CVector3f x1028_; // 0x1028: the direction the head turns to (fn_14_11D64)
  TUniqueId x1034_; // 0x1034: the body (fn_14_13830)
  TUniqueId x1036_; // 0x1036: the face target (SetPlayerFaceTarget)
  TUniqueId x1038_; // 0x1038: the retreat platform (SelectRetreatPlatform)
  TUniqueId x103A_; // 0x103A: the destroyed bomb pit (BombPitDestroyed)
  TUniqueId x103C_; // 0x103C: the MDFY/FOLW waypoint
  TUniqueId x103E_; // 0x103E: the IS04/ACTV object
  TUniqueId x1040_; // 0x1040
  rstl::vector< TUniqueId > x1044_; // 0x1044: the MDFY/PLAY platforms
  int x10D0_;      // 0x10D0: the projectile attack (-1; 0 pulse, 1 charge, 2 missiles, 3 gun)
  float x10D4_;    // 0x10D4: the projectile attack timer
  int x10D8_;      // 0x10D8: the burst count
  int x10DC_;      // 0x10DC: the first-shot latch (-1; SelectDeathHeadAttack clears it)
  float x10E0_;    // 0x10E0: the gravity repulsor's health (data +0x18)
  int x10E4_;      // 0x10E4: the lives left (2)
  int x10E8_;      // 0x10E8: the splitters left to deploy (StartSplitterDeployment)
  float x10EC_;    // 0x10EC: the time since the last splitter (fn_14_13AE0; 4 s apart)
  float x10F0_;    // 0x10F0: the deployment time (fn_14_13AE0)
  uchar x10F4_;    // 0x10F4: the flag byte (0x80 IsIdle, 0x10 HeadArmorDestroyed, 0x02
                   // EchoTargetDestroyed; 0x20 stops the body's attacks; 0x08, set by the ctor,
                   // and 0x40 pick the scan info in fn_14_177C8)
  uchar x10F5_;    // 0x10F5: the second flag byte
#else
  uchar x7c0_[0x10F8 - 0x7c0]; // the parameter block and everything above it in retail
#endif
};

#ifdef TARGET_PC
// fn_14_DF7C: the DGHD loader (the module's second SDigitalGuardian_FuncPtrs slot).
CEntity* PortLoadDigitalGuardianHead(CStateManager& mgr, CInputStream& input, CEntityInfo& info);
#endif

#endif
