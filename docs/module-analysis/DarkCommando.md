# DarkCommando REL (module 3): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CDarkCommando.cpp` (Dark Pirate
Commando, its `CShadowDecoy`, `CDarkCommandoGrenade` EMP grenade and `CDarkCommandoChargeBeam`);
the loader defaults are `include/MetroidPrime/ScriptLoader/SLdrDarkCommando.hpp`. The port's fixes
are `TARGET_PC` blocks in those two files. Retail asm:
`build/G2ME01/DarkCommando/asm/MetroidPrime/Enemies/CDarkCommando.s` (after `python3 configure.py && $T/ninja`).
Per-function table: `DarkCommando_functions.tsv` (B = module body, H = weak copy/template or unnamed
static, T = REL runtime).

## Behaviour fixes (dc1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_3_19C LoadDarkCommando (inlined SLdrDarkCommando ctor, struct at r1+0x400) | 0x218 `stw 0x438` editor 0x38 = 3; 0x234 `stb 0x66c` isEchoEmitter 1; 0x244 `stw 0x6e4` visorFlags 0xF; 0x254 `stw 0x694` ambient CColor(1,1,1,1) | 0 / false / 0 / 0 |
| fn_3_14B8 SLdrDarkCommandoEMPData ctor | 0x14D4 `li r0,-1`; 0x150C/0x151C `stw r0,0x40/0x44(r31)`: sound_GrenadeBounce/Explode -1 | 0 |
| fn_3_8BEC CDarkCommandoGrenade::Think | 0x8C24 `extrwi. r0,r0,1,24; beq 0x8D5C`: the EMP interference and the delete run only once exploded | only `mEMPTime += dt` gated |
| fn_3_3104 BuildActionChoices | 0x33F4 `clrlwi. r0,r3,24; beq 0x34A4`: returns when the RayCollideWorld line of sight is blocked (false) | returned when clear (inverted) |

Checked equivalent (no fix): SelectMeleeVariant, PreRender, Dead, CShadowDecoy::Think (redundant
null check), CDarkCommandoChargeBeam::Think (`5 - t/60` form), ThinkRagDoll (radii copy loop),
every state body (switch reordering only: Lurk, Taunt, FaceTarget, MeleeAttack, FireEMP,
PostFireChargeBeam, ShadowDash, WarpIn/WarpOut), KnockBack, SLdrDarkCommandoData (fn_3_B14),
ChargeBeamData (fn_3_10E8) and ShadowDashData ctor defaults; BuildActionChoices' `Magnitude() > 15`
AsNormalized (result discarded, dead code); SolveGrenadeLaunch (retail keeps the best angle/speed in
f28/f27, seeded with 0.349 and grenadeMinLaunchSpeed, and stores after the loop; the one caller
passes those same seeds); LaunchGrenade (retail computes the launch locator before the target
lookup, no side effects); FireChargeBeamShot (retail inlines TCachedToken::IsLoaded on
ProjectileInfo()->Token()); SelectDashTarget (target/self positions hoisted; same compares,
`>=`/`<=` via `cror`, NaN falls to the else arm in both); ApplyMeleeDamage, UpdateAdditiveAim, the
CDarkCommando ctor (inline CSfxHandle store). `upstream_hollow.py DarkCommando`: none.
Noted, not changed: CreateShadowDecoyEffect locks the token in retail (TLockedToken/GetObj), upstream
keeps a plain TToken.

## Behaviour fixes (dc2, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_3_1650 JoinTeam | 0x1674-0x1680 `lhz r5,0xc90; cmplw r5,kInvalidUniqueId; bne 0x16E4`: a set team id skips the whole join | re-joined on every SelectDashTarget; `CScriptTeamAiMgr::JoinTeam` then reset the member's role |

Checked equivalent (dc2): ShouldMeleeAttack, AcceptScriptMsg (OffGround `lbz 0x594; extrwi 1,26`
= frozen, skip), ApplyMeleeDamage (wrist 0xc79 / blade 0xc78 min/max, 3*scale.y, `<= 0` timer),
LeaveTeam, ReserveHint, CShadowDecoy::Touch, ShouldGetUp (states 0xe Hurled / 2 LieOnGround),
UpdateAdditiveAim (null target: no AdditiveIdle either), WarpIn, Lurk, Taunt, FaceTarget (`<=`
0.1745 via cror, NaN skips), InEMPRange/InChargeBeamRange (same shape, 0x814/0x818 vs 0x85c/0x860).

## Spawns

Temple Grounds dark (world 3bfa3eff) 06_temple_sacredpath_dark (area 55): two Dark Commandos, u051a
(e0037014e, near (-6.4,282.4,-38.7)) and u041c (e00370056, (5.6,244.6,-30.4), inactive until
`send u041c Activate`); 650 hp each. Team: 3bfa3eff 11_temple_grandpuzzle layer 8 has three commandos
(u065c/u065a/u0659) and their CScriptTeamAiMgr u065b (`layer 8` activates them). Others:
docs/modules.tsv row DarkCommando.

