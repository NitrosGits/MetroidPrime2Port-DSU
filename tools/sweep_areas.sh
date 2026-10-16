#!/bin/bash
# Area regression sweep: boots the save's own world (no `MP2_WORLD`), teleports each shard to its
# first area with `MP2_TELEPORT`, then walks the rest of its slice with `NEXTAREA` tokens and ends
# every run with `MP2_FRAMES`. Built on `tools/run_gameplay.sh` (see its header for the traps it
# checks: a user dir without the memory card, and an input script that presses `START` - neither can
# happen here, the generated script has no button tokens that open a menu).
#
#   USERDIR=<dir holding the card> [SHARD=<n>|all] [SHARDS=4] [COUNT=63] [DWELL=1400]
#   [TAIL=1000] [HEAL=400] [TAG=sweep] [DISPLAY_BASE=86] [ASAN=1] tools/sweep_areas.sh
#
# `COUNT` is the number of area *slots* in the world (Temple Grounds has 63, slots 0..62); the sweep
# asks for them by index, which is what `[warp] world <id> areas:` prints and what the warp's `kArea`
# handler takes as a plain number. `SHARDS` slices `COUNT` into contiguous runs so a full sweep is
# several short runs on several displays instead of one long one; `SHARD=all` (the default) launches
# all of them in parallel and waits.
#
# Every area gets `DWELL` loop iterations: the `NEXTAREA` that asks for it plus, `HEAL` iterations
# later, a heal (a spawn in a hazard should not end the sweep) and then the rest of the dwell in the
# area, which is the gameplay the sweep is for. Tokens are counted in `PortPumpEvents` iterations,
# the same counter `MP2_INPUT` always uses (~60/s against ~54 game updates/s measured).
set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
USERDIR=${USERDIR:-}
COUNT=${COUNT:-63}
SHARDS=${SHARDS:-4}
DWELL=${DWELL:-1400}
TAIL=${TAIL:-1000}
HEAL=${HEAL:-400}
TAG=${TAG:-sweep}
DISPLAY_BASE=${DISPLAY_BASE:-86}
LOGDIR=${LOGDIR:-/tmp/opencode}
SHARD=${SHARD:-all}

if [ -z "$USERDIR" ] || [ ! -d "$USERDIR" ]; then
  echo "[sweep] USERDIR must name a directory that holds the memory card" >&2
  exit 2
fi
[ -d "$USERDIR/USA" ] || echo "[sweep] warning: $USERDIR has no USA/ - a run would fall back to the front end" >&2
command -v python3 >/dev/null || { echo "[sweep] python3 is needed for the slice arithmetic" >&2; exit 2; }
mkdir -p "$LOGDIR"
# A fresh full sweep starts a fresh manifest; a single shard must not wipe its siblings' lines.
[ "$SHARD" = "all" ] && rm -f "$LOGDIR/sweep_$TAG.txt"

# slice(i) -> "<first> <count>": the contiguous run of area slots shard i walks.
slice() {
  python3 - "$COUNT" "$SHARDS" "$1" <<'PY'
import sys
count, shards, i = (int(v) for v in sys.argv[1:4])
per = -(-count // shards)
first = i * per
print(first, max(0, min(per, count - first)))
PY
}

# The generated script for one shard: no button tokens at all, only warp tokens and heals.
write_input() { # <first> <n> <path>
  python3 - "$1" "$2" "$DWELL" "$HEAL" "$3" <<'PY'
import sys
first, n, dwell, heal = (int(v) for v in sys.argv[1:5])
path = sys.argv[5]
tokens = []
for k in range(1, n):
    t = k * dwell
    tokens.append("NEXTAREA@%d" % t)
    tokens.append("HEAL@%d" % (t + heal))
open(path, "w").write(",".join(tokens) + "\n")
print(len(tokens))
PY
}

# The manifest is what makes the report able to say whether an unplayed area is a hole in the token
# cycle or simply the point a run died at: the logs alone cannot tell those apart.
manifest_write() { # <index> <first> <count> <frames> <input> <log>
  printf 'shard %s first %s count %s frames %s input %s log %s\n' "$1" "$2" "$3" "$4" "$5" "$6" \
    >> "$LOGDIR/sweep_$TAG.txt"
}

run_shard() { # <index>
  local i=$1 first n frames
  read -r first n <<<"$(slice "$i")"
  if [ "$n" -le 0 ]; then
    echo "[sweep] shard $i: nothing to do"
    return 0
  fi
  frames=$(( ( n - 1 ) * DWELL + TAIL ))
  local input="$LOGDIR/in_${TAG}_$i.txt"
  local tokens
  tokens=$(write_input "$first" "$n" "$input")
  # 32 KiB per shard, so parallel shards cannot race on one memory card.
  local user="$LOGDIR/u_${TAG}_$i"
  rm -rf "$user"
  cp -r "$USERDIR" "$user"
  local display=$(( DISPLAY_BASE + i ))
  local secs=$(( frames / 35 + 240 ))
  manifest_write "$i" "$first" "$n" "$frames" "$input" "$LOGDIR/log_${TAG}_$i.log"
  echo "[sweep] shard $i: areas $first..$(( first + n - 1 )) ($n), $frames updates, display :$display," \
       "$tokens tokens, log $LOGDIR/log_${TAG}_$i.log"
  RUNTAG="${TAG}_$i" DISPLAY_NUM=":$display" USERDIR="$user" INPUT_FILE="$input" \
    TELEPORT="$first" FRAMES="$frames" SECS="$secs" LOG="$LOGDIR/log_${TAG}_$i.log" \
    ASAN="${ASAN:-}" "$ROOT/tools/run_gameplay.sh"
  local rc=$?
  echo "[sweep] shard $i: exit=$rc (0 clean, 124 timeout, 134 abort, 139 crash)"
  return $rc
}

if [ "$SHARD" != "all" ]; then
  run_shard "$SHARD"
  exit $?
fi

pids=()
for i in $(seq 0 $(( SHARDS - 1 ))); do
  run_shard "$i" &
  pids+=($!)
done
rc=0
for p in "${pids[@]}"; do
  wait "$p" || rc=1
done
echo "[sweep] all shards done; report: python3 tools/sweep_report.py $LOGDIR/log_${TAG}_*.log"
exit $rc
