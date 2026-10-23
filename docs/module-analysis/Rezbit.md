# Rezbit REL (module 53): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CRezbit.cpp` (CRezbit) plus the
`SLdrRezbit.hpp` loader. The port's fixes are `TARGET_PC` blocks in those two files. Retail asm:
`build/G2ME01/Rezbit/asm/MetroidPrime/Enemies/CRezbit.s`. Per-function table:
`Rezbit_functions.tsv`.

## Behaviour fixes (rez1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_53_7240 AcceptScriptMsg, kSM_Alert/Damage/ResistedDamage | 0x7434/0x7448/0x7474 `lbz; rlwimi r0,r3,7,24,24; stb 0x420`: CPatterned `mHitByPlayerProjectile`. UpdateTimers turns it into mAlert on the next Think | `mAlert = true` (0xd70) |
| fn_53_61C4 FollowAttackPath, Activate | 0x6240 `sth r3,0x6c0(r29)`: `SetDestination` | `SetLastDestination` (0x6c2) |
| fn_53_1370 JoinTeam | 0x13F4 `li r5,1`, 0x13F8 `li r6,2`, 0x13FC `li r7,-1`: Melee, Projectile, Invalid | Projectile, Unknown, Invalid |
| fn_53_141C CheckVerticalClearance | 0x146C `cmplwi r3,0; beq 0x14C0`: with no target, the two target-height checks are skipped | null dereference |
| fn_53_1E48 DeflectMissiles | 0x2170 `fmuls` once: rodata_70 (0.017453292) x missileDeflectRate, i.e. `FromDegrees(rate)` | `FromDegrees(0.01745 * rate)` (two conversions) |
| fn_53_1E48 DeflectMissiles | 0x2204 `cmpwi 0xc08,5; beq 0x2228`: the near-list loop stops when 5 missiles are held | no cap (reserved_vector<5> overflow) |
| fn_53_2414 StartVirusAttack, RayCollideWorld filter | 0x24CC-0x2574: include 0x80 = Solid, Character, CollisionActor (data_3A8/3AC/3B0); exclude 0x88 = Player, ProjectilePassthrough (data_3B4/3B8); type 0x90 = 3 | all five materials in Include |
| fn_53_2660 FireEnergyBolt | 0x26B0 `mr. r29,r3; beq 0x295C`: return when there is no target. 0x2900 `addi r5,r1,0x84`: the bolt sound plays at gunPos (stored 0x26F4-0x2708) | null dereference; sound at GetTranslation |
| fn_53_2FB8 SpawnLaser | 0x31F8 `addi r5,r29,0xcfc`: the laser sound starts at mLaserEnd | GetTranslation |
| fn_53_3A48 LowerShields | 0x3A7C `bne 0x3AD8`: the mShieldTimer rearm sits with the DeliverCmd, inside the AdditiveLoopReaction arm | rearmed every call |
| fn_53_3C68 UpdateShield | 0x3D5C `beq 0x3E0C`: the mShieldSfx UpdateEmitter (0x3E0C-0x3E48) runs with or without the shield actor, still inside `!mShieldBroken` | only inside `if (shield)` |
| fn_53_1040 SLdrRezbitData ctor | 0x10AC `li r0,-1`; stw 0x124, 0x128: sound_VirusHUD/HUDReboot -1. 0x1270/0x12A4 CColor alpha 0.5 (rodata_50, and 0.5 x4 inner) | 0, 0; 127/255 |
| fn_53_168 LoadRezbit (SLdrRezbit ctor inlined, struct at r1+0x400) | 0x1F8 CColor(1 x4) -> +0x294 ambient; 0x200 `li r0,0xf` / 0x204 stw +0x2e4 visor flags | black, 0 |

Unlike the DarkCommando/Splinter loaders, retail stores no +0x38 = 3 and no echo-emitter flag here.

## Survey (rez1)

