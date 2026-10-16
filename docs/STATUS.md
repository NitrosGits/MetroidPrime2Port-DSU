# Session status (detail)

The Status section of AGENTS.md links here: this is the per-session detail, rewritten rather than
appended, dated as in the heading there.

Gates: `tools/gen_files_cmake.py --check` clean, and the upstream matching build green after this
session's port edit in an upstream file (`Game Code: 94.14% fuzzy, 63.29% matched`, `rc=0`). Commits:
`befa10ead` (the sweep tools), `c600728c6` (the ball camera fix), `6f7d192ea` (`B` presses, the
RELOAD verdict).

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
1. **Areas 14, 21 and 35 stop updating.** The last line is always the post-arrival
   `[warp] dropped the areas around N`; the loop keeps rendering (`CGraphics::EndScene`/`PaceFrame`)
   and `[harness] no game update for 30.0 s` is the only report. With `B` it is a pure stall; with `A`
   the run instead quits to the front end and reloads slot 0. Measured under gdb: **`CMFGame::OnMessage`
   is never called again** (a breakpoint set after the stall sat four minutes), so the tick does not
   reach the game object and `CMFGame`'s flow state machine - the first hypothesis - is not the
   suspect. Cheap repro (2.5 min): `MP2_TELEPORT=21 MP2_FRAMES=6000 MP2_INPUT="HEAL@2500,B@3000+4"`.
   At the stall, dump `CIOWinManager`'s win list and break in `CGameArchitectureSupport::UpdateTicks`:
   the candidates are that `CMFGame` leaves the IO win stack (a quit to the front end the skip does not
   re-boot - which would also explain what `A` does there) and that `CMain::CheckReset`'s button chord
   fires from the virtual pad's state.
2. **Area 53's hop never lands** in a long run (Temple Grounds 53 = `531079ba`): the direct stream
   starts (`711 of 1024 free`) and never completes while the game plays on in 52 to the frame limit. In
   isolation the hop arrives, but the game then moves *itself* to area 56 (a dock) and freezes with the
   same no-tick signature as (1), so this is probably (1) behind another door - fix (1) first.
3. An `MP2_INPUT` token that fires **before the game's first update** (~34 s, iteration ~2040) is
   deferred ("a token arrived while the boot request was pending; serving it after") and then never
   served: `/tmp/opencode/handfix.log` shows `NEXTAREA@1400` never hopping while the same token at 3500
   did. Either serve it or log that it was dropped.

**Still unverified**: hazards as gameplay, the scan logbook's persistence, escape/endgame paths,
lock-on handoff, morph ball and gyro input; and the empty-`optional_object` read audit
(`CGunWeapon::IsChargeAnimOver` first).

**Android APK** (asked for 2026-10-17, same session). **M0 done**; detail, evidence and the phone
measurement in `docs/ANDROID.md` (its section 4, "M0 detail"). In short: `tools/android_probe.sh`
cross-builds arm64 into `build/android-aarch64/libmetroid_prime2_port.so` (`AArch64`, `DYN`,
exporting `SDL_main`/`aurora_main`; 465 undefined symbols, all platform ones), the bionic fixes are
`platform/main.cpp` (no `execinfo`; logcat + tombstones; explicit `<aurora/main.h>`) and
`platform/rel.cpp` (arena fallback via a `/proc/self/maps` gap scan below 4 GiB), and the one gate
- the REL arena - was measured **on the phone**: 32 MiB at `0x12000000`, `0x81000000` honoured
exactly, no `MAP_32BIT`. New: `tests/port_arena_placement.cpp` + its ctest target
(`-DMP_ARENA_FORCE_PORTABLE` exercises the Android path on Linux), `MP_ANDROID_NOD_STUB` (the M0
library links the nod stub, so it cannot read a disc). Next: M1 - a Gradle/Java app that boots on
the device, and the real nod cross-build (Rust target already installed in
`../MetroidPrimePort/build/android-rust`).

**Next steps, in order**: (1) the update stall, with the repro above and the two candidates named;
(2) the 53 -> 56 freeze if it survives (1); (3) the hazards and the layer areas of `1baa96c2` area 11
as gameplay; (4) the family bodies in the priority order; (5) a no-present option.
