#!/bin/bash
# Batch smoke suite for the MP2 port: every family area boots into gameplay, runs a console script
# and reports what it saw. The families run concurrently, each on its own X display, console port,
# card copy and cache dir.
#
#   tools/smoke/run.sh [--build <dir>] [--out <dir>] [--jobs N] [--save <dir>]
#                      [--display-base <n>] [--headless] [--list] [family ...]
#
#   --build         build dir with metroid_prime2_port (default <repo>/build-port-link)
#   --out           output dir (default /tmp/opencode/smoke-<git short hash>)
#   --jobs          families to run at once (default 4). Every game takes one of the machine-wide
#                   run slots (tools/run_gameplay.sh -> slotrun.sh), so this is not the real cap.
#   --save          the memory card each family copies as its user dir (default /tmp/opencode/mi_u)
#   --display-base  first X display number used (default 760)
#   --headless      no picture: no Xvfb, no GPU device (HEADLESS=1 for tools/run_gameplay.sh,
#                   docs/RUNNING.md "Headless runs"); also SMOKE_HEADLESS=1
#   --list          print the family table and exit
#   --capped        run every family under the 60 FPS cap (SMOKE_FAST=0 does the same)
#
# With no family names every family runs. Per family it writes <out>/log_<family>.log (the game's
# log), <out>/con_<family>.out (the console transcript), <out>/u_<family>/ (the card copy),
# <out>/c_<family>/ (the shader cache) and <out>/<family>.rc (the run's exit code).
# tools/smoke/compare.py compares two output dirs.
set -u

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)

BUILD=$ROOT/build-port-link
OUT=
JOBS=${SMOKE_JOBS:-4}
SAVE=${SMOKE_SAVE:-/tmp/opencode/mi_u}
DISPBASE=${SMOKE_DISPLAY_BASE:-760}
LIST=0
FAST=${SMOKE_FAST:-1}
HEADLESS=${SMOKE_HEADLESS:-0}
FAMS=()

while [ $# -gt 0 ]; do
  case "$1" in
    --build) BUILD=$2; shift 2 ;;
    --out) OUT=$2; shift 2 ;;
    --jobs) JOBS=$2; shift 2 ;;
    --save) SAVE=$2; shift 2 ;;
    --display-base) DISPBASE=$2; shift 2 ;;
    --list) LIST=1; shift ;;
    --capped) FAST=0; shift ;;
    --headless) HEADLESS=1; shift ;;
    -h|--help) sed -n '2,23p' "$0"; exit 0 ;;
    *) FAMS+=("$1"); shift ;;
  esac
done

# Concurrent runs (lanes, batch gates) each hold a slot, a flock kept for the script's life, and
# shift their displays and console ports by SLOTSTEP per slot, so two runs never share an X display
# or a console port. SLOTSTEP is wider than the family table (one display and one port each), which
# is what keeps two suites apart: at 20 the 23-family table of 2026-10-23 overlapped the next slot.
# Slot 0 (the bare table) is left to copies of this script that predate the slots.
# An explicit --display-base / SMOKE_DISPLAY_BASE keeps the old fixed numbering.
SLOTSTEP=40
PORTOFF=0
if [ "$LIST" = 0 ] && [ -z "${SMOKE_DISPLAY_BASE:-}" ] && [ "$DISPBASE" = 760 ]; then
  mkdir -p /tmp/opencode/locks
  for slot in $(seq 1 16); do
    exec {SLOTFD}>"/tmp/opencode/locks/smoke-slot.$slot"
    if flock -n "$SLOTFD"; then
      DISPBASE=$((760 + slot * SLOTSTEP)); PORTOFF=$((slot * SLOTSTEP)); break
    fi
    exec {SLOTFD}>&-
  done
  [ "$PORTOFF" = 0 ] && { echo "[smoke] no free slot (16 runs at once?)" >&2; exit 2; }
fi

