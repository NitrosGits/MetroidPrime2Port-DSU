# Splitter (module 75) split24 lane: split23's run (its con, member writes and return overrides)
# plus one batch of gdb calls (the ds43 way) for the arms shipped data never takes: at the
# SPLIT24_AT-th (default 6000) CSplitterCommandModule::Think entry gdb sources
# tools/cov/specs/splitter_split24_calls.py, which writes the members each arm needs, calls the
# body and writes every member back.  The GetConnectedObject breakpoint below answers the head's
# own id while $split24_gco is 1 (HasDockingPath's true arm, fn_75_5864).
#
#   COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py BUILD=$PWD/build-port-cov SECS=3600 \
#     tools/cov/force.sh tools/cov/specs/splitter_split24.py <out> 1baa96c2,914f1381 \
#     --con tools/cov/specs/splitter_split23.con --fast --frames 12000
import os
import gdb

_S23 = os.path.join(os.path.dirname(os.path.abspath(os.environ['COV_SPEC'])), 'splitter_split23.py')
exec(compile(open(_S23).read(), _S23, 'exec'))

if gdb.convenience_variable('split24_on') is None:
    gdb.set_convenience_variable('split24_on', 1)
    gdb.set_convenience_variable('split24_gco', 0)
    _HERE = os.path.dirname(os.path.abspath(os.environ['COV_SPEC']))

    class _Split24Gco(gdb.FinishBreakpoint):
        def stop(self):
            gdb.execute('set var $rax = $split24_gcoid')
            return False

        def out_of_scope(self):
            pass

    class _Split24GcoBp(gdb.Breakpoint):
        def stop(self):
            v = gdb.convenience_variable('split24_gco')
            if v is not None and int(v) == 1:
                _Split24Gco(gdb.newest_frame(), internal=True)
            return False

    for _m in _addrs('CPatterned::GetConnectedObject('):
        _Split24GcoBp('*' + _m, internal=True)

    # Gated return overrides for the forced calls: while the convenience variable is set (not
    # void) the callee returns its value; with a caller prefix only a direct call from it counts.
    class _GateFinish(gdb.FinishBreakpoint):
        def __init__(self, frame, value):
            super().__init__(frame, internal=True)
            self.value = value

        def stop(self):
            gdb.execute('set var $rax = %s' % self.value)
            return False

        def out_of_scope(self):
            pass

    class _GateBp(gdb.Breakpoint):
        def __init__(self, where, var, caller):
            super().__init__(where, internal=True)
            self.var, self.caller = var, caller

        def stop(self):
            v = gdb.convenience_variable(self.var)
            if v is None:
                return False
            if self.caller:
                up = gdb.newest_frame().older()
                if up is None or not up.name() or not up.name().startswith(self.caller):
                    return False
            _GateFinish(gdb.newest_frame(), str(v))
            return False

    for _callee, _var, _caller in (
            ('CPatterned::FindBestStepDirection(', 's24_sd', 'CSplitterMainChassis::PortFindDodgeDirection'),
            ('CVector3f::IsMagnitudeSafe(', 's24_ims', 'CSplitterCommandModule::FollowDockingPath'),
            ('CSplitterCommandModule::InDetectionRange(', 's24_idr', None),
            ('CScriptTeamAiMgr::GetTeamAiRole(', 's24_role', None),
            ('CPatterned::PathOver(', 's24_po', None)):
        for _m in _addrs(_callee):
            _GateBp('*' + _m, _var, _caller)

    # The beam effect and a second head, recorded at their Think for the forced calls.
    class _Split24Rec(gdb.Breakpoint):
        def __init__(self, where, var, other):
            super().__init__(where, internal=True)
            self.var, self.other = var, other

        def stop(self):
            this = int(gdb.parse_and_eval('$rdi'))
            if self.other is not None:
                first = gdb.convenience_variable(self.other)
                if first is None:
                    gdb.set_convenience_variable(self.other, this)
                    return False
                if int(first) == this:
                    return False
            gdb.set_convenience_variable(self.var, this)
            self.enabled = False
            return False

    _beam = gdb.Breakpoint('*' + _addrs('CSplitterBeamEffect::Think(')[0])
    _beam.ignore_count = int(os.environ.get('SPLIT24_BEAM_AT', '20'))
    gdb.set_convenience_variable('s24_beambp', _beam.number)
    _beam.commands = 'source %s/splitter_split24_beam.py\ndelete %d\ncontinue' % (_HERE, _beam.number)
    for _m in _addrs('CSplitterCommandModule::Think('):
        _Split24Rec('*' + _m, 's24h2', 's24h1')

    _heads = _addrs('CSplitterCommandModule::Think(')
    _think = gdb.Breakpoint('*' + _heads[0])
    _think.ignore_count = int(os.environ.get('SPLIT24_AT', '6000'))
    gdb.set_convenience_variable('s24_think', _think.number)
    _think.commands = 'source %s/splitter_split24_calls.py\ndelete %d\ncontinue' % (_HERE, _think.number)

# fn_75_EBF4 KnockBack: the con's boostball and cannonball hits with 0xD41's 0x80 bit cleared
# for the call (both weapon-type arms).
# SPLIT24_KB=0 leaves them alone (the 0x80 arm, the game's own state).
if os.environ.get('SPLIT24_KB', '1') == '1':
    FORCE_RET += [
        ('fn_75_EBF4', 'CSplitterMainChassis::KnockBack(', 1,
         [(0, [('$this->mFlagsD41', '$this->mFlagsD41 & 0x7f')])], 'knock back 0x80 clear'),
    ]
# fn_75_EB04 Death: the first collision description's actor id invalid for the call (the cast's
# null arm, 0xEB5C-0xEB60).
FORCE_RET += [
    ('fn_75_EB04', 'CSplitterMainChassis::Death(', 1,
     [(0, [('$this->mCollisionActorManager->mJointDescriptions.mItems[0].mActorId.value', '0xffff')])],
     'null collision actor'),
]
