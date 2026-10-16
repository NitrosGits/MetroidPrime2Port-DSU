#!/bin/bash
# Grab one frame of a test run's X display, so a run can show what the game looked like: a rendered
# area, a fade, or a menu that is holding the run up. Pair it with `tools/run_gameplay.sh`, which needs
# an X display (Xvfb :86 -screen 0 1280x720x24 -ac) and prints the phase times a shot can be timed to
# ("[harness] the first game update is at ... s").
#
#   tools/shot.sh <display-number> <out.png> [seconds-to-wait]
set -u
D=${1:?usage: shot.sh <display-number> <out.png> [seconds-to-wait]}
OUT=${2:?usage: shot.sh <display-number> <out.png> [seconds-to-wait]}
SLEEP=${3:-0}
D=${D#:}                       # accept both ":86" and "86"
SIZE=${SIZE:-1280x720}         # the size the test display is created with (Xvfb -screen 0 ...)
[ "$SLEEP" != "0" ] && sleep "$SLEEP"
DISPLAY=":$D" ffmpeg -loglevel error -f x11grab -video_size "$SIZE" -i ":$D" -frames:v 1 -y "$OUT"
ls -l "$OUT"
