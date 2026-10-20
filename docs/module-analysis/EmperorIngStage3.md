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
| 0x14 | Think | `fn_18_47D4` | ported (the base's Think, the stampede-react check, the timers, the melee contact and the knock-back; four per-frame calls and the sound are stand-ins) |
| 0x18 | AcceptScriptMsg | `fn_18_7B14` | partly ported (see section 4) |
| 0x24 | PreRender | `fn_18_4650` | stand-in (the base's pre-render plus the damage flash) |
| 0x2C | Render | `fn_18_47B4` | stand-in, forwards to CPatterned |
| 0x3C | GetHealthInfo | `fn_18_B4` | real (a call through the vtable slot 0x38) |
| 0x48 | GetTouchBounds | `fn_18_8` | real (the actor's bounding box in an optional) |
| 0x54 | GetAimPosition | `fn_18_8E84` | stand-in, forwards to CPatterned |
| 0x6C | GetSortingBounds | `fn_18_45B4` | stand-in, forwards to CPatterned |
| 0x70 | DoUserAnimEvent | `fn_18_9048` | ported (slice 5); `4D0`/`BB8`/`1530` stand-ins |
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
| 0x6B4 | `CPatterned::mAnimationState` | the base class's animation state (`CAnimationState`): the module's states store 0 (`kAS_NotReady`), 1 (`kAS_Ready`) and 3 (`kAS_Over`) there (0x5368, 0x5538, 0x7564, 0x759C, 0x76B0, 0x4F14, 0x50A8) |
| 0x7C0 | the `CCollisionActorManager*` | `fn_18_3BB0` makes it on kSM_AreaLoaded (ten leg OBBs, then the spheres mainHeart_SDK, tentacle_6_1 and Skeleton_Root); `fn_18_3B14` reads its descriptions 0xA (the first heart) and 0xB (the second); kSM_Delete destroys it; the constructor clears it |
| 0x7D0 | the time since the last attack | the module's Think grows it every frame (0x494C); the melee and ground-pound triggers want it at least 2.0 |
| 0x814 | the vulnerable damage threshold | the constructor copies the data's `vulnerableDamageThreshold` (the data's +0x18) |
| 0x818 | the facing the idle state turns towards the player | `fn_18_3804` writes it; the constructor zeroes it (0x9B80) |
| 0x824 | the noise flag byte | `fn_18_8E50` sets its bit 30 (the mask 0x02) when the actor hears the player fire; the constructor clears the word's high bits |
| 0x864 | the idle state's timer | `fn_18_788C` sets it to FLT_MAX on enter, clears it when the player is over fifty units away and grows it by the frame time |
| 0x868 | the heart that is on | `fn_18_1DF4` writes it; the constructor sets it to -1 (0x9CB4), so Start's heart switch sees a change and sets the hearts' health |
| 0x8A4 | the dodge's step direction | `fn_18_6E80` picks `kSD_Left` 2, `kSD_Right` 3 or `kSD_Down` 5 (-1 none); the constructor sets -1 |
| 0x86C | the hearts' vulnerability | the constructor copies the immune vulnerability into it (0x9CB8..0x9CC4); `fn_18_1BD8` gives it to both hearts |
| 0x830 / 0x840 / 0x850 | the module's three lists | the count at +0x4, the entries at +0xC, each entry 0xC bytes (a weight at +0x0, a threshold at +0x4, a timer at +0x8). The first is the attack list (`fn_18_2BF0` fills it), the second is the one the taunt average reads and the third the one the flag check reads |
| 0x8C0 | the id of the object the module watches | `fn_18_125C` clears it when the object is gone (0x12AC) |
| 0x158 | `CPhysicsActor::mMass` | the host's `GetMass()`: the off-ground force is the gravity constant times it, negated (0x7FA4..0x7FC0) |
| 0x1C0 | `CPhysicsActor::mConstantForce` | the host's `SetConstantForceWR()`: the message handler's `kSM_OffGround` writes it (0x7FB0..0x7FC0) |
| 0x158 | a CPatterned field the module reads | the off-ground speed: the off-ground direction is the gravity constant times it, negated (0x7FA4..0x7FC0) |
| 0x1C0 | the off-ground direction | the message handler's `kSM_OffGround` writes it (0x7FB0..0x7FC0) |
| 0x8EC | the eye-shut time | the EyeShut state's activate reads it (0x52BC) and clears it |
| 0x8F0 | the eye-shut duration | the EyeShut state's activate computes it (0x52F4) and the update counts it down |
| 0x8F4 | the eye-shut random time | the EyeShut state's activate rolls it (0x5338) and the update counts it down |
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

The FSM functions not named in this note (or in its slice-2 list in section 5) are stand-ins that log the retail function they stand in for.

### The functions the run reaches

| fn | size | the retail body |
|---|---|---|
| `7ADC` Start | 0x38 | on the deactivation message (2) it calls `fn_18_1DF4` with the enraged heart (0) and the flag set |
| `8DA4` TooClose | 0xAC | the module's distance (0x7D4) must be at least 2.0 (rodata 0x274) and the player nearer than 20 units (rodata 0x270); a vector that cannot be normalized counts as too close. The vector comes from `fn_18_2D1C` (0x34, the player minus the actor, flattened to XY); `fn_18_2C98` (0x20) copies a vector retail then never reads, so it is left out |
| `8A4C` ShouldTaunt | 0x80 | the look check `fn_18_F8` (0x11C) with 60.0 (rodata 0x2E4), then `fn_18_2298` (0x24, a stand-in: its own callee `fn_18_AB9C` walks the bone list at 0x840, which the port does not model), whose answer has to be above 50.0 (rodata 0x290), then the taunt timer (0x7C8) past the taunt frequency (0x91C) |
| `898C` ShouldChangeVulnerabilityToInvulnerable | 0x94 | the look check with 90.0 (rodata 0x2CC); the vulnerable state (3) is true, the invulnerable state (2) needs the damage taken while vulnerable (0x810) below zero (rodata 0x258) or the vulnerable damage threshold (0x814) below rodata 0x310 (1.1920929e-07f) |
| `85A4` ShouldEyeShut | 0x5C | the look check with 90.0 (rodata 0x2CC) and the eye-shut timer (0x7CC) above 8.0 (rodata 0x2A8): the retail takes the compare's GT bit (the mfcr and the extrwi at 0x85E0), so the timer has to have grown past eight seconds |
| `8518` ShouldJumpAttack | 0x74 | the look check with 35.0 (rodata 0x30C), the jump-attack timer (0x7E0) at least 2.5 (rodata 0x298) and the flag byte 0x824's bit 29 (the mask 0x04) |
| `8C4C` ShouldMeleeAttack | 0x13C | the time since the last attack (0x7D0) at least 2.0 (rodata 0x274), the module's average over the bone list (`fn_18_22BC`, 0x24, a stand-in: its own callee `fn_18_AB9C` walks the list at guest 0x830, which the port does not model) at least 10.0 (rodata 0x294); the vector to the player (`fn_18_2D1C`) has to be normalisable and shorter than 30.0 (rodata 0x2FC), and the player has to be in front of the actor: the dot of the actor's flattened facing (`fn_18_2C98`) and the direction to the player above the cosine of 1.3089969 (rodata 0x318, the retail takes the cosine of the constant at 0x8D40 and compares the dot with it) |
| `8ACC` ShouldGroundPound | 0x180 | the time since the last attack (0x7D0) at least 2.0 (rodata 0x274); the vector from the ing's "head" locator (rodata 0x704) to the player has to be normalisable, and the player has to be below the ing: the dot of the normalised direction to the player and the ing's down axis (the negated Z column of the transform) above the cosine of 0.7853982 (rodata 0x314, the cosine of the constant at 0x8C04) |

Both triggers the run reaches are ported now, `fn_18_8A4C`'s taunt average included
(`fn_18_2298`, the list at guest 0x840).

### `fn_18_5264` (0x2F8), the EyeShut state

- Activate (0x52B0): the eye-shut time. The module's time (0x8EC) times two percent (rodata
  0x274) over a hundred (rodata 0x27C), at most 2.5 (rodata 0x298), goes to 0x8F0 and the
  module's time is cleared; a random time between one (rodata 0x264) and 1.2 (rodata 0x2EC) times
  that, at most 5.0 (rodata 0x2BC), goes to 0x8F4; the eye-shut time then is at least 2.5
  (rodata 0x298) plus that random time; the animation state at 0x6B4 is set to `kAS_Ready` (1).
- Update (0x5370): the body's eye-shut commands - the three commands the module's own command
  classes (the vtables at `lbl_18_data_744`, `_750` and `_78C`) carry, the flag's state machine
  (`fn_801B73E4`, which watches the body's animation) and the base member at guest 0x350. A
  stand-in.
- Deactivate (0x5518): the eye-shut timer (0x7CC) is cleared, both hearts get the module's
  vulnerability at 0x86C (`fn_18_1BD8`, 0x5C) and the heart switch runs with the heart that is on
  (`fn_18_1BB0`, 0x28, which is `fn_18_1DF4` with the heart at 0x868 and no vulnerability
  change); the animation state at 0x6B4 is set to `kAS_NotReady` (0).

### The look check `fn_18_F8` (0x11C)

The name says "random check", but the body has no random: the parameter (f1) is the angle in
degrees the player may be off the actor's facing by. The vector from the actor's
`DarkBeam_LCTR_SDK` locator (`fn_18_DBC`, 0x40: the actor's transform times the model's scaled
locator transform, rodata 0xA8) to the player is normalised, flattened to XY (rodata 0x258) and
has to be normalisable; the angle between it and the actor's facing (the transform's Y column,
guest 0x28/0x38/0x48) is turned into degrees (rodata 0x25C = 57.295776) and has to be below the
parameter. The triggers that use it pass 60.0 (0x8A4C), 90.0 (0x898C, 0x85A4) and 35.0 (0x8518).

### `fn_18_788C` (0x250), the Idle state

Activate: the timer at guest 0x864 becomes FLT_MAX (rodata 0x2C8) and the body's steering is
set up - the walk speed over the run speed (`GetLocomotionSpeed` with `pas::kLA_Walk` over
`pas::kLA_Run`), the steering blend mode (the write of one at 0x7908) and the steering speed
range (0x7918). Update: the vector to the player; `fn_18_1A54` (0xE0) looks along the actor's
facing for something to hit and `fn_18_3804` (0x25C) turns the actor towards the player and
writes the facing at guest 0x818; over fifty units (rodata 0x290) the timer is cleared
(rodata 0x258); the locomotion command carries the normalised direction to the player when the
look hit and the timer is below half a second (rodata 0x2A0), the zero vector otherwise; then
the timer grows by the frame time.

#### `fn_18_1A54` (0xE0), the look

The ray starts at the furthest point of the actor's collision box along the facing (the actor's
own transform, guest 0x24, through the vtable slots 0x7C and 0xC) and runs the length the
caller gives; the material filter is the type zero (always) with every material in the include
list. The answer is "the way is clear": the retail reads the result's byte at +0x28 (the
result's mValid, copied by `fn_18_3AC`, 0x20) and answers true when it is zero - so true means
the ray hit nothing.

