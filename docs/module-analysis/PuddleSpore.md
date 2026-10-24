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

All of the bodies marked before pud1 have every live arm taken. GetUp and Attack miss only the switch default.
EStateMsg has three values, so that default is dead.

## Arms after pud1 (2026-10-24, base 4ff52d8e9)

Second pass over the 11 behaviour bodies ps1 left unmarked. Drivers: `puddlespore_pud1.con` (TurnAround at five
positions around the spore, one morphed and one with the spore on the player, then Run/Attack/GetUp and the two
script messages) and `puddlespore_pud1c.con` (a long Run window). Force specs: `pud1_force.py`, `pud1_force2.py`.
Six dirs merge with

    python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/pud1_{d,e,f,g,h,i}/gcov --module PuddleSpore

| run | driver | force |
|---|---|---|
| d | puddlespore_pud1.con (first version) | none |
| f | puddlespore_pud1.con (first version) | pud1_force.py |
| e | puddlespore_pud1.con | none |
| g | puddlespore_pud1.con | pud1_force.py |
| h | puddlespore_morph.con | none (morphed contact: ShouldTurn's morph arm) |
| i | puddlespore_pud1c.con | pud1_force2.py (mAnimPhase 1 then 2) |

| function | arms | evidence / dead edge |
|---|---|---|
| 2FB0 DoUserAnimEvent | ALL-ARMS | no-particle arm `forced (pud1_force.py: mShockWaveEffect = kInvalidAssetId)`; 0x1d fixed, see below |
| 330C AcceptScriptMsg | ALL-ARMS | null-manager arms `forced (pud1_force.py: mCollisionActorManager.mPtr = 0)` at the AIUpdateDisabled (0x340C) and Delete (0x3424) entries; Activate/Deactivate came from `send` (0x33F4/0x3400) |
| 1880 ShouldTurn | ALL-ARMS | the morph arm (0x1934 cmpwi 1) needs a morphed player over the box: the morph driver, in Run |
| 1574 Run | 9/11 → dead | phase-1 body `forced (pud1_force2.py)`; edge 559:3/4 dead: the msg switch only reaches 0x15AC (0), 0x15D8 (1) and 0x16A0 (2), and EStateMsg has three values (StateMachineCommon.hpp:4) |
| 135C TurnAround | 7/8 → dead | CanIssueCommand false arm `forced (pud1_force.py: mAnimationState kAS_NotReady)`; edge 592:3/4 dead, same three-value msg switch (0x1364/0x1380/0x138C) |
| 2B98 KnockOffPlayers | 27/28 → dead | edge 289:1/2 dead: 0x2BF0 (`x838 >= player min.z`) implies min.z <= max.z, so 0x2C54 (`min.z - max.z >= 0.2f`) is never taken |
| 19C4 SetupCollisionManager | 42/43 → dead | edge 475:3/4 (the TCastToPtr null at 0x1D24) dead: those ids are the manager's own, one `rs_new CCollisionActor` per joint in the call that fills them (CCollisionActorManager.cpp:121-196) |
| 2DA4 WeaponHitsWeakSpot | 9/10 | 248:3/4 open (0x2DF8: WeaponHits false while open — a weapon mode the open vulnerability rejects; power, charge and missile all hit) |
| 23F0 UpdatePlayerContact | 35/42 | 311:1/2, 338:1/2 (cast null, as 19C4), 341:1/2 (no touch bounds), 353/358/382 (hasGroundCollider false: the player always has kMT_GroundCollider here), 370:1/2 (zero-length push direction) |
| 34E0 Think | 15/18 | 97:1/2 and 99:1/6 need mShockWaveSfx == 0, but `CSfxHandle(0)` makes mID `(++mRefCount << 14) | 0` (CSfxHandle.cpp:5), never 0; 104:1/2 needs the player exactly at the wave's x/y (0x3678 CanBeNormalized) |
| 42C LoadPuddleSpore | 12/16 | 670:1/2 (modelData null: the shipped model always loads), 678/692 edges (temporaries' exception clean-up) |

Fixes in this pass (TARGET_PC only):

- DoUserAnimEvent case 0x1d: retail does not set `handled` — 0x3190..0x31C4 (SendScriptMsgs, x85c bit 0x20 clear)
  leave r30 (handled) at 0, so 0x31C8 clrlwi./bne does not skip and 0x31E4 calls CPatterned::DoUserAnimEvent.
- LoadPuddleSpore: retail passes the loaded flavor word through (0x864 lwz r6,0x4d8(r1), no compare before
  0x868 bl fn_51_37D4); upstream narrows it to 0/1.

Not driven arm by arm: the SLdr ctor and dtor, `__sinit`, and the weak copies (H).

## Smoke family (proposal)

`spor`: area `3dfd2249,cbc72d79` (0d_swamp_hall_dark), driver `tools/cov/specs/puddlespore.con`, about 1900 frames
fast. Expect `objs PuddleSpore` to list e00150015, then nothing after Delete. The pass condition is 0
`[port] caught` / `reach-stub`.
