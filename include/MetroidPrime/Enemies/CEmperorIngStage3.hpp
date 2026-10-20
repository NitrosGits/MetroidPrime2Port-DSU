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

private:
  void EnsureBodyActive(CStateManager& mgr);

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
  // fn_18_8E50: the Listen flag byte at guest 0x824; its bit 30 (the mask 0x02) is set when the
  // actor hears the player fire.
  uchar mNoiseFlags;                     // guest 0x824, 0
};
#endif

#endif
