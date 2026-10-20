#ifndef _CDIGITALGUARDIAN
#define _CDIGITALGUARDIAN

#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "REL/REL_Setup.h"

#include "Kyoto/Audio/CSfxHandle.hpp"
#include "rstl/optional_object.hpp"

class CCollisionActorManager;
class CElementGen;
class CParticleGen;

#ifdef TARGET_PC
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/ScriptLoader/Structs/SLdrAudioPlaybackParms.hpp"
#include "MetroidPrime/ScriptLoader/Structs/SLdrDamageInfo.hpp"
#include "MetroidPrime/ScriptLoader/Structs/SLdrDamageVulnerability.hpp"
#include "MetroidPrime/ScriptLoader/Structs/SLdrEchoParameters.hpp"
#include "MetroidPrime/ScriptLoader/Structs/SLdrShockWaveInfo.hpp"

// Host copy of the module's parameter block (SLdrDigitalGuardianData). Retail reads it into a
// stack struct and copies it into the object verbatim (fn_14_C68C, guest 0x7C0, 0x640 bytes), so
// the host keeps the loader's own types: one member per row of
// docs/module-analysis/DigitalGuardian.md section c.2, named after the SLdr property it holds.
// Nothing in this milestone reads the values - the class keeps them for the state machine the
// later milestones add - so the fields are only a faithful copy of what the loader read.
class CDigitalGuardianData {
public:
  CDigitalGuardianData()
  : mScannableInfoCrippled(kInvalidAssetId)
  , mNearLegStabRange(0.f)
  , mFarLegStabRange(0.f)
  , mMinLegStabAttackTime(0.f)
  , mLegStabDamage()
  , mToeTargetHP(0.f)
  , mToeTargetModel(kInvalidAssetId)
  , mToeTargetExplosion(kInvalidAssetId)
  , mSoundToeTarget()
  , mSoundToeTargetAttack()
  , mSoundToeTargetExplosion()
  , mSoundToeTargetHit()
  , mSoundShockWave()
  , mShockWaveInfo()
  , mVortexAttackDuration(0.f)
  , mVortexAttractionForce(0.f)
  , mMaxVortexAttractionDistance(0.f)
  , mVortexLinearVelocity(0.f)
  , mVortexLinearAcceleration(0.f)
  , mVortexDamage()
  , mUnknownFB5263E8(0)
  , mUnknown6AAF33E3(0)
  , mUnknown4F5D725C(0.f)
  , mSoundVortexFlash()
  , mLegModel(kInvalidAssetId)
  , mShinArmor(kInvalidAssetId)
  , mKneeArmorHP(0.f)
  , mSoundKneeArmorHit()
  , mSoundKneeVulnerable()
  , mKneeArmor(kInvalidAssetId)
  , mKneeEchoParameters()
  , mUnknownA324E26C(0.f)
  , mUnknown6A754EBD(0.f)
  , mJumpTimer(0.f)
  , mUnknown8106CDA9(0.f)
  , mUnknown9E1B8105(0.f)
  , mUnknownA08FCC70(0.f)
  , mUnknown3254A16B(0.f)
  , mTransmissionBeacon(kInvalidAssetId)
  , mTransmissionBeaconFx(kInvalidAssetId)
  , mTransmissionBeaconEchoParameters()
  , mSoundTransmissionBeacon()
  , mSoundBeaconExtract()
  , mSoundBeaconRetract()
  , mTransmissionBeaconHP(0.f)
  , mTransmissionBeaconExplosion(kInvalidAssetId)
  , mTransmissionBeaconEchoHit(kInvalidAssetId)
  , mSoundBeaconExplode()
  , mSoundBeaconHit()
  , mKneeVulnerability()
  , mVortexVulnerability()
  , mToeTargetVulnerability() {}

