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
| A3-2 | AnimData POI | src/MetroidPrime/CAnimData.cpp:920 | host-safety | crash | suspected: `random.Next() % range`, range = POI value from the CHAR/ANIM int32 POI (kPT_RandRate, disc data, :917); range 0 = SIGFPE. Needs a disc scan of kPT_RandRate values to confirm any 0 exists. | open (suspected; PPC divide-by-zero is silent, x86 traps) |
| A3-3 | ChozoGhost | src/MetroidPrime/Enemies/CChozoGhost.cpp:839 | host-safety | crash | suspected: `Next() % chance.GetNumBolts()` (uint mNumBolts, CChozoGhost.hpp:48, from script/param data); a chance row with 0 bolts = SIGFPE. | open (suspected; PPC divide-by-zero is silent, x86 traps) |
| A3-4 | Splinter | src/MetroidPrime/Enemies/CSplinter.cpp:2254,2515 | host-safety | crash | suspected: `% (mUnknown9f4 + 1 - mUnknown9f0)` (ctor params i1,i2, :288-289) is 0 when f4 == f0-1; script-fed. Low likelihood. | open (suspected; PPC divide-by-zero is silent, x86 traps) |
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

## Sweep log

| sweep | lane report | base | date | result |
|-------|-------------|------|------|--------|
| 3 host safety | audit-host-safety.md | eb2c98828 | 2026-10-28 | 8 suspected (5 crash-if-data, 3 minor); cats 1-4 clean; cat 5 ~45/154 checked; cat 6 (990, regex too broad) and src/Dolphin, Runtime, MetroTRK unreviewed |
| 2 frame rate | audit-frame-rate.md | eb2c98828 | 2026-10-28 | tick/frame split in report; 1 gameplay, 5 minor/visual; follow-up: A2-7/8; movie, env FX, swoosh, decal, morph ball, gun, overlays harmless; CSlideShow mAlpha and port_savestate sPromptTicks not traced |