#### `fn_18_3804` (0x25C), the turn

The facing at guest 0x818 is cleared first (0x382C, 0x3840, 0x3848) and only written at the
end (0x3970..0x3980), so a turn that cannot be done leaves the facing at zero. The angle
between the actor's facing (the transform's Y column) and the direction to the target
(0x38A4) is snapped to the nearest 45 degrees by `fn_18_3744` (0xC0: the table at rodata 0x244
holds 0, pi/4, pi/2, 3pi/4 and pi), signed by the up vector's dot with the cross product of
the two (`fn_18_39A0`, 0x3C, the cross product) and rotated around Z (0x3958). The idle
state's locomotion command then carries that facing as its face vector.

### `fn_18_47D4` (0x3E4), the module's Think

The base class's Think runs first (0x4818). Then:

- The stampede-react check (0x481C..0x4928): only while the flag at guest 0x825 is clear, and
  only when the player is below the ing's head - the vector from the ing's "head" locator
  (rodata 0x704) to the player, normalised, against the ing's down axis (the negated Z column
  of the transform), past 0.70685834 radians (rodata 0x2E8) - sets the flag's bit 0.
- The timers (0x492C..0x4984): the taunt (0x7C8), eye-shut (0x7CC), attack-cooldown (0x7D0),
  too-close (0x7D4) and jump-attack (0x7E0) timers grow by the frame time; the attack timer
  (0x7E4) and the damage taken while vulnerable (0x810) shrink by it.
