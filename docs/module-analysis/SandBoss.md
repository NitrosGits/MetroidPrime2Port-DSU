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
GetScanVisorRenderBounds CC84, ScanVisorRender CB30, GetDeathExplosionParticle A4.

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
| 0xDFC | -1 |
| 0xE00 | `CProjectileInfo` dark beam (darkBeamProjectile, darkBeamDamage) |
| 0xE28 | `CProjectileInfo` charge beam (chargeBeamInfo weapon) |
| 0xE50 / 0xE60 / 0xE70 | optional PART tokens: head-armour explosion, stampede-armour explosion, stampede sand fountain |
| 0xE80..0xE90 | nine `TUniqueId`s; 0xE88 = safe-zone target (`FoundSafeZoneTarget`); 0xE8E = stampede cover point |
| 0xE94 | 0 |
| 0xEF8 / 0xEFC | id / 8 |
| 0xF00..0xF07 | eight locator seg ids (the `lbl_55_rodata_48` names); 0xF08 = `Head_1` seg id |
| 0xF0C | colour (black) |
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
`// @ fn_55_X` mark; `[stand-in]` = a log-once shell (`[sandboss] ... is a port stand-in`): triggers
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
| 1AEC | CSandBoss::SeekStampedePointStep | [stand-in] log-once |
| 1B94 | CSandBoss::ShouldRepeaterAttackCheck | [stand-in] log-once |
| 1C70 | CSandBoss::SyncAttacker | [stand-in] log-once |
| 1D74 | CSandBoss::RotateHelper1D74 | [stand-in] log-once |
| 1F84 | CSandBoss::RotateHelper1F84 | [stand-in] log-once |
| 2030 | CSandBoss::RotateHelper2030 | [stand-in] log-once |
| 22E4 | CSandBoss::RotateToPlayerStep | [stand-in] log-once |
| 2498 | CSandBoss::RotateHelper2498 | [stand-in] log-once |
| 254C | CSandBoss::RotateHelper254C | [stand-in] log-once |
| 25D0 | CSandBoss::RotateHelper25D0 | [stand-in] log-once |
| 26C0 | CSandBoss::BuildPointList | [stand-in] log-once |
| 2750 | CSandBoss::UpdateAttacks | [stand-in] log-once |
| 2CA4 | CSandBoss::UpdatePointList | [stand-in] log-once |
| 2D40 | CSandBoss::CreateChargeBeam | [stand-in] log-once |
| 311C | CSandBoss::ChargeHelper311C | [stand-in] log-once |
| 31E8 | CSandBoss::ChargeHelper31E8 | [stand-in] log-once |
| 3324 | CSandBoss::ChargeHelper3324 | [stand-in] log-once |
| 3434 | CSandBoss::ChargeHelper3434 | [stand-in] log-once |
| 37F4 | CSandBoss::ChargeHelper37F4 | [stand-in] log-once |
| 3ACC | CSandBoss::StartTripleChargeBeam | [stand-in] log-once |
| 3F44 | CSandBoss::StartDoubleChargeBeam | [stand-in] log-once |
| 42A8 | CSandBoss::ShouldDarkBeamAttackCheck | [stand-in] log-once |
| 44CC | CSandBoss::FireDarkBeam | [stand-in] log-once |
| 4C60 | CSandBoss::HandleScreenShake | [stand-in] log-once |
| 51BC | CSandBoss::DestroySphereHelper | [stand-in] log-once |
| 528C | CSandBoss::HandleGroundImpact | [stand-in] log-once |
| 560C | CSandBoss::CrackSphere | [stand-in] log-once |
| 595C | CSandBoss::CountAttachedSiblings | [proven] host body |
| 59E4 | CSandBoss::NoSiblingHasSphereLocked | [proven] host body |
| 5A84 | CSandBoss::IsAttachedLeader | [proven] host body |
| 5B38 | CSandBoss::IsBallInRange | [stand-in] log-once |
| 5C30 | CSandBoss::AttachToSphereHelper | [stand-in] log-once |
| 5C6C | CSandBoss::Helper5C6C | [stand-in] log-once |
| 5E5C | CSandBoss::Helper5E5C | [stand-in] log-once |
| 5FEC | CSandBoss::HandleObjectDrop | [stand-in] log-once |
| 6250 | CSandBoss::SphereStateHelper6250 | [stand-in] log-once |
| 62E4 | CSandBoss::SphereStateHelper62E4 | [stand-in] log-once |
| 6434 | CSandBoss::SphereStateHelper6434 | [stand-in] log-once |
| 64E4 | CSandBoss::Helper64E4 | [stand-in] log-once |
| 6698 | CSandBoss::UpdateArmorResponseTypes | [proven] host body |
| 6960 | CSandBoss::SetArmorPieceState | [proven] host body |
| 6A68 | CSandBoss::BuildArmorModels | [proven] host body |
| 6EA0 | CSandBoss::FindConnections | [proven] host body |
| 70D0 | CSandBoss::SyncCollisionHealth | [proven] host body |
| 724C | CSandBoss::SetupCollisionActors | [proven] host body |
| 783C | CSandBoss::SetCollisionVulnerabilities | [proven] host body |
| 78E0 | CSandBoss::SnapJawsCollisionSetup | [stand-in] log-once |
| 79B0 | CSandBoss::HandleHitObject | [proven] host body |
| 7BB0 | CSandBoss::UpdateFlashColor | [proven] host body |
| 7C68 | CSandBoss::StampedeHelper | [stand-in] log-once |
| 7E60 | CSandBoss::Helper7E60 | [stand-in] log-once |
| 8008 | CSandBoss::Helper8008 | [stand-in] log-once |
| 8258 | CSandBoss::HandleDamage | [stand-in] log-once |
| 83AC | CSandBoss::RotateToPlayerHelper | [stand-in] log-once |
| 84A8 | CSandBoss::UpdateGroupCinematicState | [proven] host body |
| 86D0 | CSandBoss::UpdateTimers | [proven] host body |
| 8740 | CSandBoss::GetArmoredBounds | [proven] host body |
| 88D0 | CSandBoss::RenderSphere | [proven] host body |
| 89E8 | CSandBoss::RenderWithArmor | [proven] host body |
| 8A8C | CSandBoss::RenderArmor | [proven] host body |
| 8C88 | CSandBoss::CheckBodyStateForDarkBeam | [stand-in] log-once |
| 8CB4 | CSandBoss::ProbeDarkBeam | [proven] host body |
| 8D24 | CSandBoss::ProbeTripleCharge | [proven] host body |
| 8D94 | CSandBoss::ProbeDoubleCharge | [proven] host body |
| 8E18 | CSandBoss::SetCinematicStateExitSphere | [proven] host body |
| 8E24 | CSandBoss::SetCinematicStateChoking | [proven] host body |
| 8E30 | CSandBoss::SetCinematicStateStunned | [proven] host body |
| 8E3C | CSandBoss::SetCinematicStateNormal | [proven] host body |
| 8E48 | CSandBoss::SetCinematicStateStampede | [proven] host body |
| 8E54 | CSandBoss::SetCinematicStateUnderGround | [proven] host body |
| 8E60 | CSandBoss::ResetAttackTimes | [stand-in] log-once |
| 8F88 | CSandBoss::ResetHeadArmorHP | [proven] host body |
| 8F94 | CSandBoss::PlayHeadArmorExplosion | [stand-in] log-once |
| 917C | CSandBoss::EndStampede | [proven] host body |
| 9190 | CSandBoss::BeginStampedeCode | [proven] host body |
| 91A4 | CSandBoss::PreventSyncAttacks | [proven] host body |
| 91B8 | CSandBoss::AllowSyncAttacks | [proven] host body |
| 91CC | CSandBoss::UnLockSphere | [proven] host body |
| 91E0 | CSandBoss::LockSphere | [proven] host body |
| 91F4 | CSandBoss::WaitAttachToSphere | [proven] host body |
| 9200 | CSandBoss::WaitStampede | [proven] host body |
| 920C | CSandBoss::SelectSafeZoneTarget | [stand-in] log-once |
| 9438 | CSandBoss::RestorePlayerCollision | [stand-in] log-once |
| 952C | CSandBoss::RemovePlayerCollision | [stand-in] log-once |
| 9600 | CSandBoss::SetSuckAirArmor | [stand-in] log-once |
| 96B0 | CSandBoss::SetAttachedArmor | [stand-in] log-once |
| 974C | CSandBoss::SetStampedeArmor | [stand-in] log-once |
| 97DC | CSandBoss::SetDoubleChargeArmor | [stand-in] log-once |
| 986C | CSandBoss::SetTripleChargeArmor | [stand-in] log-once |
| 98FC | CSandBoss::Stampede | [stand-in] log-once |
| 9BB8 | CSandBoss::SeekStampedePoint | [stand-in] log-once |
| 9E3C | CSandBoss::SelectStampedePoint | [stand-in] log-once |
| A170 | CSandBoss::SnapJaws | [stand-in] log-once |
| A360 | CSandBoss::TripleCharge | [stand-in] log-once |
| A6CC | CSandBoss::DoubleCharge | [stand-in] log-once |
| A9DC | CSandBoss::RepeaterAttack | [stand-in] log-once |
| AC68 | CSandBoss::DarkBeamAttack | [stand-in] log-once |
| AED8 | CSandBoss::SpitOutBall | [stand-in] log-once |
| B06C | CSandBoss::JumpOffSphere | [stand-in] log-once |
| B2D8 | CSandBoss::DestroySphere | [stand-in] log-once |
| B544 | CSandBoss::BallInMouth | [stand-in] log-once |
| B658 | CSandBoss::SuckBall | [stand-in] log-once |
| B718 | CSandBoss::SuckAir | [stand-in] log-once |
| B98C | CSandBoss::RotateToPlayer | [stand-in] log-once |
| BAAC | CSandBoss::AttachToSphere | [stand-in] log-once |
| BD30 | CSandBoss::Deactivate | [stand-in] log-once |
| BDAC | CSandBoss::Dead | [stand-in] log-once |
| BEA4 | CSandBoss::UnderGround | [stand-in] log-once |
| BF64 | CSandBoss::Recover | [stand-in] log-once |
| BFF8 | CSandBoss::SyncAttachToSphere | [stand-in] log-once |
| C0A0 | CSandBoss::SyncStampede | [stand-in] log-once |
| C148 | CSandBoss::ArmorDestroyed | [proven] host body |
| C154 | CSandBoss::HeadArmorDestroyed | [proven] host body |
| C174 | CSandBoss::IsLastStage | [proven] host body |
| C188 | CSandBoss::IsBallInSuckRange | [stand-in] log-once |
| C1CC | CSandBoss::IsFacingPlayer | [stand-in] log-once |
| C258 | CSandBoss::ShouldDestroySphere | [proven] host body |
| C2A0 | CSandBoss::ShouldSnapJaws | [stand-in] log-once |
| C378 | CSandBoss::ShouldDoubleCharge | [stand-in] log-once |
| C534 | CSandBoss::ShouldTripleCharge | [stand-in] log-once |
| C730 | CSandBoss::FoundSafeZoneTarget | [proven] host body |
| C750 | CSandBoss::ShouldRepeaterAttack | [stand-in] log-once |
| C894 | CSandBoss::ShouldDarkBeamAttack | [stand-in] log-once |
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
| 1027C | SandBossChargeBeam::RayCollisionCheckWithWorld | [stand-in] log-once |
| 107B0 | SandBossChargeBeam::SandBossChargeBeam | [stand-in] log-once |
| 108E4 / 10C18 | SandBossChargeBeam::~SandBossChargeBeam | [stand-in] empty, base dtor |
| 10CB4 | CSandBoss::SortStampedePoints | [stand-in] log-once |
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