  CAssetId mScannableInfoCrippled;       // 0x00 scannableInfoCrippled
  float mNearLegStabRange;               // 0x04 nearLegStabRange
  float mFarLegStabRange;                // 0x08 farLegStabRange
  float mMinLegStabAttackTime;           // 0x0C minLegStabAttackTime
  SLdrDamageInfo mLegStabDamage;         // 0x10 legStabDamage
  float mToeTargetHP;                    // 0x20 toeTargetHP
  CAssetId mToeTargetModel;              // 0x24 toeTargetModel
  CAssetId mToeTargetExplosion;          // 0x28 toeTargetExplosion
  SLdrAudioPlaybackParms mSoundToeTarget;         // 0x2C sound_ToeTarget
  SLdrAudioPlaybackParms mSoundToeTargetAttack;   // 0x44 sound_ToeTargetAttack
  SLdrAudioPlaybackParms mSoundToeTargetExplosion; // 0x5C sound_ToeTargetExplosion
  SLdrAudioPlaybackParms mSoundToeTargetHit;      // 0x74 sound_ToeTargetHit
  SLdrAudioPlaybackParms mSoundShockWave;         // 0x8C sound_ShockWave
  SLdrShockWaveInfo mShockWaveInfo;               // 0xA4 shockWaveInfo
  float mVortexAttackDuration;           // 0xD4 vortexAttackDuration
  float mVortexAttractionForce;          // 0xD8 vortexAttractionForce
  float mMaxVortexAttractionDistance;    // 0xDC maxVortexAttractionDistance
  float mVortexLinearVelocity;           // 0xE0 vortexLinearVelocity
  float mVortexLinearAcceleration;       // 0xE4 vortexLinearAcceleration
  SLdrDamageInfo mVortexDamage;          // 0xE8 vortexDamage
  int mUnknownFB5263E8;                  // 0xF8 unknown_0xfb5263e8
  int mUnknown6AAF33E3;                  // 0xFC unknown_0x6aaf33e3
  float mUnknown4F5D725C;                // 0x100 unknown_0x4f5d725c
  SLdrAudioPlaybackParms mSoundVortexFlash;      // 0x104 sound_VortexFlash
  CAssetId mLegModel;                    // 0x11C legModel
  CAssetId mShinArmor;                   // 0x120 shinArmor
  float mKneeArmorHP;                    // 0x124 kneeArmorHP
  SLdrAudioPlaybackParms mSoundKneeArmorHit;     // 0x128 sound_KneeArmorHit
  SLdrAudioPlaybackParms mSoundKneeVulnerable;   // 0x140 sound_KneeVulnerable
  CAssetId mKneeArmor;                   // 0x158 kneeArmor
  SLdrEchoParameters mKneeEchoParameters;         // 0x15C kneeEchoParameters
  float mUnknownA324E26C;                // 0x170 unknown_0xa324e26c
  float mUnknown6A754EBD;                // 0x174 unknown_0x6a754ebd
  float mJumpTimer;                      // 0x178 jumpTimer
  float mUnknown8106CDA9;                // 0x17C unknown_0x8106cda9
  float mUnknown9E1B8105;                // 0x180 unknown_0x9e1b8105
  float mUnknownA08FCC70;                // 0x184 unknown_0xa08fcc70
  float mUnknown3254A16B;                // 0x188 unknown_0x3254a16b
  CAssetId mTransmissionBeacon;          // 0x18C transmissionBeacon
  CAssetId mTransmissionBeaconFx;        // 0x190 transmissionBeaconFx
  SLdrEchoParameters mTransmissionBeaconEchoParameters; // 0x194 transmissionBeaconEchoParameters
  SLdrAudioPlaybackParms mSoundTransmissionBeacon;  // 0x1A8 sound_TransmissionBeacon
  SLdrAudioPlaybackParms mSoundBeaconExtract;       // 0x1C0 sound_BeaconExtract
  SLdrAudioPlaybackParms mSoundBeaconRetract;       // 0x1D8 sound_BeaconRetract
  float mTransmissionBeaconHP;           // 0x1F0 transmissionBeaconHP
  CAssetId mTransmissionBeaconExplosion; // 0x1F4 transmissionBeaconExplosion
  CAssetId mTransmissionBeaconEchoHit;   // 0x1F8 transmissionBeaconEchoHit
  SLdrAudioPlaybackParms mSoundBeaconExplode;       // 0x1FC sound_BeaconExplode
  SLdrAudioPlaybackParms mSoundBeaconHit;           // 0x214 sound_BeaconHit
  SLdrDamageVulnerability mKneeVulnerability;    // 0x22C kneeVulnerability
  SLdrDamageVulnerability mVortexVulnerability;  // 0x388 vortexVulnerability
  SLdrDamageVulnerability mToeTargetVulnerability; // 0x4E4 toeTargetVulnerability
};
#endif

