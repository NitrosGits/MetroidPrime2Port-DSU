# MP1 port sync plan (2026-10-23)

What `../MetroidPrimePort` (MP1, 4,529 commits, HEAD `174975c2c`) has gained that this tree can take.
The owner of MP1 is the owner of this port: its code may be copied verbatim, then adapted (TARGET_PC
rules still apply to upstream files). Feature inventory against MP1 is `docs/QOL.md`; this file is
the work list.

## How this was measured

- Aurora: our `extern/aurora` was vendored from MP1 near `2b2dadac8` (2026-09-29; 26 blob
  differences, the closest MP1 tree). MP1 has 353 aurora-touching commits after it; none were
  taken by subject. About 200 are not Remastered/PBR. List: `git -C ../MetroidPrimePort log
  --no-merges 2b2dadac8..HEAD -- extern/aurora`.
- F1 menu: MP2 already ported MP1's frame (page window, theme, sub-tabs, search, pad navigation;
  `0ca0a556b`, 2026-10-18) inside `platform/port_overlay*.cpp`. MP1's menu is
  `platform/debug_ui/*.cpp` (26 files, 12.5k lines). Settings index: MP1 351 rows, MP2 83.
- Platform: ~360 MP1 commits under `platform/`, `android/`, `tools/` since 2026-09-29 in
  portable areas (input, settings, debug_ui, console, savestate, android, log, disc, audio).

## Work packages, in order

1. **F1 menu = MP1's `platform/debug_ui/`** (the request). Copy the directory and
   `platform/port_settings_index.inc` + `tools/check_settings_index.py`; replace the drawing in
   `platform/port_overlay*.cpp`/`port_ui_common.cpp` with it, keeping our settings storage and
   getters (`platform/include/port_overlay.h`) as the backend. Keep MP2-only pages (Cheats, Save
   states); drop pages with no backend here (Remastered, Archipelago, Tracker, Mods until 8);
   a row whose feature MP2 lacks is left out, not stubbed, and listed in the report.
   - F1a: frame + Game, System, Debug, Cheats, Save states, Randomizer.
   - F1b (after 2 and 3): Controls and Video sub-tabs with MP1's full rows.
2. **Aurora resync to MP1 HEAD**, then re-apply our additions (GOTCHAS "Vendored-Aurora additions").
   Fixes: GXCopyTex partial clears, real GXSetDstAlpha, EFB alpha by pixel format, missing colour
   0/1 read as white, TEV packed alpha compare, GX_TG_COLOR/TEXCOORDn texgen, LOD bias and
   minification, GXGetTexObjLODAll, GXSetScissorBoxOffset, SET_REG_FIELD, imgui upload race,
   scissor rounding, overflowing draw dropped instead of abort, missing UV set as zero, THP bounds,
   DVD short-read retry. Perf/memory: pipeline thread pool + idle eviction, GPU-resident vertex
   arrays/display lists, growable frame staging, texture bind-group memo, damaged cache rebuild.
   Features: dynamic resolution, GPU self-test, driver workarounds, low-power adapter.
3. **Input engine sync** (`platform/input/`, gyro via GamepadMotionHelpers): manual/auto gyro
   calibration, smoothing/tightening, gyro spaces and modes; separate vertical sensitivity (mouse,
   touch, gyro); Quake/Source mouse scale and DPI; pointer aim; turbo; Nintendo pads by label;
   port 0 to a pad connected mid-game; lock-on toggle fixes; menu/map rebindable buttons.
4. **Robustness and logging**: hang watchdog (phase, stack), crash report into the log,
   exceptions out of the main loop reported, settings/GPU caps/storage logged at start, console
   and log never block on a stalled reader, disc: refuse an incomplete/wrong image and ask again,
   shader-compilation toast, "Press F1" launch hint, stacked toasts.
5. **Settings plumbing**: one key table with a round-trip test, Reset configuration (F1 System),
   every ini key documented and checked.
