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

### Remaining QoL (user 2026-10-25: all of it, except the not-portable list)

Each needs MP2 game-side hooks found fresh; a row with no possible MP2 backend is reported, not stubbed.

14. **ptr2 pointer menus, rest**: front end (title/file select, pointer -> CScriptGuiMenu), CAutoMapper
    map gestures (drag, pinch, tap), quit screen.
15. **game1 Game rows**: scan speed, elevator ride skip/speed, on-screen IGT, language, restart to
    menu, Original experience toggle.
16. **vid1 HUD/scan**: cutscene black bars, sharp scan window, scan image spread, HUD opacity,
    custom crosshair colour, secondary crosshairs.
17. **vid2 rendering/system video**: sun and projected shadows, HD font, shader-compilation toast and
    F1-hint switches, GPU self-test / renderer / driver pickers (guarded to their platforms).
18. **sys1 System**: Discord presence, disc image check, menu sounds, memory-card import/export,
    data folder panel.
19. **ctl1 Controls**: button glyphs and prompts, swap visor buttons, tap minimap, Reset aim resets both.
20. **misc1**: throughput display and simulation rate, save-state hotkeys switch, Keep room objects,
    console `fx`, pipeline memory-pressure trim (`c401d338d`).
21. **mod2 mods follow-ups**: model reload, combined Mods page with texture packs, Open folder,
    Android mod picker.
22. **spr1 Spring Ball** (user 2026-10-25): MP1's Spring Ball option (jump in Morph Ball; Echoes has
    none natively), its binding, gyro flick and Android swipe-up triggers (`8773bfd0b`, `c35a19db4`
    area; find the rest by `git log -i --grep=spring` in MP1).

Skipped from this range: Phazon Beam cheat and stacked beam rate (MP1 items), mods page/import
fixes (wait for package 8), decomp matches.

Not portable: Remastered import/PBR/HDR, MP1 boss/enemy fixes, MP1 savestate schema (MP2's object
set differs; the approach can be reused), Archipelago, MP1 randomizer logic.

## Status

