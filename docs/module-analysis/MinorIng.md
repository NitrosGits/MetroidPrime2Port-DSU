# MinorIng (module 44)

The MinorIng REL's host body: `src/MetroidPrime/Enemies/PortCMinorIng.cpp`,
`include/MetroidPrime/Enemies/CMinorIng.hpp`. The Ing family analysis (module 29 `Ing`
and module 44 `MinorIng` together) is in `IngFamily.md`; this note is the MinorIng
half: what is ported, what is not, and the layout facts the port depends on.

## Status (2026-10-21 16:55)

- 128/217 functions marked (was 126). The new ones: `fn_44_4DDC` (the target's aim
  position) and `fn_44_204C` (the lure point on the nav mesh), plus (from the earlier
  slices) `fn_44_E0` (the module registration), `fn_44_2550` (the cover-hint claim),
  `fn_44_D00` (the health-info copy), `fn_44_5894` (the team leave), `fn_44_34B0` (the
  ray the snap to the floor uses), `fn_44_3614` (the copy of the ray result),
  `fn_44_FE8` (the grenade data the loader builds), `fn_44_C44` (the copy of it into
  the data), `fn_44_D54` (the copy of the ing spot into the data), `fn_44_9B4` (the
  data build) and `fn_44_AB78` (the best animation for a parm).
- Every function of the module is classified in "The function classification" below and
  in `docs/module-analysis/MinorIng_functions.tsv`: **75 H** (a copy of something the
  host already has), **13 T** (a data/vtable thunk or dead in the REL), **129 B** (module
  behaviour, 128 of them marked). The unmarked B list is one entry: `fn_44_ABCC`, the
  module's static constructor, whose three FSM tables the host builds at runtime
  (`tools/drivers/minoring_tables.py` checks them entry for entry).
- `fn_44_204C`'s third branch (the height query succeeds) does not run in area 13: the
  nav mesh query returns `kR_NoSourcePoint` there for the lure point, for the player and
  with the search's flags at 3 and at 0x301, so the area has no region under either point.
  The query is the Ing lane's `PortIngLuredMeshPoint` (in `PortIngShared.hpp`), so it is
  not this lane's to fix.
- `tools/asmcheck.py check MinorIng src/MetroidPrime/Enemies/PortCMinorIng.cpp
  --root .` reports no correctness finding (MASK/XBIT/CROP/XCMP/XSTORE/OFFS).
- Run: `USERDIR=/tmp/opencode/mi_u WORLD=3bfa3eff,686f5a1b FRAMES=5000 GODMODE=1
  MP2_DEBUG_MINORING=1 DISPLAY_NUM=:105 SECS=220 tools/drivers/minoring_run.sh` (area 13
  of 63, Agon Wastes). 290 `[minoring]` lines, 0 `[port] caught`, 0 `[reach-stub]`, the
  run ended on the frame limit (exit 0). The twelve FSM states the module can reach all
  ran (the branch table below).
- The flag bytes of the host are the retail's bits now (the header's bitfields are declared
  in the guest's LSB-first order and carry their positions), and the note's 0xd3d table is
  corrected: `mHasPatrolPath` is 0x80 and `mAttackPatternProjectile` is 0x01, the other way
  round from what the note said before.

## The function classification (all 217 functions)

`docs/module-analysis/MinorIng_functions.tsv` has one row per function of the REL (module
44): its offset, its size, what it is, its kind, its class, the module functions that call
it, the host's part and whether the port marks it. The rows come from
`tools/drivers/minoring_scan.py` (it walks the REL's asm - every `.fn` gives an offset, a
size and its `bl` edges, every data object gives the table that names it) and from the
port's own source (the marked bodies); `tools/drivers/minoring_class.py` writes the table.
The classification below is the hand-written part and the rule is stated so the table can
be audited.

Three classes, applied in this order:

- **H - host copy** (75 functions, 0x2750 bytes): the body is a copy of something the host
  already has, so the port writes no body for it. Six sources: the crtexe REL runtime
  (9 `runtime`), compiler-emitted destructors that exist only for exception handling
  (30 `weak-dtor`), the REL's own copy of a host type's member function (29 `inline-dol`),
  upstream's generated loader code (5 `inline-hdr`), the REL's own vector and template
  code (2 `template`) and the REL's own copy of CImpactVisorEffect (10 `inline-dol`).
- **T - data / vtable thunk, or dead** (13 functions, 0x1c4 bytes): the body only returns a
  constant, a member address or a flag (9 `virtual` thunks in the vtable `data_620`), or
  the function is dead in the REL (4 `unreferenced`: no module text calls it and no table
  names it - see "The functions the REL never calls").
