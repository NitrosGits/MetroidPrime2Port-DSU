# Forcing spec (tools/cov/force.sh) for the arms of TurnAround 8464 and GetUp B3D4 the fights never take.
# (gdb convenience variables only - not $fs/$gs, which are x86 registers.)
#   TurnAround, one Update: 0xbc4 = the translation (and 0x6b4 = kAS_Ready, so CanIssueCommand can pass)
#     -> the delta is not magnitude-safe -> 0x6b4 = 3 (asm 0x85BC-0x8630).
#   GetUp, one Update: body state id = 2 (kAS_LieOnGround) -> Search(pos, pos) (asm 0xB448-0xB46C);
#     the Getup command is then taken, so the next Updates see body state 1 (CanIssueCommand false).
#     The SetPendingDeath arm needs Search == kR_NoSourcePoint, which Search(p, p) never returns: the
#     DOL's Search (0x8013DCC8) returns 1 with no area and 0 when close_enough(src, dst) (0x8013DD38).
CLASS = 'CSpacePirate'
BS = '$this->mBodyController.mPtr->mBodyStateInfo.mState'
FORCE = [
    ('8464 TurnAround', 'CSpacePirate::TurnAround(', ['$rdx', '$this->mAnimationState.mState'],
     ['$ta = $_isvoid($ta) ? 0 : $ta',
      '$ta_e = ($rdx == 1 && $ta == 0) ? 1 : 0',
      '$this->xbc4_.mX = $ta_e ? $this->mPosition.mX : $this->xbc4_.mX',
      '$this->xbc4_.mY = $ta_e ? $this->mPosition.mY : $this->xbc4_.mY',
      '$this->xbc4_.mZ = $ta_e ? $this->mPosition.mZ : $this->xbc4_.mZ',
      '$this->mAnimationState.mState = $ta_e ? 1 : $this->mAnimationState.mState',
      '$ta = $ta_e ? 1 : $ta'],
     0, 'one Update with 0xbc4 = the translation (no safe delta)'),
    ('B3D4 GetUp', 'CSpacePirate::GetUp(', ['$rdx', '$this->mAnimationState.mState', BS],
     ['$gu = $_isvoid($gu) ? 0 : $gu',
      '$gu_a = ($gu == 0 && $rdx == 1) ? 1 : 0',
      BS + ' = $gu_a ? 2 : ' + BS,
      '$gu = $gu_a ? 1 : $gu'],
     0, 'body state id = 2 (kAS_LieOnGround) on one Update'),
]