// The REL's CDigitalGuardian (module 14): the Sanctuary Fortress Guardian boss body, script type
// DGRD ("Mechanism: Quadraxis"). 0x1290 bytes on the GameCube (the loader's __nw__ at
// 0x000003F0), built by fn_14_B100; its vtable lbl_14_data_484 (82 words, 33 local overrides)
// and the analysis are docs/module-analysis/DigitalGuardian.md.
//
// Milestone 1 (this file's first slice) is load-and-think: the loader, the constructor and the
// vtable's accessors, so the actor registers and thinks. The state machine (fn_14_9DFC installs
// the 12 triggers, 14 states and 13 codes), the vortex, the knee armour, the transmission
// beacon, the toe targets and the head module (CDigitalGuardianHead, guest 0x10F8) are later
// milestones, and every override that belongs to them forwards to CPatterned behind a once-only
// StandIn log.
class REL_EXPORT CDigitalGuardian : public CPatterned {
public:
#ifdef TARGET_PC
  // Argument order is the port's own (the REL loader's call at 0x00000468 is
  // (uid, editor properties, entity info, transform, model, actor parameters, patterned info)
  // plus the parameter block on the stack).
  CDigitalGuardian(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                   const CTransform4f& xf, const CModelData& modelData,
                   const CActorParameters& actParms, const CPatternedInfo& pInfo,
                   const CDigitalGuardianData& data);
  ~CDigitalGuardian() override;

  // CEntity / CActor
  void PreThink(float dt, CStateManager& mgr) override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  void PreRenderAllViewports(CStateManager& mgr) override;
  const CHealthInfo* GetHealthInfo() const override;
  const CDamageVulnerability* GetDamageVulnerability() const override;
  rstl::optional_object< CAABox > GetTouchBounds() const override;
  CVector3f GetAimPosition(const CStateManager& mgr, float dt) const override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;
  CScannableObjectInfo* GetScannableObjectInfo() const override;

  // CAi
  void TakeDamage(const CVector3f& direction, float magnitude) override;
  bool CanBeShot(const CStateManager&, int) override { return true; }
  bool IsListening() const override { return true; }
  bool Listen(CStateManager&, const CVector3f&, EListenNoiseType) override { return false; }
  CVector3f GetOrigin(const CStateManager&, const CTeamAiRole&, const CVector3f&) const override;

  // CPatterned. The accessors below are the REL's own one-line bodies (the module's .text starts
  // with them), so they are real, not stand-ins.
  void RenderSystemsToBeDrawnLast(const CStateManager& mgr, uint mask, uint target) const override;
  uchar GetModelAlphau8(const CStateManager&) const override { return mColor.GetAlphau8(); }
  CProjectileInfo* ProjectileInfo() override { return nullptr; }
  using CPatterned::GetSearchPath;
  CPathFindSearch* GetSearchPath() override { return nullptr; }
  using CPatterned::GetPointSearchPath;
  CPathFindPointSearch* GetPointSearchPath() override { return nullptr; }
  void SetupStateMachine(CStateManager& mgr) override;
  CRagDoll* GetRagDoll() const override { return nullptr; }
  TUniqueId GetAttackTarget() const override { return kInvalidUniqueId; }
  bool IsOnGround() const override { return CPatterned::IsOnGround(); }
  float GetGravityConstant() const override { return kDefaultGravityAccel; }
  bool IsScanVisorSelfRender() const override { return true; }
  CAABox GetScanVisorRenderBounds(const CStateManager&) const override;
  void ScanVisorRender(const CStateManager&, const CTransform4f&, const CModelFlags&) const override;
  const rstl::optional_object< TCachedToken< CGenDescription > >&
  GetDeathExplosionParticle() const override;

