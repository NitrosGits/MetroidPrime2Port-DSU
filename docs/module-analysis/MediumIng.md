# MediumIng (module 41)

The Hunter Ing REL. The upstream `src/MetroidPrime/Enemies/CMediumIng.cpp` has the class, the
loader, the ctors and empty stubs; the port's bodies are in
`src/MetroidPrime/Enemies/PortCMediumIng.cpp` (the upstream stubs they replace sit under
`#ifndef TARGET_PC`). Function table: `MediumIng_functions.tsv` (class B/H/T, slice, marker).

## Status (2026-10-23, lanes mi1 + mi2 + mi3)

- mi1 (slice 1, `PortCMediumIng.cpp`): Think, AcceptScriptMsg, PreRender, Render, Touch, Death,
  the mist/safe-zone/listen bodies, CalculateSeparation and the helpers UpdateTimers 3274,
  ScanProjectileThreats 4644, SetMistCollision 49CC, IsMistDecayed 5DC8, IsBlocked 5E0C,
  UpdateMistState 6078; the tentacle. mi2 (slice 2, TARGET_PC in `CMediumIng.cpp`): every FSM
  state, SelectTarget, Set*Destination, FindSafeZoneRetreatPoint, FindClosest*, the tentacle
  helpers 4308/43E8/4470/4598 and the jump-route statics 6F18/6D08/6BFC.
- mi3: one definition each for 49CC/5DC8/5E0C (mi1's kept; both equal to the asm); the states
  played in area 46. 32 of 156 B functions marked; per-state open arms: `MediumIng_states.md`.
- Unmarked bodies with open arms: most states (team-AI, jump-point, safe-zone-success and null
  guards), CalculateSeparation 2024 (hx < hy, the zone loop). JumpRouteEstimate 6D08 /
  SearchJumpRoute 6BFC never ran: area 46 has no AiJumpPoint.
- Death runs more than once per kill, as in retail: 1B18 has no guard (CPatterned::Death, then
  RemoveEmitter 0xce8/0xcec, AddMaterial 0x25) and CPatterned::Death guards on mAlive. The first
  call comes from CStateManager::ApplyLocalDamage (mAlive 1); later ones (mAlive 0) come from
  CPatterned::KnockBack under projectile damage while the dead Ing is still shot (gdb
  `$_any_caller_matches(".*KnockBack", 12)`: 27 of 28 in the forced run).
- Dead 30B0 removes the Ing only by DeathDelete when the body state is kAS_Death (0x31D0), or by
  DeleteObjectRequest once the mist effect drains (mE1, 0x31E8). A console kill (`damage <id> 999` without knockback) leaves the
  Ing in Dead indefinitely; a shot kill gets it deleted. Treat the console case as a harness
  artefact.
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
| 2024 | CalculateSeparation | not run | slice 2 (PathFind) |

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
