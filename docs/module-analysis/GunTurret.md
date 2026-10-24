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

### Historical gt1 unmarked inventory (superseded by later checkpoints)

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

## gb6 checkpoint (2026-10-24)

Base `31c12b0ba`; actual source markers, not the stale TSV column: **73 → 81/89**.
Eight new markers; eight bodies still unmarked. Evidence root:
`/tmp/opencode/reports/gb6-evidence/gunturret/`.

### Constructor correction

Standalone loader-data constructors must initialize absent sounds to -1, not 0.
Retail base `13E0 li r0,-1` feeds the thirteen sound stores `13F4..1424`
(guest offsets `b4..e4`); top `7FEC li r0,-1` feeds `7FF8/7FFC`
(guest `58/5c`). The pre-existing loader repair helpers masked this mismatch for
normal loads; retain them, but also set the constructors' sound defaults under
`TARGET_PC`. All non-PC code remains unchanged apart from marker comments.

### New marker / arm table

| fn | verification |
|---|---|
| 360 / 1C38 / 7B54 | Fresh objdiff 100% for all three; forced getter oracle asserts true / address of the base collision primitive / true. |
| 1344 | Fresh objdiff 100%; real base data destructor called three times, all 5/5 host blocks run. |
| 13C0 / 7FB8 | Forced heap construction, field/default assertions, destruction and free; both default oracles PASS. All executable header lines run; every zero-count branch has gcov `throw=true` (host EH cleanup, absent from the retail bodies), not an untaken gameplay arm. Constructors each called three times. |
| 3520 | Forced absent top, frozen top, aim on/off, first shot on/off, low/high step, targets above/below and left/right; real segment 6 and live top 0424. **42/42 lines, 32/32 edges ALL-ARMS**. |
| 18EC | Forced scanning/non-scanning, destroyed/intact, absent/live top, Attack/AttackExit/Patrol/IntoPan/PanLeft/PanRight/Spawn/Withdraw/default, both raise thresholds. **12/12 lines, 17/17 edges ALL-ARMS**. |

Run: `BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py SECS=300
tools/cov/force.sh tools/cov/specs/gb6_gunturret_arms.py <out>
42b935e4,0i_sand_hall --con tools/drivers/gb6_gunturret.con --fast --frames 1400`.
Coverage-only `gb6_gunturret_keep_inline.cmake` retains callable inline constructors;
ordinary host/matching compiler flags are unchanged. Post-marker run explicitly exited
normally, with all three oracle logs PASS, zero failed calls/assignments/Python errors,
zero `[port] caught` / reach-stub. Raw module gcno/gcda and gcov JSON retained in
`raw-coverage/`; `header-coverage.txt` records the exception-edge audit.

### Static audit and gates

Reviewed full retail listings for all eight markers. Constructor objdiff percentages
(46.223% / 64.912%) are not claimed matched; their PC overrides are retail-backed and
tested. Runtime bodies are 99.443% / 98.549%; both tested all arms.
`asmcheck-runtime.txt` findings are name/dataflow limitations, not proposed changes:
`xfc03_` is guest `8c8`, loaded at `3878` and compared at `3884/3898`;
`3778` compares computed pitch with zero, not member `x8ac_`.
Quaternion calls `38E0/38EC/38F8` and multiplies `3908/3918` are exactly
`YXZRotation(0,pitch,yaw)` (header expands `Z * X * Y`). Scan bounds' string
cleanup at `19FC` belongs to the temporary locator name; const top casting at
`1A70` is equivalent to the retail cast; `CAABox::Include` expands to the two
`AccumulateBounds` calls at `1AF4/1B00`. This also resolves the two rotation-call
findings in `retail-runtime.txt`.

Host and coverage links: 0 FAILED; manifest check rc0; upstream configure/build
and shasum: **87 files OK**. TARGET_PC strip-diff clean (comments excluded),
`git diff --check` clean. No wider family sweep: only changed bodies and their
callers' area were run, per lane brief.

### Remaining eight

`4B0` LoadGunTurretBase, `1E14` LaunchProjectile, `3130` FindTarget,
`5C3C` PlayerInRange, `6518` base constructor, `76D8` base static tables,
`7B98` LoadGunTurretTop, `9AA4` top static tables. Full retail listings are
retained as `asm-<offset>.txt`. Static initializers match 100% but are not marked
in this checkpoint; the other six still require targeted arm verification.
