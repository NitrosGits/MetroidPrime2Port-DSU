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

Not portable: Remastered import/PBR/HDR, MP1 boss/enemy fixes, MP1 savestate schema (MP2's object
set differs; the approach can be reused), Archipelago, MP1 randomizer logic.

## Status

- 2026-10-23: F1a merged into port (13f9548dc): MP1 debug_ui layout in `platform/debug_ui/`; omitted MP1 rows listed in /tmp/opencode/reports/f1a.md. aur1 (Aurora resync, ../wt-mp2-aur1, sync/aur1) running. inp1 (input sync + Controls rows, ../wt-mp2-inp1, sync/inp1) running. Next: F1b (Video rows) after aur1.
