# DestructibleBarrier (module 13) db3 fixture: SLdrDestructibleBarrier::~SLdrDestructibleBarrier
# (fn_13_BA0, include/MetroidPrime/ScriptLoader/SLdrDestructibleBarrier.hpp:15 declaration, :97 the
# inline empty definition).  This file is BOTH the force.sh spec argument and COV_GDB_SCRIPT; it writes
# nothing at all (read-only probe).
#
# Retail fn_13_BA0 (`__dt__23SLdrDestructibleBarrierFv`, weak, 0xBA0..0xC1C): 0xBB8 `mr. r30,r3` /
# 0xBBC `beq` null-this guard, then the four member destructors in this order - 0xBC8
# `bl __dt__19SLdrActorParametersFv` (+0x214), 0xBD4 `bl __dt__23SLdrDamageVulnerabilityFv` (+0xB8),
# 0xBE0 `bl __dt__14SLdrHealthInfoFv` (+0xB0), 0xBEC `bl __dt__20SLdrEditorPropertiesFv` (+0) - and then
# 0xBF0 `extsh. r0,r31` / 0xBF4 `ble` against the MWCC delete flag, 0xBF8 `mr r3,r30` and 0xBFC
# **`bl Free__7CMemoryFPCv`**.  The flag-positive arm therefore frees through CMemory::Free, not through
# a C++ `operator delete`/Itanium `D0` entry: MWCC's weak dtor carries its own delete flag and the
# game-heap free call, so "deleting variant" here means that CMemory free, and a host-side equivalent
# would be the port's own `PortDelete` routing (PortMwccNew.cpp frees game-heap pointers via
# CMemory::Free and everything else via std::free).
#
# Host build (static): `_ZN23SLdrDestructibleBarrierD1Ev` == `D2Ev` at the recorded 0xde1b20 (four member
# calls in the retail order, no null guard, no flag), no `D0Ev` symbol, and the only call site in that
# binary was 0xde26fc in LoadDestructibleBarrier's `.cold` epilogue with `this = lea -0x670(%rbp)` - the
# loader's own stack object.  Measured on the accepted boot (r2): the normal path INLINES the empty body,
# so the four member dtors run with gdb showing an inlined `SLdrDestructibleBarrier::~SLdrDestructibleBarrier`
# frame whose caller is LoadDestructibleBarrier; the out-of-line copy never ran.
#
# EVIDENCE ACCOUNTING (strict, offline revision - NOT run on any boot; the boot ran the earlier version,
# archived as db3_sldr_dtor.executed.py): the success criterion is a **strict consecutive ordered chain**
# of the four member destructors, not the union of members seen per root.  Within one chain the root
# (member `this` minus the member offset), the inferior, the thread and the frame context (loader/inlined
# pc pair) must be identical, the members must arrive in the retail order with no duplicate and no gap,
# the loader frame bounds are re-read and logged on EVERY member, and the chain must complete.  A changed
# context, an unreadable frame/register/context, a duplicate, an out-of-order member or a chain still
# pending at exit is a failure.  The per-root member aggregate is kept only as a diagnostic named
# `root_groups`; it is never a success criterion.
#
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/db3_sldr_dtor.py tools/cov/force.sh \
#     tools/cov/specs/db3_sldr_dtor.py /tmp/opencode/db3_rN 42b935e4,08_sand_casualpirates \
#     --con tools/cov/specs/db3_dbar_dtor.con --fast --frames 600
import hashlib
import os

import gdb

