# SandBoss REL (module 55): behaviour of `CSandBoss` (Amorbis / Dark Amorbis)

Analysis + host body, 2026-10-19. Spawns: Agon Wastes `03_sand_temple_dark`, layer 5 (`docs/module_spawns.tsv`).

Sources:

- REL ELF `../MetroidPrime2Port/build/G2ME01/SandBoss/SandBoss.plf`, linked against `build/G2ME01/main.elf`
  into an ET_EXEC with `rel_plf2exec.py` (module at 0x81000000), and decompiled with REA's Ghidra lane
  (Ghidra 12.1.4). The pseudocode for all 317 functions is `/tmp/opencode/sandboss/pc/fn_55_<off>.c`
  (concatenated: `/tmp/opencode/sandboss/all.c`). Regenerate it with the same tool chain if `/tmp` is gone.
- Disassembly `../MetroidPrime2Port/build/G2ME01/SandBoss/asm/auto_00_*_text.s`, `auto_03_*` (rodata),
  `auto_04_*` (data: FSM tables, vtables).

The goal is behaviour, not matching code. Tags: **[proven]** means read from an instruction or a table
entry. **[inferred]** means a strong reading that is not pinned to a symbol. `fn_55_X` names are REL
`.text` offsets.

Reading the pseudo-C (m2c) [proven]:

- The first argument is the return-value slot when the function returns a struct; then come
  `double dt` (states only), `int this`, `int mgr`, `int msg` (0 Activate, 1 Update, 2 Deactivate),
  `float dt`. Check each function against the offsets it dereferences.
- A pointer-typed parameter's offsets are **scaled by the pointee size** (`int*` `+ 0x597` is byte
  0x165C, `int*` `[0x114]` is byte 0x450, `short*` `+ 4` is byte 8, `int*` `+ 0x3a3` is byte 0xE8C).
  A scalar `int` parameter's offsets are already bytes (`+ 0x898`, `+ 0x14fc`, `+ 0xf00`).
- `func_0x80000000(...)` is an indirect call m2c lost: identify it from the arguments at each site
  (e.g. `(this+0x6b4, *(this+0x48c), 0xc)` is `mAnimationState.CanIssueCommand(*BodyController(),
  pas::kAS_Generate)`; `TCastToPtr`-shaped sites take the `ObjectById` result).
- `*(int*)(x + 0x6b4) = v` is `mAnimationState.SetState()`: 0 kAS_NotReady, 1 kAS_Ready, 3 kAS_Over.
  Body controller state at `+0x37C` is a `pas::EAnimationState` (5 Locomotion, 12 Generate, 18
  ProjectileAttack).
- `lbl_55_data_*` command words: `0xD` kBSC_AbortScripted, `0xA` kBSC_ExitState, `0xF` CBCGenerateCmd
  (type word 1, 5 or 9), `6` CBCProjectileAttackCmd, `7` CBCLoopAttackCmd. The 12-byte entries at
  `lbl_55_data_970/97C/988/994/9A0` are the REL's own weak copies of those classes' vtables (their
  dtors are inline), not data to port.

How the fight advances [proven]: `mRound` (0xDE4) only moves in `AcceptScriptMsg` on
`kSM_Increment` whose **sender** is this worm or whose `mWaitMode` is 0, and the only senders are
`DestroySphere`'s deactivate loop (fn_55_B2D8, one message per attached sibling) and
`JumpOffSphere`'s deactivate (fn_55_B06C, to itself). A run whose probe never reaches the
sphere/ball states therefore stays in the stampede cycle and never evaluates an attack trigger.

**Shape in one line:** a `CPatterned` (EPatternedAI 0x31) driven by an FSM2 asset, with 21 triggers, 20
states and 25 code functions. Amorbis is several `CSandBoss` instances that coordinate through a sibling
id list (`0xDD8` count, `0xDDC` ids). They share armour sets (attached, stampede, tail), a dark-beam
projectile, a charge-beam `CPlasmaProjectile` subclass, and a collision-actor chain over the bones
`Head_1`, `Spine_6`..`Spine_1` and `Skeleton_Root`.

## 1. Tables [proven]

`fn_55_11184` (static init) fills three name+PTMF tables from `lbl_55_data_4..`. All PTMFs are
non-virtual (the second word is -1). `SetupStateMachine` (`fn_55_CAA4`) calls `CPatterned::SetupStateMachine`
and then sets triggers (`lbl_55_data_100`, 0x15), states (`lbl_55_data_340`, 0x14) and code
(`lbl_55_data_5AC`, 0x19).

| Triggers | fn | States | fn | Code | fn |
|---|---|---|---|---|---|
| StateOver | CA90 | Recover | BF64 | SetTripleChargeArmor | 986C |
| IsUnderGround | CA84 | UnderGround | BEA4 | SetDoubleChargeArmor | 97DC |
| IsRecovered | CA70 | Dead | BDAC | SetStampedeArmor | 974C |
| CanStartNewRound | CA1C | Deactivate | BD30 | SetAttachedArmor | 96B0 |
| BeginStampede | CA10 | AttachToSphere | BAAC | SetSuckAirArmor | 9600 |
| FoundStampedePoint | C9D8 | RotateToPlayer | B98C | RemovePlayerCollision | 952C |
| StampedeOver | C9B8 | SuckAir | B718 | RestorePlayerCollision | 9438 |
| ShouldDarkBeamAttack | C894 | SuckBall | B658 | SelectSafeZoneTarget | 920C |
| ShouldRepeaterAttack | C750 | BallInMouth | B544 | WaitStampede | 9200 |
| FoundSafeZoneTarget | C730 | DestroySphere | B2D8 | WaitAttachToSphere | 91F4 |
| ShouldTripleCharge | C534 | JumpOffSphere | B06C | LockSphere | 91E0 |
| ShouldDoubleCharge | C378 | SpitOutBall | AED8 | UnLockSphere | 91CC |
| ShouldSnapJaws | C2A0 | DarkBeamAttack | AC68 | AllowSyncAttacks | 91B8 |
| ShouldDestroySphere | C258 | RepeaterAttack | A9DC | PreventSyncAttacks | 91A4 |
| IsFacingPlayer | C1CC | DoubleCharge | A6CC | BeginStampede | 9190 |
| IsBallInSuckRange | C188 | TripleCharge | A360 | EndStampede | 917C |
| IsLastStage | C174 | SnapJaws | A170 | PlayHeadArmorExplosion | 8F94 |
| HeadArmorDestroyed | C154 | SelectStampedePoint | 9E3C | ResetHeadArmorHP | 8F88 |
| ArmorDestroyed | C148 | SeekStampedePoint | 9BB8 | ResetAttackTimes | 8E60 |
| SyncStampede | C0A0 | Stampede | 98FC | SetCinematicStateUnderGround | 8E54 |
| SyncAttachToSphere | BFF8 | | | SetCinematicStateStampede | 8E48 |
| | | | | SetCinematicStateNormal | 8E3C |
| | | | | SetCinematicStateStunned | 8E30 |
| | | | | SetCinematicStateChoking | 8E24 |
| | | | | SetCinematicStateExitSphere | 8E18 |

Main vtable `lbl_55_data_760`. Its overrides, by slot name: dtor DB60, PreThink D6BC, Think D620,
AcceptScriptMsg D8BC, PreRender D290, AddToRenderer D208, Render D308, PreRenderAllViewports D08C,
GetHealthInfo D8, GetDamageVulnerability CF6C, GetTouchBounds 10, GetAimPosition CDF8,
GetSortingBounds CF8C, DoUserAnimEvent D384, GetScannableObjectInfo CDB0, TakeDamage 4C, CanBeShot AC,
IsListening 8, Listen B4, GetOrigin BC, RenderSystemsToBeDrawnLast D304, GetModelAlphau8 5C,
ProjectileInfo CDF0, GetSearchPath 64, GetPointSearchPath 6C, SetupStateMachine CAA4, GetRagDoll 74,
GetAttackTarget 7C, IsOnGround 8C, GetGravityConstant 98, IsScanVisorSelfRender 0,
GetScanVisorRenderBounds CC84, ScanVisorRender CB30, GetDeathExplosionParticle 0x124. Slot numbers
now come from `lbl_55_data_760` itself (rel.data 0x81012AE8, 82 words): **0xa4 is `TakeDamage` (4C)**,
whose body only sets the 0x448 hit-flash timer, and 7E60/8008 call it with unused arguments;
0x124 is the `GetDeathExplosionParticle` getter (`return this + 0x754`).

