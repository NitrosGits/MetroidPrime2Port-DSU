#!/bin/bash
# DigitalGuardian area-53 runs (world 1baa96c2, area 53): the guardian layers (3, 11) and
# Activate, then $4 (a bash step file, sourced with C, LOGF, BODY, HEAD) runs. Usage:
#   TPF=8 SECS=900 tools/drivers/dg_area53.sh <tag> <console port|auto> <display|auto> <steps>
# /tmp/opencode/mi_u must be a user dir (save) that boots into the area; each run copies it.
# `auto` takes a per-run slot (a flock on /tmp/opencode/locks/dg53-slot.<n>, held for the script's
# life): port 48900 + n, display :4900 + n. A port already listening or a display already up is
# an error (exit 3) before the game starts, and so is a game that cannot listen on the port.
set -u
TAG=$1; PORT=$2; DISP=$3; STEPS=$4
cd "$(dirname "$0")/../.."
if [ "$PORT" = auto ] || [ "$DISP" = auto ]; then
  mkdir -p /tmp/opencode/locks; SLOTN=
  for n in $(seq 1 32); do
    exec {SLOTFD}>"/tmp/opencode/locks/dg53-slot.$n"
    if flock -n "$SLOTFD"; then SLOTN=$n; break; fi
    exec {SLOTFD}>&-
  done
  [ -z "$SLOTN" ] && { echo "dg_area53: no free slot" >&2; exit 3; }
  [ "$PORT" = auto ] && PORT=$((48900 + SLOTN))
  [ "$DISP" = auto ] && DISP=:$((4900 + SLOTN))
  echo "dg_area53: slot $SLOTN port $PORT display $DISP"
fi
if ss -ltnH "sport = :$PORT" | grep -q .; then echo "dg_area53: console port $PORT is taken" >&2; exit 3; fi
if [ -e "/tmp/.X11-unix/X${DISP#:}" ]; then echo "dg_area53: display $DISP is taken" >&2; exit 3; fi
export MP2_CONSOLE=$PORT MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=${TPF:-4}
Xvfb $DISP -screen 0 1280x720x24 -ac >/dev/null 2>&1 & XPID=$!; sleep 2; xdpyinfo -display $DISP >/dev/null 2>&1 || { echo no-display; kill $XPID; exit 1; }
rm -rf /tmp/opencode/u_$TAG; cp -r /tmp/opencode/mi_u /tmp/opencode/u_$TAG
C() { python3 tools/mp2con.py -p $PORT "$@"; }
# This run's game: the metroid_prime2_port whose environment has RUNTAG=$TAG (not whoever holds
# the port, which may be another run's game).
gamepid() {
  for p in $(pgrep -f '^[^ ]*metroid_prime2_port( |$)'); do
    tr '\0' '\n' < /proc/$p/environ 2>/dev/null | grep -qx "RUNTAG=$TAG" && echo $p
  done | head -1
}
LOGF=/tmp/opencode/$TAG.log
(
  for _ in $(seq 1 200); do grep -q "gameplay reached" $LOGF 2>/dev/null && break; sleep 1; done
  if grep -aq "cannot listen on" $LOGF; then
    echo "dg_area53: the game cannot listen on $PORT; stopping"
    GP=$(gamepid); [ -n "$GP" ] && kill -TERM $GP; exit 3
  fi
  sleep 3
  BODY=u054e; HEAD=u051b
  C "layer 3"; sleep 2; C "layer 11"; sleep 2
  C "send $BODY Activate"; C "send $HEAD Activate"; sleep 2
  C "objs Guardian"
  source $STEPS
  GP=$(gamepid); echo "KILL $GP"; [ -n "$GP" ] && kill -TERM $GP
) > /tmp/opencode/${TAG}_console.log 2>&1 &
printf 'SUP@300[+1400]\n' > /tmp/opencode/${TAG}_input.txt
USERDIR=/tmp/opencode/u_$TAG GODMODE=1 DISPLAY_NUM=$DISP FRAMES=400000 SECS=${SECS:-1500} RUNTAG=$TAG INPUT_FILE=/tmp/opencode/${TAG}_input.txt \
  WORLD=1baa96c2,09_cliff_temple_dark LOG=$LOGF tools/run_gameplay.sh
echo "run rc=$?"
wait
rm -rf /tmp/opencode/u_$TAG /tmp/opencode/c_$TAG
kill $XPID 2>/dev/null
echo done
