# PlantScarabSwarm (module 49) psc6 read-only probe for CPlantScarabSwarm::SpawnGrenades fn_49_C6C.
# This file is BOTH the force.sh spec argument and COV_GDB_SCRIPT (the fixture lives in the lane's own
# spec; no shared tooling).  It writes nothing: at each SpawnGrenades entry it records the real slot the
# caller passed, the slot's boid index, the animation's character index and the int32 POI entries the body
# is about to walk (the static CAnimData::mInt32POINodes with count = animData.mPassedIntCount, exactly
# what GetInt32POIList returns).  Note that this buffer is STATIC and shared by every CAnimData and every
# animation advance appends into it at that object's own count, so a log line is the buffer state at that
# read instant (normally the last advance's node, not necessarily the passed animData's own list): it is a
# measurement of what the body walked, not an enumeration of the shipped POI data.
#
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/psc6_grenade_probe.py tools/cov/force.sh \
#     tools/cov/specs/psc6_grenade_probe.py /tmp/opencode/psc6_r1 3dfd2249,0g_swamp_hall \
#     --con tools/cov/specs/psc6_grenade_0g.con --fast --frames 1200
import os

import gdb

OUT = os.environ['COV_OUT']
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
BODY = '_ZN17CPlantScarabSwarm13SpawnGrenadesER13CStateManagerR9CAnimDatai'
THIS = '((CPlantScarabSwarm*)$rdi)'
ANIM = '((CAnimData*)$rdx)'
LIST = "CAnimData::mInt32POINodes"
# rstl::reserved_vector stores its items in a byte array, so the node pointer is a cast of it.
NODE = "((CInt32POINode*)(void*)&CAnimData::mInt32POINodes.mData)"
counts = {'calls': 0, 'no_slot': 0, 'with_nodes': 0, 'zero_count': 0}
seen = []


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return 'err:' + str(e)[:40]


def evf(expr):
    try:
        return float(gdb.parse_and_eval(expr))
    except gdb.error:
        return 'err'


def once(key):
    if key in seen:
        return False
    seen.append(key)
    return True


class Bp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + BODY, internal=True)

    def stop(self):
        counts['calls'] += 1
        slot = ev('$rcx')
        slots = ev(THIS + '->mAttackSlotBoids.mCount')
        boid = ev(THIS + '->mAttackSlotBoids.mItems[%d]' % slot) if (
            isinstance(slot, int) and isinstance(slots, int) and 0 <= slot < slots) else 'slot?'
        if boid == -1 or boid == 'slot?':
            counts['no_slot'] += 1
            if once(('noslot', slot, str(boid))):
                log('C6C: slot=%s boidIndex=%s -> early return (cpp:290)' % (slot, boid))
            return False
        anim_chars = ev(ANIM + '->mCharIdx')
        passed = ev(ANIM + '->mPassedIntCount')
        stored = ev(LIST + '.mCount')
        if not isinstance(passed, int) or passed <= 0:
            counts['zero_count'] += 1
        else:
            counts['with_nodes'] += 1
        nodes = []
        for i in range(min(passed, 16) if isinstance(passed, int) else 0):
            nodes.append((evf(NODE + '[%d].mWeight' % i),
                          ev(NODE + '[%d].mCharIdx' % i),
                          ev(NODE + '[%d].mVal' % i),
                          ev(NODE + '[%d].mType' % i),
                          ev(NODE + '[%d].mNameHash' % i)))
        key = (passed, anim_chars, tuple(nodes))
        if once(('key',) + key):
            log('C6C: boidIndex=%s slot=%s animChar=%s passedInt=%s stored=%s nodes=%s' %
                (boid, slot, anim_chars, passed, stored, nodes))
        return False


def on_exit(event):
    with open(TABLE, 'a') as fh:
        fh.write('calls=%d no_slot=%d with_nodes=%d zero_count=%d\n' %
                 (counts['calls'], counts['no_slot'], counts['with_nodes'], counts['zero_count']))


gdb.events.exited.connect(on_exit)
Bp()
log('psc6: read-only probe installed on SpawnGrenades')
