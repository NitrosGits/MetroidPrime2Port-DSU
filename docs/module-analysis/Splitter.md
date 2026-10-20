# Splitter REL (module 75): `CSplitterMainChassis` (SPLL) / `CSplitterCommandModule` (SPLU)

Host-port analysis, dated 2026-10-20. **Milestone 1 only**: the actor loads, registers and thinks.
The FSM, the beam-effect actor and the command module are not ported yet.

The Splitter is the Sanctuary Fortress "Quad" robot: a **body** (main chassis, script type `SPLL`)
that carries a detachable **head** (command module, `SPLU`). The body has legs, a spin attack and a
leg stab; the head scans, fires the laser pulse/sweep, raises the light/dark shields and docks back
into the body. The Sanctuary Guardian (module 14) deploys and destroys these robots.

**Sources (read-only):**

- m2c pseudo-C, one file per function: `/tmp/opencode/pseudo/Splitter/` (307 `.c` files, named
  `fn_75_<hexoff>`, plus `RELMain.c`, `RELExit.c`, `_prolog.c`, `_epilog.c`).
- function inventory: `config/G2ME01/rels/Splitter/symbols.txt` (670 lines, 300 `fn_75_*` entries
  with `.text` offset and size).
- section dumps: `build/G2ME01/Splitter/asm/auto_04_00000000_data.s` (.data, 0xEB0 bytes: vtables,
  FSM name tables, `CMemory` vtables), `auto_03_00000000_rodata.s` (.rodata, 0x890 bytes: FSM names,
  model names), `auto_05_00000000_bss.s`, `auto_00_00000000_text.s` / `_000081F8` / `_00010778`
  (the three .text sections).
- upstream loader structs: `include/MetroidPrime/ScriptLoader/SLdrSplitterMainChassis.hpp`,
  `SLdrSplitterCommandModule.hpp`.
- the port's existing surface: `src/MetroidPrime/Enemies/SSplitterMainChassis_FuncPtrs.cpp`,
  `include/MetroidPrime/Enemies/CSplitterMainChassis.hpp`,
  `include/MetroidPrime/ScriptLoaderRel.hpp` lines 237-244.

## 0. Facts already established (do not re-derive)

- Module id **75**, `Splitter`, **72804** text bytes, **307** functions (`docs/modules.tsv` line 76).
  Script types **SPLL** and **SPLU**; in-game names `Mechanism: Quad MB` (main body) and
  `Mechanism: Quad CM` (command module).
- Class names are confirmed by the DOL's cast helpers the REL calls:
  `TCastToPtr<20CSplitterMainChassis>__FP7CEntity` and
  `TCastToPtr<22CSplitterCommandModule>__FP7CEntity` (`fn_75_58E8`, `fn_75_23F0`).
- DOL symbols (`config/G2ME01/symbols.txt` lines 9495-9498):
  `AutoDestruct__20CSplitterMainChassisFf` @0x80218C6C,
  `RelProxy_LoadSplitterCommandModule` @0x80218C98,
  `RelProxy_LoadSplitterMainChassis` @0x80218CC4,
  `SetSSplitterMainChassis_FuncPtrs__FP29SSplitterMainChassis_FuncPtrs` @0x80218CF0.
- `RELMain` = `.text:0x8234` (size 0x20) calls `fn_75_8254` (0x8254, size 0x5C).
  `RELExit` = `.text:0x8210` (size 0x24) calls `SetSSplitterMainChassis_FuncPtrs(0)`.
- `fn_75_8254` fills the 5-word table `.bss:0x20` (`lbl_75_bss_20`, 0x18 bytes) with
  `fn_75_82B0`, `fn_75_FC`, `0x00000000`, `0xFFFFFFFF`, `fn_75_5A54` and registers it with
  `SetSSplitterMainChassis_FuncPtrs` ([0x0000827C]..[0x0000829C]). The last three words are copied
  verbatim from `.data:0xCAC` (`lbl_75_data_CAC` = {0, 0xFFFFFFFF, fn_75_5A54}).
  **Loader 1 = `fn_75_82B0` (main chassis), loader 2 = `fn_75_FC` (command module).**
- Spawn areas: 9 Sanctuary Fortress areas, 10 spawn entries (`docs/module_spawns.tsv` lines 966-975).
- The port's `SSplitterMainChassis_FuncPtrs` = { `mLoadMainChassis`, `mLoadCommandModule`,
  `mAutoDestruct` } and the two proxies
  (`RelProxy_LoadSplitterMainChassis`, `RelProxy_LoadSplitterCommandModule`) exist but the module's
  table is never registered, so the proxies return null today.
- The retail source file name of every class is obfuscated: `operator new` is called with the file
  string `"??(??)"` (rodata 0x2C9 for the SPLL/SPLU classes, rodata 0x794 for the beam actor).

## a. Class inventory

| class | script type | guest size | vtable (.data) | ctor | base | dtor (slot 0x08) |
|---|---|---|---|---|---|---|
| `CSplitterCommandModule` | SPLU | **0xF70** (3952) | **0x448** (82 words) | `fn_75_6B08` (0x6B08, 0x414) | `CPatterned` (0x7C0) | `fn_75_6798` → body `fn_75_6890` |
| `CSplitterMainChassis` | SPLL | **0xD48** (3400) | **0xCB8** (82 words) | `fn_75_F5F8` (0xF5F8, 0x538) | `CPatterned` (0x7C0) | `fn_75_F560` (0xF560, 0x98) |
| beam-effect actor (name unknown; model string "Splitter Beam Effect") | – | **0x1A0** (416) | **0xE34** (31 words) | `fn_75_11884` (0x11884, 0x208) | `CActor` (0x198) | `fn_75_11804` (0x11804, 0x80) |

Evidence for the sizes (`li r3, N` immediately before `bl __nw__FUlPCcPCc`):