Strings [proven]: locators `Head_1`, `Spine_6`..`Spine_1`, `Skeleton_Root` (`lbl_55_rodata_48`, eight
entries). Effect names `StampedeSandFountainFx`, `StampedeArmorExplosionFx`, `ArmorExplosionFx`. Boss
name `BossSandBoss`. Beam name `SandBossChargeBeam`.

## 2. Constructor `fn_55_E170` member map [proven]

The constructor calls `CPatterned(0x31, ..., EMovementType 0, EColliderType 1, EBodyType 1)` and sets the vtable to `lbl_55_data_760`.
`param_9` is the loader's `SLdrSandBossData`. The retail struct lays out the same fields as
`include/MetroidPrime/ScriptLoader/SLdrSandBoss.hpp`, in word offsets: scan 0, commandIndex 1, cracked
spheres 2-4, snapJawDamage 5, spitOutDamage 9, minDarkBeam 0xd, darkBeamProjectile 0xf, darkBeamDamage
0x10, headArmorHP 0x16, headArmorExplosion 0x1a, stampede 0x1b (breakStampedeHP 0x1e, armorExplosion
0x1f, damage 0x22, sandFountain 0x26), doubleCharge 0x27, tripleCharge 0x39, chargeBeamInfo 0x4b,
withTailArmorModel 0x6f, withTailArmorSkinRules 0x70, vulnerabilities 0x71 / 0xc8 / 0x11f.

| off | what |
|---|---|
| 0x7C0 | full copy of `SLdrSandBossData` (`fn_55_FBF4`), 0x5D8 bytes; `0x7C4` = commandIndex, `0x818` = headArmorHP |
| 0xD98 | `CCollisionActorManager*` (0 at construction) |
| 0xD9C | `CBoneTracking` on `Head_1` (max angle 1.0471976 rad, max turn 1.5707964 rad/s, flags 1) |
| 0xDD8 / 0xDDC | sibling `CSandBoss` count / ids (`reserved_vector<TUniqueId,4>`) |
| 0xDE4 | round / stage (0..2); `IsLastStage` = 2; `CanStartNewRound`: commandIndex <= round < 3 |
| 0xDE8 | the `CPatternedInfo` turn speed (corrected: not an actor-params field) |
| 0xDEC | secondary `SCAN` token (scannableInfo1) |
| 0xDF0 | wait mode: 1 WaitStampede, 2 WaitAttachToSphere |
| 0xDF4 | cinematic state: 1 UnderGround, 2 Stampede, 3 Normal, 4 Stunned, 5 Choking, 6 ExitSphere |
| 0xDFC | charge-beam phase: -1 none, 0 grow, 1 hold, 2 shrink (fn_55_2750 / 31E8 / 311C) |
| 0xE00 | `CProjectileInfo` dark beam (darkBeamProjectile, darkBeamDamage) |
| 0xE28 | `CProjectileInfo` charge beam (chargeBeamInfo weapon) |
| 0xE50 / 0xE60 / 0xE70 | optional PART tokens: head-armour explosion, stampede-armour explosion, stampede sand fountain |
| 0xE80..0xE90 | nine `TUniqueId`s; 0xE88 = safe-zone target (`FoundSafeZoneTarget`); 0xE8E = stampede cover point |
| 0xE94 | charge-beam list: count 0xE94, data 0xE98, 0x20-byte entries, capacity 3 (ends at 0xEF8) |
| 0xEF8 | the charge-beam `CScriptEffect` (PLAY connection): ACTV/DCTV, global scale, translation |
| 0xEFC | 8 |
| 0xF00..0xF07 | eight locator seg ids (the `lbl_55_rodata_48` names); 0xF08 = `Head_1` seg id |
| 0xF0C | colour (black); 0xF10 flash timer (fn_55_7BB0) |
| 0xF14 | charge-beam phase timer (float; grow counts down from 0.6 s, shrink from chargeBeamInfo.shutdownTime) |
| 0xF18 | doubleCharge.minChargeBeamAttackTime (attack timer seed) |
| 0xF1C | minDarkBeamAttackTime (timer seed) |
| 0xF24 | stampede HP (breakStampedeHP); `StampedeOver` when <= 0 |
| 0xF2C | head armour HP (headArmorHP); `HeadArmorDestroyed` when <= 0; `ResetHeadArmorHP` reloads 0x818 |
| 0xF3C | 1 |
| 0xF4C / 0x11D0 | two eight-slot armour model sets (corrected: the second is at 0x11D0): `reserved_vector<optional<CModelData>,8>`, count set to 8 with every slot empty, then `fn_55_6A68` assigns attached slots 0..6 and stampede slots 0..7; the host keeps fixed arrays |
| 0x1454 | `int[8]`, all 0 (corrected: `lbl_55_data_740` is an int 0, not a vtable); per-locator armour state |
| 0x1478 | vector reserved to 0x10 (`fn_55_10FA8`) |
| 0x1488 | copy of the anim data's skinned-model token (the normal body model) |
| 0x1494 | `CSkinnedModel` token: withTailArmorModel + withTailArmorSkinRules |
| 0x14A0 | `CTransform4f` (identity) |
| 0x14D0 | `CQuaternion` of the spawn rotation |
| 0x14E0 / 0x14FC / 0x1518 / 0x1534 / 0x1550 | `CDamageInfo`: doubleCharge, tripleCharge, stampede, snapJaw, spitOut |
| 0x156C / 0x159C / 0x15CC | `CDamageVulnerability`: normal, stampede, suckAir |
| 0x15FC / 0x162C | both `CDamageVulnerability::ReflectVulnerabilty()` (corrected; the earlier ImmuneRicochet/PassThru reading was wrong) |
| 0x165C..0x165E | flag bits (see section 3) |

At the end the constructor disables knock-back physics and anim reactions, resolves the locator seg
ids, builds the armour (`fn_55_6A68`) and turns off the shadow.

### 2a Charge-beam list, phases and projectile [proven]

- `0xE94` is a `reserved_vector` (count `0xE94`, data `0xE98`, capacity 3: `0xE98 + 3*0x20 = 0xEF8`)
  of `{ TUniqueId beam @+0x00, CVector3f start @+0x04, CVector3f end @+0x10, CSfxHandle sfx @+0x1C }`.
  The ids are `SandBossChargeBeam` (a `CPlasmaProjectile` subclass, 0x6B8 allocated).
- Phases (`0xDFC`, timer `0xF14`): -1 none, 0 grow (0.6 s; scale = 1 - timer/0.6), 1 hold (scale =
  the model scale), 2 shrink (timer = `chargeBeamInfo.shutdownTime`, scale = timer/shutdownTime).
  `0x918` = `mData.unknown_0x7619e561.chargeBeamInfo.shutdownTime` (0x12C + 0x2C, 0.25 s default);
  `0x8EC` = the same `chargeBeamInfo` for `TLdrToBeamInfo`.
- Shrink end: `kSM_Deactivate` (0x44435456) to the `0xEF8` `CScriptEffect`, `RemoveEmitter` for each
  entry, `DeleteObjectRequest` for each beam, list cleared, phase -1. `fn_55_311C` starts a volley:
  effect transform = `Translate(centre)`, `kSM_Activate` (0x41435456), phase 0, timer 0.6.
- `0x60` is `CActor::mModelData`; the effect scale is `GetModelData()->GetScale()` times the phase
  factor. Hold also moves the effect to entry[0]'s start (`0xE9C`).
