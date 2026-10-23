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

## Spawns

Temple Grounds dark (world 3bfa3eff) 06_temple_sacredpath_dark (area 55): two Dark Commandos, u051a
(e0037014e, near (-6.4,282.4,-38.7)) and u041c (e00370056, (5.6,244.6,-30.4), inactive until
`send u041c Activate`); 650 hp each. Others: docs/modules.tsv row DarkCommando.

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
