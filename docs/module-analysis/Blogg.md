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

Marked by blog1 (8; forced where noted): SBloggPhaseData ctor, GetPlayer, IsPlayerWithin,
GetDirectionToPlayer, CanGrabBall, IsPlayerReachable, StopPlayer, AttachPlayerToMouth. The null-player
arms are forced (`tools/cov/specs/blogg_force.py`, `blogg_forced.con` at 07_swamp_shaft, 2800 frames:
every 5th GetPlayer sees mPlayerId invalid; also forces pursuit times and mIsMegaBlogg, which left
ShouldEndPursuit 719:1/4, ShouldEndBallPursuit, PlayerInBallMode 739-740, DetectBall 747:3/6 still short).
Gcov dirs (kept): /tmp/opencode/blog1/{force1,shaft,forced,guard}/gcov; merge force1 first.
asmcheck: only the rstl::string_l temporary's destructor call in CanGrabBall/AttachPlayerToMouth.

Unmarked (arms or lines not run): the fixed AcceptScriptMsg (114/137 arms) and FindNearestHint
(29/31); GetHealthPhase/ChoosePhaseValue/GetNextPositionIndex/IsPlayerInMouthRange/FindAttackPositions/
GetIngSnatchingPoint/Normal/ShouldPatrol/InProjectileRange/CantMoveToPlayer (never ran); ShouldCharge,
CanMeleeAttack, CanRangedAttack, CanTaunt, the ball-pursuit triggers (short-circuit arms); every state
body (Exit/Deactivate or timer arms missing); Think, DoUserAnimEvent, ApplyCollisionActorDamage, Death,
ComputeAttackPositions, PathToAttackPosition, CollectHints, the mouth/body GetDamageVulnerability.
Next: drive a Mega Blogg (phase data; spawn not located by blogg1), force the trigger short-circuits.

## Coverage (blog2, 2026-10-24)

Why blog1's forces missed arms: nothing made `CanMeleeAttack` false (`mMeleePursuitEnded` /
`mChargeOver`), the shaft Bloggs rarely evaluate the pursuit triggers (the 8-Blogg
grappleguardian area does), and the persistent force.py writes interleaved. Now
`tools/cov/specs/blogg_force2.py` (force_ret.py, call-scoped) + `blogg_guardian2.con`.

| fn | arms | how |
|---|---|---|
| ShouldEndPursuit (5BF4) | 4/4 | forced (phase 0 `mMeleePursuitEnded=1`, phase 1 `mPlayerPursuitTime=1e9`) |
| DetectBall (5A98) | 6/6 | forced (`mIsMegaBlogg=1`, `mCanBite=0`), rest natural |
| ReleaseHints (1B78) | all | forced (`mHintIds[0]=0xffff`) null-hint arm |
| ShouldEndBallPursuit (5B48) | 9/12 | unmarked: player morphed arms (729:1, 731:1, 733:0) never seen; only 3 calls |
| PlayerInBallMode (5AF4) | 4/6 | unmarked: player morphed never seen with a Blogg evaluating it (743:1, 744:1) |
| FindFluid / IsInhabitingFluid | 2/4, 3/4 | unmarked: no Blogg gets a fluid id (the two ScriptWater u061a/u0591 never register one) |

Marked by blog2 (asmcheck clean apart from the unported CanMeleeAttack call and the bitfield NOPOS): 5BF4, 5A98, 1B78.

## rez6 (2026-10-24)

Marked (20 of 22; 105/107 behaviour rows): GetSearchPath 2A08, ProjectileInfo 45D8, GetContactDamage 4B00, ~CBloggMouthVulnerability 0,
~CBloggBodyVulnerability 17A8 (header inline); GetNextPositionIndex 2A10, CanRangedAttack 5D88, CanMeleeAttack 5E80, FindNearestHint 1C0C,
FindAttackPositions 3B1C, ComputeAttackPositions 7E90, Dead 38C4, GrabBall 65F4, LaunchBloggProjectile 43D8, DoUserAnimEvent 4B4C,
LoadBlogg 108, SLdrBlogg ctor E5C, LoadTypedefBloggStruct 10E0, CBlogg ctor A02C, `__sinit` BEFC (marker on `skTriggers`: 45 ptmf blobs
named through the TSV equal skTriggers+skStates+skCodeFuncs in order; bss_14 = 0.5235988 = skAttackAngleStep, bss_18 = (0,0,5) = skWaterTestOffset).
**Left: Think 87C8 and AcceptScriptMsg 8F7C** (see below).

