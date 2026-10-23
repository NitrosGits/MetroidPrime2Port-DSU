# IngSpaceJumpGuardian REL (module 34): analysis notes

CIngSpaceJumpGuardian (in-game "Jump Guardian", the Ing guardian of the Space Jump Boots): jumps
between CScriptAiJumpPoints, fires mini-portal plasma beams, taunts, spawns shock waves on landing and
a blob effect. Code is upstream's `src/MetroidPrime/Enemies/CIngSpaceJumpGuardian.cpp`, loader
`include/MetroidPrime/ScriptLoader/SLdrIngSpaceJumpGuardian.hpp`. Retail asm:
`build/G2ME01/IngSpaceJumpGuardian/asm/MetroidPrime/Enemies/CIngSpaceJumpGuardian.s`. Per-function
table: `IngSpaceJumpGuardian_functions.tsv` (B upstream body, H weak/implicit copy, T REL runtime).

## Survey (sjg1, 2026-10-23)

Every sub-100% body diffed against retail (registers/stack normalized, rodata by value).

| function(s) | verdict |
|---|---|
| SelectJumpTarget, FilterJumpPoints | **fixed**: jump point is read through `GetJumpPoint` at +0x180, upstream used the wrong member |
| CollisionDamage | **fixed**: weapon direction is the forward column; knock-back gated on mDamageable bit 0x08 |
| Jump OffGround/Launching | **fixed**: `SetMomentumWR` (retail), not a velocity set |
| DoUserAnimEvent -> SpawnShockWave | **fixed**: retail passes the event's locator name; TARGET_PC overload `SpawnShockWave(CStateManager&, const rstl::string&)` placed after the original so the strip-diff stays exact |
| ComputeJumpVelocity | **fixed**: apex height `startZ < targetZ ? targetZ : startZ` |
| CameraShake | **fixed**: `FindConnectedObject` (retail) instead of `GetConnectedObject` |
| SpawnBlobEffect | **fixed**: `effect->SetNextDrawNode(GetUniqueId())` (0x112C `sth 0x12a`) |
| SLdrIngSpaceJumpGuardian ctor (fn_34_B7C) | **fixed**: ambient colour white, visor flags 0xF, sound/sound_MiniPortal -1, beam inner CColor(0.5,0.5,0.5,0.5), outer (0.6,0.6,0,0.5) |
| ctor 47%, Jump 82%, FindBestMiniPortals 86%, Load 73%, CreateLight, FindJumpPoints, UpdateBlob, GetAimPosition | equivalent: material masks 30/31/49/32/43 and portal masks 33/32/47/20/59 match, strings match, CreateLight light-id order and push_back growth equivalent |
| SpawnBlobEffect / Dead token | retail passes `TToken`, upstream a `TLockedToken`; kept (no behaviour difference seen) |

## Shipped data

Guardian u051f (editor 001900e9) in Agon `42b935e4,02_sand_sandfalls_dark` (area 25), layer 2,
inactive until Activate. HP 1100, dies to about 12 shots; script connections delete it after death.

## Coverage (sjg1)

Coverage build, spec `tools/cov/specs/ingspacejumpguardian.py`, drivers
`tools/drivers/ingspacejumpguardian_sandfalls.con` (fight to death) and
`..._sandfalls_forced.con` (forced calls, then fight), god mode:

    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/ingspacejumpguardian.py <out> \
      42b935e4,02_sand_sandfalls_dark --con tools/drivers/ingspacejumpguardian_sandfalls.con --fast --frames 6000
    python3 tools/cov/gcov_report.py build-port-cov <out>/gcov --module IngSpaceJumpGuardian

| arm | how |
|---|---|
| Listen noise arms | forced (`noise 0/4/6 [x y z]`) |
| PathOver, ShouldTaunt, PathShagged, IsFacing* | forced (`trigcall u051f <name>`) |
| FindBestMiniPortals, SelectAttackAction, SelectJumpTarget, SetFace*/SetIntroJumpTarget | forced (`codecall u051f <name>`) |
| Taunt/MiniPortalAttack/FaceTarget/SteerToDest/PathFind/Jump msgs 0/1/2 | forced (`statecall u051f <state> <msg>`) |
| intro pattern, jumps, shock waves, beams, blob, death | natural (`send u051f Activate`, shoot loop) |

Marked (30): Think, Listen, ShouldJump, PathOver, FindBestMiniPortals, SetupCollision, CreateLight,
GetCurrentStruct, SpawnBlobEffect, PreRender, AddToRenderer, GetDamageVulnerability,
SetupStateMachine, StateOver, HasIntroPattern, IntroPatternOver, HasJumpTarget, ShouldTaunt,
ShouldMiniPortalAttack, FoundMiniPortalAttack, SkipToNextJumpPoint, PathExists, Start,
EnableEnergyBar, SelectTarget, SetIntroJumpTarget, SetFaceJumpTarget/AttackTarget/Waypoint,
SpawnShockWave (on the PC overload).

Unmarked, with the untaken arms: ctor (EH/data arms 159-194), dtor (deleting arm), AcceptScriptMsg
(212/218/224), DoUserAnimEvent (effect/eyes event cases), Render, GetAimPosition, IsFacingTarget/
IsFacingWaypoint (cast fail), PathShagged, Dead (503-514 never run: the actor is deleted by script
first), FollowIntroPattern, FaceTarget/Jump/Taunt/MiniPortalAttack/PathFind/SteerToDest sub-arms,
SelectJumpTarget, SelectAttackAction (776), ComputeJumpVelocity, TouchDamage, CollisionDamage,
UpdateCollisionVulnerabilities, SetCollisionVulnerability, UpdateLight, CameraShake (null shaker),
FilterJumpPoints, FindJumpPoints, UpdateBlob, Load (null-model arm), the SLdr ctor/dtor (header),
GetSearchPath/IsListening/GetGravityConstant (not instrumented).
