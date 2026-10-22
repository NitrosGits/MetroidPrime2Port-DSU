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
# The two DoUserAnimEvent bodies (fn_75_6394 the head's, fn_75_EFA4 the chassis's) are forced
# with the event's type in the register: the host is x86-64, so this=rdi, the manager=rsi, the
# node=rdx, the event's type=ecx (the fourth integer argument) and the dt=xmm0.  The members the
# arms read are written through $this at the body's entry (the members are read after the entry,
# unlike the registers of a copied argument).  The chassis's own members are written through an
# explicit cast, since this spec's CLASS is the head's class.
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
# the chassis's own members: this spec's CLASS is the head's class, so the chassis is cast here.
CH = '((CSplitterMainChassis*)$rdi)'
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
    '32F4': ('32F4 UpdateFlashAndDeath', 'CSplitterCommandModule::PortUpdateFlashAndDeath(',
             'the death timer at 0xEF0 is above zero: the module does not die yet'),
    '1188': ('1188 SpawnBeamEffect', 'CSplitterCommandModule::PortSpawnBeamEffect(',
             'the detection range at 0x3D0 is not above zero: no beam effect is made'),
    'D61C': ('D61C Dodge', 'CSplitterMainChassis::Dodge(',
             'the dodge direction is 5 and there is no head: the shield is not raised'),
    '6394LOCK': ('6394 DoUserAnimEvent', 'CSplitterCommandModule::PortDoUserAnimEvent(',
                 'the event is a break-lock one and the flags are clear: the orbit request'),
    '6394P0': ('6394 DoUserAnimEvent', 'CSplitterCommandModule::PortDoUserAnimEvent(',
               'the event is a projectile one: the head shoots at the player'),
    '6394P0NP': ('6394 DoUserAnimEvent', 'CSplitterCommandModule::PortDoUserAnimEvent(',
                 'the event is a projectile one and there is no player at 0xEFE'),
    'EFA4LOCK': ('EFA4 DoUserAnimEvent', 'CSplitterMainChassis::PortDoUserAnimEvent(',
                 'the event is a break-lock one and the flag byte is clear: the orbit requests'),
    'EFA4FLAG': ('EFA4 DoUserAnimEvent', 'CSplitterMainChassis::PortDoUserAnimEvent(',
                 'the flag byte at 0xD41 has its 0x10 bit: no orbit request'),
    'EFA4NOPLAYER': ('EFA4 DoUserAnimEvent', 'CSplitterMainChassis::PortDoUserAnimEvent(',
                     'there is no player at 0xCC0: no orbit request'),
    'EFA4MAT': ('EFA4 DoUserAnimEvent', 'CSplitterMainChassis::PortDoUserAnimEvent(',
                'the event is a material one and the flag byte has its 0x80 bit: the reflected'
                ' vulnerability'),
    'EFA4MATCLEAR': ('EFA4 DoUserAnimEvent', 'CSplitterMainChassis::PortDoUserAnimEvent(',
                     'the event is a material one and the flag byte is clear: nothing happens'),
    'EFA4OTHER': ('EFA4 DoUserAnimEvent', 'CSplitterMainChassis::PortDoUserAnimEvent(',
                  'another event type: the base class is asked and nothing else happens'),
    '32F4D': ('32F4 UpdateFlashAndDeath', 'CSplitterCommandModule::PortUpdateFlashAndDeath(',
              'the death timer at 0xEF0 is at or below zero: the module dies'),
    'E87C': ('E87C SetIngPossessed', 'CSplitterMainChassis::SetIngPossessed(bool, float,',
             'the possession flag is set: the command module is told it is possessed'),
    'E87C2': ('E87C SetIngPossessed', 'CSplitterMainChassis::SetIngPossessed(bool, float,',
              'the possession flag is set and the command module is the head u0520'),
    'E87CN': ('E87C SetIngPossessed', 'CSplitterMainChassis::SetIngPossessed(bool, float,',
              'the possession flag is set and the command module keeps its own id'),
    '32F4DEAD': ('32F4 UpdateFlashAndDeath', 'CSplitterCommandModule::PortUpdateFlashAndDeath(',
                 'the head is dead: the death timer is not counted'),
    '32F4F': ('32F4 UpdateFlashAndDeath', 'CSplitterCommandModule::PortUpdateFlashAndDeath(',
              'the flash weight at 0xF08 is above one: the weight is capped'),
    'D61C': ('D61C Dodge', 'CSplitterMainChassis::Dodge(',
             'the dodge direction at 0xD04 is 5 and the head at 0xC6C (u0531) exists'),
    'D61CN': ('D61C Dodge', 'CSplitterMainChassis::Dodge(',
              'the dodge direction at 0xD04 is 5 and there is no head at 0xC6C'),
    'SEPHEAD': ('23F0 SeparationVector', 'CSplitterCommandModule::PortSeparationVector(',
                'the head\'s area id is not the other actors\': the other-area arm'),
    'SEPC': ('8EC8 SeparationVector', 'CSplitterMainChassis::PortSeparationVector(',
             'the chassis\'s area id is not the other actors\': the other-area arm'),
    'D61CP': ('D61C Dodge', 'CSplitterMainChassis::Dodge(',
              'the Dodge state is entered with the console, nothing is written'),
    'D61CX': ('D61C Dodge', 'CSplitterMainChassis::Dodge(',
              'the message is 3: no case of the switch is taken'),
    'EFA4MATN': ('EFA4 DoUserAnimEvent', 'CSplitterMainChassis::PortDoUserAnimEvent(',
                 'the material event and there is no head at 0xC6C'),
    'E87CN': ('E87C SetIngPossessed', 'CSplitterMainChassis::SetIngPossessed(bool, float,',
              'the possession flag is set and there is no head at 0xC6C'),
    'LOOKAT': ('184C LookAtTarget', 'CSplitterCommandModule::PortLookAtTarget(',
               'the target at 0xF02 is the chassis: the cast to a CBeamProjectile fails'),
    'LOOKATN': ('184C LookAtTarget', 'CSplitterCommandModule::PortLookAtTarget(',
                'there is no target at 0xF02: the body ends at once'),
    'DESPAWN': ('1790 DespawnProjectile', 'CSplitterCommandModule::PortDespawnProjectile(',
                'the base is dead: the effect states do not update'),
    'PATHZ': ('986C UpdatePath', 'CSplitterMainChassis::PortUpdatePath(',
              'the chassis is above the other actors: the separation\'s z is not zero'),
    'AIMNC': ('5F8C GetAimPosition', 'CSplitterCommandModule::GetAimPosition(',
              'the shield state is 2 and the head is not docked: there is no chassis'),
    'AIMC': ('5F8C GetAimPosition', 'CSplitterCommandModule::GetAimPosition(',
             'the shield state is not 2 and the timer at 0xF1C is below one: the chassis\'s aim'
             ' is mixed in'),
    'AIMNC2': ('5F8C GetAimPosition', 'CSplitterCommandModule::GetAimPosition(',
               'the shield state is not 2 and the head is not docked: there is no chassis'),
    'COLLIDNM': ('6178 CollidedWith', 'CSplitterCommandModule::CollidedWith(',
                 'the material list is empty: no collision matches'),
    'COLID': ('6178 CollidedWith', 'CSplitterCommandModule::CollidedWith(',
              'the collided id is not invalid: the body ends'),
    'COLFLAG': ('6178 CollidedWith', 'CSplitterCommandModule::CollidedWith(',
                'the flag byte at 0xF6A already has its 0x01 bit: the body ends'),
    'DAMAGE7': ('EBF4 KnockBack', 'CStateManager::ApplyKnockBack(',
                'the damage is 25 with the boost ball (the weapon type 7): the chassis is'
                ' knocked back'),
    'DAMAGE0': ('EBF4 KnockBack', 'CStateManager::ApplyKnockBack(',
                'the damage is 25 with the power beam (the weapon type 0): the chassis is'
                ' knocked back'),
    'KNOCKFLAG': ('EBF4 KnockBack', 'CSplitterMainChassis::KnockBack(',
                  'the flag byte at 0xD41 has its 0x80 bit: both reactions are turned off'),
    'SHIELD1': ('2BE0 GetShieldEffect', 'CSplitterCommandModule::PortGetShieldEffect(',
                'the head\'s state at 0xEE0 is 1: the LightShield effect'),
    'SHIELD2': ('2BE0 GetShieldEffect', 'CSplitterCommandModule::PortGetShieldEffect(',
                'the head\'s state at 0xEE0 is 2: the DarkShield effect'),
    'LANDED0': ('F5F8 ctor', 'loc:PortCSplitterMainChassis.cpp:1901',
                'the data\'s flag word at 0x68 has its 0x1 bit: the landed count at 0xD00 is 0'),
    'NULLCAST': ('9AF8 SetCollisionVulnerability', 'loc:PortCSplitterMainChassis.cpp:683',
                 'the collision actor\'s id is invalid: the cast to a CCollisionActor fails'),
    'MOVE': ('2154 MoveTo', 'CSplitterCommandModule::PortMoveTo(',
             'the timer at 0xEF4 is above zero: the head is teleported to the destination'),
    'RELO': ('E768 ReleaseDock', 'CSplitterCommandModule::PortFindChassis(',
             'the head has an old pick: the chassis is asked to release the reservation'),
    'RELO2': ('E768 ReleaseDock', 'CSplitterCommandModule::PortFindChassis(',
              'the old pick is the chassis the head reserved: the reservation is cleared'),
    'RELO3': ('E768 ReleaseDock', 'CSplitterCommandModule::PortFindChassis(',
              'the old pick\'s reservation is another head\'s: nothing is cleared'),
    'RESA': ('E788 ReserveDock', 'CSplitterMainChassis::PortReserveDock(',
             'the chassis is dead: the head is not reserved'),
    'RESB': ('E788 ReserveDock', 'CSplitterMainChassis::PortReserveDock(',
             'the chassis is not active: the head is not reserved'),
    'RESC': ('E788 ReserveDock', 'CSplitterMainChassis::PortReserveDock(',
             'the chassis has no head: the head is reserved'),
    'RESD': ('E788 ReserveDock', 'CSplitterMainChassis::PortReserveDock(',
             'the chassis has a reservation: the head is not reserved'),
    'RESE1': ('E788 ReserveDock', 'CSplitterMainChassis::PortReserveDock(',
              'the chassis has landed 5 times: the head is reserved'),
    'RESE2': ('E788 ReserveDock', 'CSplitterMainChassis::PortReserveDock(',
              'the chassis has landed 0 times: the head is reserved'),
    'RESE3': ('E788 ReserveDock', 'CSplitterMainChassis::PortReserveDock(',
              'the chassis has landed another number of times: the head is not reserved'),
    'RESF': ('E788 ReserveDock', 'CSplitterMainChassis::PortReserveDock(',
             'the flag byte at 0xD41 has its 0x80 bit: the head is not reserved'),
    'RESG': ('E788 ReserveDock', 'CSplitterMainChassis::PortReserveDock(',
             'the flag byte at 0xD40 has its 0x01 bit: the head is not reserved'),
    'DEATHT': ('AFE8 UpdateTimers', 'CSplitterMainChassis::PortUpdateTimers(',
               'the flag byte at 0xD41 has its 0x01 bit and the timer at 0xD20 is zero: the'
               ' chassis dies'),
    'DEATHALIVE': ('AFE8 UpdateTimers', 'CSplitterMainChassis::PortUpdateTimers(',
                   'the chassis is dead: the timer at 0xD20 is not counted down'),
    'TIMERHI': ('AFE8 UpdateTimers', 'CSplitterMainChassis::PortUpdateTimers(',
                'the timer at 0xD20 is above zero: the chassis does not die yet'),
    'TURNX': ('2994 TurnUpright', 'CSplitterCommandModule::PortTurnUpright(',
              'the head\'s Y column has an x: the head looks along its flat Y column'),
    'TURNY': ('2994 TurnUpright', 'CSplitterCommandModule::PortTurnUpright(',
              'the head\'s Y column has a y: the head looks along its flat Y column'),
    'FINDAREA': ('2280 FindChassis', 'CSplitterCommandModule::PortFindChassis(',
                 'the head\'s area id is not the chassis\'s: the other-area arm'),
    'FINDNORES': ('2280 FindChassis', 'CSplitterMainChassis::PortReserveDock(',
                  'the chassis is dead: the reserve fails'),
    'FINDDIST': ('2280 FindChassis', 'CSplitterCommandModule::PortFindChassis(',
                 'the head is far away: the chassis is not nearer than the best'),
    'TURNZERO2': ('2994 TurnUpright', 'CSplitterCommandModule::PortTurnUpright(',
                  'the head\'s Y column has no x and no y and its Z column is the up vector:'
                  ' the head only turns'),
    'D3C': ('AFE8 UpdateTimers', 'CSplitterMainChassis::PortUpdateTimers(',
            'the id at 0xCC0 is valid and the counter at 0xC48 is not zero: the timer at 0xD3C'
            ' is counted up'),
    'TURNCLEAR': ('2994 TurnUpright', 'CSplitterCommandModule::PortTurnUpright(',
                  'the flag byte at 0xF6A has no 0x20 bit: the head does not turn upright'),
    'TURNSLERP': ('2994 TurnUpright', 'CSplitterCommandModule::PortTurnUpright(',
                  'the head\'s Z column points away from the up vector: the head is slerped'),
    'TURNZERO': ('2994 TurnUpright', 'CSplitterCommandModule::PortTurnUpright(',
                 'the head\'s Y column has no x and no y: the head only turns'),
    'VULN7': ('EBF4 KnockBack', 'CStateManager::ApplyKnockBack(',
              'the boost ball (the weapon type 7) knocks the chassis back'),
    'VULN6': ('EBF4 KnockBack', 'CStateManager::ApplyKnockBack(',
              'another weapon type knocks the chassis back: the knock back reaction is off'),
    'VULNA': ('EBF4 KnockBack', 'CSplitterMainChassis::KnockBack(',
              'the flag byte at 0xD41 has its 0x80 bit: both reactions are turned off'),
    # The combined runs: each body's arms in one run, since the bodies are different.
    'RUN1': ('run 1', 'CSplitterCommandModule::CollidedWith(',
             'the damage with the boost ball, the collided id and the empty material list'),
    'RUN2': ('run 2', 'CSplitterCommandModule::CollidedWith(',
             'the damage with the boost ball and the flag byte already set'),
    'RUN3': ('run 3', 'CSplitterMainChassis::KnockBack(',
             'the damage with the boost ball and the flag byte at 0xD41'),
}
# the head's own members, written through $this (the spec's CLASS is the head's class).
HEAD = '((CSplitterCommandModule*)$rdi)'
# the head's flag byte at 0xF6A and the data's flag word at 0x7C0 are cleared, so the orbit
# request's arm runs; the head's id at 0xEFE is the player.
# the head's flag byte at 0xF6A (its 0x10 bit) and the data's flag word at 0x7C0 (its 0x4 bit)
# must be SET for the orbit request (asm 0x63E4, 0x63F0: the branch skips when the bit is clear).
LOCK = ['$ecx = 0x16', '$this->mAutoDestructFlags = 0x10', '$this->mData.mUnknownBD80FD94 = 0x4']
if WHICH in ('6394DEAD', '6394NOPLAYER'):
    _label, _where, _what = BODIES['6394']
