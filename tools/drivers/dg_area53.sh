#!/bin/bash
# DigitalGuardian area-53 runs (world 1baa96c2, area 53): the guardian layers (3, 11) and
# Activate, then $4 (a bash step file, sourced with C, LOGF, BODY, HEAD) runs. Usage:
#   TPF=8 SECS=900 tools/drivers/dg_area53.sh <tag> <console port> <display> <steps>
# /tmp/opencode/mi_u must be a user dir (save) that boots into the area; each run copies it.
set -u
TAG=$1; PORT=$2; DISP=$3; STEPS=$4
cd "$(dirname "$0")/../.."
export MP2_CONSOLE=$PORT MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=${TPF:-4}
Xvfb $DISP -screen 0 1280x720x24 -ac >/dev/null 2>&1 & XPID=$!; sleep 2; xdpyinfo -display $DISP >/dev/null 2>&1 || { echo no-display; kill $XPID; exit 1; }
rm -rf /tmp/opencode/u_$TAG; cp -r /tmp/opencode/mi_u /tmp/opencode/u_$TAG
C() { python3 tools/mp2con.py -p $PORT "$@"; }
LOGF=/tmp/opencode/$TAG.log
(
  for _ in $(seq 1 200); do grep -q "gameplay reached" $LOGF 2>/dev/null && break; sleep 1; done
  sleep 3
  BODY=u054e; HEAD=u051b
  C "layer 3"; sleep 2; C "layer 11"; sleep 2
  C "send $BODY Activate"; C "send $HEAD Activate"; sleep 2
  C "objs Guardian"
  source $STEPS
  GP=$(ss -ltnpH "sport = :$PORT" | grep -o 'pid=[0-9]*' | head -1 | cut -d= -f2); echo "KILL $GP"; [ -n "$GP" ] && kill -TERM $GP
) > /tmp/opencode/${TAG}_console.log 2>&1 &
printf 'SUP@300[+1400]\n' > /tmp/opencode/${TAG}_input.txt
USERDIR=/tmp/opencode/u_$TAG GODMODE=1 DISPLAY_NUM=$DISP FRAMES=400000 SECS=${SECS:-1500} RUNTAG=$TAG INPUT_FILE=/tmp/opencode/${TAG}_input.txt \
  WORLD=1baa96c2,09_cliff_temple_dark LOG=$LOGF tools/run_gameplay.sh
echo "run rc=$?"
wait
rm -rf /tmp/opencode/u_$TAG /tmp/opencode/c_$TAG
kill $XPID 2>/dev/null
echo done
