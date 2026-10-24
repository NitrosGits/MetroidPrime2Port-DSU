# PuddleSpore (module 51) forcing spec for tools/cov/force.sh, same area and driver as
# tools/cov/specs/puddlespore_pud1.con (merge its gcov dir with the natural runs' in
# tools/cov/gcov_report.py):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/pud1_force.py <out> \
#     3dfd2249,cbc72d79 --con tools/cov/specs/puddlespore_pud1.con --fast --frames 2200

def null_mgr(gdb):
    # fn_51_330C 0x340C/0x3424: only AIUpdateDisabled (0x58415544) and Delete (0x5844454c)
    # test the collision manager, and SetupCollisionManager has run since the first Think, so
    # the null arms need the member cleared at the entry of exactly those two messages.
    try:
        mid = int(gdb.parse_and_eval('((CScriptMsg*)$rdx)->m_msg'))
    except gdb.error:
        return
    if mid & 0xFFFFFFFF in (0x58415544, 0x5844454C):
        gdb.execute('set var ((CPuddleSpore*)$rdi)->mCollisionActorManager.mPtr = 0')


def not_ready(gdb):
    # fn_51_135C 0x1434 bl ShouldRetry: CAnimationState::CanIssueCommand is false while the
    # state is not kAS_Ready (0). Only the Update message (1) reads it.
    if int(gdb.parse_and_eval('$rdx')) & 0xFFFFFFFF == 1:
        gdb.execute('set var ((CPuddleSpore*)$rdi)->mAnimationState.mState = 0')


_runhits = [0]


def run_phase1(gdb):
    # fn_51_1574 0x15D8..0x169C: the phase-1 arm. Forced only for the first Run window
    # (hits 7..24) so the state machine still reaches Attack/GetUp afterwards (mAnimPhase 2).
    _runhits[0] += 1
    if 7 <= _runhits[0] <= 24:
        gdb.execute('set var ((CPuddleSpore*)$rdi)->mAnimPhase = 1')


CLASS = 'CPuddleSpore'
FORCE = [
    ('330C null collision manager', 'CPuddleSpore::AcceptScriptMsg(', [], [null_mgr], 0,
     'mCollisionActorManager cleared at the AIUpdateDisabled/Delete entries: the null arms'),
    ('1574 Run phase 1', 'CPuddleSpore::Run(', [], [run_phase1], 0,
     'mAnimPhase forced to 1 on every Update after the natural phase-0 frames: case 1'),
    ('2FB0 no shock-wave particle', 'CPuddleSpore::DoUserAnimEvent(', [],
     ['$this->mData.mShockWaveInfo.mShockWaveEffect = 0xffffffffu'], 2,
     'particle desc id invalid: 0x3014 beq takes the no-particle arm'),
    ('135C announcement not ready', 'CPuddleSpore::TurnAround(', [], [not_ready], 0,
     'mAnimationState kAS_NotReady on Update: the CanIssueCommand false arm'),
]
