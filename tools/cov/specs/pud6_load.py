# PuddleSpore (module 51) pud6 fixture for LoadPuddleSpore (fn_51_42C,
# src/MetroidPrime/Enemies/CPuddleSpore.cpp:678-710).  This file is BOTH the force.sh spec argument and
# COV_GDB_SCRIPT.  PUD6_ARM selects the arm; each run makes exactly one bounded edit on ONE real load.
#
#   PUD6_ARM=null   686 `if (!modelData) return nullptr`.  retail: LdrToModelData returns an empty optional
#                   iff GetResourceTypeById(model) and GetResourceTypeById(animation.ancs) are both 0
#                   (ScriptLoader.cpp:446-459).  The model id is already kInvalidAssetId (edx=0xffffffff), so
#                   one unresolvable animation.ancs word empties it.  Site: the `call LdrToModelData` at
#                   LoadPuddleSpore+748 (rcx = &animationInformation = rbp-0x4d0, ancs is its first word);
#                   the original word is restored and read back at +753 (the `cmpb $0,-0x308(%rbp)` that
#                   tests the optional's valid flag).  The function must return nullptr, the valid byte
#                   must read 0 after the call; the second (natural) load must still return an entity.
#   PUD6_ARM=flag   707 `unknown_0x5cdc877d ? kCT_One : kCT_Zero`.  Site: the `cmpb $0,-0x3e8(%rbp)` at
#                   +1171 reads the loaded bool; the byte is inverted there (recorded BEFORE the write),
#                   restored and read back at the following `je` (+1178).  The function must return an entity.
#
# Window: the FIRST LoadPuddleSpore invocation only.  Every instruction site is checked against its
# expected disassembly at install time; a mismatch installs nothing (fail closed).  A FinishBreakpoint
# armed at the entry restores the word again at the return (idempotent safety net).  The summary fails
# closed on: no forced load, unrestored obligation, unreturned load, wrong return, unreadable valid flag,
# install failure.  The driver's area makes a single load, so no natural load is required.  Edge evidence itself comes from the gcda.
#
#   PUD6_ARM=null BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/pud6_load.py \
#     tools/cov/force.sh tools/cov/specs/pud6_load.py /tmp/opencode/pud6_r1 3dfd2249,cbc72d79 \
#     --con tools/cov/specs/pud2_puddlespore.con --fast --frames 2150
import hashlib
import os

import gdb

OUT = os.environ.get('COV_OUT', '/tmp')
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
FUNC = '_Z15LoadPuddleSporeR13CStateManagerR12CInputStreamR11CEntityInfo'
ARM = os.environ.get('PUD6_ARM', 'null')
BAD_ANCS = 0xDEAD0001          # no resource has this id
SPECS = {
    # arm: (write site, expected insn fragments, restore site, expected fragments)
    'null': (748, ('call', 'LdrToModelData'), 753, ('cmpb', '-0x308(%rbp)')),
    'flag': (1171, ('cmpb', '-0x3e8(%rbp)'), 1178, ('je',)),
}
state = {'installed': 0, 'binary': 'unchecked', 'loads': [], 'current': None, 'problems': []}


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def problem(msg):
    state['problems'].append(msg)
    log('pud6 FAIL: ' + msg)


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return 'err:' + str(e)[:48]
    except Exception as e:
        return 'exc:' + repr(e)[:48]


def num(v):
    return v if isinstance(v, int) else None


def fmt(v):
    return '0x%x' % v if isinstance(v, int) else str(v)


def rd(addr, size):
    t = {1: 'unsigned char', 4: 'unsigned int'}[size]
    return num(ev('*(%s*)0x%x' % (t, addr)))


def edit_target(rec):
    """(address, size, forced value) for the armed load, from live registers; None if unreadable."""
    rbp = num(ev('$rbp'))
    if rbp is None:
        return None
    if ARM == 'null':
        rcx = num(ev('$rcx'))
        if rcx is None or rcx != rbp - 0x4d0:
            problem('load %d: rcx %s is not &animationInformation (rbp-0x4d0 = %s)'
                    % (rec['id'], fmt(rcx), fmt(rbp - 0x4d0)))
            return None
        return rcx, 4, BAD_ANCS
    addr = rbp - 0x3e8
    cur = rd(addr, 1)
    if cur not in (0, 1):
        problem('load %d: flag byte at %s reads %s, not a bool' % (rec['id'], fmt(addr), fmt(cur)))
        return None
    return addr, 1, 1 - cur


