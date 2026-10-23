# Glowbug REL (module 26): analysis notes

CGlowbug (GBUG, "Mechanism: Lightflyer") is a small floating bug. It patrols, and when the player
comes into its attack range it telegraphs and then fires an electric beam at them. When it dies it flashes,
falls and breaks apart. The module's code is upstream's `src/MetroidPrime/Enemies/CGlowbug.cpp`, and the loader is
`include/MetroidPrime/ScriptLoader/SLdrGlowbug.hpp`. Retail asm:
`build/G2ME01/Glowbug/asm/MetroidPrime/Enemies/CGlowbug.s`. The per-function table is `Glowbug_functions.tsv`
(B upstream body, H weak/template copy, T REL runtime).

## Placements (`tools/scan_glowbug_props.py`)

The disc has five GBUGs:

- three in Great Temple `0i_temple_hall`, layer 0: e000500f0, e000500f8 and e000500f9
- one in `0g_temple_hall`, layer 3: 0c0100e6
- one in Torvus `03_swamp_forgottenbridge`, layer 0: 000701c2, which starts inactive

docs/modules.tsv also lists `0n_sand_hall_dark` and `03_swamp_forgottenbridge_dark`. Those areas only load
the module and have no GBUG object, so no dark-world Glowbug exists in shipped data.

All five have the same values for every property the code branches on:

- minAttackRange 0.0 and maxAttackRange 12.0
- isInLightWorld 1
- stateMachine a5800138 and ANCS b8d34763
- every asset is valid: deathFlash 6ad002cf, breakApart 774a6b4a, attack ELSC 0bbd0c33, telegraph ce920ca6,
  echo 101f88aa and scanModel 90e535c5
- connections are only PTRL/FOLW, DEAD/ZERO and RATL/ZERO, and none is APRC

FSM a5800138 (`tools/fsm_state_triggers.py 0xa5800138`):

| state | transitions |
|---|---|
| Start | Default → Patrol |
| Patrol | ShouldAttack → AttackTelegraph |
| AttackTelegraph | ShouldAttack → Patrol; AttackTelegraphDelay → Attack |
| Attack | AttackFinished → Patrol |
| Dead | none |

No state uses AnimOver.

## Survey (gb2, 2026-10-23)

| function | verdict |
|---|---|
| SLdrGlowbug ctor (fn_26_D68) | **fixed** (TARGET_PC). Retail sets defaults that upstream left at the base values: attackTelegraphSound and attackSound = -1 (0xD90, 0xDF4, 0xDF8); isEchoEmitter true (0xDBC / 0xE08); knockback 2.0 (0xDB4 / 0xE04); ambient (1,1,1,1) (0xDC0 / 0xE20); visorFlags 0xF (0xE1C / 0xE2C) |
| every other body | equivalent to the retail asm (read against the raw asm, because `asmcheck.py check` found no functions in this module) |

## Coverage (gb2)

The coverage build uses spec `tools/cov/specs/glowbug.py` and driver `tools/cov/specs/glowbug_templehall.con`:

    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/glowbug.py <c> 863fcd72,0i_temple_hall \
      --con tools/cov/specs/glowbug_templehall.con --fast --frames 2600
    python3 tools/cov/gcov_report.py build-port-cov <c>/gcov [<f>/gcov] --module Glowbug

The driver does these steps:

1. Turns on god mode and gives missiles and the scan visor.
2. Sends `Create` and `Activate` to the three bugs.
3. Places the player at 6, 12, 3, 20 and 0.5 units from them.
4. Shoots one bug with a beam and with a missile, which kills it.
5. Scans e000500f9 repeatedly (`scan` every 30 updates), which drives ScanVisorRender.
6. Places the player 0.5 units from e000500f0, then kills it with two missiles.
7. Sends `Deactivate` and then `Delete` to e000500f9.

No dark area has a GBUG, so `tools/cov/specs/glowbug_force.py` forces the dark arms. Run it with
`COV_GDB_SCRIPT=tools/cov/cov_force.py tools/cov/force.sh` and the same arguments. It forces:

- mIsInDarkWorld at Death and Dead: the DeathGlow arm, the dark material set, and the DGNR break-apart.
- mAttackPhase 2 at Death, which is the state after a kill mid-attack, because only Patrol clears the phase.
  This covers the arms of Think and Render where mAlive is false.

Across the merged runs (one plain, three forced), each game exited with rc 0 and logged 0 `[port] caught` and 0 `reach-stub`.

- **Every arm taken:** Dead, Death, Attack, AttackTelegraph, Patrol, ShouldAttack.
- **Ran, with no branches:** IsScanVisorSelfRender, AttackTelegraphDelay, AttackFinished, IsInRange,
  SetupStateMachine, PreRender, AddToRenderer, the dtor, SetFuncPtrs, RELMain, RELExit.
- **Never ran:** AnimOver. FSM a5800138 has no AnimOver trigger, so it stays unmarked.

Every untaken arm is dead in shipped data. Line numbers are cpp:taken/total, from gcov on the current file:

| function | untaken arms | why dead |
|---|---|---|
| ctor | 63-69, 89, 94/97/100 (invalid-asset arms); 72/76/77/88/90/91 (EH throw/cleanup) | every asset is valid in all five placements |
| AcceptScriptMsg | 125-128 (APRC → camera shaker) | no placement has an APRC connection |
| Think | 163:1/6, 168:1/6 (echo/electric generator null) | the assets are valid |
| Think | 168:5/6 (beam id invalid in phase 2) | phase 2 is entered only after ShouldAttack found a target; mBeamTargetId is assigned only that valid id and is never reset |
| Think | 170:1/2 (beam target not a CActor) | the player is the only candidate |
| Think | 183, 185 (camera shaker) | no shaker id (no APRC) |
| Render | 208:1/6, 211:1/6 | the assets are not null |
| GetScanVisorRenderBounds, ScanVisorRender | 235:1/2, 244:1/2 | the scan model is valid everywhere |
| FindAttackTarget | 382:2/4 (inside the minimum range) | minAttackRange is 0, so `MagSquared < 0` is never true |
| FindAttackTarget | 385/386 (score not better) | with a single candidate, its finite score is always below the FLT_MAX start |
| LoadGlowbug | 407:1/2 (null model data) | all five placements use ANCS b8d34763, which loads in 0i (three bugs spawned) |

Not driven arm by arm: the SLdr dtor, `__sinit` and the H copies.

## Smoke family (proposal)

`gbug`: area `863fcd72,0i_temple_hall`, driver `tools/cov/specs/glowbug_templehall.con`, 2600 frames, `--fast`.

- Expect three Glowbugs in `objs Glowbug`, then e000500f8 and e000500f0 Dead.
- e000500f9 is gone after `Delete`.
- Pass: 0 `[port] caught` / `reach-stub`.
