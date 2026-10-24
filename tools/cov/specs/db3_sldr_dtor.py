# DestructibleBarrier (module 13) db3 fixture: SLdrDestructibleBarrier::~SLdrDestructibleBarrier
# (fn_13_BA0, include/MetroidPrime/ScriptLoader/SLdrDestructibleBarrier.hpp:15 declaration, :97 the
# inline empty definition).  This file is BOTH the force.sh spec argument and COV_GDB_SCRIPT; it writes
# nothing at all (read-only probe).
#
# Retail fn_13_BA0 (`__dt__23SLdrDestructibleBarrierFv`, weak, 0xBA0..0xC1C): `mr. r30,r3` / `beq`
# null-this guard, then the four member destructors (0xBC8 SLdrActorParameters +0x214, 0xBD4
# SLdrDamageVulnerability +0xB8, 0xBE0 SLdrHealthInfo +0xB0, 0xBEC SLdrEditorProperties +0), then
# `extsh. r0,r31` / `ble` and `bl __dl__FPv` - the in-place weak dtor carries the MWCC deleting-flag
# variant that a `delete` through a base pointer would take.
#
# Host build (checked statically before the run): `_ZN23SLdrDestructibleBarrierD1Ev` == `D2Ev` at
# 0xde1b20, an out-of-line copy of the four member-dtor calls with no null guard and no deleting flag,
# and there is no `D0Ev` symbol at all.  Its only call site in the binary is 0xde26fc, in
# `LoadDestructibleBarrier`'s epilogue (.cold), with `this` = `lea -0x670(%rbp)` - the loader's own
# stack object.  So on this build the live path is the member chain, and the retail null/delete guards
# have no host counterpart; the loader's stack lifetime is the only destruction path.
#
# MEASURED on the first boot: the normal loader path never calls the out-of-line 0xde1b20 family.  The
# destructor is INLINED into LoadDestructibleBarrier's epilogue, so the four member-destructor calls run
# with gdb showing an inlined `SLdrDestructibleBarrier::~SLdrDestructibleBarrier()` frame whose caller is
# LoadDestructibleBarrier, and the out-of-line D1/D2 copy (the one an EH cleanup or a deleting use would
# call) stays untouched.  The fixture therefore attributes invocations from the member calls: each
# member's `this` is the root object plus a fixed host offset (0x0 EditorProperties, 0xbc HealthInfo,
# 0xc4 DamageVulnerability, 0x220 ActorParameters, from 0xde1b20's own `lea`s), so the root groups the
# four calls of one invocation, and the root must lie inside LoadDestructibleBarrier's [rsp, rbp].
# The out-of-line dtor breakpoint is kept as an informational counter (never required).
# The summary fails closed on: no invocation, a root that is null or outside the loader frame, a
# caller chain that is not (inlined dtor -> LoadDestructibleBarrier), an invocation missing a member
# destructor, and install failure.
#
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/db3_sldr_dtor.py tools/cov/force.sh \
#     tools/cov/specs/db3_sldr_dtor.py /tmp/opencode/db3_r1 42b935e4,08_sand_casualpirates \
#     --con tools/cov/specs/db3_dbar_dtor.con --fast --frames 600
import hashlib
import os

import gdb

OUT = os.environ.get('COV_OUT', '/tmp')
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
DTOR = '_ZN23SLdrDestructibleBarrierD1Ev'
LOADER = 'LoadDestructibleBarrier'
MEMBERS = [
    ('SLdrActorParameters', '_ZN19SLdrActorParametersD1Ev', 0x220),
    ('SLdrDamageVulnerability', '_ZN23SLdrDamageVulnerabilityD1Ev', 0xc4),
    ('SLdrHealthInfo', '_ZN14SLdrHealthInfoD1Ev', 0xbc),
    ('SLdrEditorProperties', '_ZN20SLdrEditorPropertiesD1Ev', 0x0),
]
state = {'installed': 0, 'binary': 'unchecked', 'calls': [], 'members_seen': [],
         'outofline_hits': 0, 'problems': []}


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
        return frame.name() or ''
    except Exception:
        return ''


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
    def __init__(self, label, sym, offset):
        super().__init__('*' + sym, internal=True)
        self.label, self.offset = label, offset

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
        if 'SLdrDestructibleBarrier::~SLdrDestructibleBarrier' not in inline_name:
            # a member dtor somewhere else: not this function's invocation
            return
        if not isinstance(member_this, int):
            problem('%s: member this unreadable (%s)' % (self.label, member_this))
            return
        root = member_this - self.offset
        call = next((c for c in state['calls'] if c['root'] == root), None)
        if call is None:
            call = {'id': len(state['calls']) + 1, 'root': root, 'members': {},
                    'in_frame': None, 'loader': loader_name, 'inline': inline_name}
            state['calls'].append(call)
        call['members'][self.label] = call['members'].get(self.label, 0) + 1
        if call['in_frame'] is None and loader is not None:
            try:
                rsp = int(loader.read_register('rsp'))
                rbp = int(loader.read_register('rbp'))
                call['in_frame'] = rsp <= root <= rbp
                call['frames'] = (rsp, rbp)
            except Exception as e:
                problem('%s: loader frame registers unreadable (%r)' % (self.label, e))
        if LOADER not in (loader_name or ''):
            problem('root 0x%x: the frame above the inlined dtor is %r, not %s'
                    % (root, loader_name, LOADER))
        log('member dtor %s: member this=%s -> root 0x%x, inlined=%s, caller=%s, '
            'in_loader_frame=%s' % (self.label, fmt(member_this), root, inline_name,
                                    loader_name or '(unknown)', call['in_frame']))


def summarize():
    problems = list(state['problems'])
    calls = state['calls']
    if state['installed'] != 1:
        problems.append('fixture not installed')
    if not calls:
        problems.append('the destructor never ran (no loader stack lifetime observed)')
    for c in calls:
        missing = [m for m, _s, _o in MEMBERS if m not in c['members']]
        if missing:
            problems.append('invocation %d (root 0x%x): member destructor(s) not seen: %s'
                            % (c['id'], c['root'], missing))
        if c['in_frame'] is False:
            problems.append('invocation %d: root 0x%x is not inside the loader frame [%s,%s]'
                            % (c['id'], c['root'], fmt(c.get('frames', ('?', '?'))[0]),
                               fmt(c.get('frames', ('?', '?'))[1])))
    status = 'OK' if not problems else 'FAIL'
    line = ('status=%s binary=%s installed=%d invocations=%d roots=%s in_loader_frame=%s '
            'members=%s outofline_hits=%d reason=%s\n'
            % (status, state['binary'], state['installed'], len(calls),
               [fmt(c['root']) for c in calls], [c['in_frame'] for c in calls],
               [sorted(c['members']) for c in calls], state['outofline_hits'],
               '; '.join(problems) if problems else '-'))
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
    for label, sym, off in MEMBERS:
        MemberBp(label, sym, off)
    state['installed'] = 1
    log('db3: fixture installed (binary %s, inlined dtor attribution, %d member breakpoints, '
        'out-of-line %s informational)'
        % ((_sha or '?')[:12], len(MEMBERS), DTOR[:40]))
except Exception as e:
    problem('breakpoint install failed: %r' % e)
