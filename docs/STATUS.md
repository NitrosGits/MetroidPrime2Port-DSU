# Session status (detail)

The Status section of AGENTS.md links here: this is the per-session detail, rewritten rather than
appended, dated as in the heading there.

## 2026-10-29: 0.6.1 released

- Bumped `versionCode` 7 / `versionName` "0.6.1" and added the 0.6.1 metainfo release entry;
  artefacts are built from that commit (`ad26e59e6`) by the release workflows.
- Tag `v0.6.1` = `ad26e59e6`; GitHub pre-release
  https://github.com/Odrannnn/MetroidPrime2Port/releases/tag/v0.6.1 with the twelve assets
  (`gh release view v0.6.1 --json assets` lists them all `uploaded`). CI: linux-release
  40791251657, windows 40791254843, macos 40791258032, ios 40791260907, android 40791264448,
  all `success` on `ad26e59e6`.
- Flatpak built locally from the same commit (12,058,096 B) and installed once for
  verification: `flatpak info` reports `Version: 0.6.1`, `files/` holds only the binary, the
  debug data and `share/{licenses,metainfo,applications,icons}`, and it starts and reaches the
  disc prompt. Test install removed afterwards.
- Crash soak on the release commit: `build/s-release-2026-10-29/soak/summary.tsv`, **96/96 rows
  `ok`**, exit 0 (mods/none × validate/release × MSAA 1/4 × the 12-area tour, 117 s per
  (mode, toggles) game). Log `/tmp/opencode/soak-061.log`.
- Artefact checks from `build/release-assets-0.6.1/out`: APK `versionCode='7'
  versionName='0.6.1'` signed by `5c380a13…8bd20`; IPA and macOS `Info.plist`
  `CFBundleShortVersionString 0.6.1`; macOS executable kept `-rwxr-xr-x` in the zip; x86_64
  binary needs at most `GLIBC_2.34`; no `.dol`/`.rel`/`.pak`/disc image in the tarballs, either
  AppImage, the macOS zip, the Flatpak or the APK (its `assets/` is licences plus textures). No
  version string exists in the Linux/Windows binaries, so the #4 fix was confirmed in them by
  the new `Scan Peek Resolve` label (4 hits in each; 0 in the 0.6.0 Windows exe), and their
  provenance is the CI runs above on `ad26e59e6`.
- Remaining known gap: the Flatpak runtime `org.freedesktop.Platform` 24.08 is now reported
  end-of-life by flatpak; moving the manifest to a supported runtime is the next release task.
- Release APK built locally with the project key (signer SHA-256 still
  `5c380a13...8bd20`, so it updates over 0.6.0 without uninstalling) and installed on the
  test phone (`25102PCBEG`, Android 16) as an in-place update: versionName 0.6.1 confirmed,
  save and settings kept.
- On-device check of issue #4: the game resumed in the scan visor with `msaa=4`, read-back
  logs showed six distinct EFB scan ids (28811/59536 non-zero pixels), and a logbook scan
  completed. Screenshots `/tmp/opencode/phone-0.6.1-launch.png` (mid-scan) and
  `phone-0.6.1-scan2.png` (after).

## 2026-10-29: issue #3 emulator save framing

### Trilogy mirror integration test

Tested the actual `CSaveGameScreen::Update` mirror hook with
`emulator_folder=/tmp/opencode/issue3-mirror-{out,in}/emulator`,
`share_emulator_card=1` and `mirror_trilogy_save=1` in isolated settings files.
Native front-end runs used `MP2_SKIP_FRONTEND=0`, 5500 updates, 8 ticks/loop and
40-update A presses at updates 100, 300, 600, 1000, then every 500 through 4000.
Three-update presses can be skipped entirely at 8 ticks/loop; those initial runs
never reached the save screen and were not counted as mirror verification.

- Outbound: the actual native save-station G2ME card (0%, 2.4 seconds) was newer
  than a copy of `13-gbatemp-100.bin`. The mirror replaced Echoes file 1, made an
  exact `save.bin.bak`, and equalized the card/bin mtimes. Independent byte comparison
  found 813 changed bytes, all within the target Echoes parts, its summary and the
  global CRC. The other slots and the Prime 1/Prime 3 data were unchanged.
- Inbound: the same unmodified Trilogy fixture was newer than another copy of the
  G2ME card. The mirror updated the card, kept an exact pre-import GCI backup, left
  the NAND file unchanged and equalized mtimes. A subsequent native boot loaded
  the imported 100% / 06:13 Echoes progress into gameplay at update 273, exit 0.
- Dolphin 2512 booted the USA Trilogy disc with the actual outbound mirror output.
  Its game-selection screen showed Echoes in Temple Grounds at 0% / 00:00, alongside
  the unchanged Prime 1 and Prime 3 progress. Selecting Echoes reached first-person
  gameplay in Temple Grounds (99 energy, 5 missiles). Screenshot:
  `/tmp/opencode/issue3-trilogy-mirror-loaded.png`. The mirrored Echoes parts remained
  byte-identical through the menu. Dolphin was stopped after the successful load.

The initial emulator setup seeded only `save.bin`, omitting Wii's `banner.bin`.
Both the mirrored output **and the untouched source fixture** were then rejected as
corrupt. A clean Trilogy boot generated both files; its empty `save.bin` was exactly
`01-firstboot.bin`. Adding that game-generated banner to the isolated test NAND
resolved the rejection. It was a test-setup failure, not evidence against the mirror.
The mirror expects a complete Trilogy save directory already created by the game.

Evidence: `/tmp/opencode/issue3-mirror-{out,in}/`,
`/tmp/opencode/log_issue3-mirror-import-load.log`,
`/tmp/opencode/issue3-trilogy-games.png`. This tests USA G2ME card <-> USA Trilogy
NAND sharing, not PAL conversion.

**Trilogy-disc failure reproduced, then fixed:** created a real
native `01-R3ME-MetroidPrime2.gci` by mounting the Trilogy disc in the port, starting
a new game and saving at the station. Re-entering the native front end with sharing
and mirroring enabled logged `Trilogy mirror: import failed: there is no Echoes save
on the card to put the Trilogy file into: save once in the port first`. Both files
remained unchanged. The old `GameFilesNewest()` filtered headers by the first three bytes
`G2M`, so the existing R3ME file is ignored before conversion. The payload converter
does support R3ME/R3MP; the wrapper's file selection does not. Reproduction:
`/tmp/opencode/log_issue3-mirror-r3-enabled.log`, isolated data
`/tmp/opencode/issue3-mirror-r3/`. That test-only checkpoint was superseded by the fix below.

### R3ME mirror fix

`GameFilesNewest()` now selects the mounted release's exact four-byte game code and
maker `01`, through `port_save_mirror.h::IsCardForRelease`. Simply admitting all R3M
files would still choose the wrong layout when a newer GameCube card shares the same
region folder. Both sync direction detection and the actual import/export now use
the mounted release only. G2MP conversion remains explicitly unsupported, as before.

Verification:

- `port_save_mirror_tests` covers the full G2ME/G2MP/R3ME/R3MP selection matrix,
  wrong makers, unrelated games, malformed codes and truncated headers. It and the
  framing/conversion tests passed through CTest; the selector also passed ASan/UBSan.
