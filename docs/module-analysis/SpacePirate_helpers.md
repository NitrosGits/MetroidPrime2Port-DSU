# SpacePirate (module 72) - the stateless helper half

The module's note (docs/module-analysis/SpacePirate.md) and its function table
(docs/module-analysis/SpacePirate_functions.tsv) are the module session's. This note is the
helper lane's slice: the **B rows that are neither an FSM state nor an FSM trigger**, the bodies
this lane added, and the arm tables of the ones a run proved.

The split: the module session (branch `decomp/spacepirate`) owns the FSM states and triggers
(`PortCSpacePirateStates.cpp`, `PortCSpacePirateTriggers.cpp`) and the module's own file
(`PortCSpacePirate.cpp`). This lane only **adds** the bodies that are missing; it never moves an
existing one. The three bodies it added live in `src/MetroidPrime/Enemies/PortCSpacePirateHelpers.cpp`
(`tools/modules_port.txt` + the generated `files.cmake`). The retail offsets are the REL's `.text`
offsets (`build/G2ME01/SpacePirate/asm/auto_00_00000000_text.s`); the classification (B/H/T) is the
module session's, taken from its function table.

## 1. The B rows that are not states or triggers

74 rows. `added` = the body is in `PortCSpacePirateHelpers.cpp` (this lane); `module` = the body is
in one of the module session's files; `header` = the body is an inline in `CSpacePirate.hpp`.