- **B - module behaviour** (129 functions, 0x8d88 bytes): real behaviour - these are the
  port's bodies. 128 of them carry a `// @ fn_44_` marker; the one that does not is
  `fn_44_ABCC`, the module's static constructor (below).

| class | functions | .text bytes |
|---|---|---|
| H host copy | 75 | 0x2750 (10,064) |
| T thunk or dead | 13 | 0x1c4 (452) |
| B module behaviour | 129 | 0x8d88 (36,232) |
| **total** | **217** | **0xb69c (46,748)** |

By kind: 53 `method`, 34 `fsm-trigger`, 30 `weak-dtor`, 29 `inline-dol`, 22 `virtual`,
15 `fsm-state`, 9 `virtual` (thunk), 7 `runtime`, 5 `inline-hdr`, 4 `unreferenced`,
2 `rel-entry`, 2 `template`, and one each of `rel-entry`, `loader`, `dtor`, `ctor` and
`static-init` (= 217).

### The unmarked B list

`fn_44_ABCC` (0x8b8), the module's static constructor. It is the REL's `.ctors` entry, so
`ModuleConstructors` calls it when the module is loaded. It copies the three FSM tables
into `.data` - `data_1A8` (the 34 triggers), `data_47C` (the 15 states) and `data_5A8` (the
5 code functions) - and registers the two global strings it builds (`fn_44_2984` is the
destructor it registers with them). The port needs no body for it: the host installs its
own tables in `CMinorIng::SetupStateMachine` and keeps the names as literals.
`tools/drivers/minoring_tables.py` reads both sides and compares them entry for entry:

    == triggers: the REL's lbl_44_data_1A8 has 34 entries, the host's skTriggers has 34
    == states: the REL's lbl_44_data_47C has 15 entries, the host's skStates has 15
    == code functions: the REL's lbl_44_data_5A8 has 5 entries, the host's skCodeFuncs has 5
    MISMATCHES: 0

So the module has no unmarked behaviour left: the other 88 unmarked functions are 75 H and
13 T, and the 13 T are the vtable thunks and the four dead bodies, none of which the retail
ever calls.

### The branch table of the run

The run in the Agon Wastes' `0a_sand_hall_dark` (the sand world, area 13 of 63) with
`MP2_DEBUG_MINORING=1` logged 290 module lines. The FSM states that ran:

| state | how it ran |
|---|---|
| `Start` | the actor's first state (1) |
| `Idle` | the idle state, 59 times (the Ing waits) |
| `PathFind` | the path find state, 35 times (with `FoundMovementPos` and `ShouldPathFind` firing) |
| `PointPathFind` | the point path find state, 35 times (with `PointPathOver` and `FoundPointPath` firing) |
| `IntoPuddleForm` | the form change, 26 times (`AnimOver` fired it) |
| `IntoCorporealForm` | the form change back, 25 times (`ShouldFire` fired it) |
| `ProjectileAttack` | the attack state, 25 times (`AnimOver` fired it) |
| `Stunned` | the stun state, twice (`PuddleHitByWeapon` fired it) |
| `Dead` | the death state, twice |
| `FollowAttackPattern` | the attack path state, once (`SelectTarget` fired it) |
| `AlwaysPointPathFind` | the always point path find state, once |
| `MoveAwayFromSafeZone` | the safe zone state, once |