| size | allocation site | class |
|---|---|---|
| 0xF70 | [0x00000340] `li r3,0xf70` → [0x0000034C] `bl __nw__` in `fn_75_FC` | `CSplitterCommandModule` |
| 0xD48 | [0x000084F4] `li r3,0xd48` → [0x00008500] `bl __nw__` in `fn_75_82B0` | `CSplitterMainChassis` |
| 0x1A0 | [0x00001250] `li r3,0x1a0` → [0x00001258] `bl __nw__` in `fn_75_1188` | beam actor |
| 0x6A8 | [0x000019D0] `li r3,0x6a8` → [0x000019D8] `bl __nw__` in `fn_75_1974` | `CPlasmaProjectile` (DOL class, laser pulse shot) |

Other classes the module touches: the DOL's `CPlasmaProjectile` (0x6A8, allocated by the REL in
`fn_75_1974` — the laser-pulse shot) and the `CMemory` vtables in `.data` (12-byte entries
`{0, 0xFFFFFFFF, fn}` — small destructors, e.g. `.data:0x0` → `fn_75_5A40`, `.data:0xCAC` →
`fn_75_5A54`).

## b. FSM

Two independent state machines, one per class. Each registers **names only** — the tables are arrays
of `{const char*, 0, 0, 0}` (16 bytes/entry) — so the transitions live in the area script
([probable]; the REL has no transition data).

**FSM1 — command module** (initialiser `fn_75_5E8C`, vtable slot 0xF0 `SetupStateMachine`):
`(*mgr)->unk18(mgr, &lbl_75_data_CC, 0x11)` = 17 triggers, `unk14(mgr, &lbl_75_data_284, 0x0E)` =
14 states, `unk1C(mgr, &lbl_75_data_3B8, 7)` = 7 codes.

| kind | index: name |
|---|---|
| trigger | 0 StateOver · 1 InDetectionRange · 2 IsScanning · 3 IsInitiallyDocked · 4 IsDocked · 5 HasDockingPath · 6 HasDockingTarget · 7 DockingPathOver · 8 HasTarget · 9 InHoverRange · 10 InLaserPulseRange · 11 ShouldDodge · 12 ShouldAttack · 13 ShouldFireAgain · 14 ShouldLaserSweep · 15 PathOver · 16 PathShagged |
| state | 0 Start · 1 Idle · 2 SpawnIdle · 3 Dead · 4 Scanning · 5 FaceTarget · 6 PathFind · 7 Hover · 8 Dodge · 9 LaserPulse · 10 LaserSweep · 11 LostChassisReaction · 12 SeekMainChassis · 13 FollowDockingPath |
| code | 0 NotifyDocking · 1 SelectTarget · 2 SetTargetDest · 3 SetDockingDest · 4 FindBestDodgeDirection · 5 RaiseShields · 6 ResetAttackTimes |

**FSM2 — main chassis** (initialiser `fn_75_E9E4`, vtable slot 0xF0 `SetupStateMachine`):
`(*mgr)->unk18(mgr, &lbl_75_data_720, 0x1C)` = 28 triggers, `unk14(mgr, &lbl_75_data_9E8, 0x16)` =
22 states, `unk1C(mgr, &lbl_75_data_BCC, 0x0B)` = 11 codes.

| kind | index: name |
|---|---|
| trigger | 0 StateOver · 1 IsIntact · 2 IsDisabled · 3 ShouldWaitForSnatch · 4 ShotAt · 5 LostHead · 6 HasHead · 7 HasRetreatPoint · 8 HasTarget · 9 CanReachTarget · 10 HasLineOfSight · 11 PathShagged · 12 PathOver · 13 PatrolPathOver · 14 ShouldLegStab · 15 ShouldMorphballStab · 16 ShouldSpinAttack · 17 ShouldLaserSweep · 18 ShouldDodge · 19 IsSpinAttackOver · 20 SpunIntoPlayer · 21 SpunIntoGeometry · 22 BreakOutOfSpin · 23 TooManyBounces · 24 DeployFromHoldingTube · 25 ShouldDeploy · 26 DeployPathOver · 27 PrepareForDocking |
| state | 0 Start · 1 Idle · 2 Inactive · 3 Activate · 4 Patrol · 5 Scanning · 6 PathFind · 7 LegStabAttack · 8 SpinAttack · 9 Dodge · 10 LaserSweepAttack · 11 SpinCollisionReaction · 12 SpinBounce · 13 SpinTelegraph · 14 SpinToIdle · 15 SpawnDrop · 16 HoldingTube · 17 WaitForDocking · 18 WaitForSnatch · 19 FollowDeployPath · 20 DeploymentLanding · 21 HeadExplosion |
| code | 0 SelectTarget · 1 SetTargetDest · 2 SetRetreatDest · 3 SetupSpinAttack · 4 EndSpinAttack · 5 SetupLaserSweep · 6 SetMorphballStabLeg · 7 FindBestStabLeg · 8 FindBestDodgeDirection · 9 BounceOffPlayer · 10 BounceOffGeometry |

Names are the rodata strings: FSM1 triggers at rodata 0xA0..0x17D, states 0x189..0x206, codes
0x218..0x274; FSM2 triggers 0x3E4..0x55B, states 0x56D..0x663, codes 0x671..0x714.

## c. Parameters

### SPLL — `SLdrSplitterMainChassisData`, property id **0x15E03A2F**, loaded by `fn_75_8714`

| loader offset | field (default) | object offset |
|---|---|---|
| 0x00 | int `unknown_0xcef5c2fe` (124; bit flags, bits 0/4/6 used) | 0x828 |
| 0x04-0x0C | `legStabAttackInterval` 2.0 / `legStabMinAttackRange` 2.5 / `legStabMaxAttackRange` 6.0 | 0x82C-0x834 |
| 0x10 | `SLdrDamageInfo legStabDamage` (0x10): type 11, dmg 20, kb 5 | 0x7C0 (converted) + 0x838 (raw) |
| 0x20-0x2C | `minDodgeInterval` 3.0 / `dodgeChance` 100 / `deploymentSpeed` 40 / `scanDuration` 3.0 | 0x848-0x854 |
| 0x30-0x38 | `laserSweepInterval` 6.0 / `laserSweepMinAttackRange` 30 / `laserSweepMaxAttackRange` 60 | 0x858-0x860 |
| 0x3C-0x50 | `spinAttackLinearVelocity` 20 / `…Acceleration` 20 / `…Deceleration` 60 / `spinAttackTurnSpeed` 20 / `spinAttackInterval` 6.0 / `spinAttackIntervalPenalty` 6.0 | 0x864-0x878 |
| 0x54-0x5C | `spinAttackTelegraphTime` 1.5 / `spinAttackMinAttackRange` 10 / `spinAttackMaxAttackRange` 30 | 0x87C-0x884 |
| 0x60 | int `unknown_0xd5f34476` (4) | 0x888 |
| 0x64 | f32 `spinAttackMaxTime` (30) | 0x88C |
| 0x68 | `SLdrDamageInfo spinAttackDamage` (0x10): type 11, dmg 20, kb 5 | 0x7DC (converted) + 0x890 (raw) |
| 0x78 | int `sound_Alerted` (0) | 0x8A0 |
| 0x7C | `SLdrIngPossessionData ingPossessionData` (0x17C) | 0x8A4 |
| 0x1F8 | `SLdrDamageVulnerability spinAttackVulnerability` (0x30) | 0x7F8 (converted) + 0xA20 (raw) |

