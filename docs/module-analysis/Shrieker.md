# Shrieker REL (module 69): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CShrieker.cpp` (CShrieker) plus the
`SLdrShrieker.hpp` loader. The port's fixes are `TARGET_PC` blocks in those two files. Retail asm:
`build/G2ME01/Shrieker/asm/MetroidPrime/Enemies/CShrieker.s`. Per-function table:
`Shrieker_functions.tsv`.

Bitfields (verified against the asm): byte 0xa08 = 0x80 mScriptedProximity, 0x40 mScriptedAlert,
0x20 mHasPatrolPath, 0x10 mBuried, 0x08 mChasing, 0x04 mPlayerInProximity, 0x02 mShriekTriggered,
0x01 mDodgeRight; byte 0xa09 = 0x80 mDying, 0x40 mLaunched, 0x20 mSeparating,
0x10 mDeathKnockBackStarted.

## Behaviour fixes (shr1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_69_1170 SLdrShrieker ctor | 0x11FC `li r9,3` / 0x124C `stw r9,0x38`; 0x1260 `stfs` rodata_C (2.0) at +0x88 (knockback resistance); 0x126C `stb 1,0x26c` (echo emitter); 0x1270 CColor(1 x4) -> 0x1284 +0x294 ambient; 0x1288 `li r0,0xf` / 0x1294 +0x2e4 visor flags; 0x11C8 `li r11,-1` / 0x123C `stw r11,0x4c0` (sound_MissileDeflection, never set to 0) | none of these; sound 0 |
| fn_69_2A50 InMaxRange | 0x2AAC `extrwi. r0,r0,1,28` (0x08 mChasing) / 0x2AD4 `beq`: the range shrinks by 0.25 x max while chasing | tested mBuried (0x10) |
| fn_69_1AA4 FaceTarget | 0x1B40 `extrwi. r0,r0,1,28` / 0x1B44 `bne 0x1C10`: the hover-height correction runs when !mChasing | !mBuried |
| fn_69_5024 Render | 0x5164 `fcmpo 0,ratio; ble` / 0x517C `fcmpo 1,ratio; bge`: `CMath::Clamp(0, ratio, 1)` | `min_val(ratio, 1)` (differs only for a negative ratio; NaN passes through in both) |

## Survey (shr1)

