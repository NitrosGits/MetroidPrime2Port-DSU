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
`fn_75_1974` — the laser-pulse shot). The 12-byte entries `{0, 0xFFFFFFFF, fn}` in `.data` are the
**function records** of the FSM tables (§b), not destructor vtables: `.data:0x0` → `fn_75_5A40`
(the head's `StateOver` trigger) and `.data:0xCAC` → `fn_75_5A54` (the module's auto-destruct,
whose record the func-ptrs table of `fn_75_8254` copies).

## b. FSM

Two independent state machines, one per class. A table entry is **16 bytes**:
`{const char* name, 0, 0xFFFFFFFF, fn}` - the name pointer is in `.data`, the string in `.rodata`
and the **function word is filled at load time** by the module's static initialisers, which the
REL keeps in its `.ctors`: `fn_75_7C80` (the head) and `fn_75_FED0` (the chassis). Each copies the
three-word records `{0, 0xFFFFFFFF, fn}` of `.data` into the tables, one record per entry, in
order - so the retail function of every name is known. (The tables are **not** names only.)

**FSM1 - command module** (initialiser `fn_75_5E8C`, vtable slot 0xF0 `SetupStateMachine`):
`(*mgr)->unk18(mgr, &lbl_75_data_CC, 0x11)` = 17 triggers, `unk14(mgr, &lbl_75_data_284, 0x0E)` =
14 states, `unk1C(mgr, &lbl_75_data_3B8, 7)` = 7 codes. The records are `.data 0x0..0x2D0`
(`fn_75_7C80`, the `.ctors` entry, fills them).

| triggers | | states | | codes | |
|---|---|---|---|---|---|
| # | name | fn | # | name | fn |
|---|---|---|---|---|---|
| 0 | StateOver | `fn_75_5A40` | 9 | InHoverRange | `fn_75_570C` |
| 1 | InDetectionRange | `fn_75_5944` | 10 | InLaserPulseRange | `fn_75_565C` |
| 2 | IsScanning | `fn_75_58E8` | 11 | ShouldDodge | `fn_75_5644` |
| 3 | IsInitiallyDocked | `fn_75_58DC` | 12 | ShouldAttack | `fn_75_5624` |
| 4 | IsDocked | `fn_75_58BC` | 13 | ShouldFireAgain | `fn_75_5610` |
| 5 | HasDockingPath | `fn_75_5864` | 14 | ShouldLaserSweep | `fn_75_55F8` |
| 6 | HasDockingTarget | `fn_75_57F0` | 15 | PathOver | `fn_75_55B0` |
| 7 | DockingPathOver | `fn_75_57D4` | 16 | PathShagged | `fn_75_5568` |
| 8 | HasTarget | `fn_75_57B4` |  |  |  |

| # | name | fn | # | name | fn |
|---|---|---|---|---|---|
| 0 | Start | `fn_75_5564` | 7 | Hover | `fn_75_4D04` |
| 1 | Idle | `fn_75_5538` | 8 | Dodge | `fn_75_4AC0` |
| 2 | SpawnIdle | `fn_75_5498` | 9 | LaserPulse | `fn_75_4864` |
| 3 | Dead | `fn_75_5448` | 10 | LaserSweep | `fn_75_44DC` |
| 4 | Scanning | `fn_75_523C` | 11 | LostChassisReaction | `fn_75_42F8` |
| 5 | FaceTarget | `fn_75_50B0` | 12 | SeekMainChassis | `fn_75_4138` |
| 6 | PathFind | `fn_75_4E90` | 13 | FollowDockingPath | `fn_75_3FE0` |

| # | name | fn | # | name | fn |
|---|---|---|---|---|---|
| 0 | NotifyDocking | `fn_75_3EEC` | 4 | FindBestDodgeDirection | `fn_75_3B34` |
| 1 | SelectTarget | `fn_75_3E60` | 5 | RaiseShields | `fn_75_3B20` |
| 2 | SetTargetDest | `fn_75_3D20` | 6 | ResetAttackTimes | `fn_75_3ADC` |
| 3 | SetDockingDest | `fn_75_3BEC` |  |  |  |

**FSM2 - main chassis** (initialiser `fn_75_E9E4`, vtable slot 0xF0 `SetupStateMachine`):
`(*mgr)->unk18(mgr, &lbl_75_data_720, 0x1C)` = 28 triggers, `unk14(mgr, &lbl_75_data_9E8, 0x16)` =
22 states, `unk1C(mgr, &lbl_75_data_BCC, 0x0B)` = 11 codes. The records are `.data 0x5D0..0x714`
(`fn_75_FED0` fills them).

| triggers | | states | | codes | |
|---|---|---|---|---|---|
| # | name | fn | # | name | fn |
|---|---|---|---|---|---|
| 0 | StateOver | `fn_75_E64C` | 14 | ShouldLegStab | `fn_75_E3A4` |
| 1 | IsIntact | `fn_75_E62C` | 15 | ShouldMorphballStab | `fn_75_E2F0` |
| 2 | IsDisabled | `fn_75_E620` | 16 | ShouldSpinAttack | `fn_75_E1D0` |
| 3 | ShouldWaitForSnatch | `fn_75_E5D0` | 17 | ShouldLaserSweep | `fn_75_E0B0` |
| 4 | ShotAt | `fn_75_E5B8` | 18 | ShouldDodge | `fn_75_E098` |
| 5 | LostHead | `fn_75_E5AC` | 19 | IsSpinAttackOver | `fn_75_E084` |
| 6 | HasHead | `fn_75_E58C` | 20 | SpunIntoPlayer | `fn_75_E078` |
| 7 | HasRetreatPoint | `fn_75_E56C` | 21 | SpunIntoGeometry | `fn_75_E06C` |
| 8 | HasTarget | `fn_75_E54C` | 22 | BreakOutOfSpin | `fn_75_E060` |
| 9 | CanReachTarget | `fn_75_E530` | 23 | TooManyBounces | `fn_75_E040` |
| 10 | HasLineOfSight | `fn_75_E524` | 24 | DeployFromHoldingTube | `fn_75_E034` |
| 11 | PathShagged | `fn_75_E4DC` | 25 | ShouldDeploy | `fn_75_E028` |
| 12 | PathOver | `fn_75_E494` | 26 | DeployPathOver | `fn_75_E00C` |
| 13 | PatrolPathOver | `fn_75_E478` | 27 | PrepareForDocking | `fn_75_DFEC` |

