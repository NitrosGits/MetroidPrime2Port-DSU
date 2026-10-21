#ifndef _CSANDWORM
#define _CSANDWORM

#include "Kyoto/Math/CVector3f.hpp"
#include "MetroidPrime/CActor.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#ifdef TARGET_PC
#include "Kyoto/Animation/CSegId.hpp"
#include "Kyoto/Animation/CharacterCommon.hpp"
#include "Kyoto/Audio/CSfxHandle.hpp"
#include "Kyoto/CToken.hpp"
#include "Kyoto/Math/CAABox.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CTransform4f.hpp"
#include "Kyoto/TToken.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CPhysicsActor.hpp"
#include "MetroidPrime/CRELFileToken.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSandworm.hpp"
#include "MetroidPrime/StateMachineCommon.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"
#include "rstl/optional_object.hpp"
#include "rstl/reserved_vector.hpp"
#include "rstl/single_ptr.hpp"

class CBCGenerateCmd;
class CBCMeleeAttackCmd;
class CBCTauntCmd;
class CBouncingBomb;
class CCollisionActorManager;
class CElementGen;
class CGenDescription;
class CProjectedShadow;
class CSandwormEye;
class CTriggerData;

// Port of the retail Sandworm REL (module 56): the Sandigger/Alpha form and the Bomb Guardian.
// docs/module-analysis/Sandworm.md and Sandworm_functions.tsv map every function (fn_56_*) to a
// work package; the bodies live in src/MetroidPrime/Enemies/PortCSandworm*.cpp. Member names are
// the retail offsets; a short comment gives the meaning where a function proves it.

// x1490. fn_56_28F0 ctor, fn_56_285C dtor, fn_56_26C4 EndsWith.
struct SSandwormAttackHistory {
  SSandwormAttackHistory();                 // fn_56_28F0
  bool EndsWith(int type, int count) const; // fn_56_26C4: the last `count` entries are `type`

  int x0_picked;                              // -1; 1 spit, 5 bomb toss, 6 bomb spread, 7 fountain
  int x4_current;                             // -1
  rstl::reserved_vector< int, 5 > x8_history; // capacity probable
};

// x14D8. fn_56_13B80.
struct SSandwormCharge {
  SSandwormCharge(float rangeMin, float rangeMax, float impulseH, float impulseV); // fn_56_13B80

  CVector3f x0_;
  float xc_rangeMin;
  float x10_rangeMax;
  float x14_impulseH; // clamped to <= 50 (fn_56_128B0)
  float x18_impulseV; // clamped to <= 50
  CVector3f x1c_;
  float x28_;
  float x2c_;
  u8 x30_flags; // b7 ReadyToCharge (fn_56_97F0), b6
};

// x1528. fn_56_138F0 ctor, fn_56_126EC dtor (compiler-generated here).
struct SSandwormSpit {
  SSandwormSpit(CAssetId projectile, const CDamageInfo& damage, CAssetId visorEffect, float x5c,
                float rangeMin, float rangeMax); // fn_56_138F0

  CProjectileInfo x0_info;
  CTransform4f x28_xf;
  float x58_;
  float x5c_;        // unknown_0x61f75902
  float x60_rangeMin; // spitAttackMinRange
  float x64_rangeMax; // spitAttackMaxRange
  CVector3f x68_;
  rstl::optional_object< TLockedToken< CGenDescription > > x74_visorEffect; // 'PART'
};

// x15AC. fn_56_137F8: morphball toss.
struct SSandwormToss {
  SSandwormToss(const CDamageInfo& damage, float impulseH, float impulseV); // fn_56_137F8

  CTransform4f x0_xf;
  CDamageInfo x30_damage; // morphballTossDamage
  float x4c_impulseH;     // clamped to <= 50
  float x50_impulseV;     // clamped to <= 50
  float x54_;
  u8 x58_flags; // b7 teleport the player to x0_xf (Think, fn_56_5E94); b6 GrabAttackFinished; b5
};

// x1620. fn_56_13790: pincer swipe.
struct SSandwormMelee {
  SSandwormMelee(const CDamageInfo& damage, float impulseH, float impulseV); // fn_56_13790

  CDamageInfo x0_damage; // pincerSwipeDamage
  float x1c_;
  bool x20_;
  float x24_impulseH; // clamped to <= 50
  float x28_impulseV; // clamped to <= 50
};

// x1670. fn_56_136E0.
struct SSandworm1670 {
  SSandworm1670(); // fn_56_136E0

  rstl::reserved_vector< CQuaternion, 13 > x0_; // count only is set by the ctor [probable type]
  CVector3f xd4_;
  CQuaternion xe0_;
  CVector3f xf0_;
  float xfc_;
};

// x1770. fn_56_12780 dtor (compiler-generated here); models built in fn_56_128B0.
struct SSandwormPincers {
  CAssetId x0_left;  // pincersL CMDL
  CAssetId x4_right; // pincersR CMDL
  float x8_scale;    // pincerScale
  rstl::optional_object< CModelData > xc_leftModel;
  rstl::optional_object< CModelData > x5c_rightModel;
};

