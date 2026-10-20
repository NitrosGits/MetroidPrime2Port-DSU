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
| 0x10 | `PreThink` | `fn_9_CAC8` (0x78) | stand-in |
| 0x14 | `Think` | `fn_9_C944` (0x184) | stand-in |
| 0x18 | `AcceptScriptMsg` | `fn_9_CB40` (0x754) | stand-in |
| 0x24 | `PreRender` | `fn_9_C6D4` (0xc4) | stand-in |
| 0x28 | `AddToRenderer` | `fn_9_C4FC` (0x20) | host (a straight base call) |
| 0x2C | `Render` | `fn_9_C598` (0x13c) | stand-in |
| 0x34 | `PreRenderAllViewports` | `fn_9_C51C` (0x7c) | stand-in |
| 0x40 | `GetDamageVulnerability` | `fn_9_C2CC` (0x34) | host (the +0xC65 test) |
| 0x70 | `DoUserAnimEvent` | `fn_9_C798` (0x1ac) | stand-in |
| 0x84 | `CollidedWith` | `fn_9_BF44` (0x2b0) | stand-in |
| 0x9C | `KnockBack` | `fn_9_C300` (0x1fc) | stand-in |
| 0xA8 | `CanBeShot` | `fn_9_A4` | host (true) |
| 0xAC | `IsListening` | `fn_9_8` | host (true) |
| 0xB0 | `Listen` | `fn_9_BE4C` (0xf8) | stand-in |
| 0xB4 | `GetOrigin` | `fn_9_AC` | host (the translation) |
| 0xC8 | `GetModelAlphau8` | `fn_9_68` | host (the base's alpha byte) |
| 0xDC | `ProjectileInfo` | `fn_9_D474` (0x8) | host (this+0xA14) |
| 0xE0 | `GetSearchPath` | `fn_9_0` | host (this+0x928) |
| 0xE4 | `GetPointSearchPath` | `fn_9_70` | host (null) |
| 0xE8 | `GetContactDamage` | `fn_9_C1F4` (0xd8) | stand-in |
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
| 0 | Lurk | `fn_9_ADD4` | 0x6D2 |
| 1 | Ambush | `fn_9_ADA0` | 0x6D7 |
| 2 | Alert | `fn_9_AE04` | 0x6DE |
| 3 | FaceTarget | `fn_9_ABDC` | 0x6E4 |
| 4 | SelectTarget | `fn_9_AB08` | 0x6EF |
| 5 | SetTargetDest | `fn_9_AA20` | 0x6FC |
| 6 | SetRetreatDest | `fn_9_A97C` | 0x70A |
| 7 | SetJumpDest | `fn_9_A8E8` | 0x719 |
| 8 | SetPathMeshDest | `fn_9_A838` | 0x725 |
| 9 | MeleeAttack | `fn_9_A4BC` | 0x735 |
| 10 | EGrenadeAttack | `fn_9_A2EC` | 0x741 |
| 11 | PostEGrenadeAttack | `fn_9_A110` | 0x750 |
| 12 | JumpPointFind | `fn_9_9F24` | 0x763 |
| 13 | JetBoost | `fn_9_9BB4` | 0x771 |
| 14 | PathFind | `fn_9_9AAC` | 0x77A |
| 15 | Lurk | `fn_9_ADD4` | 0x6D2 | on activate, and only when the commando has a target, the 0x10 bit of the flag byte at guest this+0xC64 is set | `xc64_` (0) |
| Alert | `fn_9_AE04` | 0x6DE | activate: the target id goes into the field at guest this+0xBCC and the animation state becomes "ready"; update: the animation state is asked whether the alert command may be issued (the guest's command id 0x10) and the attack pattern is delivered; deactivate: the animation state back to "not ready" | the host's base has no attack pattern to deliver, so `DeliverAttackPattern` is a named stand-in |
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

Five of the 34 triggers are host code; the other 29 are still stand-ins. Each body is read off
the asm with `tools/asmcheck.py annotate` and carries the guest offset it came from.

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
| HasTarget | `fn_9_BA78` | 0x599 | the target the commando holds (guest this+0xBCA) is still in the world | the base's `GetObjectById` |
| HasNewTarget | `fn_9_BA20` | 0x5A3 | there is a target and it is a new one: `HasTarget(...)` and the id at guest this+0xBCC differs from the target id | `xbcc_` (new, kInvalidUniqueId) |

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
  (nothing in the module writes it, so it stays 0); `xb5c_` is the float at +0xB5C (the
  constructor stores 0.f there at 0x0000DABC); `xb60_` is the float at +0xB60 (the constructor
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
| Lurk | `fn_9_ADD4` | 0x6D2 | on activate, and only when the commando has a target, the 0x10 bit of the flag byte at guest this+0xC64 is set | `xc64_` (0) |
| Alert | `fn_9_AE04` | 0x6DE | activate: the target id goes into the field at guest this+0xBCC and the animation state becomes "ready"; update: the animation state is asked whether the alert command may be issued (the guest's command id 0x10) and the attack pattern is delivered; deactivate: the animation state back to "not ready" | the host's base has no attack pattern to deliver, so `DeliverAttackPattern` is a named stand-in |
| Patrol | `fn_9_996C` | 0x783 | the message picks the locomotion (activate: relaxed, half the turn speed, the steering ratio of the walk and run speeds; deactivate: combat, twice the turn speed) and then `mWaypointNavigation.Patrol(mgr, msg, dt, *this)` | the guest's body-controller flag at +0x4C (0x00009A1C, 0x00009A60) has no host counterpart |
| SelectTarget | `fn_9_AB08` | 0x6EF | on activate, when the old target is gone: take the player as the target, point the bone tracking at it, send `kSS_Attack` with `kSM_None`, and switch to the combat locomotion | the guest's visual knowledge (this+0xAB0) has no host class: `SetVisualTarget` is a named stand-in |
| FaceTarget | `fn_9_ABDC` | 0x6E4 | activate: the module's own helper `fn_9_3CBC` asks the target for its aim position (guest this+0xB94) and the state measures the angle between the actor's facing and the direction to it (0.34906584 rad = 20 degrees) - the animation is "over" when the target is in front and "ready" when it is not; update: the animation state's own command test, the direction to the target, its magnitude test and the locomotion command that walks there; deactivate: the animation state back to "not ready" | the guest's `CTryCommand::ShouldRetry` is the base's animation state, so the port calls `mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Turn)` (the guest's command id 8) |

The player id the guest takes (its state manager's +0x14FC, the id at +8 of it, 0x0000AB4C) has no
member at that offset in the host's state manager, so the port uses the player actor's id.

- `fn_9_3CBC` is the guest's own helper, ported as the private member `x3CBC_`: it looks the
  target up by the id at this+0xBCA and, when it exists, asks it for its aim position (CActor's
  vtable slot 0x54, `GetAimPosition`) with the state manager and the dt, and copies the answer
  into the out vector. It is 0xA8 bytes but trivial, so it is ported rather than stood in for.
- The guest's `CTryCommand::ShouldRetry` (guest this+0x6B4) is the base's animation state, so
  the port asks the animation state whether the command may be issued
  (`mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Turn)`, the guest's command id 8).
- The actor's facing is the transform's forward column (`GetTransform().GetForward()`), not the
  base's face vector: the guest's three loads are its 0x28, 0x38 and 0x48.

In area 59 (Temple Grounds/05_temple_federationsite_dark) the state machine walks the states the
actor reaches: Patrol, SelectTarget, FaceTarget, then Alert - and it stays there, because Alert
puts the target id into the field at guest this+0xBCC (the store at 0x0000AE44), which is what
`HasNewTarget` compares with the target id: after Alert the trigger is false again, exactly as in
retail. `HasTarget` and `HasNewTarget` are the first triggers of the module that return true in
the port - the commando's target is in the world. FaceTarget ticks every frame; the angle test
answers "ready" (the target is not yet in front) and the animation state is otherwise the base's
own, so `StateOver` stays false. No state stands in for the walk any more: every state the actor
reaches is host code.

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
| 0xBE0..0xBEC | four floats | 5.0f, 5.0f, then the two animation distances, 0 |
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

## 5. What is not ported yet (milestone 2 and forward)

- The state machine's other 48 bodies: the two tables (34 triggers, 36 states) are registered,
  eighteen of the triggers and five of the states are host code (the sections above), but every
  other function they point at is a stand-in that logs the retail function it stands in for and
  returns the neutral value.
- The guest's `CVisualKnowledge` (this+0xAB0) and the `CTryCommand` the base keeps at +0x6B4:
  the host has no class for either, so the calls that use them are stand-ins.
- The melee (`MeleeAttack`, the blade's collision actor and its damage), the E-grenade (the
  `Commando E-Grenade` script object), the shield (arm, charge, the mould) and the burst fire.
- The three `CToken` pairs (the script objects at this+0x8F0, +0x90C and +0x7C8) and the two
  `CElementGen` effects.
- The animation distances at +0xBE0/+0xBE4/+0xBE8 and the locomotion-speed term at +0xB90.
- The CVisualKnowledge at +0xAB0 (the host has no such class yet).
