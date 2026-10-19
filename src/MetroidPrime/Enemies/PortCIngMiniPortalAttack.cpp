// The Ing REL's (module 29) two mini-portal classes, ported for behaviour rather than bytes: the
// portal's data (CIngMiniPortalInfo, fn_29_12100 and its copy fn_29_1201C) and the actor a portal
// spawns (CIngMiniPortalAttack, fn_29_11E8C). Neither is exercised by a run yet, so no `// @`
// markers. The member maps are docs/module-analysis/IngFamily.md, sections A.1 and A.3.
//
// fn_29_11490 is a plain `CModelData()`, inlined below as the base class's model argument.
// fn_29_114B0 is member destruction in reverse plus restoring the class vtable, i.e. the implicit
// destructor, so it has no body.

#include "MetroidPrime/Enemies/CIngMiniPortalAttack.hpp"

#include "Collision/CMaterialList.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CModelData.hpp"

// fn_29_12100 (not yet exercised in a run)
CIngMiniPortalInfo::CIngMiniPortalInfo(TUniqueId target, float x4, float x8,
                                      const TLockedToken< CGenDescription >& effect, ushort sound,
                                      float x1c, float x20, const CDamageInfo& damage,
                                      const SLdrPlasmaBeamInfo& beamInfo)
: mTarget(target)
, mX4(x4)
, mX8(x8)
, mEffect(effect)
, mSound(sound)
, mX1c(x1c)
, mX20(x20)
, mDamage(damage)
, mBeamInfo(beamInfo) {
  // Retail copies the token, copies the cached object word at token+0x08 and locks it; the host
  // TLockedToken copy constructor is that (CToken copy, cached pointer, Lock()).
}

// fn_29_1201C (not yet exercised in a run)
// Retail copies the two floats and the id up front, copies and locks the token, then the sound, the
// two floats and the damage field by field, and finally the beam info as 0x12 words (fn_29_E9E8).
CIngMiniPortalInfo::CIngMiniPortalInfo(const CIngMiniPortalInfo& other)
: mTarget(other.mTarget)
, mX4(other.mX4)
, mX8(other.mX8)
, mEffect(other.mEffect)
, mSound(other.mSound)
, mX1c(other.mX1c)
, mX20(other.mX20)
, mDamage(other.mDamage)
, mBeamInfo(other.mBeamInfo) {}

// fn_29_11E8C (not yet exercised in a run)
// Base: CActor(uid, name, info, 0, xf, CModelData(), CMaterialList(kMT_Projectile),
// CActorParameters(), kInvalidUniqueId); the single material is retail's lbl_29_data_CE0 = 0x23.
CIngMiniPortalAttack::CIngMiniPortalAttack(TUniqueId uid, const rstl::string& name,
                                           const CEntityInfo& info, const CTransform4f& xf,
                                           TUniqueId owner, const CVector3f& scale,
                                           const CIngMiniPortalInfo& portalInfo)
: CActor(uid, name, info, 0, xf, CModelData(), CMaterialList(kMT_Projectile), CActorParameters(),
         kInvalidUniqueId)
, mInfo(portalInfo)
// The beam: the asset id at beamInfo +0x04 (weaponSystem, portalInfo +0x44) and the damage at
// portalInfo +0x24.
, mProjectileInfo(portalInfo.GetBeamInfo().weaponSystem, portalInfo.GetDamage())
, mScale(scale)
, mAimPoint(CVector3f::Zero()) // sZeroVector
, mTime(0.f)                  // lbl_29_rodata_7E0, read as 0.0f
, mProjectileId(kInvalidUniqueId)
, mRelFile(rstl::string_l("Grenchler.rel"), 0)
, mFlags(0) {
  // 0x228: the REL token names "Grenchler.rel". The static rstl::string at lbl_29_bss_7C that the
  // ctor passes is written by the module's static initialiser fn_29_121F4 (it builds
  // rstl::string("Grenchler.rel") there and registers fn_29_2144 as its destructor), so the name is
  // not empty; loadMode 0 makes the token queue the load, as retail does. Grenchler is a real REL
  // (config/G2ME01/rels/Grenchler).
  //
  // 0x230 has bit 7 cleared and the other bits kept, i.e. read from uninitialised memory, which the
  // host does not do (zero above); fn_29_11CA4 sets bit 7 when the portal has aimed.
  //
  // 0x12a, last: retail stores the owner id into CActor's nextDrawNode.
  SetNextDrawNode(owner);
}
