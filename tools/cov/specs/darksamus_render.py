# DarkSamus render pass (fn_10_17F04 AddToRenderer, fn_10_185CC the fade factor, fn_10_4AA8), a
# forcing run on the coverage build: the phazon beam made by `uevent 27` and updated by
# `statecall PhazonAttack` (darksamus_render.con). The arms the fight cannot reach are written at
# AddToRenderer's entry, keyed by the count $ar of its calls while the beam exists, and put back
# at the next CDarkSamus::Render or Think entry (the coverage build is -O2, so no line breakpoints):
#   1    the enrage sphere 0x1028 lent the new beam (no particles yet: empty bounds)
#   2    not alive (0x420 bit 0x40)
#   3    the stage's fourth bool set, the dark visor, the fade window holding (fade 1)
#   4    the stage's bool set, the combat visor, the fade window fading (fade 0.5); the sweep
#        beam 0xD7C, both boost ball swooshes (0xEBC bit 0x40 clear) and 0x1028 lent the beam
#   5    no stage, the dark visor; the swooshes lent the beam with 0xEBC bit 0x40 set
#   6    the stage's bool clear, the dark visor; the swoosh 0xE3C lent the beam, 0xE4C null
#   60, 120, 200, 300  0x1028 lent the beam again, now with particles (its bounds)
# The lent pointers are not owned (the auto_ptrs' mHas stays false), so nothing deletes them.
# (Not $sp or $fp: gdb's stack and frame pointers.) The stage is forced through a finish breakpoint on GetBattleStage ($stm 1 bool set, 2 null,
# 3 bool clear; stage.log says whether the area has one).
# Not forced: line 405's host GetPlayerState() fallback to mPlayerStates[0] (the manager's
# mPlayerState is always set; CPatterned::AddToRenderer reads it unchecked just before).
# Run: BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darksamus_render.py <out>
#      "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_render.con --fast --frames 1000
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DarkSamus
import gdb
import os

CLASS = 'CDarkSamus'
B = '$this->mGuestEF8.mItem'


def at(n):
    return '(%s != 0 && $ar == %d)' % (B, n)


def any_of(*ns):
    return '(' + ' || '.join(at(n) for n in ns) + ')'


M = '((CStateManager*)$rsi)'
PS = '((CPlayerState*)$psp)'
ENTRY = [
    '$ar = $_isvoid($ar) ? 0 : $ar',
    '$ar = %s != 0 ? $ar + 1 : $ar' % B,
    # not alive
    '$fa = %s ? 1 : 0' % at(2),
    '$this->mAlive = $fa ? 0 : $this->mAlive',
    # the visor
    '$psp = (long)(%s->mPlayerState ? %s->mPlayerState : %s->mPlayerStates[0])' % (M, M, M),
    '$fv = %s ? 1 : 0' % any_of(3, 5, 6),
    '$vis = %s->mCurrentVisor' % PS,
    '%s->mCurrentVisor = $fv ? 3 : %s->mCurrentVisor' % (PS, PS),
    # the stage
    '$stm = %s ? 1 : %s ? 2 : %s ? 3 : 0' % (any_of(3, 4), at(5), at(6)),
    # the fade window (0xE04 the start, 0xE08 the hold, 0xE0C the fade)
    '$fe = %s ? 1 : 0' % any_of(3, 4),
    '$e4 = $this->mGuestE04', '$e8 = $this->mGuestE08', '$ec = $this->mGuestE0C',
    '$this->mGuestE04 = %s ? $this->mElapsedTime : %s ? $this->mElapsedTime - 1.5 : $this->mGuestE04'
    % (at(3), at(4)),
    '$this->mGuestE08 = %s ? 10 : %s ? 1 : $this->mGuestE08' % (at(3), at(4)),
    '$this->mGuestE0C = $fe ? 1 : $this->mGuestE0C',
    # the lent effects
    '$lent = %s ? 1 : 0' % any_of(1, 4, 5, 6, 60, 120, 200, 300),
    '$this->mSweepBeamGen.mItem = %s ? %s : $this->mSweepBeamGen.mItem' % (at(4), B),
    '$this->mBoostBallSwooshE3C = %s ? (CParticleSwoosh*)%s : $this->mBoostBallSwooshE3C'
    % (any_of(4, 5, 6), B),
    '$this->mBoostBallSwooshE4C = %s ? (CParticleSwoosh*)%s : $this->mBoostBallSwooshE4C'
    % (any_of(4, 5), B),
    '$fb = $this->mBoostBallFlag40',
    '$this->mBoostBallFlag40 = %s ? 1 : %s ? 0 : $this->mBoostBallFlag40' % (at(5), any_of(4, 6)),
    '$this->mGuest1028.mItem = %s ? %s : $this->mGuest1028.mItem' % (any_of(1, 4, 60, 120, 200, 300), B),
]
RESTORE = [
    '$fa = $_isvoid($fa) ? 0 : $fa',
    '$this->mAlive = $fa ? 1 : $this->mAlive', '$fa = 0',
    '$fv = $_isvoid($fv) ? 0 : $fv',
    '%s->mCurrentVisor = $fv ? $vis : %s->mCurrentVisor' % (PS, PS), '$fv = 0',
    '$fe = $_isvoid($fe) ? 0 : $fe',
    '$this->mGuestE04 = $fe ? $e4 : $this->mGuestE04',
    '$this->mGuestE08 = $fe ? $e8 : $this->mGuestE08',
    '$this->mGuestE0C = $fe ? $ec : $this->mGuestE0C', '$fe = 0',
    '$lent = $_isvoid($lent) ? 0 : $lent',
    '$this->mSweepBeamGen.mItem = $lent ? 0 : $this->mSweepBeamGen.mItem',
    '$this->mBoostBallSwooshE3C = $lent ? 0 : $this->mBoostBallSwooshE3C',
    '$this->mBoostBallSwooshE4C = $lent ? 0 : $this->mBoostBallSwooshE4C',
    '$this->mBoostBallFlag40 = $lent ? $fb : $this->mBoostBallFlag40',
    '$this->mGuest1028.mItem = $lent ? 0 : $this->mGuest1028.mItem', '$lent = 0',
]
# The stage's bool is put back by StageRestore (a store through $stp needs a non-null $stp).


