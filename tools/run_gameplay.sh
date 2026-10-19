#!/bin/bash
# One test run of the port: skip the front end, boot the memory card's save, optionally restart into
# another world, drive the game with an `MP2_INPUT` script, and end the run cleanly at `FRAMES` game
# updates. See platform/README.md, "Test runs: skipping the front end".
#
#   USERDIR=<dir holding the card> [WORLD=<world>[,<area>]] [INPUT_FILE=<script>] [FRAMES=<n>]
#   [SKIP_INTRO=0|1] (default 1: MP2_SKIP_INTRO, the port gets to a controllable player on its own)
#   [SECS=<timeout>] [DISPLAY_NUM=:<n>] [CACHEDIR=<dir>] [TELEPORT=<area>] [RUNTAG=<tag>]
#   [GODMODE=1] (MP2_GODMODE: the player takes no damage; default off, an ordinary run takes damage)
#   [LOG=<file>] [ASAN=1] [BUILD=<build dir>] tools/run_gameplay.sh
#
# `WORLD`/`TELEPORT`/the `MP2_INPUT` tokens are hex asset ids or names; the player's own run length is
# `FRAMES` (game updates, not loop iterations). `SECS` is the backstop that ends a run which never
# reaches the limit.
#
# Two ways a run quietly tests the wrong thing, both seen for real, so both are checked here:
#   * a user dir without the memory card falls back to the front end, and the run then plays the front
#     end's world instead of the save's - a fresh MP2_USER_PATH does exactly that;
#   * an `MP2_INPUT` script that presses `START` in game opens the pause menu, and a paused game
#     performs no `CStateManager::Update`, so `MP2_FRAMES` is never reached and only `SECS` ends the
#     run (the harness reports `[harness] no game update for ... s` when that happens).
set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
USERDIR=${USERDIR:-}
INPUT_FILE=${INPUT_FILE:-}
LOG=${LOG:-/tmp/opencode/log_${RUNTAG:-run}.log}
CACHEDIR=${CACHEDIR:-/tmp/opencode/c_${RUNTAG:-run}}
DISPLAY_NUM=${DISPLAY_NUM:-:86}
DISC=${MP2_DISC:-/run/media/odran/Leo/Portable/roms/gc/Metroid Prime 2 - Echoes.iso}
# The script changes directory into the build below, so a relative log or cache path would land there.
case "$LOG" in /*) ;; *) LOG=$PWD/$LOG ;; esac
case "$CACHEDIR" in /*) ;; *) CACHEDIR=$PWD/$CACHEDIR ;; esac

if [ -z "$USERDIR" ] || [ ! -d "$USERDIR" ]; then
  echo "[tools] USERDIR must name a directory that holds the memory card (see platform/README.md)" >&2
  exit 2
fi
[ -d "$USERDIR/USA" ] || echo "[tools] warning: $USERDIR has no USA/ - the run may fall back to the front end" >&2
[ -f "$DISC" ] || echo "[tools] warning: the disc image $DISC does not exist (set MP2_DISC)" >&2
if [ -n "$INPUT_FILE" ] && [ ! -f "$INPUT_FILE" ]; then
  echo "[tools] INPUT_FILE=$INPUT_FILE does not exist" >&2
  exit 2
fi
if [ -n "$INPUT_FILE" ] && grep -qE '(^|,)START@' "$INPUT_FILE"; then
  echo "[tools] warning: $INPUT_FILE presses START - in game that opens the pause menu, and a paused" >&2
  echo "[tools]          game stops updating, so MP2_FRAMES is never reached (platform/README.md)" >&2
fi
mkdir -p "$CACHEDIR"   # an absent cache dir only costs shader-cache errors in the log

IN=""
[ -n "$INPUT_FILE" ] && IN=$(cat "$INPUT_FILE")

if [ "${ASAN:-}" = "1" ]; then
  BUILD=$ROOT/build-port-asan
  export ASAN_OPTIONS="detect_leaks=0:alloc_dealloc_mismatch=0:abort_on_error=0:halt_on_error=0:log_path=/tmp/opencode/asan_${RUNTAG:-run}:print_stacktrace=0:malloc_context_size=24"
  export GLIBC_TUNABLES=
else
  BUILD=${BUILD:-$ROOT/build-port-link}
fi
cd "$BUILD" || exit 1

rm -f "$LOG"
WORLD_DESC=${WORLD:-the-save-world}
echo "run: tag=${RUNTAG:-run} log=$LOG build=$BUILD secs=${SECS:-460} display=$DISPLAY_NUM world=$WORLD_DESC frames=${FRAMES:-16000} input=$(printf '%s' "$IN" | wc -c) bytes user=$USERDIR at $(date +%T)" > "$LOG.info"

envs=(
  "MP2_DEBUG_IDS=1" "MP2_SKIP_CINEMATICS=1" "MP2_SKIP_FRONTEND=1" "MP2_SKIP_INTRO=${SKIP_INTRO:-1}" "MP2_DISC=$DISC"
  "MP2_INPUT=$IN" "MP2_USER_PATH=$USERDIR" "MP2_CACHE_PATH=$CACHEDIR"
  "MP2_FRAMES=${FRAMES:-16000}"
)
[ -n "${GODMODE:-}" ] && envs+=("MP2_GODMODE=$GODMODE")
[ -n "${WORLD:-}" ] && envs+=("MP2_WORLD=$WORLD")
[ -n "${TELEPORT:-}" ] && envs+=("MP2_TELEPORT=$TELEPORT")

DISPLAY=$DISPLAY_NUM SDL_VIDEODRIVER=x11 MESA_VK_WSI_DEBUG=sw SDL_AUDIODRIVER=dummy \
  env "${envs[@]}" timeout -k 10 "${SECS:-460}" ./metroid_prime2_port > "$LOG" 2>&1 &
GAME=$!
# STOP_ON_STALL=1: a paused game (a save screen with nothing to answer it, the ending's "Save
# progress?" prompt in Temple Grounds 58) never reaches MP2_FRAMES, so only the timeout would end
# the run. The harness's "no game update for" line is the signal; end the run on it, exit 125.
STALLED=
if [ -n "${STOP_ON_STALL:-}" ]; then
  ( while kill -0 "$GAME" 2>/dev/null; do
      if grep -q '^\[harness\] no game update for' "$LOG" 2>/dev/null; then
        echo "[gameplay] STOP_ON_STALL: the game stopped updating; ending the run early" >> "$LOG"
        kill -TERM "$GAME"; break
      fi
      sleep 2
    done ) &
  WATCH=$!
fi
wait "$GAME"
RC=$?
if [ -n "${WATCH:-}" ]; then
  kill "$WATCH" 2>/dev/null
  grep -q '^\[gameplay\] STOP_ON_STALL' "$LOG" && RC=125
fi
echo "exit=$RC at $(date +%T)" >> "$LOG"
# The run's own exit code, so a caller can chain on it: 125 is a STOP_ON_STALL early end, 0 is the game's clean quit path (MP2_FRAMES),
# 124 the backstop `timeout`, 139 a crash and 134 an abort.
exit $RC