else:
    _label, _where, _what = BODIES[WHICH]
FORCE = [
    (_label, _where, [], ['$this->mAlive = 0'], 1, _what),
]
# The event's type is written in ecx (the fourth integer argument) and the members the arms read
# are written through $this in the same stop.
if WHICH == '6394':
    FORCE = [(_label, _where, ['$ecx'], ['$ecx = 0'], 1, _what)]
elif WHICH == '6394P0':
    FORCE = [(_label, _where, ['$ecx'], ['$ecx = 0'], 1, _what)]
elif WHICH == 'EFA4':
    FORCE = [(_label, _where, ['$ecx'], ['$ecx = 0x16'], 1, _what)]
elif WHICH == 'EFA4M':
    FORCE = [(_label, _where, ['$ecx'], ['$ecx = 0x5'], 1, _what)]
elif WHICH == '6394FLAG':
    FORCE = [(_label, _where, ['$ecx'],
              ['$ecx = 0x16', '$this->mAutoDestructFlags = 0',
               '$this->mData.mUnknownBD80FD94 = 0x4'], 1, _what)]
elif WHICH == '6394DATA':
    FORCE = [(_label, _where, ['$ecx'],
              ['$ecx = 0x16', '$this->mAutoDestructFlags = 0x10',
               '$this->mData.mUnknownBD80FD94 = 0'], 1, _what)]
