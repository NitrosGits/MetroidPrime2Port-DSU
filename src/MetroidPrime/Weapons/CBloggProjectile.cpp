#include "MetroidPrime/Weapons/CBloggProjectile.hpp"

CBloggProjectile::CBloggProjectile(bool active, const TToken< CWeaponDescription >& description,
                                   EWeaponType type, const CTransform4f& xf,
                                   EMaterialTypes excludeMaterial, const CDamageInfo& damage,
                                   TUniqueId uid, TAreaId areaId, TUniqueId owner,
                                   TUniqueId homingTarget, uint attribs, bool underwater,
                                   const CVector3f& scale, const CImpactVisorEffect& visorEffect,
                                   bool unused, bool playImpactSound, float value)
: CEnergyProjectile(active, description, type, xf, excludeMaterial, damage, uid, areaId, owner,
                    homingTarget, attribs, underwater, scale, visorEffect, unused, playImpactSound,
                    false, 1.f, 4.f, 4.f)
, x568_(value) {}

#ifdef TARGET_PC
// fn_80213508: 0x80213530 bl CGameProjectile::ApplyDamageToOneActor; the rest of the body
// (0x80213544 ObjectById, 0x80213548 TCastToPtr<CActor>, 0x80213554 TCastToPtr<CPlayer>)
// discards its results.
void CBloggProjectile::ApplyDamageToOneActor(CStateManager& mgr, const CDamageInfo& damageInfo,
                                             TUniqueId id, const CVector3f& direction) {
  CGameProjectile::ApplyDamageToOneActor(mgr, damageInfo, id, direction);
}
#else
void CBloggProjectile::ApplyDamageToOneActor(CStateManager& mgr, const CDamageInfo& damageInfo,
                                             TUniqueId id, const CVector3f& direction) {}
#endif

CBloggProjectile::~CBloggProjectile() {}
