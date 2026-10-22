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
