"""ldr3: IngSnatchingSwarm arm forcing at instruction sites, never production probes.

COV_GDB_SCRIPT for tools/cov/force.sh with spec tools/cov/specs/ingsnatchingswarm.py (CLASS):
  COV_GDB_SCRIPT=tools/cov/specs/ldr3_iss_verify.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
    tools/cov/specs/ingsnatchingswarm.py <out> 3bfa3eff,10_temple_pathofeyes \
    --con tools/cov/specs/ingsnatchingswarm_pathofeyes.con --fast --frames 2400

Each site is a pair of breakpoints in one straight-line run of the frozen binary
(sha256 FROZEN): ARM at the compare that decides the arm, RESTORE at the conditional
jump right after it, so only that one branch outcome changes. ARM snapshots the
register or memory bytes, writes the forced value and reads it back; RESTORE checks
it is the same thread and the same frame ($rsp) as ARM, writes the original bytes
back and reads them back. Every site fires once (after `skip` natural hits). No
inferior calls: stop() only reads and writes registers and memory. Log: ldr3_iss.jsonl.
"""
import gdb
import hashlib
import json
import os

FROZEN = 'febd723c9a9bebfdcb82d2084f9fc469024a6d3490719c2ad3f5efcea329ae1d'
SEL = 'CIngSnatchingSwarm::SelectTarget('
ARC = 'CIngSnatchingSwarm::FollowArc('
CTOR = 'CIngSnatchingSwarm::CIngSnatchingSwarm('

# (label, body, arm offset, restore offset, target, forced, skip, natural-precondition)
# target: '$reg' (whole register), '$xmmN.f' (lane 0 as float), or 'mem:<base reg>+<off>:<size>'.
SITES = [
    # 0x324 mov 0x3c(%r15),%eax / 0x32f cmp $0xffffffff,%eax / 0x332 je: data.mExplosionEffect
    # == kInvalidAssetId -> the empty optional (lines 144-148).
    ('ctor explosion invalid', CTOR, 0x32f, 0x332, '$rax', 0xffffffff, 0, None),
    # 0x79 testb $0x1,0x30(%rax) / 0x7d je: patterned->GetActive() false (line 475).
    ('select inactive', SEL, 0x79, 0x7d, 'mem:rax+0x30:1', 'clear1', 2, None),
    # 0x9f cmp %eax,0x8(%r15) / 0xa3 je: area ids differ (line 476).
    ('select other area', SEL, 0x9f, 0xa3, '$rax', 'plus1', 2, None),
    # 0x378 test %al,%al / 0x37a jne: IsIngPossessed() true -> count 1 (line 477).
    ('select possessed', SEL, 0x378, 0x37a, '$rax', 'al1', 2, 'al0'),
    # 0x246 test %rax,%rax / 0x249 je: GetObjectById null (line 499).
    ('select entity null', SEL, 0x246, 0x249, '$rax', 0, 2, None),
    # 0x29a comiss %xmm0,%xmm3 / 0x29d jbe: distance < bestDistance (line 506, first term).
    ('select closer', SEL, 0x29a, 0x29d, '$xmm0.f', -1.0, 0, None),
    # 0x2a8 cmp %ebx,%eax / 0x2aa jne: bestCount != count (line 506, second term).
    ('select count differs', SEL, 0x2a8, 0x2aa, '$rax', 'neq_rbx', 0, None),
    # 0x21 cmpl $0x4,0x534(%rdi) / 0x28 je: mArcPoints.size() != 4 (line 657).
    ('arc size', ARC, 0x21, 0x28, 'mem:rdi+0x534:4', 0, 20, None),
    # 0x5f comiss %xmm1,%xmm2 / 0x62 jbe: speed <= 0 (line 657).
    ('arc speed', ARC, 0x5f, 0x62, '$xmm2.f', 0.0, 40, None),
    # 0x7d comiss %xmm1,%xmm4 / 0x80 jbe: duration <= 0 (line 659).
    ('arc duration', ARC, 0x7d, 0x80, '$xmm4.f', 0.0, 60, None),
    # 0x126 comiss .LC0(1.1920929e-7),%xmm0 / 0x13b jbe: length <= epsilon (line 671).
    ('arc tiny step', ARC, 0x126, 0x13b, '$xmm0.f', 0.0, 80, None),
]

state = {'hits': {}, 'done': set(), 'pending': {}}


def log(event, **data):
    with open(os.path.join(os.environ['COV_OUT'], 'ldr3_iss.jsonl'), 'a') as f:
        f.write(json.dumps(dict(event=event, **data), sort_keys=True) + '\n')


def ev(s):
    return gdb.parse_and_eval(s)


def check(value, label):
    if not value:
        log('FAIL', label=label)
        raise gdb.GdbError(label)


def symbol(prefix):
    bodies = {}
    for line in open(os.environ['COV_NM']):
        raw, _, dem = line.rstrip('\n').partition('\t')
        parts = raw.split()
        if len(parts) == 3 and parts[1] in 'tTW' and dem.startswith(prefix) and 'clone' not in dem \
                and '.cold' not in parts[2]:
            bodies.setdefault(parts[0], []).append(parts[2])
    # The C1/C2 ctor aliases share one body address.
    check(len(bodies) == 1, 'one native body: ' + prefix + ' ' + str(bodies))
    return sorted(next(iter(bodies.values())))[0]


