#ifndef _CEMPERORINGSTAGE3
#define _CEMPERORINGSTAGE3

#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "REL/REL_Setup.h"

#ifdef TARGET_PC
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/ScriptLoader/SLdrEmperorIngStage3.hpp"
#endif

// The REL's CEmperorIngStage3 (module 18): the mutated Emperor Ing, the boss the player fights
// in the Great Temple's moth lord sanctuary (dark), layer 4. 0xED0 bytes on the GameCube (the
// loader's rs_new at 0x0000C598), built by fn_18_9A8C, its vtable lbl_18_data_4F0 (80 entries, 21
// of them local) and the analysis is docs/module-analysis/EmperorIngStage3.md.
//
// Milestone 1 (this file's first slice) is load-and-stand: the loader, the constructor and the
// accessors the vtable gives one-line bodies to. The state machine's tables (fn_18_943C installs
// the 21 triggers, 21 states and 2 code functions), the four attacks (melee, stampede, jump slide,
// ground pound), the two beams, the portal attack and the light swarm are milestone 2.
//
// The host layout is its own: the retail object is 0xED0 bytes, CPatterned is 0x7C0, and the guest
// offset of every member the constructor touches is in the comment above it. Host objects
// outgrow retail, so the class is allocated with sizeof, not with 0xED0.
#ifdef TARGET_PC
class REL_EXPORT CEmperorIngStage3 : public CPatterned {
public:
  // Argument order is the REL loader's call at 0x0000C620 (fn_18_C380 -> fn_18_9A8C): uid,
  // editor-property name, entity info, transform, model, actor parameters, patterned info, then
  // the script's data block (the REL passes it on the stack, the slot 0x308 at 0x0000C5B8).
  CEmperorIngStage3(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                     const CTransform4f& xf, const CModelData& modelData,
                     const CActorParameters& actParms, const CPatternedInfo& pInfo,
                     const SLdrEmperorIngStage3Data& data);
  ~CEmperorIngStage3() override;

  // CEntity
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void Render(const CStateManager& mgr) const override;
  const CHealthInfo* GetHealthInfo() const override;
  rstl::optional_object< CAABox > GetTouchBounds() const override;
  CVector3f GetAimPosition(const CStateManager& mgr, float param) const override;
  CAABox GetSortingBounds(const CStateManager& mgr) const override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;