Chain: loader struct → runtime struct (`fn_75_FD58`: `legStabDamage` at +0x00, `spinAttackDamage` at
+0x1C, vulnerability at +0x38, then `fn_75_FC10` copies loader+0x00..0x7B to runtime+0x68..0xE3,
`fn_75_77C0` the possession data to +0xE4, `fn_75_7504` the vulnerability to +0x260) → object
(`fn_75_FB30(&this+0x7C0, runtime)`, then `fn_75_FC10(this+0x828, runtime+0x68)`).

### SPLU — `SLdrSplitterCommandModuleData`, property id **0x560CF813**, loaded by `fn_75_970`

| loader offset | field (default) | object offset |
|---|---|---|
| 0x00 | int `unknown_0xbd80fd94` (10) | 0x7C0 |
| 0x04-0x18 | `maxLinearVelocity` 20 / `maxTurnSpeed` 720 / `scanningTurnSpeed` 20 / `minLaserPulseAttackTime` 4 / `minLaserPulseRange` 10 / `maxLaserPulseRange` 20 | 0x7C4-0x7D8 |
| 0x1C | int `maxLaserPulseShots` (3) | 0x7DC |
| 0x20 | `CAssetId laserPulseProjectile` (invalid) | 0x7E0 |
| 0x24 | `SLdrDamageInfo laserPulseDamage` (0x10): type 11, dmg 10 | 0x7E4 (raw) + 0xCD4 (converted) |
| 0x34-0x38 | int `minDodges` (1) / int `maxDodges` (3) | 0x7F4-0x7F8 |
| 0x3C-0x48 | `dodgeChance` 100 / `resetShieldTime` 10 / `shieldHP` 100 / `laserSweepTurnSpeed` 45 | 0x7FC-0x808 |
| 0x4C | `SLdrDamageInfo laserSweepDamage` (0x10): type 11, dmg 10 | 0x80C (raw) + 0xCF0 (converted) |
| 0x5C | `SLdrPlasmaBeamInfo laserSweepBeamInfo` (0x48): len 500, exp 4, life 1, pulse 20, shut 0.25 | 0x81C |
| 0xA4 | `SLdrUnknownStruct42` (0x20): angle 20, 4 colours, scale/fade/open speed | 0x864 (angle), 0x868-0x880 |
| 0xC4-0xDC | the seven sound ids `sound_LaserSweep`, `sound_LaserChargeUp`, `sound_Docking`, `sound_Scanning`, `sound_LightShield`, `sound_DarkShield`, `sound_ShieldOn` (all 0) | 0x884-0x89C |
| 0xE0 | `SLdrIngPossessionData ingPossessionData` (0x17C) | 0x8A0 |
| 0x25C | `SLdrDamageVulnerability lightShieldVulnerability` (0x30) | 0xA1C (raw) + 0xD0C (converted) |
| 0x3B8 | `SLdrDamageVulnerability darkShieldVulnerability` (0x30) | 0xA4C (raw) + 0xD3C (converted) |

Chain: loader → runtime (`fn_75_791C`: the DamageInfos at +0x514/+0x530, the vulnerabilities at
+0x54C/+0x57C; `fn_75_73A0` copies loader+0x00..0x5B 1:1 to runtime+0x00) → object
(`fn_75_72BC(&this+0x7C0, runtime)`).

**Fields the loaders do not copy**: `SLdrEditorProperties`, `SLdrPatternedAITypedef` and
`SLdrActorParameters` are read by both loaders and handed to the DOL helpers
(`LdrToEntityInfo`, `LdrToTransform4f`, `LdrToActorParameters`, `LdrToPatternedInfo`); the patterned
block also carries `turnSpeed = 60`, `minAttackRange = 12`, `maxAttackRange = 37`,
`health.health = 150`, `hI_KnockBackResistance = 2`, `creatureSize = 2`.

**Recommended host structs**: one POD per script type, laid out exactly like the loader structs
(`SLdrSplitterMainChassisData`, `SLdrSplitterCommandModuleData` — both already exist upstream and
are usable as-is), plus a `CSplitterParams` POD that keeps the *converted* forms
(`CDamageInfo` ×2, `CDamageVulnerability` ×1 for SPLL; ×2 + ×2 for SPLU). Do not keep the raw
copies: they duplicate the converted ones.

## d. Spawn and the run harness

| area (MLVL) | layer | area (MLVL) | layer |
|---|---|---|---|
| `02_cliff_reactorcore` | 3 | `09_cliff_temple` | 2 |
| `03_cliff_chicane` | 3 and 6 | `09_cliff_temple_dark` | 10 |
| `04_cliff_contraption` | 2 | `0g_cliff_hall` | 2 |
| `06_cliff_spiderballguardian` | 6 | `0n_cliff_hall` | 2 |
| | | `0q_cliff_hall` | 0 |

- Booting into one of them: `USERDIR=<dir holding the card> TELEPORT=0q_cliff_hall FRAMES=6000
  DISPLAY_NUM=:86 LOG=/tmp/opencode/splitter.log tools/run_gameplay.sh` (`tools/run_gameplay.sh`;
  `TELEPORT` takes an area MLVL name or hex id — `docs/RUNNING.md`). Add `GODMODE=1` so the robot
  is not shot out of the test, and `MP2_INPUT`/`INPUT_FILE` only if the run has to move.
