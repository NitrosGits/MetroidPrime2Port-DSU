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
    '1CA0D': ('1CA0 ProjectileAim', 'CSplitterCommandModule::PortHeadProjectileAim(',
              'the head is dead: the aim ends at once'),
    '1CA0P': ('1CA0 ProjectileAim', 'CSplitterCommandModule::PortHeadProjectileAim(',
              'there is no player at 0xEFE: the aim ends at once'),
    '6394FLAG': ('6394 DoUserAnimEvent', 'CSplitterCommandModule::PortDoUserAnimEvent(',
                 'the head\'s flag byte at 0xF6A has its 0x10 bit: no orbit request'),
    '6394DATA': ('6394 DoUserAnimEvent', 'CSplitterCommandModule::PortDoUserAnimEvent(',
                 'the data\'s flag word at 0x7C0 has its 0x20000000 bit: no orbit request'),
    '6394NOPLAYER': ('6394 DoUserAnimEvent', 'CSplitterCommandModule::PortDoUserAnimEvent(',
                     'there is no player at 0xEFE: no orbit request'),
    '6394OTHER': ('6394 DoUserAnimEvent', 'CSplitterCommandModule::PortDoUserAnimEvent(',
                  'another event type: the base class is asked and nothing else happens'),
}
if WHICH in ('6394DEAD', '6394NOPLAYER'):
    _label, _where, _what = BODIES['6394']
else:
    _label, _where, _what = BODIES[WHICH]
FORCE = [
    (_label, _where, [], ['$this->mAlive = 0'], 1, _what),
]
# The two DoUserAnimEvent bodies are forced with the event type in the register instead of the
# alive flag: the fight does not play the head's projectile event, and the chassis's own
# DoUserAnimEvent is not reached at all.  The host is x86-64: this=rdi, the manager=rsi, the
# node=rdx, the event type=ecx (the fourth integer argument) and the dt=xmm0.
if WHICH == '6394':
    FORCE = [(_label, _where, [], ['$ecx = 0'], 1, _what)]
elif WHICH == 'EFA4':
    FORCE = [(_label, _where, [], ['$ecx = 0x16'], 1, _what)]
elif WHICH == 'EFA4M':
    FORCE = [(_label, _where, [], ['$ecx = 0x5'], 1, _what)]
elif WHICH == '6394FLAG':
    FORCE = [(_label, _where, [], ['$ecx = 0x16', '$this->mAutoDestructFlags = 0x10'], 1, _what)]
elif WHICH == '6394DATA':
    FORCE = [(_label, _where, [],
              ['$ecx = 0x16', '$this->mData.mUnknownBD80FD94 = 0x20000000'], 1, _what)]
elif WHICH == '6394NOPLAYER':
    FORCE = [(_label, _where, [], ['$ecx = 0x16', '$this->mSyncedId.value = 0xffff'], 1, _what)]
elif WHICH == '6394OTHER':
    FORCE = [(_label, _where, [], ['$ecx = 3'], 1, _what)]
elif WHICH == '1CA0D':
    FORCE = [(_label, _where, [], ['$this->mAlive = 0'], 1, _what)]
elif WHICH == '1CA0P':
    FORCE = [(_label, _where, [], ['$this->mSyncedId.value = 0xffff'], 1, _what)]
elif WHICH == '6394DEAD':
    # the head's projectile event (the type in r6) and the dead head: both at once, since the
    # fight never plays the projectile event, so the aim is only reached through the first.
    FORCE = [('6394 DoUserAnimEvent', 'CSplitterCommandModule::PortDoUserAnimEvent(',
              [], ['$ecx = 0'], 1, 'the event is a projectile one: the head shoots at the player'),
             ('1CA0 ProjectileAim', 'CSplitterCommandModule::PortHeadProjectileAim(',
              [], ['$this->mAlive = 0'], 1, 'the head is dead: the aim ends at once')]
elif WHICH == '6394NOPLAYER':
    FORCE = [('6394 DoUserAnimEvent', 'CSplitterCommandModule::PortDoUserAnimEvent(',
              [], ['$ecx = 0'], 1, 'the event is a projectile one: the head shoots at the player'),
             ('1CA0 ProjectileAim', 'CSplitterCommandModule::PortHeadProjectileAim(',
              [], ['$this->mSyncedId.value = 0xffff'], 1,
              'there is no player at 0xEFE: the aim ends at once')]
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
