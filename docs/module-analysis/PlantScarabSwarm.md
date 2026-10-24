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
| Think 1B64 | 112 1/2 (no free attack slot: attackerCount 3, live but not hit), 102 3/4 (compound loop condition). psc5: 86/90/108 ran; 112 ran only in the second launch's slot-full setup (it is live: CSwarmBasics.cpp:642/1409 deactivate a boid without clearing its slot); 102's second operand and one host static-guard edge are dead. Body marked (arm table in the sixth pass) |
| PreRenderBoid 12AC | 218 1/2 (slot -1 path), not reached even with the force. psc3: 218:1/2 proved dead in the shipped configuration (4 base models, 3 slots, shared bit 7; only two mask clears, neither of the shared bit). psc4: 229:1/2 ran with a real slot-0 boid after a transient shared pose, so the body is marked (arm table in the fifth pass) |
| RenderBoid 1254 | slot -1 arm (cpp:241), forced but not reached |
| ApplySteeringBehaviors 1228 | attacking-boid arm (cpp:254) |
| BoidCollidedCallback 11C0 | never called |
| BoidCollidedWithPlayerCallback 1144 | launched arm (cpp:264), forced but not reached |
| SpawnGrenades C6C | POI arms (weight, character, value; ANCS data). psc6 sampled 20/26 and could not prove the rest (the POI buffer is static shared storage); psc7 measured them with a transient node-field fixture on the real first node - 26/26 forced, natural calls unchanged - so the body is marked (arm table in the eighth pass) |

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
  0x12F4 `and.`, 0x12F8 `beq`.
- 229:1/2 (a slot-0 attacker finding bit 4 already cleared by a shared pose in the same pass) is live and
  did **not** run in psc3 - it is the one arm the marker waits for. psc1's recorded profile (12/12 lines,
  7/8 with only 218:1/2 open) implies both of its arms ran there, its force having cleared bit base with
  the no-restore `!` form; that gcov dir is gone, so psc3 does not re-cite it as evidence.

Marker: **absent at psc3** - 229:1/2 was still outside a preserved psc3 profile. The fifth pass below ran
it and added `// @ fn_49_12AC`.

## Fifth pass (psc4, 2026-10-24, base 791ddc44e)