# The family table: the world to boot into, the game updates to run, the backstop, the console
# port, the command file, the input file, the console driver and extra environment. A cmd_*.txt
# script is timed in game updates (`@f<n>`, gameplay from ~f240), never in wall seconds.
# The module families (grenchler, grenguard, flyingpirate, mediuming, metroid, sporb, blogg, swarm,
# gunturret, rezbit)
# boot the area the module lanes' coverage drivers used and repeat their console steps, so a checked
# module's actors keep loading: tools/cov/specs/{grenchler_entrance,grenchler_guardian,
# flyingpirate_sandfalls,mediuming_dead,metroid_player,sporb_hall,blogg_shaft,swarmbasics_sand,
# gunturret_0i_sand_hall,tail21_rezbit}.con and tools/drivers/.
# Append a new family at the end of ALL (its X display is its index) and give it the next free
# console port.
declare -A W=( [swamp1]="3dfd2249,04_swamp_temple_dark"
               [swamp2]="3dfd2249,04_swamp_temple_dark"
               [sandboss]="42b935e4,7575048a"
               [sandworm]="42b935e4,01_sand_entrance"
               [boostg]="3dfd2249,06_swamp_boostballguardian_dark"
               [boostg_fsm]="3dfd2249,06_swamp_boostballguardian_dark"
               [digig]="1baa96c2,09_cliff_temple_dark"
               [cmdpirate]="3bfa3eff,05_temple_federationsite_dark"
               [emp1]="863fcd72,07_temple_mothlordsanctuary_dark"
               [spacepirate]="42b935e4,08_sand_casualpirates"
               [ing]="1baa96c2,2735236b"
               [ep]="1baa96c2,b666b655"
               [plain]="-"
               [emp3]="863fcd72,07_temple_mothlordsanctuary_dark"
               [turret]="M06_ShootingGallery,06_ShootingGallery"
               [agon54]="42b935e4,08_sand_casualpirates"
               [darksamus]="42b935e4,10_sand_alamostorage"
               [splitter]="1baa96c2,0q_cliff_hall"
               [grenchler]="3dfd2249,01_swamp_entrance"
               [grenguard]="3dfd2249,08_swamp_grappleguardian_dark"
               [flyingpirate]="42b935e4,02_sand_sandfalls"
               [mediuming]="3bfa3eff,02_temple_exploration_dark"
               [metroid]="42b935e4,08_sand_casualpirates"
               [sporb]="3dfd2249,0e_swamp_hall"
               [blogg]="3dfd2249,07_swamp_shaft"
               [swarm]="42b935e4,01_sand_entrance"
               [gunturret]="42b935e4,0i_sand_hall"
               [rezbit]="1baa96c2,02_cliff_reactorcore" )
declare -A F=( [swamp1]=2000 [swamp2]=2000 [sandboss]=2000 [sandworm]=2000 [boostg]=2000
               [boostg_fsm]=2000 [digig]=2000 [cmdpirate]=2000 [emp1]=2000 [spacepirate]=2400
               [ing]=16000 [ep]=6000 [plain]=2000 [emp3]=2400 [turret]=2400 [agon54]=2400
               [darksamus]=4000 [splitter]=2000 [grenchler]=2400 [grenguard]=2400
               [flyingpirate]=2400 [mediuming]=2400 [metroid]=2400 [sporb]=2400 [blogg]=2400
               [swarm]=2400 [gunturret]=2400 [rezbit]=2400 )
declare -A S=( [swamp1]=280 [swamp2]=280 [sandboss]=280 [sandworm]=280 [boostg]=280
               [boostg_fsm]=280 [digig]=280 [cmdpirate]=280 [emp1]=280 [spacepirate]=280
               [ing]=420 [ep]=330 [plain]=280 [emp3]=280 [turret]=280 [agon54]=280
               [darksamus]=280 [splitter]=280 [grenchler]=280 [grenguard]=280
               [flyingpirate]=280 [mediuming]=280 [metroid]=280 [sporb]=280 [blogg]=280
               [swarm]=280 [gunturret]=280 [rezbit]=280 )
