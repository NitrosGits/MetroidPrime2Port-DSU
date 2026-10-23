# Parasite REL (module 47): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CParasite.cpp` (one `CParasite` class for
the Brizgee, Crystallite and Parasite loaders); the loader defaults are
`include/MetroidPrime/ScriptLoader/SLdrParasite.hpp`, `SLdrBrizgee.hpp`, `SLdrCrystallite.hpp`. The
port's fixes are `TARGET_PC` blocks in those three headers. Retail asm:
`build/G2ME01/Parasite/asm/MetroidPrime/Enemies/CParasite.s`. Per-function table:
`Parasite_functions.tsv` (B = module body, H = weak copy/template or unnamed static, T = REL runtime).

## Behaviour fixes (para1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_47_1D04 SLdrParasite ctor | 0x1E00 `stfs 0x8c` knock-back resistance 2.0; 0x1E24 `stw 0x298` ambient CColor(1,1,1,1); 0x1E34 `stw 0x2e8` visorFlags 0xF | 0 / 0 / 0 |
| fn_47_1438 SLdrBrizgee ctor | 0x148C/0x1498/0x14A4 `stw -1` shellBreak/poisonHit/playerPoison sounds; 0x14B4 knock-back 2.0; 0x14CC ambient; 0x14DC visor 0xF | 0 (a real sfx id) / 0 / 0 / 0 |
| fn_47_148 LoadCrystallite (inlined SLdrCrystallite ctor, struct r1+0x508) | 0x198 `stfs 0x590` knock-back 2.0; 0x21C `stw 0x79c` ambient; 0x220 `stw 0x7ec` visor 0xF | 0 / 0 / 0 |

Checked equivalent (no fix): objdiff is 100% for every CParasite body except LoadCrystallite (95.6%,
the inlined ctor), LoadBrizgee/LoadParasite (99.4/99.3%, regalloc and rodata labels; the ctor-call
float arguments were compared label by label by value), the SLdr ctors (the fixes above), the
CParasite ctor (96.9%, bitfield scheduling) and PreRender (81%: retail stores an uninitialised stack
word to 0xFC before the shader byte to 0x101; no reader, left alone). AcceptScriptMsg's fall-through
from `kSM_ResistedDamage` into `kSM_ReflectedDamage` is retail's too (100%). fn_47_2B40 and the
`rstl::destroy` instance are the CJointCollisionDescription vector destroy (host copies).
`upstream_hollow.py Parasite`: none.

## Types and spawns

The loaders pass only `kPT_Parasite` (0, LoadParasite), `kPT_IceZoomer` (3, LoadBrizgee) and
`kPT_Crystallite` (10, LoadCrystallite): the `kPT_Oculus` (1) and `kPT_Geemer` (2) arms are dead
(no writer). `tools/scan_area_actors.py PARA` finds no PARA placement in the shipped paks, so the
`kPT_Parasite` arms are dead in shipped data (reachable only with console `spawn PARA`).
Brizgee: Agon 02_sand_sandfalls layer 9 (u05e5, u05d9, u05d6, 12 hp; shell collision actors
u0610-u0612), Torvus 0m_swamp_hall. Crystallite: Agon 01_sand_entrance_dark layer 2 (u052d, u04fc,
1 hp). Others: docs/modules.tsv row Parasite.

## Coverage (para1)

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON`; runs (each con's header has its command line):

    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/parasite.py <out> 42b935e4,02_sand_sandfalls \
      --con tools/cov/specs/parasite_sandfalls_fight.con --fast --frames 3600      # and _forced.con, 1250
    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/parasite.py <out> 42b935e4,01_sand_entrance_dark \
      --con tools/cov/specs/parasite_entrancedark_fight.con --fast --frames 3600   # and _forced.con, 1600
    python3 tools/cov/gcov_report.py build-port-cov <out1>/gcov ... --module Parasite

The fights are natural (beams, missiles, kills); the `_forced` drivers run every state body
(`statecall` 0/1/2), trigger (`trigcall`) and script message (`send`), walk the machine (`fsm`) and
put the actors on the player (contact damage). The Brizgee driver leaves out
Generate/Deactivate/Attack/Retreat/Halt: their body commands need anims the Brizgee ANCS lacks and
the host crashes in CBSGenerate/CBSJump::Start on the bad anim index (forced only; the Brizgee FSM
never enters them). The Crystallite runs Deactivate last (its Activate deletes the actor).

Marked by para1 (31; every live arm taken or branch-free with every line run; objdiff 100% except
PreRender, see above): ~CParasite, PreThink, GetAimPosition, ThinkAboutMove, Stuck, AttackOver,
ShotAt, PatrolPathOver, IsOnGround, AnimOver, Landed, Generate, Deactivate, Run, PathFind, Jump,
FaceTarget, TargetPlayer, Death, Crouch, GetUp, Touch, UpdatePFDestination, PreRender, Render,
GetDamageVulnerability, SetupIceZoomerCollision, DestroyActorManager, MassiveDeath,
SetupStateMachine, SetFuncPtrs.

| function | arms | how |
|---|---|---|
| ThinkAboutMove | ground-collider arm | forced (`statecall <id> Jump 0`, held 30 frames) |
| Generate, Deactivate | progress 0/1/2 arms | forced (`statecall <id> Generate 0` then `Generate 1` / `Deactivate 1` per 4 frames, Crystallite) |
| ShotAt, TargetPlayer, FaceTarget | all | forced (`trigcall ShotAt`, `statecall TargetPlayer 0/1/2`) |
| Jump, PathFind | `switch (msg)` default | dead: `EStateMsg` has three values (StateMachineCommon.hpp) |
| PathFind | `mType == kPT_Parasite` | dead in shipped data (no PARA placement) |
| Crouch, GetUp | `case kPT_Geemer` | dead: no loader writes type 2 |
| GetDamageVulnerability | Oculus case and its `mHalted` arms; default (Parasite) | dead: no type-1 writer; no PARA placement |

Unmarked (live arms not run): the CParasite ctor (exception and type arms), AcceptScriptMsg (the
Crystallite AreaLoaded generated-object arms, ResistedDamage weapon types), Think (occlusion, line
of sight, frozen, contact-damage timer arms), UpdateJumpVelocity, CloseToWall/HitSomething (0/5),
CollidedWith, ShouldAttack, Patrol (pause timer), Attack/Retreat/Halt (jump arms; Brizgee lacks the
anims), TelegraphAttack (one chain arm), TargetPatrol (no-waypoint arm), DoFlockingBehavior,
GetContactDamage (shell-broken Brizgee), GetCollisionResponseType (non-Crystallite), UpdateShell,
UpdateCollisionActors, SetupIceZoomerVulnerability, the loaders (`!modelData`), LoadParasite,
UpdateWalkerAnimation, MassiveFrozenDeath, AddDoorRepulsors, the Recursive/GetClosestWaypoint pair.
Next: break a Brizgee shell (damage on a shell collision actor, then contact damage with
`mVulnerable`), `spawn PARA ... ancs u052d` for the Parasite-only arms, a jump driver on the
Crystallite (Attack/Retreat until `kAS_Jump`), MassiveFrozenDeath (freeze + power bomb).
