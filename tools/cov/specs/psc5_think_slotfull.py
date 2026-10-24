# PlantScarabSwarm (module 49) psc5 second launch gdb side: the last edge of CPlantScarabSwarm::Think
# (cpp:112), the inner slot loop's exhausted exit (0x1E0C bdnz falls through to 0x1E10).
# The separate 0x1D74 cmplwi / 0x1D78 ble tests zero count at entry, not full slots.
#
# Why it is not simply dead: `attackers` (cpp:93-98) is a snapshot taken before the loop and is never
# incremented in it, so a snapshot below mAttackerCount does not by itself mean a slot is free when a
# candidate boid reaches the loop.  All three slots can be held while `attackers` is smaller:
# CSwarmBasics deactivates a boid without clearing its attack slot in two places
# (CSwarmBasics.cpp:642, the 30-frames-off-surface case, and CSwarmBasics.cpp:1409, the waypoint case),
# and a deactivated holder is no longer counted by cpp:95 but keeps mAttackSlot/mAttackSlotBoids.
#
# This fixture builds exactly that state for one Think call at a time, on the real object:
#   - all mAttackSlotBoids[0..2] owned (guard), one holder with mActive == 1 and mAttacking == 1,
#   - that holder's mActive is set 0 for the call (so `attackers` drops below mAttackerCount and the
#     attack block at cpp:100 runs while no slot is free),
#   - mTimeSinceLastAttack is set to mAttackTimer for the call (the natural window is 0.5 s and every
#     allocation resets it to 0, so without this a candidate would rarely pass cpp:108),
# and both writes are restored at the body's return through cached absolute pointers, before anything
# else in the frame sees them.  No branch is forced, no mask is touched, no game file changes; at most
# 30 calls are prepared, once each.
#
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/psc5_think_slotfull.py tools/cov/force.sh \
#     tools/cov/specs/psc5_think_slotfull.py /tmp/opencode/psc5_r2 3dfd2249,0g_swamp_hall \
#     --con tools/cov/specs/psc5_think_0g2.con --fast --frames 1200
import os

import gdb

OUT = os.environ['COV_OUT']
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
THINK = '_ZN17CPlantScarabSwarm5ThinkEfR13CStateManager'
THIS = '((CPlantScarabSwarm*)$rdi)'
counts = {'think': 0, 'attempts': 0, 'written': 0}
seen = []
max_attempts = 30


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
    """Puts the holder's mActive and the timer's exact bits back at the body's return."""

    def __init__(self, frame, holder, this, t_addr, t_bits):
        super().__init__(frame, internal=True)
        self.holder, self.this, self.t_addr, self.t_bits = holder, this, t_addr, t_bits

    def stop(self):
        try:
            gdb.execute('set var ((CSwarmBasics::CBoid*)0x%x)->mActive = 1' % self.holder)
            gdb.execute('set {unsigned int}0x%x = 0x%x' % (self.t_addr, self.t_bits))
            log('slot-full: restored holder 0x%x mActive=1 and this=0x%x mTimeSinceLastAttack bits '
                '0x%08x' % (self.holder, self.this, self.t_bits))
        except gdb.error as e:
            log('slot-full: RESTORE FAILED (holder 0x%x this=0x%x): %s'
                % (self.holder, self.this, str(e)[:80]))
        return False

    def out_of_scope(self):
        log('slot-full: holder 0x%x left without a return, not restored' % self.holder)


class ThinkBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + THINK, internal=True)

    def stop(self):
        this = ev('$rdi')
        counts['think'] += 1
        if counts['attempts'] >= max_attempts:
            return False
        slots = ev(THIS + '->mAttackSlotBoids.mCount')
        attackers = ev(THIS + '->mAttackerCount')
        models = ev(THIS + '->mAttackModels.mCount')
        states = ev(THIS + '->mAttackModelStates.mCount')
        boids = ev(THIS + '->mBoids.mCount')
        if slots != 3 or attackers != 3 or models != 3 or states != 3:
            return False
        owners = [ev(THIS + '->mAttackSlotBoids.mItems[%d]' % i) for i in range(3)]
        if any(not isinstance(o, int) or o < 0 or o >= boids for o in owners):
            return False
        holder = -1
        for idx in owners:
            addr = ev('(long)&((CPlantScarabSwarm*)$rdi)->mBoids.mItems[%d]' % idx)
            if not isinstance(addr, int) or addr == 0:
                continue
            if ev('((CSwarmBasics::CBoid*)0x%x)->mActive' % addr) == 1 and \
               ev('((CSwarmBasics::CBoid*)0x%x)->mAttacking' % addr) == 1:
                holder = addr
                break
        if holder == -1:
            return False
        t_addr = ev('(long)&((CPlantScarabSwarm*)$rdi)->mTimeSinceLastAttack')
        if not isinstance(t_addr, int) or t_addr == 0:
            return False
        t_bits = ev('*(unsigned int*)0x%x' % t_addr)
        t_val = ev(THIS + '->mTimeSinceLastAttack')  # logging only (int() of a float)
        counts['attempts'] += 1
        try:
            Restore(gdb.newest_frame(), holder, this, t_addr, t_bits)
        except (gdb.error, ValueError) as e:
            log('slot-full: no finish breakpoint (%s)' % str(e)[:60])
            return False
        try:
            gdb.execute('set var ((CSwarmBasics::CBoid*)0x%x)->mActive = 0' % holder)
            gdb.execute('set var ((CPlantScarabSwarm*)0x%x)->mTimeSinceLastAttack = '
                        '((CPlantScarabSwarm*)0x%x)->mAttackTimer' % (this, this))
        except gdb.error as e:
            log('slot-full: WRITE FAILED (holder 0x%x this=0x%x): %s'
                % (holder, this, str(e)[:80]))
            return False
        counts['written'] += 1
        log('slot-full: this=0x%x attempt %d; slots owned by boid indices %s, holder 0x%x set '
            'mActive=0 so attackers 3 -> 2 for this call; mTimeSinceLastAttack %d -> mAttackTimer; '
            'models=%d states=%d' % (this, counts['attempts'], owners, holder, t_val, models, states))
        return False


def on_exit(event):
    with open(TABLE, 'a') as fh:
        fh.write('think=%d attempts=%d written=%d\n' %
                 (counts['think'], counts['attempts'], counts['written']))


gdb.events.exited.connect(on_exit)
ThinkBp()
log('psc5 r2: slot-full fixture installed on Think')
