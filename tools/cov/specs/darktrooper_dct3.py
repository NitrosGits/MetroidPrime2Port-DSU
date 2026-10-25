"""dct3: arm coverage of CDarkTrooper's ctor (fn_12_5A48) from one frozen binary.

The body has five decision sites (retail offsets from the module's CDarkTrooper.s, host code
addresses from the frozen build-port-link binary as ctor symbol + offset):

  E1  first sleeper compare    retail 0x5EC4/0x5EC8            host cmp at ctor+0x53E (call +0x535)
  E2  second sleeper compare   retail 0x5F7C/0x5F80            host cmp at ctor+0x7CA (call +0x7C1)
  E3  scan id != kInvalid      retail 0x5FA0/0x5FA4            host cmpl at ctor+0x55C
  E4  flotsam                  retail 0x605C/0x6060            host cmpb at ctor+0x56A
  E5  __nw null (OOM)          retail 0x5FC8                    written argument only (throwing new)
  plus the C++ temporary-ownership guards (retail 0x6024/0x602C/0x6044 = the host's exception
  landing pads; no run reaches them by design).

The frozen binary's ctor frame keeps the three inputs at fixed slots (verified in the static dry
run against 0xd4c12e / 0xd4c14c / 0xd4c15a):
    initialAnim                0x1A0(%rsp)   stack argument, compared at ctor+0x53E/+0x7CA
    scannableInfoWhenAttacking 0x1E0(%rsp)   stack argument, compared at ctor+0x55C
    flotsam                    0x28(%rsp)    spilled argument, compared at ctor+0x56A
and the members it writes: mIsSleeper = byte 0xBDC bit 0x01, mLocomotionType = int 0xBD8,
mAttackScanInfo = pointer 0x9F8 (rstl::single_ptr), highlighted-in-dark-visor = byte 0x178 bit 0x02.

Arms are taken naturally wherever the shipped placements reach them (tools/scan_darktrooper_loader.py:
11/35 flotsam, 33/35 with a SCAN id, initial_anim 1/9/11/12/13). Only an arm the data never reaches
is forced, and then for ONE object chosen by uid:

    DCT3_FORCE='<uid>:E1'   or   DCT3_FORCE='<uid>:E2'

The write happens at the real FindBestAnimation's return, on that object's initialAnim word
(single-object filter by uid; the caller frame must be the ctor and its $rbx the object; the original
word is recorded, the write is read back, and it is restored at the ctor's return).

Every breakpoint is a line or a real symbol, so nothing depends on an absolute address (PIE). The
pure part - symbol selection, edge classification, force filter/protocol, checks - is exercised
without gdb by tools/cov/specs/darktrooper_dct3_selftest.py.
"""
import os

# gdb_cov.py / force.py read these two; this spec builds its own breakpoints.
CLASS = 'CDarkTrooper'
SPEC = []
FORCE = []

CTOR_PREFIX = 'CDarkTrooper::CDarkTrooper'
BEST = 'CPASDatabase::FindBestAnimation(CPASAnimParmData const&, int) const'
OFF_ANIM, OFF_SCAN, OFF_FLOTSAM = 0x1A0, 0x1E0, 0x28
OFF_MIS_SLEEPER, OFF_LOCO, OFF_SCANINFO, OFF_VISOR_BYTE = 0xBDC, 0xBD8, 0x9F8, 0x178
BIT_MIS_SLEEPER, BIT_HIGHLIGHT = 0x01, 0x02
INVALID_ASSET = 0xFFFFFFFF
ARM_LINES = {'E1true': 150, 'E2true': 156, 'E3true': 162, 'E4true': 167}
EDGES = ['E1true', 'E1false', 'E2true', 'E2false', 'E3true', 'E3false', 'E4true', 'E4false']
# filled by _install(): the breakpoint classes and state, for the mocked selftest
API = {}


# ----------------------------------------------------------------- pure part (selftested)
def ctor_symbols(nm_lines):
    """One ctor symbol per address from an nm dump: C1 and C2 are aliases in this binary, and the
    .cold clone is a different body. Two breakpoints on one address would run the entry twice."""
    out, seen = [], set()
    for line in nm_lines:
        raw, _, demangled = line.rstrip('\n').partition('\t')
        parts = raw.split()
        if len(parts) != 3 or '.cold' in demangled:
            continue
        if not demangled.startswith(CTOR_PREFIX) or parts[0] in seen:
            continue
        seen.add(parts[0])
        out.append(parts[2])
    return out


