# GunTurret REL (module 28): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CGunTurretBase.cpp` and `CGunTurretTop.cpp`
(the Luminoth, "Humility" and "Growler" turrets: a base that raises, pans and withdraws a separate top
actor that aims and fires) plus the `SLdrGunTurretBase.hpp` / `SLdrGunTurretTop.hpp` loaders. The
port's fixes are `TARGET_PC` blocks in those files. Retail asm:
`build/G2ME01/GunTurret/asm/MetroidPrime/Enemies/CGunTurret{Base,Top}.s`.

## Behaviour fixes (gt1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_28_7FB8 SLdrGunTurretTop ctor | 0x806C `stfs f0,0xac(r31)` rodata_1A4 (2.0) knock-back resistance; 0x8080 `stw r4,0x2b8(r31)` ambient CColor(1,1,1,1) (rodata_19C); 0x8084 `stw r0,0x308(r31)` visor flags 0xF | 0 / black / 0 |
| fn_28_13C0 SLdrGunTurretBase ctor | 0x1528 `stfs f9,0x13c(r31)` rodata_8 (2.0) knock-back resistance; 0x152C `stb r7,0x320(r31)` echo emitter 1; 0x1540 ambient CColor(1,1,1,1); 0x1544 visor flags 0xF | 0 / false / black / 0 |
| fn_28_1E14 LaunchProjectile | 0x1E6C-0x1EB4: TCachedToken try-cache on the projectile token (+0x884): cached pointer +0x88C, else lock byte +0x888 and the token's loaded word, then GetObj into +0x88C | `IsLoaded()` (no lock test, never caches) |

The guest offsets are SLdr-relative; the port's SLdrEditorProperties is 0xC larger, so the fixes use
named fields (patterned +0x4C knock-back, +0x230 echo emitter; actor +0x14 ambient, +0x64 visor).
The -1 sound defaults of both loaders were already ported (`x13C0_SoundDefaults`,
`x7FB8_SoundDefaults`).

## Survey (gt1)

Objdiff of the 16 functions below 100%. Apart from the fixes, the differences are regalloc,
scheduling and block order:
- CGunTurretTop: PowerDown (same `bge` shape), Revive (f4 = rodata_19C = 1.0), GetTouchBounds
  (REL-local optional_object ctor), LoadGunTurretTop.
- CGunTurretBase: PlayerInRange and FindTarget (filter: include 0x21, exclude 0x14|0x20, type 3, as
  upstream), UpdateGunOrientation (block order; the pitch clamp at 0x3778-0x37A8 is min/max as upstream),
  ResetAttack, GetScanVisorRenderBounds, the ctor, LoadGunTurretBase.
- fn_28_2594 is the `rstl::destroy<SParticleEffect>` wrapper.

`tools/upstream_hollow.py GunTurret` and `retail_diff --module GunTurret`: no findings.

asmcheck on the 46 marked bodies (CALL 14, BIT 3, NOPOS 3, BRANCH 1) is all tool limits: CALLs to
REL-local copies (fn_28_4B60 CDamageVulnerability `=`, fn_28_9A68 optional_object<CAABox>, fn_28_E8
~CPatterned), `internal_dereference`, `CMemory::Free`, `TCastToPtr`, and GetClosestCameraDistanceSq
inlined from the `PlayLoopedSfx`/`PlaySfx` helpers; mAlert/mOccluded/mHitByPlayerProjectile have no
stated bit positions (x9cc_80 / x9cc_40 are those two bits); PreRender's BRANCH is UpdateGunPose's
conditions merged in.

Per-function table: `GunTurret_functions.tsv`. B = upstream body, H = weak copy, template or
unnamed REL-local, T = REL runtime.

## Spawns

All load without a layer command (save mi_u); `objs GunTurret`.

| area | base / top ids |
|---|---|
| `3bfa3eff,0d_temple_hall` | u0463 / u0464 (131,272,-33) |
| `3bfa3eff,0p_temple_hall` | u0465 / u046a, u0437 / u0439 |
| `42b935e4,0i_sand_hall` | u043d / u043a, u0423 / u0424 (~350,-180,20); fires at the player start |
| `42b935e4,0j_sand_hall` | u044f / u0453 (430,-271,18) |
| `42b935e4,0l_sand_hall` | u047a / u047d, u0428 / u042c |
| `1baa96c2,0a_cliff_hall` | u047d / u0480, u0478 / u0477 |
| `1baa96c2,0m_cliff_hall` | u0501 / u0506, u04fc / u04fd |

