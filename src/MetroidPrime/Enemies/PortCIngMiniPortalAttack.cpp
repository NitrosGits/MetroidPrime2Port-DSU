// The Ing REL's (module 29) two mini-portal classes, ported for behaviour rather than bytes: the
// portal's data (CIngMiniPortalInfo, fn_29_12100 and its copy fn_29_1201C) and the actor a portal
// spawns (CIngMiniPortalAttack, fn_29_11E8C). Its Think, AcceptScriptMsg and the open/fire/aim
// helpers are ported too, but neither class is exercised by a run yet, so no `// @` markers. The
// member maps are docs/module-analysis/IngFamily.md, sections A.1 and A.3.
//
// fn_29_11490 is a plain `CModelData()`, inlined below as the base class's model argument.
// fn_29_114B0 is member destruction in reverse plus restoring the class vtable, i.e. the implicit
// destructor, so it has no body.

#include "MetroidPrime/Enemies/CIngMiniPortalAttack.hpp"

#include "Collision/CMaterialList.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/Weapons/CBeamInfo.hpp"
#include "MetroidPrime/Weapons/CPlasmaProjectile.hpp"
#include "MetroidPrime/Weapons/CWeaponAssetInfo.hpp"

namespace {

// lbl_29_rodata_7F0 (1.2217305): the most the aim point turns in one frame, 70 degrees.
const float kMaxAimTurn = 1.2217305f;

} // namespace

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

// fn_29_11CA4 (not yet exercised in a run)
// Retail vtable slot 0x14 (Think). The portal ages 0x220; with the fired beam (0x224) still alive,
// at mInfo's lifetime (info+0x08) it stops the beam (retail slot 0x90 = CPlasmaProjectile::
// ResetBeam(mgr, false)) and requests it deleted, and between the aim time (info+0x04) and the
// lifetime it fires once (retail slot 0x94 = Fire(xf, mgr, false)) and sets bit 7 of 0x230, from
// then on re-aiming the live beam each frame (retail slot 0x8c = UpdateFx(xf, dt, mgr)). The beam's
// transform is a LookAt from the actor's translation to mAimPoint. A portal left with no beam
// deletes itself, and CActor::Think runs last.
void CIngMiniPortalAttack::Think(float dt, CStateManager& mgr) {
  mTime += dt;

  if (CPlasmaProjectile* beam = static_cast< CPlasmaProjectile* >(mgr.ObjectById(mProjectileId))) {
    if (mTime >= mInfo.GetLifeTime()) {
      beam->ResetBeam(mgr, false);
      mgr.DeleteObjectRequest(mProjectileId);
      mProjectileId = kInvalidUniqueId;
    } else if (mTime >= mInfo.GetAimTime()) {
      if ((mFlags & 0x80) != 0) {
        beam->UpdateFx(CTransform4f::LookAt(GetTranslation(), mAimPoint, CVector3f::Up()), dt,
                       mgr);
      } else if (const CActor* target =
                     static_cast< const CActor* >(mgr.GetObjectById(mInfo.GetTarget()))) {
        AimAtTarget(mgr, *target);
        beam->Fire(CTransform4f::LookAt(GetTranslation(), mAimPoint, CVector3f::Up()), mgr, false);
        mFlags |= 0x80;
      }
    }
  }

  if (mProjectileId == kInvalidUniqueId) {
    mgr.DeleteObjectRequest(GetUniqueId());
  }
  CActor::Think(dt, mgr);
}

// fn_29_11C14 (not yet exercised in a run)
// Retail vtable slot 0x18 (AcceptScriptMsg): XDEL (kSM_Delete) drops the fired beam, XCRT
// (kSM_Create) opens the portal. CStateManager::AddObject delivers kSM_Create to a new actor, so
// fn_29_11A7C is the normal path one frame after the spawn.
void CIngMiniPortalAttack::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CActor::AcceptScriptMsg(mgr, msg);
  switch (msg.GetMessage()) {
  case kSM_Delete:
    mgr.DeleteObjectRequest(mProjectileId);
    break;
  case kSM_Create:
    Open(mgr);
    break;
  default:
    break;
  }
}

