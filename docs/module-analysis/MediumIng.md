# MediumIng (module 41)

Current: **100/100 behaviour bodies** (ming6, 2026-10-24); completion and evidence below.

The Hunter Ing REL. The upstream `src/MetroidPrime/Enemies/CMediumIng.cpp` has the class, the
loader, the ctors and empty stubs; the port's bodies are in
`src/MetroidPrime/Enemies/PortCMediumIng.cpp` (the upstream stubs they replace sit under
`#ifndef TARGET_PC`). Function table: `MediumIng_functions.tsv` (class B/H/T, slice, marker).

## Status (2026-10-24, lanes mi1 + mi2 + mi3 + mi4 + mi5 + ming1 + ming3 + ming4)

- mi1 (slice 1, `PortCMediumIng.cpp`): Think, AcceptScriptMsg, PreRender, Render, Touch, Death,
  the mist/safe-zone/listen bodies, CalculateSeparation and the helpers UpdateTimers 3274,
  ScanProjectileThreats 4644, SetMistCollision 49CC, IsMistDecayed 5DC8, IsBlocked 5E0C,
  UpdateMistState 6078; the tentacle. mi2 (slice 2, TARGET_PC in `CMediumIng.cpp`): every FSM
  state, SelectTarget, Set*Destination, FindSafeZoneRetreatPoint, FindClosest*, the tentacle
  helpers 4308/43E8/4470/4598 and the jump-route statics 6F18/6D08/6BFC.
