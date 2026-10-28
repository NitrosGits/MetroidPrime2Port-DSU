# Code audit ledger (port branch)

Plan: Project knowledge "Full code audit (port branch)". Started 2026-10-28.

Lanes write findings to `/tmp/opencode/reports/audit-<slice>.md` in the row format below; the
orchestrator verifies each
(upstream PrimeDecomp code included: it is not trusted, see Review 5b) against code evidence (pseudo-C, MP1 port code or data, disc data) and
merges it here. A run only confirms or reproduces a crash.

- **class**: `asset-name`, `frame-rate`, `host-safety`, `target-pc`, `logic`, `render`, `platform`, `upstream` (decomp code wrong vs retail asm)
- **severity**: `crash` / `gameplay` / `visual` / `minor`
- **status**: `open` / `confirmed` / `fixed <sha>` / `not-a-bug`
- Ids: `A<sweep/review>-<n>` (A1-3 = sweep 1, third finding; A5b-2 = Review 5b).

Scripts: `tools/audit/` (see its README).

## Findings

| id | area | file:line | class | severity | evidence | status |
|----|------|-----------|-------|----------|----------|--------|
| A0-1 | CommandoPirate | src/MetroidPrime/Enemies/PortCCommandoPirate.cpp | asset-name | gameplay | E-grenades launched from a misread locator (burst at the feet) | fixed 051efd4c6 |
| A0-2 | Rezbit | src/MetroidPrime/CRezbitEffect.cpp | frame-rate | gameplay | hack freeze counted per frame, too short uncapped | fixed 4cd2ec2b9 |
| A3-1 | ScriptGuiMenu | src/MetroidPrime/ScriptObjects/CScriptGuiMenu.cpp:231-233,251-253 | host-safety | crash | SELECTED (suspected): `count = mItems.size()`; with `mWrapSelection` set and a menu with no kSS_Connect connections (BuildItemList :181-195 leaves mItems empty; :120 already guards the empty case for drawing), kSM_Increment/Decrement does `% count` with count 0 = SIGFPE. Also :37 kSM_Activate indexes `mItems[mSelection]` on an empty vector. | open (suspected; PPC divide-by-zero is silent, x86 traps) |
| A3-2 | AnimData POI | src/MetroidPrime/CAnimData.cpp:920 | host-safety | crash | suspected: `random.Next() % range`, range = POI value from the CHAR/ANIM int32 POI (kPT_RandRate, disc data, :917); range 0 = SIGFPE. Needs a disc scan of kPT_RandRate values to confirm any 0 exists. | not-a-bug (disc_scan.py poi: no kPT_RandRate POI in any of 484 ANCS) |
| A3-3 | ChozoGhost | src/MetroidPrime/Enemies/CChozoGhost.cpp:839 | host-safety | crash | suspected: `Next() % chance.GetNumBolts()` (uint mNumBolts, CChozoGhost.hpp:48, from script/param data); a chance row with 0 bolts = SIGFPE. | not-a-bug for shipped data (disc_scan.py chog: 0 ChozoGhost placements; mods only) |
| A3-4 | Splinter | src/MetroidPrime/Enemies/CSplinter.cpp:2254,2515 | host-safety | crash | suspected: `% (mUnknown9f4 + 1 - mUnknown9f0)` (ctor params i1,i2, :288-289) is 0 when f4 == f0-1; script-fed. Low likelihood. | not-a-bug (disc_scan.py splinter: 61 placements, none with i2+1==i1) |
| A3-5 | SwarmBasics sfx | src/MetroidPrime/Enemies/CSwarmBasics.cpp:2027 | host-safety | crash | suspected: `% mActiveBoidIndices.size()` with no empty check before it in this sound-POI loop (:2015-2027); empty when all boids are inactive while the anim POI fires. | open (suspected; PPC divide-by-zero is silent, x86 traps) |
| A3-6 | StateMachine | src/MetroidPrime/Enemies/CStateMachine.cpp:84 | host-safety | minor | suspected: `&mStates[in.ReadInt32()]` index from the FSM asset with no bound check; :64-68 trigger name loop leaves `name` unterminated if a name fills the buffer. Shipped assets only. | open (suspected; PPC divide-by-zero is silent, x86 traps) |
| A3-7 | MotionSpline | src/Kyoto/Math/CMotionSpline.cpp:468-469 | host-safety | minor | suspected: for count==2 reads `mControlPoints[index]` using the raw `index`, not the clamped `segment` (:462); index>=2 reads past the array. | open (suspected; PPC divide-by-zero is silent, x86 traps) |
| A3-8 | SlideShow | src/MetroidPrime/CSlideShow.cpp:616,976-977 | host-safety | minor | suspected: `% mGalleries.size()` / `i % mColumns` (mColumns from `gallery.mSlides[mSlide].second` :709; Reset sets 0 :1040, so a texture drawn before assignment divides by 0). Needs the draw-order check. | open (suspected; PPC divide-by-zero is silent, x86 traps) |
| A2-1 | GunTurret | src/MetroidPrime/Enemies/CGunTurretBase.cpp:659-662 | frame-rate | gameplay | PreRender (per rendered frame) calls UpdateGunPose(mgr, 1/60): aim prediction and turn (UpdateGunOrientation turnSpeed*dt) run N times per tick uncapped; retail-faithful at 60; MP1 port updates the turret pose in Think(dt) (CScriptGunTurret.cpp:245-276) | confirmed |
| A2-2 | TargetReticles | src/MetroidPrime/CTargetReticles.cpp:1313-1315 (set :847) | frame-rate | minor | `--mNoDrawTicks` per Draw; 2-frame reticle hide shrinks uncapped | confirmed |
| A2-3 | ScriptPickup | src/MetroidPrime/ScriptObjects/CScriptPickup.cpp:117-123,194 | frame-rate | minor | mFramesSinceLastSeen counts per PreRenderAllViewports; off-screen pickup animation stops after ~1 tick instead of 5 | open |
| A2-4 | ScriptPlayerActor | src/MetroidPrime/ScriptObjects/CScriptPlayerActor.cpp:371,527-531 | frame-rate | minor | mDeallocateBackupCountdown=2 decremented per PreRender; backup model may be freed early (pop on suit swap) | open (suspected) |
| A2-5 | SplitterCommandModule | src/MetroidPrime/Enemies/PortCSplitterCommandModule.cpp:471,586-587,614,627-628 | frame-rate | visual | ++mUnknown184 and two mRandom.Float() per render frame; flicker rate scales with fps | open |
| A2-6 | camera filter / renderer noise | CTargetReticles.cpp:1453; Cameras/CCameraFilter.cpp:248,262,329,343; CCubeRenderer.cpp:2984,3788,4509 | frame-rate | visual | per-frame rand() noise; rate scales with fps | open |
| A2-7 | audio | src/MetroidPrime/main.cpp:907-908,1107 | frame-rate | visual (audio) | CSfxManager::Update(1/60) and CStreamAudioManager::Update(1/60) run once per loop, outside UpdateTicks: uncapped, sfx timers, pitch bends, stream fades and music ramps run loop-fps/60 too fast; with MP2_TICKS_PER_FRAME=k, k times too slow. Fix: dt = ticks this loop / 60 | confirmed |
| A2-8 | touch turbo | platform/port_touch.cpp:440-455,540 | frame-rate | minor | turbo A toggles every kTurboPeriod loop iterations, not ticks; rate drifts uncapped and can outpace per-tick input | open |
| A3-9 | AnimSource | src/Kyoto/Animation/CAnimSource.cpp:38-66 | host-safety | crash | suspected: rotations/offsets size / numFrames; numFrames 0 for an empty (missing-pak) ANIM = SIGFPE | open (suspected) |
| A3-10 | particle UV | src/Kyoto/Particles/CUVElement.cpp:45-46 | host-safety | crash | suspected: width / mStrideW, height / mStrideH from particle element data; 0 stride = SIGFPE | open (suspected) |
| A3-11 | GuiTableGroup | src/GuiSys/CGuiTableGroup.cpp:103-135 | host-safety | minor | suspected: % mElementCount with no workers | open (suspected) |
| A5b-1 | Collision wall push | src/MetroidPrime/CGameCollision.cpp:1192 | upstream | gameplay | **Confirmed code difference.** PushActorAwayFromWalls snapshots the initial cache-overflow bit in retail, but C++ re-reads the mutable flag each iteration. Retail 0x801236A4 `lbz r4,0x1ce4(r1)`; 0x801236B0 `extrwi r29,r4,1,24`; loop 0x80123824 `cmplwi r29,0`; 0x80123828 `beq 0x80123914`; 0x80123830 `bl ClearCache`. r29 is not refreshed in the loop. Source `if (cache.HasCacheOverflowed())` instead observes the just-rebuilt narrow cache; ClearCache resets mLeafOverflow (src/WorldFormat/CMetroidAreaCollider.cpp:1155–1159). After an initially overflowing broad query and a successful first narrow rebuild, later directions stop rebuilding and can use an incomplete query cache. Concrete collision correction differs; no runtime occurrence reproduced. | confirmed (orchestrator re-read the asm: r29 set once at 0x801236B0 after the first BuildAreaCollisionCache, never in the loop; lbl_8041B76C = .float 0.3) |
| A5b-2 | Physics broadphase margins | src/MetroidPrime/CPhysicsActor.cpp:354–356 | upstream | minor | **Confirmed wrong constants.** GetMotionVolume uses `(0.5,0.5,up+1.0)` and `(0.5,0.5,down+1.5)`; retail expands both ends by `(0.3,0.3,step+0.3)`. At 0x800E9F5C and 0x800E9FBC, `lfs f3,lbl_8041B76C@sda21(r0)` loads 0x3E99999A = 0.3f; 0x800E9F6C/0x800E9FCC `fadds f1,f3,f0` forms step+0.3; 0x800E9F78–0x800E9F80 adds the same f3 to max X/Y, and 0x800E9FD8–0x800E9FE0 subtracts it from min X/Y. Both PC/non-PC paths use the C++ constants with no replacement. Returned broadphase bounds are unnecessarily enlarged and can add candidate/cache work; actual gameplay impact or candidate-cap overflow is unproven, hence minor rather than an asserted collision failure. | confirmed (orchestrator re-read the asm: r29 set once at 0x801236B0 after the first BuildAreaCollisionCache, never in the loop; lbl_8041B76C = .float 0.3) |
| A1-1 | Metroid | src/MetroidPrime/Enemies/CMetroid.cpp:43-47,1118 | asset-name | minor | Pelvis, L_/R_varias2_SDK in no CINF; lookup gives identity | not-a-bug (strings are in retail Metroid.rel: retail behaviour) |
| A1-2 | DarkSamus | src/MetroidPrime/Enemies/PortCDarkSamusFsm.cpp:1944 | asset-name | visual | effect BoostThingy in no effect table; silent no-op | not-a-bug (string is in retail DarkSamus.rel at the cited call) |
| A1-3 | SpankWeed | src/MetroidPrime/Enemies/CSpankWeed.cpp:125 | asset-name | visual | Arm_11/Arm_12/Arm_end, no disc placement to attribute | not-a-bug (strings in retail SpankWeed.rel) |
| A1-4 | EmperorIng Stage1 | PortCEmperorIngStage1*.cpp (lookups on heart/target actors) | asset-name | minor | brainSpine_scale, sphere_base, Damage, Telegraph checked only disc-wide | not-a-bug (strings in retail EmperorIngStage1.rel) |
| A1-18 | AutoMapper | src/MetroidPrime/CAutoMapper.cpp:1892 | asset-name | visual | `GetString("Teleport_Destination")`; retail main.dol has `Teleport Destination` (space) next to `AutoMapperPaneMode`; no underscore form in retail. Miss gives skInvalidString in the teleporter pane. Upstream typo | confirmed |
| A1-26 | RadialHud | src/MetroidPrime/HUD/PortRadialHud.cpp:72 | asset-name | minor | FindWidget(`model_inner_ring`): not in DOL (widget names live in the FRME); needs a check against the Trilogy FRME | open (unverified) |
| A5b-3 | Splinter | src/MetroidPrime/Enemies/CSplinter.cpp:2169 | upstream | minor | CanSeePlayer Range upper bound 0.08f (0x3DA3D70A); retail rodata_F0 = 0x3DA3D70B (fn_74_8E64 0x8F34 lfs), one ULP off | confirmed (no visible effect) |

