# SwampBossStage2 REL (module 79): behaviour of `CSwampBossStage2`

Chykka adult / Dark Chykka. Port: `include/MetroidPrime/Enemies/CSwampBossStage2.hpp`,
`src/MetroidPrime/Enemies/PortCSwampBossStage2.cpp`. Retail object size 0xF20 (loader
`fn_79_170`, ctor `fn_79_BE3C`). Data section 0x8100e1e8, rodata 0x8100dd10.

**Port status: partial.** The skeleton, the ctor, the FSM tables, every trigger, Lurk, Think and
AcceptScriptMsg are ported. Most states and helpers are still `TODO(uncertain)` stubs (§6).

## 1. Function map

| Retail | Port | Status |
|---|---|---|
| 170 | `PortLoadSwampBossStage2`, `RELMain`/`RELExit` | ported |
| BE3C | ctor | ported (vulnerability init is uncertain) |
| B8D4 | dtor | ported |
| A4B4 | `SetupStateMachine` | ported |
| AFB4 | `Think` | ported |
| B3CC | `AcceptScriptMsg` | ported (5830 chain walk is TODO) |
| A540 | `GetAimPosition` | ported |
| 2690 | `GetScannableObjectInfo` | ported (light/dark scan) |
| 44 | `TakeDamage` | equals the base |
| A76C / A78C | `PreRender` / `Render` | ported |
| A59C | `AddToRenderer` | ported (actor+0x150 bit-0 gate and AddParticleGen +0x44 args uncertain) |
| A838 | `DoUserAnimEvent` | base call only (TODO, §3) |
| 61C4 | `ProcessDamage` (collision damage) | stub (§3) |
| 5E3C / 5D40 | `GetIngSnatchingNormal/Point` | ported |
| 67A4 | `InitGrowths` | ported |
| 6E68 + 6B78 | `SetupCollisionManager` | ported |
| 25C0 | `Activate` | ported (SetBossParams string index TODO) |
| 5BC8 | `SetGrowthTargetEffects` | ported |
| 5D58 | `SetHeadVulnerable` | uncertain |
| 9F7C…A498 | 19 triggers | ported |
| 72FC | `ResetAttackPhase` | ported |
| 9A3C | `Lurk` | ported (6134 facing approximated) |
| other states | see §4 | stubs |

These retail vtable entries equal the CPatterned/CAi defaults and are not overridden:
0 IsListening, 8 GetTouchBounds, 54 GetModelAlpha, 5C/64 search paths, 6C GetRagDoll,
74 GetAttackTarget, 84 IsOnGround, 90 gravity, 9C death explosion, A4 CanBeShot, AC Listen,
B4 GetOrigin, D0 GetHealthInfo.

## 2. Members (from ctor BE3C)

| Offset | Meaning |
|---|---|
| 7c0 | `SLdrSwampBossStage2Data` by value (phases at 7d8/814/850/88c, size 0x3c) |
| a98 | `CCollisionActorManager*` (10 spheres, rodata_0, mass 1000) |
| ac4..ac7 | wing locator seg ids (rodata_50: L_back, L_front, R_back, R_front) |
| acc + i·0x50 | growth models (optional CModelData ×4) |
| c10..c1c | growth HP (= wingGrowthHealth) |
| c24..c30 | growth flash timers (data_410 = 0) |
| c4c / c50 | light / dark health pools (phase.health) |
| c58..c60 | node uid vector; c64 centre actor; c66 second actor |
| c74..c77 | flag bitfields (head mode c74b7, growths vulnerable c74b6, …) |
| c78/c7c/c80 | additive anim id / weight / rate (max 0.85) |
| c94 | flyer phase 0..3 (even = light, odd = dark) |
| c98 / c9c | attack index / attack type (8 = none) |
| ca8 | `CProjectileInfo` (spit) |
| cf0/cf4/cf8 | spit node, barrage count, node spacing (20) |
| cfc/d08/d14 | PART tokens: splash, wing damage, blow |
| d38 / d3c | water surface Z / CScriptWater uid |
| d40 | swoop `CDamageInfo` |
| d6c | 500.0, collision-actor full HP |
| d94 | recent-damage list {time, dmg} |
| e2c / e5c | light / dark `CDamageVulnerability` |
| e90 | bubble telegraph `CElementGen` |
| ea4 | `CMissileRepeller` (struct38) |
| ee4 | blow `CDamageInfo` |
| f18..f1b | growth big-hit flags; f1c last touched uid |