elif WHICH == '6394NOPLAYER':
    FORCE = [(_label, _where, ['$ecx'],
              ['$ecx = 0x16', '$this->mAutoDestructFlags = 0x10',
               '$this->mData.mUnknownBD80FD94 = 0x4', '$this->mSyncedId.value = 0xffff'], 1, _what)]
elif WHICH == '6394P0NP':
    FORCE = [('6394 DoUserAnimEvent', 'CSplitterCommandModule::PortDoUserAnimEvent(',
              ['$ecx'], ['$ecx = 0'], 1,
              'the event is a projectile one: the head shoots at the player'),
             ('1CA0 ProjectileAim', 'CSplitterCommandModule::PortHeadProjectileAim(',
              [], ['$this->mSyncedId.value = 0xffff'], 1,
              'there is no player at 0xEFE: the aim ends at once')]
elif WHICH == '6394OTHER':
    FORCE = [(_label, _where, ['$ecx'], ['$ecx = 3'], 1, _what)]
elif WHICH == '1CA0D':
    FORCE = [(_label, _where, [], ['$this->mAlive = 0'], 1, _what)]
elif WHICH == '1CA0P':
    FORCE = [(_label, _where, [], ['$this->mSyncedId.value = 0xffff'], 1, _what)]
elif WHICH == '6394DEAD':
    # the head's projectile event (the type in ecx) and the dead head: both at once, since the
    # fight never plays the projectile event, so the aim is only reached through the first.
    FORCE = [('6394 DoUserAnimEvent', 'CSplitterCommandModule::PortDoUserAnimEvent(',
              ['$ecx'], ['$ecx = 0'], 1,
              'the event is a projectile one: the head shoots at the player'),
             ('1CA0 ProjectileAim', 'CSplitterCommandModule::PortHeadProjectileAim(',
              [], ['$this->mAlive = 0'], 1, 'the head is dead: the aim ends at once')]
