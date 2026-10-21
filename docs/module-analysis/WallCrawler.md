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

## 6. The functions that carry no marker

57 functions live in the REL's `.text`; 25 carry a `// @ fn_83_<off>` marker. The other 32 are
**31 host copies and 2 dead leaves** - the classification is section 7, and **no module-behaviour
function is left unmarked**: every one of the 24 B functions carries its marker.

## g. Function classification (all 57 symbols)

Three classes, applied by rule so the table can be audited:

- **H - host copy** (31 functions): the body is a copy of code the host already has, so the port
  writes no body for it. Three sources: the REL's weak copies of the host's `CPatterned`/`CAi`
  inline virtuals (15, all in the module's vtable), the compiler-emitted destructors (11) and the
  crtexe REL runtime (5).
- **T - dead leaf** (2 functions): the body has no retail caller. The scan is the module's own
  `.text`: no `bl` reaches the function, it is in no vtable and in no data table, and its symbol is
  local (`scope` is not `global` in `config/G2ME01/rels/WallCrawler/symbols.txt`), so no other
  module can reach it either.
- **B - module behaviour** (24 functions): real behaviour - these are the port's bodies. The last
  column says whether the body carries a `// @ fn_83_` marker.

| class | functions | bytes |
|---|---|---|
| H host copy | 31 | 0x7D0 (2,000) |
| T dead leaf | 2 | 0x5C (92) |
| B module behaviour | 24 | 0x2098 (8,344) |
| **total** | **57** | **0x28C4 (10,436)** |

The 57 bodies cover 0x28C4 bytes with the padding between them (the module's text section is 0x2720
bytes and the REL setup 0x1A4; the symbols' sizes sum to 0x28A0).

### g.1 H - host copy (31)

| id | size | what it is | the host's counterpart |
|---|---|---|---|
| `fn_83_0` | 0x10 | the vtable slot 0xA4: the host's `skDamageHitTime` into 0x448 | `CPatterned`'s inline virtual |
| `fn_83_10` | 0x8 | the vtable slot 0xC8: the byte at 0x44F | `CPatterned`'s inline virtual |
| `fn_83_18` | 0x8 | the vtable slot 0xDC: false | `CPatterned`'s inline virtual |
| `fn_83_20` | 0x8 | the vtable slot 0xE0: false | `CPatterned`'s inline virtual |
| `fn_83_28` | 0x8 | the vtable slot 0xE4: false | `CPatterned`'s inline virtual |
| `fn_83_30` | 0x8 | the vtable slot 0xF4: false | `CPatterned`'s inline virtual |
| `fn_83_38` | 0x10 | the vtable slot 0x10C: `GetAttackTarget` -> `kInvalidUniqueId` | `CPatterned`'s inline virtual |
| `fn_83_48` | 0xC | the vtable slot 0x110: `IsOnGround` (the bit 1 of the byte at 0x34C) | `CPatterned`'s inline virtual |
| `fn_83_54` | 0xC | the vtable slot 0x114: `GetGravityConstant` -> `kDefaultGravityAccel` | `CPatterned`'s inline virtual |
| `fn_83_60` | 0x8 | the vtable slot 0x124: -> `this+0x754` | `CPatterned`'s inline virtual |
| `fn_83_68` | 0x8 | the vtable slot 0xA8: true | `CPatterned`'s inline virtual |
| `fn_83_70` | 0x8 | the vtable slot 0xAC: false | `CPatterned`'s inline virtual |
| `fn_83_78` | 0x8 | the vtable slot 0xB0: false | `CPatterned`'s inline virtual |
| `fn_83_80` | 0x1C | the vtable slot 0xB4: copies three floats from the argument's 0x54..0x5C | `CPatterned`'s inline virtual |
| `fn_83_9C` | 0x2C | the vtable slot 0x3C: forwards to the vtable slot 0x38 | `CPatterned`'s inline virtual |
| `fn_83_48C` | 0x3C | the compiler's destructor: `CMemory::Free` of an rstl string | rstl |
| `fn_83_DA8` | 0x3C | the same destructor for the other optional member | rstl |
| `fn_83_DE4` | 0x5C | the destructor of the class whose vtable is `data_188` | the host class |
| `fn_83_E40` | 0x5C | the destructor of the class whose vtable is `data_194` | the host class |
| `fn_83_E9C` | 0x48 | the destructor of the class whose vtable is `data_1A0` | the host class |
| `fn_83_1A24` | 0x58 | `~CAreaCollisionCache` (called at asm 0x1944, the local of `AlignToFloor`) | the host class |
| `fn_83_1A7C` | 0x50 | the destructor of the cache's member at +0x18 | the host class |
| `fn_83_1ACC` | 0x84 | the destructor of the cache's leaf array (the 0x910 stride) | the host class |
| `fn_83_20FC` | 0x1E4 | `~CPatterned` (`__vt__10CPatterned`, `__dt__6CTokenFv`, `__dt__3CAiFv`, ...) | `CPatterned` |
| `fn_83_22E0` | 0x94 | `~CAiKnockBackMgr` (`__vt__15CAiKnockBackMgr`) | `CAiKnockBackMgr` |
| `fn_83_2684` | 0x48 | the destructor of the class whose vtable is `data_158` | the host class |
| `_unresolved` | 0xC4 | the REL runtime's unresolved-symbol trap | crtexe |
| `_epilog` | 0x24 | the REL runtime's module exit | crtexe |
| `_prolog` | 0x24 | the REL runtime's module entry | crtexe |
| `ModuleDestructors` | 0x4C | the REL runtime's static destructor table | crtexe |
| `ModuleConstructors` | 0x4C | the REL runtime's static constructor table | crtexe |

### g.2 T - dead leaf (2)

| id | size | what it is | the scan | port |
|---|---|---|---|---|
| `fn_83_140` | 0x8 | `GetConstraint`: returns the word at 0x834 | no `bl` to 0x140 in the module's `.text`, not in `data_C` or any other table, and the symbol is local | the port has the accessor; nothing calls it |
| `fn_83_1F7C` | 0x54 | `GetClosestPointOnPlane`: the host's `CPlane::GetClosestPoint` inlined | the same scan: no `bl` to 0x1F7C, and the symbol is local | the port has it and it carries a marker (it is a host method, so the port keeps it for its callers) |

The scan is the module's own `.text`: every `bl` target was collected per function, and the data
section (`auto_04_00000000_data.s`) was read for the vtable and the other tables. The REL exports
nothing (no `SetFuncPtrs`), so a local symbol has no caller outside the module.

### g.3 B - module behaviour (24)

`port` column: **marked** = the body carries a `// @ fn_83_` marker. All 24 do, so the list of the
unmarked B functions is **empty**.

| id | size | what it is | port |
|---|---|---|---|
| `fn_83_C8` (`RELMain`) | 0x4 | the module's entry point | marked |
| `fn_83_CC` (`RELExit`) | 0x4 | the module's exit point | marked |
| `GetConstraintPlane__12CWallCrawlerCFv` | 0x8 | the constraint plane at 0x838 | marked |
| `GetNextWaypoint__12CWallCrawlerF...` | 0x3C | the slot 0x148: the waypoint's `NextWaypoint` | marked |
| `SetConstraint__12CWallCrawlerF...` | 0x2C | the store at 0x834 and the plane rebuild | marked |
| `UpdateConstraintPlane__12CWallCrawlerF...` | 0x344 | the constraint plane from three waypoints | marked |
| `GetTouchBounds__12CWallCrawlerCFv` | 0xB8 | the base box transformed by the primitive transform | marked |
| `Think__12CWallCrawlerFfR13CStateManager` | 0x828 | the material, the bend and the contact damage | marked |
| `Render__12CWallCrawlerCFRC13CStateManager` | 0x20 | `CPatterned::Render` | marked |
| `UpdateWPDestination__12CWallCrawlerF...` | 0x194 | the waypoint advance, the pause and the arrival | marked |
| `PreThink__12CWallCrawlerFfR13CStateManager` | 0x204 | the guard, the motion state and the floor snap | marked |
| `fn_83_129C` | 0x6C | the `CMotionState` copy before `AddMotionState` | marked |
| `GetCollisionPrimitive__12CWallCrawlerCFv` | 0x8 | -> `&mColSphere` (0x7F0) | marked |
| `AlignToFloor__12CWallCrawlerF...` | 0x6B8 | the floor search, the tumble and the re-orient | marked |
| `fn_83_19C8` | 0x5C | the `CCollisionSurface` copy | marked |
| `AlignToPlane__12CWallCrawlerF...` | 0x164 | the clamped arc onto the normal | marked |
| `fn_83_1CB4` | 0x1C4 | `PointOnSurface` | marked |
| `fn_83_1E78` | 0x40 | the cross product | marked |
| `ProjectVectorToPlane__12CWallCrawlerF...` | 0x4C | the projection onto a plane | marked |
| `fn_83_1F04` | 0x78 | the closest point on a plane | marked |
| `AcceptScriptMsg__12CWallCrawlerF...` | 0xBC | the materials on `kSM_Create` | marked |
| `__dt__12CWallCrawlerFv` | 0x70 | the destructor | marked |
| `__ct__12CWallCrawlerF...` | 0x310 | the constructor | marked |
| `fn_83_26CC` | 0x54 | the `CCollisionSurface` constructor | marked |

### g.4 Count check

Every function id of the module's symbol list must appear exactly once in the tables above. The
check reads the symbol list of record and the table rows of this section (the table shortens the
REL's mangled names, so the rows are mapped back to the ids first):

```
$ python3 - <<'PYCHK'
import re, pathlib, collections
sym = pathlib.Path('config/G2ME01/rels/WallCrawler/symbols.txt').read_text()
ids = re.findall(r'^(\S+) = \.text:0x[0-9A-Fa-f]+; // type:function', sym, re.M)
doc = pathlib.Path('docs/module-analysis/WallCrawler.md').read_text()
sec = doc.split('## g. Function classification')[1].split('\n## 8.')[0]
rows = re.findall(r'^\| `([^`]+)`', sec, re.M)
alias = {'fn_83_C8': 'RELMain', 'fn_83_CC': 'RELExit',
         'fn_83_1F7C': 'GetClosestPointOnPlane__12CWallCrawlerFRC9CVector3fRC6CPlane'}
mapped = []
for r in rows:
    if r in alias:
        mapped.append(alias[r])
    elif r.endswith('...'):
        head = r[:-3]
        hit = [i for i in ids if i.startswith(head)]
        mapped.append(hit[0] if len(hit) == 1 else r)
    else:
        mapped.append(r)
c = collections.Counter(mapped)
print('symbols:', len(ids))
print('classified rows:', len(rows))
print('missing:', [i for i in ids if i not in c])
print('duplicated:', [i for i, n in c.items() if n > 1])
print('extra:', [r for r in c if r not in ids])
PYCHK
symbols: 57
classified rows: 57
missing: []
duplicated: []
extra: []
```

## 8. The slices on this branch

### 8.1 The behaviour fixes (2026-10-21, branch `decomp/wallcrawler`)

- **The flag layout is now the retail's.** The header's bitfields carried the wrong masks (they
  were one bit off): the retail's byte 0x860 is `mAlignToFloor` 0x80, `mHasAlignSurface` 0x40,
  `mPlayerObstructed` 0x20, `mDisableMove` 0x10, then the tumbling flag 0x08, the re-orienting
  flag 0x04, the kART_Six flag 0x02 and the kART_Five flag 0x01; byte 0x861's 0x80 is
  `x861_24_` (bending enabled). The four unnamed members were renamed `x860_28_`..`x860_31_`
  and every bitfield now carries its retail mask as a comment, so `tools/asmcheck.py check`
  reports no MASK hit for the module any more.
- **AlignToFloor had two behaviour differences**, both fixed:
  1. the best distance started at `radius`; the retail's threshold is **radius + margin** (f24 is
     the radius argument at asm 0x1390 and the margin at 0x810 is added to it at asm 0x141C,
     before the first comparison at asm 0x159C). The earlier reading stopped at asm 0x1390 and
     missed the addition; section 8.2 has the correction and the fix.
  2. the port ran an extra "player-obstruction scan" that the retail does not have (see the
     bullet above); it could set `mPlayerObstructed` and stop the crawler.
- **Where the constraint (0x834) comes from.** The WallCrawler REL has no loader, so no shipped
  placement creates a bare `CWallCrawler`; the class is only the base of the modules that do
  (`CPillBug`, `CParasite`, `CKralee`, `COctapedeSegment`, `CWallWalker`). Of those, only the
  PillBug sets a constraint: its init data carries the property `planarConstraint` (id
  `0xde7e9f94`, `SLdrPillBug.hpp`), and `CPillBug::PatrolImpl`/`ReturnToPath` pass it to
  `SetConstraint(mgr, mPlanarConstraint)`, which stores it in 0x834 and rebuilds the constraint
  plane (`UpdateConstraintPlane`). The other families' init data has no such property.
- **Shipped placements with a non-zero constraint** (scan of every MREA script layer, property id
  `0xde7e9f94`): 15 PillBugs, all with a constraint - 12 with `1` and 3 with `2`, none with `3`:
  Agon Wastes `0b_sand_hall` (3x 1), `0d_sand_hall` (4x 1, 1x 2), `05_portalpuzzle` L2 (2x 1) and
  Sanctuary Fortress `06_cliff_spiderballguardian` L8 (3x 1, 2x 2). So the constrained arm of
  `fn_83_1310` (asm 0x15AC-0x17A4) **is** reachable and was run: a temporary probe (removed
  before the commit) logged `[wc] constrained floor search: x834=1` in Agon `0b_sand_hall` and
  `0d_sand_hall` and `x834=2` in `0d_sand_hall`. The constraint-3 arm (the absolute value) has no
  shipped instance.
- The base class is shared with `CPillBug`, `CKralee`, `COctapedeSegment` and `CParasite`, which
  call this `AlignToFloor`; the tighter best distance and the removed `mPlayerObstructed` write
  change what they inherit. Those sessions should re-check their own runs (their RELs have their
  own floor-alignment functions).
- **Still not exercised:** the kART_Five bend and the boost-ball and contact damage (no player
  input). The `// @ fn_83_1310` marker keeps: both variants of the floor search (unconstrained and
  constrained) ran in the Agon Wastes areas above; only the constraint-3 sub-arm has no shipped
  instance.

### 8.2 The classification and the corrections (2026-10-21, branch `decomp/wallcrawler`)

**The 57 functions are classified in section g: 31 H (host copies), 2 T (dead leaves) and 24 B
(module behaviour).** Every B function carries a `// @ fn_83_` marker, so **the list of the unmarked
B functions is empty** - the slice had no new body to write. What the slice did instead:

- **Every constant, mask, compare and store of the port's bodies now carries its asm offset** (140
  of them in `PortCWallCrawler.cpp`), as the lane rules ask.
- **Two behaviour differences were found by re-reading the asm and fixed:**
  1. **The best distance of the floor search is `radius + margin`, not `radius`.** The earlier
     reading stopped at asm 0x1390, where f24 (the threshold) is set to the radius argument, and
     missed asm 0x141C, where the margin at 0x810 is added to f24 before the first comparison
     (asm 0x159C and 0x16E8). The search box is the same `radius + margin` (asm 0x13A4-0x13F4).
  2. **The agreement of the constrained search is a two-dimensional dot.** The constraint normal's
     z is loaded at asm 0x15C0 and immediately clobbered by the surface-flag mask (asm 0x15C4), so
     the z term of the dot is scaled by `(float)0x10000000` instead of by the constraint's own z
     (asm 0x1680). The port used the full 3D dot.
- **The run** (the Agon Wastes `0b_sand_hall`, the world 42b935e4, the area slot 40, the MREA
  84303dd5), 40000 game updates, `tools/drivers/wallcrawler_crawlers.py` as the driver:
  **0 `[port] caught`, 0 reach-stub**.

| fn | the arms | the run | marker |
|---|---|---|---|
| `fn_83_580` (Think) | the material follows the actor's movable flag | ran (both ways) | marked |
| | the bend, the kART_Six side (the reaction type 6, asm 0x868) | ran (37k deliveries) | marked |
| | the bend, the kART_Five side (the reaction type 5, asm 0x9C4) | **did not run**: no crawler was re-orienting (0x04) without the Six bend playing | marked |
| | the contact damage (the timer at 0x440) | ran (311 times, the crawler next to the player) | marked |
| | the boost-ball damage | **did not run**: the player falls through the floor when it morphs at the area's spawn point, so it never touches a crawler while boosting | marked |
| `fn_83_1310` (AlignToFloor) | the unconstrained search | ran | marked |
| | the constrained search, `x834_` = 1 | ran (the PillBugs of `0b_sand_hall`) | marked |
| | the constrained search, `x834_` = 2 | ran in the earlier slice (`0d_sand_hall`), not re-run here | marked |
| | the constrained search, `x834_` = 3 | no shipped instance (the property scan of the note's section 8.1) | marked |
| | the surface / the tumble / the re-orient | ran (the flags 0x08 and 0x04 were both seen) | marked |
| `fn_83_1098` (PreThink) | the guard, the motion state, the floor snap | ran (the crawlers think every frame) | marked |
| `fn_83_148` (UpdateConstraintPlane), `fn_83_F04` (UpdateWPDestination), `fn_83_1B50` (AlignToPlane), `fn_83_1CB4`/`fn_83_1F04`/`fn_83_1E78` (the point on surface), `fn_83_1FD0` (AcceptScriptMsg), `fn_83_4C8` (GetTouchBounds), `fn_83_2374` (the ctor), `fn_83_129C`/`fn_83_19C8`/`fn_83_26CC` (the copies) | their live arms | ran | marked |

The two open arms (the kART_Five bend and the boost-ball damage) are the same two the earlier slice
listed. They are **not** forced in this run: the Five bend needs a crawler that is on a surface and
still turned away from its normal, and the boost-ball damage needs the player morphed next to a
crawler - the morph drops the player through the floor in that area (a port bug outside this
module's slice; the contact damage ran in the same area with the player on its feet).

**The gates:** `$T/ninja -C build-port-link metroid_prime2_port` 0 FAILED;
`python3 tools/gen_files_cmake.py --check` rc 0; `tools/review_decomp_diff.sh <base>` clean; the run
as above.
