# Forcing spec (tools/cov/force.sh --driver, 06_sand_alamofront) for Captured 5610, CoverNearlyBlown
# A988 and CoveringFire A8D0. (gdb convenience variables only - not $fs/$gs, which are x86 registers.)
#   Captured, the 19th Update after the forced one: 0x6b4 = kAS_NotReady -> CanIssueCommand false
#     (asm 0x5790; this run's Captured issues every frame). One Update: body state id = 0 (kAS_Fall, not kAS_Generate) and 0x6b4 = 1 (kAS_Ready) ->
#     CanIssueCommand(kAS_Generate) true, the generate command (asm 0x576C-0x57A8).
#   CoverNearlyBlown, one call on a pirate holding a cover point (0x900 valid): the attack target
#     (0xa94) set to the invalid id -> no CPlayer, the trigger stays false (asm 0xA9F0 `beq`). Restored
#     at the next CoverBlown entry.
#   CoveringFire, one call: this pirate's area id (CEntity +0x4) set to 99 -> every in-attack pirate
#     is in another area (asm 0xA944-0xA950). Restored at the next CoverBlown entry.
#   Ambushing 7630, the second Activate (the first takes the floor arm): 0x8f4 bit 0x40 (x8f4_25_ceilingAmbush, data flag 0x2) set -> the
#     ceiling locomotion arm (asm 0x7650); restored at the next Ambushing call. No shipped pirate
#     ambushes in the run areas (IsAmbushing is always 0).
#   Crouch 98D8 and WallHang 69AC, the second Activate each (the first takes the no-cover arm): 0x900 = u0531, the 06_sand_alamofront L2 cover
#     point 001d01e9 (`objs cover`) -> the cover arms (asm 0x9918 / 0x69F0). The fights' forced
#     Crouch/WallHang never held a cover point (CoverFind takes one only in Cover/TargetCover paths).
# Run (2026-10-21, all five forces hit; CoveringFire is only reached in some runs, rerun if its
# forced.log line is missing):
#   SP_FORCE=Captured SP_FORCE_EID=001d0051 SP_STEPS='wait 15; fsm {#173} Crouch; wait 3; fsm {#173} Crouch; wait 3; fsm {#173} WallHang; wait 5; fsm {#173} WallHang; wait 5;
#   fsm {#189} LaunchGrenade; wait 4; deleteteams; fsm {#431} LaunchGrenade; wait 4; fsm {#174} Ambushing; wait 3; fsm {#174} Ambushing; wait 3'
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
      '$ca = $ca_a ? 1 : $ca',
      '$ca = ($ca >= 1 && $ca < 30 && $rdx == 1) ? $ca + 1 : $ca',
      '$this->mAnimationState.mState = $ca == 20 ? 0 : $this->mAnimationState.mState'],
     0, 'body state id = 0 and 0x6b4 = kAS_Ready on one Update; 0x6b4 = kAS_NotReady on the 19th after it'),
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
    ('7630 Ambushing', 'CSpacePirate::Ambushing(', ['$rdx', '$this->x8f4_25_ceilingAmbush'],
     ['$am = $_isvoid($am) ? 0 : $am',
      '$am == 2 ? (((CSpacePirate*)$am_p)->x8f4_25_ceilingAmbush = 0) : 0',
      '$am = $am == 2 ? 3 : $am',
      '$am = ($rdx == 0 && $am < 2) ? $am + 1 : $am',
      '$am_p = $am == 2 ? $this : ($_isvoid($am_p) ? 0 : $am_p)',
      '$this->x8f4_25_ceilingAmbush = $am == 2 ? 1 : $this->x8f4_25_ceilingAmbush'],
     0, 'x8f4_25_ceilingAmbush = 1 on the second Activate (cleared at the next Ambushing call)'),
    ('98D8 Crouch', 'CSpacePirate::Crouch(', ['$rdx', '$this->x900_coverPoint.value'],
     ['$cr = $_isvoid($cr) ? 0 : $cr',
      '$cr = $rdx == 0 ? $cr + 1 : $cr',
      '$this->x900_coverPoint.value = ($rdx == 0 && $cr == 2) ? 0x0531 : $this->x900_coverPoint.value'],
     0, '0x900 = u0531 (a cover point) on the second Activate'),
    ('69AC WallHang', 'CSpacePirate::WallHang(', ['$rdx', '$this->x900_coverPoint.value'],
     ['$wh = $_isvoid($wh) ? 0 : $wh',
      '$wh = $rdx == 0 ? $wh + 1 : $wh',
      '$this->x900_coverPoint.value = ($rdx == 0 && $wh == 2) ? 0x0531 : $this->x900_coverPoint.value'],
     0, '0x900 = u0531 (a cover point) on the second Activate'),
    ('AA64 CoverBlown (restores)', 'CSpacePirate::CoverBlown(', ['$rdx != 0'],
     ['$cn = $_isvoid($cn) ? 0 : $cn', '$cf = $_isvoid($cf) ? 0 : $cf',
      '$cn == 1 ? (((CSpacePirate*)$cn_p)->mAttackTarget.value = $cn_old) : 0',
      '$cn = $cn == 1 ? 2 : $cn',
      '$cf == 1 ? (((CSpacePirate*)$cf_p)->mAreaId.value = $cf_old) : 0',
      '$cf = $cf == 1 ? 2 : $cf'],
     0, 'restores the CoverNearlyBlown/CoveringFire writes (not a forced arm)'),
]