elif WHICH == '6394LOCK':
    # the break-lock event (the type 0x16) with the flags clear and the player at 0xEFE: the
    # head asks the player for the orbit.
    FORCE = [(_label, _where, ['$ecx'], LOCK, 1, _what)]
elif WHICH == 'EFA4LOCK':
    # the chassis's break-lock event with the flag byte at 0xD41's 0x10 bit set: the two orbit
    # requests (asm 0xEFE8, 0xEFF0: the branch skips when the bit is clear).
    # the flag byte at 0xD41 has its 0x10 bit clear: the base class's call.
    FORCE = [(_label, _where, ['$ecx'], ['$ecx = 0x16', CH + '->mFlagsD41 = 0'], 0, _what)]
elif WHICH == 'EFA4FLAG':
    FORCE = [(_label, _where, ['$ecx'], ['$ecx = 0x16', CH + '->mFlagsD41 = 0x10'], 0, _what)]
elif WHICH == 'EFA4NOPLAYER':
    FORCE = [(_label, _where, ['$ecx'],
              ['$ecx = 0x16', CH + '->mFlagsD41 = 0x10', CH + '->mSyncId.value = 0xffff'], 0, _what)]
elif WHICH == 'EFA4MAT':
    # the material event with the flag byte's 0x80 bit set: the reflected vulnerability and the
    # head at 0xC6C (u0531) resetting its shield (asm 0xF054, 0xF084).
    FORCE = [(_label, _where, ['$ecx'],
              ['$ecx = 0x5', CH + '->mFlagsD41 = 0x80', CH + '->mCommandModuleId.value = 0x531'],
              0, _what)]
elif WHICH == 'EFA4MATCLEAR':
    FORCE = [(_label, _where, ['$ecx'], ['$ecx = 0x5', CH + '->mFlagsD41 = 0'], 0, _what)]
elif WHICH == '32F4':
    # the flash weight at 0xF08 above zero, the flag byte's 0x04 bit and the death timer above
    # zero: the module counts down but does not die.
    FORCE = [(_label, _where, [],
              ['$this->mAutoDestructFlags = 4', '$this->mFlashAlpha = 0.001f',
               '$this->mDeathTimer = 100.f'], 1, _what)]