def put(addr, size, value):
    t = {1: 'unsigned char', 4: 'unsigned int'}[size]
    gdb.execute('set {%s}0x%x = 0x%x' % (t, addr, value))
    return rd(addr, size)


def restore(rec, why):
    addr, size, orig = rec['addr'], rec['size'], rec['orig']
    now = rd(addr, size)
    try:
        back = put(addr, size, orig)
    except gdb.error as e:
        problem('load %d: restore (%s) raised: %s' % (rec['id'], why, str(e)[:60]))
        return False
    if back == orig:
        rec['restored'] = True
        log('load %d: restored ORIGINAL %s at %s via %s (read back %s; before it was %s)'
            % (rec['id'], fmt(orig), fmt(addr), why, fmt(back), fmt(now)))
        return True
    problem('load %d: restore (%s) readback %s, wanted %s' % (rec['id'], why, fmt(back), fmt(orig)))
    return False


class LoadReturn(gdb.FinishBreakpoint):
    def __init__(self, frame, rec):
        super().__init__(frame, internal=True)
        self.rec = rec

    def stop(self):
        rec = self.rec
        rec['returned'] = True
        rec['ret'] = num(ev('$rax'))
        if state['current'] is rec:
            state['current'] = None
        if rec['attempted'] and not rec['restored']:
            restore(rec, 'return safety net')
        log('load %d: returned %s (forced=%d)' % (rec['id'], fmt(rec['ret']), int(rec['attempted'])))
        return False

    def out_of_scope(self):
        self.rec['returned'] = False
        problem('load %d: LoadPuddleSpore left without a return' % self.rec['id'])


class EntryBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + FUNC, internal=True)

    def stop(self):
        try:
            rec = {'id': len(state['loads']) + 1, 'attempted': False, 'confirmed': False,
                   'restored': False, 'returned': False, 'ret': None, 'addr': None, 'size': 0,
                   'orig': None, 'valid_after': None, 'armed': False}
            state['loads'].append(rec)
            state['current'] = rec
            try:
                LoadReturn(gdb.newest_frame(), rec)
                rec['armed'] = True
            except Exception as e:
                problem('load %d: FinishBreakpoint not created (%r)' % (rec['id'], e))
        except Exception as e:
            problem('entry exception: %r' % (e,))
        return False


class WriteBp(gdb.Breakpoint):
    def __init__(self, spec):
        super().__init__(spec, internal=True)

    def stop(self):
        try:
            self._stop()
        except Exception as e:
            problem('write-site exception: %r' % (e,))
        return False

    def _stop(self):
        rec = state['current']
        if rec is None or rec['id'] != 1 or rec['attempted'] or not rec['armed']:
            return
        target = edit_target(rec)
        if target is None:
            return
        addr, size, value = target
        orig = rd(addr, size)
        if orig is None:
            problem('load %d: original at %s unreadable, nothing written' % (rec['id'], fmt(addr)))
            return
        rec.update(addr=addr, size=size, orig=orig, attempted=True)   # BEFORE the write
        err = None
        try:
            now = put(addr, size, value)
        except gdb.error as e:
            err, now = str(e)[:60], None
            problem('load %d: write raised: %s (rolling back)' % (rec['id'], err))
        if err is None and now != value:
            problem('load %d: write read back %s, wanted %s (rolling back)'
                    % (rec['id'], fmt(now), fmt(value)))
        if err is not None or now != value:
            restore(rec, 'immediate rollback')
            return
        rec['confirmed'] = True
        log('load %d: FORCED %s at %s: %s -> %s (arm %s)'
            % (rec['id'], 'ancs word' if ARM == 'null' else 'flag byte', fmt(addr), fmt(orig),
               fmt(value), ARM))


class RestoreBp(gdb.Breakpoint):
    def __init__(self, spec):
        super().__init__(spec, internal=True)

    def stop(self):
        try:
            rec = state['current']
            if rec is not None and rec['attempted'] and not rec['restored']:
                if ARM == 'null':
                    rbp = num(ev('$rbp'))
                    rec['valid_after'] = rd(rbp - 0x308, 1) if rbp is not None else None
                    log('load %d: optional valid byte after the call = %s'
                        % (rec['id'], fmt(rec['valid_after'])))
                restore(rec, 'restore site')
        except Exception as e:
            problem('restore-site exception: %r' % (e,))
        return False


