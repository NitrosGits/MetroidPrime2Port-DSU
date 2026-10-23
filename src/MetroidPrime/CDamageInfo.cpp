#include "MetroidPrime/CDamageInfo.hpp"

#include "Kyoto/Math/CloseEnough.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"

static inline bool does_damage(const CWeaponTypeVulnerability& vulnerability) {
  return !close_enough(vulnerability.mDamageMultiplier, 0.f) &&
         vulnerability.mEffect != CWeaponTypeVulnerability::kE_Immune;
}

static inline bool check_hurts(const CWeaponTypeVulnerability& vulnerability) {
  bool hurts = true;
  if (!does_damage(vulnerability) && vulnerability.mEffect != CWeaponTypeVulnerability::kE_Immune) {
    hurts = false;
  }
  return hurts;
}

float CDamageInfo::GetDamage(const CDamageVulnerability& dVuln) const {
  if (dVuln.GetEffect(mWeaponMode) == CWeaponTypeVulnerability::kE_Immune) {
    return 0.f;
  }
  return GetVulnerableDamage(dVuln);
}

float CDamageInfo::GetVulnerableDamage(const CDamageVulnerability& dVuln) const {
  const CWeaponTypeVulnerability vulnerability = dVuln.GetVulnerability(mWeaponMode);
  return mDamage * vulnerability.mDamageMultiplier;
}

float CDamageInfo::GetRadiusDamage(const CDamageVulnerability& dVuln) const {
  if (dVuln.GetEffect(mWeaponMode) == CWeaponTypeVulnerability::kE_Immune) {
    return 0.f;
  }
  return GetVulnerableRadiusDamage(dVuln);
}

float CDamageInfo::GetVulnerableRadiusDamage(const CDamageVulnerability& dVuln) const {
  const CWeaponTypeVulnerability vulnerability = dVuln.GetVulnerability(mWeaponMode);
  return mRadiusDamageAmount * vulnerability.mDamageMultiplier;
}

float CDamageInfo::GetKnockBackPower(const CDamageVulnerability& dVuln, float distance) const {
  if (dVuln.GetEffect(mWeaponMode) == CWeaponTypeVulnerability::kE_Immune) {
    return 0.f;
  }
  return GetVulnerableKnockBackPower(dVuln, distance);
}

float CDamageInfo::GetVulnerableKnockBackPower(const CDamageVulnerability& dVuln,
                                               float distance) const {
  const CWeaponTypeVulnerability vulnerability = dVuln.GetVulnerability(mWeaponMode);
  if (!check_hurts(vulnerability)) {
    return 0.f;
  }

  const float radius = mDamageRadius;
  const bool hasFalloff = radius != 0.f && distance != 0.f;
  float falloff = hasFalloff ? (radius - distance) / radius : 1.f;
  if (vulnerability.mDamageMultiplier <= 0.5f) {
    falloff *= 2.f;
  }
  return falloff * mKnockbackPower;
}

CDamageInfo::CDamageInfo(const CDamageInfo& other, float dt)
: mWeaponMode(other.mWeaponMode)
, mDamage(other.mDamage * (60.f * dt))
, mRadiusDamageAmount(mDamage)
, mDamageRadius(other.mDamageRadius)
, mKnockbackPower(other.mKnockbackPower)
, mDamageSfxId(other.mDamageSfxId)
, mDamageLoopSfxId(other.mDamageLoopSfxId)
, mSamusVoiceSfxId(other.mSamusVoiceSfxId)
, mNoImmunity(true)
, mApplyRadiusDamage(other.mApplyRadiusDamage) {}
#ifdef TARGET_PC

// fn_800B5F8C (DOL; only caller CParasite contact damage).
CDamageInfo CDamageInfo::WithNoImmunity() const {
  CDamageInfo ret;
  ret.mWeaponMode = mWeaponMode;               // 0x800B5F8C lwz 0x0 / stw 0x0
  ret.mDamage = mDamage;                       // 0x800B5F94 lfs 0x4 / stfs 0x4
  ret.mRadiusDamageAmount = ret.mDamage;       // 0x800B5FA8 lfs 0x4(r3) / stfs 0x8(r3)
  ret.mDamageRadius = mDamageRadius;           // 0x800B5F9C lfs 0xc / stfs 0xc
  ret.mKnockbackPower = mKnockbackPower;       // 0x800B5FA4 lfs 0x10 / stfs 0x10
  ret.mDamageSfxId = mDamageSfxId;             // 0x800B5FAC lhz 0x14 / sth 0x14
  ret.mDamageLoopSfxId = mDamageLoopSfxId;     // 0x800B5FB4 lhz 0x16 / sth 0x16
  ret.mSamusVoiceSfxId = mSamusVoiceSfxId;     // 0x800B5FC4 lhz 0x18 / sth 0x18
  ret.mNoImmunity = true;                      // 0x800B5FD8 rlwimi r0,r5(=1),7,24,24: 0x1A |= 0x80
  ret.mApplyRadiusDamage = mApplyRadiusDamage; // 0x800B5FE4 rlwimi r0,r6,0,25,25: src 0x1A & 0x40
  return ret;
}
#endif

void CDamageInfo::SetDamageFromVulnerability(const CDamageVulnerability& dVuln, float damage) {
  const float multiplier = dVuln.GetVulnerability(mWeaponMode).mDamageMultiplier;
  if (close_enough(multiplier, 0.f)) {
    mDamage = 0.f;
  } else {
    mDamage = damage / multiplier;
  }
}
