#!/bin/bash
# One batch-smoke family run (tools/smoke/run.sh calls it; that script holds the family table).
# Read-only: no source file is touched, nothing is committed.
#
#   fam.sh <tag>
#
# The run's shape comes from the environment (run.sh sets it):
#   SMOKE_OUT      output dir: log_<tag>.log, con_<tag>.out, u_<tag>/ (the card copy), c_<tag>/
#   SMOKE_BUILD    build dir that holds metroid_prime2_port
#   SMOKE_DISPLAY  X display for this run (run_gameplay.sh starts and stops its own Xvfb there)
#   SMOKE_PORT     MP2_CONSOLE port for this run
#   SMOKE_WORLD    "<world>,<area>" or "-" for the save's own world
#   SMOKE_FRAMES   game updates to run
#   SMOKE_SECS     backstop timeout in seconds
#   SMOKE_CMD      console command file, or "-" for no console
#   SMOKE_INPUT    MP2_INPUT script, or "-" for none
#   SMOKE_DRIVER   console driver: generic (tools/drivers/con.py) | ing | ep | darksamus
#   SMOKE_SAVE     the memory card that is copied as this run's user dir
#
# Extra "NAME=VALUE" arguments go into the game's environment (e.g. MP2_BG_FSM=1).
set -u

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
TAG=${1:?usage: fam.sh <tag>}
shift
OUT=${SMOKE_OUT:?SMOKE_OUT must be set}
BUILD=${SMOKE_BUILD:-$ROOT/build-port-link}
DISP=${SMOKE_DISPLAY:?SMOKE_DISPLAY must be set}
PORT=${SMOKE_PORT:?SMOKE_PORT must be set}
WORLD=${SMOKE_WORLD:--}
FRAMES=${SMOKE_FRAMES:-2000}
SECS=${SMOKE_SECS:-280}
CMDF=${SMOKE_CMD:--}
INPF=${SMOKE_INPUT:--}
KIND=${SMOKE_DRIVER:-generic}
SAVE=${SMOKE_SAVE:-/tmp/opencode/mi_u}

LOG=$OUT/log_${TAG}.log
CON=$OUT/con_${TAG}.out
CACHE=$OUT/c_${TAG}
USER=$OUT/u_${TAG}
RCFILE=$OUT/${TAG}.rc

mkdir -p "$OUT" || exit 2
rm -rf "$USER" "$CACHE"
mkdir -p "$CACHE"
if [ ! -d "$SAVE" ]; then
  echo "[fam] no save at $SAVE (the memory card to copy); set SMOKE_SAVE" >&2
  exit 2
fi
cp -r "$SAVE" "$USER" || exit 2

# A game left behind by a killed script would keep a run slot and an Xvfb: find it by the user
# dir it runs with (no other run uses it) and end it.
kill_game() {
  local p pid
  for p in /proc/[0-9]*; do
    pid=${p#/proc/}
    [ "$pid" = "$$" ] && continue
    tr '\0' '\n' 2>/dev/null < "$p/environ" | grep -qx "MP2_USER_PATH=$USER" && kill -TERM "$pid" 2>/dev/null
  done
  return 0
}

CONPID=
cleanup() {
  [ -n "$CONPID" ] && kill "$CONPID" 2>/dev/null
  kill_game
}
trap cleanup EXIT

export MP2_CONSOLE=$PORT

case "$KIND" in
  generic)
    # tools/drivers/con.py sends the command file's lines and answers when the game is up.
    if [ "$CMDF" != "-" ]; then
      ( sleep 2; python3 "$ROOT/tools/drivers/con.py" "$PORT" "$CMDF" "$CON" >/dev/null 2>&1 ) &
      CONPID=$!
    fi
    ;;
  ing|ep)
    # these drivers wait for "gameplay reached" in the game log themselves.
    ( sleep 2; python3 "$HERE/drivers/${KIND}_con.py" "$LOG" 110 > "$CON" 2>&1 ) &
    CONPID=$!
    ;;
  darksamus)
    ( sleep 4; python3 "$HERE/drivers/darksamus_con.py" "$PORT" 110 "$LOG" > "$CON" 2>&1 ) &
    CONPID=$!
    ;;
  *)
    echo "[fam] unknown driver kind $KIND" >&2
    exit 2
    ;;
esac

ENVS=(GODMODE=1 DISPLAY_NUM="$DISP" FRAMES="$FRAMES" SECS="$SECS" RUNTAG="smoke_${TAG}"
      CACHEDIR="$CACHE" LOG="$LOG" USERDIR="$USER" BUILD="$BUILD")
[ "$WORLD" != "-" ] && ENVS+=("WORLD=$WORLD")
[ "$INPF" != "-" ] && ENVS+=("INPUT_FILE=$INPF")
for kv in "$@"; do ENVS+=("$kv"); done

env "${ENVS[@]}" "$ROOT/tools/run_gameplay.sh"
RC=$?
echo "gameplay rc=$RC"

# The driver may still be waiting for a console that never came up (a crashed game): give it a
# minute, then stop it, so a family never outlives its game by much.
if [ -n "$CONPID" ]; then
  for _ in $(seq 1 120); do
    kill -0 "$CONPID" 2>/dev/null || break
    sleep 0.5
  done
  if kill -0 "$CONPID" 2>/dev/null; then
    echo "[fam] console driver still running; stopping it"
    kill "$CONPID" 2>/dev/null
  fi
  wait "$CONPID" 2>/dev/null
  CONPID=
fi

echo "=== summary $TAG ==="
echo "exit=$RC"
if [ -f "$LOG" ]; then
  grep -a '\[harness\] gameplay reached\|no game update for\|\[harness\] the first game update' "$LOG" | tail -4
  echo "caught/reach-stub: $(grep -a -c '\[port\] caught\|reach-stub' "$LOG")"
  echo "stand-in lines:   $(grep -a -c 'is a port stand-in' "$LOG")"
else
  echo "(no log at $LOG)"
fi
echo "$RC" > "$RCFILE"
exit $RC