  // CAi
  void TakeDamage(const CVector3f& direction, float magnitude) override;
  bool CanBeShot(const CStateManager&, int) override;
  bool IsListening() const override;
  bool Listen(CStateManager& mgr, const CVector3f& position, EListenNoiseType type) override;
  CVector3f GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                      const CVector3f& aimPos) const override;

  // CPatterned
  uchar GetModelAlphau8(const CStateManager& mgr) const override;
  CProjectileInfo* ProjectileInfo() override;
  CPathFindSearch* GetSearchPath() override;
  CPathFindPointSearch* GetPointSearchPath() override;
  CRagDoll* GetRagDoll() const override;
  TUniqueId GetAttackTarget() const override;
  bool IsOnGround() const override;
  float GetGravityConstant() const override;
  void SetupStateMachine(CStateManager& mgr) override;

  // The body's state machine. Retail fn_18_943C (vtable slot 60) calls the base's setup and
  // then registers the three tables fn_18_E180 builds: 21 triggers (lbl_18_data_34C), 21 states
  // (lbl_18_data_100) and 2 codes (lbl_18_data_4B4). The names are retail's - the state machine
  // asset's nodes are matched by name - and each comment is the retail function the entry points
  // at. The triggers whose retail body is small enough are ported; the rest are stand-ins that log
  // the retail function they stand in for.
  //
  // Triggers (lbl_18_data_34C, 21 entries).
  bool TooClose(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8DA4
  bool ShouldAttack(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8D88
  bool ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8C4C
  bool ShouldTaunt(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8A4C
  bool ShouldStampede(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_897C
  bool ShouldDodge(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8A34
  bool ShouldChangeVulnerabilityToVulnerable(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8A20
  bool ShouldChangeVulnerabilityToInvulnerable(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_898C
  bool ShouldDarkBeamAttack(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8968
  bool ShouldLightBeamAttack(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8954
  bool ShouldLightSwarmAttack(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_87F0
  bool ShouldGroundPound(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8ACC
  bool ShouldGenerateDarkFliers(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_868C
  bool ShouldPortalAttack(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8678
  bool ShouldJumpSlideAttack(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8664
  bool ShouldGrowHeart(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8648
  bool JumpSlideAttackOver(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8600
  bool ShouldEyeShut(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_85A4
  bool AbortedJumpSlide(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_858C
  bool ShouldJumpAttack(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8518
  bool ShouldStampedeReact(CStateManager& mgr, const CTriggerData& data) const;  // fn_18_8500

  // States (lbl_18_data_100, 21 entries).
  void Start(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_7ADC
  void Idle(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_788C
  void MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_76D0
  void GainDistance(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_7504
  void Stampede(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_71D8
  void Dodge(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_6E80
  void Taunt(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_6D84
  void ChangeVulnerability(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_6A40
  void EyeShutResetVulnerability(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_6B80
  void DarkBeamAttack(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_6904
  void LightBeamAttack(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_67C0
  void LightSwarmAttack(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_6698
  void GroundPound(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_6390
  void GenerateDarkFliers(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_6258
  void PortalAttack(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_60AC
  void JumpSlideAttack(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_5844
  void GrowHeart(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_5614
  void EyeShut(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_5264
  void JumpAttack(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_4EB4
  void StampedeReact(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_4CC8
  void Dead(CStateManager& mgr, EStateMsg msg, float dt);  // fn_18_4C3C

  // Codes (lbl_18_data_4B4, 2 entries).
  void ResetAttackTimer(CStateManager& mgr, float dt);  // fn_18_84A8
  void ChoseAction(CStateManager& mgr, float dt);  // fn_18_8200

private:
  void EnsureBodyActive(CStateManager& mgr);
  // fn_18_F8: the module's random check, used by the attack triggers. It is a stand-in for now, so
  // the triggers that need it answer false.
  bool RandomCheck() const;
  // fn_18_2298: the taunt average over the module's bone list. Its own callee fn_18_AB9C needs the
  // list at guest 0x840, which the port does not model, so this stays a stand-in too.
  void TauntAverage() const;

  // Host layout. The guest offsets are the REL's (fn_18_9A8C); the full member map, including
  // the parts this milestone does not build, is in docs/module-analysis/EmperorIngStage3.md.
  float mVulnerableDamageThreshold;      // guest 0x814, the data's vulnerableDamageThreshold
  float mTauntFrequency;                 // guest 0x91C, the data's tauntFrequency
  CHealthInfo mYellowHealth;             // guest 0x920, the data's yellowHealth
  CHealthInfo mHealth;                   // guest 0x940, the data's health
  CDamageVulnerability mRedVulnerability;   // guest 0x960, the data's redVulnerability
  CDamageVulnerability mLightVulnerability; // guest 0x990, the data's lightVulnerability
  CDamageVulnerability mDarkVulnerability;  // guest 0x9C0, the data's darkVulnerability
  CDamageInfo mMeleeDamage;              // guest 0x9F0, the data's meleeDamage
  CDamageInfo mStampedeDamage;           // guest 0xA0C, the data's stampedeDamage
  CDamageInfo mJumpSlideDamage;          // guest 0xA28, the data's jumpSlideDamage
  CDamageInfo mGroundPoundDamage;        // guest 0xA44, the data's groundPoundDamage
  // fn_18_44: GetModelAlphau8 reads the byte at guest 0x44F. It sits in CPatterned's own area in
  // the retail object, so the host keeps it here.
  // The module's own two flag bytes at guest 0x824 and 0x825. The retail word at 0x824 is 32 bits
  // wide, but every test the module makes is a single byte's bit, so the host keeps the two bytes.
  uchar x824_;                           // guest 0x824, 0: 0x02 the listen flag, 0x04/0x08 the
                                        // jump-attack and dodge flags
  uchar x825_;                           // guest 0x825, 0: 0x01 the stampede-react flag, 0x04 the
                                        // aborted-jump-slide flag
  int x7c0_;                             // guest 0x7C0, 0: cleared by the constructor
  int mVulnerabilityState;               // guest 0x7C4, 0: 1 is the vulnerable state
  float mX7c8_;                          // guest 0x7C8, 0.0f: the taunt timer
  float mX7cc_;                          // guest 0x7CC, 0.0f: the eye-shut timer
  float mX7d4_;                          // guest 0x7D4, 0.0f: the too-close distance
  float mX7e0_;                          // guest 0x7E0, 10000.0f: the jump-attack timer
  float mAttackTimer;                    // guest 0x7E4, 4.0f in the constructor
  int mAction;                           // guest 0x7E8, -1 in the constructor
  float mX810_;                          // guest 0x810, 0.0f: the damage taken while vulnerable
};
#endif

#endif
