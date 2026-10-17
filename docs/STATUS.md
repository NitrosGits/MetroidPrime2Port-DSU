# Session status (detail)

The Status section of AGENTS.md links here: this is the per-session detail, rewritten rather than
appended, dated as in the heading there.

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
2. **Area 53's hop never lands** in a long run (Temple Grounds 53 = `531079ba`): the direct stream
   starts (`711 of 1024 free`) and never completes while the game plays on in 52 to the frame limit. In
   isolation the hop arrives, but the game then moves *itself* to area 56 (a dock) and freezes with the
   same no-tick signature as (1), so this is probably (1) behind another door - fix (1) first.
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
