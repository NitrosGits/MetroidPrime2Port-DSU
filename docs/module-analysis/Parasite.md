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
Generate/Deactivate/Attack/Retreat/Halt: their body commands need anims the Brizgee ANCS lacks, and
retail plays the resulting anim -1 into CAnimData::BuildAnimationTree, which indexes the anim list
unchecked at 0x80029BAC (undefined behaviour, a host crash before brz1; forced only, the Brizgee FSM
never enters them). The console now drops such a command instead and prints a message, so forcing
these states is safe. The Crystallite runs Deactivate last (its Activate deletes the actor).

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

## ldr2: the three loaders and the ctor (2026-10-25)

`tools/scan_parasite_loader.py` (new) walks every MLVL/area/layer of the disc for the three object
types the loaders are registered under (`src/MetroidPrime/ScriptLoader.cpp`: BRZG `LoadBrizgee`
0xCF8, CRLT `LoadCrystallite` 0x148, PARA `LoadParasite` 0x1568) and checks each placement's
property ids against the `case` list of the loader's own `SLdr<Type>.inc`:

| type | placements | dispatch ids | ids in every placement | unknown ids | ancs |
|---|---|---|---|---|---|
| CRLT | 2 (Agon 01_sand_entrance_dark L2) | 10 | 10/10 | 0 | 0xfa76936b ANCS |
| BRZG | 9 (Agon 02_sand_sandfalls L9/L3, Torvus 0m_swamp_hall L0) | 20 | 20/20 | 0 | 0x009469da ANCS |
| PARA | **0** | 22 | - | - | - |

So for CRLT and BRZG every `case` arm runs on any one load, the `default: ReadBytes(nullptr, size)`
arm is dead by data (no placement carries an unknown id) and the `if (!modelData)` arm is dead by
data (the ancs is a real ANCS in every placement, and with `kInvalidAssetId` as the model
`LdrToModelData` only comes back empty for an ancs in no pak). PARA has no placement at all, so
none of its 22 case arms is data-reachable.

Runs (cov build sha256 `0df3bf35…`; `tools/cov/specs/parasite.py` + `parasite_ldr2_entrancedark.con`
for 42b935e4,01_sand_entrance_dark 3600 f, `parasite_sandfalls_fight.con` for
42b935e4,02_sand_sandfalls 3600 f, `--fast`, caught/reach-stub 0):

| body | gcov | arms |
|---|---|---|
| LoadCrystallite 0x148 | called 2, 14/14 lines, 15/16 branches | `SLdrCrystallite.inc`: 10/10 cases taken 2x each, `default:` 0 |
| LoadBrizgee 0xCF8 | called 3, 14/14 lines, 11/12 branches | `SLdrBrizgee.inc`: 20/20 cases taken 3x each, `default:` 0 |
| LoadParasite 0x1568 | called 1 (console `spawn PARA ancs u052d`), 14/14 lines, 10/11 branches | 2/22 cases (0x255a4580, 0xb3774750 - the two blocks CmdSpawn writes) |
| CParasite ctor 0x6770 | called 6 (3 BRZG, 2 CRLT, 1 PARA), 67/75 lines, 20/26 branches | kPT_IceZoomer 3x, `default` 3x; kPT_Geemer/kPT_Oculus never |