## 3. Decoded but not yet ported

- **DoUserAnimEvent (A838)**, by event:
  - 0: FireSpit.
  - 1: drop flyers (IM00 to dd0/ca4, phase.unknown_0xef3efec0 times; e28 = 0.3).
  - 7: surface (bubble, z = d38 + 0.01).
  - 8/9: swoop window.
  - 0xe/0xf: head mode, c80 = 0.2 / −0.5.
  - 0x12: face player plus head vulnerable.
  - 0x16: orbit source.
  - 0x1b: chain launch 55B0.
  - 0x21/0x22: circle counting.
  - 0x23/0x24: IS08/IS09 plus materials plus shockwave.
  - 0x25/0x26: spit node plus chain launch.
  - 0x27–0x2b: blow/wind flags.
  - 0x2d: flinch sounds a18/a34/a4c.
- **ProcessDamage (61C4)**:
  - Damage = 500 − collision-actor HP, de-duplicated on f1c.
  - Body hits reduce c4c (clamped ≥ 3) or, in head mode, c50.
  - Growth hits need weapon·forward > 0. They flash c24[i]; dmg ≥ breakStunDamage sets f18[i]; dmg ≥ a64 plays a68; a dead growth spawns the wing effect 3774 and plays a80.
  - Every hit sets the damage colour, calls TakeDamage, sets f10 = max(f10, min(dmg/20, 1)) and resets the HP to 500.
- **State commands** (from the pseudocode):

  | State | Command |
  |---|---|
  | Flinch | Step(Backward, Dodge) |
  | Recover | Step(Forward, Dodge) |
  | DropFlyer | LoopAttack(One) |
  | SpitBarrage | LoopAttack(head ? Two : Zero) |
  | Death | KnockDown(fwd, One) |
  | BecomeDark | KnockBack(One) |
  | BecomeLight | KnockBack(Two) |
  | Blow | Melee(One) |
  | Swoop | Melee(side ? Zero : Two) |
  | SpitWater | ProjectileAttack |
  | Taunt | Taunt(Two) |
  | Circle | Generate(side ? Three : Four) on Deactivate |
  | Reel | LoopHitReaction(Three) |

## 4. FSM

- **Triggers 0–18:** StateOver, AllGrowthsDead, OvipositorDead, ZeroHealth, PlayerInWater,
  ShouldSpit, ShouldBarrage, ShouldTaunt, ShouldSwoop, SwoopOver, CircleDamage, CircleOver,
  ShouldBlow, IsLevel, SwitchedPlatform, Stunned, StunTimeOut, OneGrowthDead, ShouldDropFlyer.
- **States 0–16:** Start, BecomeLightFlyer, BecomeDarkFlyer, DeathSequence, Lurk, DarkLurk,
  SpitWater, SpitBarrage, Taunt, Level, Swoop, Blow, Circle, Reel, Recover, Flinch, DropFlyer.
- **Code 0:** ResetAttackPhase.
- **Name tables:** data_2E4 / E8 / 400.

## 5. Constants (rodata)

- **Sphere radii (rodata_0):** wings 3, spine_5_scale2 5, L_mouth 5, spine6 3, spine_5 4, spine_3/1 3.
- **Effect names (rodata_60):** WingTarget_LB/LF/RB/RF.
- **Floats:**
  - Hover/move: 25, 720, 0.125 (hover distance scale), 360 (slerp deg/s), 0.85 (additive max),
    0.3 (flash time), 0.15 (flash amplitude), 500 (collision HP), 1000 (sphere mass).
  - Splash: 1.6 / 0.8 / 0.2 (splash scale), 8 (splash distance²).
  - Angles: −70/70 (dark-flyer face jitter), 2° (in rad).
- **Material bits:** data_418/41C = 0x31, data_420 = 0x3b.

## 6. Runtime status (2026-10-20)

