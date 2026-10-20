#ifndef _CSCRIPTPLAYERTURRET
#define _CSCRIPTPLAYERTURRET

#include "MetroidPrime/CActor.hpp"
#ifdef TARGET_PC
#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/Math/CAABox.hpp"
#include "Kyoto/SObjectTag.hpp"
#include "MetroidPrime/CDamageInfo.hpp"

class CFinalInput;
#endif

// Partial layout of the REL player turret object, covering only the fields the DOL reads
// directly. Guessed class and member names; the gaps are unidentified.
class CScriptPlayerTurret : public CActor {
public:
  // CEntity
  ~CScriptPlayerTurret() override;
  CEntity* TypesMatch(int typeId) const override;

  float GetMaxAimAngle() const { return mMaxAimAngle; }
  void SetTargetPosition(const CVector3f& position) { mTargetPosition = position; }

#ifdef TARGET_PC
  // The REL's CScriptPlayerTurret (module 63): the shooting gallery's player turret. The guest
  // object is 0x230 bytes (the loader's rs_new at 0x00000494) and CActor is 0x158 there, so the
  // turret's own fields run 0x158..0x22F; every offset below is the guest one, read from the
  // constructor fn_63_30E8. The analysis is docs/module-analysis/ScriptPlayerTurret.md.
  //
  // Milestone 1 is load-and-think: the loader, the constructor, the vtable's GetTouchBounds and
  // the five member functions the module hands the DOL through SPlayerTurret_FuncPtrs. Think
  // (fn_63_23C0), AcceptScriptMsg (fn_63_1BE8) and PreRender (fn_63_2D68) are too big for this
  // slice and forward to CActor behind a once-only stand-in log.
  CScriptPlayerTurret(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                      const CTransform4f& xf, const CActorParameters& actParms, uint flags,
                      float maxHorizRotationLeft, float maxHorizRotationRight,
                      float maxVertElevationUp, float maxVertElevationDown, float damageAngle,
                      float horizSpeed, float vertSpeed, float fireRate,
                      const CDamageInfo& weaponDamage, CAssetId weaponEffect,
                      CAssetId weaponEffectMultiPlayer, ushort sfxTurretRotation,
                      ushort sfxSinglePlayerImpact, ushort sfxMultiPlayerImpact,
                      ushort sfxSinglePlayerProjectile, ushort sfxMultiPlayerProjectile);

  // The vtable lbl_63_data_40: slot 5 is fn_63_23C0, slot 6 fn_63_1BE8, slot 9 fn_63_2D68 and
  // slot 18 fn_63_3034; the slots in between keep CActor's or CEntity's own entry.
  void Think(float dt, CStateManager& mgr) override;                        // slot 5, fn_63_23C0
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override; // slot 6, fn_63_1BE8
  void PreRender(CStateManager& mgr) override;                              // slot 9, fn_63_2D68
  rstl::optional_object< CAABox > GetTouchBounds() const override;          // slot 18, fn_63_3034

  // The five member functions the module hands the DOL through SPlayerTurret_FuncPtrs
  // (lbl_63_bss_20 +0x4 .. +0x3C). The types are the ones the DOL's table declares.
  CTransform4f GetCameraTransform(CStateManager& mgr);    // fn_63_1070
  CTransform4f GetTurretTransform(CStateManager& mgr);    // fn_63_F6C
  void ExitTurret(CStateManager& mgr);                    // fn_63_1B70
  void ProcessInput(const CFinalInput& input, CStateManager& mgr); // fn_63_1134
  TUniqueId GetHullActorId();                             // fn_63_840

private:
  // Host layout, in the guest's order. The guest offsets are in the comments.
  CAABox mBounds;                     // guest 0x158: a one-unit box around the actor's position
  uint mFlags;                        // guest 0x170: the flagsPlayerTurret property
  float mMaxHorizRotationLeft;        // guest 0x174, radians
  float mMaxHorizRotationRight;       // guest 0x178, radians
  float mMaxVertElevationUp;          // guest 0x17C, radians
  float mMaxVertElevationDown;        // guest 0x180, radians
  float mMaxAimAngle;                 // guest 0x184, radians; retail stores the damage angle here
  TUniqueId mPlayerId;                // guest 0x188
  float mHorizSpeed;                  // guest 0x18C, radians
  float mVertSpeed;                   // guest 0x190, radians
  float mFireRate;                    // guest 0x194
  TUniqueId mBaseActorId;             // guest 0x198: the actor that carries the model
  TUniqueId mTopActorId;              // guest 0x19A: the actor moved onto the hull's locator
  uint mAnimId1;                      // guest 0x19C
  uint mAnimId2;                      // guest 0x1A0
  uint mUnknown1A4;                   // guest 0x1A4
  uint mUnknown1A8;                   // guest 0x1A8
  uint mUnknown1AC;                   // guest 0x1AC
  float mHorizRotation;               // guest 0x1B0: the current horizontal rotation
  float mVertRotation;                // guest 0x1B4: the current vertical rotation
  float mUnknown1B8;                  // guest 0x1B8
  float mUnknown1BC;                  // guest 0x1BC
  float mUnknown1C0;                  // guest 0x1C0
  CVector3f mBaseForward;             // guest 0x1C4..0x1CC: the editor transform's forward vector
  float mUnknown1D0;                  // guest 0x1D0
  CColor mLightColor;                 // guest 0x1D4..0x1DC
  CAssetId mWeaponEffect;             // guest 0x1E0
  CAssetId mWeaponEffectMulti;        // guest 0x1E4
  CDamageInfo mWeaponDamage;          // guest 0x1F0
  float mUnknown20C;                  // guest 0x20C
  CVector3f mTargetPosition;          // guest 0x210: the editor transform's translation
  uint mSfxHandle;                    // guest 0x21C
  TUniqueId mHullActorId;             // guest 0x220
  ushort mSfxTurretRotation;          // guest 0x222
  ushort mSfxSinglePlayerImpact;      // guest 0x224
  ushort mSfxMultiPlayerImpact;       // guest 0x226
  ushort mSfxSinglePlayerProjectile;  // guest 0x228
  ushort mSfxMultiPlayerProjectile;   // guest 0x22A
  bool mPlayerInTurret : 1;           // guest 0x22C bit 0x80
  bool mUnknown22C_6 : 1;             // guest 0x22C bit 0x40
#else
  uchar x158_[0x2c];
  float mMaxAimAngle; // 0x184
  uchar x188_[0x88];
  CVector3f mTargetPosition; // 0x210
#endif
};
CHECK_OFFSETOF(CScriptPlayerTurret, mMaxAimAngle, 0x184)
CHECK_OFFSETOF(CScriptPlayerTurret, mTargetPosition, 0x210)

#endif // _CSCRIPTPLAYERTURRET
