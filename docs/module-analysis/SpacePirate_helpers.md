# SpacePirate (module 72) - the stateless helper half

The module's note (docs/module-analysis/SpacePirate.md) and its function table
(docs/module-analysis/SpacePirate_functions.tsv) are the module session's. This note is the
helper lane's slice: the **B rows that are neither an FSM state nor an FSM trigger**, the bodies
that were ported into `src/MetroidPrime/Enemies/PortCSpacePirateHelpers.cpp`, and the arm tables
of the ones a run proved.

The split: the module session (branch `decomp/spacepirate`) owns the FSM states and triggers
(`PortCSpacePirateStates.cpp`, `PortCSpacePirateTriggers.cpp`) and the module's own file
(`PortCSpacePirate.cpp`); this lane owns the stateless helper half
(`PortCSpacePirateHelpers.cpp`). The retail offsets are the REL's `.text` offsets
(`build/G2ME01/SpacePirate/asm/auto_00_00000000_text.s`); the classification (B/H/T) is the
module session's, taken from its function table.

## 1. The B rows that are not states or triggers

74 rows. `mine` = the body is in `PortCSpacePirateHelpers.cpp` (this lane); `module` = the body is
in one of the module session's files; `header` = the body is an inline in `CSpacePirate.hpp`.

| offset | size | retail function | kind | body | marker |
|---|---|---|---|---|---|
| 0x0 | 0x8 | IsScanVisorSelfRender (`li r3, 1`) | virtual | header | see 3 |
| 0x8 | 0x8 | GetSearchPath (`addi r3, r3, 0x920`) | virtual | header | marked (module) |
| 0x10 | 0x8 | IsListening (`li r3, 1`) | virtual | header | marked (module) |
| 0x18 | 0xC | GetAttackTarget (`lhz r0, 0xa94(r4)`) | virtual | header | see 3 |
| 0x24 | 0xC | GetGravityConstant (`lfs f1, rodata B00`) | virtual | header | see 3 |
| 0x90 | 0x24 | RELExit | rel entry | the host's REL runtime | - |
| 0xB4 | 0x20 | RELMain | rel entry | the host's REL runtime | - |
| 0xD4 | 0x6C | RegisterFuncPtrs (SSpacePirate_FuncPtrs) | rel entry | the host's module registry (`platform/compiled_modules.cpp`) | - |
| 0x140 | 0xACC | LoadSpacePirate | loader | module (`PortLoadSpacePirate`) | - (the host's loader, not retail's body) |
| 0xC0C | 0x94 | CSpacePirateData::BuildGrenadeData | method | header (`BuildGrenadeData()`) | - |
| 0x17A0 | 0x28 | GetRagDoll (`lwz r3, 0xb30(r3)`) | virtual | header | see 3 |
| 0x17C8 | 0x8C | GetTouchBounds | virtual | module | marked (module) |
| 0x1854 | 0x68 | GetTouchBounds helper (the actor's bounding box) | method | **mine** | see 3 |
| 0x18BC | 0x74 | TryToBeCaptured | virtual | **mine** | see 3 |
| 0x1980 | 0x6E0 | Think | virtual | module | marked (module) |
| 0x24C4 | 0x78 | PreThink | virtual | module | marked (module) |
| 0x253C | 0x310 | UpdateAttacks (Think helper) | method | module | marked (module) |
| 0x28F0 | 0x5B0 | UpdateAimBodyState (Think helper) | method | module | marked (module) |
| 0x2EA8 | 0xA4 | the sfx emitter update (Think helper) | method | **mine** | see 3 |
| 0x2F4C | 0x2D4 | UpdateCloak (Think helper) | method | module | marked (module) |
| 0x3220 | 0x17C | ShouldFrenzy (from UpdateAttacks) | method | **mine** | see 3 |
| 0x339C | 0xA0 | ScanVisorRender | virtual | module | marked (module) |
| 0x343C | 0x190 | GetScanVisorRenderBounds | virtual | module | marked (module) |
| 0x35CC | 0x100 | GetSortingBounds | virtual | module | marked (module) |
| 0x36CC | 0xF8 | RenderLauncherModel | method | module | marked (module) |
| 0x37C4 | 0x120 | Render | virtual | module | marked (module) |
| 0x38E4 | 0x128 | PreRender | virtual | module | marked (module) |
| 0x3A0C | 0x7C | PreRenderAllViewports | virtual | module | marked (module) |
| 0x3A88 | 0x2C | GetDamageVulnerability(pos, dir, info) | virtual | module | marked (module) |
| 0x3AB4 | 0x34 | GetDamageVulnerability() | virtual | module | marked (module) |
| 0x3AE8 | 0x40 | GetModelAlphau8 | virtual | module | marked (module) |
| 0x3B28 | 0x254 | DoUserAnimEvent | virtual | module | marked (module) |
| 0x3D7C | 0x2B4 | SolveGrenadeLaunch | method | **mine** | see 3 |
| 0x4030 | 0x144 | GetGrenadeAimPoint | method | **mine** | see 3 |
| 0x4174 | 0x3B8 | SpawnGrenade | method | **mine** | see 3 |
| 0x45F8 | 0x4C0 | FireProjectile | method | module | marked (module) |
| 0x4F0C | 0x1C | ProjectileInfo (`&xbd4`) | virtual | module | marked (module) |
| 0x4F28 | 0x9C | InitLauncherModel | method | module | marked (module) |
| 0x51C8 | 0xB8 | TrooperWarpOut | method | **mine** | see 3 |
| 0x5280 | 0x78 | the leash / frozen check | method | module | marked (module) |
| 0xC538 | 0xF0 | Death | virtual | module | marked (module) |
| 0xC684 | 0x358 | KnockBack | virtual | module | marked (module) |
| 0xC9DC | 0xB8 | SetNonCinematicCollision | method | **mine** | see 3 |
| 0xCA94 | 0x9C | SetCinematicCollision | method | **mine** | see 3 |
| 0xCB30 | 0x150 | GetTargetPos | method | module | marked (module) |
| 0xCC80 | 0x1B0 | CheckBlade | method | module | marked (module) |
| 0xCE30 | 0x470 | GetStrafeDir | method | **mine** | see 3 |
| 0xD2A0 | 0x16C | CantJumpBack | method | module | marked (module) |
| 0xD40C | 0x1FC | AvoidActors | method | module | marked (module) |
| 0xD608 | 0x118 | ShuffleRandomVector | method | **mine** | see 3 |
| 0xD720 | 0xE0 | UpdateHeldPosition | method | module | marked (module) |
| 0xD800 | 0x348 | UpdateGuardPosition | method | module | marked (module) |
| 0xDB48 | 0x8C | LineOfSightRay | method | module | marked (module) |
| 0xDBD4 | 0x288 | CheckForProjectiles | method | module | marked (module) |
| 0xDE5C | 0x80 | UpdateTeamTarget | method | module | marked (module) |
| 0xDEDC | 0x50 | SquadReset | method | module | marked (module) |
| 0xDF2C | 0xA8 | SquadAdd | method | module | marked (module) |
| 0xDFD4 | 0xBC | SquadJoin | method | module (states) | - (the module session's file) |
| 0xE090 | 0x1C | GetOrigin | virtual | module | marked (module) |
| 0xE0AC | 0x2C | DetachActorFromPirate | method | module | marked (module) |
| 0xE0D8 | 0x48 | AttachActorToPirate | method | module | marked (module) |
| 0xE120 | 0x48 | SetAttackTarget | virtual | module | marked (module) |
| 0xE168 | 0x174 | SetVelocityForJump | method | module | marked (module) |
| 0xE2DC | 0x38 | CheckTargetable | method | module | marked (module) |
| 0xE314 | 0x4 | the empty function (Dead, AcceptScriptMsg) | method | - | - (a `blr` only; the port does not need the symbol) |
| 0xE318 | 0xC0 | Listen | virtual | module | marked (module) |
| 0xE3D8 | 0xC0 | Touch | virtual | **mine** | see 3 |
| 0xE498 | 0x944 | AcceptScriptMsg | virtual | module | marked (module) |
| 0xEDEC | 0x1E8 | ChooseTarget | method | **mine** | see 3 |
| 0xEFD4 | 0xCC | ChooseTeamTarget | method | **mine** | see 3 |
| 0xF0A0 | 0x6C | SetupStateMachine | virtual | module | marked (module) |
| 0xF49C | 0xDBC | the ctor | method | module | - |
| 0x10A10 | 0x254 | CSpacePirateData(SLdr) ctor | ctor | module (the loader fills the struct) | - |
| 0x10C64 | 0x80 | the launcher block ctor | ctor | module (the loader fills the struct) | - |
| 0x10CE4 | 0x40 | the module's CValidEntityPredicate (a CScriptWaypoint) | virtual | **mine** | see 3 |

## 2. What this lane changed

`src/MetroidPrime/Enemies/PortCSpacePirateHelpers.cpp` (new) holds the helper half: the bodies
moved out of the module session's files plus the ones that were missing.

- **ported here** (moved, bodies unchanged): `SetCinematicCollision` (0xCA94),
  `SetNonCinematicCollision` (0xC9DC), `ChooseTarget` (0xEDEC), `ChooseTeamTarget` (0xEFD4),
  `ShouldFrenzy` (0x3220), `TrooperWarpOut` (0x51C8), `Touch` (0xE3D8), `TryToBeCaptured`
  (0x18BC), `SolveGrenadeLaunch` (0x3D7C), `GetGrenadeAimPoint` (0x4030), `SpawnGrenade` (0x4174),
  `ShuffleRandomVector` (0xD608), `GetStrafeDir` (0xCE30).
- **ported here** (new): the sfx emitter update (0x2EA8), the touch-bounds helper (0x1854) and the
  module's waypoint predicate (0x10CE4).

The module session's files lost those definitions and nothing else: the call sites are unchanged
(names, arguments), except

- `PortCSpacePirate.cpp`'s Think calls the new `UpdateSfxEmitter()` (retail asm 0x1B7C, right
  after the cloak update at 0x1B74);
- `PortCSpacePirate.cpp`'s GetTouchBounds returns `GetActorTouchBounds()` (retail asm 0x183C calls
  fn_72_1854);
