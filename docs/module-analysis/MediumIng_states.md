# MediumIng (module 41) — FSM states and helpers (lanes mi2, mi3)

TARGET_PC bodies in `src/MetroidPrime/Enemies/CMediumIng.cpp`, ported from
`build/G2ME01/MediumIng/asm/MetroidPrime/Enemies/CMediumIng.s` (mi2) and played in area 46 (mi3).
Status: `marked` = every live arm ran (`// @` marker); `played` = ran, the arms listed are open;
`not run`. The only untaken arm of each `switch (msg)` is the default (EStateMsg > 2, never sent
by CGenericFSM2): dead. Evidence: gcov (build-port-cov) over a natural run (`mediuming_play.con`)
and two forced drivers (`mediuming_forced.con`, `mediuming_forced2.con`), see MediumIng.md "Run".
Upstream triggers all match the asm; none changed.

| Function | fn_41_ | Status | Open arms / notes |
|---|---|---|---|
| SubStart | 7394 | marked | |
| PathFind | 72C4 | marked | Update calls CalculateSeparation (0x7374 fn_41_2024) |
| FollowAttackPattern | 71BC | played, forced (`fsm u0417 FollowAttackPattern`) | waypoint destination arm (no attack-pattern waypoint in area 46) |
| FollowApproachPattern | 70B4 | played, forced (`fsm u0417 FollowApproachPattern`) | same |
| MistIn | 5878 | played | spawn mode 0 / 1 arms (0x5960 `lwz 0x9E8`; area 46's Ings use another mode). Deactivate `stw 2,0x944` = kM_WorldUp |
| MistOut | 4B8C | played | one arm each of `if (mSfx1)` 0x4D24 / `if (mSfx2)` 0x4D48 |
| ArmAttack | 3F3C | played | team-AI arm (0x3F8x), null tentacle 0x41C0, mD6 0x4230. EndAttack unconditional on Deactivate; retract bit 0x31C\|=0x20 |
| MeleeAttack | 3AE4 | played | team-AI arm, HasMist on Deactivate |
| MistAttack | 37B0 | marked | forced (`place u0417 player 5; fsm u0417 MistAttack`) |
| Jump | 5728 | played | jump-point arm 0x5774 (no AiJumpPoint in area 46), 0x5838 |
| Dash | 4FE8 | played | CanBeNormalized false 0x50C4 / 0x5110, 0x51A0 |
| Taunt | 4E1C | marked | rodata_0 taunt table {1, 4} |
| WaitForLocomotion | 1E78 | marked | |
| Idle | 5724 | marked | empty in retail; forced (`fsm u0417 Idle`) |
| SafezoneReaction | 542C | played, forced (`send u0454 XENZ from u0451`) | mD1 0x54EC |
| MoveToSafeZone | 5240 | played, forced (`fsm u0454 MoveToSafeZone`) | FindSafeZoneRetreatPoint success 0x52A0, 0x5380 |
| BackUp | 2E70 | marked | |
| Dead | 30B0 | played | kAS_LieOnGround arm 0x31D0 (`lwz 0x37c; cmpwi 2`; was kAS_Death = 4 until review33) and the mist-effect drain 0x31E8..0x3204 stayed untaken (gcov, before the fix), though the shot-killed u0417 was deleted (~f1600, forced run) by a path not traced |
| SelectTargetState | 30AC | marked | empty in retail |
| SelectTarget | 698C | played | the jump-point loop (no AiJumpPoint). 6D08/6BFC get the mutated target (sp+0x28) |
| SetLeashTarget | 6880 | played (`tp` to area 40) | danger-actor arm 0x6908 |
| SetRetreatDestination | 2564 | played | retreat actor found 0x25D8 |
| SetMeshPathDestination | 2A74 | played | reachable-point fail arm |
| SetDashDestination | 261C | played | point in safe zone 0x298C, reachable fail |
| FindSafeZoneRetreatPoint | 13E4 | played | ran with no safe zone only (0x1424 ret 0) |
| FindClosestRetreatActor | 1BA8 | played | FindClosestConnectedActor: no connection of the state matched |
| FindClosestDangerActor | 1CE8 | played | same |

Helpers (TARGET_PC, guessed names): ExtendTentacle 4308 (index 0..7 against 4 tentacles: 0x4330
returns for the out-of-range ones, as retail; null tentacle 0x4360 open), RetractTentacles 43E8,
PositionTentacles 4470, SetTentaclesActive 4598 (null-tentacle guards open); file-static
PathCostSquared 6F18 (all arms), JumpRouteEstimate 6D08 and SearchJumpRoute 6BFC (not run: no
AiJumpPoint). mi2's UpdateCollisionFilter/ShouldUseWideSearch/IsSweepBlocked were duplicates of
mi1's SetMistCollision 49CC / IsMistDecayed 5DC8 / IsBlocked 5E0C and are gone (mi3).
