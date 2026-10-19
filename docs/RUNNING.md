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

## Area regression sweep (63 Temple Grounds areas, 4 shards, START-free)

    USERDIR=/tmp/opencode/u_a10 tools/sweep_areas.sh            # TAG/SHARDS/DWELL/COUNT to taste
    python3 tools/sweep_report.py '/tmp/opencode/log_<tag>_*.log'

Parallel software-Vulkan shards can lose the device; confirm verdicts with lone runs.