- The module's per-frame calls (0x4988..0x4A20), in retail order: `fn_18_AC44` (0xB4) three
  times, `fn_18_3B64` (0x28), `fn_18_39E0` (0xA4), `fn_18_3448` (0x1AC), `fn_18_27DC` (0x38C),
  `fn_18_22E0` (0x4C0), `fn_18_1978` (0xDC), `fn_18_1370` (0x1C0), `fn_18_125C` (0x68, ported:
  the module watches the object its id at guest 0x8C0 names and clears the id when the object
  is gone) and `fn_18_E40` (0x41C). `fn_18_125C` and `fn_18_27DC` (the melee contact: the
  three leg locator boxes grown by 2.5 against the player's touch bounds, the melee damage,
  kMS_ApplyJump and the knock-back start) and `fn_18_3448` (the hearts) are ported; `22E0`,
  `1370`, `E40` are stand-ins, so nothing advances the dark-flier timer (0x7D8) and `CanDarkFlier` stays false.
- The player knock-back (0x4A24..0x4A6C): while the timer at 0x8F8 is above zero it runs down
  and the DOL's `fn_801BF3DC` - `CPlayerKnockBackMgr::ApplyPlayerKnockBackForce` - pushes the
  player along 0x8FC normalised, power 100, 0. Ported.