- Beam update: `LookAt(start, end, up)` -> `CPlasmaProjectile::UpdateFx` (vtable +0x8C),
  `CSfxManager::UpdateEmitter(sfx, mCollisionPoint (0x444), up, 0x7f)`, and the damage info `0x14E0`
  (when `0x165C` b1, the double-charge probe) or `0x14FC` is applied to the player when the beam tip
  is within 10 units (`dist^2 < 100`) together with `0x421` bit1 and `0x73C = 1.0f`.
- `fn_55_26C0` counts entries whose object exists, whose byte `0x6A6` has bit5 set and whose int
  `0x5E8` is not 3: the plasma projectile's bitfield byte and its expansion state (`!= kES_Release`).
  `fn_55_2CA4` calls vtable +0x90 (`ResetBeam(mgr, false)`) on every listed beam.
- `fn_55_1027C` (the beam's world raycast override): no world raycast at all. It clears `idOut` and
  its own 0x3F6 last-resolved id, clips `start..end` against the 0x6A8 plane when the segment runs
  into it (`denom = dot(dir, normal) < 0`, `t = -(dot(start, normal) - constant) / denom`) and takes
  the result only when `t` is inside the magnitude; that result's material is `CMaterialList` bit 59
  (`kMT_Solid`, the `1 << lbl_55_data_73C` word). Then `CGameProjectile::CanCollideWith(player, mgr)`
  (player = `mgr+0x14FC`): a nearer hit calls the player's vtable +0x4C with (beam, mgr), replaces the
  result, sets `idOut` and 0x3F6. The near list and static-geometry test are unused.
- Volley geometry (3ACC/3F44, and 3434 sweeping the ends): each participating sibling contributes its
  mouth-locator point; the beams' start is the centroid (`/3`, `/2`), each end is `bossPos + radius * dir`
  with `radius` = `tripleCharge.innerRadius` (0x8C4) / `doubleCharge.innerRadius` (0x87C) and `dir` a
  normalized sum of the siblings' facings (`GetColumn(kDY)`, +0x28/0x38/0x48; triple: the three
  neighbouring pairs, double: the sum and its horizontal mirror, x and y negated), falling back to one
  worm's facing when the sum is not magnitude-safe. Sounds: `tripleCharge.sound_ChargeBeam` (0x8DC) / `doubleCharge.sound_ChargeBeam`
  (0x894). 0x8C8 = `tripleCharge.outerRadius` (fn_55_3434 oscillates between the two radii).
- `fn_55_2D40`: `TLdrToBeamInfo(chargeBeamInfo, 0x91 | flag ? 0x100 : 0)`, `AllocateUniqueId`,
  `rs_new SandBossChargeBeam(mChargeBeamInfo.Token() (0xE28), beamInfo, uid, area, owner, plane)`;
  the plane is normal up with constant `up . translation`; then `Fire(LookAt(start, end, up), mgr,
  false)`, `AddObject`, a looping `AddEmitter(sfx, mCollisionPoint, 0x7f, area, acoustics, med
  priority)` and the entry push. The subclass writes 16 bytes at `0x6A8` (that plane: three normal
  floats then the constant); `fn_55_1027C` clips the ray to it and `fn_55_31E8` stores 3 there.

## 3. Flags [proven from the triggers/code functions]

- `0x165C` bit7 IsUnderGround; bit6 BeginStampede (trigger); bit4 stampede active (Begin/EndStampede code); bit3 dark-beam probe result; bit1 double-charge probe result; bit0 triple-charge probe result.
- `0x165D` bit7 attached to the sphere (counted by `fn_55_595C`); bit6 ArmorDestroyed; bit5 not recovered (`IsRecovered` = !bit5); bit4 suck-air armour (gates IsBallInSuckRange); bit0 sphere locked (Lock/UnLockSphere).
- `0x165E` bit7/6/5 probe-in-progress (double / triple / dark beam); bit4 sync attacks allowed.

Sibling helpers: `fn_55_595C` counts the attached siblings; `fn_55_5A84` reports "attached and lowest
commandIndex among attached" (the leader); `fn_55_59E4` reports that no sibling has the sphere locked.
The probes `fn_55_8CB4`, `fn_55_8D24` and `fn_55_8D94` set a 0x165E bit, run one FSM update (state-machine
vtable +0x24) and read back the 0x165C result bit.

## 4. Function map

Slice A (2026-10-19). `[proven]` = the host body follows the retail pseudocode and carries a
`// @ fn_55_X` mark (a run reached it); `[ported]` = the same, but no run has reached it yet
(`// fn_55_X (ported, not exercised yet)`); `[stand-in]` = a log-once shell (`[sandboss] ... is a port stand-in`): triggers
and bool helpers return false, states/code functions/void helpers do nothing, the charge beam forwards
to `CPlasmaProjectile`. Files: `PortCSandBoss.cpp` (core, collision/armour, tables, loader),
`PortCSandBossStates.cpp` (triggers/states/code functions, 78E0, 8C88), `PortCSandBossAttacks.cpp`
(attack helpers, charge beam).