Per-property store and reader (retail asm vs the port's generated include): LoadCrystallite's ten
cases are, by offset from the sldr base `r1+0x508`, `+0x2f8`=waypointApproachDistance,
`+0x2fc`=wallTurnSpeed, `+0x300`=floorTurnSpeed, `+0x304`=downTurnSpeed, `+0x308`=visibleDistance,
`+0x30c`=forwardMovingPriority, `+0x310`=stunTime (0x800/0x804/0x808/0x80C/0x810/0x814/0x818, each
`ReadFloat`), plus LoadTypedefEditorProperties / LoadTypedefPatternedAITypedef /
LoadTypedefActorParameters at 0x334/0x344/0x354 - the port's member order and readers are the
same, and every port offset is retail's +0x0c, the host `rstl::string` being 24 bytes against the
DOL's 12 (checked at both ends: BRZG poisonTime 0x49c -> 0x4a8, playerPoisonSound 0x4a8 -> 0x4b4).
BRZG's readers match too: LoadTypedefDamageVulnerability at 0xFC0 (shellVulnerability),
LoadTypedefDamageInfo at 0xFE0/0x1000 (shellContactDamage, poisonDamage), ReadFloat at 0xFEC
(poisonTime), and the three sounds are inline 32-bit reads (0x95C/0x960/0x964). LoadParasite's
`default: ReadBytes(nullptr, size)` arm is at 0x1908.

Ctor arms: the two dead-by-no-writer arms are the only ones left. The ctor has three call sites in
the retail REL, all in this file, and each passes a constant type arg (the 21st argument, stored at
`0x38(r1)`): LoadCrystallite 0x70C `li r26,0xa` → 0x768; LoadBrizgee 0x12E8 `li r26,0x3` → 0x135C;
LoadParasite 0x1B7C `li r12,0x0` → 0x1C04, i.e. 10, 3, 0 only - no writer of 1 (kPT_Oculus) or 2
(kPT_Geemer) exists, and a REL cannot be called by another REL. Retail's chain: 0x6C54 `lwz
0x82c` (mType), `cmpwi 2`/`beq` Geemer → clears `x681_40` (CKnockBackMgr mEnableFreeze) and falls
into Oculus; `cmpwi 1`/`bge` Oculus → `EnableKnockBackPhysics(false)`; `cmpwi 4`/`bge` skip (so 0 and
>=4 take `default`); else the kPT_IceZoomer `TLockedToken<CSkinnedModel>` arm; then 0x6E50 `cmpwi
1`/`bne` the Oculus block clearing `x681_20/10/08/04` (mEnableShock/Burn/BurnDeath/ExplodeDeath).
The port's switch has the same four arms and the same order. The only other untaken edges in the
ctor are the two `__nw` null checks inside the IceZoomer arm (asmcheck 181:5/7, 181:6/7 = retail
0x6CC8/0x6CE8, allocation failure) and its `Free__7CMemoryFPCv` cleanup.

LoadParasite stays unmarked: no PARA placement exists, so its 22 case arms are dead by data, and
the console cannot take that back - `CmdSpawn` writes only the EditorProperties (0x255a4580) and
PatternedAITypedef (0xb3774750) blocks plus scalar `prop` values, while 0x7e397fed is an
`SLdrActorParameters` block read by `LoadTypedefActorParameters`, which no scalar `prop` can feed.
2 of 22 arms ran in the run above.

Unmarked (live arms not run): AcceptScriptMsg (the
Crystallite AreaLoaded generated-object arms, ResistedDamage weapon types), Think (occlusion, line
of sight, frozen, contact-damage timer arms), UpdateJumpVelocity, CloseToWall/HitSomething (0/5),
CollidedWith, ShouldAttack, Patrol (pause timer), Attack/Retreat/Halt (jump arms; Brizgee lacks the
anims), TelegraphAttack (one chain arm), TargetPatrol (no-waypoint arm), DoFlockingBehavior,
GetContactDamage (shell-broken Brizgee), GetCollisionResponseType (non-Crystallite), UpdateShell,
UpdateCollisionActors, SetupIceZoomerVulnerability, LoadParasite (ldr2: 0 PARA placements on the
disc, console cannot supply 0x7e397fed - see the ldr2 table above),
UpdateWalkerAnimation, MassiveFrozenDeath, AddDoorRepulsors, the Recursive/GetClosestWaypoint pair.
Next: break a Brizgee shell (damage on a shell collision actor, then contact damage with
`mVulnerable`), `spawn PARA ... ancs u052d` for the Parasite-only arms, a jump driver on the
Crystallite (Attack/Retreat until `kAS_Jump`), MassiveFrozenDeath (freeze + power bomb).
