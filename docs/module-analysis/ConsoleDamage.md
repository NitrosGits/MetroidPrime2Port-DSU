# Console damage no-ops (dmgnoop, 2026-10-24)

Base: `034846b36b47e4dfd7fe79810fd0ef1183a9fa57`. All six reported **HP** no-ops are
retail-correct. Shredder additionally exposed a console sender bug: its resisted-hit
reaction uses a fuse accumulator instead of HP. No game-code vulnerability/health fix is needed.

## Per-actor evidence

REL offsets below are the first address column in `build/G2ME01/<Module>/asm/`;
DOL addresses are in `build/G2ME01/asm/`. Script properties were read from the disc
with `tools/echoes_disc.py` (big-endian), independently of the host objects.

| Family / actor | Class and cause | Retail evidence | Fix |
|---|---|---|---|
| agon54 / `e00360030` CSpacePirate | **(a)** WarpIn pass-through: `xc04_warpPoint != -1`. The smoke hits 50 updates after Activate, during the warp. | SpacePirate `fn_72_3AB4`: `3AC0 lwz 0xc04`, `3AC4 cmpwi -1`, `3AC8 beq 3AD4`; otherwise `3ACC bl PassThroughVulnerabilty`. Directional override `3A88` dispatches the same getter. | None; later vulnerable shots can hurt it. |
| boostg / `e001c016e` CIngBoostBallGuardian | **(a)** Root actor is immune in Lurk and every other form; use its collision actors. | IngBoostBallGuardian `fn_30_D230`, `D23C bl ImmuneVulnerabilty`; collision vulnerability routing is `30E4`/`3190`. | None. |
| cmdpirate / `e003b010e` CCommandoPirate | **(a)** Scene corpse, already -1 HP. Vulnerability allows power, but dead health is not subtracted again. | CommandoPirate `CE04 lbz 0x91c`, `CE08 clrlwi. ...,31`, `CE0C beq` skips the corpse arm only if bit 0 is clear; `CE40` clears alive; `CE5C..CE64` writes rodata `4FC = -1.0` to health+4. DOL ApplyLocalDamage `8003D8B4..8003D8F0` returns after recording damage when HP <= 0. | None. |
| digig / `e00350060` CDigitalGuardian | **(a)** Root actor is always pass-through, including Stage1Logic. Damage belongs on the boss parts. | DigitalGuardian `fn_14_9FE4`, `9FF0 bl PassThroughVulnerabilty`. | None. |
| emp3 / `e000b0094` CEmperorIngStage3 | **(a)** Root's shipped power vulnerability is pass-through, multiplier zero; heart collision actors receive boss damage. EyeShut also governs heart vulnerability, but is not required for root immunity. | EmperorIngStage3 vtable `.data 4F0`, slots 16/17 use CPatterned; DOL `80074348 bl GetDamageVulnerability__3CAi`. MREA `d7c3b839`, layer 4, EMS3 `100b0094`: power/missile `(0%, effect 2)`. Heart immunity helper `fn_18_1C34`; EyeShut `5264`. | None. |
| shrd / `u04e2`, editor `00020081` CShredder | **(a)** Power is effect 3 (no direct HP damage) with multiplier 5 for its fuse. **(b)** The console used the player as sender, so resisted damage never entered the weapon-only fuse update. | MREA `7e0be699`, layer 3, SHRD `0c020081`: power `(500%, effect 3)`, missile `(100%, effect 0, ignore-radius true)`. Shredder `1FFC` casts sender to CWeapon, `2004 beq 2080` skips a non-weapon; `2024 bl GetVulnerableDamage`, `2038 fadds`, `203C stfs 0x80c` accumulates fuse damage. | Supply an actual player-owned weapon/projectile sender in CmdDamage. |

Common damage gate: DOL CStateManager `8003E470 bl WeaponHits`, `8003E478 beq 8003E514`
takes resisted messages instead of ApplyLocalDamage (`8003E4C4`). CDamageInfo
`800B62EC cmpwi 3` / `800B62F4` returns zero for effect 3, while GetVulnerableDamage
`800B62AC fmuls` deliberately uses its multiplier. ScriptLoader `8023B2B0 fdivs`
divides the property percentage by `lbl_8041DC8C = 100.0`.

Commando's MREA `645ef243`, layer 0, CMDO `003b010e`, property `7abed4ce = 0x80`
sets the first flag byte's bit 0 (bit reversal in `fn_9_EDC0`, `EED4..EF60`). Its
starting health property is healthy; the AreaLoaded corpse arm intentionally writes -1.