- The console verb `layer <n>` (`platform/port_console.cpp:1812` → `CmdLayer` → `PortWarp::Layer`)
  activates a layer of the current area; `objs`/`obj <id>` list the world's actors
  (`platform/port_console.cpp` help text) and are how the spawned robot is found.
- **Easiest area: `0q_cliff_hall`** — the only Splitter spawn on **layer 0**, i.e. the layer that is
  live as soon as the area loads, so no `layer` command is needed. `09_cliff_temple_dark` (layer 10)
  is the next simplest if `0q_cliff_hall` has no reachable robot.
- A successful run shows: the log has no `[port] caught` and no `[reach-stub]`; the console `objs`
  lists the robot (uid `uXXXX`) with its class; `obj <uid>` reports its health (150) and the actor
  keeps thinking (its FSM state changes).

## e. Layout

`CSplitterMainChassis`, guest size **0xD48**, base `CPatterned` = **0x7C0** (the port's
`CHECK_SIZEOF(CPatterned, 0x7c0)`); the derived block starts at 0x7C0.

| offset | what | evidence |
|---|---|---|
| 0x7C0 | `CDamageInfo legStabDamage` (0x1C) | `fn_75_FB30` +0x00 ← runtime+0x00 (`fn_75_FD58`) |
| 0x7DC | `CDamageInfo spinAttackDamage` (0x1C) | `fn_75_FB30` +0x1C |
| 0x7F8 | `CDamageVulnerability spinAttackVulnerability` (0x30) | `fn_75_FB30` +0x38 → `fn_75_70A0` |
| 0x828 | int flags (`unknown_0xcef5c2fe`) | `fn_75_F5F8` (`unk68 & 1`, bits 4/6) |
| 0x82C-0x834 | leg-stab interval / min / max range | `fn_75_FC10` copy (loader+0x04..0x0C) |
| 0x838 | `SLdrDamageInfo legStabDamage` (raw) | `fn_75_FC10` copy (loader+0x10) |
| 0x848-0x88C | min-dodge interval/chance, deployment speed, scan duration, laser-sweep interval/min/max, the 13 spin-attack floats + `unknown_0xd5f34476`, spin-attack max time | `fn_75_FC10` copy (loader+0x20..0x64) |
| 0x890 | `SLdrDamageInfo spinAttackDamage` (raw) | `fn_75_FC10` copy (loader+0x68) |
| 0x8A0 | int `sound_Alerted` | `fn_75_FC10` copy (loader+0x78) |
| 0x8A4 | `SLdrIngPossessionData` (0x17C) | `fn_75_77C0` (loader+0x7C) |
| 0xA20 | `SLdrDamageVulnerability` (raw) | `fn_75_7504` (loader+0x1F8) |
| 0xA50-0xB7B | derived state (0x32C bytes) | **unknown** |
| 0xB7C | `CPathFindSearch` | `fn_75_F5F8` `__ct__15CPathFindSearch...(&arg0->unkB7C, …)` |
| 0xC68-0xC6E | u32 = 0, 2 × `TUniqueId` (invalid) | `fn_75_F5F8` |
| 0xC70 | `CVisualKnowledge` (0x40) | `fn_75_F5F8` `__ct__16CVisualKnowledge...(&arg0->unkC70, …)` |
| 0xCB0-0xCB8 | u32 (from the patterned block), s32 anim id (-1), f32 (1.0) | `fn_75_F5F8` (the anim id is later set by `FindBestAnimation`) |
| 0xCBC-0xCC6 | 5 × `TUniqueId` (invalid) | `fn_75_F5F8` |
| 0xCC8 / 0xCC9 | u8 (0xFF) pose/anim indices | `fn_75_F5F8` |
| 0xCCC-0xCFC | 4 × `CVector3f` (zero) | `fn_75_F5F8` |
| 0xD00-0xD08 | u32, s32 (-1), s32 (-1) | `fn_75_F5F8` |
| 0xD0C-0xD3C | f32 block (0xD0C = 1.0, 0xD30 = 0.2, 0xD34 = 1.0) | `fn_75_F5F8` |
| 0xD40 / 0xD41 | flag bytes | `fn_75_F5F8` (`unkD40`, `unkD41`) |

`CSplitterCommandModule`, guest size **0xF70**, derived block also starts at 0x7C0
(`fn_75_6B08` → `fn_75_72BC(&arg0->unk7C0, runtime)`):

| offset | what | evidence |
|---|---|---|
| 0x7C0-0x81B | the SPLU parameter block (see §c) | `fn_75_73A0` 1:1 copy |
| 0x81C | `SLdrPlasmaBeamInfo laserSweepBeamInfo` | loader+0x5C |
| 0x864-0x880 | `SLdrUnknownStruct42` (angle, colours, scale, fade, open speed) | loader+0xA4 |
| 0x884-0x89C | the seven sound ids | loader+0xC4..0xDC |
| 0x8A0 | `SLdrIngPossessionData` (0x17C) | `fn_75_77C0` (loader+0xE0) |
| 0xA1C / 0xA4C | raw light/dark shield vulnerabilities | `fn_75_7504` (loader+0x25C/+0x3B8) |
| 0xA7C-0xD6B | derived state (0x2F0 bytes) | **unknown** |
| 0xD6C | `CPathFindSearch` (3 roots) | `fn_75_6B08` `__ct__15CPathFindSearch...(&arg0->unkD6C, NULL, 3U, …)` |
| 0xE58 | u32 = 0 | `fn_75_6B08` |
| 0xE5C | `CProjectileInfo` (laser pulse) | `fn_75_6B08` `__ct__15CProjectileInfoFUiRC11CDamageInfo(&arg0->unkE5C, runtime+0x20, runtime+0x514)` |
| 0xE84 | `CProjectileInfo` (laser sweep) | `fn_75_6B08` (the second `CProjectileInfo`; its asset id is **unknown**) |
| 0xEAC | `CDamageVulnerability` (shield) | `fn_75_6B08` `fn_75_70A0(&arg0->unkEAC, …)` |
| 0xEDC-0xEE8 | counters | `fn_75_6B08` |
| 0xEF0 / 0xEF4 | auto-destruct delay / timer | `fn_75_5A54`, `fn_75_21FC` |
| 0xEF8 | u8 = 0xFF | `fn_75_6B08` |
| 0xEFA | `TUniqueId` of the **main chassis** | `fn_75_58E8` (`GetObjectById` + `TCastToPtr<CSplitterMainChassis>`) |
| 0xEFC | `TUniqueId` (second link) | `fn_75_31A8` |
| 0xEFE / 0xF00 / 0xF02 | `TUniqueId` (invalid; 0xF02 = the laser-pulse projectile id) | `fn_75_6B08`, `fn_75_1974` |
| 0xF04-0xF20 | `CColor` (white), floats, counters | `fn_75_6B08` |
| 0xF24-0xF2C | `CVector3f` from the editor properties | `fn_75_6B08` (`arg4->unk4/14/24`) |
| 0xF30-0xF5F | 2 × s16 (-1), 4 × `CVector3f` (zero) | `fn_75_6B08` |
| 0xF64 | `CSfxHandle` of the beam effect | `fn_75_1188` |
| 0xF68 | `TUniqueId` of the spawned beam effect | `fn_75_1188` |
| 0xF6A | flag byte (bit 1 = destruct timer running, bit 2 = destruct pending) | `fn_75_5A54`, `fn_75_21FC` |