Objdiff of every function below 100% plus the retail asm. Apart from the fixes above, the
differences are regalloc, scheduling, stack layout, inlined `rstl::find` loops and REL-local
template copies (the unnamed fn_69_* rows). Checked and equivalent:
- CShriekerData ctor, FindTarget, PathFind, CloseToStart, IsFacingTarget (forward 0x28/0x38/0x48),
  GetHeightAboveMesh, SetBuriedCollision, UpdateValidTarget (player 0's state, already TARGET_PC).
- AcceptScriptMsg: an extra empty 'XAUD' case; Launching velocity (15 sin, 15 cos, 0) matches.
- ReactToDamage: weapon types 0-3 -> combo ? hostile : 0; 4, 6 -> hostile; others 1.
  fn_69_57C0 is a REL copy of string ==.
- Think, KnockBack (regalloc only); ShouldMelee (role x10 != 0, StartAttack type 0 = melee).
- ShouldDodge: `<= 0` timer, `Range(0,100) < pct`, offset = mCheckRadius(0xa24) x right, PathExists
  == 0, RayCollideWorld filter Include Solid (data_2E8, type 1), near list must be empty; else arm
  toggles mDodgeRight; two tries.
- ApplyMeleeDamage: include Trigger/Immovable/Solid/NonSolidDamageable, exclude Projectile/PowerBomb
  (type 3); radius `max(0, v x scale)` (0x25AC `fcmpo 0,r; bge` keeps 0 on NaN, as max_val does);
  ApplyDamage filter Include Solid (type 3, empty exclude).
- DeflectMissiles: type 6 or (0 with attrib 0x80); `magSq < r^2`; the sound loop plays for the
  first new id only, stops the old one when its timer > 0.5, at mPosition (0x54).
- DoUserAnimEvent (cases 0x1d/0x21/0, radii 0x7e0/0x824), ApplySeparation (10 x scale.x,
  CBCLocomotionCmd(total, 0, 0.5)), LoadShrieker (same argument order; the two volumes and three
  damage infos map to the same SLdr fields), __sinit (same statics, other order).

## Spawns

`3dfd2249,02_swamp_greatbridge` (area 9): three Shriekers load at boot, no layer switch:
e000900c9, e0009007b, e00090071 (buried at (25.7,-114.9,28.1), (29.3,-125.1,27.8),
(21.2,-133.0,28.1)). They belong to team e0009007a. `04_swamp_temple[L3]`: not used.

No smoke family loads Shrieker (the swamp families boot `04_swamp_temple_dark`). Proposed family
`shrieker`: world `3dfd2249,02_swamp_greatbridge`, 2400 updates, a cmd script of
`@f600 god 1`, `@f800 place e000900c9 player 14`, `@f1200 place e000900c9 player 8`,
`@f1800 fsm e000900c9 Melee`, `@f2000 shoot e000900c9 missile`, `@f2200 damage e000900c9 999`.

## Coverage

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON -DMP_SDK_HEADERS_ONLY=OFF`. Run
`BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/shrieker.py <out> 3dfd2249,02_swamp_greatbridge
--con <driver> --fast --frames <n>`, then merge with
`python3 tools/cov/gcov_report.py build-port-cov <gcov dirs...> --module Shrieker`.

Drivers: `tools/drivers/shrieker_greatbridge.con` (6400: natural approach Buried -> ShriekAttack ->
TargetPlayer, the state table by `fsm`, two missiles into the deflection field, messages, three
deaths: unburied power, buried power, dark combo) and `shrieker_forced.con` (3200: statecall
BuriedRumbling 0/1/2 and Retreat, ReturnToStart at the start point, dark combo on a buried one).

Marked (30): ALL-ARMS or branchless and fully line-covered, or whose only untaken arm is the dead
default arm of the EStateMsg switch (UprootShriek, ShriekAttack, Bury, Melee, ReturnToStart,
DeliverCommand<CBCStepCmd>; proof as CommandoPirate.md 6.6: EStateMsg has three values and the
FSM2 only sends those). All are 100% in objdiff except the checked-equivalent CShriekerData ctor,
SetBuriedCollision, GetHeightAboveMesh and UpdateValidTarget.

| body | how it ran |
|---|---|
| Buried, TargetPlayer, UprootShriek, ShriekAttack, Dead, PlayerEntersProximity, EnterUprootShriek, LineOfSight, HasPatrolPath, FadeAlpha, SetBuriedCollision, GetHeightAboveMesh, UpdateValidTarget, AddToRenderer, PreRender, GetDamageVulnerability x2, ProjectileInfo, EndTeamAttack, SetupStateMachine, CShriekerData ctor, ~CShrieker, DeliverCommand | natural (approach, fight, deaths) |
| Melee, Dodge, Patrol, Bury, ReturnToStart | forced (fsm) |
| BuriedRumbling, Retreat | forced (statecall) |

Not marked (partial arms): Think (scripted proximity/alert combos), AcceptScriptMsg (AreaLoaded
connection arms), Render (echo visor on a faded actor: `visor echo` needs the visor; null
projectile), DoUserAnimEvent (null projectile), KnockBack (second hit after the death knockback),
ResetAttackTimer, ApplySeparation, QuitTeam/JoinTeam (null team), FaceTarget, DeflectMissiles,
DetectPlayer (morph-ball arm), ApplyMeleeDamage (non-actor in the near list), IsFacingTarget,
UpdateTargetPosition, FindTarget, InMaxRange, ShouldDodge (ray hits nothing), ShouldFire,
ShouldMelee, PathShagged, PlayerLeashReached, LeavePatrol, CloseToStart, PathFind, ReactToDamage
(weapon-type arms), the CShrieker ctor, LoadShrieker. Uncovered: SetTeamMemberTarget,
GetTranslationCopy, PlayerLeavesProximity.
