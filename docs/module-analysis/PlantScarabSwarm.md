# PlantScarabSwarm REL (module 49): analysis notes

CPlantScarabSwarm (PSSM, "Morphology: Seedburster") is a CSwarmBasics swarm. When the player is within the attack
proximity and angle, up to `attackerCount` boids take an attack slot. They play the into-attack and attack
animations on per-slot models and launch bouncy grenades (SpawnGrenades, triggered by the animation POIs). The
module's code is upstream's `src/MetroidPrime/Enemies/CPlantScarabSwarm.cpp`, with the loader
`include/MetroidPrime/ScriptLoader/SLdrPlantScarabSwarm.hpp`. Retail asm:
`build/G2ME01/PlantScarabSwarm/asm/MetroidPrime/Enemies/CPlantScarabSwarm.s`. Per-function table:
`PlantScarabSwarm_functions.tsv` (71 fns; B upstream body, H weak/template copy, T REL runtime).

Placements (`tools/scan_pssm_props.py`): Torvus Bog 0g_swamp_hall e0c1e00b6 and ok_swamp_hall e0c2d009a and
e0c2d0076, all on layer 3. All three have attackerCount=3, attackProximity=15, attackTimer=0.5,
maxAttackAngle=60, intoAttackAnimation=2, attackAnimation=1, isVulnerableToSafeZone=0, and ANCS 0x8bb689b4.
The driver area is `3dfd2249,0g_swamp_hall`.

## Survey (pss1, 2026-10-23)

| function | verdict |
|---|---|
| SLdr ctor (fn_49_858) | **fixed** (TARGET_PC, SLdrPlantScarabSwarm.hpp:72-84): grenadeBounceSound and grenadeExplosionSound are -1 (0x89c `li r0,-0x1` / 0x8c8 `stw 0x2fc`, 0x8cc `stw 0x308`); ambient is (1,1,1,1) (0x8d0 `bl CColor ctor`, rodata_0 / 0x8e4 `stw 0x50`); visorFlags is 0xf (0x8e8 `li r0,0xf` / 0x8f4 `stw 0xa0`) |
| PreRenderBoid (fn_49_12AC) | **fixed** (TARGET_PC, cpp:220-226): on the slot -1 path retail tests mSharedAttackBit (0x12E8 `lwz 0x5f4`) but clears mAttackModelBitBase (0x1308 `lwz 0x5f8` / 0x1314 `andc`); upstream cleared mSharedAttackBit |
| every other body | equivalent to the asm. The remaining diff is noise: the loader's scale (sOneVector), the shape of push_back_unsafe/uninitialized_copy, and register/branch layout |

## Coverage (pss1)

Specs: `tools/cov/specs/plantscarabswarm.py` (breakpoints) and `plantscarabswarm_force.py` (one-call forces).
Driver: `plantscarabswarm_swamp.con`. It sets layer 3 and god mode, teleports next to e001e00b6 and idles so the
attacks run, then shoots it (power, charged, missile), sends Deactivate/Activate, and Deletes it.

    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/plantscarabswarm.py /tmp/opencode/pss1/r1 \
      3dfd2249,0g_swamp_hall --con tools/cov/specs/plantscarabswarm_swamp.con --fast --frames 3000
    BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh \
      tools/cov/specs/plantscarabswarm_force.py /tmp/opencode/pss1/f1 3dfd2249,0g_swamp_hall \
      --con tools/cov/specs/plantscarabswarm_swamp.con --fast --frames 3000
    python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/pss1/r1/gcov /tmp/opencode/pss1/r2/gcov \
      /tmp/opencode/pss1/f1/gcov --module PlantScarabSwarm

Runs: 0 `[port] caught` / `reach-stub`, exit 0, grenades spawned. The force run forced Think's maxAttackAngle to 0
535 times and PreRenderBoid's slot bit 462 times.

The following are marked because every live arm ran:

- AcceptScriptMsg, ~CPlantScarabSwarm, KillBoid, IsSpaceAboveBoidClear, GetBoidTopPosition, GetGrenadeBounds and
  LoadPlantScarabSwarm.
- SetFuncPtrs, RELMain and RELExit.
- AllocateSkinnedModels (cpp:143) and UpdateSwarmAnimations (cpp:162): the open arm is "no shared attack model".
  It is dead in data, because the shared model exists when attackAnimation != -1 and every placement has 1.
- UpdateAllBoidMovement: the open arm is `!mAnimated`. It is dead because the ctor passes `true` to CSwarmBasics
  (cpp:35).
- UpdateBoid: the open arm is the safe-zone kill (cpp:194). It is dead in data, because every placement has
  isVulnerableToSafeZone=0.

Open arms (cpp line), so these bodies stay unmarked:

| function | open arms |
|---|---|
| ctor 2300 | 50 1/4 and 3/4 (invalid animRes, intoAttack -1: data), 65 1/2 (attackAnimation -1: data), 71 1/3 (rs_new null/EH), EH cleanup lines |
| Think 1B64 | 112 1/2 (no free attack slot: attackerCount 3, live but not hit), 102 3/4 (compound loop condition) |
| PreRenderBoid 12AC | 218 1/2 (slot -1 path), not reached even with the force |
| RenderBoid 1254 | slot -1 arm (cpp:241), forced but not reached |
| ApplySteeringBehaviors 1228 | attacking-boid arm (cpp:254) |
| BoidCollidedCallback 11C0 | never called |
| BoidCollidedWithPlayerCallback 1144 | launched arm (cpp:264), forced but not reached |
| SpawnGrenades C6C | POI arms at cpp:292-295 (combinations of weight, character and value; ANCS data) |

Not driven arm by arm: the SLdr ctor (fixed above) and dtor, plus the weak copies (H).

## Smoke family (proposal)

`pssm`: area `3dfd2249,0g_swamp_hall`, driver `tools/cov/specs/plantscarabswarm_swamp.con`, 3000 frames, fast. Expect
`objs PlantScarabSwarm` to list e001e00b6 until the Delete. The pass condition is 0 `[port] caught` / `reach-stub`.

## Second pass (psc1, 2026-10-24, base a309e96ed)

The pss1 spec, forcing spec and driver are in the tree; this pass re-ran them at the current base and added
one force (`1228`'s attacking boid). Two runs, both `[port] caught/reach-stub: 0` and both exited
normally (gcov written):

    python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/psc_{a,b}/gcov --module PlantScarabSwarm

| body | arms | open (this base) |
|---|---|---|
| 12AC PreRenderBoid | 12/12 lines, 7/8 | 218:1/2 - the slot -1 path with the *shared* attack bit set in the draw mask (the existing force clears the attack-model bit, which takes 217's arm but not 218's true arm; the force must set `1 << mSharedAttackBit` instead) |
| 1254 RenderBoid | 6/7, 3/4 | 242:0/2 - the slot -1 arm; the one-call write of `$rsi->mAttacking`/`mAttackSlot` did not reach it (the boid may be a copy at entry) |
| 1228 ApplySteeringBehaviors | 4/4, 1/2 | 256:1/2 - an attacking boid; the new `$rsi->mAttacking = 1` force did not fire (the entry's phase counter) |
| 1B64 Think | 35/35, 46/51 | 90:1/2, 92:3/4, 102:3/4, 108:6/7, 112:1/2 - the attack-slot tests; the existing mMaxAttackAngle force did not move them |
| C6C SpawnGrenades | 29/29, 20/26 | 296:1/2, 297:0/2, 298:0-3/4, 299:1/2 - the POI weight/character/value combinations (ANCS data) |
| 11C0 BoidCollidedCallback, 1144 BoidCollidedWithPlayerCallback | 0 lines | never called in the driven fight; the callbacks come from CSwarmBasics on a boid collision |

No marker this pass: the closest body, PreRenderBoid, is one arm short.
