# Session status (detail)

The Status section of AGENTS.md links here: this is the per-session detail, rewritten rather than
appended, dated as in the heading there.

## 2026-10-17 (fifth session): the EFB-into-CPU copy - the scan visor's spare buffer

**What it is.** The console's `GXCopyTex(dest, clear)` copies a region of the framebuffer into main
memory at `dest`, in the format the preceding `GXSetTexCopyDst` chose. Aurora resolves the same copy
into a *texture*, which serves every consumer that samples it as a texture, but the game also treats
some of those destinations as bytes: the scan visor uploads the spare buffer as a C8 texture whose
texels are palette indices, the camera filters upload it as a half-size RGB565 backdrop, and the scan
window's pane does the same at its own size. The port now mirrors those copies for the CPU:
- `PortEfbCopy::Request` (new, `src/MetroidPrime/PortEfbCopy.cpp`) is called immediately before the
  `GXCopyTex` it mirrors. It issues `GXRequestEfbCpuCopy`, a FIFO command that takes the copy's
  geometry, destination size and texel format from the `GXSetTexCopySrc/Dst` state that is current at
  that point, so a call site cannot disagree with the copy.
- `aurora::gfx::efb_copy` (`extern/aurora/lib/gfx/efb_copy.cpp`) is the read-back, in the shape of
  `scan_peek`: the request rides on the pass the copy closes, the pass's *resolve target* (the same
  pixels and format the texture path produced) is copied to a mappable buffer at encode time, mapped
  in `after_submit`, and converted on the CPU to the copy's texel format - A8/R8/G8/B8/I8 as one
  byte, RGB565 big-endian, which is what the game's upload of console data expects. One to three
  frames of latency; a seqlock hands the bytes to the game thread.
- `PortEfbCopy::Fill` is called from both `CGraphics::LoadDolphinSpareTexture` overloads, just before
  the upload, and copies the latest completed read-back for that buffer and size into it. Nothing is
  written from the render thread, so an upload never sees half a copy.
- Three call sites, one mechanism: `CCubeRenderer::DrawScanVisor` (A8, viewport - the visor's palette
  indices), `CCameraBlurPass::GetFbCopy` (RGB565 half, or I8 full - the camera filters) and
  `CPlayerVisor::DrawScanEffect` (RGB565 - the scan window). The results screen reads the same buffer
  and is covered by the same fill once a matching copy has been requested at its size.
- Knobs: `MP2_EFB_COPY=0` turns the whole thing off (the A/B), `MP2_DEBUG_EFB_COPY=1` logs the fill
  rate, and `MP2_NO_COPY_TEX=1` keeps a copy out of Aurora's copy-texture map so the game's own
  upload of the CPU bytes is what the GPU samples - which is how the desktop verification isolates
  the new path.

**Verified.** Desktop, camera filter, end to end: with `MP2_NO_COPY_TEX=1` the pause screen's
backdrop is a blurred frame with the copy on (`[efb-copy] first spare-buffer copy landed (320x224,
143360 bytes, sequence 2)`) and **black** with `MP2_EFB_COPY=0` - `q1_2.png` against `q2_2.png` in
`/tmp/opencode`. Gameplay and the same pause screen with the copy-texture path left on are unchanged
(`r1_a.png`, `r2_a.png`). Phone (release build installed, `MP2_INPUT=LEFT@600+2`): the visor opens,
`[efb-copy] 120 fills from 357 requests (640x448 into ...)` shows the A8 viewport copy landing every
frame, the frame rate is 60 fps in the visor (300 game frames per 5.00 s, the same as before the
visor), and the A/B with `MP2_EFB_COPY=0` shows the broken overlay - a large flat olive wedge over the
scene (`phone_off.png`) where the copy on gives a coherent tinted view (`phone_v3.png`).
**Not verified:** scanning an actual scannable and reading its highlight colour on the phone - the
save's view here has nothing scannable in the scan window (`[scan] no target: every one of the 2048
id pixels is zero`), and the desktop cannot show the visor at all (below). That check is the person's.

**Correcting the record on the visor's desktop device loss.** It is **not** fixed by `5e3e122ee`
(zeroing the spare buffer). Reproduced today, three times, with `LEFT@200+2`/`LEFT@600+2` in areas 0
and 2: `VK_ERROR_DEVICE_LOST`, GPUVM fault at the same address `0x80011ca000`, `SQC (data)`, every
time - and it still happens with the visor's overlay draw skipped, `MP2_SCAN_PEEK=0`,
`MP2_NO_COPY_TEX=1` and `MP2_EFB_COPY=0` all at once, so it is in the visor's own rendering and not
in any EFB-copy path. `RADV_DEBUG=hang` puts the fault in a plain one-sample fragment shader (a blit),
i.e. a bind group whose texture or view is already gone; the address is stable across runs and is not
in the driver's address-binding report. The earlier "it plays to its frame limit" reading came from
runs with `input=0 bytes` - they never pressed LEFT, so they never entered the visor. The same goes
for the "flat white viewport" screenshot (`visor4_a.png`): that run had no input either, so the white
is the save-load fade, not the visor. The real desktop symptom is the crash; the real phone symptom
(the wrong highlight colours) is what this session's copy fixed.

**Gates.** `tools/gen_files_cmake.py --check` clean (the new port source is in `PORT_ONLY`, so
`files.cmake` gained one line); the upstream matching build is green after this session's four port
edits in upstream files - `Game Code: 94.14% fuzzy, 63.29% matched`, `rc=0` - and every one of them is
inside `#ifdef TARGET_PC` with the `#else` text byte-identical to the revision before the session
(`DolphinCGraphics.cpp` keeps its non-PC `GXInitTexObj`/`GXInitTexObjCI` calls verbatim in an `#else`
branch for exactly that reason).

