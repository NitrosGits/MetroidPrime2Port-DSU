# DarkSamus (module 10) vtable overrides (slice dsvt1): AcceptScriptMsg, Get{Orbit,Aim}Position,
# GetCollisionPrimitive, CollidedWith and its helpers. Sanctuary 10_cliff_rooftop layer 2, boss
# u04f7, with tools/cov/specs/darksamus_vt.con and the coverage build (BUILD=build-port-cov):
# gcov_report gives the arms, these keys say which ran. The arms the fight does not reach are
# written at the entry of a body (gdb convenience counters, one forced write per arm; the
# registers are the x86-64 entry registers, so a written argument register is what the body sees).
CLASS = 'CDarkSamus'


def K(v):
    return '($ds_%s = ($_isvoid($ds_%s) ? 0 : $ds_%s + 1))' % (v, v, v)


def E0(reg):  # the left material of a collision list's first contact
    return ('((CCollisionInfo*)(((CCollisionInfoList*)%s)->mList.mData))[0].mMaterialLeft.mValue'
            % reg)


def N(reg):
    return '((CCollisionInfoList*)%s)->mList.mCount' % reg


PLATFORM, FLOOR, WALL, CEILING = 1 << 49, 1 << 31, 1 << 30, 1 << 29
MSG = '((CScriptMsg*)$rdx)->m_msg'
# AcceptScriptMsg: 'ACTV' alternates the boss bar's float 0x908 (> 0 / not); the landing messages
# in the dive alternate the bounce bit; the third 'XDMG' sets 0xC20 to now (the 1.0 s block);
# each 'XALD' numbers itself for the FindConnectedObject entry below.
ACC_ACTV = ('(%s == 0x41435456 ? (%s, $this->mData.mUnknown74FA22F0 = ($ds_ak %% 2 == 0 ? 700.0 : '
            '-1.0)) : 0)' % (MSG, K('ak')))
ACC_LAND = ('((%s == 0x584c4e44 || %s == 0x584c5347) ? ($this->mCurrentAttack == 0x20 ? (%s, '
            '$this->mDive.mBounced = $ds_lk %% 2) : 0) : 0)' % (MSG, MSG, K('lk')))
ACC_XDMG = ('(%s == 0x58444d47 ? (%s, ($ds_dk == 2 ? ($this->mGuestC20 = $this->mElapsedTime) : 0))'
            ' : 0)' % (MSG, K('dk')))
ACC_ENR = ('(!$_isvoid($ds_ea) ? ($ds_ea == 1 ? ($this->mCurrentAttack = $ds_eo, $ds_ea = 2) : 0) : '
           '(%s == 0x58444d47 ? ($ds_dk == 5 ? ($ds_eo = $this->mCurrentAttack, '
           '$this->mCurrentAttack = 0x800000, $ds_ea = 1) : 0) : 0))' % MSG)
ACC_XALD = '(%s == 0x58414c44 ? (%s, $ds_fc = $ds_xk) : 0)' % (MSG, K('xk'))
# FindConnectedObject from the 2nd sent 'XALD' (the 4th 'XALD' overall): a state nothing is
# connected with (the invalid uid); from the 3rd: the first connection that is not 'IS05' (an
# AI hint, an object that is not a battle stage).
FCO = ('($_isvoid($ds_fc) ? 0 : ($ds_fc == 2 ? ($rdx = 0x7a7a7a7a, $ds_fc = 0) : ($ds_fc == 3 ? '
       '($ds_ci = (((CEntity*)$rdi)->mConnections.mItems[0].state == 0x49533035 ? 1 : 0), '
       '$rdx = ((CEntity*)$rdi)->mConnections.mItems[$ds_ci].state, '
       '$rcx = ((CEntity*)$rdi)->mConnections.mItems[$ds_ci].msg, $ds_fc = 0) : 0)))')


# Get{Orbit,Aim}Position: the 2nd call a boost ball uid with no actor (0x3FE), the 3rd puts it
# back; the 3rd and 4th the attack position (0, 5, 0) and (0, 0, 5), the 5th the saved one back.
def POS(k, member):
    m = '$this->' + member
    return ('(%s, ($ds_%s == 1 ? ($this->mBoostBallUid.value == 0xffff ? '
            '($this->mBoostBallUid.value = 0x3fe) : 0) : ($ds_%s == 2 ? '
            '(($this->mBoostBallUid.value == 0x3fe ? ($this->mBoostBallUid.value = 0xffff) : 0), '
            '$ds_%sx = %s.mX, $ds_%sy = %s.mY, $ds_%sz = %s.mZ, %s.mX = 0.0, %s.mY = 5.0, '
            '%s.mZ = 0.0, 1) : ($ds_%s == 3 ? (%s.mY = 0.0, %s.mZ = 5.0, 1) : ($ds_%s == 4 ? '
            '(%s.mX = $ds_%sx, %s.mY = $ds_%sy, %s.mZ = $ds_%sz, 1) : 0)))))'
            % (K(k), k, k, k, m, k, m, k, m, m, m, m, k, m, m, k, m, k, m, k, m, k))