def best_symbols(nm_lines):
    out, seen = [], set()
    for line in nm_lines:
        raw, _, demangled = line.rstrip('\n').partition('\t')
        parts = raw.split()
        if len(parts) == 3 and demangled.startswith(BEST) and parts[0] not in seen:
            seen.add(parts[0])
            out.append(parts[2])
    return out


def parse_force(text):
    """'<uid>:E1' -> (uid, 'E1'); anything else -> None (no force)."""
    text = (text or '').strip()
    if ':' not in text:
        return None
    uid, _, edge = text.partition(':')
    edge = edge.strip().upper()
    if edge not in ('E1', 'E2'):
        return None
    try:
        return (int(uid, 0), edge)
    except ValueError:
        return None


def best_of(rec, index):
    """The index FindBestAnimation returned on call `index` (1-based), or None."""
    best = rec.get('best') or []
    return best[index - 1] if len(best) >= index else None


def arm_edges(rec):
    """The eight edge outcomes for one constructed object.

    "the second compare ran" is exact: FindBestAnimation is called a second time only on the
    first compare's false path, so two recorded calls prove E1false.
    """
    arms = rec.get('arms') or ()
    reached2 = len(rec.get('best') or []) >= 2 or 'E2true' in arms
    scan, flotsam = rec.get('scan'), rec.get('flotsam')
    return {
        'E1true': 'E1true' in arms,
        'E1false': reached2,
        'E2true': 'E2true' in arms,
        'E2false': reached2 and 'E2true' not in arms,
        'E3true': 'E3true' in arms,
        'E3false': scan is not None and scan == INVALID_ASSET and 'E3true' not in arms,
        'E4true': 'E4true' in arms,
        'E4false': flotsam is not None and flotsam == 0 and 'E4true' not in arms,
    }


def effective_anim(rec, call_index):
    """The initialAnim word the compare at call_index actually saw (a forced write replaces it)."""
    for f in rec.get('forced') or ():
        if f['edge'] == ('E1' if call_index == 1 else 'E2'):
            return f['write']
    return rec.get('anim')


def expected_sleeper(rec):
    """(mIsSleeper, locomotion type or None) the two compares must produce."""
    anim, anim2 = effective_anim(rec, 1), effective_anim(rec, 2)
    best7, best9 = best_of(rec, 1), best_of(rec, 2)
    if best7 is not None and anim is not None and best7 == anim:
        return (True, 7)
    if best9 is not None and anim2 is not None and best9 == anim2:
        return (True, 9)
    return (False, None)


def force_step(spec, rec, call_index, index):
    """Should this FindBestAnimation return force initialAnim, and to what index?"""
    if spec is None or rec is None or index is None:
        return None
    uid, edge = spec
    if rec.get('uid') != uid:
        return None
    if (edge == 'E1' and call_index != 1) or (edge == 'E2' and call_index != 2):
        return None
    return index


def checks_for(rec):
    """[(name, ok, detail)] for one object, from its record."""
    out = []
    final = rec.get('final') or {}
    want_alive, want_loco = expected_sleeper(rec)
    got_alive, got_loco = bool(final.get('sleeper')), final.get('loco')
    ok = got_alive == want_alive
    if want_alive:
        ok = ok and got_loco == want_loco
    elif got_loco in (7, 9):
        ok = False
    out.append(('sleeper uid=%s anim=%s best7=%s best9=%s' % (
        rec.get('uid'), rec.get('anim'), best_of(rec, 1), best_of(rec, 2)), ok,
        'want=%s/%s got=%s/%s' % (want_alive, want_loco, got_alive, got_loco)))
    scan, arms = rec.get('scan'), (rec.get('arms') or ())
    if scan is not None:
        want = scan != INVALID_ASSET
        ok = bool(final.get('scaninfo')) == want and ('E3true' in arms) == want
        out.append(('scan uid=%s id=%#x' % (rec.get('uid'), scan), ok,
                    'mAttackScanInfo=%s E3true=%s' % (
                        bool(final.get('scaninfo')), 'E3true' in arms)))
    flotsam = rec.get('flotsam')
    if flotsam is not None:
        ok = ('E4true' in arms) == bool(flotsam)
        detail = 'E4true=%s visorByte=%s' % ('E4true' in arms, final.get('visor_byte'))
        if flotsam:
            ok = ok and final.get('highlight') == 0
        out.append(('flotsam uid=%s arg=%d' % (rec.get('uid'), flotsam), ok, detail))
    for f in rec.get('forced') or ():
        out.append(('force uid=%s %s' % (rec.get('uid'), f['edge']),
                    f['restored'] and f['readback'] == f['write'],
                    'orig=%d wrote=%d read=%d restored=%s' % (
                        f['orig'], f['write'], f['readback'], f['restored'])))
    return out


