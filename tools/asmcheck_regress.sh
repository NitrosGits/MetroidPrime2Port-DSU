#!/usr/bin/env bash
# Regression for tools/asmcheck.py: the six decomp slips fixed by hand must be flagged on the
# pre-fix tree (<commit>^) and gone on the fix commit, and the audited bit ops (docs/STATUS.md:
# ElitePirate + Sandworm audited clean; SpacePirate/Ing non-slip ops) must raise no MASK/XBIT/CROP.
# Trees are read with `git archive` into a temp dir; the working tree is never touched.
#   tools/asmcheck_regress.sh            # PASS/FAIL per case, exit 1 on any FAIL
#   KEEP=1 tools/asmcheck_regress.sh     # keep the extracted trees (path printed)
set -u
cd "$(git rev-parse --show-toplevel)"
AC="python3 $PWD/tools/asmcheck.py"
E=src/MetroidPrime/Enemies
TMP=$(mktemp -d "${TMPDIR:-/tmp}/asmcheck-regress.XXXXXX")
[ -n "${KEEP:-}" ] && echo "trees: $TMP" || trap 'rm -rf "$TMP"' EXIT
fail=0

tree() { # rev -> extracted dir
  local d="$TMP/$(echo "$1" | tr '^/~' 'P__')"
  if [ ! -d "$d" ]; then
    mkdir -p "$d" && git archive "$1" include "$E" | tar -x -C "$d" || { echo "FAIL extract $1"; fail=1; }
  fi
  echo "$d"
}

check() { # module rev cpp-glob [fn...] -> findings on stdout
  local mod=$1 rev=$2 glob=$3; shift 3
  local d f; d=$(tree "$rev")
  for f in "$d"/$E/$glob; do
    [ -f "$f" ] && $AC check "$mod" "$f" --root "$d" "$@" 2>&1 | grep -v -e '^MISSING' -e '^== '
  done
}

expect() { # name want(hit|none) pattern module rev cpp-glob [fn...]
  local name=$1 want=$2 pat=$3; shift 3
  local out; out=$(check "$@")
  if grep -Eq "$pat" <<<"$out"; then got=hit; else got=none; fi
  if [ "$got" = "$want" ]; then
    echo "PASS $name ($want: $pat)"
  else
    echo "FAIL $name (want $want, got $got: $pat)"
    grep -E "$pat" <<<"$out" | head -5 | sed 's/^/    /'
    fail=1
  fi
}

# 8a9fbb50c: CSpacePirate Leash `==` should be `>` (fcmpo; mfcr; extrwi 1,1 = GT)
expect leash-pre   hit  '^CROP fn_72_52F8 .*xbb0' SpacePirate 8a9fbb50c^ 'PortCSpacePirate*.cpp' fn_72_52F8
expect leash-post  none '^CROP fn_72_52F8 '       SpacePirate 8a9fbb50c  'PortCSpacePirate*.cpp' fn_72_52F8
# c412a6e5f: Ambushing reads x8f4_30_ (0x02), asm extrwi 1,25 = 0x40 (ceilingAmbush)
expect ambush-pre  hit  '^MASK fn_72_7630 .*x8f4' SpacePirate c412a6e5f^ 'PortCSpacePirate*.cpp' fn_72_7630
expect ambush-post none '^MASK fn_72_7630 '       SpacePirate c412a6e5f  'PortCSpacePirate*.cpp' fn_72_7630
# c412a6e5f: CheckBlade reads alwaysAggressive (0x20), asm 0xCCA4 tests 0x40 (appliedBladeDamage)
expect blade-pre   hit  '^MASK fn_72_CC80 .*x8f8' SpacePirate c412a6e5f^ 'PortCSpacePirate*.cpp' fn_72_CC80
expect blade-post  none '^MASK fn_72_CC80 '       SpacePirate c412a6e5f  'PortCSpacePirate*.cpp' fn_72_CC80
# 8efb0e851: Ing XENZ never sets mSafeZoneIngSpot (asm 0xCE40 rlwimi 5,26,26 on 0xe5e)
expect xenz-pre    hit  '^BIT fn_29_C9A4 .*xe5e_20' Ing 8efb0e851^ PortCIng.cpp fn_29_C9A4
expect xenz-post   none '^BIT fn_29_C9A4 .*xe5e_20' Ing 8efb0e851  PortCIng.cpp fn_29_C9A4
# 8efb0e851: Ing XEXZ guards the mSafeZoneCount (0xe2c) decrement; asm 0xCE74 is unconditional
expect xexz-pre    hit  '^XCMP fn_29_C9A4 .*xe2c'  Ing 8efb0e851^ PortCIng.cpp fn_29_C9A4
expect xexz-post   none '^XCMP fn_29_C9A4 .*xe2c'  Ing 8efb0e851  PortCIng.cpp fn_29_C9A4
# 08c486cbf: PreRender/Render were marked `// @` while incomplete; the fix unmarked them
expect prerender-pre  hit  '^(BIT|CMP|CALL|BRANCH) fn_29_C010 ' Ing 08c486cbf^ PortCIng.cpp fn_29_C010
expect prerender-post none '^(BIT|CMP|CALL|BRANCH) fn_29_C010 ' Ing 08c486cbf  PortCIng.cpp fn_29_C010
expect render-pre     hit  '^(BIT|CMP|CALL|CALLS|BRANCH) fn_29_C0B4 ' Ing 08c486cbf^ PortCIng.cpp fn_29_C0B4
expect render-post    none '^(BIT|CMP|CALL|CALLS|BRANCH) fn_29_C0B4 ' Ing 08c486cbf  PortCIng.cpp fn_29_C0B4

# audited ops stay quiet (every checked function of each tree)
expect quiet-elitepirate none '^(MASK|XBIT|CROP) ' ElitePirate decomp/elitepirate 'PortCElitePirate*.cpp'
expect quiet-sandworm    none '^(MASK|XBIT|CROP) ' Sandworm    decomp/wallcrawler  'PortCSandworm*.cpp'
expect quiet-spacepirate none '^(MASK|XBIT) '      SpacePirate c412a6e5f          'PortCSpacePirate*.cpp'
expect quiet-ing         none '^MASK '             Ing         decomp/ing         'PortCIng*.cpp'

[ $fail = 0 ] && echo "asmcheck_regress: all PASS" || echo "asmcheck_regress: FAIL"
exit $fail