## f. Vtable

Three vtables; all start with two zero words (offset-to-top + null RTTI), so slot *n* sits at byte
`4*n`. Slot names follow `docs/module-analysis/vtable-offsets.md`.

**`lbl_75_data_448`** — `CSplitterCommandModule`, 82 words (0x148), and
**`lbl_75_data_CB8`** — `CSplitterMainChassis`, 82 words (0x148). Local overrides only
(inherited slots keep the DOL symbol):

| slot | method | command module | main chassis |
|---|---|---|---|
| 0x08 | `~CEntity()` (deleting dtor) | `fn_75_6798` | `fn_75_F560` |
| 0x0C | `TypesMatch(int)` | `fn_8009AEFC` | `fn_8009AF34` |
| 0x10 | `PreThink(float, CStateManager&)` | inherited | `fn_75_F1A0` |
| 0x14 | `Think(float, CStateManager&)` | `fn_75_6490` | `fn_75_F0E8` |
| 0x18 | `AcceptScriptMsg(CStateManager&, const CScriptMsg&)` | `fn_75_654C` | `fn_75_F1C0` |
| 0x24 | `PreRender(CStateManager&)` | `fn_75_633C` | `fn_75_EF64` |
| 0x28 | `AddToRenderer(const CStateManager&) const` | `fn_75_6310` | `fn_75_EF44` |
| 0x2C | `Render(const CStateManager&) const` | `fn_75_6368` | `fn_75_EF84` |
| 0x3C / 0x48 / 0x84 | `GetHealthInfo` / `GetTouchBounds` / `CollidedWith` | `fn_75_D0` / `fn_75_18` / `fn_75_6178` | `fn_75_D0` / `fn_75_18` / `fn_75_ECA4` |
| 0x40 | `GetDamageVulnerability() const` | `fn_75_616C` | `fn_75_EF24` |
| 0x54 | `GetAimPosition(const CStateManager&, float) const` | `fn_75_5F8C` | inherited |
| 0x70 | `GetSortingBounds(const CStateManager&) const` | `fn_75_6394` | `fn_75_EFA4` |
| 0x98 | `Death(CStateManager&, const CVector3f&, EScriptObjectState)` | inherited | `fn_75_EB04` |
| 0x9C | `KnockBack(CStateManager&, const CKnockBackInfo&)` | `fn_75_6174` | `fn_75_EBF4` |
| 0xA8 / 0xC8 | `CanBeShot` / `GetModelAlphau8` | `fn_75_AC` / `fn_75_64` | `fn_75_AC` / `fn_75_64` |
| 0xAC | `IsListening() const` | `fn_75_10` | `fn_75_8200` (returns true) |
| 0xB0 | `Listen(CStateManager&, const CVector3f&, EListenNoiseType)` | `fn_75_5F18` | `fn_75_EA70` |
| 0xDC | `ProjectileInfo()` | `fn_75_8` (null) | `fn_75_8208` (null) |
| 0xE0 | `GetSearchPath()` | `fn_75_0` | `fn_75_81F8` (returns `this+0xB7C`) |
| 0xE4 | `GetPointSearchPath()` | `fn_75_6C` | `fn_75_6C` |
| 0xF0 | `SetupStateMachine(CStateManager&)` | `fn_75_5E8C` (FSM1) | `fn_75_E9E4` (FSM2) |
| 0xF4 | `GetRagDoll() const` | `fn_75_74` | inherited |
| 0xF8 | `CanBeIngPossessed(CStateManager&) const` | `fn_75_5E50` | `fn_75_E99C` |
| 0x100 | `SetIngPossessed(bool, CStateManager&)` | inherited | `fn_75_E90C` |
| 0x104 | `SetIngPossessed(bool, float, CStateManager&)` | inherited | `fn_75_E87C` |
| 0x108 | `SetAttackTarget(CStateManager&, TUniqueId)` | inherited | `fn_75_E810` |

All other slots are inherited DOL symbols (`SetActive`, `ClearFluidList`, `HealthInfo`,
`GetDamageVulnerability(pos,dir,dmg)`, `Touch`, `GetOrbitPosition`, `GetHomingPosition`,
`GetScannableObjectInfo`, `ProcessSoundEvent`, `GetCollisionPrimitive`, `GetPrimitiveTransform`,
`GetStepDownHeight`, `GetStepUpHeight`, `IsOnStaticGround`, `GetWeight`, `DamageVulnerability`,
`TakeDamage`, `GetOrigin`, `RenderSystemsToBeDrawnFirst/Last`, `Freeze`, `ThinkAboutMove`,
`Burn`, `Shock`, `MassiveDeath`, `MassiveFrozenDeath`, `GetContactDamage`,
`UpdateHitDamageTime`, `CanBeUnPossessed`, `GetAttackTarget`, `IsOnGround`, `GetGravityConstant`,
`IsScanVisorSelfRender`, `GetScanVisorRenderBounds`, `ScanVisorRender`, `GetDeathTimeScale`,
`TryToBeCaptured`, `IssueDeathBodyCommand`, `GetFadeOnDeathTime`, `GetIngSnatching*`).