def read_target(target):
    inf = gdb.selected_inferior()
    if target.startswith('mem:'):
        _, where, size = target.split(':')
        reg, off = where.split('+')
        address = int(ev('$' + reg)) + int(off, 16)
        return ('mem', address, bytes(inf.read_memory(address, int(size))))
    if target.endswith('.f'):
        reg = target[:-2]
        return ('xmm', reg, int(ev(reg + '.v4_int32[0]')) & 0xffffffff)
    return ('reg', target, int(ev(target)) & 0xffffffffffffffff)


def write_target(saved, forced):
    kind, where, original = saved
    inf = gdb.selected_inferior()
    if kind == 'mem':
        if forced == 'clear1':
            data = bytes([original[0] & ~1]) + original[1:]
        else:
            data = int(forced).to_bytes(len(original), 'big')  # 0 only: endianness irrelevant
        inf.write_memory(where, data)
        return bytes(inf.read_memory(where, len(original))) == data, data.hex()
    if kind == 'xmm':
        gdb.execute('set var %s.v4_float[0] = %r' % (where, float(forced)), to_string=True)
        return float(ev(where + '.v4_float[0]')) == float(forced), repr(float(forced))
    if forced == 'plus1':
        value = (original + 1) & 0xffffffff
    elif forced == 'al1':
        value = (original & ~0xff) | 1
    elif forced == 'neq_rbx':
        value = (int(ev('$rbx')) + 1) & 0xffffffff
    else:
        value = int(forced)
    gdb.execute('set var %s = %d' % (where, value), to_string=True)
    return int(ev(where)) & 0xffffffffffffffff == value, hex(value)


def restore_target(saved):
    kind, where, original = saved
    inf = gdb.selected_inferior()
    if kind == 'mem':
        inf.write_memory(where, original)
        actual = bytes(inf.read_memory(where, len(original)))
        return actual == original, original.hex(), actual.hex()
    if kind == 'xmm':
        signed = original - (1 << 32) if original & 0x80000000 else original
        gdb.execute('set var %s.v4_int32[0] = %d' % (where, signed), to_string=True)
        actual = int(ev(where + '.v4_int32[0]')) & 0xffffffff
        return actual == original, hex(original), hex(actual)
    signed = original - (1 << 64) if original >> 63 else original
    gdb.execute('set var %s = %d' % (where, signed), to_string=True)
    actual = int(ev(where)) & 0xffffffffffffffff
    return actual == original, hex(original), hex(actual)


def owner():
    return (gdb.selected_thread().global_num, int(ev('$rsp')) & 0xffffffffffffffff)


class Arm(gdb.Breakpoint):
    def __init__(self, site, location):
        super().__init__(location, internal=True)
        self.site = site

    def stop(self):
        label, _, _, _, target, forced, skip, pre = self.site
        if label in state['done'] or label in state['pending']:
            return False
        n = state['hits'][label] = state['hits'].get(label, 0) + 1
        if n <= skip:
            return False
        if pre == 'al0' and int(ev('$rax')) & 0xff:
            return False
        saved = read_target(target)
        ok, written = write_target(saved, forced)
        state['pending'][label] = (owner(), saved)
        log('ARM', label=label, hit=n, owner=list(owner()), target=target,
            original=saved[2].hex() if isinstance(saved[2], bytes) else hex(saved[2]),
            forced=written, readback_ok=ok)
        if not ok:
            log('FAIL', label='forced readback ' + label)
        return False


class Restore(gdb.Breakpoint):
    def __init__(self, site, location):
        super().__init__(location, internal=True)
        self.site = site

    def stop(self):
        label = self.site[0]
        if label not in state['pending']:
            return False
        who, saved = state['pending'][label]
        if owner() != who:
            log('OWNER_MISMATCH', label=label, armed=list(who), here=list(owner()))
            return False
        del state['pending'][label]
        ok, original, actual = restore_target(saved)
        state['done'].add(label)
        log('RESTORE', label=label, owner=list(who), original=original, readback=actual, ok=ok)
        if not ok:
            log('FAIL', label='restore readback ' + label)
        return False


def finished(event):
    missing = [s[0] for s in SITES if s[0] not in state['done']]
    log('EXIT', done=sorted(state['done']), missing=missing, pending=sorted(state['pending']),
        hits=state['hits'])


def install():
    binary = gdb.current_progspace().filename
    check(bool(binary), 'GDB executable identity')
    with open(binary, 'rb') as f:
        digest = hashlib.file_digest(f, 'sha256').hexdigest()
    log('BINARY', path=binary, sha256=digest)
    check(digest == FROZEN, 'frozen binary')
    check(int(ev('&((CIngSnatchingSwarm*)0)->mArcPoints.mCount')) == 0x534, 'mArcPoints.mCount 0x534')
    for site in SITES:
        base = symbol(site[1])
        arm = Arm(site, '*%s+%d' % (base, site[2]))
        rest = Restore(site, '*%s+%d' % (base, site[3]))
        log('SITE', label=site[0], symbol=base, arm=site[2], restore=site[3],
            arm_loc=arm.location, restore_loc=rest.location)
    gdb.events.exited.connect(finished)


install()
