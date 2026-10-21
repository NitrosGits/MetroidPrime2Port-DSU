# WallCrawler REL (module 83): behaviour of `CWallCrawler`

Analysis + host body, 2026-10-19. The source is the archive build's disassembly
`../MetroidPrime2Port/build/G2ME01/WallCrawler/asm/auto_00_00000000_text.s` (complete `.text`, 0x28A0
bytes), plus `auto_03` (rodata) and `auto_04` (data, vtables). DOL call targets were named from
`config/G2ME01/symbols.txt`. REA's Ghidra lane rejects relocatable PPC objects (`ET_EXEC only`), so the
module was read by hand. The goal is behaviour, not matching code. The implementation is
`src/MetroidPrime/Enemies/PortCWallCrawler.cpp`.

Tags: **[proven]** means read from an instruction or a table entry. **[inferred]** means a strong reading
that is not pinned to a symbol. All `fn_83_X` names are REL `.text` offsets.

**Shape in one line:** MP1's `CWallWalker` with three additions. It has a waypoint *constraint plane*
(`x834_`/`x838_`). Its `Think` drives an additive-reaction "bend" and does its own contact and
boost-ball damage. MP1's `BendingAnimationHack` is gone.

## 1. Function map

| fn | size | role | evidence |
|---|---|---|---|
| `fn_83_2374` | 0x310 | constructor (section 2) | [proven] stores every member |
| `fn_83_208C` / `20FC` | 0x70 / 0x1E4 | dtor / `~CPatterned` chain | [proven] vtable slot 0 |
| `fn_83_1098` | 0x204 | **PreThink** | [proven] slot, calls `CPatterned::PreThink` |
| `fn_83_580` | 0x828 | **Think** | [proven] slot, calls `CPatterned::Think` |
| `fn_83_1FD0` | 0xBC | **AcceptScriptMsg** | [proven] slot, calls `CPatterned::AcceptScriptMsg` |
| `fn_83_EE4` | 0x20 | Render = `CPatterned::Render` | [proven] |
| `fn_83_4C8` | 0xB8 | **GetTouchBounds** | [proven] |
| `fn_83_1308` | 0x8 | GetCollisionPrimitive = `&mColSphere` (0x7f0) | [proven] |
| `fn_83_D8` | 0x3C | GetNextWaypoint (slot 0x148): `wp->NextWaypoint(mgr)`, `reverse` unused | [proven] |
| `fn_83_114` / `140` | 0x2C / 0x8 | SetConstraint (store, then `UpdateConstraintPlane` if non-zero) / GetConstraint | [proven] |
| `fn_83_148` | 0x344 | **UpdateConstraintPlane** | [proven] |
| `fn_83_D0` | 0x8 | GetConstraintPlane = `this+0x838` | [proven] |
| `fn_83_F04` | 0x194 | **UpdateWPDestination** | [proven] |
| `fn_83_1310` | 0x6B8 | **AlignToFloor** | [proven] |
| `fn_83_1B50` | 0x164 | **AlignToPlane(normal, degrees)** | [proven] |
| `fn_83_1CB4` | 0x1C4 | PointOnSurface | [proven] |
| `fn_83_1E78`..`1F7C` | small | cross product, closest point on plane, vector projection | [proven] |
| `fn_83_129C`, `19C8` | | `CMotionState` and `CCollisionSurface` copies | [proven] |
| `fn_83_0`..`9C` | ≤0x2C | weak copies of `CPatterned`/`CAi` inline virtuals (for example `skDamageHitTime` into 0x448) | [proven] bodies match the header inlines |

The REL has **no loader**. The WallWalker loader is in the WallWalker REL (`CWallWalker.cpp`), so the
old stand-in's null `LoadWallWalker` was dropped.

## 2. Members (ctor `fn_83_2374`)

0x810 margin, 0x814 `mAlignAngVel` (**degrees**/s), 0x818 tumble angle, 0x81c patrol pause, 0x820
advance-wp radius, 0x824 obstruction dist, 0x828 bend weight, 0x82c `EType`, 0x830 think counter, and
0x834 constraint (0). 0x838 is the constraint `CPlane`: normal **(0,1,0)** at 0x838 (`x838_`), constant
0 at 0x844 (`x844_`). Then f1..f6 go to 0x848..0x85c (`x848_`..`x85c_`). In the old stand-in, f1..f6
were shifted one slot down, onto `x844_`..`x858_`.

