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
| PreRenderBoid (fn_49_12AC) | **fixed** (TARGET_PC, cpp:218-223): on the slot -1 path retail tests mSharedAttackBit (0x12E4 `lwz r0,0x5f4(r30)`, 0x12F4 `and.`) but clears mAttackModelBitBase (0x1308 `lwz r0,0x5f8(r30)` / 0x1314 `slw` / 0x1318 `andc` / 0x131C `stw`); upstream cleared mSharedAttackBit |
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
| PreRenderBoid 12AC | 218 1/2 (slot -1 path), not reached even with the force. psc3: 218:1/2 proved dead in the shipped configuration (4 base models, 3 slots, shared bit 7; only two mask clears, neither of the shared bit), so 229:1/2 is the one live arm left |
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

## Third pass (psc2, 2026-10-24, base a0e2a87c0) — corrected by psc3

Four forced runs, not three: `/tmp/opencode/psc2_{a,b,c,d}` (game.log, forced.log, forced.txt, drive.log,
shell log) are preserved. What those logs show, against what psc2 concluded:

- **Fact: no crash was observed.** Not one of the four game.logs has a "Program received signal", a gdb
  stop, a backtrace or a Python traceback; each ends on watchdog lines. The wrapper's own record is
  "no normal inferior exit in game.log", i.e. gdb/the game were killed and gcov never flushed. "Died
  without a clean exit" is right; "the attack-state forces crash" is a hypothesis the logs do not carry.
- **Fact: the frames were still advancing, very slowly.** 68-93 watchdog lines per run: 5 s per
  presented frame from about presented frame 254-267, then 15 s and 30 s, main thread in
  'game update' and the render thread in 'after submit' / 'waiting for a frame slot'. The `obj` answers
  at f600/f900 came back; the console lines after them never ran because the run was killed first.
  The two stacks that resolve to port code are both geometry paths, not render or model paths:
  psc2_b's `CSwarmBasics::UpdateBoid+0x97d`, and psc2_c/d's 0xf50c5f, which objdump places at
  `CPlantScarabSwarm::Think+0x35f` - the instruction right after `call IsSpaceAboveBoidClear`
  (`f50c5a call f4f170`), i.e. the line-108 space-above collision query (both stacks also show a
  libc syscall frame below).
- **Unknown: the cause.** No exception is observed or evidenced in the four logs (none carries a
  signal, a gdb stop, a backtrace or a traceback), but that is absence of evidence, not proof: the
  wrapper's own record ends at "no normal inferior exit". The missing-shared-model hypothesis is
  refuted for the measured path (see the fourth pass), not by these logs. The difference from the psc1
  runs (same driver, 3000 frames, exit 0) is the far
  heavier write schedule: period 1 and 4, 6297-7381 writes per run, 6000+ of them raising gdb errors.
  psc3's two 1200-frame runs have no watchdog line at all and exit normally, but they stop before the
  update at which psc2 stalled, so they do not bound the cause either.