**Next.**
- The visor's desktop device loss (above) - the one thing standing between the desktop and a
  screenshot of the scan visor.
- The phone check the person can do in a minute: scan a scannable and see whether its highlight is a
  single colour from the palette rather than garbage.
- The other CPU-bytes consumers the same mechanism can now serve cheaply: `CFluidPlaneCPU` (reads the
  half-size RGB565 copy as data), `CCubeRenderer::GetRenderToTexBuffer`'s reflections, and the
  mip-chain path in `GenerateScreenMipmaps`, which are still on the texture path.

## Current state as of 2026-10-17 (moved out of `AGENTS.md`, verbatim)

## Status (2026-10-17, fourth session: M2/M3 verified, three crashes/bugs fixed, PC-first loop)

Detail, including the repro commands for every open wall, is in `docs/STATUS.md`.

- Gates green: `tools/gen_files_cmake.py --check`, and the upstream matching build after this session's
  one port edit in an upstream file (`Game Code: 94.14% fuzzy, 63.29% matched`, `rc=0`). Commits:
  `befa10ead` (sweep tools), `c600728c6` (ball camera fix), `6f7d192ea` (`B` presses, RELOAD verdict).
- `tools/sweep_areas.sh` + `tools/sweep_report.py` walk all 63 Temple Grounds areas in 4 shards in ~10
  minutes and print a per-area verdict table. Press `B`, never `START`, never `A` (see Gotchas).
- Fixed and verified twice: the handed-hop crash in `CBallCamera::FindDesiredPosition` (a dangling
  reference in `CCameraManager::GetLastCameraTransform()` that GCC folded to null). `MP2_WARP_HANDED=1`
  now forces the handed route, which makes that path a 90-second reproduction.
- Open, worst first: (1) ~~areas 14, 21 and 35 stop updating~~ - **resolved `05efe2ecc`: the player was
  dying** in the dark-world areas and the run then sat on the lose-game movie's "continue" screen
  (`docs/STATUS.md` wall 1 has the backtraces); the sweep heals twice per area now, has a `DIED`
  verdict, and a shard over areas 12..23 is clean; the sub-question is whether `A` picks "continue"
  there, which would make the RELOAD verdict's "walks into a save station" story wrong; (2) area 53's
  hop never lands in a long run - **resolved `a5b08f7ae`**: the destination was stuck in
  `kP_WaitForValidation` because a dock in area 52 preloads 53 and holds its validation until a door
  the warp never opens, and `PortWarpSettle` now releases that hold. Reaching area 53 also turns out
  to *be the ending* (the credits; `TELEPORT=53` is that, not a stall), so the report still owes a
  `CREDITS` verdict; (3) ~~an `MP2_INPUT` token before the game's
  first update is deferred and never served~~ - **resolved `4401a70e2`**: a token's frame is a game
  update now (`platform/README.md`, "Input"), so frame 0/1 is the state the game's first update reads
  and a token whose frame has gone by fires at the first frame the script sees; the counter still
  advances through a pause, so a script can press `B` to dismiss one.
