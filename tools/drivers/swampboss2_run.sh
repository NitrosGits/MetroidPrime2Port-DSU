#!/bin/bash
# Run the SwampBossStage2 driver: starts the port with the debug console and drives it.
#
#   tools/drivers/swampboss2_run.sh [driver args...]
#
# The game runs with MP2_TICKS_PER_FRAME (8 by default, ~3.5x real time) and the console on
# MP2_CONSOLE; the driver (tools/drivers/swampboss2_chain.py) waits for the boss, sends it the
# kSM_AreaLoaded message and forces its AI states.
set -u
WT=$(cd "$(dirname "$0")/../.." && pwd)
D=${SB2_DIR:-/tmp/opencode/sb2dark}
DISP=${DISP:-:511}
PORT=${PORT:-51790}
TAG=${TAG:-sb2}
FRAMES=${FRAMES:-3000}
SECS=${SECS:-180}
TICKS=${TICKS:-8}
mkdir -p "$D"
pgrep -x Xvfb -a | grep -q -- "Xvfb $DISP " || (Xvfb "$DISP" -screen 0 1280x720x24 -ac >"$D/xvfb${DISP#:}.log" 2>&1 &)
sleep 1
xdpyinfo -display "$DISP" >/dev/null 2>&1 || { echo "no display $DISP"; exit 1; }
rm -rf "$D/u_$TAG"
cp -r /tmp/opencode/mi_u "$D/u_$TAG"
mkdir -p "$D/c_$TAG"
export MP2_CONSOLE=$PORT
export MP2_TICKS_PER_FRAME=$TICKS
cd "$WT" || exit 1
env USERDIR="$D/u_$TAG" GODMODE=1 DISPLAY_NUM="$DISP" FRAMES="$FRAMES" SECS="$SECS" \
    RUNTAG="sb2_$TAG" CACHEDIR="$D/c_$TAG" LOG="$D/log_$TAG.log" \
    WORLD=3dfd2249,04_swamp_temple_dark \
    tools/run_gameplay.sh &
GAME=$!
sleep 2
python3 "$WT/tools/drivers/swampboss2_chain.py" "$PORT" "$@" > "$D/driver_$TAG.out" 2>&1
kill "$GAME" 2>/dev/null
wait "$GAME" 2>/dev/null
echo "=== driver ==="
cat "$D/driver_$TAG.out"
echo "=== arms ==="
grep -a -o "\[arm\] [0-9]* .*" "$D/log_$TAG.log" | sort -u
