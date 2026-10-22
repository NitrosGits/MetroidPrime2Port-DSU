# Forcing spec (tools/cov/force.sh --driver) for the arms of Cover A3C0, Generate C1A4 and ShouldWarpIn 77D0
# the fights never take. Run: SP_STEPS as below, tools/cov/force.sh <this> <out> 42b935e4,06_sand_alamofront
# --driver 'python3 tools/drivers/spacepirate_drive.py {port} 2 3' --frames 9000. (gdb convenience variables only - not $fs/$gs, which are x86 registers.)
#   Cover, one Activate: body state id = 0x13 (kAS_Cover, body +0x37c) -> the bail (asm 0xA41C-0xA420).
#   Generate, one Activate (SP_STEPS `fsm {#431} Taunt; wait 2; fsm {#431} Generate`, 06_sand_alamofront):
#     0x8f9 bit 0x01 (x8f9_31_sentAttackMsg) cleared -> the kSS_Attack/kSM_None message (asm 0xC20C-0xC24C; Taunt and Attack set the bit before
#     it naturally), and 0x8f4 bit 0x40 (x8f4_25_ceilingAmbush) set -> the ceiling destination and the
#     kJT_Ambush jump (a pirate that entered Generate from Attack/Lurk, or the SP_FORCE one, never jumped).
#   ShouldWarpIn 77D0, one call on a trooper (0x8f6 bit 0x80) with 0xc04 = -1 and 0x8fa bit 0x20 set:
#     that bit cleared -> arm 3, the trooper not activated yet (asm 0x77E8-0x77F8).
CLASS = 'CSpacePirate'
BS = '$this->mBodyController.mPtr->mBodyStateInfo.mState'
FORCE = [
    ('A3C0 Cover', 'CSpacePirate::Cover(', ['$rdx', BS],
     ['$cv = $_isvoid($cv) ? 0 : $cv',
      '$cv_a = ($cv == 0 && $rdx == 0) ? 1 : 0',
      BS + ' = $cv_a ? 0x13 : ' + BS,
      '$cv = $cv_a ? 1 : $cv'],
     0, 'body state id = 0x13 (kAS_Cover) on one Activate'),
    ('C1A4 Generate', 'CSpacePirate::Generate(', ['$rdx', '$this->x8f4_25_ceilingAmbush'],
     ['$ge = $_isvoid($ge) ? 0 : $ge',
      '$ge_a = ($ge == 0 && $rdx == 0) ? 1 : 0',
      '$this->x8f4_25_ceilingAmbush = $ge_a ? 1 : $this->x8f4_25_ceilingAmbush',
      '$this->x8f9_31_sentAttackMsg = $ge_a ? 0 : $this->x8f9_31_sentAttackMsg',
      '$ge = $ge_a ? 1 : $ge'],
     0, 'x8f9_31_sentAttackMsg cleared and x8f4_25_ceilingAmbush set on one Activate'),
    ('77D0 ShouldWarpIn', 'CSpacePirate::ShouldWarpIn(', ['$this->xc04_warpPoint', '$this->x8f6_24_trooper', '$this->x8fa_26_'],
     ['$sw = $_isvoid($sw) ? 0 : $sw',
      '$sw_a = ($sw == 0 && $this->xc04_warpPoint == -1 && $this->x8f6_24_trooper && $this->x8fa_26_) ? 1 : 0',
      '$this->x8fa_26_ = $sw_a ? 0 : $this->x8fa_26_',
      '$sw = $sw_a ? 1 : $sw'],
     0, 'x8fa_26_ cleared on one activated trooper call'),
]
