# MinorIng (module 44)

The MinorIng REL's host body: `src/MetroidPrime/Enemies/PortCMinorIng.cpp`,
`include/MetroidPrime/Enemies/CMinorIng.hpp`. The Ing family analysis (module 29 `Ing`
and module 44 `MinorIng` together) is in `IngFamily.md`; this note is the MinorIng
half: what is ported, what is not, and the layout facts the port depends on.

## Status (2026-10-20)

- 109/217 functions marked (was 103). The new ones: `fn_44_E0` (the module
  registration), `fn_44_2550` (the cover-hint claim), `fn_44_D00` (the health-info
  copy), `fn_44_5894` (the team leave), `fn_44_34B0` (the ray the snap to the floor
  uses) and `fn_44_3614` (the copy of the ray result).
- `tools/asmcheck.py check MinorIng src/MetroidPrime/Enemies/PortCMinorIng.cpp
  --root .` reports no correctness finding (MASK/XBIT/CROP/XCMP/XSTORE/OFFS).
- Run: `USERDIR=/tmp/opencode/mi_u WORLD=3bfa3eff,686f5a1b FRAMES=7000 GODMODE=1
  MP2_DEBUG_MINORING=1 DISPLAY_NUM=:93 SECS=240 tools/run_gameplay.sh` (area 13 of
  63, Agon Wastes). 0 `[port] caught`, 0 `[reach-stub]`.

## The stand-ins the host names

Each of these is a retail function of the module that the host does not model; the
port names it once per run, so the log says which body is missing. The names are
`[minoring] stand-in <fn> (<size> bytes)` lines.

| retail function | size | why the host does not have it |
|---|---|---|
| `fn_44_9D58` | 0x878 | the retail constructor: the host builds `CMinorIng` with its own ctor and the loader fills `CMinorIngData` through the generated `SLdrMinorIng` reader |
| `fn_44_110` | 0x844 | the retail loader reads the REL's own binary format; the host reads the generated `SLdrMinorIng` |
| `fn_44_7058` | 0x654 | the massive death spawns the inglets (`CBouncyGrenade`); the inglet's health (guest 0xb0), its grenade data (guest 0xd0) and its spawn data (guest 0x8e0..0x8f8) are not read by the host's loader |

`fn_44_97E4` (the destructor) and `fn_44_34B0` (the ray) were stand-ins and are
ported now: the destructor is the host's member destruction (the retail's body is
the members' own teardown) and the ray is `CMinorIng::RayDown`.

## The functions that are still not ported

`tools/port_progress.py` counts 108 of the module's 217 functions as unmarked.
The table lists each with its retail size and the module functions that call it
(the call graph is read from the REL's disassembly, so "called from" is the retail
call, not the host's). The ones the live cycle reaches are the ctor/dtor/loader
chain, the projectile's spawn parameters (`fn_44_65C4` -> `fn_44_660C` ->
`fn_44_66C0` -> `fn_44_6700` -> `fn_44_6720` -> `fn_44_6748` and `fn_44_64AC` ->
`fn_44_6500` -> `fn_44_655C` -> `fn_44_657C`), the damage relay
(`fn_44_25E4` -> `fn_44_28D8` -> `fn_44_2930`, `fn_44_2CD8` -> `fn_44_2EF4` ->
`fn_44_2F24` -> `fn_44_2FB0`) and the cover scan (`fn_44_4E84` -> `fn_44_4DDC`).

### Over 0x80 bytes (the stand-in size line)