- `ShuffleRandomVector` became a member (the retail's fn_72_D608 is one), so the header declares it
  in the new `#ifdef TARGET_PC // helpers` block.

## 3. The arm tables

The bodies this lane marked, with the arms of the retail asm and the run that proved them. The
coverage harness is `tools/cov/run.sh` with `tools/cov/specs/spacepirate_helpers.py` (the module's
own spec plus the helper half) and the driver `tools/drivers/spacepirate_helpers_drive.py` (the
module's driver under a new name). The rare arms are forced with gdb: a `tbreak` on a live pirate
and one call each, the command in brackets.

| body | arms (retail asm) | the run |
|---|---|---|
| `0` IsScanVisorSelfRender | no branch (`li r3, 1`, asm 0x0-0x4) | forced (gdb `p ((CSpacePirate*)$rdi)->IsScanVisorSelfRender()` -> true) |
| `18` GetAttackTarget | no branch (`lhz r0, 0xa94(r4)`, asm 0x18-0x20) | forced (gdb `p ...->GetAttackTarget()` -> 0xffff) |
| `24` GetGravityConstant | no branch (rodata 0xB00 = 50.0, asm 0x24-0x2C) | forced (gdb `p ...->GetGravityConstant()` -> 50) |
| `1854` GetActorTouchBounds | no branch (the box out of GetBoundingBox, asm 0x1868; the valid flag, asm 0x1870) | natural: 400 hits, 08_sand_casualpirates |
| `18BC` TryToBeCaptured | no branch (the "Captured" state, asm 0x18D8; `li r3, 1`, asm 0x1918) | forced (gdb `p ...->TryToBeCaptured(mgr)` -> true) |
| `C9DC` SetNonCinematicCollision | no branch (asm 0xC9DC-0xCA90) | forced (gdb `p ...->SetNonCinematicCollision(mgr)`) |
| `CA94` SetCinematicCollision | no branch (asm 0xCA94-0xCAF0) | forced (gdb `p ...->SetCinematicCollision(mgr)`) |
| `2EA8` UpdateSfxEmitter | the handle at 0xc00 is 0 (asm 0x2EC0) 11468 entries; the handle plays (asm 0x2ED4) 45; the handle is queued but silent (asm 0x2EEC) 3; the handle is neither (asm 0x2EF4 -> 0x2F30) 1 - the handle is cleared | the handle is the retail's own: the KnockBack stores the hurled / death sound's handle at 0xc00 (asm 0xC944 / 0xC9B8), which the port now does; the sound and the dead handle are forced (gdb: `CSfxManager::AddEmitter` of the pirate's hurled sound into 0xc00, and a bogus handle 0x3039 into 0xc00) |

Marked: 8 bodies. The module goes from 95 to **103 of 262** marked
(`tools/port_progress.py`).

Not marked, and why:

| body | what is missing |
|---|---|
| `17A0` GetRagDoll | the ragdoll (0xb30) is never set in the port, so the "primed and not over" arm (asm 0x17AC-0x17BC) cannot run |
| `3220` ShouldFrenzy | the run armed the frenzy (the true result) and left it unarmed (the false one), but the four inner arms (the pending-chance bit, the charge-player list, the morphed target, the low-health path) are not all traced |
| `51C8` TrooperWarpOut | 0 calls: the "WarpOut" script message never reached a trooper in the runs |
| `3D7C` / `4030` / `4174` the grenade bodies | the forced call runs them (the no-target arm and the whole spawn), but the solve's arms (the candidate loop's accept / closest-miss paths) are not all traced; the two helpers are inlined into the spawn, so they carry no symbol of their own |
| `CE30` GetStrafeDir | 28 branches in the asm; the run's calls do not cover them all |
| `D608` ShuffleRandomVector | 0 calls: the Shuffle state's "too close" branch (which is the only caller) never ran |
| `EDEC` ChooseTarget / `EFD4` ChooseTeamTarget | the arms that need a squad (a team the pirate belongs to) did not run: the areas' team AI managers do not give the pirates a team |
| `E3D8` Touch | the ragdoll's push (asm 0xE3F8-0xE47C) needs a ragdoll, which the port never creates |
| `E314` the empty function | a `blr` only; the port does not need the symbol |

## 4. The coverage runs

All runs: `tools/cov/run.sh tools/cov/specs/spacepirate_helpers.py <out> <world,area> --driver
"python3 tools/drivers/spacepirate_helpers_drive.py {port} <layer> 6" --frames 14000 --fast --env
MP2_DEBUG_SPACEPIRATE=1`, 0 `[port] caught` and 0 `[reach-stub]` in every one.

| run | area | what it proved |
|---|---|---|
| 1 | 08_sand_casualpirates | the helper half's entry counts: `1854` 400 hits, `2EA8` 4557, `3220` 149 (the false result), `CE30` 400, `EDEC` 400, `EFD4` 8 |
| 2 | 07_sand_alamoportalobservation L3 | `3220` 17 hits, all the true result (the frenzy armed); `EFD4` 14 hits with the squad's cast and the in-range test |
| 3 | 08_sand_casualpirates, after the 0xc00 handle change | the same, with the emitter's handle: the handle was still 0 in every entry - a pirate is never knocked back while it is alive in these areas, and the death sound's handle is not stored (the death's follow-up is an explode) |
| 4 | 08_sand_casualpirates | as 3, with the call sites counted |
| 5 | 07_sand_alamoportalobservation L3 | as 3, with the call sites counted |
| 6 | 08_sand_casualpirates | the final binary: `3220` both results (16 true, 1 false), the rest as 1 |
| forcing | 08_sand_casualpirates (gdb) | the rare arms: the four vtable helpers, TryToBeCaptured, the two collision helpers, the emitter's handle and the dead handle |

Runs 1, 2 and 3 (and the final run) ended with the game exiting normally; runs 4 and 5 were cut off
by the harness' 900 s limit - the machine was busy and the `calls` entries slow the game down - but
their counts are complete up to the cut (the table is rewritten every 15 s).

The `calls` counts of the harness are polluted on this build (the call-site breakpoints of one
label also count the calls of the neighbouring functions), so the arm counts of the emitter were
read from a run whose spec has that one entry only.

## 5. What the port was missing (and now has)

- The retail's KnockBack stores the sfx emitter's handle at 0xc00 (asm 0xC944 for the hurled sound,
  0xC9B8 for the death sound). The port dropped the handle, so the emitter helper (0x2EA8) could
  never do anything. The port stores it now (`xc00_` is a `CSfxHandle`).
- The retail's ShouldFrenzy (0x3220) also arms the frenzy while the module's charge-player list
  (bss_40) is not empty (asm 0x3298-0x32A4). The port had that as a comment; it is code now.

