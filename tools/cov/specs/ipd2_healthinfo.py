# IngPuddle (module 32) ipd2: fn_32_0 CIngPuddle::HealthInfo, the inline CHealthInfo* getter
# (include/MetroidPrime/Enemies/CIngPuddle.hpp).  force.py, no writes at all: the oracle only reads.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/ipd2_healthinfo.py <out> \
#     42b935e4,06_sand_alamofront_dark --con tools/cov/specs/ingpuddle.con --fast --frames 3200
#
# The engine reaches the getter through CActor's vtable (CStateManager's damage paths call
# damagee.HealthInfo(), and a CActor& cannot be devirtualized), so the out-of-line copy of this inline
# function is the one that runs.  For every call the oracle captures `this` ($rdi) and the caller
# frame, arms a finish breakpoint and checks the actual return value against this + mHealthInfo's own
# offset - the offset comes from the debug info, never a literal - and logs both.  Nothing is written,
# nothing is forced; the puddles are damaged by the driver's shots and `damage` command.
CLASS = 'CIngPuddle'

import os

import gdb

_OUT = os.environ.get('COV_OUT', '/tmp')
_LOG = os.path.join(_OUT, 'ipd2_healthinfo.log')

_st = {'calls': 0, 'match': 0, 'mismatch': 0, 'noReturn': 0, 'unreadable': 0, 'off': None,
       'logged': 0, 'callers': {}}


def _log(msg):
    try:
        with open(_LOG, 'a') as fh:
            fh.write(msg + '\n')
    except Exception:
        pass


def _i(g, e):
    return int(g.parse_and_eval(e))


def _off(g):
    if _st['off'] is None:
        _st['off'] = _i(g, '(long)&((CIngPuddle*)0)->mHealthInfo')
        _log('mHealthInfo offset in this build: 0x%x (%d) - retail fn_32_0 is `addi r3,r3,0x460`'
             % (_st['off'], _st['off']))
    return _st['off']


class _Ret(gdb.FinishBreakpoint):
    """Reads the getter's actual return value and compares it with this + mHealthInfo."""

    def __init__(self, frame, info):
        super().__init__(frame, internal=True)
        self.info = info

    def stop(self):
        info = self.info
        st = _st
        try:
            ret = _i(gdb, '$rax')
        except Exception as e:
            st['noReturn'] += 1
            _log('HEALTHINFO %s UNREADABLE return: %s' % (info['tag'], e))
            return False
        want = info['this'] + info['off']
        same = ret == want
        if same:
            st['match'] += 1
        else:
            st['mismatch'] += 1
        if st['logged'] < 6 or not same:
            st['logged'] += 1
            _log('HEALTHINFO %s this=0x%x return=0x%x expected(this+mHealthInfo)=0x%x %s'
                 ' caller=(level %s, %s)'
                 % (info['tag'], info['this'], ret, want, 'MATCH' if same else 'MISMATCH',
                    info['clvl'], info['cname']))
        return False


def healthinfo_oracle(g):
    st = _st
    st['calls'] += 1
    tag = 'ipd2-%d' % st['calls']
    try:
        this = _i(g, '$rdi')
        off = _off(g)
        frame = g.newest_frame()
        caller = frame.older() if frame is not None else None
        clvl = caller.level() if caller is not None else None
        cname = caller.name() if caller is not None else None
    except Exception as e:
        st['unreadable'] += 1
        if st['unreadable'] <= 5:
            _log('HEALTHINFO %s this/frame/caller unreadable: %s' % (tag, e))
        return
    key = '%s (level %s)' % (cname, clvl)
    st['callers'][key] = st['callers'].get(key, 0) + 1
    try:
        _Ret(frame, {'tag': tag, 'this': this, 'off': off, 'clvl': clvl, 'cname': cname})
    except Exception as e:
        st['noReturn'] += 1
        _log('HEALTHINFO %s: could not arm the return breakpoint: %s' % (tag, e))


def _summary(event=None):
    st = _st
    ok = (st['calls'] > 0 and st['mismatch'] == 0 and st['noReturn'] == 0
          and st['unreadable'] == 0 and st['match'] == st['calls'])
    shown = sorted(st['callers'].items(), key=lambda kv: -kv[1])[:6]
    _log('SUMMARY exit: calls=%d match=%d mismatch=%d noReturn=%d unreadable=%d offset=0x%x'
         ' callers=%s status=%s'
         % (st['calls'], st['match'], st['mismatch'], st['noReturn'], st['unreadable'],
            st['off'] or 0, shown, 'OK' if ok else 'FAIL'))


try:
    gdb.events.exited.connect(_summary)
except Exception:
    pass


FORCE = [
    ('32_0 HealthInfo', 'CIngPuddle::HealthInfo(', [], [healthinfo_oracle], 0,
     'per call: captures this and the caller frame, then checks the actual return value against'
     ' this + mHealthInfo (nothing written, nothing forced)'),
]
