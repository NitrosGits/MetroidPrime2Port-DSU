# Forcing spec (tools/cov/force.sh --driver, 06_sand_alamofront) for Captured 5610, CoverNearlyBlown
# A988 and CoveringFire A8D0. (gdb convenience variables only - not $fs/$gs, which are x86 registers.)
#   Captured, one Update: body state id = 0 (kAS_Fall, not kAS_Generate) and 0x6b4 = 1 (kAS_Ready) ->
#     CanIssueCommand(kAS_Generate) true, the generate command (asm 0x576C-0x57A8).
#   CoverNearlyBlown, one call on a pirate holding a cover point (0x900 valid): the attack target
#     (0xa94) set to the invalid id -> no CPlayer, the trigger stays false (asm 0xA9F0 `beq`). Restored
#     at the next CoverBlown entry.
#   CoveringFire, one call: this pirate's area id (CEntity +0x4) set to 99 -> every in-attack pirate
#     is in another area (asm 0xA944-0xA950). Restored at the next CoverBlown entry.
#   Crouch 98D8 and WallHang 69AC, one Activate each: 0x900 = u0531, the 06_sand_alamofront L2 cover
#     point 001d01e9 (`objs cover`) -> the cover arms (asm 0x9918 / 0x69F0). The fights' forced
#     Crouch/WallHang never held a cover point (CoverFind takes one only in Cover/TargetCover paths).
# Run (2026-10-21, all five forces hit; CoveringFire is only reached in some runs, rerun if its
# forced.log line is missing):
#   SP_FORCE=Captured SP_STEPS='wait 15; fsm {#173} Crouch; wait 3; fsm {#173} WallHang; wait 5;
#   fsm {#189} LaunchGrenade; wait 4; deleteteams; fsm {#431} LaunchGrenade; wait 4'
#   tools/cov/force.sh <this> <out> 42b935e4,06_sand_alamofront --driver
#   'python3 tools/drivers/spacepirate_drive.py {port} 2 3' --frames 12000
CLASS = 'CSpacePirate'
BS = '$this->mBodyController.mPtr->mBodyStateInfo.mState'
FORCE = [
    ('5610 Captured', 'CSpacePirate::Captured(', ['$rdx', '$this->mAnimationState.mState', BS],
     ['$ca = $_isvoid($ca) ? 0 : $ca',
      '$ca_a = ($ca == 0 && $rdx == 1) ? 1 : 0',
      BS + ' = $ca_a ? 0 : ' + BS,
      '$this->mAnimationState.mState = $ca_a ? 1 : $this->mAnimationState.mState',
      '$ca = $ca_a ? 1 : $ca'],
     0, 'body state id = 0 and 0x6b4 = kAS_Ready on one Update'),
    ('A988 CoverNearlyBlown', 'CSpacePirate::CoverNearlyBlown(', ['$this->x900_coverPoint.value != 0xffff'],
     ['$cn = $_isvoid($cn) ? 0 : $cn',
      '$cn_a = ($cn == 0 && $this->x900_coverPoint.value != 0xffff) ? 1 : 0',
      '$cn_p = $cn_a ? $this : ($_isvoid($cn_p) ? 0 : $cn_p)',
      '$cn_old = $cn_a ? $this->mAttackTarget.value : ($_isvoid($cn_old) ? 0 : $cn_old)',
      '$this->mAttackTarget.value = $cn_a ? 0xffff : $this->mAttackTarget.value',
      '$cn = $cn_a ? 1 : $cn'],
     0, 'attack target = invalid id on one call (restored at the next CoverBlown)'),
    ('A8D0 CoveringFire', 'CSpacePirate::CoveringFire(', ['$this->mAreaId.value'],
     ['$cf = $_isvoid($cf) ? 0 : $cf',
      '$cf_a = ($cf == 0) ? 1 : 0',
      '$cf_p = $cf_a ? $this : ($_isvoid($cf_p) ? 0 : $cf_p)',
      '$cf_old = $cf_a ? $this->mAreaId.value : ($_isvoid($cf_old) ? 0 : $cf_old)',
      '$this->mAreaId.value = $cf_a ? 99 : $this->mAreaId.value',
      '$cf = $cf_a ? 1 : $cf'],
     0, 'area id = 99 on one call (restored at the next CoverBlown)'),
    ('98D8 Crouch', 'CSpacePirate::Crouch(', ['$rdx', '$this->x900_coverPoint.value'],
     ['$cr = $_isvoid($cr) ? 0 : $cr',
      '$cr_a = ($cr == 0 && $rdx == 0) ? 1 : 0',
      '$this->x900_coverPoint.value = $cr_a ? 0x0531 : $this->x900_coverPoint.value',
      '$cr = $cr_a ? 1 : $cr'],
     0, '0x900 = u0531 (a cover point) on one Activate'),
    ('69AC WallHang', 'CSpacePirate::WallHang(', ['$rdx', '$this->x900_coverPoint.value'],
     ['$wh = $_isvoid($wh) ? 0 : $wh',
      '$wh_a = ($wh == 0 && $rdx == 0) ? 1 : 0',
      '$this->x900_coverPoint.value = $wh_a ? 0x0531 : $this->x900_coverPoint.value',
      '$wh = $wh_a ? 1 : $wh'],
     0, '0x900 = u0531 (a cover point) on one Activate'),
    ('AA64 CoverBlown (restores)', 'CSpacePirate::CoverBlown(', ['$rdx != 0'],
     ['$cn = $_isvoid($cn) ? 0 : $cn', '$cf = $_isvoid($cf) ? 0 : $cf',
      '$cn == 1 ? (((CSpacePirate*)$cn_p)->mAttackTarget.value = $cn_old) : 0',
      '$cn = $cn == 1 ? 2 : $cn',
      '$cf == 1 ? (((CSpacePirate*)$cf_p)->mAreaId.value = $cf_old) : 0',
      '$cf = $cf == 1 ? 2 : $cf'],
     0, 'restores the CoverNearlyBlown/CoveringFire writes (not a forced arm)'),
]