One launch (1 of the slice's 2), `3dfd2249,0g_swamp_hall`, 1200 frames, fast, `[port] caught/reach-stub: 0`,
`[Inferior 1 (process N) exited normally]`. Spec and gdb side in one lane file: `tools/cov/specs/psc4_plant_arm.py`
(the fixture lives there, so no shared tooling changes), driver `tools/cov/specs/psc4_plant_0g.con`.

    BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/psc4_plant_arm.py tools/cov/force.sh \
      tools/cov/specs/psc4_plant_arm.py /tmp/opencode/psc4_r1 3dfd2249,0g_swamp_hall \
      --con tools/cov/specs/psc4_plant_0g.con --fast --frames 1200
    python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/psc4_r1/gcov --module PlantScarabSwarm

Fixture: the slot-0 side is a **real** boid - the swarm's own mAttackSlotBoids[0] owner, attacking, drawn
after the pose boid because the pose boid is the first active boid the pass draws. Only the shared pose is
supplied, transiently: on the first PreRenderBoid call of a pass whose mask is 0xffffffff, with slot 0
owned and a clean boid, its `mAttacking`/`mAttackSlot` are written for that call only and restored at the
body's return through the cached absolute boid pointer, before any movement or render uses them. No mask
byte is ever written: the bit-4 clear is the body's own 0x1318 `andc`. Nothing ran before slot 0 had a real
owner (diag: "pose: waiting, no real slot-0 owner yet ... slots=[-1, -1, -1]").

Measured, per instance (`this`, boid and owner pointers; `/tmp/opencode/psc4_r1/diag.log`, `fixture.txt`):

    pose: this=0x555558a83340 boid=0x7fffab41e240 mask before=0xffffffff (bitBase 1, shared 1); shared
      model+state present; models=3 states=3 modelDatas=4 attackers=3 sharedBit=7; real slot-0 owner
      index 7; transient attacking=1 slot=-1 for this call
    pose: restored boid 0x7fffab41e240 attacking=0 slot=-1; mask 0xffffffff -> 0xffffffef (body cleared bitBase)
    229 FALSE ARM: this=0x555558a83340 boid=0x7fffab41e300 slot=0 mask=0xffffffef (shared bit 1, bitBase 0
      clear); models=3 states=3 base=4 sharedBit=7 slots=[7, 8, -1]
    passes=122 poses=1 false_arm=1

So a real slot-0 boid's own `*drawMask & (1 << (mAttackModelBitBase + 0))` read bit 4 clear later in the
same pass, after the shared pose's CachePose, with the real body doing the clearing (0x1318) and the
counts validated at the pose (3 attack models, 3 skinned states, 4 base models, 3 attacker slots, slot 0
owned by boid index 7). Profile now: PreRenderBoid **12/12 lines, 7/8 branches**, untaken @ 218:1/2 only
(psc3's 12/12 + 6/8 merged with this run).

Arms of fn_49_12AC (cpp:214-236 before the marker line, 215-237 after):

| arm (cpp) | asm | status |
|---|---|---|
| 214 `boid->mAttacking` true / false | 0x12CC `lbz r0,0xb2(r4)`, 0x12D0 `extrwi. r0,r0,1,28`, 0x12D4 `beq` -> 0x137C base | both ran |
| 216 `slot == -1` true / false | 0x12D8 `lwz r29,0x9c(r4)`, 0x12DC `cmpwi r29,-0x1`, 0x12E0 `bne` | both ran |
| 218 shared-bit test true / false | 0x12E4 `lwz r0,0x5f4(r30)`, 0x12F0 `slw`, 0x12F4 `and.`, 0x12F8 `beq` | true ran (psc4 pose); false dead (below) |
| 219 `CachePose(*mSharedAttackModel, *mSharedAttackState)` | 0x12FC `lwz r4,0x5e8`, 0x1300 `lwz r5,0x5ec`, 0x1304 `bl` | ran |
| 223 clear mAttackModelBitBase | 0x1308 `lwz r0,0x5f8`, 0x1314 `slw`, 0x1318 `andc`, 0x131C `stw` | ran |
| 229 slot-bit test true / false | 0x1324 `lwz r0,0x5f8`, 0x1330 `add r0,r0,r29`, 0x1334 `slw`, 0x1338 `and.`, 0x133C `beq` | both ran (false in psc4) |
| 230 `CachePose(mAttackModels[slot], mAttackModelStates[slot])` | 0x1340 `mulli r5,r29,0x24`, 0x1344 `lwz r6,0x5b4`, 0x1348 `lwz r4,0x5c4`, 0x134C `mulli r0,r29,0x4c`, 0x1358 `bl` | ran |
| 231 clear base+slot | 0x135C `lwz r0,0x5f8`, 0x1368 `add`, 0x1370 `andc`, 0x1374 `stw` | ran |
| 235 base `CSwarmBasics::PreRenderBoid` | 0x137C `bl` | ran |

Dead-arm proof for 218's false side, shipped configuration only (modelDatas 4, attackerCount 3,
mAttackModelBitBase 4, mSharedAttackBit 7): the mask starts at `-1` (CSwarmBasics cpp:1027) and the only
stores that clear mask bits are 0x1318 (bit 4), 0x1370 (bit base+slot) and the base body's
`mIndex & (mModelDatas.size() - 1)` (bits 0..3); `mAttackSlot` is only ever written -1 or
0..mAttackerCount-1 (Think cpp:118, UpdateBoid cpp:196-199, KillBoid cpp:277-283), so base+slot <= 6 < 7
and no path clears the shared bit, i.e. the test cannot fail. Not claimed for any other configuration.

Marker: **added** `// @ fn_49_12AC` (cpp:214). The same commit corrects the TARGET_PC comment's offsets to
the listing: the test load is 0x12E4 `lwz r0,0x5f4(r30)` (was 0x12E8) and the clear store is 0x1318 `andc`
with `stw` at 0x131C (0x1314 is `slw`). The profile above was taken before those line changes, so it maps
to the pre-marker numbering. RenderBoid 1254 and ApplySteering 1228 markers untouched; strip-diff of the
non-PC text against 791ddc44e is marker-only, matching build 87 files OK, 94.14% fuzzy / 63.29% matched.

## Sixth pass (psc5, 2026-10-24, base 3b3cd4799)

Two launches (the slice's cap), both `3dfd2249,0g_swamp_hall`, 1200 frames, fast,
`[port] caught/reach-stub: 0`, `[Inferior 1 (process N) exited normally]`, gcov flushed. Fixtures are lane
files: `tools/cov/specs/psc5_think.py` (launch 1) and `psc5_think_slotfull.py` (launch 2), drivers
`psc5_think_0g.con` and `psc5_think_0g2.con`.

Numbering: `// @ fn_49_1B64` was added at cpp:85 after launch 1, so launch 1's profile uses the pre-marker
line numbers and launch 2's the current ones (one higher from 86 on). Both are given below.

**Launch 1** - read-only except one transient `mAttackerCount = 0` for a single call (restored at the
body's return through the cached absolute `this`); driver phases: normal fight, the swarm `place`d under
the floor, Deactivate/Activate.

    think=976 inactive=90 zero_write=1 space_calls=3658 space_false=84
    profile: 35/35 lines, 48/51 edges, untaken @ 92:3/4, 102:3/4, 112:1/2   (pre-marker numbering)

**Launch 2** - slot-full fixture (below): `35/35 lines, 46/51 edges`, untaken @ 87:1/2, 91:1/2, 93:3/4,
103:3/4, 109:6/7. Its driver is a plain fight, so it has no Deactivate, no zero-attacker call and no
under-floor phase; **112 (now 113) is not in that list: the loop-exit edge ran 328 times.** Across the two
runs every edge of Think ran except the two dead ones (93:3/4 = old 92:3/4, the host static-guard race;
103:3/4 = old 102:3/4, the loop-invariant operand).

**The cpp:112 correction the review asked for.** The first pass called this arm dead with a free-slot
argument, and that argument is invalid: `attackers` is a snapshot taken at cpp:93-98 and never incremented
inside the attack loop, so `attackers < mAttackerCount` does not by itself mean a slot is free when a later
boid reaches the inner loop - earlier boids of the same call can take slots without changing `attackers`.
The free-slot invariant also does not hold, because CSwarmBasics deactivates a boid *without clearing its
attack slot* in two shipped paths: CSwarmBasics.cpp:642 (`boid.mFramesNotOnSurface >= 30`) and
CSwarmBasics.cpp:1409 (the next waypoint is not a `CScriptWaypoint`). Such a holder keeps `mAttackSlot`
and `mAttackSlotBoids[i]` while cpp:95 stops counting it, so all three slots can be held with
`attackers < mAttackerCount`.

What does hold is the timer half: shipped `attackTimer` is 0.5 on all three placements
(`tools/scan_pssm_props.py`; copied into `mAttackTimer` by the ctor, cpp:38), every allocation sets
`mTimeSinceLastAttack = 0` (cpp:120), and the gate is `mTimeSinceLastAttack >= mAttackTimer` (cpp:108:
0x1CF8 `fcmpo`, 0x1CFC `cror eq,gt,eq`, 0x1D00 `bne`), so after the first allocation in a call the gate
fails for the rest of that call - at most one allocation per call. NaN is safe in the same direction
(unordered leaves EQ clear, the `bne` skips), and a non-positive `mAttackTimer` is not shipped. The arm
can run repeatedly in one call when candidates find all three slots held: unsuccessful searches do not
reset the timer. Only successful allocation is limited to once per call.

Launch 2 built that state directly, on the real object, one call at a time (at most 30 calls, all restored
at the body's return through cached absolute pointers; no branch forced, no mask write, no game change):
with all three `mAttackSlotBoids` owned, one holder (active and attacking) had `mActive` set 0 for the call
and `mTimeSinceLastAttack` set to `mAttackTimer`, so `attackers` dropped to 2 and the attack block ran with
no free slot.

    slot-full: this=0x5555589fcb00 attempt 1; slots owned by boid indices [0, 1, 2], holder
      0x7fffaae0d180 set mActive=0 so attackers 3 -> 2 for this call; mTimeSinceLastAttack 0 ->
      mAttackTimer; models=3 states=3
    slot-full: restored holder 0x7fffaae0d180 mActive=1 ... mTimeSinceLastAttack bits 0x00000000
    think=976 attempts=30 written=30

and that run's gcov: current line 113 (`i < mAttackerCount`, pre-marker 112) 1008 back-edges / **328
condition-false exits**, line 114 (pre-marker 113, slot free) 13 allocations / 995 slot-occupied skips.

| arm (pre-marker / current cpp) | asm | edges | status |
|---|---|---|---|
| 86 / 87 GetActive | 0x1BA4 `extrwi. r0,r0,1,24` (x20 & 0x80), 0x1BA8 `beq` -> 0x1E74 | 886/90 (run 1) | both ran (Deactivate) |
| 90 / 91 mAttackerCount != 0 | 0x1BB4 `cmplwi r0,0`, 0x1BB8 `beq` -> 0x1E38 | 885/1 (run 1) | both: false side forced once; also dead in shipped data (attackerCount 3 everywhere) |
| 92 / 93 static filter guard | 0x1BD8 `bne` -> 0x1C24; init 0x1BDC-0x1C20 has no other test | 1/884 and 1/0 (both runs) | retail guard both ways; the zero edge is the host `__cxa_guard_acquire()==0` race, dead |
| 94 / 95 `it != end` | 0x1C58, 0x1C5C | both | ran |
| 95 / 96 mActive && mAttacking | 0x1C40/0x1C44, 0x1C48/0x1C4C | both | ran |
| 99 / 100 min_val | 0x1C6C, 0x1C70 | both | ran |
| 100 / 101 attackers < mAttackerCount | 0x1C84, 0x1C88 `bge` -> 0x1E38 | 845/40 + 970/6 | ran |
| 102 / 103 `it != end && attackers < count` | 0x1E28 `beq` -> 0x1E38, 0x1E34 `blt` -> 0x1C94 | 42250/845 and 42250/0; 48500/970 and 48500/0 | second operand dead: `attackers` is written only at cpp:93/96 before the loop and cpp:100 guarantees it is < mAttackerCount, so it is loop-invariant |
| 103 / 104 `!mActive \|\| mAttacking` | 0x1C98/0x1C9C, 0x1CA0/0x1CA4 | both | ran |
| 107 / 108 MagSquared < proximity^2 | 0x1CE8, 0x1CEC | 7403/5930, 6732/7140 | ran |
| 108 / 109 timer >= mAttackTimer | 0x1CF8, 0x1CFC, 0x1D00 | 3656/3747, 2051/4681 | ran |
| 108 / 109 IsSpaceAboveBoidClear | 0x1D14, 0x1D18 | 3572/**84** (run 1) | ran: 84 false answers from the under-floor phase |
| 110 / 111 angle <= maxAttackAngle | 0x1D58, 0x1D5C, 0x1D60 | both | ran |
| 112 / 113 `i < mAttackerCount` (loop exhaustion) | 0x1E0C `bdnz` falls through to 0x1E10 after all slots; 0x1D74/0x1D78 is the separate zero-count entry guard | 24/0 (run 1), 1008/**328** (run 2) | **live and run** in launch 2's slot-full setup; natural exhaustion was not observed in these runs |
| 113 / 114 slot free | 0x1D84, 0x1D88 `bne` -> 0x1E04 | 12/12, 13/995 | ran |
| 129-131 / 131-133 SpawnGrenades loop | 0x1E6C, 0x1E70 | both | ran |

gcov's JSON has one more edge on cpp:108/109 (a throw edge, 0 hits - the EH landing pad of the inlined
temporaries); `tools/cov/gcov_report.py` excludes throw edges, which is why that line counts as taken.

Marker: **`// @ fn_49_1B64` at cpp:85** (committed in ba95fc80e; this rework changes docs and lane specs
only, no source). Profiles are reproducible from the archived inputs in `/tmp/opencode/psc5_evidence`:
run 1 needs the pre-marker source (`CPlantScarabSwarm.pre-marker.cpp`, sha256 5878b3a7...41b) and its
matching `.gcno` + `.gcda` (616726d6...c4fc / 55e5c394...2917) - it was rebuilt from that source on
purpose, because the marker's rebuild invalidates the original notes; run 2 matches the current tree
(`psc5_r2.matching.gcno` 3d906014...c67b with `psc5_r2.gcda` 98db9467...c3e6, binary
0f7c0ed4...4794). Raw gcov JSON for both: `psc5_r1_raw.gcov.json`, `psc5_r2_raw.gcov.json`; reports
`psc5_r{1,2}_gcov_report.txt`.

## Seventh pass (psc6, 2026-10-24, base 41506a1af)

Two launches (the slice's cap), `3dfd2249,0g_swamp_hall`, 1200 frames, fast, **read-only probe, no writes
at all**, both `[port] caught/reach-stub: 0` and `[Inferior 1 (process N) exited normally]`. Spec and gdb
side in one lane file, `tools/cov/specs/psc6_grenade_probe.py`, driver `psc6_grenade_0g.con`. The body is
**not marked**: the six POI arms are still open (measured, not proved dead).

Profile (merged over both launches): SpawnGrenades 29/29 lines, **20/26 edges**, untaken @ 298:1/2,
299:0/2, 300:0/4, 300:2/4, 300:3/4, 301:1/2 - the same six the earlier passes left open.

What the probe measures: at every SpawnGrenades entry the real slot argument (`$rcx`), the slot's boid
index, the animation's `mCharIdx`, and the whole int32 POI list the body is about to walk (the static
`CAnimData::mInt32POINodes` with `count = animData.mPassedIntCount`).

    run 1: calls=2904 no_slot=1816 with_nodes=17 zero_count=1071
    run 2: calls=2928 no_slot=2214 with_nodes=11 zero_count=703
    the 28 node-bearing calls (two boots) all walked one node:
      boidIndex=0 slot=0 animChar=0 passedInt=1 stored=16
      nodes=[(weight 1.0, charIdx -1, mVal 26, mType 6, nameHash 2203981133)]

Why that is not a dead proof - the list is shared static storage, not a per-animation table:

- `CAnimData::GetInt32POIList` (CAnimData.hpp:196-199) returns `(mPassedIntCount, mInt32POINodes.data())`,
  and `mInt32POINodes` is a **static** `reserved_vector< CInt32POINode, 16 >` (CAnimData.cpp:40) shared by
  every CAnimData in the process. It is resized once with default nodes - weight 1.0, character -1,
  **value 0**, type kPT_EmptyInt32 (CAnimData.cpp:119-121) - and `clear()`ed (not zeroed) when the last
  CAnimData dies (145-147).
- Every animation advance *appends into that shared buffer*:
  `mPassedIntCount += mAnimRoot->GetInt32POIList(time, mInt32POINodes.data(), 16, mPassedIntCount, 0)`
  (CAnimData.cpp:469-470, 484-485), plus the "pass everything" calls with `CCharAnimTime::Infinity()`
  (718-719, 754-755, 837-838). The write index is the *advancing object's* count, so entry j holds the
  j-th node of whichever animation last wrote index j.
- Order in one tick of `CPlantScarabSwarm::Think` (cpp:85-133): `CSwarmBasics::Think` (CSwarmBasics.cpp:755)
  -> `UpdateSwarmAnimations` (868) -> virtual dispatch into `CPlantScarabSwarm::UpdateSwarmAnimations`
  (cpp:150-168), which advances the four base models first (CSwarmBasics.cpp:895-899), then
  `mAttackModels[i]` for slotted i in slot order, then `mSharedAttackModel` (attack animation #1); then
  `UpdateAllBoidMovement` (869); back in Think the attack block may `SetPhase(0.f)` one attack model
  (CAnimData.cpp:1055; that resets only that object's counts, `ResetPOILists` at 442/554/569 does not clear
  the shared buffer); then the SpawnGrenades loop (cpp:129-132) reads `mAttackModels[i].mPassedIntCount`
  and walks buffer[0, count). So the node the body walks is normally the *last writer's* node - the shared
  attack animation - not the into-attack animation whose `animData` is passed, and the buffer can hold any
  of the animations advanced in that window (any interleaved index).
- Consequence: the measured tuple describes the buffer state of these two boots at the read instants; it
  does not enumerate the shipped POI data of every animation that can occupy buffer[0, count) - the four
  base models' animation, attack animation #2 and the shared attack animation #1, per placement - nor the
  default value-0 nodes, nor the Infinity() pass-all fills. Proving the six arms dead needs those tables
  (no POI dumper exists in tools/, and this slice has no launches left).

Corrected RNG bound (the first version of this note had it wrong): `CRandom16::Float()`
(CRandom16.cpp:43-48) is `1.52590222e-5f * Next()` with `Next() = (mSeed >> 16) & 0xffff` in [0, 65535].
The float32 constant is 1.5259021893143654e-05, and the correctly rounded float32 product with 65535 is
**exactly 1.0**, so `Float()` is in **inclusive [0, 1]**. The gate is `<=` (0xCF0 `cror eq,lt,eq`,
0xCF4 `bne`), so a node with weight exactly 1.0 always passes; only a node with weight < 1 can fail. The
weight-false arm is therefore live in general and merely unobserved here.

Arm status (retail asm; edge counts from the merged profile):

| arm (asm) | condition | edges | status |
|---|---|---|---|
| 0xCAC / 0xCB0 | `boidIndex == -1` -> 0xF88 | 1088 / 1816 | both ran |
| 0xCC0 / 0xCC8 | `count <= 0` -> 0xF88 | 17 / 1071 | both ran |
| 0xCCC / 0xCD0 | `nodes == nullptr` -> 0xF88 | 17 / 0 | dead by construction: `data()` is the address of the static `uchar mData[16*sizeof(T)]`, never null |
| 0xCE4..0xCF4 | `Float() <= node.GetWeight()` -> 0xF78 | 17 / 0 | **open** (unobserved): live for any node with weight < 1 |
| 0xD00 / 0xD04 | `character == -1` -> 0xD14 | 17 / 0 | **open** (unobserved): live for any node with character >= 0 |
| 0xD0C / 0xD10 | `character == animData.mCharIdx` -> 0xF78 | 0 / 0 | **open** with 0xD04: the asm skips this compare while character == -1 |
| 0xD14 / 0xD18 | `node.GetValue() == kUE_ObjectDrop (26)` -> 0xF78 | 17 / 0 | **open** (unobserved): live for any node with value != 26, including the default value-0 nodes |
| 0xF80 / 0xF84 | loop `i < count` -> 0xCE0 | 17 / 17 | both ran |
| 0xE14 | retail `grenade == nullptr` -> 0xF18 | - | no host branch (operator new cannot return null here; cpp:312 has no gcov arm) |
| EH | landing pads of the string / CModelData / CActorParameters temporaries (cpp:310, 311, 313, 314) | 0 | throw edges, excluded by tools/cov/gcov_report.py |

Marker: **removed** (`// @ fn_49_C6C` is gone; the only source change of this pass is reverted). Evidence
kept in `/tmp/opencode/psc6_evidence`: both runs' `.gcda` (89b16ac0...bb5a, dde0cc47...136b) with the
matching `.gcno` 3d906014...c67b (one build served both), run-1 source copy (ee9a078a...c3e3), binary
78dd2b18...fb38, raw gcov JSON, per-line edge extracts and the per-run and merged reports. What would
close the six arms: log each model's own POI list right after *its* advance (so the buffer writer is known),
and dump the shipped POI nodes of the base, into-attack (#2) and shared attack (#1) animations for all
three placements.

## Eighth pass (psc7, 2026-10-24, base f024872f3)

One launch (of the slice's two), `3dfd2249,0g_swamp_hall`, 1200 frames, fast,
`[port] caught/reach-stub: 0`, `[Inferior 1 (process N) exited normally]`, gcov flushed. Fixture
`tools/cov/specs/psc7_grenade_fields.py` (spec + gdb side in one lane file), driver reused:
`psc6_grenade_0g.con`.

    BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/psc7_grenade_fields.py tools/cov/force.sh \
      tools/cov/specs/psc7_grenade_fields.py /tmp/opencode/psc7_r1 3dfd2249,0g_swamp_hall \
      --con tools/cov/specs/psc6_grenade_0g.con --fast --frames 1200
    python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/psc7_r1/gcov --module PlantScarabSwarm

Profile: SpawnGrenades **29/29 lines, 26/26 edges, ALL-ARMS**.

Fixture, and what its numbers mean: on a node-bearing call only - a claimed slot whose boid is active,
`animData` non-null and 0 < mPassedIntCount <= 16 (a slot is only ever claimed by a boid that passed
Think's proximity, timer, space-above and angle gates, cpp:107-110) - it snapshots the RAW words of the
first node the body will walk (mWeight, mCharIdx, mVal at absolute addresses derived from the static
`CAnimData::mInt32POINodes` plus the CInt32POINode layout: 0x3f800000 / 0xffffffff / 0x1a), installs a
FinishBreakpoint *before* any write, then cycles one phase per call and writes back the ORIGINAL words at
the body's return, reading them back to prove it. Count, pointer, node type, name, slot and object
ownership are never touched, and the RNG is never forced.

    calls=2928 node_calls=11 writes=9 restored=9 mismatch=0 reentrant=0
    weight0:        targets 0x00000000/0xffffffff/0x1a   (x3)
    equal:          targets 0x3f800000/animChar/0x1a     (x2)
    unequal:        targets 0x3f800000/1/0x1a            (x2, nonnegative character != animChar)
    nonobjectdrop:  targets 0x3f800000/0xffffffff/0x0    (x2)
    natural:        no writes                            (x2)
    every restore: "restored ORIGINAL words [0x3f800000, 0xffffffff, 0x1a] ... (read back
    [0x3f800000, 0xffffffff, 0x1a])", with no "writer changed the words" note and no reentrancy.

Edges (merged nothing: one run; forced arms are labelled **forced node-field fixture**, not shipped-asset
behaviour):

| arm (retail asm) | condition | edges | status |
|---|---|---|---|
| 0xCAC / 0xCB0 | `boidIndex == -1` -> 0xF88 | 704 / 2224 | ran (natural) |
| 0xCC0 / 0xCC8 | `count <= 0` -> 0xF88 | 11 / 693 | ran (natural) |
| 0xCE4..0xCF4 | `Float() <= node.GetWeight()` -> 0xF78 | 8 / **3** | both ran: false side = 3 `weight0` forced calls |
| 0xD00 / 0xD04 | `character == -1` -> 0xD14 | 4 / **4** | both ran: 4 natural/weight0/nonobjectdrop nodes vs 4 forced equal/unequal nodes |
| 0xD0C / 0xD10 | `character == animData.mCharIdx` -> 0xF78 | 4/4 and 2/2 | both ways ran (forced equal vs forced unequal) |
| 0xD14 / 0xD18 | `node.GetValue() == kUE_ObjectDrop (26)` -> 0xF78 | 4 / **2** | both ran: false side = 2 `nonobjectdrop` forced calls |
| 0xF80 / 0xF84 | loop `i < count` -> 0xCE0 | 11 / 11 | ran |
| 0xCCC / 0xCD0 | `nodes == nullptr` -> 0xF88 | - | still dead by construction (no gcov edge here): `GetInt32POIList` returns `mInt32POINodes.data()`, the address of the static byte array (CAnimData.cpp:40) |
| 0xE14 | retail `grenade == nullptr` -> 0xF18 | - | no host branch (operator new cannot return null here) |
| EH | temporaries' landing pads (cpp:310, 311, 313, 314) | 0 | throw edges, excluded by tools/cov/gcov_report.py |

The `weight0` phase also shows the RNG was non-zero on those calls (all three took the false side), so
`Float() <= 1.0` keeps passing on the natural node exactly as the inclusive-[0,1] bound says.

Marker: **added** `// @ fn_49_C6C` (cpp:288). Evidence archived before that edit and any rebuild, in
`/tmp/opencode/psc7_evidence`: `psc7_r1.gcda` 916c1157...21f7, matching `psc7_r1.matching.gcno`
3d906014...c67b, run source `CPlantScarabSwarm.r1-source.cpp` ee9a078a...c3e3 (identical to the base),
binary d22c4c0f...3f93, `psc7_r1_raw.gcov.json`, `psc7_r1_spawngrenades_edges.txt`, the fixture log and
counters, and the report file. The psc6 read-only sample (`/tmp/opencode/psc6_evidence`) is kept as the
natural-call observation; this pass supersedes its conclusions with the direct measurement.