## 5. Runtime

2026-10-19, `build-port-link`, Agon Wastes `03_sand_temple_dark` (`WORLD=42b935e4,7575048a`, area 36),
`GODMODE=1 FRAMES=3000`, `INPUT_FILE` holding `LAYER5@400`. (`MP2_INPUT=` on the command line does
nothing: `tools/run_gameplay.sh` overwrites it with `INPUT_FILE`'s contents. `LAYER5@200` fires before the
warp's gameplay frame 240.) Layer 5 is `Amorbis Boss`; `[layer] area 36 layer 5 activated`.

- Three `CSandBoss` instances (uids 0169, 0183, 01da) construct and register, and get XALD/XCRT and
  later XDEL. No `[port] caught`, no `[reach-stub]`; the run ends at the frame limit (exit 0).
- They stay inactive (the intro cinema layers activate them in retail), so no stand-in runs on its own.
- With a temporary probe sending each instance `kSM_Activate` (removed before committing), the FSM
  starts in `UnderGround` and stays there: the stand-ins seen are `UnderGround` (BEA4), `UpdateAttacks`
  (2750) and the trigger `SyncStampede` (C0A0, returns false). No crash for the remaining ~2000 frames.
- Not tested: ASAN (no `build-port-asan` in the worktree), the intro cinema path, any attack.
