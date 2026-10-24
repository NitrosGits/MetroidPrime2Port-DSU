# Lumite REL (module 39): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CLumite.cpp` (Lumite, the hopping
sunlight-shy wall crawler of Agon Wastes) and `SLumite_FuncPtrs.cpp`; the loader defaults are
`include/MetroidPrime/ScriptLoader/SLdrLumite.hpp`. The port's fixes are `TARGET_PC` blocks in
`CLumite.cpp` and `SLdrLumite.hpp`. Retail asm:
`build/G2ME01/Lumite/asm/MetroidPrime/Enemies/CLumite.s` (after `python3 configure.py && $T/ninja`).
Per-function table: `Lumite_functions.tsv` (B = module body, H = weak copy or unnamed
dtor/thunk/static init, T = REL runtime).

Unnamed retail bodies: fn_39_6634 is the CLumite ctor (called from LoadLumite), fn_39_6948 is the
SHopPlan ctor (calls Reset), fn_39_6A84 is the SSurfaceTarget ctor, fn_39_6D6C is the SSunlightEffect
ctor (the PART GetObj), and fn_39_6FD0/70EC/71B8 are DeliverCommand<CBCStepCmd/CBCTauntCmd/
CBCProjectileAttackCmd> (callers: Taunt+TurnTowards / Taunt / SmallShot+BigShot). The
SShotAttack ctor is inlined into fn_39_6634.

## Behaviour fixes (lum1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_39_B08 SLdrLumite ctor (inlined in LoadLumite) | 0xB78/0xB88 `stw -1,0x340/0x344`; 0xB98 `stw 3,0x38`; 0xBB4 knockback 2.0 (`stfs 0x88`); 0xBCC `stb 1,0x26c` echo emitter; 0xBD0..0xBE4 ambient CColor(1,1,1,1); 0xBF4 visorFlags 0xF | 0 / 0 / 0 / 1.0 / false / 0 / 0 |
| fn_39_38E8 Taunt | 0x39E4 `lfs rodata_A4` = 0.8; `fcmpo; bge` | 0.75 |
| fn_39_33DC TurnTowards (Update) | 0x3628 `lwz 0x48c; lbz 0x594; extrwi 1,24`: re-ready when the body controller's animation is over | tested mTurnActive (never set) |
| fn_39_262C Hop | 0x2688..0x26D4: abort test, NextState, return; then `li r4,0; bl SetAttackState` | SetAttackState(0) before the abort test |
| fn_39_3E4C FacingHint | 0x3EBC `lfs 0x48/0x38/0x28(r31)`: forward column | right column |
| fn_39_2244 BuildHopCandidates | 0x22DC `lwz r31,0x838(r27)`: kOL_AiWaypoint list; 0x2414 `blt`/0x2420 `bgt` skip, so a NaN distance is kept | kOL_All; `d < min \|\| d > max` |
| fn_39_1CF4 FindSafeZone | 0x1E3C `fcmpo; blt` reject: NaN accepted, `!(MagSquared() < 1)` | `MagSquared() >= 1` |
| fn_39_15F4 SelectHopTarget | 0x18D0..0x1924: Range() x, y, z drawn in order | three Range() calls as arguments (order unspecified on the host) |

Checked equivalent (no fix): `__sinit` trigger/state/code tables, the CLumite ctor, the LoadLumite
property mapping, Think, DoUserAnimEvent, AcceptScriptMsg, PreRender, GetKneeName/Index,
ProjectileInfo, StickToSurface, DamageSafeZoneOccupant, PlaySound, UpdateSunlight (the 0x150c
player read is the existing TARGET_PC GetPlayerState(0)), DistanceToPlayer, ResetAttackTimer,
BuildProjectileTransform (up column), FireSmall/BigShot, IsTouchingSunTrigger, CollidedWith,
SpawnZoneImpact, ChooseHop (retail `ble` vs upstream `beq` give the same result here), TurnToHopPoint,
Dead, CanTaunt, CanAttack, ReadyToMove, QuitTeam, SHopPlan::Reset, GetOrbit/AimPosition, the
DeliverCommand templates, the small getters/triggers (GetHopSpeedScale, In*ShotRange, Landed,
IsHeckler, FacingHopPoint, SetAttackState, Small/BigShot, Turn, StickToNearestSurface,
DisableGroundCollision, SetupStateMachineHelper).

