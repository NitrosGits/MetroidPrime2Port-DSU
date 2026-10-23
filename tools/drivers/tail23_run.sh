#!/bin/bash
# usage: tail23_run.sh <tag> <display> <console port> <cmdfile> [frames] [secs]
# One tail23 coverage run: 0f_cliff_hall, GODMODE, fast mode, its own card copy, cache and gcov dir.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TAG=$1 DISP=$2 PORT=$3 CMD=$4 FRAMES=${5:-2600} SECS=${6:-320}
OUT=/tmp/opencode/tail23
mkdir -p "$OUT"
USERDIR=$OUT/user_$TAG
rm -rf "$USERDIR" && mkdir -p "$USERDIR" && cp -a /tmp/opencode/mi_u/. "$USERDIR"/
cd "$ROOT" || exit 1
"$ROOT/tools/games_running.sh" --wait
export MP2_CONSOLE="$PORT" MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8 GODMODE=1
export DISPLAY_NUM="$DISP" FRAMES="$FRAMES" SECS="$SECS" WORLD="1baa96c2,42c85ad9"
export BUILD="$ROOT/build-port-cov"
export LOG="$OUT/$TAG.log" CACHEDIR="$OUT/c_$TAG" RUNTAG="$TAG" USERDIR="$USERDIR"
"$ROOT/tools/run_gameplay.sh" &
GAME=$!
python3 "$ROOT/tools/drivers/con.py" "$PORT" "$CMD" "$OUT/$TAG.con.log" &
CON=$!
# The driver's own connect loop waits 10 min: end it as soon as the game is gone.
while kill -0 "$CON" 2>/dev/null; do
  if ! kill -0 "$GAME" 2>/dev/null; then sleep 2; kill "$CON" 2>/dev/null; break; fi
  sleep 2
done
wait "$CON" 2>/dev/null
wait "$GAME"; RC=$?
echo "gameplay rc=$RC" >> "$OUT/$TAG.con.log"
echo "[tail23] $TAG rc=$RC  log=$LOG  con=$OUT/$TAG.con.log"
exit $RC