The smoke family `turret` runs M06_ShootingGallery (the player's turret), not this module.

## Coverage

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON`. Runs (spec `tools/cov/specs/gunturret.py`):
- `gunturret_find.con` (discovery, 900 frames) in the seven areas.
- `gunturret_<area>.con` (`--fast --frames 3400`) in the seven areas: scan, `fsm` through IntoPan /
  PanLeft / PanRight / AttackExit / Flinch / CloseDoor / OpenDoor / Withdraw on one base and
  PowerDown / PowerUp on its top, `damage` and `kill` the top (Death -> DestroyGun -> Withdraw ->
  Revive), `statecall` IntoPan/AttackExit/Withdraw on the other base, then `tp` away.
- `gunturret_scan.con` (0i_sand_hall, `--fast --frames 2000`): scan visor held on the top, then the
  base (ScanVisorRender / GetScanVisorRenderBounds), `statecall` Flinch.
- Forced, `COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh` with the 0i_sand_hall driver
  (no `--fast`): `gunturret_force.py`, `gunturret_force2.py`, `gunturret_force_top.py`;
  `gunturret_force3.py` + `gunturret_force3.con` (`--fast`).

Merge: `python3 tools/cov/gcov_report.py build-port-cov <gcov dirs...> --module GunTurret`.
All 19 runs merged: 80/86 functions run, 91.6% lines (954/1041), 70.8% branches (442/624); no `[port] caught` or reach-stub line in any run.

### Branch table of the marked bodies

Every marked body has all lines run. Arms that only a console command or a forced write reached:

| fn | body | arm | how |
|---|---|---|---|
| fn_28_46BC | AttackExit | all | forced (`fsm <base> AttackExit`) |
| fn_28_46B8 | Flinch | body (empty) | forced (`statecall <base> Flinch`) |
| fn_28_3D58 / 2908 / 1B50 | DestroyGun, ResetAttack, LowerGun | all | forced (`kill <top>`) |
| fn_28_5FB8 / 5BE8 | WithdrawDelay, WithdrawOver | all | forced (`fsm <base> Withdraw`) |
| fn_28_616C | ShouldAttack | not alert / occluded | forced (`force_ret` mAlert = 0, mOccluded = 1) |
| fn_28_496C / 48D0 / 483C / 46F4 / 2908 | Spawn, Sleep, Patrol, Attack, ResetAttack | top lookup null | forced (`force_ret` mTopId = 0xFFFF) |
| fn_28_8BB4 / 8F90 | Top PowerDown, PowerDownOver | all | forced (`fsm <top> PowerDown`) |
| fn_28_8420 / 823C | Top Death, Revive | all | forced (`kill <top>`) |
| fn_28_8420 | Top Death | base lookup null | forced (`force_ret` mBaseId = 0xFFFF) |
| fn_28_8ED4 / 8E38 | Top Sleep, Patrol | light lookup null | forced (`force_ret` mLightId = 0xFFFF) |
| fn_28_157C / 8688 | GetModelBounds (both) | all | forced (`scan`) |

Dead arm: Spawn's switch default (EStateMsg has only Activate/Update/Deactivate).

### Unmarked

- Covered lines but an arm missing: AttackExitDone (gun destroyed: `gunturret_force3.py` never got a
  hit, the FSM leaves AttackExit by another trigger), UpdateGunOrientation (top body controller frozen),
  InDetectionHeight (non-zero up/down limits never false), the Base dtor (non-deleting call), the
  Top ctor (asset-less charge effects, no additive anim), both AcceptScriptMsg, both loaders
  (null model), Top ScanVisorRender/GetScanVisorRenderBounds (null model / no base).
- Partly covered: the Base ctor, PlayerInRange, Delay, OpenDoor, CloseDoor, IntoPan, PanLeft,
  PanRight, Withdraw, Think, UpdateGunPose, FindTarget, UpdateAttack, GetGunFirePosition,
  LaunchProjectile, Base ScanVisorRender/GetScanVisorRenderBounds, Top Think.
- Never run: DoUserAnimEvent (no user events reach it), ShouldPan, both Attacked triggers, Top
  AnimOver, GetRenderAlpha; the SLdr ctors and `__sinit` tables (not in the coverage scope).
