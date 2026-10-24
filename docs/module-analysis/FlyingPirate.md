# FlyingPirate REL (module 22): analysis notes

Pirate Aerotrooper. The module's code is upstream's `src/MetroidPrime/Enemies/CFlyingPirate.cpp`
(no `TARGET_PC` blocks; the port compiles it as-is; the SLdr defaults fix is in
`include/MetroidPrime/ScriptLoader/SLdrFlyingPirate.hpp`). Retail asm:
`build/G2ME01/FlyingPirate/asm/MetroidPrime/Enemies/CFlyingPirate.s` (after `python3 configure.py && $T/ninja`).
Per-function table: `FlyingPirate_functions.tsv` (B upstream body, H weak/implicit copy, T REL runtime).

## Survey (fp1, 2026-10-23)

Unit 97.2% fuzzy. Every function is 100% except:

| function | fuzzy | objdiff verdict |
|---|---|---|
| LoadFlyingPirate fn_22_5D4 | 99.4 | scheduling |
| SLdrFlyingPirate ctor fn_22_12BC | 52.6 | five sound defaults and two actor-parameter defaults differed: fixed (fp2, below); rest is the inline ctor order |
| CVector3f::Cross fn_22_22A8 | 92.1 | weak inline copy |
| PreRender fn_22_25C0 | 96.3 | retail reloads `lwz r5,0x60(r30)` at 0x2610 before `lwz r5,0x10(r5)`; same value |
| DoUserAnimEvent fn_22_2738 | 92.1 | `info.Token().IsLoaded()` (0x2798-0x27E4): retail's inline also caches the object pointer (`GetObj`, `stw r0,0x8(r31)` at 0x27D8) when the locked token is loaded; same truth value. Header inline (CToken), not this module; rest is stack layout |
| CFlyingPirateData ctor fn_22_A684 | 92.7 | scheduling of the CDamageInfo copies; every store's field and order checked (fp2) |
| fn_22_2A28..2D78 (10) | 0 | unnamed CImpactVisorEffect/rstl helpers called from CImpactVisorEffect::None (fn_22_29E0); `upstream_hollow.py` clears them |

No behaviour difference found. `tools/upstream_hollow.py --min 0 FlyingPirate`: only 8-byte getters
that match retail (EMPTY, 100%). `tools/retail_diff.py --module FlyingPirate`: no findings.

## Spawns that work (save mi_u)

| area | layer | ids | notes |
|---|---|---|---|
| Agon 02_sand_sandfalls (`42b935e4,02_sand_sandfalls`) | 8 | u06de (000a012a), u06cf (000a011e) | active, patrol; `tp -95 -100 -8` puts the player in range; team mgr 000a011f |
| Agon 08_sand_casualpirates | 2 | u0545 u053d u0539 u0537 | inactive until `send <id> Activate`; `tp 355 -265 4` |

AFSM states (`fsm <id> ?`): Patrol, Start, Lurk, PathFind, Attack, Jump, TargetPatrol, Walk, Land,
ProjectileAttack, GetUpNow, GetUp, Taunt, Retreat, Dead, Explode, Enraged, Dodge, Bounce, Deactivate,
TurnAround, ChooseWaypoint. `damage <id> 1000` takes MassiveDeath (object deleted); `damage 89` then
a power shot is the ordinary death (Dead, DeathOver).