- Actual front-end mirror tests on native Trilogy data passed in both directions
  under `/tmp/opencode/issue3-r3-fixed-{out,in}/`, with a **newer G2ME decoy** in the
  same card folder. The decoy stayed byte-identical; selected card/bin mtimes became
  equal. Export touched only the target Echoes slot, summary and CRC, and kept an
  exact NAND backup. Import kept an exact R3ME backup and did not change the NAND.
- The imported R3ME card loaded 100% / 06:13 progress into native gameplay at update
  273, exit 0 (`/tmp/opencode/log_issue3-r3-fixed-load.log`).
- Dolphin 2512 loaded the actual R3ME mirror export into retail Trilogy gameplay
  in Temple Grounds (99 energy, 5 missiles). The Echoes payload remained identical
  to the exported file. Screenshot: `/tmp/opencode/issue3-r3-mirror-loaded.png`;
  isolated emulator: `/tmp/opencode/issue3-r3-dolphin/`.
- G2ME mirror import and export also passed with a newer R3ME decoy left unchanged
  (`/tmp/opencode/issue3-g2-regression-{out,in}/`).
- Full native game link and source manifest check passed; the Trilogy fixture suite
  still reports 0 failures. Retail matching artifacts remain unchanged (87 hashes pass).
  PAL selection is unit-tested; no actual R3MP disc test was performed.

### GameCube framing fix

`fix/issue3-save` corrects the runtime card reader/writer, not just the import inspector.
`CCardFileInfo` now checks CRCs in console big-endian or legacy host-little-endian order,
reads the generation in the matching order, and always writes new CRC/generation words
big-endian. Payload bytes are not swapped. Generations other than 0/1 are rejected before
the slot selector uses their XOR as an index. All upstream changes are `TARGET_PC`-guarded.
Reading a valid legacy header does not rewrite it; a normal save migrates the data copies
as they are written. Retail repairs an old header's non-console CRC via its existing
header-repair path. Keep backups and never run both programs on the shared card at once.

Verification (isolated cards under `/tmp/opencode/issue3-*`; no live saves changed):

- `port_save_framing_tests` and the real-fixture `port_save_convert_tests` passed through CTest.
  Framing tests use an independent bitwise CRC/console decoder and cover both byte orders,
  both generations, unchanged payloads, legacy migration, repeated writes, CRC corruption,
  invalid generations, short buffers and a byte-palindromic CRC. ASan/UBSan passed too.
- Full native game linked. Headless native boots loaded both byte orders of the same real
  G2ME fixture into gameplay (frame 121), rather than falling back to an empty card.
- The console driver sent `station` at update 140, then wall-clock `press a` / `stick 0 0`
  at 1/1.2, 2/2.2 and 3/3.2 seconds. An actual in-game save changed both data copies;
  Python `zlib.crc32(...) ^ 0xffffffff` independently verified their big-endian CRCs.
  The existing legacy header was unchanged, as intended. A subsequent native boot
  accepted that saved card into the running world and exited normally at update 180.
- Trilogy container/payload tests passed on the local fixture set (0 failures).
- Matching build: `87 files OK`; generated source manifest check passed.
- Actual emulator test: Dolphin 2512 (`/usr/games/dolphin-emu-nogui`, JIT64/OpenGL)
  booted the owned G2ME01 disc using an isolated `-u /tmp/opencode/issue3-dolphin`
  folder and a copy of the file produced by the native save-station test above.
  Selecting Single Player loaded Temple Grounds into first-person gameplay; screenshot
  `/tmp/opencode/issue3-dolphin-loaded.png` (cropped from `screen-30.png`). Dolphin
  repaired the legacy header CRC to big-endian, but both data copies (GCI bytes
  `0x2040..end`) remained byte-identical to the port-written source. This was not a
  deleted/recreated card. The emulator ran on a dedicated Xvfb display and was stopped;
  the normal emulator user directory was not used.

Native integration, console-format validation and an actual Dolphin load passed.
Steam Deck and PrimeHack specifically remain untested. No GitHub comment, push or release was made.

## 2026-10-29: issue #4 scan readback with anti-aliasing

- `scan_peek::encode` previously dropped every multisampled EFB request. The encoder now
  supplies the original colour attachment view, and a scan-only render pass reads sample zero
  from both colour and depth into single-sample RGBA8/R32Float region targets. It does not
  average integer scan IDs or change the game's EFB. Recording preserves the source stores
  even when the following pass clears the EFB.
- Core-device 1x readback uses the same conversion: validation-enabled testing exposed an
  existing invalid partial depth texture-to-buffer copy. It also normalizes HDR colour to the
  four-byte readback layout. The Compatibility-device fallback is unchanged.
- `ScanPeekMsaaTest` renders per-sample IDs/depths and a spatial pattern on Vulkan, then checks
  actual CPU output (not just pipeline validation): 1x/4x, BGRA, HDR, clipped regions, slot reuse,
  AA toggling and attachment-store retention. The old MSAA early return was temporarily restored
  as a negative control: 4x fails while the 1x control passes. No negative-control code remains.
- Graphics suite: 112 tests pass using lavapipe with validation enabled; native game links;
  matching build: `87 files OK`. Evidence: `/tmp/opencode/issue4-all-gfx-tests.log`,
  `issue4-negative-tests.log`, `issue4-matching.log`.
- Native rendered integration check on RX 7900 XTX/RADV: Dark Trooper `e000b000c` in Temple
  Grounds area 11 completed its 1.50-second scan at 4x, after combat/scan visor re-entry, at
  1x, and after switching back to 4x. Each check reacquired with ordinary held L, not
  `PortForceOrbit`. The initial console `scan` pair was only camera positioning. The harness
  boot was stuck morphed; a temporary console setup forced `SetMorphBallState(Unmorphed, 0)`
  and first-person camera once before positioning. That setup was removed and the final native
  binary rebuilt. Logs: `/tmp/opencode/issue4-game4.log`, `issue4-game-results4.log`; screenshots:
  `/tmp/opencode/issue4-scan-aa-{1,2}.png`. No Windows, Android or reporter-specific driver test.

## 2026-10-29: scan visor widescreen registration and model proportions

- Reproduced the reported detached background with a completed Dark Trooper logbook scan
  (`e000b000c`, Temple Grounds area 11), at a 2000x781 window and `scanspread 100`.
  Generic non-logbook scans do not exercise the hierarchy panels.
- `basewidget_crazyleft/right` contain only the background; the six category rows are sibling
  trees. Register both the background and rows for the live scan-spread weight, and give every
  piece of each side the same label anchor. The BWIG roots themselves are at the origin and
  cannot serve as side anchors. `CGuiFrame::SetSpreadAnchorTree` now accepts an optional anchor;
  existing one-argument callers retain their original behavior.
- The model preview bypassed `CGuiCamera::Draw` and used its authored aspect unchanged. Apply
  the GUI's aspect widening, fit-width FOV and HUD scaling to its custom projection too: wide
  windows no longer flatten the scanned model.
