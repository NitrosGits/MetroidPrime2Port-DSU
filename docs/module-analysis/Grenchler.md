# Grenchler REL (module 27): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CGrenchler.cpp` and `CGrenchlerTail.cpp`
(Grenchler, Grapple Guardian, Dark Grenchler); the port's fixes are `TARGET_PC` blocks in
`CGrenchler.cpp`. Retail asm: `build/G2ME01/Grenchler/asm/MetroidPrime/Enemies/CGrenchler.s`
(after `python3 configure.py && $T/ninja`).

## Behaviour fixes (gren1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_27_FD2C AcceptScriptMsg, contact-damage arm (guardian, tail gone) | 0x10758 `bl CreateDamageVisorEffect` | `CreateVisorEffect` |
| fn_27_9018 Jump, Activate | 0x9080 `bl ResetBodyVulnerabilities` (reflect) | `SetupBodyVulnerabilities` |
| fn_27_DA78 Pursue, Activate | 0xDC08 `bl SetupBodyVulnerabilities` (MakeIgnoreRadius) | `ResetBodyVulnerabilities` |
| fn_27_F398 DestroyTail, non-guardian arm | 0xF434 `bl SetupBodyVulnerabilities` | `ResetBodyVulnerabilities` |
| fn_27_D3C4 UpdateMovement, non-guardian arm | 0xD71C-0xD77C: 0.15 (rodata_254) * (-1 (rodata_148) * Forward) | `2.f *` that (the guardian arm 0xD5F4-0xD678 keeps 2 * 0.15) |
| fn_27_7BA4 CreateGrappleHitVisorEffect | 0x7C20 `fcmpo; bgt skip`: NaN takes the effect arm | `<= 23.f` (cror; NaN skips) |

The NaN fix duplicates the whole `if` statement in both branches: a block that only swaps the
`if (...) {` header leaves the tools' body parser (asmcheck, retail_diff) one brace short.

## Behaviour fixes (gren2, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_27_E1A4 YellowHitReact, Activate with the tail destroyed | 0xE258 `bl ResetBodyVulnerabilities` | `SetupBodyVulnerabilities` |
| fn_27_E1A4 YellowHitReact, Deactivate | 0xE294 `bl SetupBodyVulnerabilities` | `ResetBodyVulnerabilities` |
| fn_27_EAE4 Null | 0xEB24 `bl SetupBodyVulnerabilities` | `ResetBodyVulnerabilities` (gren1 marked Null before this fix) |
| fn_27_8FC0 RestoreDefaultJointVulnerability | 0x8FEC `bl SetupBodyVulnerabilities` | `ResetBodyVulnerabilities` |
| fn_27_FD2C AcceptScriptMsg, kSM_Landed | 0xFFF4 `bl RestoreDefaultJointVulnerability` | the Reset call inline |
| fn_27_13C8 SLdrGrenchler ctor | 0x1438/0x143C: tailHitSound/tailDestroyedSound default -1 | 0 |

The port's `ResetBodyVulnerabilities(mgr)` helper is gone (no PC caller left).

## Spawns that work

| area | layers | ids (this save) | use |
|---|---|---|---|
| Torvus 01_swamp_entrance (`3dfd2249,01_swamp_entrance`) | 4, 8 after the warp (@f300+) | u05c4 u05be u0596 (200 hp; `send <id> Activate`) | natural fight, forced states |
| Torvus 08_swamp_grappleguardian_dark | 3 | u052e (guardian, 700 hp, starts in Patrol; ignores `damage` while the tail is on), u04fa (1000 hp) | guardian arms |

FSM2 state names (`fsm <id> Bogus` lists them): Dead, Start, Lurk, Patrol, Turn, Pursue, BiteAttack,
BeamAttack, Charge, Null, Maneuver, ChargeFailed, PauseBetweenBites, PauseBetweenBeams, BurstAttack,
FollowAttackPattern, ShakeOff, Grapple{Loop,Struggle,Pull,Bite,Break,Abort}, MorphballBite, Jump,
TurnToJumpEnd, Taunt, YellowHitReact, GrappleSlide, GrappleSlideBonk, ... `WalkTowardPlayer` is
entered and left at once.

