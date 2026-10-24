# PlantScarabSwarm (module 49) psc4 gdb side: the last live arm of PreRenderBoid (fn_49_12AC), cpp:229's
# false side - a slot-0 attacking boid drawn in a draw pass whose mAttackModelBitBase bit a shared pose
# already cleared.  This file is BOTH the force.sh spec argument and COV_GDB_SCRIPT (it does not read
# COV_SPEC; the fixture lives here so no shared tooling changes):
#
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/psc4_plant_arm.py tools/cov/force.sh \
#     tools/cov/specs/psc4_plant_arm.py /tmp/opencode/psc4_r1 3dfd2249,0g_swamp_hall \
#     --con tools/cov/specs/psc4_plant_0g.con --fast --frames 1200
#
# Fixture: the slot-0 side is a REAL boid (the swarm's own mAttackSlotBoids[0] owner, attacking, drawn
# after our pose boid in the same pass because the pose boid is the first active boid the pass draws).
# Only the shared pose is supplied: on the first PreRenderBoid call of a pass (mask == 0xffffffff) where
# slot 0 is owned and the boid is clean, that boid's mAttacking/mAttackSlot are written for THIS call
# only and restored at the body's return through the cached absolute boid pointer, before any later
# movement or render uses it.  No mask is ever written: the bit - mAttackModelBitBase is cleared by the
# real body (0x1318 andc), which is what makes the later real slot-0 call take the false side.
# Guard: shared attack model and skinned state non-null, this object's model/state/slot counts
# consistent, boid active, not attacking, slot -1, slot 0 free.
import os

import gdb

OUT = os.environ['COV_OUT']
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
BODY = '_ZN17CPlantScarabSwarm13PreRenderBoidEPN12CSwarmBasics5CBoidEPj'
THIS = '((CPlantScarabSwarm*)$rdi)'
BOID = '((CSwarmBasics::CBoid*)$rsi)'
counts = {'poses': 0, 'passes': 0, 'false_arm': 0}
seen = []
posed = {}


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return 'err:' + str(e)[:40]


def rd32(addr):
    try:
        return int(gdb.parse_and_eval('*(unsigned int*)0x%x' % addr))
    except gdb.error:
        return -1


def once(key):
    if key in seen:
        return False
    seen.append(key)
    return True


class Restore(gdb.FinishBreakpoint):
    """Writes the boid's two attack fields back at the body's return, through the absolute pointer."""

    def __init__(self, frame, addr, old_attacking, old_slot, mask_addr, mask_before):
        super().__init__(frame, internal=True)
        self.addr, self.oa, self.os_ = addr, old_attacking, old_slot
        self.mask_addr, self.mask_before = mask_addr, mask_before

    def stop(self):
        try:
            gdb.execute('set var ((CSwarmBasics::CBoid*)0x%x)->mAttacking = %d' % (self.addr, self.oa))
            gdb.execute('set var ((CSwarmBasics::CBoid*)0x%x)->mAttackSlot = %d' % (self.addr, self.os_))
            after = rd32(self.mask_addr)
            log('pose: restored boid 0x%x attacking=%d slot=%d; mask 0x%08x -> 0x%08x (body cleared '
                'bitBase)' % (self.addr, self.oa, self.os_, self.mask_before, after))
        except gdb.error as e:
            log('pose: RESTORE FAILED on boid 0x%x: %s' % (self.addr, str(e)[:80]))
        return False

    def out_of_scope(self):
        log('pose: boid 0x%x left without a return, not restored' % self.addr)


