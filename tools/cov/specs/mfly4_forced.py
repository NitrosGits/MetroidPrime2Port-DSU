# MysteryFlyer (module 45) mfly4 fixture: forced arms of CMysteryFlyer::JoinTeam (fn_45_C10) and
# CMysteryFlyer::AcceptScriptMsg (fn_45_23B8) on 05_portalpuzzle.  BOTH the force.sh spec and COV_GDB_SCRIPT.
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/mfly4_forced.py \
#     tools/cov/force.sh tools/cov/specs/mfly4_forced.py <out> 42b935e4,05_portalpuzzle \
#     --con tools/cov/specs/mfly4_portal.con --fast --frames 3200
#
# Why forced: the disc scan (tools/scan_mysteryflyer_loader.py) shows no shipped MYSF links to a TMAI, so
# GetAssociatedTeamId never returns a valid id; and no shipped wasp has a KnockBack body state, so
# HasBodyState(kAS_KnockBack) is never true.  The fixture changes only RESULTS of calls, at the return,
# on ONE wasp (the first object whose call reaches the hook = single-object filter), nothing else:
#  (1) JoinTeam: at the return of GetAssociatedTeamId (static, rdi=CAi*, rsi=mgr; result in ax) called
#      from CMysteryFlyer::JoinTeam / AcceptScriptMsg, the first MFORCE_TEAM_CALLS (default 3) calls of the
#      target wasp get ax = MFORCE_TEAM_UID (the uid of the real TMAI on layer 3, from `objs`).  JoinTeam
#      then runs ObjectById/TCastToPtr/JoinTeam on the REAL team; a later JoinTeam entry with the id still
#      set takes the retail "bne -> end" arm naturally.  The original ax is recorded and the write read back.
#  (2) AcceptScriptMsg kSM_Damage: at the return of CBodyController::HasBodyState called from
#      AcceptScriptMsg, the target wasp's 1st call gets al=1 (state != KnockBack naturally -> DeliverCmd
#      arm), the 2nd call gets al=1 AND, at the `jne` after `cmpl $6,0x4e0(%rbp)`, ZF=1 (state == KnockBack
#      -> skip arm).  Original al / eflags recorded, writes read back.  No memory write, no inferior call.
import hashlib
import os

import gdb

OUT = os.environ.get('COV_OUT', '/tmp')
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
TEAM_UID = int(os.environ.get('MFORCE_TEAM_UID', '0x70b'), 0)
TEAM_CALLS = int(os.environ.get('MFORCE_TEAM_CALLS', '3'))
GAT = '_ZN16CScriptTeamAiMgr19GetAssociatedTeamIdERK3CAiR13CStateManager'
HBS = '_ZNK15CBodyController12HasBodyStateEN3pas15EAnimationStateE'
ASM = '_ZN13CMysteryFlyer15AcceptScriptMsgER13CStateManagerRK10CScriptMsg'
JT = '_ZN13CMysteryFlyer8JoinTeamER13CStateManager'
state = {'binary': 'unchecked', 'target_team': None, 'team_calls': [], 'target_hbs': None, 'hbs_calls': [],
         'flip_pending': None, 'flips': [], 'jt_entries': [], 'problems': [], 'installed': 0}


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def problem(msg):
    state['problems'].append(msg)
    log('mfly4 FAIL: ' + msg)


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr))
    except Exception as e:
        return 'err:' + str(e)[:48]


def caller_name(frame):
    older = frame.older() if frame is not None else None
    return (older.name() or '') if older is not None else ''


class TeamRet(gdb.FinishBreakpoint):
    def __init__(self, frame, call):
        super().__init__(frame, internal=True)
        self.call = call

    def stop(self):
        c = self.call
        try:
            orig = ev('$ax') & 0xffff
            c['orig'] = orig
            if c['force']:
                gdb.execute('set $ax = %d' % TEAM_UID)
                back = ev('$ax') & 0xffff
                c['back'] = back
                log('team call %d (%s this=%#x): ORIGINAL ax=%#x -> forced %#x, readback %#x'
                    % (c['id'], c['caller'], c['this'], orig, TEAM_UID, back))
                if back != TEAM_UID:
                    problem('team call %d: ax readback %#x' % (c['id'], back))
            else:
                log('team call %d (%s this=%#x): natural ax=%#x' % (c['id'], c['caller'], c['this'], orig))
            c['returned'] = True
        except Exception as e:
            problem('team return exception %r' % e)
        return False

    def out_of_scope(self):
        problem('team call %d left without a return' % self.call['id'])


class TeamBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + GAT, internal=True)

    def stop(self):
        try:
            frame = gdb.newest_frame()
            who = caller_name(frame)
            this = ev('$rdi')
            if 'CMysteryFlyer' not in who:
                return False
            if state['target_team'] is None:
                state['target_team'] = this
                log('team target wasp this=%#x (first call, from %s)' % (this, who))
            call = {'id': len(state['team_calls']) + 1, 'caller': who.split('(')[0], 'this': this,
                    'force': False, 'returned': False}
            mine = [c for c in state['team_calls'] if c['this'] == this]
            if this == state['target_team'] and len(mine) < TEAM_CALLS:
                call['force'] = True
            state['team_calls'].append(call)
            TeamRet(frame, call)
        except Exception as e:
            problem('team entry exception %r' % e)
        return False


