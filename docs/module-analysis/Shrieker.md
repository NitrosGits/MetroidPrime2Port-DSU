# Shrieker REL (module 69): analysis notes

Current: **67/67 behaviour bodies** (gui1 completion below; historical counts
describe earlier surveys).

## gui1 completion (base c4b67e741, 2026-10-24)

Five new markers, no executable changes. Direct fixture
`tools/cov/specs/gui1_shrieker.py`, driver `tools/drivers/gui1_shrieker.con`,
coverage-only inline retention `gui1_keep_inline.cmake`. Greatbridge,900 frames.

| body | evidence / forced arms |
|---|---|
| 1C2C DeflectMissiles | **44/44 lines,32/32 edges**. Real CGameProjectile constructed with its native constructor from the Shrieker's loaded weapon-description token, real default CDamageInfo with damage10, real CImpactVisorEffect::None, allocated uid and AddObject registration. Near-list hook supplies empty / invalid id / real player / constructed projectile. Power without combo, Power+0x80 combo, other beam, missile outside/inside radius; collinear and noncollinear previous positions. Count/min-homing-distance and unchanged/changed orientation oracles PASS. Existing-id sound suppression runs; IsPlaying explicitly stubbed true for timer0.1/0.75 sound suppression/stop branches, not claimed natural audio playback. |
| 5554 ReactToDamage | **31/31 lines,32/34 reported edges**. Constructed live weapon types0..15, combo off/on, hostile priority0.25 and1, buried/not, positive/zero/negative health delta, nonweapon/missing sender. Exact expected HP/trigger oracles PASS. Patrol/ReturnToStart/Buried name-query returns forced at the actual virtual target; valid/missing weapons and owner-target oracles PASS. Two residual edges are solely block34's EH cleanup decision after a throwing temporary-string operation: all normal branch destinations hit, block34 has no non-exception ingress. Raw gcov JSON retained; not claimed ALL-ARMS. |
| 1170 SLdr ctor | Standalone native malloc/ctor/oracle/dtor/free; all local scalar/damage/vector defaults checked, plus corrected editor3, knockback resistance2, echo-emitter true, white ambient, visor15, sound-1. Full executable header coverage. Retail store audit retains shr1 corrections below; no matching percentage claimed for this report's unpaired fn symbol. |
| 10D0 SLdr dtor | Standalone real lifetime and natural loaders. Native member destruction; retail member/base/delete chain audited. No synthetic vtable or destructor implementation. |
| 7124 static initialization | Actual startup plus native oracles for **every state/trigger name and resolved PMF target**, tolerance(1,1,0.1), both locator strings. Retail7124..7168 initializes tolerance;716C..71D8 constructs 64-bit include Solid(59)/exclude Player(32),CollisionActor(47),type3, matching skUnusedFilter (optimized out on host; static proof, not runtime claim). Remaining initializer stores assemble the state/trigger PMF tables and construct/register both strings. Native compiler may constant-initialize/reorder these; no callable retail initializer body is fabricated. |

Final post-marker run **62/62 calls**, normal inferior exit, zero failed calls,
writes/restores, Python errors, caught/reach-stub or console errors. Constructed
projectile is removed through DeleteObjectRequest; temporary metadata and strings
freed. No console firing needed. An early diagnostic accidentally resolved the
unused default CImpactVisorEffect reach stub; final uses native None and has0
reach-stubs. State-name hook binds the object's actual vtable target, not the
first identically named local template copy.

Static audit: both named behaviour bodies match the source expressions in the
retail annotations. Deflection weapon/attrib tests and cross-product guard are
unchanged. React switch default scale1 and the two string tests unchanged.
retail_diff has the known marker/body-binding bug (labels both bodies Render);
2154 is the reserved-vector/find H copy and57C0 string== H copy, not new missing
behaviour.7124 is compiler-generated, hence no parsed port function. Full asm
annotations retained rather than treating these parser outputs as correctness.

Gates: host and coverage links0 FAILED; manifest rc0; matching checksum **87
files OK**; executable source unchanged after stripping markers, including
non-PC paths; whitespace clean. Review base c4b67e741 adds5 MARK to GUI's6.
No unchanged-family sweep. Evidence `/tmp/opencode/reports/gui1-evidence/`.

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

## gren9 lane (base ed440706f, 2026-10-24)

Direct-call forcing (force_ret CALLS, `tools/cov/specs/shrieker_force9.py`, driver `shrieker_force9.con`, greatbridge, 700 frames, run with
`COV_GDB_SCRIPT=tools/cov/force_ret.py`; hook `$g9_h 1` makes GetHeightAboveMesh report 2.0) and loader-local writes (`shrieker_force9b.py`, force.py `loc:`), merged with
`tools/drivers/shrieker_greatbridge.con` + `shrieker_forced.con` natural/forced runs (cov build; rebuild first, older gcda do not match).

