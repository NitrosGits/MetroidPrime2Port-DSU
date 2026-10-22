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
| 0x9C | `KnockBack` | `fn_9_C300` (0x1fc) | host, **marked** (the module's own: the reaction, the base's knock-back, the "GetUp" state and the two sounds; the rag-doll branch is dead) |
| 0xA8 | `CanBeShot` | `fn_9_A4` | host (true) |
| 0xAC | `IsListening` | `fn_9_8` | host (true) |
| 0xB0 | `Listen` | `fn_9_BE4C` (0xf8) | host, **marked** (section 6.1) |
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
| 19 | WallHang | `fn_9_7358` | 0x6E6 | on activate: the 0x02 bit of the flag byte at guest this+0xC65 is set, the 0x40 bit of the byte at guest this+0xC66 is cleared, the module's word at guest this+0x924 becomes 0 and the animation state becomes "ready"; then the cover point the module holds (guest this+0xBD2) is looked up - when there is none the state stops there - and the connected waypoint ("ARRV" on "NEXT") is looked for (its id goes to guest this+0xBD6) while the cover point's forward vector goes to guest this+0xBA0. On update: the burst fire's own type is cleared and the word at guest this+0x924 picks the block - 0: the angle between the actor's facing and the cover point's forward (10 degrees, rodata_498) makes the word 1 or delivers the locomotion command that looks at the cover point; 1: the wall-hang command goes to the body controller when the animation state lets it through, and while the body is in the wall-hang state the direction from the actor to the target's aim position goes into the body controller's own vector at +0x34 | the guest's wall-hang command carries the module's own animation object and the waypoint id and the guest keeps them in the base's fields at guest this+0x2F4 and +0x2F8; the host's command has no room for either, so the port delivers the host's kBSC_WallHang. The retail's body controller +0x34 is the host's command manager's target vector, so the port calls CommandMgr().SetTargetVector |
| WarpOut | `fn_9_8DF0` | 0x7BA | on activate only: the animation state becomes "ready", the module's state word (guest this+0x920) becomes 1, the 0x40 bit of the flag byte at guest this+0xC65 is cleared and the cover point the module holds (guest this+0xBD2) is released. On update: the animation state is asked whether the generate command may be issued (the guest's command id 0xc = 12 = kAS_Generate) and, when it may, the generate command goes to the body controller (the type 1 = pas::kGType_One, the zero target vector, the animation id -1 and the flags clear); when it may not, the fade runs while the body's own state is the generate state (12): the first time the 0x40 bit of the flag byte at guest this+0xC65 is set and the body's remaining animation time is kept at guest this+0xC58, and after that the ratio of the remaining time to the kept one is clamped to 1.0 and scaled by 255.0 to make the alpha byte at guest this+0x44F | the guest's command carries the module's own animation object (lbl_9_data_A2C) and the guest copies the command into the base's field at guest this+0x20C; the host's command has no room for either, so the port delivers the host's CBCGenerateCmd (the type 1, the id -1) and the host's command manager keeps the command. The guest casts the float to a byte (its own float-to-byte cast: it puts the float on the stack and reads its low byte, 0x00008FC4..0x00008FC8) - the host's counterpart is CCast::ToUint8, which the port calls |
| PostWarpOut | `fn_9_8D28` | 0x7AD |
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
| HasLineOfSight | `fn_9_BAEC` | 0x576 | `mVisualKnowledge.IsTargetVisible()` (guest +0xAE8 0x80 = the CVisualKnowledge's +0x38) | - |
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
- The counters at +0xB50, +0xB54 and +0xB58: the constructor stores 0 in all three (0x0000DA84,
  0x0000DA94 and 0x0000DAA4). `fn_9_6390` raises the one at +0xB50 every time the burst fire
  fires (0x00006594) - it is the number of shots - and compares it with the one at +0xB58, which
  it raises to the +0xB50 value plus one when the shield's arm chance roll fails (0x0000643C).
  The one at +0xB54 is the number of shield charges: the two shield charge states raise it to the
  +0xB50 value plus one when the charge starts (0x00007B78 in `fn_9_7A98` and 0x000080E4 in
  `fn_9_8028`). The port's members are `xb50_`, the new `xb54_` and `xb58_`.
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
| SelectTarget | `fn_9_AB08` | 0x6EF | on activate, when the old target is gone: take the player as the target, point the bone tracking at it, send `kSS_Attack` with `kSM_None`, and switch to the combat locomotion | the visual knowledge (this+0xAB0) is the port's CVisualKnowledge (2026-10-22) |
| FaceTarget | `fn_9_ABDC` | 0x6E4 | activate: the module's own helper `fn_9_3CBC` asks the target for its aim position (guest this+0xB94) and the state measures the angle between the actor's facing and the direction to it (0.34906584 rad = 20 degrees) - the animation is "over" when the target is in front and "ready" when it is not; update: the animation state's own command test, the direction to the target, its magnitude test and the locomotion command that walks there; deactivate: the animation state back to "not ready" | the guest's `CTryCommand::ShouldRetry` is the base's animation state, so the port calls `mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Turn)` (the guest's command id 8) |
| SetTargetDest | `fn_9_AA20` | 0x6FC | on activate only: the destination starts as the actor's own position (guest this+0x54) and the 0x80 bit of the flag byte at guest this+0xC65 is cleared; the target (the id at guest this+0xBCA) is looked up and, when it is there and either the flag byte at +0xC64 has its 0x40 bit or the byte at +0xAE8 does not have its 0x80 bit, the destination becomes the target's position and the 0x80 bit of +0xC65 is set; then the path find navigation's destination is set, its face target id (guest this+0x718, the navigation's own member at +0x24) is cleared and the module's helper `fn_9_19D8` is called | `fn_9_19D8` is 0xc4 bytes, over the 0x80 limit, so it is a named stand-in; the guest reads the target's position without a cast, so the port casts to CActor (the host's CEntity has no position) |
| SetPathMeshDest | `fn_9_A838` | 0x725 | on activate: the 0x80 bit of the flag byte at guest this+0xC65 is cleared, the path find search (guest this+0x928) looks for the closest reachable point to the actor's own position (the out point starts as the zero vector) and the path find navigation (guest this+0x6F4) takes that point as its destination; when the search finds no point the navigation takes the actor's own position and the flag byte's 0x80 bit stays clear; the navigation's face target id (guest this+0x718) is cleared either way | the host's `CPathFindSearch::FindClosestReachablePoint` takes the same two arguments as the guest's (the source and the out point) |
| PostWarpOut | `fn_9_8D28` | 0x7AD | on activate only: when the byte at guest this+0xC63 has no 0x20 bit (the guest's `extrwi. r0, r0, 1, 26` at 0x00008D54 takes the bit 26 from the MSB, which is the LSB bit 5, and the branch at 0x00008D58 skips the message when it is set) the commando tells the world it leaves - a script message whose sender is the actor's own id (the id at guest this+0x8, 0x00008D60) and whose message is `kSM_Deactivate` (0x44435456, 0x00008D70); then the module's state word (guest this+0x920) goes back to -1 (0x00008DAC..0x00008DB0) and, when the same byte has its 0x10 bit (the `extrwi. r0, r0, 1, 27` at 0x00008DB8 - the bit 27 from the MSB, the LSB bit 4), the actor asks to be deleted (0x00008DC0..0x00008DD4) | the guest builds the message struct on the stack (0x00008D78..0x00008DA4); the port uses the host's `CScriptMsg` |

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
- The guest's rag doll (guest this+0xBFC) is the port's member `xbfc_ragDoll` (a
  `rstl::single_ptr<CPirateRagDoll>`, upstream's module 50 class): the constructor stores 0 there
  (0x0000DC18) and only `fn_9_3E1C` creates it, after the death (see "The rag doll" below). The
  module's `GetRagDoll` (fn_9_78) stays `li r3, 0x0` - the guest never exposes it - so the port's
  `GetRagDoll()` stays null too and the module's own bodies read the member. The rag-doll
  branches of `fn_9_CAC8` (PreThink), `fn_9_C6D4` (PreRender) and `fn_9_C51C`
  (PreRenderAllViewports) are ported with it; the rest is the bone tracking's own PreThink and
  PreRender (the calls at 0x0000CB10 and 0x0000C77C) and the base's own calls.
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
| 0xA3C, 0xA40 | the two collision actor managers | fn_9_45A4 builds them in the kSM_Create case |
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

## 3b. The message switch (fn_9_CB40, 0x754)

fn_9_CB40 is the module's AcceptScriptMsg (the vtable slot 0x18). It calls the base's
AcceptScriptMsg first (0x0000CB84) and then switches on the message's four-character code (the
binary search at 0x0000CB88..0x0000CCC8). The byte at guest this+0x20 is CEntity's flag byte -
the DOL's CEntity::SetActive writes its 0x80 bit
(MetroidPrime2Port/build/G2ME01/asm/MetroidPrime/CEntity.s:678) - so the register r31, computed
from it at 0x0000CB60..0x0000CB80, is the entity's "active" bit.

| code | message | the case's body |
|---|---|---|
| XCRT | kSM_Create | the body's Activate (0x0000CCD8) and the module's collision actors (0x0000CCE4) |
| DCTV | kSM_Deactivate | the collision actors' SetActive(false) (0x0000CD90..0x0000CEBC) and the team (0x0000CDC8) |
| ACTV | kSM_Activate | the colour's alpha byte (guest this+0x44f) and the base's alpha delta (guest this+0x40c), then the flag byte's 0x20 bit (0x0000CD3C..0x0000CD80) |
| XDEL | kSM_Delete | the collision actors' Destroy, ReleaseCoverPoint (the id at guest this+0xBD2), DeleteObjectRequest (the id at guest this+0xBD8) and the team (0x0000CCEC..0x0000CD38) |
| ESCP | kSM_Escape | the module's warp-out helper fn_9_3D64 with true (0x0000CF2C..0x0000CF4C) |
| XALD | kSM_AreaLoaded | the path area and the connected waypoint; or - when the block's flag byte at guest this+0x91C has its 0x01 bit - the actor's death (0x0000CE04..0x0000CEF0) |
| STRT | kSM_Start | the flag byte at guest this+0xC64 loses its 0x20 bit (0x0000D060) |
| STOP | kSM_Stop | the flag byte at guest this+0xC64 gets its 0x20 bit (0x0000D074) |
| XAUD | kSM_AIUpdateDisabled | the collision actors' SetPhysicsActive(false) (0x0000CDD0..0x0000CDFC) |
| XLNN | kSM_Landed | the burst fire's type goes to 0 (guest this+0xAF0) and the flag byte at guest this+0xC63 loses its 0x08 bit (0x0000D048) |
| XDMG | kSM_Damage | the module's knock-back (only when the byte at guest this+0xC65 has its 0x04 or 0x08 bit), the module's counter at guest this+0xC50, InformListeners and AbortScriptedAnimations (0x0000D09C..0x0000D160) |
| XHIT | kSM_HitObject | the module's hit-object helper fn_9_3A5C (0x0000D25C) |
| XLAU | kSM_Launching | the actor's own off-ground message through the base's handler, the momentum and the module's jump-back velocity fn_9_3900 (0x0000CFA4..0x0000D040) |
| XRDG | kSM_ResistedDamage | the module's knock-back (0x0000D164..0x0000D258) |
| XOFF | kSM_OffGround | the momentum (unless the body is frozen) and the burst fire's type goes to 1 (0x0000CF54..0x0000CF9C) |
| ALRT | kSM_Alert | the base's flag byte at guest this+0x420 gets its 0x80 bit - the host's mHitByPlayerProjectile (0x0000D088) |
| ZERO | kSM_SetToZero | the flag byte at guest this+0xC63 gets its 0x40 bit and the module's warp-out helper runs with false (0x0000CEF8..0x0000CF24) |

The guest's own members the switch touches: the burst fire's type (guest this+0xAF0), the
counter at guest this+0xC50, the three flag bytes at guest this+0xC63, +0xC64 and +0xC65, the
word at guest this+0x920, the block's two flag bytes at guest this+0x91C and +0x91D, the
colour's alpha byte at guest this+0x44F and the alpha delta at guest this+0x40C (the base's own
two members), the base's flag byte at guest this+0x420 (its 0x80 bit) and the ids at guest
this+0xBC4 (the team), +0xBC6 (the shield's collision actor), +0xBC8 (the blade's collision
actor), +0xBD2 (the cover point) and +0xBD8 (the deleted object).

Two of the base's members are the guest's own: the colour's alpha byte (guest this+0x44F) is the
host's `CColor`'s alpha and the float at guest this+0x40C is the host's `mAlphaDelta` (the DOL's
CPatterned::UpdateAlphaDelta writes both, at 0x000747F0 and 0x000747E4). The byte at guest
this+0x440 is the host's `mCurDamageRemTime` and the one at guest this+0x444 its
`mDamageWaitTime` (the host's CHealthInfo's layout puts mHealth second, at the guest's +0x04).

### The base's animation state (guest +0x6B4)

The word at guest this+0x6B4 is the base's own animation state (the DOL's CPatterned constructor
initialises it and CPatterned::GetAnimOver at 0x80151990 reads it), not a module member: the
module writes it (the stores at 0x00006A88, 0x00008728 and 0x000087A4 among others) and the guest
asks the base's own try command about it (ShouldRetry__11CTryCommand at 0x0000ACC8). The DOL's
CTryCommand::ShouldRetry (0x801B73E4) is exactly the host's CAnimationState::CanIssueCommand -
the same three states (2 repeat, 1 ready, 3 over). The port therefore uses the base's
`mAnimationState`; there is no `x6b4_` member.

### The charge state (fn_9_8028, 0x68c)

The charge state is host code since 2026-10-21. Its activate (0x00008088) is what puts the
shield's "up" bit (the byte at guest this+0xC65's 0x04 bit) up and raises the counter at guest
this+0xB54 (0x000080DC); its update (0x00008240) turns the commando towards the charge direction
and then charges with it; its deactivate (0x0000859C) takes the shield down again (the byte's
0x04 bit, 0x00008678) and ends the team's attack. The per-branch table above has the evidence of
what the run reaches.

