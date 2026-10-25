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
| 17C8 FollowArc | **marked in ldr3** (see below); the "670:1/2" arm here was really `length > eps` (now 674; `t < 1.f` is a `setcc`, no branch) |
| 29A8 SelectTarget | **marked in ldr3** (see below) |
| 41CC ctor | **marked in ldr3** (see below) |
| 744 SLdrIngSnatchingSwarm ctor | **marked in ldr2** (see below): 26 stores + 3 member ctors, no branch; ran 15x |

### ldr3: fn_33_41CC ctor, fn_33_17C8 FollowArc, fn_33_29A8 SelectTarget (marked)

No callee was missing: the asmcheck "unported" calls in the ctor (fn_33_4DD8 `CModelData::None`,
fn_33_4AC4 `SIngSnatchingSwarmData` copy) are implicit/weak instances the C++ names as
`CModelData::None()` and `mData(data)`. What blocked the rows was coverage, so the missing arms
were forced one-shot at their compares by `tools/cov/specs/ldr3_iss_verify.py`. Each site arms at
the compare and restores at the very next conditional jump, with no other branch in between (the
dry run listed both instructions for all 11 sites on the frozen binary). It checks the thread and
`$rsp` are unchanged between the two, logs the original, forced and readback values, and never
calls the inferior. Line numbers here are the current file's; offsets are x86 offsets from the
symbol in the frozen cov binary (febd723c...).

| site | x86 compare / jump | line | forced |
|---|---|---|---|
| ctor explosion invalid | +0x32f `cmp $-1,%eax` / +0x332 `je` | 145 | forced (`$rax = 0xffffffff`) |
| select inactive | +0x79 `testb $1,0x30(%rax)` / +0x7d `je` | 477 | forced (byte &= ~1) |
| select other area | +0x9f `cmp %eax,0x8(%r15)` / +0xa3 `je` | 478 | forced (`$eax + 1`) |
| select possessed | +0x378 `test %al,%al` / +0x37a `jne` | 480 | forced (`al = 1`) |
| select entity null | +0x246 `test %rax,%rax` / +0x249 `je` | 501 | forced (`$rax = 0`) |
| select closer | +0x29a `comiss %xmm0,%xmm3` / +0x29d `jbe` | 508 | forced (`xmm0 = -1`) |
| select count differs | +0x2a8 `cmp %ebx,%eax` / +0x2aa `jne` | 508 | forced (`$eax = $ebx + 1`) |
| arc size | +0x21 `cmpl $4,0x534(%rdi)` / +0x28 `je` | 660 | forced (`mArcPoints.mCount = 0`) |
| arc speed | +0x5f `comiss %xmm1,%xmm2` / +0x62 `jbe` | 660 | forced (`xmm2 = 0`) |
| arc duration | +0x7d `comiss %xmm1,%xmm4` / +0x80 `jbe` | 662 | forced (`xmm4 = 0`) |
| arc tiny step | +0x126 `comiss .LC0(1.1920929e-7),%xmm0` / +0x13b `jbe` | 674 | forced (`xmm0 = 0`) |

Runs (all `caught/reach-stub: 0`, game rc 0, GPU Xvfb, frames 2400 on the pathofeyes driver):
`/var/tmp/ldr3/run_iss1` (natural), `run_iss2` (all 11 sites; 8 fired, the 3 SelectTarget sites
with skip 2 were reached only twice), `run_iss3` (`LDR3_ONLY=` those 3, skip 0). Every ARM has
`readback_ok`, every RESTORE has `ok` and a readback equal to the original. No FAIL and no
OWNER_MISMATCH. Merged gcov:

- SelectTarget: 34/34 lines, 40/40 branches.
- FollowArc: 14/14 lines, 8/8 branches.
- ctor: 46/46 lines. Not counting the `(throw)` call edges, 32/38 branches. The 6 left are
  gcov's 147 br12/13 and 183 br5-8 (148 and 184 after the marker). Both sit in the ctor's `.cold` landing-pad code: 147 is the
  `test %al,%al / je` on the explosion temporary's live flag at cold+0x1e14 before `~CToken`,
  and 183 is the unwind chain that ends in `_Unwind_Resume`. They are reached only through a
  `(throw)` edge, and all of those are 0. They are compiler EH cleanup with no retail arm,
  because MWCC builds this code without EH.

Asm-check against retail (asmcheck plus by hand for the ctor's initializer list):
- `CHealthInfo(data+0x64, rodata_10 = 10.f)`.
- `CPathFindSearch(0, 3, 0, rodata_30 = 1.f, 1.f, 0, 0)`.
- The 0x4370 `cmplwi 0xffff` / 0x43BC `beq` is the explosion `!= kInvalidAssetId` arm.
- Stores: 0x300/0x304 = 0, the null box with 0x320 = 1, 0x37c = 1 (FSM2 optional), 0x3d4 = 0,
  0x3f8.. translation, 0x404.. forward, 0x410/0x412 invalid ids, 0x45c = 1.f, and 0x424/0x460/0x734/0x738 = 0.
- 0x73c bits: 0x80 set, 0x40..0x04 cleared.
- The final 0x4840 `Lock` is the TARGET_PC block.
- SelectTarget: the only finding is the `TCastToPtr<CPatterned>` call, which C++ spells as a cast helper.
- FollowArc: 0 findings.

### ldr2: fn_33_744 SLdrIngSnatchingSwarm ctor

Branch-free: retail 0x744..0x890 has 26 member stores (`stw`/`stfs`/`stb`) and three calls
(`__ct__20SLdrEditorPropertiesFv` 0x758, `__ct__14SLdrDamageInfoFv` 0x774,
`__ct__23SLdrDamageVulnerabilityFv` 0x78C) and no compare or conditional branch. Store map (all
verified against the port's member order):

`+0x38`=3 (SLdrEditorProperties' own default: the DOL ctor 0x8023F0E4 stores `li r0,3` at
0x8023F124, so the port's `editorProperties()` covers it) - `+0x3c/+0x40/+0x48/+0x7c` = -1
(kInvalidAssetId: stateMachine, swarmParticleSystem, secondarySwarmParticleSystem, pART) -
`+0x44`=0.5, `+0x4c`=0.35, `+0x50`=15, `+0x54`=25, `+0x58`=10, `+0x5c`=2000, `+0x64`=0.5,
`+0x68`=4, `+0x6c`=10, `+0x70`=3, `+0x74`=2, `+0x78`=5, `+0x84`=20, `+0x8c`=10, `+0x9c`=50
(rodata_0/4/8/C/10/14/18/1C/20/24/28/2C = 0.5/0.35/15/25/10/2000/4/3/2/5/20/50) -
`+0x60`/`+0x61` = 0 (useSteeringForMovement, ignorePlayer) - `+0x80`=11 (impactDamage.dI_WeaponType)
- `+0x90/+0x94/+0x98` = -1 (sound_Impact/Idle/Move; swsldr1's `#ifdef TARGET_PC` fix).

Run: `tools/cov/run.sh tools/cov/specs/ingsnatchingswarm.py 3bfa3eff,10_temple_pathofeyes --con
ingsnatchingswarm_pathofeyes.con --fast --frames 2400` on the cov build; gcov
`SLdrIngSnatchingSwarm.hpp`: the ctor called 15, every statement line 15/15, the only branches are
the exception-table edges (`throw`) and the cleanup `call 0/1 never executed` at 0x890.