- Unverified as before: hazards as gameplay, the logbook, escape/endgame, lock-on handoff, morph ball
  and gyro input, the empty-`optional_object` audit.
- **The swarm vertex explosion is fixed** (`23c928626`): the flying bugs of the Temple Grounds crater (and every other swarm and the fish cloud) drew through the guest array path with host-skinned arrays. The user's clue - "caused by the tiny flying bugs" - is what pointed at `CFlyerSwarm` and `CSwarmBasics`; the nest's web beside them is a real object.
- This session, on the phone: **M2 and M3 verified** (the SAF picker copies a picked ISO to `files/disc.iso`
  and the next launch boots with no picker; the strict first-update proof passes; touch alone walks and
  fires). **A crash fixed**: morphing then unmorphing SIGSEGV'd because `CSamusFaceReflection`'s
  destructor is `inline` in another TU and the port filled the missing symbol with a logging link stub,
  which left the face model's resources locked; defined in the class's own TU now (`bc4c733c3`), and the
  same sequence leaves the process running. **`MP2_SKIP_INTRO=1`** (`75874aff5`): with
  `MP2_SKIP_FRONTEND=1 MP2_SKIP_CINEMATICS=1 MP2_TELEPORT=<area>` a run reaches a controllable player
  with no input - 36.0 s on the phone, 39.1 s on the desktop, against ~3 minutes of hand-pressed `A`
  before. `MP2_SKIP_FRONTEND` still gives up on an empty card, and the 34 s before the first update is
  loop-bound ARAM staging, not phone I/O (both in `docs/STATUS.md`).
- **The HUD-over-cutscene and the wedge beside it are fixed** (`457c86fe2`): the port's cinematic skip
  stopped the camera but not the scene, so the intro played on under the first-person camera with the
  HUD and visor over it - the wedge was cutscene geometry, not a broken model. **The swarm boids no
  longer explode** either (`23c928626` + `651988088` + `95d6dc3ca`), and a `TARGET_PC` guard that had
  leaked out of its block (matching build 63.29% -> 63.25%) is back. Iterate on the desktop build;
  the phone is for the final APK check (`docs/STATUS.md` has the run and shot commands).
- Open, and the reason the `MP2_DEBUG_DRAW`/bounds instrumentation exists: first-person world geometry
  that renders as a vertex explosion (dark maroon slabs and white
  strands; `/tmp/opencode/ve_fp3.png` and `m2.png` are phone frames of it). Not a reach stub (`0 of 45`
  are referenced), not a missing resource, and the face reflection only draws on a visor transition. The
  phone reproduces it on every run in that area.
- The Android APK the user asked for: plan and the M0-M3 detail in `docs/ANDROID.md` (section 4).
  **M0 done**: `tools/android_probe.sh` cross-builds this tree for arm64-v8a into
  `build/android-aarch64/libmetroid_prime2_port.so` - `AArch64`, exports `SDL_main`/`aurora_main`,
  `llvm-nm -u` all platform symbols, offline (Dawn/SDL/Rust caches in `build/android-deps`). The
  REL arena gate passed **on the user's phone** (32 MiB arena at `0x12000000`, the console's
  `0x81000000` base honoured exactly, no `MAP_32BIT`; fallback is a `/proc/self/maps` gap scan,
  covered on Linux by `port_arena_placement_tests`). **M1 done**: `android/` + `tools/android_apk.sh`
  build and install `org.metroidprime2.port`, which mounts the user's own disc through the real
  Rust nod on the phone (86 modules initialised, card loaded, renderer up, game loop running); fd
  1/2 are mirrored to logcat and the dirs resolve from `files/port_paths.txt`. Two known Android
  issues are recorded in `docs/ANDROID.md`: a one-off Dawn Vulkan-surface SIGABRT when launched
  with the screen locked, and the debug variant's `-O0` musyx inline link error.
  **M2 verified on the phone**: the `env.NAME=value` bridge in `files/port_paths.txt` (the `set`
  lines, `paths chosen` and `disc mounted` are in `docs/ANDROID.md`), the SAF picker with its
  G2ME01-header check and `.part`-then-rename copy (picked from `/sdcard`, `copying the disc
  image, 100%`, `disc image picked: content://…, copied to …/files/disc.iso`), and a relaunch
  after that boots with **no picker**. The strict `[harness]`/`[ids]` "first update" proof passed
  too: `the first game update is at 33.9 s`, `[ids] frame 0: 352/1024 ids allocated`.
  **M3 done and device-verified** (this session): `platform/port_touch.{cpp,h}` + MP1's
  `platform/touch_pad.{h,cpp}` + `tests/touch_pad.cpp` + `android/.../TouchControlsView.java`
  (the SDL virtual pad, a third aim source in `PortMouse`, the settings in `<user>/touch.cfg`, the
  JNI bridge, and a phone-only "Edit" pill for the layout editor). On the phone a run with
  nothing but touch walks the player (`[touch] axis 1 -1.00`) and fires (`[touch] button 0
  down/up`), the pad attaches as `Metroid Prime touch gamepad`, and the editor opens, switches
  GameCube/twin layout and saves it. `port_touch_pad_tests` covers the pad on the host;
  `MP2_DEBUG_PAD=1` prints one line per touch event for a device log. Next: **M4**, the release
  APK polish.

