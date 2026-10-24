#!/bin/bash
# A forcing run: the game under gdb, with the members of the listed bodies written at their
# entry, so the arms the fight cannot reach are taken (tools/cov/force.py is the gdb side).
# The fight itself is the same as a coverage run (tools/cov/run.sh): a slot with its own display
# and console port, the driver's .con file, and the same environment.
#
#   tools/cov/force.sh <spec.py> <out dir> <world,area> --con <cmdfile> | --driver '<cmd>' [--frames n] [--fast]
#   tools/cov/force.sh --attach <pid> <spec.py> <out dir>   (the game is already running)
#   (--driver as in tools/cov/run.sh: `{port}` becomes the console port; COV_GDB_SCRIPT=tools/cov/cov_force.py
#   also counts the spec's SPEC arms, as run.sh does, with the forced members already written)
#
# The spec defines FORCE = [(label, 'Body(', [exprs], [sets], skip, 'what'), ...]; the gdb side
# is $COV_GDB_SCRIPT (default tools/cov/force.py) and the class is the spec's CLASS.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
# force.sh changes into the build dir before gdb sources the script: make a relative path absolute.
[ -n "${COV_GDB_SCRIPT:-}" ] && COV_GDB_SCRIPT=$(realpath "$COV_GDB_SCRIPT")

# The attach mode: the game runs on its own (no gdb at the start, so the boot is not slowed
# down), the driver plays, and gdb takes the running process to write the forced members.  It
# needs the game started with MP2_ALLOW_PTRACE=1 (yama ptrace_scope 1 here: platform/main.cpp
# opts in with PR_SET_PTRACER_ANY), or CAP_SYS_PTRACE; where that
# is not granted, the normal mode below (gdb boots the game) is the way.
if [ "${1:-}" = "--attach" ]; then
  PID=$2; SPEC=$(realpath "$3"); OUT=$(realpath -m "$4"); shift 4
  BUILD=${BUILD:-$ROOT/build-port-link}
  mkdir -p "$OUT"
  nm "$BUILD/metroid_prime2_port" > "$OUT/nm.raw"
  nm -C "$BUILD/metroid_prime2_port" | cut -d' ' -f3- > "$OUT/nm.dem"
  paste "$OUT/nm.raw" "$OUT/nm.dem" > "$OUT/nm.txt"
  rm -f "$OUT/nm.raw" "$OUT/nm.dem"
  COV_SPEC="$SPEC" COV_NM="$OUT/nm.txt" COV_OUT="$OUT" \
    timeout -k 10 "${SECS:-900}" gdb -q -batch -nx -p "$PID" \
      -ex 'set pagination off' -ex 'set confirm off' \
      -ex 'handle SIGPIPE SIGUSR1 SIGUSR2 SIG32 SIG33 nostop noprint pass' \
      -ex "source ${COV_GDB_SCRIPT:-$ROOT/tools/cov/force.py}" \
      -ex 'continue' -ex 'quit' > "$OUT/gdb.log" 2>&1
  echo "attached to $PID, the forced log:"
  cat "$OUT/forced.log" 2>/dev/null | head -20
  exit 0
fi

SPEC=$(realpath "$1"); OUT=$(realpath -m "$2"); WORLD=$3; shift 3
CON=; DRIVER=; FRAMES=6000; EXTRA=()
while [ $# -gt 0 ]; do
  case $1 in
    --con) CON=$(realpath "$2"); shift 2 ;;
    --driver) DRIVER=$2; shift 2 ;;
    --frames) FRAMES=$2; shift 2 ;;
    --fast) EXTRA+=(MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8); shift ;;
    *) echo "unknown option $1" >&2; exit 2 ;;
  esac
done
BUILD=${BUILD:-$ROOT/build-port-link}
# a coverage build (MP_PORT_COVERAGE) writes its .gcda files into the run's own directory:
# the gcov counters of this run then merge with the other runs' (tools/cov/gcov_report.py).
. "$ROOT/tools/cov/gcov_env.sh"
gcov_env "$BUILD" "$OUT"
mkdir -p /tmp/opencode/locks; SLOT=
for s in $(seq 1 16); do
  exec {SLOTFD}>"/tmp/opencode/locks/cov-slot.$s"
  if flock -n "$SLOTFD"; then SLOT=$s; break; fi
  exec {SLOTFD}>&-