Runs (`tools/cov/specs/force_lib.py` shared helpers; all via `/force.sh` with `COV_GDB_SCRIPT`, `--fast`, <= 1900 frames):
- `blogg_force3.py` + `blogg_forced3.con` (07_swamp_shaft, no team manager) / `blogg_forced3g.con` (08_swamp_grappleguardian, 3 team managers,
  2 ScriptWaters); env `BLG_AT=1500 BLG_GROUPS=misc|attack|state|uevent` (the hook runs at the 1500th live Think and calls the leaf bodies with
  the members each arm needs, then restores them). misc: GetNextPositionIndex, CanMelee/RangedAttack (team answers by a gate), FindNearestHint
  (real hints, inactive / in-use / stale ids, LOS on/off). attack: FindAttackPositions (OnPath answered by a gate, a real water id + its
  bounds, a real team joined by four Bloggs, role owner invalid), ComputeAttackPositions (hint/no-hint, player invalid, the three direction
  loops via OnPath answer lists: 117 calls). state: the three leaf getters, GrabBall (team none/real, StartAttack 1/0, player valid/invalid,
  dt 0.016/1.0), Dead (fluid/fade combinations). uevent: DoUserAnimEvent types 0 and 26 (player valid/invalid, CanCreateProjectile refusal by
  a gate, the projectile token's bytes cleared for TryCache false: player-null call only, the player-valid path reads the token item first).
- `blogg_force4.py` + `blogg_load.con` (07_swamp_shaft): first LoadBlogg input patched (0x2dd3662e -> 0xfeedbeef skip arm, isMegaBlogg 1 so
  the first CBlogg is built through the mega arm, cleared on the returned object), first LoadTypedefBloggStruct (0x3e505ddb -> 0xfeedbeef);
  `spawn BLOG` without ancs = LoadBlogg's model-null return.

Dead arms (proofs): ComputeAttackPositions `hint != nullptr` after FindNearestHint returned its id (only returns ids it resolved), `flatAngle >= 0`
false and `flatAngle > cur` (GetAngleDiff = acos of a clamped value, never negative/NaN: CVector3f.cpp:103-117); GrabBall switch default
(EStateMsg has three values); LaunchBloggProjectile homing=true (both callers pass false) and rs_new null; CBlogg ctor null arms of
`BodyController()` and of the two rc_ptr copies/ifs of members constructed by rs_new in the init list.

asmcheck false positives worth knowing: SLdrBlogg ctor XSTORE (every field; the checker cannot place the stack struct) - verified by hand,
store by store (0x2f8..0x4d0, 0x7ac..0x7f0, TARGET_PC extras); ctor BIT x681_10/40/80 = the inlined EnableBurn/Freeze/Slow(false).
DoUserAnimEvent type 0x1a really falls through to the base call (asm 0x4FD4 -> 0x4FE4), as the C++ does.

Left for the next slice:
- Think (87C8): untaken in the natural + forced runs: the MoveToAttackPosition/MoveToPlayer/FacePlayer turn-speed arm, the Dying speed arm,
  IsPlayerWithinChargeRange branches, the mouth-open sound arm (ChargeAttack && inRange && mMouthClosed && xbc5_28_), the morph-ball
  arm (spawned-morph-state test), mBallGrabbed, the hit-colour lerp (xb68_ > 0), and the whole mega-Blogg phase block (the first Blogg
  of blogg_force4's load has mPhases = 3; the rest of the object is non-mega). Plan: direct Think calls per mState with the members written.
- AcceptScriptMsg (8F7C): AreaLoaded pivot loop, HitObject (charge arm 0xbc4&0x2, collision-actor sender, mouth damage), Damage (the
  weapon-up arm of blogg1), bomb / power-bomb sender arm, InternalMessage0 (kill), fall-through. Plan: a `send`/direct-call queue with
  a crafted CScriptMsg and real CollisionActor / weapon senders.
