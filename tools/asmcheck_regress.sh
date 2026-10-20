#!/usr/bin/env bash
# Regression for tools/asmcheck.py: the six decomp slips fixed by hand must be flagged on the
# pre-fix tree (<commit>^) and gone on the fix commit, the audited bit ops (docs/STATUS.md:
# ElitePirate + Sandworm audited clean; SpacePirate/Ing non-slip ops) must raise no MASK/XBIT/CROP,
# fixed false positives stay gone on real trees, and a synthetic module (fx-*) pins each FP fix:
# the FP shape is quiet and the same shape with a planted slip is still flagged.
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
  [ -n "${SKIP:-}" ] && out=$(grep -Ev "$SKIP" <<<"$out")
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
# (C9A4 0xCDC4 is real: kSM_Create reads the cached mStartAsIngSpot, retail's inlined fn_29_4C88
# tests the data byte 0x9d0 bit 7; found by the fold, pinned by fold-mask below)
SKIP='^MASK fn_29_C9A4 0xCDC4:' \
expect quiet-ing         none '^MASK '             Ing         decomp/ing         'PortCIng*.cpp'

# False-positive fixes on real trees (each was a hit before the fix).
P=67663f343  # port with ElitePirate merged
# a ctor's inlined sub-object ctor (fn_15_B080 at this+0xc58) is folded in
expect fold-ctor     none '^XSTORE fn_15_A190 .*xc5c' ElitePirate $P PortCElitePirate.cpp fn_15_A190
# `addic. rX, this, 0x9e8`: placement new of an optional member passes it by address
expect placement-new none '^XSTORE fn_15_A190 .*x9e8' ElitePirate $P PortCElitePirate.cpp fn_15_A190
# `x += d; if (x > 1) x = 1;`: MWCC compares the sum it stores (join of the two values)
expect clamp-store   none '^XCMP fn_15_8EA0 '         ElitePirate $P PortCElitePirate.cpp fn_15_8EA0
# the id goes to fn_15_2448 through a stack copy of the field: the callee's compare folds in
expect fold-stackarg none '^XCMP fn_15_24F0 .*x9fa'   ElitePirate $P PortCElitePirate.cpp fn_15_24F0
# `subfic` keeps the field as a diff operand of the compare
expect subfic        none '^XCMP fn_15_1C7C '         ElitePirate $P PortCElitePirate.cpp fn_15_1C7C
# ... and the fold still finds real slips: kSM_Create's mStartAsIngSpot (see quiet-ing)
expect fold-mask     hit  '^MASK fn_29_C9A4 0xCDC4:' Ing $P PortCIng.cpp fn_29_C9A4
# guest-class members commented `CIng: 0x..` have no IngBoostBallGuardian position: NOPOS, no XBIT
expect bbg-xbit      none '^XBIT fn_30_EC6C '  IngBoostBallGuardian 87a580c7e 'PortCIngBoostBallGuardian.cpp' fn_30_EC6C
expect bbg-nopos     hit  '^NOPOS fn_30_EC6C .*mStartAsIngSpot' IngBoostBallGuardian 87a580c7e 'PortCIngBoostBallGuardian.cpp' fn_30_EC6C

# Synthetic module Fx: per pattern, `good` (the FP shape, quiet now) and `slip` (the same shape
# with a real slip, still flagged). The asm is `ins args` lines, addressed from `.fn fn_99_<X>`.
FX=$TMP/fx
mkdir -p $FX/Fx/asm $FX/good/include $FX/good/src $FX/slip/include $FX/slip/src
python3 -c '
import sys
a = 0
for ln in sys.stdin:
    ln = ln.strip()
    if ln.startswith(".fn"):
        a = int(ln.split()[1].rstrip(",").split("_")[2], 16)
    if ln.startswith("."):
        print(ln)
    elif ln:
        print("/* %08X %08X  00 00 00 00 */\t%s" % (a, a + 0x100, ln))
        a += 4' >$FX/Fx/asm/text.s <<'ASM'
