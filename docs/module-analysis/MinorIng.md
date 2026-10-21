# MinorIng (module 44)

The MinorIng REL's host body: `src/MetroidPrime/Enemies/PortCMinorIng.cpp`,
`include/MetroidPrime/Enemies/CMinorIng.hpp`. The Ing family analysis (module 29 `Ing`
and module 44 `MinorIng` together) is in `IngFamily.md`; this note is the MinorIng
half: what is ported, what is not, and the layout facts the port depends on.

## Status (2026-10-21)

- 114/217 functions marked (was 103). The new ones: `fn_44_E0` (the module
  registration), `fn_44_2550` (the cover-hint claim), `fn_44_D00` (the health-info
  copy), `fn_44_5894` (the team leave), `fn_44_34B0` (the ray the snap to the floor
  uses), `fn_44_3614` (the copy of the ray result), `fn_44_FE8` (the grenade data
  the loader builds), `fn_44_C44` (the copy of it into the data), `fn_44_D54` (the
  copy of the ing spot into the data), `fn_44_9B4` (the data build) and `fn_44_AB78`
  (the best animation for a parm).
- `tools/asmcheck.py check MinorIng src/MetroidPrime/Enemies/PortCMinorIng.cpp
  --root .` reports no correctness finding (MASK/XBIT/CROP/XCMP/XSTORE/OFFS).
- Run: `USERDIR=/tmp/opencode/mi_u WORLD=3bfa3eff,686f5a1b FRAMES=7000 GODMODE=1
  MP2_DEBUG_MINORING=1 DISPLAY_NUM=:93 SECS=240 tools/run_gameplay.sh` (area 13 of
  63, Agon Wastes). 0 `[port] caught`, 0 `[reach-stub]`.
