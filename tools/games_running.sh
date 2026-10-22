#!/bin/bash
# Prints how many MP2/MP3 port games are running (MP2 and MP3 share the machine).
# Counts processes whose executable is metroid_prime[23]_port and whose parent is not one
# (a game's forked child is the same game). `pgrep -fc` overcounts: it also matches the
# timeout/gdb wrappers and every command line that mentions the name.
# Lanes: start a run only while this prints < 3 (MP2_MAX_GAMES overrides).
#   tools/games_running.sh          -> count
#   tools/games_running.sh --wait   -> blocks until a slot is free, then returns
is_game() {
  local exe
  exe=$(readlink "/proc/$1/exe" 2>/dev/null) || return 1
  case ${exe##*/} in metroid_prime2_port | metroid_prime3_port) return 0 ;; esac
  return 1
}
count() {
  local n=0 d pid ppid
  for d in /proc/[0-9]*; do
    pid=${d#/proc/}
    is_game "$pid" || continue
    ppid=$(awk '/^PPid:/ {print $2}' "$d/status" 2>/dev/null)
    is_game "$ppid" && continue
    n=$((n + 1))
  done
  echo "$n"
}
if [ "${1:-}" = --wait ]; then
  max=${MP2_MAX_GAMES:-3}
  while [ "$(count)" -ge "$max" ]; do sleep 20; done
  exit 0
fi
count
