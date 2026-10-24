# PlantScarabSwarm (module 49) psc5 gdb side: CPlantScarabSwarm::Think fn_49_1B64 arms.  This file is
# BOTH the force.sh spec argument and COV_GDB_SCRIPT (it does not read COV_SPEC; the fixture lives in
# the lane's own spec, no shared tooling):
#
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/psc5_think.py tools/cov/force.sh \
#     tools/cov/specs/psc5_think.py /tmp/opencode/psc5_r1 3dfd2249,0g_swamp_hall \
#     --con tools/cov/specs/psc5_think_0g.con --fast --frames 1200
#
# Read-only except ONE transient write: on the first Think call of this object after N natural calls,
# mAttackerCount is set to 0 for that call only and restored at the body's return through the cached
# absolute `this` pointer (cpp:90's "no attackers" side; the shipped data has attackerCount 3 on every
# placement).  No branch is forced, no mask is touched, no game file changes.
# Also records, read-only: GetActive() at entry (cpp:86, exercised by the driver's Deactivate) and every
# IsSpaceAboveBoidClear result (cpp:108's third test), which is the arm that needs real geometry.
import os

import gdb

OUT = os.environ['COV_OUT']
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
THINK = '_ZN17CPlantScarabSwarm5ThinkEfR13CStateManager'
SPACE = '_ZNK17CPlantScarabSwarm21IsSpaceAboveBoidClearERK13CStateManagerRKN12CSwarmBasics5CBoidE'
THIS = '((CPlantScarabSwarm*)$rdi)'
counts = {'think': 0, 'inactive': 0, 'zero_write': 0, 'space_calls': 0, 'space_false': 0}
seen = []
write_after = 40


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return 'err:' + str(e)[:40]


def once(key):
    if key in seen:
        return False
    seen.append(key)
    return True


class Restore(gdb.FinishBreakpoint):
    def __init__(self, frame, this, old):
        super().__init__(frame, internal=True)
        self.this, self.old = this, old

    def stop(self):
        try:
            gdb.execute('set var ((CPlantScarabSwarm*)0x%x)->mAttackerCount = %d' % (self.this, self.old))
            log('zero-attacker: restored this=0x%x mAttackerCount=%d' % (self.this, self.old))
        except gdb.error as e:
            log('zero-attacker: RESTORE FAILED on this=0x%x: %s' % (self.this, str(e)[:80]))
        return False

    def out_of_scope(self):
        log('zero-attacker: this=0x%x left without a return, not restored' % self.this)


class ThinkBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + THINK, internal=True)
        self.n = {}

    def stop(self):
        this = ev('$rdi')
        counts['think'] += 1
        self.n[this] = self.n.get(this, 0) + 1
        active = ev('((CEntity*)0x%x)->mActive' % this)
        if active == 0:
            counts['inactive'] += 1
            if once(('inactive', this)):
                log('86: this=0x%x GetActive()==0, the early return runs (cpp:86 true side)' % this)
        if self.n[this] < write_after or counts['zero_write']:
            return False
        if ev(THIS + '->mAttackerCount') != 3:
            return False
        try:
            Restore(gdb.newest_frame(), this, 3)
        except (gdb.error, ValueError) as e:
            log('zero-attacker: no finish breakpoint (%s)' % str(e)[:60])
            return False
        try:
            gdb.execute('set var ((CPlantScarabSwarm*)0x%x)->mAttackerCount = 0' % this)
        except gdb.error as e:
            log('zero-attacker: WRITE FAILED on this=0x%x: %s' % (this, str(e)[:80]))
            return False
        counts['zero_write'] += 1
        log('90: this=0x%x mAttackerCount 3 -> 0 for this call (cpp:90 false side, attack block '
            'skipped); restored at the body return' % this)
        return False


class SpaceRestore(gdb.FinishBreakpoint):
    """Read-only: the value IsSpaceAboveBoidClear answers (cpp:108's third test)."""

    def __init__(self, frame, this):
        super().__init__(frame, internal=True)
        self.this = this

    def stop(self):
        counts['space_calls'] += 1
        try:
            ret = int(gdb.parse_and_eval('$rax')) & 0xff
        except gdb.error:
            ret = -1
        if ret == 0:
            counts['space_false'] += 1
        if once(('space', self.this, ret)):
            log('108: IsSpaceAboveBoidClear answered %d on this=0x%x (space-above test)' % (ret, self.this))
        return False


class SpaceBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + SPACE, internal=True)

    def stop(self):
        this = ev('$rdi')
        try:
            SpaceRestore(gdb.newest_frame(), this)
        except (gdb.error, ValueError):
            pass
        return False


def on_exit(event):
    with open(TABLE, 'a') as fh:
        fh.write('think=%d inactive=%d zero_write=%d space_calls=%d space_false=%d\n' %
                 (counts['think'], counts['inactive'], counts['zero_write'], counts['space_calls'],
                  counts['space_false']))


gdb.events.exited.connect(on_exit)
ThinkBp()
SpaceBp()
log('psc5: fixture installed on Think and IsSpaceAboveBoidClear')