declare -A P=( [swamp1]=49701 [swamp2]=49702 [sandboss]=49703 [sandworm]=49704 [boostg]=49705
               [digig]=49706 [cmdpirate]=49707 [emp1]=49708 [spacepirate]=49709
               [boostg_fsm]=49710 [emp3]=49711 [turret]=49712 [agon54]=49713 [ing]=49714
               [ep]=49715 [darksamus]=49716 [plain]=49717 [splitter]=49718
               [grenchler]=49719 [grenguard]=49720 [flyingpirate]=49721 [mediuming]=49722
               [metroid]=49723 [sporb]=49724 [blogg]=49725 [swarm]=49726
               [gunturret]=49727 [rezbit]=49728 )
declare -A C=( [swamp1]=cmd_swamp1.txt [swamp2]=cmd_swamp2.txt [sandboss]=cmd_sandboss.txt
               [sandworm]=cmd_sandworm.txt [boostg]=cmd_boostg.txt [boostg_fsm]=cmd_boostg_fsm.txt
               [digig]=cmd_digig.txt [cmdpirate]=cmd_cmdpirate.txt [emp1]=cmd_emp1.txt
               [spacepirate]=cmd_spacepirate.txt [emp3]=cmd_emp3.txt [turret]=cmd_turret.txt
               [agon54]=cmd_agon54.txt [splitter]=cmd_splitter.txt
               [grenchler]=cmd_grenchler.txt [grenguard]=cmd_grenguard.txt
               [flyingpirate]=cmd_flyingpirate.txt [mediuming]=cmd_mediuming.txt
               [metroid]=cmd_metroid.txt [sporb]=cmd_sporb.txt [blogg]=cmd_blogg.txt
               [swarm]=cmd_swarm.txt [gunturret]=cmd_gunturret.txt [rezbit]=cmd_rezbit.txt )
declare -A I=( [ep]=in_ep.txt )
declare -A V=( [ing]=ing [ep]=ep [darksamus]=darksamus )
# Fast mode (MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8, unpaced: ~5x shorter runs) for every family whose
# script is timed in game updates (cmd_*.txt via con.py). The V drivers time their steps in wall
# seconds, so those families stay under the cap.
FASTENV="MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8"
declare -A X=( [boostg_fsm]="MP2_BG_FSM=1" [spacepirate]="MP2_DEBUG_SPACEPIRATE=1"
               [ing]="MP2_DEBUG_ING=1 MP2_DEBUG_BEAM=1" )

# The order fixes each family's X display, so two runs of the suite never share one.
ALL=(swamp1 swamp2 sandboss sandworm boostg boostg_fsm digig cmdpirate emp1 spacepirate
     ing ep plain emp3 turret agon54 darksamus splitter grenchler grenguard flyingpirate
     mediuming metroid sporb blogg swarm gunturret rezbit)
declare -A IDX=()
for i in "${!ALL[@]}"; do IDX[${ALL[$i]}]=$i; done

if [ "$LIST" = 1 ]; then
  printf "%-12s %-9s %-6s %-5s %-7s %-22s %s\n" family frames secs port display world extra
  for f in "${ALL[@]}"; do
    printf "%-12s %-9s %-6s %-5s :%-6s %-22s %s\n" "$f" "${F[$f]}" "${S[$f]}" "${P[$f]}" \
      "$((DISPBASE + IDX[$f]))" "${W[$f]}" "${X[$f]:-}"
  done
  exit 0
fi

