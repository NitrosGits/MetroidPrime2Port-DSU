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
| fn_27_FD2C AcceptScriptMsg, kSM_Landed | 0xFFF4 `bl RestoreDefaultJointVulnerability` | `ResetBodyVulnerabilities(mgr)` (Reset, not Setup) |
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

## Markers from merged coverage (gren3, 2026-10-24)

Reran the gren1/gren2 drivers (`/tmp/opencode/gren3/runall.sh`, build-port-cov), merged gcov: 249 functions, 41 with
every arm taken. 24 more bodies marked after a static asm check (`asmcheck` NOPOS/BIT hits verified by hand, MWCC
bitfields MSB-first): 4978 6240 6274 7798 8FB4 8FC0 94D0 A5B8 B800 B824 B8F0 BFF0 D3C4 E188 EC50 EDB0 EEE0 12118
12430 12EA8 12FB4 13034 1320C 139D4. UpdateMovement (D3C4): 3.6 (rodata_250) and 1.7 (rodata_258) confirmed; the
TARGET_PC arm without the 2x stays as is. Ctor constants checked: -1000, 1.0, 0.15.

Not closed (no forced arms run this slice): bodies with 1-2 untaken arms (StartElectricBeam, CreateCollisionManager,
ReleasePlayer, Charge, GrappleSlide/Bonk/Struggle/Break, Patrol, GetContactDamage, BeamBadAngle, BeamHitWall,
PickGrappleSide, PauseBetweenBites, DeliverCommand defaults) and zero-coverage bodies (IsBeamBlockedByHint,
IsNearAvoidHint, GetJumpCost, StruggleOver, StopStruggling). They need new `.con` drivers / forced arms.

## gren4 (forced arms)

Marked with all arms taken (arms marked F need forced vars/statecall msg 3; driver `tools/cov/specs/grenchler_gren4.con`, new `gren.*` console vars in `PortConsoleVar`): StartElectricBeam (F), CreateCollisionManager (F), ReleasePlayer (F), GrappleStruggle (F), GrappleBreak (F), Patrol (F), BeamHitWall (F), PickGrappleSide (F), StruggleOver (F), StopStruggling (F).

Still open at gren4 close: see gren5 below.

## gren5 (hints, jump cost, slide, charge)

