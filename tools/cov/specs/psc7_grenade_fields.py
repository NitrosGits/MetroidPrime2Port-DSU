# PlantScarabSwarm (module 49) psc7 fixture for CPlantScarabSwarm::SpawnGrenades fn_49_C6C: the POI
# predicate arms that psc6 could only observe are measured here by cycling the REAL first CInt32POINode's
# scalar fields for one call at a time.  This file is BOTH the force.sh spec argument and
# COV_GDB_SCRIPT (the fixture lives in the lane's own spec; no shared tooling).
#
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/psc7_grenade_fields.py tools/cov/force.sh \
#     tools/cov/specs/psc7_grenade_fields.py /tmp/opencode/psc7_r1 3dfd2249,0g_swamp_hall \
#     --con tools/cov/specs/psc6_grenade_0g.con --fast --frames 1200
#
# Every coverage figure from a forced call is labelled "forced node-field fixture", never shipped-asset
# behaviour.  The fixture acts only on a node-bearing call - a claimed slot whose boid is active, animData
# non-null and 0 < mPassedIntCount <= 16 - which is what makes it in-range: Think assigns attack slots only
# to boids that passed its proximity, timer, space-above and angle gates (cpp:107-110).
#
# It writes exactly three scalars of the FIRST node the body will walk, at absolute addresses computed from
# the static CAnimData::mInt32POINodes and the CInt32POINode layout, never count/pointer/type/name/slot/
# ownership.  The original RAW words are snapshotted first, a FinishBreakpoint is installed BEFORE any
# write, and the same original words are written back at the body's return (before any later animation
# processing), then read back and logged - so the log proves the ORIGINAL was restored, not the target.
#
# Phases, one per node-bearing call, cycling: weight0 (weight 0, the RNG is not forced), equal (weight 1,
# character == animData.mCharIdx, value 26), unequal (weight 1, non-negative character != mCharIdx),
# nonobjectdrop (weight 1, character -1, value 0), natural (no writes).
import os

import gdb

OUT = os.environ['COV_OUT']
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
BODY = '_ZN17CPlantScarabSwarm13SpawnGrenadesER13CStateManagerR9CAnimDatai'
THIS = '((CPlantScarabSwarm*)$rdi)'
ANIM = '((CAnimData*)$rdx)'
PHASES = ['weight0', 'equal', 'unequal', 'nonobjectdrop', 'natural']
F32_0 = 0x00000000
F32_1 = 0x3f800000
counts = {'calls': 0, 'node_calls': 0, 'writes': 0, 'restored': 0, 'mismatch': 0, 'reentrant': 0}
state = {'active': False}
seen = []


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


def field_offset(tname, fname):
    """Byte offset of a field, walking one level of base classes (host layout, from the DWARF)."""
    t = gdb.lookup_type(tname)
    for f in t.fields():
        if f.name == fname:
            return f.bitpos // 8
        if getattr(f, 'is_base_class', False):
            for bf in f.type.fields():
                if bf.name == fname:
                    return (f.bitpos + bf.bitpos) // 8
    return None


def rd32(addr):
    try:
        return int(gdb.parse_and_eval('*(unsigned int*)0x%x' % addr))
    except gdb.error:
        return -1


class Restore(gdb.FinishBreakpoint):
    """Writes the snapshot ORIGINAL words back at the body's return and proves it by reading them back."""

    def __init__(self, frame, addrs, orig, target, phase):
        super().__init__(frame, internal=True)
        self.addrs, self.orig, self.target, self.phase = addrs, orig, target, phase

    def stop(self):
        state['active'] = False
        now = [rd32(a) for a in self.addrs]
        try:
            for a, o in zip(self.addrs, self.orig):
                gdb.execute('set {unsigned int}0x%x = 0x%x' % (a, o))
            back = [rd32(a) for a in self.addrs]
        except gdb.error as e:
            counts['mismatch'] += 1
            log('%s: RESTORE FAILED at 0x%x/0x%x/0x%x: %s'
                % (self.phase, self.addrs[0], self.addrs[1], self.addrs[2], str(e)[:80]))
            return False
        if back == self.orig:
            counts['restored'] += 1
            log('%s: restored ORIGINAL words %s at 0x%x/0x%x/0x%x (read back %s)%s'
                % (self.phase, [hex(x) for x in self.orig], self.addrs[0], self.addrs[1],
                   self.addrs[2], [hex(x) for x in back],
                   '' if now == self.target else
                   ' [NOTE: a writer changed the words while the forced call ran: before '
                   'the restore they read %s, our targets were %s]'
                   % ([hex(x) for x in now], [hex(x) for x in self.target])))
        else:
            counts['mismatch'] += 1
            log('%s: RESTORE MISMATCH wanted %s, read %s'
                % (self.phase, [hex(x) for x in self.orig], [hex(x) for x in back]))
        return False

    def out_of_scope(self):
        state['active'] = False
        log('%s: left without a return, not restored' % self.phase)


