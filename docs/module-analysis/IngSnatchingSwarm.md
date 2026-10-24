# IngSnatchingSwarm REL (module 33): analysis notes

The Ing swarm (ISSW) that pours out of a portal, flies a Bezier arc, possesses a CPatterned host
(`CanBeIngPossessed`, a valid `mIngModel`) or dives at the player. Code is upstream's
`src/MetroidPrime/Enemies/CIngSnatchingSwarm.cpp`; the loader is `SLdrIngSnatchingSwarm.hpp`
(defaults fixed by swsldr1). Retail asm: `build/G2ME01/IngSnatchingSwarm/asm/MetroidPrime/Enemies/CIngSnatchingSwarm.s`.
Per-function table: `IngSnatchingSwarm_functions.tsv` (B upstream body, H weak/implicit copy, T REL runtime).

## Survey and fixes (iss1, 2026-10-23)

Every sub-100% body was diffed normalized (labels, relocations, registers, constants resolved)
against upstream. No behaviour difference in: the SIngSnatchingSwarmData ctor (bitfields 0x98: 0x80
useSteering, 0x40 ignorePlayer), MoveTowards, CreateParticle, UpdateMovement (Min/Max NaN forms
match), ExitPortal, SelectTarget (cmpw operand order on an equality test), SetPortalSwarmPath,
CalculateArcLength, UpdateParticles (the ShouldLoiter arg is a stack 0.0) and LoadIngSnatchingSwarm.
Upstream's extra 4 .rodata bytes are the `zero$1779` static. Two fixes (`TARGET_PC`):

| where | retail | port before |
|---|---|---|
| ctor (0x4790 `addi r3,r25,0x374`, 0x4840 `bl Lock__6CTokenFv`) | locks the FSM2 token | never locked: Think never built the FSM |
| FollowArc (0x1830 `fcmpo f1(1.0),f31`, 0x1834 `bge`) | `1 < t ? 1 : t` | `t < 1 ? t : 1` (NaN-only difference) |

Header: the 0x73c flag byte is MSB-first (0x80 alive, 0x40 started, 0x20 checkCollision, 0x10
hitWorld; asm 0x990, 0xA48, 0xBA8), stated in comments for asmcheck.

## Spawns (disc scan of SCGN generated objects)

ISSW lives only in generated-object sections: Temple Grounds `10_temple_pathofeyes` (ISSW 000e018b
from GENR 000e018c, layer 2, fired by TIMR 000e0189), `03_temple_combattraining` (GENR 101901d2,
L4), Great Temple `07_temple_mothlordsanctuary`, Agon `05_portalpuzzle`, `03_sand_temple`, Torvus
`08_swamp_grappleguardian`, Sanctuary `09_cliff_temple` (GENR 82500e1 on TIMR 82500e4).
Console editor ids drop the layer bits: address the GENR as `e000e018c`.

## Driver

`tools/cov/specs/ingsnatchingswarm_pathofeyes.con` with spec `ingsnatchingswarm.py`, area
`3bfa3eff,10_temple_pathofeyes`, `--fast --frames 2400`: `send e000e018c SetToZero` generates a
swarm; part 1 activates pirates 000e0197/196/19b as hosts, part 2 kills them (SetPlayerDest from
afar hits the world, then `tp` + `place <swarm> player` for HitPlayer/DamagePlayer, `damage` 500
for the Damage arm/IsDead). One run covers all 27 marks; caught/reach-stub 0. No forced arms.

## Unmarked bodies

| function | missing arms (line numbers at iss1) |
|---|---|
| ctor 0x41CC, dtor 0x3D64 | optional/token null arms, deleting-dtor flag |
| Think, GetStateMachine, InitializeStateMachine | FSM token not loaded / already built |
| AcceptScriptMsg | kSM_AreaLoaded with ignorePlayer (off in the pathofeyes instance; other areas not checked) |
| Render | never called: the swarm draws through AddToRenderer particle gens |
| Touch, GetTouchBounds, UpdateTouchBounds | non-physics toucher, CCollisionActor owner, ignorePlayer, empty bounds |
| InSnatchingRange, DiveToTarget, SnatchTarget, SelectTarget | target lost mid-dive, possessed/invalid candidates |
| SetTargetDest, SetPlayerDest | path-find success (spline) arm, null target, zero direction |
| Explode | no explosion effect; zero hit normal |
| Dead | two arms (356, 368) untaken in this run; not investigated |
| CalculateArcLength, FollowArc | arc without 4 points, speed/duration <= 0, tiny step |
| UpdateParticles | one history arm |

## Coverage (iss2: the 8 rows left)

The TSV's marker column is stale; the 8 B rows without a `// @ fn_33_*` marker (counted with
port_progress.py's own marker rule) are the ones below. One natural run (iss1's driver, 2400 f) plus
one forcing run (`tools/cov/specs/ingsnatchingswarm_force.py` with `COV_GDB_SCRIPT=tools/cov/force_ret.py`,
same driver), both `caught/reach-stub: 0`.

| fn | state |
|---|---|
| 0 HealthInfo | marked: objdiff 100% (`li r3,0x10(r3) / blr` = `&mHealthInfo`) |
| 6D4 ~SLdrIngSnatchingSwarm | marked: objdiff 100% |
| 1908 CalculateArcLength | marked: **ALL-ARMS** after forcing `mArcPoints.mCount = 0` on every other call (size != 4) |
| F7C UpdateParticles | marked: **ALL-ARMS** after stubbing `CMain::IsMaxSpeed()` to 1 for half the calls (FORCE_INNER, `$rax`) |
| 17C8 FollowArc | 7/8: only 670:1/2 (`return t < 1.f` false) left. Two forcing runs: size != 4, speed <= 0 and duration <= 0 all taken, but neither `$xmm0 = 1e6` (time) nor `mArcLength = 0.001` (a tiny duration, so t = time/duration >> 1) took the arc-end arm - it also needs the body to reach the return, which the arc-end hit apparently does not |
| 29A8 SelectTarget | 35/40: 475:1/6, 475:5/6, 476:3/6, 499:1/2, 506:3/4. The `CanBeIngPossessed` -> 0 and `IsIngPossessed` -> 1 FORCE_INNER stubs fired (`forced.log`) but the arms they gate stayed untaken, and the best-candidate arms need two candidates with different swarm counts - next attempt: a second swarm (GENR) or a `loc:` write of `candidates[j].mSwarmCount` |
| 41CC ctor | 29/38: 143/145/147 (the `mExplosionEffect != kInvalidAssetId` ternary, data: the shipped GENR always sets it) and 183 (the mStateMachineToken setup) |
| 744 SLdrIngSnatchingSwarm ctor | not byte-identical (objdiff 72.9%): GCC inlines the sub-struct ctors MWCC kept out of line; the defaults are the ones swsldr1 fixed |