class HbsRet(gdb.FinishBreakpoint):
    def __init__(self, frame, call):
        super().__init__(frame, internal=True)
        self.call = call

    def stop(self):
        c = self.call
        try:
            orig = ev('$al') & 0xff
            c['orig'] = orig
            if c['force']:
                gdb.execute('set $al = 1')
                back = ev('$al') & 0xff
                c['back'] = back
                log('hbs call %d (this=%#x): ORIGINAL al=%d -> forced 1, readback %d, flip=%d'
                    % (c['id'], c['this'], orig, back, int(c['flip'])))
                if back != 1:
                    problem('hbs call %d: al readback %d' % (c['id'], back))
                if c['flip']:
                    state['flip_pending'] = c['this']
            else:
                log('hbs call %d (this=%#x): natural al=%d' % (c['id'], c['this'], orig))
            c['returned'] = True
        except Exception as e:
            problem('hbs return exception %r' % e)
        return False

    def out_of_scope(self):
        problem('hbs call %d left without a return' % self.call['id'])


class HbsBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + HBS, internal=True)

    def stop(self):
        try:
            frame = gdb.newest_frame()
            who = caller_name(frame)
            if 'CMysteryFlyer::AcceptScriptMsg' not in who:
                return False
            this = int(frame.older().read_register('rbx'))
            if state['target_hbs'] is None:
                state['target_hbs'] = this
                log('hbs target wasp this=%#x' % this)
            mine = [c for c in state['hbs_calls'] if c['this'] == this]
            call = {'id': len(state['hbs_calls']) + 1, 'this': this, 'force': False, 'flip': False,
                    'returned': False}
            if this == state['target_hbs'] and len(mine) < 2:
                call['force'] = True
                call['flip'] = len(mine) == 1
            state['hbs_calls'].append(call)
            HbsRet(frame, call)
        except Exception as e:
            problem('hbs entry exception %r' % e)
        return False


class FlipBp(gdb.Breakpoint):
    """At the `jne` after `cmpl $0x6,0x4e0(%rbp)`: ZF=1 (state == KnockBack) for the flagged call only."""

    def __init__(self, spec):
        super().__init__(spec, internal=True)

    def stop(self):
        try:
            this = int(gdb.newest_frame().read_register('rbx'))
            if state['flip_pending'] is None or state['flip_pending'] != this:
                return False
            insn = gdb.execute('x/i $pc', to_string=True)
            if 'jne' not in insn:
                problem('flip site is not a jne: ' + insn.strip())
                return False
            orig = ev('$eflags')
            gdb.execute('set $eflags = $eflags | 0x40')
            back = ev('$eflags')
            state['flips'].append((orig, back))
            state['flip_pending'] = None
            log('flip (this=%#x): ORIGINAL eflags=%#x -> %#x (ZF=1), readback %#x' % (this, orig, orig | 0x40, back))
            if not (isinstance(back, int) and back & 0x40):
                problem('eflags readback %s' % back)
        except Exception as e:
            problem('flip exception %r' % e)
        return False


class JtBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + JT, internal=True)

    def stop(self):
        try:
            state['jt_entries'].append(ev('$rdi'))
            log('JoinTeam entry this=%#x (entry %d)' % (ev('$rdi'), len(state['jt_entries'])))
        except Exception as e:
            problem('jt exception %r' % e)
        return False


def summarize():
    p = list(state['problems'])
    if state['installed'] != 1:
        p.append('fixture not installed')
    forced_team = [c for c in state['team_calls'] if c['force'] and c.get('returned')]
    if not forced_team:
        p.append('no forced team call returned')
    hb = [c for c in state['hbs_calls'] if c['force'] and c.get('returned')]
    if len(hb) < 2:
        p.append('forced HasBodyState calls returned: %d (<2)' % len(hb))
    if not state['flips']:
        p.append('no ZF flip happened')
    if any(not c.get('returned') for c in state['team_calls'] + state['hbs_calls']):
        p.append('unreturned call(s)')
    status = 'OK' if not p else 'FAIL'
    line = ('status=%s binary=%s installed=%d team_calls=%d forced_team=%d hbs_calls=%d forced_hbs=%d flips=%d '
            'joinTeam_entries=%d reason=%s\n' % (status, state['binary'], state['installed'],
            len(state['team_calls']), len(forced_team), len(state['hbs_calls']), len(hb), len(state['flips']),
            len(state['jt_entries']), '; '.join(p) or '-'))
    with open(TABLE, 'a') as fh:
        fh.write(line)
    log('mfly4 summary: ' + line.strip())


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
    TeamBp()
    HbsBp()
    JtBp()
    FlipBp('*(' + ASM + '+0x107)')
    state['installed'] = 1
    log('mfly4: fixture installed (binary %s, team uid %#x x%d)' % (state['binary'][:12], TEAM_UID, TEAM_CALLS))
except Exception as e:
    problem('install failed: %r' % e)