**`lbl_75_data_E34`** — the beam-effect actor, 31 words (0x7C), `CActor`-based:
local overrides are 0x08 `fn_75_11804` (dtor), 0x14 `fn_75_10DC4` (Think), 0x24 `fn_75_10F1C`
(PreRender), 0x28 `fn_75_10EF0` (AddToRenderer), 0x2C `fn_75_10778` (Render), 0x34 `fn_75_1167C`
(PreRenderAllViewports), 0x3C `fn_75_D0` (GetHealthInfo).

## g. Function map

| range | what |
|---|---|
| 0x000-0x0B4 | 16 tiny functions (8-0x3C bytes): the shared inline virtuals (`GetTouchBounds` `fn_75_18`, `GetModelAlphau8` `fn_75_64`, `CanBeShot` `fn_75_AC`, `IsListening` `fn_75_10`, `GetAttackTarget` `fn_75_7C`, `IsOnGround` `fn_75_8C`, `GetGravityConstant` `fn_75_98`, `GetDeathExplosionParticle` `fn_75_A4`, `GetRagDoll` `fn_75_74`, `ProjectileInfo` `fn_75_8`, `GetSearchPath` `fn_75_0`, `GetPointSearchPath` `fn_75_6C`) |
| 0x0FC-0x4AB | **SPLU (command module) loader** `fn_75_FC` (0x370) + its helpers: `fn_75_46C` (copies the animation set into a `CModelData`), `fn_75_4AC`, `fn_75_4F4` (a `CToken` copy), `fn_75_630` (the `SLdrUnknownStruct42` loader, 0x1BC) |
| 0x4AC-0x5497 | behaviour bodies of the command module (its FSM states/triggers/codes); the exact split is **unknown** |
| 0x5498-0x5A9F | the destructors of the `CMemory` vtables that live in `.data` (e.g. `.data:0x0` → `fn_75_5A40`, `.data:0xCAC` → `fn_75_5A54` = the auto-destruct starter) |
| 0x5AA0-0x7BDB | command-module class: `SetupStateMachine` `fn_75_5E8C`, `Listen` `fn_75_5F18`, `GetAimPosition` `fn_75_5F8C` (0x1E0), `CollidedWith` `fn_75_6178`, `GetSortingBounds` `fn_75_6394`, `Think` `fn_75_6490`, `PreThink` `fn_75_654C` (0x24C), dtor `fn_75_6798`/`fn_75_6890`, `fn_75_6A74`, **ctor `fn_75_6B08`** (0x414); `LdrTo*` converters `fn_75_70A0`..`fn_75_7888`; SPLU runtime builder `fn_75_791C` |
| 0x7C80-0x81F7 | `fn_75_7C80` (0x578, jump table — own dump `auto_fn_75_7C80_text.s`): a big dispatch body; role **unknown** |
| 0x8200-0x824F | `fn_75_8200` (returns 1), `fn_75_8208` (returns 0): the main chassis' `IsListening` / `ProjectileInfo` |
| 0x8254-0x82AF | `fn_75_8254`: fills and registers the loader table (§0) |
| 0x82B0-0x8713 | **SPLL (main chassis) loader** `fn_75_82B0` (0x364) |
| 0x8714-0x91E3 | SPLL data loader `fn_75_8714` (0x460) + `LdrTo*` helpers `fn_75_8B74`, `fn_75_8BF0`, `fn_75_8D50`, `fn_75_8DF8`, `fn_75_8EC8` |
| 0x91E4-0xD7D3 | behaviour bodies of the main chassis (its FSM states/triggers/codes) — the largest cluster |
| 0xD7D4-0xFAFF | main-chassis class: `SetIngPossessed` `fn_75_E87C`/`fn_75_E90C`, `SetAttackTarget` `fn_75_E810`, `CanBeIngPossessed` `fn_75_E99C`, `SetupStateMachine` `fn_75_E9E4`, `Listen` `fn_75_EA70`, `Death` `fn_75_EB04`, `KnockBack` `fn_75_EBF4`, `CollidedWith` `fn_75_ECA4`, `GetSortingBounds` `fn_75_EFA4`, `PreThink` `fn_75_F1A0`, `AcceptScriptMsg` `fn_75_F1C0` (0x3A0), dtor `fn_75_F560`, **ctor `fn_75_F5F8`** (0x538) |
| 0xFB30-0xFE67 | the SPLL parameter plumbing: `fn_75_FB30` (runtime→object), `fn_75_FC10` (loader→runtime copy), `fn_75_FD58` (runtime builder) |
| 0xFE68-0x10777 | `fn_75_FED0` (0x8A8) and `fn_75_10778` (0x64C): large bodies; role **unknown** |
| 0x10778-0x11887 | the beam-effect actor: `fn_75_10778` (Render), `fn_75_10DC4` (Think), `fn_75_10F1C` (PreRender), `fn_75_10EF0`, `fn_75_1167C`, dtor `fn_75_11804`, ctor `fn_75_11884`, `fn_75_11A8C` (model data) |
| 0x1188-0x1577 | the beam-effect spawner `fn_75_1188` (0x344) + helpers `fn_75_14CC`, `fn_75_1524` |
| 0x1578-0x1E37 | the laser-pulse shot (`CPlasmaProjectile`): `fn_75_1578` (0x218), `fn_75_1790`, `fn_75_184C`, **spawner `fn_75_1974`** (0x298), `fn_75_1C0C` (beam info), `fn_75_1CA0` |

## i. Port plan

**WP1 — milestone 1 (the actor loads, registers and thinks)**

- New host headers `include/MetroidPrime/Enemies/CSplitterMainChassis.hpp` (extend the existing
  pointer-only one) and `CSplitterCommandModule.hpp`, both deriving from the host `CPatterned` so the
  base size stays 0x7C0; add `CHECK_SIZEOF(CSplitterMainChassis, 0xd48)` and
  `CHECK_SIZEOF(CSplitterCommandModule, 0xf70)`.
