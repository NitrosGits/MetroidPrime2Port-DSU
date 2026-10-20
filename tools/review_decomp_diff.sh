#!/bin/bash
# Usage: tools/review_decomp_diff.sh <base> [head]   (default head: HEAD)
# Flags the slips MiniMax lanes make in decomp patches, on added lines of src/ and include/ C++:
#   NULL   null guards (retail rarely has them; each needs a reason or goes)
#   ADDR   retail address literals (0x80xxxxxx) - thunks that compare against retail addresses
#   STUB   stub/stand-in/TODO/forwarding text added next to ported code
#   MASK   bit tests (& 0x.., rlwinm-derived) - re-derive each mask from the asm, not the selftest
#   MARK   new `// @` markers - check the body really is the function and its callees are ported
# Prints "<tag> <file>:<line>: <text>" and a count per tag. A hit is a question, not a verdict.
set -u
base=${1:?usage: review_decomp_diff.sh <base> [head]}
head=${2:-HEAD}
git diff -U0 "$base" "$head" -- 'src/*.cpp' 'src/*.hpp' 'include/*.hpp' 'platform/*.cpp' |
awk '
  /^\+\+\+ b\// { f = substr($0, 7); next }
  /^@@/ { split($3, a, ","); ln = substr(a[1], 2) + 0; next }
  /^\+/ {
    t = substr($0, 2)
    if (t ~ /(==|!=)[ \t]*(nullptr|NULL)|(nullptr|NULL)[ \t]*(==|!=)|if[ \t]*\([ \t]*![ \t]*[A-Za-z_][A-Za-z_0-9>.-]*[ \t]*\)/) hit("NULL")
    if (t ~ /0x8[0-1][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]/) hit("ADDR")
    if (t ~ /[Ss]tub|[Ss]tand-in|TODO|FIXME|not ported|unported/) hit("STUB")
    if (t ~ /&[ \t]*0x[0-9A-Fa-f]+/ && t !~ /^[ \t]*\/\//) hit("MASK")
    if (t ~ /^[ \t]*\/\/ @ fn_/) hit("MARK")
    ln++
  }
  function hit(tag) { printf "%s %s:%d: %s\n", tag, f, ln, t; n[tag]++ }
  END { for (k in n) printf "== %s %d\n", k, n[k] }
'