The code functions that ran: `SelectTarget` (five times), `SetPointCoverDest`,
`FindCoverPoint`. The triggers that fired: `PathShagged`, `ShouldFire`,
`PuddleHitByWeapon`, `ShouldPathFind`, `PointPathOver`, `FoundPointPath`,
`HasCoverPoint`, `Delay` and `AnimOver` (the base class's two). The data build's log line
ran once per placement ("inglets 0..0 spawn distance 40", four placements), so the loader
and `fn_44_9B4` ran; all four placements of this area have `minGeneration` and
`maxGeneration` 0, so the inglet spawn loop of `fn_44_7058` did not run in this run (the
note's earlier run of an area with a 0..3 placement saw two inglets).


## The stand-ins the host names

Each of these is a retail function of the module that the host does not model; the
port names it once per run, so the log says which body is missing. The names are
`[minoring] stand-in <fn> (<size> bytes)` lines.

| retail function | size | why the host does not have it |
|---|---|---|
| - | - | nothing: the module's stand-ins are all ported now |

`fn_44_97E4` (the destructor), `fn_44_34B0` (the ray), `fn_44_9D58` (the constructor), `fn_44_110`
(the loader) and `fn_44_7058` (the massive death) were stand-ins and are ported now: the destructor
is the host's member destruction (the retail's body is the members' own teardown), the ray is
`CMinorIng::RayDown`, the constructor fills the members in the retail's order, the loader reads the
same properties into the same members and the massive death spawns the inglets.

## The inglet's data (modelled)

The retail's loader reads the inglet's own data into the `CMinorIngData`: its health
at the data's 0xb0 (`fn_44_9B4` copies it there with `fn_44_D00`), its grenade data
at 0xd0 (`fn_44_C44` copies the "bouncing projectile" struct that `fn_44_FE8` builds
at the loader's sp+0x1f8) and seven spawn values at 0x120..0x138. All three are host
members now:

- `mIngletHealth` (guest 0xb0) is the health of the `unknown_0x3da35851` block
  (`fn_44_1CE0` reads it first); a run logs 4.0 HP and 1.0 knock-back resistance
  for the inglets of that area, so the read is sane.
- `mGrenadeData` (guest 0xd0) is the `CBouncyGrenadeData` the inglet spawn hands to
  the grenade. The retail's `fn_44_FE8` fills it from the same block: the mass
  (+0x00) and the speed (+0x04) are the block's `mass` and `bounciness`, the damage
  info (+0x08..+0x23) is the block's `damage`, the bounce count (+0x24) is
  `numBounces`, the four ids (+0x28..+0x34) are the block's `explosion` (twice),
  `trail` and `effect`, the sounds (+0x38/+0x3a) are `sound_Bounce` and
  `sound_Explode`, and the four floats (+0x3c..+0x48) are the rodata constants 0.1
  and 150 (rodata 0x28 and 0x2c). The flag byte at +0x4c takes the caller's bool
  argument: fn_44_FE8 is a leaf, so its +0xf is the caller's stack byte, and the
  caller's `li r0, 1` (asm 0x680) stored at +0xc (asm 0x6D4) puts the word's low byte
  - the one the callee reads (asm 0x103C) - at +0xf (big-endian), so the flag is set. A run of area 13 logs
  4.0 mass, 0.5 speed, 5 bounces, 5.0 damage, the sounds 8717 and 8716 and the ids
  0x04213c3c (twice), 0x36f5ce3e and 0x56330f8c; all four resolve in the resource
  pool, so the read is sane.
- The seven spawn values (guest 0x120..0x138, `fn_44_9B4`'s stores) are the block's
  `minLaunchSpeed` (0x120), `maxLaunchSpeed` (0x124), `minGeneration` (0x128),
  `maxGeneration` (0x12c), `maxTurnAngle` (0x130), `unknown_0xfbf8ea0a` (0x134) and
  `unknown_0x47f99fbc` (0x138). A run of area 13 logs 10.0/15.0, 0/0, 40.0, 40.0 and
  2.0: the launch speed range, no inglets in this area, the turn angle, the distance
  the inglet count falls off at and the inglet's explode distance.

Still to model: the inglet spawn itself (`fn_44_7058`).

### The loader's parts (fn_44_110)

The retail's loader (fn_44_110, 0x844) is a property loop over the REL's own binary
format. The host reads the same properties with the generated `SLdrMinorIng` reader
(the analysis checked the ids one by one), so the loop itself is upstream's code and
takes no marker here. Its parts:

| retail function | size | the host's part |
|---|---|---|
| `fn_44_14D8` | 0x188 | the `SLdrMinorIng` default constructor; upstream's generated one sets the same defaults |
| `fn_44_16C4`, `fn_44_1830` | 0x108, 0x11C | the two sub-blocks' default constructors, called by `fn_44_14D8` (upstream's generated ones) |
| `fn_44_194C` | 0x394 | the ing spot reader: the generated `LoadTypedefMinorIngIngSpot` |
| `fn_44_1CE0` | 0x358 | the inglet's block reader: the generated `LoadTypedefUnknownStruct34` |
| `fn_44_9B4` | 0x290 | the data build: the host's `BuildMinorIngData` (marked) |
| `fn_44_10D4` | 0x40 | the patterned info: the host's `LdrToPatternedInfo` |

`fn_44_9B4` copies the three blocks the loader read into the data, and the host's
body does the same with the marked copies `fn_44_D54` (the ing spot), `fn_44_D00`
(the inglet's health) and `fn_44_C44` (the grenade data). The retail's ing spot copy
runs in three steps (`fn_44_D54` calls `fn_44_E08`, which calls `fn_44_EB8`); the
host's `CIngSpotData` copy is the class's own, so those two take no marker.

#### The seven bool properties (the loader's flags)

The retail's loader reads seven bool properties and `fn_44_9B4` packs them into three bytes of
the data (asm 0xA5AC-0xA5C4), so each property id has one bit:

| data byte | bit | property id | the host's field |
|---|---|---|---|
| 0x154 | 0x80 | (the inglet block's allow-lock-on) | `mIngSpotAllowLockOn` |
| 0x155 | the whole byte | 0x9207f51 | `mFlags155` (`unknown_0x09207f51`) |
| 0x156 | 0x80 | 0x399d1eaa | `allowProjectileDuringAttackPattern` |
| 0x156 | 0x40 | 0xbfd0e644 | `unknown_0xbce16644` |
| 0x156 | 0x20 | 0x142433d3 | `unknown_0x142433d3` |
| 0x156 | 0x10 | 0xb6cc0063 | `stayOnPointPathFinding` |
| 0x156 | 0x08 | 0xe601f7bd | `allowProjectileDuringStayOnPointPathFinding` |
| 0x156 | 0x04 | 0x2107e4fb | `allowPuddleLockOn` |

The loader's `flags155`/`flags156` locals build these bytes the same way.

### The constructor's parts (fn_44_9D58)

The retail's constructor (fn_44_9D58, 0x878) builds the actor and calls four module
functions. The host builds `CMinorIng` with its own constructor, so the body itself
takes no marker, but its parts are:

| retail function | size | the host's part |
|---|---|---|
| `fn_44_AB78` | 0x54 | the best animation for a parm: the host's `FindBestAnimationId` (marked) |
| `fn_44_A5D0` | 0x74 | a vector resize; the host's `rstl::vector` does it |
| `fn_44_A680` | 0x184 | a vector of float pairs for the two animation states; the host's `CPatternedInfo` holds them |
| `fn_44_A9DC` | 0x19C | the copy of the data block into the actor; the host's `CMinorIngData` is its own shape, so the copy is the member's own |

What the retail's constructor fills, in its order (the host does the same):

- the two hit-effect tokens (asm 0x9EB0 for the normal one, asm 0xA02C for the heavy one) with
  their "is set" bytes at 0x94c and 0x95c; a valid asset id locks the token, an invalid one leaves
  it empty. The host's `rstl::optional_object< TLockedToken< CGenDescription > >` carries both.
- the path find search (asm 0xA108: the null area, 0x301, the patterned info's pathfinding index
  and 1.0/1.0 from the rodata at 0x44), the point search, the surface helper and the ing-spot
  navigation.
- the form (asm 0xA198) and the ten ids (asm 0xA1B8-0xA204): the retail stores 0xffff in each.
- the two-int container at 0xce0: `fn_44_A5D0` resizes it to two zero ints (asm 0xA5B4). Nothing
  reads them; the retail's destructor (asm 0x9820) and `fn_44_396C` (asm 0x39A4) only test the
  container for emptiness, which is never the case after the constructor. The host keeps the two
  zeros as `mUnusedZeros`.
- the health tracker at 0xd14: the retail reads the patterned info's health (asm 0xA1BC reads the
  `CPatternedInfo`'s health at +0x54), not the actor's own health info.
- the flag bytes: the retail clears every flag of 0xd3d (asm 0xA284-0xA2DC) and every flag of
  0xd3e (asm 0xA2E4-0xA330), most significant bit first.

The constructor is marked (`// @ fn_44_9D58`) and the check finds nothing in it. Its branches ran
in the Temple Grounds' area 13 and in the Agon Wastes' `0a_sand_hall_dark`, each with a temporary
hook that was reverted: the model has animation data (the Agon Wastes) or not (the Temple
Grounds), the lock-on locator is valid (the Agon Wastes) or not, and the two hit-effect assets are
set or not. The rest of the retail's branches (the token's own) are always taken.

### The massive death (fn_44_7058)

The retail's massive death (fn_44_7058, 0x654) spawns the inglets, and the host's
`CMinorIng::MassiveDeath` does the same now:

- the guards (asm 0x70CC-0x70F0): the health info's cause-of-death weapon mode (the CWeaponMode at
  +0x0c) keeps its flags in the byte at +0x0e, and that byte must not be 2 or 0x12 (the host's
  CWeaponMode has three flags, so it can never read 0x12); and an Ing in the puddle form (1) only
  spawns inglets when the data's byte 0x155 is set (the property 0x9207f51, the host's `mFlags155`).
- the count (asm 0x71D8-0x7238): the damage the Ing took (the base class's `mLastHP`, the health at
  the end of the last frame, less the health info's HP) clamped to 0..the inglet's spawn distance
  (the data's 0x134) gives the fraction of the maximum count (the data's 0x12c) that is spawned,
  rounded up and never below the minimum count (the data's 0x128).
- the materials (asm 0x7140-0x71D8): the inglet collides with the seeker-target material and, when
  the ing spot allows lock-on (the data's byte 0x154's bit 7), with the target and orbit materials
  too.
- the spawn point (asm 0x7274-0x7300): the Ing's position plus half the transform's up vector, and
  the look-at takes the same up vector. The direction is the normalised vector to the target.
- the angles (asm 0x73F4-0x7464): a random 35..60 degrees around the local X, then the table's
  value in tens of degrees less 50 around the local Z. The table is the REL's lbl_44_rodata_0
  (`{0, 8, 2, 6, 9, 3, 0, 5, 1, 7}`) and the index walks it from a random start.
- the inglet (asm 0x74A8-0x762C): a `CBouncyGrenade` named "Inglet" with the launch speed random out
  of the data's range (0x120..0x124), the grenade data (0xd0), the explode distance (0x138), the
  max homing angle (0x130), the health (0xb0), the flags 0xf, the inverted box and the Ing as the
  parent; it removes the projectile material from itself.
- the retail's stack box (the six floats at asm 0x7468-0x74A4) is doubled and then dropped: the
  bounds the inglet gets are the constant inverted box, so the host leaves it out.

The loader logs the inglets' count once per spawn (`[minoring] inglets <min>..<max> spawn distance
<d>`), so a run shows which placement has any. The Agon Wastes' `0a_sand_hall_dark` (the sand
world, area 13) has a placement with 0..3; the run that reached it (0 `[port] caught`, 0
`[reach-stub]`) confirmed the spawn: the Ing killed there left two inglets, the count the formula
gives for the damage it took (2 = ceil((1 - 20/40) * 3)).

#### fn_44_7058's branches

The body is marked (`// @ fn_44_7058`). Every branch the host can reach ran in the Agon Wastes'
`0a_sand_hall_dark` (the sand world, area 13), each with a temporary hook that was reverted:

| branch | how it ran |
|---|---|
| the death weapon's flags != 2 and != 0x12 (asm 0x70CC) | the flags ran at 0 and at 2 (a comboed light weapon) |
| the flags == 0x12 (asm 0x70D4) | unreachable: the host's CWeaponMode has three flags, so its byte is 0..7 |
| the form is not the puddle, or the data's 0x155 is set (asm 0x70DC) | both: the form ran at 0 and at 1 (the ing spot) |
| the damage is below zero (asm 0x7120) | the damage ran at -20 and at 20 |
| the damage is past the spawn distance (asm 0x7130) | the damage ran at 20 and at 100 |
| the count is under the minimum (asm 0x7228) | the count ran at 0 with the minimum at 1 |
| the count is zero (asm 0x7238) | the maximum count ran at 0 |
| the ing spot allows lock-on (asm 0x717C) | the materials ran with and without the target and orbit bits |
| the spawn loop (asm 0x739C) | it ran with 0, 1, 2 and 3 inglets |
| the inglet is added (asm 0x7614) | it ran with every count above |

The two branches that are not reachable on the host are the failed allocation (asm 0x74C4) and the
failed constructor (asm 0x74D4).


## The unmarked host-copy chains are the host's own types

The functions the "unported chain" class holds are not MinorIng
behaviour the host lacks: each is the REL's own copy of a host type's member function (a copy
constructor, an assignment or a destructor) or a body the host already has. They take no marker
(the retail's body is compiler output the host's C++ already does); each is cited where the retail
calls it. In the classification above they are the class `H`.

| retail function | size | what it is | the host's part | called from |
|---|---|---|---|---|
| `fn_44_1134` | 0x28 | the REL's copy of CModelData (the null-checked wrapper) | the host's `CModelData` owns its copy | `fn_44_1114` |
| `fn_44_115C` | 0x13C | the REL's copy of CModelData (the three model slots, the scale, the id; the source's byte at +0x0c cleared) | the host's `CModelData` owns its copy | `fn_44_1134` |
| `fn_44_2EF4` | 0x30 | the REL's copy of CDamageInfo (the assignment that returns the destination) | the host's `CDamageInfo` owns its copy | `fn_44_2CD8` |
| `fn_44_2F24` | 0x8C | the REL's copy of CDamageInfo (the mode, the four floats, the three ids; the flag byte at +0x1c) | the host's `CDamageInfo` owns its copy | `fn_44_2EF4` |
| `fn_44_2FB0` | 0x54 | the REL's copy of CDamageInfo (the null-guarded body of fn_44_2F24) | the host's `CDamageInfo` owns its copy | `fn_44_2F24` |
| `fn_44_2930` | 0x54 | the REL's copy of a member's destructor (CMemory::Free of the pointer at +0x0c, then of the block) | the host's `CEntityInfo` owns its destruction | `fn_44_28D8` |
| `fn_44_9B24` | 0x94 | the REL's copy of a member's destructor (the vtable restore, the CToken at +0x04) | the host's members own their destruction | `fn_44_9940` |
| `fn_44_9C10` | 0x58 | the REL's copy of a member's destructor (the member at +0x00) | the host's members own their destruction | `fn_44_9BB8` |
| `fn_44_9C68` | 0x58 | the REL's copy of a member's destructor (the member at +0x18) | the host's members own their destruction | `fn_44_9C10` |
| `fn_44_9CC0` | 0x98 | the REL's copy of a member's destructor (the two-bit counter in the byte at +0x00, its bit 5 the "alive" flag, the release of the pointer at +0x0c) | the host's members own their destruction | `fn_44_9C68` |
| `fn_44_4DDC` | 0xA8 | the target's aim position (the id at 0xcce, the vtable's 0x54 slot with dt 0) | the host's `TargetAimPosition` | `fn_44_4E84` |
| `fn_44_EB8` | 0x130 | the REL's copy of the ing spot's vector member (it allocates the array and copies the 12-byte entries) | the host's `CIngSpotData` owns its copy | `fn_44_E08` |
| `fn_44_204C` | 0x15C | the lure point on the nav mesh (the search's flags 3 for the height query, 0x301 after it) | the host's `PortIngLuredMeshPoint` | `fn_44_21A8`, `fn_44_22B0` |

`fn_44_115C`'s three optional model slots and `fn_44_2F24`'s layout are the ones that name the
types: the host's `CModelData` holds the three model slots as `rstl::optional_object` (the retail's
copy locks the source's token and takes its refcount) and the host's `CDamageInfo` is 0x1c bytes
with the mode, the four floats, the three sound ids and the two flag bits, so the retail's copy of
it (asm 0x2F24) moves exactly the host's fields.


## The functions the REL never calls

The scan (`tools/drivers/minoring_scan.py`) gives every function of the module its callers
(the `bl` edges in the REL's text) and the data objects that name it (the vtable `data_620`,
the FSM tables, the `.ctors`/`.dtors` lists). Thirty functions have neither: no module text
calls them and no table names them, so they are dead in the REL. They are the retail's own
unused copies (the delete operators and the member destructors the compiler emitted for the
exception tables) plus three bodies the retail never used, so the host has no counterpart
and needs no code.

Three more have no module text caller but are live, because something outside the module's
text names them: `fn_44_110` (the module's loader, which the game's loader calls),
`fn_44_2984` (the `rstl::string` destructor the module's static constructor registers as a
global destructor) and `fn_44_ABCC` (the module's static constructor, the `.ctors` entry
`ModuleConstructors` calls).

| retail function | size | what it is |
|---|---|---|
| `fn_44_954` | 0x60 | delete of a CDamageVulnerability holder (the delete operator) |
| `fn_44_E64` | 0x54 | delete of a heap block with a pointer at +0x0c |
| `fn_44_107C` | 0x58 | delete of a CDamageVulnerability holder |
| `fn_44_1298` | 0x6C | delete of a CToken holder |
| `fn_44_1304` | 0x54 | delete of a CToken holder |
| `fn_44_1358` | 0x64 | delete of a CAnimData holder |
| `fn_44_13BC` | 0x60 | delete of a CModelData holder |
| `fn_44_141C` | 0xBC | delete of an SLdr block (SLdrDamageInfo, SLdrActorParameters, ...) |
| `fn_44_1660` | 0x64 | delete of an SLdrDamageInfo block |
| `fn_44_17CC` | 0x64 | delete of an SLdrDamageInfo block |
| `fn_44_24C0` | 0x14 | sets the bit 0x02 of the byte at 0xd3e (mShouldEvaporate) |
| `fn_44_2C9C` | 0x3C | delete of a heap block |
| `fn_44_51D8` | 0x8C | delete of an array of 8-byte elements |
| `fn_44_57C4` | 0x80 | the team manager's member target (the team id at 0xccc, the target id at 0xcce) |
| `fn_44_5A04` | 0xB4 | faces the target (the direction from the target at 0xcce, FaceDirection) |
| `fn_44_67C8` | 0x6C | delete of a CToken holder |
| `fn_44_6834` | 0x54 | delete of a CToken holder |
| `fn_44_6888` | 0x3C | delete of a heap block |
| `fn_44_68C4` | 0x3C | delete of a heap block |
| `fn_44_6F98` | 0x8 | the body that returns &this->0x254 |
| `fn_44_8AFC` | 0x3C | delete of a heap block |
| `fn_44_9700` | 0x58 | delete of a CToken holder |
| `fn_44_A644` | 0x3C | delete of a heap block |
| `fn_44_A804` | 0x3C | delete of a heap block |
| `fn_44_A840` | 0x8C | delete of an array of 8-byte elements |
| `fn_44_A8CC` | 0x3C | delete of a heap block |
| `fn_44_A908` | 0x3C | delete of a heap block |
| `fn_44_A944` | 0x3C | delete of a heap block |
| `fn_44_A980` | 0x5C | delete of a CToken holder |

`fn_44_57C4` and `fn_44_5A04` are the only two of the thirty that are real behaviour
rather than a destructor: the first would hand the team manager its member target, the
second would face the Ing towards the target. The retail never calls either, so the host
does not need them; they are the class `T` in the classification above (dead), not `B`.

## The functions that are still not ported

None. The classification above is the answer: of the 217 functions of the module, 129 are
module behaviour and 128 of them carry a marker; the one that does not is `fn_44_ABCC`,
whose three FSM tables the host builds at runtime and `tools/drivers/minoring_tables.py`
checks entry for entry. The 88 other unmarked functions are 75 copies of host code and 13
thunks or dead bodies, all listed in
`docs/module-analysis/MinorIng_functions.tsv` with their class and their host's part.

## Layout facts the port depends on (all read from the REL's disassembly)

- `0xcd8` is the safe zone the XENZ message names; `0xcda` is the zone that lured
  the actor (`fn_44_36FC` and `fn_44_3780` read and write `0xcda`). They are two
  members: the port has `mSafeZoneEnterId` and `mSafeZoneId`.
- The safe-zone entry waits for the timer at `0xd1c` (`mTimerD1C`), not for the
  move-away timer at `0xd20` (`fn_44_9150`, asm 0x9610).
- Byte `0xd3d` (MSB first): `mHasPatrolPath` 0x80, `mPathFindActive` 0x40,
  `mPointPathFinding` 0x20, `mFoundMovementPos` 0x10, `mRecalculatePath` 0x08, `mIdle`
  0x04, `mFollowingAttackPath` 0x02, `mAttackPatternProjectile` 0x01. The area-loaded
  message sets `mHasPatrolPath` (asm 0x928C) and the waypoint increment sets
  `mAttackPatternProjectile` (asm 0x96D8). Each flag is pinned by the function that reads
  it: `fn_44_6178` (HasPatrolPath) reads 0x80, `fn_44_8C1C` (SelectTarget) reads 0x40,
  `fn_44_4A68` (IsPointPathFinding) reads 0x20, `fn_44_3430` (FoundMovementPos) reads
  0x10, `fn_44_6228` (RecalculatePath) reads 0x08, `fn_44_8128` (PathFind) reads 0x04,
  `fn_44_6210` (IsFollowingAttackPath) reads 0x02 and the waypoint message sets 0x01. The
  note had `mHasPatrolPath` and `mAttackPatternProjectile` the other way round before
  2026-10-21; the asm settles it.
- Byte `0xd3e` (MSB first): `mPuddleHitByWeapon` 0x80, `mCancelStun` 0x40,
  `mKnockAnimCancel` 0x20, `mTouched` 0x10, `mShouldMoveUnderBall` 0x08, `mInsideSafeZone`
  0x04, `mShouldEvaporate` 0x02. The safe-zone messages set and clear `mInsideSafeZone`
  (asm 0x9640 and 0x9698). The reading functions agree: `fn_44_5D64` (PuddleHitByWeapon)
  reads 0x80, `fn_44_5D58` (CancelStun) 0x40, `fn_44_3DE0` (CancelAnim) 0x20,
  `fn_44_5F60` (ShouldFire) 0x10, `fn_44_5D34` (ShouldMoveUnderBall) 0x08,
  `fn_44_621C` (InsideSafeZone) 0x04 and `fn_44_5D28` (ShouldEvaporate) 0x02.
- The host's two flag bytes are the retail's bits: `CMinorIng`'s bitfields are declared in
  the guest's LSB-first order, so the little-endian compiler gives every flag the retail's
  own mask, and the header carries the positions (`// 0xd3d 0x80`) so `tools/asmcheck.py`
  can check them. Before that the host's masks were the bit-reversal of the retail's in
  both bytes - self-consistent, so the behaviour was the same, but the checker could not
  pair the C++'s writes with the asm's bits (95 `BIT` findings; 18 after the change, all
  of them on the base class's own bytes).
- `CHealthInfo` is 0x20 bytes with the retail's layout (three floats, two weapon
  modes, four ids, two flag bits), so `fn_44_D00`'s copy is a plain assignment.

## The functions that are not marked

The classification above is the list: `docs/module-analysis/MinorIng_functions.tsv` has
every unmarked function of the module with its retail size, its class and the host's part.
Of the 89, 75 are copies of host code (the REL runtime, the compiler-emitted destructors,
the REL's own copies of a host type's member functions, upstream's generated loader code,
the FSM2 node destructors and the REL's own copy of CImpactVisorEffect), 13 are thunks or
dead bodies, and one is `fn_44_ABCC` (the module's static constructor, whose tables the
host builds at runtime).

The vtable `data_620` holds 31 of the module's own entries (the rest of the 82 slots are
the DOL's symbols). They break down as: 22 the host's `CMinorIng` overrides itself (marked
below), nine that only return a constant, a member address or a flag (the class `T`
thunks). The 22 marked ones are fn_44_0 (the path
find search), fn_44_8 (the point path find search), fn_44_5AB8 (the projectile info), fn_44_10 (the
listening flag), fn_44_8E10 (PreRender), fn_44_8DF0 (AddToRenderer), fn_44_6FA0 (GetAimPosition)
and fn_44_70 (GetHealthInfo, which forwards to the virtual `HealthInfo()` the retail calls),
plus the base class's own slots the REL carries copies of. The check finds no difference in
any of them, and a run (the Temple Grounds' area 13) showed all of
them running. The loader (fn_44_110) and the destructor
(fn_44_97E4) are marked too: the check finds the retail's property loop and the members' own
teardown against the host's generated reader and empty body, which is a tool limit - the property
ids and their order and the teardown order were checked by hand.

## Gotchas

- The host's trigger functions are `const` (upstream's `CPatterned`'s
  `TStateMachineFunctionTypes< CPatterned >::TriggerFunc`), so a trigger that writes a flag
  (the retail's `fn_44_4A74` clears `mPointPathFinding`, `fn_44_6268` writes
  `mFollowingAttackPath`) needs a `const_cast` in the host. `tools/asmcheck.py` does not see
  a write through a `const_cast`, so it reports those two as `BIT` findings ("the C++ never
  writes it") - a tool limit, not a gap. The retail's trigger functions are not const.

- `tools/asmcheck.py` maps a retail function to the C++ definition whose comment
  block cites it first, and the last such definition wins. A helper whose comment
  names a `fn_44_` id steals that id from the body that implements it - the three
  comments in this port were fixed on 2026-10-20 (commit "the asmcheck mapping
  pointed at the wrong bodies").
- A whole-line `// fn_44_XXXX` comment *inside* a body marks the block below it as
  another function, and asmcheck leaves that block out. The `fn_44_9D58` cites in
  the constructor's body and in `IsMoveClear` do that, so those blocks are not
  covered by the check.
- `fn_44_9D58` (the retail constructor) is not checked at all: the host's
  constructor is the host's own, so it carries no marker. It fills the same members in the
  same order as the retail's, so the stored state matches. The same holds for
  `fn_44_110` (the loader) and `fn_44_7058` (the massive death).
- The container at 0xce0 is two zero ints in the host (`mUnusedZeros`); the check reports two
  `CMP` findings for it (`fn_44_396C` and `fn_44_97E4` test it for emptiness, which the retail's
  vector never is after the constructor). They are completeness findings, not correctness ones.
- The retail `RELMain` (the module entry at 0xC0) cannot be marked: asmcheck
  looks a function up by its `fn_<id>_<offset>` name or by a bare offset, and the
  module's entry is a named symbol.
- `fn_44_2550`'s DOL call (asm 0x2574, `fn_8022A2DC`) is the host's
  `CScriptAIHint::SetInUse(true)`; the names do not match, so asmcheck's CALL tag
  cannot pair them.