- mi3: one definition each for 49CC/5DC8/5E0C (mi1's kept; both equal to the asm); the states
  played in area 46. 32 of 156 B functions marked; per-state open arms: `MediumIng_states.md`.
- Unmarked bodies with open arms: most states (team-AI, jump-point, safe-zone-success and null
  guards), CalculateSeparation 2024 (flee CanBeNormalized false 0x2298, other-area neighbour 0x2094;
  the zone loop ran with `mediuming_zone.con`). JumpRouteEstimate 6D08 /
  SearchJumpRoute 6BFC never ran: area 46 has no AiJumpPoint.
- Death runs more than once per kill, as in retail: 1B18 has no guard (CPatterned::Death, then
  RemoveEmitter 0xce8/0xcec, AddMaterial 0x25) and CPatterned::Death guards on mAlive. The first
  call comes from CStateManager::ApplyLocalDamage (mAlive 1); later ones (mAlive 0) come from
  CPatterned::KnockBack under projectile damage while the dead Ing is still shot (gdb
  `$_any_caller_matches(".*KnockBack", 12)`: 27 of 28 in the forced run).
- mi4: Dead 30B0 and PathCostSquared 6F18 marked (34/155 B). Dead leaves by DeathDelete
  (0x31D0 kAS_LieOnGround) after a shot kill and after console `damage <id> 999`; its mE1 drain
  arm is dead (SetFlagE1 unreferenced). SelectTarget 698C / SetDashDestination 261C match the asm
  (open arms in `MediumIng_states.md`). Plain-`bge` clamps (NaN -> 0 / keep left) fixed in Dead,
  SetDash, CalculateSeparation. 3064 is compiler glue (H). The upstream-matched bodies (triggers,
  ctors, loader) carried no marker until ming1: the port compiles upstream's code unchanged.
- mi5: SelectTarget 698C, SetDashDestination 261C and CalculateSeparation 2024 marked (37/155 B)
  from `mediuming_forced3.con` (console `pathfail`, `setarea`, `spawn AJMP`, `codecall`) merged
  with the zone and play runs. CalculateSeparation's flee CanBeNormalized-false arm 0x2298 is dead:
  retail Flee returns the normalized delta or the transform's forward column, both unit length.
- ming1: gcov of the seven drivers (`mediuming_{play,forced,forced2,forced3,dead,zone,dash}.con`)
  marks 20 upstream triggers/helpers whose every line and branch ran (1E28 1E4C 2328 2390 23E4
  2438 2534 2558 2B74 2BCC 2BD8 2C2C 2E04 2E50 3268 4628 5BD8 5C24 5C68 5D7C; retail and host are
  both branchless or both arms ran) and SetMeshPathDestination 2A74 (both reachable-point arms).
  55 compiler-glue rows reclassified H. `tools/cov/specs/mediuming_triggers.py` (ret per trigger)
  is too slow for full drivers (about 366 frames in 900 s); use it with short forced scripts only.
  Still open: the Should*Attack/Leash/HasPattern/IsLuredBySafeZone/ValidateSafeZone arms, team AI
  (QuitTeam/JoinTeam, Arm/MeleeAttack 0x890/0x961), IncrementFrustatedCounter/IsMisting (never
  called), and the state arms in `MediumIng_states.md`.
- asmcheck: the bitfield run mC0..mE2 (0xCFC..0xCFE, MSB first: mC0 = 0xCFC 0x80) has no stated
  anchor in the upstream header, so every bit is NOPOS/BIT. Checked by hand for the marked
  states.

## Where it spawns

Sky Temple Grounds area 46 (`3bfa3eff,02_temple_exploration_dark`): CMediumIng u0417 (208.5,
-185.8, -47.2) and u0454, eight inactive tentacles u0491..u0494, u0498..u049b at (0,0,0). Agon
Wastes `04_sand_mining_dark` (layers 4..6) has none on the default card.

## Run

    tools/cov/run.sh tools/cov/specs/mediuming.py <out> 3bfa3eff,02_temple_exploration_dark \
      --con tools/cov/specs/mediuming.con --fast --frames 1000

1000 updates, exit 0, `[port] caught` / `reach-stub` 0. The driver clears mC1 (the Ing shows), takes
the scan visor, makes noises 6 and 4, shoots it (hp 150 -> 98), places it on the player (Touch),
extends and retracts two tentacles (they end inactive after the shake) and kills it (fsm Dead).
Screenshots: before (mC1 set, nothing drawn) / after (the Ing drawn) / scan visor / tentacles /
death, kept by the lane in `/tmp/opencode/mi1-shots/`.

## Arms (the run above; gdb spec keys)

| fn | function | arms taken (hits) | not taken |
|----|----------|-------------------|-----------|
| 7804 | Think | mC1 0 (1124) / 1 (514); mMistState 0..4 | mC0 1 (no mist attack without slice 2) |
| 3274 | UpdateTimers | mC2 0 (1200) / 1 (562) | - |
| 4644 | ScanProjectileThreats | mMistState 0 (1124) | other mist states skip the scan by design |
| 6078 | UpdateMistState | states 0, 1, 2, 3, 4; mC5 0/1 | - |
| 5DC8 | IsMistDecayed | ret 1 (2572) / 0 (184) | - |
| 49CC | SetMistCollision | solid 0 / 1 | - |
| 5E0C | IsBlocked | ret 0 | ret 1 (nothing in the way at the spot) |
| 1680 | UpdateMistEffect | mC0 0 (1762) | mC0 1 |
| 198C | CreateMistEffect | 2 (one per Ing) | - |
| 155C | UpdateCurrentSafeZone | 2 | - |
| 7478 | Listen | noise 2, 4, 6 with mAlive; ret 0 and 1 (noise 4) | dead Ing |
| 79C8 | AcceptScriptMsg | 8 message ids (XCRT, XALD, ... 400 cap) | - |
| 75A4 | PreRender | mC1 0 (156) / 1 (64), mC3 1 | mC3 0 |
| 74F8 | Render | 156, combat and scan visor | - |
| 761C | Touch | mC5 0 (3647) / 1 (353) | mD2 / mD3 1 (slice-2 attack flags) |
| 1B18 | Death | 2 | - |
| A4 | CanBeShot | ret 1 | - |
| 9F84 | T Think | extending, retracting (UpdateShake ran) | - |
| 94C8 | T PreRender | mExtending 0 (20) / 1 (20) | - |
| 93FC / 94A4 | T GetTouchBounds / GetProgress | 608 / 636 | - |
| 91B0 | T Touch | mCanDamage 1 (400 cap) | mCanDamage 0 (early return) |
| 9A34 / 9CE0 / 9ED0 | T Extend / Retract / UpdateShake | 2 / 2 / 2 | - |
| 2024 | CalculateSeparation | via PathFind; zone loop (mi4 `mediuming_zone.con`); other-area neighbour 0x2094 forced (`setarea u0417 40`, mi5) | flee CanBeNormalized false (dead) |

## States run (mi3)

    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/mediuming_states.py <out> \
      3bfa3eff,02_temple_exploration_dark --con tools/cov/specs/mediuming_play.con \
      --fast --frames 6000 --env MP2_GODMODE=1      # natural; forced: mediuming_forced{,2}.con
    tools/cov/gcov_report.py build-port-cov <out>/gcov ... --module MediumIng

Exit 0, `caught`/`reach-stub` 0, no NaN. Natural play reaches SubStart, MistIn, PathFind,
ArmAttack (tentacles extend), MeleeAttack, Jump, Dash, Taunt, BackUp and WaitForLocomotion.
`mediuming_forced.con` kills u0417 with charged shots (Dead, Death x2) and sends XENZ
(SafezoneReaction). `mediuming_forced2.con` activates safe zone u0451 (crystal u0452), then
MoveToSafeZone, MistAttack, Follow*Pattern, Idle and SelectTargetState via `fsm`, a leash teleport
to area 40, and mist states 1 and 4. Screenshots: `/tmp/opencode/mi3-shots/`.

## Port notes

- Host guards (each commented in the source): no blob effect when the ing-spot blob token is
  invalid; a null path area reads as no region; tentacle PreRender returns on a null anim data or
  fewer than three pose elements; alpha bytes are clamped like psq_st's u8 saturation.
- The tentacle ctor sets mCanDamage under TARGET_PC (retail leaves it to the zeroed heap).
- `CHealthInfo::GetLastDamageWeaponMode()` is a TARGET_PC getter for the Touch arm.
- Touch's static interference reads `mgr.PlayerState(0)` (retail: mgr+0x150C).
- CalculateSeparation uses CPatterned's steering (0x61C), not CMediumIng's own `mSteering`.

## ming3 (partial)

- Driver `tools/cov/specs/mediuming_ming3.con`, new `ming.{quitteam,jointeam,validate,conn,destx/y/z,destme}` console vars.
- Marked: HasApproachPattern (fn_41_2454), HasAttackPattern (fn_41_24C4): all 4/4 arms taken.
- Dead proofs: IncrementFrustatedCounter (fn_41_2548), IsMisting (fn_41_5CAC): the name appears only in the
  CMediumIng.cpp func-ptr tables, no caller in /tmp/opencode/pseudo/MediumIng/, 0 refs in the 110 FSM2/AFSM assets on disc.
- Still open (unmarked): QuitTeam (346 not-in-team arm), JoinTeam (358 cast-null arm, likely dead: the team predicate
  guarantees CScriptTeamAiMgr), Leash (495), ValidateSafeZone (522 Echo-keep arm), IsDestInsideCurrentRegion (543 true arm:
  FindClosestRegion returns null at the Ing in area 46 on the host), ShouldMist/Melee/ArmAttack (need line of sight).

## ming4

- Driver `tools/cov/specs/mediuming_ming4.con` (ming3 steps to f488, then f500+); new read-only console var `ming.los` (LOS tracker).
- Marked, all live arms run (ming4 gcov): QuitTeam (1ED0), JoinTeam (1F78), Leash (5CCC), ValidateSafeZone (1378),
  IsDestInsideCurrentRegion (2C6C), ShouldMist/Melee/ArmAttack (337C/3518/3640).
- JoinTeam null-cast arm is dead: GetAssociatedTeamId = CheckConnectedObject_if(CTeamAiPredicate), only CScriptTeamAiMgr matches.
- IsDestInsideCurrentRegion: retail fn_41_2C6C passes the Ing's WORLD position and mDestPos straight to
  CPFArea FindClosestRegion (CPathFindArea.cpp:386-423), which works in area-local space; unlike CPathFindSearch.cpp:73-74 there is
  no TransposeMultiply. Area 46's transform translates (201.17,-177.64,-62.69), so the world point is outside every region bound and
  null is returned: faithful retail quirk, no shared-code bug. True arm: player at world (-21.26,21.08,18.5), `place u0417 player 0`.
- An Ing placed inside an Echo (InternalMessage0) active zone vanishes within ~5 frames; Echo-keep ValidateSafeZone is run with the player outside.
- Arm too-close arm: needs IsInSafeZone (active zone, player inside) and the team id cleared (`ming.team 65535`) so CanStartAttack passes.

## ming5

- Driver `tools/cov/specs/mediuming_ming5.con` (spec `mediuming_ming2.py`, area 02_temple_exploration_dark, `--fast --frames 940`, `MP2_GODMODE=1`); the
  SafezoneReaction d1=1 and d1=0 arms need separate runs (a second Generate never issues while the body stays in Generate), merge gcov dirs.
- New vars (PortCMediumIng.cpp): `ming.connrt|conndg <uid>` adds a Retreat/DeGenerate Follow connection (uid >= 0xFFFF = nonexistent editor id,
  -1 clears), `ming.inpos`, `ming.fb0`.
- Marked, all live arms run (forced via fsm/statecall/place/var): MistAttack (37B0, 21/22), Dash (4FE8), SafezoneReaction (542C), MoveToSafeZone (5240),
  SetRetreatDestination (2564), FindSafeZoneRetreatPoint (13E4), FindClosestRetreat/DangerActor (1BA8/1CE8), SetLeashTarget (6880).
- Dead arms: switch `default` of MistAttack/Dash/SafezoneReaction/MoveToSafeZone (the FSM only sends Activate/Update/Deactivate);
  SetLeashTarget GetObjectById-null else arm (FindClosestConnectedActor only returns ids it resolved to active CActors).
- MoveToSafeZone FindSafeZoneRetreatPoint failure: `place u0451 5000 5000 5000` (zone off the nav mesh -> GetHeightOfPointAboveMesh kR_NoSourcePoint).
- Echo-zone vanish is retail behaviour: CScriptSafeZone::InhabitantIdle (CScriptSafeZone.cpp ~766-770, fn_66_1F80) calls DamageActor (~779-785,
  fn_66_1E50) on AI actors inside an active non-Normal zone. No port bug.

## ming6 completion (2026-10-24)

Base `ce667470d`, branch `decomp/ming6`: source-marker inventory **83 → 100/100**.
Seventeen markers, no B/H reclassification. Production logic changes are only the
two TARGET_PC alpha comparisons below. Getter `0` freshly matches100% objdiff and
its native address oracle confirms `&mPathFindSearch`.

### Retail clamp correction and regression

- MistIn: `5924 fcmpo cr0,f1,f0` compares amount with1.0; `5928 bge 5930`
  branches **to** the clamp. Unordered has LT clear, so NaN must clamp to1.
  Use `!(amount < 1.f)`, not `amount >= 1.f`.
- MistOut: `4C20 fcmpo cr0,f0,f1` compares0.0 with amount; `4C24 bge 4C2C`
  branches **to** the clamp. NaN must clamp to0. Use `!(0.f < amount)`.
- Both pre-fix quiet-NaN oracles failed in `nan-before2/forced.log`; after the
  correction both PASS in the full final run. Finite alpha0/1/2 cases cover both
  clamp directions. This is a branch-to-action case, not a branch-skipping-action
  inversion. All changed logic is inside the existing TARGET_PC block.

### Arm table (new markers only)

| fn | body / fixtures | final lines / edges |
|---|---|---|
| 0 | GetSearchPath; exact matched-body exception plus pointer oracle | objdiff100% |
| 150 | LoadMediumIng; shipped valid loads and `spawn MING prop 0xfeedbeef 42` (unknown property, no ANCS). Consumed-length oracles for valid and null-model loads PASS; **every generated `.inc` executable line hit**, including default skip. |18/19,17/19; two EH cleanup edges below |
| 85D8 | Main ctor; two real actors, field oracles for mist defaults, flags, invalid ids, search flags30b and height5/radius2 |65/65,14/14 ALL-ARMS |
| A0C0 | Tentacle ctor; eight real instances; field oracles for damage/extend/retract flags, times, length1 and copied attack time |19/19,9/9 ALL-ARMS |
| 4308 | ExtendTentacle; indices-1/0/1/4, missing id, real even/odd tentacles |7/7,8/8 ALL-ARMS |
| 43E8 | RetractTentacles; empty/nonempty list, missing/live id |5/5,4/4 ALL-ARMS |
| 4470 | PositionTentacles; empty/nonempty list, missing/live id, real skeleton/locators |11/11,9/9 ALL-ARMS |
| 4598 | SetTentaclesActive; on/off, empty list, missing/live id |6/6,4/4 ALL-ARMS |
| 6D08 | JumpRouteEstimate; missing jump, missing landing actor, real jump/landing/player at the same location; exact zero/FLT_MAX oracles |9/9,4/4 ALL-ARMS |
| 6BFC | SearchJumpRoute; missing jump, missing landing, first-search failure (no area), second-search failure (far destination), success with actual nav mesh |14/14,8/8 ALL-ARMS |
| 70B4 /71BC | Both follow-pattern states; non-Activate, no connection, real waypoint before/behind actor through temporary matching script connection. In-position oracles PASS |each9/9,6/6 ALL-ARMS |
| 5878 | MistIn; all three messages, spawn modes0/1/2, D0 on/off, command-ready/current-Generate/other body, both times and alpha clamps, NaN regression |34/34,15/16; default dead |
| 4B8C | MistOut; all messages, times0/3, ready/not-ready, alpha clamp sides/NaN, empty handles and two **real AddLoop emitters**, both removed |28/28,11/12; default dead |
| 5728 | Jump; all messages, invalid/valid jump id, in-position on/off |17/17,7/8; default dead |
| 3F3C | ArmAttack; team-id bypass, admission denied/accepted, mist, command readiness/body, times, E0, timer, tentacle index0/5, missing tentacle, positive/negative length, all-retracted and D6 |50/50,33/34; default dead |
| 3AE4 | MeleeAttack; team variants, mist/ready/body, time<0.5/<2/>2, counter0/4, timer below/above threshold, target outside/inside radius and real ApplyDamage |44/44,27/28; default dead |

Switch defaults are dead under the FSM's Activate/Update/Deactivate contract,
as for the earlier marked states. Loader's only remaining source block114 at
line323 is in host exception cleanup: both zero edges114→115/117 have no block
executions. Native cold disassembly shows RAII model/spline cleanup and
`_Unwind_Resume`, not a skipped property or successful-load branch.

### Run / fixture boundaries

`BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py SECS=480
tools/cov/force.sh tools/cov/specs/mediuming_ming6.py <out>
3bfa3eff,02_temple_exploration_dark --con tools/drivers/mediuming_ming6.con
--fast --frames 1300`.

Final post-marker/fix GPU run explicitly **exited normally**,103/103 suite calls
successful, all oracles PASS, zero failed sets/restores/calls/Python errors,
zero caught/reach-stub. Four explicitly logged GDB StartAttack return stubs
(deny/accept for each attack state) model the parent's team-admission input;
they do **not** claim natural team-manager lifecycle coverage. Other fixtures
call real helpers/nav/body-command/damage/audio paths. No native probes.

State clocks: actual root and nested FSM2 clocks are saved, set, then restored
per timed call; changing only the root misses a submachine's virtual GetTime.
Earlier diagnostic fixture errors (integer `2f` spelling, bool-to-float oracle
conversion) were corrected before the clean final run. Nav fixtures spawn a
real AJMP/WAYP and modify only their input positions/landing id; original fields
and the Ing's connection-vector metadata are restored after each call.

Coverage-only `mediuming_ming6_keep_static.cmake` retains callable local jump/audio
helpers; ordinary host and matching compiler options remain unchanged.

### Static audit and gates

Full retail listings for17 bodies and relevant anonymous helpers retained in
`/tmp/opencode/reports/ming6-evidence/`. All34 asmcheck findings resolved:

- Constructor initializer-list calls (path search, steering) are present; tool
  scans only the body. Tentacle A294/A2EC are temporary entity-info/connection
  vector destruction;8C08/8B5C are CMayaSpline/CCameraShakerData member copies.
- Loader1124/103C are generated SLdrMediumIng ctor/dtor; DF8/F90 are spline/
  container read/teardown glue; AB0 is optional-model copy (valid byte4c);
  A40 is camera-shake teardown;9B8 is the matched100% MediumIngData dtor.
  Export-name aliases map LoadActorParameters/LoadEditorTransform to native
  LdrToActorParameters/LdrToTransform4f. Generated `.inc` reads account for all
  Read/LoadTypedef and call/branch-count warnings; no reader case is omitted.
- Const jump casts inline through TCastToPtr. PositionTentacles' string cleanup
  belongs to its temporary locator name.
- Follow-pattern6f0 mask80 stores are `mWaypointNavigation.SetInPosition(true)`;
  its result was checked on both waypoint sides. Jump's inherited in-position
  flag is34c mask80 (`57B0/57B4`); the tool has no alias position.
- HasMist5C74 is the existing inlined helper, checked with both attack parents;
 53C0 is the13-float CMotionState copy after PredictLinearMotion, not a missing
  gameplay function. Prior ming1 H classifications remain unchanged.
- The two retail-diff NaN findings triggered the real fixes above; rechecking
  both clamp bodies now reports **no findings**. Remaining six retail-diff call
  aliases/glue findings are explained above; none unresolved.

Host/coverage links0 FAILED; manifest rc0; upstream configure+ninja and shasum
**87 files OK**. Non-PC strip-diff clean, whitespace clean. Review against
`ce667470d`:17 MARK and2 DEL (the two replaced clamp conditions), no other tags.
Only this changed family/callers' area was run, no unchanged-family sweep.
