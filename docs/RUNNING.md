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
an FSM2 sub-machine is entered through it, and the reply names the machine; a name the machine does
not have is reported as `unknown state 'X'; valid names: ...` with the names it has - the actor's
own states and, for an FSM2, the states of its sub-machines - a state that was entered and left
at once (the machine is back where it was) says `entered and left at once; now X`, and an actor
whose machine was never set up (it was not created yet) says `the actor has no state machine state
(is it active?)`), `place`,
`send`, `damage` (direct, no projectile; also the combos `darkburst`/`sunburst`/`sonicboom` and
`powerbomb`, which knockback rule sets tell apart) and `shoot <id> [charged] [d] [dir x y z]` (a real beam projectile
from d units, travelling along dir, for modules that only count projectile hits on a joint).
`place <id> <x> <y> <z>` moves an actor and `place <id> player [d]` puts it d (6) units in front
of the player; both take an optional rotation after the point: `place <id> <x> <y> <z> <yaw> [pitch
[roll]]` in degrees (the yaw about the world's up, the pitch about the right one, the roll about the
forward one) - without it the actor keeps its rotation.
`material <id> [+|-]<m> ...` prints an actor's material list and adds (`+`, the default) or removes
(`-`) materials, by name without `kMT_` (any case) or bit number: `material u0400 -solid` takes
kMT_Solid off the player the way entering a turret does, so a line-of-sight ray ending at its aim
point (ElitePirate ShouldFire) can come back clear.
`bodystate <id> <n>` sets a patterned actor's body controller state directly (n is the
`pas::EAnimationState` index: 0 ready, 2 lie on ground, 19 cover, 20 wall hang); the state machine
reaches only some of the body states, and the coverage runs need the others.
`statecall <id> <state> <msg> [dt]` runs one FSM2 state body once (msg 0 Activate, 1 Update, 2
Deactivate) outside the machine, from the actor's whole state table: states the FSM2 has no node for.
`codecall <id> <code> [dt]` does the same for a code function (codes otherwise run only on transitions).
`touch <mover id> <other id>` calls the mover's `CollidedWith` the way the collision code does
(`CGameCollision::MakeCollisionCallbacks`): with a one-entry collision list whose 0x30 slot holds
the *other* actor's material list. It needs no real collision, so a module whose own work is in
that callback can be driven (the SwampBossStage1 "player touched" flag, the retail's fn_78_9A44).
`scaninfo <id>` prints the 'SCAN' asset an actor's GetScannableObjectInfo answers (no scanning needed).
`a; b; c` on one line runs all of them in the same game tick (a one-frame window); one reply each.
`grapple <id> [d]` (ids from `objs grapple`) wakes a grapple point, stands the player d (8) units
from it and holds L: with `give grappleBeam 1` the player locks on and grapples (`status`: orbit 5).
A bare `tp` + `press l` misses: the lock-on press needs the orbit candidate lists, rebuilt every 20 updates.
`scan <id> [d] [updates]` twice ~30 updates apart (scan visor + L, then tp d (40) units, forced orbit lock) scans an actor; `scan` alone prints progress (game state 1 = scan display up).
`spawn <fourcc> [n] [d <dist>] [ancs <id>|u<id> [char [anim]]] [fsm <id>] [model <id>|u<id>] [inactive]
[prop <propid> <value>]...` creates n (1) script objects of a type through its `ScriptLoader` entry,
d (8) units in front of the player and facing it, for loaders no disc placement reaches (CHOG, COIN,
DBAR, RIPR, RSFA, SPNK): the property stream holds only EditorProperties (name, transform, active)
and what the options add, so every other SLdr default applies. Each gets editor id
`<area>FF00`+ in layer 0 (so `e0036ff00` names the first in area 0x36), AddObject and kSM_AreaLoaded.
`ancs`/`model` take a hex asset id or an actor (`u`/`e` id) whose ANCS+character or static model is
borrowed as a stand-in body (the types' own assets are in no pak); `fsm` sets the patterned state
machine; `prop` adds a raw 4-byte top-level property (`1.5` is a float, else an integer). Refusals
(logged, no crash): no loader for the type, an asset not in the resource factory, a loader that
returns null (patterned types without `ancs`), DBAR without `model` (its ctor needs the left chunk
model). A patterned actor whose body controller is still off after AreaLoaded (CHOG without its own
state machine) is activated by `spawn`, which says so. A COIN rolls its lifetime and impulse only on
the Activate that wakes it: `spawn COIN 3 inactive model u<actor> prop 54a8c481 20.0 prop 7dd63999 30.0`
then `send e<area>ff00 Activate`.
Ids may be `e<editor id>` (unique ids are reused across layer loads); `covdump` writes a coverage build's gcov counters mid-run (runs that hang in shutdown keep them).

## Area regression sweep (any world, shards, START-free)

    USERDIR=/tmp/opencode/u_a10 tools/sweep_areas.sh            # TAG/SHARDS/DWELL/COUNT to taste
    python3 tools/sweep_report.py '/tmp/opencode/log_<tag>_*.log'
    # every campaign world, 2 games at a time, ~8 min in fast mode:
    MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8 USERDIR=... WORLD=all TAG=all SHARDS=2 tools/sweep_areas.sh
    python3 tools/sweep_report.py '/tmp/opencode/log_all_*_*.log'   # ends with a per-world light/dark table

Without `WORLD` the sweep walks the save's world (Temple Grounds, 63 slots) by `MP2_TELEPORT`.
`WORLD=<id|agon|torvus|sanctuary|great temple|temple grounds|MLVL name part|all>` sweeps that world:
`tools/world_areas.py` reads its slot list from the disc (`--list` shows every MLVL), `COUNT`
defaults to its slot count and each shard boots with `MP2_WORLD="<world>,<first slot's MREA>"`.
`AREAS=<first>:<count> SHARD=0` re-runs one slice. Temple Grounds' ending (slots 47/50/53/56/58)
rolls the credits and ends a shard, so slots 48, 49, 54, 55, 57, 59-62 need their own `AREAS=`
runs, and 58 (`game_end_part5`) stops on its save prompt.

Parallel software-Vulkan shards can lose the device; confirm verdicts with lone runs.

## Booting into a world and an area

`MP2_WORLD` and `MP2_TELEPORT` take a hex asset id or a name, but they share one slot, and
`MP2_TELEPORT` is applied second, so it wins - and with the fast harness the separate teleport is
served *before* the world is loaded, so the area is looked up in the old world and the run ends
with "[warp] area -1 does not exist in this world". **Give the world and the area in one
request**: `MP2_WORLD="1baa96c2,2735236b"` (the world, a comma, the area) or
`MP2_WORLD="42b935e4,01_sand_entrance_dark"` (the area's MLVL name). The area can **not** be a
slot index here: `MP2_WORLD="863fcd72,7"` resolves `7` as asset id 00000007 and lands in slot 0 with
no error (measured 2026-10-23); `tools/world_areas.py <world>` gives each slot's MREA id.

## Batch smoke (every family, in parallel)

`tools/smoke/run.sh [--build <dir>] [--out <dir>] [--jobs N] [family ...]` boots every family area
(one game each, on its own X display, console port, card copy and cache dir) and drives it with the
console scripts in `tools/smoke/cmd_*.txt`; `--list` prints the family table. Results land in
`/tmp/opencode/smoke-<git hash>` by default (the card copied per family is `--save`, default
`/tmp/opencode/mi_u`), and `tools/smoke/compare.py <new out> <old out>` prints exit, caught,
reach-stub, stand-in and gameplay seconds per family, failing when a family regressed.

## Headless runs

For runs that need no picture (console scripts, logic checks, the smoke): `MP2_HEADLESS=1` (the
game sets `AURORA_HEADLESS=1` and `SDL_VIDEODRIVER=offscreen`), `HEADLESS=1 tools/run_gameplay.sh`
(no Xvfb, `DISPLAY`/`WAYLAND_DISPLAY` unset) or `tools/smoke/run.sh --headless`
(`SMOKE_HEADLESS=1`). Aurora makes Dawn's Null device and no surface, and renders no frame: the
FIFO is still processed (the register state the game reads back), but nothing is drawn, encoded,
submitted or presented. The host input scan is skipped too (no hidapi, no haptics: closing every
`/dev/input/event*` cost ~0.6 s of each boot); the console and `PADSetPortOverride` still drive pads.

What differs from a normal run (render side only; the game logic is the same):
- Screenshots (F12, console `screenshot`, `MP_SMOKE_SHOT`, PNG requests) are refused with a warning.
- Read-backs never land: scan peeks and EFB CPU copies are dropped and `GXPeekZRegion` returns
  false, so the game keeps its own "nothing read yet" values (Splitter beam length at the far
  plane, scan target 0xFF) and `CVisorFlare::UpdateOcclusion` takes its retail `GXCopyTex` path
  (console `status` shows ~30 copies and ~47 draws a frame instead of 1-2).
- No copy destination is ever recorded, so `perf` counters describe recording, not rendering.

Measured 2026-10-23 (agon54, one game): Aurora init 0.53 s -> 0.01 s, input probe 0.6 s -> 0,
first game update 5.2 s -> 4.1 s wall. "Gameplay reached" stays ~9.1 s: it is a frame-count wait
at 60 Hz (frame pacing, `CGraphics::EndScene`), which headless does not change. Full smoke
(`--jobs 4`, 18 families, shared machine): wall 364 s -> 356 s, CPU (user+sys of every game)
193 s -> 121 s; counts, positions and `[warp]`/`[ids]` lines match within run-to-run noise.

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