| # | name | fn | # | name | fn |
|---|---|---|---|---|---|
| 0 | Start | `fn_75_DFE8` | 11 | SpinCollisionReaction | `fn_75_D2A0` |
| 1 | Idle | `fn_75_DFE4` | 12 | SpinBounce | `fn_75_D13C` |
| 2 | Inactive | `fn_75_DF88` | 13 | SpinTelegraph | `fn_75_D0A4` |
| 3 | Activate | `fn_75_DDF8` | 14 | SpinToIdle | `fn_75_CF44` |
| 4 | Patrol | `fn_75_DCFC` | 15 | SpawnDrop | `fn_75_CCF0` |
| 5 | Scanning | `fn_75_DCF8` | 16 | HoldingTube | `fn_75_CC98` |
| 6 | PathFind | `fn_75_DC08` | 17 | WaitForDocking | `fn_75_C9E4` |
| 7 | LegStabAttack | `fn_75_DA38` | 18 | WaitForSnatch | `fn_75_C8F4` |
| 8 | SpinAttack | `fn_75_D7D4` | 19 | FollowDeployPath | `fn_75_C51C` |
| 9 | Dodge | `fn_75_D61C` | 20 | DeploymentLanding | `fn_75_C034` |
| 10 | LaserSweepAttack | `fn_75_D3E0` | 21 | HeadExplosion | `fn_75_BEEC` |

| # | name | fn | # | name | fn |
|---|---|---|---|---|---|
| 0 | SelectTarget | `fn_75_BE28` | 6 | SetMorphballStabLeg | `fn_75_B66C` |
| 1 | SetTargetDest | `fn_75_BC70` | 7 | FindBestStabLeg | `fn_75_B57C` |
| 2 | SetRetreatDest | `fn_75_BBC0` | 8 | FindBestDodgeDirection | `fn_75_B2A0` |
| 3 | SetupSpinAttack | `fn_75_BAA8` | 9 | BounceOffPlayer | `fn_75_B1D8` |
| 4 | EndSpinAttack | `fn_75_B9B0` | 10 | BounceOffGeometry | `fn_75_B104` |
| 5 | SetupLaserSweep | `fn_75_B678` |  |  |  |

Names are the rodata strings: FSM1 triggers at rodata 0xA0..0x17D, states 0x189..0x206, codes
0x218..0x274; FSM2 triggers 0x3E4..0x55B, states 0x56D..0x663, codes 0x671..0x714. The
**transitions** live in the AFSM resource (the state machine asset), the **state bodies** in the
REL's tables above - the state machine runs a state or a code by name, and the table entry with
that name gives the function.

Two of the 99 bodies are ported: the head's `IsScanning` trigger (`fn_75_58E8`,
`PortChassisTimerRunning`) and its `NotifyDocking` code (`fn_75_3EEC`, `PortDockToChassis`). The
other 97 are once-logging stand-ins in the port (2026-10-20), each naming the retail function it
stands in for.

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

**WP2 part 1 done (2026-10-20): the tables are wired.** Both `SetupStateMachine`s register the
tables with the function of every name (§b): the two ported bodies (the head's `IsScanning`
trigger `fn_75_58E8` and its `NotifyDocking` code `fn_75_3EEC`) and 97 once-logging stand-ins,
each naming the retail function it stands in for. The head's `NotifyDocking` code is where the
retail runs `fn_75_3EEC`, so the port does not call it from the Think any more.

**The run (2026-10-20) enters**: the head's **Start** state and the chassis's **Start** and
**Patrol** states. The triggers it asks are the head's `IsInitiallyDocked`, `HasDockingTarget`
and `HasTarget` and the chassis's `IsDisabled`, `DeployFromHoldingTube`, `ShotAt`,
`PatrolPathOver`, `LostHead`, `ShouldWaitForSnatch` and `PrepareForDocking`; the codes are the
head's `RaiseShields` and `SelectTarget`. Every one of them is a stand-in except the head's
`IsScanning` (the ported `fn_75_58E8`), which the run does not reach.

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
- `fn_75_7C80` (0x578) and `fn_75_FED0` (0x8A8) are the module's **static initialisers** (the
  REL's `.ctors`): they fill the function word of the FSM tables from the three-word records of
  `.data` (§b). `fn_75_10778` (0x64C) is still **unknown**.
- The derived blocks 0xA50-0xB7B (body) and 0xA7C-0xD6B (head) are **unmapped**.
- The **transitions** live in the AFSM resource (the state machine asset the actor carries); the
  REL's tables give the **bodies** by name (§b). The area script owns the placements and the
  connections, not the transitions.
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