  // The body's state machine. Retail fn_14_9DFC (vtable slot 60) calls the base's setup and
  // then registers the three tables fn_14_D904 builds: 12 triggers (lbl_14_data_9C), 14 states
  // (lbl_14_data_204) and 13 codes (lbl_14_data_380). The names are retail's - the state
  // machine asset's nodes are matched by name - and each comment is the retail function the
  // entry points at. The bodies are stand-ins in this slice; a later one ports them.
  //
  // Triggers (lbl_14_data_9C, 12 entries).
  bool StateOver(CStateManager& mgr, const CTriggerData& data) const;         // fn_14_9AA8
  bool IsIdle(CStateManager& mgr, const CTriggerData& data) const;             // fn_14_9A9C
  bool HasFaceTarget(CStateManager& mgr, const CTriggerData& data) const;      // fn_14_9A60
  bool KneeDestroyed(CStateManager& mgr, const CTriggerData& data) const;      // fn_14_9A48
  bool AllKneesDestroyed(CStateManager& mgr, const CTriggerData& data) const;  // fn_14_9A0C
  bool ShouldProtectKnees(CStateManager& mgr, const CTriggerData& data) const; // fn_14_9994
  bool ShouldVortexAttack(CStateManager& mgr, const CTriggerData& data) const; // fn_14_9930
  bool ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const;  // fn_14_97F0
  bool ShouldProjectileAttack(CStateManager& mgr,
                              const CTriggerData& data) const;                 // fn_14_97A4
  bool ShouldJumpAttack(CStateManager& mgr, const CTriggerData& data) const;  // fn_14_9678
  bool VortexStruckPlayer(CStateManager& mgr, const CTriggerData& data) const; // fn_14_966C
  bool BreakOutOfVortex(CStateManager& mgr, const CTriggerData& data) const;  // fn_14_9638

  // States (lbl_14_data_204, 14 entries). Start hides CPatterned::Start, which is what retail
  // does too: the module's table replaces the base's "Start" entry.
  void Start(CStateManager& mgr, EStateMsg msg, float dt);                     // fn_14_9634
  void Idle(CStateManager& mgr, EStateMsg msg, float dt);                      // fn_14_95B8
  void FaceTarget(CStateManager& mgr, EStateMsg msg, float dt);                // fn_14_94BC
  void VortexAttack(CStateManager& mgr, EStateMsg msg, float dt);              // fn_14_9260
  void VortexEndAttack(CStateManager& mgr, EStateMsg msg, float dt);           // fn_14_9164
  void VortexRetreat(CStateManager& mgr, EStateMsg msg, float dt);             // fn_14_9070
  void ExitVortexAttack(CStateManager& mgr, EStateMsg msg, float dt);          // fn_14_8E44
  void MeleeAttackTelegraph(CStateManager& mgr, EStateMsg msg, float dt);      // fn_14_8CCC
  void MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt);               // fn_14_89D4
  void ProjectileAttack(CStateManager& mgr, EStateMsg msg, float dt);          // fn_14_8794
  void JumpAttack(CStateManager& mgr, EStateMsg msg, float dt);                // fn_14_8580
  void KneeDestroyedReaction(CStateManager& mgr, EStateMsg msg, float dt);     // fn_14_8090
  void CrippledReaction(CStateManager& mgr, EStateMsg msg, float dt);          // fn_14_7D8C
  void Crippled(CStateManager& mgr, EStateMsg msg, float dt);                  // fn_14_7CB4

  // Codes (lbl_14_data_380, 13 entries).
  void SetupBossEnergyBar(CStateManager& mgr, float dt);                       // fn_14_7BD4
  void ResetProtectKneesTime(CStateManager& mgr, float dt);                    // fn_14_7B90
  void ResetJumpTime(CStateManager& mgr, float dt);                            // fn_14_7B84
  void ResetAttackTimes(CStateManager& mgr, float dt);                         // fn_14_7B24
  void SetProtectKneesFaceTarget(CStateManager& mgr, float dt);               // fn_14_77EC
  void SetVortexAttackMode(CStateManager& mgr, float dt);                      // fn_14_775C
  void SetNormalAttackMode(CStateManager& mgr, float dt);                      // fn_14_76D0
  void DisableHeadAttacks(CStateManager& mgr, float dt);                       // fn_14_768C
  void ResetHeadAttacks(CStateManager& mgr, float dt);                         // fn_14_7648
  void SelectExitVortexTarget(CStateManager& mgr, float dt);                   // fn_14_7600
  void SelectBestMeleeAttackLeg(CStateManager& mgr, float dt);                // fn_14_7504
  void SelectMeleeAttackRange(CStateManager& mgr, float dt);                  // fn_14_73D4
  void SetJumpFaceTarget(CStateManager& mgr, float dt);                        // fn_14_7208
#endif
  // CEntity
  CEntity* TypesMatch(int typeId) const override;