class StageRet(gdb.FinishBreakpoint):
    def __init__(self, frame, mode):
        super().__init__(frame, internal=True)
        self.mode = mode

    def stop(self):
        rax = int(gdb.parse_and_eval('$rax'))
        with open(os.path.join(os.environ['COV_OUT'], 'stage.log'), 'a') as fh:
            fh.write('mode %d stage 0x%x\n' % (self.mode, rax))
        if self.mode == 2:
            gdb.execute('set var $rax = 0')
        elif rax != 0:
            f = '((CDarkSamusBattleStage*)%d)->mStageInfo.unknown_0x53fdcb5b' % rax
            gdb.execute('set var $stp = %d' % rax)
            gdb.execute('set var $sfo = %s' % f)
            gdb.execute('set var %s = %d' % (f, 1 if self.mode == 1 else 0))
        return False


class StageBp(gdb.Breakpoint):
    def stop(self):
        mode = gdb.parse_and_eval('$stm')
        mode = 0 if mode.type.code == gdb.TYPE_CODE_VOID else int(mode)
        if mode:
            gdb.execute('set var $stm = 0')
            StageRet(gdb.newest_frame(), mode)
        return False


class StageRestore(gdb.Breakpoint):
    def stop(self):
        sp = gdb.parse_and_eval('$stp')
        if sp.type.code != gdb.TYPE_CODE_VOID and int(sp) != 0:
            gdb.execute('set var ((CDarkSamusBattleStage*)%d)->mStageInfo.unknown_0x53fdcb5b = $sfo'
                        % int(sp))
            gdb.execute('set var $stp = 0')
        return False


def addr(name):
    # the address force.py's own breakpoints use (COV_NM, `nm` with the demangled name after a tab)
    for ln in open(os.environ['COV_NM']):
        parts = ln.rstrip('\n').split('\t')
        if len(parts) == 2 and parts[1].startswith(name):
            return '*' + parts[0].split()[2]
    raise RuntimeError('no symbol for ' + name)


StageBp(addr('CDarkSamus::GetBattleStage('), internal=True)
for fn in ('CDarkSamus::Render(', 'CDarkSamus::Think('):
    StageRestore(addr(fn), internal=True)

FORCE = [
    ('17F04 entry', 'CDarkSamus::AddToRenderer(', [], ENTRY, 0, 'keyed by $ar: see the header'),
    ('17F04 restore R', 'CDarkSamus::Render(', [], RESTORE, 0, 'the forced members put back'),
    ('17F04 restore T', 'CDarkSamus::Think(', [], RESTORE, 0, 'the forced members put back'),
]