---

Gates: `tools/gen_files_cmake.py --check` clean, and the upstream matching build green after this
session's port edit in an upstream file (`Game Code: 94.14% fuzzy, 63.29% matched`, `rc=0`). Commits:
`befa10ead` (the sweep tools), `c600728c6` (the ball camera fix), `6f7d192ea` (`B` presses, the
RELOAD verdict).

**Still open, and how to settle each**
- **A flickering triangle from the viewport's lower-left, camera-attached** (the person's words), seen
  before the cinematic fix. That fix explains one wedge - cutscene geometry under the first-person
  camera - so this may be the same thing. It is **unconfirmed**: the person has not yet played the
  build with `457c86fe2` on the phone. If it persists in ordinary gameplay it is a second bug; the
  cheapest reproduction is the desktop crater run (`USERDIR=/tmp/opencode/mp2card/user TELEPORT=0
  tools/run_gameplay.sh`), not the phone.
- **The packed-UV lead is closed, as an instrument bug rather than a game bug** (`6b33c3336`): the
  materials whose TEX0 indices only fit the packed array are the ones flagged for it, and the draws
  that looked wrong are the world-shadow pass's black material, which returns before any array is
  chosen and whose texgen reads positions only. The scan now checks each surface against the array its
  own material selects.
- **The areas 14/21/35 update stall** (below) is being worked with `CIOWinManager`'s win list as the
  instrument, which is the thing to read at the stall rather than the functions that stop being called.

**Fixed this session, all verified on the desktop build** (rendering, cinematics and HUD questions
reproduce there, so they are not worth driving through the phone):
- **The HUD drawn over the landing cutscene, and the large translucent wedge beside it, are one bug**
  (`457c86fe2`). The port's cinematic skip stopped the cinematic *camera* (`StopCinematics`) and left
  the *scene* playing, so the intro ran on under the first-person camera with the HUD, the visor and
  the faceplate over it - the wedge is cutscene geometry seen from the wrong camera, not a broken
  model. Retail's skip (`CStateManager::SpecialSkipCinematic`) also fires the scene's skip
  connections, so it jumps to its end; that is what the port calls now, with `StopCinematics` as the
  fallback for a scene with no skip function. Controlled A/B: with `SKIP_INTRO=0` the cinematic has
  no HUD at all, with `SKIP_INTRO=1` it had the whole HUD over it, and with the fix `SKIP_INTRO=1`
  skips the intro outright and reaches the crater at the same time (33.9 s / 35.9 s).
- **A port edit that leaked out of its `TARGET_PC` guard** (`95d6dc3ca`). The swarm residency guard
  added a helper and an `if` in `CSwarmBasics.cpp` without the guard, and the upstream matching build
  dropped from `63.29%` to `63.25%` matched - less than one file, which is exactly why the number is
  worth reading after every upstream edit rather than glancing at "files OK". Guarded, and checked by
  stripping `TARGET_PC` blocks from all 18 files this session touched and diffing against the
  pre-session revision: identical everywhere.

