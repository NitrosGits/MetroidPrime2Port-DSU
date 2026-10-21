# DarkSamus FSM part two, the second forcing run: the HeavyHit table entries 1..3, its no-entry and
# cannot-normalize arms, PhazonSuck's taunt re-delivery (CanIssueCommand true), the MissileDeflect
# arms, the phazon path states, and GetUp / Patrol (no FSM2 node: run with the console's statecall).
# Run: tools/cov/run.sh tools/cov/specs/darksamus_fsm2_force2.py <out> "1baa96c2,10_cliff_rooftop"
#      --con tools/cov/specs/darksamus_fsm2_force2.con --fast --frames 5300
CLASS = 'CDarkSamus'
import os
_SRC = os.path.join(os.path.dirname(os.path.abspath(os.environ['COV_SPEC'])), '..', '..', '..',
                    'src/MetroidPrime/Enemies/PortCDarkSamusFsm2.cpp')
_LINES = open(_SRC).read().split('\n')


def line_of(text, exact=False):  # the MissileDeflect decisions as line breakpoints (by their text)
    return 'loc:PortCDarkSamusFsm2.cpp:%d' % (
        1 + next(i for i, l in enumerate(_LINES) if (l == text if exact else text in l)))


XF = '$this->mTransform'
HD = '$this->mHitDirectionFlat'
# HeavyHit: Activate k = 1..3 aims the hit direction at the table entry k (-forward, -right,
# right), k = 4 straight up (no entry hits), k = 5 zeroes the flat forward and right of the
# transform (both cannot-normalize arms); DoStepCmd, the next call in the body, puts them back.
HH_K = '($rdx == 0 ? ($_isvoid($hk) ? ($hk = 0) : ($hk = $hk + 1)) : -1)'
HH_SET = ('($rdx != 0 ? 0 : ($hk == 1 ? ({hd}.mX = -{xf}.m01, {hd}.mY = -{xf}.m11, {hd}.mZ = 0) : '
          '($hk == 2 ? ({hd}.mX = -{xf}.m00, {hd}.mY = -{xf}.m10, {hd}.mZ = 0) : '
          '($hk == 3 ? ({hd}.mX = {xf}.m00, {hd}.mY = {xf}.m10, {hd}.mZ = 0) : '
          '($hk == 4 ? ({hd}.mX = 0, {hd}.mY = 0, {hd}.mZ = 1) : '
          '($hk == 5 ? ($s00 = {xf}.m00, $s01 = {xf}.m01, $s10 = {xf}.m10, $s11 = {xf}.m11, '
          '{xf}.m00 = 0, {xf}.m01 = 0, {xf}.m10 = 0, {xf}.m11 = 0, $hr = 1) : 0))))))'
          ).format(hd=HD, xf=XF)
HH_RESTORE = ('($_isvoid($hr) ? 0 : ($hr == 1 ? ({xf}.m00 = $s00, {xf}.m01 = $s01, '
              '{xf}.m10 = $s10, {xf}.m11 = $s11, $hr = 2) : 0))').format(xf=XF)
# PhazonSuck ($ps is gdb's eflags alias): $dps is 1 while the state runs; its third activation's first CanIssueCommand is
# forced true (the animation state Ready, the asked state one the body is not in).
PS_K = ('($rdx == 0 ? ($dps = 1, $dpa = ($_isvoid($dpa) ? 0 : $dpa + 1)) : '
        '($rdx == 2 ? ($dps = 0) : ($_isvoid($dps) ? 0 : $dps)))')
PS_IN = '($_isvoid($dps) ? 0 : $dps)'
CI_SET = ('($_isvoid($dps) ? 0 : ($dps != 1 ? 0 : ($dpa != 2 ? 0 : ($_isvoid($dpf) ? '
          '($dpf = 1, *(int*)$rdi = 1, $rdx = 0x7f) : 0))))')
# MissileDeflect: per activation the Update count $mk; Update 3 puts 0xC04 4+ s in the past (the
# time arm), Update 6 at the elapsed time (neither arm until the .con's shot comes within 28).
MD_K = '($rdx == 0 ? ($mk = 0) : ($rdx == 1 ? ($mk = $mk + 1) : $mk))'
MD_SET = ('($rdx != 1 ? 0 : ($mk == 3 ? ($this->mGuestC04 = $this->mElapsedTime - 10) : '
          '($mk == 6 ? ($this->mGuestC04 = $this->mElapsedTime) : 0)))')
MD_KEY = '($mk <= 30 ? $mk : 99)'
SPEC = [
    ('10C9C HeavyHit', 'CDarkSamus::HeavyHit(', [HH_K, HH_SET, '$rdx', '$this->mGuestC1C'],
     'calls max=3000'),
    ('DoStepCmd (restore)', 'CDarkSamus::DoStepCmd(',
     [HH_RESTORE, '($_isvoid($hk) ? -1 : $hk)', '$rsi', '$this->mGuestC1C'], 'max=3000'),
    ('4AC8 PhazonSuck', 'CDarkSamus::PhazonSuck(', [PS_K, '$rdx'], 'calls max=3000'),
    ('8698 RunTauntCmd', 'CDarkSamus::RunTauntCmd(', [PS_IN, '$rsi', '$rdx'], 'max=3000'),
    ('CanIssueCommand|ret', 'CAnimationState::CanIssueCommand(', [CI_SET, PS_IN, '$rdx'],
     'ret max=5000'),
    ('178E4 MissileDeflect', 'CDarkSamus::MissileDeflect(',
     [MD_K, MD_SET, MD_KEY, '$rdx', '$this->mGuestC04 != 0',
      '$this->mGuestC04 + 4 < $this->mElapsedTime', '$this->mGuestBFC80'], 'calls max=3000'),
    ('178E4 Activate result (mk, 0xBFC bit)', line_of('mGuestC04 = 0.f;                                          // 0x00017978'),
     ['$mk', '(int)this->mGuestBFC80'], 'max=100'),
    ('178E4 Update clears 0xC04 (mk; not the time-scale line: Deactivate shares it)', line_of('        mGuestC04 = 0.f;', exact=True), ['$mk'], 'max=100'),
    ('1BC58 distance (mk, dist)', line_of('if (dist < best) {'), ['$mk', '(int)dist'], 'max=100'),
    ('6460 GoToPhazonEnrage', 'CDarkSamus::GoToPhazonEnrage(', ['$rdx'], 'calls max=3000'),
    ('652C GoToPhazonAttack', 'CDarkSamus::GoToPhazonAttack(', ['$rdx'], 'calls max=3000'),
    ('65CC GetPhazonWaypoint', 'CDarkSamus::GetPhazonWaypoint(', ['$rdx'], 'calls max=3000'),
    ('6374 FindPathToWaypoint', 'CDarkSamus::FindPathToWaypoint(', ['$rdx'], 'calls max=3000'),
    ('7054 PhazonRise', 'CDarkSamus::PhazonRise(', ['$rdx', '$this->mGuest1044'],
     'calls max=3000'),
    ('18EAC GetUp', 'CDarkSamus::GetUp(', ['$rdx'], 'calls'),
    ('1142C Patrol', 'CDarkSamus::Patrol(', ['$rdx', '$this->mTransform.m03',
                                             '$this->mTransform.m13'], 'calls'),
]