- **Fact: those write errors are harness errors, not game faults.** psc2_a's `$rsi->mAttacking` is the
  CStateManager& (set failed every call, "Attempt to extract a component of a value that is not a
  structure"); psc2_c/d's PreRenderBoid write `*(unsigned int*)$rdx` succeeded but its *restore* failed
  ("Cannot access memory at address 0xbe728392") because force_ret.py re-evaluates a register lvalue at
  the return, where rdx is no longer the draw mask. That write needed no restore: `$rdx` is a `uint*`
  pointing at the `drawMask` local of CSwarmBasics::PreRender (cpp:1027), a stack variable.
- The psc2 spec files were reverted and are not in the tree; only their logs survive.

The old conclusion ("every remaining arm is behind a shared attack model, which no placement carries;
forcing the boids crashes unless a shared attack model exists") is refuted for its model claim by the
shipped data and by measurement (below), and the four logs contain no observed instance of a forcing
crash. No stub or fake model allocation is involved or needed.

## Fourth pass (psc3, 2026-10-24, base 71c81f4bf = psc2 notes + port 053afe3e0 merged)

Two runs, one per area, 1200 frames, fast; both `[port] caught/reach-stub: 0` and both
"`[Inferior 1 (process N) exited normally]`" with gcov written and no watchdog line. Specs
`tools/cov/specs/psc3_plant_probe.py` (read-only) and `psc3_plant_force.py` (guarded boid-state write, at
most two calls, no mask write); drivers
`psc3_plant_0g.con`, `psc3_plant_ok.con`.

    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/psc3_plant_probe.py /tmp/opencode/psc3_r1 \
      3dfd2249,0g_swamp_hall --con tools/cov/specs/psc3_plant_0g.con --fast --frames 1200
    BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/psc3_plant_force.py /tmp/opencode/psc3_r2 \
      3dfd2249,ok_swamp_hall --con tools/cov/specs/psc3_plant_ok.con --fast --frames 1200
    python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/psc3_r1/gcov \
      /tmp/opencode/psc3_r2/gcov --module PlantScarabSwarm

The three placements are identical in the shipped data (`tools/scan_pssm_props.py`): attackerCount 3,
attackProximity 15, attackTimer 0.5, maxAttackAngle 60, intoAttackAnimation 2, attackAnimation 1,
isVulnerableToSafeZone 0, ANCS 0x8bb689b4. The ctor gates on `animRes.GetId() != kInvalidAssetId &&
intoAttackAnimation != -1` (cpp:44-49) for the attack models and on `attackAnimation != -1` (cpp:65) for
the shared one, so both are built on every shipment. Measured per placement (see the attribution note
under the table: the r2 Think line is deduped by counts, not by object pointer):

| placement | run | measured |
|---|---|---|
| 0g_swamp_hall e0c1e00b6 | r1, 976 Think hits, 1914 PreRenderBoid hits | 0g_swamp_hall has exactly one PSSM placement, so this run's deduped line is attributable: attackerCount 3, attackModels 3, modelDatas 4, mSharedAttackModel / mSharedAttackState / mSharedAttackDeltas all non-null, mSharedAttackBit 7, mAttackModelBitBase 4. 85 attacking-boid draws (slot 0: 40, 1: 19, 2: 26) and **no attacking boid with slot -1**. The draw mask carried the shared bit on all 1914 calls (the bitBase bit on 1261) |
| ok_swamp_hall e0c2d009a | r2 | loaded and active (models 4, dl 1, animated 1) but boids=0/20 active at f600 and f1190, so no boid of this swarm was drawn. Its Think still runs, so the run's deduped model line may equally come from this object |
| ok_swamp_hall e0c2d0076 | r2 | 20 created, 17 active at f900/f1100, models 4. Its draw-side evidence is the force's per-call guard line (shared model and skinned state non-null on the forced call, on that call's real `$rsi` boid): 0076 is the only swarm with live boids in this run, so the forced draw is its. The Think line is **not** attributable, and mSharedAttackDeltas is measured only as "some instance in the area has it non-null", not per placement |

Attribution limit: `psc3_plant_force.py`'s `think_diag` dedups on (shared bit number, attack-model count,
base model count), not on the object pointer, so it proves "an instance with this configuration was
seen", nothing per object. Per-object proof of the shared model and skinned state comes only from the
per-call guard line written when the force's guard passed on a real boid (below).

What the runs settled, per arm of PreRenderBoid (fn_49_12AC, cpp:214-236), which now reads
**12/12 lines, 6/8 branches**, untaken @ 218:1/2, 229:1/2:

- 217's true arm (attacking, slot -1) and 218:0/2 (the shared pose) ran in r2, so lines 219 (CachePose of
  the real `*mSharedAttackModel` / `*mSharedAttackState`) and 223 are covered. RenderBoid went
  ALL-ARMS (7/7, 4/4): the forced boid was drawn from the real shared skinned state. The force wrote only
  the boid's `mAttacking`/`mAttackSlot` on the real boid argument (`$rsi`; `$rdx` is the `uint*` mask
  pointer) and touched no mask; the forced boid then died through retail's own path (UpdateBoid's
  `else if (boid.mAttacking)` -> KillBoid). Both writes, the one in the "bitBase still set" pass and the
  one in the "bitBase already cleared" pass, took **this same true arm**: 218 tests the shared bit.
- 218:1/2 is **dead, not missing** in the verified shipped configuration (4 base models, 3 attack slots,
  mSharedAttackBit 7): `uint drawMask = -1` (CSwarmBasics cpp:1027) sets the shared bit, and the module's
  only two mask-clearing stores are 0x1318 `andc` with its `stw` at 0x131C (bit mAttackModelBitBase,
  0x5f8, read at 0x1308) and 0x1370 `andc` with its `stw` at 0x1374 (`slw` at 0x136C; bit base+slot); the
  base PreRenderBoid clears the bits of `mIndex & (mModelDatas.size() - 1)`, i.e. bits 0..3 with 4 base
  models, so no path clears bit 7. The test itself is 0x12E4 `lwz r0,0x5f4(r30)`, 0x12F0 `slw`,
  0x12F4 `and.`, 0x12F8 `beq`. This is not a claim about other configurations: the bit numbers move with
  attackerCount, and with 8 or more base models the base clear (`size()-1` = 7 masks) could reach the
  shared bit and make the arm live.
- 229:1/2 (a slot-0 attacker finding bit 4 already cleared by a shared pose in the same pass) is live and
  did **not** run in psc3 - it is the one arm the marker waits for. psc1's recorded profile (12/12 lines,
  7/8 with only 218:1/2 open) implies both of its arms ran there, its force having cleared bit base with
  the no-restore `!` form; that gcov dir is gone, so psc3 does not re-cite it as evidence.

Marker: **absent** - 229:1/2 is still outside a preserved psc3 profile. `// @ fn_49_12AC` goes above the
body once it is in one: the next run extends the r2 force so that, in the pass where the shared pose has
already cleared bit 4, a later boid is written with `mAttacking = 1, mAttackSlot = 0` instead of the
repeated slot -1 write (the spec's two writes today differ only in the bitBase bit, which 218 does not
read).
