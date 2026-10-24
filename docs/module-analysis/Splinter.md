# Splinter REL (module 74): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CSplinter.cpp` (CSplinter, its
SSurfaceAlignment/SJumpData/SSacData/SSpitData/SParticleData helpers, and CSplinterAcidSac) plus the
`SLdrSplinter.hpp` loader. It covers the Splinter, the Dark Splinter, the Worker Splinter and the
Ing-possessed Mega Splinter. The port's fixes are `TARGET_PC` blocks in those two files. Retail asm:
`build/G2ME01/Splinter/asm/MetroidPrime/Enemies/CSplinter.s`.

## Behaviour fixes (splin1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_74_C3C SLdrSplinter ctor | 0xCB4 `li r0,3` -> +0x38; 0xD04 rodata_54 (2.0) -> +0x88 knock-back resistance; 0xD2C echo emitter 1; 0xD44 ambient CColor(1,1,1,1); 0xD54 visor flags 0xf | 0 / false / black |
| fn_74_BE3C SSurfaceAlignment ctor | 0xBE90-0xBE98 `li r3,-1; li r5,0; li r0,2`: include all, exclude ceiling+wall, kFT_Exclude | `MakeInclude(ceiling, wall)` |
| fn_74_1600 DoUserAnimEvent, kUE_Unknown37/38/39 (Mega spit end, orbit on/off) | 0x177C/0x1790, 0x17A4/0x17EC, 0x17FC: every exit goes to 0x1824, the `CPatterned::DoUserAnimEvent` call | `return` (base handler skipped) |
| fn_74_A928 ApplyAmbushVelocity, apex lift | 0xA9EC f3 = rodata_60 (0.0); `fcmpu; beq` -> rodata_8C (0.8); 0xAA28 `fadds f28,f3,f0`: apex = (h == 0 ? 0.8 : 0) + max(z, start.z). A NaN height takes 0.0 | apex = (h ? h : 0.8) + max |

kUE_Landing (0x1648 `beq 0x183C`) returns before the base call in retail too.

## Survey (splin1)

Objdiff of all 33 functions below 100%. Apart from the fixes above, the differences are regalloc,
scheduling, block order, string/rodata addressing, and inlined vs REL-local template copies
(the unnamed fn_74_* rows of the table). Checked and equivalent:
- LoadSplinter: block order only.
- The CSplinter ctor: retail preloads the stack arguments into registers; the same values reach the
  same members.
- `__sinit`: the trigger (46) and state (27) tables. Each entry pairs a name with a virtual
  pointer-to-member; resolving the vtable slots (lbl_74_data_854) gives the same name -> function pairs
  as upstream. Retail lists InHeckleRange before IsDarkSplinter/IsHeckler and ShouldEmergeFromCocoon
  before ShouldDrop. `SetTriggerFunctions` keys them by name, so the order does not matter.
- IsNearWall (start z + rodata_F4, side offsets, ray tests), CanEvade (-0.0 z from `*-1`),
  UpdatePitchBend, JumpToPoint and AcceptScriptMsg's CBCUnknown18Cmd argument, CreateSac (scale 1.0,
  strings).
- Register/temp-only: IsGrabbingPlayer, FindGrabbed, InitPathArea, IsAttackBlocked, ExplodeSac,
  Think, Jump, AddEvadeCheck, ApplyJumpVelocity, ApplyLaunchVelocity, FindHideHint, FindJumpPoints,
  the SSacData ctor, PathFind, ScoreJump, AttackPointIsLegal.
- CSplinterAcidSac::GetTouchBounds: retail accumulates (x+0, y+0, z+30), upstream (x+0.4, y+0.4,
  z+30). The box is the same.
- CanSeePlayer: retail rodata_F0 is 0.080000006, 1 ulp above upstream's 0.08f (the random range).

asmcheck findings on the 60 marked bodies (BIT 33, CALL 21, NOPOS 19, BRANCH 9, XCALL 1) are all
tool limits. The CSplinter/SJumpData/SSacData bitfields have no stated bit positions. The CALLs go to
REL-local copies of header inlines and templates (fn_74_*), TCastToPtr, `internal_dereference` and
CMemory::Free. BRANCH counts include inline-header conditions. The XCALL is CanSeePlayer's virtual
`IsWorkerSplinter` (vtable +0x1A8).

Per-function table: `Splinter_functions.tsv`. B = upstream body, H = weak copy, template or
unnamed REL-local, T = REL runtime.

## Spawns

`objs Splinter` after `layer <n>` (save mi_u). Most splinters load inactive: `send <id> Activate`,
then `send <id> Alert`.

| area | layers | ids |
|---|---|---|
| `3bfa3eff,02_temple_exploration` | 0 / 4,6 | u055b u055c u057c (inactive, cocoon) / u05cf u05d0 u05d1 u05d3 |
| `3bfa3eff,01_temple_hive02` | 4 | u0630 u0633 u0636 (inactive, acid sacs u0649-u064b), u0584 u0591 u0594 u05ac (a19) |
| `3bfa3eff,03_temple_combattraining` | 2,3 | u0546 u055b u0561 u057c u057d u0582 |
| `3bfa3eff,04_temple_unknownenemies` | 2,3 | u0498 u0499 |
| `3bfa3eff,05_temple_federationsite` | 4 | u0573 u0576 u057b u057c |
| `3bfa3eff,0c_temple_hall` | 2,3 | u046e u048a |
| `3bfa3eff,0d_temple_hall` | 3 | u0461 u0470 u0478 u047e |
| `3bfa3eff,0e_temple_hive_hall` | 3,4 | u0487 u048a u048d u0602 u0604 |
| `42b935e4,0h_sand_hall_dark` | 0 | u043f u0444 (Dark Splinters) |
| `42b935e4,07_sand_alamoportalobservation_dark` | 4 | u05ec u05f9 u0603 u0607 u0611 |
| `863fcd72,07_temple_mothlordsanctuary` | 2 | u04d3 u04d8 u04df u04ea u04ec u04f4 |