done
[ -z "$SLOT" ] && { echo "no free cov slot" >&2; exit 2; }
DISP=${DISPLAY_NUM:-:$((1100 + SLOT))}
PORT=${COV_PORT:-$((50100 + SLOT))}
DISC=${MP2_DISC:-/run/media/odran/Leo/Portable/roms/gc/Metroid Prime 2 - Echoes.iso}
mkdir -p "$OUT/c"
rm -rf "$OUT/u" && cp -r "${COV_SAVE:-/tmp/opencode/mi_u}" "$OUT/u"
nm "$BUILD/metroid_prime2_port" > "$OUT/nm.raw"
nm -C "$BUILD/metroid_prime2_port" | cut -d' ' -f3- > "$OUT/nm.dem"
paste "$OUT/nm.raw" "$OUT/nm.dem" > "$OUT/nm.txt"
rm -f "$OUT/nm.raw" "$OUT/nm.dem"
XVFB=
if ! xdpyinfo -display "$DISP" >/dev/null 2>&1; then
  Xvfb "$DISP" -screen 0 1280x720x24 -ac > "$OUT/xvfb.log" 2>&1 &
  XVFB=$!
  trap 'kill "$XVFB" 2>/dev/null' EXIT
  for _ in $(seq 1 50); do xdpyinfo -display "$DISP" >/dev/null 2>&1 && break; sleep 0.1; done
fi
DRV=
[ -n "$CON" ] && { python3 "$ROOT/tools/drivers/con.py" "$PORT" "$CON" > "$OUT/drive.log" 2>&1 & DRV=$!; }
[ -n "$DRIVER" ] && { bash -c "${DRIVER//\{port\}/$PORT}" > "$OUT/drive.log" 2>&1 & DRV=$!; }
cd "$BUILD" || exit 1
env DISPLAY="$DISP" SDL_VIDEODRIVER=x11 MESA_VK_WSI_DEBUG=sw SDL_AUDIODRIVER=dummy AURORA_PRESENT_EVERY=6 \
  MP2_DEBUG_IDS=1 MP2_SKIP_CINEMATICS=1 MP2_SKIP_FRONTEND=1 MP2_SKIP_INTRO=1 MP2_DISC="$DISC" \
  MP2_USER_PATH="$OUT/u" MP2_CACHE_PATH="$OUT/c" MP2_FRAMES="$FRAMES" MP2_GODMODE=1 MP2_WORLD="$WORLD" \
  MP2_CONSOLE="$PORT" "${EXTRA[@]}" COV_SPEC="$SPEC" COV_NM="$OUT/nm.txt" COV_OUT="$OUT" \
  timeout -k 10 "${SECS:-900}" gdb -q -batch -nx \
    -ex 'set pagination off' -ex 'set confirm off' -ex 'handle SIGPIPE SIGUSR1 SIGUSR2 SIG32 SIG33 nostop noprint pass' \
    -ex "source ${COV_GDB_SCRIPT:-$ROOT/tools/cov/force.py}" -ex run -ex 'bt 20' \
    --args ./metroid_prime2_port > "$OUT/game.log" 2>&1 &
GDB=$!
trap 'kill -TERM "$GDB" 2>/dev/null' TERM INT
wait "$GDB"; RC=$?
# gdb's own rc says nothing about the game: -batch returns 1 when the trailing `bt` finds no
# stack (a normal exit), and 0 when it stopped the game at a signal and killed it on quit. Only
# gdb's inferior exit record counts.
if grep -q "^\[Inferior 1 (process [0-9]*) exited normally\]" "$OUT/game.log"; then
  RC=0
elif CODE=$(grep -o "^\[Inferior 1 (process [0-9]*) exited with code [0-9]*\]" "$OUT/game.log" |
              grep -o "code [0-9]*" | tail -1) && [ -n "$CODE" ]; then
  RC=$((10#${CODE#code }))
elif [ "$RC" -ne 124 ]; then
  echo "no normal inferior exit in game.log (stopped at a signal, or killed)" >&2
  RC=1
fi
[ -n "$DRV" ] && kill "$DRV" 2>/dev/null
echo "game rc=$RC"
echo "caught/reach-stub: $(grep -a -c '\[port\] caught\|reach-stub' "$OUT/game.log")"
cat "$OUT/forced.log" 2>/dev/null | head -40