| function | arms | forced (cmd) |
|---|---|---|
| IsFacingTarget 3024 | 2/2 | forced (mTargetId := invalid) |
| InMaxRange 2A50 | 6/6 | forced (no target; mChasing 0/1 natural) |
| FaceTarget 1AA4 | 14/14 | forced (no target / target = self: zero direction / player with GetHeightAboveMesh stubbed 0, 1, or 2.0 and mChasing 0/1) |
| ShouldMelee 2D64 | 14/14 | forced (GetTeamAiRole stub null / Initial / other, StartAttack stub 0/1, team id invalid / valid, mMeleeRangeSquared 0 / 1e9, no target, timer) |
| CloseToStart 317C | 10/10 | forced (mStartPosition moved 1000 in x / y / z / not) |
| Think 51E0 | 20/20 | forced (mBuried 1, mShriekTriggered 0, scripted proximity / alert 0/0, 1/0, 1/1) |
| KnockBack 4DA8 | 14/14 | forced (mAlive 0, mDeathKnockBackStarted 1; CPatterned::KnockBack stubbed) |
| LoadShrieker 154 | 15/17 | forced (model data invalid; the 2 untaken arms are the unwind edge of the `LdrToDamageVulnerability` temporary on the `CShriekerData` call) |
| CShrieker ctor 62C0 | 36/37 | forced (pART / meleeEffect invalid at the loader; the 1 untaken arm is the `mEffectDescs(2, ...)` vector allocation edge) |

Also marked: GetSearchPath 0 / IsListening 8 (header, 100%, `return`s), the implicit CShriekerData copy ctor 6D08 (header, 100%). The asmcheck tool does not find the
fn_69 markers of these bodies (`MISSING ... no C++ definition cites it`): its marker-to-body pairing fails on this file, the markers sit directly above each definition.
Left unmarked, with their untaken arms: FindTarget 4970 (the `distance < bestDistance` compare inside the one-player loop, 3 arms: dead with a single player), ShouldDodge 2B20 (1 arm: a
non-empty near list), ApplyMeleeDamage 24F4 (a non-actor in the near list), DeflectMissiles 1C2C (6), DoUserAnimEvent 333C (2), PathFind 465C (3), Render 5024 (3),
ReactToDamage 5554 (14), AcceptScriptMsg 5988 (~15), ApplySeparation 146C (4).

## shr3 lane (base 9558d7424, 2026-10-24)

New TARGET_PC fixtures (`CShrieker::PortConsoleVar`, `var <id> shr.<name> [v]`): shr.conn <1-7> (script connections for AreaLoaded; 0 clears), shr.deflect <0/1/2> (deflected ids: clear / the first
live game projectile / an id with no object), shr.radius <v/10> (missile deflection radius), shr.sep <1/2/3> (ApplySeparation: onto another Shrieker / in place / with the area id changed).
Drivers/specs: `tools/cov/specs/shrieker_shr3.con` (greatbridge, 1700 frames), `shrieker_shr3.py` (force_ret CALLS + spec-local gdb hooks: BuildNearList results by caller, LaunchProjectile forced null,
a Launching message on a dying launched body; run it with `--con tools/drivers/shrieker_greatbridge.con --frames 2600` so the shriek effect exists), `shrieker_shr3b.py` (pART invalid at the loader
for every Shrieker), plus `shrieker_force9.py` / `shrieker_force9b.py` from gren9; merge with the natural gb / forced drivers (rebuild first: older gcda do not match).

| function | arms | forced (cmd) |
|---|---|---|
| AcceptScriptMsg 5988 | 41/41 | forced (shr.conn 1-7 + AreaLoaded; Create on an active body; Launching alive, and a CALLS direct call with mAlive 0, mDying 1, mLaunched 1) |
| Render 5024 | 12/12 | forced (shr.deflect 1/2, shr.radius 1 / -10; echo visor on a faded body) |
| DoUserAnimEvent 333C | 13/13 | forced (LaunchProjectile result null at its return; pART invalid at the loader: no shriek effect) |
| ApplyMeleeDamage 24F4 | 20/20 | forced (BuildNearList from ApplyMeleeDamage returns one id with no object) |
| ShouldDodge 2B20 | 16/16 | forced (CALLS: PathExists stub, RayCollideWorld stub 0 / 1; BuildNearList from ShouldDodge non-empty) |
| PathFind 465C | 13/14 | forced (CALLS: no target; PathShagged stub 1). The untaken arm is the EStateMsg switch default (three values, as UprootShriek et al.) |
| ApplySeparation 146C | 14/15 | forced (shr.sep 1 / 2 / 3). The untaken arm is `other == nullptr` (146C): dead, the list qualifier `CListeningAiList::IsQualified` (GameObjectLists.cpp:29) requires `TCastToConstPtr<CPatterned>` non-null |
| FindTarget 4970 | 11/14 | forced (no target / id with no object / separating). The 3 untaken arms are the one-player loop's `distance < bestDistance` skip edge (retail 0x4A38 `fcmpo f31, f27(FLT_MAX); bge`) and the second-player iteration (0x4A78 `cmplw r27, mgr+0x14f8; blt`): the port has one player (mNumPlayers 1; two only in retail's split-screen game mode), distances are finite, so `distance >= FLT_MAX` never holds |

Not marked: DeflectMissiles 1C2C (a Power-type shot with the combo attribute: no combo weapon in the port's console / shipped drivers; a missile exactly collinear with the centre for the
`CanBeNormalized` arm) and ReactToDamage 5554 (weapon types other than power / dark / light / annihilator / bomb / missile for the `scale != 1` arm, the Patrol / ReturnToStart state-name arms with
a live weapon).
