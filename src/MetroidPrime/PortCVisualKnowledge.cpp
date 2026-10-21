// Port-only: the DOL's CVisualKnowledge (0x801F55F0..0x801F59D0), which upstream has no source for.
#include "MetroidPrime/CVisualKnowledge.hpp"

#include "Kyoto/CRandom16.hpp"
#include "MetroidPrime/CActor.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/TCastTo.hpp"

// 0x801F5888: the ray includes solids (.sdata 0x80418798, 59) and excludes characters, the player,
// collision actors, projectile passthrough and line-of-sight-excluded materials (0x8041879C..
// 0x804187AC: 33, 32, 47, 20, 56); include/exclude filter (3). No target, the times at 0 (0x8041D610).
CVisualKnowledge::CVisualKnowledge(TUniqueId owner, CSegId locator, float minInterval,
                                   float randomInterval)
: x0_owner(owner)
, x2_locator(locator)
, x8_filter(CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid),
      CMaterialList(kMT_Character, kMT_Player, kMT_CollisionActor, kMT_ProjectilePassthrough,
                    kMT_ExcludeFromLineOfSightTest)))
, x20_target(kInvalidUniqueId)
, x24_minInterval(minInterval)
, x28_randomInterval(randomInterval)
, x2C_timer(0.f)
, x30_visibleTime(0.f)
, x34_hiddenTime(0.f)
, x38_visible(false) {}

// 0x801F55F0: the timer counts down; once it is out (at most 0) the target is not visible until a
// ray from the owner to the target's aim point (vtable 0x54, dt 0) hits no world: it starts at the
// owner's aim point, or with a locator (not CSegId::Null(), 99) at the owner's scaled locator. The
// timer is then minInterval + randomInterval * Float(). Without the owner or the target it stays
// out. The visible or hidden time grows by dt, the other is 0.
void CVisualKnowledge::Update(CStateManager& mgr, float dt) {
  if (x2C_timer > 0.f) {
    x2C_timer -= dt;
  }
  if (x2C_timer <= 0.f) {
    x38_visible = false;
    // Port limit: retail only null-tests the two lookups (0x801F5670, 0x801F568C) and calls
    // vtable 0x54 (GetAimPosition) on them as actors; the casts add a type check it lacks.
    const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(x20_target));
    const CActor* owner =
        target != nullptr ? TCastToConstPtr< CActor >(mgr.GetObjectById(x0_owner)) : nullptr;
    if (owner != nullptr) {
      const CVector3f end = target->GetAimPosition(mgr, 0.f);
      CVector3f start = owner->GetAimPosition(mgr, 0.f);
      if (x2_locator != CSegId::Null()) {
        start = (owner->GetTransform() * owner->GetScaledLocatorTransform(x2_locator))
                    .GetTranslation();
      }
      x38_visible = mgr.RayCollideWorld(start, end, x8_filter, owner);
      x2C_timer = x24_minInterval + x28_randomInterval * mgr.Random()->Float();
    }
  }
  if (x38_visible) {
    x30_visibleTime += dt;
    x34_hiddenTime = 0.f;
  } else {
    x34_hiddenTime += dt;
    x30_visibleTime = 0.f;
  }
}

// 0x801F585C: a new target, not visible, the timer and both times at 0.
void CVisualKnowledge::SetVisualTarget(TUniqueId target) {
  x20_target = target;
  x38_visible = false;
  x2C_timer = 0.f;
  x30_visibleTime = 0.f;
  x34_hiddenTime = 0.f;
}