## Coverage (fp1)

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON`; each run
`BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/flyingpirate.py <out> <world,area> --con <con> --fast --frames N`,
then `python3 tools/cov/gcov_report.py build-port-cov <gcov dirs...> --module FlyingPirate`.
Drivers (tools/cov/specs/): `flyingpirate_sandfalls.con` (natural fight, shots, missile, kills, 3600 f),
`flyingpirate_forced.con` (every state by `fsm`, script messages, noises, `touch`, death at 1 hp, 3400 f),
`flyingpirate_triggers.con` (`trigcall` on the triggers with/without argument, `statecall` msg 0-3, 2200 f),
`flyingpirate_casual.con`, `flyingpirate_casual2.con` (casualpirates, four pirates, kills by charged
shot, missile, power bomb, dark/light/annihilator, 3300/3400 f). No `[port] caught`/reach-stub.

Marked (34; every gcov arm taken or branch-free with all lines run; 100% objdiff, PreRender 96.3%):

| function | arms / how |
|---|---|
| MassiveDeath 75BC | `damage <id> 1000` (natural: shots) |
| CoverCheck 61C4, UpdateCanFireMissiles 5D4C, ShouldAttack 5C80, ShouldMove 3D08, Jump 45C4, AddToRenderer 2634 | natural fight |
| DeathOver 3804 | natural deaths (casual, forced runs) |
| Enraged 3AF0, Explode 3708 | forced (`fsm u06de Enraged` / `Explode`) |
| Land 4258, ProjectileAttack 3E6C | forced (`fsm`), default arm forced (`statecall <s> 3`: no shipped caller sends msg 3) |
| Deactivate 36EC | forced (`statecall u06de Deactivate 0/1/2`) |
| ShotAt 48CC, Attacked 4898 | forced (`trigcall <t>` and `trigcall <t> 5`: both ternary arms) |
| branch-free (lines all run): GetOrigin 8B74, LineOfSightTest 8704, PatternOver 6B8C, PatternShagged 6B6C, HearShot 6B50, GetUpNow 66BC, ResetFireMissilesCheck 5E3C, SpotPlayer 5940, LineOfSight 5934, Landed 45AC, AggressionCheck 3C3C, AnimOver 37E4, HasAttackPattern 3530, ProjectileInfo 3528, PreRenderAllViewports 2718, PreRender 25C0, Render 25A0, PreThink 2394, SetupStateMachine 1570 | natural / `trigcall` |

`asmcheck.py check FlyingPirate` on the marked bodies: 31 findings, all tool limits on 100%-matching
code (bitfields without a stated bit position, inlined helpers/rstl calls, MassiveDeath's inlined
CExplosion ctor, Explode's MassiveDeath is a vtable call).

Near-complete, not marked (gcov line:arm untaken):

| function | untaken |
|---|---|
| Dead 386C | 1588:0/2 (`GetTime() >= 6` with spin-to-death) |
| Lurk 6244 | 1024:1/2 |
| ShouldDodge 4B64 | 1349:3/4 |
| HearPlayer 6ACC | 906:1/2, 907:1/2 (detection-height arm) |
| Taunt 6800 | 940:5/6, 941:3/4 |
| Stuck 3C48 | 1549:1/2, 1550:1/2 |
| CollidedWith 23F4 | 1774:3/4, 1776:1/2 |
| Attack 5A24 | 1152:1/2, 1153:1/2, 1172:1/2 |
| CheckFireMissiles 7C04 | 678:1/2 |
| AvoidActors 84B0, AddToTeam 8AC8, RemoveFromTeam 8A20, DeliverGetUp 8FA4 | one arm each (497, 506/507, 566, 420) |
| Patrol 73A4, UpdatePatrolFacing 6F98, CheckForProjectiles 8790, Listen 9034, DoUserAnimEvent 2738 | 1-3 arms |

Partly covered: AcceptScriptMsg (Launching with a cover point), KnockBack (Hurled while alive), Think,
GetDodgeDirection, PathFind, Retreat, TurnAround, Walk, ChooseWaypoint, CoverFind, InPosition (needs a
cover point), ShouldSpecialAttack (flavor One with an attack object), ShouldRetreat, FireProjectile,
GetTargetPos, GetUp, Bounce, Dodge, InRange, StartSpinToDeath, TargetPatrol, UpdateLandingSmoke,
UpdateParticleEffects (aqua arms: no aqua pirate in these areas), ctor, LoadFlyingPirate. Not run:
SLdr ctor deep check, CFlyingPirateData ctor marker (92.7%).

## Coverage and checks (fp2, 2026-10-23)

Forced runs on the coverage build (`force_ret.py`: each write is undone when the call returns, unless
the entry says it stays), merged with fp1's five runs:
`COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/flyingpirate_fp2.py <out> <world,area> --con <con> --fast --frames N`
with `flyingpirate_fp2.con` (sandfalls: HearPlayer/fsm/place, 4200 f), `flyingpirate_sandfalls.con`
(3700 f) and `flyingpirate_casual.con` (casualpirates, 4500 f; the kills feed KnockBack/Dead). No
`[port] caught`/reach-stub. Lines are gcov lines of `CFlyingPirate.cpp` at d0ab81f9c (before fp2's markers).

Marked (17):

| function | arms / how |
|---|---|
| Listen 9034 | natural; 406/407 forced (Listen height) |
| UpdateParticleEffects 8E3C | natural; aqua arms 427-445 forced (UpdateParticleEffects aqua) |
| AddToTeam 8AC8 | natural; 498 forced (AddToTeam not a team) |
| GetTargetPos 7EC4 | natural; 663/664 forced (the spec's TargetBp: mTargetId = another live pirate, active / inactive for the call) |
| CheckFireMissiles 7C04 | natural; 680 forced (CheckFireMissiles blocked) |
| HearPlayer 6ACC | natural; 913 forced (HearPlayer deaf) |
| Taunt 6800 | natural; 946/947 area arm forced (Taunt other area) |
| Lurk 6244 | natural; 1031 forced (Lurk dest = self) |
| Attack 5A24 | natural; 1164 forced (Attack checked), 1183 forced (Attack on player) |
| InRange 58E4 | natural; 1208/1209 forced (InRange ranges: min 0, 1e6/1e6, 1e6/0). Retail tests the player's absolute Z and translation, not a delta (0x58E4-0x5930); the C++ does the same |
| ShouldDodge 4B64 | natural; 1365 forced (ShouldDodge shot at); 1361 `x6a1_25_` (0xbb9 mask 0x40) is dead: only ever stored 0 (0x64DC Lurk deactivate, 0x9A9C ctor) |
| Dodge 4900 | natural; 1378 forced (Dodge no direction); 0x4000 picks 2 (left), else 3 (right) |
| ShouldSpecialAttack 4718 | false arm natural (5853); the rest is dead: `mFlavor` (0x41c) is always 0 (ctor `li r7,0` at 0x9714), so `cmpwi r0,1` at 0x4738 never matches |
| Stuck 3C48 | natural; 1570 forced (Stuck no path), base arm forced (Stuck base stuck) |
| Dead 386C | natural; 1610 forced (Dead spin time 7: mSpinToDeath 1 + GetTime() 7 on Update; the fights' spins end on a collision before Dead sees them) |
| CFlyingPirateData ctor A684 | branch-free; asm stores match the C++ field for field (0x54 = default CDamageInfo for mWpscDamage) |
| SLdrFlyingPirate ctor 12BC (in SLdrFlyingPirate.hpp) | fixed: see below |

SLdr ctor fix (`#ifdef TARGET_PC`, each citing its store): sound_Projectile -1 (0x318 at 0x1300),
sound_Impact/sound_Spiral -1 (0x360/0x364), sound_Hurled/sound_Death -1 (0x380/0x384); the hpp had 0.
Added `actorInformation.lighting.ambientColor = CColor(1,1,1,1)` (0x294 at 0x1434) and
`actorInformation.visor.visorFlags = 0xf` (0x2E4 at 0x1444). Every other default matches the rodata.

