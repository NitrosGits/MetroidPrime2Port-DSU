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
} // namespace PortTriReticle