OUT = os.environ.get('COV_OUT', '/tmp')
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
DTOR = '_ZN23SLdrDestructibleBarrierD1Ev'
LOADER = 'LoadDestructibleBarrier'
DTOR_NAME = 'SLdrDestructibleBarrier::~SLdrDestructibleBarrier'
# retail/host destruction order: ActorParameters, DamageVulnerability, HealthInfo, EditorProperties
MEMBERS = [
    ('SLdrActorParameters', '_ZN19SLdrActorParametersD1Ev', 0x220),
    ('SLdrDamageVulnerability', '_ZN23SLdrDamageVulnerabilityD1Ev', 0xc4),
    ('SLdrHealthInfo', '_ZN14SLdrHealthInfoD1Ev', 0xbc),
    ('SLdrEditorProperties', '_ZN20SLdrEditorPropertiesD1Ev', 0x0),
]
state = {'installed': 0, 'binary': 'unchecked', 'completed': [], 'pending': None,
         'root_groups': {}, 'outofline_hits': 0, 'problems': []}


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def problem(msg):
    state['problems'].append(msg)
    log('db3 FAIL: ' + msg)


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return 'err:' + str(e)[:48]
    except Exception as e:
        return 'exc:' + repr(e)[:48]


def fmt(v):
    return '0x%x' % v if isinstance(v, int) else str(v)


def frame_name(frame):
    try:
        return (frame.name() or '') if frame is not None else ''
    except Exception:
        return ''


def frame_pc(frame):
    try:
        return int(frame.pc()) if frame is not None else None
    except Exception:
        return None


def frame_reg(frame, reg):
    try:
        return int(frame.read_register(reg)) if frame is not None else None
    except Exception:
        return None


def context_ids():
    """(inferior, thread) of the current selection; None parts make the chain fail closed."""
    inferior = thread = None
    try:
        inferior = int(gdb.selected_inferior().num)
    except Exception:
        pass
    try:
        thread = int(gdb.selected_thread().global_num)
    except Exception:
        pass
    return inferior, thread


class OutOfLineBp(gdb.Breakpoint):
    """The out-of-line D1/D2 copy: informational only (the normal path inlines the empty body)."""

    def __init__(self):
        super().__init__('*' + DTOR, internal=True)

    def stop(self):
        state['outofline_hits'] += 1
        log('out-of-line %s hit %d (this=%s, caller=%s)'
            % (DTOR, state['outofline_hits'], fmt(ev('$rdi')),
               frame_name(gdb.newest_frame().older()) or '(unknown)'))
        return False


