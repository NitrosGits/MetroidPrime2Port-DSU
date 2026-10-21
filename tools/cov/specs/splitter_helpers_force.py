# Splitter (module 75) forcing spec for tools/cov/force.sh: the arms the shipped spawns cannot
# reach.  The head's three render gates (fn_75_6310, fn_75_633C, fn_75_6368) ask the base's flag
# byte at 0x420 first: with the actor dead they do not call the base class.  A dead head is
# deleted, so no run has one - the arm is taken by writing the alive flag at the body's entry
# (one call each).  The head's Listen (fn_75_5F18) has the same gate.
#
# One body per run: the force makes the head dead, and a dead head is not rendered any more, so
# forcing one gate would keep the other gates from being called at all.  The runs' counters add
# up (tools/cov/gcov_report.py takes several prefix dirs), so the merged profile has every arm.
#
# Run with COV_GDB_SCRIPT=tools/cov/cov_force.py and BUILD=build-port-cov, so the run's counters
# (and the forced arm) land in the coverage profile that tools/cov/gcov_report.py reads:
#
#   SPLITTER_FORCE=6310 COV_GDB_SCRIPT=tools/cov/cov_force.py BUILD=build-port-cov \\
#     tools/cov/force.sh tools/cov/specs/splitter_helpers_force.py <out> 1baa96c2,914f1381 \\
#     --con tools/cov/specs/splitter_hive_temple.con --fast --frames 12000
import os

CLASS = 'CSplitterCommandModule'
WHICH = os.environ.get('SPLITTER_FORCE', '6310')
BODIES = {
    '6310': ('6310 AddToRenderer', 'CSplitterCommandModule::PortAddToRenderer(',
             'the head is dead: the base class is not called'),
    '633C': ('633C PreRender', 'CSplitterCommandModule::PortPreRender(',
             'the head is dead: the base class is not called'),
    '6368': ('6368 Render', 'CSplitterCommandModule::PortRender(',
             'the head is dead: the base class is not called'),
    '5F18': ('5F18 Listen', 'CSplitterCommandModule::PortListen(',
             'the head is not alive: the Listen returns false without asking the distance'),
    '6394': ('6394 DoUserAnimEvent', 'CSplitterCommandModule::PortDoUserAnimEvent(',
             'the event is a projectile one: the head shoots at the player'),
    'EFA4': ('EFA4 DoUserAnimEvent', 'CSplitterMainChassis::PortDoUserAnimEvent(',
             'the event is a break-lock one: the chassis asks the player for the orbit'),
    'EFA4M': ('EFA4 DoUserAnimEvent', 'CSplitterMainChassis::PortDoUserAnimEvent(',
              'the event is a material one: the chassis reflects its vulnerability'),
}
_label, _where, _what = BODIES[WHICH]
FORCE = [
    (_label, _where, [], ['$this->mAlive = 0'], 1, _what),
]
# The two DoUserAnimEvent bodies are forced with the event type in the register (r6) instead of
# the alive flag: the fight does not play the head's projectile event, and the chassis's own
# DoUserAnimEvent is not reached at all.
if WHICH == '6394':
    FORCE = [(_label, _where, [], ['$r6 = 0'], 1, _what)]
elif WHICH == 'EFA4':
    FORCE = [(_label, _where, [], ['$r6 = 0x16'], 1, _what)]
elif WHICH == 'EFA4M':
    FORCE = [(_label, _where, [], ['$r6 = 0x5'], 1, _what)]
SPEC = [
    # fn_75_6310: the head's AddToRenderer runs the base class's call only while the actor is
    # alive (the base's flag byte at 0x420, its 0x40 bit).
    ('6310 AddToRenderer', 'CSplitterCommandModule::PortAddToRenderer(',
     ['$this->GetAlive() != 0'], 'ret calls'),
    # fn_75_633C: the head's PreRender, the same gate.
    ('633C PreRender', 'CSplitterCommandModule::PortPreRender(', ['$this->GetAlive() != 0'], 'ret calls'),
    # fn_75_6368: the head's Render, the same gate.
    ('6368 Render', 'CSplitterCommandModule::PortRender(', ['$this->GetAlive() != 0'], 'ret calls'),
    # fn_75_5E50: the head is only possessed while it is docked (the chassis's id at 0xEFA).
    ('5E50 CanBeIngPossessed', 'CSplitterCommandModule::PortCanBeIngPossessed(',
     ['$this->mChassisId.value != 0xffff'], 'ret'),
    # fn_75_5F18: the head's Listen: alive, the noise type 4 (kLNT_PathObstruction) and the
    # distance to the noise (the squared distance under 400).
    ('5F18 Listen', 'CSplitterCommandModule::PortListen(', ['$this->GetAlive() != 0', '$rcx'], 'ret'),
]