- New host bodies `src/MetroidPrime/Enemies/PortCSplitterMainChassis.cpp` and
  `PortCSplitterCommandModule.cpp` (one class per file, the port's convention).
- New/extended `src/MetroidPrime/Enemies/SSplitterMainChassis_FuncPtrs.cpp`: give it real
  `RELMain`/`RELExit` (the module today only gets the shared stub) that fill
  `SSplitterMainChassis_FuncPtrs` with { `mLoadMainChassis` → the SPLL loader,
  `mLoadCommandModule` → the SPLU loader, `mAutoDestruct` → the REL's `fn_75_5A54` body } and call
  `SetSSplitterMainChassis_FuncPtrs`; `RELExit` clears it. Keep the proxies' null guards.
- The two loaders read the `SLdrSplitter*` structs the upstream headers already provide, fill the
  parameter block (§c) and construct the host class.
- The overrides the REL adds (§f) forward to `CPatterned`'s implementation and log **once**
  ("stand-in log") the first time each is reached, so a run shows which retail behaviour is missing.
- Shared files that change: `tools/modules_port.txt` (point `Splitter` at the new file instead of
  `src/MetroidPrime/PortModuleStubs.cpp`), `files.cmake` (regenerate with
  `tools/gen_files_cmake.py`), `docs/modules.tsv` (status `port-body`).

**WP1 done (2026-10-20).** Both classes have a host body: `include/MetroidPrime/Enemies/CSplitterMainChassis.hpp`
(the upstream pointer-only header, grown under `#ifdef TARGET_PC`), the new
`include/MetroidPrime/Enemies/CSplitterCommandModule.hpp`, and the two files
`src/MetroidPrime/Enemies/PortCSplitterMainChassis.cpp` (the chassis, its loader and the module's
`RELMain`/`RELExit`) and `PortCSplitterCommandModule.cpp` (the head and its loader). The retail type
ids are **134** (SPLL) and **135** (SPLU) (`kET_SplitterMainChassis`, `kET_SplitterCommandModule`).
The base call is retail's: `EPatternedAI` **0x38** for the chassis (movement 0 / collider 1 / body 1)
and **0x39** for the head (movement 1 / collider 1 / body 7 - it flies), both with flavor 0. The
§f overrides that are retail's own bodies are real (`fn_75_18` touch bounds, `fn_75_64` alpha,
`fn_75_AC` `CanBeShot`, `fn_75_10`/`fn_75_8200` `IsListening`, `fn_75_81F8`/`fn_75_0` search path,
`fn_75_8` projectile info, `fn_75_EF24` the pass-through vulnerability, `fn_75_6174` the head's empty
`KnockBack`); the rest forward to `CPatterned` behind the once-only `[splitter] ... is a port
stand-in` log. The module's table is registered with {SPLL loader, SPLU loader, auto-destruct}; the
auto-destruct is a **documented stand-in** - `fn_75_5A54`'s body is still unknown (§j). The run:
`0q_cliff_hall` (layer 0, live on load) lists 8 objects - 5 `CSplitterMainChassis` and 3
`CSplitterCommandModule` - and they think; `09_cliff_temple_dark`'s layer 10 ('Boss Stage 2
Splitters') loads no robot, so the boss-stage actors come from the area script, not from the layer.

**WP2 — the FSM.** Implement `SetupStateMachine` for both classes with the §b tables (17/14/7 and
28/22/11 names) through the port's `CStateManager`/`SStateMachine2` path
(`src/MetroidPrime/Enemies/CGenericFSM2.cpp`, cf. `CSplinter::SetupStateMachineHelper`), then port
the state bodies in the order §g lists them.

**WP3 — the command module and the split-off halves.** Port `CSplitterCommandModule` (the head:
scanning, laser pulse/sweep, shields, docking), the beam-effect actor (0x1A0, `fn_75_1188` +
`fn_75_11804`/`fn_75_11884`) and the link between the two actors (`TUniqueId` at 0xEFA/0xEFC). The
Digital Guardian's `AllSplittersDead` / `StartSplitterDeployment` / `DestroyActiveSplitters`
triggers (module 14) are the integration test.

## j. Open

- The retail `SSplitterMainChassis_FuncPtrs` layout: the REL writes 5 words
  {loader, loader, 0, 0xFFFFFFFF, fn_75_5A54}, but the port's struct has 3 members and the DOL's
  `AutoDestruct__20CSplitterMainChassisFf` reads one of them. Which word the DOL calls, and what
  `fn_75_5A54` (which touches offsets up to 0xF6A, only valid for the **command module**) is for,
  is **unknown**.
- `fn_75_5A54` is called from `fn_75_21FC` (the command module's Think helper) — so the auto-destruct
  is the head's, not the body's. The body has no auto-destruct in the REL.
- `fn_75_7C80` (0x578, jump table) and `fn_75_FED0`/`fn_75_10778` (0x8A8/0x64C): roles **unknown**.
- The derived blocks 0xA50-0xB7B (body) and 0xA7C-0xD6B (head) are **unmapped**.
- Whether the area script (not the REL) owns the transitions is **probable, not proven**: the tables
  hold names only.
- The beam actor's model name is "Splitter Beam Effect" (rodata 0x2D0); its class name is
  **unknown**.
- `fn_75_1974` creates the laser-pulse `CPlasmaProjectile`; which `CAssetId` it uses comes from
  the command module's 0x7E0 slot (`laserPulseProjectile`), but the second `CProjectileInfo`
  (0xE84) source fields are **unknown**.

### Stand-ins the area-4 layer-3 fight reaches (2026-10-20)

The ElitePirate smoke in this area logs five Splitter stand-ins (the run: `/tmp/opencode/smoke/ep_run_ep.sh`,
log `/tmp/opencode/smoke/ep2.log`); they are the only stand-in lines in it and the fight reaches no
other Splitter shell. In the order the log prints them, with the retail sizes from
`config/G2ME01/rels/Splitter/symbols.txt`:

| stand-in | fn | size |
|---|---|---|
| `CSplitterMainChassis::AcceptScriptMsg` (slot 0x18) | `fn_75_F1C0` | 0x3A0 |
| `CSplitterMainChassis::SetIngPossessed` (slot 0x104) | `fn_75_E87C` | 0x90 |
| `CSplitterCommandModule::AcceptScriptMsg` (slot 0x18) | `fn_75_654C` | 0x24C |
| `CSplitterCommandModule::Think` (slot 0x14) | `fn_75_6490` | 0xBC |
| `CSplitterMainChassis::Think` (slot 0x14) | `fn_75_F0E8` | 0xB8 |

`fn_75_F1C0` (0x3A0) and `fn_75_654C` (0x24C) are over the 0x200 a slice covers, so they stay
once-logging stand-ins for now. The other three are the size a slice can take, but each of them
reaches further into the module than its own body:

- `fn_75_6490` (the command module's Think, 0xBC): the active bit (the byte at 0x20, 0x64B4/0x64B8
  `extrwi` bit 24), then `CPatterned::Think` (0x64C0), the collision actor manager's update (the
  manager at 0xE58, 0x64C8-0x64D4, the update options 0) and six module calls - `fn_75_31A8`
  (0x64E0), `fn_75_32F4` (0x64F0), `fn_75_3004` (0x64FC), `fn_75_184C` (0x650C), `fn_75_1188` (0x651C)
  and `fn_75_21FC` (0x652C). Those are the line-of-sight tracker and the derived state, so a
  faithful port needs them read first.
- `fn_75_E87C` (0x90): the base's `SetIngPossessed(bool, float, CStateManager&)` (0xE8A0), then a
  test of the flag byte's 0x80 (0xE8A4 `clrlwi r0, r30, 24`), the command module's id (the u16 at
  0xC6C, 0xE8AC `lhz`), `ObjectById` (0xE8BC) and the cast (0xE8C0), then the command module's
  vtable slot 0x104 with (1, mgr, &lbl_75_rodata_390 (0.0f), 0.0f) (0xE8CC-0xE8EC).
- `fn_75_F0E8` (0xB8): the main chassis's Think, the same shape as the command module's.

So the Splitter porting is a per-function slice with its own module callees, not one body; the
first one to take is `fn_75_E87C` (0x90, the fewest module callees).

#### fn_75_E87C (0x90) read, ready to port - two open points

The asm (0xE87C-0xE90C) is `SetIngPossessed(bool, float, CStateManager&)` on the main chassis:
the base call `SetIngPossessed__10CPatternedFbfR13CStateManager` (0xE8A0), the flag test
(0xE8A4 `clrlwi. r0, r30, 24` - `clrlwi` clears the left 24 bits and keeps the low byte,
so this tests the flag argument (`r30`, the possession bool) for non-zero), the command module's id (the u16 at
0xC6C, 0xE8AC `lhz`), `ObjectById` (0xE8BC) and the `CSplitterCommandModule` cast (0xE8C0), then the
command module's vtable slot 0x104 with (1, mgr, &lbl_75_rodata_390 (0.0f), 0.0f) (0xE8CC-0x8EC).