| offset | size | retail function | kind | body | marker |
|---|---|---|---|---|---|
| 0x0 | 0x8 | IsScanVisorSelfRender (`li r3, 1`) | virtual | header | this lane (section 3) |
| 0x8 | 0x8 | GetSearchPath (`addi r3, r3, 0x920`) | virtual | header | marked (module) |
| 0x10 | 0x8 | IsListening (`li r3, 1`) | virtual | header | marked (module) |
| 0x18 | 0xC | GetAttackTarget (`lhz r0, 0xa94(r4)`) | virtual | header | this lane (section 3) |
| 0x24 | 0xC | GetGravityConstant (`lfs f1, rodata B00`) | virtual | header | this lane (section 3) |
| 0x90 | 0x24 | RELExit | rel entry | the host's REL runtime | - |
| 0xB4 | 0x20 | RELMain | rel entry | the host's REL runtime | - |
| 0xD4 | 0x6C | RegisterFuncPtrs (SSpacePirate_FuncPtrs) | rel entry | the host's module registry (`platform/compiled_modules.cpp`) | - |
| 0x140 | 0xACC | LoadSpacePirate | loader | module (`PortLoadSpacePirate`) | - (the host's loader, not retail's body) |
| 0xC0C | 0x94 | CSpacePirateData::BuildGrenadeData | method | header (`BuildGrenadeData()`) | - |
| 0x17A0 | 0x28 | GetRagDoll (`lwz r3, 0xb30(r3)`) | virtual | header | - (the ragdoll arm is dead) |
| 0x17C8 | 0x8C | GetTouchBounds | virtual | module | marked (module) |
| 0x1854 | 0x68 | GetTouchBounds helper (the actor's bounding box) | method | **added** | this lane (section 3) |
| 0x18BC | 0x74 | TryToBeCaptured | virtual | module | this lane (section 3) |
| 0x1980 | 0x6E0 | Think | virtual | module | marked (module) |
| 0x24C4 | 0x78 | PreThink | virtual | module | marked (module) |
| 0x253C | 0x310 | UpdateAttacks (Think helper) | method | module | marked (module) |
| 0x28F0 | 0x5B0 | UpdateAimBodyState (Think helper) | method | module | marked (module) |
| 0x2EA8 | 0xA4 | the sfx emitter update (Think helper) | method | **added** | this lane (section 3) |
| 0x2F4C | 0x2D4 | UpdateCloak (Think helper) | method | module | marked (module) |
| 0x3220 | 0x17C | ShouldFrenzy (from UpdateAttacks) | method | module | - (the inner arms are not all traced) |
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
| 0x3D7C | 0x2B4 | SolveGrenadeLaunch | method | module | - (the solve's arms are not all traced) |
| 0x4030 | 0x144 | GetGrenadeAimPoint | method | module | - (the arms are not all traced) |
| 0x4174 | 0x3B8 | SpawnGrenade | method | module | - (the arms are not all traced) |
| 0x45F8 | 0x4C0 | FireProjectile | method | module | marked (module) |
| 0x4F0C | 0x1C | ProjectileInfo (`&xbd4`) | virtual | module | marked (module) |
| 0x4F28 | 0x9C | InitLauncherModel | method | module | marked (module) |
| 0x51C8 | 0xB8 | TrooperWarpOut | method | module | this lane (section 3) |
| 0x5280 | 0x78 | the leash / frozen check | method | module | marked (module) |
| 0xC538 | 0xF0 | Death | virtual | module | marked (module) |
| 0xC684 | 0x358 | KnockBack | virtual | module | marked (module) |
| 0xC9DC | 0xB8 | SetNonCinematicCollision | method | module | this lane (section 3) |
| 0xCA94 | 0x9C | SetCinematicCollision | method | module | this lane (section 3) |
| 0xCB30 | 0x150 | GetTargetPos | method | module | marked (module) |
| 0xCC80 | 0x1B0 | CheckBlade | method | module | marked (module) |
| 0xCE30 | 0x470 | GetStrafeDir | method | module | - (28 branches, not all traced) |
| 0xD2A0 | 0x16C | CantJumpBack | method | module | marked (module) |
| 0xD40C | 0x1FC | AvoidActors | method | module | marked (module) |
| 0xD608 | 0x118 | ShuffleRandomVector | method | module | - (0 calls in the runs) |
| 0xD720 | 0xE0 | UpdateHeldPosition | method | module | marked (module) |
| 0xD800 | 0x348 | UpdateGuardPosition | method | module | marked (module) |
| 0xDB48 | 0x8C | LineOfSightRay | method | module | marked (module) |
| 0xDBD4 | 0x288 | CheckForProjectiles | method | module | marked (module) |
| 0xDE5C | 0x80 | UpdateTeamTarget | method | module | marked (module) |
| 0xDEDC | 0x50 | SquadReset | method | module | marked (module) |
| 0xDF2C | 0xA8 | SquadAdd | method | module | marked (module) |
| 0xDFD4 | 0xBC | SquadJoin | method | module | this lane (section 3) |
| 0xE090 | 0x1C | GetOrigin | virtual | module | marked (module) |
| 0xE0AC | 0x2C | DetachActorFromPirate | method | module | marked (module) |
| 0xE0D8 | 0x48 | AttachActorToPirate | method | module | marked (module) |
| 0xE120 | 0x48 | SetAttackTarget | virtual | module | marked (module) |
| 0xE168 | 0x174 | SetVelocityForJump | method | module | marked (module) |
| 0xE2DC | 0x38 | CheckTargetable | method | module | marked (module) |
| 0xE314 | 0x4 | the empty function (Dead, AcceptScriptMsg) | method | - | - (a `blr` only; the port does not need the symbol) |
| 0xE318 | 0xC0 | Listen | virtual | module | marked (module) |
| 0xE3D8 | 0xC0 | Touch | virtual | module | - (the ragdoll's push is not ported) |
| 0xE498 | 0x944 | AcceptScriptMsg | virtual | module | marked (module) |
| 0xEDEC | 0x1E8 | ChooseTarget | method | module | - (the arms are not all traced) |
| 0xEFD4 | 0xCC | ChooseTeamTarget | method | module | this lane (section 3) |
| 0xF0A0 | 0x6C | SetupStateMachine | virtual | module | marked (module) |
| 0xF49C | 0xDBC | the ctor | method | module | - |
| 0x10A10 | 0x254 | CSpacePirateData(SLdr) ctor | ctor | module (the loader fills the struct) | - |
| 0x10C64 | 0x80 | the launcher block ctor | ctor | module (the loader fills the struct) | - |
| 0x10CE4 | 0x40 | the module's CValidEntityPredicate (a CScriptWaypoint) | virtual | **added** | - (the host has no predicate parameter) |

## 2. What this lane added

`src/MetroidPrime/Enemies/PortCSpacePirateHelpers.cpp` (new; `tools/modules_port.txt` and the
generated `files.cmake`):

- **fn_72_2EA8** `UpdateSfxEmitter()` - the Think helper that keeps the pirate's sfx emitter
  (the handle at 0xc00) where the pirate is. The Think calls it (retail asm 0x1B7C).
- **fn_72_1854** `GetActorTouchBounds()` - the actor's own bounding box in the optional that
  GetTouchBounds returns (retail asm 0x183C calls it).
- **fn_72_10CE4** `CSpacePirateWaypointPredicate` - the module's own CValidEntityPredicate: the
  object with the id has to be a CScriptWaypoint. The host's `CheckConnectedObject` takes no
  predicate, so the WallHang state (the module's file) casts the object itself; this is the
  retail's predicate, ready for a host that takes one.

The header gets an append-only `#ifdef TARGET_PC // helpers` block with the two new declarations.
`xc00_` becomes a `CSfxHandle` (four bytes, the layout is unchanged) because the emitter's handle
is a handle, not an int.

Nothing was moved: the module session's files are byte for byte the branch's own text apart from
the markers below, the two call sites of the new bodies and the ctor's handle init.

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
| `2EA8` UpdateSfxEmitter | the handle at 0xc00 is 0 (asm 0x2EC0) 11468 entries; the handle plays (asm 0x2ED4) 45; the handle is queued but silent (asm 0x2EEC) 3; the handle is neither (asm 0x2EF4 -> 0x2F30) 1 - the handle is cleared | the handle is the retail's own: the KnockBack stores the hurled / death sound's handle at 0xc00 (asm 0xC944 / 0xC9B8) - the port drops it (section 5), so the playing / queued entries are forced (gdb: `CSfxManager::AddEmitter` of the pirate's hurled sound into 0xc00) and the dead handle is forced (gdb: a bogus handle 0x3039 into 0xc00) |
| `51C8` TrooperWarpOut | the actor is alive (asm 0x51F4), the 0x8fa bit 25 clear (asm 0x5200), the warp point -1 (asm 0x520C) and the 0x8f6 bit 24 set (asm 0x5218) -> the "WarpOut" state (asm 0x521C); each of the four tests false ends the body | forced (tools/cov/force.sh, the spec's 51C8 entry): one call per arm, the keys `1, 0, -1, 1` (the true arm), `1, 0, -1, 0` (not a trooper), `0, 0, -1, 0` (not alive), `1, 1, -1, 0` (the 0x8fa bit 25), `1, 0, 5, 0` (a warp point) - the kSM_SetToZero message reaches the body (the port's AcceptScriptMsg, asm 0x5200) |
| `EFD4` ChooseTeamTarget | no squad (0xba0 == 0xffff, asm 0xF010); a squad id that resolves to nothing (asm 0xF02C); a squad that is out of range (asm 0xF04C); a squad in range (asm 0xF060) | forced (the same spec's EFD4 entry, 07_sand_alamoportalobservation L3): the keys 0xffff (1 call), 0x7ffe (1 call) and the layer's team id 1427 (3 calls); the call counts: `ChoosePlayer` 4, `IsTeamMemberInRange` 3, `FindBestIndividualAttackTarget` 1 - the driver puts the layer's other pirates next to the actor and then far from it, so the range test takes both ways |
| `DFD4` SquadJoin | the squad id is invalid -> the team is looked up (asm 0xE000); it is still invalid -> the body ends (asm 0xE02C); the id resolves to no team -> the body ends (asm 0xE04C); the 0x8f4 bit 27 (the melee role) -> kTAR_Melee (asm 0xE060) or kTAR_Projectile (asm 0xE05C) | forced (the same spec's DFD4 entry): the keys 0xffff,0 (3 calls), 0xffff,1 (1 call, the melee role) and 0x7ffe,0 (1 call); the call counts: `GetAssociatedTeamId` 4, `ObjectById` 3, `TCastToPtr<CScriptTeamAiMgr>` 3, `JoinTeam` 2 (the melee and the projectile role) |

Marked: 11 bodies (`0`, `18`, `24`, `1854`, `18BC`, `C9DC`, `CA94`, `2EA8`, `51C8`, `EFD4`,
`DFD4`) - the module goes 106 -> 117 of 262 marked (`tools/port_progress.py`).

Not marked, and why:

| body | what is missing |
|---|---|
| `17A0` GetRagDoll | the ragdoll (0xb30) is never set in the port, so the "primed and not over" arm (asm 0x17AC-0x17BC) cannot run |
| `3220` ShouldFrenzy | the run armed the frenzy (the true result) and left it unarmed (the false one), but the four inner arms (the pending-chance bit, the charge-player list, the morphed target, the low-health path) are not all traced |
| `51C8` TrooperWarpOut | 0 calls: the "WarpOut" script message never reached a trooper in the runs |
| `3D7C` / `4030` / `4174` the grenade bodies | a forced call runs them (the no-target arm and the whole spawn), but the solve's arms (the candidate loop's accept / closest-miss paths) are not all traced |
| `CE30` GetStrafeDir | 28 branches in the asm; the run's calls do not cover them all |
| `D608` ShuffleRandomVector | 0 calls: the Shuffle state's "too close" branch (which is the only caller) never ran |
| `EDEC` ChooseTarget | the port's body is not retail's yet (section 5): the retail's "a team target was chosen" flag (asm 0xEF48) and its squad branch (asm 0xEF50-0xEFA4) are missing, so the body cannot be marked |
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
| 7 | 08_sand_casualpirates | the merged branch (port + decomp/spacepirate 0cb54aef2): the game exits with code 0, 0 `[port] caught`, 0 `[reach-stub]`; `1854` 400 hits, `2EA8` 400 entries, `EDEC` 400, `E3D8` 400 |
| forcing | 08_sand_casualpirates (gdb) | the rare arms: the four vtable helpers, TryToBeCaptured, the two collision helpers, the emitter's handle and the dead handle |
| forcing 2 | 08_sand_casualpirates (tools/cov/force.sh) | `51C8` TrooperWarpOut: one call per arm, the kSM_SetToZero message (`SP_SETZERO=14`) |
| forcing 3 | 07_sand_alamoportalobservation L3 (tools/cov/force.sh + cov_force.py) | `EFD4` ChooseTeamTarget and `DFD4` SquadJoin: the squad id and the melee role are written at the entry, the driver's team phase puts the layer's other pirates next to the actor and then far from it (`SP_TEAM_PHASE=1`) |

Runs 1, 2 and 3 (and the final run) ended with the game exiting normally; runs 4 and 5 were cut off
by the harness' 900 s limit - the machine was busy and the `calls` entries slow the game down - but
their counts are complete up to the cut (the table is rewritten every 15 s).

The `calls` counts of the harness are polluted on this build (the call-site breakpoints of one
label also count the calls of the neighbouring functions), so the arm counts of the emitter were
read from a run whose spec has that one entry only.

## 5. Three gaps in the module's own files (reported, not changed)

All three are in `PortCSpacePirate.cpp` / `PortCSpacePirateStates.cpp`, the module session's files,
so this lane only reports them:

- The retail's KnockBack stores the sfx emitter's handle at 0xc00 - the hurled sound (asm 0xC93C's
  `AddEmitter` of the data's sound at 0x878, the handle stored at 0xC944) and the death sound
  (asm 0xC9B0, the handle at 0xC9B8). The port calls `CSfxManager::AddEmitter` and drops the
  handle, so fn_72_2EA8 can never see one; the handle is a `CSfxHandle` now, so storing it is a
  one-line change in each of the two branches.
- The retail's ShouldFrenzy (0x3220) also arms the frenzy while the module's charge-player list
  (bss_40, asm 0x3298-0x32A4) is not empty. The port has that as a comment; the list is the
  module's `mChargePlayerList`, so the test is `!mChargePlayerList.empty()`.
- The retail's ChooseTarget (0xEDEC) has two pieces the port's body misses: the "a team target was
  chosen" flag (the retail sets r30 at asm 0xEE74 and keeps the team target at asm 0xEF48 - the
  port lets the nearest-player swap at the end overwrite the team target) and the squad branch
  (asm 0xEF50-0xEFA4: a valid squad whose member is within 20 units returns the attack target).
  The port's body is therefore not retail's, which is why it carries no marker.
