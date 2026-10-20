# EmperorIngStage3 REL (module 18): analysis for a host body

Analysis and milestone-1 port of the REL `EmperorIngStage3` (the mutated Emperor Ing, the boss of
the Great Temple's moth lord sanctuary, dark, layer 4). Sources: the REL's disassembly
(`../MetroidPrime2Port/build/G2ME01/EmperorIngStage3/asm/`), the pseudo-C in
`/tmp/opencode/pseudo/EmperorIngStage3/`, `docs/modules.tsv` and the sibling Stage 1 port
(`docs/module-analysis/EmperorIngStage1.md`). All `fn_18_X` names are REL `.text` offsets.

## 1. Module shape

| item | value | evidence |
|---|---|---|
| class size | 0xED0 | the loader's `rs_new(0xED0, "CEmperorIngStage3")` at 0xC598 |
| loader | `fn_18_C380` | reads the four SLdr properties and builds the actor |
| constructor | `fn_18_9A8C` | the vtable store is at 0x9B04 (`lbl_18_data_4F0`) |
| vtable | `lbl_18_data_4F0` (80 slots, 21 local) | the slot order matches CPatterned's declaration order |
| module init | `fn_18_C350` | stores the loader in `lbl_18_bss_30` and calls the DOL's `SetSEmperorIngStage3_FuncPtrs` |
| base ctor | EPatternedAI 0x0F, flavor 0, movement 0 (ground), collider 1, body 1 (bipedal) | `li r4, 0xf` at 0x9AF0, `li r7, 0x0` at 0x9AC8, the stack arguments at 0x9AD8..0x9AE8 |

The loader reads `SLdrEmperorIngStage3` (upstream generates it): the editor properties, the
patterned AI typedef, the actor parameters and the module's own data block. The model comes from
`LdrToModelData(scale, asset, the patterned info's animation set, true)` at 0xC510.

## 2. Vtable overrides (slot -> the module's function)

| slot | method | fn | milestone 1 |
|---|---|---|---|
| 0x08 | the deleting destructor | `fn_18_94C8` | the host destructor |
| 0x14 | Think | `fn_18_47D4` | stand-in, forwards to CPatterned |
| 0x18 | AcceptScriptMsg | `fn_18_7B14` | stand-in, forwards to CPatterned |
| 0x24 | PreRender | `fn_18_4650` | stand-in (the base's pre-render plus the damage flash) |
| 0x2C | Render | `fn_18_47B4` | stand-in, forwards to CPatterned |
| 0x3C | GetHealthInfo | `fn_18_B4` | real (a call through the vtable slot 0x38) |
| 0x48 | GetTouchBounds | `fn_18_8` | real (the actor's bounding box in an optional) |
| 0x54 | GetAimPosition | `fn_18_8E84` | stand-in, forwards to CPatterned |
| 0x6C | GetSortingBounds | `fn_18_45B4` | stand-in, forwards to CPatterned |
| 0x70 | DoUserAnimEvent | `fn_18_9048` | stand-in, forwards to CPatterned |
| 0xA4 | TakeDamage | `fn_18_9044` | real (the retail body is a single `blr`) |
| 0xA8 | CanBeShot | `fn_18_90` | real (`li r3, 0x1`) |
| 0xAC | IsListening | `fn_18_0` | real (`li r3, 0x1`) |
| 0xB0 | Listen | `fn_18_8E50` | real (a player fire sets the flag byte at 0x824) |
| 0xB4 | GetOrigin | `fn_18_98` | stand-in, forwards to CPatterned |
| 0xC8 | GetModelAlphau8 | `fn_18_44` | real (the byte at 0x44F) |
| 0xDC | ProjectileInfo | `fn_18_4C` | real (null) |
| 0xE0 | GetSearchPath | `fn_18_54` | real (null) |
| 0xE4 | GetPointSearchPath | `fn_18_5C` | real (null) |
| 0xF0 | SetupStateMachine | `fn_18_943C` | stand-in, forwards to CPatterned |
| 0xF4 | GetRagDoll | `fn_18_64` | real (null) |
| 0x10C | GetAttackTarget | `fn_18_6C` | real (kInvalidUniqueId) |
| 0x110 | IsOnGround | `fn_18_7C` | real (the base's mOnGround) |
| 0x114 | GetGravityConstant | `fn_18_8E78` | real (rodata 0x2A4 = 200.0f) |

The inherited slots are spelled out by name in the same vtable (TypesMatch, PreThink, SetActive,
HealthInfo, the two GetDamageVulnerability overloads, Touch, Death, KnockBack, the render systems,
Freeze, ThinkAboutMove, Burn, Shock, MassiveDeath, ...).

## 3. Member map (guest offsets, from `fn_18_9A8C`)

| offset | member | proof |
|---|---|---|
| 0x44F | the model alpha byte | `fn_18_44` reads it; the constructor does not write it, so it starts 0 |
| 0x814 | the vulnerable damage threshold | the constructor copies the data's `vulnerableDamageThreshold` (the data's +0x18) |
| 0x824 | the noise flag byte | `fn_18_8E50` sets its bit 30 (the mask 0x02) when the actor hears the player fire; the constructor clears the word's high bits |
| 0x91C | the taunt frequency | the constructor copies the data's `tauntFrequency` |
| 0x920 | the yellow health | `LdrToHealthInfo` of the data's `yellowHealth` |
| 0x940 | the health | `LdrToHealthInfo` of the data's `health` |
| 0x960 / 0x990 / 0x9C0 | the red / light / dark vulnerabilities | `LdrToDamageVulnerability` of the data's three |
| 0x9F0 / 0xA0C / 0xA28 / 0xA44 | the melee / stampede / jump slide / ground pound damage | `LdrToDamageInfo` of the data's four |
| 0xA60, 0xAD4 | two projectile infos | the attacks, milestone 2 |
| 0xA88, 0xAFC | the two beam infos | the attacks, milestone 2 |
| 0xB54, 0xBC0, 0xC2C, 0xC98 | the four stage records (0x6C each) | the data's `stage1`..`stage4`, milestone 2 |
| 0xD34 | the light swarm | milestone 2 |
| 0xE90 | the portal attack data | milestone 2 |

The constructor also ends with `SetDrawShadow(this, 0)` (0xA2D0).

## 4. The state machine (milestone 2)

`fn_18_943C` (the vtable slot 0xF0) calls `CPatterned::SetupStateMachine` first and then hands
three name tables to the machine: the triggers (`lbl_18_data_34C`, 21 entries) through the
machine's vtable slot +0x18 (0x9470), the states (`lbl_18_data_100`, 21 entries) through +0x14
(0x9490) and the codes (`lbl_18_data_4B4`, 2 entries) through +0x1C (0x94B0). Each table holds
28 three-word entries; the ones the machine uses carry the node name, the rest are empty.

| table | the names it holds |
|---|---|
| triggers (21) | TooClose, ShouldTaunt, ShouldChangeVulnerabilityToVulnerable, ShouldLightBeamAttack, ShouldGenerateDarkFliers, ShouldGrowHeart, AbortedJumpSlide |
| states (21) | Start, GainDistance, Taunt, DarkBeamAttack, GroundPound, JumpSlideAttack, JumpAttack |
| codes (2) | ResetAttackTimer |

The function pointers live in the module's `.data` records and are written by a static initialiser,
so the port registers its own tables when the FSM is ported.

## 5. Runtime (milestone 1)

Great Temple world `863fcd72`, area 11 (`07_temple_mothlordsanctuary_dark`), layer 4: the actor
registers, takes kSM_Create and runs Think every frame. The run's log shows the module's stand-ins
firing (`[emp3] ... Think`, `AcceptScriptMsg`, `PreRender`, `DoUserAnimEvent`,
`GetSortingBounds`, `SetupStateMachine`), `grep -c '[port] caught\|reach-stub'` = 0 and the run
ends on the frame limit (3200 frames, 45 s).