Emperor's `fn_18_9044` TakeDamage is a retail `blr`, but that is **not** why direct
HP stays unchanged: ApplyLocalDamage writes HP before calling TakeDamage. The root's
pass-through vulnerability prevents ApplyLocalDamage from being reached at all.

## Console correction

`platform/port_console.cpp` now creates an effect-free CGameProjectile for beams/missiles,
or a CWeapon with the matching bomb attribute for bombs. Body attacks retain the player
source. Source position/direction describe the impact rather than the player's feet/facing;
the requested weapon mode, amount and combo flag reach the receiver. The player remains owner.

This matches CGameProjectile::ApplyDamageToOneActor: DOL `800342B4 lhz 0x15c` (owner),
`800342C8 lhz 0x8` (projectile id), `800342D8 bl ApplyDamage`.
CPatterned also needs the proper type: `80079814`/`80079848` cast to CGameProjectile;
the resisted branch checks its player owner at `80079854..80079874` before setting the
projectile-hit flag at `80079880..80079884`.

The ephemeral source is registered until the queued Damage/ResistedDamage message is delivered;
its queued deletion follows in FIFO order. No gun asset, visuals, flight, collision search or
ammo consumption is needed. `shoot` remains the command for flight/collision checks.
The target id is saved before ApplyDamage so the post-damage lookup cannot dereference a deleted actor.

## Runtime proof

- Original six-family smoke reproduced every reported HP value; existing `MP2_DEBUG_BALL=1`
  showed hits=0 for all except the corpse (hits=1, damage=10, HP already -1).
- GDB at Shredder AcceptScriptMsg and its return: a resisted 100-power hit changed
  `mDamageTaken 11.0994911 -> 511.099487`, `mHitByPlayerProjectile 0 -> 1`, HP 500 -> 500.
  No member forcing was used for this observation.
- A separate **forced old-sender** run changed only the resisted message's sender back to
  player `u0400` before its handler. Both tested Shredders gained zero fuse damage and kept
  hit flag 0. The second remained present at f800. This is a controlled reproduction of the
  old command's sender semantics, not a claim that it ran an old executable.
- `tools/smoke/cmd_shrd.txt:19-23` adds a second Shredder's natural detonation check:
  power hit at f600, no kill/XDMG, absent at f800 after the fix; present with the old sender.
  Its separate missile control shows HP **500 -> 499**. Requiring power itself to lower HP
  would contradict the retail effect-3 rule; the corrected power effect is fuse damage/detonation.
- Final six-family run: `--headless --capped --jobs 2`; each rc=0, caught=0, reach-stub=0,
  console errors=0. Areas: agon54 `42b935e4,08_sand_casualpirates` (2500 updates), boostg
  `3dfd2249,06_swamp_boostballguardian_dark` (2200), cmdpirate
  `3bfa3eff,05_temple_federationsite_dark` (2200), digig `1baa96c2,09_cliff_temple_dark` (2200),
  emp3 `863fcd72,07_temple_mothlordsanctuary_dark` (2600), shrd
  `3dfd2249,01_swamp_entrance` (1100).
- An intermediate unpaced emp3 run connected after f2396 and missed layer activation;
  it is not evidence of a passing damage check. The capped rerun executed every command.
- A separate 700-update Agon check covered bomb, powerbomb, darkburst, boostball and missile
  modes: HP 60 -> 59 -> 58 -> 56 -> 55 -> 54; rc=0, caught/reach-stub/errors=0.
  `objs GameProjectile` and `objs CWeapon` both returned zero afterwards (source cleanup).

Slim transcripts, probe results and build logs: `/tmp/opencode/reports/dmgnoop-evidence/`.
Report: `/tmp/opencode/reports/dmgnoop.md`. No game function was changed or newly marked.

## Build gates

`build-port-link/metroid_prime2_port` builds; `build-port`'s mp_game/mp_platform/mp_port_entry
builds with `CPLUS_INCLUDE_PATH=$PWD/build-port-link/_deps/sdl-src/include`.
The unmodified headers-only CMake path omits SDL headers for main.cpp: without that environment
the pre-existing entry target fails at `platform/main.cpp:50`. No CMake/source workaround is included.
`python3 configure.py && $T/ninja` passes with Game Code 63.29% matched; explicit
`build/tools/dtk shasum -c config/G2ME01/build.sha1` validates all 87 files.
`python3 tools/gen_files_cmake.py --check` and `git diff --check` pass.
Only port console and smoke-check code changed, so the non-PC source path is untouched.