class Bp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + BODY, internal=True)
        self.posed_this_pass = False

    def stop(self):
        this = ev('$rdi')
        boid = ev('$rsi')
        maskp = ev('$rdx')
        mask = rd32(maskp)
        if mask == 0xffffffff:
            counts['passes'] += 1
            self.posed_this_pass = False
        shared_bit = ev(THIS + '->mSharedAttackBit')
        base = ev(THIS + '->mAttackModelBitBase')
        if isinstance(shared_bit, int) and isinstance(base, int) and shared_bit < 32 and base < 32:
            bit_shared = (mask >> shared_bit) & 1
            bit_base = (mask >> base) & 1
        else:
            bit_shared = bit_base = '?'
        attacking = ev(BOID + '->mAttacking')
        slot = ev(BOID + '->mAttackSlot')
        active = ev(BOID + '->mActive')

        # The arm itself: a real slot-0 attacker reaching its bit test with the bitBase bit cleared.
        if attacking == 1 and slot == 0 and bit_base == 0:
            if once(('false', this, boid)):
                counts['false_arm'] += 1
                log('229 FALSE ARM: this=0x%x boid=0x%x slot=0 mask=0x%08x (shared bit %s, bitBase %s '
                    'clear); models=%s states=%s base=%s sharedBit=%s slots=[%s]' %
                    (this, boid, mask, bit_shared, bit_base,
                     ev(THIS + '->mAttackModels.mCount'), ev(THIS + '->mAttackModelStates.mCount'),
                     base, shared_bit,
                     ', '.join(str(ev(THIS + '->mAttackSlotBoids.mItems[%d]' % i)) for i in range(3))))

        # At most three poses per object, and none once the arm has been seen: the pose is transient,
        # the real slot-0 owner only has to be drawn later in the same pass.
        if self.posed_this_pass or counts['false_arm'] or posed.get(this, 0) >= 3:
            return False
        if mask != 0xffffffff or bit_base != 1 or bit_shared != 1:
            return False
        if active != 1 or attacking != 0 or slot != -1:
            return False
        if ev(THIS + '->mSharedAttackModel.mPtr != 0') != 1 or \
           ev(THIS + '->mSharedAttackState.mPtr != 0') != 1:
            if once(('noguard', this)):
                log('pose: skipped, shared model/state missing on this=0x%x' % this)
            return False
        models = ev(THIS + '->mAttackModels.mCount')
        states = ev(THIS + '->mAttackModelStates.mCount')
        base_models = ev(THIS + '->mModelDatas.mCount')
        attackers = ev(THIS + '->mAttackerCount')
        slot_count = ev(THIS + '->mAttackSlotBoids.mCount')
        owner = ev(THIS + '->mAttackSlotBoids.mItems[0]') if slot_count == attackers else -2
        if models != attackers or states != attackers or base_models != base or models < 1:
            if once(('badcounts', this)):
                log('pose: skipped, counts inconsistent this=0x%x models=%s states=%s attackers=%s '
                    'modelDatas=%s base=%s' % (this, models, states, attackers, base_models, base))
            return False
        if owner == -1:
            if once(('noowner', this)):
                log('pose: waiting, no real slot-0 owner yet on this=0x%x (slots=[%s])' %
                    (this, ', '.join(str(ev(THIS + '->mAttackSlotBoids.mItems[%d]' % i)) for i in range(3))))
            return False
        try:
            fb = Restore(gdb.newest_frame(), boid, attacking, slot, maskp, mask)
        except (gdb.error, ValueError) as e:
            log('pose: no finish breakpoint (%s)' % str(e)[:60])
            return False
        del fb
        try:
            gdb.execute('set var ((CSwarmBasics::CBoid*)0x%x)->mAttacking = 1' % boid)
            gdb.execute('set var ((CSwarmBasics::CBoid*)0x%x)->mAttackSlot = -1' % boid)
        except gdb.error as e:
            log('pose: WRITE FAILED on boid 0x%x: %s' % (boid, str(e)[:80]))
            return False
        self.posed_this_pass = True
        counts['poses'] += 1
        posed[this] = posed.get(this, 0) + 1
        log('pose: this=0x%x boid=0x%x mask before=0x%08x (bitBase %s, shared %s); shared model+state '
            'present; models=%s states=%s modelDatas=%s attackers=%s sharedBit=%s; real slot-0 owner '
            'index %s; transient attacking=1 slot=-1 for this call' %
            (this, boid, mask, bit_base, bit_shared, models, states, base_models, attackers, shared_bit,
             owner))
        return False


def on_exit(event):
    with open(TABLE, 'a') as fh:
        fh.write('passes=%d poses=%d false_arm=%d\n' %
                 (counts['passes'], counts['poses'], counts['false_arm']))


gdb.events.exited.connect(on_exit)
Bp()
log('psc4: fixture installed on %s' % BODY)
