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

## Sweep log

| sweep | lane report | base | date | result |
|-------|-------------|------|------|--------|
| 3 host safety | audit-host-safety.md | eb2c98828 | 2026-10-28 | 8 suspected (5 crash-if-data, 3 minor); cats 1-4 clean; cat 5 ~45/154 checked; cat 6 (990, regex too broad) and src/Dolphin, Runtime, MetroTRK unreviewed |