### The trigger that enters the scripted charge (fn_9_B1E0, 0x70)

The ShouldBoost trigger is host code since 2026-10-21: the byte at guest this+0xC66's 0x80 bit
(0x0000B1F0) and the byte at guest this+0xC64's 0x01 bit (0x0000B1FC) have to be right and the
AI waypoint the module follows (the id at guest this+0x6C0) has to allow the boost - the flags of
the waypoint (the word at its +0x164) have to have their 0x20 bit (0x0000B22C).

### The waypoint picker (fn_9_2A98, 0x214)

The waypoint picker is host code since 2026-10-21: the charge speed (the block's +0x12C) has to
be above zero (0x00002AD0), the AI waypoint has to be there and allow the boost (0x00002B00), and
the module's own connection search finds the waypoint's "arrived" connection (0x00002B44: the
state kSS_Arrived, the message kSM_Next and the module's own waypoint predicate), whose waypoint
becomes the module's waypoint (0x00002B8C) with its position (0x00002B90) and its forward column
(0x00002BAC) as the module's own vectors. The module's time to the waypoint (the float at guest
this+0xB88) is the distance over the charge speed (0x00002BF8).

### The shield effect (fn_9_2048, 0x330)

The shield effect is host code since 2026-10-21: the byte at guest this+0xC64's 0x01 bit stops
it (0x00002064), the module's state word at guest this+0xC4C picks the effect (the charge at 1,
the armed shield at 0, anything else nothing) and **both** effects put their explosion into the
world (the AddObject at 0x000022E8), start the block's explode sound (0x00002324) and reset the
module's state word and its two scale floats (0x0000234C..0x00002360) - the three fields are reset
even when there is no effect at all (the guest reaches 0x000022D8 with no explosion when the state
word is neither 0 nor 1 or the token is not loaded, and the reset is at 0x0000234C). The guest's two
CGenDescription tokens (the ones at guest this+0xC08 and this+0x0C18, which the constructor
fills from the block) are the block's charge effect and its explode effect; the port reads the
same two asset ids (the block's +0x78 and +0x58).

### The module's collision actor managers (fn_9_45A4, 0x304)

fn_9_45A4 is the module's own helper, called by its kSM_Create case. It builds two managers, each
with one sphere joint:

- the shield's (guest this+0xA3C): a sphere on the left wrist (guest this+0xC62), radius 1.0f
  (rodata_43C), mass 1000.0f (rodata_4B0), named "Shield_CollisionActor" (the module's rodata at
  0x893). Its collision actor gets the shield's charge vulnerability (the block's +0xF4 = guest
  this+0x8B4) and the response type 0x12 = 18 = kWCR_EnemyShielded (0x000046EC..0x000046F4).
  The actor's id goes to guest this+0xBC6.
- the blade's (guest this+0xA40): a sphere on the second gun locator (guest this+0xC5F), the same
  radius and mass, named "Blade_CollisionActor" (the module's rodata at 0x8A9). Its collision
  actor gets the pass-through vulnerability (0x00004814..0x00004820). The actor's id goes to guest
  this+0xBC8.

The guest then adds the bit 0x8000 (the material 15, kMT_Glass) to the actor's own material
filter's exclude list (0x00004830..0x0000487C). The module's Think updates both managers with the
object-space option (0x0000C9B8 and 0x0000C9CC).

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
| `fn_9_C798` | the default arm only: the base's own `DoUserAnimEvent` for `kUE_LoopedSoundStop` (67 times in the run) | `kUE_Projectile` (the E-grenade's throw, the grenade state `fn_9_A2EC` is a stand-in), `kUE_DamageOn` (the melee's damage event, the melee state `fn_9_A4BC` is a stand-in), `kUE_DeGenerate` and `kUE_BecomeRagDoll` (the death animations, the Dead state `fn_9_69C8` was a stand-in then - it is ported since 2026-10-21), `kUE_Landing` and `kUE_TakeOff` (the jump animations, the JetBoost state `fn_9_9BB4` is a stand-in), `kUE_BreakLockOn`, `kUE_EventStart` and `kUE_Activate` (the shield's animations; the ArmShield state `fn_9_86B4` is ported since 2026-10-21, but area 59's own state machine gives its trigger no target state, so the state is only reached through the console's `fsm` - see the per-branch table) |
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
`fn_9_69C8` (the Dead state, 0x78), `fn_9_A7A8` (SetCoverDest,
0x90), `fn_9_A8E8` (SetJumpDest, 0x94), `fn_9_6BCC` (BreakCover, 0x9c), `fn_9_A97C`
(SetRetreatDest, 0xa4), `fn_9_8D28` (PostWarpOut, 0xc8), `fn_9_728C` (WallDetach, 0xcc),
`fn_9_AFD4` (CoverBlown, 0xe0), `fn_9_6A40`
(GetUp, 0x130), `fn_9_7724` (Crouch, 0x148), `fn_9_6C68` (CoverAttack, 0x154), `fn_9_A2EC`
(EGrenadeAttack, 0x174), `fn_9_8B70` (the JumpBack state, 0x1b8), `fn_9_A110`
(PostEGrenadeAttack, 0x1dc), `fn_9_9BB4` (JetBoost, 0x1fc), `fn_9_8DF0` (WarpOut, 0x218),
`fn_9_786C`
(RestoreOrientation, 0x22c), `fn_9_6DBC` (Cover, 0x284),
`fn_9_A4BC` (MeleeAttack, 0x290),
`fn_9_7358` (WallHang, 0x310),
`fn_9_9064` (WarpIn, 0x528) and `fn_9_8028` (ShieldCharge, 0x68c). The ones the area 59 run
reached - `fn_9_1C0C`, `fn_9_3E1C` and `fn_9_B250` - are host code since 2026-10-21 (the
paragraphs above), and so are `fn_9_B1E0` (ShouldBoost, 0x70), `fn_9_961C`
(FollowAttackPattern, 0x350), `fn_9_7A98` (ScriptedShieldCharge, 0x590), `fn_9_2474` (the turn
helper, 0x580), `fn_9_2A98` (the waypoint picker, 0x214) and `fn_9_86B4` (the ArmShield state,
0x280); the rest of that list still stands in.

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

- The body controller's byte at guest this+0x48C + 0x594 is the retail `CBodyController`'s flag
  byte, and its 0x20 bit is the **frozen** flag, not an unused bit: the retail class layout (the
  actor pointer, the command manager 0x368, the state info 0x204, the rotation and five words)
  puts the flags at +0x594, and MWCC allocates the bitfields from the MSB, so the bits are
  mAnimationOver 0x80, mActive 0x40, mFrozen 0x20, mHasBeenFrozen 0x10 and mPlayDeathAnims 0x08.
  The Dark Commando (module 3) reads the same byte with the same mask (the extrwi at 0x00006920
  and 0x00006978) and its own source guards the same two calls with
  `mBodyController->IsFrozen()` (src/MetroidPrime/Enemies/CDarkCommando.cpp:451 and :459), so the
  port's `BodyController()->IsFrozen()` is that bit. `fn_9_1C0C` only sets the 0x04 bit of the byte
  at guest this+0xC64 and deactivates the module's collision actor manager when the byte at
  guest this+0xC66 has its 0x20 bit **and** the body is frozen (0x00001C3C..0x00001C70).
- The state word's dispatch (0x00001D14..0x00001ED4): 1 drives the renderable at guest this+0xC40
  and 0 the one at this+0xC3C, but the state-1 branch tests the renderable at this+0xC40 for null
  (0x00001D30..0x00001D38) and falls through to the state-0 block when it is null. The port has
  no renderable at this+0xC40, so its state-1 branch always falls through to the state-0 block.

The module's Think (`fn_9_C944`, 0x184) is host code since 2026-10-20: the base's Think, the
bone tracking's Think (only when the body controller is not frozen) and the module's own helpers
as named stand-ins - plus the two collision actor managers and
the visual knowledge, which the port has no classes for. `fn_9_662C` (0x39c) is host code too
since 2026-10-20.

`fn_9_3E1C` (0x324, the module's rag-doll driver) is host code since 2026-10-21 (the port's
member `x3E1C_UpdateRagDoll`); its rag-doll path and the rag doll's creation, the stand-ins
until 2026-10-22, are ported - see "The rag doll" below.

`fn_9_B250` (0x410, the ShouldShieldCharge trigger) is host code since 2026-10-21 (the port's
`ShouldShieldCharge`): the flag byte at guest this+0xC64 must have its 0x40 bit, the block's
flag byte at +0x15D (guest this+0x91D, `mData.mFlags1`) must not have its 0x40 bit and that same
byte's 0x01 bit must be clear; the module's counter at guest this+0xB50 (the number of shots the
burst fire has fired - `fn_9_6390` raises it at 0x00006594) must be above the module's counter at
guest this+0xB54 (the number of shield charges - the two shield charge states raise it at
0x00007B78 and 0x000080E4); the target (the id at guest this+0xBCA) must be there, the height
difference at most 5.0f (lbl_9_rodata_468) and the squared distance inside the charge band.
The band's two floats are the block's +0x124 and +0x128 (guest this+0x8E4 and this+0x8E8): the
loader's shield half starts at the block's +0xD8 and its own minimum and maximum charge
distance are at +0x4C and +0x50 (`fn_9_EF80` stores them at 0x0000F064 and 0x0000F06C), so they
are `mShield.mChargeMinAttackDist` and `mShield.mChargeMaxAttackDist`. Then the team: when the
module's team id at guest this+0xBC4 is not kInvalidUniqueId the team AI manager must let the
commando start a melee attack (the CanStartAttack call at 0x0000B3D0 with the attack type 0, the
team id and the actor's own id at guest this+0x8 - the Dark Commando's own melee ask passes the
same type at 0x000060D4). Finally three rays from the actor's own bounding box (the centre, and
the centre plus and minus the box's x extent times the transform's right column - the guest's
this+0x24, +0x34 and +0x44, the transform's m00, m10 and m20) along the direction to the target,
each as long as the distance to it, must all hit the world (0x0000B548, 0x0000B598 and
0x0000B5E8). The guest's material filter includes the material 59 (kMT_Solid) and excludes the
materials 32 (kMT_Player) and 49 (kMT_Platform) - the module's data words at lbl_9_data_7D0,
lbl_9_data_7D4 and lbl_9_data_7D8.

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
  to the point that distance along the given vector (0x000033B0..0x000033D8) must be clear of the
  world (the RayCollideWorld call at 0x0000342C - the answer is false when the ray is blocked) and
  the point must be on the path (the OnPath call at 0x00003440 - the answer is true when it returns
  0). The guest's filter includes the material 59 (kMT_Solid - the module's .data word at
  lbl_9_data_80C) and excludes the material 47 (kMT_CollisionActor - the word at lbl_9_data_810):
  the two `__shl2i` calls at 0x000033DC and 0x000033F8, the type kFT_IncludeExclude at 0x0000341C.
  `ShouldJumpBack` calls it with the forward column negated and the module's float
  at guest this+0xBE0 (the new member `xbe0_`; the constructor stores the animation distance
  times the model's scale there at 0x0000E61C, and the same for the float at +0xBE4 - `xbe4_` -
  at 0x0000E56C; the port leaves both at the constructor's first value, 0.f). The jump-back
  distance is still zero, so the ray's end point is the actor's own centre: the guest's
  degenerate ray counts as clear (the host's RayCollideWorld answers true when the direction
  cannot be normalized) and the trigger then answers what the path finder says about the centre.
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

### The run with the message switch (2026-10-21)

The area 59 run with the ported message switch and the charge state (`tools/run_gameplay.sh`,
`TELEPORT=05_temple_federationsite_dark`, `FRAMES=9000`, the card in /tmp/opencode/u_ds9,
`MP2_GODMODE=1`, the console's `tp 75.9 371.6 -47.1` over `MP2_CONSOLE`, the console's `face`
towards the commando and the player firing at it with the console's `press a`) reaches gameplay at
10.0 s, ends with exit=0 and `grep -c '[port] caught|reach-stub'` = 0. The commando walks to the
player (it ends at (78.6, 370.9, -46.1) beside the player at (81.2, 371.4)) and the shots hit
it: the base's own knock-back runs, the module's message switch sees `kSM_Damage` and
`kSM_ResistedDamage`, and in the run where the shots kill it the rag-doll event fires and the
commando's rag-doll helper (fn_9_3E1C) stands in. The run's stand-ins are three in the last run
(the CVisualKnowledge's Update and SetVisualTarget, guest this+0xAB0, and the JumpBack state
fn_9_8B70) and five in the runs where the shots reach the commando (the module's own KnockBack
fn_9_C300 and its Listen fn_9_BE4C join them). The two stand-ins the
earlier slice removed - the module's AcceptScriptMsg and the module's collision actor managers -
stay gone. A second run with the same teleport and
without the firing ends the same way (exit=0, `grep -c '[port] caught|reach-stub'` = 0) with two
stand-ins, the CVisualKnowledge's Update and SetVisualTarget.

The arms of the five functions the task names, read out of a run with a temporary branch log
(reverted, not committed):

| function | arms that ran | arms that did not run |
|---|---|---|
| `fn_9_CB40` | `kSM_Create` (1), `kSM_AreaLoaded` (1), `kSM_Landed` (826), `kSM_OffGround` (122), `kSM_Damage` (6), `kSM_ResistedDamage` (3), `kSM_Launching` (1), `kSM_Delete` (1) and the default arm (1713) | `kSM_Deactivate`, `kSM_Activate`, `kSM_Escape`, `kSM_Start`, `kSM_Stop`, `kSM_AIUpdateDisabled`, `kSM_HitObject`, `kSM_Alert` and `kSM_SetToZero` - the script connections that would send them do not reach this actor in this area, and no host code sends `kSM_HitObject` to an enemy |
| `fn_9_C798` | the default arm (46) and `kUE_BecomeRagDoll` (3) - the commando died and the rag-doll event fired | `kUE_Projectile`, `kUE_DamageOn`, `kUE_DeGenerate`, `kUE_Landing`, `kUE_TakeOff`, `kUE_BreakLockOn`, `kUE_EventStart` and `kUE_Activate` - they come from the module's own animations, and the states that play them (the melee, the E-grenade, the shield and the jump) are stand-ins |
| `fn_9_BF44` | the early return only (1264) | the shield's own arms - they need the 0x04 bit of the byte at guest this+0xC65, and only the module's shield-charge states (fn_9_7A98 and fn_9_8028) set that bit |
| `fn_9_1C0C` | the early return only (2253) | the arms past it - the module's state word at guest this+0xC4C stays -1, because only the shield's animation events (fn_9_1F88 and fn_9_1EF8) set it, and those need the shield's bits at guest this+0xC65 |
| `fn_9_B250` | the first gate only (415) | every arm past it - the 0x40 bit of the byte at guest this+0xC64 is set by the module's own timer helper (fn_9_662C) when the aggressiveness roll wins, and the counter at guest this+0xB54 that the gate also asks about is raised only by the module's shield-charge states |

None of the five is marked: every one of them still has an arm that the port can reach and that
did not run (`fn_9_C798`'s damage and death events, `fn_9_BF44`'s and `fn_9_1C0C`'s shield arms,
`fn_9_B250`'s charge arms and `fn_9_CB40`'s script messages). The three markers that the earlier
slice put on `fn_9_BF44`, `fn_9_1C0C` and `fn_9_B250` are gone for that reason.

A run without the firing (the same teleport, the player standing still) is clean too - exit=0
and `grep -c '[port] caught|reach-stub'` = 0 - and in that run the commando survives, shoots at
the player and its `fn_9_B250` trigger answers **true**: the module's ShieldCharge state
(fn_9_8028) is entered, so the arms of `fn_9_B250` past the first gate (the target, the height,
the charge band and the three rays) do run. Its arm at the counter (guest this+0xB54) is still
out of reach, because only the charge states raise that counter. The run's stand-ins are the
CVisualKnowledge's Update and SetVisualTarget and the ShieldCharge state itself.

The firing run has one more line in the log: the shots also hit a metroid in the room, and that
module's own stand-in (`CMetroid::ApplyDamageGrowth`) logs a reach-stub. It belongs to the
metroid's module, not to this one.

### The per-branch table (2026-10-21)

The table below lists every branch of the five functions the task names, with the class the run
gives it:

- **a** - the branch is compiler-generated (the switch's dispatch, the prologue's own tests).
- **b** - the branch ran; the probe's line is the line of the temporary probe in the instrumented
  build (the probes are not committed - the committed file is the clean one).
- **c** - the branch is unreachable with the shipped data; the evidence is in the table.
- **d** - the branch did not run, so the body gets no `// @ fn_9_<hex>` marker.

The runs the table draws on are the area 59 runs of 2026-10-21 (`tools/run_gameplay.sh`,
`TELEPORT=05_temple_federationsite_dark`, the card in /tmp/opencode/u_ds9, `MP2_GODMODE=1`, the
driver `tools/drivers/commandopirate_shield.py` (the console's `tp` relative to the commando's live
position, the console's `aim` and `press a`, and the console's `fsm` for the states area 59's asset
cannot reach), `MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8` for the fast runs. The console has no
`aim <yaw> <pitch>` in older trees; the merged port has it, and the driver uses it.

**The charge needs the player 10 to 20 units away.** The commando's block says so: the charge band
is 10.0 to 20.0 units (the block's +0x124 and +0x128), the charge speed is 55.0 (the block's
+0x12C), the arm chance is 75.0 (the block's +0x140) and the aggressiveness is 25.0 (the block's
+0x4C) - the port prints all four from its constructor. The commando walks to about two units
from the player, so the band check only passes while it approaches, and the aggressiveness bit (the
0x40 bit of the byte at guest this+0xC64, set by the module's timer helper fn_9_662C) has to be
set during that window. The driver steps the player back to thirteen units every round, which keeps
the commando crossing the band until the roll wins: the module's charge state (fn_9_8028) then
runs its activate, its updates and its deactivate, and the trigger answers true.

| function | branch | class | evidence |
|---|---|---|---|
| `fn_9_CB40` | the switch's binary search (0x0000CB88..0x0000CCC8) | a | the compiler's own dispatch |
| | `kSM_Create` (0x0000CCCC) | b | once per run |
| | `kSM_AreaLoaded` (0x0000CE04) | b | once per run |
| | `kSM_Landed` (0x0000D048) | b | 5204-10926 per run |
| | `kSM_Damage` (0x0000D09C) | b | 3-12 per run (the runs where the shots land) |
| | `kSM_ResistedDamage` (0x0000D164) | b | 17-47 per run |
| | `kSM_HitObject` (0x0000D25C) | b | 272-600 per run (the shield's own hit) |
| | `kSM_OffGround` (0x0000CF54) | b | 19-122 per run |
| | `kSM_Launching` (0x0000CFA4) | b | once per run |
| | `kSM_Delete` (0x0000CCEC) | b | once per run (at the end) |
| | the default arm (0x0000D278) | b | 4365-21853 per run |
| | `kSM_Deactivate` (0x0000CD88), `kSM_Activate` (0x0000CD3C), `kSM_Escape` (0x0000CF2C), `kSM_Start` (0x0000D060), `kSM_Stop` (0x0000D074), `kSM_AIUpdateDisabled` (0x0000CDD0), `kSM_Alert` (0x0000D088), `kSM_SetToZero` (0x0000CEF8) | d | no script connection of this actor sends them in area 59 |
| | `fn_9_3A5C` (the hit-object helper) | b | 272-600 per run |
| `fn_9_C798` | the jump table (0x0000C8FC) | a | the compiler's own dispatch |
| | the default arm | b | 113-390 per run |
| | `kUE_Activate` (the entry 29) | b | once in the run where the shield's animation played |
| | `kUE_BecomeRagDoll` (and `kUE_DeGenerate`, the same case) | b | three times in the run where the shots kill the commando |
| | `kUE_Projectile` | d | the E-grenade's animation is played by the module's unported grenade state |
| | `kUE_DamageOn` | d | the base's damage animation can fire it, but the commando's own flinch does not in the port (the commando takes damage - `kSM_Damage` runs - and the event never does) |
| | `kUE_Landing`, `kUE_TakeOff` | d | the module's jump animations come from its unported jet-boost state |
| | `kUE_BreakLockOn` | d | the shield's lock-on animation comes from the module's unported shield states |
| | `kUE_EventStart` | d | the shield's animation event - the module's animation that carries it comes from the unported shield states |
| `fn_9_BF44` | the early return (0x0000BF7C) | b | 3943-11571 per run |
| | the shield's object hit (0x0000BF90) and the charge damage (0x0000BFB0) and the flag byte (0x0000C03C) | b | 12-47 per run, while the shield is up |
| | the collision list (0x0000C050) and the floor entry (0x0000C1A4) | b | 2-18 per run |
| | the filter hit (0x0000C134) and the steep normal (0x0000C170) | d | the commando's shield has to hit a wall or a ceiling. The driver walks the player into a wall with the console's `stick` and puts the commando next to it, but the sphere on the commando's left wrist never reaches the wall surface: the charge stops two units short of the player and the commando's own collision list only ever reports the floor |
| `fn_9_1C0C` | the early return (0x00001C34) | b | 2885-14885 per run |
| | the frozen branch (0x00001C3C), the scale (0x00001CB4, 0x00001CE4, 0x00001CEC), the state-1 branch (0x00001D14) and the state-0 block (0x00001E04) | d | they need the module's state word at guest this+0xC4C, which only the shield's animation events (fn_9_1F88, fn_9_1EF8) set, and those need the module's unported shield animations |
| `fn_9_B250` | the first gate (0x0000B2B8) | b | 856-5663 per run |
| | the counter (0x0000B2D4) | b | 18-156 per run (after the charges, the counter catches up) |
| | the height (0x0000B324) | b | 14-681 per run |
| | the charge band (0x0000B370) | b | 24-461 per run (the commando is inside or outside the band) |
| | the first ray (0x0000B548) | b | 10-872 per run (the way to the target is blocked) |
| | the third ray (0x0000B5E8) | b | 38 per run (the third ray is blocked) |
| | the true arm (0x0000B5F4) | b | once to three times per run - the charge starts |
| | the flag bytes (0x0000B2C0, 0x0000B2CC) and the target (0x0000B2F8) and the second ray (0x0000B598) | d | they need the script's flags to forbid the charge, the target to be gone or the second ray to be blocked |
| `fn_9_961C` (the FollowAttackPattern state) | the message dispatch (0x0000965C..0x00009678) | a | the compiler's own dispatch |
| | the activate (0x00009680) and its two waypoint sources (0x00009684 and 0x000096B8) | b | 4 activates per run: the first takes the "ATTK"/"FOLW" connection, the later ones the waypoint the scripted charge left at guest this+0xBDA. **forced (fsm)**: the area's own route into the state goes through the RestoreOrientation state, which is still a port stand-in, so the state is only entered when the console forces it |
| | the waypoint lookup (0x000096DC) and the "the waypoint is behind" test (0x00009730) | b | the waypoint was found every time and the test answered "in front" (the commando faces the waypoint it charges at) |
| | the byte at guest this+0xC66 (0x00009754) | b | the 0x80 bit is set on every activate |
| | the update (0x00009760) and the AI waypoint lookup (0x00009778) | b | 162 updates per run |
| | the AI waypoint's flag test (0x00009788, 0x00009790), the reach (0x000097A0..0x0000982C) and the float at guest this+0xBEC (0x00009860) | c | unreachable in the shipped data: the 0x02/0x04 test wants the jump flags, and the only area with an attack-pattern commando (the Swamp's timber) has no AI waypoint with either bit (its waypoints carry 0x20 or 0). 26 AI waypoints in the game carry the 0x02 bit, but none of them is in an area with a commando that has an "ATTK"/"FOLW" connection |
| | the body's jump state (0x0000986C), the chain's end (0x0000989C), the target's aim position (0x000098D8) and the body's target vector (0x0000990C) | c | unreachable in the shipped data: the jump state comes from the waypoint navigation's jump command, which needs the same 0x02 flag |
| | the jump-back target (0x00009918) and the deactivate (0x00009934) | b | the update's copy ran every update, the deactivate 4 times |
| `fn_9_7A98` (the scripted charge) | the activate (0x00007B24), the waypoint picker (0x00007B7C), the material filter (0x00007B94) and the push vector (0x00007C14) | b | 4 activates per run; the picker answered true every time |
| | the loop-attack command (0x00007CC4), the body in the loop attack (0x00007D18), the waypoint reached (0x00007DDC) and the next waypoint (0x00007E00) | b | 31700 updates per run, 3 of them "the waypoint is reached" |
| | the loop-attack command (0x00007CC4), the body in the loop attack (0x00007D18), the waypoint reached (0x00007DDC), the next waypoint (0x00007E00) and the turn (0x00007E54) | b | 402 updates per run: 17 "the waypoint is reached" and the turn the other times |
| | the abort (0x00007E3C, the 0x04 bit of guest this+0xC64) and the 0x08 bit (0x00007E40) | d | the module's states that set those bits are not reached in this run |
| | the deactivate (0x00007E5C) | b | 4 times |
| `fn_9_2474` (the turn helper) | the speed test (0x000024B4), the normalized direction (0x00002560), the time (0x000025A4) and the movement (0x00002918) | d | the scripted charge calls it only while the byte at guest this+0xC64 has its 0x08 bit (0x00007E44..0x00007E48) - the module's user-event handler sets that bit when the shield's charge animation takes off (fn_9_C798, 0x0000C868) and the shield is up (the 0x04 bit of guest this+0xC65), and no shipped run has the shield up during the charge, so the helper does not run |
| | the loop attack type 4 (0x00002840), 5 (0x000027B0) and 6 (0x00002720) | b | all three ran: the type 6 arm once (the second commando's chain runs the other way), the type 4 arm once, the type 5 arm the rest |
| | the actor's forward column instead of the delta (0x0000256C) | c | unreachable in the flow: the scripted charge's own "the waypoint is reached" test (0x00007DDC) catches a delta of zero first, and the turn helper is only called when it did not |
| | the push vector (0x0000291C..0x000029A4) | d | the push vector is the shield's collision normal (fn_9_BF44); the commando's shield never hits a wall in this run |
| `fn_9_2A98` (the waypoint picker) | the speed test (0x00002AD0), the waypoint lookup (0x00002AD8) and the 0x20 flag test (0x00002B00) | b | 19 calls per run; the speed is 55 and the waypoint always has the 0x20 flag |
| | the "the next waypoint" branch (0x00002B08..0x00002BA8) and the "keep the waypoint" branch (0x00002C0C) | b | 15 "next" calls and 4 "keep" calls per run |
| | the waypoint gone (0x00002AF8) | b | twice per run (the charge's activate, before the navigation has advanced the destination) |
| | the speed at or below zero (0x00002AD0) | c | the area's block gives the charge speed 55 |
| | the 0x20 flag clear (0x00002B00) and the next waypoint gone (0x00002B80) | d | the module's waypoint always has the 0x20 flag in this run, and the chain's waypoints all have a successor |
| `fn_9_8B70` (the JumpBack state) | the activate (0x00008BA8) and the target (0x00008BBC..0x00008BF4) | b | one activate per run (the state is entered on its own); the target was there |
| | the update (0x00008BFC) and the step command (0x00008C08..0x00008C70) | b | 993 updates per run; the step command is issued (the id 1 = kBSC_Step, the direction 1 = kSD_Backward and the type 0 = kStep_Normal) |
| | the deactivate (0x00008D0C) | b | once per run |
| | the target gone (0x00008BC0..0x00008BC4) and the body in the state 3 (0x00008CD0..0x00008D04) | d | the target is always there, and the jump command is issued on every update, so the "the body is in the state 3" arm is not reached (the guest's structure is the same) |
| `fn_9_786C` (the RestoreOrientation state) | the activate (0x000078B0) and the deactivate (0x00007A00..0x00007A78) | b | four of each per run (the state is forced with the console's `fsm`, **forced (fsm)** - the commando only reaches it after a warp) |
| | the update's "the commando stands upright" arm (0x000078F0..0x000078F4) | b | four times per run: the commando is upright, so the animation state goes "over" at once |
| | the update's angle arm (0x00007918..0x00007924) and its turn (0x00007938..0x000079E0) | d | both need a commando that is tipped over, and nothing in the shipped flow tips it (the WarpOut and PostWarpOut states, which do, are still stand-ins) |
| `fn_9_8DF0` (the WarpOut state) | the activate (0x00008E30) | b | forced (fsm): the animation state becomes "ready", the module's state word at guest this+0x920 becomes 1, the 0x40 bit of the byte at guest this+0xC65 is cleared and the cover point is released |
| | the update's generate command (0x00008E68..0x00008EF0) | b | forced (fsm): the animation state lets the generate command through and the body controller takes it |
| | the update's "the body is not in the generate state" exit (0x00008F4C..0x00008F54) | b | forced (fsm): twice in the run |
| | the update's fade start (0x00008F5C..0x00008F78) | b | forced (fsm): the body's state is the generate state (12) and the 0x40 bit of the byte at guest this+0xC65 is clear, so the flag is set and the remaining animation time is kept at guest this+0xC58 |
| | the update's fade (0x00008F8C..0x00008FCC) | b | forced (fsm): 118 times in the run, the ratio of the remaining time to the kept one |
| | the update's epsilon guard (0x00008F8C..0x00008F90) | d | the kept time (guest this+0xC58) is written by the WarpOut's own fade start (0x00008F78) and nowhere else in the state - the WarpIn's write (0x0000949C) and the constructor's (0x0000E0D0) are the module's other two - and the WarpOut always writes it before the guard is reached, because the 0x40 bit of the byte at guest this+0xC65 is only set by that same fade start. The value is the body's remaining animation time while the body is in the generate state, so it is the length of the generate animation the state itself started |
| | the deactivate (0x00008FD4) | b | forced (fsm): the state machine's own transition out of the state |
| `fn_9_8D28` (the PostWarpOut state) | the activate (0x00008D48..0x00008D50) | b | forced (fsm): the state machine's own transition from the WarpOut state enters it |
| | the deactivate message (0x00008D54..0x00008DA8) | b | forced (fsm): the byte at guest this+0xC63 has no 0x20 bit. Its only writer is the kSM_Activate case (0x0000CD7C), which is behind the block's flag at guest this+0x91D, and no shipped CommandoPirate has that flag, so the bit is clear on the shipped data and the message always goes out |
| | the delete request (0x00008DB4..0x00008DD4) | d | the only writer of the 0x10 bit of the byte at guest this+0xC63 is the module's warp-out helper fn_9_3D64 (0x00003DF4), and the helper returns early when the block's flag at guest this+0x91D is clear - the "sound" property's bit 6 (the loader's mFlags1). None of the nine shipped CommandoPirate placements has it (the scan of every CMDO in the disc), so the bit is never set and the commando is never deleted here |
| `fn_9_7358` (the WallHang state) | the activate (0x0000739C) | b | forced (fsm): the 0x02 bit of the byte at guest this+0xC65 is set, the 0x40 bit of the byte at guest this+0xC66 is cleared, the module's word at guest this+0x924 becomes 0 and the animation state becomes "ready" |
| | the activate's "there is no cover point" exit (0x000073D4..0x000073D8) | b | forced (fsm): the commando holds no cover point - the CoverFind state finds none in the Swamp's timber area |
| | the activate's cover point path (0x000073DC..0x00007470) | d | the commando has to hold a cover point. The scan of the disc puts the nine CommandoPirate placements in three areas - the Temple's 05_temple_federationsite_dark, the Swamp's 05_swamp_timber! and the Cliff's 01_cliff_entrance - and only the last two have cover points (eleven and four). The CoverFind state (fn_9_709C) finds none of them, and the Cliff's run says why: the block's search radius is 50 units (the block's +0x54) and the three cover points nearest the commando are 62, 97 and 108 units away, so the radius rules them out; the two that were inside it fail the path find's reach (the guest's PathExists at 0x0000720C). The radius is the reason - a wider one would reach the cover points |
| | the update's angle test, the "the commando faces the cover point" arm (0x000074BC..0x00007528) | b | forced (fsm): twice in the run |
| | the update's locomotion command (0x000074D0..0x00007520) | d | it wants the angle over 10 degrees, which takes the cover point's forward - the commando holds no cover point, so the vector at guest this+0xBA0 is the zero vector and the angle stays under it. It needs the cover point path above |
| | the update's wall-hang command (0x00007534..0x00007588) | b | forced (fsm): twice in the run |
| | the update's "the body is in the wall-hang state" arm (0x000075B8..0x00007628) | b | forced (fsm): 196 times in the run - the wall-hang command puts the body in the wall-hang state |
| | the deactivate (0x00007630) | b | forced (fsm) |
| `fn_9_728C` (the WallDetach state) | the activate (0x000072C4) and the deactivate (0x00007330) | b | forced (fsm) |
| | the update's "the body is not in the wall-hang state" arm (0x00007318..0x00007328) | b | forced (fsm): once in the run, before the body was in the wall hang |
| | the update's "the body is in the wall-hang state" arm (0x00007318) | b | forced (fsm): 96 times in the run - the WallHang state's own command put the body in the wall-hang state |
| `fn_9_BA78` (the HasTarget trigger) | the true arm (the target is in the world) and the false arm | b | the gcov coverage build (build-port-cov) with tools/cov/run.sh: the body has no branch, and the gdb spec's ret counts show true 269 times in area 59 and false 295 times in the Cliff's entrance |
| `fn_9_B7FC` (the AttackPatternOver trigger) | the true arm (the waypoint navigation has no destination) and the false arm | b | the same runs: the body has no branch, and the ret counts show true twice in the Cliff's entrance and false 121 times in area 59 |
| `fn_9_BDCC` (the StateOver trigger) | the true arm (the animation state is "over") and the false arm | b | area 59: the trigger answers true 7 times and false 196 times in the run (tools/cov/run.sh with tools/cov/specs/commandopirate_triggers.py) |
| `fn_9_B818` (the IsFacingTarget trigger) | the early return (the target has no aim position, 0x0000B860) | b | forced (gdb): the trigger's target id (the id at guest this+0xBCA, the port's xbca_) is written to an invalid id at the trigger's entry, so the module's helper x3CBC_ fails (tools/cov/force.sh with tools/cov/specs/commandopirate_isfacing.py) |
| | the angle arm (the compare at 0x0000B8D4) - true and false | b | area 59: the trigger answers true 435 times and false 5 times in the run |
| `fn_9_3338` (the jump-back helper) | the ray test (0x00003430..0x00003434) and the path check (0x00003440) | b | the coverage run (build-port-cov) in area 59 with the warpout driver: the gcov report gives the helper 7/7 lines and 2/2 branches - the ray came back clear and blocked and the path finder answered both ways |
| `fn_9_3090` (the drowning) | the alive test (0x000030B8), the fluid id (0x000030D8), the water's cast (0x00003104), the trigger flag (0x00003110) and the head under the water (0x0000315C) | d | the commando has to drown: it needs a fluid, a water script object whose trigger byte has the 0x80 bit and the head below the water's bounds - the run's commando never enters a fluid |
| `fn_9_3900` (the jump-back velocity) | the flag byte at guest this+0xC63 (0x0000393C) and the ballistic velocity (0x000039A8..0x00003A18) | b | the console's `send <id> 0x584c4155` (the module's own launching message, the kSM_Launching case of fn_9_CB40, the call at 0x0000D040) reaches the helper: the coverage build in area 59 with the driver's four sends in a row gives the helper 14/15 lines and both the early out and the ballistic velocity |
| `fn_9_3A5C` (the hit-object helper) | the shield's path (0x00003A78..0x00003A90) and the blade's path (0x00003B90..0x00003BA8) | d | the module's shield has to be up (the 0x04 bit of the byte at guest this+0xC65) or its blade out (the 0x20 bit) and the sender has to be the module's own collision actor (guest this+0xBC6 or this+0xBC8); the area 59 run's commando never has the shield up, and the blade's own damage needs the melee state's blade |
| | the touched object's player test (0x00003AD0..0x00003ADC) and the damage timer (0x00003AE0..0x00003AF4) | d | they need the shield's or the blade's collision actor to have touched the player |
| `fn_9_3E1C` (the rag-doll driver) | every arm | b | see "The rag doll": ALL-ARMS over the natural and forced runs (35/35 lines, 30/30 branches) |
| `fn_9_19D8` (the team helper) | the team id (0x00001A04), the block's two flag bits (0x00001A10 and 0x00001A1C), the invalid team id (0x00001A48) and the team lookup (0x00001A60..0x00001A6C) | d | the coverage run in area 59 with the c300 driver gives the body 6/8 lines and 4/10 branches: the commando has no team id and the block's flags allow it, so only the "join" path runs |
| `fn_9_1A9C` (the avoid-actors helper) | the list walk (0x00001AD0..0x00001AE8), the area test (0x00001AF0..0x00001AFC) and the magnitude-safe test (0x00001B5C) | d | the same run gives it 14/16 lines and 8/12 branches: the other actors are none, this one or of another area, so the separation is never taken |
| `fn_9_1EF8` (the shield's turn-off helper) | the flag byte (0x00001F0C..0x00001F14), the emitter handle (0x00001F18..0x00001F20) and the two floats (0x00001F64..0x00001F70) | d | the same run gives it 0/9 lines: the shield's turn-off event never runs in area 59 |
| `fn_9_1F88` (the shield's turn-on helper) | the flag byte (0x00001F9C..0x00001FA4), the 0x20 bit (0x00001FA8..0x00001FB4), the emitter handle (0x00001FB8..0x00001FC0) and the state word (0x00001FF4..0x00002018) | d | the same run gives it 0/9 lines: the shield's turn-on event never runs in area 59 |
| `fn_9_2A98` (the waypoint picker) | the speed (0x00002AD0..0x00002AD4), the waypoint (0x00002AD8..0x00002AF8), the 0x20 flag (0x00002B00..0x00002B04) and the connection search (0x00002B08..0x00002B84) | d | the same run gives it 0/25 lines: the module's waypoint picker only runs in the scripted charge, which area 59 does not reach |
| `fn_9_45A4` (the collision actors) | the two spheres (0x00004604..0x00004620 and 0x00004728..0x00004744), the ids (0x000046CC and 0x000047F8) and the filter (0x00004830..0x0000487C) | b | the module's kSM_Create case builds them: the run's commando has both collision actors, and the material filter's exclude list gets the bit 0x8000 (the material 15, kMT_Glass) |
| | the collision actor's null tests (0x000046E4 and 0x00004810) | d | the coverage run's commando has both collision actors, so the null arms did not run - the body gets no marker |
| `fn_9_C300` (the KnockBack override) | the rag-doll branch (0x0000C324..0x0000C3AC) and the rag doll's early out (0x0000C3B0..0x0000C3B8) | b | see "The rag doll": the torso push forced (the flags 3 forcing), the early out natural (a corpse shot) |
| | the knock-back itself (0x0000C3C0..0x0000C3EC) | b | the area 59 boot: the console's `damage` reaches the module's override through the module's own helper fn_9_2D0C (the call at 0x00002EF0) |
| | the alive branch (0x0000C3F0..0x0000C404) | b | the coverage run (build-port-cov, tools/cov/run.sh with tools/cov/specs/commandopirate_c300.py and tools/drivers/commandopirate_c300.py, area 59): 190 of the 200 calls land on the live commando |
| | the alive branch's "the reaction is kAR_Hurled" arm (0x0000C3FC, `lwz r0,0x630` + `cmpwi r0,3`) | b | the guest's +0x630 is the knock-back manager (+0x620) +0x10, `SReactionParameters::mReaction` (the CRuleSetEvaluator base is 0x10 bytes; the reaction flags are +0x20, the DOL's ExecuteAction `ori`s them at 0x801BD9BC) - the port read the flags before 2026-10-21. The rule set gives kAR_Hurled to the console's `damage <id> 1 60 supermissile` (forced (damage)): once in the run, the state machine goes to "GetUp" and the hurled sound plays |
| | the dead branch (0x0000C47C..0x0000C484) | b | forced (kill + damage): the driver's `kill <id>` and its eleven `damage` lines in the same tick land on the dead commando (10 calls); 8 of them have the reaction kAR_Hurled |
| | the follow-up check and the death sound (0x0000C48C..0x0000C4D4) | b | the same run: the death sound twice, a death follow-up (7..10) six times. The host compiles the four compares into one range test, so gcov's 2/2 there does not say which of 7, 8, 9 and 10 ran |
| `fn_9_ADA0` (the Ambush state) | the activate (0x0000ADA8) and the other messages (0x0000ADB4) | b | the state is forced with the console's `fsm` (**forced (fsm)**): the activate sets the crouch locomotion, the update does nothing |
| `fn_9_6BCC` (the BreakCover state) | the activate (0x00006BEC) and the other messages (0x00006BF0) | b | forced (fsm): the exit-state command, the cover point's release and the byte at guest this+0xC66 |
| `fn_9_728C` (the WallDetach state) | the activate (0x000072C4), the update (0x000072DC) and the deactivate (0x00007330) | b | forced (fsm): all three messages ran |
| | the update's "the body is in the wall hang" arm (0x00007318..0x00007320) | d | the commando is never in the wall hang state (the WallHang state is still a stand-in) |
| `fn_9_A7A8` (the SetCoverDest state) | the activate (0x0000A7B0) and the other messages (0x0000A7C0) | b | forced (fsm) |
| | the cover point found (0x0000A7DC..0x0000A7E4) and the byte at guest this+0xC66 with its 0x40 bit (0x0000A800..0x0000A808) | d | the commando holds no cover point, and the byte's 0x40 bit is never set |
| `fn_9_A8E8` (the SetJumpDest state) | the activate (0x0000A8F0) and the other messages (0x0000A900) | b | forced (fsm) |
| | the jump point found (0x0000A930..0x0000A938) | d | the module's jump point id (guest this+0xBD0) is kInvalidUniqueId in this area |
| `fn_9_A97C` (the SetRetreatDest state) | the activate (0x0000A984) and the other messages (0x0000A994) | b | forced (fsm) |
| | the retreat point found (0x0000A9C4..0x0000A9CC) | d | the module's retreat point id (guest this+0xBCE) is kInvalidUniqueId in this area |
| `fn_9_69C8` (the Dead state) | the base's dead handler (0x000069EC), the activate (0x000069F0) and the other messages (0x000069F8) | b | forced (fsm): the driver's `fsm Dead` enters the state |
| | the byte at guest this+0xC66 with its 0x20 bit (0x00006A0C..0x00006A14) | b | **forced (the spec writes the bit)**: the bit is set by the knock-back helper (fn_9_2D0C) when the shield is up.  The forcing spec (tools/cov/specs/commandopirate_states2.py) writes it from the state's second entry on, so the first activate runs the arm with the bit clear and the second one with it set - the merged coverage of the two runs gives the body 6/6 lines and 4/4 branches (ALL-ARMS), so it is marked |
| `fn_9_6A40` (the GetUp state) | the activate (0x00006A78), the update's "the commando lies on the ground" arm (0x00006AA0..0x00006AC4) and the getup command (0x00006ACC..0x00006B18) | b | the coverage run in area 59 with the core driver (tools/cov/run.sh with tools/cov/specs/commandopirate.py and tools/drivers/commandopirate_core.py, 8000 frames, game exit code 0, no caught/reach-stub): 16/17 lines and 8/10 branches - the arm that sets the pending death (the commando on the ground and the path find with no source point) did not run, so the body gets no marker |
| `fn_9_6DBC` (the Cover state) | the activate (0x00006E08..0x00006E2C), the cover point (0x00006E34..0x00006E48) and the cover command (0x00006E5C..0x00006EE8) | b | the same run: 14/23 lines and 5/14 branches - the cover arms need a cover point and the commando holds none in area 59, so the body gets no marker |
| `fn_9_B168` (ShouldCrouch) and `fn_9_B114` (ShouldWallHang) | the cover point missing (0x0000B184..0x0000B194 and 0x0000B134..0x0000B138) | b | the Cover state's triggers run when the console forces that state (forced (fsm)) |
| | the cover point's own answer (0x0000B18C and 0x0000B140) | d | the commando holds no cover point in this area |
| `fn_9_B1E0` (ShouldBoost) | the byte at guest this+0xC66 (0x0000B1F0), the byte at guest this+0xC64 (0x0000B1FC) and the AI waypoint (0x0000B218) | b | 162 calls per run: the state sets the 0x80 bit of guest this+0xC66 and the trigger is evaluated every tick of the state |
| | the 0x20 flag test (0x0000B22C) | b | 158 calls with the waypoint's flags word 0 and 4 with the word 0x20 |
| | the true arm (0x0000B234) | b | 4 times: the scripted charge starts |
| | the 0x80 bit of guest this+0xC66 clear, the 0x01 bit of guest this+0xC64 set and the AI waypoint gone | c | unreachable: the FollowAttackPattern state sets the 0x80 bit and the waypoint on its activate (0x000096CC and 0x00009754) and this trigger is only evaluated while that state is current; nothing in the module ever sets the 0x01 bit of guest this+0xC64 (the constructor clears it at 0x0000E1AC and every store to that byte sets another bit) |

**The states the area's asset cannot reach.** Area 59's own state machine gives the
`ShouldArmShield` trigger no target state (`PathFind : ShouldArmShield -> (null)`), so the
ArmShield state (fn_9_86B4) is only reachable when the console forces it: the driver sends
`fsm <uid> ArmShield` and the state runs its activate, its updates and its deactivate
(**forced (fsm)**). The same asset enters the scripted charge (fn_9_7A98) through the
`ShouldBoost` trigger of its FollowAttackPattern state (`FollowAttackPattern : ShouldBoost ->
ScriptedShieldCharge`), so the scripted charge is reachable once that trigger is ported.

**What area 59's own asset has (2026-10-21).** The FollowAttackPattern state (fn_9_961C) is host
code since 2026-10-21, and the area 59 run shows two things that keep the scripted charge out of
reach there, both in the shipped data and not in the port's code:

* the commando of area 59 has exactly one script connection: `on Modify send Follow -> 003b0112
  CScriptAIWaypoint` (the console's `obj` prints it). The guest's state looks for the connection
  in the **"ATTK"** state (0x4154544b, `kSS_Attack`) with the "FOLW" message (0x464f4c57,
  `kSM_Follow`) - the fourCCs it builds at 0x0000969C..0x000096B4, the same pair its
  `HasAttackPattern` trigger (fn_9_B900) uses at 0x0000B908..0x0000B924. The area's "MDDY"
  ("Modify", 0x4d444659) connection is a different state, so the guest's lookup finds nothing and
  the waypoint at guest this+0x6C0 stays kInvalidUniqueId. The state therefore leaves at once
  (the `AttackPatternOver` trigger, fn_9_B7FC, answers true) and the asset's own
  `HasAttackPattern` trigger is false, so the state is never entered without the console's `fsm`.
* the area's only AI waypoint (003b0112, the one the commando is connected to) has the word at
  its +0x164 (the host's `CScriptAIWaypoint::GetFlags`, the retail's "flags" property) equal to
  **zero**. The `ShouldBoost` trigger's own test (fn_9_B1E0, the rlwinm at 0x0000B22C) wants that
  word's 0x20 bit, and the FollowAttackPattern state's own flag test (0x00009788, 0x00009790)
  wants its 0x02 or 0x04 bit, so neither can pass with this waypoint. The scripted charge
  (fn_9_7A98) is therefore unreachable in area 59 with the shipped data.

### The Swamp's timber area and the cover, jump and retreat points (2026-10-21)

The only area in the shipped data with a commando that has an attack pattern is
`04_Swamp_World/05_swamp_timber!` (the layer "1st Pass": two CMDO placements with `ATTK` and
`FOLW` connections; area 59's commando has only `MDDY` and `FOLW`, and the cliff area's six
commandos have no attack pattern at all). The driver `tools/drivers/commandopirate_pattern.py`
boots that area (`MP2_WORLD="swamp,05_swamp_timber!"`), forces the module's own states with the
console's `fsm` and now runs the two point searches (`CoverFind` and `JumpPointFind`) before the
destination states, so that the destination states have a point to find.

The run of 2026-10-21 (60000 frames, exit=0, no caught/reach-stub) shows that the shipped data
does not give the arms the port waits for:

- the cover and jump point searches find nothing: `SetCoverDest` (`fn_9_A7A8`), `SetJumpDest`
  (`fn_9_A8E8`) and `SetRetreatDest` (`fn_9_A97C`) run their activate, but the "the point is
  there" arms never fire - the layer has no free cover point and no jump point, so the searches
  leave the ids at guest this+0xBD2 and +0xBD0 invalid.
- the WallDetach state's "the body is in the wall hang" arm (`fn_9_728C`) needs the body's state
  20 (`pas::kAS_WallHang`), which only the unported WallHang state (`fn_9_7358`) can give.
- the Dead state's shield effect (`fn_9_69C8`) needs the 0x20 bit of the byte at guest
  this+0xC66, and that bit is only set by the module's own knock-back helper (`fn_9_2D0C`), which
  runs only while the byte at guest this+0xC65 has its 0x04 or 0x08 bit (the message switch,
  0x0000D0A0). (`fn_9_C300` itself is ported and marked since 2026-10-21.)
- the PostWarpOut state (`fn_9_8D28`, ported on 2026-10-21) never runs either: the module never
  warps out, because the WarpOut state (`fn_9_8DF0`) is still a stand-in and no script message
  of the shipped data reaches it.

So the only new markers the Swamp run gives are the two the previous slice already had
(`fn_9_ADA0` and `fn_9_6BCC`, both forced with the console's `fsm`); the other eight bodies of
that slice keep one arm each.

### The attack pattern in the shipped data (2026-10-21)

`tools/scan_commando_pattern.py` (new in this slice) reads every world's MREA script layers
straight from the disc (`tools/echoes_disc.py`'s readers) and lists the CommandoPirate placements
(the "CMDO" script object), their script connections and the AI waypoints ("AIWP") of the layer
with their flag word (the property 0xc6705a00, the word at the waypoint's +0x164). The whole game
has **three** areas with commandoes and **two** commandoes with an attack pattern:

| area | layer | commando | its "ATTK"/"FOLW" connection | the chain's waypoints |
|---|---|---|---|---|
| 04_Swamp_World / 05_swamp_timber! | the MREA's third script layer (the game's layer 2, "1st Pass") | 0018001e | 00180077 (flags 0) | 00180086, 00180084, 0018001f, 00180089, 00180083, 00180087, 00180088 (all 0x20) |
| | | 00180082 | 0018009b (flags 0) | 0018008e (0), 00180095, 0018008a, 00180090, 00180099, 0018008b, 0018008d, 0018009d, 00180007 (all 0x20) |
| 01_Temple_Hub / 05_temple_federationsite_dark (area 59) | 0 | 003b010e | none - the only connection is "Modify"/"Follow" | 003b0112 (flags 0) |
| 05_Cliff_World / 01_cliff_entrance | 3 | six commandoes | none | no AI waypoint in the layer |

So the Swamp's timber area is the one place where the module's attack pattern exists: the
commando's first waypoint has the flags word 0 and the chain's next ones have the 0x20 bit - the
bit the `ShouldBoost` trigger (fn_9_B1E0) wants. The waypoints' chain is the "ARRV"/"NEXT"
connection the guest's FollowAttackPattern state checks at 0x0000989C and that the waypoint
navigation walks.

The run there (`tools/run_gameplay.sh` with `MP2_WORLD="swamp,05_swamp_timber!"`, the card in
/tmp/opencode/u_cop2, `MP2_GODMODE=1`, `MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8` and the driver
`tools/drivers/commandopirate_pattern.py`): the area's actor is inactive in this harness (the
room's own layer script does not run), so the driver wakes it with the console's `send <id>
Activate`, and it forces the FollowAttackPattern state with the console's `fsm` - the area's own
route into that state goes through the RestoreOrientation state, which is still a port stand-in.
Then the module's own code runs: the navigation walks the chain, the `ShouldBoost` trigger answers
true and the scripted shield charge runs. The run ends with exit=0 and `grep -c '[port]
caught|reach-stub'` = 0; the stand-ins are the Listen, the CVisualKnowledge's Update and
SetVisualTarget and the RestoreOrientation state (fn_9_786C).

**The picker's connection search had the wrong entity.** The guest's waypoint picker (fn_9_2A98)
searches the **waypoint's** own connections for the "ARRV"/"NEXT" one (its r4 at the
FindConnectedObject_if call, 0x00002AF4..0x00002B44, is the waypoint the flags were read from);
the port searched the *commando's* connections, so the "take the next waypoint" branch always
answered "no waypoint" and the scripted charge never advanced along the chain. It is
`waypoint->FindConnectedObject_if(...)` now (0x00002B44).

### The scripted charge's end condition (2026-10-21)

The scripted charge (fn_9_7A98) has **no distance or time test of its own**: its update
(0x00007C40..0x00007E58) only asks whether the loop-attack command may be issued, whether the body
is in the loop attack, whether the byte at guest this+0xC64 has its 0x04 bit (the abort, the
exit-state command at 0x00007E3C), whether the commando has reached the waypoint (the delta's
projection on the transform's second column is zero or less, or the squared distance is at most the
bounding box's height squared - 0x00007D70..0x00007DF0) and whether the byte at guest this+0xC64
has its **0x08 bit** (0x00007E44..0x00007E48) - the turn helper (fn_9_2474) and with it the
charge's own movement run only while that bit is set. The bit is the module's "the charge is
airborne" flag: the user-event handler (fn_9_C798) sets it when the shield's charge animation takes
off (0x0000C868) and clears it when it lands (0x0000C88C), both only while the shield is up (the
0x04 bit of guest this+0xC65).

**The port had that test inverted** (`== 0` instead of `!= 0`), so the commando charged at every
update of the charge state - whether or not it was in the air - and the charge's impulse
(`MoveInOneFrameOR`, 0x00002918) added up until the commando left the map (the run ended with it
tens of thousands of units away). With the guest's polarity the commando stays where the retail's
condition says it should: in the Swamp run (2026-10-21) it ends 2 units from where it started
instead of 600 units away.

The charge's loop-attack command also carries the guest's flag byte's 0x80 bit (the skip-into,
0x00007C90), which the port did not set.

**The JumpBack state is a *step*, not a jump.** The command the guest delivers (0x00008C70) has the
id 1 - `kBSC_Step` - with the direction 1 (`kSD_Backward`, 0x00008C2C) and the type 0
(`kStep_Normal`, 0x00008C30); the animation state it asks for is the step's too (3 = `kAS_Step`,
0x00008C08) and the "the body is in the state 3" arm (0x00008CD0) is the step's. The guest's
command data is built on the stack (0x00008C18..0x00008C6C) and copied into the command manager's
slot (0x00008C74..0x00008CBC) - the target (0,0,0) and the has-target flag 0 - and the step's own
target goes into the body's target vector instead (0x00008CF8..0x00008D04), which the host's step
state faces (CBSStep.cpp:36).

**The DOL's `CBSJump` against the host's** (`build/G2ME01/asm/MetroidPrime/BodyState/CBSJump.s`
against `src/MetroidPrime/BodyState/CBSJump.cpp`; the DOL's file is the authority, and the
inlined accessors are why a `grep` for the callee name finds nothing):

* the **launch** is the DOL's too: `CBSJump::UpdateBody` **0x800FC5D0..0x800FC604** loads the
  actor's mass (actor+0x158) times this+0x20..0x28 and stores actor+0x16C..0x000FC174 -
  `SetConstantForceWR` and `GetMass` are inline, so there is no `bl` to find. The host's
  `CBSJump.cpp:150` (and :179) does the same thing, so **the host matches the DOL here**.
* the **command's flags**: the DOL reads the **facing-flags byte at cmd+0x30** (0x800FCF30) and puts
  its 0x80 bit into this+0x38's 0x40 bit (0x800FCF38..0x000FCF3C), and that bit is the
  `if (mWallJump)` test at **0x800FCF78**; the DOL reads the **word at cmd+0x2C** (0x800FCF60) into
  this+0x10. The host's `CBCJumpCmd` (include/MetroidPrime/BodyState/CBodyStateCmdMgr.hpp:306-307)
  has the same layout, so **there is no flag swap** and the host matches the DOL here too.
* the **destination** comes from the command's own waypoint1 in both (the DOL's `CBSJump::Start`
  0x800FCEF8..0x000FCF04 copies cmd+0x10..0x18 into this+0x14..0x1C; the host's
  `CBSJump.cpp:41`), and the host's use of the body's target vector (CBSJump.cpp:136, :155, :183) is
  for the facing only, as in the DOL.

So the commando that left the map was **not** a host bug: the port had delivered a *jump* command
where the guest delivers a *step* (see above), and the step's launch is what the DOL's jump state
does. The port's JumpBack now delivers the step ✓ and the commando stays where it is.

**The loop attack types and the skip-into flag.** The turn helper's command is the host's
`pas::kLAT_Five` / `pas::kLAT_Six` (the guest's 5 and 6, 0x000027D8 and 0x00002748) and
`pas::kLAT_Four` (4, 0x00002868) now, and the command's skip-into flag is set (the guest's flag
byte's 0x80 bit, 0x00002740, 0x000027D0 and 0x00002860) with the host's `SetSkipInto`.

### Function classification (all 256 symbols, 2026-10-21)

`CommandoPirate_functions.tsv` is the machine-readable copy: 256 rows, one per function symbol of
`config/G2ME01/rels/CommandoPirate/symbols.txt`, with the class column and the port's state.

Three classes, applied by rule so the table can be audited:

- **H - host copy** (80 functions, 0x3228 bytes): the body is a copy of something the host already
  has, so the port writes no body for it: the compiler-emitted destructor/copy chains (the bodies
  whose only calls are `__dt__`/`__ct__`/`Free`/`__copy`), the REL runtime (`RELMain`,
  `RELExit`, `_prolog`, `_epilog`, `ModuleConstructors`, `ModuleDestructors`,
  `__destroy_global_chain`, `__register_global_object`, `fn_9_F9F8`) and the loader's
  typedef/property readers (the bodies whose only calls are `LoadTypedef*`/`LdrTo*`/`ReadFloat`).
- **T - data / vtable thunk** (40 functions, 0x4DC bytes): the body only returns a constant, a
  member address or a flag, or fills static command data - the port writes it as one line in the
  vtable or as a plain accessor.
- **B - module behaviour** (136 functions, 0xCF94 bytes): real behaviour - these are the port bodies.

| class | functions | .text bytes |
|---|---|---|
| H host copy | 80 | 0x3228 (12840) |
| T data / vtable thunk | 40 | 0x4DC (1244) |
| B module behaviour | 136 | 0xCF94 (53140) |
| **total** | **256** | **0x10698 (67224)** |

Count check: 256 rows = the 256 `.text` function symbols of `symbols.txt` (the REL's 246 module
functions with a body in the asm, the 8 REL runtime symbols and the 2 entry points).

The port's state per function (the `port` column): 54 marked, 61 ported but not marked, 141
not ported at all (the H and T ones, and the module's own loader/ctor/dtor helpers, which the
host's classes already have).

**The unmarked B list** (the B functions the port does not mark, 98 of them): the stand-in first -
`fn_9_5410` (the E-grenade's creator) - and then the ported ones whose every arm has not run yet
(the table above names them). The
full list is the `class`/`port` columns of `CommandoPirate_functions.tsv`.

**The markers.** `fn_9_8028` (the charge state), `fn_9_86B4` (the ArmShield state), `fn_9_ADA0`
(the Ambush state), `fn_9_6BCC` (the BreakCover state), `fn_9_8DF0` (the WarpOut state),
`fn_9_8D28` (the PostWarpOut state) and `fn_9_728C` (the WallDetach state) are marked, and so are
`fn_9_BDCC` (the StateOver trigger), `fn_9_B818` (the IsFacingTarget trigger),
`fn_9_BA78` (the HasTarget trigger) and `fn_9_B7FC` (the AttackPatternOver trigger):
every message arm of both ran - the charge state in the normal flow, the ArmShield state through
the console's `fsm` (**forced (fsm)**). `fn_9_B1E0` (ShouldBoost) is marked too: its two arms that
ran (the 0x20 flag test and the true arm) ran in the Swamp's timber area, and its three other arms
are unreachable in the shipped data (the class-**c** rows above). `fn_9_3338` (the jump-back
helper) is marked too: the coverage run in area 59 gives it 7/7 lines and 2/2 branches - the ray
and the path finder answered both ways. `fn_9_C300` (the KnockBack override) is marked: 15/15
lines and 11/11 branches with tools/drivers/commandopirate_c300.py (forced (damage), forced (kill +
`fn_9_3900` (the jump-back velocity) is marked as well:
the console's `send <id> 0x584c4155` delivers the module's own launching message, so the helper
damage); the rag-doll branch is dead). The four functions of the first
slice (`fn_9_7A98`, `fn_9_2474`, `fn_9_2A98` and `fn_9_B1E0`) and the FollowAttackPattern state
(`fn_9_961C`) are not marked: each of them still has a class-**d** arm (the charge's abort and
0x08 bits, the turn helper's push vector, the picker's 0x20-clear and next-gone arms and the
state's "the waypoint is behind" arm).

### The shield's own bits (2026-10-21)

The byte at guest this+0xC65 and the byte at guest this+0xC64 decide what the shield does, and
the writers are what the arms wait for:

| bit | set by | cleared by |
|---|---|---|
| this+0xC65 0x04 (the shield is up) | the shield-charge states fn_9_7A98 (0x00007B34) and fn_9_8028 (0x00007FC4, 0x00008098, 0x0000867C) | the constructor |
| this+0xC65 0x08 (the shield is arming) | the ArmShield state fn_9_86B4 (0x00008714) | the ArmShield state's deactivate (0x00008908) |
| this+0xC65 0x10 | the module's timer helper fn_9_662C (0x00006A84) | the same helper (0x00006B58) |
| this+0xC65 0x20 (the blade is out) | the MeleeAttack state fn_9_A4BC (0x0000A598) | the same state (0x0000A6F0) |
| this+0xC65 0x02 | the charge states fn_9_7A98 and fn_9_8028 | the charge states and the WallHang state fn_9_7358 |
| this+0xC64 0x80 (the shield may arm) | the burst-fire helper fn_9_6390 (0x00006434) when the block's arm chance (the block's +0x140) wins the roll | the same helper (0x00006964) and the ArmShield state's deactivate (0x000088D0) |
| this+0xC64 0x40 (the commando is aggressive) | the timer helper fn_9_662C (0x000066C8) when the block's aggressiveness (the block's +0x4C) wins the roll | the constructor |

So the ArmShield state (fn_9_86B4, now ported) is only entered when the burst-fire helper's arm
chance roll wins: it needs more shots than the counter at guest this+0xB58 (0x000063E4..0x000063FC).
In the area 59 run the commando never armed its shield, so the state did not run and the shield's
arms stayed out of reach. The shield's "up" bit (0x04) comes only from the charge states, which
are still stand-ins.

The module's remaining stand-ins, smallest first, with their retail sizes: the states below the
FollowAttackPattern state. The FollowAttackPattern state `fn_9_961C` (0x350), the scripted shield
charge `fn_9_7A98` (0x590), its turn helper `fn_9_2474` (0x580), the charge state `fn_9_8028`
(0x68c), the ArmShield state `fn_9_86B4` (0x280), the shield effect `fn_9_2048` (0x330), the
trigger `fn_9_B1E0` (ShouldBoost, 0x70), the waypoint picker `fn_9_2A98` (0x214), the JumpBack
state `fn_9_8B70` (0x1b8) and the RestoreOrientation state `fn_9_786C` (0x22c) are host code
since 2026-10-21.

### The FollowAttackPattern state (fn_9_961C, 2026-10-21)

The state is host code (the port's `FollowAttackPattern`):

* the guest's own waypoint navigation patrols first (the call at 0x00009658, the navigation at
  guest this+0x6B8);
* the activate (0x00009680) takes the module's waypoint: the id at guest this+0xBDA when it is
  not kInvalidUniqueId (0x00009684..0x00009694), else the object connected in the "ATTK"/"FOLW"
  state (0x0000969C..0x000096B8); the id goes to guest this+0x6C0 (0x000096CC). The waypoint (a
  CScriptWaypoint) is then looked up (0x000096DC) and, when it is behind the commando (the delta's
  projection on the actor's forward column, guest this+0x28/0x38/0x48, is zero or less -
  rodata_494 = 0, 0x00009730), the navigation's "in position" flag gets its 0x80 bit
  (0x0000973C..0x00009748). The byte at guest this+0xC66 always gets its 0x80 bit
  (0x0000974C..0x00009758) - the bit the `ShouldBoost` trigger reads (0x0000B1F0);
* the update (0x00009760) looks the AI waypoint up (0x00009774) and, when the word at its +0x164
  has its 0x02 bit (0x00009788) or its 0x04 bit (0x00009790), the commando's reach is the body's
  max speed (0x000097A0) times (0.1f + 1.5f * dt) (rodata_42C, rodata_47C) times the model's
  scale Y (the model data at guest this+0x60, 0x000097C0) plus the module's jump distance (the
  float at guest this+0xBE8, 0x000097CC); when the commando is nearer than that (0x00009824) the
  navigation's in-position flag is set (0x00009830) and the float at guest this+0xBEC takes 3
  (rodata_448) when the waypoint's flags have their 0x02 bit and 0 (rodata_494) when they do not
  (0x00009840..0x00009860);
* then, when the body is in the jump state (the body's state id 13, 0x0000986C) and the waypoint
  has no successor (its "ARRV"/"NEXT" connection, 0x0000989C, is kInvalidUniqueId - the chain the
  host's waypoint navigation walks), the target's aim position (the module's helper fn_9_3CBC,
  0x000098D8) becomes the body's target vector (the command manager's fifth CVector3f, the body
  controller's +0x34, 0x0000990C..0x00009914);
* the navigation's destination position (guest this+0x6D4..0x6DC) is copied to the module's
  jump-back target (guest this+0xBF0, 0x00009918..0x0000992C), and the deactivate (0x00009934)
  clears the 0x80 bit of the byte at guest this+0xC66 (0x0000993C..0x00009940).

**Two guest fields the port now names.** The byte at guest this+0x6F0 is the bitfield byte of the
waypoint navigation (the navigation sits at guest this+0x6B8 and its flags are at +0x38, the last
word of it): the 0x80 bit MWCC allocates first is the navigation's "in position" flag, so the port
uses the host's `CWaypointNavigation::SetInPosition` (the host's class keeps the same member
order: mMoveSpeed, mPauseRemainingTime, mDestination, mLastDestination, mPatrolState,
mSegmentStart, mDestinationPosition at +0x1C = guest this+0x6D4, mFaceVector, mWobbleSteering and
the flag byte at +0x38 = guest this+0x6F0). Guest this+0x6C0 is the same navigation's
**destination** field (+0x8), so the port uses the host's navigation destination for it
(`GetDestination`/`SetDestination`) and has no member of its own: the guest's field is one field
that the navigation advances on its own (its `UpdateDest` walks the "ARRV"/"NEXT" chain when the
in-position flag is set), so a second copy of it in the port would go stale. Every place the
guest reads or writes guest this+0x6C0 - the waypoint picker (fn_9_2A98, 0x00002AD8 and
0x00002B8C), the `ShouldBoost` trigger (fn_9_B1E0, 0x0000B204), the FollowAttackPattern state
(fn_9_961C, 0x000096CC and 0x00009760) and the scripted charge's deactivate (0x00007ED8) - goes
through the host's navigation. The `AttackPatternOver` trigger (fn_9_B7FC) already did.

**The turn helper's time (fn_9_2474).** The guest's `Magnitude` call at 0x000025A4 takes the raw
delta at its own stack slot (sp+0xD4, the `addi r3, r1, 0xd4` at 0x0000258C), not the copy of the
direction the helper made at sp+0xC8 (the normalized delta or the actor's own forward column).
The port had the direction's length there (always 1 after the normalize), so its time to the
waypoint was 1 / the charge speed; it is now the delta's length over the charge speed, the same
value the waypoint picker stores at guest this+0xB88 (fn_9_2A98, 0x00002BF8..0x00002C04). The
slerp factor (0x00002634..0x00002678) is then 1 - the time / the module's time to the waypoint:
0 while the commando is as far from the waypoint as it was when the waypoint was picked, and
rising to 1 as it closes in - the retail's "turn harder the closer it gets" behaviour, which the
port's old value (always ~1) did not have. The push vector (guest this+0xBB8) is scaled by the
charge speed and the frame time, exactly like the movement above it (the two fmuls chains at
0x0000297C..0x00002994, the `MoveInOneFrameOR` call at 0x000029A4) - the port already did that.

**The jump distance at guest this+0xBE8** is ported too: the constructor's store at 0x0000E4D4 is
the jump animation's travel distance (the parm data fn_9_5D74 builds at 0x0000E49C: the state id
13 = kAS_Jump with the parms (0, 0, 0)) times the model's scale Y (the load at 0x0000E4B0). It is
the term the FollowAttackPattern state's reach adds (0x000097CC).

**The run (2026-10-21).** The area 59 run with the ported state (`tools/run_gameplay.sh`,
`TELEPORT=05_temple_federationsite_dark`, `FRAMES=16000`, the card in /tmp/opencode/u_cop1 - the
note's card /tmp/opencode/u_ds9 was gone - `MP2_GODMODE=1`, `MP2_FRAME_LIMIT=0
MP2_TICKS_PER_FRAME=8`, the driver `tools/drivers/commandopirate_shield.py` and, after it, the
console's `fsm <uid> FollowAttackPattern`) ends with exit=0 and `grep -c '[port] caught|reach-stub'`
= 0. The run's stand-ins are three (the CVisualKnowledge's Update and SetVisualTarget and the
JumpBack state fn_9_8B70); the `fn_9_961C` stand-in log is gone. The FollowAttackPattern state
runs only when the console forces it (**forced (fsm)**): 5 activates, 137 updates and 5
deactivates per forced run, and the `ShouldBoost` trigger is evaluated 132 times - it stops at
its 0x20 flag test, so the scripted charge (fn_9_7A98) and the turn helper (fn_9_2474) still do
not run.

## 5. What is not ported yet (milestone 2 and forward)

- The state machine's other bodies: the two tables (34 triggers, 36 states) are registered and
  most of the functions they point at are ported, but thirteen are still stand-ins that log the
  retail function they stand in for and return the neutral value: the states `fn_9_5410` (the
  E-grenade's creator, 0x370), `fn_9_6A40` (GetUp, 0x130), `fn_9_6C68` (CoverAttack, 0x154),
  `fn_9_6DBC` (Cover, 0x284), `fn_9_7358` (WallHang, 0x310), `fn_9_7724` (Crouch, 0x148),
  `fn_9_8DF0` (WarpOut, 0x218), `fn_9_9064` (WarpIn, 0x528), `fn_9_9BB4` (JetBoost, 0x1fc),
  `fn_9_A110` (PostEGrenadeAttack, 0x1dc), `fn_9_A2EC` (EGrenadeAttack, 0x174) and
  `fn_9_A4BC` (MeleeAttack, 0x290), plus the trigger `fn_9_AFD4` (CoverBlown, 0xe0).
- The guest's `CVisualKnowledge` (this+0xAB0): ported 2026-10-22 (see "The visual knowledge").
- The `CTryCommand` the base keeps at +0x6B4 is the base's animation state, so the host's
  animation state answers it (`mAnimationState.CanIssueCommand`).
- The FollowAttackPattern state (fn_9_961C, 0x350) is host code since 2026-10-21: it sets the
  waypoint at guest this+0x6C0 and the 0x80 bit of the byte at guest this+0xC66, which the
  ShouldBoost trigger reads. The scripted shield charge (fn_9_7A98) is still not entered in
  area 59: the area's commando has no "ATTK"/"FOLW" connection and the area's only AI waypoint
  has the flags word at its +0x164 equal to zero (the section above).
- The melee (`MeleeAttack`, the blade's collision actor and its damage), the E-grenade (the
  `Commando E-Grenade` script object), the shield (arm, charge, the mould) and the burst fire.
- The three `CToken` pairs (the script objects at this+0x8F0, +0x90C and +0x7C8) and the two
  `CElementGen` effects.
- The animation distances at +0xBE0, +0xBE4 and +0xBE8 are ported (the ctor's two step
  distances, the scale's Y times the parms (1, 2) and the scale's X times the parms (3, 2), and
  the jump distance, the scale's Y times the kAS_Jump parms (0, 0, 0)); the locomotion-speed term
  at +0xB90 is not.
- The CVisualKnowledge at +0xAB0: ported 2026-10-22 (see "The visual knowledge").

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

## 6. The stateless helpers (2026-10-21, the helpers lane)

`CommandoPirate_functions.tsv` has one row per function of the REL (256 rows = the 256
`type:function` symbols of `config/G2ME01/rels/CommandoPirate/symbols.txt`). Beside the class
(H = host copy, T = data / vtable thunk, B = module behaviour) and the port's state, each row now
carries the **callers** (the `bl` sites in the REL and the `.data` tables that point at the
function - the vtable `lbl_9_data_828`, the FSM tables, the jump table) and the **owner**:
`cpcore` for the states and overrides the core lane ported (Listen, CoverBlown, MeleeAttack,
EGrenadeAttack, PostEGrenadeAttack, JetBoost, WarpIn, Crouch, Cover, CoverAttack, GetUp, the
vtable overrides and the E-grenade chain), `commando-lane` for the other rows of the FSM2 tables
(`lbl_9_data_198` and `lbl_9_data_568`, the 34 triggers and 34 states), `cphelpers` for the rest
(the stateless helpers, the loader and the constructor).

The stateless helpers live in the core lane's files (the states and the overrides it ported) and
in the ones this lane adds. The port's own file may only call them where a stand-in call site was
one line.

Which of the 77 unmarked B functions are the helpers lane's: the module's loader and constructor
cluster (`fn_9_168`, `fn_9_D904` and their helpers - the host's classes have them), the host
glue the REL compiles itself (the collision manager's arrays, the memory blocks), and the
stateless helpers. Of the latter, only the ones the port's own code reaches are portable now:
the rest are called from the FSM states and triggers (the CommandoPirate lane's) or need the
"Commando E-Grenade" script object, which the host has no class for.

### 6.1 `fn_9_BE4C` - what the actor hears (vtable slot 0xb0): **marked**

The port had it as a named stand-in that forwarded to the base's `Listen`; the module's own body
is the core lane's now (`CCommandoPirate::Listen` in
`src/MetroidPrime/Enemies/PortCCommandoPirateCore.cpp`, the same body this lane wrote in
`PortCCommandoPirateHelpers.cpp` before the two branches merged - the marker is on that body).

Retail's body (0x0000BE4C..0x0000BF3C):

```
0x0000BE4C  lbz r0, 0x420(r3)        ; the base's flag byte
            extrwi. r0, r0, 1, 25    ; its 0x40 bit (the host's mAlive)
            beq -> return 0          ; not alive: the actor hears nothing
            cmpwi r6, 4 / 0          ; the noise type: a path noise (4) or a shot (0)
0x0000BE7C  the squared distance against 1600.0f (rodata_4E4); inside it the 0x80 bit of the
            module's byte at guest this+0xc63 is set (0x0000BEFC..0x0000BEC0)
0x0000BEC8  the squared distance against the hearing radius (the data block's field at guest
            this+0x81c), then the base's height range at guest this+0x3d4 against 0.0f
            (rodata_494): the height range wins the tie, otherwise the noise's own height
            distance decides (0x0000BF20..0x0000BF38)
0x0000BF2C  the constant 0.0f is written to the module's float at guest this+0xb60
```

| branch | status | evidence |
|---|---|---|
| 0x0000BE4C (the actor is not alive) | (b) ran (forced) | forced under gdb: the alive flag (the base's byte at guest this+0x420, its 0x40 bit) is cleared at the body's entry (`tools/cov/specs/commandopirate_force.py`), so the early-out runs - in the game the flag's only writer is the module's area-load death case (the port's `kSM_AreaLoaded`, 0x0000CE40), which sets the HP to -1 as well (0x0000CE5C) and deletes the actor in the same update |
| 0x0000BE7C (a path noise, too far) | (b) ran | coverage: the far path noise (the driver puts the commando 80 m from the player) returned 0 |
| 0x0000BF0C (a path noise, heard) | (b) ran | coverage: 1 hit, the return value 1 |
| 0x0000BEF4 (a shot, too far) | (b) ran | coverage: the far shot returned 0 |
| 0x0000BF20 (the height range) | (b) ran (forced) | forced under gdb: the base's height range at guest this+0x3d4 is written to 0.05 at the body's entry and the driver raises the noise by 5 cm, so the noise's height distance is over the range and the arm runs - in the game area 59's commando has the range at 0, so the tie arm runs instead |
| 0x0000BF34 (a shot, heard) | (b) ran | coverage: 3 hits, the return value 1 |
| 0x0000BF3C (another noise type) | (b) ran | coverage: 3 hits (the noises 1, 2 and 3), the return value 0 |

The runs: Temple Grounds 05_temple_federationsite_dark (area 59), the CommandoPirate lane's driver
`tools/drivers/commandopirate_warpout.py` (it wakes the commando, puts the player five metres in
front of it, makes the five noise types, raises one noise by 5 cm and then moves the commando 80 m
away for two more), under `tools/cov/run.sh` with `tools/cov/specs/commandopirate.py` (the
coverage run: the helper was called nine times, no probe, no rebuild) and under
`tools/cov/force.sh` with `tools/cov/specs/commandopirate_force.py` (the two forced arms, one
gdb write each). `grep -c '[port] caught|reach-stub'` on the run log is 0.

### 6.2 `fn_9_3900` - the jump-back velocity: **marked** (2026-10-22)

The module's jump-back velocity (fn_9_3900, 0x15c) runs only from the module's own launching
message - the kSM_Launching case of the message switch (fn_9_CB40), the call at 0x0000D040 - which
the area 59 run does not send.  The console's `send <id> <msg>` takes a number, though, so the
module's message (the fourcc 0x584C4155) can be delivered to the commando:

    send <id> 0x584c4155

The driver `tools/drivers/commandopirate_launch.py` sends it four times in a row: the helper sets
the 0x08 bit of the flag byte at guest this+0xC63 (0x00003A24) and the module's landed message
clears it again (0x0000D054), so the sends have to follow each other for the helper's early out
(0x0000393C) to run.  The cov spec `tools/cov/specs/commandopirate_launch.py` counts the helper's
entries.  The coverage run in area 59 (build-port-cov, `tools/cov/run.sh`, 8000 frames, game exit
code 0, no caught/reach-stub) gives the helper 15/15 lines and 2/2 branches, so the body is
marked.

The helper's formula is the guest's: `f31` is the gravity the vtable call at 0x00003974 returns in
`f1` (0x00003988), the apex is the float at guest this+0xBEC over the higher of the two heights
(0x00003994..0x000039A4), the z velocity is `sqrt(2 * g * (apex - pos.z))` (0x000039A8..0x000039BC,
`rodata_44C` = 2.0), the time is `zVelocity / g + sqrt(2 * (apex - target.z) / g)`
(0x000039C8..0x000039EC) and the velocity is `(dx / time, dy / time, zVelocity)`
(0x00003A00..0x00003A14).

### 6.3 `fn_9_2D0C` - the knock-back helper (2026-10-22)

The helper's body, with the guest's addresses:

- the weapon (the shield's own collision actor at guest this+0xBC6 -> the weapon that touched
  it, 0x00002D38..0x00002D74; the module's effect fn_9_2048 when the byte at guest this+0xC66 has
  its 0x20 bit, 0x00002D78..0x00002D90; otherwise the sender itself, 0x00002D98..0x00002DA8);
- the knock-back info (the weapon's forward column, its id, its owner id, its damage info and the
  direct flag, 0x00002DB8..0x00002E00);
- the knock-back manager's anim reaction range when the byte at guest this+0xC64 has its 0x01 bit
  (0x00002E04..0x00002E1C: `SetAnimReactionRange(kAR_Hurled, kAR_Fall)`), the knock-back itself
  (the vtable slot 0x9C, 0x00002E38) and the range the guest restores right after it
  (0x00002E3C..0x00002E48: `SetAnimReactionRange(kAR_None, kAR_Fall)`);
- the flag byte at guest this+0xC64: the manager's **active reaction** (the word at guest
  this+0x630, the manager's own +0x10 - the CRuleSetEvaluator base is 0x10 bytes and the reaction
  flags are the +0x20) is compared with 5 and 2 (0x00002E50 and 0x00002E58): 2 to 4 sets the
  byte's 0x04 bit (0x00002E64..0x00002E70), anything else ors the byte's 0x01 bit into its 0x04
  bit (the `extrwi` 1,29 = 0x04 and the `clrlwi` 31 = 0x01 or-ed and put back by the `rlwimi`
  2,29,29, 0x00002E78..0x00002E8C);
- the ground ray (the filter's include list kMT_Solid, `lbl_9_data_814`, and its exclude list
  kMT_Player, kMT_Character, kMT_CollisionActor and kMT_Platform, `lbl_9_data_81C`,
  `lbl_9_data_818`, `lbl_9_data_820` and `lbl_9_data_824`, the type 3 = kFT_IncludeExclude,
  0x00002F20..0x00002FC4) and, when the ray is clear, the death (0x00003018), the zeroed health
  (0x0000302C), the recorded damage source (0x00003044..0x00003068) and the byte's 0x04 bit
  (0x0000306C..0x00003078).

### 6.4 The six FSM states checked against the asm (2026-10-22)

`tools/asmcheck.py check CommandoPirate` (with `--all`, since the bodies are unmarked) finds no
difference in the six states: `fn_9_69C8` (the Dead state) and `fn_9_786C` (the
RestoreOrientation state) are clean, and the other four only raise the host-API findings:

- `fn_9_6A40` (the GetUp state) and `fn_9_7A98` (the ScriptedShieldCharge state) raise
  NOPOS: the base's bitfields `mPendingDeath` and `mVerticalMovement` have no stated bit
  position for the module, so the tool leaves their masks unchecked.  The guest's
  `mPendingDeath` is the byte at guest this+0x421's bit 3 (`lbz r0, 0x421` and
  `rlwimi r0, r3, 4, 27, 27` at 0x00006AB8..0x00006AC4) - the members live in the shared
  base class (include/MetroidPrime/Enemies/CPatterned.hpp), which this lane does not edit.
- the CALL findings name `CTryCommand::ShouldRetry` where the asm calls
  `CAnimationState::CanIssueCommand` (0x00006AD8, 0x00006E6C, 0x00007540, 0x00007C4C - the
  animation state ids 1 = kAS_Getup, 0x13 = kAS_Cover, 0x14 = kAS_WallHang and 9 =
  kAS_LoopAttack, which are the host's values too) and `CEntity::CheckConnectedObject_if`
  where the port calls `CEntity::FindConnectedObject_if` (0x00007414) - the host's two
  functions have the same body (src/MetroidPrime/CEntity.cpp:124 and 198).

### 6.5 The six states' untaken arms (2026-10-22, slice 30)

The tooling: `tools/drivers/commandopirate_states2.py` (the console's `fsm` forces the WallHang,
the RestoreOrientation, the ScriptedShieldCharge, the Cover and the GetUp; an unknown fourcc
reaches a message switch's default arm) and `tools/cov/specs/commandopirate_states2.py` (the
forcing spec: the WallHang's and the Cover's cover point - the Swamp's u05c8 - and the byte at
guest this+0xC66's 0x20 bit for the Dead state, which the knock-back helper sets when the shield
is up).  The run: the coverage build with
`COV_GDB_SCRIPT=tools/cov/cov_force.py tools/cov/force.sh tools/cov/specs/commandopirate_states2.py
<out> swamp,05_swamp_timber! --driver 'python3 tools/drivers/commandopirate_states2.py {port}'`
(8000 frames, game exit code 0, no caught/reach-stub).

| body | lines | branches | the arms that still did not run |
|---|---|---|---|
| `fn_9_7358` (the WallHang) | 21/39 | 7/22 | the message switch's default (0x000073A0..0x000073A4), the "no cover point" exit (0x000073D4..0x000073D8), the cover point's forward (0x00007478..0x0000748C), the update's angle test (0x000074BC..0x00007528), the wall-hang command's `CanIssueCommand` (0x00007534..0x00007548) and the body's own state (0x000075B8..0x00007628) - the last three need the body in the wall-hang state, which only the wall-hang command gives |
| `fn_9_786C` (the RestoreOrientation) | 17/29 | 9/14 | the message switch's default (0x000078B0..0x000078B4) and the update's angle arms (0x00007918..0x000079E0) - they need a commando that is tipped over, and nothing in the shipped flow tips it |
| `fn_9_7A98` (the ScriptedShieldCharge) | 55/60 | 23/30 | the message switch's default (0x00007B24..0x00007B28), the activate's arm (0x00007B38..0x00007B3C) and the update's arms (0x00007C64..0x00007CFC) - the charge's own abort and its waypoint arms need the module's shield-charge states |
| `fn_9_6DBC` (the Cover) | 19/23 | 9/14 | the message switch's default (0x00006E08..0x00006E0C), the "the body is already in the cover state" arm (0x00006E10..0x00006E14), the cover's attack direction (0x00006E50..0x00006E68), the cover command's `CanIssueCommand` (0x00006E5C..0x00006E74) and the "the body cannot shoot" arm (0x00006F48..0x00006F9C) |
| `fn_9_6A40` (the GetUp) | 15/17 | 5/10 | the message switch's default (0x00006A78..0x00006A7C) and the update's "the commando lies on the ground" arm (0x00006A90..0x00006AC4) - it needs the body in the lie-on-ground state and the path find with no source point |
| `fn_9_69C8` (the Dead) | 6/6 | 4/4 | **marked**: the two runs (the force from the first entry on and the force from the second entry on) give both arms of the byte's 0x20-bit test |

### 6.6 The message switch's default arm is dead (2026-10-22, slice 31)

`EStateMsg` has three values (include/MetroidPrime/StateMachineCommon.hpp:4-8:
`kStateMsg_Activate`, `kStateMsg_Update`, `kStateMsg_Deactivate`), and the state machine never
sends anything else: the only senders are the FSM2's own `ExecuteState` and `ExecuteSubflow`
calls, and they pass exactly those three -
include/MetroidPrime/CGenericFSM2StateImpl.hpp:21 (`kStateMsg_Deactivate`), :85 and :337
(`kStateMsg_Activate`), :96 and :101 (`kStateMsg_Update`) and :370 and :374
(`kStateMsg_Deactivate`).  The module runs on the FSM2 (its machine's type is 1), so the default
arm of a state's message switch is **dead** (class **c**), not untaken: the six bodies' untaken
"3/4" switch arms are that arm.

### 6.7 What the six states still need (2026-10-22, slice 31)

The forcing run of slice 31 (`tools/cov/force.sh` with
`tools/cov/specs/commandopirate_states2.py` and `tools/drivers/commandopirate_states2.py`, the
Swamp's 05_swamp_timber!, 8000 frames, game exit code 0, no caught/reach-stub) gives:

| body | lines | branches | the live arms that did not run |
|---|---|---|---|
| `fn_9_7358` (the WallHang) | 30/39 | 10/22 | the cover point's forward, the update's angle test, the wall-hang command's `CanIssueCommand` and the body's own state - the last two need the body controller in the wall-hang state |
| `fn_9_786C` (the RestoreOrientation) | 17/29 | 9/14 | the update's angle arms - they need a commando that is tipped over |
| `fn_9_7A98` (the ScriptedShieldCharge) | 55/60 | 23/30 | the activate's arm and the update's arms - the charge's abort and waypoint arms need the module's shield-charge states |
| `fn_9_6DBC` (the Cover) | 19/23 | 8/14 | the "the body is already in the cover state" arm, the cover's attack direction, the cover command's `CanIssueCommand` and the "the body cannot shoot" arm |
| `fn_9_6A40` (the GetUp) | 16/17 | 8/10 | the update's "the commando lies on the ground" arm (0x00006A90..0x00006AC4) - it needs the body in the lie-on-ground state (the console's `bodystate` gives it that) and the path find with no source point |
| `fn_9_69C8` (the Dead) | 0/6 | 0/4 | the state did not run in that run |

So none of the six can be marked yet.  The blocker for the body-state arms is the forcing
itself: the spec writes `$this->mBodyController->mBodyStateInfo.mState`, but the body controller
is an `rstl::single_ptr` (include/MetroidPrime/Enemies/CPatterned.hpp:409) and gdb reports "Could
not find operator->" for it - the write needs the pointer's target reached another way.

## The rag doll (cpragdoll, 2026-10-22)

The rag doll is upstream's `CPirateRagDoll` (module 50, include/MetroidPrime/Enemies/CPirateRagDoll.hpp),
created with `rs_new` the way CDarkTrooper/CDarkCommando create theirs, and kept in the new member
`xbfc_ragDoll` (guest this+0xBFC). Retail facts: the rag doll's vtable +0x8 Prime, +0xc Update,
+0x10 PreRender; its flag byte +0x70: 0x40 primed, 0x20 keeps making small movements, 0x01 over;
the module's vtable (lbl_9_data_828) +0x114 = fn_9_10 (the gravity constant, 50.f), +0x128
GetDeathTimeScale; the rag doll's thud is the block's `mSoundImpact` (guest this+0x7C0); the
sphere radii are rodata_3C8 (`skCommandoRagDollRadii`, 14 floats). It is created only after a
death: the BecomeRagDoll event, the drowning (`fn_9_3090`), the death with the block's flag 0x01,
and the timer path (0x00004020..0x0000410C, the flags 3 when the block's flag byte at guest
this+0x91C has its 0x01 bit, 0x000040A0..0x000040C0).

`fn_9_3E1C` (`x3E1C_UpdateRagDoll(mgr, dt, noRagDoll)`): with a rag doll it primes it once
(0x00003E54..0x00003EB8: the transform goes to identity at the same translation and the body's
playback rate to 0) and then updates it with the death time scale and the top of the active water
the actor is in (-2^127, rodata_4A8, without one; the water's `GetActive()` is the entity flag
byte's 0x80 bit, 0x00003F10..0x00003F20). Once it is over and no longer moving it starts the fade
(`mFadeToDeath`, `mAlphaDelta` -1/3 rodata_4AC, the projectile-passthrough material, zero momentum
and `Stop`, 0x00003F80..0x00003FF0). The update path then falls through into the drowning timer
(retail has no early return there).

Ported alongside, because the bodies had assumed the rag doll null: Think's primed-rag-doll arm
(fn_9_C944, the frozen body is unfrozen, 0x0000CA74..0x0000CA78), PreThink (CAC8), PreRender
(C6D4: the rag doll's PreRender, vtable +0x10), PreRenderAllViewports (C51C) and the KnockBack
override (C300: the torso impulse, the knock-back direction times 200000.f (rodata_4EC) times the
damage's knock-back power, 0x0000C324..0x0000C3AC). Two fixes outside the slice: `fn_9_4D84`'s
false gate takes the idle branch at 0x00004F4C (it returned) and its gate includes the rag-doll
test (0x00004DAC..0x00004DB4); `fn_9_3090`'s water test is `GetActive()` (lbz +0x20, extrwi 0x80),
not the trigger flags.

Runs (the coverage build, merged with tools/cov/gcov_report.py; 0 `[port] caught`/`reach-stub` in
each): area 59 natural (the c300 and core drivers), Torvus 05_swamp_timber! natural and forced
(tools/cov/force.sh with tools/cov/specs/commandopirate_ragdoll.py and
tools/drivers/commandopirate_ragdoll.py; the driver waits for a30, the area that holds the waters,
before it kills anything). force.py does not gate on a spec's exprs (it logs them); the sets carry
their own conditions.

| Body | Arm | How |
|---|---|---|
| `fn_9_3E1C` | the null rag doll / the timer / the creation (0x00003E3C, 0x00003FF8..0x00004118) | natural (the kills and the corpse shots) |
| | prime (0x00003E54..0x00003EB8), the update, the fade (0x00003F80..0x00003FF0) | natural |
| | the flags 3 creation (0x000040A0..0x000040C0) and the small movements (0x00003F90) | forced (the spec's 4090: the block flag 0x01 on every call) |
| | the timer running out with a rag doll (0x00004028) | forced (4020: the timer at 0xC00 at 0.001) |
| | an active water (0x00003F2C), an inactive one (0x00003F1C) | forced (3ED4: the primed rag doll's fluid id u058b for u0508, u05bd for u0500) |
| | a fluid that is not a water (0x00003F10) | forced (an earlier run of the spec, the fluid id the commando's own u0500 on every call) |
| `fn_9_C300` | the torso push (0x0000C324..0x0000C3AC) | forced (4090) |
| | the early out with a rag doll (0x0000C3B0..0x0000C3B8) | natural |
| `fn_9_C944` | the primed rag doll with a frozen body (0x0000CA74..0x0000CA78) | forced (CA74: the body frozen) |
| `fn_9_CAC8`, `fn_9_C6D4`, `fn_9_C51C` | every arm | natural |
| `fn_9_4D84` | the aim path, the 0xC65 0x02 flip, both target-vector arms, the idle branch and its two early outs | forced (a one-off spec: the melee cooldown 0xB7C above 1, the 0x02 bit and the block flag 0x10 in turn) |
| | `fn_9_3CBC` finds no target (0x00004E04) | not run (unchanged by this slice) |
| | alive with a rag doll (0x00004DAC) | dead: every writer of the rag doll is on a death path |

Think (fn_9_C944) was unmarked here (the CVisualKnowledge stand-in); marked since, see "The visual
knowledge". `fn_9_3090` stays unmarked (12/29 lines).

The flags 3 forcing gives the corpse a big launch when it is shot after drowning (u0500 ended
~9e6 units away in x in three forced runs; never in the natural runs, where the drowned corpse
faded in place). The forcing sets the block flag 0x01 on every call, a state the shipped
commandos may never reach; whether retail launches the same way is not checked.

## The visual knowledge (cpvis, 2026-10-22)

The guest's CVisualKnowledge at this+0xAB0 (0x3C bytes, up to the burst fire at 0xAF0) is the
DOL class the port already has (`include/MetroidPrime/CVisualKnowledge.hpp`, the DG head uses it).
The ctor builds it with the owner's id (this+0x8, 0x0000DA40), no locator (0xFF, 0x0000DA28) and
the intervals 0.1f / 0.05f (rodata_42C / rodata_470, 0x0000DA50 / 0x0000DA5C; 0x0000DA64).
Think updates it (0x0000C9F8), SelectTarget sets the player as its target (0x0000AB68). The
"module's byte at +0xAE8" is its flag byte (+0x38): the visible bit 0x80, set and cleared by the
DOL's Update, cleared by the shot (fn_9_57A0, 0x00005B70..0x00005B7C, now
`SetTargetVisible(false)`, an inline setter added to the header) and read by HasLineOfSight
(0x0000BAEC) and SetTargetDest (0x0000AA94). The old note that "nothing sets the bit" (HasLineOfSight's
true arm dead) was wrong: Update sets it.

Runs (build-port-link, gdb specs, area 59 `3bfa3eff,05_temple_federationsite_dark`, 0
`[port] caught`/`reach-stub` each): `tools/cov/specs/commandopirate_vis.py` with the core and the
states drivers (9000 frames each); `commandopirate_vis_force.py` under `COV_GDB_SCRIPT=
tools/cov/cov_force.py tools/cov/force.sh` with the core driver (`msgs`); the same spec under
`tools/cov/run.sh --con tools/cov/specs/commandopirate_vis_inactive.con` (1000 frames).

| Body | Arm | How |
|---|---|---|
| `fn_9_C944` Think | inactive return (0x0000C974) | natural (the .con's Deactivate: 220 of 880 calls) |
| | no rag doll / primed rag doll / unprimed rag doll | natural (core: 1519 / 608 / 1) |
| | frozen body without a rag doll (0x0000C9DC) | forced (vis_force: every 40th call; 15 skips of the bone tracking) |
| | primed rag doll frozen (0x0000CA74) | forced (the cpragdoll slice's CA74) |
| | host-only collision-manager null tests | dead: the only writer is kSM_Create (fn_9_45A4), which `CStateManager::AddObject` delivers (CStateManager.cpp:2789) before any Think; never null in ~11k calls |
| `fn_9_AB08` SelectTarget | not activate / target still there / new target | natural (core: 6 / 2 / 1) |
| `fn_9_BAEC` HasLineOfSight | true / false | natural (core 7 / 6, states 18 / 1986) |
| `fn_9_AA20` SetTargetDest | visible 0 / 1 with +0xC64 0x40 clear | natural (states 767 / 18); stays unmarked (not this slice) |
| `fn_9_57A0` the shot's clear (0x00005B70) | ran | natural (states: 335) |

Marked: `fn_9_C944`, `fn_9_AB08` (`fn_9_BAEC` keeps its marker with the new evidence).