## Coverage (build-port-cov, driver `tools/drivers/lumite_states.py`)

Runs: `42b935e4,0a_sand_hall` (twice) and `0j_sand_hall_dark` (natural play, damage, a kill with
the remaining hp + 0.5), plus a forced run (`FORCE_DEAD=1`: `fsm <id> Dead`). Spec:
`tools/cov/specs/lumite.py`. Merged: 70/77 functions, 706/843 lines (83.7%), 349/550 branches,
caught/reach-stub 0.

    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/lumite.py <out> 42b935e4,0a_sand_hall \
      --driver "python3 tools/drivers/lumite_states.py {port} <out>/states.log" --fast --frames 6000

Within 6000 frames the driver fully cycles only the first Lumite. Driver env goes inside
`--driver` (`env FORCE_DEAD=1 python3 ...`); `--env` sets only the game's env.

Marks (36): bodies with every line run and every live arm taken. Dead arms: the ctor's two
untaken arms are compiler exception-cleanup edges; TurnTowards' switch default and
DeliverCommand's `else if (Deactivate)` false arm need a msg outside Activate/Update/Deactivate,
which `EStateMsg` (StateMachineCommon.hpp) does not have and CGenericFSM2 never passes.

Not marked, with the reason:
- Live arms untaken: Think (inactive return; one 238 combination), Taunt (one surface-search
  combination), ChooseHop, TurnToHopPoint (hint lookup failed / hop failed), CanAttack (heckler
  arm), FacingHint (zero direction), ResetAttackTimer (no random), In*ShotRange (in-range arm),
  ProjectileInfo (attack state neither 1 nor 2), GetScannableObjectInfo (alpha 0).
- Partial: Hop 50/56, SelectHopTarget 33/43, BuildHopCandidates 25/31, UpdateSunlight 42/45,
  IsTouchingSunTrigger, DoUserAnimEvent, CollidedWith, AcceptScriptMsg, PreRender,
  DamageSafeZoneOccupant, SpawnZoneImpact, team calls (no CScriptTeamAiMgr in the tested rooms).
- Never run: GetKneeIndex/Name, ClearLineOfFire, TrySunlightHop, FindSafeZone, InSunlight, Null.
  The safe-zone/sunlight hop paths need a Lumite next to a lit safe zone; the tested rooms have none.
- Dead is reached only forced (`fsm <id> Dead`, 18/23 lines): a real lethal `damage`, even with
  hp + 0.5, deletes the Lumite at once, so Dead never runs naturally in these runs.

## lum1 slice (2026-10-24): coverage state, no markers added

`tools/drivers/lumite_natural.py` (no `fsm` forcing; wakes inactive SafeZone/AIHint, places the player at 4..45 m, small damages;
`MP2_GODMODE=1`, areas 0a_sand_hall, 0j_sand_hall_dark, 02_sand_sandfalls layer 5) reaches the safe-zone, sunlight and hop code
that the state-cycling driver never did. Merged with the older runs, every unmarked function still has at least one untaken arm:

- Closest: DeliverCommand 1389:1/2; Think 229:1/2, 251:3/6; StickToSurface 267:0/2; UpdateSunlight 298:1/2, 343:2/4, 350:0/2;
  IsTouchingSunTrigger 383:1/4, 383:3/4, 390:1/2; CanAttack 679:1/2; CanTaunt 715:0/2; ResetAttackTimer 775:1/2; FacingHint 748:1/2;
  RandomVector 808:0/2; Taunt 878:3/4; TurnTowards 900:3/4; Start/EndTeamAction 644:0/2, 651:0/2; ClearLineOfFire 731:1/2, 734:3/4;
  LoadLumite 1334:1/2, 1339:0/2, 1343:0/2; PreRender 421:0/2; SpawnZoneImpact 408:3/4, 412:0/2.
- Larger: CollidedWith, JoinTeam/QuitTeam/GetTeamRole, ReadyToMove, Hop, SelectHopTarget, TrySunlightHop, FindSafeZone, ChooseHop,
  BuildHopCandidates, Dead (dark/annihilator death arms), DamageSafeZoneOccupant 401.
- Dead by policy: ctor 179:0/2, 179:1/2 (cleanup edges).
- Next: a force spec (`mSurface.mKind=2`, `mZoneId`, team/death-weapon members) for the lists above, then asm check per function.
