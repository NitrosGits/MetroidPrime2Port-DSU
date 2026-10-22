# Splitter (module 75) split22 lane: the chassis's and the head's range triggers, the chassis's
# Listen and knock-back, and the result keys of the no-branch triggers.  Run with
# COV_GDB_SCRIPT=tools/cov/force_ret.py and BUILD=build-port-cov (every write lasts one call):
#
#   COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/splitter_split22.py <out> 1baa96c2,914f1381 \
#     --con tools/cov/specs/splitter_split22.con --fast --frames 9000
#
# The period 40 repeats each phase every 40th call of the body (the triggers run every frame
# while the FSM tests them), so the forced arms are taken after the target is acquired too.
CLASS = 'CSplitterMainChassis'
HD = '((CSplitterCommandModule*)$this)'
P = 40
_LEG = [
    # fn_75_E2F0 ShouldMorphballStab / fn_75_E3A4 ShouldLegStab:
    # 5: the data's 0x4 bit clear (0xE308 / 0xE3BC; every shipped flag word has it);
    # 10: the timer 0xD10 above 0 (0xE324 / 0xE3D8); 15: the target id 0xCC0 invalid (0xE340 / 0xE3F4);
    # 20: min range 100 (true for E2F0; E3A4's first compare 0xE444 fails);
    # 25: min 0, max 100 (E2F0 0xE37C false; E3A4 true); 30: min 0, max 0 (E3A4 0xE450 fails).
    (5, [('$this->mData.mUnknownCEF5C2FE', '$this->mData.mUnknownCEF5C2FE & ~4')]),
    (10, [('$this->mLegStabTimer', '5.f')]),
    (15, [('$this->mLegStabTimer', '0.f'), ('$this->mSyncId.value', '0xffff')]),
    (20, [('$this->mLegStabTimer', '0.f'), ('$this->mData.mLegStabMinAttackRange', '100.f')]),
    (25, [('$this->mLegStabTimer', '0.f'), ('$this->mData.mLegStabMinAttackRange', '0.f'),
          ('$this->mData.mLegStabMaxAttackRange', '100.f')]),
    (30, [('$this->mLegStabTimer', '0.f'), ('$this->mData.mLegStabMinAttackRange', '0.f'),
          ('$this->mData.mLegStabMaxAttackRange', '0.f')]),
]
_SPIN = [
    # fn_75_E1D0 ShouldSpinAttack: 5: 0xD40's 0x80 clear (0xE1F4); 10: the timer 0xD14 above 0
    # (0xE20C); 15: in the team 0xCBE (its own id: not a team, CanStartAttack false, 0xE24C);
    # 20: the target 0xCC0 invalid (0xE268); 25: min 0, max 100 (true); 30: min 100 (0xE2B8 fails);
    # 35: min 0, max 0 (0xE2C4 fails).
    (5, [('$this->mFlagsD40', '$this->mFlagsD40 & 0x7f')]),
    (10, [('$this->mFlagsD40', '$this->mFlagsD40 | 0x80'), ('$this->mSpinPenaltyTimer', '5.f')]),
    (15, [('$this->mFlagsD40', '$this->mFlagsD40 | 0x80'), ('$this->mSpinPenaltyTimer', '0.f'),
          ('$this->mTeamId.value', '$this->mUniqueId.value')]),
    (20, [('$this->mFlagsD40', '$this->mFlagsD40 | 0x80'), ('$this->mSpinPenaltyTimer', '0.f'),
          ('$this->mSyncId.value', '0xffff')]),
    (25, [('$this->mFlagsD40', '$this->mFlagsD40 | 0x80'), ('$this->mSpinPenaltyTimer', '0.f'),
          ('$this->mData.mSpinAttackMinAttackRange', '0.f'),
          ('$this->mData.mSpinAttackMaxAttackRange', '100.f')]),
    (30, [('$this->mFlagsD40', '$this->mFlagsD40 | 0x80'), ('$this->mSpinPenaltyTimer', '0.f'),
          ('$this->mData.mSpinAttackMinAttackRange', '100.f')]),
    (35, [('$this->mFlagsD40', '$this->mFlagsD40 | 0x80'), ('$this->mSpinPenaltyTimer', '0.f'),
          ('$this->mData.mSpinAttackMinAttackRange', '0.f'),
          ('$this->mData.mSpinAttackMaxAttackRange', '0.f')]),
]
_PULSE = [
    # fn_75_565C InLaserPulseRange: 5: the target 0xEFE invalid (0x5688); 10: min 0, max 100
    # (true); 15: min 100 (0x56D8 fails); 20: min 0, max 0 (0x56E4 fails).
    (5, [(HD + '->mSyncedId.value', '0xffff')]),
    (10, [(HD + '->mData.mMinLaserPulseRange', '0.f'), (HD + '->mData.mMaxLaserPulseRange', '100.f')]),
    (15, [(HD + '->mData.mMinLaserPulseRange', '100.f')]),
    (20, [(HD + '->mData.mMinLaserPulseRange', '0.f'), (HD + '->mData.mMaxLaserPulseRange', '0.f')]),
]
_HOVER = [
    # fn_75_570C InHoverRange: 5: the target invalid (0x5738); 10: both ranges 100 (true);
    # 15: both 0 (false, 0x5788 cror lt,eq).
    (5, [(HD + '->mSyncedId.value', '0xffff')]),
    (10, [(HD + '->mData.mMinLaserPulseRange', '100.f'), (HD + '->mData.mMaxLaserPulseRange', '100.f')]),
    (15, [(HD + '->mData.mMinLaserPulseRange', '0.f'), (HD + '->mData.mMaxLaserPulseRange', '0.f')]),
]
FORCE_RET = [
    ('fn_75_E2F0', 'CSplitterMainChassis::ShouldMorphballStab(', P, _LEG, 'bit, timer, target, ranges'),
    ('fn_75_E3A4', 'CSplitterMainChassis::ShouldLegStab(', P, _LEG, 'bit, timer, target, ranges'),
    ('fn_75_E1D0', 'CSplitterMainChassis::ShouldSpinAttack(', P, _SPIN, 'bit, timer, team, target, ranges'),
    ('fn_75_565C', 'CSplitterCommandModule::InLaserPulseRange(', P, _PULSE, 'target, ranges'),
    ('fn_75_570C', 'CSplitterCommandModule::InHoverRange(', P, _HOVER, 'target, ranges'),
    # fn_75_EA70 Listen: every 3rd call the chassis is not alive (0xEA7C).
    ('fn_75_EA70', 'CSplitterMainChassis::Listen(', 3, [(1, [('$this->mAlive', '0')])], 'not alive'),
    # fn_75_EBF4 KnockBack: every 2nd call 0xD41's 0x80 bit is set (0xEC20).
    ('fn_75_EBF4', 'CSplitterMainChassis::KnockBack(', 2,
     [(1, [('$this->mFlagsD41', '$this->mFlagsD41 | 0x80')])], 'spin bit set'),
]
SPEC = [
    ('57B4 HasTarget', 'CSplitterCommandModule::HasTarget(', ['$rdi != 0'], 'ret'),
    ('58BC IsDocked', 'CSplitterCommandModule::IsDocked(', ['$rdi != 0'], 'ret'),
    ('58DC IsInitiallyDocked', 'CSplitterCommandModule::IsInitiallyDocked(', ['$rdi != 0'], 'ret'),
    ('565C InLaserPulseRange', 'CSplitterCommandModule::InLaserPulseRange(', ['$rdi != 0'], 'ret'),
    ('570C InHoverRange', 'CSplitterCommandModule::InHoverRange(', ['$rdi != 0'], 'ret'),
    ('DFEC PrepareForDocking', 'CSplitterMainChassis::PrepareForDocking(', ['$rdi != 0'], 'ret'),
    ('E034 DeployFromHoldingTube', 'CSplitterMainChassis::DeployFromHoldingTube(', ['$rdi != 0'], 'ret'),
    ('E5AC LostHead', 'CSplitterMainChassis::LostHead(', ['$rdi != 0'], 'ret'),
    ('E5B8 ShotAt', 'CSplitterMainChassis::ShotAt(', ['$rdi != 0'], 'ret'),
    ('E620 IsDisabled', 'CSplitterMainChassis::IsDisabled(', ['$rdi != 0'], 'ret'),
    ('E2F0 ShouldMorphballStab', 'CSplitterMainChassis::ShouldMorphballStab(', ['$rdi != 0'], 'ret'),
    ('E3A4 ShouldLegStab', 'CSplitterMainChassis::ShouldLegStab(', ['$rdi != 0'], 'ret'),
    ('E1D0 ShouldSpinAttack', 'CSplitterMainChassis::ShouldSpinAttack(', ['$rdi != 0'], 'ret'),
    ('EA70 Listen', 'CSplitterMainChassis::Listen(', ['$ecx'], 'ret'),
    ('EBF4 KnockBack', 'CSplitterMainChassis::KnockBack(', ['$rdi != 0'], 'calls'),
]