[ ${#FAMS[@]} -gt 0 ] || FAMS=("${ALL[@]}")
for f in "${FAMS[@]}"; do
  if [ -z "${W[$f]:-}" ]; then
    echo "[smoke] unknown family '$f' (tools/smoke/run.sh --list)" >&2
    exit 2
  fi
done

[ -n "$OUT" ] || OUT=/tmp/opencode/smoke-$(git -C "$ROOT" rev-parse --short HEAD)
mkdir -p "$OUT" || exit 2
[ -x "$BUILD/metroid_prime2_port" ] || { echo "[smoke] no game in $BUILD" >&2; exit 2; }

echo "[smoke] out=$OUT build=$BUILD jobs=$JOBS save=$SAVE fast=$FAST headless=$HEADLESS families=${FAMS[*]}"
# fam.sh -> run_gameplay.sh read it from the environment.
export HEADLESS
# A coverage build (CMake option MP_PORT_COVERAGE) writes its .gcda files per family, under
# <out>/gcov_<family>/gcov: the families run concurrently and would otherwise write over each
# other. fam.sh sets it; nothing changes for a normal build.
if nm -g "$BUILD/metroid_prime2_port" 2>/dev/null | grep -q '__gcov_dump'; then
  echo "[smoke] coverage build: the profiles land in $OUT/gcov_<family>/gcov"
fi

run_one() {   # <tag>: one family; its own display, port, card copy and cache dir
  local tag=$1 rc
  local extra=() cmd="-" inp="-"
  read -ra extra <<< "${X[$tag]:-}"
  if [ "$FAST" = 1 ] && [ -z "${V[$tag]:-}" ]; then
    read -ra fastenv <<< "$FASTENV"
    extra+=("${fastenv[@]}")
  fi
  [ -n "${C[$tag]:-}" ] && cmd="$HERE/${C[$tag]}"
  [ -n "${I[$tag]:-}" ] && inp="$HERE/${I[$tag]}"
  echo "[smoke] $tag start $(date +%T) display=:$((DISPBASE + IDX[$tag])) port=$((P[$tag] + PORTOFF))"
  SMOKE_OUT="$OUT" SMOKE_BUILD="$BUILD" SMOKE_DISPLAY=":$((DISPBASE + IDX[$tag]))" \
    SMOKE_PORT="$((P[$tag] + PORTOFF))" SMOKE_WORLD="${W[$tag]}" SMOKE_FRAMES="${F[$tag]}" SMOKE_SECS="${S[$tag]}" \
    SMOKE_CMD="$cmd" SMOKE_INPUT="$inp" SMOKE_DRIVER="${V[$tag]:-generic}" SMOKE_SAVE="$SAVE" \
    "$HERE/fam.sh" "$tag" ${extra[@]+"${extra[@]}"} > "$OUT/$tag.run.log" 2>&1
  rc=$?
  echo "$rc" > "$OUT/$tag.rc"
  echo "[smoke] $tag done rc=$rc $(date +%T)"
  return 0
}

for f in "${FAMS[@]}"; do
  while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do sleep 0.5; done
  run_one "$f" &
done
wait

printf "\n%-12s %-5s %-9s %-7s %-5s %-8s\n" family exit gameplay caught stub standin
for f in "${FAMS[@]}"; do
  log=$OUT/log_${f}.log
  rc=$(cat "$OUT/$f.rc" 2>/dev/null || echo "?")
  gs=$(grep -a -o 'gameplay reached at [0-9.]* s' "$log" 2>/dev/null | tail -1 | awk '{print $4}')
  caught=$(grep -a -c '\[port\] caught' "$log" 2>/dev/null | head -1)
  stub=$(grep -a -c 'reach-stub' "$log" 2>/dev/null | head -1)
  standin=$(grep -a -c 'is a port stand-in' "$log" 2>/dev/null | head -1)
  printf "%-12s %-5s %-9s %-7s %-5s %-8s\n" "$f" "$rc" "${gs:-?} s" "${caught:-0}" \
    "${stub:-0}" "${standin:-0}"
done | tee "$OUT/summary.txt"
echo "[smoke] done $(date +%T)"