Marked with all arms taken (driver `tools/cov/specs/grenchler_gren5.con` in 08_swamp_grappleguardian_dark, plus `grenchler_gren5_ent.con` in 01_swamp_entrance for Charge's path-over arm; gcov of both unioned). New `PortConsoleVar` vars: `gren.hint`, `gren.hintcall`, `gren.jumpcost`, `gren.slidestart`, `gren.swoosh`, `gren.contact`, `gren.path`; `gren.hitzero 2`. No behaviour differences found against the asm.

| function | arms | forced (cmd) |
|---|---|---|
| GetContactDamage 1A68 | 6/6 | forced (gren.contact 0-3) |
| BeamBadAngle 2E08 | 6/6 | forced (gren.swoosh 1-4) |
| IsBeamBlockedByHint 3C1C | 20/20 | forced (gren.hint 1-8, gren.hintcall 2/3) |
| IsNearAvoidHint 3EEC | 12/12 | forced (gren.hint, gren.hintcall 1) |
| PauseBetweenBites 45D4 | 6/6 | forced (statecall, tail destroyed/kept) |
| GetJumpCost 4EA8 | 8/8 | forced (gren.jumpcost 1-5) |
| Charge 67C0 | 8/8 | forced (gren.path 1-4; path found and run out only in 01_swamp_entrance) |
| GrappleSlideBonk A02C | 6/6 | forced (statecall, gren.slidestart) |
| GrappleSlide A114 | 10/10 | forced (gren.hitzero 2, gren.slidestart) |
| DeliverCommand<ProjectileAttack/Step/MeleeAttack/Taunt> 13CA8/13DB4/13ED0/13FDC | 6/6 each | forced (statecall msg 0-3) |

asmcheck leftovers are tool limits: unported-callee notes (DestroyGrappleBeam, SetSubmerged, CLineSeg ctor are cited elsewhere or inlined), bitfield NOPOS/BIT (MSB-first, checked by hand), Charge BRANCH count (inlined helpers).

## gren6 (small predicates, 2026-10-24)

Driver `tools/cov/specs/grenchler_gren6.con` (guardian area, frames 1500, `--fast`), alone on a fresh cov build
(older gcda mismatch the new notes). Asm checked line by line (constants 2, 1, 0, pi/2, 0.2, 1.5882496,
1.675516 read from the rodata; JustHit NaN arms: `ble` taken -> false, the C++ `>` agrees).

| function | arms | forced (cmd) |
|---|---|---|
| UpdateDamageFlash 1FAC | 2/2 | forced (gren.flash 0/5, gren.fncall 4) |
| SpawnBeamHitEffect 3048 | 7/7 | forced (gren.fncall 5, 6 = no explosion asset) |
| JustHit 48C8 | 8/8 | forced (gren.guardian, gren.lasthit, gren.turn 0/1, gren.fncall 1) |
| PlayerBehindMe 85C4 | 2/2 | forced (gren.x2c, gren.fncall 2) |
| LaunchToJumpTarget 96E0 | 2/2 | forced (gren.launched, gren.fncall 3) |

New vars (`PortConsoleVar`): gren.lasthit, guardian, x2c, launched, flash, turn, fncall.

## gren2 (visor effects, 2026-10-24)

Driver `grenchler_gren6.con` extended (@f700-@f750). Asm of 7848/7A38 read line by line (strings at
rodata_284+0x562/0x581, optional flags 0xd8c/0xef0, asset id `addis/cmplwi 0xffff`, ctor args). Dead arms:
the `__nw` null test (`mr. r29,r3; beq`) never fires (rs_new does not return null), which accounts for the last
3/5 (2981) and 2/4 (2997) arms.

| function | arms | forced (cmd) |
|---|---|---|
| CreateVisorEffect 7848 | 8/9 + dead | gren.fncall 7 live, 8 asset invalid, 9 token unset |
| CreateDamageVisorEffect 7A38 | 7/8 + dead | gren.fncall 10 live, 11 token unset |

Not marked: CreateGrappleHitVisorEffect 7BA4 (fncall 12/13; the `magnitude > 23` skip arm was not reached, the
tp to 60+ units away did not take), and every other candidate: their remaining arms need forced states
(SpawnTail/DestroyTail ing-possessed arms, PullPlayer player state, GetManeuverTarget, FindJumpPoint, etc.).
New fncall cases 7-13.

## gren3 (lane, base 933e8fe89)

Probes: `gren.fncall 14-24`, `gren.mhint`, `gren.jp` (tools/cov/specs/grenchler_gren6.con guardian, grenchler_gren3_ent.con entrance u05c4). Merged gcov, final cov build.

| Function | Offset | Result |
|---|---|---|
| SpawnTail | fn_27_F1C8 | marked, all arms |
| CreateGrappleHitVisorEffect | fn_27_7BA4 | marked, only dead 2939:3 (`__nw`/EH) |
| GetManeuverTarget | fn_27_5640 | marked, only dead 3387:1 (EH edge) |
| MarkPathRegion | fn_27_E390 | open: 1251:0,2,3 (null region / same region / flags!=0) |
| FindJumpPoint | fn_27_E634 | open: 1211:1 (PathExists fail, +10000) never taken even with far target |
| SEffectA/SEffectD ctors | 127FC/11FCC | open: invalid-asset arms (445,450,455; 500-501) |
| CanBeShot/GetSearchPath/SConeVulnerability ctor | 94/0/13A94 | asm-matched statically, no hpp gcov evidence |

## gren4 evidence (branch decomp/gren4b)

Forced runs: `tools/cov/specs/grenchler_force4.py` + `grenchler_gren4_ent.con` (merged g4n+g4a+g4f gcov).
- MarkPathRegion E390: null region, same region, flags != 0 forced via CALLS scenarios; ALL-ARMS.
- FindJumpPoint E634: PathExists forced to 0 (FORCE_INNER, period 1); failure arm taken.
- SEffectA 127FC / SEffectD 11FCC: invalid-asset arms forced; remaining uncovered arms are null-new/throw edges (dead).
- CanBeShot 94 / GetSearchPath 0 / SConeVulnerability ctor 13A94: header lines, branch-free. gcov: GetSearchPath ~9000 hits, ctor 2-3 hits, CanBeShot never called (`return true;`, asm read).

## gren5 evidence (branch decomp/gren5b)

Marked (all arms shown, merged gcov of natural + forced runs, caught/reach-stub = 0 everywhere):
PreRender BB80, PreRenderAllViewports 177C, PullPlayer 2A78, Maneuver 533C, Pursue DA78, DestroyTail F398, Jump 9018.
Runs: `tools/cov/specs/grenchler_force5.py` (one CALLS group per run via `F5=<label>`; entrance room for
non-guardian, `08_swamp_grappleguardian_dark` for guardian-only arms). Direct calls with stubbed
callees; water uid is decimal (u0459). DestroyTail 1001:3/5 and 4/5 are `__nw` null / throw edges (dead arms).
Not done: FollowAttackPattern (needs waypoint-navigation scenarios), LoadGrenchler, and the ctors/dtors/`__sinit`s.

## gren6 lane (base 494a2ba23, 2026-10-24)

Fixture vars (TARGET_PC, `PortConsoleVar`): `gren.fap <n>` builds scripted waypoints / AI waypoints with editor ids, adds an
attack/follow connection to the Grenchler, runs FollowAttackPattern Activate+Update (+Deactivate, msg 3 for n 1) and deletes
the fixtures; `gren.jt <n>` builds jump points + waypoints around the player (the player must be 20+ away: the driver lifts it
44 up in the same tick) and runs FindJumpTarget. Driver `tools/cov/specs/grenchler_gren6b.con` (01_swamp_entrance, 750 frames);
gdb arm `tools/cov/specs/grenchler_force6.py` (force_ret CALLS: GetInUse stubbed to 1 for a direct call).

| function | arms | forced (cmd) |
|---|---|---|
| FollowAttackPattern 425C | 20/20 | forced (gren.fap 1-7): no connection, waypoint ahead/behind (0x4388 `cror eq,lt,eq; bne` = `<= 0` sets mInPosition 0x4394), AI waypoint with / without arrived-next link, body in Jump (0x43CC cmpwi 0xd = kAS_Jump 13) or not, mSubmerged flipped, msg 2 / 3 |
| FindJumpTarget 4AA4 | 30/30 | forced (gren.jt 1-15; 6 = waypoint 3D+20 behind: a Grenchler standing in water triples maxCost at 0x4BCC); jump point in use (0x4C58 `beq`): forced (GetInUse stub, grenchler_force6.py) |

Asm checked line by line: Frustrated is vtable +0x1A8 (0xA54 vtable word 106 = fn_27_2084); xa24 bit 0x40 = mStartedSubmerged (0x4B04 rlwimi
6,25,25), 0x20 = mTargetValid (0x4D9C rlwimi 5,26,26); x6f0 0x80 = CWaypointNavigation::mInPosition (0x4394); object list 0x838 = index 5
kOL_AiWaypoint; cost compares `blt / bgt / bge` skip = `cost < 0 || cost > maxCost`, `cost < best` (NaN lands the same). No behaviour differences.