elif WHICH == '32F4D':
    # the same, but the death timer at or below zero: the module dies.
    FORCE = [(_label, _where, [],
              ['$this->mAutoDestructFlags = 4', '$this->mFlashAlpha = 0.001f',
               '$this->mDeathTimer = -1.f'], 1, _what)]
elif WHICH == 'D61CX':
    # the message is 3 (the third argument, in rdx): no case of the switch is taken
    # (asm 0xD650, 0xD654).
    FORCE = [(_label, _where, [], ['$rdx = 3'], 0, _what)]
elif WHICH == 'EFA4MATN':
    # the material event with the flag byte's 0x80 bit set and no head at 0xC6C: the shield
    # reset is skipped (asm 0xF09C, 0xF0A0).
    FORCE = [(_label, _where, ['$ecx'],
              ['$ecx = 0x5', CH + '->mFlagsD41 = 0x80', CH + '->mCommandModuleId.value = 0xffff'],
              0, _what)]
elif WHICH == 'D61CP':
    FORCE = [(_label, _where, [], [], 0, _what)]
elif WHICH == 'D61C':
    # the chassis's Dodge state: the dodge direction at 0xD04 is 5 and the head at 0xC6C is the
    # head u0531, so the head raises its dodge shield (asm 0xD690, 0xD6A0).
    FORCE = [(_label, _where, [],
              [CH + '->mDodgeDirection = 5', CH + '->mCommandModuleId.value = 0x531'], 0, _what)]
elif WHICH == 'D61CN':
    # the same, but there is no head at 0xC6C: the cast fails (asm 0xD690).
    FORCE = [(_label, _where, [],
              [CH + '->mDodgeDirection = 5', CH + '->mCommandModuleId.value = 0xffff'], 0, _what)]
elif WHICH == 'SEPHEAD':
    # the head's area id is a value no other actor has: the "an AI in another area" arm of the
    # listening-AI walk (asm 0x2470-0x247C).
    FORCE = [(_label, _where, [], ['$this->mAreaId.value = 12'], 0, _what)]
elif WHICH == 'SEPC':
    FORCE = [(_label, _where, [], [CH + '->mAreaId.value = 12'], 0, _what)]
elif WHICH == 'E87C':
    # the possession flag (the second argument, in esi) is set and the command module's id at
    # 0xC6C is the head u0531's: the head is looked up and told it is possessed.
    FORCE = [(_label, _where, [], ['$esi = 1', CH + '->mCommandModuleId.value = 0x531'], 1, _what)]
elif WHICH == 'PATHZ':
    # The chassis's own position, so the separation from the other actors has a z component: the
    # port's fn_75_986C tests the separation's z only when its x and y are zero, so this force
    # alone does not reach that arm (the only actor above the chassis, the docked head, is
    # skipped by the walk) - it is kept as the attempt the note records.
    FORCE = [(_label, _where, [],
              [CH + '->mPosition.mZ = 2.0'], 0, _what)]
elif WHICH == 'AIMNC':
    # the shield state at 0xEDC is 2 and the head is not docked: the chassis lookup fails.
    FORCE = [(_label, _where, [],
              ['$this->mShieldState = 2', '$this->mChassisId.value = 0xffff'], 0, _what)]
elif WHICH == 'AIMC':
    # the shield state at 0xEDC is not 2 and the timer at 0xF1C is below one: the second branch
    # runs and finds the chassis.
    FORCE = [(_label, _where, [],
              ['$this->mShieldState = 1', '$this->mCountupTimerB = 0.5f'], 0, _what)]
elif WHICH == 'AIMNC2':
    # the shield state at 0xEDC is not 2, the timer at 0xF1C is below one and the head is not
    # docked: the second branch runs and finds no chassis.
    FORCE = [(_label, _where, [],
              ['$this->mShieldState = 1', '$this->mCountupTimerB = 0.5f',
               '$this->mChassisId.value = 0xffff'], 0, _what)]
elif WHICH == 'COLLIDNM':
    # the material list (the REL's lbl_75_bss_8) is empty: no collision matches.
    FORCE = [(_label, _where, [],
              ['sSplitterCollisionMaterialList = CMaterialList()'], 0, _what)]
elif WHICH == 'COLID':
    # the collided id (the second argument) is not the invalid one: the body ends.
    FORCE = [(_label, _where, [], ['((TUniqueId*)$rsi)->value = 0x1234'], 0, _what)]
elif WHICH == 'COLFLAG':
    # the flag byte at 0xF6A already has its 0x01 bit: the body ends.
    FORCE = [(_label, _where, [], ['$this->mAutoDestructFlags |= 0x1'], 0, _what)]
elif WHICH == 'DAMAGE7':
    # the damage info (the fourth argument of CStateManager::ApplyKnockBack): the boost ball.
    FORCE = [(_label, _where, [],
              ['((CDamageInfo*)$r8)->mWeaponMode.mWeaponType = 7'], 0, _what)]
elif WHICH == 'DAMAGE0':
    # the damage info: the missile (the weapon type 6), so the knock back reaction is off.
    FORCE = [(_label, _where, [],
              ['((CDamageInfo*)$r8)->mWeaponMode.mWeaponType = 6'], 0, _what)]