The run of 2026-10-20 after `fn_75_F1C0` and `fn_75_654C` were ported no longer logs the two
`AcceptScriptMsg` shells - they are ported - and no `[port] caught` line. It logs the five
module-callee shells the two handlers make instead: `fn_75_3460` (0x218, the head's XCRT case),
`fn_75_AC68` (0x84, the chassis's XAND case), `fn_75_AAC4` (0xC8) and `fn_75_8D50` (0xA8, the
chassis's XDEL case) and `fn_75_1790` (0xBC, the head's XDEL case).

The **head's Think does not run in that fight** - the beam-effect spawner's own shell
(`fn_75_10D0`) would have logged - so `fn_75_1188` is not exercised by the area-4 layer-3 smoke.

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

The two DOL calls in `fn_75_F0E8` are host engine functions, not module ones (the DOL's symbol map,
`config/G2ME01/symbols.txt`):

| call | DOL symbol | name |
|---|---|---|
| 0xF144 | `fn_80135D84` | `CCollisionActorManager::Update(float, CStateManager&, EUpdateOptions)` (0x540) |
| 0xF154 | `fn_801F55F0` | `CVisualKnowledge::Update(CStateManager&, float)` (0x26C) |

The first is the host's `CCollisionActorManager::Update` on the manager at 0xC68 (the update options
0 = `kUO_ObjectSpace`); the second is the host's `CLineOfSightTracker::Update` on the visual
knowledge at 0xC70 (the host's `CLineOfSightTracker` is the same thing - see
`CMinorIng.hpp`'s `mLineOfSight`).

**Condition for later**: the retail leaves the collision actor manager at 0xC68 null (the ctor
writes 0 at 0x0000F6BC) until `fn_75_A4E4` (0x418) builds it. The port makes an empty manager
instead, so that stand-in must go - and the port's manager must go back to null - when
`fn_75_A4E4` is ported.

#### fn_75_184C (0x128) - the command module's look-at

The body: the target id (the u16 at 0xF02, 0x1874 `lhz`) is looked up (0x1888 `ObjectById`) and,
when it is there, the module's locator transform (the segment id at 0xEF8, 0x18A0
`GetLctrTransform`) gives the position; the offset vector (0xF50, 0xF54 and 0xF58, added at
0x18E8-0x18F0) is added to it, the transform looks at that point (0x1900 `LookAt` with the up
vector) and the result goes to the target's virtual at the vtable slot 0x8C (0x1924-0x192C). The
sound (the handle at 0xF5C read at 0x1930, the target's position at 0x444 and the byte 0x7f, 0x194C
`UpdateEmitter`) is the DOL's `CSfxManager::UpdateEmitter`, which the host spells differently.

Two calls stay shells: the DOL's CEntity virtual at the slot 0x8C (the DOL's slot 35 is not named
in the port) and the DOL's `CSfxManager::UpdateEmitter`. The four fields the body reads (0xF02,
0xEF8, 0xF50-0xF58 and 0xF5C) are written by the retail's state machine, which the port has not
entered, so they keep their defaults.

#### fn_75_31A8 (0x14C) - the command module's "is the chassis ours" read (not ported yet)

Read on 2026-10-20, left for a slice with room (the port needs six new fields and the chassis's
own 0xCC0 and 0x420 semantics, so it is not a quick body):

- the chassis id (the u16 at 0xEFA, 0x31C8 `lhz`): if it is the invalid id the function goes to
  0x3254, otherwise the chassis is looked up (0x31E8 `GetObjectById`) and cast (0x31EC); a null
  cast or a chassis whose command module id (0xC6C) is not ours (0x3200-0x3204) goes to 0x3238;
- ours: 0xEFE takes the chassis's 0xCC0 (0x3210-0x3218), then `fn_75_1578` (0x3224) and the timer
  0xF14 is set to 3.0f (`lbl_75_rodata_90`, 0x322C-0x3230);
- 0x3238: the chassis id at 0xEFA is set to the invalid id (0x3240) and 0x12A is written (0x324C);
- 0x3254: the data's flag byte (0x7C0) bit 0x8 (0x3258 `rlwinm` 28-28) and the timer 0xF14 <= 0.0f
  (0x326C-0x3274) and the id 0xEFC being the invalid one (0x327C-0x3280) gate `fn_75_2280`
  (0x3284) and the timer is set to 3.0f again (0x328C-0x3290);
- 0x3298: the id 0xEFC (0x32A4) is looked up and cast (0x32B4-0x32B8); if the chassis's byte at
  0x420 has the word's bit 25 set (0x32C4-0x32CC) the function ends, else 0xEFC is set to the
  invalid id (0x32D0-0x32D8).

Resolved since (2026-10-20, for the next slice):

