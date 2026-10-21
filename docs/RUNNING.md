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
`a; b; c` on one line runs all of them in the same game tick (a one-frame window); one reply each.
`grapple <id> [d]` (ids from `objs grapple`) wakes a grapple point, stands the player d (8) units
from it and holds L: with `give grappleBeam 1` the player locks on and grapples (`status`: orbit 5).
A bare `tp` + `press l` misses: the lock-on press needs the orbit candidate lists, rebuilt every 20 updates.

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