## Sweep log

| sweep | lane report | base | date | result |
|-------|-------------|------|------|--------|
| 3 host safety | audit-host-safety.md | eb2c98828 | 2026-10-28 | 8 suspected (5 crash-if-data, 3 minor); cats 1-4 clean; cat 5 ~45/154 checked; cat 6 (990, regex too broad) and src/Dolphin, Runtime, MetroTRK unreviewed |
| 2 frame rate | audit-frame-rate.md | eb2c98828 | 2026-10-28 | tick/frame split in report; 1 gameplay, 5 minor/visual; follow-up: A2-7/8; movie, env FX, swoosh, decal, morph ball, gun, overlays harmless; CSlideShow mAlpha and port_savestate sPromptTicks not traced |
| 3 follow-up | audit-host-safety.md (Follow-up) | bf743331d | 2026-10-28 | A3-2..4 cleared by disc scans; cat 5 done (A3-9..11); cat 6 narrowed to 10 hits, none new |
| 5b phase 1 | audit-upstream-units.md (+ -inventory.md, .json) | eb2c98828 | 2026-10-28 | 525 unlinked units, 502 flagged defs + 80 annotations in matching units (review queue); 10 DOL units compared, 2 confirmed; no REL units yet |
| 5b phase 2 | audit-upstream-units.md (Phase 2) | eb2c98828 | 2026-10-28 | 10 units (DOL: CCollidableAABox, CCollidableSphere, COBBox, CSpatialPrimitive, CollisionUtil; REL: AIMannedTurret, AtomicAlpha, AtomicBeta, BacteriaSwarm, Blogg): no findings; selected-body triage; REL linker helper failed, asm read from the matching build |
| 1 asset names | audit-asset-names.md | eb2c98828 | 2026-10-28 | 667 lookups: 631 hit, 41 miss (all retail strings), 346 unchecked (runtime names, unplaced modules); sound/particle-resource lookups not scanned |
| 1 follow-up | audit-asset-names.md (Follow-up) | e4ac24546 | 2026-10-28 | `asset_names.py --rel`: every literal vs its module's retail REL/main.dol strings: 4946 checked, 3854 retail, 1066 port-only (34 Trilogy), 26 triaged → A1-18, A1-26; FRME widget and STRG names not covered |
| 5b phase 3 | audit-upstream-units.md (Phase 3) | eb2c98828 | 2026-10-28 | 15 tier-(b) defs in matching units (none skip retail behaviour) + REL EmperorIngStage2Tentacle, SwarmBasics, FlyerSwarm, MysteryFlyer, DarkCommando, Splinter: A5b-3 minor. Plan-order bosses are Port*.cpp (Review 5). link_rel.sh works with its default MP2_ARCHIVE (archive/decomp build); it fails only when pointed at upstream's build/G2ME01/main.elf (lane set MP2_ARCHIVE=$PWD) |