Open points before it can be written:

1. Resolved: the test is the flag's low byte, so it is the possession bool (`if (possessed)`) -
   the annotator's `mask 0xff` reading is right.
2. Resolved: 0xC6C is above the base's fields, so it is the main chassis's own - the command
   module's id. The port keeps it in `CSplitterMainChassis::mCommandModuleId`; the retail's ctor
   writes kInvalidUniqueId into it (fn_75_F5F8 @0x0000F6E4, the value read at 0x0000F6CC) and
   the state machine's own code replaces it later (0x0000AF74, 0x0000E720, 0x0000EBD0 - not
   ported yet, so the port's id stays the invalid one).
3. Resolved: the command module's vtable slot 0x104 is the DOL's
   `SetIngPossessed(bool, float, CStateManager&)` (the command module does not override it), so
   the call is the host's CPatterned virtual with (true, 0.0f, mgr) - the retail passes
   lbl_75_rodata_390 (0.0f) as the duration.

#### fn_75_6490 (0xBC) - the command module's Think

The body: the active gate (the byte at 0x20, 0x64B4 `lbz` and 0x64B8 `extrwi` bit 24 - the byte's
0x80), `CPatterned::Think` (0x64C0), the collision actor manager's update (the manager at 0xE58,
0x64C8 `lwz`; the update options 0 at 0x64D0, which is `kUO_ObjectSpace`; 0x64D4 the update) and
then the module's own six calls, in order:

| call | fn | size |
|---|---|---|
| 0x64E0 | `fn_75_31A8` | 0x14C |
| 0x64F0 | `fn_75_32F4` | 0x16C |
| 0x64FC | `fn_75_3004` | 0x1A4 |
| 0x650C | `fn_75_184C` | 0x128 |
| 0x651C | `fn_75_1188` | 0x344 |
| 0x652C | `fn_75_21FC` | 0x84 |

All six are over 0x80, so none of them is ported in this slice: they are the retail's own per-frame
updates (the line-of-sight tracker and the derived state the state machine reads), and the port
keeps them as named once-logging stand-ins that return neutral values.

#### fn_75_F0E8 (0xB8) - the main chassis's Think

The body: the active gate (the byte at 0x20, 0xF10C `lbz` and 0xF110 `extrwi` bit 24 - the byte's
0x80), `CPatterned::Think` (0xF118), the docking link (the partner id at 0xC68, 0xF11C `lwz` and
0xF120 `cmplwi 0`: with no partner the retail looks for one - 0xF130 `fn_75_A4E4` - otherwise it
updates the dock - 0xF144 `fn_80135D84`, the DOL's), the line-of-sight tracker (the tracker at
0xC70, 0xF154 `fn_801F55F0`, the DOL's) and the module's own three calls:

| call | fn | size |
|---|---|---|
| 0xF130 | `fn_75_A4E4` | 0x418 |
| 0xF164 | `fn_75_AFE8` | 0x11C |
| 0xF174 | `fn_75_ACEC` | 0x2FC |
| 0xF180 | `fn_75_9520` | 0x11C |

All four are over 0x80 and the DOL's two have no module size, so each stays a named once-logging
shell in the port.