.fn fn_99_100, global
lwz r0, 0x10(r3)
cmpwi r0, 0x3
mfcr r0
extrwi r3, r0, 1, 1
blr
.endfn fn_99_100
.fn fn_99_300, global
li r0, 0x5
stw r0, 0x18(r3)
blr
.endfn fn_99_300
.fn fn_99_200, global
stwu r1, -0x10(r1)
mflr r0
stw r0, 0x14(r1)
bl fn_99_300
lwz r0, 0x14(r1)
mtlr r0
addi r1, r1, 0x10
blr
.endfn fn_99_200
.fn fn_99_400, global
bl Something__Fv
blr
.endfn fn_99_400
.fn fn_99_500, global
bl Other__Fv
blr
.endfn fn_99_500
.fn fn_99_600, global
bl fn_99_400
bl fn_99_500
blr
.endfn fn_99_600
.fn fn_99_700, global
lfs f0, 0x0(r4)
stfs f0, 0x24(r3)
blr
.endfn fn_99_700
.fn fn_99_800, global
lwz r0, 0x30(r3)
cmpwi r0, 0x2
bne .L_00000814
li r0, 0x1
stw r0, 0x18(r3)
.L_00000814:
blr
.endfn fn_99_800
.fn fn_99_900, global
lwz r0, 0x10(r3)
cmpwi r0, 0x0
beq .L_00000914
li r0, 0x1
stw r0, 0x18(r3)
.L_00000914:
blr
.endfn fn_99_900
.fn fn_99_A00, global
lis r4, gTok@ha
addi r4, r4, gTok@l
addi r3, r3, 0x38
bl __ct__4CTokFRC4CTok
blr
.endfn fn_99_A00
.fn fn_99_B00, global
lbz r0, 0x34(r3)
li r4, 0x1
rlwimi r0, r4, 6, 25, 25
stb r0, 0x34(r3)
blr
.endfn fn_99_B00
.fn fn_99_C00, global
lbz r0, 0x44(r3)
ori r0, r0, 0x1
stb r0, 0x44(r3)
blr
.endfn fn_99_C00
.fn fn_99_D00, global
lfs f1, 0x24(r3)
lfs f0, 0x0(r4)
fcmpo cr0, f1, f0
mfcr r0
extrwi r3, r0, 1, 1
blr
.endfn fn_99_D00
.fn fn_99_E00, global
lis r4, gTok@ha
addi r4, r4, gTok@l
addi r3, r3, 0x38
bl __ct__4CTokFi
blr
.endfn fn_99_E00
ASM
cat >$FX/good/include/Fx.hpp <<'HPP'
struct SSub {
  int mA;   // 0x0
  float mB; // 0x4
};
class CTok {
public:
  CTok(const CTok&);
};
extern CTok gTok;
class CFx {
public:
  bool IsBig() const;
  void Init();
  void Caller();
  void Caller2();
  void Helper2();
  void Helper3();
  void SetB(const float& b);
  void Poke();
  void Tick();
  void CopyTok(const CTok& t);
  void SetFlag();
  void SetFlagC();
  int GetState() const { return mState; }
  int GetOther() const { return mOther; }
  bool DebugFx() const;
  bool BBig(const float& t) const;
  void CopyTok();
  void CopyTok2();

  int mCount;      // 0x10
  float mSpeed;    // 0x14
  int mOther;      // 0x18
  SSub mSub;       // 0x20
  int mState;      // 0x30
  bool mFlagA : 1; // 0x34 bit 7
  bool mFlagB : 1; // Ing: 0x40 bit 0; Fx: 0x34 bit 6
  bool mFlagC : 1; // Ing: 0x44 bit 0
  CTok mTok;       // 0x38
};
HPP
cp $FX/good/include/Fx.hpp $FX/slip/include/
cat >$FX/good/src/PortFx.cpp <<'CPP'
#include "Fx.hpp"
#include <stdio.h>

// @ fn_99_100
bool CFx::IsBig() const { return mCount > 3; }

// @ fn_99_200: fn_99_300 is inlined here
void CFx::Init() { mOther = 5; }

// @ fn_99_600
void CFx::Caller() {
  // fn_99_400
  Helper2();
  Helper3();
}

void CFx::Helper2() { Something(); }

void CFx::Helper3() { // fn_99_500
  Other();
}

// @ fn_99_700
void CFx::SetB(const float& b) { mSub.mB = b; }

// @ fn_99_800
void CFx::Poke() {
  if (GetState() == 2) {
    mOther = 1;
  }
}

// @ fn_99_900
void CFx::Tick() {
  if (DebugFx()) {
    printf("a\n");
  }
  if (DebugFx()) {
    printf("b\n");
  }
  if (DebugFx()) {
    printf("c\n");
  }
  if (DebugFx()) {
    printf("d\n");
  }
  if (DebugFx()) {
    printf("e\n");
  }
  if (DebugFx()) {
    printf("f\n");
  }
  if (mCount != 0) {
    mOther = 1;
  }
}

// @ fn_99_A00
void CFx::CopyTok() { mTok = gTok; }

// @ fn_99_B00
void CFx::SetFlag() { mFlagB = true; }

// @ fn_99_C00
void CFx::SetFlagC() { mFlagC = true; }

// @ fn_99_D00
bool CFx::BBig(const float& t) const { return mSub.mB > t; }
CPP
cat >$FX/slip/src/PortFx.cpp <<'CPP'
#include "Fx.hpp"
#include <stdio.h>