## Survey (gren1, 2026-10-23)

`tools/upstream_hollow.py Grenchler`: no hollow bodies. Objdiff of every CGrenchler function under
100%: apart from the six fixes above, only regalloc, scheduling, inlined vs called ctors, string and
rodata addressing. Checked equivalent: ctor (rodata_280 = 160.0; retail assigns the optional token
through `optional_object::operator=` where upstream constructs it in place, same result on an empty
optional), Dead, UpdateEffects, GrappleSlide, CrystalDamaged, PlayerBehindMe, JustHit, Bored,
EnableGroundCollision, UpdateDamageFlash, InBiteRange, UpdateGuardianBlend, PreRenderBoneTracking,
MarkPathRegion, Charge, Maneuver, SpawnTail, LaunchToJumpTarget, GetJumpCost, FindJumpTarget,
FindJumpPoint, UpdateCapturedPlayer, FollowAttackPattern, IsBeamBlockedByHint, StartElectricBeam,
GrapplePull, GetManeuverTarget, CreateCollisionManager, IssueDeathBodyCommand (inlined CBCHurledCmd;
argument order not fully traced). AcceptScriptMsg's `ResetBodyVulnerabilities(mgr)` (TARGET_PC
link shim) has the same body as retail's `RestoreDefaultJointVulnerability`. Not deep-checked:
the SLdrGrenchler ctor (59%; the loader overwrites its defaults), ~CGrenchler (89.9%),
CGrenchlerTail.

Per-function table: `Grenchler_functions.tsv` (B upstream body, H weak copy/template, T REL runtime).