// fn_29_11A7C (not yet exercised in a run)
// The portal opening: a 0x180-byte "Ing Mini Portal Fx" CExplosion from mInfo's effect token
// (info+0x0c) in the actor's own transform and scale, with a CEntityInfo(area, null connections,
// active, kInvalidEditorId); then a 3D CSfxManager emitter (info+0x1c max distance, info+0x20
// falloff, info+0x18 sound, position = the actor's translation); then the shot. Both the emitter
// and fn_29_11840 run only if the explosion was created.
void CIngMiniPortalAttack::Open(CStateManager& mgr) {
  const CEntityInfo info(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId);
  CExplosion* explosion =
      rs_new CExplosion(mInfo.GetEffect(), mgr.AllocateUniqueId(), info,
                        rstl::string_l("Ing Mini Portal Fx"), GetTransform(), 0, mScale,
                        CColor::White(), -1);
  if (!explosion) {
    return;
  }
  mgr.AddObject(*explosion);

  CAudioSys::C3DEmitterParmData parms(mInfo.GetSfxMaxDist(), mInfo.GetSfxFalloff(), 1, 0x7f, 0);
  parms.mPos = GetTranslation();
  parms.mSfxId = mInfo.GetSound();
  CSfxManager::AddEmitter(parms, GetCurrentAreaId().Value(), true, false,
                          CSfxManager::kMedPriority);

  Fire(mgr);
}

// fn_29_11840 (not yet exercised in a run)
// The shot: aim (fn_29_11570), then TLdrToBeamInfo over mInfo's beam info (info+0x40) with
// attributes 0x291, allocate mProjectileId (retail 0x224) and spawn the 0x6a8-byte "Ing Mini Portal
// Beam" CPlasmaProjectile at the identity transform with material 0x14 (kMT_ProjectilePassthrough).
// Retail takes the EWeaponType from the CProjectileInfo damage (0x1ec is the damage; its weapon
// mode's low 16 bits are the type) and, like the host's other plasma beams, records the portal as
// the beam's next draw node (retail 0x12a).
void CIngMiniPortalAttack::Fire(CStateManager& mgr) {
  const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mInfo.GetTarget()));
  if (!target) {
    return;
  }
  AimAtTarget(mgr, *target);

  const CBeamInfo beamInfo = TLdrToBeamInfo(mInfo.GetBeamInfo(), 0x291);
  mProjectileId = mgr.AllocateUniqueId();
  const CDamageInfo& damage = mProjectileInfo.GetDamage();
  CPlasmaProjectile* beam = rs_new CPlasmaProjectile(
      mProjectileInfo.Token(), rstl::string_l("Ing Mini Portal Beam"),
      EWeaponType(damage.GetWeaponMode().GetRawType()), beamInfo, CTransform4f::Identity(),
      kMT_ProjectilePassthrough, damage, mProjectileId, GetCurrentAreaId(), GetUniqueId(),
      CWeaponAssetInfo(), false, 0x20000);
  if (!beam) {
    return;
  }
  mgr.AddObject(*beam);
  beam->SetNextDrawNode(GetUniqueId());
}

// fn_29_11570 (not yet exercised in a run)
// Aims: the target's aim position is sampled at t = 0 and again at the beam's travel time (retail
// 0x1d0, beam info +0x38), where t = |0.5 * (aim0 + target translation) - translation| / speed.
// The two samples are blended 0.25 * ((aim1 + target translation) - aim0) + 0.75 * aim1; the
// result is measured against the actor transform's second basis column (retail 0x28/0x38/0x48 =
// GetForward) and mAimPoint (retail 0x214) takes the blend when within kMaxAimTurn, else the
// forward slewed towards the blend by exactly that angle. A blend too short to normalise falls
// back to translation + forward.
void CIngMiniPortalAttack::AimAtTarget(CStateManager& mgr, const CActor& target) {
  const CVector3f translation = GetTranslation();
  const CVector3f forward = GetTransform().GetForward();
  const CVector3f startAim = target.GetAimPosition(mgr, 0.f);
  const CVector3f targetPos = target.GetTranslation();
  const CVector3f toTarget = 0.5f * (startAim + targetPos) - translation;

  const float travelSpeed = mInfo.GetBeamInfo().travelSpeed;
  const float time = travelSpeed > 0.0f ? toTarget.Magnitude() / travelSpeed : 0.0f;

  const CVector3f endAim = target.GetAimPosition(mgr, time);
  const CVector3f blended = 0.25f * ((endAim + targetPos) - startAim) + 0.75f * endAim;
  const CVector3f delta = blended - translation;

  if (!delta.IsMagnitudeSafe()) {
    mAimPoint = translation + forward;
    return;
  }
  if (CVector3f::GetAngleDiff(forward, delta) <= kMaxAimTurn) {
    mAimPoint = blended;
    return;
  }
  mAimPoint = translation + CVector3f::Slerp(forward, delta.AsNormalized(),
                                             CRelAngle::FromRadians(kMaxAimTurn));
}