// @ fn_99_100
bool CFx::IsBig() const { return mOther > 3; }

// @ fn_99_200: fn_99_300 is inlined here
void CFx::Init() { mState = 5; }

// @ fn_99_600
void CFx::Caller() {
  // fn_99_444
  Helper2();
  Helper2();
}

void CFx::Helper2() { Something(); }

void CFx::Helper3() { // fn_99_500
  Other();
}

// @ fn_99_700
void CFx::SetB(const float& b) { mSub.mA = b; }

// @ fn_99_800
void CFx::Poke() {
  if (GetOther() == 2) {
    mOther = 1;
  }
}

// @ fn_99_900
void CFx::Tick() {
  if (mCount == 1) mOther = 1;
  if (mCount == 2) mOther = 2;
  if (mCount == 3) mOther = 3;
  if (mCount == 4) mOther = 4;
  if (mCount == 5) mOther = 5;
  if (mCount != 0) {
    mOther = 1;
  }
}

// @ fn_99_A00
void CFx::CopyTok() { mTok = gTok; }

// @ fn_99_B00
void CFx::SetFlag() { mFlagA = true; }

// @ fn_99_C00
void CFx::SetFlagC() { mFlagC = true; }

// @ fn_99_D00
bool CFx::BBig(const float& t) const { return mSub.mA > t; }

// @ fn_99_E00
void CFx::CopyTok2() { mTok = gTok; }
CPP
fx() { # name want pattern variant [check args...]
  local name=$1 want=$2 pat=$3 v=$4; shift 4
  local out got; out=$($AC check $FX/Fx/asm $FX/$v/src/PortFx.cpp --root $FX/$v "$@" 2>&1)
  if grep -Eq "$pat" <<<"$out"; then got=hit; else got=none; fi
  if [ "$got" = "$want" ]; then
    echo "PASS fx-$name ($want: $pat)"
  else
    echo "FAIL fx-$name (want $want, got $got: $pat)"
    grep -E "$pat" <<<"$out" | head -5 | sed 's/^/    /'
    fail=1
  fi
}
# 1. a compare inside `return` (mfcr extract)
fx return-good none '^(CMP|XCMP) fn_99_100 ' good
fx return-slip hit  '^XCMP fn_99_100 .*x18'  slip
# 2. an inlined local callee (fn_99_300) is folded into its caller and counts as the call
fx fold-good   none '^(XSTORE|CALL) fn_99_200 ' good
fx fold-slip   hit  '^XSTORE fn_99_200 .*x30' slip
# 3. a whole-line `// fn_X` on/above a call names it; a signature-line `// fn_X` maps fn -> def
fx callcmt-good none '^CALL fn_99_600 '            good
fx callcmt-slip hit  '^CALL fn_99_600 .*fn_99_400' slip
fx sigid-slip   hit  '^CALL fn_99_600 .*fn_99_500 \(Helper3\)' slip
# 4. member-of-member offsets (mSub.mB = 0x24)
fx subfield-good none '^(STORE|XSTORE|CMP|XCMP) fn_99_(700|D00) ' good
fx subfield-slip hit  '^XSTORE fn_99_700 .*x20' slip
fx subfield-cmp  hit  '^XCMP fn_99_D00 .*x20'   slip
# 5. a trivial inline getter reads its member
fx getter-good none '^(CMP|XCMP) fn_99_800 ' good
fx getter-slip hit  '^XCMP fn_99_800 .*x18'  slip
# 6. debug-only `if (DebugFx()) { printf }` blocks don't count as conditions
fx debug-good  none '^BRANCH fn_99_900 ' good
fx debug-slip  hit  '^BRANCH fn_99_900 ' slip
# 7. a copy ctor (`mTok = gTok`) is no CALL miss; another ctor still is
fx copyctor-good none '^CALL fn_99_A00 '            good
fx copyctor-slip hit  '^CALL fn_99_E00 .*CTokFi'    slip
# 8. per-module positions (`Ing: 0x40 bit 0; Fx: 0x34 bit 6`); no position for Fx -> NOPOS
fx pos-good    none '^(MASK|XBIT) fn_99_B00 ' good
fx pos-slip    hit  '^MASK fn_99_B00 .*x34_80' slip
fx pos-module  hit  '^XBIT fn_99_B00 .*x40'    good --module Ing
fx nopos       hit  '^NOPOS fn_99_C00 .*mFlagC' good
fx nopos-quiet none '^(MASK|XBIT) fn_99_C00 '  good

[ $fail = 0 ] && echo "asmcheck_regress: all PASS" || echo "asmcheck_regress: FAIL"
exit $fail