- The ing's sound (0x4A70..0x4BB8): the emitter (the handle at 0x908 and the object at 0x90C)
  follows that object. A stand-in; DoUserAnimEvent's event 39 (`fn_18_9048`) writes them.

### The lists and the Think's per-frame calls

`fn_18_AC44` (0xB4) walks a list: every entry's timer grows by the frame time, and an entry
whose timer is past its threshold is due - `fn_18_ACF8` (0x4C) hands it to `fn_18_AD44` (0xB4, a
stand-in). `fn_18_AB9C` (0x64) is the weighted average over a list: every entry counts its
weight times what is left of its threshold (one minus the timer over the threshold, capped at
one). `fn_18_22BC` (0x24) is that average over the list at guest 0x830 (`addi r3, r3, 0x830`;
an earlier port read 0x840, which is `fn_18_2298`'s taunt list), so the melee trigger
(`fn_18_8C4C`) sees a real number now - the list is empty on the host, so it is zero, and the
trigger stays false. `fn_18_1978` (0xDC) is the flag check over the list at guest 0x850: while
its average is below one (rodata 0x264) a player more than fifty units away (rodata 0x290,
`fn_18_2CB8` is the distance) sets the flag byte 0x824's bit 29 (the mask 0x04), and otherwise a
random draw (0.0 to 100.0, rodata 0x258 and 0x27C) under ten (rodata 0x294) sets that bit and
the bit 28 (the mask 0x08) otherwise.

`fn_18_3B64` (0x28) is the manager's `Update(kUO_ObjectSpace)` and `fn_18_39E0` (0xA4) adds or
removes the target materials on the two hearts; both run in the Think.

### The body-command states

The module's vtables 0x78C/0x750/0x744/0x720/0x72C/0x75C are weak copies of the host's
`CBodyStateCmd` classes, and the DOL's `fn_801B73E4` is `CAnimationState::CanIssueCommand`, so
the states use the host classes and `DeliverCmd`. Ported: EyeShut `5264` (loop attack, additive
reaction 4, exit past the shut time), GainDistance `7504` (a backward step, aimed at the player),
JumpAttack `4EB4` (activate: 0.35 of the way to the player; update: a `kJT_Normal` jump to the
player's translation), Dodge `6E80` (0x358: left/right looks of fifteen units, a step in the
picked direction, the hearts immune while it runs via `fn_18_1C34`, restored by `fn_18_1BD8`),
MeleeAttack `76D0` (0x1B8: a severity-zero melee attack, aimed during the first 0.9 s) and the
list helpers `2BF0` (list 0x830) and `2B68` (list 0x840).

### `fn_18_7B14` (0x668), the message handler

The base class's handler runs first, then one case per message: kSM_Damage, kSM_Create,
kSM_AreaLoaded (0x330: the INTERNAL_STATE_00/01/02 connections into 0x8C2..0x8C8 and the portal
list 0x8CC, `fn_18_3BB0`, the boss health and `SetBossParams` with "BossTempleBossStage3"),
kSM_Delete (the manager's `Destroy`), kSM_Launching (ported: the base's off-ground message, the off-ground force and the jump velocity to 0x910 with the apex 7.5 above the higher end), kSM_HitObject, kSM_OffGround.

### `fn_18_1DF4` (0x4A0), the heart switch

The two heart collision actors come from `fn_18_3ACC` (0x48) through `fn_18_3B14`, the
manager's collision descriptions 0xA and 0xB, looked up by id in the state manager.

The body: heart 0 is the enraged heart (the hearts get the yellow health at guest 0x920 and the
`EnragedHeart` effect), heart 1 the dark heart (the actor's own health, the `DarkHeart`
effect), heart 2 the light heart (the same health, the `LightHeart` effect); the heart that is
on is remembered at guest 0x868 and, when the caller asks for it, the hearts get that heart's
vulnerability (0x960 red, 0x9C0 dark, 0x990 light).

### The flag bytes

The retail keeps a 32-bit word at guest 0x824, but every test the module makes is a single byte's
bit, so the host keeps the two bytes: `x824_` (0x824: the listen flag 0x02, the jump-attack flag
0x04, the dodge flag 0x08, the heart-hit flag 0x80), `x825_` (0x825: the stampede-react flag
0x01, the grow-heart flag 0x02, the aborted jump-slide flag 0x04, the room-check flag 0x08, the
no-contact-damage flag 0x10, the slide-steer flag 0x20, the slide-done flag 0x40) and `x826_`
(0x826: the heart-reacted flag 0x80, the slide's end-time-set flag 0x40). The masks are the asm's: `extrwi` with the bit 28 or 29 is the mask 0x08 or
0x04 (PowerPC bit N is 1u << (31-N)), and the bit 0 is the mask 0x01.

## 5. Runtime

Great Temple world `863fcd72`, area 11 (`07_temple_mothlordsanctuary_dark`), layer 4: the actor
registers, takes kSM_Create and runs Think every frame. The run's log shows the module's stand-ins
firing (`[emp3] ... Think`, `AcceptScriptMsg`, `PreRender`, `DoUserAnimEvent`,
`GetSortingBounds`, `Render`), `grep -c '[port] caught\|reach-stub'` = 0 and the run ends on the
frame limit (3200 frames, 45 s).

With the state machine registered (milestone 2) the run also reaches the FSM: the states entered
are **Start** (`fn_18_7ADC`, whose deactivation handler reaches `fn_18_1DF4`) and
**Idle** (`fn_18_788C`); the triggers the run evaluates are TooClose `8DA4`, ShouldMeleeAttack
`8C4C`, ShouldGroundPound `8ACC`, ShouldTaunt `8A4C`,
ShouldChangeVulnerabilityToInvulnerable `898C`, ShouldEyeShut `85A4` and ShouldJumpAttack
`8518` (the ported ones answer silently).

With the look check, the heart switch and the Idle state ported, a run that walks and fires
after the layer is loaded logs the module's own stand-ins `fn_18_3ACC` (the heart lookup),
`fn_18_1A54` (the look) and `fn_18_3804` (the turn) - the Idle state's own body is retail's - and
the EyeShut state `fn_18_5264` is entered, which means the look check answered true. The run
still ends on the frame limit (2400 updates) with `grep -c '[port] caught\|reach-stub'` = 0.

With the look and the turn ported as well, the idle state measures the ing's angle to the player
(15 degrees in the run) and the 45 degree snap makes that turn a no-op, so the facing at guest
0x818 stays zero - the ing already faces the player - and the FSM walks from Idle into EyeShut.
The heart lookup still finds no actor, because the holder at guest 0x7C0 is the product of the
module's `fn_18_3BB0`, which is still a stand-in.

With the two triggers and the EyeShut path ported, the run's log no longer names
`fn_18_8C4C` or `fn_18_8ACC` (they are retail's now) and names `fn_18_5264 update` instead of
the whole state. The FSM still stayed in EyeShut then: the state's exit trigger reads the
eye-shut timer (guest 0x7CC) and the melee and ground-pound triggers read the time since the
last attack (guest 0x7D0), and only the module's Think (`fn_18_47D4`, 0x3E4, still a stand-in)
grows those timers.

With the Think ported the timers grow, and the state walk starts: the attack timer (guest
0x7E4) shrinks below zero, `ShouldAttack` fires and the FSM leaves Idle for the GainDistance
state (`fn_18_7504`), and the log names the module's own per-frame calls (`fn_18_AC44` and the
rest), the `ChoseAction` code (`fn_18_8200`) and the `ResetAttackTimer` code (`fn_18_84A8`).
The EyeShut state is not reached in that run: its trigger wants the eye-shut timer past eight
seconds *and* the player within ninety degrees of the ing's facing, and the FSM had left Idle
by then.

With the lists ported the walk goes on: the FSM leaves Idle for the GainDistance state
(`fn_18_7504`, whose activate and deactivate paths are ported) and then for the JumpAttack
state (`fn_18_4EB4`, a stand-in). The ing itself does not move: the GainDistance state's update
path - the steering - is still a stand-in. The run ends on the frame limit with
`grep -c '[port] caught\|reach-stub'` = 0.

With the collision actors, AreaLoaded and the body-command states ported (world `863fcd72`,
area 11, layer 4, player at `tp 2 8 88`): the ing (u0582) jumps at the player, then the FSM
cycles JumpAttack -> Idle -> Dodge -> Idle -> MeleeAttack in an 80 s run; thirteen collision
actors exist (u058d..u0599; u0598, the second heart, inactive), 0 caught / reach-stub lines.
`damage u0582 10` leaves the ing at 1000/1000: the module's TakeDamage is retail's `blr`, damage
lands on the heart collision actors and reaches the ing as kSM_Damage (`fn_18_36BC`);
`damage u0597 10` (mainHeart_SDK) takes it from 100 to 90.

Slice 2 ports ChoseAction `8200` (seven scored actions, the jump table at data 0x654: stampede,
jump slide, light swarm, dark beam, light beam, portal, dark flier; a failed check scores
-FLT_MAX, strict `>` argmax, an eight-entry history at 0x7EC), `B04` (the stage by health),
`AA4`, the seven checks (`458C`, `431C`, `4468`, `4528`, `44C4`, `4344`, `436C`), `84A8`,
`868C`, `87F0`, `27DC`, the knock-back, kSM_Launching, and the states the walk reaches:
Stampede `71D8`, Taunt `6D84`, ChangeVulnerability `6A40` (with `PickHeart` `408`) and
PortalAttack `60AC`. A 160 s run (`damage u0597 40` / `u0598 40` every 9 s): 0 caught /
reach-stub lines; the FSM cycles EyeShut, JumpAttack, PortalAttack, Idle, Dodge and Taunt.
mainHeart_SDK goes 100 -> 60 on the first hit and stays there (EyeShut makes the hearts immune);
ChangeVulnerability and Stampede are not reached and the stage does not end.

Slice 3 ports the hearts' vulnerability route: `3448` (the heart effects' external params via
`4BB8`), `2F04` (the hit timer at 0x860; a dead heart moves state 0 to 1 with a `kS_Two`
knock-back when the base's `CKnockBackMgr` at 0x620 enables it, and state 2 to the base's
`mPendingDeath`, 0x10 of 0x421), `3164` (`CBCAdditiveFlinchCmd` = body command 0x1C; state 2
moves the heart's damage past the ing's health, capped by 0x814, onto the ing and 0x8EC; 0x448
is `mDamageCooldownTimer`), EyeShutResetVulnerability `6B80`, GrowHeart `5614` (with `A00`, the
health lost; 0xB50 is the threshold's reset copy) and the contact damage `2D50` (the DOL's
`fn_80137718` is `CCollisionActor::GetLastTouchedObject`; 0x440/0x444 are
`mCurDamageRemTime`/`mDamageWaitTime`; the old port inverted the 0x825 0x10 test). The vulnerable
hearts take only the heart's beam (`SetHeartState`), so the run used a local, uncommitted console
`damage ... dark|light` option: `damage u0597/u0598 150 power|dark|light` every 6 s, 220 s.
0 caught / reach-stub lines. The FSM walks EyeShut -> GrowHeart -> Stampede -> EyeShut ->
Stampede -> GrowHeart -> Dodge -> JumpSlideAttack; the ing goes 1000 -> 850 -> 750 -> 600 -> 500
over two vulnerable windows (the boss bar at half), the hearts reset to the yellow 100 and die at
84 s, then the FSM stays in JumpSlideAttack (`5844`, a stand-in) and the stage does not end.
ChangeVulnerability is not seen in the 3 s samples (it is a one-frame step).

Slice 4 ports JumpSlideAttack `5844` (0x630: loop attack type 5, the slide time from the loop
attack animation's duration plus `25C`'s room ahead, the done flag 0x40 of 0x825, the heart swap
on exit), its trigger `8600` (the done flag and AnimOver) and `25C` (a 1000-unit ray along the
facing from the collision box). The contact damage `2D50` now passes the facing, not the
translation: the transform sits at guest 0x24, so 0x28/0x38/0x48 are its Y column. With the
console's `damage <id> <n> dark|light` (commit `port_console`), the same 220 s run: the FSM walks
EyeShut, GrowHeart, Stampede, Taunt, Dodge and JumpSlideAttack three times; the ing goes 1000 ->
750 -> 500 -> 250 -> 0 at 105 s, the FSM enters Dead, the ing is deleted and the post-fight
cinematic plays (`/tmp/opencode/emp3run/shot_stage_end.png`). 0 caught / reach-stub lines.
Reached stand-ins left: `4C3C`, Think's `1370`/`22E0`/`E40` and the sound.

Slice 5 ports DoUserAnimEvent `9048` (the 40-entry event table: the slide flags 0x08/0x10/0x20
of 0x825, the INTERNAL_STATE_01 actors moved to the event locator, 'IS04' on kUE_ScreenShake,
the heart switches, the jump attack's shock wave from the data's `jumpAttackShockWaveInfo`
(guest 0xE90) with the data's `sound` (0xECC) at 0x908/0x90C), `12C4` (stop the beam, guest
0x8AC is the CPlasmaProjectile `1530` makes, 0x8B0 its sound) and `1B88`. `4D0` (module 29's
object), `BB8` and `1530` are logged stand-ins. Run: JumpAttack now cycles, the ing reaches 0 at
111 s and the FSM enters Dead; 0 caught / reach-stub lines.

### The base-class members (the sweep)

The port's members whose guest offset is below the module's first own field (the ctor's first
store past CPatterned's size, 0x7C0) are base-class members: 0x158 -> `CPhysicsActor::mMass`
(the host's `GetMass()`), 0x1C0 -> `CPhysicsActor::mConstantForce` (the host's
0x6B4 -> `CPatterned::mAnimationState`, which the module's states write with
`mAnimationState.SetState()` (0 = `kAS_NotReady`, 1 = `kAS_Ready`, 3 = `kAS_Over`).
and the host's 0x1B8, so the host's named members are used and never a raw offset.
