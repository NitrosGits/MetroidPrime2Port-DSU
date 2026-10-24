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
# is what keeps two suites apart: at 20 the 23-family table of 2026-10-23 overlapped the next slot,
# and at 40 the 41-family table of 2026-10-23 (fishcloud, the last, at index 40) did too. It must
# exceed the last index of the table (spp was 44 in the 45-family table of 2026-10-23, which SLOTSTEP
# 45 still cleared, and 50 was full at drkc, the 49th; it is 100, and the check after ALL below fails
# the run if the table outgrows it).
# Slot 0 (the bare table) is left to copies of this script that predate the slots.
# An explicit --display-base / SMOKE_DISPLAY_BASE keeps the old fixed numbering.
SLOTSTEP=100
# The lock files name 50-wide blocks (block b = displays 760+50b.., ports +50b..), the unit older
# copies of this script (SLOTSTEP 50, one lock per slot) still take: a run holds every block its
# SLOTSTEP spans, so a SLOTSTEP-100 run and an old run never share a display or port (with one lock
# per slot they did, and killed each other's games, 2026-10-23).
BLOCK=50
PORTOFF=0
if [ "$LIST" = 0 ] && [ -z "${SMOKE_DISPLAY_BASE:-}" ] && [ "$DISPBASE" = 760 ]; then
  mkdir -p /tmp/opencode/locks
  NB=$((SLOTSTEP / BLOCK))
  for first in $(seq 1 "$NB" 32); do
    FDS=()
    for b in $(seq "$first" $((first + NB - 1))); do
      exec {fd}>"/tmp/opencode/locks/smoke-slot.$b"
      if flock -n "$fd"; then FDS+=("$fd"); else exec {fd}>&-; break; fi
    done
    if [ ${#FDS[@]} = "$NB" ]; then
      DISPBASE=$((760 + first * BLOCK)); PORTOFF=$((first * BLOCK)); break
    fi
    for fd in "${FDS[@]}"; do exec {fd}>&-; done
  done
  [ "$PORTOFF" = 0 ] && { echo "[smoke] no free slot (too many runs at once?)" >&2; exit 2; }
fi

# The family table: the world to boot into, the game updates to run, the backstop, the console
# port, the command file, the input file, the console driver and extra environment. A cmd_*.txt
# script is timed in game updates (`@f<n>`, gameplay from ~f240), never in wall seconds.
# The module families (grenchler, grenguard, flyingpirate, mediuming, metroid, sporb, blogg, swarm,
# gunturret, rezbit, parasite, lumite, shrieker, ingswarm, darktrooper, bacteriaswarm, tryclops,
# wisptentacle, octapede, amt, dbar, sjguard, fishcloud, spa, wallwalker, isbg, spp, mysf, spor,
# scriptgui)
# boot the area the module lanes' coverage drivers used and repeat their console steps, so a checked
# module's actors keep loading: tools/cov/specs/{grenchler_entrance,grenchler_guardian,
# flyingpirate_sandfalls,mediuming_dead,metroid_player,sporb_hall,blogg_shaft,swarmbasics_sand,
# gunturret_0i_sand_hall,tail21_rezbit,parasite_sandfalls_fight,
# ingsnatchingswarm_pathofeyes,bacteriaswarm_temple}.con and tools/drivers/ (lumite and shrieker repeat
# tools/drivers/lumite_states.py and shrieker_greatbridge.con, darktrooper
# tools/drivers/darktrooper_unknown.con, tryclops tryclops_grab.con and wisptentacle
# wisptentacle_hall.con, octapede octapede_cliffhall.con, amt aimannedturret_alamofront.con, dbar
# destructiblebarrier_forced.con, sjguard ingspacejumpguardian_sandfalls.con, fishcloud
# fishcloud_sacredpath.con, spa tools/drivers/spa_sandtempledark.con, wallwalker
# wallwalker_temple.con, isbg isbg.con, spp spp_dark_landingsite.con, mysf mysteryflyer.con,
# spor puddlespore.con and scriptgui gui_menu_slider.con - scriptgui boots the FrontEnd world, which
# needs SKIP_FRONTEND=0 in the X table below).
# Append a new family at the end of ALL (its X display is its index) and give it the next free
# console port.
declare -A W=( [swamp1]="3dfd2249,04_swamp_temple_dark" [swamp2]="3dfd2249,04_swamp_temple_dark"
               [sandboss]="42b935e4,7575048a" [sandworm]="42b935e4,01_sand_entrance"
               [boostg]="3dfd2249,06_swamp_boostballguardian_dark"
               [boostg_fsm]="3dfd2249,06_swamp_boostballguardian_dark"
               [digig]="1baa96c2,09_cliff_temple_dark"
               [cmdpirate]="3bfa3eff,05_temple_federationsite_dark"
               [emp1]="863fcd72,07_temple_mothlordsanctuary_dark"
               [spacepirate]="42b935e4,08_sand_casualpirates" [ing]="1baa96c2,2735236b"
               [ep]="1baa96c2,b666b655" [plain]="-"
               [emp3]="863fcd72,07_temple_mothlordsanctuary_dark"
               [turret]="M06_ShootingGallery,06_ShootingGallery"
               [agon54]="42b935e4,08_sand_casualpirates" [darksamus]="42b935e4,10_sand_alamostorage"
               [splitter]="1baa96c2,0q_cliff_hall" [grenchler]="3dfd2249,01_swamp_entrance"
               [grenguard]="3dfd2249,08_swamp_grappleguardian_dark"
               [flyingpirate]="42b935e4,02_sand_sandfalls"
               [mediuming]="3bfa3eff,02_temple_exploration_dark"
               [metroid]="42b935e4,08_sand_casualpirates" [sporb]="3dfd2249,0e_swamp_hall"
               [blogg]="3dfd2249,07_swamp_shaft" [swarm]="42b935e4,01_sand_entrance"
               [gunturret]="42b935e4,0i_sand_hall" [rezbit]="1baa96c2,02_cliff_reactorcore"
               [parasite]="42b935e4,02_sand_sandfalls" [lumite]="42b935e4,0a_sand_hall"
               [shrieker]="3dfd2249,02_swamp_greatbridge" [ingswarm]="3bfa3eff,10_temple_pathofeyes"
               [darktrooper]="3bfa3eff,04_temple_unknownenemies"
               [bacteriaswarm]="3bfa3eff,04_temple_unknownenemies_dark"
               [tryclops]="1baa96c2,42c85ad9" [wisptentacle]="3bfa3eff,0t_temple_hall_dark"
               [octapede]="1baa96c2,0p_cliff_hall" [amt]="42b935e4,06_sand_alamofront"
               [dbar]="42b935e4,08_sand_casualpirates" [sjguard]="42b935e4,02_sand_sandfalls_dark"
               [fishcloud]="3bfa3eff,06_temple_sacredpath" [spa]="42b935e4,03_sand_temple_dark"
               [wallwalker]="1baa96c2,09_cliff_temple" [isbg]="1baa96c2,06_cliff_spiderballguardian"
               [spp]="3bfa3eff,01_temple_landingsite_dark" [mysf]="3bfa3eff,0e_temple_hall"
               [spor]="3dfd2249,cbc72d79" [scriptgui]="69802220,cee3af0d"
               [kral]="3bfa3eff,0b_temple_hall" [gbug]="863fcd72,0i_temple_hall"
               [pssm]="3dfd2249,0g_swamp_hall" [stod]="3dfd2249,09_swamp_catacombs_dark"
               [pbug]="42b935e4,0b_sand_hall" [szone]="42b935e4,02_sand_sandfalls_dark"
               [splinter]="3bfa3eff,02_temple_exploration" [rptl]="3bfa3eff,12_temple_finalgate"
               [chog]="42b935e4,08_sand_casualpirates" [drkc]="3bfa3eff,06_temple_sacredpath_dark"
               [plac]="42b935e4,00_mapstation_sand" [plct]="42b935e4,05_portalpuzzle"
               [ming]="42b935e4,0a_sand_hall_dark" [snak]="3dfd2249,01_swamp_entrance"
               [eyeb]="3bfa3eff,0q_temple_hall_dark" [kroc]="3dfd2249,0h_swamp_hall"
               [atma]="863fcd72,0l_temple_hall" [atmb]="1baa96c2,0b_cliff_hall"
               [shrd]="3dfd2249,01_swamp_entrance" [fsw]="42b935e4,0e_sand_hall"
               [iss]="3bfa3eff,10_temple_pathofeyes" [ibs]="1baa96c2,0b_cliff_hall_dark"
               [mree]="42b935e4,0c_sand_hall" [puff]="42b935e4,07_sand_alamoportalobservation_dark"
               [rubk]="1baa96c2,07cliff_gyro" [emp2]="863fcd72,07_temple_mothlordsanctuary_dark"
               [cball]="42b935e4,04_sand_mining" [fobj]="42b935e4,05_portalpuzzle"
               [skyr]="3bfa3eff,03_temple_combattraining" [dsbs]="42b935e4,10_sand_alamostorage"
               [isg]="1baa96c2,06_cliff_spiderballguardian"
               [ingp]="42b935e4,06_sand_alamofront_dark" )
declare -A F=( [swamp1]=2400 [swamp2]=2400 [sandboss]=2400 [sandworm]=2400 [boostg]=2200
               [boostg_fsm]=2000 [digig]=2200 [cmdpirate]=2200 [emp1]=2000 [spacepirate]=2400
               [ing]=16000 [ep]=6000 [plain]=2000 [emp3]=2600 [turret]=2400 [agon54]=2500
               [darksamus]=4000 [splitter]=2400 [grenchler]=2500 [grenguard]=2500
               [flyingpirate]=2400 [mediuming]=2400 [metroid]=2600 [sporb]=2600 [blogg]=2400
               [swarm]=2600 [gunturret]=2600 [rezbit]=2600 [parasite]=2400 [lumite]=2600
               [shrieker]=2600 [ingswarm]=2400 [darktrooper]=2400 [bacteriaswarm]=2400
               [tryclops]=2600 [wisptentacle]=6200 [octapede]=3000 [amt]=9400 [dbar]=1200
               [sjguard]=6000 [fishcloud]=2500 [spa]=2000 [wallwalker]=3700 [isbg]=5700 [spp]=1700
               [mysf]=3200 [spor]=2000 [scriptgui]=700 [kral]=1300 [gbug]=2700 [pssm]=3000
               [stod]=1500 [pbug]=3600 [szone]=2500 [splinter]=3400 [rptl]=2200 [chog]=1020
               [drkc]=4100 [plac]=2000 [plct]=2200 [ming]=1200 [snak]=1000 [eyeb]=2000 [kroc]=2000
               [atma]=2400 [atmb]=2400 [shrd]=1100 [fsw]=2400 [iss]=3000 [ibs]=2600 [mree]=2400
               [puff]=2400 [rubk]=1000 [emp2]=1200 [cball]=1000 [fobj]=800 [skyr]=800 [dsbs]=2000
               [isg]=3000 [ingp]=3200 )
declare -A S=( [swamp1]=280 [swamp2]=280 [sandboss]=280 [sandworm]=280 [boostg]=280 [boostg_fsm]=280
               [digig]=280 [cmdpirate]=280 [emp1]=280 [spacepirate]=280 [ing]=420 [ep]=330
               [plain]=280 [emp3]=280 [turret]=280 [agon54]=280 [darksamus]=280 [splitter]=280
               [grenchler]=280 [grenguard]=280 [flyingpirate]=280 [mediuming]=280 [metroid]=280
               [sporb]=280 [blogg]=280 [swarm]=280 [gunturret]=280 [rezbit]=280 [parasite]=280
               [lumite]=280 [shrieker]=280 [ingswarm]=280 [darktrooper]=280 [bacteriaswarm]=280
               [tryclops]=280 [wisptentacle]=280 [octapede]=280 [amt]=280 [dbar]=280 [sjguard]=280
               [fishcloud]=280 [spa]=280 [wallwalker]=280 [isbg]=280 [spp]=280 [mysf]=280 [spor]=280
               [scriptgui]=280 [kral]=280 [gbug]=280 [pssm]=280 [stod]=280 [pbug]=280 [szone]=280
               [splinter]=280 [rptl]=280 [chog]=280 [drkc]=280 [plac]=280 [plct]=280 [ming]=280
               [snak]=280 [eyeb]=280 [kroc]=280 [atma]=280 [atmb]=280 [shrd]=280 [fsw]=280 [iss]=280
               [ibs]=280 [mree]=280 [puff]=280 [rubk]=280 [emp2]=280 [cball]=280 [fobj]=280
               [skyr]=280 [dsbs]=280 [isg]=280 [ingp]=280 )
declare -A P=( [swamp1]=49701 [swamp2]=49702 [sandboss]=49703 [sandworm]=49704 [boostg]=49705
               [digig]=49706 [cmdpirate]=49707 [emp1]=49708 [spacepirate]=49709 [boostg_fsm]=49710
               [emp3]=49711 [turret]=49712 [agon54]=49713 [ing]=49714 [ep]=49715 [darksamus]=49716
               [plain]=49717 [splitter]=49718 [grenchler]=49719 [grenguard]=49720
               [flyingpirate]=49721 [mediuming]=49722 [metroid]=49723 [sporb]=49724 [blogg]=49725
               [swarm]=49726 [gunturret]=49727 [rezbit]=49728 [parasite]=49729 [lumite]=49730
               [shrieker]=49731 [ingswarm]=49732 [darktrooper]=49733 [bacteriaswarm]=49734
               [tryclops]=49735 [wisptentacle]=49736 [octapede]=49737 [amt]=49738 [dbar]=49739
               [sjguard]=49740 [fishcloud]=49741 [spa]=49742 [wallwalker]=49743 [isbg]=49744
               [spp]=49745 [mysf]=49746 [spor]=49747 [scriptgui]=49748 [kral]=49749 [gbug]=49750
               [pssm]=49751 [stod]=49752 [pbug]=49753 [szone]=49754 [splinter]=49755 [rptl]=49756
               [chog]=49757 [drkc]=49758 [plac]=49759 [plct]=49760 [ming]=49761 [snak]=49762
               [eyeb]=49763 [kroc]=49764 [atma]=49765 [atmb]=49766 [shrd]=49767 [fsw]=49768
               [iss]=49769 [ibs]=49770 [mree]=49771 [puff]=49772 [rubk]=49773 [emp2]=49774
               [cball]=49775 [fobj]=49776 [skyr]=49777 [dsbs]=49778 [isg]=49779 [ingp]=49780 )
declare -A C=( [swamp1]=cmd_swamp1.txt [swamp2]=cmd_swamp2.txt [sandboss]=cmd_sandboss.txt
               [sandworm]=cmd_sandworm.txt [boostg]=cmd_boostg.txt [boostg_fsm]=cmd_boostg_fsm.txt
               [digig]=cmd_digig.txt [cmdpirate]=cmd_cmdpirate.txt [emp1]=cmd_emp1.txt
               [spacepirate]=cmd_spacepirate.txt [emp3]=cmd_emp3.txt [turret]=cmd_turret.txt
               [agon54]=cmd_agon54.txt [splitter]=cmd_splitter.txt [grenchler]=cmd_grenchler.txt
               [grenguard]=cmd_grenguard.txt [flyingpirate]=cmd_flyingpirate.txt
               [mediuming]=cmd_mediuming.txt [metroid]=cmd_metroid.txt [sporb]=cmd_sporb.txt
               [blogg]=cmd_blogg.txt [swarm]=cmd_swarm.txt [gunturret]=cmd_gunturret.txt
               [rezbit]=cmd_rezbit.txt [parasite]=cmd_parasite.txt [lumite]=cmd_lumite.txt
               [shrieker]=cmd_shrieker.txt [ingswarm]=cmd_ingswarm.txt
               [darktrooper]=cmd_darktrooper.txt [bacteriaswarm]=cmd_bacteriaswarm.txt
               [tryclops]=cmd_tryclops.txt [wisptentacle]=cmd_wisptentacle.txt
               [octapede]=cmd_octapede.txt [amt]=cmd_amt.txt [dbar]=cmd_dbar.txt
               [sjguard]=cmd_sjguard.txt [fishcloud]=cmd_fishcloud.txt [spa]=cmd_spa.txt
               [wallwalker]=cmd_wallwalker.txt [isbg]=cmd_isbg.txt [spp]=cmd_spp.txt
               [mysf]=cmd_mysf.txt [spor]=cmd_spor.txt [scriptgui]=cmd_scriptgui.txt
               [kral]=cmd_kral.txt [gbug]=cmd_gbug.txt [pssm]=cmd_pssm.txt [stod]=cmd_stod.txt
               [pbug]=cmd_pbug.txt [szone]=cmd_szone.txt [splinter]=cmd_splinter.txt
               [rptl]=cmd_rptl.txt [chog]=cmd_chog.txt [drkc]=cmd_drkc.txt [plac]=cmd_plac.txt
               [plct]=cmd_plct.txt [ming]=cmd_ming.txt [snak]=cmd_snak.txt [eyeb]=cmd_eyeb.txt
               [kroc]=cmd_kroc.txt [atma]=cmd_atma.txt [atmb]=cmd_atmb.txt [shrd]=cmd_shrd.txt
               [fsw]=cmd_fsw.txt [iss]=cmd_iss.txt [ibs]=cmd_ibs.txt [mree]=cmd_mree.txt
               [puff]=cmd_puff.txt [rubk]=cmd_rubk.txt [emp2]=cmd_emp2.txt [cball]=cmd_cball.txt
               [fobj]=cmd_fobj.txt [skyr]=cmd_skyr.txt [dsbs]=cmd_dsbs.txt [isg]=cmd_isg.txt
               [ingp]=cmd_ingp.txt )
declare -A I=( [ep]=in_ep.txt )
declare -A V=( [ing]=ing [ep]=ep [darksamus]=darksamus )
# Fast mode (MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8, unpaced: ~5x shorter runs) for every family whose
# script is timed in game updates (cmd_*.txt via con.py). The V drivers time their steps in wall
# seconds, so those families stay under the cap. NC is the second exception: a family whose area
# boots so quickly that its unpaced updates are over (0.3 s for 2400) before con.py's 0.5 s retry
# gets a console connection - it stays capped (~40 s, ~110 s for wisptentacle's 6200), or nothing
# would be driven at all. amt is the tight case: unpaced, con.py connects at 0.5 s when the run is
# already at ~560 of its 9400 updates (its first check is f600), so it stays capped (~155 s).
# spa is the other kind: unpaced, `layer 10`'s two PLAC objects only appear around f1300 (the layer
# load trails the fast updates), so every send before that answers "no object with editor id"; capped,
# they are in by f340 as in its driver. Capped, its 2000 updates take ~35 s.
# wallwalker is spa's case with a thinner margin, measured: unpaced, 3600 updates run from 0.4 s to
# 2.6 s (~1500 a second) and con.py connects at 0.5 s, already at ~update 300 - the script's own
# first step - so `layer 3` and the f330 `objs WallWalker` would come within a few updates of each
# other with nothing between them if the machine were faster. Capped, the layer is in by f330 as in
# its driver; its 3600 updates take ~62 s.
# isbg is wallwalker's case at the boss's own scale, measured: unpaced, 5700 updates run from 0.3 s
# to 2.5 s (~2600 a second) and con.py connects at 0.5 s around update 300, the `layer 13` step
# itself; the guardian then needs ~500 frames to appear (its notes), and `send e0014007a Activate`
# at f1000 is only ~200 updates behind that, so a faster machine would activate nothing. Capped,
# the guardian is in for f900 as in its driver; its 5700 updates take ~95 s. spp runs fast: its two
# PLCTs are layer 0 of the booted area, so no layer step can trail, and its transcript is identical
# capped and uncapped. mysf is spp's case: its three wasps are layer 0 of 0e_temple_hall and already
# listed `inactive` in the f340 `objs`, and its 3200-update transcript is byte-identical capped and
# uncapped. spor and scriptgui are the thin-margin kind, measured: unpaced, spor's 2000 updates run
# from 0.1 s to 0.6 s (its log: the frame limit is up at 0.6 s, ~4000 a second) and con.py connects at
# 0.5 s, so only ~100 ms of run is left for the whole script - a faster machine would connect after
# the run had ended and drive nothing at all (an empty transcript still reports 0 caught/stub). Capped,
# its 2000 updates take ~33 s. scriptgui has room but not determinism: unpaced its 700 updates run
# from 0.3 s to 1.0 s, and one `objs Gui` line reads `inactive` uncapped but active capped - the
# widget's state follows the update rate, so the transcript is not a fixed baseline. Capped, the
# front end's 700 updates take ~25 s.
FASTENV="MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8"
declare -A NC=( [ingswarm]=1 [darktrooper]=1 [bacteriaswarm]=1 [tryclops]=1 [wisptentacle]=1
               [octapede]=1 [amt]=1 [spa]=1 [wallwalker]=1 [isbg]=1 [spor]=1 [scriptgui]=1 [kral]=1
               [pssm]=1 [stod]=1 [pbug]=1 [szone]=1 [rptl]=1 [chog]=1 [drkc]=1 [plac]=1 [plct]=1
               [ming]=1 [snak]=1 [eyeb]=1 [kroc]=1 [atma]=1 [atmb]=1 [shrd]=1 [fsw]=1 [iss]=1
               [ibs]=1 [mree]=1 [puff]=1 [rubk]=1 [cball]=1 [fobj]=1 [skyr]=1 [isg]=1 )
# Extra environment per family: tools/run_gameplay.sh's variables, not the game's directly. The
# harness passes them on to the game, and run_gameplay.sh maps a few of its own onto MP2_* names
# (SKIP_FRONTEND -> MP2_SKIP_FRONTEND, SKIP_INTRO, SKIP_CINEMATICS, FAST_LOAD). scriptgui needs the
# front end, so it asks for SKIP_FRONTEND=0 here: `MP2_SKIP_FRONTEND=0` would be overwritten by
# run_gameplay.sh's own default of 1 (tools/run_gameplay.sh header).
declare -A X=( [boostg_fsm]="MP2_BG_FSM=1" [spacepirate]="MP2_DEBUG_SPACEPIRATE=1"
               [ing]="MP2_DEBUG_ING=1 MP2_DEBUG_BEAM=1" [scriptgui]="SKIP_FRONTEND=0" )
# The order fixes each family's X display, so two runs of the suite never share one.
ALL=(swamp1 swamp2 sandboss sandworm boostg boostg_fsm digig cmdpirate emp1 spacepirate ing ep plain
     emp3 turret agon54 darksamus splitter grenchler grenguard flyingpirate mediuming metroid sporb
     blogg swarm gunturret rezbit parasite lumite shrieker ingswarm darktrooper bacteriaswarm
     tryclops wisptentacle octapede amt dbar sjguard fishcloud spa wallwalker isbg spp mysf spor
     scriptgui kral gbug pssm stod pbug szone splinter rptl chog drkc plac plct ming snak eyeb kroc
     atma atmb shrd fsw iss ibs mree puff rubk emp2 cball fobj skyr dsbs isg ingp)
declare -A IDX=(  )
for i in "${!ALL[@]}"; do IDX[${ALL[$i]}]=$i; done
if [ ${#ALL[@]} -ge $SLOTSTEP ]; then
  echo "[smoke] ${#ALL[@]} families do not fit SLOTSTEP=$SLOTSTEP: raise it" >&2; exit 2
fi

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
  if [ "$FAST" = 1 ] && [ -z "${V[$tag]:-}" ] && [ -z "${NC[$tag]:-}" ]; then
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