## Coverage (dc1)

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON` (`-DMP_SDK_HEADERS_ONLY=OFF`); runs (not `--fast`:
the driver is slower than the game and runs out of frames):

    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/darkcommando.py <out> 3bfa3eff,06_temple_sacredpath_dark \
      --driver 'python3 tools/drivers/darkcommando_states.py {port} <out>/states.log' --frames 24000
    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/darkcommando.py <out> 3bfa3eff,06_temple_sacredpath_dark \
      --con tools/cov/specs/darkcommando_fight.con --frames 4200 --env MP2_GODMODE=1
    python3 tools/cov/gcov_report.py build-port-cov <out1>/gcov <out2>/gcov --module DarkCommando

`darkcommando_states.py` forces every FSM state by name (`SelectAttackTarget`, `SelectAction`,
`SelectDashTarget`, `Set*FaceVect`, `ActivateCloak`/`DeactivateCloak` are code funcs inside
states, not states: `fsm` cannot reach them), damages, replays the attacks and kills both;
`darkcommando_fight.con` is the natural fight (charge beam, EMP grenade, shadow dash, cloak). Merged:
88 bodies, 64 with branches, 7 with every arm taken.

Marked by dc1 (24; every arm taken or branch-free with every line run, objdiff 100% or
label/argument-only differences): ReleaseHint, GetHint, SetCloakTarget, SelectAttackTarget, Start,
ShouldWarpOut, ShouldFireChargeBeam, ShouldFireEMP, ShouldTaunt, StateOver, SetupStateMachine,
ProjectileInfo, GetAimPosition, AddToRenderer, PreRenderAllViewports, PreThink, ~CDarkCommando,
~CDarkCommandoChargeBeam, CDarkCommandoChargeBeam::AcceptScriptMsg, ~CDarkCommandoGrenade (upstream
adds a null check), CDarkCommandoGrenade ctor, ~CShadowDecoy, CShadowDecoy::GetOrbitPosition/GetAimPosition.

Unmarked (arms not run): the fixed BuildActionChoices (11/16 arms) and CDarkCommandoGrenade::Think
(10/12; the `eMPDuration > 0` false arm and the not-yet-expired arm need a forced duration);
ApplyMeleeDamage and SelectMeleeVariant (melee never landed: 0/10, 1/12), ShouldMeleeAttack,
KnockBack (12/24), every state body (exit or timer arms missing), Think, AcceptScriptMsg,
DoUserAnimEvent, FireChargeBeamShot, LaunchGrenade, SolveGrenadeLaunch, GetGrenadeTargetPosition,
InEMPRange/InChargeBeamRange, the CShadowDecoy bodies, the charge-beam ctor/Think/collision,
ThinkRagDoll, JoinTeam/LeaveTeam, UpdateSfx, LoadDarkCommando (one arm), the CDarkCommando ctor.
Next: a melee driver (player within melee range, `fsm MeleeAttack` until the blade hits), force the
grenade's EMP duration and the team (`CTeamAiMgr`) arms.

## Coverage (dc2)

Drivers/specs: `tools/drivers/darkcommando_melee.py <port> [log] [layer|-] [leave]` (activates every
commando, places one at several distances, forces `fsm MeleeAttack`, then FireEMP at 12-30 units;
`leave` ends with 3x `send Deactivate`); `tools/cov/specs/darkcommando_force.py` (gdb forces,
self-restoring); `darkcommando_emp_force.py` (grenade EMP duration only). Runs merged (force first):

    MP2_GODMODE=1 BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darkcommando_force.py <f> 3bfa3eff,11_temple_grandpuzzle \
      --driver 'python3 tools/drivers/darkcommando_melee.py {port} <f>/melee.log 8 leave' --frames 9000
    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/darkcommando.py <t> 3bfa3eff,11_temple_grandpuzzle \
      --driver 'python3 tools/drivers/darkcommando_melee.py {port} <t>/melee.log 8' --frames 9000 --env MP2_GODMODE=1
    (+ the dc1 states run, and the melee driver in 06_temple_sacredpath_dark, 9000 frames, godmode)

Merged: 88 bodies, 64 with branches, 11 with every arm taken. Marked by dc2 (14):

| fn | arms | note |
|---|---|---|
| fn_3_8BEC CDarkCommandoGrenade::Think | 12/12 | duration arms forced (darkcommando_force: eMPDuration) |
| fn_3_9184 CShadowDecoy::Touch | 2/2 | natural |
| fn_3_15A8 LeaveTeam | 6/6 | not-a-member / not-a-team forced (darkcommando_force: LeaveTeam calls 2/3) |
| fn_3_2D24 UpdateAdditiveAim, fn_3_6030 ShouldGetUp | all | natural |
| fn_3_1650 JoinTeam | 5/6 | cast-null dead: the id comes from GetAssociatedTeamId in the same call (after the fix) |
| fn_3_1F34 ReserveHint | 1/2 | cast-null dead: the only caller (SelectDashTarget) passes hint ids it collected from the AiWaypoint list in the same call |
| fn_3_5DA8 WarpIn, fn_3_5A2C Lurk, fn_3_58D8 Taunt, fn_3_484C FaceTarget | all but 1 | the `else if (msg == kStateMsg_Deactivate)` false arm is dead (EStateMsg is 0..2); Lurk/Taunt knocked-back and FaceTarget zero vector forced |
| fn_3_3690 ApplyMeleeDamage | 9/10 | `!mMeleeDamageApplied` false arm dead: 0xcb4 bit 0x01 is only written 0 (0x5664 MeleeAttack Activate, 0x7388 ctor); not-attacking / zero blade forced |
| fn_3_3054 InEMPRange, fn_3_2FA4 InChargeBeamRange | 9/10 | target-null dead: the only caller, BuildActionChoices, returns on a null target first; min range forced |

Still unmarked: ShouldMeleeAttack, SelectMeleeVariant, MeleeAttack, FireChargeBeam (target-null
arms reachable from the FSM but not run), BuildActionChoices (1238-1245 arms this run),
DeactivateCloak (1/4), UpdateCloak (2/4 dark-visor arm), FireEMP, ShadowDash, PostFireChargeBeam,
WarpOut and GetUp (0%: never entered), KnockBack, ThinkRagDoll, the charge beam and decoy bodies.
