# Running the port (desktop)

Moved out of AGENTS.md to keep the per-turn prompt small. `T` is the toolchain dir from AGENTS.md
Commands; all runs need `Xvfb :86 -screen 0 1280x720x24 -ac` (check with `pgrep -x Xvfb` + `xdpyinfo`).

## Boot under gdb (from build-port-link)

    DISPLAY=:86 SDL_VIDEODRIVER=x11 MESA_VK_WSI_DEBUG=sw SDL_AUDIODRIVER=dummy \
      MP2_DISC="/run/media/odran/Leo/Portable/roms/gc/Metroid Prime 2 - Echoes.iso" \
      MP2_USER_PATH=/tmp/mp2u MP2_CACHE_PATH=/tmp/mp2c MP_PORT_FRAMES=300 \
      timeout -k 5 120 gdb -q -batch -ex run -ex bt ./metroid_prime2_port > /tmp/mp2boot.log 2>&1
    grep -n 'reach-stub\|signal\|^#[0-9]\|exited\|ssert\|fatal' /tmp/mp2boot.log

Sample a running boot: `gdb ... -x tools/gdb_interrupt.py -ex run -ex 'thread apply all bt 14'`
(`MP_SAMPLE_SECS=240`). Screenshot: `ffmpeg -f x11grab -video_size 1280x720 -i :86 -frames:v 1 x.png`.

## Straight into gameplay

Needs a save in `<user>/USA/Card A` (a copy of `/tmp/opencode/u_a10`). Same env as above plus
`MP2_USER_PATH=<copy> MP2_SKIP_FRONTEND=1 MP2_SKIP_CINEMATICS=1 MP2_SKIP_INTRO=1 MP2_TELEPORT=<area>`
-> `[harness] gameplay reached at 39.1 s` for area 5 (36.0 s without `MP2_TELEPORT`; first update
34.0 s; desktop 2026-10-17). Phone: the same four as `env.MP2_...` lines in `files/port_paths.txt`.

PC-first debugging: rendering, cinematics and HUD questions reproduce on desktop; keep the phone for
the final APK check. `MP2_USER_PATH` must be the directory that holds `USA/Card A` (one level above
it the runner warns "has no USA/" and falls back to the front end):

    USERDIR=/tmp/opencode/mp2card/user FRAMES=6000 TELEPORT=0 DISPLAY_NUM=:86 LOG=/tmp/opencode/pc.log \
      tools/run_gameplay.sh &        # SKIP_INTRO=0 leaves the intro cinematic playing (default 1)
    tools/shot.sh 86 /tmp/opencode/pc.png 50          # timed to the log's phase lines