elif WHICH == 'KNOCKFLAG':
    # the flag byte at 0xD41 has its 0x80 bit: both reactions are turned off.
    FORCE = [(_label, _where, [], [CH + '->mFlagsD41 |= 0x80'], 0, _what)]
elif WHICH == 'RUN1':
    FORCE = [
        ('6178 CollidedWith', 'CSplitterCommandModule::CollidedWith(', [],
         ['((TUniqueId*)$rsi)->value = 0x1234'], 0,
         'the collided id is not invalid: the body ends'),
        ('6178 CollidedWith', 'CSplitterCommandModule::CollidedWith(', [],
         ['sSplitterCollisionMaterialList = CMaterialList()'], 0,
         'the material list is empty: no collision matches'),
    ]
elif WHICH == 'RUN2':
    FORCE = [
        ('EBF4 KnockBack', 'CStateManager::ApplyKnockBack(', [],
         ['((CDamageInfo*)$r8)->mWeaponMode.mWeaponType = 6'], 0,
         'the damage hurts but its weapon type is not 7: the knock back reaction is off'),
        ('6178 CollidedWith', 'CSplitterCommandModule::CollidedWith(', [],
         ['$this->mAutoDestructFlags |= 0x1'], 0,
         'the flag byte at 0xF6A already has its 0x01 bit: the body ends'),
    ]
elif WHICH == 'RUN3':
    FORCE = [
        ('EBF4 KnockBack', 'CSplitterMainChassis::KnockBack(', [],
         [CH + '->mFlagsD41 |= 0x80'], 0,
         'the flag byte at 0xD41 has its 0x80 bit: both reactions are turned off'),
    ]
elif WHICH == 'VULN7':
    # The damage info's weapon type is the boost ball (7) and the vulnerability (the fifth
    # argument) is the default one, so the damage reaches the chassis.
    FORCE = [(_label, _where, [],
              ['((CDamageVulnerability*)$r9)->mVulnerabilities['
               '((CDamageVulnerability*)$r9)->mWeaponIndices[7]].mDamageMultiplier = 1.0f',
               '((CDamageVulnerability*)$r9)->mVulnerabilities['
               '((CDamageVulnerability*)$r9)->mWeaponIndices[7]].mEffect = 0',
               '((CDamageInfo*)$r8)->mWeaponMode.mWeaponType = 7'], 0, _what)]
elif WHICH == 'VULN6':
    # The same, but the weapon type is the missile (6): the knock back reaction is turned off.
    FORCE = [(_label, _where, [],
              ['((CDamageVulnerability*)$r9)->mVulnerabilities['
               '((CDamageVulnerability*)$r9)->mWeaponIndices[6]].mDamageMultiplier = 1.0f',
               '((CDamageVulnerability*)$r9)->mVulnerabilities['
               '((CDamageVulnerability*)$r9)->mWeaponIndices[6]].mEffect = 0',
               '((CDamageInfo*)$r8)->mWeaponMode.mWeaponType = 6'], 0, _what)]
elif WHICH == 'VULNA':
    # The flag byte at 0xD41 has its 0x80 bit: both reactions are turned off.
    FORCE = [(_label, _where, [],
              [CH + '->mFlagsD41 |= 0x80'], 0, _what)]
elif WHICH == 'SHIELD1':
    # The head's state at 0xEE0 is 1: the LightShield particle effect (asm 0x2C08-0x2C34).
    FORCE = [(_label, _where, [], ['$this->mCollisionState = 1'], 0, _what)]
elif WHICH == 'SHIELD2':
    # The head's state at 0xEE0 is 2: any state but 1 gives the DarkShield effect
    # (asm 0x2C08-0x2C50).
    FORCE = [(_label, _where, [], ['$this->mCollisionState = 2'], 0, _what)]
elif WHICH == 'LANDED0':
    # The data's flag word at 0x68 has its 0x1 bit, so the counter at 0xD00 is 0 and not 5
    # (asm 0xF7EC-0xF7FC: lwz 0x68(r31); clrlwi. r0, r0, 31; beq; mr r3, r4).  The write is at the
    # line before the assignment (the initialiser list has already copied the data into mData).
    FORCE = [(_label, _where, [],
              ['this->mData.mUnknownCEF5C2FE |= 1'], 0, _what)]
elif WHICH == 'NULLCAST':
    # The collision actor's id is the invalid one, so the cast to a CCollisionActor fails and
    # the walk skips the actor (asm 0x9B44-0x9B4C).  The id is in r15 when the line is reached
    # (the build keeps it there: movzwl 0x48(%rax),%r15d; mov %r15d,%esi).
    FORCE = [(_label, _where, [], ['$r15 = 0xffff'], 0, _what)]
elif WHICH == 'FINDAREA':
    # The head's area id is a value the chassis does not have: the other-area arm of the walk
    # (asm 0x22F8-0x2304).
    FORCE = [(_label, _where, [], ['$this->mAreaId.value = 12'], 0, _what)]
elif WHICH == 'FINDNORES':
    # The chassis is dead: the reserve fails and the walk skips it (asm 0x2318-0x2320).
    FORCE = [(_label, _where, [], [CH + '->mAlive = 0'], 0, _what)]
elif WHICH == 'FINDDIST':
    # The head is far away: the chassis is not nearer than the best (asm 0x2324-0x234C).
    FORCE = [(_label, _where, [], ['$this->mTransform.m03 = 5000.f'], 0, _what)]