6. **Console**: dispatch table, documented `help` checked by a test; `title`, `window`, `kill`,
   `fx`, `status --json`.
7. **Audio**: music/SFX volume and sound mode in F1 Game (if not already wired), music keeps
   playing through a state load, mutes not saved.
8. Later, bigger: pointer menus (mouse/touch in the front end and pause screens; game GUI code),
   suit colours, mods/texture-pack loader, update check, Android touch layout editor additions,
   quick resume.

### MP1 after 174975c2c (to `3dde164eb`, 2026-10-25, 172 commits)

About 120 are Remastered/HDR, Dread Samus import or third-person camera (not portable). The rest:

9. **Follow-ups to merged packages** (small, low risk): dynamic resolution UI rework and review
   fixes (`5cf4ac1f4`, `429fdb7bf`, `278f7923c`, issue #186); Remap Presets button (`ffaa3d4ab`,
   #196); Morph Ball binding rows split from gameplay, touch bindings only where they work
   (`7d8840bb4`, `7beb75226`, `3d1f7cbf6`, `9c2baa5e4`, `4b04a4a61`, #167) and the Android Morph
   Ball touch page (`2034083d7`); lock-on free aim with and without pointer aim (`83ff2ecba`,
   `1881b35d4`, `1b414b923`, #188; extends inp1's empty-lock path).
10. **Aurora/pipelines**: failed pipeline reads as unavailable (`8e81db43e`); lazy pipelines kept
    until memory runs short (`c401d338d`); per-context pipeline use recorded and an area's pipelines
    prefetched as it streams (`98438c0c6`, `02145b77b`; MP2's CGameArea/CStateManager hooks in
    TARGET_PC blocks); Compatibility devices under validation (`e452fc3a1`, only the non-HDR parts).
11. **Game-side fixes, adapted to MP2 code**: fog volumes copy depth, not alpha (`184b9965e`,
    CCubeRenderer, #191); HUD billboard effects spread past 4:3 instead of stretching (`a7edba309`,
    `239f95483`, `91b5c12f5`, `c1106efab`; particle headers); scan window follows pointer aim
    (`312b693c7`, `588b0577e`, `3dc11112f`; MP2's scan visor differs, check first).
12. **Save states / audio** (merges into 7): restore the playing music (`bf86ab937`, `26d51db53`);
    a save mid-cinematic resumes it (`7478ebbbd`).
    DONE (sav1): music (CScriptStreamedMusic restart) and cinematic shot time restored in
    `platform/port_savestate_room.cpp`; no MP2 hold/CScriptMidi hook; audio rows already present.
13. **Game options**: walk speed 50-200% (`62d94159f`); hint delay and Show current hint
    (`319685eed`; only if MP2's hint system has the same hook).

Skipped from this range: Phazon Beam cheat and stacked beam rate (MP1 items), mods page/import
fixes (wait for package 8), decomp matches.

Not portable: Remastered import/PBR/HDR, MP1 boss/enemy fixes, MP1 savestate schema (MP2's object
set differs; the approach can be reused), Archipelago, MP1 randomizer logic.

## Status

- 2026-10-23: F1a merged (13f9548dc; omitted MP1 rows: /tmp/opencode/reports/f1a.md); Aurora re-vendored from MP1 174975c2c (38e3b7dea; /tmp/opencode/reports/aur1.md). f1b merged (fd244995f; omissions: /tmp/opencode/reports/f1b.md). inp1 merged (88b066a4d; skips and the unexercised empty-lock OrbitPoint path: /tmp/opencode/reports/inp1.md). rob1 package 4 merged (aff7f7825: crash report, hang watchdog, startup log, disc refuse loop, stacked toasts; /tmp/opencode/reports/rob1.md). Sync paused here per user (2026-10-23); packages 5-8 not started. Reports /tmp/opencode/reports/<lane>.md. User: once these three finish (review, merge), start nothing else.