`asmcheck.py check` on the 16 CFlyingPirate-file marks: 41 findings, all tool limits (no bitfield
position table for this module: BIT/NOPOS name the wrong field, e.g. 4718's `xbb9_04` is mIsMoving;
inlined CCollisionPrimitive/TCastToPtr calls; Dodge's BRANCH count is the inlined helpers).

Gotcha: a flyer runs with `mVerticalMovement` set, so KnockBack's untaken arm at 705 is the *false*
side (force 0, not 1). A gcov `f` (fallthrough) arm is not the true side of the source condition.

Not marked (untaken after fp2):

| function | untaken |
|---|---|
| DeliverGetUp 8FA4 | 420 lie-on-ground arm (inlined into its callers in the coverage build: no entry to force) |
| RemoveFromTeam 8A20 | 508 IsPartOfTeam false (the FORCE_INNER never fired) |
| KnockBack 78B0 | 722 Hurled while frozen (IsFrozen is inline); 705/708/733/742 now taken (forced) |
| FireProjectile 2E34 | 1714 `distance < 6 && dot <= 0.5`, 1731 no additive reaction anim |
| DoUserAnimEvent 2738 | 1747 projectile token not loaded |
| CollidedWith 23F4 | 1806 one arm, 1808 SharesMaterials false |

The rest of fp1's "partly covered" list is unchanged (cover-point arms: no shipped cover point in
these areas; Think, Patrol, PathFind, Retreat, TurnAround, Walk, GetUp, Bounce, TargetPatrol, ...).

