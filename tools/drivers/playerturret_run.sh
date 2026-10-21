#!/bin/bash
# Run the player turret in the Agon Wastes (sand_world, 06_sand_alamofront, layer 3) and drive it
# with tools/drivers/playerturret_drive.py: the driver finds the turret, stands the player on it
# (the trigger that activates the turret), fires, and sends the two script messages the game's own
# flow never sends to a player turret (Stop and Damage).
#
#   playerturret_run.sh [out dir] [build dir] [NAME=VALUE ...]
#
# PHASE=main (the default) walks the message cases the game never sends to a player turret;
# PHASE=stop ends with the kSM_Stop case, which needs a player in the turret and kills it.
#
# The extra NAME=VALUE pairs go into the game's environment (e.g. MP2_TURRET_MP=1 forces the two
# multiplayer arms of the module, which the port cannot reach on its own).  The run's files are
# <out>/game.log (the game's log), <out>/con.out (the console transcript) and <out>/run.log.
set -u

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
OUT=${1:-/tmp/opencode/pt_turret}
BUILD=${2:-$ROOT/build-port-link}
shift 2 2>/dev/null || true
PORT=${MP2_TURRET_PORT:-49722}
DISP=${MP2_TURRET_DISPLAY:-:88}
USERDIR=${USERDIR:-/tmp/opencode/mi_u}
FRAMES=${FRAMES:-3000}

mkdir -p "$OUT" || exit 2
rm -f "$OUT/game.log" "$OUT/con.out" "$OUT/run.log"

ENVS=("MP2_CONSOLE=$PORT" "WORLD=sand_world,06_sand_alamofront" "FRAMES=$FRAMES" "GODMODE=1"
      "DISPLAY_NUM=$DISP" "RUNTAG=pt_turret" "LOG=$OUT/game.log" "CACHEDIR=$OUT/cache"
      "USERDIR=$USERDIR" "BUILD=$BUILD")
# The extra assignments come last, so they win over the defaults above.
for kv in "$@"; do ENVS+=("$kv"); done

env "${ENVS[@]}" "$ROOT/tools/run_gameplay.sh" > "$OUT/run.log" 2>&1 &
GAME=$!
# The console comes up with the game; the driver waits for it.
( sleep 2; python3 "$HERE/playerturret_drive.py" "$PORT" "$OUT/con.out" "${PHASE:-main}" ) &
DRIVER=$!
wait "$GAME"
RC=$?
# The driver may still be waiting for a console that never came up: stop it.
kill "$DRIVER" 2>/dev/null
wait "$DRIVER" 2>/dev/null
echo "run: rc=$RC out=$OUT"
grep -a '\[harness\] gameplay reached\|no game update for\|the frame limit is up' "$OUT/game.log" | tail -3
echo "caught/reach-stub: $(grep -a -c '\[port\] caught\|reach-stub' "$OUT/game.log" 2>/dev/null)"
exit "$RC"
