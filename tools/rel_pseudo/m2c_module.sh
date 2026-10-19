#!/bin/bash
# Usage: tools/rel_pseudo/m2c_module.sh <Module> [outdir]
# Dumps m2c pseudo-C (<outdir>/<fn>.c, stderr in <fn>.err) for every .fn in one REL module's asm,
# default outdir /tmp/opencode/pseudo/<Module>; asm root from MP2_REL_ASM, m2c from M2C.
# Prints "<Module> fns=<n> fail=<n>" at the end.
set -u

M=${1:-}
if [ -z "$M" ]; then
  echo "usage: m2c_module.sh <Module> [outdir]" >&2
  exit 2
fi
O=${2:-/tmp/opencode/pseudo/$M}
M2C=${M2C:-/tmp/opencode/m2cenv/bin/m2c}
ASM_ROOT=${MP2_REL_ASM:-/run/media/odran/Leo/projects/Restored-projects/Chatgpt/MetroidPrime2Port/build/G2ME01}
A=$ASM_ROOT/$M/asm

if [ ! -d "$A" ]; then
  echo "$M fns=0 fail=0 (no asm dir $A)" >&2
  exit 1
fi

shopt -s nullglob
text=("$A"/*_text.s)
rodata=("$A"/*_rodata.s)
data=("$A"/*_data.s)
bss=("$A"/*_bss.s)
shopt -u nullglob

srcs=()
for f in "${text[@]}" "${rodata[@]}" "${data[@]}" "${bss[@]}"; do
  [ -f "$f" ] && srcs+=("$f")
done

if [ ${#text[@]} -eq 0 ]; then
  echo "$M fns=0 fail=0 (no *_text.s in $A)"
  exit 0
fi

mkdir -p "$O"
errlog=$O/_stderr
: >"$errlog"

n=0
fail=0
declare -A seen=()
for t in "${text[@]}"; do
  while read -r fn; do
    [ -n "$fn" ] || continue
    fn=${fn%,}
    [ -n "${seen[$fn]:-}" ] && continue
    seen[$fn]=1
    n=$((n + 1))
    "$M2C" --target ppc-mwcc-c++ -f "$fn" "${srcs[@]}" >"$O/$fn.c" 2>"$O/$fn.err"
    rc=$?
    if [ "$rc" -ne 0 ]; then
      fail=$((fail + 1))
      printf 'FAIL %s (m2c exit %s)\n' "$fn" "$rc" >>"$errlog"
    elif [ ! -s "$O/$fn.c" ]; then
      fail=$((fail + 1))
      printf 'FAIL %s (empty output)\n' "$fn" >>"$errlog"
    fi
  done < <(sed -n 's/^\.fn[[:space:]]\+\([^,[:space:]]*\).*/\1/p' "$t")
done

echo "$M fns=$n fail=$fail"