## Coverage (fpir1: the 11 rows still unmarked)

The TSV's marker column is stale; the list is the TSV's B rows minus the `// @ fn_22_*` markers in
src/include: 0, 470, 478, 480, 5D4, 1228, 15DC, 2738, 6040, 96D4, AA84. All 11 annotated against the
asm; no body differed from retail (so no code change, only markers and one member offset). One
natural run at this commit (`flyingpirate.py` + `flyingpirate_sandfalls.con`, sandfalls, 3600 f:
`caught/reach-stub: 0`) covers the five marked rows; the rest are blocked by shipped data, and the
forced fp2 runs cannot move them (see the tables above).

Marked (5, all objdiff 100% - matched bodies need no coverage run; each line also ran in the gcov
build):

| fn | evidence |
|---|---|
| 0 ~CFlyingPirate | 100%; D0/D2 both 2/2 lines. The asm is the member chain: 0x98 `__dt__rstl::reserved_vector<TLockedToken<CGenDescription>,3>`, 0xa8 `__dt__6CTokenFv`, ... down to the vtable store at 0x34 and the 0x24 null-this guard |
| 470 GetSearchPath | 100%; `0x470 addi r3,r3,0x970 / blr` = `&mPathFindSearch`, so that member is at 0x970 (now in the header) |
| 478 IsListening | 100%; `0x478 li r3,0x1 / blr` = `return true` |
| 480 GetGravityConstant | 100%; `0x484 extrwi. r0,r0,1,26` (xbb8_20 = mIsAquaPirate) picks `skAquaGravityConstant` 5.0f (0x490) or `skGravityConstant` 50.0f (0x49c). gcov: the aqua arm is untaken here (no aqua pirate in the sandfalls) |
| 1228 ~SLdrFlyingPirate | 100%; 0x1250/0x125c/0x1268 the three SLdrDamageInfo dtors, then ActorParameters, PatternedAITypedef, EditorProperties and the 0x1294 delete-flag guard - reverse declaration order |

Left unmarked (6):

| fn | untaken (this run's gcov lines) |
|---|---|
| 5D4 LoadFlyingPirate | 2041:1/2 (`if (!modelData) return nullptr`: LdrToModelData has to fail), 2050:0/2, 2050:1/2 (the CFlyingPirateData argument list); objdiff 99.4 |
| 15DC Think | 28 arms (1865:0/2, 1870:0/2, 1876, 1907, 1909, 1911, 1918, 1928, 1929, 1934, 1939, 1945, 1949, 1950, 1964, 1968, 1969, 1986, 1995, 2021): the frozen/cinematic-cam/aqua/water and burst-fire branches; objdiff 99.98 |
| 96D4 CFlyingPirate ctor | 219/220/221 (the `(mData.mType & 2) ? 4 : 3` path-find type), 273, 285, 286 (aqua/type variants); objdiff 99.93 |
| 2738 DoUserAnimEvent | 1782:1/3 (the switch), 1784/1785 (`mIsAttackingObject` and `Token().IsLoaded()`), 1792/1795 (the `!mIsAttackingObject && mIsAquaPirate` homing arm): needs an aqua pirate that is not attacking an object; objdiff 92.1 (the inline token cache, header-side) |
| 6040 CoverFind | 14/21 lines, 7/22 arms: the whole cover-point loop needs a `CScriptCoverPoint` in the area, and the shipped FPRT areas have none |
| AA84 __sinit_CFlyingPirate_cpp | objdiff 100%, but it is compiler-generated from the trigger/state/code tables: no source line to put a marker on (same as the other modules' __sinit rows) |