| fn | role | evidence |
|---|---|---|
| 0 | CSandBoss::IsScanVisorSelfRender | [proven] host body |
| 8 | CSandBoss::IsListening | [proven] host body |
| 10 | CSandBoss::GetTouchBounds | [proven] host body |
| 4C | CSandBoss::TakeDamage | [proven] host body |
| 5C | CSandBoss::GetModelAlphau8 | [proven] host body |
| 64 | CSandBoss::GetSearchPath | [proven] host body |
| 6C | CSandBoss::GetPointSearchPath | [proven] host body |
| 74 | CSandBoss::GetRagDoll | [proven] host body |
| 7C | CSandBoss::GetAttackTarget | [proven] host body |
| 8C | CSandBoss::IsOnGround | [proven] host body |
| 98 | CSandBoss::GetGravityConstant | [proven] host body |
| A4 | CSandBoss::GetDeathExplosionParticle | [proven] host body |
| AC | CSandBoss::CanBeShot | [proven] host body |
| B4 | CSandBoss::Listen | [proven] host body |
| BC | CSandBoss::GetOrigin | [proven] host body |
| D8 | CSandBoss::GetHealthInfo | [proven] host body |
| 178 | loader (PortLoadSandBoss, SLdrSandBoss.inc) | [proven] host body |
| 1AEC | CSandBoss::GetStampedeDelay | [proven] host body |
| 1B94 | CSandBoss::ShouldRepeaterAttackCheck (counts active safe zones) | [proven] host body |
| 1C70 | CSandBoss::SyncAttacker (mgr, delta) | [proven] host body (caller: DarkBeamAttack AC68) |
| 1D74 | CSandBoss::RotateHelper1D74 | [stand-in] log-once |
| 1F84 | CSandBoss::FaceTowardPoint (dt, point) | [proven] host body |
| 2030 | CSandBoss::RotateHelper2030 | [stand-in] log-once |
| 22E4 | CSandBoss::RotateToPlayerStep | [stand-in] log-once |
| 2498 | CSandBoss::RotateHelper2498 | [stand-in] log-once |
| 254C | CSandBoss::RotateHelper254C | [stand-in] log-once |
| 25D0 | CSandBoss::RotateHelper25D0 | [stand-in] log-once |
| 26C0 | CSandBoss::CountActiveChargeBeams (returns the count) | [proven] host body |
| 2750 | CSandBoss::UpdateChargeBeams | [proven] host body (volleys) |
| 2CA4 | CSandBoss::ResetChargeBeams | [proven] host body |
| 2D40 | CSandBoss::CreateChargeBeam (start, end, sfx, charged) | [proven] host body |
| 311C | CSandBoss::StartChargeBeamGrow | [proven] host body |
| 31E8 | CSandBoss::EndChargeBeamVolley | [proven] host body |
| 3324 | CSandBoss::UpdateChargeBeamRetarget | [proven] host body |
| 3434 | CSandBoss::UpdateTripleChargeSweep | [proven] host body |
| 37F4 | CSandBoss::UpdateDoubleChargeSweep | [proven] host body |
| 3ACC | CSandBoss::StartTripleChargeBeam (mgr, locator) | [proven] host body |
| 3F44 | CSandBoss::StartDoubleChargeBeam (mgr, locator) | [proven] host body |
| 42A8 | CSandBoss::ShouldDarkBeamAttackCheck (returns the picked uid) | [proven] host body |
| 44CC | CSandBoss::FireDarkBeam (mgr, dt) | [proven] host body |
| 4C60 | CSandBoss::HandleScreenShake | [stand-in] log-once |
| 51BC | CSandBoss::DestroySphereHelper | [ported] host body, not exercised yet |
| 528C | CSandBoss::HandleGroundImpact | [stand-in] log-once |
| 560C | CSandBoss::CrackSphere | [ported] host body, not exercised yet |
| 595C | CSandBoss::CountAttachedSiblings | [proven] host body |
| 58B4 | CSandBoss::CollectAttachedSiblings (attached ids) | [proven] host body |
| 59E4 | CSandBoss::NoSiblingHasSphereLocked | [proven] host body |
| 5A84 | CSandBoss::IsAttachedLeader | [proven] host body |
| 5B38 | CSandBoss::IsBallInRange (const CPlayer&) | [ported] host body, not exercised yet |
| 5C30 | CSandBoss::AttachToSphereHelper | [proven] host body |
| 5C6C | CSandBoss::Helper5C6C (mgr, dir) | [ported] host body, not exercised yet |
| 5E5C | CSandBoss::Helper5E5C | [ported] host body, not exercised yet |
| 5FEC | CSandBoss::HandleObjectDrop | [ported] host body, not exercised yet |
| 6250 | CSandBoss::SphereStateHelper6250 (CPlayer&) | [ported] host body, not exercised yet |
| 62E4 | CSandBoss::SphereStateHelper62E4 (dt, CPlayer&) -> bool | [ported] host body, not exercised yet |
| 6434 | CSandBoss::SphereStateHelper6434 (dt) | [ported] host body, not exercised yet |
| 64E4 | CSandBoss::Helper64E4 | [proven] host body |
| 6698 | CSandBoss::UpdateArmorResponseTypes | [proven] host body |
| 6960 | CSandBoss::SetArmorPieceState | [proven] host body |
| 6A68 | CSandBoss::BuildArmorModels | [proven] host body |
| 6EA0 | CSandBoss::FindConnections | [proven] host body |
| 70D0 | CSandBoss::SyncCollisionHealth | [proven] host body |
| 724C | CSandBoss::SetupCollisionActors | [proven] host body |
| 783C | CSandBoss::SetCollisionVulnerabilities | [proven] host body |
| 78E0 | CSandBoss::SnapJawsCollisionSetup (mgr, bounds) | [proven] host body (only caller: A170) |
| 79B0 | CSandBoss::HandleHitObject | [proven] host body |
| 7BB0 | CSandBoss::UpdateFlashColor | [proven] host body |
| 7C68 | CSandBoss::StampedeHelper | [proven] host body |
| 7E60 | CSandBoss::Helper7E60 | [stand-in] log-once |
| 8008 | CSandBoss::Helper8008 | [stand-in] log-once |
| 8258 | CSandBoss::HandleDamage | [stand-in] log-once |
| 83AC | CSandBoss::RotateToPlayerHelper | [proven] host body |
| 84A8 | CSandBoss::UpdateGroupCinematicState | [proven] host body |
| 86D0 | CSandBoss::UpdateTimers | [proven] host body |
| 8740 | CSandBoss::GetArmoredBounds | [proven] host body |
| 88D0 | CSandBoss::RenderSphere | [proven] host body |
| 89E8 | CSandBoss::RenderWithArmor | [proven] host body |
| 8A8C | CSandBoss::RenderArmor | [proven] host body |
| 8C88 | CSandBoss::CheckBodyStateForDarkBeam | [proven] host body |
| 8CB4 | CSandBoss::ProbeDarkBeam | [proven] host body |
| 8D24 | CSandBoss::ProbeTripleCharge | [proven] host body |
| 8D94 | CSandBoss::ProbeDoubleCharge | [proven] host body |
| 8E18 | CSandBoss::SetCinematicStateExitSphere | [proven] host body |
| 8E24 | CSandBoss::SetCinematicStateChoking | [proven] host body |
| 8E30 | CSandBoss::SetCinematicStateStunned | [proven] host body |
| 8E3C | CSandBoss::SetCinematicStateNormal | [proven] host body |
| 8E48 | CSandBoss::SetCinematicStateStampede | [proven] host body |
| 8E54 | CSandBoss::SetCinematicStateUnderGround | [proven] host body |
| 8E60 | CSandBoss::ResetAttackTimes | [proven] host body |
| 8F88 | CSandBoss::ResetHeadArmorHP | [proven] host body |
| 8F94 | CSandBoss::PlayHeadArmorExplosion | [ported] host body, not exercised yet |
| 917C | CSandBoss::EndStampede | [proven] host body |
| 9190 | CSandBoss::BeginStampedeCode | [proven] host body |
| 91A4 | CSandBoss::PreventSyncAttacks | [proven] host body |
| 91B8 | CSandBoss::AllowSyncAttacks | [proven] host body |
| 91CC | CSandBoss::UnLockSphere | [proven] host body |
| 91E0 | CSandBoss::LockSphere | [proven] host body |
| 91F4 | CSandBoss::WaitAttachToSphere | [proven] host body |
| 9200 | CSandBoss::WaitStampede | [proven] host body |
| 920C | CSandBoss::SelectSafeZoneTarget | [proven] host body |
| 9438 | CSandBoss::RestorePlayerCollision | [ported] host body, not exercised yet |
| 952C | CSandBoss::RemovePlayerCollision | [ported] host body, not exercised yet |
| 9600 | CSandBoss::SetSuckAirArmor | [ported] host body, not exercised yet |
| 96B0 | CSandBoss::SetAttachedArmor | [proven] host body |
| 974C | CSandBoss::SetStampedeArmor | [proven] host body |
| 97DC | CSandBoss::SetDoubleChargeArmor | [proven] host body |
| 986C | CSandBoss::SetTripleChargeArmor | [proven] host body |
| 98FC | CSandBoss::Stampede | [proven] host body |
| 9BB8 | CSandBoss::SeekStampedePoint | [proven] host body |
| 9E3C | CSandBoss::SelectStampedePoint | [proven] host body |
| A170 | CSandBoss::SnapJaws | [proven] host body |
| A360 | CSandBoss::TripleCharge | [proven] host body |
| A6CC | CSandBoss::DoubleCharge | [proven] host body |
| A9DC | CSandBoss::RepeaterAttack | [proven] host body |
| AC68 | CSandBoss::DarkBeamAttack | [proven] host body |
| AED8 | CSandBoss::SpitOutBall | [ported] host body, not exercised yet |
| B06C | CSandBoss::JumpOffSphere | [ported] host body, not exercised yet |
| B2D8 | CSandBoss::DestroySphere | [ported] host body, not exercised yet |
| B544 | CSandBoss::BallInMouth | [stand-in] log-once |
| B658 | CSandBoss::SuckBall | [stand-in] log-once |
| B718 | CSandBoss::SuckAir | [ported] host body, not exercised yet |
| B98C | CSandBoss::RotateToPlayer | [proven] host body |
| BAAC | CSandBoss::AttachToSphere | [proven] host body |
| BD30 | CSandBoss::Deactivate | [ported] host body, not exercised yet |
| BDAC | CSandBoss::Dead | [ported] host body, not exercised yet |
| BEA4 | CSandBoss::UnderGround | [proven] host body |
| BF64 | CSandBoss::Recover | [ported] host body, not exercised yet |
| BFF8 | CSandBoss::SyncAttachToSphere | [proven] host body |
| C0A0 | CSandBoss::SyncStampede | [proven] host body |
| C148 | CSandBoss::ArmorDestroyed | [proven] host body |
| C154 | CSandBoss::HeadArmorDestroyed | [proven] host body |
| C174 | CSandBoss::IsLastStage | [proven] host body |
| C188 | CSandBoss::IsBallInSuckRange | [ported] host body, not exercised yet |
| C1CC | CSandBoss::IsFacingPlayer | [stand-in] log-once |
| C258 | CSandBoss::ShouldDestroySphere | [proven] host body |
| C2A0 | CSandBoss::ShouldSnapJaws | [proven] host body |
| C378 | CSandBoss::ShouldDoubleCharge | [proven] host body |
| C534 | CSandBoss::ShouldTripleCharge | [proven] host body |
| C730 | CSandBoss::FoundSafeZoneTarget | [proven] host body |
| C750 | CSandBoss::ShouldRepeaterAttack | [proven] host body |
| C894 | CSandBoss::ShouldDarkBeamAttack | [proven] host body |
| C9B8 | CSandBoss::StampedeOver | [proven] host body |
| C9D8 | CSandBoss::FoundStampedePoint | [proven] host body |
| CA10 | CSandBoss::BeginStampede | [proven] host body |
| CA1C | CSandBoss::CanStartNewRound | [proven] host body |
| CA70 | CSandBoss::IsRecovered | [proven] host body |
| CA84 | CSandBoss::IsUnderGround | [proven] host body |
| CA90 | CSandBoss::StateOver | [proven] host body |
| CAA4 | CSandBoss::SetupStateMachine | [proven] host body |
| CB30 | CSandBoss::ScanVisorRender | [proven] host body |
| CC84 | CSandBoss::GetScanVisorRenderBounds | [proven] host body |
| CDB0 | CSandBoss::GetScannableObjectInfo | [proven] host body |
| CDF0 | CSandBoss::ProjectileInfo | [proven] host body |
| CDF8 | CSandBoss::GetAimPosition | [proven] host body |
| CF6C | CSandBoss::GetDamageVulnerability | [proven] host body |
| CF8C | CSandBoss::GetSortingBounds | [proven] host body |
| D08C | CSandBoss::PreRenderAllViewports | [proven] host body |
| D208 | CSandBoss::AddToRenderer | [proven] host body |
| D290 | CSandBoss::PreRender | [proven] host body |
| D304 | CSandBoss::RenderSystemsToBeDrawnLast | [proven] host body |
| D308 | CSandBoss::Render | [proven] host body |
| D384 | CSandBoss::DoUserAnimEvent | [proven] host body |
| D620 | CSandBoss::Think | [proven] host body |
| D6BC | CSandBoss::PreThink | [proven] host body |
| D8BC | CSandBoss::AcceptScriptMsg | [proven] host body |
| DB60 | CSandBoss::~CSandBoss | [proven] host body |
| E170 | CSandBoss::CSandBoss | [proven] host body |
| 1027C | SandBossChargeBeam::RayCollisionCheckWithWorld (plane + player clip) | [proven] host body |
| 107B0 | SandBossChargeBeam::SandBossChargeBeam | [proven] host body |
| 108E4 / 10C18 | SandBossChargeBeam::~SandBossChargeBeam | [stand-in] empty, base dtor |
| 10CB4 | CSandBoss::SortStampedePoints | [proven] host body |
| 11184 | static initialiser of the FSM tables | [proven] host static tables (PortCSandBoss.cpp) |