## Coverage (gren1)

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON`; each run:
`BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/grenchler.py <out> 3dfd2249,01_swamp_entrance --con <con> --fast --frames N`,
then `python3 tools/cov/gcov_report.py build-port-cov <gcov dirs...> --module Grenchler`.
Drivers: `grenchler_entrance.con` (natural fight, shots of every beam, missile, kill; 3800 frames),
`grenchler_forced.con` (fsm through Jump/Taunt/BeamAttack/Charge/Patrol/Turn/BurstAttack/
MorphballBite/GrappleSlide(Bonk), statecall of the six Grapple states; 2800 frames),
`grenchler_guardian.con` (08_swamp_grappleguardian_dark: both actors fight, missiles/charged shots,
kill; 3200 frames). Merged: 249 bodies, 25 with every arm taken.

Marked by gren1 (31; every arm taken or branch-free with every line run, objdiff 100% or checked
equivalent): GrappleAbort, PauseBetweenBeams, TurnToFaceTarget, JustBiteAttacked, ChargeFailed,
SReflectInfo::Reset, PushAttackHistory, ManeuverDone, GetManeuverRangeWeight, PlayerSubmerged,
GetFadeOnDeathTime, ChargeFinished, ApplyBiteDamage, DisableGroundCollision, Backstep, IsFacing,
BiteAttack, Lurk, SetAttackState, SetupStateMachineHelper, ThinkBoneTracking,
GetPlayerBeamTargetPosition, UpdateTeammateScan, PreThink, Null, InRange, Bored, GetVulnerableAngle,
SBeamAttack ctor, SReflectInfo ctor, HasAttackPattern. `retail_diff --module Grenchler`: only
"unported callee not named" findings (calls into unmarked helpers).

Covered but not marked (slice cap of ~30): about 50 more bodies are branch-free with every line
run (Stuck, Frustrated, TookKnockback, IncrementAttached, CanBite, IsPointVisibleToPlayer,
ResetSteering, ...); a follow-up slice can mark them after a per-function asm check.

Near-complete, not marked (gcov line:arm untaken; msg-switch defaults are not provably dead):

| function | untaken |
|---|---|
| DeliverCommand<...> (4 instances) | 301:3/4 msg switch default, 307:0/2 |
| UpdateMovement | 1434:2/4 |
| InBiteRange | 1053:3/4 (guardian with action 8) |
| PreRender | 1770:1/2, 1777:1/2 |
| Patrol, GrappleSlide, GrappleSlideBonk, GrappleStruggle, GrappleBreak, Charge, MorphballBite | msg switch default 3/4 |
| IsGuardianCharging | 2250:3/4 (crystal state 4) |
| ResetBodyVulnerabilities(mgr, vuln) | 2265:1/2 (no collision manager) |
| StartElectricBeam | 2836:0/2 |
| CreateCollisionManager | 2486:2/3, 2489:1/2 |

Never ran: the guardian grapple chain (CGrenchlerTail, DestroyTail, SpawnTail, GrappleLoop's real
entry, PullPlayer's arms, ReleasePlayer, the visor effects, UpdateBeamTrace, Jump's body after
Activate). `grapple <id>` only takes CScriptGrapplePoint ids; the guardian's tail is not one, so a
grapple run needs real lock-on (`press l`) on the tail or a new console command.

## Guardian grapple chain (gren2)

The guardian's vulnerability is 0 for every weapon (retail too, 0x10180), so the crystals cannot be
broken in a run: the chain is entered with the console vars (`forced (var)`): `var u052e gren.scanned 1;
var u052e gren.crystal 4; fsm u052e Pursue`, the tail broken with `var u052e gren.tail 1`, the
non-guardian DestroyTail arm with `var u0596 gren.tail 1` in 01_swamp_entrance. Drivers:
`grenchler_grapple.con`, `grenchler_grapple2.con`, `grenchler_grapple3.con` (08 dark; grapple3 ends with
the `forced (statecall)` defaults MorphballBite 3, GrappleBreak 3, Null 0) and
`grenchler_entrance_tail.con` (missile hits: PlayTailHitSound, contact damage 0x10758). The runs
exercise gren1's DestroyTail (0xF434) and contact-damage (0x10758) fixes.

Marked by gren2 (61; every line run and every arm taken or branch-free, asm checked):
EndMorphballCapture, UpdateCapturedPlayer, Stuck, PlayerStuck, CrystalDamaged, Frustrated,
TookKnockback, IncrementAttached, DecrementAttached, BeamHitSticky, BeamHitPlayer, RemoveExplosion,
CanGrapple, PlayTailHitSound, PlayTailDestroyedSound, ShakeOff, AttackPatternOver, PauseOver,
IsNearPath, PickJumpTarget, HasValidJumpTarget, IsPointVisibleToPlayer, CanBite, ReturnToPatrol,
TurnToPlayer, UpdateChargeSteering, ResetSteering, CreateGrappleBeam, RemoveVisorEffect, GrabPlayer,
Render, Alerted, Attacked, EnableGroundCollision, IsGuardianCharging, FacingPlayer,
IsPlayerLookingAtMe, ForceGrapple, IsFacingTarget, GetAimPosition, GetOrbitPosition,
PreRenderBoneTracking, ShouldTrackPlayer, FluidFXThink, UpdateGuardianBlend, PlayYellowHitReact,
YellowHitReact (forced (var)), Submerged, GetPathArea, InBurstRange, InChargeRange,
GetTargetPosition, SetPathArea, GetTailHealth, MorphballBite (default arm forced (statecall)),
~CGrenchler (retail: member dtors only; the 9/14 gcov arms are the compiler's member-dtor checks),
CGrenchlerTail ctor/Think/GetTouchBounds/PreRender/StopMoving. port_progress: Grenchler 92/377.

Not marked (gcov line:arm untaken):

| function | untaken |
|---|---|
| DestroyTail | 966:0/2, 973:3/5, 973:4/5 (ing-possessed arms; no ing-possessed Grenchler in the data) |
| ReleasePlayer | 2/4 arms |
| CreateDamageVisorEffect | 6/8 (its 0x10758 call ran) |
| SpawnBeamHitEffect | 6/7 |
| GrappleBreak | 4043:3/4 (the msg-3 statecall did not reach it) |
| CGrenchlerTail::CollidedWith | 62:3/4 |
| RestoreDefaultJointVulnerability | never ran |
| UpdateDamageFlash | 3/9 lines |
| GrappleSlide, GrappleSlideBonk, GrappleStruggle | not entered: no immune dynamic solid lies in the beam path in either area |