private:
#ifdef TARGET_PC
  void EnsureBodyActive(CStateManager& mgr);

  // The guest members the body's state machine reads (docs/module-analysis/DigitalGuardian.md
  // section e). The host class has its own layout, so the guest offsets are in the names and
  // the values are the ones the retail constructor (fn_14_B100) stores.
  u16 xFB0_;        // 0xFB0: a knee id, kInvalidUniqueId (fn_14_705C counts these)
  u16 xFB2_;        // 0xFB2: a knee id, kInvalidUniqueId (fn_14_705C counts these)
  u16 xFB4_;        // 0xFB4: a knee id, kInvalidUniqueId (fn_14_705C counts these)
  u16 xFB6_;        // 0xFB6: a knee id, kInvalidUniqueId (fn_14_705C counts these)
  int x10C4_;       // 0x10C4: how many locator ids follow at 0x10C8 (4 in the ctor)
  CSegId x10C8_[4]; // 0x10C8: the four locator segment ids, all invalid (0xFF) in the ctor
  u16 x1080_;       // 0x1080: the object id fn_14_3974 looks the object up with
  u16 x1082_;       // 0x1082: the head module's uid (fn_14_9994 looks the head up with it)
  uchar x1010_;     // 0x1010: the flag byte the "IM01"/"IM02" messages set (fn_14_A6A8)
  uchar x1011_;     // 0x1011: the flag byte the "IM07"/"IM08" messages set (fn_14_A6A8)
  uchar x1012_;     // 0x1012: the flag byte the "IM03"/"IM04" messages set (fn_14_A6A8)
  uchar x1013_;     // 0x1013: the flag byte the "IM05"/"IM06" messages set (fn_14_A6A8)
  TUniqueId x1168_; // 0x1168: the exit vortex target's waypoint id (fn_14_7600)
  int x11B8_;       // 0x11B8: s32, 0 (fn_14_9638 compares it with 1)
  float x11E0_;     // 0x11E0: f32, 0.0f (fn_14_97F0 compares it with 0.0f)
  float x11E4_;     // 0x11E4: f32, 0.0f (fn_14_9678 compares it with 0.0f)
  float x1274_;     // 0x1274: f32, 0 (fn_14_9994 compares it with 0.0f)
  CVector3f x1278_; // 0x1278: the face target's position, zero (fn_14_9A60 reads x, y and z)
  int x1284_;       // 0x1284: s32, -1 (fn_14_9A48 and fn_14_9A0C read it)
  uchar x128C_;     // 0x128C: the flag byte; bit 7 = "is idle" (fn_14_9A9C reads bit 7)

  // The five models the module's AddToRenderer (fn_14_A004) draws, and the state they read.
  // The guest offsets are the REL's (docs/module-analysis/DigitalGuardian.md section e); the
  // host class has its own layout. Each one is an optional: the guest's CModelData is 0x4C
  // bytes and the byte after it is the optional's valid flag, which the draws test.
  CCollisionActorManager* xE60_; // 0xE60: the first collision actor manager (fn_14_61DC builds it)
  CCollisionActorManager* xE64_; // 0xE64: the second collision actor manager (fn_14_5C30 builds it)
  // 0xE6C (0x4c, valid flag 0xEB8): the leg model, built by fn_14_6864 and drawn by
  // fn_14_1994 at the four knee locators.
  rstl::optional_object< CModelData > xE6C_;
  // 0xEBC (0x4c, valid flag 0xF08): the shin model, built by fn_14_6864 and drawn by
  // fn_14_18B4 at the four knee locators.
  rstl::optional_object< CModelData > xEBC_;
  // 0xF0C (0x4c, valid flag 0xF58): the knee model, built by fn_14_6864.
  rstl::optional_object< CModelData > xF0C_;
  // 0xF5C (0x4c, valid flag 0xFA8): the toe target's model, built by fn_14_6864 and drawn by
  // fn_14_1700 at the four ankle locators.
  rstl::optional_object< CModelData > xF5C_;
  // 0x11EC (0x4c, valid flag 0x1238): the transmission beacon's model, built by fn_14_6864 and
  // drawn by fn_14_17DC at the neck locator.
  rstl::optional_object< CModelData > x11EC_;
  uchar x10B9_;      // 0x10B9: the neck locator segment id (fn_14_2578)
  uchar x10C0_;      // 0x10C0: the first leg locator segment id (fn_14_1994, fn_14_18B4)
  uchar x10C1_;      // 0x10C1: the second leg locator segment id
  uchar x10C2_;      // 0x10C2: the third leg locator segment id
  uchar x10C3_;      // 0x10C3: the fourth leg locator segment id
  uchar x10D4_;      // 0x10D4: the first toe locator segment id (fn_14_1700)
  uchar x10D5_;      // 0x10D5: the second toe locator segment id
  uchar x10D6_;      // 0x10D6: the third toe locator segment id
  uchar x10D7_;      // 0x10D7: the fourth toe locator segment id
  int x10DC_[4];     // 0x10DC: the four per-leg words (0x10DC, 0x10E0, 0x10E4, 0x10E8); the
                     // value 3 skips the leg (fn_14_1700)
  CParticleGen* x123C_;  // 0x123C: the particle generator the echo emitter's render draws (a
                         // CElementGen in retail); fn_14_D664 creates it
  TUniqueId x10CC_;      // 0x10CC: the guard's collision actor (fn_14_28D0 wakes it)
  CSfxHandle x1240_;   // 0x1240: the beacon's sound emitter handle (fn_14_28D0 sets it)
  float x1264_;        // 0x1264: the beacon's height (fn_14_17DC, fn_14_2578, fn_14_2CC4)
  int x1268_;          // 0x1268: the beacon's state word (fn_14_2CC4)
  uint x1270_;       // 0x1270: the packed colour the echo emitter's model is drawn with

  // The four model renders fn_14_A004 calls (the calls at 0xA0A0, 0xA0B4, 0xA0E8 and 0xA0FC),
  // named by their retail offset.
  void x1994_(const CStateManager& mgr, const CTransform4f& xf, const CModelFlags& flags) const;
  void x18B4_(const CStateManager& mgr, const CTransform4f& xf, const CModelFlags& flags) const;
  void x17DC_(const CStateManager& mgr, const CTransform4f& xf) const;
  void x1700_(const CStateManager& mgr, const CTransform4f& xf, const CModelFlags& flags) const;

  // fn_14_2578 (0xF4 bytes): the echo emitter's transform.
  void x2578_(CTransform4f& out, const CTransform4f& xf) const;

  // fn_14_D664 (0x178 bytes): the beacon's particle generator.
  CElementGen* xD664_(CAssetId id);

  // fn_14_2CC4 (0x19C bytes): the beacon's update.
  void x2CC4_(CStateManager& mgr, float dt);

  // fn_14_3E88 (0x88 bytes): the knee colliders' active flag.
  void x3E88_(CStateManager& mgr, bool active);

  // fn_14_3974 (0x58 bytes): the object with the id at guest 0x1080 gets its active flag set.
  void x3974_(CStateManager& mgr, bool active);

  // fn_14_676C (0xF8 bytes): the collision actor managers' builder, and the three builders it
  // calls - fn_14_61DC (0x590 bytes), fn_14_5C30 (0x2F4 bytes) and fn_14_59BC (0x274 bytes) -
  // which are over the callee limit and stay stand-ins.
  void x676C_(CStateManager& mgr);
  void x61DC_(CStateManager& mgr);
  void x5C30_(CStateManager& mgr);
  void x59BC_(CStateManager& mgr);

  // fn_14_28D0 (0x158 bytes) and fn_14_2A28 (0x29C bytes): the beacon's arrival and its
  // effect update. Both are over this slice's callee limit, so they are stand-ins.
  void x28D0_(CStateManager& mgr);
  void x2A28_(CStateManager& mgr, float dt);

  // fn_14_6864 (0x1BC bytes): the constructor's tail builds the module's five models.
  void x6864_();

  // The retail's per-segment rotation: the character's rest pose rotation for that segment.
  CTransform4f SegmentRotation_(const CSegId& seg) const;

  // fn_14_308C: the id of the connected waypoint that is active and closest to the given
  // position (the helper fn_14_9678 measures against).
  TUniqueId x308C_(CStateManager& mgr, const CVector3f& position) const;

  // fn_14_705C: how many of the four knee ids (guest this+0xFB0, 0xFB2, 0xFB4 and 0xFB6) are
  // not kInvalidUniqueId.
  int x705C_() const;

  // Host layout (retail's object is 0x1290, CPatterned is 0x7c0). The guest offsets are the
  // REL's and are in docs/module-analysis/DigitalGuardian.md section e; the host class has its
  // own layout and only keeps what this milestone needs.
  CDigitalGuardianData mData; // guest 0x7C0 (0x640), the block fn_14_C68C copies in
#else
  uchar x7c0_[0x1290 - 0x7c0]; // CDigitalGuardianData and everything above it in retail
#endif
};

#endif
