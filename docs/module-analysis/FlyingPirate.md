# FlyingPirate REL (module 22): analysis notes

Pirate Aerotrooper. The module's code is upstream's `src/MetroidPrime/Enemies/CFlyingPirate.cpp`
(no `TARGET_PC` blocks; the port compiles it as-is). Retail asm:
`build/G2ME01/FlyingPirate/asm/MetroidPrime/Enemies/CFlyingPirate.s` (after `python3 configure.py && $T/ninja`).
Per-function table: `FlyingPirate_functions.tsv` (B upstream body, H weak/implicit copy, T REL runtime).

## Survey (fp1, 2026-10-23)

Unit 97.2% fuzzy. Every function is 100% except:

| function | fuzzy | objdiff verdict |
|---|---|---|
| LoadFlyingPirate fn_22_5D4 | 99.4 | scheduling |
| SLdrFlyingPirate ctor fn_22_12BC | 52.6 | not deep-checked; the loader overwrites its defaults (as SLdrMetroidAlpha) |
| CVector3f::Cross fn_22_22A8 | 92.1 | weak inline copy |
| PreRender fn_22_25C0 | 96.3 | retail reloads `lwz r5,0x60(r30)` at 0x2610 before `lwz r5,0x10(r5)`; same value |
| DoUserAnimEvent fn_22_2738 | 92.1 | `info.Token().IsLoaded()` (0x2798-0x27E4): retail's inline also caches the object pointer (`GetObj`, `stw r0,0x8(r31)` at 0x27D8) when the locked token is loaded; same truth value. Header inline (CToken), not this module; rest is stack layout |
| CFlyingPirateData ctor fn_22_A684 | 92.7 | scheduling of the CDamageInfo copies |
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