- 2026-10-23: F1a merged (13f9548dc; omitted MP1 rows: /tmp/opencode/reports/f1a.md); Aurora re-vendored from MP1 174975c2c (38e3b7dea; /tmp/opencode/reports/aur1.md). f1b merged (fd244995f; omissions: /tmp/opencode/reports/f1b.md). inp1 merged (88b066a4d; skips and the unexercised empty-lock OrbitPoint path: /tmp/opencode/reports/inp1.md). rob1 package 4 merged (aff7f7825: crash report, hang watchdog, startup log, disc refuse loop, stacked toasts; /tmp/opencode/reports/rob1.md). Sync paused here per user (2026-10-23); packages 5-8 not started. Reports /tmp/opencode/reports/<lane>.md. User: once these three finish (review, merge), start nothing else.
- 2026-10-25: inventory of MP1 through 3dde164eb added (packages 9-13). Running (Sonnet, claude-code/claude-sonnet-5-5): fu1 pkg 9 (../wt-mp2-fu1 sync/fu1, ses_e8fa4ff59ffe7yYX95SwZ3FoOL), cfg1 pkgs 5+6 (../wt-mp2-cfg1 sync/cfg1, ses_e8fa4ff57ffeN3F4J7XkaKoF9O), sav1 pkgs 12+7 (../wt-mp2-sav1 sync/sav1, ses_e8fa4ff53ffeTcwAXFvByLAvFE). Reports /tmp/opencode/reports/<lane>.md. Not started: 8, 10, 11, 13.
- 2026-10-25: cfg1 (packages 5+6) merged: Reset configuration (F1 System), `--settings-selftest` ctest, console `kCommands` table + docs/CONSOLE.md test, `title`/`window`/`status --json`. ctest -L port 12/12, settings index OK. Reset modal not clicked in a real run (unit test only); `fx` skipped (no MP2 backend). fu1, sav1 still running.
- 2026-10-25: fu1 (package 9) merged: resolution scale / dynamic resolution rework, Remap Presets button, Morph Ball binding rows, lock-on free aim (`lock_on_free_aim`, `MP2_LOCK_ON_FREE_AIM`; off-centre shots under a lock don't home). ctest: only `*_NOT_BUILT` fail; settings index 164 rows. Free aim checked in an Xvfb run; no screenshots of the new rows, no Morph Ball bomb-drop or off-lock homing run. Skipped: Android touch Morph Ball page and presets, `dynamic_res` key rename. sav1 still running.
- 2026-10-25: sav1 (packages 12+7) merged: save states restart the CScriptStreamedMusic that was playing and set a mid-cinematic shot's elapsed time back (`platform/port_savestate_room.cpp`); music restart seen in a live run, cinematic time advance unproven. Package 7 audio rows already existed. Matching build 87 files OK; ctest only `*_NOT_BUILT` fail. Packages 5, 6, 7, 9, 12 done; not started: 8, 10, 11, 13.
- 2026-10-25: user approved 8, 10, 11, 13. Running (Sonnet): fx1 pkg 11 (../wt-mp2-fx1 sync/fx1, ses_e8f834af3ffekSMKC60ciDGSxP), pip1 pkg 10 (../wt-mp2-pip1 sync/pip1, ses_e8f834af1ffeR6HpXghxNMpTLl), opt1 pkg 13 + package 8 inventory (../wt-mp2-opt1 sync/opt1, ses_e8f834ae8ffeZUC9yFQbwAVObi; /tmp/opencode/reports/pkg8_inventory.md). Package 8 lanes follow the inventory.
- 2026-10-25: pip1 (package 10) merged: failed pipeline reads unavailable, per-area pipeline use + prefetch (CGameArea::StartStreamIn, CMFGame::Draw; GUI/player/gun under GLOBAL), bounded sets, Compatibility-device hunks. Lazy cache opt-in `MP2_PIPELINE_LAZY=1` (two lazy runs clean, prefetch seen); default boot clean (2500 frames, exit 0). Skipped c401d338d memory-pressure trim (no MP2 memory-log module). Prefetch benefit unmeasured.
- 2026-10-25: opt1 (package 13) merged: `walk_speed` 50-200% (CPlayerDynamics, measured 5.92/11.84/23.69 at 50/100/200), `hint_delay` + Show current hint (`hintshow`; effect unobserved, save had no waiting hint). Package 8 inventory: /tmp/opencode/reports/pkg8_inventory.md (order: quick resume, pointer layer + pause screens, mods loader slice, update check (needs decision), suit colours, touch editor).
- 2026-10-25: Running (Sonnet): qr1 pkg 8 quick resume (ses_e8f780405ffemyYdLag7uIpAWT), ptr1 pkg 8 pointer layer + pause screens (ses_e8f780403ffeIbBYxF3pfQwqjh), fx1 pkg 11 still running. Queued: mod1 (mods loader) after a slot frees; suit colours after mods/disc reader. Update check awaits user decision; Android touch editor deferred (untestable here).
- 2026-10-25: fx1 merged (pkg 11): HUD billboard spawn spread past 4:3 (CElementGen/Electric/Swoosh, CHUDBillboardEffect), scan window + scan-point picking follow pointer aim (CPlayerOrbit, CPlayerVisor); fog depth copy already present. Build/ctest/upstream green. Unverified at runtime: no HUD billboard effect triggered; pointeraim stayed centred under Xvfb so the window shift was not seen. Skipped: ELC2, visor goo, 0.7x1.4 window, shield mask, sharp zoom (absent in MP2).
- 2026-10-25: Running (Sonnet): mod1 pkg 8 mods loader slice (ses_e8f7156cbffeqouIfGf2iuyyMh), with qr1 and ptr1.
- 2026-10-25: qr1 (package 8 item 6, quick resume) merged: snapshots + crash marker + boot prompt + `resume` console; MP1 3a4fec9cb skipped (MP2 has no spawn-layer overrides). Report /tmp/opencode/reports/qr1.md.
  Merge gates: build, ctest (only NOT_BUILT fail), settings index 168 rows, upstream build green. Unexercised: pad/mouse prompt answers (test hook used), live crashed-again prompt.
- 2026-10-25: ptr1 merged (pkg 8 item 1 a+b): pointer layer + gestures (platform/port_pointer*), GuiSys hit-test hooks, CPauseScreen pointer input; settings pointer_menus, map_swap_fingers; console `pointer`. Live: hover/click/arrow/back in pause options. Unverified: slider drag, scan/inventory rotate+zoom. Skipped: CAutoMapper map, quit screen; step (c) front end needs pointer->CScriptGuiMenu mapping. Merge conflicts with qr1 (CMake tests, mouse menu gate) resolved; combined boot reached gameplay at 4.1 s, rc 0.
- 2026-10-25: mod1 merged (pkg 8 item 3, mods loader slice): <user>/mods/<mod>/ (or MP2_MODS) loose <8hex>.<TYPE> resource overrides patched into PAKs and whole disc-file overrides via Aurora DVD overlays; Game tab Mods list, keys mods/mods_disabled; docs/MODS.md. Live: STRG override shown and reverted when disabled. Unrun: whole disc-file replacement end to end. Skipped: model reload, combined Mods page, texture pipelines, Open-folder, Android picker. Merge gates green, boot to gameplay 4.1 s. Next: suit colours (needs this loader); update check awaits user decision; Android touch editor deferred.
- 2026-10-25: Running (Sonnet): col1 pkg 8 suit colours (ses_e8f5d39c0ffe93nR3w7nx5AdwL), Echoes asset tables researched fresh.
- 2026-10-25: user: no update checker for now (dropped). Running (Sonnet): tch1 pkg 8 Android touch layout editor (ses_e8f53a168ffegqE1U3P55o8EDg): APK build + own emulator AVD if feasible; with col1.
- 2026-10-25: col1 merged (pkg 8 item 2, suit colours): HSV remap + CMPR codec, Echoes texture ids (Varia 2 slots, Dark, Light; ball/suit textures), generated TXTRs through the mods loader; keys suit_colour_varia/dark/light, Video > Display. Live: morph ball recoloured; defaults generate nothing. Unshown: Dark/Light screenshots; Samus body/cutscene meshes not located (keep colours); restart to apply. Merge gates green (ctest incl. settings index), boot to gameplay 4.2 s.
- 2026-10-25: tch1 merged (Android touch layout editor: TouchControlsView, 128-bit touch state, ball page/custom buttons, JNI, nod stub). Gates PASS (link, ctest 34/45 = all but NOT_BUILT, settings index, upstream, desktop boot). APK builds; editor unverified on device: needs a real disc + real nod build (-PandroidNodStub=false).
- 2026-10-25: user: port all remaining QoL (packages 14-21) plus Spring Ball (22). Two Sonnet slots are taken by the decomp session, so lanes are started one Sonnet + one worker at a time.
- 2026-10-25: Running: ptr2 pkg 14 (Sonnet, ../wt-mp2-ptr2, ses_e8d5ac8a9ffe8VnkAry5PfRI6J), sys1 pkg 18 (worker, ../wt-mp2-sys1, ses_e8d5ac8a8ffemNb9r4kX0lOpUo). Queued: spr1, game1, vid1, vid2, ctl1, misc1, mod2.
- 2026-10-25: sys1 merged (pkg 18): Discord presence (off by default, local IPC), disc image check (background, G2ME01 chunk table), menu sounds (CSfxManager hook), card import/export + data folder panel. Gates PASS (link, ctest, settings index 185 rows, upstream, boot). Unrun: import/export dialogs, real Discord client, menu-sound audio. Skipped: raw/Trilogy card formats, Android folder move. Worker lane was provider-blocked twice; redone on Sonnet.
- 2026-10-25: Running: spr1 pkg 22 Spring Ball (Sonnet, ../wt-mp2-spr1, ses_e8d47e64affeYa2mt6T0WLrYX1), ptr2. Queued: game1, vid1, vid2, ctl1, misc1, mod2.
- 2026-10-25 spr1 merged (pkg 22): Spring Ball action bound to shift chords in ball context, routed to the shared request; gyro flick (spring_ball_flick, spring_ball_flick_rate); request cleared when it springs. Gates green (ctest 34/45 as expected, settings 187/100, upstream OK, boot reached gameplay). Live: spring raises z -48.33→-44.95, off has no effect. Unrun: binding/chords, flick (no gyro), F1 rows.
- 2026-10-25: Running: game1 pkg 15 (Sonnet, ../wt-mp2-game1, ses_e8d3b53c1ffeHKlAoUNpd2iHHq), ptr2 rework. Queued: vid1, vid2, ctl1, misc1, mod2.
