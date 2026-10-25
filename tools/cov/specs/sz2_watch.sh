#!/bin/bash
# sz2 launcher with a stall watchdog: runs tools/cov/force.sh in the foreground; if game.log and
# sz2.jsonl have not grown for $WATCH_IDLE seconds, the game (this worktree's binary only) gets
# SIGSEGV so gdb stops it and prints `bt 20` and `thread apply all bt 12` into game.log.
#   tools/cov/specs/sz2_watch.sh <out dir> [force.sh args after the out dir...]
set -u
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
OUT=$1; shift
mkdir -p "$OUT"
"$ROOT/tools/cov/force.sh" "$ROOT/tools/cov/specs/sz2_think.py" "$OUT" "$@" &
RUN=$!
IDLE=${WATCH_IDLE:-120}; last=""; since=$SECONDS; fired=0
while kill -0 "$RUN" 2>/dev/null; do
  sleep 5
  now="$(stat -c %s "$OUT/game.log" "$OUT/sz2.jsonl" 2>/dev/null | tr '\n' ' ')"
  if [ "$now" != "$last" ]; then last=$now; since=$SECONDS; continue; fi
  if [ $((SECONDS - since)) -ge "$IDLE" ] && [ "$fired" = 0 ]; then
    PID=; for p in $(pgrep -x metroid_prime2_); do
      [ "$(readlink -f /proc/$p/exe)" = "$(readlink -f "$ROOT/build-port-link/metroid_prime2_port")" ] && PID=$p; done
    echo "watchdog: no output for ${IDLE}s, SIGSEGV to game pid ${PID:-none}" | tee "$OUT/watchdog.log"
    { date; ps -eo pid,ppid,stat,wchan:24,pcpu,etime,cmd --forest | grep -E "[g]db|[m]etroid_prime2_|[t]ools/cov"
      for p in $PID $(pgrep -f "^gdb -q -batch -nx"); do
        echo "== $p"; cat /proc/$p/status 2>/dev/null | grep -E "^(Name|State|SigQ|SigPnd|ShdPnd|TracerPid)"
        for t in /proc/$p/task/*; do echo "$(basename $t) $(cut -d' ' -f2,3 $t/stat) $(cat $t/wchan 2>/dev/null)"; done
      done; } > "$OUT/procdump.log" 2>&1
    [ -n "$PID" ] && kill -SEGV "$PID"
    fired=1
  fi
done
wait "$RUN"
