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
  (0x2530) makes the "playerBehind" flag, and that flag goes straight to fn_79_5C74: it activates
  each live growth only while the player is behind the boss and deactivates all of them when it
  is clear. An inactive collision actor is neither moved by `CCollisionActorManager::Update` nor
  in the object space the beam collides against, so while the player stands in front the growth
  spheres sit still and cannot be shot; only the six body spheres are reachable. (The port's code
  was right all along; only its comment said "in front" - it is fixed now.)
- **How to hit them:** the player must be *behind* the boss at the moment `Reel` calls
  fn_79_24E0, i.e. on the opposite side from the boss's facing - the boss faces east in this
  area, so the player has to be west of it - and level with a growth. Once the growths are
  active their spheres sit on their joints, so the aim is the joint position logged above.
