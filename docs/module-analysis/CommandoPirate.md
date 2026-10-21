# CommandoPirate REL (module 9): analysis for a host body

Analysis only (2026-10-20, milestone 1). Sources: `config/G2ME01/rels/CommandoPirate/symbols.txt`
(sizes), the archive build's disassembly `../MetroidPrime2Port/build/G2ME01/CommandoPirate/asm/`
(`auto_00_00000000_text.s`, `auto_03_00000000_rodata.s`, `auto_04_00000000_data.s`), the module's
own `.rel` (the string pool) and the DOL symbol names it calls. Confidence tags: **[proven]** = read
from an instruction or table entry; **[probable]** = strong evidence but not proven; **[?]** = guess.
All `fn_9_X` below are REL `.text` offsets.

The module is the **Pirate Commando** (script type `CMDO`, in-game names "Morphology: Pirate
Commando" and "Morphology: Dark Pirate Commando"). Its sibling, the Dark Commando (module 3), is
upstream code (`include/MetroidPrime/Enemies/CDarkCommando.hpp`,
`src/MetroidPrime/Enemies/CDarkCommando.cpp`) and shares the model, the "Head_1" bone and the
locator names - the port's class must not collide with it.

## 1. Function map

| fn | size | role | evidence |
|---|---|---|---|
| `_prolog`/RELMain | 0x20 | calls `fn_9_138` | [proven] |
| `fn_9_138` | 0x30 | `lbl_9_bss_28 = fn_9_168`; DOL `fn_802188B0` (= `SetSCommandoPirate_FuncPtrs`) | [proven] |
| `_epilog`/RELExit | 0x24 | clears the registration | [proven] |
| `fn_9_168` | 0x76c | **loader** (`LoadCommandoPirate`), section 2 | [proven] |
| `fn_9_D904` | 0x12f4 | **CCommandoPirate ctor**, section 3 | [proven]: called after `rs_new 0xc68` |
| `fn_9_EA24` | 0x160 | copies the parameter block into `this+0x7C0` | [proven] |
| `fn_9_EDC0` | 0x2c0 | builds the parameter block (the loader's stack object at sp+0x358) | [proven] |
| `fn_9_98C` / `fn_9_F50C` | 0x94 / 0xb0 | the E-grenade half of the block | [proven] |
| `fn_9_1308` / `fn_9_EF80` | 0x310 / 0x178 | the shield half of the block | [proven] |
| `fn_9_1620` | 0x310 | reads the `0xfb435257` property block (the E-grenade) | [proven] |
| `fn_9_E30` | 0x224 | `SLdrCommandoPirate`'s constructor (the defaults) | [proven] |
| `fn_9_A20` | 0x40 | `CModelData` copy into the loader's temp | [proven] |
| `fn_9_D68` | 0xc8 | `SLdrEditorProperties` destructor | [proven] |
| `fn_9_F58C` | 0x338 | `CElementGen` from a script object (the two effects at +0xC3C/+0xC40) | [probable] |
| `fn_9_F9F8` | 0xc0 | the module's tail helper | [?] |

### Module head accessors (the tiny ones, all real)

| fn | size | vtable slot | body | guest offset read |
|---|---|---|---|---|
| `fn_9_0` | 0x8 | 0xE0 `GetSearchPath` | `addi r3, r3, 0x928` | this+0x928 |
| `fn_9_8` | 0x8 | 0xAC `IsListening` | `li r3, 0x1` | - |
| `fn_9_10` | 0xc | 0x114 `GetGravityConstant` | `lbl_9_rodata_400` (50.0f) | - |
| `fn_9_1C` | 0x3c | 0x48 `GetTouchBounds` | `GetBoundingBox` then `fn_9_F9BC` | - |
| `fn_9_58` | 0x10 | 0xA4 `TakeDamage` | `mDamageCooldownTimer = skDamageHitTime` | this+0x448 |
| `fn_9_68` | 0x8 | 0xC8 `GetModelAlphau8` | one byte load | this+0x44f |
| `fn_9_70` | 0x8 | 0xE4 `GetPointSearchPath` | `li r3, 0x0` | - |
| `fn_9_78` | 0x8 | 0xF4 `GetRagDoll` | `li r3, 0x0` | - |
| `fn_9_80` | 0x10 | 0x10C `GetAttackTarget` | stores `kInvalidUniqueId` | - |
| `fn_9_90` | 0xc | 0x110 `IsOnGround` | `rlwinm r3, r0, 29, 31, 31` (bit 3) | this+0x34c |
| `fn_9_9C` | 0x8 | 0x124 `GetDeathExplosionParticle` | `addi r3, r3, 0x754` | this+0x754 |
| `fn_9_A4` | 0x8 | 0xA8 `CanBeShot` | `li r3, 0x1` | - |
| `fn_9_AC` | 0x1c | 0xB4 `GetOrigin` | three floats copied out | this+0x54 (the position) |
| `fn_9_C8` | 0x2c | 0x3C `GetHealthInfo` | calls vtable slot 0x38 | - |

### Vtable `lbl_9_data_828` (82 words: two leading zeros and 80 slots)

Slots follow `docs/module-analysis/vtable-offsets.md` (the CEntity -> CActor -> CPhysicsActor -> CAi
-> CPatterned chain). The REL's own entries:

| slot | method | fn | port state |
|---|---|---|---|
| 0x08 | dtor | `fn_9_D47C` | host |
| 0x10 | `PreThink` | `fn_9_CAC8` (0x78) | host (the bone tracking's own PreThink, then the base's) |
| 0x14 | `Think` | `fn_9_C944` (0x184) | host (the base's Think, the bone tracking's, `fn_9_662C`'s timers and the module's other helpers as named stand-ins) |
| 0x18 | `AcceptScriptMsg` | `fn_9_CB40` (0x754) | stand-in |
| 0x24 | `PreRender` | `fn_9_C6D4` (0xc4) | host (the base's, then the bone tracking's own) |
| 0x28 | `AddToRenderer` | `fn_9_C4FC` (0x20) | host (a straight base call) |
| 0x2C | `Render` | `fn_9_C598` (0x13c) | host (the base's render, the space warp; the module's own renderables stay stand-ins) |
| 0x34 | `PreRenderAllViewports` | `fn_9_C51C` (0x7c) | host (the base's; the guest's rag-doll branch is dead) |
| 0x40 | `GetDamageVulnerability` | `fn_9_C2CC` (0x34) | host (the +0xC65 test) |
| 0x70 | `DoUserAnimEvent` | `fn_9_C798` (0x1ac) | host (the module's switch; the arms natural play does not reach are listed in section 4) |
| 0x84 | `CollidedWith` | `fn_9_BF44` (0x2b0) | host (the module's own handling; the arms natural play does not reach are listed in section 4) |
| 0x9C | `KnockBack` | `fn_9_C300` (0x1fc) | stand-in |
| 0xA8 | `CanBeShot` | `fn_9_A4` | host (true) |
| 0xAC | `IsListening` | `fn_9_8` | host (true) |
| 0xB0 | `Listen` | `fn_9_BE4C` (0xf8) | stand-in |
| 0xB4 | `GetOrigin` | `fn_9_AC` | host (the translation) |
| 0xC8 | `GetModelAlphau8` | `fn_9_68` | host (the base's alpha byte) |
| 0xDC | `ProjectileInfo` | `fn_9_D474` (0x8) | host (this+0xA14) |
| 0xE0 | `GetSearchPath` | `fn_9_0` | host (this+0x928) |
| 0xE4 | `GetPointSearchPath` | `fn_9_70` | host (null) |
| 0xE8 | `GetContactDamage` | `fn_9_C1F4` (0xd8) | host (the module's charge or blade damage) |
| 0xF0 | `SetupStateMachine` | `fn_9_BDE0` (0x6c) | host (registers the module's two tables) |
| 0xF4 | `GetRagDoll` | `fn_9_78` | host (null) |
| 0x10C | `GetAttackTarget` | `fn_9_80` | host (kInvalidUniqueId) |
| 0x114 | `GetGravityConstant` | `fn_9_10` | host (50.f) |

The REL does **not** override `TypesMatch`: the vtable's slot 0x0C is `TypesMatch__10CPatternedCFi`,
the base's. The port therefore adds no `kET_` value for this class (the brief's rule 4) and the
actor answers to the base's type tag.

### Strings (the module's own `.rodata` pool, "StateOver" + N)

| offset | string | used for |
|---|---|---|
| +0x35D | `??(??)` | the `rs_new` debug name (0x000008DC) |
| +0x364 | `Commando Explosion Fx` | an effect |
| +0x37A | `Commando Drowning Fx` | an effect |
| +0x38F | `Shield_CollisionActor` | the shield's collision actor |
| +0x3A5 | `Blade_CollisionActor` | the blade's collision actor |
| +0x3BA | `Commando E-Grenade` | the grenade's script object |
| +0x3CD | `Head_1` | the bone tracking's bone and the first locator (this+0xC5C) |
| +0x3D4 | `Mid_Launch_LCTRL` | locator this+0xC5D |
| +0x3E4 | `gun_LCTR` | locators this+0xC5E and this+0xC5F |
| +0x3ED | `R_wrist` | locator this+0xC60 |
| +0x3F5 | `R_elbow` | locator this+0xC61 |
| +0x3FD | `L_wrist` | locator this+0xC62 |
| +0x405 | `CommandoPirate.rel` | the module's own file name |

### The state machine's tables

`fn_9_BDE0` (vtable slot 60) calls the base's setup and then registers two tables on the
state manager: the triggers (`lbl_9_data_198`, .data 0x198, 34 = 0x22) through the slot at
+0x18 and the states (`lbl_9_data_568`, .data 0x568, 36 = 0x24) through +0x14. The module has
no third table. Each entry is 16 bytes `{name, 0, 0xFFFFFFFF, fn}`; the function pointers come
from the three-word records `{0, 0xFFFFFFFF, fn}` in .data 0x0..0x198 (the triggers,
`fn_9_F9F8`'s stores at 0x0000FAD4..0x0000FF24) and .data 0x3B8..0x568 (the states, the stores
at 0x0000FF34..0x000103C4). `fn_9_F9F8` is the module's .ctors builder.

| # | trigger | fn | .rodata |
|---|---|---|---|
| 0 | StateOver | `fn_9_BDCC` | 0x504 |
| 1 | ShouldWarpIn | `fn_9_BD98` | 0x50E |
| 2 | ShouldMeleeAttack | `fn_9_BCB8` | 0x51B |
| 3 | ShouldFireEGrenade | `fn_9_BBC8` | 0x52D |
| 4 | ShouldRetreat | `fn_9_BB7C` | 0x540 |
| 5 | ShouldJumpBack | `fn_9_BB10` | 0x54E |
| 6 | ShouldAmbush | `fn_9_BB04` | 0x55D |
| 7 | BreakAmbush | `fn_9_BAF8` | 0x56A |
| 8 | HasLineOfSight | `fn_9_BAEC` | 0x576 |
| 9 | UnderFire | `fn_9_BAD0` | 0x585 |
| 10 | HeardShot | `fn_9_BAB4` | 0x58F |
| 11 | HasTarget | `fn_9_BA78` | 0x599 |
| 12 | HasNewTarget | `fn_9_BA20` | 0x5A3 |
| 13 | PathShagged | `fn_9_B9D8` | 0x5B0 |
| 14 | PathOver | `fn_9_B990` | 0x5BC |
| 15 | IsOffPath | `fn_9_B958` | 0x5C5 |
| 16 | HasAttackPattern | `fn_9_B900` | 0x5CF |
| 17 | IsFacingTarget | `fn_9_B818` | 0x5E0 |
| 18 | AttackPatternOver | `fn_9_B7FC` | 0x5EF |
| 19 | TooClose | `fn_9_B76C` | 0x601 |
| 20 | ShouldDodge | `fn_9_B690` | 0x60A |
| 21 | ShouldArmShield | `fn_9_B660` | 0x616 |
| 22 | ShouldShieldCharge | `fn_9_B250` | 0x626 |
| 23 | ShouldBoost | `fn_9_B1E0` | 0x639 |
| 24 | AbortShieldCharge | `fn_9_B1D4` | 0x645 |
| 25 | IsAggressive | `fn_9_B1C8` | 0x657 |
| 26 | FoundJumpPoint | `fn_9_B1A8` | 0x664 |
| 27 | ShouldCrouch | `fn_9_B168` | 0x673 |
| 28 | ShouldWallHang | `fn_9_B114` | 0x680 |
| 29 | ShouldCover | `fn_9_B0F4` | 0x68F |
| 30 | FoundCover | `fn_9_B0D4` | 0x69B |
| 31 | ShouldCoverAttack | `fn_9_B0B4` | 0x6A6 |
| 32 | CoverBlown | `fn_9_AFD4` | 0x6B8 |
| 33 | AbortSeekCover | `fn_9_AFB8` | 0x6C3 |

| # | state | fn | .rodata |
|---|---|---|---|
| 0 | PathFind | `fn_9_9AAC` | 0x783 | activate: the body controller's steering blend mode (the guest's field at +0x4C, the command manager's `mSteeringMode`) becomes full speed, the steering speed range is set to 1.0f twice and the 0x80 bit of the flag byte at guest this+0xC63 is cleared; deactivate: the blend mode goes back to normal and the 0x80 bit of the byte at +0xC65 is set; then the path find navigation runs while the block's flag byte at +0x15C does not have its 0x08 bit and the byte at +0xC65 has its 0x80 bit | the module's helper `fn_9_1A9C` (0x170 bytes) is ported: it walks the listening-AI list and steers away from the other AIs of the same area |
| Lurk | `fn_9_ADD4` | 0x6D2 |
| 1 | Ambush | `fn_9_ADA0` | 0x6D7 |
| 2 | Alert | `fn_9_AE04` | 0x6DE |
| 3 | FaceTarget | `fn_9_ABDC` | 0x6E4 |
| 4 | SelectTarget | `fn_9_AB08` | 0x6EF |
| 5 | SetTargetDest | `fn_9_AA20` | 0x6FC | activate: the path find destination becomes the target's position, the path find navigation's face target id is cleared and the module's helper `fn_9_19D8` (0xc4 bytes) is called |
| 6 | SetRetreatDest | `fn_9_A97C` | 0x70A |
| 7 | SetJumpDest | `fn_9_A8E8` | 0x719 |
| 8 | SetPathMeshDest | `fn_9_A838` | 0x725 |
| 9 | MeleeAttack | `fn_9_A4BC` | 0x735 |
| 10 | EGrenadeAttack | `fn_9_A2EC` | 0x741 |
| 11 | PostEGrenadeAttack | `fn_9_A110` | 0x750 |
| 12 | JumpPointFind | `fn_9_9F24` | 0x763 |
| 13 | JetBoost | `fn_9_9BB4` | 0x771 |
| 14 | PathFind | `fn_9_9AAC` | 0x77A |
| 15 | PathFind | `fn_9_9AAC` | 0x783 | activate: the body controller's steering blend mode (the guest's field at +0x4C, the command manager's `mSteeringMode`) becomes full speed, the steering speed range is set to 1.0f twice and the 0x80 bit of the flag byte at guest this+0xC63 is cleared; deactivate: the blend mode goes back to normal and the 0x80 bit of the byte at +0xC65 is set; then the path find navigation runs while the block's flag byte at +0x15C does not have its 0x08 bit and the byte at +0xC65 has its 0x80 bit | the module's helper `fn_9_1A9C` (0x170 bytes) is ported: it walks the listening-AI list and steers away from the other AIs of the same area |
| Lurk | `fn_9_ADD4` | 0x6D2 | on activate, and only when the commando has a target, the 0x10 bit of the flag byte at guest this+0xC64 is set | `xc64_` (0) |
| Alert | `fn_9_AE04` | 0x6DE | activate: the target id goes into the field at guest this+0xBCC and the animation state becomes "ready"; update: the animation state is asked whether the alert command may be issued (the guest's command id 0x10) and the commando taunts - the relaxed taunt (`pas::kTT_Two`) while the body controller's locomotion type is relaxed, `pas::kTT_Four` otherwise; deactivate: the animation state back to "not ready" | the guest's command id 0x15 is the body state command `kBSC_Taunt`; the host's `CBCTauntCmd` |
| Patrol | `fn_9_996C` | 0x783 |
| 16 | FollowAttackPattern | `fn_9_961C` | 0x78A |
| 17 | WarpIn | `fn_9_9064` | 0x79E |
| 18 | WarpOut | `fn_9_8DF0` | 0x7A5 |
| 19 | PostWarpOut | `fn_9_8D28` | 0x7AD |
| 20 | JumpBack | `fn_9_8B70` | 0x7B9 |
| 21 | Dodge | `fn_9_8934` | 0x7C2 |
| 22 | ArmShield | `fn_9_86B4` | 0x7C8 |
| 23 | ShieldCharge | `fn_9_8028` | 0x7D2 |
| 24 | ScriptedShieldCharge | `fn_9_7A98` | 0x7DF |
| 25 | RestoreOrientation | `fn_9_786C` | 0x7F4 |
| 26 | Crouch | `fn_9_7724` | 0x807 |
| 27 | WallHang | `fn_9_7358` | 0x80E |
| 28 | WallDetach | `fn_9_728C` | 0x817 |
| 29 | CoverFind | `fn_9_709C` | 0x822 |
| 30 | SetCoverDest | `fn_9_A7A8` | 0x82C |
| 31 | Cover | `fn_9_6DBC` | 0x839 |
| 32 | CoverAttack | `fn_9_6C68` | 0x83F |
| 33 | BreakCover | `fn_9_6BCC` | 0x84B |
| 34 | GetUp | `fn_9_6A40` | 0x856 |
| 35 | Dead | `fn_9_69C8` | 0x85C |


### The ported trigger bodies

Twenty-seven of the 34 triggers are host code; the other 7 are still stand-ins. Each body is
read off the asm with `tools/asmcheck.py annotate` and carries the guest offset it came from.

| trigger | fn | .rodata | body | reads |
|---|---|---|---|---|
| StateOver | `fn_9_BDCC` | 0x504 | `mAnimationState.GetState() == CAnimationState::kAS_Over` | the base's animation state (guest +0x6B4) |
| ShouldWarpIn | `fn_9_BD98` | 0x50E | `x920_ == -1 && (mData.mFlags1 & 0x80) != 0 && (xc63_ & 0x20) != 0` | the block's +0x15D, `x920_`, `xc63_` |
| ShouldAmbush | `fn_9_BB04` | 0x55D | `(mData.mFlags0 & 0x80) != 0` | the block's +0x15C |
| BreakAmbush | `fn_9_BAF8` | 0x56A | `(mData.mFlags0 & 0x40) != 0` | the block's +0x15C |
| HasLineOfSight | `fn_9_BAEC` | 0x576 | `(xae8_ & 0x80) != 0` | `xae8_` (new, 0) |
| HeardShot | `fn_9_BAB4` | 0x58F | `xb60_ < 1.5f` | `xb60_` (new, 1.5f, `lbl_9_rodata_47C`) |
| HasAttackPattern | `fn_9_B900` | 0x5CF | `GetConnectedObject(mgr, kSS_Attack, kSM_Follow) != kInvalidUniqueId` | the base's |
| AttackPatternOver | `fn_9_B7FC` | 0x5EF | `mWaypointNavigation.GetDestination() == kInvalidUniqueId` | the base's waypoint navigation (guest +0x6C0) |
| AbortShieldCharge | `fn_9_B1D4` | 0x645 | `(xc64_ & 0x04) != 0` | `xc64_` (0) |
| IsAggressive | `fn_9_B1C8` | 0x657 | `(xc64_ & 0x40) != 0` | `xc64_` (0) |
| FoundJumpPoint | `fn_9_B1A8` | 0x664 | `xbd0_ != kInvalidUniqueId` | `xbd0_` (new, kInvalidUniqueId) |
| ShouldCover | `fn_9_B0F4` | 0x68F | `(mData.mFlags0 & 0x08) == 0 && (xc63_ & 0x01) != 0` | the block's +0x15C, `xc63_` (0) |
| FoundCover | `fn_9_B0D4` | 0x69B | `xbd2_ != kInvalidUniqueId` | `xbd2_` (new, kInvalidUniqueId) |
| ShouldCoverAttack | `fn_9_B0B4` | 0x6A6 | `xb5c_ <= 0.f` | `xb5c_` (new, 0.f) |
| AbortSeekCover | `fn_9_AFB8` | 0x6C3 | `xb8c_ >= xb90_` | `xb8c_`, `xb90_` (new, 0.f) |
| UnderFire | `fn_9_BAD0` | 0x585 | `xb64_ < 0.75f` | `xb64_` (0.75f, `lbl_9_rodata_4E0`) |
| IsOffPath | `fn_9_B958` | 0x5C5 | the actor's own position is not on the path search's path (the guest's call at 0x0000B970 and its != 0 idiom at 0x0000B974..0x0000B97C) | the path search at guest this+0x928 and the position at +0x54 |
| HasTarget | `fn_9_BA78` | 0x599 | the target the commando holds (guest this+0xBCA) is still in the world | the base's `GetObjectById` |
| HasNewTarget | `fn_9_BA20` | 0x5A3 | there is a target and it is a new one: `HasTarget(...)` and the id at guest this+0xBCC differs from the target id | `xbcc_` (new, kInvalidUniqueId) |
| ShouldRetreat | `fn_9_BB7C` | 0x540 | the flag byte at guest this+0xC63 has its 0x40 bit set, the block's flag bytes at +0x15D and +0x15C do not have their 0x80 and 0x08 bits, and the id at guest this+0xBCE is not kInvalidUniqueId | `xbce_` (new, kInvalidUniqueId) |
| ShouldArmShield | `fn_9_B660` | 0x626 | the flag byte at guest this+0xC64 has its 0x80 bit set and its 0x01 bit clear, and the block's flag byte at +0x15D does not have its 0x20 bit | `xc64_` (0) |
| PathShagged | `fn_9_B9D8` | 0x5B0 | the flag byte at guest this+0xC63 has its 0x80 bit set, or the base's own `PathShagged` says so | `xc63_` (0) |
| PathOver | `fn_9_B990` | 0x5BC | the flag byte at guest this+0xC65 does not have its 0x80 bit set, or the base's own `PathOver` says so | `xc65_` (0x80) |
| TooClose | `fn_9_B76C` | 0x60A | the target is closer than the actor's minimum attack range: the squared distance against the squared radius at guest this+0x3DC | the base's minimum attack range (`mMinAttackRange`) |
| ShouldMeleeAttack | `fn_9_BCB8` | 0x51B | the block's +0x15C must not have its 0x02 bit and the melee cooldown (guest +0xB7C) must be above 1.0f; then the target must be inside the base's attack band (the squared distance between the minimum attack range at guest +0x3DC and the maximum at +0x3E0) and the height difference under 3.0f | the base's minimum and maximum attack range, `xb7c_` |
| ShouldDodge | `fn_9_B690` | 0x60A | the block's +0x15C must not have its 0x20 bit and the flag byte at guest +0xC63 must have its 0x04 bit; the target must be in front of the actor (the dot product with the forward column) and either the under-fire distance (`xb64_`) is under 0.75f or the heard-shot distance (`xb60_`) under 1.5f | the block's +0x15C, `xc63_`, `xb64_`, `xb60_` |
| ShouldFireEGrenade | `fn_9_BBC8` | 0x52D | the flag byte at guest +0xC63 must have its 0x02 bit, no E-grenade script object may be alive and the block's +0x15D must not have its 0x10 bit; the target's aim position must be inside the grenade's band (the block's +0x60 and +0x64) | `xc63_`, the block's +0x15D, the grenade distances |
| IsFacingTarget | `fn_9_B818` | 0x5E0 | the angle between the actor's forward and the (flattened) direction to the target's aim position is at most 20 degrees; a target that is gone answers true | the module's own helper `fn_9_3CBC` |

- `StateOver` reads guest +0x6B4, which is the base `CPatterned`'s animation state - the host
  already has it, as the DigitalGuardian's own `StateOver` (fn_14_9AA8) does. The guest's test is
  the cntlzw idiom (`subfic r0, r0, 0x3` at 0x0000BDD0, `cntlzw` at 0x0000BDD4, `srwi r3, r0, 5` at
  0x0000BDD8): one iff the id is 3, the host enum's `kAS_Over`.
- `ShouldAmbush` and `ShouldWarpIn` read guest +0x91C and +0x91D, which are inside the loader's
  parameter block (the block runs from +0x7C0 to +0x91E): they are its last two bytes, the flags
  the data ctor `fn_9_EDC0` packs from the `sound` property (0x7abed4ce) - `mFlags0` gets its
  bits 14, 11, 8, 5, 2, 31, 28 and 25, `mFlags1` its bits 6, 3, 0 and 29 (the packing is at
  0x0000EED4..0x0000EF60). They are script-supplied, so the port reads them from the data member
  (`mData.mFlags0` / `mData.mFlags1`), not from members of its own.
- `x920_` is the object's own word: the constructor sets it to -1 (fn_9_D904 @0x0000D994) and the
  module's states rewrite it (0x8DB0, 0x90EC, 0x9358, 0x9554). It is not loader-supplied.
- `xc63_` is the object's own flag byte: the constructor clears it (0x0000E0F0) and the module's
  states and the `Listen` override set and clear its bits (0x6744, 0x67C0, 0x6858, 0x70E0, 0x70F4,
  0x9560, 0xBEC0). It is not loader-supplied.
- `xb64_` is the float at +0xB64: the constructor sets it to 0.75f (`lbl_9_rodata_4E0`, 0x0000DAD8)
  - the same constant the trigger compares against - and the `Dead` state rewrites it (0x6980,
  0x699C), so `UnderFire` is false until something lowers it.
- `HasAttackPattern` calls the base's `GetConnectedObject` (already ported, in
  `src/MetroidPrime/Enemies/CPatternedAiFunctions.cpp`); the two FourCCs the guest builds inline
  are `kSS_Attack` and `kSM_Follow`.

- The new members and where their values come from: `xae8_` is the module's byte at +0xAE8
  (nothing in the module writes it, so it stays 0); `xb7c_` is the float at +0xB7C, the cooldown
  since the last melee attack: the module's per-frame helper `fn_9_662C` adds the frame time to
  it (0x000068E8) and `fn_9_A60C` clears it when the melee ends (0x0000A700); the port's
  `ShouldMeleeAttack` needs it above 1.0f, which the ported helper gives; the other timers
  `fn_9_662C` advances are `xb68_` (+0xB68), `xb6c_` (+0xB6C), `xb70_` (+0xB70), `xb74_`
  (+0xB74), `xb78_` (+0xB78) and `xb84_` (+0xB84), and the counter it lowers is `xc50_`
  (+0xC50);
  `xb5c_` is the float at +0xB5C (the
  constructor stores 0.f there at 0x0000DABC); `xc4c_` is the module's own state word at +0xC4C
  (`fn_9_1C0C` reads it at 0x00001C30, sets it to -1 at 0x00001D0C and dispatches on it at
  0x00001D14; the constructor does not write it, so the port starts it at -1, the value the aim
  gate `fn_9_4CA4` wants); `xb60_` is the float at +0xB60 (the constructor
  stores 1.5f, `lbl_9_rodata_47C`, at 0x0000DACC; the `Listen` override rewrites it at 0x0000BF38);
  `xb8c_` and `xb90_` are the floats at +0xB8C and +0xB90 (both 0.f from the constructor, 0x0000DB08
  and 0x0000DB0C; the constructor's later locomotion term, 4.f * (this+0x814 / the locomotion speed)
  at 0x0000E668..0x0000E67C, is not ported, so +0xB90 can stay lower than retail's); `xbd0_` and
  `xbd2_` are the jump point and cover point ids at +0xBD0 and +0xBD2 (the constructor stores
  kInvalidUniqueId at 0x0000DBC0 and 0x0000DBC8).
- `AttackPatternOver` reads guest +0x6C0, the base's waypoint navigation destination - the same
  member the Grenchler's own `AttackPatternOver` reads - so the port uses the base's navigation
  rather than a member of its own.

In area 59 (Temple Grounds/05_temple_federationsite_dark) the actor keeps the state machine in
`Patrol`/`SelectTarget`/`FaceTarget`, so only the triggers those states own are evaluated:
`StateOver`, `ShouldWarpIn`, `ShouldAmbush`, `UnderFire` and `HasAttackPattern` run, and none of
them returns true (the script leaves the `sound` property at 0, so both flag bytes are clear).
The eleven triggers of the cover, shield, grenade, melee and jump states do not run there.

### The ported state bodies

| state | fn | .rodata | body | notes |
|---|---|---|---|---|
| PathFind | `fn_9_9AAC` | 0x783 | activate: the body controller's steering blend mode (the guest's field at +0x4C, the command manager's `mSteeringMode`) becomes full speed, the steering speed range is set to 1.0f twice and the 0x80 bit of the flag byte at guest this+0xC63 is cleared; deactivate: the blend mode goes back to normal and the 0x80 bit of the byte at +0xC65 is set; then the path find navigation runs while the block's flag byte at +0x15C does not have its 0x08 bit and the byte at +0xC65 has its 0x80 bit | the module's helper `fn_9_1A9C` (0x170 bytes) is ported: it walks the listening-AI list and steers away from the other AIs of the same area |
| Lurk | `fn_9_ADD4` | 0x6D2 | on activate, and only when the commando has a target, the 0x10 bit of the flag byte at guest this+0xC64 is set | `xc64_` (0) |
| Alert | `fn_9_AE04` | 0x6DE | activate: the target id goes into the field at guest this+0xBCC and the animation state becomes "ready"; update: the animation state is asked whether the alert command may be issued (the guest's command id 0x10) and the commando taunts - the relaxed taunt (`pas::kTT_Two`) while the body controller's locomotion type is relaxed, `pas::kTT_Four` otherwise; deactivate: the animation state back to "not ready" | the guest's command id 0x15 is the body state command `kBSC_Taunt`; the host's `CBCTauntCmd` |
| Patrol | `fn_9_996C` | 0x783 | the message picks the locomotion (activate: relaxed, half the turn speed, the steering ratio of the walk and run speeds; deactivate: combat, twice the turn speed) and then `mWaypointNavigation.Patrol(mgr, msg, dt, *this)` | the guest's body-controller flag at +0x4C (0x00009A1C, 0x00009B60) is the command manager's steering blend mode: full speed on activate, normal on deactivate |
| SelectTarget | `fn_9_AB08` | 0x6EF | on activate, when the old target is gone: take the player as the target, point the bone tracking at it, send `kSS_Attack` with `kSM_None`, and switch to the combat locomotion | the guest's visual knowledge (this+0xAB0) has no host class: `SetVisualTarget` is a named stand-in |
| FaceTarget | `fn_9_ABDC` | 0x6E4 | activate: the module's own helper `fn_9_3CBC` asks the target for its aim position (guest this+0xB94) and the state measures the angle between the actor's facing and the direction to it (0.34906584 rad = 20 degrees) - the animation is "over" when the target is in front and "ready" when it is not; update: the animation state's own command test, the direction to the target, its magnitude test and the locomotion command that walks there; deactivate: the animation state back to "not ready" | the guest's `CTryCommand::ShouldRetry` is the base's animation state, so the port calls `mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Turn)` (the guest's command id 8) |
| SetTargetDest | `fn_9_AA20` | 0x6FC | on activate only: the destination starts as the actor's own position (guest this+0x54) and the 0x80 bit of the flag byte at guest this+0xC65 is cleared; the target (the id at guest this+0xBCA) is looked up and, when it is there and either the flag byte at +0xC64 has its 0x40 bit or the byte at +0xAE8 does not have its 0x80 bit, the destination becomes the target's position and the 0x80 bit of +0xC65 is set; then the path find navigation's destination is set, its face target id (guest this+0x718, the navigation's own member at +0x24) is cleared and the module's helper `fn_9_19D8` is called | `fn_9_19D8` is 0xc4 bytes, over the 0x80 limit, so it is a named stand-in; the guest reads the target's position without a cast, so the port casts to CActor (the host's CEntity has no position) |
| SetPathMeshDest | `fn_9_A838` | 0x725 | on activate: the 0x80 bit of the flag byte at guest this+0xC65 is cleared, the path find search (guest this+0x928) looks for the closest reachable point to the actor's own position (the out point starts as the zero vector) and the path find navigation (guest this+0x6F4) takes that point as its destination; when the search finds no point the navigation takes the actor's own position and the flag byte's 0x80 bit stays clear; the navigation's face target id (guest this+0x718) is cleared either way | the host's `CPathFindSearch::FindClosestReachablePoint` takes the same two arguments as the guest's (the source and the out point) |

The player id the guest takes (its state manager's +0x14FC, the id at +8 of it, 0x0000AB4C) has no
member at that offset in the host's state manager, so the port uses the player actor's id.

- `fn_9_3CBC` is the guest's own helper, ported as the private member `x3CBC_`: it looks the
  target up by the id at this+0xBCA and, when it exists, asks it for its aim position (CActor's
  vtable slot 0x54, `GetAimPosition`) with the state manager and the dt, and copies the answer
  into the out vector. It is 0xA8 bytes but trivial, so it is ported rather than stood in for.
- `fn_9_1A9C` (0x170 bytes) is the guest's own helper, ported as the private member
  `x1A9C_AvoidActors` and called by the PathFind state right after the path find navigation
  (0x00009B8C). It walks the listening-AI object list (the state manager's +0x830, the same list
  the Space Pirate's `AvoidActors` walks) and, for every other `CPatterned` of the same area
  (the entities' +0x4 area id), asks `CSteeringBehaviors::Separation` (the behaviors at
  this+0x61c, the other actor's position at its +0x54) for the separation from it. The distance
  is 5.0f (`lbl_9_rodata_468`) times the model's X scale - the model data sits at this+0x60 and
  its scale is the first member, so the multiply uses the scale's first float (0x00001B2C). A
  separation that is magnitude-safe becomes a locomotion command: the normalized separation as the
  move vector, a zero look direction and the weight 0.5f (`lbl_9_rodata_440`). The guest's
  `CObjectList::operator[]`, `TCastToPtr<CPatterned>` and the command manager's `DeliverCmd` are
  the host's own.
  The area 59 run (Temple Grounds/05_temple_federationsite_dark, 1200 frames) shows the helper
  running on every PathFind tick and finding seven other listening AIs of the area, but the
  nearest one is 10.2 units away while the separation radius is 5.0f * 1.5 = 7.5, so every
  separation is zero and no locomotion command is delivered there.
- The path find in area 59 finds no path to the player, and that is not a port mistake: the
  search's parameters are retail's (the guest's constructor at 0x0000D990: the area null, the flags
  1, the patterned info's pathfinding index, the radii 1.0f and the root position kRP_Center), the
  path area comes from the guest's own kSM_AreaLoaded case (0x0000CEF0) and the area's path data
  is retail's (the port's CPFArea loads the region connection matrix out of the pak,
  src/MetroidPrime/PathFinding/CPathFindArea.cpp:123). A probe of the same search in the run: the
  commando's own region links to its neighbours up to 30-60 units (the forward 10/30/60 and the
  right 10/30 succeed), while the player's platform - 111 units away and 3.5 units higher - is not
  linked to it by the area's ground connections (`kR_NoPath`), and the points off the mesh answer
  `kR_NoDestPoint`. The commando therefore stands where it is, as the area's data says.
- `fn_9_662C` (0x39c bytes) is the module's per-frame timer helper, and the only caller is the
  module's own Think (`fn_9_C944`, the call at 0x0000CA28) - no state calls it. It is ported as
  the private member `x662C_UpdateTimers`: the body is behind the base's flag word at guest
  this+0x420, whose bit 0x40 (the bit 25, `CPatterned::mAlive` - MWCC allocates the bitfields
  from the MSB) is the gate (the test at 0x00006660) and whose bit 0x80 (the bit 24,
  `mHitByPlayerProjectile`) clears the under-fire timer at +0xB64 (0x00006968). Then the helper
  advances the module's timers (+0xB60, +0xB68, +0xB6C, +0xB70, +0xB74, +0xB78, +0xB7C - the
  melee cooldown - +0xB84 and +0xB8C), sets the flag bits at +0xC63 (0x04, 0x01 and 0x02) and
  +0xC64 (0x40, 0x02, 0x04 and 0x80) when a timer expires, and lowers the module's own float
  at +0xC50 by the frame time times the shield's first unknown float (the data's +0x138) over its
  second (the data's +0x13C). The random checks compare the block's aggressiveness (+0x4C), cover
  check (+0x50) and search radius (+0x54) and the grenade's attack chance (+0xBC) and minimum
  attack interval (+0xC0) against `Random()->Range(0, 100)` (rodata_4CC).
- The area 59 run cannot put the player where the commando can path to it: the save's player
  position (83.5, 290.4, -43.6) is off the nav mesh (the probe's points 60-120 units around the
  commando answer `kR_NoDestPoint` in that direction) and the harness's input script can only move
  the player along the four camera-relative directions, which the walls block (holding the stick
  up stops the player at (61.7, 301.8) after 24 units, the other directions do not move it at
  all). The commando's own region does link 30-60 units around it, so a player placed there
  would be reachable - but the melee and the grenade triggers cannot fire there anyway (the
  cooldown and the flag bit above). The only new trigger that answers true in the run is
  `IsFacingTarget`.
- `fn_9_19D8` (0xc4 bytes) is the module's team helper, called by `fn_9_AA20` (and by the
  other destination states): it reads the team id at guest this+0xBC4, takes the associated team
  id from the team AI manager and joins it with the melee role (the guest's roles 1, -1, -1).
  It is ported as the private member `x19D8_JoinTeam` (the same shape as the Dark Commando's own
  `JoinTeam`, src/MetroidPrime/Enemies/CDarkCommando.cpp:1497); the team id is the new member
  `xbc4_teamId` (guest +0xBC4).
- The guest's rag doll (guest this+0xBFC) is always null - the constructor stores 0 there
  (0x0000DC18) and the module's `GetRagDoll` returns null (fn_9_78) - so the rag-doll branches of
  `fn_9_CAC8` (PreThink), `fn_9_C6D4` (PreRender) and `fn_9_C51C` (PreRenderAllViewports) are dead
  here. What is left is the bone tracking's own PreThink and PreRender (the calls at
  0x0000CB10 and 0x0000C77C, with the model's scale, the actor's transform and the body
  controller) and the base's own calls - the same three bodies the Dark Commando has.
- The path find search needs its area: the guest's `kSM_AreaLoaded` case (fn_9_CB40) sets the
  search's area pointer (guest this+0x928) to the area's path area - the world's area at the
  actor's area id (this+0x04), its post-constructed data (+0x104) and that data's path area
  (+0xEC); the store is at 0x0000CEF0. The port does the same in its `AcceptScriptMsg`
  (`mPathFindSearch.SetArea(...)`), as the Space Pirate's port does.
- The guest's `CTryCommand::ShouldRetry` (guest this+0x6B4) is the base's animation state, so
  the port asks the animation state whether the command may be issued
  (`mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Turn)`, the guest's command id 8).
- The actor's facing is the transform's forward column (`GetTransform().GetForward()`), not the
  base's face vector: the guest's three loads are its 0x28, 0x38 and 0x48.

In area 59 (Temple Grounds/05_temple_federationsite_dark) the state machine walks the states the
actor reaches: Patrol, SelectTarget, FaceTarget, Alert and then Lurk - Alert puts the target id
into the field at guest this+0xBCC (the store at 0x0000AE44), which is what `HasNewTarget`
compares with the target id, so that trigger is false again after Alert, exactly as in retail.
`HasTarget` and `HasNewTarget` are the first triggers of the module that return true in the port
- the commando's target is in the world. In `Lurk` the combat triggers are evaluated for the first
time: `ShouldRetreat` and `ShouldArmShield` answer false, and `PathShagged` answers true - which is
what takes the commando on to the `PathFind` state, where the path find navigation runs. FaceTarget ticks every frame; the angle test answers
"ready" (the target is not yet in front) and the animation state is otherwise the base's own.
Alert's taunt is delivered once (`pas::kTT_Four`, the actor is not relaxed by then), the
animation state goes "over" and the walk leaves Alert for `Lurk`, where the combat triggers
(`ShouldMeleeAttack`, `ShouldDodge`, `ShouldRetreat`, `ShouldArmShield`, `ShouldShieldCharge`,
`ShouldFireEGrenade`, `TooClose`, `PathShagged`, `PathOver`, `PathFind`) are evaluated for the
first time.

### The dodge picker (fn_9_3678)

fn_9_3678 (0x288 bytes) is the module's dodge picker: the Dodge state's activate calls it (the
call at 0x00008974), the result goes into the module's field at guest this+0xBDC (the store at
0x00008978) and, when it is not -1 (the compare at 0x00008980), the animation state goes "ready"
(the store at 0x0000898C). It answers 2 (the host's `pas::kSD_Left`) or 3 (its `pas::kSD_Right`),
or -1 when neither side is free.

The two candidates are the actor's **right** column and its negation. The guest's transform is a
CTransform4f at this+0x24 (the FromMatrix call's argument at 0x000024C4), so its kDX column - the
right one - is this+0x24, +0x34 and +0x44 (the three loads at 0x000036A8..0x000036E4); the
forward column is this+0x28, +0x38 and +0x48 (what the jump-back trigger reads).

The listening-AI list (the state manager's +0x830, the load at 0x000036E8) is walked: every other
AI of the same area (the area ids at +0x04, compared at 0x00003710..0x0000371C) that is nearer
than the module's float at guest this+0xBE4 (its square at 0x000036B8) blocks the side it stands
on - the dot product with the right column (the fmadds chain at 0x0000376C..0x0000377C) says the
side, and the angle to it must be under 60 degrees (lbl_9_rodata_4A4 = 1.0471976f, the GetAngleDiff
calls at 0x0000379C and 0x000037E0). A side that is still free is then checked by the module's own
helper fn_9_3338 (the calls at 0x00003834 and 0x00003878): the point that distance along the
side has to be clear. When both sides are free, the state manager's random (its CRandom16 at
+0x16E4, the call at 0x00003894) picks one: its bit 14 (the rlwinm at 0x00003898) keeps the
right, the clear bit keeps the left.

The port's member is `x3678_PickDodgeDirection`
(src/MetroidPrime/Enemies/PortCCommandoPirate.cpp); the retail function is marked
`// @ fn_9_3678`.

## 2. The loader (fn_9_168)

The property ids and their order are the generated `SLdrCommandoPirate` reader's, and the REL's own
switch matches it id by id (fn_9_168 @0x000001C8..0x00000568). Two sub-blocks are read by the REL's
own readers, not by the generated ones: `0xfb435257` (the E-grenade, fn_9_1620) and `0x388e16c9`
(the shield, fn_9_1308). Both readers' output layouts are the generated `SLdrUnknownStruct10` and
`SLdrCommandoShield` layouts, with the shield's damage info and vulnerability at +0x00 and +0x10.

The loader converts two damage infos (`LdrToDamageInfo`, fn_9_168 @0x000005E0 and @0x00000738),
builds the E-grenade half (fn_9_98C then fn_9_F50C) and the shield half (fn_9_EF80), assembles
the block (fn_9_EDC0), allocates the actor (`rs_new 0xc68`) and calls the constructor
(fn_9_D904 @0x00000964).

## 3. The constructor (fn_9_D904)

Base: `CPatterned` with EPatternedAI **5**, flavor 0, movement 0, collider 1, body 1 (the words
stored to the stack arguments at 0x0000DA50..0x0000DA5C). The guest's own members:

| guest | member | how the ctor fills it |
|---|---|---|
| 0x7C0 | the parameter block (0x15E) | fn_9_EA24 copies it |
| 0x920, 0x924 | two ints | -1 (0x0000D994, 0x0000D9A4) |
| 0x928 | `CPathFindSearch` | area null, flags 1, the patterned info's pathfinding index (+0xEC), radii 1.0f, no avoidance filter, root 0 (0x0000D990) |
| 0xA14 | `CProjectileInfo` | the block's projectile (+0x28) and its damage (+0x2C) (0x0000D9B8) |
| 0xA3C, 0xA40 | two ints | 0 |
| 0xA44 | a damage vulnerability | the shield's charge vulnerability is copied here first (the block's +0xF4, 0x0000D9D4), then the actor's own vulnerability with the ignore radius is copied over it (0x0000E638) |
| 0xA74 | `CBoneTracking` | the model's animation data, "Head_1", 1.2217305f, 3.1415927f, 0 (0x0000DA08) |
| 0xAB0 | `CVisualKnowledge` | the object's id, segment 0xFF, 0.1f, 0.05f |
| 0xAF0 | `CBurstFire` | the module's five `SBurst` entries (lbl_9_data_7A8) |
| 0xB50..0xB70 | three ints and six floats | 0, then 1.5f, 0.75f, 0 |
| 0xB74 | a float | the block's +0xC0 (the E-grenade's min attack interval) |
| 0xB78, 0xB7C | two floats | 2.0f, 1.0f |
| 0xB80..0xB90 | five floats | 0; +0xB90 becomes 4.0f * (this+0x814 / the locomotion speed) |
| 0xB94..0xBC0 | four `CVector3f` | zero |
| 0xBC4..0xBDA | eleven `u16` | `kInvalidUniqueId` |
| 0xBDC | an int | -1 |
| 0xBE0..0xBEC | four floats | the ctor's three animation distances: 0xBE0 is the scale's Y times the step distance of the parms (1, 2) (0x0000E61C), 0xBE4 the scale's X times the step distance of the parms (3, 2) (0x0000E56C), 0xBE8 the scale's Y times the jump distance of the parms (0, 0) (0x0000E4D4); 0xBEC is 0 |
| 0xBF0 | a `CVector3f` | zero |
| 0xBFC, 0xC04 | two ints | 0 |
| 0xC08, 0xC18, 0xC28 | three `CToken` pairs | the script objects at this+0x8F0, +0x90C, +0x7C8 |
| 0xC38 | an int | 0 |
| 0xC3C, 0xC40 | two `CElementGen*` | fn_9_F58C(this+0x914), fn_9_F58C(this+0x910) |
| 0xC44..0xC58 | three floats and two ints | 0, -1, 0 |
| 0xC5C..0xC62 | seven locator segment ids | the seven `GetLocatorSegId` calls |
| 0xC63, 0xC64 | two flag bytes | cleared bit by bit |
| 0xC65 | a flag byte | 0x80 (bit 7) set, the rest cleared |
| 0xC66 | a flag byte | cleared |
| 0x681 | a base flag byte | `|= 0x10` |

`GetDamageVulnerability` (fn_9_C2CC) reads the 0x04 bit of the byte at +0xC65: when it is set the
actor returns the vulnerability at +0xA44 (the actor's own, with the ignore radius), otherwise the
base's. The constructor leaves that bit clear, so the base's vulnerability is what a live actor
has; the shield's own branch is milestone 2.

### The parameter block (fn_9_EDC0, 0x15E bytes at this+0x7C0)

| guest | member | source |
|---|---|---|
| +0x00, +0x02, +0x04 | three `u16` | `sound_Impact`, `sound_Hurled`, `sound_Death` |
| +0x06 | a `u16` | `alwaysFF` |
| +0x08 | an int | `alwaysFF_0x467c3d94` |
| +0x0C | `CDamageInfo` (0x1C) | `bladeDamage` |
| +0x28 | an asset id | `projectile` |
| +0x2C | `CDamageInfo` (0x1C) | `projectileDamage` |
| +0x48 | a `u16` | `sound_Projectile` |
| +0x4C..+0x64 | seven floats | `aggressiveness`, `coverCheck`, `searchRadius`, `dodgeCheck`, `hearingRadius`, `intraBurstShotTime`, `intraBurstShotVariation` |
| +0x68 | the E-grenade half (0x70) | fn_9_98C + fn_9_F50C |
| +0xD8 | the shield half (0x84) | fn_9_EF80 |
| +0x15C, +0x15D | two flag bytes | the `sound` property's bits |

The flag bytes take the `sound` property's bits in the guest's order (fn_9_EDC0
@0x0000EED4..0x0000EF60): +0x15C holds 0x80 <- bit 14, 0x40 <- bit 11, 0x20 <- bit 8, 0x10 <- bit 5,
0x08 <- bit 2, 0x04 <- bit 31, 0x02 <- bit 28, 0x01 <- bit 25; +0x15D holds 0x80 <- bit 6, 0x40 <- bit
3, 0x20 <- bit 0, 0x10 <- bit 29.

The E-grenade half: +0x00 the grenade's mass, +0x04 its `unknown_0xed086ce0`, +0x08 the blade's
damage info, +0x24 the bounce count, +0x28 and +0x2C the explosion asset (twice), +0x30 the trail,
+0x34 the effect, +0x38 the bounce sound, +0x3A zero (the guest reads the high half of a stack
word), +0x3C..+0x48 the guest's 0.1f/150.f/0.1f/150.f, +0x4C a flag byte, +0x50 the EMP duration,
+0x54 the attack chance, +0x58 the min attack interval, +0x5C the post-attack pause, +0x60 the min
attack distance, +0x64 the max attack distance, +0x68 the min launch speed, +0x6C the max launch
speed.

The shield half: +0x00 the charge damage info, +0x1C the charge vulnerability, +0x4C the min
charge distance, +0x50 the max charge distance, +0x54 the charge speed, +0x58 the explode effect,
+0x5C the explode sound, +0x60 and +0x64 the two unknown floats, +0x68 the arm chance, +0x6C the
arm time, +0x70 the arm time variation, +0x74 the arm explode effect, +0x78 the charge effect,
+0x7C the arm effect, +0x80 and +0x82 the two turn sounds.

## 4. Spawn areas

`docs/module_spawns.tsv` lists the module for three (world, area, layer) triples:

| world | area | layer |
|---|---|---|
| Sanctuary Fortress | 01_cliff_entrance | 3 |
| Temple Grounds | 05_temple_federationsite_dark | 0 |
| Torvus Bog | 05_swamp_timber! | 2 |

Milestone 1's run is in Temple Grounds/05_temple_federationsite_dark (layer 0, the layer the
module is listed for): the actor registers, takes kSM_Create and thinks.

The milestone 1 run (`tools/run_gameplay.sh`, `TELEPORT=05_temple_federationsite_dark`,
`FRAMES=1200`, the card in `/tmp/opencode/atk_user`) reached gameplay at frame 242 (10.0 s), ran to
the frame limit at 26.0 s in area 59 of 63, and ended with exit=0. The stand-ins that fired:
`AcceptScriptMsg` (the kSM_Create), `PreThink`, `Think`, `SetupStateMachine`, `DoUserAnimEvent`,
`PreRender`, `PreRenderAllViewports` and `CollidedWith`. `grep -c '[port] caught\|reach-stub'` on
the run log is 0.

`fn_9_C798` (the user-event switch) and `fn_9_BF44` (the module's own collision handling) are
ported (2026-10-21) but not marked, because natural play does not reach their module's arms. In
the area 59 run (`TELEPORT=05_temple_federationsite_dark`, `FRAMES=2400`,
`MP2_GODMODE=1`, the player teleported next to the commando with `tp 75.9 371.6 -47.1`) the
armed cases are:

| function | arms that ran | arms that did not run |
|---|---|---|
| `fn_9_C798` | the default arm only: the base's own `DoUserAnimEvent` for `kUE_LoopedSoundStop` (67 times in the run) | `kUE_Projectile` (the E-grenade's throw, the grenade state `fn_9_A2EC` is a stand-in), `kUE_DamageOn` (the melee's damage event, the melee state `fn_9_A4BC` is a stand-in), `kUE_DeGenerate` and `kUE_BecomeRagDoll` (the death animations, the Dead state `fn_9_69C8` is a stand-in), `kUE_Landing` and `kUE_TakeOff` (the jump animations, the JetBoost state `fn_9_9BB4` is a stand-in), `kUE_BreakLockOn`, `kUE_EventStart` and `kUE_Activate` (the shield's animations, the ArmShield state `fn_9_86B4` is a stand-in) |
| `fn_9_BF44` | the early return (the flag byte at guest this+0xC65 has no 0x04 bit - the shield is not up), 1152 calls in the run | the shield's own arms: the damage to an object that is not a player, the 0x02 bit of the flag byte at guest this+0xC64 and the walk over the collision list (the filter hit, the steep normal and the kMT_Floor entry) - all of them need the 0x04 bit of the byte at guest this+0xC65, which only the shield's states set |

So the two functions get their `// @` markers when the states that play those animations are
ported. The helpers `fn_9_1F88` (the kUE_EventStart helper) and `fn_9_1EF8` (the kUE_Activate
helper) are ported too (2026-10-21); they run with the two events above, so they are not marked
either. `fn_9_5410` (the E-grenade's creator) stays a named stand-in: the host has no
script-object class for the "Commando E-Grenade" (the module's own object constructor
`fn_9_F418` and destructor `fn_9_F290` are not ported).

The same run with the PathFind state and its helper ported (2026-10-20) shows the path find
navigation running on every tick of the PathFind state (activate, update, deactivate), but the
commando stands still (its position is the same on every tick): the state that sets the path
destination (`fn_9_AA20`, SetTargetDest) was still a stand-in then, so the path find search had
no destination. `grep -c '[port] caught\|reach-stub'` on that log is still 0.

With `fn_9_AA20` ported (2026-10-20) the commando gets a destination: the path find
navigation's destination is the target's position (the player at 83.5, 290.4, -43.6 while the
commando stands at 72.3, 401.4, -47.1) and the state runs on every tick. The commando still does
not walk there: the path search returns `kR_NoPath` for that destination (the same search
succeeds for a point five units away, so the port's path data and the area's path area - set
from the guest's `kSM_AreaLoaded` case - are fine; the player's spot is simply not connected to
the commando's region by the area's ground connections). With a near destination in a probe run
the commando walked 44 units, so the destination, the navigation and the locomotion command all
work. The stand-ins that fired in this run are `AcceptScriptMsg`, `Think`, `DoUserAnimEvent`,
`CollidedWith`, `fn_9_9F24` (JumpPointFind, 0x1ec), `fn_9_B958` (IsOffPath, 0x38) and
`fn_9_B250` (ShouldShieldCharge, 0x410 - the size the earlier list had as 0xd4);
`grep -c '[port] caught\|reach-stub'` is 0.

With the four combat triggers ported (2026-10-20) only `IsFacingTarget` answers true in that
run (the commando turns to the target and the state machine walks on to the jump point and the
off-path states). `ShouldMeleeAttack` stays false because its cooldown timer (`xb7c_`) is only
advanced by the module's per-frame helper `fn_9_662C` (0x39c bytes, not ported), `ShouldDodge`
and `ShouldFireEGrenade` need the flag bits at guest +0xC63 that the module's unported states
set. The commando still does not walk: the path search finds no path to the player (above).

With the ctor's two step distances and the dodge picker ported (2026-10-20) the run in area 59
(`tools/run_gameplay.sh`, `TELEPORT=05_temple_federationsite_dark`, `FRAMES=2400`, the card in
/tmp/opencode/atk_user, `MP2_GODMODE=1`) reaches gameplay at 10.3 s, runs to the frame limit at
48.8 s and ends with exit=0; `grep -c '[port] caught\|reach-stub'` is 0. The commando's two
distances are 7.886234 (guest +0xBE0, the jump-back) and 7.084501 (+0xBE4, the side step). The
player teleported next to the commando (`tp 75.9 371.6 -47.1` through the debug console) makes
the commando walk to it (it ends at 77.6, 371.8, -46.1 beside the player at 80.6, 370.4), and the
Dodge state's activate then runs the picker. It answers -1 every time in that spot: both side
rays hit the world (the jump-back helper's ray from the bounding-box centre, 7.08 units
sideways), so neither side is free, no dodge command is issued and the animation state never
enters kAS_Step. The `ShouldJumpBack` trigger runs there too (three calls) and answers false.

With the off-path trigger (`fn_9_B958`) and the SetPathMeshDest state (`fn_9_A838`) ported
(2026-10-20) the same run still reaches gameplay at 10.1 s, runs to the frame limit at 46.2 s and
ends with exit=0, with `grep -c '[port] caught\|reach-stub'` = 0. `IsOffPath` is now the guest's
own test - the actor's position against the path search - and the state machine's walk changes
with it: the commando reaches the SetPathMeshDest state (the stand-in log for `fn_9_A838` is
gone) and the path find navigation takes the closest reachable point the search finds.

With the emitters (`fn_9_4BC0`), the JumpPointFind state (`fn_9_9F24`) and the CoverFind state
(`fn_9_709C`) ported (2026-10-20) the run still ends cleanly: gameplay at 10.0 s, the frame limit
at 46.0 s, exit=0 and `grep -c '[port] caught\|reach-stub'` = 0. The three stand-in logs are gone
and the commando still walks to the player.

The module's remaining stand-ins, smallest first, with their retail sizes: `fn_9_ADA0` (the
Ambush state, 0x34), `fn_9_B168` (ShouldCrouch, 0x40), `fn_9_B114` (ShouldWallHang, 0x54),
`fn_9_B1E0` (ShouldBoost, 0x70), `fn_9_69C8` (the Dead state, 0x78), `fn_9_A7A8` (SetCoverDest,
0x90), `fn_9_A8E8` (SetJumpDest, 0x94), `fn_9_6BCC` (BreakCover, 0x9c), `fn_9_A97C`
(SetRetreatDest, 0xa4), `fn_9_8D28` (PostWarpOut, 0xc8), `fn_9_728C` (WallDetach, 0xcc),
`fn_9_AFD4` (CoverBlown, 0xe0), `fn_9_6A40`
(GetUp, 0x130), `fn_9_7724` (Crouch, 0x148), `fn_9_6C68` (CoverAttack, 0x154), `fn_9_A2EC`
(EGrenadeAttack, 0x174), `fn_9_8B70` (the JumpBack state, 0x1b8), `fn_9_A110`
(PostEGrenadeAttack, 0x1dc), `fn_9_9BB4` (JetBoost, 0x1fc), `fn_9_8DF0` (WarpOut, 0x218),
`fn_9_786C`
(RestoreOrientation, 0x22c), `fn_9_86B4` (ArmShield, 0x280), `fn_9_6DBC` (Cover, 0x284),
`fn_9_A4BC` (MeleeAttack, 0x290), `fn_9_1C0C` (0x2ec),
`fn_9_7358` (WallHang, 0x310), `fn_9_3E1C` (0x324), `fn_9_961C` (FollowAttackPattern, 0x350),
`fn_9_B250` (ShouldShieldCharge, 0x410), `fn_9_9064` (WarpIn, 0x528), `fn_9_7A98`
(ScriptedShieldCharge, 0x590) and `fn_9_8028` (ShieldCharge, 0x68c). The ones the area 59 run
reaches are `fn_9_1C0C`, `fn_9_3E1C` and `fn_9_B250`.

`fn_9_4BC0` (the module's two sound emitters, 0xe4), `fn_9_9F24` (the JumpPointFind state, 0x1ec)
and `fn_9_709C` (the CoverFind state, 0x1f0) are host code since 2026-10-20: the emitters follow
the actor with the two handles at guest this+0xC04 and this+0xC38 (new members `xc04_` and
`xc38_`), the jump point search walks the AiWaypoint list for the nearest usable jump point
(the id at guest this+0xBD0) and the cover search walks the same list for the nearest free cover
point (the id at guest this+0xBD2, copied to +0xBD4, a new member `xbd4_`). The host class is
0xC40 bytes now (it was 0xC38; the retail's is 0xC68) - the loader allocates with `rs_new`, so it
uses the host's own size, and the header's `CHECK_SIZEOF` (a no-op outside the retail build)
still names the retail's 0xC68.

`fn_9_4D84` (0x228, one of the module's Think helpers) is host code since 2026-10-20 (the port's
member `x4D84_UpdateAim`): the alive test is the host's `GetAlive()` (the byte at guest
this+0x420's 0x40 bit), the aim command is the guest's id 0x1b = 27 = `kBSC_AdditiveAim` (with
the 0x02 bit of the flag byte at guest this+0xC65 as the command's third field, which the host's
`CBCAdditiveAimCmd` needed a setter for) and the idle command is 0x1a = 26 =
`kBSC_AdditiveIdle`. Its helper `fn_9_4CA4` (0xd8 bytes) is host code too - the port's member
`x4CA4_CanAim`, below.

`fn_9_4CA4` (0xd8 bytes, the gate the aim helper asks first) is host code since 2026-10-20 (the
port's member `x4CA4_CanAim`): the body controller (guest this+0x48C) holds its state info at
+0x36C, whose current state is asked through its vtable slot 0x28 (the vcall at 0x00004CC8) - the
slot is `CBodyState::CanShoot` (the DOL's `__vt__10CBodyState`: the dtor at +0x08, `IsInAir`
+0x0C, `IsDead` +0x10, `IsDying` +0x14, `IsMoving` +0x18, `ApplyGravity` +0x1C,
`ApplyHeadTracking` +0x20, `ApplyAnimationDeltas` +0x24, `CanShoot` +0x28, `UnkVtable2C` +0x2C,
`Start` +0x30, `UpdateBody` +0x34; the host's `CBodyState` has the same order). The guest keeps
the low byte of the answer (the clrlwi at 0x00004CD4) and gives up when it is zero (the beq at
0x00004CD8). Then the module's flag bytes: the byte at guest this+0xC64 must have its 0x10 bit
(the extrwi at 0x00004CE0) and must not have its 0x20 bit (the extrwi at 0x00004CF4), the byte at
guest this+0xC65 must not have its 0x01 bit (the clrlwi at 0x00004D00), its 0x04 bit (the extrwi
at 0x00004D08) or its 0x08 bit (the extrwi at 0x00004D10), and the block's flag byte at guest
this+0x91C (`mData.mFlags0`) must not have its 0x10 bit (the extrwi at 0x00004CEC). The module's
own word at guest this+0xC4C (the new member `xc4c_`) must be -1 (the cmpwi at 0x00004D1C), the
melee cooldown at guest this+0xB7C (`xb7c_`) must be above 1.0f (rodata_43C, the compare at
0x00004D30) and the body must be neither frozen (its byte at guest this+0x48C + 0x594, the 0x20
bit, the extrwi at 0x00004D40) nor electrocuting (its float at guest this+0x48C + 0x5B8 above
0.0f, the compare at 0x00004D54).

The word at guest this+0xC4C is the module's own state word: `fn_9_1C0C` reads it (0x00001C30),
sets it to -1 (0x00001D0C) and dispatches on it (0x00001D14). The constructor does not write it,
so the port starts it at -1 - the value the gate wants, and the value the module's own helper
leaves it at.

With the gate ported the area 59 run (the note's run: `tools/run_gameplay.sh`,
`TELEPORT=05_temple_federationsite_dark`, `FRAMES=2400`, the card in /tmp/opencode/atk_user,
`MP2_GODMODE=1`, the player teleported next to the commando with `tp 75.9 371.6 -47.1`) reaches
gameplay at 10.2 s, ends with exit=0 and `grep -c '[port] caught\|reach-stub'` = 0 - and the aim
path runs: the gate passes every frame (the body state is the locomotion state, whose `CanShoot`
is true, the byte at guest this+0xC64 has its 0x10 bit, the byte at guest this+0xC65 is 0x80,
the block's flag byte is 0, `xc4c_` is -1, the melee cooldown is above 1.0f and the body is
neither frozen nor electrocuting), so `fn_9_4D84` now delivers the additive aim command.

`fn_9_6390` (0x29c, one of the module's Think helpers) is host code since 2026-10-20 (the port's
member `x6390_UpdateBurstFire`): the module's shot timer at guest this+0xB5C (`xb5c_`) falls by
the frame time while it is above zero (0x000063C0..0x000063E0); the flag byte at guest this+0xC64
must not have its 0x80 bit and the counter at guest this+0xB50 (`xb50_`, new) must be above the
one at +0xB58 (`xb58_`, new) - the guest compares them unsigned (0x000063E4..0x000063FC) - and
then the block's shield arm chance (the block's +0x140 = guest this+0x900, `mShield.mArmChance`)
is rolled against a random number in [0, 100] (rodata_4CC): at least the roll sets the flag
byte's 0x80 bit, else the counter at +0xB58 takes the counter at +0xB50 plus one
(0x00006400..0x00006444). The two counters start at 0 (the constructor, 0x0000DA84 and
0x0000DAA4). While the actor is alive the aim gate `fn_9_4CA4` has to agree (0x00006448..
0x00006460) and the burst fire's own type (guest this+0xAF0) must not be -1 (0x00006464). The
target (the id at guest this+0xBCA) is looked up (0x00006470) and, when the actor is alive and
the target is there, the module's timer at +0xB5C must have run out (0x00006498..0x000064DC).
Unless the flag byte at guest this+0xC65 has its 0x02 bit, the delta to the target dotted with
the actor's own up column (guest this+0x28, +0x38 and +0x48 - the transform's Y column) must be
above zero: the target must be higher than the commando (0x000064E0..0x000064FC). The same delta
the other way round, dotted with the target's own up column (the target's +0x28, +0x38 and
+0x48), must be below zero, and then the burst fire's type takes two more when it is below two
(0x00006500..0x0000655C). The burst fire starts, the module's timer at +0xB5C takes the base's
average attack time (`GetAverageAttackTime`) plus a random number times the base's attack time
variation (guest this+0x3E8, the base's `mAttackTimeVariation`), and the counter at +0xB50 goes
up by one (0x00006560..0x00006594). Then the burst fire is updated with the frame time
(0x00006598) and, when it wants to fire (its byte at guest this+0xB04's 0x80 bit - the burst
fire's own `mShouldFire`), the module's helper `fn_9_57A0` (0x370 bytes, over the 0x80 limit, so
a named stand-in) runs and the burst fire's own timer (guest this+0xAF8) takes the block's
intra-burst shot time (the block's +0x60) plus a random number times the block's intra-burst
shot variation (the block's +0x64) around its middle - but only when that timer is above zero
(0x000065A8..0x000065F8).

The module's burst fire (guest this+0xAF0) is the host's `CBurstFire`, built from the module's
four `SBurst` entries: the REL's data table `lbl_9_data_7A8` (.data 0x7A8) points at the
module's .rodata at 0x0, 0x134, 0x1E4 and 0x318 and its fifth entry is null - the weights are
15, 20, 26 and 40, the shot angles are the 22.5 degree steps (6, 5, 3, 2, 1 / 16, 4, 8 / 16, 5,
8, 11, 14 / 7, 9, 13) with the -1 that ends a burst, and every entry's time to the next shot is
0.1f with a 0.05f variation. The constructor's own burst type is 0 (0x0000E420), not the -1 the
`CBurstFire` constructor leaves, so the helper's burst path is live from the start.

`fn_9_57A0` (0x370, the burst fire's shot) is host code since 2026-10-20 (the port's member
`x57A0_FireShot`): the shot leaves the gun locator (guest this+0xC5E, `mLocatorGun`) and, while the
actor is alive (the byte at guest this+0x420's 0x40 bit), it is aimed at the target (the id at
guest this+0xBCA). The target's position is the aim point; when the target is the player and the
player's own state word (the guest's player+0x12F8) is 3 - the host's morph ball transition state
`kMS_Unmorphing` - the player's position is used and the line of sight ray is skipped
(0x00005840..0x00005874, the test at 0x00005A2C). Otherwise the intercept position is predicted
(`PredictInterceptPos`, 0x000058E8). The gun's own direction is the right wrist locator (guest
this+0xC60) minus the right elbow locator (guest this+0xC61) (0x00005940..0x00005994) and the shot
needs the angle between it and the aim point to be at most 30 degrees (rodata_4C8), or at most 45
degrees (rodata_4C0) when the target is within 6 units (rodata_49C) (0x0000599C..0x000059D4).
The line of sight ray runs from the gun to the aim point with the guest's own filter - the include
list is the material 59 (`kMT_Solid`) and the exclude list the materials 32 and 20 (`kMT_Player`
and `kMT_ProjectilePassthrough`), the filter type 3 (0x000059D8..0x00005A48) - and when it hits nothing the module's line of sight byte (guest
this+0xAE8, `xae8_`) loses its 0x80 bit (0x00005B70..0x00005B7C). The burst fire's own error
(`GetDistanceCompensatedError`) is rotated by the actor's frame and added to the aim point, the
shot's transform looks from the gun to it (`LookAt`, 0x00005B0C) and the projectile is launched
with the guest's type 6, no attributes, no homing and the scale 1.0f (rodata_43C)
(0x00005B58). After a shot the muzzle flash is played: the additive animation's state id is 0x18
and its one parameter is the enum 2, or 8 when the body is in its locomotion state (the body's own
state word, the host's `GetCurrentStateId`) and that state says it is moving (the body state's
vtable slot 0x18, which the retail's `__vt__10CBodyState` makes `IsMoving` - the base's own
`ApplyHeadTracking` call at vptr+0x20 in `CBodyStateInfo.s` fixes the vtable's base), and the
shot's sound (the block's +0x48 = guest this+0x808, `mSoundProjectile`) is emitted at the actor's
position.

Two readings are the port's own and are marked as such in the code: the guest builds the player's
aim position with its own copy of the player (the vcall at 0x000058B4) - the port asks the player
for its aim position (`GetAimPosition`) - and the guest's visor effect (fn_9_6054, fn_9_5F3C) is
the default one, which is the host's `CImpactVisorEffect::None()`.

With the shot ported the area 59 run (the note's run, the card in /tmp/opencode/atk_user,
`MP2_GODMODE=1`, the player teleported next to the commando with `tp 75.9 371.6 -47.1` through the
debug console - `MP2_CONSOLE=<port>`, one plain command per connection) reaches gameplay at
10.0 s, ends with exit=0 and `grep -c '[port] caught|reach-stub'` = 0, and the commando shoots:
with a temporary branch log (reverted) the shot helper takes the predicted-target path, the aim
gate fails while the commando turns (the distance and the angle branches), the line of sight is
blocked once, and then the helper fires - the projectile, the muzzle flash and the shot's sound
all run.

`fn_9_3090` (0x2a8, the module's drowning) is host code since 2026-10-20 (the port's member
`x3090_Drown`): while the actor is alive (the byte at guest this+0x420's 0x40 bit) it must be in a
fluid (the host's `InFluidId`), the fluid must be a water script object (the guest's cast at
0x000030FC) whose own byte at +0x20 has its 0x80 bit (the extrwi at 0x0000310C - the port reads
the trigger flags' low byte), and the head locator (guest this+0xC5C) must be below the water's
trigger bounds (the GetTriggerBoundsWR call at 0x0000311C). Then the module's own float at guest
this+0xC00 (new member `xc00_`) takes 0.01f (rodata_4A0), the health info's second float is
cleared, the actor dies with the script object state 0x5241544C ("RATL" - the host's
`kSS_DeathRattle`) and its own forward vector as the death direction (the vcall at 0x000031C8),
the drowning explosion is built (the module's particle from the block's +0x08, the guest's token at
guest this+0xC28; the guest only builds it when that token is loaded, the test at 0x000031CC) and
the drowning sound (the block's +0x06 = guest this+0x7C6) is emitted at the actor's own position
with the volume 0x7f.

Area 59 has no water at all, so the drowning was driven with a temporary hook (reverted): the
helper was run with a forced fluid, a trigger standing in for the water and the head forced under
its bounds. Every statement of the body ran - the module's float, the health info, the death and
the sound - and the explosion was skipped because the block's +0x08 particle is not in the pool,
which is what the guest does with an unloaded token too.

`fn_9_C1F4` (0xd8, the contact damage - the vtable slot 0xe8) is host code since 2026-10-21 (the
port's `GetContactDamage`): the module's flag byte at guest this+0xC65 picks the damage - its 0x04
bit (the extrwi at 0x0000C20C, the mask 1u << (31 - 29) = 0x04) gives the shield's charge damage
(the block's +0xD8 = guest this+0x898), its 0x20 bit (the extrwi at 0x0000C260, the mask
1u << (31 - 26) = 0x20) gives the blade damage (the block's +0x0C = guest this+0x7CC) and without
either the base's own contact damage stands (the call at 0x0000C2B4). The guest copies the damage
into the base's member and returns it; the port returns it directly (the host's base getter is a
const one). All three branches ran: the area 59 run reaches the getter (the stand-in log is
gone), and a temporary hook (reverted) set the flag byte to drive the 0x04 and the 0x20 branches.

With the burst-fire helper ported the area 59 run (the note's run) still ends cleanly: gameplay
at 10.1 s, exit=0 and `grep -c '[port] caught\|reach-stub'` = 0. The helper runs every frame and
the burst fire asks to fire at least once (the stand-in log for `fn_9_57A0` is in the run), so
the marker on the body is earned.

The read of `fn_9_4D84` that led to the port:
the byte at guest this+0x420 is `CPatterned`'s flag byte and its 0x40 bit is the actor's alive
bit (the host's `GetAlive()`), the body commands are the guest's command id 0x1b = 27 =
`pas::kBSC_AdditiveAim` and 0x1a = 26 = `pas::kBSC_AdditiveIdle` (the host's EBodyStateCmd
matches the DOL's order), the helper it calls first (`fn_9_4CA4`, 0xd8 bytes) was over the 0x80
limit and stood in until 2026-10-20, and the last step writes the aim direction into the body
controller's own vector at +0x40 (the command manager's sixth CVector3f, the command manager
sits at body + 4 and its six vectors run from +0x00 to +0x48). What is missing is the retail's
command payload (the static data objects `lbl_9_data_A78` and `lbl_9_data_AA0` the commands
carry) and the aim flag (the 0x02 bit of the flag byte at guest this+0xC65, the command's third
field): the host's `CBCAdditiveAimCmd` has an `mAimType` with a getter but no setter, so the
flag cannot be expressed without a change to the shared body-command header.

`fn_9_1C0C` (0x2ec, the module's renderable driver) is host code since 2026-10-21: the module's
own scale (the float at guest this+0xC44) is advanced towards its target (guest this+0xC48) and
the module's state word (guest this+0xC4C) follows it, while the module's two own renderables
(guest this+0xC3C and this+0xC40) and its collision actor manager (guest this+0xA3C) stay named
stand-ins - the port has no classes for them.

The module's Think (`fn_9_C944`, 0x184) is host code since 2026-10-20: the base's Think, the
bone tracking's Think (only when the body controller is not frozen) and the module's own helpers
as named stand-ins - `fn_9_3E1C` (0x324) - plus the two collision actor managers and
the visual knowledge, which the port has no classes for. `fn_9_662C` (0x39c) is host code too
since 2026-10-20.

- The Dodge state `fn_9_8934` (0x1e0) is host code since 2026-10-20: the activate part asks
  the module's helper `fn_9_3678` (0x288 bytes, over the 0x80 limit, so a named stand-in that
  returns the neutral value - the dodge id at guest +0xBDC stays -1 and the animation state is
  not armed), the update part asks the animation state for the guest's command id 3 and delivers
  the step command (the direction 1 = `kSD_Backward`, the type 2 = `kStep_BreakDodge`) or, when
  the command may not be issued and the body state is 3, writes the direction to the target's aim
  position into the body controller's target vector, and the deactivate part clears the animation
  state, the dodge id, the flag byte at +0xC63's 0x04 bit and the timer at +0xB68. With the
  player teleported next to the commando the state runs all three parts (two activates, two
  updates, two deactivates in the run).
- The step command the Dodge state delivers (0x00008A10) carries the direction from the module's
  own dodge id at guest this+0xBDC (the load at 0x000089BC) and the type 2 (`kStep_BreakDodge`);
  the host's `CBCStepCmd` takes them in that order (the base, the direction, the type).
- `fn_9_3338` (0x148 bytes) is the module's jump-back helper, ported as
  `x3338_IsJumpBackClear`: the ray from the actor's bounding-box centre (0x00003374, 0x00003380)
  to the point that distance along the given vector (0x000033B0..0x000033D8) must not hit the
  world (the RayCollideWorld call at 0x0000342C) and must be on the path (the OnPath call at
  0x00003440). `ShouldJumpBack` calls it with the forward column negated and the module's float
  at guest this+0xBE0 (the new member `xbe0_`; the constructor stores the animation distance
  times the model's scale there at 0x0000E61C, and the same for the float at +0xBE4 - `xbe4_` -
  at 0x0000E56C; the port leaves both at the constructor's first value, 0.f). So the jump-back
  trigger runs but stays false: the jump-back distance is still zero, so the ray's end point is
  the actor's own centre.
- The two constructor stores the port still owes: `fn_9_D904` sets the float at guest this+0xBE0
  (the jump-back distance) at 0x0000E61C and the one at +0xBE4 (the dodge picker's radius) at
  0x0000E56C - each is an animation's travel distance times the model's scale X (the load at
  0x0000E560 and the multiply at 0x0000E568; the same for the second at 0x0000E610 and 0x0000E618).
  The distance comes from the retail `CPatterned::GetAnimationDistance` (the calls at 0x0000E55C
  and 0x0000E60C) and the host has that query (include/MetroidPrime/Enemies/CPatterned.hpp:206,
  used the same way by the Space Pirate's port, src/MetroidPrime/Enemies/PortCSpacePirate.cpp:334).
  What the port does not have yet is the *animation* each store asks for: the guest builds the
  `CPASAnimParmData` objects with the module's own helper `fn_9_5D74` (the call at 0x0000E54C,
  which fills the parms at sp+0x310, sp+0xC8, sp+0xC0, sp+0xB8, sp+0xB0, sp+0xA8 and sp+0xA0
  from the module's animation list), so the animation ids are still unknown - porting `fn_9_5D74`
  (or reading the module's animation list out of its .rel) is the next step before the two
  stores can be ported at their retail spot.
- `fn_9_5D74` (0x184 bytes) is the module's helper that builds the `CPASAnimParmData` objects
  the constructor asks the animation distances of: it fills nine of them, each eight bytes - the
  state id at +0 and the parm at +4 (the stores at 0x00005DA4/0x00005DAC and the seven that
  follow) - the constructor passes the nine destinations (the stack slot at sp+0x310 and the
  pointers at sp+0xC8, sp+0xC0, sp+0xB8, sp+0xB0, sp+0xA8 and sp+0xA0, the call at
  0x0000E54C). The two parms the ctor then measures are the one at sp+0x310 (the store at
  0x0000E56C) and the one built at sp+0x140 (the store at 0x0000E61C); their state ids and parm
  values are still to be read out of the module's data tables, so the two constructor stores are
  still not ported.
- `fn_9_3678` (0x288 bytes) is the Dodge state's picker - it is the only thing that sets the
  dodge id, and it is not ported yet, so the dodge id stays -1 and the animation state is never
  readied (no dodge). What it does: it walks the listening-AI object list (the state manager's
  +0x830, the first index at +0x2008) and looks for another actor that is not this one and is in
  the same area (the +0x4 area id, the compare at 0x00003718), that is nearer than the module's
  own float at guest this+0xBE4 (the squared compare at 0x00003758), that is in front of the
  actor (the delta dotted with the transform's basis columns at +0x24/+0x34/+0x44, the compare at
  0x00003780) and within 60 degrees of it (rodata_4A4 = 1.0471976, the GetAngleDiff calls at
  0x0000379C and 0x000037E0); it then rolls the random (0x00003894) and asks the jump-back helper
  `fn_9_3338` (0x148 bytes, over the 0x80 limit) twice, and returns the chosen step direction or
  -1 (the result register starts at -1, 0x000036D4).
- The stand-ins the walking commando reaches are `fn_9_8934` (the Dodge state, 0x1e0),
  `fn_9_9F24` (the JumpPointFind state, 0x1ec), `fn_9_B958` (the IsOffPath trigger, 0x38),
  `fn_9_B250` (the ShouldShieldCharge trigger, 0x410) and, with the player close,
  `fn_9_709C` (the CoverFind state, 0x1f0) and `fn_9_BB10` (the ShouldJumpBack trigger, 0x6c -
  ported 2026-10-20; its helper `fn_9_3338` is 0x148 bytes, over the 0x80 limit, so it is a named
  stand-in).
- The grenade path: the bit 0x02 of the flag byte at guest +0xC63 is the "grenade pending" bit.
  The module's timer helper `fn_9_662C` sets it (0x00006850, the mask 1u << (31 - 30) = 0x02) when
  the grenade's attack chance (the data's +0xBC) wins the roll, and the E-grenade's own creator
  `fn_9_5410` (0x370 bytes) clears it again once the grenade object exists (0x00005764). The
  E-grenade fire state is `fn_9_A2EC` (0x174 bytes) - it asks the animation state for the grenade
  command (the guest's command id 0x12, 0x0000A34C) and delivers the grenade command - and it is
  not ported yet, so the E-grenade is never thrown. `ShouldFireEGrenade` also needs the target's
  aim position inside the grenade's band (the data's +0x60 and +0x64).

With the player teleported into the commando's own path region (the console's `tp x y z` over
`MP2_CONSOLE`, the point 30 units in front of the commando at (75.9, 371.6, -47.1)) the commando
walks: it leaves (72.3, 401.4, -47.1) and reaches (64.6, 368.9, -46.1), and `ShouldMeleeAttack`
answers true (the target 8.8-9.5 units away, inside the commando's attack band of 3.0 to 10.0).
`ShouldFireEGrenade` stays false: its flag bit at guest +0xC63 is set by the module's unported
grenade state. Without that `tp` the player starts 111 units away, off the commando's region, and
nothing walks. The run's stand-ins are the six above plus `AcceptScriptMsg`, `CollidedWith`,
`DoUserAnimEvent`, `fn_9_9F24`, `fn_9_8934`, `fn_9_B250`, `fn_9_B958` and `fn_9_3338`;
`grep -c '[port] caught\|reach-stub'` is 0. The E-grenade fire state (`fn_9_A2EC`) is never
entered, so no grenade is thrown.

## 5. What is not ported yet (milestone 2 and forward)

- The state machine's other bodies: the two tables (34 triggers, 36 states) are registered,
  twenty-nine of the triggers and nine of the states are host code (the sections above), but every
  other function they point at is a stand-in that logs the retail function it stands in for and
  returns the neutral value. The triggers that are still stand-ins are `ShouldShieldCharge`
  (fn_9_B250, 0x410), `ShouldBoost` (fn_9_B1E0, 0x70), `ShouldCrouch` (fn_9_B168, 0x40),
  `ShouldWallHang` (fn_9_B114, 0x54) and `CoverBlown` (fn_9_AFD4, 0xe0); the JumpBack state
  (fn_9_8B70, 0x1b8) is a stand-in too.
- The guest's `CVisualKnowledge` (this+0xAB0): the host has no class for it, so the call that
  uses it is a stand-in.
- The `CTryCommand` the base keeps at +0x6B4 is the base's animation state, so the host's
  animation state answers it (`mAnimationState.CanIssueCommand`).
- The melee (`MeleeAttack`, the blade's collision actor and its damage), the E-grenade (the
  `Commando E-Grenade` script object), the shield (arm, charge, the mould) and the burst fire.
- The three `CToken` pairs (the script objects at this+0x8F0, +0x90C and +0x7C8) and the two
  `CElementGen` effects.
- The animation distances at +0xBE0 and +0xBE4 are ported (the ctor's two step distances, the
  scale's Y times the parms (1, 2) and the scale's X times the parms (3, 2)); +0xBE8 (the jump
  distance) and the locomotion-speed term at +0xB90 are not.
- The CVisualKnowledge at +0xAB0 (the host has no such class yet).

`fn_9_C598` (0x13c, the actor's render - the vtable slot 0x2c) is ported (2026-10-21, the port's
`Render`): the guest fades the actor with its colour's alpha (the psq_l at 0x0000C5C4 reads the
alpha byte at guest this+0x44F and rodata_4F0 is 1/255) and, while the actor is alive (the byte
at guest this+0x420's 0x40 bit) and its own word at guest this+0x920 is 1 (the port's `x920_`),
it draws the space warp at the bounding box's centre point with the sine of pi times the alpha
(rodata_4F4, the FastSinR call at 0x0000C5F8 and the DrawSpaceWarp call at 0x0000C634). With an
alpha above zero the base's own render runs (the call at 0x0000C650) and the module's own
renderable follows - the one at guest this+0xC3C when the module's state word at guest this+0xC4C
(the port's `xc4c_`) is 0 and the one at guest this+0xC40 when it is 1. The port has no such
renderables, so those two remain named stand-ins and the function is not marked. The base's render
ran in the area 59 run (the stand-in log is gone); the two renderables and the warp's draw did not
(the module's state word is not 0 or 1 in the run, and the actor's alpha is 1.0, which makes the
sine 0).