def insn_ok(offset, frags):
    try:
        text = gdb.execute('x/i *%s+%d' % (FUNC, offset), to_string=True) or ''
    except gdb.error as e:
        return False, 'x/i failed: %s' % str(e)[:60]
    ok = all(f in text for f in frags)
    return ok, text.strip()


def summarize():
    problems = list(state['problems'])
    loads = state['loads']
    if state['installed'] != 1:
        problems.append('fixture not installed')
    if not loads:
        problems.append('no LoadPuddleSpore call at all')
    forced = [r for r in loads if r['attempted']]
    if not forced:
        problems.append('the forced %s load never happened (loads=%d)' % (ARM, len(loads)))
    unreturned = [r['id'] for r in loads if r['armed'] and not r['returned']]
    if unreturned:
        problems.append('load(s) without a return: %s' % unreturned)
    unrestored = [r['id'] for r in forced if not r['restored']]
    if unrestored:
        problems.append('attempted mutation(s) not restored: %s' % unrestored)
    for r in forced:
        if r['returned'] and r['ret'] is None:
            problems.append('load %d: return value unreadable' % r['id'])
        elif ARM == 'null' and r['returned'] and r['ret'] != 0:
            problems.append('load %d: forced load returned %s, the null arm must return nullptr'
                            % (r['id'], fmt(r['ret'])))
        elif ARM == 'null' and r['valid_after'] != 0:
            problems.append('load %d: optional valid byte after the call is %s, wanted 0'
                            % (r['id'], fmt(r['valid_after'])))
        elif ARM == 'flag' and r['returned'] and not r['ret']:
            problems.append('load %d: flag-flipped load returned %s, wanted an entity'
                            % (r['id'], fmt(r['ret'])))
    # The driver's area makes ONE load, so a natural entity is informational only; survival of the game
    # is judged from the run itself (exit code, frame count, no signal), not from this fixture.
    natural = [r for r in loads if not r['attempted'] and r['returned'] and r['ret']]
    unread = [r['id'] for r in loads if not r['attempted'] and r['returned'] and r['ret'] is None]
    if unread:
        problems.append('natural load(s) with an unreadable return: %s' % unread)
    status = 'OK' if not problems else 'FAIL'
    line = ('status=%s arm=%s binary=%s installed=%d loads=%d forced=%d restored=%d returns=%s '
            'natural_entities=%d reason=%s\n'
            % (status, ARM, state['binary'], state['installed'], len(loads), len(forced),
               sum(1 for r in forced if r['restored']),
               ','.join(fmt(r['ret']) for r in loads), len(natural),
               '; '.join(problems) if problems else '-'))
    with open(TABLE, 'a') as fh:
        fh.write(line)
    log('pud6 summary: ' + line.strip())
    return status, problems


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def binary_sha():
    try:
        ps = gdb.current_progspace()
        fn = ps.filename if ps is not None else None
    except gdb.error as e:
        return None, 'progspace error: %s' % str(e)[:60]
    if not fn:
        return None, 'no binary path'
    path = fn if os.path.isabs(fn) else os.path.join(os.getcwd(), fn)
    try:
        return sha256_file(path), path
    except OSError as e:
        return None, 'cannot read %s: %s' % (path, str(e)[:60])


gdb.events.exited.connect(lambda event: summarize())
_sha, _why = binary_sha()
state['binary'] = _sha or ('unavailable: ' + str(_why))
try:
    if ARM not in SPECS:
        raise ValueError('PUD6_ARM=%r is not null|flag' % ARM)
    w_off, w_frags, r_off, r_frags = SPECS[ARM]
    for off, frags in ((w_off, w_frags), (r_off, r_frags)):
        ok, text = insn_ok(off, frags)
        log('pud6: site +%d %s: %s' % (off, 'OK' if ok else 'MISMATCH', text))
        if not ok:
            raise ValueError('site +%d does not hold %s (%s)' % (off, frags, text))
    EntryBp()
    WriteBp('*%s+%d' % (FUNC, w_off))
    RestoreBp('*%s+%d' % (FUNC, r_off))
    state['installed'] = 1
    log('pud6: fixture installed (binary %s, arm %s)' % ((_sha or '?')[:12], ARM))
except Exception as e:
    problem('install failed: %r' % (e,))
