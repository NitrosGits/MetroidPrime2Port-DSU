#ifndef _CSNAKEWEEDSWARM
#define _CSNAKEWEEDSWARM

#include "MetroidPrime/CActor.hpp"

#ifdef TARGET_PC
#include "Kyoto/Audio/CSfxHandle.hpp"
#include "Kyoto/Math/CAABox.hpp"
#include "MetroidPrime/CDamageInfo.hpp"

class CAnimRes;
#endif

// Original class name from the Wii SEL exports. The class lives in the SnakeWeedSwarm REL; the DOL
// only proves the CActor base through its TypesMatch parent, so the layout is left opaque.
class CSnakeWeedSwarm : public CActor {
public:
#ifdef TARGET_PC
  // The REL's CSnakeWeedSwarm (module 71), host body milestone 1 - see
  // docs/module-analysis/SnakeWeedSwarm.md and PortCSnakeWeedSwarm.cpp. 0x298 bytes on the GameCube
  // (the loader's rs_new size), built by fn_71_2B84. It is MP1's CSnakeWeedSwarm on Echoes' 0x158
  // CActor: the members below sit at MP1's offsets plus 0x70. Argument order is the loader's call
  // at 0x000005D0: (uid, name, entityInfo, position, scale, animRes, actorParameters, then the
  // loader's floats/damage/sounds, which the host passes as the generated property struct's
  // values).
  CSnakeWeedSwarm(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                  const CVector3f& pos, const CVector3f& scale, const CAnimRes& animRes,
                  const CActorParameters& actParms, float density, float maxDepth,
                  float locationVariance, float detectionRadius, float grabRadius,
                  float waitTimeOut, float waitTimeOutVariance, float retreatDepth,
                  float moveSpeed, float moveSpeedVariance, float maxSlope, float minSize,
                  float maxSize, float heightOffset, const CDamageInfo& contactDamage,
                  float damageWaitTime, ushort sfxLooped, ushort sfxIntoGround,
                  ushort sfxOutOfGround);

  // The REL's vtable lbl_71_data_30 names what is overridden (slot numbers count its two header
  // words, as the other module analyses do). Slots 4 (PreThink), 7 (SetActive), 8
  // (ClearFluidList), 11 (Render) and 12.. keep the DOL's base-class symbols.
  void Think(float dt, CStateManager& mgr) override;                        // slot 5, fn_71_1E58
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override; // slot 6, fn_71_274C
  void PreRender(CStateManager& mgr) override;                              // slot 9, fn_71_1934
  void AddToRenderer(const CStateManager& mgr) const override;              // slot 10, fn_71_163C
  rstl::optional_object< CAABox > GetTouchBounds() const override;          // slot 18, fn_71_2494
  void Touch(CActor& actor, CStateManager& mgr) override;                   // slot 19, fn_71_1CC0

  // The two member functions the module hands the DOL through SSnakeWeedSwarm_FuncPtrs
  // (lbl_71_bss_40 +0x4 and +0x10, the pointers-to-member lbl_71_data_18 / lbl_71_data_24).
  void ApplyRadiusDamage(CVector3f pos, const CDamageInfo& info, CStateManager& mgr); // fn_71_1AF8
  void ScareSnakeWeeds(CStateManager& mgr, const CVector3f& pos, float radius);       // fn_71_1B3C
#endif
  // CEntity
  ~CSnakeWeedSwarm() override;
  CEntity* TypesMatch(int typeId) const override;

#ifdef TARGET_PC
private:
  // Host layout; guest offsets from the constructor fn_71_2B84 and the accessors that read them.
  CVector3f mScale;             // 0x158 [proven] the loader's scale argument (r8)
  float mDensity;               // 0x164..0x198: the 14 loader floats [probable order, MP1's]
  float mMaxDepth;
  float mLocationVariance;
  float mDetectionRadius;
  float mGrabRadius;
  float mWaitTimeOut;
  float mWaitTimeOutVariance;
  float mRetreatDepth;
  float mMoveSpeed;
  float mMoveSpeedVariance;
  float mCosMaxSlope;           // 0x18c [proven] cos(maxSlope * lbl_71_rodata_50 = pi/180)
  float mMinSize;
  float mMaxSize;
  float mHeightOffset;
  bool mHasGround;              // 0x1b0 bit 0x80 [proven] GetTouchBounds' test
  CAABox mTouchBounds;          // 0x1b4 [proven] what GetTouchBounds returns
  CDamageInfo mContactDamage;   // 0x1cc [proven] 0x1c-byte block the ctor fills
  float mDamageWaitTime;        // [?] loader float after the damage
  ushort mSfxLooped;            // 0x25c [proven] three CSfxManager ids
  ushort mSfxIntoGround;        // 0x25e
  ushort mSfxOutOfGround;       // 0x260
  CSfxHandle mSfxHandle;        // 0x264 [proven] removed on kSM_Delete (fn_71_274C)
  // Guest members this milestone does not hold, because nothing in it reads them: the uint at
  // 0x19c, the CBoid vector at 0x1a0, the byte at 0x1b1, the position/normal workspaces at
  // 0x1e8/0x20c, the four CModelData slots at 0x22c (built from the animRes), the which-model int at
  // 0x250, the boid-position and boid-placement vectors at 0x254/0x258, the two optional
  // CGenDescription tokens at 0x268/0x270 and CElementGens at 0x278/0x280 (the loader passes
  // invalid ids and a zero count for them), and the ints/floats at 0x288..0x290.
#endif
};

#endif // _CSNAKEWEEDSWARM