- The thirteen "unported chain" functions are classified: they are the host's own type members
  (the CModelData, CDamageInfo and CIngSpotData copies, the members' destructors) or bodies the
  host already has (`fn_44_4DDC`, `fn_44_204C`), so they take no marker and are cited where the
  retail calls them (the note has the table).

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


## The thirteen unmarked chains are the host's own types

The functions the "unported chain" class of `/tmp/opencode/mi_unmarked.tsv` holds are not MinorIng
behaviour the host lacks: each is the REL's own copy of a host type's member function (a copy
constructor, an assignment or a destructor) or a body the host already has. They take no marker
(the retail's body is compiler output the host's C++ already does); each is cited where the retail
calls it.

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

The list's "truly unreferenced" rows are dead in the REL: the module's own text never calls them
and none of its tables names them (the vtables, the FSM tables, the ctor/dtor lists and the
relocations were all searched). Three of them are named by address and are live: the module's
static constructor and the two destructors it registers. The rest are the retail's own unused
copies, so the host has no counterpart and needs no code.

| retail function | size | what it is |
|---|---|---|

## The functions that are still not ported

`tools/port_progress.py` counts 103 of the module's 217 functions as unmarked.
The table lists each with its retail size and the module functions that call it
(the call graph is read from the REL's disassembly, so "called from" is the retail
call, not the host's). The ones the live cycle reaches are the ctor/dtor/loader
chain and the projectile's spawn parameters (`fn_44_65C4` -> `fn_44_660C` ->
`fn_44_66C0` -> `fn_44_6700` -> `fn_44_6720` -> `fn_44_6748` and `fn_44_64AC` ->
`fn_44_6500` -> `fn_44_655C` -> `fn_44_657C`); the damage relay
(`fn_44_25E4` -> `fn_44_28D8` -> `fn_44_2930`, `fn_44_2CD8` -> `fn_44_2EF4` ->
`fn_44_2F24` -> `fn_44_2FB0`) and the cover scan (`fn_44_4E84` -> `fn_44_4DDC`)
are the chains the classification above covers, so they are not in the tables.

### Over 0x80 bytes (the stand-in size line)

| retail function | size | called from |
|---|---|---|
| `fn_44_660C` | 0xB4 | fn_44_65C4 |
| `_unresolved` | 0xC4 | - |
| `fn_44_16C4` | 0x108 | fn_44_14D8 |
| `fn_44_1830` | 0x11C | fn_44_14D8 |
| `fn_44_97E4` | 0x15C | - |
| `fn_44_A680` | 0x184 | fn_44_9D58 |
| `fn_44_14D8` | 0x188 | fn_44_110 |
| `fn_44_A9DC` | 0x19C | fn_44_9D58 |
| `fn_44_9940` | 0x1E4 | fn_44_97E4 |
| `fn_44_1CE0` | 0x358 | fn_44_110 |
| `fn_44_194C` | 0x394 | fn_44_110 |
| `fn_44_7058` | 0x654 | - |
| `fn_44_110` | 0x844 | - |
| `fn_44_9D58` | 0x878 | fn_44_110 |

### 0x80 bytes and under

| retail function | size | called from |
|---|---|---|
| `fn_44_0` | 0x8 | - |
| `fn_44_8` | 0x8 | - |
| `fn_44_10` | 0x8 | - |
| `fn_44_28` | 0x8 | - |
| `fn_44_30` | 0x8 | - |
| `fn_44_60` | 0x8 | - |
| `fn_44_68` | 0x8 | - |
| `fn_44_5AB8` | 0x8 | - |
| `fn_44_48` | 0xC | - |
| `fn_44_54` | 0xC | - |
| `fn_44_18` | 0x10 | - |
| `fn_44_38` | 0x10 | - |
| `fn_44_59E8` | 0x1C | - |
| `__register_global_object` | 0x1C | - |
| `RELMain` | 0x20 | - |
| `fn_44_1114` | 0x20 | fn_44_10D4 |
| `fn_44_655C` | 0x20 | fn_44_6500 |
| `fn_44_6700` | 0x20 | fn_44_66C0 |
| `fn_44_76AC` | 0x20 | fn_44_7058 |
| `fn_44_8DF0` | 0x20 | - |
| `fn_44_8E10` | 0x20 | - |
| `RELExit` | 0x24 | - |
| `_epilog` | 0x24 | - |
| `_prolog` | 0x24 | - |
| `fn_44_6720` | 0x28 | fn_44_6700 |
| `fn_44_70` | 0x2C | - |
| `fn_44_6FA0` | 0x2C | - |
| `fn_44_10D4` | 0x40 | fn_44_110 |
| `fn_44_66C0` | 0x40 | fn_44_660C |
| `fn_44_657C` | 0x48 | fn_44_655C |
| `fn_44_65C4` | 0x48 | fn_44_6384 |
| `fn_44_7918` | 0x48 | - |
| `ModuleDestructors` | 0x4C | - |
| `ModuleConstructors` | 0x4C | - |
| `fn_44_64AC` | 0x54 | fn_44_6384 |
| `fn_44_28D8` | 0x58 | fn_44_25E4, fn_44_7058 |
| `fn_44_9BB8` | 0x58 | fn_44_97E4 |
| `__destroy_global_chain` | 0x58 | - |
| `fn_44_E08` | 0x5C | fn_44_D54 |
| `fn_44_6500` | 0x5C | fn_44_64AC, fn_44_65C4 |
| `fn_44_78BC` | 0x5C | - |
| `fn_44_8404` | 0x5C | - |
| `fn_44_87E0` | 0x5C | - |
| `fn_44_A5D0` | 0x74 | fn_44_9D58 |
| `fn_44_6748` | 0x80 | fn_44_6720 |

## Layout facts the port depends on (all read from the REL's disassembly)

- `0xcd8` is the safe zone the XENZ message names; `0xcda` is the zone that lured
  the actor (`fn_44_36FC` and `fn_44_3780` read and write `0xcda`). They are two
  members: the port has `mSafeZoneEnterId` and `mSafeZoneId`.
- The safe-zone entry waits for the timer at `0xd1c` (`mTimerD1C`), not for the
  move-away timer at `0xd20` (`fn_44_9150`, asm 0x9610).
- Byte `0xd3d` (MSB first): `mAttackPatternProjectile` 0x80, `mPathFindActive` 0x40,
  `mPointPathFinding` 0x20, `mFoundMovementPos` 0x10, `mRecalculatePath` 0x08,
  `mIdle` 0x04, `mFollowingAttackPath` 0x02, `mHasPatrolPath` 0x01. The XALD message
  sets `mHasPatrolPath` (asm 0x928C) and the waypoint increment sets
  `mAttackPatternProjectile` (asm 0x96D0).
- Byte `0xd3e` (MSB first): `mPuddleHitByWeapon` 0x80, `mCancelStun` 0x40,
  `mKnockAnimCancel` 0x20, `mTouched` 0x10, `mShouldMoveUnderBall` 0x08,
  `mInsideSafeZone` 0x04, `mShouldEvaporate` 0x02. The safe-zone messages set and
  clear `mInsideSafeZone` (asm 0x9640 and 0x9698).
- `CHealthInfo` is 0x20 bytes with the retail's layout (three floats, two weapon
  modes, four ids, two flag bits), so `fn_44_D00`'s copy is a plain assignment.