elif WHICH == 'TURNZERO2':
    # The head's Y column has no x and no y and its Z column is the up vector, so the angle is
    # below the turn and the head only turns (asm 0x2A38-0x2A7C).
    FORCE = [(_label, _where, [],
              ['$this->mTransform.m10 = 0.f', '$this->mTransform.m11 = 0.f',
               '$this->mTransform.m12 = 1.f', '$this->mTransform.m20 = 0.f',
               '$this->mTransform.m21 = 0.f', '$this->mTransform.m22 = 1.f'], 0, _what)]
elif WHICH == 'TIMERHI':
    # The flag byte at 0xD41 has its 0x01 bit and the timer at 0xD20 is above zero: the chassis
    # does not die yet (asm 0xB0B8, 0xB0C0, 0xB0C8).
    FORCE = [(_label, _where, [],
              [CH + '->mFlagsD41 |= 0x1', CH + '->mTimerD20 = 10.f'], 0, _what)]
elif WHICH == 'TURNX':
    # The head's Y column has an x (asm 0x2A48-0x2A7C): the head looks along it.
    FORCE = [(_label, _where, [],
              ['$this->mTransform.m10 = 1.f', '$this->mTransform.m11 = 0.f',
               '$this->mTransform.m12 = 0.f'], 0, _what)]
elif WHICH == 'TURNY':
    # The head's Y column has a y (asm 0x2A48-0x2A7C): the head looks along it.
    FORCE = [(_label, _where, [],
              ['$this->mTransform.m10 = 0.f', '$this->mTransform.m11 = 1.f',
               '$this->mTransform.m12 = 0.f'], 0, _what)]
elif WHICH == 'DEATHT':
    # The flag byte at 0xD41 has its 0x01 bit and the timer at 0xD20 is zero (the Digital
    # Guardian's DestroyActiveSplitters gives the first chassis the delay 0.0): the chassis dies
    # at the next timer update (asm 0xB094, 0xB0A0, 0xB0B8, 0xB0C0, 0xB0F0).
    FORCE = [(_label, _where, [],
              [CH + '->mFlagsD41 |= 0x1', CH + '->mTimerD20 = 0.f'], 0, _what)]
elif WHICH == 'DEATHALIVE':
    # The same, but the chassis is dead: the timer at 0xD20 is not counted down (asm 0xB0A0).
    FORCE = [(_label, _where, [],
              [CH + '->mFlagsD41 |= 0x1', CH + '->mTimerD20 = 0.f', CH + '->mAlive = 0'],
              0, _what)]
elif WHICH == 'D3C':
    # The id at 0xCC0 is valid and the counter at 0xC48 is not zero: the timer at 0xD3C is
    # counted up (asm 0xB060, 0xB064, 0xB06C, 0xB070, 0xB07C).
    FORCE = [(_label, _where, [],
              [CH + '->mSyncId.value = 0x521', CH + '->mIntC48 = 1'], 0, _what)]
elif WHICH == 'TURNCLEAR':
    # The flag byte at 0xF6A has no 0x20 bit: the body returns at once (asm 0x29BC-0x29C8).
    FORCE = [(_label, _where, [], ['$this->mAutoDestructFlags &= ~0x20'], 0, _what)]
elif WHICH == 'TURNSLERP':
    # The head's Z column points away from the up vector: the angle is above the turn and the
    # head is slerped (asm 0x2A38-0x2A44, 0x2B10, 0x2B38, 0x2B64, 0x2B74, 0x2BA4, 0x2BB0).
    FORCE = [(_label, _where, [],
              ['$this->mTransform.m20 = 0.f', '$this->mTransform.m21 = 0.f',
               '$this->mTransform.m22 = -1.f'], 0, _what)]
elif WHICH == 'TURNZERO':
    # The head's Y column has no x and no y: the head only turns (asm 0x2A48-0x2A7C).
    FORCE = [(_label, _where, [],
              ['$this->mTransform.m10 = 0.f', '$this->mTransform.m11 = 0.f',
               '$this->mTransform.m12 = 1.f'], 0, _what)]
elif WHICH == 'MOVE':
    # The timer at 0xEF4 is above zero: the head is teleported to the destination (asm 0x2180).
    FORCE = [(_label, _where, [], ['$this->mAutoDestructTimer = 10.0f'], 0, _what)]
elif WHICH == 'RELO':
    # The head's checked chassis id at 0xEFC is a live chassis (u0521): the search asks it to
    # release the reservation (the old pick's release, fn_75_E768).
    FORCE = [(_label, _where, [], ['$this->mCheckedChassisId.value = 0x521'], 0, _what)]
elif WHICH == 'RELO2':
    # The same, and the chassis the head just reserved is the old pick: the reservation is the
    # head's, so the release clears it (fn_75_E768's other arm).
    FORCE = [('E788 ReserveDock', 'CSplitterMainChassis::PortReserveDock(', [],
              [CH + '->mCommandModuleId.value = 0xffff'], 0,
              'the chassis has no head: the head is reserved'),
             (_label, _where, [], ['$this->mCheckedChassisId.value = 0x521'], 0, _what)]
