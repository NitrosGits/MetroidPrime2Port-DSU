# Blogg REL (module 7): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CBlogg.cpp` (Blogg, Bloggling, Dark Blogg;
`mIsMegaBlogg`); the loader defaults are `include/MetroidPrime/ScriptLoader/SLdrBlogg.hpp`. The port's
fixes are `TARGET_PC` blocks in those two files. `CBloggProjectile` is DOL-side. Retail asm:
`build/G2ME01/Blogg/asm/MetroidPrime/Enemies/CBlogg.s` (after `python3 configure.py && $T/ninja`).
Per-function table: `Blogg_functions.tsv` (B = Blogg body, H = weak copy/template, T = REL runtime).

## Behaviour fixes (blogg1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_7_8F7C AcceptScriptMsg, kSM_HitObject charge arm | 0x9774 `rlwimi r0,r3,1,30,30`: 0xbc4 bit 0x2 (`mChargeOver`) | `xbc4_25_` (bit 0x40) |
| fn_7_8F7C AcceptScriptMsg, kSM_Damage weapon arm | 0x98B4-0x98C4 `lfs 0x2c/0x3c/0x4c` of the weapon: `GetTransform().GetUp()` | `GetForward()` |
| fn_7_1C0C FindNearestHint | 0x1E44 `lbz r0,0xc0(r1); cmplwi; bne .L_1E98`: a blocked LOS ray skips the facing-dot update too | dot ran for blocked hints |
| fn_7_E5C SLdrBlogg ctor | 0xEA0/0xEA8 mouthOpenSound -1; 0xF28 editor 0x38 = 3; 0xF2C health knockback 2.0; 0xF34 isEchoEmitter 1; 0xF38-0xF4C ambient CColor(1,1,1,1); 0xF5C visorFlags 0xF; 0x1078/0x107C ing-possessed health 150 / knockback 2.0 | 0 / unset |

Checked equivalent (no fix): GetNextPositionIndex (both case-0 arms return 0), Dead (0x420 reload
only), CanRangedAttack (0xbc4 bit 0x40 = `xbc4_25_`), UpdateAimWeights (constants 60/180/pi/0.75/pi/2,
same structure; asmcheck BRANCH 22 vs 6 is MWCC's inlined clamps), ComputeAttackPositions,
LaunchBloggProjectile, LoadBlogg, vulnerability ctors/dtors, CollidedWith, Touch, the state bodies
(vtable/label-only objdiff differences), SetupStateMachine, ~CBlogg. `upstream_hollow.py Blogg`: none.
Bitfields hand-checked against the asm (asmcheck NOPOS, info only): mCanBite 0xbc4&0x20, mBallGrabbed
&0x8, mTauntReady &0x4, mChargeOver &0x2, mMeleePursuitEnded 0xbc5&0x80, mAbortBallGrab 0xbc5&0x40.

## Spawns

Torvus Bog (world 3dfd2249): 07_swamp_shaft (area 40) layers 2/3/5, six Bloggs u05fc u05f1 u05da
u05c5 u0598 u0589 around (-165,-120,-45); 08_swamp_grappleguardian (area 55) layers 2/4/7, eight
Bloggs u0654 u0651 u064d u0649 u0646 u0641 u05c5 u05be around (-165,32,-34). Also 09_swamp_catacombs,
0q/0r_swamp_hall (docs/modules.tsv).

## Coverage (blogg1)

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON`; each run:
`BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/blogg.py <out> 3dfd2249,<area> --con <con> --fast --frames N`,
then `python3 tools/cov/gcov_report.py build-port-cov <gcov dirs...> --module Blogg`.
Drivers: `blogg_shaft.con` (07_swamp_shaft natural fight, every beam, missiles, charged shots, kill;
3000 frames), `blogg_forced.con` (07_swamp_shaft: fsm through every state, GrabBall/Thrash/SpitBall
with the player in morph ball, statecall of GrabBall/Thrash/SpitBall/Stunned/Taunt/Dead; 2800),
`blogg_guardian.con` (08_swamp_grappleguardian natural fight and kill; 2900). Merged: 102 bodies,
10 with every arm taken.

Marked by blogg1 (33; every arm taken or branch-free with every line run, objdiff 100% or
label-only/checked equivalent): both vulnerability ctors and GetCollisionResponseType, SetIngPossessed
x2, ~CBlogg, HasCollisionTimeElapsed, PreRender, AddToRenderer, Render, SetupStateMachine,
EndMeleePursuit, ComputeTauntProbability, AnimOver, IsPlayerStunned, CollidedWithWall, CanBitePlayer,
InMeleeRange, InBiteRange, BallGrabbed, ShouldAbortBallGrab, IsFacingPlayer, ProjectileAttackDelay,
IsChargeOver, IsPlayerWithinChargeRange, IsHitInMouthDirection, ShouldPrepareToAttack, CanReachPlayer,
InAttackPosition, InValidPosition, Touch, UpdateAimWeights.

Unmarked (arms or lines not run): the fixed AcceptScriptMsg (114/137 arms) and FindNearestHint
(29/31); GetHealthPhase/ChoosePhaseValue/GetNextPositionIndex/IsPlayerInMouthRange/FindAttackPositions/
GetIngSnatchingPoint/Normal/ShouldPatrol/InProjectileRange/CantMoveToPlayer (never ran); ShouldCharge,
CanMeleeAttack, CanRangedAttack, CanTaunt, the ball-pursuit triggers (short-circuit arms); every state
body (Exit/Deactivate or timer arms missing); Think, DoUserAnimEvent, ApplyCollisionActorDamage, Death,
ComputeAttackPositions, PathToAttackPosition, CollectHints, the mouth/body GetDamageVulnerability.
Next: drive a Mega Blogg (phase data; spawn not located by blogg1), force the trigger short-circuits.
