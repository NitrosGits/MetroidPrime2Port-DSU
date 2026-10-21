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
| 5D58? | `SetHeadVulnerable` | uncertain, and 5D58 is not a function start in `symbols.txt` |
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

- **Ported:** every state and helper in sections 1 and 3 except 4F88 SendDecrement (no caller
  anywhere in the module: no state, trigger, code function or vtable slot references it, so it
  stays a documented no-op) and 1B38 (an additive flinch with no caller found). 3354
  PlayerOnGrapplePoint is ported (the player's orbit target casts to CScriptGrapplePoint) and runs
  from Think every frame.
- **Verified in `04_swamp_temple_dark` with LAYER6 forced:** 0 `[port] caught`, 0 `reach-stub`,
  `exit=0`.
  - Without lock-on, player shots can't reach the growths, so the full fight was driven by
    console `damage` with a test-only bypass of the weapon/same-toucher checks.
  - **Light phase:** Start → Lurk ↔ SpitWater, then Reel and Recover once the growths break.
    **Taunt (fn_79_8A3C) is now reached too:** a probe run (no `damage`, one console `damage 250`
    at most, `aim` + `press a` volleys) cycles `Lurk ↔ Taunt` for 2400+ frames at `mAttackType`
    0, 1, 2, 4 and 7. Zeroing `mHeadHealth` (console `damage 250` on a body joint: head 100 →
    −150) sends the boss back to **Reel**, i.e. `Stunned` → the LightLurk submachine's Reel.
    **Swoop, Level and Circle are reached as well** (see "the light-phase attack chain" below).
  - **Dark phase:** BecomeDarkFlyer → DarkLurk → DropFlyer / SpitBarrage; HP goes 500 → 375 →
    250 → BecomeLightFlyer.
  - **Death:** DeathSequence (knockdown, then body state 2 = lying); the boss sends Dead and
    the boss bar clears.
  - **Where the graph lives:** the boss is a `CGenericFSM2` actor (`machine type 1`), so the FSM
    is an FSM2 asset, not a `CStateMachine`: top machine Start / BecomeDarkFlyer /
    BecomeLightFlyer / DeathSequence, submachines `LightFlyer` (Start, Reel, Recover, Flinch +
    `LightLurk`: Lurk, SpitWater, Taunt, Swoop, Level, Circle, SpitBarrage) and `DarkFlyer`
    (DarkLurk, SpitWater, DropFlyer, Taunt, SpitBarrage, Level, Blow).
  - **Phase attack slots (from the SCLY pattern data, first…fifth):** light1 `0 1 2 4 8`,
    dark1 `5 3 6 3 8`, light2 `1 4 2 4 8`, dark2 `3 5 6 8 8` (tauntChance 0.20/0.10/0.10/0.05,
    attack delay 2–4 s, health 100/620/120/620). So Swoop (4) is scheduled in both light phases
    and Blow (6) only exists in the dark ones.
- **Marker hygiene (2026-10-20):** two markers were dropped because
  `tools/port_progress.py` reports them as not a function start in the module's `symbols.txt` and no
  trace ran them: `fn_79_6250` (it had been put on `StateOver`, whose real offset is still open - the
  doc's `A498` body is a small integer compare, not the animation-over test) and `fn_79_5D58`
  (`SetHeadVulnerable`, carried over from the first pass). 74 unique retail functions are marked.
- **The asset's edges (2026-10-20 probe).** The trigger dump has to print the transition *targets*
  (`SB2TargetKind`/`SB2TargetName`), not just `SStateMachine2Transition::mName`, which is empty for
  these edges - that is why the lists used to look empty. With the targets printed the machines
  read (the transition list of a node is in the order the asset stores it):

      top               : Start -> sub:LightFlyer/Start
                         BecomeDarkFlyer   : trig:StateOver -> sub:DarkFlyer/DarkLurk
                         BecomeLightFlyer  : trig:StateOver -> sub:LightFlyer/Start
                                            trig:StateOver -> sub:DarkFlyer/DarkLurk
                         DeathSequence     : (no edges)
                         AllGrowthsDead -> state:BecomeDarkFlyer
                         OvipositorDead -> state:BecomeLightFlyer
                         ZeroHealth -> state:DeathSequence

      LightFlyer (entry): trig:Stunned -> Stunned -> state:Reel
                                          else     StateOver -> state:Start
      LightFlyer.Start   : sub:LightLurk/Lurk
      LightFlyer.Reel    : trig:StunTimeOut trig:OneGrowthDead
                           StunTimeOut   -> state:Recover
                           OneGrowthDead -> state:Flinch
      LightFlyer.Recover : trig:StateOver -> state:Start
      LightFlyer.Flinch  : trig:StateOver -> state:Start

      LightLurk.Lurk     : trig:PlayerInWater trig:ShouldSwoop trig:ShouldTaunt
                           trig:ShouldSpit trig:ShouldBarrage
                           PlayerInWater -> state:Taunt
                           ShouldSwoop   -> state:Level
                           ShouldTaunt   -> state:Taunt
                           ShouldSpit    -> state:SpitWater
                           ShouldBarrage -> state:SpitBarrage
      LightLurk.Level    : trig:IsLevel   -> state:Swoop
      LightLurk.Swoop    : trig:SwoopOver -> state:Circle
      LightLurk.Circle   : trig:CircleOver trig:CircleDamage trig:StateOver -> state:Lurk
      LightLurk.SpitWater / Taunt / SpitBarrage : trig:StateOver -> state:Lurk

      DarkFlyer.DarkLurk : trig:ShouldTaunt trig:ShouldDropFlyer trig:ShouldSpit
                           trig:PlayerInWater trig:ShouldBarrage trig:ShouldBlow
                           ShouldTaunt     -> state:Taunt
                           ShouldDropFlyer -> state:DropFlyer
                           ShouldSpit      -> state:SpitWater
                           PlayerInWater   -> state:Taunt
                           ShouldBarrage   -> state:SpitBarrage
                           ShouldBlow      -> state:Level
      DarkFlyer.Level    : trig:IsLevel -> state:Blow

      code ResetAttackPhase -> state:Taunt   (no state in the asset references this code node)

- **The light-phase attack chain now runs end to end.** A traced run in `04_swamp_temple_dark`
  (LAYER6 forced) with the player left on dry land gives `exit=0`, 0 `[port] caught`, 0
  `reach-stub`, and walks `Lurk` (frame 3005, `mAttackType` 4, `mAttackReady` 1) → **Level**
  (3180) → **Swoop** (3450) → **Circle** (3745) → `Lurk` (4420, `mAttackType` 7) → `Taunt` (4595).
  - **Why the earlier runs never saw it:** `Lurk` tests `PlayerInWater` *first*, and that edge goes
    to **Taunt**, so a player standing in the water swallows every attack type (`mAttackType`
    0/1/2 included) and the boss only ever cycles `Lurk ↔ Taunt`. The old runs teleported the
    player next to the boss, which is over the water; their dumps show `pwater=1` for most of the
    run, while the successful one shows `pwater=0` throughout.
  - `ShouldSwoop` needs `mAttackReady && mAttackType == 4`; light1's fourth slot is 4, so the
    fourth `Lurk` activation queues it (the `RecentDamage > 100` override does the same).
  - `Level → Swoop` is `IsLevel` (fn_79_A13C): the body must be in `Locomotion`, `mMoveSpeed`
    < 0.2, the boss within 2 units of the target point and within 1 unit of its height, and its
    forward within 2° of `mLevelTarget` (the chain node picked on `Level`'s activation). The boss
    reaches it on its own: `Level` steers to the target point while facing `mLevelTarget`.
  - `Swoop → Circle` is `SwoopOver` (fn_79_A31C) = bit 5 of 0xc76 = `mCircleCounting`, which the
    animation event 0x21 (`kUE_EffectOn`) raises while the boss circles. The circle is counted
    twice (`mCircleCount` 2) and the boss returns to `Lurk`.
- **Still not reached, and why (2026-10-20):**
  - **Blow:** attack 6 only exists in the dark phases, and the dark phase needs
    `AllGrowthsDead`, i.e. all four growths at 0 HP. `mGrowthHealth` only drops in the growth
    branch of `ProcessDamage` (61C4), which needs a `CWeapon` whose forward has dot > 0 with the
    boss's and `mGrowthsVulnerable`. Console `damage` carries no weapon, so the growths cannot die
    without a real beam hit on a growth.
    **The "the platform's beam cannot damage a collision actor" note in the first pass was wrong:**
    the beam's ray path is fine (SwampBoss.md, "Shooting, measured" - the projectile follows the
    camera's aim exactly). What was missing was a way to *aim*: the harness aimed from a camera
    that was falling and from stale joint positions, so every shot missed. The dot > 0 test is
    real, though: the shot's direction has to be within 90 degrees of the boss's facing, so the
    player has to shoot when the boss faces away (or from behind it).
  - **Flinch:** its entry edge is now known - `LightFlyer.Reel --OneGrowthDead--> Flinch`
    (`mGrowthDestroyed && mGrowthDeadTime > 0.3`) - but a growth death needs the same weapon hit
    as Blow, so the edge is identified but not exercisable yet.
  - **The circle damage path** (`CircleDamage -> Lurk`): the trigger runs in the Circle state
    (`mCircleDamaged`), but nothing damaged the boss there; a body joint hit would do it (the
    head pool is clamped at 3 while the circle counts, so it does not stun the boss).
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
    - **The facing test, re-derived from the asm** (fn_79_61C4, 0x62E4..0x632C): the three floats
      it loads from each transform are `+0x28`, `+0x38`, `+0x48`, and `CActor::mTransform` sits at
      `+0x24`, so they are `m01`, `m11`, `m21` - the *forward* vector's components, 0x10 apart in
      the row-major 4x4. The sum is `w_fwd.x*b_fwd.x + w_fwd.y*b_fwd.y + w_fwd.z*b_fwd.z`, compared
      against 0 with `fcmpo`/`ble` (so `<= 0` rejects). The port's
      `Dot(weapon->GetTransform().GetForward(), GetTransform().GetForward()) > 0.f` is the same
      test, not an approximation.
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
  - Riding-platform clear in water.
  - 5830 chain walk.
  - Scan by form.
  - Actor +0x150 draw gate.
  - AddParticleGen slot.
  - mDrawParticles mask.
- **Upstream edits still needed** (TARGET_PC): CMissileRepeller::SetActive, the CPlayer radar
  setters, and possibly CGameProjectile SetProjExtent. None made yet.

## 7. The unmarked functions (2026-10-21 lane)

`config/G2ME01/rels/SwampBossStage2/symbols.txt` lists 236 functions. 76 carried a marker before
this pass, 82 do now. The rest are not 154 missing pieces of behaviour; they split in three.

### 7.1 The REL's own copies of host/DOL classes - cited, not marked

The REL carries its own constructors, copy constructors, assignment operators and destructors for
the loader structs, its own `rstl::vector`/list growth and free helpers, its own `CToken` copies,
the loader's read/convert helpers and the vtable stubs. The host already has all of them
(`rstl::vector`, `rstl::list`, `CToken`, `CTransform4f`, `CModelData`, `CCollisionActorManager`,
`LdrTo*`, `LoadTypedef*`), so a port copy would only duplicate the host. These are cited here
rather than reimplemented and marked:

    0 140 1588 16D8 1714 178C 1A64 1AA0 1C44 1C80 1E04 200C 2084 28D4 2CEC 2D3C 2D94 31C0 32C0
    3EAC 3EF4 44 460 4670 46B8 476C 47AC 47CC 47F4 4874 48E0 491C 4958 49B4 49D4 4A0 4A1C 4AA8
    4AE0 4B50 4BC8 4C0 4E8 54 5C 624 64 6644 690 6974 69A4 69EC 6AB0 6C 6D9C 6E4 6E5C 7044 707C
    709C 70C4 7184 71E0 7264 729C 72FC 74 748 74B8 7514 7690 7B84 7DC8 825C 84 8B58 8F50 90
    9148 94D8 9C 9F7C A064 A3D8 A4 A418 A498 A4AC AC B370 B4 B7F8 B8A8 BD54 BDE8 C68C C710 C7A8
    C7FC C92C C9A4 C9FC CA58 CA94 CB30 CB68 CBD4 CBF4 CC1C CCB0 D0 D094 D0F8 D174 D218 D2B0
    D2EC D384 D3C0 D480 D4E8 D524 D598 D60C

The loader's own copy chains are the same kind of body: `1228`/`134C` (the loader struct's ctor
and copy ctor), `7A8`/`830`/`980`/`170` (the property readers), `B8D4`/`BB70`/`CC58` (the
object's destructor), `C74C`/`6D40` (the `__copy` helpers), `C7FC`/`C68C` (the collision-actor
manager's copies).

### 7.2 Module logic that is still unmarked

| fn | what | why it is not marked |
|---|---|---|
| 55B0, 50AC, 5830 | the platform-chain walk: 55B0 wakes the chain, 5830 finds the "PLTR/FOLL" node and 50AC recurses over the "ARRV/NEXT" connections | not ported: the port's `LaunchPlatformChain` is an approximation and `AcceptScriptMsg`'s chain walk is still a `TODO(uncertain)` (the `5830 chain walk` in §6) |
| 5030, 527C | put the chain nodes on their recorded positions / swing them about the chain origin | the port has the two bodies (`PlaceChainNodes`, `SwingChainNodes`) but they are reached only through 55B0, which is not ported, so a run cannot exercise them |
| 7E24 | the `Blow` state | the port's `Blow` is partial (it keeps the blow damage and the wind flag, the rest is still open) |
| 1B38 | an additive flinch with no caller | documented no-op, like 4F88 |
| 4F88 | `SendDecrement` | no caller anywhere in the module |
| 5D40 | `GetIngSnatchingPoint` | the port has the body but the ing-snatch transition never runs in the port |
| 3774 | the wing-damage burst | the port has the body; it needs a growth to die, which needs a real beam hit |
| 6B78 | the second half of `SetupCollisionManager` (the per-joint vulnerability, health and materials) | the port merges it into `SetupCollisionManager` (marked `fn_79_6E68`); it deserves its own body |
| A0C8, B8B4 | a scan helper and `OnScanStateChange` | not ported (the scan-by-form work is still open) |

### 7.3 This lane's slice (the most-called unmarked bodies the port does have)

The five most-called unmarked functions that carry behaviour are **66DC** (6 call sites),
**1E40** (4), **2E84** (4), **55B0** (4) and **50AC** (4); 55B0/50AC are the platform-chain
walk, which the port does not implement (§7.2), so this lane took the next three most-called
ones the port does have a body for - 2A84, 3578 and 33BC (2 call sites each).

| fn | port body | verdict |
|---|---|---|
| 66DC | `CollisionActorPosition` | **fixed and marked**: the fallback when the collision actor is gone is the **zero vector** (0x6724 loads `sZeroVector`), not the boss's own position; the port returned `GetTranslation()`. The run calls it three times a frame from `Think` (5487 calls in one run), so the body is fully exercised; the "no actor" arm cannot fire while the collision manager and its actors exist. `asmcheck.py check` on it: 0 findings. A traced run (both arms instrumented) shows 11250 hits on the "actor found" arm and 0 on the zero-vector fallback, so the fallback is commented in the code instead of being exercised |
| 1E40 | `StartBubbleTelegraph` | **fixed, not marked**: the generator's *global* translation is set (vtable +0x20 = `CParticleGen::SetGlobalTranslation`, index 8 of the particle-gen vtable), not the local one; the old generator is deleted (vtable +0x08 = the deleting destructor, 0x1F40) as the port's `single_ptr` assignment already does. The anim event that calls it (0x07/0x28) never fired in the reachable run, so the body is not marked |
| 2E84 | `PickSwoopTarget` | **fixed, not marked**: the candidate score is measured **from the boss** (0x30F0), not from the player, and the "too close" filter is measured **from the node nearest the player** (0x30AC), not from the node the walk picked. It is called from `DarkLurk` only, and the reachable run never reached the dark phase |
| 2A84 | `SpawnShockWave` | **verified, not marked**: the transform keeps the boss's rotation and takes the new translation (0x2AC0, 0x2AF0); the name is `SwampBossStage2 ShockWave` (rodata 0x2C9); the launch id is the boss's own unique id. It is called from the anim event 0x23, which the reachable run never produced |
| 3578 | `SpawnWindEffect` | **verified, not marked**: the direction is flat (the Z component is the constant 0 of rodata_9C, 0x35E8), the effect sits 50 units short of the node (rodata_B0, 0x3614) and the name is `Chykka wind effect` (rodata 0x2E3). It is called from the `Blow` state, which needs the dark phase |
| 33BC | `DecayDamageMemory` | **verified, not marked**: every entry's time drops by dt and an entry that reaches zero is erased (0x3400, 0x3410). The run only ever had an empty damage memory (the console damage lands while the growths are vulnerable, and the port adds to the memory only for a body hit outside the `Reel`), so the erase arm was never exercised |

**Two retail details the port had wrong, re-derived from the asm**

- The collision actor's position is `CActor::mPosition` at +0x54 (the retail's `fn_79_6054`
  returns `actor + 0x54` and `fn_79_66DC` reads +0x54/+0x58/+0x5c). The host's
  `CActor::GetTranslation()` is the same member, so the port's read was right; only the
  "no actor" fallback was wrong.
- `CParticleGen`'s vtable in the REL is `[0] type-info, [1] D1, [2] D0, [3] Update, [4] Render,
  [5] SetOrientation, [6] SetTranslation, [7] SetGlobalOrientation, [8] SetGlobalTranslation,
  [9] SetGlobalScale, [10] SetLocalScale, [11] SetParticleEmission`. That makes vtable +0x08 the
  deleting destructor (called with the delete flag 1) and +0x20 `SetGlobalTranslation`;
  `CEntity::SetActive` at +0x1c fixes the same base offset (its 7th virtual).

### 7.4 The run this lane used (2026-10-21)

`tools/run_gameplay.sh`, world `3dfd2249`, area `04_swamp_temple_dark`, `MP2_GODMODE=1`, the
smoke5 `cmd_swamp2.txt` console script (`objs swampboss`, `layer 6`, `objs swampboss`,
`obj u05bb`), 2000 frames: `exit=0`, 0 `[port] caught`, 0 `reach-stub`, 0 stand-in lines.

With the boss's functions traced, the reachable fight in that area is the light phase only:
`Lurk` <-> `SpitWater`/`Taunt`, plus `Reel`/`Recover` after a console `damage` on the boss
(`hp 500 -> 250`). The traces show `pwater=0` (the player is not in the water) and the attack
type cycling 0, 1, 7 - the type 4 (the swoop) that `ShouldSwoop` needs never came, so
`Level`/`Swoop`/`Circle`, `DarkLurk` and `Blow` were never entered. That is why 1E40, 2E84,
2A84, 3578 and 33BC are compared and fixed but not marked: **nothing in the reachable run
executes them**, and a marker means the body ran.

The dark phase still needs a real beam hit on a growth (the growth branch of `ProcessDamage`
needs a `CWeapon` whose forward has dot > 0 with the boss's and `mGrowthsVulnerable`), which is
the same gap §6 lists for `Blow`/`Flinch`.

### 7.5 The dark phase and the swoop (2026-10-21, second lane pass)

- **The light phase's swoop is reachable.** With the player on dry land and *no* console damage
  (the damage pulls the boss into `Reel`, whose `Deactivate` resets the attack index, so the
  attack cycle never reaches its fourth slot), the boss walks the light phase's attack slots
  0, 1, 2, 4 - traced as `atk=4 idx=4` - so the `ShouldSwoop` edge is reachable.
- **The dark phase is not reachable without a code hook.** The growth branch of `ProcessDamage`
  (fn_79_61C4, 0x62C4..0x6338) needs three things at once: the hit must be on one of the four
  growth joints, the joint's last-touched object must be a `CWeapon` whose forward has dot > 0
  with the boss's, and `mGrowthsVulnerable` must be set (only the `Reel` state sets it). In the
  host the joint's last-touched object is the *damager* of the damage message
  (`CStateManager::ApplyDamage` sends `kSM_Damage` with the damager as the sender,
  `CStateManager.cpp:2982`), and every player weapon spawns a projectile that is the damager
  (`CGameProjectile::ApplyDamageToOneActor` passes `GetUniqueId()`), so retail's
  `TCastToPtr<CWeapon>` on that id fails and the growth's health never drops. The first pass hit
  the same wall and used a "test-only bypass" of the weapon/same-toucher checks (§6); that is a
  code hook, so the dark phase stays out of reach here.
  The beam has to hit a radius-3 sphere 50 units up and 50 to the side of the player as well:
  several hundred `aim` + `press a` volleys never landed on a joint, so even the message path
  above was never entered.
- **Consequence for the slice:** fn_79_2E84 is called only from `DarkLurk` (fn_79_9754) and
  fn_79_3578 only from the `Blow` state (fn_79_7E24), so both stay unrunnable while the dark
  phase is out of reach; fn_79_1E40/2A84 hang off the light phase's anim events 0x07/0x23/0x28
  (the swoop's splash) and fn_79_33BC needs a hit the run never landed. All four stay
  compared-and-fixed but unmarked.
- **fn_79_7E24 (Blow) is not ported in this pass:** it is the dark phase's blow (540
  instructions of state body plus its callees) and the dark phase cannot be reached in the port,
  so the port's `Blow` stays the partial body it was. It needs a port-side decision on the
  weapon/same-toucher checks first.

### 7.6 The beam and the growth joints (2026-10-21, third lane pass)

- **The cast is not the blocker.** `CGameProjectile : public CWeapon`
  (`include/MetroidPrime/Weapons/CGameProjectile.hpp:27`), and `CEnergyProjectile` derives from
  it, so the projectile that damages a joint *is* a `CWeapon`: retail's `TCastToPtr<CWeapon>` on
  the joint's last-touched object should succeed. A probe build that logged the cast result on
  the boss's `kSM_Damage` message was built for exactly this, but it never logged - the damage
  message never arrived, because no beam hit a joint (see below). The probe was removed again.
- **The growth joints do not move.** With the boss hovering, the four growth spheres keep fixed
  positions (traced): g0 (-176.6, -103.2, 85.1), g1 (-174.5, -104.0, 97.2), g2 (-181.6, -142.9,
  83.6), g3 (-179.5, -143.6, 95.7) - the boss's own origin moves around (-181..-184,
  -115..-132, 58..66) while the joint spheres stay put, so the aim can be computed exactly.
- **The aim still misses.** `tp <x> <y> <z>` (the player is teleported and keeps its heading),
  `face <yaw>` and `aim <yaw> <pitch>` (the camera's angles) were used to place the player
  14-17 units east of a growth, level with it, and fire (`press a`) with the growth's yaw
  computed from those positions; sixteen such combinations over the run never produced a hit
  on any of the ten joint spheres. The beam does not reach: the player falls (gravity) as soon
  as it is teleported into the air, the camera sits above and behind the player so the beam
  starts high, and the wings are radius-3 spheres 50 units up and 50 to the side.
- **What is left to try:** a beam that starts level with the player (the camera's pitch offset
  would have to be known exactly), or a shorter distance to a growth, or a shot that comes from
  the growth's own height (the player would have to be standing on something at z ~ 85).
  Until a joint is hit, `fn_79_2E84` (only reachable from `DarkLurk`), `fn_79_3578` (only from
  `Blow`) and `fn_79_1E40`/`fn_79_2A84`/`fn_79_33BC` stay compared-but-unmarked.

### 7.7 The growth spheres are static while their joints move (2026-10-21, fourth lane pass)

A probe build logged every joint sphere's position next to its locator's world transform
(`CPatterned::GetLctrTransform` of the same name) over several frames, plus the result of
retail's `TCastToPtr<CWeapon>` on the joint's last-touched object:

    boss=(-176.1,-123.8,65.4) fwd=(0.98,-0.18,0.00)
      s0=(-176.6,-103.2,85.1)/j=(-172.3,-104.1,90.7)  s1=(-174.5,-104.0,97.2)/j=(-171.6,-105.1,102.9)
      s2=(-181.6,-142.9,83.6)/j=(-184.3,-142.2,88.3)  s3=(-179.5,-143.6,95.7)/j=(-183.6,-143.2,100.5)
      s4=(-185.7,-116.5,80.6)/j=(-185.4,-117.0,78.6)  (the six body spheres track their joints)
    boss=(-172.6,-123.8,65.7) fwd=(0.93,-0.37,0.00)
      s0=(-176.6,-103.2,85.1)/j=(-166.6,-105.9,83.4)  s1=(-174.5,-104.0,97.2)/j=(-175.8,-101.3,90.1)

- **The cast works.** The first joint hit logs `touched=3040 weapon=1`: the beam's projectile *is*
  a `CWeapon` (`CGameProjectile : public CWeapon`, CGameProjectile.hpp:27), so retail's
  `TCastToPtr<CWeapon>` on the joint's last-touched object succeeds. §7.5's "the cast is the
  blocker" reading was wrong.
- **A beam hit works.** That hit took the joint from 500 to 498 HP: with the player teleported
  level with the spheres and facing them the beam reaches them.
- **The four growth spheres are frozen** at the positions they had when they were made, while
  their joints (and the six body spheres) move with the boss. That is the expected consequence of
  retail's own rule: `UpdatePlayerSideMaterials` (fn_79_24E0) takes the dot of (player - boss)
  with the transform's forward column (0x28/0x38/0x48) against rodata_9C, `srwi r31,r0,31`
  (0x255C) makes the "playerBehind" flag, and that flag goes straight to fn_79_5C74: it activates
  each live growth only while the player is behind the boss and deactivates all of them when it
  is clear. An inactive collision actor is neither moved by `CCollisionActorManager::Update` nor
  in the object space the beam collides against, so while the player stands in front the growth
  spheres sit still and cannot be shot; only the six body spheres are reachable. (The port's code
  was right all along; only its comment said "in front" - it is fixed now.)
- **How to hit them:** the player must be *behind* the boss at the moment `Reel` calls
  fn_79_24E0, i.e. on the opposite side from the boss's facing - the boss faces east in this
  area, so the player has to be west of it - and level with a growth. Once the growths are
  active their spheres sit on their joints, so the aim is the joint position logged above.

### 7.8 The dark phase, driven (2026-10-21, fifth lane pass)

The dark phase is reachable without a code hook: a driver script talks to the port's debug
console (`MP2_CONSOLE=<port>`, `tools/mp2con.py`) and plays the fight. The driver is kept at
`/tmp/opencode/sb2dark/drive.py` (with `run.sh`); it needs the boss's position, facing, the
growth healths and the ten collision actors, which a temporary probe printed (removed again
before the commit).

- **The recipe.** Layer 6 of area 32 (`layer 6`), then a loop that reads the state, keeps the
  player *behind* the boss and level with a growth (§7.7: the growths are only active while the
  player is behind the boss), aims from the player's live position and fires. A beam hit takes
  the growth from 60 HP (`mData.wingGrowthHealth`) down by 2, so 30 hits per growth, 120 in all.
  The growths die, `AllGrowthsDead` fires and the boss becomes a dark flyer (`BecomeDarkFlyer`,
  phase 1); the dark phase's attack slots (`5 3 6 3 8`) then give `DropFlyer` and, through
  `Level`, `Blow`.
- **The stun.** The boss is stunned by the `Stunned` trigger (`mHeadHealth <= 0`), and the head
  health only drops outside the `Reel` (inside it the growths are vulnerable and the body branch
  is skipped). Real beam hits on the body do 2 damage each, which is 50 hits per stun, so the
  driver uses the console's `damage <body joint uid> 25` instead - 25 is 5% of the joint's
  500 HP, and the joint's HP is the boss's own (the head health), so the boss is stunned and
  never dies. `damage u06d9` (the boss itself) does *not* work: it is the *joint's* health that
  the boss reads. The boss ignores a hit from the same object as the last one, so the beam hit
  that follows has to land on another joint (the driver alternates two body joints).
- **Facts the driver needed (all measured):**
  - The player's beam is a `CEnergyProjectile`, not a `CBeamProjectile`, so `MP2_DEBUG_BEAM`
    (the port's beam debug, in `CBeamProjectile::UpdateFx`) never prints for it.
  - The gun fires from 2.15 units above the player's origin and the player falls about 22
    units/s (and floats in the arena's water at z ~ 67), so the pitch has to aim at the growth
    from the player's *live* position.
  - `CPlayer::Teleport` calls `ForceGunOrientation`, which calls `mGun->Holster`: every `tp`
    or `face` holsters the gun, and the next press draws it again, so the shot lands a few
    frames after the teleport - which is also what makes a level shot hit the growth.
  - The gun fires on the *press edge* only: a press in every frame holds the button down and
    the gun never fires again (holding the button charges the beam instead), so the driver
    presses every other iteration.
  - `mData.stunTime` is 15 s (the `Reel`'s length), `mData.breakStunDamage` 30 and
    `mData.stunnedFlinchSoundDamageThreshold` 4, so a 2-damage hit never breaks a growth.
  - The light phase's vulnerability: power 1.0, dark 2.0, light 0.5, annihilator 1.0 (the dark
    beam would do double damage, but the test save does not own it).
- **Marked from this pass** (each body ran in a run that reached the dark phase):
  `fn_79_1E40` (the bubble telegraph, the anim event 0x28), `fn_79_2A84` (the splash shock
  wave, the anim event 0x23), `fn_79_3578` (the wind effect, the `Blow` state),
  `fn_79_2E84` (`PickSwoopTarget`, the `DarkLurk` state; both the dive and the plain arm and
  the "too close" skip ran) and `fn_79_33BC` (the damage memory; the entries reached zero and
  were erased).

### 7.9 Every branch of the marked functions (2026-10-21, sixth lane pass)

The retail's conditional branches of the five marked functions, what each one is, and how it was
proved. The probe (one temporary `[arm]` line per arm, once per arm) was removed again before the
commit; the lines quoted are from the runs. Classes: **(a)** compiler-generated (the null test
after `new`, an inlined `rstl`/`CToken`/refcount helper, a loop condition), **(b)** a real arm
that ran, **(c)** a real arm that the shipped data cannot reach, **(d)** a real reachable arm
that did not run (the marker would have to go).

**fn_79_1E40** (9 branches)

| branch | what | class |
|---|---|---|
| 0x1E64 | the effect's asset id is 0xffff -> the early return; the port's `if (mData.bubbleTelegraphEffect == kInvalidAssetId) return;` | (c) the taken path is the "this area has no bubble effect" case; the body ran: `[arm] 1 1E40 body` |
| 0x1E8C | the new `CElementGen` is null | (a) the compiler's null test after `new` |
| 0x1F14 | the new pointer is the old one | (a) `rstl::single_ptr`'s self-assignment check |
| 0x1F20 | the old generator is null -> no delete | (a) the null test before the deleting destructor |
| 0x1F2C | the old token is null -> no destructor | (a) `CToken`'s null test |
| 0x1F64 | the new pointer is null | (a) the compiler's null test after `new` |
| 0x1F70 | the new token is null | (a) `CToken`'s null test |
| 0x1F8C | the token's destructor is skipped when null | (a) `CToken`'s destructor guard |
| 0x1FA0 | the second token's destructor is skipped when null | (a) `CToken`'s destructor guard |

**fn_79_2A84** (5 branches)

| branch | what | class |
|---|---|---|
| 0x2ABC | the shock wave's effect id is 0xffff -> the early return; the port's `if (mData.splashShockWave.shockWaveEffect == kInvalidAssetId) return;` | (c) the "this area has no shock wave" case; the body ran: `[arm] 3 2A84 body` |
| 0x2B14 | the new `CShockWave` is null | (a) the null test after `new` |
| 0x2BC8 | the string's `internal_dereference` is skipped when null | (a) an inlined `rstl::string` helper |
| 0x2BD8 | the string's destructor is skipped when null | (a) an inlined `rstl::string` helper |
| 0x2BEC | the new wave is null -> no `AddObject` | (a) the null test after `new` |

**fn_79_3578** (5 branches)

| branch | what | class |
|---|---|---|
| 0x35AC | the wind node does not cast to `CActor` -> the early return; the port's `if (!node) return;` | (c) the area has the wind node (IS08), so the taken path never comes; the body ran: `[arm] 5 3578 body` (and `[probe] fn_79_3578 body` x4 in the earlier runs) |
| 0x366C | the new `CExplosion` is null | (a) the null test after `new` |
| 0x372C | the string's `internal_dereference` is skipped when null | (a) an inlined `rstl::string` helper |
| 0x3740 | the string's destructor is skipped when null | (a) an inlined `rstl::string` helper |
| 0x3750 | the new explosion is null -> no `AddObject` | (a) the null test after `new` |

**fn_79_2E84** (12 branches)

| branch | what | class |
|---|---|---|
| 0x2ED0 | `mAttackType == 5` -> the cross (dive) walk; the port's `if (mAttackType == 5)` | (b) `[arm] 6 2E84 cross arm atk5` |
| 0x2ED4, 0x2EDC, 0x2EE8 | the switch's range and case tests that route every other attack type to the same plain path | (a) the compiler's lowering of the same two-way test; the plain path ran: `[arm] 9 2E84 plain arm atk3`, `[arm] 11 2E84 plain arm other` |
| 0x2FA4 | the node is the one nearest the player -> skip it; the port's `continue` | (b) `[arm] 7 2E84 skip nearest node` |
| 0x3004 | the boss's side of the player-to-centre line -> break out of the walk; the port's `break` | (b) `[arm] 8 2E84 break on the far side` |
| 0x3018 | the node walk's condition | (a) a loop condition |
| 0x309C | the connected object does not cast to `CActor` -> skip it | (a) the cast's null test (the port's `if (!node) continue;`) |
| 0x30A8 | `mAttackType != 5` -> no "too close" filter; the port's `mAttackType == 5 &&` | (b) the filter side ran: `[arm] 13 2E84 the dive filter applies` |
| 0x30EC | the candidate is closer to the node nearest the player than the threshold -> skip it | (b) `[arm] 14 2E84 too close skip` |
| 0x314C | the score is not better than the best so far -> skip it; the port's `else` | (b) `[arm] 16 2E84 score not better` (the other side: `[arm] 15 2E84 target updated`) |
| 0x317C | the candidate walk's condition | (a) a loop condition |

**fn_79_33BC** (2 branches)

| branch | what | class |
|---|---|---|
| 0x3410 | the entry's time is above zero -> keep it; the port's `if (it->mTime <= 0.f) erase; else ++it;` | (b) both sides ran: `[arm] 17 33BC erase` and `[arm] 18 33BC entry kept` |
| 0x3430 | the list's "is this last element?" test | (a) a loop condition |

**fn_79_3774** (the wing damage burst, 5 branches) - marked from this pass as well: the burst runs
when a growth dies, which the beam made happen four times in the run (`[arm] 20 3774 body`).

| branch | what | class |
|---|---|---|
| 0x37FC | the new `CExplosion` is null | (a) the null test after `new` |
| 0x38B0 | the string's `internal_dereference` is skipped when null | (a) an inlined `rstl::string` helper |
| 0x38C4 | the string's destructor is skipped when null | (a) an inlined `rstl::string` helper |
| 0x38D0, 0x38D4 | the new explosion is null -> no `AddObject` | (a) the null test after `new` |

The port's own index guard (`if (idx < 0 || idx >= ARRAY_SIZE(kGrowthLocators)) return;`) is not in
the retail, which trusts the growth slot; it is a port limit (the host indexes a C array) and never
fires.

### 7.10 fn_79_7E24 (the `Blow` state): what the asm says, and why it is not ported yet

The dark phase now reaches `Blow` (the driver's runs: `dark fsm Level -> Blow`), so the state's
body is finally reachable - but the port's `Blow` is a reconstruction, not retail's body, and
porting it needs the flag semantics settled first. From the retail asm (`auto_00_00000000_text.s`,
0x7E24..0x8238):

- **The message dispatch** (r5): 0 = activate, 1 = update, 2 = deactivate (the host's
  `ESteMsg` order: `kStateMsg_Activate`, `kStateMsg_Update`, `kStateMsg_Deactivate`).
- **Activate (0x7E74):** the animation state at +0x6b4 becomes 1 (`kAS_Ready`) and the flag
  word at +0xc74 loses **bits 25, 26 and 27** (the byte at 0xc77's bits 1, 2 and 3). The port
  instead clears `mBubbleCreated`, `mWindPending` and `mBlowing` (bits 30, 29, 28).
- **Update (0x7EA8):** `fn_801B73E4` (the retail's `CanIssueCommand`) with the command id 7
  (`kAS_MeleeAttack`); if it can issue, a `CBCMeleeAttackCmd` is delivered with the severity
  from `lbl_79_data_670` and the direction from the vector at +0x48c. Then the blow window:
  the player's aim point against the head (+0xaa0), the forward dot (+0x28/+0x38/+0x48) and the
  right dot against `rodata_10C * the scale at +0x60`, plus `mGrappleTime == 0` (+0xd8c). Inside
  the window the player is pushed (`PushSustainedDamage`, `DisableControls(0xe2)`, the impulse
  from +0x9a8/+0x9ac) and takes the blow damage (`mBlowDamageInfo` at +0xee4) every frame.
  The wind effect (`fn_79_3578`) is called when **bit 25** is set, and bit 25 is cleared
  afterwards; the sustained damage is popped when bit 27 goes back to 0.
- **Deactivate (0x820C):** the sustained damage is popped and bit 27 cleared.

**Why this is not ported in this slice:** the retail's `Blow` keys the wind effect and the
sustained-damage cleanup on **bits 25 and 27**, while the port's `Blow` uses `mBubbleCreated`
(bit 30) and `mWindPending` (bit 29) and its `DoUserAnimEvent` cases 0x29/0x2a/0x2b set those
two bits. The retail's anim-event handler (fn_79_A838) sets **bit 24** after it calls
`fn_79_1E40` (the bubble telegraph) and sets bit 27 in the same handler, so the port's flag
names do not line up with the retail's bits either (the port's `mBigHit` is the retail's bit 24,
the port's `mBubbleTelegraphed` is bit 31). Porting `Blow` faithfully therefore means first
re-deriving the flag bits 24..27 across `DoUserAnimEvent` (a marked function), `ProcessDamage`
and `Blow` - a separate slice, not a body swap. The port's `Blow` stays as it is (it is not
marked).

### 7.11 The flag bits 24..31 of the word at +0xc74 (2026-10-21, seventh lane pass)

The retail keeps eight flags in the byte at +0xc77 (the top byte of the flag word at +0xc74).
Every reader and writer of that byte in the module was read out of the asm and matched with the
port's users: `fn_79_3900` (the water splash), `fn_79_61C4` (`ProcessDamage`), `fn_79_7870`
(`Reel`), `fn_79_7E24` (`Blow`), `fn_79_91A4` (`BecomeDarkFlyer`), `fn_79_A838`
(`DoUserAnimEvent`) and the constructor `fn_79_BE3C`. The asm's `rlwimi`/`extrwi` bit fields
count from the MSB, so the instruction's field 24 is the *least* significant bit 31 and field 31
is bit 24 (the annotator prints this as `xc77_80` = the byte's bit 7 = the word's bit 31).

| word bit | the byte's bit | the retail's behaviour | the port's flag (before/after) | the users |
|---|---|---|---|---|
| 24 | 0x01 | never read or written after the constructor zeroes it | `mBigHit` -> `mUnusedC77Bit0` | none |
| 25 | 0x02 | set by `ProcessDamage` when a growth takes a hit (0x6400); the anim event 0x2d reads it to pick the flinch sound and clears it (0xAEDC) | `mReelEffect` (unchanged) | `ProcessDamage`, `DoUserAnimEvent` |
| 26 | 0x04 | the one-shot for the growth target effects: `Reel` sets it (0x7AA8) and its update skips turning them on again while it is set (0x7A64) | `mSuppressWaterSfx` -> `mTargetEffectsOn` | `Reel` |
| 27 | 0x08 | "the boss is in the dark flyer form": set by `BecomeDarkFlyer` (0x9298), cleared there (0x94AC); the water splash (fn_79_3900) reads it three times | `mBlowPushed` -> `mDarkFlyerForm` | `BecomeDarkFlyer`, `UpdateWater` |
| 28 | 0x10 | "the player is being blown": the `Blow` sets it when it pushes the player (0x806C) and clears it when the blow ends (0x81D4, 0x822C) | `mBlowing` (unchanged) | `Blow` |
| 29 | 0x20 | "the blow window is open": the anim event 0x2a sets it and 0x2b clears it; the `Blow`'s window, push and damage run only while it is set (0x7F74) | `mWindPending` -> `mBlowWindowOpen` | `DoUserAnimEvent`, `Blow` |
| 30 | 0x40 | "the wind effect is pending": the anim event sets it, the `Blow` consumes it (`fn_79_3578`) and clears it (0x8200) | `mBubbleCreated` -> `mWindEffectPending` | `DoUserAnimEvent`, `Blow` |
| 31 | 0x80 | "the bubble telegraph has been created": the anim events call `fn_79_1E40` once and set it (0xA94C); `BecomeDarkFlyer` clears it (0x926C) | `mBubbleTelegraphed` (unchanged) | `DoUserAnimEvent`, `BecomeDarkFlyer` |

**Result: every bit the port uses was already right** - no user had a wrong bit, so the only
change is the names, which were guesses. The port's flag names now say what the retail's bits
do, and the header's comments carry the bit numbers.

### 7.12 fn_79_7E24 (`Blow`): the body is retail's, and its branches (2026-10-21, seventh lane pass)

§7.10 read the retail's `Blow` out of the asm; comparing it with the port's body line by line showed
that the port's `Blow` **is** retail's body: the same three messages (activate, update,
deactivate), the same melee attack command (`pas::kAS_MeleeAttack` is the command id 7 and
`pas::kS_One` the severity 1 in the retail's command), the same blow window (the forward dot
against the head at +0xa9c, the right dot against `rodata_10C` = 18 model scales and
`mGrappleTime` = 0), the same push (`mBlowWindowOpen` picks the push member at +0x9ac over the
one at +0x9a8), the same damage and the same wind effect (the flag bit 30). The one thing the
host cannot express is the retail's melee command's target position: the host's
`CBCMeleeAttackCmd` only takes the severity, and the retail's command carries the body
controller's position with the "has target" flag clear, so the two behave the same.

The retail's conditional branches, with the once-per-arm probe's lines (the probe was removed
again before the commit):

| branch | what | class |
|---|---|---|
| 0x7E54, 0x7E60, 0x7E6C | the message dispatch: 1 = update, 0 = activate, 2 = deactivate, 3 and up = the end | (b) `[arm] 30 7E24 update`, `[arm] 44 7E24 activate`, `[arm] 45 7E24 deactivate` |
| 0x7EBC | the melee attack command cannot be issued yet | (b) both sides: `[arm] 31 7E24 melee command issued`, `[arm] 32 7E24 melee command skipped` |
| 0x7F74 | the blow window is closed (the flag bit 29) -> no window, no push and no damage | (b) both sides: `[arm] 33 7E24 blow block entered`, `[arm] 40 7E24 the blow block is closed` |
| 0x801C | the player's aim point is not in front of the head (the forward dot) | (b) the skip side: `[arm] 37 7E24 outside: the forward dot` |
| 0x8040 | the aim point is more than 18 model scales to the side (the right dot) | (b) the skip side: `[arm] 47 7E24 outside: the right dot` |
| 0x804C | the player is grappling (`mGrappleTime` at +0xd8c is not 0) | (d) not reached, and it is an **unported harness path**, not an unreachable arm: the console can give the grapple beam (`give grappleBeam 1` - the item is `CPlayerState::kIT_GrappleBeam` = 0x17, and `ShouldDrawGrapple` needs it) and can wake the area's grapple points (`send u066b Activate`; the console's message table has `Activate`), but with the lock-on on, the grapple beam owned and the player 8 units from the grapple point in front of the boss, the grapple point never becomes the player's orbit target and `mGrappleTime` stayed at 0.00 in every attempt |
| 0x8058 | the player is already being blown -> no new push setup | (b) the other side of the same bit: `[arm] 35 7E24 push setup` and `[arm] 38 7E24 the blow damage` (the bit is set in the frames that follow the first) |
| 0x80AC | the weak push (the member at +0x9a8 instead of +0x9ac) | (c) the block is only entered while the flag bit 29 is set (0x7F74) and the flag is read again here, so the weak value is dead code in the shipped flow |
| 0x8110 | the player is not being blown -> no blow damage | (b) both sides: `[arm] 38 7E24 the blow damage`, `[arm] 39 7E24 no blow damage` |
| 0x81B8, 0x81C0 | the sustained damage is popped when the blow ends | (b) the pop side: `[arm] 41 7E24 the sustained damage is popped` |
| 0x81E4 | the wind effect is waiting (the flag bit 30) -> fn_79_3578 | (b) both sides: `[arm] 42 7E24 the wind effect`, `[arm] 43 7E24 no wind effect` |
| 0x8214 | the deactivate pops the sustained damage | (d) not reached: the FSM2 evaluates a state's transitions **after** that state's update (the port's `CGenericFSM2State::Update`: `ExecuteState(update)` then `EvaluateTransitions`), so the update's cleanup (0x81B8) has already popped the sustained damage and cleared `mBlowing` whenever the blow window (the anim event 0x2b) closed first - and it did in every run (`[arm] 41`). The arm is only reachable if the state is left while the window is still open; the driver now sends the stun (a console `damage <body joint uid> 25`, 5% of the joint's 500 HP, which takes the head health to zero) the moment the probe shows the window open, but the run that did it never reached the `Blow` with the window open, so the arm is still unproven |

**`fn_79_7E24` is therefore not marked**: its body is retail's (every branch above is matched to
the port's code), but two real arms - "the player is grappling" (0x804C) and "the deactivate pops
the sustained damage" (0x8214) - did not run, and the rule is that a marker means every arm ran.
The arms that did run are in the table with their probe lines; the probe is gone again. The two
open arms need harness work, not more port work: 0x804C needs a grapple beam (the console has
none) and 0x8214 needs a transition that fires while the blow window is open.

### 7.13 The module's remaining unmarked functions, by call count (2026-10-21, seventh lane pass)

Counted from the retail asm (the `bl` call sites inside the module):

| fn | call sites | what |
|---|---|---|
| `fn_79_32C0` | 32 | the loader's `rstl::vector` free - a host copy (7.1) |
| `fn_79_1228`, `fn_79_178C`, `fn_79_1A64`, `fn_79_1AA0`, `fn_79_2D3C`, `fn_79_6974`, `fn_79_B7F8`, `fn_79_D0F8` | 8 each | the loader's own constructors/copies and the loader's string helper (7.1) |
| `fn_79_31C0` | 6 | the loader's `rstl::vector` copy (7.1) |
| `fn_79_460`, `fn_79_4958`, `fn_79_6AB0` | 6 each | loader/host copies (7.1) |
| `fn_79_50AC`, `fn_79_55B0` | 4 each | the platform-chain walk - **not ported** (the port's `LaunchPlatformChain` is an approximation) |
| `fn_79_7E24` | 3 | the `Blow` state - verified against the asm in 7.12, **not marked** because two of its arms did not run |

The next three behaviour functions to take are `fn_79_55B0` and `fn_79_50AC` (the chain walk, which
also needs `fn_79_5030`/`fn_79_527C`) and `fn_79_6B78` (the second half of
`SetupCollisionManager`, which the port merges into the marked `fn_79_6E68`).


### 7.14 fn_79_6B78: the per-joint setup, split out of `SetupCollisionManager` (2026-10-21, eighth lane pass)

The retail builds the collision actors in `fn_79_6E68` and then sets each joint up in
`fn_79_6B78`; the port did both in one body, so the second half is now its own
`SetupCollisionJoints` (marked). Its branches:

| branch | what | class |
|---|---|---|
| 0x6BE8 | the collision actor is gone -> the walk ends | (c) the manager creates all ten actors, so the shipped area never takes it (the port now ends the walk with `break`, as the retail does) |
| 0x6CF8 | the joint is not one of the four growths -> no orbit and seeker materials | (b) both sides: `[arm] 50 6B78 a growth joint gets the orbit and seeker materials` and `[arm] 51 6B78 a plain joint gets no materials` |
| 0x6D20 | the walk's condition | (a) a loop condition |

The body runs when the boss activates (`Activate` -> `SetupCollisionManager`), so every run that
reaches the fight exercises it.

### 7.15 The platform chain walk: fn_79_5030, fn_79_527C, fn_79_55B0, fn_79_50AC (2026-10-21, ninth lane pass)

The retail's chain walk is four bodies: `fn_79_5030` puts the chain nodes on their recorded
positions, `fn_79_527C` swings them about the chain's origin and pulls them back inside the edge
radius, `fn_79_55B0` wakes the chain and starts the walk, and `fn_79_50AC` is the recursion over
the chain's "ARRV/NEXT" connections (it also walks the "PLTR/FOLL" links). The port has the first
two (`PlaceChainNodes`, `SwingChainNodes`) and an approximation of the third
(`LaunchPlatformChain`); the recursion, `fn_79_50AC`, is not ported, so the chain walk stays
approximate.

**fn_79_5030** (the port's `PlaceChainNodes`)

| branch | what | class |
|---|---|---|
| 0x5078 | the chain node does not cast to `CActor` -> it is left alone | (c) the chain's nodes are actors, so the shipped area never takes it |
| 0x5094 | the walk's condition | (a) a loop condition |
| - | the body itself | (d) it never ran: the chain walk is launched by the anim event 0x1b (`kUE_EventStart`), and that event did not fire in any driver run, so `fn_79_5030` and `fn_79_527C` are unreachable in the port as it stands |

**fn_79_527C** (the port's `SwingChainNodes`)

| branch | what | class |
|---|---|---|
| 0x52D0 | the chain node does not cast to `CActor` -> it is left alone | (c) the chain's nodes are actors |
| 0x53B0 | the node is further from the centre than the edge radius -> it is pulled back | (d) not reached: the body never runs (see 5030) |
| 0x541C | the walk's condition | (a) a loop condition |
| - | the body itself | (d) it never ran (see 5030) |

**fn_79_55B0** (the port's `LaunchPlatformChain`): the retail's body is the same three steps (the
chain actor, the patrol node on the "FOLL" connection, the next node on the "ARRV/NEXT"
connection) but it also runs the `fn_79_50AC` recursion, which the port does not have, so
`fn_79_55B0` is **not** marked: the port's body is an approximation, not retail's. Its three
branches (0x55FC, 0x56F0, 0x5740: the chain actor, the patrol node and the next node are
gone) are the port's own null guards, all (c) - the chain's nodes are actors.

**fn_79_50AC**: not ported (the recursion over the chain's connections), not marked.

### 7.16 The chain walk ported: fn_79_5830 and fn_79_50AC (2026-10-21, tenth lane pass)

The retail's chain walk is two bodies the port did not have: `fn_79_5830` (the chain actor's
"PTRL/FOLW" node, then the walk) and `fn_79_50AC` (the recursion over the "ARRV/NEXT"
connections). Both are ported now (`CollectChainNodes`, `WalkChainNodes`) and the retail's
`fn_79_B3CC` call site is wired: the `kSM_AreaLoaded` case collects the light chain
(`mChainANodes`/`mChainAPositions`) and the dark chain (`mChainBNodes`/`mChainBPositions`),
which is what the retail's `fn_79_B3CC` does (0xB6F8 and 0xB73C, the "IS02" and "IS07"
connections).

Both are **not marked**: the bodies have not run (see the classes below).

**fn_79_5830** (`CollectChainNodes`)

| branch | what | class |
|---|---|---|
| 0x5878 | the chain actor does not cast to `CActor` -> the walk ends | (c) the chain is an actor |
| 0x58B8 | the "PTRL/FOLW" node does not cast to `CActor` -> the walk ends | (c) the chain's node is an actor |
| - | the body | (d) it did not run: the walk is driven by the boss's `kSM_AreaLoaded` message, and the harness creates the boss with the layer (so it never gets that message) - the console can send it (`send <boss uid> AreaLoaded`) but the run that did so sent it before the boss existed |

**fn_79_50AC** (`WalkChainNodes`)

| branch | what | class |
|---|---|---|
| 0x5224 | the "ARRV/NEXT" node does not cast to `CActor` -> it is skipped | (c) the chain's nodes are actors |
| 0x51F0 | the connection is the node's own -> it is skipped | (c) the chain's nodes do not link to themselves |
| 0x524C | the walk's condition | (a) a loop condition |
| - | the body | (d) it did not run (see 5830) |

The retail's `fn_79_55B0` (the anim event's chain launch: the chain actor, the "PTRL/FOLW" node,
`fn_79_5030` for the recorded positions, the "ARRV/NEXT" node, the heading difference, the
rotation and `fn_79_527C` for the swing) is what the port's `LaunchPlatformChain` does, so that
body is the port's `LaunchPlatformChain` - it is not marked, because the anim event 0x1b that
calls it never fires in a driver run (the chain walk is the anim event's job, not the script
message's) and the retail's 55B0 also does not run the recursion.

### 7.17 The chain walk, the anim event 0x1b and the grapple (2026-10-21, eleventh lane pass)

The driver is committed: `tools/drivers/swampboss2_chain.py` (the console part) and
`tools/drivers/swampboss2_run.sh` (it starts the port with `MP2_CONSOLE` and
`MP2_TICKS_PER_FRAME=8` and runs the driver). It waits for the boss in the `objs` list, sends it
the `kSM_AreaLoaded` message and forces the boss's AI states with `fsm <id> <state>`.

**The anim event 0x1b** (the chain launch) is carried by the **light phase's `Lurk` state's
animation**: a probe on the port's `DoUserAnimEvent` case 0x1b printed the state at the moment
the event fired - `[probe] the chain launch in the state Lurk` - and the chain launch then ran
(`fn_79_55B0` -> `fn_79_5030` -> `fn_79_527C`). It is not forced: `Lurk` is a sub-machine state,
so `fsm <id> Lurk` cannot enter it by name (the console says "state 'Lurk' not entered"), and the
launch happened in the state the boss reaches on its own.

**fn_79_5030** (the port's `PlaceChainNodes`) - **marked**

| branch | what | class |
|---|---|---|
| 0x5078 | the chain node does not cast to `CActor` -> it is left alone | (c) the chain's nodes are actors |
| 0x5094 | the walk's condition | (a) a loop condition |
| - | the body | (b) `[arm] 60 5030 a chain node is put on its recorded position` (the light phase's `Lurk`, no forcing) |

**fn_79_527C** (the port's `SwingChainNodes`) - **not marked** (the edge-radius arm 0x53B0 was not
reached: the swung node stayed inside the edge radius, so the driver would have to `place` a chain
node outside it - forced (place))

| branch | what | class |
|---|---|---|
| 0x52D0 | the chain node does not cast to `CActor` -> it is left alone | (c) the chain's nodes are actors |
| 0x53B0 | the node is further from the centre than the edge radius -> it is pulled back | (d) it did not run: the swung nodes stayed inside the edge radius |
| 0x541C | the walk's condition | (a) a loop condition |
| - | the body | (b) `[arm] 63 527C a chain node is swung` |

**fn_79_55B0** (the port's `LaunchPlatformChain`) - **marked**: the retail's body makes two
virtual calls on the chain actor (the vtable slot +0x18 = `CEntity::AcceptScriptMsg`, at 0x5654
and 0x56B0, each with a message id in a `CScriptMsg`; the second one's id is the "DECR" fourcc
built at 0x5668-0x5670, the first is the activation) - so the port's two script messages are the
retail's, not extra ones. The rest of the body matches step for step (the chain actor, the
"PTRL/FOLW" node, `fn_79_5030`, the "ARRV/NEXT" node, the two `atan2`s, the rotation,
`fn_79_527C`, the patrol's position), and it ran (it is what calls `fn_79_5030` and
`fn_79_527C`: `[arm] 60`, `[arm] 63`). Its three null guards are (c): the chain actor, the patrol
node and the next node are all actors in the shipped area.

**fn_79_5830 / fn_79_50AC** (the chain's node list and the ARRV/NEXT recursion) - **not marked**
(the arms are not proved yet), but the message *does* arrive: a temporary probe as the first line
of `CSwampBossStage2::AcceptScriptMsg` printed the message and the uid it went to, and a second
probe inside the case printed that the case ran:

    [probe] msg 58414c44 to u0673              (0x58414c44 = kSM_AreaLoaded)
    [probe] the kSM_AreaLoaded case ran on u0673

The engine delivers it itself at the area load (CScriptObjectLoaderHelper sends `kSM_AreaLoaded`
to every object it loads) and the console's `send <boss uid> AreaLoaded` (the driver does it once
the boss is in the `objs` list) delivers a second one - so the chain walk is reached without any
forcing. The per-arm probe for these two bodies could not be run in a build that picked the module
source up (the module object in `build-port-link/modules/` went stale), so the arms stay
unproved and the two functions stay unmarked. Their arms are otherwise (c): the chain actor, its
"PTRL/FOLW" node and the "ARRV/NEXT" nodes are all actors in the shipped area.

**The grapple (fn_79_7E24's 0x804C) is blocked by shared host code, not by the module**

- `src/MetroidPrime/ScriptObjects/CScriptGrapplePoint.cpp:68` - the grapple point only gets
  `kMT_Orbit` when it is activated; it never gets `kMT_Target`.
- `src/MetroidPrime/Player/CPlayerOrbit.cpp:1004` - the lock-on's near list is built with
  `CMaterialFilter::MakeInclude(CMaterialList(kMT_Target))`, so a grapple point is not in it.
- `src/MetroidPrime/Player/CPlayerOrbit.cpp:1070` - `ValidateAimTargetId` rejects a target without
  `kMT_Target`.
- `src/MetroidPrime/Player/CPlayerOrbit.cpp:1478,1492,1506` - `BeginGrapple` is reached from the
  *orbit* target, which is the same aim target, so the grapple needs the same material.
- Retail: the grapple points are `CScriptGrapplePoint`s that the arm's beam connects to
  (`fn_79_7E24`'s window test is on `mGrappleTime`, +0xd8c); the port's `CPlayer::BeginGrapple`
  (CPlayerOrbit.cpp:1361) is the same path.

With the grapple beam owned (`give grappleBeam 1` - `CPlayerState::kIT_GrappleBeam` = 0x17, and
`CPlayerState::ShouldDrawGrapple` needs it) and the point woken (`send u066b Activate`), the point
still never becomes the player's orbit target, so `mGrappleTime` stayed at 0.00 in every run.
The fix belongs in the shared `CScriptGrapplePoint`/`CPlayerOrbit` code, not here.