No Mega Splinter (Ing-possessed, spit) ran in these areas. The spit arms of DoUserAnimEvent and the
SSpitData ctor are uncovered.

## Coverage

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON`. Run
`BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/splinter.py <out> <world,area> --con
tools/cov/specs/splinter_<area>_{fight,ball}.con --fast --frames 3400`, then merge with
`python3 tools/cov/gcov_report.py build-port-cov <gcov dirs...> --module Splinter`.

The `_fight` drivers wake, alert, fight, then damage (plain, knock-back, missile) and kill. The `_ball`
drivers sit in morph ball among the splinters and boost. `splinter_te.con`, `splinter_te_fight.con` and
`splinter_te_ball.con` cover 02_temple_exploration's layer-0 cocoon trio. `splinter_find*.con` are
discovery drivers. All 27 runs merged: see `gcov_report.py` for the current totals.

`splinter_cable.con` (hive_hall, layers 3 4, `--frames 1800`) drives the cable workers u048d/u048a/u0487
(the same instances as 02_temple_hive u0636/u0633/u0630, which never activate from the hive02 area) through
fsm WaitOnCable/Descend/GrabSac/Rise/Drop/DropAndExplode/Retreat/Scream. `splinter_mball.con` (same area,
`--frames 1400`) adds `statecall`/`trigcall` for GrabMBall (not a node in the FSM asset, so `fsm` is a no-op),
MBallMoved, Rise and GrabSac. `splinter_all.sh <outroot>` reruns every driver 3 at a time.

Marked in the second pass (15 bodies, 75 total): AttachMorphBall, Retreat, GrabMBall, MorphballWaiting,
CableShot, HasUnpoppedSac, GetSac, PopSac, DropAndExplode, KillSelf, HoldingSac, ReachedTop, GrabbedIt,
Scream and the CSplinterAcidSac ctor, each with all live arms run and checked against the asm. asmcheck
findings are tool limits (unported callee CALLs, the xab0_40 bitfield, inline-header BRANCH counts).

Not marked, with reason:
- MBallMoved: arms for an inactive player, a player in another area and the not-morphed case (730, 731,
  733) did not all run. The first two cannot happen in play.
- Forced arms (spl2): `tools/cov/specs/splinter_grab_force.py` with `splinter_grab.con`/`splinter_grab_mb.con`
  (`SPL_GRAB`, `SPL_TOP`, `SPL_CARRY`, `SPL_MB` env variants; `statecall` since `fsm` cannot enter GrabMBall).
  IsGrabbingPlayer, DetachMorphBall (forced (statecall Rise + SPL_GRAB, attached-actor set)) and Rise (all 22
  arms, forced (statecall Rise)) are marked; asmcheck findings are unported callees, the xab0_40 bitfield, and
  ReachedTop inlined in the asm. gcov kept in /tmp/opencode/spl2_gcov/<run>/.
- spl3: CreateSac 263C (arm `mSacAncs == -1` forced by tools/cov/specs/splinter_sac_force.py) and FindGrabbed 33C8 (invalid arm natural on the cable; live-id arm via splinter_grab_force.py SPL_GRAB=0x48a) marked, ALL-ARMS merged. The other 19 B rows of 24B8-3AC4 were already bulk-marked (objdiff 100%).
- Still not marked: ExplodeSac (mParticle.mEffect invalid in hive_hall, body past line 827 unreachable),
  GrabSac (905 Update arm untaken: HasUnpoppedSac false on those calls), MBallMoved (745:1/4 and line 749
  need a really morphed player; 9/10 arms).
- RunAndHide/FindHideHint, FacingPlayerExactly, SetUnhideTime, ReachedMBall, ProjectileInfo,
  MegaSplinterSpit (no Mega splinter), CSplinterAcidSac::HealthInfo/GetDamageVulnerability: never reached.

## spl4 (2026-10-24)

Marked from natural gcov (every arm ran, checked against asm; asmcheck BIT/NOPOS findings are the nested
`mJump` bitfield and `mIsWorker` not resolved by name, masks verified by hand): ApplyJumpVelocity A67C
(mIsHighJump = 0x40), UpdatePitchBend 9D88 (0x934 bit 0x04 = mIsWorker), Jump 9014 (0x9bc: 0x80 mLanded,
0x10 mHasTarget, 0x08 mJumping, 0x01 mHasLineOfSight).
Left unmarked (arms not run): FallFromCocoon 78BC, JumpToPoint 8A7C, IsNearWall 9344, ScoreJump 96C8,
FindJumpPoints 99BC (ScoreJump never passes its >6 return), ApplyLaunchVelocity A7C8 (needs a found jump
point), ApplyAmbushVelocity A928 (Mega+Ing-possessed arm; not dead). These need forced runs.