elif WHICH == 'RELO3':
    FORCE = [('E788 ReserveDock', 'CSplitterMainChassis::PortReserveDock(', [],
              [CH + '->mCommandModuleId.value = 0xffff'], 0,
              'the chassis has no head: the head is reserved'),
             ('E768 ReleaseDock', 'CSplitterMainChassis::PortReleaseDock(', [],
              [CH + '->mSecondLinkId.value = 0x520'], 0,
              'the reservation is another head\'s: nothing is cleared'),
             (_label, _where, [], ['$this->mCheckedChassisId.value = 0x521'], 0, _what)]
elif WHICH in ('RESA', 'RESB', 'RESC', 'RESD', 'RESE1', 'RESE2', 'RESE3', 'RESF', 'RESG'):
    # The chassis's dock reserve (fn_75_E788): the conditions are a chain, so every run keeps the
    # earlier ones true (the head id at 0xC6C invalid, the reservation at 0xC6E invalid, the
    # landed count at 0xD00 5) and breaks the one the arm reads.
    sets = []
    if WHICH not in ('RESA', 'RESB'):
        sets.append(CH + '->mCommandModuleId.value = 0xffff')
    if WHICH == 'RESD':
        sets.append(CH + '->mSecondLinkId.value = 0x1234')
    if WHICH in ('RESE1', 'RESF', 'RESG'):
        sets.append(CH + '->mLandedCount = 5')
    elif WHICH == 'RESE2':
        sets.append(CH + '->mLandedCount = 0')
    elif WHICH == 'RESE3':
        sets.append(CH + '->mLandedCount = 3')
    if WHICH == 'RESA':
        sets.append(CH + '->mAlive = 0')
    elif WHICH == 'RESB':
        sets.append(CH + '->mActive = 0')
    elif WHICH == 'RESF':
        sets.append(CH + '->mFlagsD41 |= 0x80')
    elif WHICH == 'RESG':
        sets.append(CH + '->mFlagsD40 |= 0x1')
    FORCE = [(_label, _where, [], sets, 0, _what)]
elif WHICH == 'DESPAWN':
    # the base's alive flag (the byte at 0x420's 0x40 bit) is clear: the effect states do not
    # update (asm 0x1820).
    FORCE = [(_label, _where, [], ['$this->mAlive = 0'], 0, _what)]
elif WHICH == 'LOOKAT':
    # the target id at 0xF02 is the chassis u0532: the cast to a CBeamProjectile fails.
    FORCE = [(_label, _where, [], ['$this->mLookAtTargetId.value = 0x532'], 0, _what)]
elif WHICH == 'LOOKATN':
    # the target id at 0xF02 is invalid: the body ends at once.
    FORCE = [(_label, _where, [], ['$this->mLookAtTargetId.value = 0xffff'], 0, _what)]
elif WHICH == 'E87CN':
    FORCE = [(_label, _where, [], ['$esi = 1', CH + '->mCommandModuleId.value = 0xffff'], 1, _what)]
elif WHICH == 'E87C2':
    FORCE = [(_label, _where, [CH + '->mCommandModuleId.value'],
              ['$esi = 1', CH + '->mCommandModuleId.value = 0x520'], 0, _what)]
elif WHICH == 'E87CN':
    FORCE = [(_label, _where, [CH + '->mCommandModuleId.value'], ['$esi = 1'], 0, _what)]
elif WHICH == '32F4F':
    # the flash weight at 0xF08 above one: the countdown does not go below zero and the weight
    # is capped at 1.0f.
    FORCE = [(_label, _where, [],
              ['$this->mAutoDestructFlags = 4', '$this->mFlashAlpha = 5.f',
               '$this->mDeathTimer = 100.f'], 1, _what)]
elif WHICH == '32F4DEAD':
    # the same as 32F4, but the head is dead: the death timer is not counted.
    FORCE = [(_label, _where, [],
              ['$this->mAutoDestructFlags = 4', '$this->mFlashAlpha = 0.001f',
               '$this->mDeathTimer = 100.f', '$this->mAlive = 0'], 1, _what)]
elif WHICH == 'EFA4OTHER':
    FORCE = [(_label, _where, ['$ecx'], ['$ecx = 3'], 0, _what)]
elif WHICH == '1188':
    FORCE = [(_label, _where, [], [HEAD + '->mDetectionRange = 0.f'], 1, _what)]
elif WHICH == 'D61CX':
    # the message is 3 (the third argument, in rdx): no case of the switch is taken
    # (asm 0xD650, 0xD654).
    FORCE = [(_label, _where, [], ['$rdx = 3'], 0, _what)]
elif WHICH == 'EFA4MATN':
    # the material event with the flag byte's 0x80 bit set and no head at 0xC6C: the shield
    # reset is skipped (asm 0xF09C, 0xF0A0).
    FORCE = [(_label, _where, ['$ecx'],
              ['$ecx = 0x5', CH + '->mFlagsD41 = 0x80', CH + '->mCommandModuleId.value = 0xffff'],
              0, _what)]
elif WHICH == 'D61CP':
    FORCE = [(_label, _where, [], [], 0, _what)]
elif WHICH == 'D61C':
    FORCE = [(_label, _where, [],
              [CH + '->mDodgeDirection = 5', CH + '->mCommandModuleId.value = 0xffff'], 1, _what)]
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
