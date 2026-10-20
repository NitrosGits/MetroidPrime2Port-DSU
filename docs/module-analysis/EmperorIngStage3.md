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
28 four-word entries {name, 0, 0xFFFFFFFF, fn}; the machine uses the first 21 (or 2) and the rest
are empty. The static initialiser `fn_18_E180` copies the function pointers out of the .data
records {0, 0xFFFFFFFF, fn}: the states' twenty-one at 0x4..0xF4, the triggers' twenty-one at
0x250..0x344 and the codes' two at 0x49C..0x4A8.

| table | entry -> the retail function |
|---|---|
| triggers (lbl_18_data_34C) | TooClose `8DA4`, ShouldAttack `8D88`, ShouldMeleeAttack `8C4C`, ShouldTaunt `8A4C`, ShouldStampede `897C`, ShouldDodge `8A34`, ShouldChangeVulnerabilityToVulnerable `8A20`, ShouldChangeVulnerabilityToInvulnerable `898C`, ShouldDarkBeamAttack `8968`, ShouldLightBeamAttack `8954`, ShouldLightSwarmAttack `87F0`, ShouldGroundPound `8ACC`, ShouldGenerateDarkFliers `868C`, ShouldPortalAttack `8678`, ShouldJumpSlideAttack `8664`, ShouldGrowHeart `8648`, JumpSlideAttackOver `8600`, ShouldEyeShut `85A4`, AbortedJumpSlide `858C`, ShouldJumpAttack `8518`, ShouldStampedeReact `8500` |
| states (lbl_18_data_100) | Start `7ADC`, Idle `788C`, MeleeAttack `76D0`, GainDistance `7504`, Stampede `71D8`, Dodge `6E80`, Taunt `6D84`, ChangeVulnerability `6A40`, EyeShutResetVulnerability `6B80`, DarkBeamAttack `6904`, LightBeamAttack `67C0`, LightSwarmAttack `6698`, GroundPound `6390`, GenerateDarkFliers `6258`, PortalAttack `60AC`, JumpSlideAttack `5844`, GrowHeart `5614`, EyeShut `5264`, JumpAttack `4EB4`, StampedeReact `4CC8`, Dead `4C3C` |
| codes (lbl_18_data_4B4) | ResetAttackTimer `84A8`, ChoseAction `8200` |

The port registers the three tables with these names and the module's own member functions; the
eleven triggers whose retail body is 0x40 bytes or less are ported:

| trigger | the retail body |
|---|---|
| ShouldAttack `8D88` | the attack timer (0x7E4) against zero (rodata 0x258) |
| ShouldStampede `897C` | the action (0x7E8) is 0 |
| ShouldDodge `8A34` | the flag word 0x824's bit 28 (the mask 0x08) is set |
| ShouldChangeVulnerabilityToVulnerable `8A20` | the vulnerability state (0x7C4) is 1 |
| ShouldDarkBeamAttack `8968` | the action (0x7E8) is 3 |
| ShouldLightBeamAttack `8954` | the action (0x7E8) is 4 |
| ShouldPortalAttack `8678` | the action (0x7E8) is 5 |
| ShouldJumpSlideAttack `8664` | the action (0x7E8) is 1 |
| ShouldGrowHeart `8648` | the vulnerable damage threshold (0x814) against rodata 0x310 (1.1920929e-07f) |
| AbortedJumpSlide `858C` | the flag word 0x824's bit 29 (the mask 0x400) is set |
| ShouldStampedeReact `8500` | the flag word 0x824's bits 8..14 (the mask 0x7F00) are not zero |

The other thirty-three FSM functions are stand-ins that log the retail function they stand in for.

### The functions the run reaches

| fn | size | the retail body |
|---|---|---|
| `7ADC` Start | 0x38 | on the deactivation message (2) it calls `fn_18_1DF4` (0xC4, a stand-in) |
| `8DA4` TooClose | 0xAC | the module's distance (0x7D4) must be at least 2.0 (rodata 0x274) and the player nearer than 20 units (rodata 0x270); a vector that cannot be normalized counts as too close. The vector comes from `fn_18_2D1C` (0x34, the player minus the actor, flattened to XY); `fn_18_2C98` (0x20) copies a vector retail then never reads, so it is left out |
| `8A4C` ShouldTaunt | 0x80 | the random check `fn_18_F8` (0x11C, a stand-in), then `fn_18_2298` (0x24, a stand-in: its own callee `fn_18_AB9C` walks the bone list at 0x840, which the port does not model), then the taunt timer (0x7C8) past the taunt frequency (0x91C) |
| `898C` ShouldChangeVulnerabilityToInvulnerable | 0x94 | the random check; the vulnerable state (3) is true, the invulnerable state (2) needs the damage taken while vulnerable (0x810) below zero (rodata 0x258) or the vulnerable damage threshold (0x814) below rodata 0x310 (1.1920929e-07f) |
| `85A4` ShouldEyeShut | 0x5C | the random check and the eye-shut timer (0x7CC) below 8.0 (rodata 0x2A8) |
| `8518` ShouldJumpAttack | 0x74 | the random check, the jump-attack timer (0x7E0) at least 2.5 (rodata 0x298) and the flag byte 0x824's bit 29 (the mask 0x04) |

The three the run reaches that are too big for this slice stay stand-ins: `788C` Idle (0x250),
`8C4C` ShouldMeleeAttack (0x13C) and `8ACC` ShouldGroundPound (0x180).

### The flag bytes

The retail keeps a 32-bit word at guest 0x824, but every test the module makes is a single byte's
bit, so the host keeps the two bytes: `x824_` (0x824: the listen flag 0x02, the jump-attack flag
0x04, the dodge flag 0x08) and `x825_` (0x825: the stampede-react flag 0x01, the aborted
jump-slide flag 0x04). The masks are the asm's: `extrwi` with the bit 28 or 29 is the mask 0x08 or
0x04 (PowerPC bit N is 1u << (31-N)), and the bit 0 is the mask 0x01.

## 5. Runtime

Great Temple world `863fcd72`, area 11 (`07_temple_mothlordsanctuary_dark`), layer 4: the actor
registers, takes kSM_Create and runs Think every frame. The run's log shows the module's stand-ins
firing (`[emp3] ... Think`, `AcceptScriptMsg`, `PreRender`, `DoUserAnimEvent`,
`GetSortingBounds`, `Render`), `grep -c '[port] caught\|reach-stub'` = 0 and the run ends on the
frame limit (3200 frames, 45 s).

With the state machine registered (milestone 2) the run also reaches the FSM: the states entered
are **Start** (`fn_18_7ADC`, whose deactivation handler reaches the stand-in `fn_18_1DF4`) and
**Idle** (`fn_18_788C`); the triggers the run evaluates are TooClose `8DA4`, ShouldMeleeAttack
`8C4C`, ShouldGroundPound `8ACC`, ShouldTaunt `8A4C`,
ShouldChangeVulnerabilityToInvulnerable `898C`, ShouldEyeShut `85A4` and ShouldJumpAttack
`8518` (the ported ones answer silently and log only the random check `fn_18_F8`).
