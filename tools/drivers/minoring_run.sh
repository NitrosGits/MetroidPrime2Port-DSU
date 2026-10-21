#!/bin/bash
# One MinorIng run in the Agon Wastes' 0a_sand_hall_dark (the sand world, area 13 of 63) - the
# area the module's inglets and its massive death live in.  It runs the game with
# MP2_DEBUG_MINORING=1 (the port's own log) and prints the module's lines, the number of
# "[port] caught" (the port's own errors) and "[reach-stub]" (a body the port does not have).
#
#   USERDIR=<dir with the memory card> DISPLAY_NUM=:<n> FRAMES=<n> SECS=<n>
#   tools/drivers/minoring_run.sh
#
# The memory card must be the one the save is on (the port skips the front end and boots the
# card's save), so USERDIR must hold the same card the other runs use.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
USERDIR=${USERDIR:-/tmp/opencode/mi_u}
DISPLAY_NUM=${DISPLAY_NUM:-:97}
FRAMES=${FRAMES:-5000}
SECS=${SECS:-220}
WORLD=${WORLD:-3bfa3eff,686f5a1b}
GODMODE=${GODMODE:-1}
LOG=${LOG:-/tmp/opencode/minoring_run.log}
RUNTAG=${RUNTAG:-minoring}

MP2_DEBUG_MINORING=1 USERDIR="$USERDIR" DISPLAY_NUM="$DISPLAY_NUM" FRAMES="$FRAMES" SECS="$SECS" \
  WORLD="$WORLD" GODMODE="$GODMODE" LOG="$LOG" RUNTAG="$RUNTAG" \
  "$ROOT/tools/run_gameplay.sh"
RC=$?

echo "--- run exit=$RC, log=$LOG"
echo "--- [minoring] lines: $(grep -c '^\[minoring\]' "$LOG" 2>/dev/null)"
grep '^\[minoring\]' "$LOG" 2>/dev/null | head -60
echo "--- [port] caught: $(grep -c '^\[port\] caught' "$LOG" 2>/dev/null)"
grep '^\[port\] caught' "$LOG" 2>/dev/null | head -20
echo "--- [reach-stub]: $(grep -c '\[reach-stub\]' "$LOG" 2>/dev/null)"
grep '\[reach-stub\]' "$LOG" 2>/dev/null | head -20
exit $RC
