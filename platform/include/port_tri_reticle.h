#pragma once

// Pure Trilogy reticle equations for Echoes. Visor/state mapping is Echoes' EReticleState
// (Combat 0, Scan 1, Echo 2, Dark 3); the curves are MP1's (fn_80236A98, fn_80307210/80307370),
// not yet confirmed in rs5mp2_p.dol (the combat code there loads the same 3-arm/120 degree layout).
namespace PortTriReticle {
inline float Clamp(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }
// Aim arms/centre draw in every visor but Scan; the lock arms only in Combat (MP1 AimVisible/LockVisible).
inline bool AimVisible(int echoesState) { return echoesState == 0 || echoesState == 2 || echoesState == 3; }
inline bool LockVisible(int echoesState) { return echoesState == 0; }
inline float ArmResponse(float d) { return d <= 10.f ? 1.f : d >= 30.f ? .5f : 1.f - (d - 10.f) * .025f; }
inline float CentreResponse(float d) { return d <= 10.f ? 1.f : d >= 30.f ? .6f : 1.f - (d - 10.f) * .02f; }
// Per update, not dt scaled.
inline float ScaleStep(float value, float target) {
  return Clamp(value + Clamp(target - value, -1.f / 45.f, 1.f / 45.f), 0.f, 1.f);
}
// Lock fade: rises over `fadeIn` s while locked, falls over `fadeOut` s when released.
inline float FadeStep(float t, bool on, float dt, float fadeIn, float fadeOut) {
  return Clamp(t + (on ? dt / fadeIn : -dt / fadeOut), 0.f, 1.f);
}
// Locked-reticle timing from the time a target has been locked (MP1 fn_8023B228; the 0.25 s
// steps and 3 -> 0.35 base scale are not confirmed in the Echoes DOL).
inline float LockBaseScale(float lockTime) {
  const float u = Clamp(lockTime / .25f, 0.f, 1.f);
  return 3.f * (1.f - u) + .35f * u;
}
inline float LockBracketAlpha(float lockTime) { return Clamp((lockTime - .25f) / .25f, 0.f, 1.f) * (50.f / 255.f); }
inline float LockTechTime(float lockTime) { return Clamp(.5f * ((lockTime - .25f) / .25f), 0.f, .496f); }
inline bool LockTechSpins(float lockTime) { return (lockTime - .25f) / .25f >= 1.f; }
// Lock arm alpha: 200/255 until 0.25 s, then decays to 0.35.
inline float LockArmAlpha(float alpha, float lockTime, float dt) {
  return lockTime >= .25f ? Clamp(alpha - dt * (200.f / 255.f) / 3.5f, .35f, 200.f / 255.f) : 200.f / 255.f;
}
// Scan progress segments (rs5mp2_p.dol 0x8025e438): six of them, segment i at -60*i degrees,
// value clamp(6 * progress - 5 + i, 0, 1) (fmuls 6*progress, fsel pair at 0x8025e46c/0x8025e484).
inline float ScanSegment(float progress, int i) { return Clamp(6.f * progress - 5.f + static_cast< float >(i), 0.f, 1.f); }
} // namespace PortTriReticle