- **Ported:** every state and helper in sections 1 and 3 except 4F88 SendDecrement, 3354
  PlayerOnGrapplePoint (both stubs, `TODO(uncertain)`) and 1B38 (an additive flinch with no
  caller found).
- **Verified in `04_swamp_temple_dark` with LAYER6 forced:** 0 `[port] caught`, 0 `reach-stub`.
  - Without lock-on, player shots can't reach the growths, so the full fight was driven by
    console `damage` with a test-only bypass of the weapon/same-toucher checks.
  - **Light phase:** Start → Lurk ↔ SpitWater, then Reel and Recover once the growths break.
  - **Dark phase:** BecomeDarkFlyer → DarkLurk → DropFlyer / SpitBarrage; HP goes 500 → 375 →
    250 → BecomeLightFlyer.
  - **Death:** DeathSequence (knockdown, then body state 2 = lying); the boss sends Dead and
    the boss bar clears.
  - **Not seen:** Taunt, Level, Swoop, Blow, Circle, Flinch.
- **Crash fixes:**
  - **Scan tokens:** the host `CToken()` leaves `mObjRef` uninitialized, so the ctor builds the
    tokens in its init list.
  - **mAdditiveAnim:** looked up by 5AE4 (PAS AdditiveReaction, enum 5). Without it,
    UpdateAdditive adds anim −1 and segfaults.
- **Verified facts:**
  - **Body state:** bodyctl +0x37c holds the current body-state ID, so `IsLevel` checks
    `GetCurrentStateId() == Locomotion` against `TargetPoint`.
  - **Collision setup (6E68 → 6B78):**
    - Each joint copies the boss's HealthInfo with HP = 500, adds `kMT_Platform` to its filter's
      exclude list (data_418) and starts with the light vulnerability.
    - The growths (i < 4) also get Orbit and SeekerTarget.
    - The body gets Unknown54 and Scannable, plus a `kFT_Never` filter.
  - **Vulnerabilities:** light = the patterned vulnerability; dark =
    `LdrToDamageVulnerability(ingPossession.ingVulnerability)`.
  - **5D00** is `CEntity::SetActive` (vtable +0x1c).
  - **Ctor end:** `EnableKnockBackPhysics(false)` and `EnableAllAnimReactions(false)`, then
    67A4, 5AE4, 2704.
  - **Boss params:** Activate calls SetBossParams with string `BossSwampBossStage2`.
  - **ProcessDamage (61C4):**
    - Damage = 500 − joint HP. A hit from the same toucher as `mLastTouched` is ignored, so only
      the first console `damage` lands (its toucher is the player, uid 0).
    - A growth hit needs a CWeapon facing the same way as the boss (dot > 0) and
      `mGrowthsVulnerable`.
- **The Dead script chain (area 32) needs Stage 1:**
  - Dead → relay 00200267 → SetToZero → relay 00200236 → Open switch 002000e0 → switch
    00200238 → switch 00200244 → relay 00200243 → sequence timer 00200240. That timer
    increments layer controller 00200239 (the post-battle cinematic).
  - Switches 00200238 and 00200244 are opened only by layer controllers 002001bd and 002001bf
    ("Arrived"). Those are incremented by sequence timer 00200234, which runs after the Stage 1
    death: relay 0020022e → switches 00200230 and 00200232 → relay 00200235.
  - With LAYER6 forced, that chain never runs, so timer 0020022f retries forever. The stall
    comes from the test harness, not from the boss. An end-to-end check needs a Stage 1 →
    Stage 2 run.
- **Remaining `TODO(uncertain)`:**
  - 5D58 head vulnerability swap.
  - 3900 splash → water link.
  - 4324 spit radius.
  - ZeroHealth index field.
  - Circle bit 5 of 0xc76.
  - Riding-platform clear in water.
  - 5830 chain walk.
  - Scan by form.
  - Actor +0x150 draw gate.
  - AddParticleGen slot.
  - mDrawParticles mask.
- **Upstream edits still needed** (TARGET_PC): CMissileRepeller::SetActive, the CPlayer radar
  setters, and possibly CGameProjectile SetProjExtent. None made yet.