| retail function | size | called from |
|---|---|---|
| `fn_44_2F24` | 0x8C | fn_44_2EF4 |
| `fn_44_51D8` | 0x8C | - |
| `fn_44_A840` | 0x8C | - |
| `fn_44_FE8` | 0x94 | fn_44_110 |
| `fn_44_9B24` | 0x94 | fn_44_9940 |
| `fn_44_9CC0` | 0x98 | fn_44_9C68 |
| `fn_44_4DDC` | 0xA8 | fn_44_4E84 |
| `fn_44_D54` | 0xB4 | fn_44_9B4, fn_44_A9DC |
| `fn_44_5A04` | 0xB4 | - |
| `fn_44_660C` | 0xB4 | fn_44_65C4 |
| `fn_44_C44` | 0xBC | fn_44_9B4, fn_44_A9DC |
| `fn_44_141C` | 0xBC | - |
| `_unresolved` | 0xC4 | - |
| `fn_44_16C4` | 0x108 | fn_44_14D8 |
| `fn_44_1830` | 0x11C | fn_44_14D8 |
| `fn_44_EB8` | 0x130 | fn_44_E08 |
| `fn_44_115C` | 0x13C | fn_44_1134 |
| `fn_44_204C` | 0x15C | fn_44_21A8, fn_44_22B0 |
| `fn_44_97E4` | 0x15C | - |
| `fn_44_A680` | 0x184 | fn_44_9D58 |
| `fn_44_14D8` | 0x188 | fn_44_110 |
| `fn_44_A9DC` | 0x19C | fn_44_9D58 |
| `fn_44_9940` | 0x1E4 | fn_44_97E4 |
| `fn_44_9B4` | 0x290 | fn_44_110 |
| `fn_44_1CE0` | 0x358 | fn_44_110 |
| `fn_44_194C` | 0x394 | fn_44_110 |
| `fn_44_7058` | 0x654 | - |
| `fn_44_110` | 0x844 | - |
| `fn_44_9D58` | 0x878 | fn_44_110 |
| `fn_44_ABCC` | 0x8B8 | - |

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
| `fn_44_6F98` | 0x8 | - |
| `fn_44_48` | 0xC | - |
| `fn_44_54` | 0xC | - |
| `fn_44_18` | 0x10 | - |
| `fn_44_38` | 0x10 | - |
| `fn_44_24C0` | 0x14 | - |
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
| `fn_44_1134` | 0x28 | fn_44_1114 |
| `fn_44_6720` | 0x28 | fn_44_6700 |
| `fn_44_70` | 0x2C | - |
| `fn_44_6FA0` | 0x2C | - |
| `fn_44_2EF4` | 0x30 | fn_44_2CD8 |
| `fn_44_2C9C` | 0x3C | - |
| `fn_44_6888` | 0x3C | - |
| `fn_44_68C4` | 0x3C | - |
| `fn_44_8AFC` | 0x3C | - |
| `fn_44_A644` | 0x3C | - |
| `fn_44_A804` | 0x3C | - |
| `fn_44_A8CC` | 0x3C | - |
| `fn_44_A908` | 0x3C | - |
| `fn_44_A944` | 0x3C | - |
| `fn_44_10D4` | 0x40 | fn_44_110 |
| `fn_44_66C0` | 0x40 | fn_44_660C |
| `fn_44_657C` | 0x48 | fn_44_655C |
| `fn_44_65C4` | 0x48 | fn_44_6384 |
| `fn_44_7918` | 0x48 | - |
| `ModuleDestructors` | 0x4C | - |
| `ModuleConstructors` | 0x4C | - |
| `fn_44_2984` | 0x50 | - |
| `fn_44_E64` | 0x54 | - |
| `fn_44_1304` | 0x54 | - |
| `fn_44_2930` | 0x54 | fn_44_28D8 |
| `fn_44_2FB0` | 0x54 | fn_44_2F24 |
| `fn_44_64AC` | 0x54 | fn_44_6384 |
| `fn_44_6834` | 0x54 | - |
| `fn_44_AB78` | 0x54 | fn_44_9D58 |
| `fn_44_107C` | 0x58 | - |
| `fn_44_28D8` | 0x58 | fn_44_25E4, fn_44_7058 |
| `fn_44_9700` | 0x58 | - |
| `fn_44_9BB8` | 0x58 | fn_44_97E4 |
| `fn_44_9C10` | 0x58 | fn_44_9BB8 |
| `fn_44_9C68` | 0x58 | fn_44_9C10 |
| `__destroy_global_chain` | 0x58 | - |
| `fn_44_E08` | 0x5C | fn_44_D54 |
| `fn_44_6500` | 0x5C | fn_44_64AC, fn_44_65C4 |
| `fn_44_78BC` | 0x5C | - |
| `fn_44_8404` | 0x5C | - |
| `fn_44_87E0` | 0x5C | - |
| `fn_44_A980` | 0x5C | - |
| `fn_44_954` | 0x60 | - |
| `fn_44_13BC` | 0x60 | - |
| `fn_44_1358` | 0x64 | - |
| `fn_44_1660` | 0x64 | - |
| `fn_44_17CC` | 0x64 | - |
| `fn_44_1298` | 0x6C | - |
| `fn_44_67C8` | 0x6C | - |
| `fn_44_A5D0` | 0x74 | fn_44_9D58 |
| `fn_44_57C4` | 0x80 | - |
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
  constructor is the host's own, and it calls four module functions that are not
  ported (`fn_44_AB78`, `fn_44_A5D0`, `fn_44_A680`, `fn_44_A9DC`), so it cannot
  carry a marker. The same holds for `fn_44_110` (the loader) and `fn_44_7058`
  (the massive death).
- The retail `RELMain` (the module entry at 0xC0) cannot be marked: asmcheck
  looks a function up by its `fn_<id>_<offset>` name or by a bare offset, and the
  module's entry is a named symbol.
- `fn_44_2550`'s DOL call (asm 0x2574, `fn_8022A2DC`) is the host's
  `CScriptAIHint::SetInUse(true)`; the names do not match, so asmcheck's CALL tag
  cannot pair them.