- Verified on Linux/Xvfb with an actual completed logbook entry: spread 0/50/100 at 2000x781,
  plus 1280x720, 1680x720, 960x720, 720x960, and HUD scale 70%. Evidence is under
  `/tmp/opencode/scanfix/` (`logbook-before-100.png`, `logbook-before-0.png`, `final-0.png`
  through `final-8.png`, `checked-matrix.png`). Test-only forced unmorph setup and widget
  probes were removed; neither is part of the fix. Native game links; matching build passes
  all 87 checks. Windows/Android device rendering has not been rerun.

## 2026-10-28: issue #2 controller bindings

The twin-stick-only camera path omitted `PortMouse::StickEnabled()` when applying aim
pitch in `CFirstPersonCamera::UpdateTransform`. Include it alongside mouse and touch:
the player already consumes both stick axes and turns the body for yaw, but native
free-look is suppressed in twin-stick mode, leaving no path to apply pitch otherwise.
Enabling mouse aim alongside twin-stick was a code-derived workaround, not a device-tested one.

The menu and trigger reports are fixed too:

- Separate menu buttons: `platform/input/port_input_devices.cpp::MenuButtonsActive`
  previously required `!PortTouch::ControlsEnabled()`. `platform/port_touch.cpp::Settings::controls`
  defaults to true on desktop too (only `aim` has a mobile-specific default). Therefore
  `ApplyMenuButtons` removed all Menu actions despite the separate-menu setting being on.
  It now uses last-used touch activity, as MP1 does: `NoteDevice` activates touch,
  real controller/keyboard/mouse input deactivates it; virtual pads, stick drift and
  synthetic touch/pen mouse events do not. Builds without touch always report inactive.
  The context path recognizes front-end and paused menus; map and scan-text contexts need a separate
  completeness review.
- Trigger capture: Aurora's `PADGetRawState` reports an analog trigger axis and synthetic
  half-pull/click buttons. The recorder previously enumerated both the buttons and
  axes, retaining every distinct input until release (`UpdateCapture`).
  Thus one right-trigger pull recorded `RT + RT click + RT`, exactly as reported.
  Capture now skips the four synthetic trigger-button aliases and records the positive
  analog axis alone. Runtime pull/click support and manual bindings are unchanged;
  chords involving distinct controls still record normally.

These findings come from source tracing against v0.6.0 and the port branch, not a Steam Deck run.
Verification: game linked; `port_radial_mouse_tests`, `port_input_engine_tests` and
`port_input_bindings_tests` passed (these cover the aim/input machinery, not the camera
integration); matching build reported `87 files OK`; generated source manifest check passed.
`port_controller_regressions` additionally passes touch/physical transitions, desktop/F1 gates,
virtual-pad/drift/synthetic-mouse filtering, full-trigger capture across thresholds, both
triggers together, shoulder+trigger chords, ordinary stick axes and keyboard/mouse inputs.

## 2026-10-29: issue #2 diagonal movement/jumps

Input normalization did not fix the force law: each horizontal axis independently added
the retail friction-compensation offset and independently saturated its force. A diagonal
could therefore receive two full forces even with unit-length input. Direct-aim movement
now evaluates the forward law once along the input direction, with planar damping/speed
limiting instead of component damping and a forward-only cap. Strafe gets the same 0.8
full-travel scaling as forward. Jump and double-jump acceleration use the same planar
movement magnitude rather than only the forward/back component. These changes are PC-only
and confined to unlocked direct aim; native controls and locked-on dash code are unchanged.

Verified: game linked; four targeted input/aim tests passed; matching build `87 files OK`;
generated source manifest check passed. The mouse test reproduces the old diagonal force
boost and checks the production force/damping helpers across sixteen headings, three stick
magnitudes, ground friction and air drag over 240 ticks each.

Headless native-game check: `tools/drivers/direct_movement.con` with
`tools/drivers/direct_movement.input`, a fresh user directory, `MP2_NEW_GAME=1`,
`WORLD=0b_sand_hall`, `MP2_MOUSE_AIM=1`, `MP2_FRAME_LIMIT=0`,
`MP2_TICKS_PER_FRAME=1`, `FRAMES=740` (RUNNING.md's console driver recipe).
The fixture changes facing so all three input directions jump along the same world-space
path from the same grounded start. At +15 ticks the horizontal distances were 1.001,
1.001 and 1.003 units, with the same sampled height (21.041, from 19.399); at +30,
1.423, 1.423 and 1.427 units. The diagonal boost is gone. Later samples hit corridor
geometry, so they are not an unobstructed landing-range measurement. An initial attempt
teleported above the room and never actually jumped; that run is not jump verification.
Device-specific Steam Deck and Android testing remains unperformed. Nothing posted on GitHub.

## 2026-10-26 (port orchestrator): widescreen gaps, ball glow, suit colours, FOV, scan copies

Widescreen, MP1's way. `82b768ae8` (fix/wide2) closes the 4:3 gaps MP1 had already fixed: splash
logos keep their 4:3 shape and the face reflection's ortho widens with the view (`4f84326e0`, MP1
`f5796fc2e`); movies fit the view with their shape kept (`b0bdb0627`, MP1 `10d237a43 6b7384a85`);
credits text is laid out in the movie's 4:3 width (`511ba7bfe`, MP1 `cfa8e060b`); the scan window
keeps its share of the width below 4:3 (`cdeb6495b`, MP1 `629170f37`); a cinematic cut blends from
its unwidened FOV (`0a01d41a7`, MP1 `227c7fdaa`); the arm cannon is drawn at retail FOV, anchored to
the 4:3 bottom below 4:3 (`7ebd852b8`, MP1 `5fb236a91 1a83a296d af5e42ade`). `7b82b73c9` widens the
portal warp/world transition and keeps retail's cinematic framing instead of cropping it; `fcd80eb77`
keeps that width below 4:3 (Vert+). `a3dbfec15` (fix/wide) makes the pause and map frames fill a wide
window (`CGuiFrame::SetSpreadPanel`: the window grows, the side panel slides, text keeps its shape,
`MapScreenWidth` follows the spread).