- the byte at 0x420, the word's bit 25 (= the byte's 0x40), is the CPatterned `mAlive` bit the
  Ing port already maps (`grep 0x420 src/MetroidPrime/Enemies/PortCIng.cpp`: "CPatterned 0x420
  bit 6"); the host's accessor is `CPatterned::GetAlive()`.
- the chassis's 0xCC0 is a u16 id written at 0xBE54 and at 0xF764 (the constructor) and read at
  0x320C, 0x9540, 0xB058, 0xB218, 0xB350, 0xB5A0 and 0xB730; what it names is still not
  established, so the port would keep it as an id member with the offset in a comment.
- the compare at 0x3200 is against **this+0x8** (0x31FC `lhz r0, 0x8(r31)`, not the stack copy at
  0x31E4) - a u16 id in the base class, the command module's own id. The port has no host member
  for the retail's CEntity+0x8 yet, and that is the one thing the body needs before it can be
  written.
- the two module calls are `fn_75_1578` (0x218) and `fn_75_2280` (0x170): both over 0x80, so
  both stay named once-logging shells.
- the command module's own fields the body touches: 0xEFA (the id it looks up), 0xEFE (the copy
  of the chassis's 0xCC0), 0xEFC (the second id it checks), 0xF14 (the timer, set to 3.0f) and
  0x12A (a base-class id, cleared together with 0xEFA at 0x3240-0x324C); the data's flag byte at
  0x7C0 bit 0x8 (0x3258 `rlwinm` 28-28) is the port's `mData.mUnknownBD80FD94 & 0x8`.

#### fn_75_3004 (0x1A4) - the command module's collision update (not ported yet)

Read on 2026-10-20, left for a slice with room. The body: a module call (fn_75_2BE0 = .text:0x00002BE0; // type:function size:0xE0 fn_75_2CE0 = .text:0x00002CE0; // type:function size:0x324 , 0x3024) whose result
is tested (0x3028) and then used through a virtual call (0x3040), the DOL's
CSfxManager::UpdateEmitter (0x3074), an object lookup and a CCollisionActor cast (0x3088-0x3094),
the command module's manager at 0xE58 (0x309C), the DOL's fn_80135C28 (0x30C0), a state in 0xEE0
that has to be 0..2 (0x30C8-0x30D8), the flag byte 0xF6A's 0x08 (0x30E0), a CCollisionActor
virtual at 0x38 (0x30F8) and its 0x4 field (0x3104-0x3110), the state 0xEE0 cleared (0x311C), the
timer 0xF10 set to a constant (0x312C), another module call (0x3130) and the flag's 0x08 again
(0x313C). Several of those are DOL functions or virtuals the port has no name for yet, so the
body needs a slice that can identify them. **One of them is named now**: the CCollisionActor's
virtual at the vtable word 0x38 (0x30F0-0x30F8) is `CCollisionActor::HealthInfo()` and the
float at its +0x4 is the health (found while porting fn_75_2CE0, which calls the same virtual
at 0x2D9C), so the port's shell for it in this body can go with the next slice.

#### fn_75_1188 (0x344) - the command module's spawn (ported 2026-10-20)

Read on 2026-10-20, left for a slice with room. It is the biggest of the six callees: an object is
looked up (0x11B0 ObjectById), a transform is built (0x11CC) and set (0x11E0), a module call
(fn_75_10D0 = .text:0x000010D0; // type:function size:0xB8 fn_75_58E8 = .text:0x000058E8; // type:function size:0x5C fn_75_11884 = .text:0x00011884; // type:function size:0x208 ) decides whether the body runs (0x1200-0x1208), the timer at 0x3D0 is compared with zero
(0x121C-0x1224), a unique id is allocated (0x1230) and kept at 0xF68 (0x1244), an actor is created
(0x1258 the new, 0x12A0 the CEntityInfo, 0x12B8 the name), the rotation uses one degree in
radians (0.017453292f, 0x12C4) and the body is driven by fn_75_11884 (0x1380). The port needs the
host's actor creation path for the middle of it, so it is a slice of its own.

The rest of `fn_75_1188` (0x1330-0x14CC) reads as the rest of the actor creation: the placement
`new` at 0x1258 takes the size **0x1A0** (`li r3, 0x1a0`) and the class name at
`lbl_75_rodata_A0 + 0x229`, the `CEntityInfo` at 0x12A0 is built from this+0x4 (the area id, twice
- 0x1294 and 0x1298), `NullConnectionList` (0x1278) and `kInvalidEditorId` (0x1268), the actor's
name is `lbl_75_rodata_A0 + 0x230` (0x12B8), and the rotation is built from the angle at 0x864
(0x12CC) times one degree in radians (0.017453292f, `lbl_75_rodata_74`, 0x12C4), rounded with
`fctiwz` (0x1308) and reduced by `fnmsubs` (0x1324) - the same pattern the other ports' spawn
code uses. The class the `new` makes is the one whose size is 0x1A0 and whose name sits at
`lbl_75_rodata_A0 + 0x229`; reading that string gives the host class to use.

**Two corrections to the read above** (found while porting it, 2026-10-20):

- The string at `lbl_75_rodata_A0 + 0x229` (rodata **0x2C9**) is **not** a class name: it is the
  obfuscated `__FILE__` string the placement `new` takes (`"??(?)"`; the actor's *name* is the
  next string, rodata **0x2D0** = `"Splitter Beam Effect"`). The beam effect actor's class name is
  still **unknown** - the host has no class for it.
- The field at **0x3D0** is **not a timer**: the decomp's matching `CPatterned` fills 0x3C8-0x3D4
  from `CPatternedInfo` + 0x4..0x10, so 0x3D0 is that struct's **+0xC, the detection range**
  (the host reads it by name, `CPatterned::GetDetectionRange()`).

**Port (2026-10-20).** `CSplitterCommandModule::PortSpawnBeamEffect` (fn_75_1188) and
`CSplitterCommandModule::PortChassisTimerRunning` (fn_75_58E8) are the port's bodies. The gate
looks the chassis up (0x5904 GetObjectById), casts it (0x5908) and tests the chassis's own field
at **0xD28** (0x5918): that is the chassis's scan timer, which its script-message handler sets
(fn_75_F1C0, 0x0000F40C-0x0000F42C: the scan duration when that is above zero, FLT_MAX
otherwise); the port does not run that handler, so the timer stays zero. With the gate false
the port clears the id at 0xF68 and removes the sound emitter (0x14A0-0x14AC); with it true and
no beam effect yet it tests 0x3D0 and then, **port limit**, allocates no id: the retail
allocates one (0x1230) and keeps it at 0xF68 (0x1244) before it creates the actor, and an id
with no object behind it would be allocated again every frame and would eat the manager's
object ids.

The **creation of the 0x1A0-byte actor and the manager's add stay a once-logging shell**: the
host has no class for the beam effect actor, so `fn_75_11884` (its constructor) and
`fn_75_10D0` (the module's own transform) are shells too. The port still builds what the retail
builds for them - the `CEntityInfo` (the area id, `NullConnectionList`, `true` and
`kInvalidEditorId`), the name and the rotation (the data's angle at 0x864 times one degree in
radians) - and hands them to the shell.

#### fn_75_F1C0 (0x3A0) - the main chassis's script-message handler (ported 2026-10-20)

The chassis's `AcceptScriptMsg` (vtable slot 0x18): the base's call (0x0000F1F4) and a switch on
the message's four-character code (0x0000F1F8-0x0000F33C).

| message | code | what the retail does | the port |
|---|---|---|---|
| XCRT | 0x58435254 | the locomotion type (fn_75_AA68) and the body controller's activation | ported |
| XDEL | 0x5844454C | the collision actor manager's destruction and two module calls | ported + 2 shells |
| ACTV | 0x41435456 | the manager's activation, only when the actor is inactive | ported |
| DCVT | 0x44435456 | the manager's deactivation, only when the actor is active | ported |
| AUDT | 0x58415544 | the manager's physics off | ported |
| ALRT | 0x414C5254 | the base's flag byte at 0x420 gets its 0x80 bit | ported |
| STRT | 0x53545254 | the flag byte at 0xD41 gets its 0x04 bit | ported |
| ACTN | 0x4143544E | the scan timer at 0xD28 (the data's scan duration, FLT_MAX when it is zero) | ported |
| XAND | 0x58414C44 | the manager's area list's value to 0xB7C and fn_75_AC68 | the write stays out, the call is a shell |
| XCMG | 0x58434D47 | fn_75_A0A0 and the base's flag byte | the call is a shell, the flag is ported |
| XRDG / XXDG | 0x58524447 / 0x58584447 | fn_75_9FA8 and the base's flag byte | the call is a shell, the flag is ported |
| XHIT | 0x58484954 | fn_75_9B9C | shell |
| XLND | 0x584C4E44 | the counter at 0xD00 is set to 5 | ported |
| IM00 | 0x494D3030 | the flag byte at 0xD40 is cleared and, with 0xD41's 0x80 bit set, the timer at 0xD2C and the counter at 0xCFC advance | ported |
| IM01 | 0x494D3031 | the flag byte at 0xD40 is set | ported |

The module's own calls in it (fn_75_AAC4 0xC8, fn_75_8D50 0xA8, fn_75_A0A0 0x2DC, fn_75_9FA8 0xF8,
fn_75_9B9C 0x40C, fn_75_AC68 0x84) are all over 0x80 and stay once-logging shells. **fn_75_AA68
(0x5C)** is ported with it: the data's flag word at 0x828 gives the locomotion type (the word's
bit 30 gives kLT_Lurk, bit 31 kLT_Crouch, anything else kLT_Relaxed).

#### fn_75_654C (0x24C) - the command module's script-message handler (ported 2026-10-20)

The head's `AcceptScriptMsg` (vtable slot 0x18): the base's call (0x6578) and a switch on the
message's code (0x657C-0x660C). XCRT activates the body controller, runs fn_75_3460 (0x218, a
shell) and rolls the manager's random (0.0f to 100.0f) to set the state at 0xEE4 to 2 when the
roll is 50.0f or more and to 1 otherwise (0x6668); XDEL destroys the collision actor manager,
deletes the beam effect at 0xF68 and runs fn_75_1790 (0xBC, a shell); ACTV and DCVT activate and
deactivate the manager; AUDT switches its physics off; ALRT sets the base's flag byte at 0x420;
XAND runs fn_75_3A8C and XCMG runs fn_75_2CC0.

**fn_75_3A8C (0x50)** and **fn_75_2CC0 (0x20)** are ported with it: the first looks the object
that is connected with the state "CONN" and the message "ATCH" up and keeps its id at 0xEFC; the
second sets the flash weight at 0xF08 to 1.0f when the message's sender id is the id at 0xF00.

#### fn_75_1790, fn_75_AC68, fn_75_8D50, fn_75_AAC4 (ported 2026-10-20, one commit each)

The four module calls the area-4 layer-3 fight reaches besides `fn_75_3460`:

| fn | size | what it does | the port |
|---|---|---|---|
| `fn_75_1790` | 0xBC | the head's laser-pulse projectile despawn: the id at 0xF02, the DOL's CEntity virtual at the vtable word 0x24, the emitter at 0xF5C, the base's flag byte at 0x420 | ported; the DOL's virtual and its RemoveEmitter stay shells |
| `fn_75_AC68` | 0x84 | the chassis's connected-object lookups: the ids at 0xCC4 ("CONN"/"ATCH") and 0xCC6 ("RTRT"/"FOLW") | ported |
| `fn_75_8D50` | 0xA8 | the chassis's team exit: the team id at 0xCBE | ported |
| `fn_75_AAC4` | 0xC8 | the chassis's player-hint release: the hint's id at 0xCC4, the hint's actor id at 0x1AC, a "DECR" message | ported; `CScriptPlayerHint` grows a TARGET_PC setter for the actor id |

`fn_75_3460` (0x218, the head's XCRT case) is still a shell: it builds the head's collision actor
manager with the joint collision descriptions of the "Beam_LCTR" locator and walks the collision
actors for the `CCollisionActor` whose id goes to 0xF00.

**Why the head's Think never runs in the area-4 fight (2026-10-20).** The head is **inactive**:
the host's `CStateManager` ticks every patterned actor (`ShouldUpdatePatterned` does not look at
the active flag), but the port's `CSplitterCommandModule::Think` returns at its
`if (!GetActive()) return;` gate, which is the retail's own gate (the byte at 0x20's 0x80).
Nothing in the port activates either Splitter actor: the `CEntityInfo`'s active flag comes from
the script's editor properties (`LdrToEntityInfo`), the host's generic FSM "Start" state is
empty (`CPatternedAiFunctions.cpp:19`), the module's own FSMs (`fn_75_5E8C`, `fn_75_E9E4`) are
not ported, and the fight's script sends the head no `kSM_Activate`. The chassis is in the same
state (its Think shells `fn_75_AFE8`/`ACEC`/`9520` never log either). This is the port's FSM
gap, not a bug in a ported body, so nothing was changed for it.

#### fn_75_3460 (0x218) - the head's collision actor manager (ported 2026-10-20)

The head's one joint sphere - the "Skeleton_Root" locator (rodata 0x0) with the radius 3.0f
(rodata 0x4) and the mass 1000.0f (rodata 0x94) - is built and the collision actor manager is
made with it (the old one deleted, the new one kept at 0xE58). Every collision actor of the
manager is then walked: the description's id (its 0x3C) is looked up and cast to a
CCollisionActor, and when the description's name is "Beam_LCTR" (rodata 0x8) the id goes to
0xF00. The module's own call `fn_75_2CE0` (0x324) stays a once-logging shell - it is the last
Splitter line the area-4 layer-3 fight logs.

The module's own state machines are still shells: `fn_75_5E8C` (the head's FSM, **0x8C**) and
`fn_75_E9E4` (the chassis's FSM, **0x8C**).

#### fn_75_2CE0 (0x324) - the head's shield (ported 2026-10-20)

The head's shield: it applies the state at 0xEE0 to the beam collision actor (the id at 0xF00).
The flag (the third argument, r5) is the "a shield goes up" one - the asm tests its low byte
(0x2D0C `clrlwi` with the word's bit 24 - the byte's 0xFF).

| what | where |
|---|---|
| the shield-on sound (the sound id at **0x89C**) is added when the flag is set; its handle is dropped (the retail writes it to a stack slot it never reads) | 0x2D1C-0x2E44 |
| the tracked emitter (the handle at **0xF60**) is removed and the handle cleared | 0x2D48-0x2D6C |
| the beam collision actor (the id at **0xF00**) is looked up and cast; with no actor the body ends | 0x2D6C-0x2D88 |
| the collision actor's health info's health (the float at **+0x4**) is set to the shield's HP (the float at **0x804**) | 0x2D8C-0x2DA0 |
| state **1**: `AddMaterial(0x21)` on the collision actor, `AddMaterial(0x36)` on the head, the light shield's vulnerability (**0xD0C**) on the actor, the effect "LightShield" on and "DarkShield" off, the light shield's sound (the id at **0x894**) added as a looping emitter whose handle goes to 0xF60 | 0x2DC4-0x2E80 |
| state **2**: `RemoveMaterial(0x21)` on the actor, `AddMaterial(0x36)` on the head, the dark shield's vulnerability (**0xD3C**), the effects swapped, the dark shield's sound (the id at **0x898**) | 0x2E88-0x2F44 |
| any other state: both materials removed, the actor's vulnerability becomes the pass-through one (fn_800DBB50), both effects off | 0x2F4C-0x2FD8 |

The host mappings: the materials 0x21 / 0x36 are `kMT_Character` / `kMT_Unknown54`; the two
effect names are the rodata strings **"LightShield"** (rodata 0x10) and **"DarkShield"**
(rodata 0x14); the collision actor's virtual at the vtable word **0x38** is
`CCollisionActor::HealthInfo()` (the float at its +0x4 is the health), so the same virtual in
fn_75_3004 is no longer nameless; the emitter's position is the guest's vector at 0x54 (the
actor's translation) and its area the guest's word at 0x4 (the actor's area); the sound ids are
masked to 16 bits (the asm's `clrlwi` 16). The port's body is
`CSplitterCommandModule::PortUpdateShield(CStateManager&, bool shieldUp)`.

#### The head's Think in the area-4 fight (2026-10-20)

**The Splitter's objects live in the *next* area.** The console prints them as **a05**
(`02_cliff_reactorcore`, the world's area 5) while the player is in **a04**
(`0c_cliff_hall`, where the Ingsmasher fight is). The port's update loop skips an entity whose
area has been occluded for more than 5 s (`CStateManager::Think`'s occlusion check and
`CGameArea::AliveUpdate`'s `mOccludedTime`), so **the Splitter does not think at all while the
player is in area 4** - the head's `if (!GetActive()) return;` gate is not even the blocker
(the `send <uid> Activate` does clear the "inactive" mark). The player has to be in area 5:
the `NEXTAREA` input token moves them there and the Splitter updates from then on.

The recipe (the input file's tokens are **comma-separated on one line**, not one per line):

    # input file: LAYER3@600+4,NEXTAREA@1500+4
    # driver: a copy of the ElitePirate driver that also activates the Splitter
    #   /tmp/opencode/smoke/split_ep_con.py  (a copy of ep_con.py) and the run script
    #   /tmp/opencode/smoke/split_run.sh
    #   WORLD=1baa96c2,b666b655 (Sanctuary Fortress), area 4 -> the layer-3 objects
    #   (the Splitter) load; the driver walks the player into the dock trigger and
    #   sends `send <uid> Activate` to the chassis and the head once each.

The run (`/tmp/opencode/smoke/split.log`, the driver `/tmp/opencode/smoke/split_con.out`)
logs, once the player is in area 5:

    [splitter] CSplitterCommandModule::PreRender / AddToRenderer / GetSortingBounds /
               Render / GetAimPosition          (the head's render-path overrides)
    [splitter] CSplitterMainChassis::CollidedWith
    [splitter] fn_75_2BE0 (0xE0) + the DOL's CSfxManager::UpdateEmitter   (fn_75_3004)
    [splitter] fn_75_10D0 (0xB8) + the DOL's CSfxManager::RemoveEmitter    (fn_75_1188)
    [splitter] fn_75_A4E4 / fn_75_AFE8 / fn_75_ACEC / fn_75_9520          (the chassis's Think)

**fn_75_1188's gate does not pass.** The gate (fn_75_58E8) looks the chassis up by the id at
0xEFA, which the retail's state machine sets and the port's names-only FSM does not, so the
gate is false and the log shows the gate-false path (the DOL's `CSfxManager::RemoveEmitter`).

**fn_75_2CE0 runs, but only up to the collision-actor lookup.** The head's create message
(XCRT) reaches fn_75_3460, which calls fn_75_2CE0 - but the head's only joint is
"Skeleton_Root", so the id at 0xF00 stays invalid, the lookup gives nothing and the body
returns at 0x2D88. The three states cannot run in the port yet: the state at 0xEE0 only
advances when the flag byte 0xF6A's 0x08 is set (the retail's "RaiseShields" code sets it),
and the port's FSM is names-only.

#### Who sets the head's chassis id at 0xEFA (read 2026-10-20)

The only store of a *valid* id at 0xEFA is **fn_75_3EEC** (0x3EEC, 0xF4) - the head's "dock to
the chassis". The other two stores are the invalid id (the ctor at 0x6C44 and the clear in
fn_75_31A8 at 0x3240). fn_75_3EEC takes the id at **0xEFC** (the connected object the XAND
message found), looks it up as a main chassis (0x3F18) and asks it to accept this head
(0x3F3C **fn_75_E6E4**, 0x84: with the chassis's own command module id (0xC6C) still the invalid
one the head's id goes there, 0xC6E is cleared, the flag byte 0xD40's 0x40 bit is cleared, the
materials 0x28 and 0x29 are removed and **fn_75_8EA4** copies four of the chassis's data
timers into 0xD10-0xD1C). When the chassis accepts, the id moves to 0xEFA (0x3F50), 0xEFC is
cleared and the id goes to 0x12A; the docking sound (the id at 0x88C) is added when the flag
byte 0xF6A's 0x02 bit and the data's flag word 0x7C0's 0x80 bit are both clear.

**fn_75_3EEC is the head's `NotifyDocking` code** (§b): the module's static initialiser puts it
in the head's code table, so the state machine runs it by name. The port binds that code to the
ported body (`PortDockToChassis`) and does not call it from the Think any more.

**The gate still fails in the area-4 fight**: the head has **no "Connect/Attach" connection**
there - its connection list is only Dead/SetToZero, Patrol/Follow and DeathRattle/Decrement
(the *chassis* has the Connect/Attach one, to the player hint) - so fn_75_3A8C leaves 0xEFC
invalid, fn_75_3EEC finds no chassis and 0xEFA stays invalid. The area's placement does not
wire the head to its chassis; the fight that does is elsewhere.

#### The shells the run logs (2026-10-20, after the fn_75_2BE0/fn_75_10D0 port)

Ported: **fn_75_2BE0** (the shield's particle effect: the state at 0xEE0 picks "LightShield" or
"DarkShield", the animation data's first particle effect with that name is the result; the
state 0 gives nothing) and **fn_75_10D0** (the module's own transform: the actor's own
transform with the "Beam_LCTR" locator's translation moved through it - it runs every frame
from the beam-effect spawner, so it is marked).

Still shells in that run: the DOL's `CSfxManager::UpdateEmitter` and `RemoveEmitter` (the head's
collision update and the beam spawner), and the chassis's **fn_75_A4E4** (0x418), **fn_75_AFE8**
(0x11C), **fn_75_ACEC** (0x2FC) and **fn_75_9520** (0x11C) - the chassis's Think.

#### The chassis's Think helpers (fn_75_AFE8, fn_75_9520) - ported 2026-10-20

The chassis's Think (fn_75_F0E8, 0xF0E8) calls four module functions with the frame time in f1
and the state manager in r4: **fn_75_A4E4** (0x418, the collision actor manager, 0xF130),
**fn_75_AFE8** (0x11C, 0xF164), **fn_75_ACEC** (0x2FC, 0xF174) and **fn_75_9520** (0x11C,
0xF180).

**fn_75_AFE8 (0x11C)** - the chassis's timers. The four timers the chassis copies from its data
(fn_75_8EA4) are counted down and the scan timer at 0xD28 too; three timers are counted up:

| field | direction | asm |
|---|---|---|
| 0xD10 / 0xD14 / 0xD18 / 0xD1C | down | 0x0000AFF8, 0x0000B004, 0x0000B010, 0x0000B01C |
| 0xD28 | down | 0x0000B028 |
| 0xD34 / 0xD30 / 0xD2C | up | 0x0000B034, 0x0000B040, 0x0000B04C |

The timer at **0xD3C** is counted up only while the id at **0xCC0** is valid (0x0000B058 `lhz`
against kInvalidUniqueId) and the counter at **0xC48** is not zero (0x0000B068); otherwise it is
cleared (0x0000B084, `lbl_75_rodata_390`, 0.0f). The flag byte at **0xD41**'s 0x01 bit (0x0000B094
`clrlwi` with the word's bit 31 - the byte's 0x01) and the alive bit of the byte at **0x420**
(0x0000B0A0 `extrwi` with the word's bit 25 - the byte's 0x40, the host's `GetAlive()`) gate the
death: the timer at **0xD20** is counted down and, at zero or below (0x0000B0C0 `fcmpo` with the
`cror eq, lt, eq` - the <= sense), the chassis dies with the transform's Y column (the vector at
0x28, 0x38 and 0x48) and the state 'RATL' (0x0000B0E0). The retail calls the base class's
`Death`, not the module's override.

**fn_75_9520 (0x11C)** - the chassis's fade. The id at 0xCC0 picks the rate: a valid id gives
`lbl_75_rodata_39C` (0.25f) and the invalid one `lbl_75_rodata_364` (1.0f), and the step is the
frame time divided by it (0x0000956C `fdivs`). While the scan timer at 0xD28 is above zero or the
id is valid, the weight at **0xCB8** is counted down and clamped at zero; otherwise it is counted
up and clamped at one (0x000095C4, the >= sense). The weight above zero adds the additive
animation with the id at **0xCB4** (0x000095FC, the two flags clear); at zero or below the
animation is deleted when it is active (0x00009610 `clrlwi` with the word's bit 24 - the byte's
0xFF).

Both are ported as `CSplitterMainChassis::PortUpdateTimers` and
`CSplitterMainChassis::PortUpdateFade`; the new fields are 0xC48, 0xCB4, 0xCB8, 0xD20, 0xD30,
0xD34 and 0xD3C.

**fn_75_ACEC (0x2FC)** and **fn_75_A4E4** (0x418) are still shells. fn_75_ACEC is the head's
placement: with the command module's id at **0xC6C** valid (0x0000AD18 `lhz`) it looks the head
up (0x0000AD40 `ObjectById`, 0x0000AD44 the `CSplitterCommandModule` cast) and builds the head's
transform from the locator's transform (0x0000AD5C `GetLctrTransform`); with the flag byte at
**0xD41**'s 0x80 (0x0000AD88 `extrwi` with the word's bit 24 - the byte's 0x80) and 0x08 bits
(0x0000AD90, the word's bit 28 - the byte's 0x08) clear and the counter at **0xD00** equal to 5
(0x0000AD98) it looks the head at the target (0x0000AEC0, the offset at 0xF24-0xF2C), otherwise
it slerps the head's facing toward the chassis's (0x0000AE30 `GetAngleDiff`, 0x0000AE60
`Slerp`, the turn rate 12.566371 rad/s) and sets the transform (0x0000AE44, 0x0000AED4,
0x0000AF4C). With no head at 0xC6C the id is cleared (0x0000AF74), the materials 0x28 and 0x29
are added (0x0000AF84) and the flag byte at 0xD41's 0x02 bit is set; the id at **0xC6E** is
cleared when it names no head (0x0000AF8C-0x0000AFC0). fn_75_A4E4 (0x418) builds the collision
actor manager.

#### The state machine entries the area-4 run reaches (2026-10-21)

The run (`/tmp/opencode/smoke/split_run.sh`, LAYER3 + NEXTAREA, the console driver that activates
the two Splitters) asks the state machine for these entries, in this order. Each one is ported
now, so the log no longer shows it:

| # | entry | fn | ported as |
|---|---|---|---|
| 1 | the head's Start state | `fn_75_5564` | `CSplitterCommandModule::Start` (the body is one `blr`) |
| 2 | the head's IsInitiallyDocked trigger | `fn_75_58DC` | `IsInitiallyDocked` (the SPLU block's first word, bit 0) |
| 3 | the head's IsDocked trigger | `fn_75_58BC` | `IsDocked` (the chassis id at 0xEFA is valid) |
| 4 | the head's HasDockingTarget trigger | `fn_75_57F0` | `HasDockingTarget` (the chassis at 0xEFC: alive, active, no command module) |
| 5 | the chassis's Start state | `fn_75_DFE8` | `CSplitterMainChassis::Start` (the body is one `blr`) |
| 6 | the chassis's IsDisabled trigger | `fn_75_E620` | `IsDisabled` (the flag byte 0xD40's 0x40) |
| 7 | the chassis's DeployFromHoldingTube trigger | `fn_75_E034` | `DeployFromHoldingTube` (the data's flags at 0x828, bit 1) |
| 8 | the chassis's Patrol state | `fn_75_DCFC` | still a shell |
| 9 | the head's RaiseShields code | `fn_75_3B20` | `RaiseShields` (the flag byte 0xF6A's 0x08) |
| 10 | the head's Hover state | `fn_75_4D04` | still a shell |
| 11 | the chassis's Scanning state | `fn_75_DCF8` | `Scanning` (the body is one `blr`) |
| 12 | the chassis's ShotAt trigger | `fn_75_E5B8` | `ShotAt` (the base's flag byte 0x420's 0x80) |
| 13 | the chassis's PatrolPathOver trigger | `fn_75_E478` | `PatrolPathOver` (the waypoint destination at 0x6C0) |
| 14 | the chassis's LostHead trigger | `fn_75_E5AC` | `LostHead` (the flag byte 0xD41's 0x02) |
| 15 | the chassis's ShouldWaitForSnatch trigger | `fn_75_E5D0` | `ShouldWaitForSnatch` (the id at 0xCBC or `IsBeingSnatched`) |
| 16 | the chassis's PrepareForDocking trigger | `fn_75_DFEC` | `PrepareForDocking` (the second link at 0xC6E) |
| 17 | the head's SelectTarget code | `fn_75_3E60` | still a shell |
| 18 | the head's HasTarget trigger | `fn_75_57B4` | `HasTarget` (the synced id at 0xEFE) |

The chassis's `fn_75_3B20` is the head's code, so it is the head's RaiseShields; the order above is
the log's.

**The port's own limits in these bodies**: the head's `HasDockingTarget` needs a chassis at 0xEFC
(the head's second link), which the area-4 placement does not wire - so it is false in the run.
The chassis's `ShouldWaitForSnatch` needs the id at 0xCBC, which the retail's `fn_75_E810`
(`SetAttackTarget`, still a shell) keeps - so only the `IsBeingSnatched` half can run.

#### fn_75_AFE8 and fn_75_9520: the branches the run does not reach (2026-10-21)

The two bodies are ported, but the area-4 run does not exercise every branch, so they carry no
`// @` marker. The probes (a temporary log at the end of each branch, reverted after the run)
gave this:

| fn | branch | ran? | what it needs |
|---|---|---|---|
| `fn_75_AFE8` | 0xD3C counted up (0x0000B074) | no | the id at 0xCC0 valid **and** the counter at 0xC48 not zero - the REL never writes 0xC48, so this branch is dead in the retail too |
| `fn_75_AFE8` | 0xD3C cleared (0x0000B084) | yes (7499 frames) | - |
| `fn_75_AFE8` | the death timer 0xD20 counted down (0x0000B0B0) | no | the flag byte 0xD41's 0x01 bit, which the retail's `fn_75_ACEC` sets (still a shell) |
| `fn_75_AFE8` | the death call (0x0000B0F0) | no | the same bit, and the timer at 0xD20 at zero |
| `fn_75_9520` | the fade counted down (0x0000958C) | no | the scan timer at 0xD28 above zero - the retail's ACTN message sets it, and that message comes from the AFSM script, not from a state body (the chassis's Scanning state `fn_75_DCF8` is a single `blr`) |
| `fn_75_9520` | the fade counted up (0x000095B4) | yes (7499 frames) | - |
| `fn_75_9520` | the additive animation added (0x000095FC) | yes (7499 frames) | - |
| `fn_75_9520` | the additive animation deleted (0x00009618) | no | the fade weight at zero, which needs the fade-down branch |

So the next port that would close the death gap is the chassis's `fn_75_ACEC` (the head's
placement, which sets the flag byte 0xD41's 0x01). The fade-down gap needs the AFSM script to
send the ACTN message that starts the scan timer at 0xD28: the chassis's Scanning state
`fn_75_DCF8` is a single `blr` (ported 2026-10-21) and does not send it.