// x1828. fn_56_13668.
struct SSandwormSfx {
  SSandwormSfx(ushort walk, ushort walkVocal, ushort melee, ushort eyeKilled, ushort bombBounce,
               ushort bombExplode); // fn_56_13668

  ushort x0_walk;
  ushort x2_walkVocal;
  ushort x4_melee;
  ushort x6_eyeKilled;
  ushort x8_bombBounce;  // fn_56_9648
  ushort xa_bombExplode; // fn_56_9640
  float xc_;
  float x10_;
  CSfxHandle x14_; // looped emitters (fn_56_3EAC)
  CSfxHandle x18_; // move loop (fn_56_403C)
  CSfxHandle x1c_;
};

// x1944. fn_56_1E9C / fn_56_1FD8 ctors, fn_56_1280C dtor.
struct SSandwormBombs {
  SSandwormBombs();                                                     // fn_56_1E9C (Sandigger)
  SSandwormBombs(CAssetId bombFx, CAssetId explosionFx, const CDamageInfo& damage); // fn_56_1FD8
  ~SSandwormBombs();                                                    // fn_56_1280C

  rstl::optional_object< TLockedToken< CGenDescription > > x0_bombFx;      // ingBossBombFX
  rstl::optional_object< TLockedToken< CGenDescription > > x10_explosionFx; // ingBossBombExplosionFX
  CDamageInfo x20_damage;                                                  // ingBossBombDamage
};

// x19A8. fn_56_1359C (Sandigger) / fn_56_149D4 (Bomb Guardian).
struct SSandwormBombPhase {
  SSandwormBombPhase();                            // fn_56_1359C
  SSandwormBombPhase(float x18, float x10);        // fn_56_149D4

  CVector3f x0_;
  u8 xc_flags; // b7 HitDuringWindUp (fn_56_35EC), b6, b5
  float x10_;  // unknown_0xefef7b45 (Bomb Guardian)
  float x14_;
  float x18_; // unknown_0x547f9400 (Bomb Guardian)
  float x1c_;
  float x20_;
  float x24_; // -1000
};

// x19D0. fn_56_1A38 ctor, fn_56_191C dtor: up to four owned eye-glow particle generators.
struct SSandwormGlow {
  SSandwormGlow(CAssetId glow, CAssetId part1, CAssetId part2); // fn_56_1A38
  ~SSandwormGlow();                                             // fn_56_191C

  bool x0_has0;
  CElementGen* x4_gen0; // eyeGlow
  bool x8_has1;
  CElementGen* xc_gen1; // eyeGlow
  bool x10_has2;
  CElementGen* x14_gen2; // pART (Bomb Guardian; AddToRenderer)
  bool x18_has3;
  CElementGen* x1c_gen3; // pART_0x8b2a15ee (Bomb Guardian; AddToRenderer)

private:
  SSandwormGlow(const SSandwormGlow&);
  SSandwormGlow& operator=(const SSandwormGlow&);
};

#endif // TARGET_PC


// Original class names from the Wii SEL exports. Both live in the Sandworm REL; the DOL only
// proves the bases through their TypesMatch parents, so the layouts are left opaque.
class CSandworm : public CPatterned {
public:
  // CEntity
  ~CSandworm() override;
  CEntity* TypesMatch(int typeId) const override;
#ifdef TARGET_PC
  // Sandigger/Alpha form (canLinkTransfer == false).
  CSandworm(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
            const CTransform4f& xf, const CModelData& modelData, const CPatternedInfo& pInfo,
            CAssetId stateMachine2, float pincerScale, float spitMin, float spitMax,
            float spitUnknown, float chargeMin, float chargeMax, float chargeImpulseH,
            float chargeImpulseV, bool flag, CAssetId pincerL, CAssetId pincerR, ushort walkSfx,
            ushort walkVocalSfx, ushort meleeSfx, ushort eyeKilledSfx, CAssetId visorEffect,
            float tossImpulseH, float tossImpulseV, float meleeImpulseH, float meleeImpulseV,
            float unknownE593, float lurkMin, float lurkMax, float frustrationRadius,
            float frustrationTimer, CAssetId projectile, const CDamageInfo& spitDamage,
            const CDamageInfo& tossDamage, const CDamageInfo& meleeDamage, CAssetId eyeGlow,
            const CActorParameters& actorParams); // fn_56_12D68
  // Bomb Guardian form (canLinkTransfer == true).
  CSandworm(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
            const CTransform4f& xf, const CModelData& modelData, const CPatternedInfo& pInfo,
            CAssetId stateMachine2, float pincerScale, float spitMin, float spitMax,
            float spitUnknown, float chargeMin, float chargeMax, float chargeImpulseH,
            float chargeImpulseV, bool flag, CAssetId pincerL, CAssetId pincerR, ushort walkSfx,
            ushort walkVocalSfx, ushort meleeSfx, ushort eyeKilledSfx, ushort bombBounceSfx,
            ushort bombExplodeSfx, CAssetId visorEffect, float tossImpulseH, float tossImpulseV,
            float meleeImpulseH, float meleeImpulseV, float unknownE593, float lurkMin,
            float lurkMax, float frustrationRadius, float frustrationTimer, CAssetId projectile,
            const CDamageInfo& spitDamage, const CDamageInfo& tossDamage,
            const CDamageInfo& meleeDamage, CAssetId eyeGlow, CAssetId part,
            CAssetId part8b2a, CAssetId bombFx, CAssetId bombExplosionFx,
            const CDamageInfo& bombDamage, float bombDropRate, float unknown547f,
            float unknownEfef, const SLdrSandwormStruct& phase0, const SLdrSandwormStruct& phase1,
            const SLdrSandwormStruct& phase2, const SLdrSandwormStruct& phase3,
            const SLdrSandwormStruct& phase4, const CActorParameters& actorParams); // fn_56_14010
  // ~CSandworm: fn_56_1223C