**The scan visor** (the person's report: "you can't lock onto things to scan"):
- **The lock-on's missing piece is fixed** (`e8de72e7b`): MP2 picks its scan target from a 32x64 region
  whose ids live in the framebuffer's destination alpha, copied into CPU buffers and histogrammed.
  The copy went through the console `GXCopyTex` path while Aurora resolves that into a *texture*, so
  the buffers were never written. Measured first: the ids *do* reach the EFB alpha (`31/2048 pixels
  non-zero, palette 4` in area 2), only the copy-out was missing. `scan_peek` mirrors Aurora's
  `depth_peek`; one to three frames of latency, which the consumer does not mind.
  **Still unproven end to end**: the `[scan] palette N selected object X` line has not been seen,
  because on the desktop the visor could not be entered at all (below) and on the phone the build with
  the fix has not been tried yet.
- **The spare buffer is zeroed at allocation** (`5e3e122ee`): entering the visor lost the device
  (GPUVM, `SQC (data)`), and the uninitialised spare buffer was the suspect, since the visor binds it
  as a C8 texture and samples its bytes as palette indices through a 248-entry TLUT; zeroing is what
  the console's heap had and keeps the worst case inside the palette. **The crash is not fixed,
  though** - see the fifth-session section at the top: it reproduces with every EFB-copy path off,
  and the verification runs behind that commit never entered the visor.
- **The shared feature - an EFB-into-CPU read-back at viewport size to fill the spare buffer - is
  in** (fifth session, top of this file): the scan visor's overlay (`CCubeRenderer::DrawScanVisor`),
  the camera filters (`CCameraFilter.cpp:569-676`) and the scan window all read it through
  `PortEfbCopy`, with the same encoder-task shape and a frame of latency.
- **Confirmed on the phone (the person's report)**: scanning works - objects now lock on and scan -
  but **the highlight colours are wrong**. That is this same missing spare-buffer copy: the visor
  colourises by sampling it, and on the phone it holds uninitialised host memory, so the palette
  lookup gives random colours. `e8de72e7b` fixed the *targeting* (which reads the separate 32x64
  region), and `5e3e122ee` stopped the desktop dying on the same buffer; neither fills it.
- **Correcting an earlier note**: `MP2_INPUT` D-pad tokens *do* switch the visor (`LEFT@200+20`
  works). What looked like the press failing was the visor opening onto its broken overlay - a white
  viewport with the HUD intact, which is index 0 of the palette after the zeroing above. The
  `MP2_DEBUG_SCAN` selection line staying silent is a separate question about that diagnostic's call
  site, not evidence the visor never opened.

**Open, and it explains why the scan visor's bug survived so long: entering the scan visor kills the**
**desktop build about two frames in** with a GPUVM fault (`VK_ERROR_DEVICE_LOST`, `SQC (data)`). The
read-back lane hit it while trying to prove a scan target gets selected, and reproduced it with the
**pre-change** binary, with its own peek disabled, with the scan-window copy disabled, and in areas
0/1/2/3 - so it is in the scan visor's own rendering and predates that work. On the phone the visor
renders, which is why the person could see the visor at all. Worth fixing first, because it is the
reason desktop verification of anything scan-related was impossible: the swapchain/device loss also
means a scan-visor run cannot be screenshotted. **Still open as of the fifth session** (top of this
file), which ruled out every EFB-copy path as the cause and left a blit sampling a destroyed bind
group.

**This session (2026-10-17, fourth).** Verified on the user's phone rather than reasoned about:
- **M2's two open checks pass.** The SAF picker copies a picked ISO to `files/disc.iso` (`disc image
  picked: content://... , copied to .../files/disc.iso`), the next launch boots with **no picker**, and
  the strict first-update proof prints `[harness] the first game update is at 33.9 s` / `[ids] frame 0`.
- **M3's touch controls work with nothing but touch**: a held press on the drawn A pill advances the
  game's own boot screen, the floating stick walks the player (`[touch] axis 1 -1.00`), A fires, and the
  Edit pill opens the layout editor, switches GameCube/twin stick and saves it.
- **Morphing and unmorphing no longer crash** (`bc4c733c3`). The face reflection's destructor is
  `inline` in another TU, so no symbol was emitted and the port filled it with a logging link stub; the
  stub left the face model's resources locked and the second reflection (built on unmorph) died with
  SIGSEGV. Measured before: `[reach-stub 0001] _ZN20CSamusFaceReflectionD1Ev` then signal 11.
- **The swarm boids no longer render as exploded geometry** (`23c928626`) - the maroon curtains and
  pale slabs that covered the Temple Grounds crater's view. Cause: `CSwarmDisplayList::Draw` bound the
  swarm's host-skinned position/normal arrays through the *guest* array path (size 0, big-endian), so
  Aurora uploaded nothing or byte-swapped floats. One fix covers `CSwarmBasics`, `CMetareeSwarm`,
  `CPlantScarabSwarm`, `CIngBlobSwarm` and `CFishCloud`, which all draw through that class. The white
  radial structure left on the wall is the bugs' nest, not a bug - the user confirmed it.

**Regression sweep after this session's fixes (63 areas, 4 shards, `TAG=postfix`).** 48 of the 53
areas the shards reached are `OK`, **including 14, 21 and 35** - the old "stop updating" wall, which was
the player dying, now plays 3000/1200/900 updates and reports `OK`, so the death diagnosis and the
second `HEAL` fixed it and area 35 (unverified when that fix landed) is confirmed. **No crash and no
reach stub in any area**, so the nine fixes this session changed nothing for the worse. Two things the
table leaves open:
- **Area 46 ends in `STALL`**, and the log's last line before the harness report is
  `[warning] [aurora::gpu] Device lost: Device was destroyed` - four shards were running software
  Vulkan in parallel, so this reads as the renderer losing its device under that load rather than a
  game stall. Worth one confirmation run on its own (single shard, `AREA=46`) before it is called
  either way.
- **Area 53 is still the hop that never lands** (`asked for but never arrived: [53]`), the same gap as
  before; shard 3 played to its frame limit, so this is the long-standing wall and not a new one.

**The sweep exists and walks the whole world cheaply.** `tools/sweep_areas.sh` boots the save's own
world, teleports each shard to its first area and walks its slice with `NEXTAREA` tokens, all through
`tools/run_gameplay.sh`; `tools/sweep_report.py` turns the shard logs into a per-area table (verdict,
updates played, id high-water, missing resources, hop route, reach stubs) plus a coverage block from
the sweep's manifest. Four shards, all 63 areas, ~10 minutes of wall clock. Press `B`, never `START`
and never `A` - the gotcha above says what each does to a run.

**Verified this session**
- **The handed-hop crash is fixed** (`c600728c6`). `CCameraManager::GetLastCameraTransform()` returned
  a reference into a temporary that had already died; GCC folded it to a null pointer and
  `CBallCamera::FindDesiredPosition` read the transform through address 0. Verified twice on Temple
  Grounds 10 -> 11 with 588 ids free, the handed route: the natural replay of the original sequence and
  a forced one (`MP2_WARP_HANDED=1`, added this session - it makes this path a 90-second reproduction
  instead of ten hops of accumulated core). Both arrive, spawn and run on; before the fix both died.
- 29 areas played clean in a single pass (0..14, 16..21, 32..35, 48..52): hops land, the post-arrival
  drop leaves 600-900 ids free, the id count stays flat, no missing resources, no crash.

**Walls the sweep found, worst first**
1. ~~**Areas 14, 21 and 35 stop updating.**~~ **Resolved (`05efe2ecc`): the player was dying.**
   A dead single player takes retail's game-over path - `CMFGame::EndGame` sets `kRM_EndMovie2`,
   `CMFGame` leaves the `CIOWinManager` stack, and `CMainFlow` puts `CPlayMovie(kWM_LoseGame)` on it,
   a "continue from last save" screen that takes no `B` - so the run sat there and the harness's
   "no game update for 30 s" was a symptom, not a hang. Measured under gdb with a backtrace per
   `AddIOWin`/`RemoveIOWin` and the two window lists side by side: in area 21,
   `[player] killed in area 21 at update 1194 by weapon type 17 - the dark-world atmosphere ...`.
   Areas 21 and 35 carry the dark flag (as do 13 and 28), and one `HEAL` per area left ~23 s between
   heals against a death ~19 s after one. The sweep now heals twice per area, the report has a `DIED`
   verdict that outranks `STALL` and `RELOAD`, and a shard over areas 12..23 exits 0 with every area
   OK. **Open sub-question**: does `A` pick "continue" on that screen? If it does, the `AGENTS.md`
   Gotcha's "`A` walks into a save station" explanation for the RELOAD verdict is wrong rather than
   incomplete. Also unverified: area 35 with the second heal.
2. ~~**Area 53's hop never lands**~~ **Resolved, and it was two separate things (`a5b08f7ae`).**
   - **The hop**: not a stall and not a death. A per-300-frame trace put the destination in `phase 9`,
     `kP_WaitForValidation` (`CGameArea.cpp:1312`), which only advances when `mValidationPaused` clears.
     A script in area 52 sends `kSM_Load` to the dock towards 53 (`CScriptDock.cpp:177`), loading it
     ahead of time and holding its validation until the door opens - and a warp never opens the door.
     The isolated hop arrived only because it left 52 before that script ran. `PortWarpSettle` now
     clears the hold while it streams (`[warp] area N: its validation was held by a dock; releasing
     it`), and the same run logs the release, `moved to area 53`, its spawn and `[area] 53`.
   - **The "moves itself to 56 and freezes" is the end credits**: `TELEPORT=53` reaches the ending -
     `[area] 56`, ids down to 60, then no updates, with "METROID PRIME 2 ECHOES STAFF CREDITS" on
     screen (`CCredits`, reached through `CScriptSpecialFunction::AcceptCredits`,
     `CScriptSpecialFunction.cpp:1066`). Nothing is wrong; a sweep walking into 53 is watching the
     credits. **Named now**: the port prints `[end] the credits are rolling` as `CCredits` goes on the
     window stack, `tools/sweep_report.py` has a `CREDITS` verdict for that phase (it ranks below
     `DIED`, above `RELOAD`/`STALL`, and its coverage line says the slice ends at the ending), and
     `tools/sweep_areas.sh` documents the slot list including the ending.
3. ~~**An `MP2_INPUT` token that fires before the game's first update is deferred and never served.**~~
   **Resolved: a token's frame is a game update now.** The script counted `PortPumpEvents` iterations,
   so every token under the boot's ~2040 iterations landed in the boot: a warp token went to
   `PortWarp`'s deferred slot while the boot request was pending and the *next* token overwrote it
   (which is why `NEXTAREA@1400` never hopped where `NEXTAREA@3500` did), and a button's hold was over
   before the game had read a pad. Frame `N` is now game update `N` - `PortHarness::GameUpdates`, the
   same axis as `MP2_FRAMES` - frame 0/1 is the pad state the game's first update reads, and a token
   whose frame has gone by fires at the first frame the script sees instead of being dropped
   (`platform/README.md`, "Input", has the table). Verified: `MP2_INPUT="HEAL@1"` and `"HEAL@0"` both
   log `[input] HEAL at frame 1 (0.0 s)` and the game serves them *in* its first update (`[warp]
   healed` prints before `[harness] the first game update is at 34.0 s`, which the same update emits
   after the warp tick); `HEAL@2500,B@3000+4` fires on updates 2500 and 3000, which the `[ids] frame
   N` lines interleave exactly; and a 3-area sweep (`tools/sweep_areas.sh`, `DWELL=400`, build-token)
   hops 0 -> 1 -> 2 at `NEXTAREA@400`/`@800` with every token served, exit 0, `sweep_report.py` all
   areas OK. Two properties worth keeping in mind: the counter still ticks while the game is *not*
   updating (a HUD memo pauses it, and the sweep's periodic `B` is what dismisses that), and it stays
   at 1 before the first update, so a screen that precedes every game update - the front end - cannot
   be driven by frame at all: `MP2_SKIP_FRONTEND` is the way past it. Warp tokens also log where they
   fired now (`[input] NEXTAREA at frame 400`), so a log can tell a token the game ignored from one
   that never fired.

**`MP2_SKIP_INTRO=1`** drives a run into gameplay from the port side, with MP1's `MP_SMOKE_WORLD`
semantics: boot the save, serve the boot warp (`MP2_TELEPORT`/`MP2_WORLD`), wait for it to land, report.
No input is synthesised. Before gameplay it (a) answers `CMFGame::EnterMessageScreen` (the HUD-memo
pause) with the game's own dismissal, `UnpauseGame` + the HUD-message frame counter, never pausing - a
real pause menu does not go through it; (b) gives a current cinematic camera the skip request START
makes (`MP2_SKIP_CINEMATICS=1` makes unseen ones skippable; without it the save's area-0 opening stays
in a cinematic camera for 900+ updates). "Gameplay reached" = 120 consecutive updates running, in
`kFS_InGame`, no warp pending/streaming/handed over, no cinematic camera; logged once as `[harness]
gameplay reached at <s> (frame <update>)`. Budget `MP2_SKIP_INTRO_FRAMES` (default 3600 game updates),
then `gave up`. `MP2_SKIP_FRONTEND` now keeps removing "SplashScreen" windows until the chain has been
quiet for 30 iterations. Desktop, `u_a10`'s card: area 5 reached at 39.1 s, area 0 at 36.0 s; the card
shows no memo, so (a) was only exercised with an injected memo (dismissed, game kept updating).
Area 0's opening visor boot-up ("SCANNING") is still on screen at 53 s, but stick input turns the
player then (the minimap rotates).
**Gap:** an empty card still plays the front end's file select (`MP2_SKIP_FRONTEND` gives up on an
empty slot instead of starting a new game).

**The ~34 s before the first game update is now the largest delay in every run**, and it is the same on
the desktop (33.8-34.0 s, five runs) and the phone (33.9 s), so it is not phone disc I/O: it is
loop-bound - the loop runs at ~60 Hz and ARAM staging moves 64 KiB per iteration (the `[aram] ...
fileSize2=` lines step by 65536).

**Still unverified**: hazards as gameplay, the scan logbook's persistence, escape/endgame paths,
lock-on handoff, morph ball and gyro input; and the empty-`optional_object` read audit
(`CGunWeapon::IsChargeAnimOver` first).

**Android APK** (asked for 2026-10-17, same session). **M0 and M1 done**; detail and evidence in
`docs/ANDROID.md` (section 4, the "M0 detail" and "M1 detail" subsections). In short: M0 is
`tools/android_probe.sh`, which cross-builds arm64 into
`build/android-aarch64/libmetroid_prime2_port.so` (`AArch64`/`DYN`, exporting
`SDL_main`/`aurora_main`, every undefined symbol a platform one) with the bionic fixes in
`platform/main.cpp` (no `execinfo`; logcat + tombstones; explicit `<aurora/main.h>`) and
`platform/rel.cpp` (arena fallback = a `/proc/self/maps` gap scan below 4 GiB), and the arena gate
measured **on the phone** (32 MiB at `0x12000000`, `0x81000000` honoured exactly, no `MAP_32BIT`).
M1 is `android/` + `tools/android_apk.sh`: `org.metroidprime2.port` installs, loads that library
with the **real Rust nod**, mounts the user's own disc on the phone, initialises all 86 modules,
loads the card, brings the renderer up and runs the loop; fd 1/2 are mirrored to logcat and the
directories come from `files/port_paths.txt`. New tests: `port_arena_placement_tests`
(`-DMP_ARENA_FORCE_PORTABLE` runs the Android arena path on Linux). Known Android issues, both in
the doc: a one-off Dawn Vulkan-surface SIGABRT when launched with the screen locked, and the debug
variant's `-O0` musyx inline link error (worked around in `android/app/build.gradle`). Unproven on
device: the front end visually (the phone was locked) and the strict "first update" - the harness
env bridge is now in place for it (`env.NAME=value` in `port_paths.txt`), and it is proven working
on the phone, but the run itself needs the phone foregrounded, awake and untouched for a couple of
minutes. **M2 implemented**: the SAF disc picker (`EnsureDiscImage` in `platform/main.cpp`) - a
`G2ME01` header check, `SDL_ShowFileDialogWithProperties`, then a `.part`-and-rename copy into
`<files>/disc.iso`; it runs before `aurora_initialize` because the system picker would otherwise
destroy the swapchain (this Aurora snapshot has no `aurora_release_lost_surface`). Its device
verification is the one still-open item. Next: **M3**, the touch controls.

**Next steps, in order**: (1) the update stall, with the repro above and the two candidates named;
(2) the 53 -> 56 freeze if it survives (1); (3) the hazards and the layer areas of `1baa96c2` area 11
as gameplay; (4) the family bodies in the priority order; (5) a no-present option.

