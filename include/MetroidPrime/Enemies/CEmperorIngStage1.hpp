#ifndef _CEMPERORINGSTAGE1
#define _CEMPERORINGSTAGE1

#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "REL/REL_Setup.h"

#ifdef TARGET_PC
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/ScriptLoader/SLdrEmperorIngStage1.hpp"
#endif

// The REL's CEmperorIngStage1 (module 16): the Emperor Ing's first stage, the boss the player
// fights in the Great Temple. 0xD18 bytes on the GameCube (the loader's rs_new at 0x0000A4B8),
// built by fn_16_900C, its vtable lbl_16_data_3C8 (80 entries, 22 of them local) and the analysis
// is docs/module-analysis/EmperorIngStage1.md.
//
// Milestone 1 (this file's first slice) is load-and-stand: the loader, the constructor and the
// accessors the vtable gives one-line bodies to. The state machine (fn_16_8AF4 installs the 6
// triggers, 12 states and 3 code functions listed in the analysis), the tentacles, the attacks
// (melee, projectile, warp, beam, taunt) and the heart stage are milestone 2 and forward to
// CPatterned behind a once-only StandIn log.
//
// The host layout is its own: the retail object is 0xD18 bytes, CPatterned is 0x7C0, and the
// guest offsets of every member the constructor touches are in the comment above each one. Host
// objects outgrow retail, so the class is allocated with sizeof, not with 0xD18.
#ifdef TARGET_PC
class REL_EXPORT CEmperorIngStage1 : public CPatterned {
public:
  // Argument order is the REL loader's call at 0x0000A51C (fn_16_A2A0 -> fn_16_900C): uid,
  // editor-property name, entity info, transform, model, actor parameters, patterned info, then
  // the script's data block (the REL passes it as the constructor's ninth argument, the stack
  // slot 0x8 at 0x0000A51C).
  CEmperorIngStage1(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                    const CTransform4f& xf, const CModelData& modelData,
                    const CActorParameters& actParms, const CPatternedInfo& pInfo,
                    const SLdrEmperorIngStage1Data& data);
  ~CEmperorIngStage1() override;

  // CEntity
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void Render(const CStateManager& mgr) const override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;
  void ProcessSoundEvent(int sfxId, float weight, int flags, float fallOff, float maxDist,
                         const CSegId& locator, ushort pitchStart, ushort pitchEnd,
                         float pitchDuration, uchar minVol, uchar maxVol, float distanceSquared,
                         const CVector3f& position, int aid, CStateManager& mgr,
                         bool translateId) override;

  // CAi
  bool CanBeShot(const CStateManager&, int) override;
  bool IsListening() const override;

  // CPatterned
  void SetupStateMachine(CStateManager& mgr) override;

private:
  void EnsureBodyActive(CStateManager& mgr);

  // Host layout. The guest offsets are the REL's (fn_16_900C); the full member map, including
  // the parts this milestone does not build, is in docs/module-analysis/EmperorIngStage1.md.
  CHealthInfo mHealthInfo;                        // guest 0xAAC, fn_16_9AB0
  CDamageVulnerability mNormalVulnerability;      // guest 0xACC, fn_16_98D0
  CDamageVulnerability mWarpVulnerability;        // guest 0xAFC, fn_16_98D0
  CDamageVulnerability mMeleeVulnerability;       // guest 0xB2C, fn_16_98D0
  CDamageVulnerability mProjectileVulnerability;  // guest 0xB5C, fn_16_98D0
  CDamageInfo mMeleeDamage;                      // guest 0xB8C
  CDamageInfo mBeamDamage;                       // guest 0xBC4
  CDamageInfo mWarpStabDamage;                   // guest 0xC10
  float mHeartExposedTime;                       // guest 0xBA8
  float mStayRetractedTime;                      // guest 0xBB4
  float mMaxTurnSpeedNormal;                     // guest 0xBF0
  float mMaxTurnSpeedMelee;                      // guest 0xBF4
  float mTurnSpeedAccel;                         // guest 0xBFC
  float mTauntFrequency;                         // guest 0xC34
  float mAttackIntervalMin;                      // guest 0xCF4
  float mAttackIntervalMax;                      // guest 0xCF8
};
#endif

#endif