class Bp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + BODY, internal=True)
        self.off = {n: field_offset('CInt32POINode', n) for n in ('mWeight', 'mCharIdx', 'mVal')}

    def stop(self):
        counts['calls'] += 1
        if state['active']:
            counts['reentrant'] += 1
            if once(('reentrant',)):
                log('REENTRANT SpawnGrenades inside a forced call: not acting (report this)')
            return False
        slot = ev('$rcx')
        anim = ev('$rdx')
        slots = ev(THIS + '->mAttackSlotBoids.mCount')
        boids = ev(THIS + '->mBoids.mCount')
        if not all(isinstance(x, int) for x in (slot, anim, slots, boids)) or anim == 0:
            return False
        if not 0 <= slot < slots:
            return False
        idx = ev(THIS + '->mAttackSlotBoids.mItems[%d]' % slot)
        if not isinstance(idx, int) or idx < 0 or idx >= boids:
            return False
        boid = ev('(long)&((CPlantScarabSwarm*)$rdi)->mBoids.mItems[%d]' % idx)
        if not isinstance(boid, int) or boid == 0:
            return False
        if ev('((CSwarmBasics::CBoid*)0x%x)->mActive' % boid) != 1:
            return False
        passed = ev(ANIM + '->mPassedIntCount')
        if not isinstance(passed, int) or passed <= 0 or passed > 16:
            return False
        base = ev('(long)&CAnimData::mInt32POINodes.mData')
        if not isinstance(base, int) or base == 0:
            base = ev('(long)&CAnimData::mInt32POINodes') + field_offset(
                'rstl::reserved_vector<CInt32POINode, 16>', 'mData')
            if not isinstance(base, int) or base == 0:
                return False
        addrs = [base + self.off['mWeight'], base + self.off['mCharIdx'], base + self.off['mVal']]
        orig = [rd32(a) for a in addrs]
        if -1 in orig:
            return False
        counts['node_calls'] += 1
        phase = PHASES[(counts['node_calls'] - 1) % len(PHASES)]
        anim_char = ev(ANIM + '->mCharIdx')
        if not isinstance(anim_char, int):
            return False
        alt_char = (anim_char + 1) if anim_char < 0x7fffffff else (anim_char - 1)
        if alt_char < 0:
            alt_char = 0 if anim_char != 0 else 1
        target = {
            'weight0': [F32_0, orig[1], orig[2]],
            'equal': [F32_1, anim_char & 0xffffffff, 0x1a],
            'unequal': [F32_1, alt_char & 0xffffffff, 0x1a],
            'nonobjectdrop': [F32_1, 0xffffffff, 0x00000000],
            'natural': orig,
        }[phase]
        if phase == 'natural':
            log('natural: boid=%d slot=%s passed=%d node=0x%x words=%s (no writes)'
                % (idx, slot, passed, base, [hex(x) for x in orig]))
            return False
        try:
            Restore(gdb.newest_frame(), addrs, orig, target, phase)
        except (gdb.error, ValueError) as e:
            log('%s: no finish breakpoint (%s), not writing' % (phase, str(e)[:60]))
            return False
        try:
            for a, w in zip(addrs, target):
                gdb.execute('set {unsigned int}0x%x = 0x%x' % (a, w))
        except gdb.error as e:
            log('%s: WRITE FAILED: %s' % (phase, str(e)[:80]))
            return False
        state['active'] = True
        counts['writes'] += 1
        log('%s: boid=%d slot=%s passed=%d animChar=%d node=0x%x originals=%s targets=%s '
            '(forced node-field fixture)' %
            (phase, idx, slot, passed, anim_char, base, [hex(x) for x in orig],
             [hex(x) for x in target]))
        return False


def on_exit(event):
    with open(TABLE, 'a') as fh:
        fh.write('calls=%d node_calls=%d writes=%d restored=%d mismatch=%d reentrant=%d\n' %
                 (counts['calls'], counts['node_calls'], counts['writes'], counts['restored'],
                  counts['mismatch'], counts['reentrant']))


gdb.events.exited.connect(on_exit)
Bp()
log('psc7: forced node-field fixture installed on SpawnGrenades')
