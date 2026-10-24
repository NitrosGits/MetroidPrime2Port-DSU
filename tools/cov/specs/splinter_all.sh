#!/bin/bash
# Rerun every Splinter coverage driver on build-port-cov, 3 at a time.
# Usage: tools/cov/specs/splinter_all.sh <outroot>   (run from the worktree root)
OUT=${1:?outroot}; mkdir -p "$OUT"
run1() {
  name=$1; area=$2; con=$3; frames=$4; out=$OUT/$name
  tools/games_running.sh --wait
  BUILD=build-port-cov timeout 1500 tools/cov/run.sh tools/cov/specs/splinter.py "$out" "$area" \
    --con "$con" --fast --frames "$frames" > "$OUT/$name.log" 2>&1
  echo "$name rc=$?"
}
export -f run1; export OUT
while read -r name area con frames; do echo "$name $area tools/cov/specs/$con $frames"; done <<'LIST' | xargs -P3 -L1 bash -c 'run1 "$@"' _
alamo_ball 42b935e4,07_sand_alamoportalobservation_dark splinter_alamo_ball.con 3400
alamo_fight 42b935e4,07_sand_alamoportalobservation_dark splinter_alamo_fight.con 3400
combat_ball 3bfa3eff,03_temple_combattraining splinter_combat_ball.con 3400
combat_fight 3bfa3eff,03_temple_combattraining splinter_combat_fight.con 3400
fed_ball 3bfa3eff,05_temple_federationsite splinter_fed_ball.con 3400
fed_fight 3bfa3eff,05_temple_federationsite splinter_fed_fight.con 3400
hall0c_ball 3bfa3eff,0c_temple_hall splinter_hall0c_ball.con 3400
hall0c_fight 3bfa3eff,0c_temple_hall splinter_hall0c_fight.con 3400
hall0d_ball 3bfa3eff,0d_temple_hall splinter_hall0d_ball.con 3400
hall0d_fight 3bfa3eff,0d_temple_hall splinter_hall0d_fight.con 3400
hive02_ball 3bfa3eff,01_temple_hive02 splinter_hive02_ball.con 3400
hive02_fight 3bfa3eff,01_temple_hive02 splinter_hive02_fight.con 3400
hivehall_ball 3bfa3eff,0e_temple_hive_hall splinter_hivehall_ball.con 3400
hivehall_fight 3bfa3eff,0e_temple_hive_hall splinter_hivehall_fight.con 3400
moth_ball 863fcd72,07_temple_mothlordsanctuary splinter_moth_ball.con 3400
moth_fight 863fcd72,07_temple_mothlordsanctuary splinter_moth_fight.con 3400
sanddark_ball 42b935e4,0h_sand_hall_dark splinter_sanddark_ball.con 3400
sanddark_fight 42b935e4,0h_sand_hall_dark splinter_sanddark_fight.con 3400
te46_ball 3bfa3eff,02_temple_exploration splinter_te46_ball.con 3400
te46_fight 3bfa3eff,02_temple_exploration splinter_te46_fight.con 3400
te_ball 3bfa3eff,02_temple_exploration splinter_te_ball.con 3200
te 3bfa3eff,02_temple_exploration splinter_te.con 3000
te_fight 3bfa3eff,02_temple_exploration splinter_te_fight.con 3200
unknown_ball 3bfa3eff,04_temple_unknownenemies splinter_unknown_ball.con 3400
unknown_fight 3bfa3eff,04_temple_unknownenemies splinter_unknown_fight.con 3400
LIST
