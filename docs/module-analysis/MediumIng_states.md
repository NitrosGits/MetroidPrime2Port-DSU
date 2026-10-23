# MediumIng (module 41) — FSM states and helpers (lane mi2)

TARGET_PC bodies in `src/MetroidPrime/Enemies/CMediumIng.cpp`, ported from
`build/G2ME01/MediumIng/asm/MetroidPrime/Enemies/CMediumIng.s`. Status "asm" = body written
and reviewed against the asm, not yet run in play (no `// @` marker). Upstream triggers all
match the asm; none changed.

| Function | fn_41_ | Status | Notes |
|---|---|---|---|
| SubStart | 7394 | asm | |
| PathFind | 72C4 | asm | |
| FollowAttackPattern | 71BC | asm | |
| FollowApproachPattern | 70B4 | asm | |
| MistIn | 5878 | asm | Deactivate `stw 2,0x944` = surface alignment kM_WorldUp |
| MistOut | 4B8C | asm | |
| ArmAttack | 3F3C | asm | EndAttack unconditional on Deactivate; tentacle retract bit 0x31C\|=0x20 |
| MeleeAttack | 3AE4 | asm | EndAttack unconditional on Deactivate |
| MistAttack | 37B0 | asm | |
| Jump | 5728 | asm | jump point = jp+0x180 (`GetJumpPoint`) |
| Dash | 4FE8 | asm | |
| Taunt | 4E1C | asm | rodata_0 taunt table {1, 4} |
| WaitForLocomotion | 1E78 | asm | |
| Idle | 5724 | asm | empty in retail |
| SafezoneReaction | 542C | asm | |
| MoveToSafeZone | 5240 | asm | |
| BackUp | 2E70 | asm | |
| Dead | 30B0 | asm | empty in retail |
| SelectTargetState | 30AC | asm | empty in retail |
| SelectTarget | 698C | asm | 6D08/6BFC get the mutated target (sp+0x28), not the player pos |
| SetLeashTarget | 6880 | asm | |
| SetRetreatDestination | 2564 | asm | |
| SetMeshPathDestination | 2A74 | asm | |
| SetDashDestination | 261C | asm | |
| FindSafeZoneRetreatPoint | 13E4 | asm | |
| FindClosestRetreatActor | 1BA8 | asm | shares FindClosestConnectedActor |
| FindClosestDangerActor | 1CE8 | asm | shares FindClosestConnectedActor |

Helpers (TARGET_PC, guessed names): UpdateCollisionFilter 49CC (data_0 0x3B Solid,
data_5A0 0x20 Player, data_5A4 0x3C NoPlatformCollision), ShouldUseWideSearch 5DC8,
IsSweepBlocked 5E0C, ExtendTentacle 4308, RetractTentacles 43E8, PositionTentacles 4470,
SetTentaclesActive 4598; file-static PathCostSquared 6F18 (sum of squared segment lengths),
JumpRouteEstimate 6D08, SearchJumpRoute 6BFC. 49CC/5DC8/5E0C are also on Think's call graph (mi1).