Host containers / inlined (no member function; rstl, token and template helpers the host's own
containers replace): 148, 4B4..1A28, 2FB4..3088, 480C..4AA8, 4B28..4C24, 4D78..4FEC, 5510..55BC, 5728, 57EC, 67A8..684C, 6E28, 6E58, 71F8, 7584..77DC, 7FAC, 81FC, 8C20, 8E10, 9B5C, A304, A670, AE7C, CC54, CF00, D1CC, DEA4, E088, F040, F220, F7F0, F9FC, FBF4, 10548..10760 (inlined into 1027C), 10AE8..10BC4, 10C78, 10E80..110F0, 10FA8.

Notes:
- `fn_55_D208` / `fn_55_D308`: retail draws body + armour from D208 (after `CPatterned::AddToRenderer`)
  and only particles from D308; the host draws body + armour in `Render`. Same output, the slot reading
  is [uncertain].
- `fn_55_D384` (DoUserAnimEvent) and `fn_55_D620` (Think) are proven but dispatch to stand-ins
  (screen shake, ground impact, `UpdateAttacks`, ...), so their effects are incomplete until slice B/C.
- Set*Armor (9600..986C) and ResetAttackTimes (8E60) are kept as stand-ins on purpose.
- Slice F (double/triple charge states): `A360`/`A6CC` are the two charge states -- activate marks the
  animation ready, seeds 0xF38 (-pi/2), 0xF3C (1) and 0xF40 (0), opens the mouth loop attack
  (`CBCLoopAttackCmd(kLAT_Zero)`) and sets the turn speed from `tripleCharge`/`doubleCharge.turnSpeed`;
  update keeps the loop attack commanded and, once the body is in `kAS_LoopAttack` and this worm is
  the leader, ends the volley (`31E8`: ExitState to every attached sibling, animation over, `2CA4`,
  phase -> 2 with the beam's shutdown time) when the state time passes `duration` or the live-beam
  count drops, else sweeps the beam ends (`3434` three bisectors of the worms' `kDY` columns, `37F4`
  the two-worm sum and its horizontal mirror, radius oscillating between inner and outer) and retargets the
  sweep (`3324`: `changeDirectionInterval/Variance/Chance` off the `CStateManager` RNG, flipping
  0xF3C); deactivate clears the probe bit, stores the loader's `unknown_0x8d4f3b88` in 0xF44, marks
  the animation not ready, ends the volley and restores the pInfo turn speed. Triggers `C534`/`C378`
  gate on `CountAttachedSiblings` (3/2), the leader (`5A84`), `0xF18`, `mSyncAttacksAllowed`,
  `0xF44 < 1`, and then the siblings' own probes (`8D24`/`8D94`); the double one also requires the
  player in front of this worm and of the first attached sibling along `GetColumn(kDY)`.
- Slice 10 (78E0): the vector it writes where retail stores three floats at collision actor +0x35C is
  `CCollisionActor::mExtendedTouchBounds`, which `GetTouchBounds` expands the box by [inferred: it is the only
  `CVector3f` in the class tail, and the tail arithmetic (mLastTouched 0x358, the vector 0x35C..0x368,
  mResponseType 0x368, `ncrc_ptr` 0x36C = the 0x370 `CHECK_SIZEOF`) only adds up with the vector at 0x35C,
  so the header's member order differs from retail's]. A170's activate passes `lbl_55_bss_8` (a lone `4.5f`
  that `fn_55_11184` stores into bss, so the other two words read as 0) and its deactivate `CVector3f::Zero()`.
  The host needs a setter, added as a `TARGET_PC` block in `include/MetroidPrime/CCollisionActor.hpp`.
- Slice 9 (the dark-beam pair, 44CC + AC68): retail `FireDarkBeam` takes `(mgr, dt)` (r3/r4/f1; the
  only caller is `DoUserAnimEvent`'s `kUE_Projectile` branch, gated on 0x165C b3). It aims on 0x165C
  **b2**: set = predicted-intercept lead (player `GetAimPosition`, `ProjectileInfo()`'s
  `PredictInterceptPos`, and the delta slewed toward the facing column `kDY` by 70 deg when it is
  further off than that, then 0xF48 += 1); clear = the safe-zone target's CONN/FOLW waypoint, else
  that actor's own translation. Both fire
  `LaunchProjectile(xf, mgr, 6, kPA_None, false, CImpactVisorEffect::None(), One())` and end with
  0xF34 = 0. `AC68` writes the bone tracking's ang speed (0xDBC) to **+pi/2** on activate and **+pi**
  on deactivate (rodata_D8/rodata_10C, not a +/-pi/2 pair); its deactivate is the only caller of
  `SyncAttacker` (fn_55_1C70) with delta -1.
- Slice E (charge-beam volleys, 3ACC 3F44 58B4 1027C): `58B4` is the attached-sibling id list the
  volleys need (both call `2CA4` first, then require exactly 3 / 2 attached worms, then fire from the
  centroid of their locator translations to `bossPos + innerRadius * dir`, `dir` from the pairwise
  differences, and call `311C` with the centroid). Two [inferred] points: retail reads each sibling's
  direction point from three floats at sibling+0x28/0x38/0x48 instead of `GetLctrTransform` (the host
  uses the same locator translation as the centroid), and the player hit calls the player's vtable
  slot +0x4C, taken to be `CActor::Touch`.
- Slice D (charge-beam list, 26C0 2750 2CA4 2D40 311C 107B0): `26C0` counts beams with
  `IsFiring()` instead of reading the private expansion state (retail also requires it not to be
  `kES_Release`); the shrink phase omits retail's per-beam `CVector3f` at beam+0x674 (host member
  not identified); `2D40` takes the retail-shaped arguments, so its callers `3ACC`/`3F44` pass
  (start, end, sound_ChargeBeam, charged) directly.

