# DarkSamus boost ball roll (ds2opus): fn_10_A3B0 BoostBallRoll, fn_10_A1C0 StartBoostBallRoll,
# fn_10_A814 PlayBoostBallCollision, fn_10_AA04 RollToBoostHint and fn_10_A06C
# UpdateBoostBallAtActor. The coverage build is -O2, so the arms are forced where a call returns:
# a breakpoint on the instruction after the call (`loc:*<mangled>+<off>`, the offsets read from
# `disassemble /s` of build-port-cov) writes the result register ($rax for a bool or a pointer,
# $xmm0 for a float). Re-read the offsets after any change to these bodies.
#   MP2_INPUT=LAYER2@400 BUILD=build-port-cov SECS=1500 DSR_MODE=<1|2> tools/cov/force.sh
#   tools/cov/specs/darksamus_boostroll.py <out> "1baa96c2,10_cliff_rooftop"
#   --con tools/drivers/darksamus_boostroll.con --fast --frames 2600
# DSR_MODE=1: the hint lookup fails on call 30 (AA04's no-hint return), the next hint's and the
# path's Normalize tests fail on calls 90/120 and on the 20th last-hint test, the bit 0x80 of
# 0xEBC is set on every call (the 25.0f arrival and the far clear), every arrival reads the next
# hint's second value 1.0 (the collision) and its value -1.0, 0.05 and 5.0 on arrivals 1-3
# (1.0f, the 0.1f and the 2.0f clamps); the second collision reads the 'PART' token absent (it
# is restored after the call), the first one's facing Normalize fails; BoostBallRoll's 50th
# ball lookup fails.
# DSR_MODE=2: the next hint is absent on call 40 (before any arrival the last hint 0xECC is
# zero, so the path is zero) and on every call after the first arrival (the arrival without a
# next hint); after the second arrival the bit 0x40 of 0x318 reads set (BoostBallRoll runs
# fn_10_A1C0), whose first Normalize test fails; the hint's own Normalize test fails on call 10.
# The gdb counters must not be named after registers ($pc is the program counter).
import os
CLASS = 'CDarkSamus'
AA04 = '_ZN10CDarkSamus15RollToBoostHintER13CStateManagerf'
A3B0 = '_ZN10CDarkSamus13BoostBallRollER13CStateManager9EStateMsgf'
A1C0 = '_ZN10CDarkSamus18StartBoostBallRollER13CStateManager'
INC = '$%s = $_isvoid($%s) ? 1 : $%s + 1'


def at(sym, off):
    return 'loc:*%s+%d' % (sym, off)


MODE = os.environ.get('DSR_MODE', '1')
COMMON = [
    ('AA04 call', 'CDarkSamus::RollToBoostHint(', ['$this->mBoostBallFlag80', '$this->mGuestEC8'],
     [INC % ('r', 'r', 'r'), '$arr = $_isvoid($arr) ? 0 : $arr'] +
     (['$this->mBoostBallFlag80 = 1'] if MODE == '1' else []), 0,
     'mode 1: the bit 0x80 of 0xEBC set'),
    ('AA04 arrival', at(AA04, 1506), [], ['$arr = $arr + 1'], 0, 'counts the arrivals'),
    ('A3B0 call', 'CDarkSamus::BoostBallRoll(', ['$edx', '$this->mBoostBallFlag20'],
     ['$arr = $_isvoid($arr) ? 0 : $arr'], 0, 'the message'),
]
if MODE == '1':
    FORCE = COMMON + [
        ('AA04 hint null', at(AA04, 92), [], ['$rax = ($r == 30) ? 0 : $rax'], 0, 'TCastToPtr null on call 30'),
        ('AA04 toNext normalize', at(AA04, 641), [], ['$rax = ($r == 90) ? 0 : $rax'], 0, 'call 90'),
        ('AA04 fromLast normalize', at(AA04, 832), [], [INC % ('fl', 'fl', 'fl'), '$rax = ($fl == 20) ? 0 : $rax'], 0, '20th test'),
        ('AA04 dir normalize', at(AA04, 908), [], ['$rax = ($r == 120) ? 0 : $rax'], 0, 'call 120'),
        ('AA04 next value', at(AA04, 1602), [],
         ['$xmm0.v4_float[0] = ($arr == 1) ? -1.0 : (($arr == 2) ? 0.05 : (($arr == 3) ? 5.0 : $xmm0.v4_float[0]))'],
         0, 'value -1 / 0.05 / 5 on arrivals 1-3'),
        ('AA04 next value again', at(AA04, 1761), [],
         ['$xmm0.v4_float[0] = ($arr == 2) ? 0.05 : (($arr == 3) ? 5.0 : $xmm0.v4_float[0])'], 0, 'the same'),
        ('AA04 next value2', at(AA04, 1645), [], ['$xmm0.v4_float[0] = 1.0'], 0, 'second value 1.0'),
        ('A814 call', 'CDarkSamus::PlayBoostBallCollision(', ['$this->mBoostBallCollisionToken.m_valid'],
         [INC % ('pcn', 'pcn', 'pcn'), '$tv = $this->mBoostBallCollisionToken.m_valid',
          '$this->mBoostBallCollisionToken.m_valid = ($pcn == 2) ? 0 : $tv'], 0, 'collision 2: token absent'),
        ('A814 restore', at(AA04, 1832), [], ['((CDarkSamus*)$rbx)->mBoostBallCollisionToken.m_valid = $tv'], 0, 'restored'),
        ('AA04 face normalize', at(AA04, 1878), [], [INC % ('fn', 'fn', 'fn'), '$rax = ($fn == 1) ? 0 : $rax'], 0, 'collision 1'),
        ('A3B0 ball null', at(A3B0, 628), [], [INC % ('bb', 'bb', 'bb'), '$rax = ($bb == 50) ? 0 : $rax'], 0, '50th lookup'),
    ]
else:
    FORCE = COMMON + [
        ('AA04 next null', at(AA04, 580), [], ['$rax = ($r == 40 || $arr >= 1) ? 0 : $rax'], 0,
         'call 40 and after the first arrival'),
        ('A3B0 bit 0x40', at(A3B0, 654), [], ['$rax = ($arr >= 2) ? 1 : $rax'], 0, 'after arrival 2'),
        ('AA04 toHint normalize', at(AA04, 153), [], ['$rax = ($r == 10) ? 0 : $rax'], 0, 'call 10'),
        ('A1C0 normalize', at(A1C0, 150), [], [INC % ('an', 'an', 'an'), '$rax = ($an == 1) ? 0 : $rax'], 0, 'first call'),
    ]