# AddPhazonBob: the 2nd call inside the volley's window, the 3rd after it (6 s ago: > 0 and past
# the 5.3 s), the 4th back.
BOB = ('(%s, ($ds_pk == 1 ? ($this->mGuestFCC = $this->mElapsedTime - 1.0) : ($ds_pk == 2 ? '
       '($this->mGuestFCC = $this->mElapsedTime - 6.0) : ($ds_pk == 3 ? '
       '($this->mGuestFCC = -1000.0) : 0))))' % K('pk'))
# GetCollisionPrimitive in the dive: the 2nd call at the target (distance 0), the 3rd puts the
# target back; the 4th bounced, the 5th the bounce bit back.
PRIM = ('($this->mCurrentAttack == 0x20 ? (%s, ($ds_gk == 1 ? ($ds_gt = $this->mDive.mTarget, '
        '$this->mDive.mTarget = $this->mPosition) : ($ds_gk == 2 ? ($this->mDive.mTarget = $ds_gt) '
        ': ($ds_gk == 3 ? ($ds_gb = $this->mDive.mBounced, $this->mDive.mBounced = 1) : '
        '($ds_gk == 4 ? ($this->mDive.mBounced = $ds_gb) : 0))))) : 0)' % K('gk'))
# CollidedWith in the attacks 0x4 / 0x80: the 3rd hit nearer the start than the target, the 4th
# at both (not nearer), the 5th the dive record back and more than two stuck dashes, the 6th back.
COLL = ('(($this->mCurrentAttack == 4 || $this->mCurrentAttack == 0x80) ? (%s, ($ds_ck == 2 ? '
        '($ds_cs = $this->mDive.mStart, $ds_ct = $this->mDive.mTarget, '
        '$this->mDive.mStart = $this->mPosition, $this->mDive.mTarget = $this->mPosition, '
        '$this->mDive.mTarget.mX = $this->mPosition.mX + 10.0) : ($ds_ck == 3 ? '
        '($this->mDive.mTarget = $this->mPosition) : ($ds_ck == 4 ? ($this->mDive.mStart = $ds_cs, '
        '$this->mDive.mTarget = $ds_ct, $ds_st = $this->mDashStuckCount, '
        '$this->mDashStuckCount = 3) : ($ds_ck == 5 ? ($this->mDashStuckCount = $ds_st) : 0))))) : 0)'
        % K('ck'))
# CollidedWith: the 1st hit in the attack 0x80 is made a dive (0x20) that has not launched, with
# no boost ball; the next hit puts the attack, the launch bit and the boost ball uid back (the fight's dive hits all come launched).
DIVE = ('($_isvoid($ds_nl) ? ($this->mCurrentAttack == 0x80 ? ($ds_nl = 1, $ds_nb = '
        '$this->mDive.mLaunched, $ds_nu = $this->mBoostBallUid.value, $this->mBoostBallUid.value = '
        '0xffff, $this->mCurrentAttack = 0x20, $this->mDive.mLaunched = 0, 1) : 0) '
        ': ($ds_nl == 1 ? ($ds_nl = 2, $this->mCurrentAttack = 0x80, $this->mDive.mLaunched = '
        '$ds_nb, $this->mBoostBallUid.value = $ds_nu, 2) : 0))')
# RecordPlatformContact (list = $rsi): the 2nd non-empty list's first contact a platform, the
# 3rd a platform floor.
PLAT = ('(%s > 0 ? (%s, ($ds_qk == 1 ? (%s = %d) : ($ds_qk == 2 ? (%s = %d) : 0))) : 0)'
        % (N('$rsi'), K('qk'), E0('$rsi'), PLATFORM, E0('$rsi'), PLATFORM | FLOOR))
# RecordWallHit (id = $rsi, list = $rdx): with no object, the 2nd list a wall, the 3rd a ceiling;
# in an attack other than 0xFFFF / 0x2000 the 1st hit's id made invalid.
WALLK = ('(((TUniqueId*)$rsi)->value == 0xffff ? ((%s > 0 ? ($this->mCurrentAttack == 0xffff ? 1 : '
         '($this->mCurrentAttack == 0x2000 ? 1 : 0)) : 0) ? (%s, ($ds_wk == 1 ? (%s = %d) : '
         '($ds_wk == 2 ? (%s = %d) : 0))) : 0) : (($this->mCurrentAttack != 0xffff ? '
         '($this->mCurrentAttack != 0x2000 ? 1 : 0) : 0) ? (%s, ($ds_vk == 0 ? '
         '(((TUniqueId*)$rsi)->value = 0xffff) : 0)) : 0))'
         % (N('$rdx'), K('wk'), E0('$rdx'), WALL, E0('$rdx'), CEILING, K('vk')))
