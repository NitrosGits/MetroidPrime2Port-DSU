# PuddleSpore REL (module 51): analysis notes

CPuddleSpore (SPOR, "Morphology: Phlogus") is a stationary Torvus Bog spore. While open it shoots a shock wave
(DoUserAnimEvent) and pulls a standing or morphed player along (UpdatePlayerContact, KnockOffPlayers).
A hit on the weak spot flips it, a charged shot or a missile for example. It then runs Run → TurnAround → GetUp
and closes. The module's code is upstream's `src/MetroidPrime/Enemies/CPuddleSpore.cpp`, with the loader
`include/MetroidPrime/ScriptLoader/SLdrPuddleSpore.hpp`. Retail asm:
`build/G2ME01/PuddleSpore/asm/MetroidPrime/Enemies/CPuddleSpore.s` (91 fns) plus `REL/REL_Setup.s` (5). Per-function
table: `PuddleSpore_functions.tsv` (B upstream body, H weak/template copy, T REL runtime).

The driver area is Torvus Bog `0d_swamp_hall_dark` (world 3dfd2249, area cbc72d79). It has one spore, e00150015.
The spore also spawns in `03_swamp_forgottenbridge_dark` (layer 11).

## Survey (ps1, 2026-10-23)

| function | verdict |
|---|---|
| SLdrPuddleSpore ctor (fn_51_E60) | **fixed** (TARGET_PC, SLdrPuddleSpore.hpp:52): retail sets three defaults that upstream left at the base values. ShockWaveTravelSound = -1 (0xE88/0xE90 `stw 0x31c`); ambient (1,1,1,1) (0xF34/0xF48 `stw 0x298`); visorFlags 0xf (0xF4C/0xF58 `stw 0x2e8`). This ctor has no knockback-resistance or echo-emitter default |
| DoUserAnimEvent (fn_51_2FB0) | **fixed** (TARGET_PC, cpp:189-199): retail closes the spore even without a shock wave. A missing particle (0x3014 `beq .L_00003178`) and a failed alloc (0x3130 `beq .L_00003178`) both reach 0x3184 `bl SetState` (kPS_Closed). Upstream only closed after spawning the wave |
| UpdatePlayerContact (fn_51_23F0) | **fixed** (TARGET_PC, cpp:346, 371): retail removes and adds `kMT_Player` without a condition (0x2810-0x2830, 0x2924-0x2944). Only `kMT_GroundCollider` depends on the contact state |
| every other body | equivalent to the asm (asmcheck) |

The 0x85c bitfields now carry their positions in the header (MWCC MSB-first). The three bits are
mStateTimerRunning 0x80, mSecondaryTimerRunning 0x40 and mOpen 0x20. The ctor's `rlwimi` at 0x390C, 0x3918
and 0x3924 confirms them.

asmcheck (`python3 tools/asmcheck.py check PuddleSpore src/MetroidPrime/Enemies/CPuddleSpore.cpp`) has 6 findings
left. They are all noise:

- ctor 0x3930 / 0x3944 BIT: these are the inlined `SetHurlVelocityEnabled(false)` (knockback mgr 0x620 + 0x90, bit
  0x40) and `EnableBurn(false)` (0x620 + 0x61, bit 0x10).
- ctor 0x3868 CALL fn_51_39C8: this is the `SPuddleSporeData` copy ctor (`mData(data)`).
- GetDamageVulnerability 0x2D44: the asm reloc names `ReflectVulnerabilty`, as the C++ does. asmcheck reports the
  aliased name.
- GetUp 0x1254 / Attack 0x1010: these are out-of-line `CTryCommand::ShouldRetry` calls behind the inline header call.
- AcceptScriptMsg NOPOS on mHitByPlayerProjectile: a base-class bit. The body stays unmarked anyway.

## Coverage (ps1)

The spec is `tools/cov/specs/puddlespore.py`. Run each driver, then merge the gcov dirs:

    for d in puddlespore puddlespore_msgs puddlespore_morph; do
      BUILD=$PWD/build-port-cov SECS=1500 timeout 1600 tools/cov/run.sh tools/cov/specs/puddlespore.py /tmp/opencode/ps1_$d \
        3dfd2249,cbc72d79 --con tools/cov/specs/$d.con --fast --frames 2700; done
    python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/ps1_*/gcov --module PuddleSpore

What each driver does:

- `puddlespore.con` (1900 frames) adds god mode, morph ball and missiles. It places the player on the spore and forces
  InActive, Attack, Run, TurnAround and GetUp. Teleports hit the contact box from each side and from above, standing
  and morphed. Then it fires shots (power, charged, missile) and sends Delete.
- `_msgs` runs Deactivate, Activate, AIUpdateDisabled and Delete, plus damage in Run and Attack.
- `_morph` covers morphed contact in InActive and GetUp, and damage with and without a weak-spot hit.

The merged report also counted a fourth run, a plain place-and-observe run. Its driver was not kept.

All 23 marked bodies have every live arm taken. GetUp and Attack miss only the switch default. EStateMsg has three
values, so that default is dead.

Open arms (cpp line), so these bodies stay unmarked:

| function | open arms |
|---|---|
| Think 34E0 | 97, 99, 104 |
| AcceptScriptMsg 330C | 136, 141 (null collision manager) |
| KnockBack 32B0 | 164 |
| DoUserAnimEvent 2FB0 | 176 (no shock-wave particle) |
| Touch 2EE4 | 225, 227 |
| WeaponHitsWeakSpot 2DA4 | 240, 242, 243 |
| KnockOffPlayers 2B98 | 281 |
| UpdatePlayerContact 23F0 | 303, 330, 333, 345, 350, 362, 374 |
| SetCollisionActive 23C4 | 396 |
| SetupCollisionManager 19C4 | 466 (3/4) |
| ShouldTurn 1880 | 496-501 |
| Run 1574 | 558; 568 0/2 (the phase-1 Update never ran: the spore left Run first) |
| TurnAround 135C | 597 |
| LoadPuddleSpore 42C | 661, 669, 676 |

Not driven arm by arm: the SLdr ctor and dtor, `__sinit`, and the weak copies (H).

## Smoke family (proposal)

`spor`: area `3dfd2249,cbc72d79` (0d_swamp_hall_dark), driver `tools/cov/specs/puddlespore.con`, about 1900 frames
fast. Expect `objs PuddleSpore` to list e00150015, then nothing after Delete. The pass condition is 0
`[port] caught` / `reach-stub`.
