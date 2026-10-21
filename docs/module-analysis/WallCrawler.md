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

The bits of byte 0x860, MSB first: `mAlignToFloor`, `mHasAlignSurface`, `mPlayerObstructed`,
`mDisableMove` (ctor arg), then `x860_27_` tumbling, `x860_28_` re-orienting, `x860_29_` kART_Six bend
playing, and `x860_30_` kART_Five bend playing. Bit 0x80 of byte 0x861 is `x861_24_` (bending
enabled), which the ctor sets to **true**. [proven]

For CWallWalker (f1..f6 = 1, 0.167, 0.6, 1.5, 0.6, 1.5) the roles are:

- `x848_`: touch-bounds scale.
- `x84c_`: floor-snap rate ×60. 0.167 gives ~10/s, the same as MP1.
- `x850_`/`x854_`: bend ramp up/down divisors while tumbling.
- `x858_`/`x85c_`: bend ramp up/down divisors while re-orienting.

## 3. Behaviour

- **PreThink.** The guard is active and not player-obstructed. With pause ≤ 0, not `mDisableMove`, and
  frozen ≈ 0 (|x| < 1e-5): if `mAlignToFloor`, the actor runs `AddMotionState(PredictMotion)` and
  `ClearForcesAndTorques`. With an align surface, it lerps toward `radius+0.01` above that surface by
  `t = 60·x84c_·dt`, then calls `MoveCollisionPrimitive(0)`. Otherwise, if the pause is > 0, it calls
  `Stop()`. The stand-in had no `Stop()` and hard-coded `10·dt`.
- **Think.**
  1. Material `kMT_Solid` follows `mMovable`: add it when movable, remove it when not.
  2. `CPatterned::Think`.
  3. The patrol pause counts down, clamped at 0.
  4. Only if the body controller **has `kAS_AdditiveReaction`**: clear `x860_27_`/`x860_28_` unless
     `x861_24_` is set. The weight ramps up while `(x27 && !x30) || (x28 && !x29)` and ramps down
     otherwise. The rate is `dt·|v| / divisor`, clamped to [0, 1]. While the weight is > 0 or a bend is
     playing: if the weight is > 1e-4, deliver `CBCAdditiveReactionCmd(kART_Six` or
     `kART_Five, w, true)` the first time and `CBCAdditiveWeightCmd(w)` after that. Otherwise deliver
     `kBSC_StopReaction`. The tumble side is chosen by `x27 || x29`.
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
- **AlignToFloor.**
  - The search box half-size is `radius+margin`, which is also the starting best distance.
  - The existing surface is re-validated with PointOnSurface. The actor searches again when it has no
    surface or on every 4th think.
  - With no constraint, distance is |plane height|.
  - With a constraint, triangles carrying `kMT_AIPassthrough` (bit 28) are skipped. `agree` is
    dot(triangle normal, constraint normal). Constraint 2 negates it and constraint 3 takes its
    absolute value. When `agree > 0.5`, the distance is multiplied by `1 + agree`. Retail does this as
    written: a better-aligned triangle scores a *larger* distance.
  - With a surface, the actor calls `AlignToPlane(normal, mAlignAngVel·dt)`, sets tumble angle 0 and
    `x27 = 0`, and sets `x28 = dot(up, n) < 0.86`.
  - Without a surface, the actor tumbles: `ω = 57.2958·|v|/radius` (degrees/s), and it calls
    `AlignToPlane(±forward` (the sign follows velocity)`, ω·dt)`, sets `x27 = 1` and `x28 = 0`, and
    adds `ω·dt` to the tumble angle.

  The stand-in mixed radians into the degree API and never set `x27`/`x28`.
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
    (`x27=0`), tumbling (`x27=1`, tumble angle growing 6–42°) and re-orienting (`x28=1`). The default
    plane was (0,1,0,0).
  - Their body controllers have no `kAS_AdditiveReaction` state, so the bend branch is correctly
    skipped.
- Runtime: Agon Wastes (`42b935e4`) `0b_sand_hall` (area 40, MREA `84303dd5`, picked from
  `docs/module_spawns.tsv`), 6000 frames, GODMODE: no `[port] caught` and no `[reach-stub]`. A
  temporary probe saw the `EType 5` crawlers (PillBug) *with* `kAS_AdditiveReaction`: the weight
  ramped and the kART_Six bend started (`x860_29_` = 1, w = 0.07).
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
- **The player-obstruction scan is ported (2026-07-20, branch `decomp/wallcrawler2`):** the retail's
  `fn_83_1310` (AlignToFloor) runs two things - the floor alignment and a **player-obstruction
  scan**, and the port now has both, so `fn_83_1310` is marked again (the scan's details are in the
  next bullet). A temporary once-log (removed before the commit) showed the flag going up in the
  Agon Wastes `0b_sand_hall` run; the Temple Grounds run never raises it, because the scan only runs
  when the constraint (0x834) is set (asm 0x1454-0x145C) and no crawler there has one.
- **What the scan does (the retail's asm 0x1410-0x17A4):** the obstruction a standing crawler keeps
  is re-tested every frame against the new position (PointOnSurface on the module's own copy at
  0x7c0, asm 0x1420-0x1438); with a constraint set, the cache's triangles are scanned: a triangle
  whose flags carry bit 12 (the mask 0x1000) is skipped, the triangle's plane gives the distance
  (|dot(normal, position) - d| - the host's `plane.GetHeight(position)`), the agreement with the
  constraint plane starts at 1.0 (`rodata_14`) and grows by the agreement while it is above 0.5
  (`rodata_24`), and the score is that accumulated agreement times the distance. The best surface
  the crawler stands on becomes the module's own copy and sets `mPlayerObstructed` (the byte 0x860's
  bit 6 - `extrwi` 1, 25 of the word). The retail's remaining `fn_83_1310` difference is the
  obstruction branch that re-searches the floor on every fourth think (asm 0x1448-0x1450).
- **Found by asmcheck, not yet ported:** the retail's `fn_83_1310` (AlignToFloor) also runs a
  **player-obstruction scan** the port does not have (asm 0x15C8-0x179C): it walks the collision
  cache's leaves and their triangles, keeps the surface that faces the player (the plane test at
  0x1610-0x1630), measures the player-to-surface distance (0x16C0-0x16E4), compares it with
  `rodata_24` and, when the point is on that surface (0x16F8-0x1700), stores the surface into the
  module's own copy (0x7c0) and sets `mPlayerObstructed` (the byte 0x860's bit 6, asm 0x179C-0x17A4).
  The port's `mPlayerObstructed` is therefore never set, so `PreThink`'s guard never blocks - the
  next slice's target. The `// @ fn_83_1310` marker came **off** in the meantime (commit
  "WallCrawler: drop the fn_83_1310 marker until the player-obstruction scan is ported"): the marker
  claimed the whole retail function, and the port still has only its floor half.
