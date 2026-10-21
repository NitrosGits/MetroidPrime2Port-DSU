#!/bin/bash
# Coverage run without probes: gdb launches the game (ptrace_scope=1 forbids attaching later) with
# non-stopping breakpoints from a spec (tools/cov/gdb_cov.py), a console driver plays the area, and
# the per-arm hit counts land in <out>/cov.txt (table) and <out>/cov.json.
# See docs/LANE_BRIEF.md, "Coverage without probes".
#
#   tools/cov/run.sh <spec.py> <out dir> <world,area> [options]
#     --con <cmdfile>     drive with tools/drivers/con.py (`@f<n>` lines; `layer <n>` first if needed)
#     --driver '<cmd>'    or any driver command; `{port}` becomes the console port
#     --frames <n>        MP2_FRAMES (default 6000): the game exits after n updates
#     --fast              MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8 (con.py `@f` times still line up)
#     --env K=V           extra game environment (repeatable), e.g. MP2_DEBUG_SPACEPIRATE=1
#   env: BUILD (default build-port-link), DISPLAY_NUM (:491), COV_PORT (49711), SECS (900),
#        COV_MAX (hits recorded per breakpoint, 400), MP2_DISC, COV_SAVE (user dir, /tmp/opencode/mi_u)
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SPEC=$(realpath "$1"); OUT=$(realpath -m "$2"); WORLD=$3; shift 3
CON=; DRIVER=; FRAMES=6000; EXTRA=()
while [ $# -gt 0 ]; do
  case $1 in
    --con) CON=$(realpath "$2"); shift 2 ;;
    --driver) DRIVER=$2; shift 2 ;;
    --frames) FRAMES=$2; shift 2 ;;
    --fast) EXTRA+=(MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8); shift ;;
    --env) EXTRA+=("$2"); shift 2 ;;
    *) echo "unknown option $1" >&2; exit 2 ;;
  esac
done
BUILD=${BUILD:-$ROOT/build-port-link}
DISP=${DISPLAY_NUM:-:491}
PORT=${COV_PORT:-49711}
if ss -ltn 2>/dev/null | grep -q "127.0.0.1:$PORT "; then
  echo "console port $PORT is busy: pick another COV_PORT" >&2; exit 1
fi
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
if [ -n "$CON" ]; then
  python3 "$ROOT/tools/drivers/con.py" "$PORT" "$CON" > "$OUT/drive.log" 2>&1 &
  DRV=$!
elif [ -n "$DRIVER" ]; then
  bash -c "${DRIVER//\{port\}/$PORT}" > "$OUT/drive.log" 2>&1 &
  DRV=$!
fi

cd "$BUILD" || exit 1
env DISPLAY="$DISP" SDL_VIDEODRIVER=x11 MESA_VK_WSI_DEBUG=sw SDL_AUDIODRIVER=dummy AURORA_PRESENT_EVERY=6 \
  MP2_DEBUG_IDS=1 MP2_SKIP_CINEMATICS=1 MP2_SKIP_FRONTEND=1 MP2_SKIP_INTRO=1 MP2_DISC="$DISC" \
  MP2_USER_PATH="$OUT/u" MP2_CACHE_PATH="$OUT/c" MP2_FRAMES="$FRAMES" MP2_GODMODE=1 MP2_WORLD="$WORLD" \
  MP2_CONSOLE="$PORT" "${EXTRA[@]}" COV_SPEC="$SPEC" COV_NM="$OUT/nm.txt" COV_OUT="$OUT" \
  timeout -k 10 "${SECS:-900}" gdb -q -batch -nx \
    -ex 'set pagination off' -ex 'set confirm off' -ex 'handle SIGPIPE SIGUSR1 SIGUSR2 SIG32 SIG33 nostop noprint pass' \
    -ex "source $ROOT/tools/cov/gdb_cov.py" -ex run -ex 'bt 20' \
    --args ./metroid_prime2_port > "$OUT/game.log" 2>&1 &
GDB=$!  # timeout runs in its own process group: stop it (and so gdb and the game) with this script
trap 'kill -TERM "$GDB" 2>/dev/null' TERM INT
wait "$GDB"; RC=$?
[ "$RC" -gt 128 ] && [ "$RC" != 137 ] && { kill -TERM "$GDB" 2>/dev/null; wait "$GDB"; }
[ -n "$DRV" ] && kill "$DRV" 2>/dev/null
[ "$RC" = 124 ] && echo "timeout: SECS=${SECS:-900} ran out"
echo "game: $(head -1 "$OUT/cov.txt" 2>/dev/null) (a signal leaves its backtrace at the end of game.log)"
echo "caught/reach-stub: $(grep -a -c '\[port\] caught\|reach-stub' "$OUT/game.log")"
[ -f "$OUT/cov.txt" ] && tail -n +2 "$OUT/cov.txt" || echo "no cov.txt (gdb did not see the game exit; see $OUT/game.log)"