  void Think(float dt, CStateManager& mgr) override;                        // fn_56_110C0
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override; // fn_56_11374

  // CActor
  void PreRender(CStateManager& mgr) override;                      // fn_56_A4F0
  void AddToRenderer(const CStateManager& mgr) const override;      // fn_56_4A14
  void Render(const CStateManager& mgr) const override;             // fn_56_E3C0
  void PreRenderAllViewports(CStateManager& mgr) override;          // fn_56_9CD8
  rstl::optional_object< CAABox > GetTouchBounds() const override;  // fn_56_14A74
  CVector3f GetOrbitPosition(const CStateManager& mgr) const override;          // fn_56_2C30
  CVector3f GetAimPosition(const CStateManager& mgr, float dt) const override;  // fn_56_321C
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override; // fn_56_646C

  // CAi
  void TakeDamage(const CVector3f& direction, float amount) override;       // fn_56_11CE8
  bool CanBeShot(const CStateManager& mgr, int) override;                    // fn_56_14B24
  bool IsListening() const override;                                         // fn_56_14B2C
  bool Listen(CStateManager& mgr, const CVector3f& position, EListenNoiseType type) override; // fn_56_14B34
  CVector3f GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                      const CVector3f& aimPos) const override; // fn_56_14B3C

  // CPatterned
  uchar GetModelAlphau8(const CStateManager& mgr) const override;      // fn_56_14ADC
  CProjectileInfo* ProjectileInfo() override;                          // fn_56_6790
  CPathFindSearch* GetSearchPath() override;                           // fn_56_14A64
  CPathFindPointSearch* GetPointSearchPath() override;                 // fn_56_14AE4
  CRagDoll* GetRagDoll() const override;                               // fn_56_14AEC
  bool CanBeIngPossessed(CStateManager& mgr) const override;           // fn_56_30F8
  bool CanBeUnPossessed(CStateManager& mgr) const override;            // fn_56_30F0
  TUniqueId GetAttackTarget() const override;                          // fn_56_14AF4
  bool IsOnGround() const override;                                    // fn_56_14B04
  float GetGravityConstant() const override;                           // fn_56_14B10
  const rstl::optional_object< TCachedToken< CGenDescription > >&
  GetDeathExplosionParticle() const override;                          // fn_56_14B1C
  CVector3f GetIngSnatchingNormal(float t) const override;             // fn_56_3180
  CVector3f GetIngSnatchingPoint(float t) const override;              // fn_56_312C
  float GetIngSnatchingModelOverlapSize() const override;              // fn_56_9860
  void RenderIngSnatchingTransition(const CStateManager& mgr) const override; // fn_56_2994

  // Retail adds these as new virtuals (vtable 0x148..0x23C); nothing overrides them, so the port
  // keeps them non-virtual. Triggers (0x148..0x1D8):
  bool Activated(CStateManager& mgr, const CTriggerData& data) const;             // fn_56_C3D0
  bool BeginUnderground(CStateManager& mgr, const CTriggerData& data) const;      // fn_56_C3E8
  bool CanAttack(CStateManager& mgr, const CTriggerData& data) const;             // fn_56_59A8
  bool CanCharge(CStateManager& mgr, const CTriggerData& data) const;             // fn_56_5AE0
  bool CanDescend(CStateManager& mgr, const CTriggerData& data) const;            // fn_56_3E1C
  bool CanGrabMorphball(CStateManager& mgr, const CTriggerData& data) const;      // fn_56_C168
  bool CanMeleeAgain(CStateManager& mgr, const CTriggerData& data) const;         // fn_56_573C
  bool CanSpitAgain(CStateManager& mgr, const CTriggerData& data) const;          // fn_56_3DE4
  bool ChargeOver(CStateManager& mgr, const CTriggerData& data) const;            // fn_56_6798
  bool DoneLurking(CStateManager& mgr, const CTriggerData& data) const;           // fn_56_58C0
  bool DoneStraightening(CStateManager& mgr, const CTriggerData& data) const;     // fn_56_4384
  bool DoneSulking(CStateManager& mgr, const CTriggerData& data) const;           // fn_56_541C
  bool FacingPlayer(CStateManager& mgr, const CTriggerData& data) const;          // fn_56_6A50
  bool FacingPlayerForCharge(CStateManager& mgr, const CTriggerData& data) const; // fn_56_6904
  bool ForceUnderground(CStateManager& mgr, const CTriggerData& data) const;      // fn_56_3E6C
  bool GrabAttackFinished(CStateManager& mgr, const CTriggerData& data) const;    // fn_56_BF38
  bool HitDuringWindUp(CStateManager& mgr, const CTriggerData& data) const;       // fn_56_35EC
  bool InAttackRange(CStateManager& mgr, const CTriggerData& data) const;         // fn_56_C350
  bool InChargeRange(CStateManager& mgr, const CTriggerData& data) const;         // fn_56_6C04
  bool InSpitRange(CStateManager& mgr, const CTriggerData& data) const;           // fn_56_C27C
  bool MeleeAttackRange(CStateManager& mgr, const CTriggerData& data) const;      // fn_56_73BC
  bool OneEyeKilled(CStateManager& mgr, const CTriggerData& data) const;          // fn_56_35F8
  bool PickedBombFountain(CStateManager& mgr, const CTriggerData& data) const;    // fn_56_3750
  bool PickedBombSpread(CStateManager& mgr, const CTriggerData& data) const;      // fn_56_3764
  bool PickedBombToss(CStateManager& mgr, const CTriggerData& data) const;        // fn_56_3778
  bool PickedSpitAttack(CStateManager& mgr, const CTriggerData& data) const;      // fn_56_378C
  bool PlayerHiding(CStateManager& mgr, const CTriggerData& data) const;          // fn_56_3E78
  bool PlayerIsOnPathMesh(CStateManager& mgr, const CTriggerData& data) const;    // fn_56_5978
  bool PlayerIsMorphball(CStateManager& mgr, const CTriggerData& data) const;     // fn_56_7390
  bool PlayerReachable(CStateManager& mgr, const CTriggerData& data) const;       // fn_56_3D44
  bool Primed(CStateManager& mgr, const CTriggerData& data) const;                // fn_56_F710
  bool ReadyToCharge(CStateManager& mgr, const CTriggerData& data) const;         // fn_56_97F0
  bool SequenceAttack(CStateManager& mgr, const CTriggerData& data) const;        // fn_56_5530
  bool ShouldSulk(CStateManager& mgr, const CTriggerData& data) const;            // fn_56_557C
  bool SnatchStarted(CStateManager& mgr, const CTriggerData& data) const;         // fn_56_981C
  bool SnatchEnded(CStateManager& mgr, const CTriggerData& data) const;           // fn_56_97FC
  bool SpitAngleOK(CStateManager& mgr, const CTriggerData& data) const;           // fn_56_69DC
  // States (0x1DC..0x234):
  void BombFountainAttack(CStateManager& mgr, EStateMsg msg, float dt); // fn_56_B8B8
  void BombSpreadAttack(CStateManager& mgr, EStateMsg msg, float dt);   // fn_56_BAB8
  void BombTossAttack(CStateManager& mgr, EStateMsg msg, float dt);     // fn_56_BC8C
  void ChargePlayer(CStateManager& mgr, EStateMsg msg, float dt);       // fn_56_8190
  void ChargeWindUp(CStateManager& mgr, EStateMsg msg, float dt);       // fn_56_99A0
  void Deactivate(CStateManager& mgr, EStateMsg msg, float dt);         // fn_56_5E5C
  void Dead(CStateManager& mgr, EStateMsg msg, float dt);               // fn_56_5D80
  void Descend(CStateManager& mgr, EStateMsg msg, float dt);            // fn_56_B320
  void EyeKilledReaction(CStateManager& mgr, EStateMsg msg, float dt);  // fn_56_3484
  void GrabAttack(CStateManager& mgr, EStateMsg msg, float dt);         // fn_56_BF44
  void LurkUnderground(CStateManager& mgr, EStateMsg msg, float dt);    // fn_56_C3F4
  void MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt);        // fn_56_6FA0
  void Null(CStateManager& mgr, EStateMsg msg, float dt);               // fn_56_112E8
  void Patrol(CStateManager& mgr, EStateMsg msg, float dt);             // fn_56_AA74
  void PickMediumAttack(CStateManager& mgr, EStateMsg msg, float dt);   // fn_56_37A0
  void Prime(CStateManager& mgr, EStateMsg msg, float dt);              // fn_56_F754
  void Pursue(CStateManager& mgr, EStateMsg msg, float dt);             // fn_56_74B8
  void Rise(CStateManager& mgr, EStateMsg msg, float dt);               // fn_56_B50C
  void Snatch(CStateManager& mgr, EStateMsg msg, float dt);             // fn_56_986C
  void SpitAttack(CStateManager& mgr, EStateMsg msg, float dt);         // fn_56_B6DC
  void StopGrabAttack(CStateManager& mgr, EStateMsg msg, float dt);     // fn_56_BEA4
  void Straighten(CStateManager& mgr, EStateMsg msg, float dt);         // fn_56_4200
  void Sulk(CStateManager& mgr, EStateMsg msg, float dt);               // fn_56_5470
  // Code function (0x238, table data_6A0 "Interrupt") and slot 0x23C.
  void Interrupt(CStateManager& mgr, float dt); // fn_56_33A4
  int Unknown23C() const;                       // fn_56_14A6C: returns 0

  // Radar callbacks registered in SSandworm_FuncPtrs.
  int GetRadarPointCount() const;                // fn_56_5910
  CVector3f GetRadarPointPosition(int index) const; // fn_56_5938

  // WP1: construction, Think and its per-frame helpers.
  void FinishConstruction();                       // fn_56_128B0
  void ResetFade();                                // fn_56_121D8: x12E8[] = 0
  void InitStateMachine(CStateManager& mgr);       // fn_56_D548
  const SLdrSandwormStruct& GetPhaseParams() const; // fn_56_3990
  bool IsPossessedBombGuardian() const;            // fn_56_3A20
  void SetBodyCmdRequest(int first, int last);     // fn_56_A9F4: x1518/x151C
  void ClearBodyCmdRequest();                      // fn_56_AA00: (-1, 13)
  CVector3f GetMoveTarget(const CStateManager& mgr) const; // fn_56_AA28
  void UpdateTimers(CStateManager& mgr, float dt); // fn_56_10AD8: x150C/x1510
  void UpdateFade(float dt);                       // fn_56_10BE8: x12E8[]
  static float ApproachValue(float cur, float target, float step); // fn_56_10CA4
  void DecayX181C(float dt);                       // fn_56_10CD4: x181C/x1820
  void UpdateColorLerp(float dt);                  // fn_56_10D2C
  void DetectIncomingProjectiles(CStateManager& mgr); // fn_56_10E04
  void UpdateBossParams(CStateManager& mgr);       // fn_56_10F90
  void SpawnResidue(CStateManager& mgr, int eye);  // fn_56_11FAC: eye unused
  void DecayX1350(float dt);                       // fn_56_5C7C
  void TeleportGrabbedPlayer(CStateManager& mgr);  // fn_56_5E94
  void IssueTauntCmd(EStateMsg msg, pas::EAnimationState state, const CBCTauntCmd& cmd); // fn_56_14B58
  void IssueGenerateCmd(EStateMsg msg, pas::EAnimationState state,
                        const CBCGenerateCmd& cmd); // fn_56_14C24
  void IssueMeleeAttackCmd(EStateMsg msg, pas::EAnimationState state,
                           const CBCMeleeAttackCmd& cmd); // fn_56_14D40

  // WP2: eyes, script messages, flinches.
  void UpdateEyes(CStateManager& mgr, float dt);                 // fn_56_441C
  void UpdateEyeVulnerability(CStateManager& mgr, float dt);     // fn_56_4AD8
  void CreateEyes(CStateManager& mgr);                           // fn_56_5060
  bool AnyEyeDead(CStateManager& mgr) const;                     // fn_56_36A0
  bool HitIsOnHeadEye(CStateManager& mgr, TUniqueId collisionActor) const; // fn_56_566C
  bool HitIsOnTailEye(CStateManager& mgr, TUniqueId collisionActor) const; // fn_56_56D4
  void PickPASAnim(CStateManager& mgr, int state);               // fn_56_7138
  void SetEyeCollisionVulnerability(CStateManager& mgr, int eye, bool vulnerable); // fn_56_CAF0
  CSandwormEye* GetTailEye(CStateManager& mgr);                  // fn_56_E134: x14C6
  CSandwormEye* GetHeadEye(CStateManager& mgr);                  // fn_56_E168: x14C4
  const CSandwormEye* GetTailEye(const CStateManager& mgr) const; // fn_56_E19C
  const CSandwormEye* GetHeadEye(const CStateManager& mgr) const; // fn_56_E1D0
  bool ShouldRenderHeadEye(const CStateManager& mgr) const;      // fn_56_E204
  bool ShouldRenderTailEye(const CStateManager& mgr) const;      // fn_56_E2C0
  bool IsEyeVisible(const CStateManager& mgr) const;             // fn_56_E340
  void AcceptScriptMsgHelper(CStateManager& mgr);                // fn_56_11330
  float AngleBetween(const CTransform4f& xf) const;              // fn_56_11CF8
  void RemoveAdditiveFlinch(CStateManager& mgr, int eye);        // fn_56_11DF8
  void AddAdditiveFlinch(CStateManager& mgr, int eye);           // fn_56_11E54

  // WP3: movement, path following, sounds, underground.
  void DescendHelper();                                          // fn_56_2718
  bool PathReachable(CStateManager& mgr, const CVector3f& pos) const; // fn_56_3D64
  void UpdateLoopedSfx(CStateManager& mgr);                      // fn_56_3EAC
  void UpdateEmitter(CSfxHandle& handle);                        // fn_56_3EF4
  void StopMoveSfx();                                            // fn_56_403C
  void StartMoveSfx(CStateManager& mgr);                         // fn_56_4088
  void StartVocalSfx(CStateManager& mgr, float chance);          // fn_56_40EC
  void StopVocalSfx(CStateManager& mgr);                         // fn_56_41BC
  int CountScriptActors(CStateManager& mgr) const;               // fn_56_534C (unreferenced)
  float SulkHelper() const;                                      // fn_56_55F8
  CVector3f PursueHelper(const CVector3f& move) const;           // fn_56_6D74
  void UpdatePursuePathSpeed(CStateManager& mgr, float dt);      // fn_56_73F4
  CVector3f FindCoverPoint(CStateManager& mgr);                  // fn_56_7A2C
  TUniqueId ScanCoverPoints(CStateManager& mgr);                 // fn_56_7C60
  bool TestCoverPoint(const CActor* point) const;                // fn_56_7E08
  void SetPathDestination(CStateManager& mgr, const CVector3f& dest, float speed); // fn_56_7E60
  void FollowPath(CStateManager& mgr, float dt);                 // fn_56_A708
  bool NearlyOnPath(const CVector3f& pos, float radius) const;   // fn_56_A8E0
  bool PathHelper(CStateManager& mgr, TUniqueId id);             // fn_56_AC80
  CTransform4f MoveToScriptActor(CStateManager& mgr, int& index) const; // fn_56_C468
  void SetSteeringSpeedRange();                                  // fn_56_10A68 (unreferenced)

  // WP4: attacks, grab, charge, spit, anim events.
  void BeginAttackHelper(int type);                              // fn_56_2770
  void BeginAttackHelper2(int type);                             // fn_56_27E8
  bool FindGrabAIHint(CStateManager& mgr) const;                 // fn_56_2CEC
  bool AIHintHelper(CStateManager& mgr) const;                   // fn_56_3048
  int PickAttackType(CStateManager& mgr);                        // fn_56_3844
  void StartMeleeSfx(CStateManager& mgr);                        // fn_56_3FB4
  void KnockPlayer(CStateManager& mgr, const CVector3f& impulse); // fn_56_57B0
  void ResetAttackTimer(CStateManager& mgr);                     // fn_56_5CA4
  void ApplyMeleeDamage(CStateManager& mgr);                     // fn_56_5F5C
  void ReleaseGrabbedPlayer(CStateManager& mgr);                 // fn_56_6144
  void GrabPlayer(CStateManager& mgr);                           // fn_56_61EC
  TUniqueId Unreferenced6324(CStateManager& mgr) const;          // fn_56_6324 (unreferenced)
  void SendGrabMsgs(CStateManager& mgr);                         // fn_56_63B8
  void SendAttackMsgs(CStateManager& mgr);                       // fn_56_6410
  float AngleToTarget(const CVector3f& target, const CVector3f& dir) const; // fn_56_6AA8
  float SpitAimAngle(const CVector3f& target) const;             // fn_56_6B64
  CVector3f ChargeHelper(const CVector3f& dir) const;            // fn_56_6CC0
  void UpdateContactDamage(CStateManager& mgr, float dt);        // fn_56_6E40
  bool InMeleeRange(CStateManager& mgr) const;                   // fn_56_72F8
  void ApplyChargeDamage(CStateManager& mgr);                    // fn_56_7EE4
  void LaunchSpitProjectile(CStateManager& mgr, const CVector3f& target); // fn_56_84B8
  void FireSpit(CStateManager& mgr);                             // fn_56_8AAC
  // fn_56_85B4..fn_56_8A34: CImpactVisorEffect / token template instantiations used by FireSpit.
  void BeginAttack(int type, EStateMsg msg);                     // fn_56_B85C
  bool GrabHelper(CStateManager& mgr);                           // fn_56_C0A4
  CVector3f PlayerDirection(float scale) const;                  // fn_56_E5F8

  // WP5: spine segments, collision actors.
  float SegmentHelper5884() const;                               // fn_56_5884
  void AddAnimatedScale();                                       // fn_56_A510
  void SetUnderground(CStateManager& mgr);                       // fn_56_AC24
  void OrientBodyToPath(CStateManager& mgr);                     // fn_56_AD14
  void BodyHelperB0B0(rstl::reserved_vector< CVector3f, 180 >& vec, int index); // fn_56_B0B0
  void SpineUpdateHelper(CStateManager& mgr);                    // fn_56_B138
  void SpineHelperC600();                                        // fn_56_C600
  void SpineHelperC704();                                        // fn_56_C704
  bool SpineHelperC7F4(int index) const;                         // fn_56_C7F4
  void SpineHelperC828(CStateManager& mgr);                      // fn_56_C828
  CVector3f SpineHelperC994(CStateManager& mgr, const CVector3f& pt) const; // fn_56_C994
  void SpineHelperCBB4(CStateManager& mgr, int index);           // fn_56_CBB4
  void CreateCollisionActorManager(CStateManager& mgr);          // fn_56_CC8C
  void BuildSphereDescriptions();                                // fn_56_D09C
  void UpdateCollisionActors(CStateManager& mgr, float dt);      // fn_56_D350
  CSegId GetSegLocatorId(int index) const;                       // fn_56_E6BC
  CSegId GetSegId(int index) const;                              // fn_56_E728
  void SpineHelperE738(int index, const CQuaternion& q);         // fn_56_E738: writes x1670_.x0_
  void SpineHelperE764();                                        // fn_56_E764
  CQuaternion SpineHelperEF20(const CQuaternion& q) const;       // fn_56_EF20
  void SpineHelperF368();                                        // fn_56_F368
  void SpineHelperF384();                                        // fn_56_F384
  void SpineHelperF5B0(CStateManager& mgr, int index);           // fn_56_F5B0 (index unused)
  void SpineHelperF718(const CSegId& segId, const CVector3f& scale) const; // fn_56_F718
  void SpineHelperF77C(CStateManager& mgr);                      // fn_56_F77C
  void SpineHelperFA30();                                        // fn_56_FA30
  void SpineHelperFB24(CStateManager& mgr);                      // fn_56_FB24
  void SpineHelperFC94(CStateManager& mgr);                      // fn_56_FC94
  CVector3f SpineHelperFFBC(int index) const;                    // fn_56_FFBC
  CVector3f SpineHelper10038() const;                            // fn_56_10038
  CVector3f GetSegmentPosition(int index) const;                 // fn_56_10090
  void UnreferencedMath10110();                                  // fn_56_10110 (unreferenced)
  void UnreferencedMath10268();                                  // fn_56_10268 (unreferenced)
  CVector3f SpineHelper10510(float t) const;                     // fn_56_10510
  int SpineHelper106D4(float t) const;                           // fn_56_106D4
  float SpineHelper107E8() const;                                // fn_56_107E8
  float SpineHelper1080C(int count) const;                       // fn_56_1080C
  CVector3f SpineHelper108BC(int index) const;                   // fn_56_108BC
  CVector3f GetLocatorPosition(int index) const;                 // fn_56_1096C
  CTransform4f GetLctrTransformWrapper(const CSegId& id) const;  // fn_56_10A20

  // WP6: Bomb Guardian bombs.
  void UpdateBombDrop(CStateManager& mgr, float dt);             // fn_56_3A8C
  bool CanDropBomb(CStateManager& mgr) const;                    // fn_56_3B24
  void ResetBombDropTimer();                                     // fn_56_3B9C
  CBouncingBomb* DropBouncingBomb(CStateManager& mgr, const CVector3f& pos); // fn_56_3BAC
  void SpawnBombToss(CStateManager& mgr);                        // fn_56_8E90
  CBouncingBomb* SpawnBouncingBomb(CStateManager& mgr, float fuseTime, float touchRadius,
                                   float gravityScale, float bounceRestitution); // fn_56_9030
  void SpawnBombFountain(CStateManager& mgr);                    // fn_56_91D4
  void AttachHeldBomb(CStateManager& mgr);                       // fn_56_9368
  void ReleaseHeldBomb(CStateManager& mgr);                      // fn_56_9440
  CBouncingBomb* GetHeldBomb(CStateManager& mgr) const;          // fn_56_9590
  void DeleteHeldBomb(CStateManager& mgr);                       // fn_56_95E0
  ushort GetBombExplodeSfx() const;                              // fn_56_9640
  ushort GetBombBounceSfx() const;                               // fn_56_9648
  ushort GetInvalidSfx() const;                                  // fn_56_9650
  void SpawnHeldBomb(CStateManager& mgr);                        // fn_56_9660
  void RotateBodyForBombAttack(CStateManager& mgr, const CVector3f& target, CVector3f& pivot,
                               float rateDeg, float dt); // fn_56_EF8C (fn_56_F328 is Cross)

  // WP7: rendering, shadow, bounds, pose.
  void UpdateBoundsHelper();                                     // fn_56_43A4
  void UpdateShadow(CStateManager& mgr);                         // fn_56_9A58
  void CreateProjectedShadow();                                  // fn_56_9C10
  void AccumulateBounds(CAABox& box, const CTransform4f& xf) const; // fn_56_9C88
  CAABox ComputeBounds() const;                                  // fn_56_A04C
  void UpdateLocatorTransforms();                                // fn_56_A348
  float SpineRenderHelperD690(const CStateManager& mgr, const CVector3f& pos) const; // fn_56_D690
  void SpineRenderHelperD83C() const;                            // fn_56_D83C
  void SpineRenderHelperD898(CStateManager& mgr) const;          // fn_56_D898
  void RenderModelHelper(const CStateManager& mgr, const CModelData& model,
                         const CTransform4f& xf) const;          // fn_56_DB6C
  void SpineRenderHelperDBA4() const;                            // fn_56_DBA4
  float SpineRenderHelperDC28(CStateManager& mgr) const;         // fn_56_DC28
  float SpineRenderHelperE008(const CStateManager& mgr, const CVector3f& pos) const; // fn_56_E008
  void UpdatePose();                                             // fn_56_11078

  // Members (offsets are the retail layout; CHECK_SIZEOF is retail-only).
  int x7C0_;
  CPathFindSearch x7C4_search; // GetSearchPath
  float x8B0_activeTime;       // += dt in Think while alive
  CVector3f x8B4_;
  u8 x8C0_flags; // b7 unknown_0x06dee4c5 & 1; b6 move to the waypoint (fn_56_AA28); b5 cleared
                 // each Think; b4 Bomb Guardian; b3 residue spawned (fn_56_10F90); b2 boss
                 // params sent; b1, b0
  u8 x8C1_flags; // b7, b6
  rstl::optional_object< SLdrSandwormStruct > x8C4_phases[5]; // Bomb Guardian only
  int x964_;
  u8 x968_;
  float x96C_;
  rstl::reserved_vector< CSegId, 13 > x970_segIds; // fn_56_E6BC per spine locator
  int x984_segCount;   // cached spine locator ids (fn_56_C704)
  u8 x988_segIds[15];
  rstl::single_ptr< CCollisionActorManager > x998_collisionActors; // fn_56_CC8C, lazily in Think
  rstl::reserved_vector< CVector3f, 180 > x99C_joints; // spine path points
  rstl::reserved_vector< CVector3f, 13 > x1210_; // radar positions (fn_56_5938)
  rstl::reserved_vector< float, 13 > x12B0_;
  float x12E8_fade[13]; // per-segment fade (fn_56_10BE8)
  float x131C_[12];
  float x134C_;
  float x1350_; // decays to 0 (fn_56_5C7C)
  CTransform4f x1354_;
  float x1384_;
  CVector3f x1388_;
  CVector3f x1394_snatchStart; // fn_56_312C / fn_56_3180
  CVector3f x13A0_snatchEnd;
  float x13AC_; // time the player has been off the path mesh in Pursue
  CVector3f x13B0_;
  rstl::single_ptr< CProjectedShadow > x13BC_shadow; // fn_56_9C10
  rstl::reserved_vector< CTransform4f, 4 > x13C0_;
  rstl::optional_object< CToken > x1484_stateMachine2; // 'FSM2'
  SSandwormAttackHistory x1490_attacks;
  float x14B0_;
  float x14B4_frustrationRadius;
  float x14B8_frustrationTimer;
  float x14BC_deadTime; // += dt in Think once dead
  u8 x14C0_flags;       // b7, b6
  TUniqueId x14C4_headEye; // "Sandworm Eye Head"
  TUniqueId x14C6_tailEye; // "Sandworm Eye Tail"
  int x14C8_;
  int x14CC_;
  int x14D0_;
  u8 x14D4_;
  SSandwormCharge x14D8_charge;
  float x150C_cycle; // fn_56_10AD8
  float x1510_cycleRate;
  u8 x1514_;
  int x1518_bodyCmdFirst; // fn_56_A9F4
  int x151C_bodyCmdLast;
  u8 x1520_flags; // b7, b6
  float x1524_fadeTarget; // fn_56_10BE8
  SSandwormSpit x1528_spit;
  SSandwormToss x15AC_toss;
  CVector3f x1608_residuePos; // fn_56_11FAC
  CVector3f x1614_residuePosHighHealth;
  SSandwormMelee x1620_melee;
  float x164C_; // unknown_0xe593f1c6
  float x1650_lurkMin;
  float x1654_lurkMax;
  float x1658_;
  float x165C_;
  float x1660_;
  float x1664_;
  u8 x1668_flags; // b7
  int x166C_;
  SSandworm1670 x1670_;
  SSandwormPincers x1770_pincers;
  float x181C_; // decays at 40/s (fn_56_10CD4)
  float x1820_; // decays at 40/s
  float x1824_;
  SSandwormSfx x1828_sfx;
  TUniqueId x1848_;
  TUniqueId x184A_;
  int x184C_;
  int x1850_;
  float x1854_;
  CPathFindSearch x1858_search2;
  SSandwormBombs x1944_bombs;
  float x1980_bombDropRate;
  float x1984_bombDropTimer; // fn_56_3B9C
  float x1988_;
  CVector3f x198C_;
  TUniqueId x1998_;
  ushort x199A_;
  CVector3f x199C_;
  SSandwormBombPhase x19A8_bombPhase;
  SSandwormGlow x19D0_glow;
  CVector3f x19F0_;
  float x19FC_;
  float x1A00_;
  float x1A04_;
  float x1A08_;
  CAABox x1A0C_bounds;
  float x1A24_;