The flags live in byte 0x860 (the word's high byte) and byte 0x861. A bit N of a word is
`1u << (31 - N)`, so the masks are (all **[proven]**, read from the ctor's `rlwimi` sequence
asm 0x258C-0x25F0 and every read of the byte):

| mask | member | who writes it |
|---|---|---|
| 0x860 `0x80` | `mAlignToFloor` | never set in this module (ctor clears it) |
| 0x860 `0x40` | `mHasAlignSurface` | AlignToFloor (asm 0x1434, 0x17A0) |
| 0x860 `0x20` | `mPlayerObstructed` | never set in this module (only PreThink reads it, asm 0x10CC) |
| 0x860 `0x10` | `mDisableMove` | the ctor's argument (asm 0x25B0) |
| 0x860 `0x08` | `x860_28_` tumbling | AlignToFloor (asm 0x180C clears, 0x1918 sets); Think clears it unless bending is on (asm 0x64C) |
| 0x860 `0x04` | `x860_29_` re-orienting | AlignToFloor (asm 0x1850 sets, 0x1928 clears); Think clears it unless bending is on (asm 0x678) |
| 0x860 `0x02` | `x860_30_` the kART_Six bend plays | Think (asm 0x8F0 sets, 0x93C clears) |
| 0x860 `0x01` | `x860_31_` the kART_Five bend plays | Think (asm 0xA4C sets, 0xA98 clears) |
| 0x861 `0x80` | `x861_24_` bending enabled | the ctor sets it (asm 0x25EC); nothing clears it |

So `mAlignToFloor` and `mPlayerObstructed` are **always false** in the retail: PreThink leaves
early at the `mAlignToFloor` test (asm 0x1124) and the `mPlayerObstructed` test (asm 0x10CC)
never fires. The port keeps both flags (the retail's layout) but must not set them either.

For CWallWalker (f1..f6 = 1, 0.167, 0.6, 1.5, 0.6, 1.5) the roles are:

- `x848_`: touch-bounds scale.
- `x84c_`: floor-snap rate ×60. 0.167 gives ~10/s, the same as MP1.
- `x850_`/`x854_`: bend ramp up/down divisors while tumbling.
- `x858_`/`x85c_`: bend ramp up/down divisors while re-orienting.

## 3. Behaviour

- **PreThink.** The guard is active and not player-obstructed (the 0x20 mask, which the module
  never sets). With pause ≤ 0, not `mDisableMove`, and
  frozen ≈ 0 (|x| < 1e-5): if `mAlignToFloor` (the 0x80 mask, which the module never sets either),
  the actor runs `AddMotionState(PredictMotion)` and
  `ClearForcesAndTorques`. With an align surface, it lerps toward `radius+0.01` above that surface by
  `t = 60·x84c_·dt`, then calls `MoveCollisionPrimitive(0)`. Otherwise, if the pause is > 0, it calls
  `Stop()`. The stand-in had no `Stop()` and hard-coded `10·dt`.
- **Think.**
  1. Material `kMT_Solid` follows `mMovable`: add it when movable, remove it when not.
  2. `CPatterned::Think`.
  3. The patrol pause counts down, clamped at 0.
  4. Only if the body controller **has `kAS_AdditiveReaction`**: clear the tumbling flag (0x08) and
     the re-orienting flag (0x04) unless `x861_24_` (bending enabled) is set (asm 0x628-0x67C). The
     weight ramps up while `(x860_28_ && !x860_31_) || (x860_29_ && !x860_30_)` and ramps down
     otherwise. The rate is `dt·|v| / divisor`, clamped to [0, 1]; the divisor is `x850_` when the
     tumbling or the kART_Six flag is set and `x858_` otherwise (asm 0x6C8-0x700). While the weight is
     > 0 or a bend is playing: if the weight is > 1e-4, deliver `CBCAdditiveReactionCmd(kART_Six` or
     `kART_Five, w, true)` the first time and `CBCAdditiveWeightCmd(w)` after that. Otherwise deliver
     `kBSC_StopReaction`. The tumble side is chosen by `x860_28_ || x860_30_`.
  5. Damage, if `mAlive` and the player's and this actor's touch bounds overlap. When the player
     **boost-balls**, `ApplyDamage(player → this, owner player, gpTweakBall->GetBoostBallDamage())`,
     and the contact timer is not touched. Otherwise, once `mCurDamageRemTime ≤ 0`,
     `ApplyDamage(this → player, GetContactDamage()` with only `mNoImmunity` set`)`, and the timer is
     reset to `mDamageWaitTime`. Both use the filter IncludeExclude(`kMT_Solid`), from `data_0` = 0x3B
     with type 3.

  None of the Think behaviour was in the stand-in. It had a made-up weight ramp keyed on
  `mTumbleAngle` and no damage at all.
- **AcceptScriptMsg.** On `kSM_Create` it adds `kMT_Immovable`, removes `kMT_Solid` and adds
  `kMT_NonSolidDamageable`, then calls `CPatterned::AcceptScriptMsg`. The stand-in only forwarded the
  message.
- **GetTouchBounds** returns `GetBaseBoundingBox().GetTransformedAABox(xf · Scale(x848_))`. `xf` comes
  from the vtable slot at 0x80; that this slot is `GetPrimitiveTransform` is **[inferred]**. The stand-in
  used `CPatterned`'s bounds.
- **UpdateConstraintPlane.** Start at `mDestObj`, or at `GetConnectedObject(kSS_Patrol, kSM_Follow)`
  when that is invalid. Walk up to 30 waypoints through `GetNextWaypoint(…, false)`, collecting
  positions. A third point is skipped when it is collinear with the first two (|dot of the normalized
  `p0-p1` and `p-p1`| > 0.984). With 3 points the plane is `CPlane(p0, p1, p2)`. With 2 points whose X
  and Y both differ by ≥ 1e-5, the plane is `CPlane(p0, p1, p1 + (0,0,1))`. Otherwise the old plane
  stays. The stand-in called `AlignToFloor` instead.
- **UpdateWPDestination.** Within the advance radius, `mDestObj = GetNextWaypoint(…, reverse=true)`. If
  the waypoint is a `CScriptAIWaypoint` with a non-zero pause, that pause becomes the patrol pause, and
  `EType` 0 also sets `kLT_Relaxed` locomotion. Then the waypoint receives `kSM_Arrived`. `SetDestPos`
  runs whether or not the actor advanced. The stand-in had no pause.
- **AlignToFloor** (fn_83_1310).
  - The search box half-size is `radius + margin` (asm 0x1390-0x13D4), but the **best distance
    starts at the radius itself** (f24 = the radius argument, asm 0x1390), not at radius+margin.
  - The standing surface is re-validated every frame: `mHasAlignSurface = PointOnSurface(mAlignSurface,
    newPos)` (asm 0x1418-0x1438). The search then runs only when there is no surface, or on every
    fourth think (`mThinkCounter & 3`, asm 0x1440-0x1450).
  - With no constraint (x834 == 0), the distance of a triangle is `|dot(normal, newPos) - d|`
    (asm 0x1460-0x14F0).
  - With a constraint, triangles carrying `kMT_AIPassthrough` (bit 28 of the 64-bit surface flags,
    asm 0x1610-0x1630) are skipped. `agree` is dot(triangle normal, constraint normal); constraint 2
    negates it, constraint 3 takes its absolute value, and constraints >= 4 leave it alone
    (asm 0x1674-0x16AC). The distance is `|plane height|`, scaled by `1 + agree` when
    `agree > 0.5` (asm 0x16B0-0x16E4) - retail does this as written: a better-aligned triangle
    scores a *larger* distance.
  - With a surface, the actor calls `AlignToPlane(normal, mAlignAngVel·dt)`, sets tumble angle 0,
    clears the tumbling flag (0x08) and sets the re-orienting flag (0x04) to
    `dot(up, n) < 0.86` (asm 0x17AC-0x1854).
  - Without a surface, the actor tumbles: `ω = 57.2958·|v|/radius` (degrees/s), and it calls
    `AlignToPlane(±forward` (the sign follows velocity)`, ω·dt)`, sets the tumbling flag (0x08),
    clears the re-orienting flag (0x04) and adds `ω·dt` to the tumble angle (asm 0x185C-0x1940).
  - There is **no player-obstruction scan** in this function: the retail never sets the 0x20 mask.
    (An earlier reading of asm 0x15C8-0x179C as such was wrong: it is the constrained variant of
    the same floor search, and it stores the surface into the module's own copy at 0x7c0 and sets
    the 0x40 mask, i.e. `mHasAlignSurface`.)

  The stand-in mixed radians into the degree API and never set the tumbling/re-orienting flags.
- **AlignToPlane** does nothing when |dot(up, n) − 1| < 1e-5 or dot < −0.999. Otherwise it applies
  `ShortestRotationArcClamped(up, n, deg·π/180)` in local space, normalizes the result, and calls
  `SetTransform` at the current position. Retail calls `SetTransformAlt`; the port has only
  `SetRotation`/`SetTransform`.

## 4. Port-only (`TARGET_PC`) changes

- `CBCAdditiveWeightCmd(float)` and `CBodyStateCmdMgr::DeliverCmd(const CBCAdditiveWeightCmd&)` were
  added. Retail builds the command inline and stores its weight into the manager's slot at
  0x358/0x35c. `CABSReaction` reads that slot.
- `mConstraintPlane` (an existing PC member) is now a copy of the retail plane at 0x838, kept for
  `GetConstraintPlane`'s reference.

## 5. Verification (2026-10-19)

- Host `build-port-link` built with 0 FAILED, and `gen_files_cmake.py --check` returned rc 0.
- The upstream matching build is unchanged at 94.14% fuzzy / 63.29% matched.
- Runtime: the `u_gm_0` card, Temple Grounds (`3bfa3eff`), `TELEPORT=10`, 6000 frames, GODMODE. The log
  has no `[port] caught` and no `[reach-stub]`.
  - A temporary probe, since removed, saw three `EType 6` crawlers. They cycled between surface
    (the tumbling flag 0x08 clear), tumbling (0x08 set, tumble angle growing 6–42°) and
    re-orienting (0x04 set). The default plane was (0,1,0,0).
  - Their body controllers have no `kAS_AdditiveReaction` state, so the bend branch is correctly
    skipped.
- Runtime: Agon Wastes (`42b935e4`) `0b_sand_hall` (area 40, MREA `84303dd5`, picked from
  `docs/module_spawns.tsv`), 6000 frames, GODMODE: no `[port] caught` and no `[reach-stub]`. A
  temporary probe saw the `EType 5` crawlers (PillBug) *with* `kAS_AdditiveReaction`: the weight
  ramped and the kART_Six bend started (`x860_30_` = 1, w = 0.07).
- Not exercised: the kART_Five bend, the boost-ball and contact damage (no player input in these
  runs).
- **2026-07-20, branch `decomp/wallcrawler2`:** the module has **no stand-in left** in the port - the
  old stand-in's null `LoadWallWalker` and its guessed bodies went with the behaviour port, and
  every function the port implements already carries a `// @ fn_83_<off>` marker (22 of them). The
  remaining module functions are the base class's weak inline copies (`0`, `10`, `18`, `20`, `28`,
  `30`, `38`, `48`, `54`, `60`, `68`, `70`, `78`, `80`, `9C` - 8 bytes each), the optional members'
  destructors (`48C`, `DA8`, `DE4`, `E40`, `E9C`, `2684`, `1A24`, `1A7C`, `1ACC`, `22E0`), the
  `~CPatterned` chain (`20FC`, 0x1E4) and `GetConstraint` (`140`, 0x8, an accessor nothing in the
  module calls). The **marking pass** added the three copy constructors the host's C++ performs
  implicitly: `129C` (0x6C, the CMotionState copy before `AddMotionState`), `19C8` (0x5C, the
  CCollisionSurface copy in AlignToFloor) and `26CC` (0x54, the CCollisionSurface's constructor in
  the module's ctor) - 25 marked in all. A Temple Grounds run (3bfa3eff, TELEPORT=10, 6000 frames,
  GODMODE) is clean: 0 `[port] caught` and no `[reach-stub]`.
- **The "player-obstruction scan" was a misreading (2026-10-21, branch `decomp/wallcrawler`):** the
  bullets above about a scan at asm 0x15C8-0x179C that sets `mPlayerObstructed` were wrong. The
  region is the **constrained variant of the floor search**: it walks the same cache triangles,
  skips the ones whose flags carry bit 28 (`kMT_AIPassthrough`, the high word's 0x1000 at asm
  0x1610-0x1630), measures the triangle's plane height (0x16C0-0x16E4), scores it by the
  agreement with the constraint plane, and stores the surface into the module's own copy at 0x7c0
  while setting the 0x40 mask - `mHasAlignSurface`, not `mPlayerObstructed`. The retail never sets
  the 0x20 mask anywhere in the module, so `mPlayerObstructed` is always false and PreThink's
  guard never blocks. The port's extra "obstruction scan" block (added in 036ca13db) was removed in
  this slice; it could set `mPlayerObstructed` and freeze a crawler that the retail keeps moving.

## 6. The functions that carry no marker (2026-10-21)

57 functions live in the REL's `.text`; 25 carry a `// @ fn_83_<off>` marker. The other 32 are
the REL's copies of host/DOL code, so they are cited here and neither reimplemented nor marked:

| fn | what it is |
|---|---|
| `0`, `10`, `18`, `20`, `28`, `30`, `38`, `48`, `54`, `60`, `68`, `70`, `78`, `80`, `9C` | the REL's weak copies of `CPatterned`/`CAi` inline virtuals (they sit in the module's vtable at 0xa4-0x124; the host's `CPatterned` provides the behaviour) |
| `140` | `GetConstraint` - an 8-byte accessor for the constraint word (0x834) that nothing in the module calls |
| `48C`, `DA8`, `DE4`, `E40`, `E9C`, `1A24`, `1A7C`, `1ACC`, `22E0`, `2684` | the optional members' destructors (rstl) |
| `20FC` | the `~CPatterned` chain (0x1E4) |
| `2720`, `27E4`, `2808`, `282C`, `2878` | the REL runtime (`_unresolved`, `_epilog`, `_prolog`, the module's constructors/destructors) |

`fn_83_9C` is the one of the weak copies that is not a plain accessor: it is the vtable slot at
0x3c and it forwards to the slot at 0x38 (a DOL function). The host's `CPatterned` covers it.

## 7. This slice (2026-10-21, branch `decomp/wallcrawler`)

- **The flag layout is now the retail's.** The header's bitfields carried the wrong masks (they
  were one bit off): the retail's byte 0x860 is `mAlignToFloor` 0x80, `mHasAlignSurface` 0x40,
  `mPlayerObstructed` 0x20, `mDisableMove` 0x10, then the tumbling flag 0x08, the re-orienting
  flag 0x04, the kART_Six flag 0x02 and the kART_Five flag 0x01; byte 0x861's 0x80 is
  `x861_24_` (bending enabled). The four unnamed members were renamed `x860_28_`..`x860_31_`
  and every bitfield now carries its retail mask as a comment, so `tools/asmcheck.py check`
  reports no MASK hit for the module any more.
- **AlignToFloor had two behaviour differences**, both fixed:
  1. the best distance started at `radius + margin`; the retail starts it at the **radius**
     (f24 = the radius argument, asm 0x1390) and only the search box is radius+margin wide.
  2. the port ran an extra "player-obstruction scan" that the retail does not have (see the
     bullet above); it could set `mPlayerObstructed` and stop the crawler.
- **Not exercised by these runs:** the constrained variant of the floor search (no crawler in
  Temple Grounds area 10 or Agon Wastes area 40 has a constraint, so x834 == 0), the kART_Five
  bend, and the boost-ball and contact damage (no player input). The `// @ fn_83_1310` marker
  therefore rests on the unconstrained arms alone.
- The base class is shared with `CPillBug`, `CKralee`, `COctapedeSegment` and `CParasite`, which
  call this `AlignToFloor`; the tighter best distance and the removed `mPlayerObstructed` write
  change what they inherit. Those sessions should re-check their own runs (their RELs have their
  own floor-alignment functions).