## 5. Runtime

2026-10-19, `build-port-link`, Agon Wastes `03_sand_temple_dark` (`WORLD=42b935e4,7575048a`, area 36),
`GODMODE=1 FRAMES=3000`, `INPUT_FILE` holding `LAYER5@400`. (`MP2_INPUT=` on the command line does
nothing: `tools/run_gameplay.sh` overwrites it with `INPUT_FILE`'s contents. `LAYER5@200` fires before the
warp's gameplay frame 240.) Layer 5 is `Amorbis Boss`; `[layer] area 36 layer 5 activated`.

- Three `CSandBoss` instances (uids 0169, 0183, 01da) construct and register, and get XALD/XCRT and
  later XDEL. No `[port] caught`, no `[reach-stub]`; the run ends at the frame limit (exit 0).
- They stay inactive (the intro cinema layers activate them in retail), so no stand-in runs on its own.
- `INPUT_FILE` tokens are comma-separated (`LAYER5@400,LAYER16@470`); space-separated ones fire only
  the first. Area 36 has 18 layers; 5 `Amorbis Boss`, 8 `Boss Intro Cinema`; 15 and 16 start active.
  Layer 8 alone does not start the fight (the intro needs the player in its trigger).
- Probe (local only, `/tmp/opencode/sb_probe.rdiff`): in `AcceptScriptMsg` `kSM_AreaLoaded`, after
  `FindConnections`, send `kSM_Activate` to self when `MP2_SB_PROBE` is set and the actor is inactive.
- Slice 2 (`MP2_SB_PROBE=1`, `LAYER5@400`, `FRAMES=6000`, exit 0, 0 caught, 0 reach-stub): the FSM goes
  `UnderGround` -> (`CanStartNewRound`, `BeginStampede`) -> `SyncStampede` true -> `SelectStampedePoint`
  stand-in (9E3C), so it stops there. `SetStampedeArmor` (974C) and `UpdateAttacks` (2750) stand-ins run.
  A once-per-function trace showed these marked functions ran: 8 10 5C D8 178 5A84 6698 6A68 6EA0 70D0
  724C 7BB0 84A8 86D0 8A8C 8E48 8E54 9190 9200 BEA4 C0A0 C9D8 CA1C CA84 CAA4 CDB0 D08C D208 D290 D304
  D308 D620 D6BC D8BC DB60 E170. Only these carry `// @`; other ported functions carry
  `// fn_55_X (ported, not exercised yet)` until a run reaches them.
- Slice 3 (same command, exit 0, 0 caught, 0 reach-stub): `SelectStampedePoint` (9E3C) ->
  `SeekStampedePoint` (9BB8, `GetStampedeDelay` 1AEC) -> `BeginStampede` -> `Stampede` (98FC,
  `StampedeHelper` 7C68) -> `StateOver`/`StampedeOver`; the stampede hits the player (`HandleHitObject`
  79B0) and the `HandleScreenShake`/`HandleGroundImpact` stand-ins run. The trace is once-per-function,
  so it does not show later rounds. Newly `// @`: 1AEC 783C 79B0 7C68 974C 98FC 9BB8 9E3C C9B8 CA10 CA90
  CDF8 CF6C CF8C 10CB4.
- Exit 13 (twice) is the shell tool's 120 s timeout killing a foreground run; run in the background.
- Slice 5 (2026-10-20): the ball-in-mouth core (5B38 6250 62E4 6434 B658 B544 C188) and then the sphere/ball
  states and helpers (5C6C 5E5C 5FEC 51BC 560C B718 AED8 B2D8 B06C) replaced their stand-ins; only
  `HandleScreenShake` (4C60) and `HandleGroundImpact` (528C) are left in the stampede path. Traced runs
  sbB/sbC/sbD/sbL (same area/layer, FRAMES 3000-6000, exit 0, 0 caught, 0 reach-stub) are identical to
  slice 4: none of the ball/attack states runs, and `mRound`/`mWaitMode`/`mCinematicState` stay 0 (the new
  `[sbstate]` tick). Newly `// @`: B4 (Listen, reached by the later runs).
- Probe knobs in `/tmp/opencode/sb_run.sh` (local only): `MP2_SB_BALL=1` parks a morphed player on each
  worm's mouth locator every frame (sets `mMorphBallState` and the translation), `MP2_SB_FSM=1` dumps the
  FSM2 graph once and prints the per-worm state tick every 180 `Think` calls. `/tmp/opencode/sb2_run.sh`
  adds `MP2_SB_HP=1` (forces `mStampedeHP = 0` each frame) and dumps the graph two levels deep.
- Slice 6 (2026-10-20, run sbM, same area/layer, `MP2_SB_FSM=1 MP2_SB_HP=1`, FRAMES 6000, exit 0, 0 caught,
  0 reach-stub): **why the FSM never left the stampede cycle.** The Stampede submachine only leaves through
  `StampedeOver` (C9B8: `mStampedeHP <= 0`), and `mStampedeHP` is only ever drained by `Helper7E60`
  (7E60: `mStampedeHP -= dt` while >0, then `kBSC_AdditiveReaction` + pain sound) which `HandleDamage`
  (8258, entered from `kSM_Damage`) calls when a weapon hits the armour. Both were stand-ins, so the HP
  never moved and the submachine sat in its terminal state forever — not an FSM-engine or `IsUnderGround`
  problem. With the HP forced to 0 the run walks the whole chain in one go: StateOver (CA90) -> StampedeOver
  (C9B8) -> EndStampede (917C) -> WaitAttachToSphere (91F4) -> SyncAttachToSphere (BFF8) ->
  SetCinematicStateNormal (8E3C) -> LockSphere (91E0) -> AttachToSphere (BAAC, AttachToSphereHelper 5C30) ->
  SetArmorPieceState (6960) -> ResetHeadArmorHP (8F88) -> SetAttachedArmor (96B0) -> ResetAttackTimes (8E60)
  -> `Attack` submachine: RotateToPlayer state evaluating ShouldSnapJaws (C2A0) / ShouldDarkBeamAttack
  (C894) / ShouldDoubleCharge (C378) / ShouldTripleCharge (C534), then back out via UnLockSphere (91CC) to
  the stampede. Stand-ins now reached: 4C60, 528C, 22E4, 83AC, C2A0, C894 (8258/7E60/8008 were not: the
  probe drains the HP directly instead of damaging the armour). Newly `// @` (20): 595C 59E4 5C30 64E4 6960
  8E3C 8E60 8F88 917C 91B8 91CC 91E0 91F4 96B0 B98C BAAC BFF8 C154 C378 C534.
- Slice 7 (2026-10-20, run sbS, same area/layer, `MP2_SB_FSM=1 MP2_SB_HP=1`, FRAMES 6000, exit 0, 0 caught,
  0 reach-stub): the attack slice's four triggers are now host bodies. `ShouldRepeaterAttack` (C750) answered
  true on the Attack submachine and it entered `RepeaterAttack` (A9DC, still a stand-in); C894 and C2A0 were
  evaluated too (`[sbtrace]`). Newly `// @`: C2A0 C750 C894.
- Slice 8 (2026-10-20, run sbT, same area/layer, `MP2_SB_FSM=1 MP2_SB_HP=1`, FRAMES 6000, exit 0, 0 caught,
  0 reach-stub): the four attack helpers are host bodies and the two signature changes went through with
  their call sites (1B94 -> `int`, 42A8 -> `TUniqueId`, 1C70 gains its `delta`; C894 now compares the pick
  to `GetUniqueId()` and probes that sibling). `[sbtrace]` shows 1B94 and 83AC running; C894 is evaluated
  but returns before its pick because it requires `mChargeBeamCooldown > 0` (0xF44), which only the
  charge-beam states' deactivate writes, so 42A8 stays unexercised and 8C88 with it. 1C70 has no caller yet.
  Newly `// @`: 1B94 83AC. Stand-ins still reached: 528C 4C60 A9DC 22E4.
- Slice 9 (2026-10-20, run sbU, same area/layer, `MP2_SB_FSM=1 MP2_SB_HP=1`, FRAMES 6000, exit 0, 0 caught,
  0 reach-stub): the dark-beam pair is a host body now but nothing reaches it. `[sbtrace]` holds no 44CC,
  AC68 or 1C70; `stand-ins:` shrinks to 528C 4C60 A9DC 22E4; `[sbtrace]` adds only 1AEC 1B94 2750 83AC on
  top of slice 8's set, and the Attack submachine still leaves through `ShouldRepeaterAttack` (C750) into
  `RepeaterAttack` (A9DC). C894 stops at its `mChargeBeamCooldown > 0` (0xF44) gate, which only the
  charge-beam states' deactivate writes, so 42A8 -> 8C88 -> `FoundSafeZoneTarget` and the DarkBeamAttack
  state never run. Both bodies keep `// fn_55_X (ported, not exercised yet)`.
- Slice 10 (2026-10-20, run sbV, same area/layer, `MP2_SB_FSM=1 MP2_SB_HP=1`, FRAMES 6000, exit 0, 0 caught,
  0 reach-stub): `RepeaterAttack` (A9DC) is a host body and the repeater runs end to end. The `[sbtrace]` order
  after `PreventSyncAttacks` (91A4) is A9DC -> 1F84 `FaceTowardPoint` at the player (its activate sets 0x165C
  b3, which is what lets the mouth's `kUE_Projectile` event fire) -> 44CC `FireDarkBeam` -> CDF0
  `ProjectileInfo` -> 1C70 `SyncAttacker` (A9DC's deactivate, retail delta -1) -> 920C `SelectSafeZoneTarget` /
  C730 `FoundSafeZoneTarget` -> AC68 `DarkBeamAttack`. So the repeater hands over to the dark beam once its
  three shots are out (0xF48), and C894's `mChargeBeamCooldown` gate opens at last. `stand-ins:` shrinks to
  528C 4C60 22E4. Newly `// @`: A9DC AC68 920C. 1C70, 1F84 and 44CC are exercised too but live in
  `PortCSandBossAttacks.cpp`, which this patch does not stage, so their markers and rows still say ported.
- `SnapJaws` (A170) never ran: `ShouldSnapJaws` (C2A0) is evaluated on every Attack pass but answers false in
  this probe (the player is out of its range/angle band), so A170 and `SnapJawsCollisionSetup` (78E0) keep
  `// fn_55_X (ported, not exercised yet)`. To exercise them the probe has to hold the player in front of the
  worm and inside CPatterned's min/max attack range when RotateToPlayer runs its trigger pass.
- `CheckBodyStateForDarkBeam` (8C88) is still unreached: only `ShouldDarkBeamAttackCheck` (42A8) calls it, and
  42A8 only runs once C894's cooldown gate opens (slice 8). The two signature changes C894/C750 needed —
  42A8 returns the picked worm's uid (C894 compares it to `GetUniqueId()` and otherwise makes that sibling run
  its dark-beam probe) and 1B94 returns the count of active safe zones — landed with their call sites in slice 8.
- Slice 11 (2026-10-20, run sbW, `MP2_SB_FRONT=1 MP2_SB_TRIG=1`, FRAMES 4000, exit 0, 0 caught, 0
  reach-stub): `/tmp/opencode/sb3_run.sh` adds two knobs. `MP2_SB_FRONT=1` parks the player at
  `translation + kDY * (mMinAttackRange + mMaxAttackRange) / 2` each frame, which makes `ShouldSnapJaws`
  (C2A0) answer true, so the `SnapJaws` state (A170) and `SnapJawsCollisionSetup` (78E0) run — both now
  `// @`. `MP2_SB_TRIG=1` logs the `RotateToPlayer` gating fields every 180 `Think` calls: the charge gates
  never fire because only one worm ever sets `mAttachedToSphere`. All 46 samples taken after `AttachToSphere`
  (BAAC) report `sib=1 lead=0` (`CountAttachedSiblings` 1, `IsAttachedLeader` false), with
  `mSyncAttacksAllowed` 0 and `mChargeBeamCooldown` 0, so `ShouldDoubleCharge`/`ShouldTripleCharge` can
  never reach their 2/3-sibling conditions. Next: add the per-worm `mData.commandIndex`, `mAttachedToSphere`
  and `mWaitMode` to that tick to see why the other two worms never finish the stampede cycle, then a probe
  that attaches all three to exercise the charge volleys (A360 A6CC 3ACC 3F44 3434 37F4 3324 31E8 26C0).
- Slice 12 (2026-10-20, run sbY, `MP2_SB_DUMP=1 MP2_SB_HP=1 MP2_SB_TRIG=1 MP2_SB_ROUND=2`, FRAMES 6000,
  exit 0, 0 caught, 0 reach-stub): **why only one worm ever attaches.** The new per-worm `[sbdump]` tick
  (uid, `mData.commandIndex`, `mAttachedToSphere`, `mSphereLocked`, `mWaitMode`, `mRound`,
  `mUnderGround`, `mStampedeHP`, state) shows the three worms as cmd 0/1/2 (uids 0183/01da/0169) and
  only cmd 0 leaves `UnderGround`: `CanStartNewRound` (CA1C, `commandIndex <= mRound`) gates the top
  machine and `mRound` only moves when a sphere is destroyed. Forcing `mRound` to the knob's value
  while fewer than three are attached lets all three run the stampede cycle, and with
  `MP2_SB_ROUND=2` all three attach (`sib=3`). The triple-charge volley then runs end to end:
  `TripleCharge` (A360) -> `StartTripleChargeBeam` (3ACC) -> `CreateChargeBeam` (2D40, SandBossChargeBeam
  107B0) -> `UpdateTripleChargeSweep` (3434) / `UpdateChargeBeamRetarget` (3324) -> `EndChargeBeamVolley`
  (31E8) with `26C0`/`2CA4`/`311C`; `SetTripleChargeArmor` (986C) runs on the way in and the charge
  beam's own `RayCollisionCheckWithWorld` (1027C) runs while the beams sweep.
- Slice 13 (2026-10-20, run sbAd, `MP2_SB_ROUND=1 MP2_SB_FAR=2 MP2_SB_DBL=1`, FRAMES 4000, exit 0,
  0 caught, 0 reach-stub): with `MP2_SB_ROUND=1` only cmd 0/1 start, so exactly two worms attach and
  the double charge fires: `DoubleCharge` (A6CC) -> `StartDoubleChargeBeam` (3F44) ->
  `UpdateDoubleChargeSweep` (37F4) -> ... -> 31E8, with `SetDoubleChargeArmor` (97DC). The `[sbdbl]`
  log inside `ShouldDoubleCharge` on the leader prints every gate: `count=2 lead=1 timer<0 sync=1 cd=0
  dotSelf=39.5 dotSib=39.5`. `MP2_SB_FAR=1` (player in front of the leader only) leaves `dotSib` at
  -39.5, so the trigger needs **both** attached worms to face the player; `MP2_SB_FAR=2` parks the
  player along the sum of the attached worms' `kDY` at `2*mMaxAttackRange + 5` (outside the SnapJaws
  band, so it does not hijack the transition order). After a volley `mChargeBeamCooldown` is 1 (the
  loader's `unknown_0x8d4f3b88`). Newly `// @` over slices 12/13 (21): 26C0 2CA4 2D40 311C 31E8 3324
  3434 37F4 3ACC 3F44 42A8 58B4 8C88 8D24 8D94 97DC 986C 1027C 107B0 A360 A6CC.
- Probe scripts (local only): `/tmp/opencode/sb4_run.sh` adds `MP2_SB_DUMP` (per-worm tick),
  `MP2_SB_ROUND=<0..2>` (forces `mRound`, so more worms pass `CanStartNewRound`), `MP2_SB_FAR=1|2`
  (parks the player in front of the leader / along the worms' summed `kDY`); `/tmp/opencode/sb5_run.sh`
  adds `MP2_SB_DBL` (`[sbdbl]` gate log in `ShouldDoubleCharge`).
- The attack submachine is now fully exercised except the Damaged branch: `SnapJaws` (A170), `RepeaterAttack`
  (A9DC), `DarkBeamAttack` (AC68), `TripleCharge` (A360) and `DoubleCharge` (A6CC) have all run; only
  `SuckAir`/`SuckBall`/`BallInMouth` (B718/B658/B544) need `HeadArmorDestroyed` (`mHeadArmorHP <= 0`,
  0xF2C) while attached, which a probe can force the way `MP2_SB_HP` forces 0xF24.
- `GetName()` on the FSM2 machine is useless as a state tick: the three top-level states are all literally
  named `UnderGround`. Read the `[sbtrace]` order instead.

- One run of the same command exited 13 right after `[area] 36` with no diagnostics; three reruns
  were clean. Unexplained.
- Slice 4 (charge-beam list and volleys, same command with `SECS=1500`, exit 0, 0 caught, 0 reach-stub):
  `UpdateChargeBeams` (2750) runs every frame on an empty list; newly `// @`: 2750. The only stand-ins
  reached are `HandleScreenShake` (4C60) and 528C; the FSM never evaluates an attack trigger, so the
  volleys (3ACC 3F44) and the beam itself stay unexercised. At `SECS=460` a loaded host only reached
  3000 of 6000 frames (exit 124).
- Not tested: ASAN, the intro cinema path, any attack, sphere/ball phases.

## 6. FSM2 graph (dumped from the loaded asset)

`[sbfsm]` probe in `Think` (`MP2_SB_FSM=1` in `/tmp/opencode/sb_run.sh`, run sbJ, 2026-10-20): the actor's
machine is a `CGenericFSM2` (`GetStateMachine2()`), not the `CStateMachine` that
`CAi::GetStateMachine()` casts the same token to — dumping the latter reads a different object (it printed
4 bogus states and segfaulted). Nodes are `state#`, `trigger#+10`, `code#+20`, `submachine#+30`; a
transition's bracket is its label and `#n` is the resolved target. Run sbM (`sb2_run.sh`, two levels deep)
adds `Attack#31` and `Damaged#30`; every machine below is now dumped.

**top**: Start#3 -> UnderGround#1 | UnderGround#1 -> CanStartNewRound#15 -> code WaitStampede#20 ->
code SetCinematicStateUnderGround#23 -> UnderGround#0 | UnderGround#0 -> SyncStampede#11 -> [Start]
submachine Stampede#30 -> trigger IsUnderGround#10 -> UnderGround#2 | UnderGround#2 -> Delay#12(3.0 s)
-> code WaitAttachToSphere#21 -> SyncAttachToSphere#14 -> [Start] submachine AttachedToSphere#31 ->
trigger IsUnderGround#13 -> code UnLockSphere#22 -> code WaitStampede#20.

**Stampede** (5 states, 5 triggers, 4 codes): Start#2 -> SetCinematicStateStampede#21 -> SetStampedeArmor#20
-> code BeginStampede#22 -> Delay#14(3.0 s) -> SelectStampedePoint#3 -> FoundStampedePoint#10 ->
SeekStampedePoint#0 -> BeginStampede#12 -> (trigger) -> Stampede#1 -> StateOver#11 -> {StampedeOver#13 ->
code EndStampede#23 -> UnderGround#4 (no triggers: the submachine's exit); repeat SelectStampedePoint#3}.

**AttachedToSphere** (2 states, 3 triggers, 6 codes): Start#1 -> SetCinematicStateNormal#24 ->
code LockSphere#20 -> state AttachToSphere#0 -> StateOver#10 -> code ResetHeadArmorHP#22 ->
code SetAttachedArmor#21 -> [Start] submachine Attack#31; trigger HeadArmorDestroyed#11 ->
code PreventSyncAttacks#23 -> [Start] submachine Damaged#30; trigger IsRecovered#12 ->
code SetCinematicStateNormal#25.

So `SuckAir`, `SuckBall`, `BallInMouth`, `SpitOutBall`, `DestroySphere`, `JumpOffSphere`, `DarkBeamAttack`,
`RepeaterAttack`, `DoubleCharge`, `TripleCharge`, `SnapJaws`, `RotateToPlayer`, `Recover`, `Dead` and
`Deactivate` all live inside `Attack`/`Damaged` (and their own nested machines) — the FSM must reach
`AttachedToSphere` before any of them can run, and that needs the top-level `UnderGround#2 -> Delay(3 s) ->
WaitAttachToSphere -> SyncAttachToSphere` chain after the stampede sub-machine exits (slice 6 reached it,
§5).

**Attack** (7 states, 9 triggers, 12 codes, entered from `AttachedToSphere`'s `SetAttachedArmor#21`):
Start#3 -> code ResetAttackTimes#30 -> code UnLockSphere#20 -> code AllowSyncAttacks#27 -> state
RotateToPlayer#0. RotateToPlayer has four unordered transitions, tried in this order: ShouldSnapJaws#10 ->
code LockSphere#21 -> code PreventSyncAttacks#28 -> state SnapJaws#1 -> (StateOver#14) -> code
ResetAttackTimes#30; ShouldDarkBeamAttack#13 -> code ShouldRepeaterAttack#18 -> code
PreventSyncAttacks#31 -> state RepeaterAttack#6 (StateOver#11 -> ResetAttackTimes) or, failing that, code
SelectSafeZoneTarget#22 -> code FoundSafeZoneTarget#12 -> code PreventSyncAttacks#29 -> code LockSphere#23
-> state DarkBeamAttack#2 (StateOver#11 -> ResetAttackTimes); ShouldDoubleCharge#15 -> code
SetDoubleChargeArmor#24 -> state DoubleCharge#4 (StateOver#16 -> code SetAttachedArmor#25 ->
ResetAttackTimes); ShouldTripleCharge#17 -> code SetTripleChargeArmor#26 -> state TripleCharge#5 (same
StateOver#16 exit). Every attack therefore returns to RotateToPlayer through ResetAttackTimes.

**Damaged** (11 states, 11 triggers, 11 codes, entered from `AttachedToSphere`'s `HeadArmorDestroyed#11` ->
code PreventSyncAttacks#23): Start#7 -> code LockSphere#24 -> code SetCinematicStateStunned#23 -> code
SetSuckAirArmor#20 -> state SuckAir#0 (IsBallInSuckRange#11 -> code RemovePlayerCollision#28 -> state
SuckBall#1 -> StateOver#12 -> SetCinematicStateChoking#27 -> BallInMouth#2 | StateOver#10 -> code
RestorePlayerCollision#21 -> Recover#8; or | BallInMouth#2: ArmorDestroyed#15 -> code ShouldDestroySphere#17
-> {IsLastStage#19 -> RestorePlayerCollision#30 -> Dead#9 | SetCinematicStateExitSphere#25 ->
DestroySphere#4 -> StateOver#16 -> RestorePlayerCollision#22 -> Recover#8} and SetCinematicStateNormal#26,
else StateOver#13 -> SpitOutBall#3 -> StateOver#14 -> SetCinematicStateStunned#29 -> RestorePlayerCollision
-> SuckAir#0 again). ShouldDestroySphere's other branch leaves the fight: IsLastStage#20 / RestorePlayerCollision
#22, and IsLastStage#10 -> Deactivate#10.