# ----------------------------------------------------------------- gdb side
def _install():
    import atexit

    import gdb

    OUT = os.environ['COV_OUT']
    STATE = {'objs': {}, 'order': [], 'checks': {},
             'force_spec': parse_force(os.environ.get('DCT3_FORCE', ''))}

    def log(tag, text):
        with open(os.path.join(OUT, 'dct3.log'), 'a') as fh:
            fh.write('%s %s\n' % (tag, text))

    def check(name, ok, detail=''):
        STATE['checks'][name] = STATE['checks'].get(name, True) and bool(ok)
        with open(os.path.join(OUT, 'dct3.log'), 'a') as fh:
            fh.write('%s %s %s\n' % ('PASS' if ok else 'FAIL', name, detail))

    def rd(addr, size, signed=False):
        raw = gdb.selected_inferior().read_memory(addr, size).tobytes()
        return int.from_bytes(raw, 'little', signed=signed)

    def wr(addr, value, size=4):
        gdb.selected_inferior().write_memory(addr, int(value).to_bytes(size, 'little'))

    def reg(frame, name):
        return int(frame.read_register(name)) & 0xFFFFFFFFFFFFFFFF

    def body_rec():
        """The object whose body we are in: the ctor keeps `this` in rbx."""
        try:
            this = int(gdb.parse_and_eval('$rbx')) & 0xFFFFFFFFFFFFFFFF
        except gdb.error:
            return None
        return STATE['objs'].get(this)

    class CtorFinish(gdb.FinishBreakpoint):
        def __init__(self, rec):
            super().__init__(gdb.newest_frame(), internal=True)
            self.rec = rec

        def stop(self):
            rec, base = self.rec, self.rec['this']
            rec['final'] = {
                'sleeper': bool(rd(base + OFF_MIS_SLEEPER, 1) & BIT_MIS_SLEEPER),
                'loco': rd(base + OFF_LOCO, 4, signed=True),
                'scaninfo': rd(base + OFF_SCANINFO, 8),
                'visor_byte': rd(base + OFF_VISOR_BYTE, 1),
            }
            rec['final']['highlight'] = rec['final']['visor_byte'] & BIT_HIGHLIGHT
            for f in rec['forced']:
                wr(f['addr'], f['orig'])
                f['restored'] = rd(f['addr'], 4, signed=True) == f['orig']
            log('FINISH', 'uid=%s arms=%s final=%s' % (
                rec['uid'], ','.join(sorted(rec['arms'])) or '-', rec['final']))
            return False

    class Entry(gdb.Breakpoint):
        def stop(self):
            this = int(gdb.parse_and_eval('$rdi')) & 0xFFFFFFFFFFFFFFFF
            try:
                uid = int(gdb.parse_and_eval('(unsigned int)uid.value')) & 0xFFFF
            except gdb.error:
                uid = int(gdb.parse_and_eval('$esi')) & 0xFFFF
            rec = {'uid': uid, 'this': this, 'rsp': None, 'best': [], 'arms': set(),
                   'anim': None, 'scan': None, 'flotsam': None, 'forced': [], 'final': None}
            STATE['objs'][this] = rec
            STATE['order'].append(this)
            CtorFinish(rec)
            log('ENTRY', 'uid=%s this=%#x' % (uid, this))
            return False

    class FirstCompare(gdb.Breakpoint):
        """Line 149: inside the ctor frame and before the first FindBestAnimation call, so this
        is where the three inputs and the frame's rsp are taken."""

        def stop(self):
            rec = body_rec()
            if rec is None:
                return False
            rec['rsp'] = reg(gdb.newest_frame(), 'rsp')
            rec['anim'] = rd(rec['rsp'] + OFF_ANIM, 4, signed=True)
            rec['scan'] = rd(rec['rsp'] + OFF_SCAN, 4)
            rec['flotsam'] = rd(rec['rsp'] + OFF_FLOTSAM, 1)
            log('INPUTS', 'uid=%s anim=%d scan=%#x flotsam=%d' % (
                rec['uid'], rec['anim'], rec['scan'], rec['flotsam']))
            return False

    class BestEntry(gdb.Breakpoint):
        def stop(self):
            caller = gdb.newest_frame().older()
            if caller is None or CTOR_PREFIX not in (caller.name() or ''):
                return False
            rec = STATE['objs'].get(reg(caller, 'rbx'))
            if rec is None:
                return False
            BestReturn(rec)
            return False

    class BestReturn(gdb.FinishBreakpoint):
        def __init__(self, rec):
            super().__init__(gdb.newest_frame(), internal=True)
            self.rec = rec

        def stop(self):
            rec = self.rec
            raw = int(gdb.parse_and_eval('$rax')) & 0xFFFFFFFFFFFFFFFF
            found, index = raw & 0xFFFFFFFF, (raw >> 32) & 0xFFFFFFFF
            if index >= 0x80000000:
                index -= 0x100000000
            rec['best'].append(index)
            call_index = len(rec['best'])
            log('BEST', 'uid=%s call=%d found=%d index=%d' % (
                rec['uid'], call_index, found, index))
            target = force_step(STATE['force_spec'], rec, call_index, index)
            if target is None or rec['rsp'] is None:
                return False
            addr = rec['rsp'] + OFF_ANIM
            orig = rd(addr, 4, signed=True)
            if orig != rec['anim']:
                check('force slot uid=%s' % rec['uid'], False,
                      'slot holds %d, recorded initialAnim %s' % (orig, rec['anim']))
                return False
            check('force slot uid=%s' % rec['uid'], True, 'slot holds initialAnim %d' % orig)
            wr(addr, target)
            readback = rd(addr, 4, signed=True)
            rec['forced'].append({'edge': STATE['force_spec'][1], 'addr': addr, 'orig': orig,
                                  'write': target, 'readback': readback, 'restored': False})
            check('force write uid=%s' % rec['uid'], readback == target,
                  'orig=%d wrote=%d read=%d' % (orig, target, readback))
            log('FORCE', 'uid=%s %s orig=%d wrote=%d read=%d' % (
                rec['uid'], STATE['force_spec'][1], orig, target, readback))
            return False

    class Arm(gdb.Breakpoint):
        def __init__(self, line, edge):
            super().__init__('CDarkTrooper.cpp:%d' % line, internal=True)
            self.edge = edge

        def stop(self):
            rec = body_rec()
            if rec is not None:
                rec['arms'].add(self.edge)
            return False

    nm = open(os.environ['COV_NM']).readlines()
    ctors, bests = ctor_symbols(nm), best_symbols(nm)
    log('SETUP', 'ctors=%d bests=%d force=%s' % (
        len(ctors), len(bests), STATE['force_spec']))
    check('symbols resolved', bool(ctors and bests),
          'ctors=%s bests=%s' % (ctors, bests))
    for sym in ctors:
        Entry('*' + sym, internal=True)
    for sym in bests:
        BestEntry('*' + sym)
    FirstCompare('CDarkTrooper.cpp:149', internal=True)
    for edge, line in ARM_LINES.items():
        Arm(line, edge)

    def dump():
        counts = {e: 0 for e in EDGES}
        rows = []
        for this in STATE['order']:
            rec = STATE['objs'][this]
            edges = arm_edges(rec)
            for e in EDGES:
                counts[e] += 1 if edges[e] else 0
            rows.append('uid=%-6s anim=%-4s best=%-10s scan=%-12s flotsam=%-4s arms=%-24s %s' % (
                rec['uid'], rec['anim'], ','.join(str(b) for b in rec['best']),
                ('%#x' % rec['scan']) if rec['scan'] is not None else '-',
                rec['flotsam'], ','.join(sorted(rec['arms'])) or '-',
                ' '.join('%s=%d' % (e, edges[e]) for e in EDGES)))
            for name, ok, detail in checks_for(rec):
                check(name, ok, detail)
        with open(os.path.join(OUT, 'dct3_table.txt'), 'w') as fh:
            fh.write('%d constructed DarkTroopers\n' % len(STATE['order']))
            fh.write('\n'.join(rows) + '\n')
            fh.write('edge counts: %s\n' % counts)
        with open(os.path.join(OUT, 'dct3_checks.txt'), 'w') as fh:
            failed = [n for n in sorted(STATE['checks']) if not STATE['checks'][n]]
            for name in sorted(STATE['checks']):
                fh.write('%s %s\n' % ('PASS' if STATE['checks'][name] else 'FAIL', name))
            fh.write('FAILED %d of %d checks\n' % (len(failed), len(STATE['checks'])))
            fh.write('edges with no object: %s\n' % (','.join(
                e for e in EDGES if not counts[e]) or '-'))
            fh.write('objects: %d\n' % len(STATE['order']))

    atexit.register(dump)
    API.update({'STATE': STATE, 'Entry': Entry, 'FirstCompare': FirstCompare,
                'BestEntry': BestEntry, 'BestReturn': BestReturn, 'Arm': Arm,
                'CtorFinish': CtorFinish, 'dump': dump})


try:
    import gdb  # noqa: F401

    _install()
except ImportError:
    pass
