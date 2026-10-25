# MysteryFlyer (module 45) mfly2 fixture for CMysteryFlyer::DoUserAnimEvent (fn_45_237C).  This file is
# BOTH the force.sh spec argument and COV_GDB_SCRIPT.  It writes exactly one register, once per run.
#
# Source (src/MetroidPrime/Enemies/CMysteryFlyer.cpp:139-145):
#   if (!(type == kUE_FadeIn ? true : false)) CPatterned::DoUserAnimEvent(mgr, node, type, dt);
# Retail (fn_45_237C, 0x237C): `cmpwi r6,0xe` (type in r6, kUE_FadeIn = 14), the ?: idiom materialises
# the flag in r0, `clrlwi. r0,0,24` / `bne` skips, else `bl DoUserAnimEvent__10CPatterned...`.
# Host (checked statically before the run): `cmp $0xe,%ecx` at the entry block, `jne` to the base call,
# `ret` on the FadeIn side - so the by-value EUserEventType argument lives in **ecx** (SysV's fourth
# integer argument) and is not spilled, which is the only ABI location the fixture touches.
#
# Shipped MYSF animations have no FadeIn event (mf1 note), so the skip arm never happens naturally.
# The fixture forces it on ONE real invocation: at the entry breakpoint of the call whose index is
# $MFORCE_CALL (default 1), it snapshots ecx, ARMS a FinishBreakpoint for that call, then writes
# ecx = 14.  At the return it writes the original ecx back and reads it back.  No stack write at all,
# no fabricated call or object, no source probe.  Every other invocation is left untouched (natural
# base-call path).
#
# The base call is observed at CPatterned::DoUserAnimEvent: the fixture records the caller frame, so a
# hit is attributed to CMysteryFlyer::DoUserAnimEvent (this function's call site) or to another caller.
# Per-call obligations: the forced call must RETURN (real return, not entry) with caller-attributed base
# hits of 0; a natural call must return with at least one caller-attributed base hit.  The summary FAILs
# on any gap (no calls, forced index never reached, forced call called the base, natural call skipped the
# base, unreached return, ecx restore readback mismatch, install failure, implausible type values).
#
#   MFORCE_CALL=1 ... -> the FadeIn skip arm (call 1 is forced); MFORCE_CALL=0 -> natural-only, every
#     call keeps its own type and the base path is the evidence (a run had exactly one call, so the two
#     arms are measured in two launches and their gcov profiles are merged).
#   MFORCE_CALL=1 BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/mfly2_douseranimevent.py \
#     tools/cov/force.sh tools/cov/specs/mfly2_douseranimevent.py /tmp/opencode/mfly2_r1 \
#     3bfa3eff,0e_temple_hall --con tools/cov/specs/mfly2_douseranimevent.con --fast --frames 3100
import hashlib
import os

import gdb

OUT = os.environ.get('COV_OUT', '/tmp')
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
FADE_IN = 14  # kUE_FadeIn, include/MetroidPrime/ActorCommon.hpp:147; retail `cmpwi r6,0xe`
MYF = '_ZN13CMysteryFlyer15DoUserAnimEventER13CStateManagerRK13CInt32POINode14EUserEventTypef'
BASE = '_ZN10CPatterned15DoUserAnimEventER13CStateManagerRK13CInt32POINode14EUserEventTypef'
try:
    FORCE_CALL = int(os.environ.get('MFORCE_CALL', '1'))
except ValueError:
    FORCE_CALL = 1
state = {'installed': 0, 'binary': 'unchecked', 'calls': [], 'current': None, 'base_this': 0,
         'base_other': 0, 'types_seen': [], 'problems': []}


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def problem(msg):
    state['problems'].append(msg)
    log('mfly2 FAIL: ' + msg)


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return 'err:' + str(e)[:48]
    except Exception as e:
        return 'exc:' + repr(e)[:48]


def fmt(v):
    return '0x%x' % v if isinstance(v, int) else str(v)


class FlyerReturn(gdb.FinishBreakpoint):
    """At the return of one DoUserAnimEvent call: restore the register, record the completed call."""

    def __init__(self, frame, call):
        super().__init__(frame, internal=True)
        self.call = call

    def stop(self):
        call = self.call
        call['returned'] = True
        if state['current'] is call:
            state['current'] = None
        if call['forced']:
            try:
                gdb.execute('set $ecx = %d' % call['ecx_original'])
                back = ev('$ecx')
            except gdb.error as e:
                problem('call %d: ecx restore failed: %s' % (call['id'], str(e)[:60]))
                back = None
            if back == call['ecx_original']:
                call['ecx_restored'] = True
                log('call %d: restored ORIGINAL ecx = %s (read back %s), baseCalls=%d'
                    % (call['id'], fmt(call['ecx_original']), fmt(back), call['base_hits']))
            else:
                problem('call %d: ecx restore readback %s, wanted %s'
                        % (call['id'], fmt(back), fmt(call['ecx_original'])))
        log('call %d: returned (forced=%d type_in=%s base_hits=%d)'
            % (call['id'], int(call['forced']), fmt(call['type_in']), call['base_hits']))
        return False

    def out_of_scope(self):
        problem('call %d: DoUserAnimEvent left without a return' % self.call['id'])


class EntryBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + MYF, internal=True)

    def stop(self):
        try:
            self._stop()
        except Exception as e:
            problem('entry exception on call %d: %r' % (len(state['calls']) + 1, e))
        return False

    def _stop(self):
        ecx = ev('$ecx')
        if not isinstance(ecx, int):
            problem('entry: ecx unreadable (%s)' % ecx)
            return
        call = {'id': len(state['calls']) + 1, 'type_in': ecx, 'forced': False, 'returned': False,
                'base_hits': 0, 'ecx_original': ecx, 'ecx_restored': False}
        state['calls'].append(call)
        state['current'] = call
        if ecx not in state['types_seen']:
            state['types_seen'].append(ecx)
        if not -1 <= ecx <= 63:
            problem('call %d: type_in %s is not a plausible EUserEventType (wrong ABI slot?)'
                    % (call['id'], fmt(ecx)))
        try:
            FlyerReturn(gdb.newest_frame(), call)
            call['armed'] = True
        except Exception as e:
            problem('call %d: FinishBreakpoint not created (%r); no write attempted'
                    % (call['id'], e))
            call['armed'] = False
            return
        if call['id'] != FORCE_CALL:
            log('call %d: natural, type_in=%s (base path expected)' % (call['id'], fmt(ecx)))
            return
        try:
            gdb.execute('set $ecx = %d' % FADE_IN)
            now = ev('$ecx')
        except gdb.error as e:
            problem('call %d: ecx write failed: %s' % (call['id'], str(e)[:60]))
            return
        if now == FADE_IN:
            call['forced'] = True
            log('call %d: FORCED type %s -> %d (kUE_FadeIn); snapshot ecx=%s kept for the return'
                % (call['id'], fmt(ecx), FADE_IN, fmt(ecx)))
        else:
            problem('call %d: ecx write did not take (read %s)' % (call['id'], fmt(now)))


class BaseBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + BASE, internal=True)

    def stop(self):
        try:
            frame = gdb.newest_frame()
            older = frame.older() if frame is not None else None
            name = (older.name() or '') if older is not None else ''
            if 'CMysteryFlyer::DoUserAnimEvent' in name:
                state['base_this'] += 1
                call = state['current']
                if call is not None:
                    call['base_hits'] += 1
                log('base DoUserAnimEvent hit from CMysteryFlyer::DoUserAnimEvent (call %s)'
                    % (call['id'] if call else '?'))
            else:
                state['base_other'] += 1
                log('base DoUserAnimEvent hit from another caller: %s' % (name or '(unknown)'))
        except Exception as e:
            problem('base breakpoint exception: %r' % e)
        return False


def summarize():
    problems = list(state['problems'])
    calls = state['calls']
    if state['installed'] != 1:
        problems.append('fixture not installed')
    if not calls:
        problems.append('no DoUserAnimEvent call')
    forced = [c for c in calls if c['forced']]
    natural = [c for c in calls if not c['forced'] and c['returned']]
    if FORCE_CALL >= 1 and not forced:
        problems.append('the forced FadeIn call never happened (MFORCE_CALL=%d, calls=%d)'
                        % (FORCE_CALL, len(calls)))
    unreturned = [c['id'] for c in calls if not c['returned']]
    if unreturned:
        problems.append('unreturned call(s): %s' % unreturned)
    for c in forced:
        if c['returned'] and c['base_hits'] != 0:
            problems.append('call %d: the forced FadeIn call still reached the CPatterned base %d time(s)'
                            % (c['id'], c['base_hits']))
        if c['returned'] and not c['ecx_restored']:
            problems.append('call %d: ecx not restored' % c['id'])
    if not natural:
        problems.append('no natural (non-FadeIn) call with a base call was observed')
    elif not any(c['base_hits'] > 0 for c in natural):
        problems.append('no natural call reached the CPatterned base')
    status = 'OK' if not problems else 'FAIL'
    line = ('status=%s mode=%s binary=%s installed=%d calls=%d forced=%d forced_returned=%d '
            'forced_base_hits=%d natural_returned=%d base_from_this=%d base_other=%d types=%s '
            'reason=%s\n'
            % (status, 'forced' if FORCE_CALL >= 1 else 'natural-only', state['binary'],
               state['installed'], len(calls), len(forced),
               sum(1 for c in forced if c['returned']),
               sum(c['base_hits'] for c in forced), len(natural), state['base_this'],
               state['base_other'], state['types_seen'],
               '; '.join(problems) if problems else '-'))
    with open(TABLE, 'a') as fh:
        fh.write(line)
    log('mfly2 summary: ' + line.strip())
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
        return None, 'no binary path in the progspace'
    path = fn if os.path.isabs(fn) else os.path.join(os.getcwd(), fn)
    try:
        return sha256_file(path), path
    except OSError as e:
        return None, 'cannot read %s: %s' % (path, str(e)[:60])


gdb.events.exited.connect(lambda event: summarize())
_sha, _why = binary_sha()
state['binary'] = _sha or ('unavailable: ' + str(_why))
try:
    EntryBp()
    BaseBp()
    state['installed'] = 1
    log('mfly2: fixture installed (binary %s, MFORCE_CALL=%d, kUE_FadeIn=%d)'
        % ((_sha or '?')[:12], FORCE_CALL, FADE_IN))
except Exception as e:
    problem('breakpoint install failed (symbols: %s / %s): %r' % (MYF[:30], BASE[:30], e))