`run_gameplay.sh` starts an Xvfb on `DISPLAY_NUM` when none is running there and stops it when the
run ends: don't start one by hand. A script that launches the game itself must kill the Xvfb it
started (the orchestrator's watcher reaps client-less ones older than 30 min).

`PRESENT_EVERY=<n>` (default 6) shows one frame in n on the Xvfb, and `RENDER_EVERY=<n>` (default:
`PRESENT_EVERY`) builds one frame in n: the frames in between are dropped before they are recorded,
so the game spends no CPU on the frames it never shows (a sandworm smoke run: 29.1 s of game CPU
with `RENDER_EVERY=1` against 8.6 s with the default 6). A frame that is not rendered keeps the last
rendered frame's EFB, so the read-backs (`GXPeekZ`, the EFB copy) return the last drawn values, and
a screenshot always comes from a rendered frame (the frame it is taken in and the one after it are
both rendered).

An EFB copy (`GXCopyTex`) is the one thing that still costs a frame: a copy into a destination that
was copied within the last N frames (N = the render interval, `AURORA_RENDERSKIP_COPYDEST_N`
overrides it) keeps its texture and is skipped with the frame, so its image lags by up to N frames;
a new or one-shot destination (a render-to-texture at an area load) is rendered in full, so the
texture is produced. `AURORA_RENDERSKIP_COPYDEST=0` turns that off: every copy then renders its
frame. The log says how much the copies cost at shutdown (`AURORA_RENDER_EVERY: N frames were
rendered in full because they hold an EFB copy (M of them a read-back), K copies were skipped
because their destination was fresh`).

## Fast runs and console scripts

`MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=<1..16>` runs exactly that many fixed 1/60 s ticks per loop
iteration (`platform/include/port_timing.h`), as fast as the machine allows: 1000 updates in area 59
took 7.4 s at 8 vs 26 s capped (2026-10-21). `MP2_FRAMES` still counts ticks. Console scripts for
`MP2_CONSOLE=<port>` runs: `tools/drivers/con.py <port> <cmdfile> [out]`, lines `@f<n> <cmd>` (after game
update n, polled with `frame`; a few updates late at high tick counts) or `@<secs> <cmd>`.
Reach states with `fsm <id> <state>` (an FSM2 state left at once reports `entered; now X`; a state in
an FSM2 sub-machine is entered through it, and the reply names the machine), `place`,
`send`, `damage` (direct, no projectile; also the combos `darkburst`/`sunburst`/`sonicboom` and
`powerbomb`, which knockback rule sets tell apart) and `shoot <id> [charged] [d] [dir x y z]` (a real beam projectile
from d units, travelling along dir, for modules that only count projectile hits on a joint).
`statecall <id> <state> <msg> [dt]` runs one FSM2 state body once (msg 0 Activate, 1 Update, 2
Deactivate) outside the machine, from the actor's whole state table: states the FSM2 has no node for.
`codecall <id> <code> [dt]` does the same for a code function (codes otherwise run only on transitions).
`scaninfo <id>` prints the 'SCAN' asset an actor's GetScannableObjectInfo answers (no scanning needed).
`a; b; c` on one line runs all of them in the same game tick (a one-frame window); one reply each.
`grapple <id> [d]` (ids from `objs grapple`) wakes a grapple point, stands the player d (8) units
from it and holds L: with `give grappleBeam 1` the player locks on and grapples (`status`: orbit 5).
A bare `tp` + `press l` misses: the lock-on press needs the orbit candidate lists, rebuilt every 20 updates.
`scan <id> [d] [updates]` twice ~30 updates apart (scan visor + L, then tp d (40) units, forced orbit lock) scans an actor; `scan` alone prints progress (game state 1 = scan display up).
Ids may be `e<editor id>` (unique ids are reused across layer loads); `covdump` writes a coverage build's gcov counters mid-run (runs that hang in shutdown keep them).

## Area regression sweep (63 Temple Grounds areas, 4 shards, START-free)

    USERDIR=/tmp/opencode/u_a10 tools/sweep_areas.sh            # TAG/SHARDS/DWELL/COUNT to taste
    python3 tools/sweep_report.py '/tmp/opencode/log_<tag>_*.log'

Parallel software-Vulkan shards can lose the device; confirm verdicts with lone runs.

## Booting into a world and an area

`MP2_WORLD` and `MP2_TELEPORT` take a hex asset id or a name, but they share one slot, and
`MP2_TELEPORT` is applied second, so it wins - and with the fast harness the separate teleport is
served *before* the world is loaded, so the area is looked up in the old world and the run ends
with "[warp] area -1 does not exist in this world". **Give the world and the area in one
request**: `MP2_WORLD="1baa96c2,2735236b"` (the world, a comma, the area) or
`MP2_WORLD="42b935e4,01_sand_entrance_dark"` (the area's MLVL name). The area can also be the
area index, but the index is the one of the world being loaded, so the name is safer.

## Batch smoke (every family, in parallel)

`tools/smoke/run.sh [--build <dir>] [--out <dir>] [--jobs N] [family ...]` boots every family area
(one game each, on its own X display, console port, card copy and cache dir) and drives it with the
console scripts in `tools/smoke/cmd_*.txt`; `--list` prints the family table. Results land in
`/tmp/opencode/smoke-<git hash>` by default (the card copied per family is `--save`, default
`/tmp/opencode/mi_u`), and `tools/smoke/compare.py <new out> <old out>` prints exit, caught,
reach-stub, stand-in and gameplay seconds per family, failing when a family regressed.

## Branch coverage of the port module bodies (gcov)

The decomp lanes prove "every arm of a ported body ran" with gdb breakpoints, one spec per
function (`tools/cov/`). A coverage build answers the same question for *every* body at once: it
compiles the port's REL module bodies (the sources `tools/modules_port.txt` lists) with gcov, and
one ordinary run then reports the branch coverage of all of them.

    T=$PWD/../MetroidPrimePort/build/review-tools/bin      # no system cmake/ninja
    $T/cmake -S . -B build-port-cov -G Ninja -DCMAKE_MAKE_PROGRAM=$T/ninja \
      -DMP_SDK_HEADERS_ONLY=OFF -DMP_PORT_COVERAGE=ON \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_LINKER_TYPE=MOLD
    $T/ninja -C build-port-cov metroid_prime2_port

`MP_PORT_COVERAGE` is off by default and the normal build (`build-port-link`) is unchanged by it:
with the option off no flag is added and no source is compiled differently. The coverage build is
its own directory because a coverage build is not the port (the counters cost time and the code is
instrumented).

A run writes its profile (the `.gcda` files) into its own directory: the run scripts set
`GCOV_PREFIX` (and `GCOV_PREFIX_STRIP`, which drops the build directory's own path components so
the tree under the prefix mirrors the build tree) when the binary is a coverage build, so
concurrent runs do not write over each other. The game flushes the counters however the run ends -
a clean quit, the harness's `timeout` (SIGTERM) or a Ctrl-C (SIGINT) - through the hook in
`platform/port_coverage.cpp`, so a killed run still leaves a profile.

    BUILD=$PWD/build-port-cov tools/smoke/run.sh --build $PWD/build-port-cov --out /tmp/opencode/cov \
      --jobs 4 sandworm
    tools/cov/gcov_report.py $PWD/build-port-cov /tmp/opencode/cov/gcov_sandworm/gcov --module Sandworm

`tools/cov/gcov_report.py <build dir> <gcov prefix dir>... [--module <Name>] [--fn fn_10_1D794]`
prints one row per body: the retail id, the C++ name, the lines hit, the branches taken, the lines
whose branch was never taken, and `ALL-ARMS` when every branch of the body was taken. Several
prefix dirs may be given: their counters are merged, so the arms of a natural run and of a forced
one add up. The id of a body is the `// @ fn_<mod>_<hex>` marker above it (or the id its comment
block names). gdb is still what forces an arm no run reaches (`tools/cov/force.sh`): coverage says
which arm is missing, not how to reach it.