Game-side fixes. `66ca362a3`: the FOV option's Retail entry restores the game's 55 immediately
instead of waiting for a slider move. `4cc49dc49` (merge `68d3a77a4`): the morph ball's model shadow
passes the actor-light array slot, so the ball is shaded again instead of flat bright (GOTCHAS, "A
model shadow's light mask"). `a5d3587f6` (fix/suit1): suit colours apply live while paused — Apply
now/reloadmods are serviced in the pause loop and the pause Inventory's Varia model is recoloured and
refreshed — plus the Light Suit main slot, kVersion 4, cannon textures reverted (`689a5fe4e`).
`c1dfbb37b` (fix/scanbands): scaled A8/R8/G8/B8 EFB copies sample Nearest, so the scan visor's index
map is no longer averaged into wrong colours; I8 copies stay filtered, being luminance images not
indices (`cba2b0837`).

Phone verification pending: none of the above has been run on Android (touch layouts at 16:9/21:9, the
pause Inventory recolor and the ball shadow there).

## 2026-10-25 (port orchestrator): batch34, last behaviour rows

Integration `integ/batch34` (`../wt-mp2-batch34`, from port 92bcb4775) merged kral2/kral2b, oct3, spa3/spa3b/spa3c,
fish2/fish2b and wisp3/wisp3b: FishCloud 56, Kralee 18, OctapedeSegment 24, ScriptPlayerActor 33 and WispTentacle 35
behaviour rows all marked; progress 5880 markers, behaviour 5491/5491 (100%). An astra (`openai/gpt-6-astra#xhigh`)
fresh-context review of every new marker found three bad proofs, all reworked: spa3 C30 count rewrite (now a real
count-3 RenderShadowBuffer with the beam model aliased as grapple), fish2 Touch 740/750 (real normalization-false
arms, now forced via `fishcloud_touch_norm_force.py`), kral2 244C (block3->5 initiallyInvisible ternary, proven by
`tools/scan_kralee_props.py`). Gates: build-port-link links, 87 files OK, gen_files_cmake --check, smoke59b vs
smoke58d no regressions. Evidence in `../evidence-archive/{kral2,oct3,spa3,fish2,wisp3}`.
Caveats: many forced arms have no natural trigger (no shipped grapple model for ScriptPlayerActor, WispTentacle
last-touched player/bomb); WallCrawler/Ripper/ScriptCoin still never spawned; H/T rows (copies 2056/4743) don't count.

## 2026-10-20 (port orchestrator): lanes in flight

Read a lane's result with the openchamber tool (`session.messages`, `lastAssistant`), review with
`tools/review_decomp_diff.sh <base>` from its worktree, accept, then send the same session its next slice.
Worktrees: `~/.local/share/opencode/worktree/3e7f972e5f2a67a4bf166b19e99a591913d8d4a4/wt-mp2-*`.

**Next-slice preamble (2026-10-21; send with each lane's next slice until all have had it):** first
`git merge port` (brings `8721dda7e`); fast runs `MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8` with
`tools/drivers/console.py` `@f<n>` scripts; reach states with console `fsm <id> <state>` / `place <id> player [d]`
/ `damage` / `send` (forced arms count when the branch table says `forced (<cmd>)`, see
DECOMP_WORKFLOW.md); put drivers in `tools/drivers/`; ask for missing console commands and shared-header
accessors in the report; also write the final report to `/tmp/opencode/reports/<lane>.md`.
Batch 7 (branch `batch7` in `../wt-mp2-batch5`, head `07d7159e1`, 10 heads from the table; host/link/matching
green 12:13) merged into port `54e8498b0`: 1785 markers, 15.21% fns / 20.13% bytes. smoke7 (`/tmp/opencode/smoke7`)
vs smoke6: 0 new errors, 17/17 rc 0; sandboss stalled once at frame 240 (rc 124, blank frame), passed solo: watch it.
Parallel smoke runner (3-4 Xvfb displays + ticks-per-frame + `@f`) next, checked against smoke7.
Orchestrator queue: grapple points never become the orbit target (CScriptGrapplePoint.cpp:68 only kMT_Orbit vs
CPlayerOrbit.cpp:1004/1070 kMT_Target only; SwampBoss-stage2.md §7.17), a shared-code bug to fix on port.
Lane watcher: `~/.cache/mp2-orch/watch.sh` (lanes file beside it), detached loop (`loop.sh`) logs events to `events.log` there.

**Accepted since batch 7 (batch 8 candidates, 2026-10-21 14:15):** CommandoPirate `ed504da00` + `24fca504f`
(2474/7A98 ported unmarked), DarkSamus `482a38568`+`4154642ea`+`5d31354a9` + darksamus2 `c9eb05d69` (1B658 138A4 1382C 13750 1367C), SwampBoss2 `1fb9cf495` + `26715ee9c`
(55B0), SwampBoss1 `46aeee651` + `23a9594eb` (belly cap fix, 35DC 3320 348C), EmperorIngStage1 `6386fff8f` + `527356dac` (tentacle anim search order),
Ing `329c5a27c`..`15b1c9b33` + orchestrator `9f7134a47` + `04abccad1` (B8C0 BCB4; aim form-9 fix) (fn_29_4604's three missing steps; the lane twice
claimed them ported without the code), DigitalGuardian `6c6eae0c7` (incl. shared fix `e403a065e`
DolphinCModel UnlockTextures), Splitter `601ceaac7`, bg2 `6c8bfd383` + `396e8ec01` (module done: only copies left unmarked), ep2 `ef9806f4a`,
CommandoPirate `d72c5d88a` (961C ported, 2474 time fix), SwampBoss2 `83d44d889` (5830 50AC 527C). New lane: Sandworm
(`../wt-mp2-sandworm`, classification first). Console `fsm` sub-machines + `damage ... powerbomb` merged `d272a3f96`; Ing lane next: `damage <ing> 100 powerbomb` for fn_29_B3C4's burning arm.
**Lane sessions were deleted from the DB (14:05)**: fresh lanes spawned as subagents of this orchestrator; ids in
`~/.cache/mp2-orch/lanes` (the table's session column is stale). Surviving old sessions: Splitter (re-sent),
EmperorIng1 (busy), DarkSamus (busy, old orchestrator's child). Shared asks queued for port: ELoopAttackType 5/6
+ CBCLoopAttackCmd skip-into setter (CommandoPirate), `;`-separated console commands per poll (Splitter),
fluid plane `Burn` on patterned actors (Ing fn_29_B3C4 burning arm). Grapple/orbit: not a bug (L pressed before the 20-update candidate rebuild); console `grapple <id> [d]` merged `cc780b874`.

Batch 5 merged into port `3bb8177db` (via branch `batch5`, worktree `../wt-mp2-batch5`): playerturret
`c377344ed`, ing `8855b1674`, spacepirate-vt `c42d82fce`, emperoring `542d6c9b8`, emperoring3 `f786b983e`,
boostguardian `da45064c5`, digitalguardian `227e42fb1`, commandopirate `cf9eb1995`, splitter `b17910bb4`
(+ `e4aead7b6` drops DG's duplicate `CCollisionActor::SetExtendedTouchBounds`). Host/link/matching
green, smoke5 17/17 clean (`/tmp/opencode/smoke5/`), 1159 markers. Upstream build must run before the
port build in a fresh worktree (writes `build/G2ME01/include/*.inc`; needs `extern/musyx` checked out).

Batch 6 (2026-10-21, merged into port as `3b2effc06`, 1677 markers): merged emperoring3
`43eb8312e`, emperoring `5a594830e`, boostguardian `673fbcb56`, minoring `c8d342614`, spacepirate-vt
`33291f63c` (conflict in PortCSpacePirateStates.cpp: kept port's WarpIn fix `4d6c4d036`, dropped the
stale fn_72_7078 marker), wallcrawler2 `036ca13db`, commandopirate `6ea58f47a`, digitalguardian
`c56aff752`, splitter `29e4bf6aa`, darksamus2 `f0de7edab`. Matching 87 OK + sha1 OK, host + link green
(07:05). smoke6 17/17 clean (rc 0, 0 caught, 0 reach-stub; stand-ins down in boostg, digig, emp1,
emp3, ep). Batch 7 = lane commits after these bases, merged by hash.

Older list (superseded by batch 6):
spacepirate-vt `33291f63c` (8 combat marks); wallcrawler2 `036ca13db` (obstruction scan, 3 marks);
emperoring `bd4c2c25a` (7EA8, 7F8C cases, 1D38, Render 531C); splitter `b1f682ff7` (1188/58E8, F1C0,
654C, 1790, AC68, 8D50, AAC4); commandopirate `843ebf292` (4D84); boostguardian `999b51bdf` (D4F8 events,
C1B0/C05C, C850, puddle vulnerability 0x1178); digitalguardian `8826277c0` (8090, 7D8C, 5910, 5384,
7CB4, 5274/266C/2E60, 5838 marked); emperoring3 `e3971fcd3` (6904/67C0 beams, full-TUniqueId fix).

Port fix 2026-10-20 (scan-visor lag, after the OOM fix `1f1c5c9a3`): `DrawScanVisor` builds a new
CGraphicsPalette (new TLUT id) every frame, so Aurora made a render-scale palette texture every frame;
`resolve_dynamic_palette_texture` now takes over an older identity's texture for the same copy (desktop:
1 alloc on entry, then 0/120 frames). The two scan-visor EFB CPU read-backs (full render-scale frame
each, every frame) are dropped: Aurora draws both from the copy textures (A/B identical; only
`MP2_NO_COPY_TEX=1` uses the bytes). Desktop-verified; tablet not yet tried (ask before installing).

| lane | session | branch @ base | task |
|---|---|---|---|
| BoostGuardian (Opus) | — (done) | boostguardian @ `673fbcb56` (204/318; 103 cited, 2 folded; loaders hand-checked, isEchoEmitter + 19 sound ids -1 defaults; 12AB0 compares correct) | merge in batch 6; 9 fns between tsv 309 and 318 unexplained |
| EmperorIng3 (Opus) | — (done) | emperoring3 @ `43eb8312e` (182/270; 88 cited, 0 todo; matching build OK) | module complete: merge in batch 6 |
| DigitalGuardian (Opus) | `ses_ea312bc93ffdb6liLXfqe2O49p` | sandboss @ `f4626cd11` (accepted for batch 7: port-only CVisualKnowledge (DOL 0x801F55F0..; shared files.cmake/gen_files_cmake.py line — orchestrator merges); A138/9E88 ported; 10/10C cited as DOL copies (unmarked); marked 17C98 171F8 1524C) | unmorph SIGSEGV CPlayer::Render→CTexture::Load; long fight → 14470, IsDead/IsIdle, 15284, A138/9E88 arms |
| Splitter (Opus) | `ses_ea312bc7fffemfIq7YQeApP3b6` | elitepirate @ `fcf5b7329` (accepted for batch 7: head undocks on chassis Death (EB04 clears 0xC6C; chassis dies only via A0A0 cannonball damage in LaserSweep); ported EB04 5D50 A0A0 42F8; marked 5AD8; boost = B) | undocked head 2994, PathFind 4E90, 3ADC 3D20 2280 565C 570C; HeadExplosion BEEC; real cannonball kill; beam render later |
| CommandoPirate (worker) | `ses_ea2c75221ffeCKO3r32cSZZVdR` | commandopirate @ `a0f626797` (0 markers; accepted for batch 7: CKnockBackMgr accessor in TARGET_PC (87 OK), ShieldCharge 8028 ported, branch table; charge band 10-20 check at 0xB370 failed) | drive band 10-20 → full shield cycle; port 7A98 + 2048 null-test skeleton |
| SpacePirate (PARKED) | `ses_ea3206eb3ffeIXL6w8JDRAddQp` | spacepirate @ `8a5800003` (84/262; accepted for batch 7: WarpIn summary corrected, case-0 materials right (0x7374 vs Deactivate 0x7528), 7078 a/b/c/d table; open: GRNT/FOLW and blocked-box arms) | parked: remaining bodies gated by shipped data |
| SwampBossStage1 (worker) | `ses_ea3206eafffeTqAzGkbdyimVXR` | swampboss @ `b209619df` (114/251; accepted: arena chain mapped (LAYER3 warp pre-loads → no Arrived; recipe send u044d Open + u0476 SetToZero); charge = A, 19.9 dmg; live-aim driver 37/330 hits, belly 21.58/50) | xd10 decay check; better hit rate → ExposeBelly; mark 35DC 3320 348C 5784 9A44 |
| SwampBossStage2 (worker) | `ses_ea2c7e7d2ffejX82iejJ2M25fe` | swampboss-s2 @ `475d1886b` (accepted: 5030/527C match but never run (anim event 0x1b never fires); give grappleBeam works, lock-on won't take the grapple point; Blow stun-in-window fires but window not caught) | port 50AC recursion + 55B0; why event 0x1b never fires; grapple lock-on condition |
| EmperorIngStage1 (worker) | `ses_ea24d8402ffe9XMGEDPkZNQs0q` | emp1b (decomp/emperoring1b) @ `1c654bc31` (91/241) | classify the 8 unexplained fns; 4954 FSM1 code-node dispatch gap; next 5 with branch tables |
| IngBoostBallGuardian (worker) | `ses_ea21cc6d0ffec8C8RnDduH1WKJ` | bg2 (decomp/boostguardian2) @ `1c654bc31` (204/318) | classify unmarked fns (host copies vs behaviour); next 5 with branch tables |
| ElitePirate (worker) | `ses_ea2119ceeffe01muX6hLQf0aFV` | ep2 (decomp/elitepirate2) @ `1c654bc31` (104/241) | classify unmarked fns; next 5 with branch tables |
| WallCrawler (worker) | — (done) | wallcrawler @ `d156dd4b3` (25/57, 32 cited; 1310 constrained arm ran: PillBug planarConstraint 1/2 in Agon 0b/0d_sand_hall; constraint 3 has no shipped instance) | module complete: merge in batch 7 |
| ScriptPlayerTurret (worker) | — (done) | playerturret @ `f679121be` (16/59 marked, 40 cited; 84C unmarked: split-screen MP unported in the harness; 1BE8 unmarked: kSM_Stop never sent) | module complete: merge in batch 7 |
| Ing (worker) | `ses_ea2a5b996ffep8QSEsdcQ7an9k` | ingfam (decomp/ing2) @ `67b20dbfb` (129/274; accepted: A.21 branch tables for 45BC 4604 426C 3948; area-57 Ings stay ing spots (StartAsIngSpot) in 9 runs) | diagnose ing-spot wake-up triggers vs asm; kill corporeal Ing → B3C4; Agon grapple |
| EmperorIng1 (worker) | — (done) | emperoring @ `5a594830e` (91/241; 140 cited; 4954 needs FSM1 code-node dispatch, 7EA8 no tentacle sound event; 8 fns between survey 233 and port_progress 241 unexplained) | merge in batch 6 |
| MinorIng (worker) | — (done) | minoring @ `c8d342614` (128/217; 28 dead, rest cited; 204C query-success arm never ran: path query returns kR_NoSourcePoint in area 13, check it is not a port bug) | merge in batch 6 |
| Inglet fix (claude) | `ses_ea4b8efdcffeJYrVuvmgGX4q6a` | merged to port as `2294724f1` (CFoldySurface byte-swap) + `67f337a57` (Aurora dstAlpha alpha pass) | done; the Aurora pass touches every blended dstAlpha draw: include in the next smoke. Puddles are unscannable in retail too |
| DarkSamus (Opus) | `ses_ea2935cc8ffeCvp1QjKlpVMOxG` | darksamus2 @ `ef73b3963` (62 markers; accepted for batch 7: DiveAttack 11FA4 + helpers, BreakDiveAttack 11C54, 100DC/FF20 ported; marked E910 12B00 12E90 12EF8 18628 18FCC 11C54; dive recipe: damage u0732 570 → final stage) | IsPlayerAimLineClear always true; stand-ins MissileJump 177BC, MeleeAttack 17BC0, BoostBallInto B164, SweepBeam CA0C, 1B658, 138A4; 11FA4 open arms |

## 2026-10-20 (thirtieth session): the scan visor's highlights flashed - one shared copy destination

The report: "the scan visor overlay colours still are not perfectly right"; asked which part, "highlights
on scannables"; asked how, "they flash".

- **What flashed.** The visor's palette-index map (an A8 viewport copy sampled as a C8 texture through
  the scan palette) alternated between the real indices and the *scan window pane's* RGB565 bytes, so
  the highlight tint on scannables changed from frame to frame. Measured with a new
  `MP2_DEBUG_SCAN_PALETTE=1` check - every populated index is `paletteIndex << 2`, so any index that is
  not a multiple of four is foreign data: **~34,500 of 356,608 bytes on roughly half the frames** while
  the visor was up; 0 on the others.
- **Three causes, each fixed:**
  1. `PortEfbCopy`'s staging slots were keyed by the destination pointer alone and had their geometry
     overwritten by every `Request`, so the visor's `Fill` could consume the pane's completed read-back
     (two copies into the same buffer per frame: `796x448 tx=1` and `208x92 tx=2`, both logged).
     A slot is now the whole copy identity - shared pointer, size, texel size - with its own staging
     buffer, and `Fill` takes the texel size too (from the destination format in
     `LoadDolphinSpareTexture`).
  2. **Aurora resolves a copy into a texture keyed by the destination pointer.** Both consumers copied
     into `CGraphics::GetDolphinSpareBuffer()`, so whichever copy was recorded last owned the resolved
     texture for both - the reason the pane showed the visor's indices as noise and vice versa.
     `PortEfbCopy::Destination(shared, w, h, texelBytes)` now hands each consumer a stable private
     destination: the scan visor's A8 viewport copy (`CCubeRenderer::DrawScanVisor`) and the window
     pane's RGB565 copy (`CPlayerVisor::DrawScanEffect`) `GXCopyTex` into it, `Request`/`Fill` it, and
     bind their texture from it.
  3. A first attempt bound the texture to a *shadow copy* while the copy still landed in the shared
     buffer; Aurora then read the linear guest bytes as tiled C8 and the overlay became diagonal
     stripes (`/tmp/opencode/final_22.png`). That is why the fix is a per-copy *destination*, not a
     shadow of the bytes.
- **Verified on the desktop** (`Xvfb :86`, `tools/run_gameplay.sh`, `MP2_INPUT=LEFT@600+4`, plus a
  turning-camera and a holding-`L` run): `bad=0` on every logged frame, no dropped slots, no torn
  reads; screenshots show the pane's magnified scene clean (`/tmp/opencode/vf_30.png` - it had white
  streaks before) and the highlight stripes steady in colour. Gates: `python3 tools/gen_files_cmake.py
  --check` rc=0, and the upstream matching build after the three upstream-file edits -
  `main.dol` sha1 `6ef9b491d0cc08bc81a124fdedb8bfaec34d0010`, Game Code 94.14% fuzzy / 63.29% matched
  (unchanged), every edit inside `#ifdef TARGET_PC` with the non-PC statements preserved verbatim.
- **Still open, same class:** the camera filters, `CPlayerGun` and the particles also copy into the
  shared spare buffer without a `Destination`, so they can still take each other's resolved texture
  (unchanged by this session, and no longer able to reach the visor's or the pane's copies). Fold them
  into `Destination` the same way when one of them shows the symptom.

## 2026-10-20 (twenty-ninth session): the F1 Teleport and items pages, Prime 1's shape

- **This commit:** the F1 overlay gets MP1's two debug pages, in this game's terms.
  - **Cheats page, Teleport block** (where MP1 keeps it, below the items; the Game page keeps
    this port's own area/world cycle buttons). A room combo (`IGetAreaAlways` names, so unloaded areas are
    named too; slots with no MREA are hidden) with a filter field, `Reload` (a world warp to the same
    world and room - the area warp deliberately ignores a request for the room it is already in),
    and a world combo built from the memory card's world table (`CSaveWorldMemory::GetFrontEndName`,
    with the GuiSys markup and the hand-placed line break stripped: the entry is
    `&line-spacing=75;Temple\nGrounds`).
  - **Cheats page, items.** Echoes' abilities and keys as checkbox grids, missiles / power bombs /
    dark and light ammo / energy tanks sliders bounded by `GetPowerUpMaxValue`, `Full health` and
    `Grant everything`. A grant sets the capacity to the game's own maximum for a counter (1 for an
    ability, 0 to remove it) and the amount to the slider's value: `AddPowerUp` for the capacity
    difference, then `SetItemAmount` for the amount. That is MP1's `SetPowerUp`/`SetPickup` shape.
    A first version moved both fields by the amount's delta, and a review caught what that does:
    sliding missiles from 255 to 55 lowered the capacity to 55 and left the amount at 255, and
    unticking an item whose amount was already 0 did nothing.
- **The first version kept a `CWorld*` and segfaulted on a world warp from the menu** - the
  outgoing world is freed while the overlay still draws the frames before the restart. The menu's
  own world warp is what found it (`[warp] 3bfa3eff: the outgoing world's pak stays readable`, then
  a fault); the console's `world` with the overlay closed survived, which is why it looked flaky.
  The room list is now a snapshot copied in `SetGameState`, keyed on `CWorld::IGetWorldAssetId()` -
  not on `gpGameState`'s id, which names the destination on the frame the warp is served while the
  outgoing world is still the live one (that key left Torvus Bog's page showing Temple Grounds'
  rooms).
- **Measured** (run on Xvfb :441, `MP2_SKIP_FRONTEND=1`, a copy of the `/tmp/opencode/locktest`
  card): the room picker lists `0  01_Temple_LandingSite` … 62 areas of Temple Grounds, and picking
  one warped (`[warp] area 9: loading` -> `[area] 9` -> `the game's own transition arrived in area
  9`); the world picker warped to `3dfd2249` and the page then read `0  00_Elev_Swamp_Temple_B`;
  `Grant everything` moved health to 1499/1499, missiles/power bombs/dark ammo to 255/10/250 with
  every box ticked; clicking the missiles slider at a quarter then put `55 / 255` on the HUD, and
  unticking and re-ticking Power Beam followed in the grid. Screenshots `/tmp/opencode/final_game.png`,
  `final_cheats.png`, `p2_torvus.png`, `r3_hud.png`.
- **Pages.** The F1 window now has the Prime 1 port's page set, as far as this port has the
  features behind each: Game (state and the area/world cycle buttons, Speedrun, Cutscenes),
  Controls, Video, Cheats (items then Teleport), Save states, Randomizer, System, Debug. MP1's
  Remastered, Mods, Archipelago and Tracker have no feature here, so no empty pages. `Debug` holds
  the free camera and photo (moved off Game), the world/area and game-update readout, and the
  console's commands; `Save states` is the old Game block on a page of its own; `Randomizer` reports
  the loaded seed and generates one with the logic fill (`PortRandomizer::SeedFilePath`,
  `GenerateSeed`; the dump log is no longer read).
- **The randomizer's user directory was the Prime 1 port's** (`MP_USER_PATH`, else SDL's pref path
  for "Metroid Prime"), so a dump or seed went somewhere the port's card and settings never look.
  It now reads `MP2_USER_PATH` first; the Randomizer page showed `/tmp/opencode/ov_user/...` under a
  run with that variable.
- **Layout.** The first version split the two: the warp pickers on the Game page and the items on
  Cheats. MP1 has both on Cheats, in the order health/grants, abilities, counters, Teleport, and
  this now does too (the port's `Previous/Next area/world` buttons stay on Game).
- Not covered: the Android page layout, and the items page in the front end beyond its
  "Waiting for gameplay..." guard.

## 2026-10-19 (twenty-eighth session): widescreen HUD spread, the MP1 way

- **`6ae31083e`, merged `f209d2f97`:** MP1's widescreen HUD. A first attempt spread widgets by their
  x position and detached the energy number from its bar; that was thrown away. MP1's mechanism is a
  per-camera opt-out (`SetAspectMatch(match, spread)`, default spreadable) plus a per-widget move in
  `CGuiFrame::Draw` after `CGuiCamera` widens the projection. `CSamusHud` marks the HUD and helmet
  cameras spreadable; the pause and message screens keep the game's proportions. Setting
  `MP2_HUD_WIDE` (key `hud_wide`, F1 > Video > Display, default on).
- Measured before/after: MP2's HUD never spread before this (the pause frame spans x 197-1088 of 1280
  at 16:9, which is MP1 with HudWide off). Now with it on the radar and energy readout sit at the left
  edge and the ammo/missile cluster at the right; with it off the HUD is 4:3 centred; the pause screen
  is identical either way. Screenshots `/tmp/opencode/hw2_{hud,pause}_{on,off}.png`.
- Merged-tree gates: host build 0 FAILED (binary 15:39), `gen_files_cmake.py --check` rc 0, every
  touched upstream file strips CLEAN, and the matching build still reads 94.14% fuzzy / 63.29% matched.
- Not covered: the selected-visor HUD frames are not marked spreadable (`CSamusHud` exposes only the
  HUD and helmet cameras), MP1's explicit anchor clusters are not ported, and the spread ignores a HUD
  scale below 100%. Nothing was walked in gameplay, per usual: the screenshots are of a standing player.

## 2026-10-19 (twenty-seventh session): every world swept

- **Method.** I used a scratch copy of `tools/sweep_areas.sh` that sets `MP2_WORLD="<world>,<first area's MREA>"` instead of
  `MP2_TELEPORT`, because the teleport only indexes the save's own world and wins when both are set. Settings: `NEXTAREA` walk,
  `GODMODE=1`, `SHARDS=4 DWELL=900 TAIL=900`, save `/tmp/opencode/ss_user`, and the binary from 15:39 (`c334bc9ae`).
  The logs are in `/tmp/opencode/wsw/log_w_<world>_<shard>.log`.
- **Worlds.** The console's `worlds` command lists five campaign worlds, FrontEnd04 (`69802220`) and six 1-area
  multiplayer maps. All of the 1-area worlds load, and boots of `233e42be` and `0x69802220` exit 0.

  | World | Id | Areas | Reached | Verdicts |
  |---|---|---|---|---|
  | Temple Grounds | 3bfa3eff | 63 | 0-53, 56, 57 | shard 3 STALL at 57->58; 54/55 not in the walk; 59-62 not covered |
  | Agon Wastes | 42b935e4 | 72 | 72 | all exit=0; reach stubs at 7 and 37 |
  | Torvus Bog | 3dfd2249 | 70 | 70 | all exit=0; reach stub at 46 |
  | Sanctuary Fortress | 1baa96c2 | 67 | 67 | all exit=0; area 16 POOL |
  | Great Temple | 863fcd72 | 12 | 12 | all exit=0 |

- **Totals across every log.** There were no `[port] caught`, `[player] killed`, missing-resource, `[model]`, assert or `[end]`
  lines. The only `[warning]` is the shutdown `Device lost: Device was destroyed`.
- **Temple Grounds 58 is a harness trap, not a game wall.** The log reads `[warp] area 58: its validation was held by a dock;
  releasing it` and then `[harness] no game update for 30.0 s (6322 updates so far, limit 13500)`, exit=124. A lone
  `WORLD=3bfa3eff,85499aa7` run reproduces it after 122 updates. The screenshot shows "Save progress to Memory Card in
  Slot A? Yes/No": the arrival spawn is inside a save station, and the sweep's `B` does not answer that prompt. Under gdb
  the main thread was idle in the frame limiter (`CGraphics::EndScene`).
- **Top blockers, earliest first:**
  1. Agon 7: `CParasite::ProjectVectorToPlane` reach stub (the parasites' plane projection is a no-op).
  2. Agon 37: reach stub for `TStateMachineStateBase<CPatterned>::~TStateMachineStateBase()`.
  3. Torvus 46: the `CParasite::ProjectVectorToPlane` stub again.
  4. Sanctuary 16: the id pool holds at `[ids] frame 14700: 982/1024 ids allocated, 982 objects` until frame 15000. That
     follows `[warp] dropped the areas around 16 (650 object ids free)`, so one more spawn burst there could exhaust the pool.
  5. Tooling: the save prompt at TG 58 (above) stops the sweep. `ResolveWarpAsset` reads an all-digit hex id as decimal
     (`MP2_WORLD=69802220` → `0x042918ec`, "neither the save nor the disc's MLVL names an area"; `0x69802220` works).
     The warp walk also never rolls credits, because slot 53 logged no `[end]`.
  `DebugDrawAABox` also hits its stub (Sanctuary 0, TG 44/48, Agon 47/59), which is harmless.
- **Coverage limit.** This sweep tests arrival plus about 15 s of standing gameplay under god mode. It does not test
  traversal, doors, elevators, bosses or the ending. Next: real `ProjectVectorToPlane` and state-dtor bodies, a `B`/`Yes`
  answer for save prompts in `sweep_areas.sh`, a hex-always parse in `ResolveWarpAsset`, and TG 59-62.

## 2026-10-19 (twenty-sixth session): interpolation phase 6, save-state resume and door locks

- **Phase 6** (`9b651429a`, MP1's section 3 "swooshes, electric effects and beam weapons"):
  `CParticleSwoosh` draws trails between the last two ticks (segments matched by age, spin the short
  way, the actor's shift dropped while it blends), `CParticleElectric` blends its line transform, and
  `CPlasmaProjectile` blends transform, length, width and angle (its Echoes `Render` did not fit MP1's
  edit, so the PC version is duplicated ahead of the original under `#else`). Beams opt out of the actor
  blend through `CActor::PortSetOwnPresentation`. `CWaveBuster`/`CFlameThrower` have no Echoes
  counterpart. `platform/include/port_tick_pair.h` and `port_tick_pair_tests` come from MP1. Uses the
  existing Smooth effects setting, so there is no new flag. Not seen on screen: both runs idled in area 0.
- **Quick resume, door locks, a patient restore** (`65a05ac34`, merged `f117ba308`): the resume state
  (`<user>/savestates/resume.mp2ss`) is written on a clean exit when `MP2_RESUME=1` and loaded at the next
  launch when the front end is skipped and no `MP2_WORLD`/`MP2_TELEPORT` was asked for; it is deleted once
  used and refused if its world is not on the disc. Doors keep their lock state in the room capture
  (applied with `kSM_Lock`/`kSM_Unlock`; one friend line in `CScriptDoor.hpp`). `TickRoom` now gives up only
  after 1,200 *idle* frames (a pending layer, a loading area or a progress change resets the count), with a
  hard cap. Verified end to end: `resume: wrote ... (Area 0)` then `loading it instead of the card's save`
  and `Samus is back in Area 0; the resume state is used up`, both runs exit 0, 0 unmatched. The door-lock
  apply and the real window-close path were not exercised.
  Superseded 2026-10-25 (qr1): the resume file is kept until the next snapshot, snapshots are taken
  during play, and a boot prompt asks; see platform/README.md "Quick resume".
- Merged-tree gates: host build 0 FAILED (binary 15:14), matching build "87 files OK",
  94.14% fuzzy / 63.29% matched. Every touched upstream file strips CLEAN.

## 2026-10-19 (twenty-fifth session): the glyph keys were right all along

- **Echoes uses Prime 1's button textures.** Opening the pause screen (START) and diffing its texture
  dumps against a gameplay run leaves 19 textures, and four are the hashes already in `kKeys`:
  `32x32 bb21e8755f36b2f0` (`AButtonIn`), `32x32 e6dbfd18d4666ee7` (`BButtonIn`), and the two `64x80`
  `AButtonOut`/`BButtonOut` sets. Keys are content hashes (`XXH64` of the decoded bytes,
  `extern/aurora/lib/gfx/texture_replacement.cpp:428`), and the art is shared between the games.
- So the earlier "the binding icons replace nothing in Echoes" was wrong: every run up to then
  dismissed the HUD memo (`MP2_SKIP_INTRO=1`) and never drew a prompt, so the textures were never
  loaded. Evidence: `/tmp/opencode/pause.png`, `/tmp/opencode/pause_only.txt`.
- The `MP2_DEBUG_PROMPTS` hook (`36775d730`) and its constructor variant were reverted: only one
  `CGuiModel` is ever built in a run (id `ffffffff`), so Echoes' prompt widget is not a `CGuiModel`
  and the hook told us nothing about it.
- **Left to do:** run `tools/make_prompt_glyphs.py` to produce a `bindings/` set, point `MP2_TEXTURES`
  at it, and confirm the pause screen's bottom bar shows keyboard glyphs instead of A/B/C/Y.

## 2026-10-19 (twenty-fourth session): the texture dump path, and where Echoes' prompts are

- **`f558e5b3f`:** a texture dump dropped every file. `dump_editable_texture_dds` wrote into
  `<cache>/texture_dumps`, but only `load_replacement_directory` ever created that folder, so a run
  with no pack installed - the one way to author a pack - had nowhere to write. The dump now creates
  the folder, and a dump skipped because the format did not convert logs why (once per format/size).
  Verified: `MP2_DUMP_TEXTURES=1` writes 147 `tex1_*.dds` into the run's own cache folder
  (`/tmp/opencode/c_run/texture_dumps`, because `tools/run_gameplay.sh` sets `MP2_CACHE_PATH` from
  `CACHEDIR` - the `MP2_CACHE_PATH` a caller passes is overridden, which is why the first attempts
  looked empty).
- **Echoes' button prompts are not 32x32 textures.** Of the nine 32x32 dumps from a Temple Grounds
  boot (`/tmp/opencode/glyphs_sheet.png`), none is a button: the game draws prompts from GUI *models*
  (`model_abutton` in `CSamusHud.cpp:157`, `CHudDecoInterfaceScan.cpp:108`), whose texture is not one
  of them. Building Echoes' `kKeys` therefore needs a dump taken while a prompt is up, plus the model's
  own texture size/hash - still open.


Older sessions: `docs/STATUS-history.md`.

The openchamber tool rejects every non-string argument (2026-10-20), so lanes run as subagents now; old lane sessions can't be resumed from here.
- 2026-10-21 16:40 batch9 merged (081b71c14): CommandoPirate a957ff47c, EmperorIng1 298b62b1c, Splitter a619f5a69, DarkSamus2 ce575f3c8, Ing blob 9f8fd3003, Ing 646a9fabd, Sandworm cad0531f5, SwampBoss2 4a51bc63c, DigitalGuardian f8ec985d5; smoke9 no regressions.
- 2026-10-21 17:20 batch10 merged (f09b42959): CommandoPirate df8fbb5b6, DarkSamus2 5fe53f11c, EmperorIng1 af5535c8f (done), Splitter 4a756509f, Ing 1a0ba410f, SwampBoss2 32c062e08, BoostGuardian 75bd1ac4d (done), Sandworm 36fb82f7c, SandBoss 5a5858859 (done), SpacePirate 727d520b8, DigitalGuardian 0d273717a + aurora 32 MiB uniform buffer (82b1c482c); 1890/4730 fns. smoke10 vs smoke9: turret (GPU reset 16:56, innocent context) and ing (X display shared with another run) pass solo (smoke10b); agon54 exit 137 = shutdown hang twice, same as spacepirate (sp-hang lane). New console `noise` (196d15fee).

## 2026-10-21 (port orchestrator, late): batches 12 and 13

- Batch 12 (25b367dda) and batch 13 (0d9af897f) merged after asm review by a dedicated Opus review lane and orchestrator checks; smoke12b/smoke13 no regressions (`--jobs 4` once gave flaky exit-137 shutdown hangs on ing/ep/plain/emp3, clean at `--jobs 2`).
- Audit-session fixes routed or applied: PlayerTurret refire `<= 0` (a19f25ae8), DarkSamus CanMissileDeflect `> 0` and RollReflectChanceForHit's first roll (f5802a210), CommandoPirate's five 662C/8028/86B4 items, the DigitalGuardian knee knock-back, the 177C8/114C8 stand-ins, the SpacePirate weapon switch, Hurled, Ing timer, trooper alpha and charge list.
- MiniMax misreads caught in review (`lis` as a small value, `__shl2i` input read as a high word, an early return skipping retail's tail) are now in LANE_BRIEF "Decode traps".

## Smoke rules (2026-10-24)

Poke your own family's actor; no `=> err` proofs; strictly increasing @f; objs checked >=40f after a kill; editor ids, not uids; no forced Dead. Beam-free lanes = `worker` + model `opencode-go/beam-free`; their reports are unreliable, so check con_*.out.