# CheckBoostBallWallHit (list = $rdx): the 2nd non-empty list with a boost ball uid (0x3FE when
# there is none) and a wall, the 3rd a floor (no wall), the 4th the uid and the flag back.
BALL = ('(%s > 0 ? (%s, ($ds_bk == 1 ? (($this->mBoostBallUid.value == 0xffff ? '
        '($this->mBoostBallUid.value = 0x3fe) : 0), %s = %d) : ($ds_bk == 2 ? (%s = %d) : '
        '($ds_bk == 3 ? (($this->mBoostBallUid.value == 0x3fe ? ($this->mBoostBallUid.value = '
        '0xffff) : 0), $this->mBoostBallFlag80 = 0) : 0)))) : 0)'
        % (N('$rdx'), K('bk'), E0('$rdx'), WALL, E0('$rdx'), FLOOR))
# CrushPlatform (id = $rdx, list = $rcx): the 2nd..4th non-empty lists from the platform u0423
# with a floor, a platform and neither as the first contact.
CRUSH = ('(%s > 0 ? (%s, ($ds_xc >= 1 ? ($ds_xc <= 3 ? (((TUniqueId*)$rdx)->value = 0x0423, '
         '%s = ($ds_xc == 1 ? %d : ($ds_xc == 2 ? %d : 0))) : 0) : 0)) : 0)'
         % (N('$rcx'), K('xc'), E0('$rcx'), FLOOR, PLATFORM))

SPEC = [
    ('1E89C AcceptScriptMsg', 'CDarkSamus::AcceptScriptMsg(',
     [MSG, '$this->mCurrentAttack', ACC_ACTV, ACC_LAND, ACC_XDMG, ACC_XALD, ACC_ENR], 'max=3000'),
    ('-- FindConnectedObject', 'CEntity::FindConnectedObject(', [FCO], 'max=3000'),
    ('10358 GetOrbitPosition', 'CDarkSamus::GetOrbitPosition(', [POS('ok', 'mAttackOrbitPos')],
     'max=8'),
    ('10210 GetAimPosition', 'CDarkSamus::GetAimPosition(', [POS('ak2', 'mAttackAimPos')],
     'max=8'),
    ('10484 AddPhazonBob', 'CDarkSamus::AddPhazonBob(', [BOB], 'max=8'),
    ('1AAC4 GetCollisionPrimitive', 'CDarkSamus::GetCollisionPrimitive(', [PRIM], 'max=3000'),
    ('1C7FC CollidedWith', 'CDarkSamus::CollidedWith(',
     ['$this->mCurrentAttack', '$this->mBoostBallUid.value != 0xffff', '$this->mDashStuckCount',
      COLL, DIVE], 'max=3000'),
    ('1D0A0 RecordPlatformContact', 'CDarkSamus::RecordPlatformContact(', [PLAT], 'max=3000'),
    ('1CF84 RecordWallHit', 'CDarkSamus::RecordWallHit(', [WALLK], 'max=3000'),
    ('A734 CheckBoostBallWallHit', 'CDarkSamus::CheckBoostBallWallHit(', [BALL], 'max=3000'),
    ('1D194 CrushPlatform', 'CDarkSamus::CrushPlatform(', [CRUSH], 'max=3000'),
    # CanReflectHit (damage = $rdx, projectile uid = $rcx): the 2nd and 3rd calls a missile, 3 s
    # after the last hits, with a cached roll of -1 for the shot (below any stage chance: true);
    # the 3rd also a vertical hit direction (no flat direction).
    ('-- CanReflectHit', 'CDarkSamus::CanReflectHit(',
     ['(%s, ($ds_rk >= 1 ? ($ds_rk <= 2 ? (((CDamageInfo*)$rdx)->mWeaponMode.mWeaponType = 6, '
      '$this->mGuestC0C = -1000.0, $this->mGuestC20 = -1000.0, '
      '($this->mReflectChanceCount == 0 ? ($this->mReflectChanceCount = 1) : 0), '
      '$this->mReflectChances[0].mUid.value = ($rcx & 0xffff), '
      '$this->mReflectChances[0].mRoll = -1.0, '
      '($ds_rk == 2 ? ($this->mHitDirection.mX = 0.0, $this->mHitDirection.mY = 0.0) : 0)) : 0) '
      ': 0))' % K('rk')], 'ret max=3000'),
]