Objdiff of every function below 100% in `report.json`. Apart from the fixes above, the differences
are regalloc, scheduling, block order, string/rodata addressing, and inlined vs REL-local template
copies (the unnamed fn_53_* rows). Checked and equivalent:
- Listen, Render, DoUserAnimEvent, Think (an extra `mr r4` before UpdateTimers is a leftover).
- Should{Laser,EnergyBolt,Virus}Attack: retail adds rodata_18 (0.0) to dx^2+dy^2.
- PathFind; BecomeRezzed/BecomeDeRezzed (same CBCGenerateCmd bits); RaiseShields; BreakShield
  (fn_53_3954 is a CEntityInfo dtor copy); UpdatePatrolMovement (case order; Left = (y,-x,z),
  Right = (-y,x,z)); SetupLaserTargets; UpdateLaser (start 0xcf0, end 0xcfc, elapsed 0xd0c,
  duration 0xd08); UpdateSeparation; CanStrafe; FindStrafeDirection.
- SpawnLaser: SetDamageDuration runs before Fire in retail; Fire does not read it.
- StartVirusAttack's ApplyDamage filter: type 3 with an empty exclude equals Include.
- DeflectMissiles' sound loop: same "first new missile only" logic.
- CRezbit ctor: same stores and constants in order (the strafe-direction array is unrolled in
  retail, `__construct_array` upstream). LoadTypedefRezbitData and __sinit: same constants/calls.
- SLdrRezbitData +0x15c = 0: SLdrPlasmaBeamInfo's ctor already zeroes it.

Kept as upstream, noted:
- SetTargetDest's min_val: retail and upstream differ only for equal values or NaN.
- CheckNeighbors' left-side arm clears `available[1]` (looks like a bug), but objdiff is 100%,
  so retail does the same.
- UpdateDeflectSfx has no retail caller (T row).

## Spawns

`1baa96c2,02_cliff_reactorcore`, `layer 3; layer 4` (save mi_u): Rezbits e0005011b and
e00050065. Other areas: `docs/modules.tsv` row 53.

## Coverage

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON -DMP_SDK_HEADERS_ONLY=OFF`. Run
`BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/rezbit.py <out> 1baa96c2,02_cliff_reactorcore
--con <driver> --fast --frames <n>`, then merge with
`python3 tools/cov/gcov_report.py build-port-cov <gcov dirs...> --module Rezbit`.

Drivers (frames): `tools/drivers/tail21_rezbit.txt` (20500), `tail21_rezbit_hp.txt` (19500),
`tail21_rezbit_shield.txt` (8000), `tail24_rezbit_missile.con` (2900, de-rez forced with
`codecall`), `rezbit_forced.con` (4200: `fsm` FollowAttackPath, `send`
Deactivate/Activate/Alert/ResistedDamage). The natural runs reach StartVirusAttack through
ApplyDamage (new filter), FireEnergyBolt (153 entries), SpawnLaser (15), JoinTeam, LowerShields
and UpdateShield.

Marked (25): ALL-ARMS or branchless and fully line-covered, with asmcheck clean apart from tool
limits. FollowAttackPath and AttackPathOver are `forced (fsm)`; the rest come from natural play.
asmcheck findings on the marked bodies are all tool limits: CALL to unmarked fn_53_141C/1614/3538
(CheckVerticalClearance, CheckNeighbors, UpdatePatrolMovement) and fn_53_3298 (CDamageInfo copy);
inlined string dtors; NOPOS/BIT on the unpositioned bitfields mAlert/mHitByPlayerProjectile
(0xd70/0x420) and the inline `mLineOfSight.HasLineOfSight()` (0xbe8).

Not marked (partial arms): AcceptScriptMsg (39/40 lines; the sfx-removal arms of Delete), the
other states and triggers, the fixed helpers (CheckVerticalClearance, DeflectMissiles,
FireEnergyBolt, StartVirusAttack, LowerShields, UpdateShield, JoinTeam), the ctor/dtors and the
loaders. Uncovered: Idle, IsDeRezzed, UpdateDeflectSfx.