## The functions that are not marked

`/tmp/opencode/mi_unmarked.tsv` lists every unmarked function of the module with its retail size,
its class and the host's part. The classes are: 13 vtable slots (the module's own overrides of
the CPatterned's virtuals, with the slot they fill), 4 slots of the state machine's function
tables, 10 the host's own inline or generated code (the SLdr readers and constructors, the model
data, the vector resize), 5 retail bodies the host does not model but that the live cycle reaches
(the host's own classes cover them: fn_44_1114 is the host's generated `LdrToPatternedInfo`,
fn_44_28D8 the host's `CEntityInfo` destructor, fn_44_9BB8 a member's own destructor, fn_44_E08
the host's `CIngSpotData` copy and fn_44_9940 the host's `CPatterned` destructor), and the rest
unported chains the module calls but the live cycle never reaches. The chain that matters is the
projectile's spawn parameters (fn_44_65C4 -> fn_44_660C -> fn_44_66C0 -> fn_44_6700 -> fn_44_6720
-> fn_44_6748, with fn_44_64AC -> fn_44_6500 -> fn_44_655C -> fn_44_657C beside it): the retail's
DoUserAnimEvent (fn_44_6384) spawns the projectile through it, while the host's uses the base's
LaunchProjectile, so porting that chain is the next slice.

The thirteen vtable slots break down as: eight the host's `CMinorIng` overrides itself (marked
below), nine that are the base class's own inline functions (the REL carries its own copies of
them, the host's `CPatterned` and `CActor` have the same base functions) and four that are the
FSM2 node destructors (the host's `CState2` node classes own theirs). The latter two groups are
cited in the code instead of marked.

The eight vtable slots the host's `CMinorIng` overrides itself are marked now: fn_44_0 (the path
find search), fn_44_8 (the point path find search), fn_44_5AB8 (the projectile info), fn_44_10 (the
listening flag), fn_44_8E10 (PreRender), fn_44_8DF0 (AddToRenderer), fn_44_6FA0 (GetAimPosition)
and fn_44_70 (GetHealthInfo, which forwards to the virtual `HealthInfo()` the retail calls). The
check finds no difference in any of them, and a run (the Temple Grounds' area 13) showed all of
them running. The loader (fn_44_110) and the destructor
(fn_44_97E4) are marked too: the check finds the retail's property loop and the members' own
teardown against the host's generated reader and empty body, which is a tool limit - the property
ids and their order and the teardown order were checked by hand.

## Gotchas

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
