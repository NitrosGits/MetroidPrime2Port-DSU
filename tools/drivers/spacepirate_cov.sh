#!/bin/bash
# SpacePirate coverage run without probes: gdb launches the game (ptrace_scope=1 forbids attaching
# later) with non-stopping entry breakpoints from spacepirate_cov.py, spacepirate_drive.py drives
# the fight over the console, and the per-function arm counts land in <out>/cov.json.
#
#   tools/drivers/spacepirate_cov.sh <out dir> [world,area] [layer|-] [rounds] [frames]
#   env: BUILD (default build-port-link), DISPLAY_NUM (:491), SP_PORT (49711), SECS (900)
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$1; WORLD=${2:-42b935e4,08_sand_casualpirates}; LAYER=${3:--}; ROUNDS=${4:-8}; FRAMES=${5:-6000}
BUILD=${BUILD:-$ROOT/build-port-link}
DISP=${DISPLAY_NUM:-:491}
PORT=${SP_PORT:-49711}
DISC=${MP2_DISC:-/run/media/odran/Leo/Portable/roms/gc/Metroid Prime 2 - Echoes.iso}
mkdir -p "$OUT/c"
rm -rf "$OUT/u" && cp -r /tmp/opencode/mi_u "$OUT/u"
nm "$BUILD/metroid_prime2_port" > "$OUT/nm.raw"
nm -C "$BUILD/metroid_prime2_port" | cut -d' ' -f3- > "$OUT/nm.dem"
paste "$OUT/nm.raw" "$OUT/nm.dem" > "$OUT/nm.txt"

XVFB=
if ! xdpyinfo -display "$DISP" >/dev/null 2>&1; then
  Xvfb "$DISP" -screen 0 1280x720x24 -ac > "$OUT/xvfb.log" 2>&1 &
  XVFB=$!
  trap 'kill "$XVFB" 2>/dev/null' EXIT
  for _ in $(seq 1 50); do xdpyinfo -display "$DISP" >/dev/null 2>&1 && break; sleep 0.1; done
fi

python3 "$ROOT/tools/drivers/spacepirate_drive.py" "$PORT" "$LAYER" "$ROUNDS" > "$OUT/drive.log" 2>&1 &
DRV=$!

cd "$BUILD" || exit 1
DISPLAY=$DISP SDL_VIDEODRIVER=x11 MESA_VK_WSI_DEBUG=sw SDL_AUDIODRIVER=dummy AURORA_PRESENT_EVERY=6 \
  MP2_DEBUG_IDS=1 MP2_SKIP_CINEMATICS=1 MP2_SKIP_FRONTEND=1 MP2_SKIP_INTRO=1 MP2_DISC="$DISC" \
  MP2_USER_PATH="$OUT/u" MP2_CACHE_PATH="$OUT/c" MP2_FRAMES="$FRAMES" MP2_GODMODE=1 MP2_WORLD="$WORLD" \
  MP2_CONSOLE="$PORT" MP2_DEBUG_SPACEPIRATE=1 SP_COV_NM="$OUT/nm.txt" SP_COV_OUT="$OUT/cov.json" \
  timeout -k 10 "${SECS:-900}" gdb -q -batch -nx \
    -ex 'set pagination off' -ex 'set confirm off' -ex 'handle SIGPIPE SIGUSR1 SIGUSR2 SIG32 SIG33 nostop noprint pass' \
    -ex "source $ROOT/tools/drivers/spacepirate_cov.py" -ex run -ex 'bt 20' \
    --args ./metroid_prime2_port > "$OUT/game.log" 2>&1
RC=$?
kill "$DRV" 2>/dev/null
echo "exit=$RC"
echo "caught/reach-stub: $(grep -a -c '\[port\] caught\|reach-stub' "$OUT/game.log")"