class MemberBp(gdb.Breakpoint):
    def __init__(self, label, sym, offset, index):
        super().__init__('*' + sym, internal=True)
        self.label, self.offset, self.index = label, offset, index

    def stop(self):
        try:
            self._stop()
        except Exception as e:
            problem('member breakpoint exception (%s): %r' % (self.label, e))
        return False

    def _stop(self):
        member_this = ev('$rdi')
        frame = gdb.newest_frame()
        inlined = frame.older() if frame is not None else None
        inline_name = frame_name(inlined)
        loader = None
        try:
            loader = inlined.older() if inlined is not None else None
        except Exception:
            loader = None
        loader_name = frame_name(loader)
        if DTOR_NAME not in inline_name:
            return  # a member dtor somewhere else: not this function's invocation
        inferior, thread = context_ids()
        loader_pc, inline_pc = frame_pc(loader), frame_pc(inlined)
        rsp, rbp = frame_reg(loader, 'rsp'), frame_reg(loader, 'rbp')
        if not isinstance(member_this, int):
            problem('%s: member this unreadable (%s)' % (self.label, member_this))
            return
        if not (isinstance(inferior, int) and isinstance(thread, int)
                and isinstance(loader_pc, int) and isinstance(inline_pc, int)
                and isinstance(rsp, int) and isinstance(rbp, int)):
            problem('%s: context/frame unreadable (inferior=%s thread=%s loader_pc=%s inline_pc=%s '
                    'rsp=%s rbp=%s)' % (self.label, inferior, thread, fmt(loader_pc), fmt(inline_pc),
                                        fmt(rsp), fmt(rbp)))
            return
        if LOADER not in loader_name:
            problem('%s: the frame above the inlined dtor is %r, not %s'
                    % (self.label, loader_name, LOADER))
            return
        root = member_this - self.offset
        key = (root, inferior, thread, loader_pc, inline_pc)
        in_bounds = rsp <= root <= rbp
        state['root_groups'][root] = state['root_groups'].get(root, 0) + 1
        if not in_bounds:
            problem('%s: root 0x%x is not inside the loader frame [%s,%s]'
                    % (self.label, root, fmt(rsp), fmt(rbp)))
        log('member %d %s: member this=0x%x root=0x%x inferior=%d thread=%d loader_pc=%s '
            'inline_pc=%s rsp=%s rbp=%s in_bounds=%s'
            % (self.index, self.label, member_this, root, inferior, thread, fmt(loader_pc),
               fmt(inline_pc), fmt(rsp), fmt(rbp), in_bounds))
        pending = state['pending']
        if pending is None:
            if self.index != 0:
                problem('chain start out of order: first member was %s (index %d)'
                        % (self.label, self.index))
                return
            state['pending'] = {'key': key, 'root': root, 'members': [self.label], 'next': 1,
                                'bounds': [in_bounds]}
            log('chain %d started at root 0x%x with %s'
                % (len(state['completed']) + 1, root, self.label))
            return
        if key != pending['key']:
            problem('chain %d changed context before completing: root 0x%x/members %s vs new %s '
                    '(root 0x%x)' % (len(state['completed']) + 1, pending['root'],
                                     pending['members'], self.label, root))
            state['pending'] = None
            if self.index == 0:
                state['pending'] = {'key': key, 'root': root, 'members': [self.label], 'next': 1,
                                    'bounds': [in_bounds]}
            return
        if self.index != pending['next']:
            problem('chain %d: %s arrived at position %d, expected index %d (duplicate or '
                    'out-of-order; members so far %s)'
                    % (len(state['completed']) + 1, self.label, self.index, pending['next'],
                       pending['members']))
            return
        pending['members'].append(self.label)
        pending['bounds'].append(in_bounds)
        pending['next'] += 1
        if pending['next'] == len(MEMBERS):
            state['completed'].append({'root': pending['root'], 'members': list(pending['members']),
                                       'bounds': list(pending['bounds'])})
            log('chain %d COMPLETE at root 0x%x: %s'
                % (len(state['completed']), pending['root'], pending['members']))
            state['pending'] = None


def summarize():
    problems = list(state['problems'])
    if state['installed'] != 1:
        problems.append('fixture not installed')
    if not state['completed']:
        problems.append('no completed four-member destruction chain was observed')
    if state['pending'] is not None:
        problems.append('a destruction chain was still pending at exit (members %s)'
                        % state['pending']['members'])
    for c in state['completed']:
        if c['members'] != [m for m, _s, _o in MEMBERS]:
            problems.append('chain at root 0x%x is not the ordered four: %s'
                            % (c['root'], c['members']))
        if not all(c['bounds']):
            problems.append('chain at root 0x%x has a member outside the loader frame: %s'
                            % (c['root'], c['bounds']))
    status = 'OK' if not problems else 'FAIL'
    line = ('status=%s binary=%s installed=%d completed_chains=%d chains=%s pending=%s '
            'root_groups=%s outofline_hits=%d reason=%s\n'
            % (status, state['binary'], state['installed'], len(state['completed']),
               [['0x%x' % c['root']] + c['members'] for c in state['completed']],
               state['pending']['members'] if state['pending'] else None,
               {'0x%x' % r: n for r, n in state['root_groups'].items()},
               state['outofline_hits'], '; '.join(problems) if problems else '-'))
    with open(TABLE, 'a') as fh:
        fh.write(line)
    log('db3 summary: ' + line.strip())
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
    OutOfLineBp()
    for i, (label, sym, off) in enumerate(MEMBERS):
        MemberBp(label, sym, off, i)
    state['installed'] = 1
    log('db3: fixture installed (binary %s, strict ordered chain over %d members, out-of-line %s '
        'informational)' % ((_sha or '?')[:12], len(MEMBERS), DTOR[:40]))
except Exception as e:
    problem('breakpoint install failed: %r' % e)