#endif // TARGET_PC
};

#ifdef TARGET_PC
class CSandwormEye : public CPhysicsActor {
#else
class CSandwormEye : public CActor {
#endif
public:
  // CEntity
  ~CSandwormEye() override;
  CEntity* TypesMatch(int typeId) const override;
#ifdef TARGET_PC
  CSandwormEye(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
               const CTransform4f& xf); // fn_56_2434
  // ~CSandwormEye: fn_56_23C4

  // CActor
  rstl::optional_object< CAABox > GetTouchBounds() const override;             // fn_56_2350
  CVector3f GetOrbitPosition(const CStateManager& mgr) const override;         // fn_56_23A8
  CVector3f GetAimPosition(const CStateManager& mgr, float dt) const override; // fn_56_238C

  void SetDead();                                        // fn_56_21F0: x2F4 b6
  bool IsDead() const;                                   // fn_56_2204
  CVector3f GetX2E8() const;                             // fn_56_2210
  void SetX2E8(const CVector3f& pos);                    // fn_56_222C
  void SetX2F4Bit7();                                    // fn_56_2248
  bool GetX2F4Bit7() const;                              // fn_56_225C
  void SetTargetable(CStateManager& mgr, bool targetable); // fn_56_22A0
  void SetTouchBounds(const CAABox& box);                // fn_56_231C

  CAABox x2D0_touchBounds; // mskNullBox
  CVector3f x2E8_pos;      // orbit and aim point
  u8 x2F4_flags;           // b7, b6 dead
  CRELFileToken x2F8_relToken; // keeps the Sandworm REL loaded
#endif // TARGET_PC
};

// Guessed names. DOL queries forward through the Sandworm REL's registered callbacks;
// they do not require the concrete enemy layout in the DOL.
int GetSandwormRadarPointCount(const CSandworm* sandworm);
CVector3f GetSandwormRadarPointPosition(const CSandworm* sandworm, int index);

#endif // _CSANDWORM
