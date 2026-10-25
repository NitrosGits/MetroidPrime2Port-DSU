# PillBug (module 48) pbug2 fixture: the RaySphereIntersection true arm of CPillBug::CloseToPath
# (fn_48_274C, built source CPillBug.cpp:480 call, :481 `return true`; retail 0x28F4).  BOTH the force.sh
# spec and COV_GDB_SCRIPT, on the Sanctuary cliff driver (the same one pbug_force.py uses):
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/pbug_force4.py \
#     tools/cov/force.sh tools/cov/specs/pbug_force4.py <out> 1baa96c2,06_cliff_spiderballguardian \
#     --con tools/cov/specs/pillbug_cliff.con --fast --frames 4500
#
# Why forced: in 53010 natural calls RaySphereIntersection never returned true (the segment ends are
# waypoints on the wall; the bug's sphere is 1.25 * its collision radius around its own position), and
# pill1/pbug_force3 failed by writing members (mDestPos was stale, `mag` was not what the test reads).
# This fixture writes nothing of the bug.  It owns ONE CloseToPath frame at a time (the innermost active
# one on the thread; the RaySphere call's args must lie inside that frame, and its older frame must be
# CloseToPath) and does, at most once per bug:
#  mode A (first bug that reaches the call): the REAL function decides.  At the call's entry the caller's
#     local `sphere` (rdi) has its centre set to `start` (*rsi, the caller's other local) and xmm0 (the
#     `mag` argument) is set to 1e6: rayToSphere = 0, so intersectSq = radSq, T = radius < mag, and the
#     real body returns true.  The centre is put back at the return and read back; the result (al) is
#     recorded and must be 1.  No memory of the bug, no inferior call.
#  mode B (first different bug; else the 50th call of the first): the call's RESULT is forced at its
#     return, the mfly4_forced.py precedent: the original al is recorded, al = 1 is written and read back.
# For both, the return of the enclosing CloseToPath is logged (al must be 1: `return true`, line 481).
import hashlib
import os

import gdb

OUT = os.environ.get('COV_OUT', '/tmp')
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
CTP = '_ZNK8CPillBug11CloseToPathER13CStateManagerRK12CTriggerData'
RSI = '_ZN13CollisionUtil21RaySphereIntersectionERK7CSphereRK9CVector3fS5_fRfRS3_'
state = {'binary': 'unchecked', 'installed': 0, 'problems': [], 'ctp': [], 'ctp_entries': 0, 'ray_calls': 0,
         'sample': 0, 'owner': {}, 'a': None, 'b': None, 'forced': [], 'ctp_rets': []}


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def problem(msg):
    state['problems'].append(msg)
    log('pbug2 FAIL: ' + msg)


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr))
    except Exception as e:
        return 'err:' + str(e)[:48]


def evf(expr):
    return float(gdb.parse_and_eval(expr))


def f3(addr):
    return [evf('*(float*)(%d + %d)' % (addr, 4 * i)) for i in range(3)]


class CtpRet(gdb.FinishBreakpoint):
    def __init__(self, frame, rec):
        super().__init__(frame, internal=True)
        self.rec = rec

    def stop(self):
        r = self.rec
        try:
            r['done'] = True
            for this, m, old in r['restore']:
                gdb.execute('set var %s->%s = %s' % (this, m, old))
                back = str(int(gdb.parse_and_eval(this + '->' + m)))
                state['restored'] = state.get('restored', 0) + 1
                if back != old:
                    problem('restore readback %s != %s (%s)' % (back, old, m))
            if r['stack_pos'] is not None and state['ctp'] and state['ctp'][-1] is r:
                state['ctp'].pop()
            else:
                if r in state['ctp']:
                    state['ctp'].remove(r)
            al = ev('$al') & 0xff
            if r.get('forced'):
                state['ctp_rets'].append((r['this'], r['forced'], al))
                log('CloseToPath (this=%#x) forced %s: returned al=%d' % (r['this'], r['forced'], al))
                if al != 1:
                    problem('forced CloseToPath returned %d' % al)
        except Exception as e:
            problem('ctp return exception %r' % e)
        return False

    def out_of_scope(self):
        self.rec['done'] = True
        if self.rec in state['ctp']:
            state['ctp'].remove(self.rec)


class CtpBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + CTP, internal=True)

    def stop(self):
        try:
            state['ctp_entries'] += 1
            frame = gdb.newest_frame()
            rec = {'this': ev('$rdi'), 'entry_sp': ev('$rsp'), 'thread': gdb.selected_thread().num,
                   'done': False, 'forced': None, 'stack_pos': len(state['ctp']), 'restore': []}
            state['ctp'].append(rec)
            # The other two open arms (line 458 mPathLoops false, line 473 equal entries): on the owned
            # frame only, calls 21..1500, one member per call, put back (and read back) at the return.
            n = state['ctp_entries']
            if 20 < n <= 1500:
                this = '((CPillBug*)%d)' % rec['this']
                if n % 3 == 0:
                    members = [('mPathLoops', '0')]
                elif n % 3 == 1 and int(gdb.parse_and_eval(this + '->mPath.mCount')) >= 2:
                    members = [('mPath.mItems[1].value', str(int(gdb.parse_and_eval(this + '->mPath.mItems[0].value'))))]
                else:
                    members = []
                for m, v in members:
                    old = str(int(gdb.parse_and_eval(this + '->' + m)))
                    gdb.execute('set var %s->%s = %s' % (this, m, v))
                    back = str(int(gdb.parse_and_eval(this + '->' + m)))
                    rec['restore'].append((this, m, old))
                    state['arms'] = state.get('arms', 0) + 1
                    if state['arms'] <= 6:
                        log('CloseToPath #%d bug %#x: %s ORIGINAL %s -> %s readback %s' % (n, rec['this'], m, old, v, back))
                    if back != v:
                        problem('arm write readback %s != %s' % (back, v))
            CtpRet(frame, rec)
        except Exception as e:
            problem('ctp entry exception %r' % e)
        return False


class RayRet(gdb.FinishBreakpoint):
    def __init__(self, frame, call):
        super().__init__(frame, internal=True)
        self.call = call

    def stop(self):
        c = self.call
        try:
            al = ev('$al') & 0xff
            c['orig'] = al
            if c['mode'] == 'B':
                gdb.execute('set $al = 1')
                back = ev('$al') & 0xff
                c['back'] = back
                log('ray call %d mode B (bug %#x): ORIGINAL al=%d -> forced 1, readback %d' % (c['id'], c['bug'], al, back))
                if back != 1:
                    problem('mode B al readback %s' % back)
            elif c['mode'] == 'A':
                c['real_result'] = al
                log('ray call %d mode A (bug %#x): the real function returned al=%d (t=%g)'
                    % (c['id'], c['bug'], al, evf('*(float*)%d' % c['t_addr'])))
                if al != 1:
                    problem('mode A real result %d' % al)
                # put the caller's sphere centre back, read it back
                for i, v in enumerate(c['orig_center']):
                    gdb.execute('set *(float*)(%d + %d) = %.9g' % (c['sphere'], 4 * i, v))
                back = f3(c['sphere'])
                c['center_back'] = back
                log('ray call %d mode A: sphere centre restored %s, readback %s' % (c['id'], c['orig_center'], back))
                if back != c['orig_center']:
                    problem('mode A centre restore readback %s != %s' % (back, c['orig_center']))
            c['returned'] = True
        except Exception as e:
            problem('ray return exception %r' % e)
        return False

    def out_of_scope(self):
        problem('ray call %d left without a return' % self.call['id'])


class RayBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + RSI, internal=True)

    def stop(self):
        try:
            frame = gdb.newest_frame()
            older = frame.older()
            if older is None or 'CloseToPath' not in (older.name() or ''):
                return False
            if not state['ctp']:
                problem('RaySphere from CloseToPath with no active entry record')
                return False
            rec = state['ctp'][-1]
            sp, sph, pos, dirp = ev('$rsp'), ev('$rdi'), ev('$rsi'), ev('$rdx')
            t_addr = ev('$rcx')
            if rec['thread'] != gdb.selected_thread().num or not (sp < rec['entry_sp']):
                return False
            for a in (sph, pos, dirp, t_addr):
                if not (sp < a < rec['entry_sp']):
                    problem('ray args outside the owned frame (%#x vs %#x..%#x)' % (a, sp, rec['entry_sp']))
                    return False
            state['ray_calls'] += 1
            n = state['ray_calls']
            bug = rec['this']
            mag = evf('$xmm0.v4_float[0]')
            if state['sample'] < 3:
                state['sample'] += 1
                log('natural sample: bug=%#x centre=%s radius=%g start=%s dir=%s mag=%g'
                    % (bug, f3(sph), evf('*(float*)(%d + 12)' % sph), f3(pos), f3(dirp), mag))
            call = {'id': n, 'bug': bug, 'mode': None, 'returned': False}
            state['owner'].setdefault(bug, n)
            if state['a'] is None:
                call['mode'] = 'A'
            elif state['b'] is None and (bug != state['a'] or n >= 50):
                call['mode'] = 'B'
            if call['mode'] == 'A':
                state['a'] = bug
                call['sphere'] = sph
                call['t_addr'] = t_addr
                call['orig_center'] = f3(sph)
                call['orig_mag'] = mag
                start = f3(pos)
                for i, v in enumerate(start):
                    gdb.execute('set *(float*)(%d + %d) = %.9g' % (sph, 4 * i, v))
                gdb.execute('set $xmm0.v4_float[0] = 1000000')
                back = f3(sph)
                backmag = evf('$xmm0.v4_float[0]')
                log('ray call %d mode A (bug %#x): ORIGINAL centre %s mag %g -> centre %s (= start), mag 1e6; readback %s %g'
                    % (n, bug, call['orig_center'], mag, start, back, backmag))
                if back != start or backmag != 1000000.0:
                    problem('mode A readback centre %s mag %g' % (back, backmag))
                rec['forced'] = 'A'
            elif call['mode'] == 'B':
                state['b'] = bug
                rec['forced'] = 'B'
            if call['mode']:
                state['forced'].append(call)
            RayRet(frame, call)
        except Exception as e:
            problem('ray entry exception %r' % e)
        return False


def summarize():
    p = list(state['problems'])
    if state['installed'] != 1:
        p.append('fixture not installed')
    fa = [c for c in state['forced'] if c['mode'] == 'A' and c.get('returned')]
    fb = [c for c in state['forced'] if c['mode'] == 'B' and c.get('returned')]
    if not fa:
        p.append('no mode A call returned')
    if not fb:
        p.append('no mode B call returned')
    if len(state['ctp_rets']) < len(fa) + len(fb):
        p.append('CloseToPath returns logged %d < forced %d' % (len(state['ctp_rets']), len(fa) + len(fb)))
    if state.get('arms', 0) != state.get('restored', 0) or not state.get('arms'):
        p.append('arm writes %s restores %s' % (state.get('arms', 0), state.get('restored', 0)))
    status = 'OK' if not p else 'FAIL'
    log('arm writes %s, restores (all read back) %s' % (state.get('arms', 0), state.get('restored', 0)))
    line = ('status=%s binary=%s installed=%d ctp_entries=%d ray_calls=%d modeA=%d modeB=%d ctp_returns_true=%d reason=%s\n'
            % (status, state['binary'], state['installed'], state['ctp_entries'], state['ray_calls'], len(fa), len(fb),
               len([r for r in state['ctp_rets'] if r[2] == 1]), '; '.join(p) or '-'))
    with open(TABLE, 'a') as fh:
        fh.write(line)
    log('pbug2 summary: ' + line.strip())


def binary_sha():
    try:
        fn = gdb.current_progspace().filename
        h = hashlib.sha256()
        with open(fn, 'rb') as fh:
            for ch in iter(lambda: fh.read(1 << 20), b''):
                h.update(ch)
        return h.hexdigest()
    except Exception as e:
        return 'unavailable: %r' % e


gdb.events.exited.connect(lambda event: summarize())
state['binary'] = binary_sha()
try:
    CtpBp()
    RayBp()
    state['installed'] = 1
    log('pbug2: fixture installed (binary %s)' % state['binary'][:12])
except Exception as e:
    problem('install failed: %r' % e)
