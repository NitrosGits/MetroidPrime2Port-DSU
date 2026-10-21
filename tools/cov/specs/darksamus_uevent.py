# DarkSamus anim-event handler (fn_10_1BEFC) and its helpers 1BDF0, 1BE04, 1BE80; the triggers its
# Landing arms feed (16B5C, 16B54, 16B48) and the no-stage arms of 14284, BB78, BBE4. A forcing run
# on the coverage build: the console's `uevent <id> <type> <dt>` (darksamus_uevent.con) delivers
# each event, and the current attack 0x980 its arms switch on is written at the handler's entry,
# keyed by the dt less 100:
#   30 every member saved                      99 every member put back (each driver line ends so)
#   1, 2, 3, 4 the attack 1..4    5 0x40    6 0x80    7 0x100    8 0x20000    9 0x100000
#   10 0x800000    11 0x1000    12 0x40000    13 0x400000    14 0x80000    19 no attack (0)
#   15 0x10000    20 0x20 (the dive)    21 8    22 0x10
#   40 the dive's bounced bit set until 99 (Landing's bounce)    41 one dash step left until 99
#   42 the deflect slow-down's time 0xC04 at 0 and the projectiles CanReflectProjectile sees read
#      kWT_Missile until 99 (with `shoot`, a power shot 40 units out, and key 60's no stage)
#   43 0xC04 at 1 until 99; 99 also puts the body's time scale back
#   44 0xD14 at 1 until 99 (SpawnBoostBall's second ctor arm)
#   60 the next fight-script lookups (GetBattleStage) answer no object: its uid is written
#      kInvalidUniqueId there until key 99 puts it back (the no-stage arms of the triggers)
#   61 the stage's fade-out flag (its info's +0x31) reads 1 at the next lookups (written where
#      GetBattleStage casts the object, TCastToPtr<CDarkSamusBattleStage>); 62 puts it back to 0,
#      the shipped value (ShouldFadeIn answers true with the stage before the run forces it)
#   45 the next projectile CanReflectProjectile sees gets a dummy uid and its own uid appended to
#      the reflected list 0xBEC (once)    46 / 47 its damage's weapon mode reads kWT_Phazon /
#      kWT_CannonBall until 99    48 it reads exploded (once)    49 its owner is the actor (once)
#   50 0xD14 at 1 until 99 (Render's silhouette pass; 0x90C is put back at 99)
#   51 / 52 0xD24 at 0.5 / 1 until 99    53 the phazon phase bit set until 99
#   54 0xDE8 at 0 until 99    55 the flash window (0xE04 the elapsed time, 0xE08 10, 0xE0C 1)
#   56 no boost ball (its uid 0xE38 reads kInvalidUniqueId) until 99, for Render's model arms
#   57 the phazon blend 0xFC0 at 0.5 until 99 (PreRender's colour lerp)
#   63 the stage's +0x33 (unknown_0x53fdcb5b) reads 1 at the next lookups; 64 puts it back
# The trigger answers are the console's `trigcall ... = true/false` lines in the game log.
# Run: BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darksamus_uevent.py <out>
#      "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_uevent.con --fast --frames 800
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DarkSamus
CLASS = 'CDarkSamus'


def k(v):
    return '($xmm0.v4_float[0] == %d)' % (100 + v)


def keep(var, member):
    return '%s = %s ? %s : ($_isvoid(%s) ? 0 : %s)' % (var, k(30), member, var, var)


def put(member, var, *pairs):
    expr = member
    for v, val in reversed(pairs + ((99, var),)):
        expr = '%s ? %s : (%s)' % (k(v), val, expr)
    return '%s = %s' % (member, expr)


MEMBERS = [('$dsk_ca', '$this->mCurrentAttack'), ('$dsk_a40', '$this->mGuestA40'),
           ('$dsk_a80', '$this->mGuestA80'), ('$dsk_ab8', '$this->mGuestAB8'),
           ('$dsk_a84', '$this->mGuestA84'), ('$dsk_a38', '$this->mGuestA38'),
           ('$dsk_a78', '$this->mGuestA78'), ('$dsk_ab0', '$this->mGuestAB0'),
           ('$dsk_fo', '$this->mFadeOut'), ('$dsk_sc', '$this->mStageChanged'),
           ('$dsk_c00', '$this->mGuestC00'), ('$dsk_d24', '$this->mGuestD24'),
           ('$dsk_f5c', '$this->mGuestF5C40'), ('$dsk_uid', '$this->mBattleStageUid.value'),
           ('$dsk_90c', '$this->mFlags90C'), ('$dsk_de8', '$this->mGuestDE8'),
           ('$dsk_pp', '$this->mPhazonPhase'), ('$dsk_e04', '$this->mGuestE04'),
           ('$dsk_e08', '$this->mGuestE08'), ('$dsk_e0c', '$this->mGuestE0C'),
           ('$dsk_bb', '$this->mBoostBallUid.value'), ('$dsk_fc0', '$this->mGuestFC0'),
           ('$dsk_c04', '$this->mGuestC04'), ('$dsk_ts', '$this->mBodyController.mPtr->mTimeScale'),
           ('$dsk_bnc', '$this->mDive.mBounced'), ('$dsk_stp', '$this->mDash.mStepsLeft'),
           ('$dsk_d14', '$this->mGuestD14')]
V = dict((m, v) for v, m in MEMBERS)
ATTACKS = [(1, 1), (2, 2), (3, 3), (4, 4), (5, 0x40), (6, 0x80), (7, 0x100), (8, 0x20000), (9, 0x100000),
           (10, 0x800000), (11, 0x1000), (12, 0x40000), (13, 0x400000), (14, 0x80000), (15, 0x10000), (20, 0x20), (21, 8), (22, 0x10), (19, 0)]
SETS = [keep(v, m) for v, m in MEMBERS] + [
    put('$this->mCurrentAttack', V['$this->mCurrentAttack'], *ATTACKS),
    '$dsk_ns = %s ? 1 : (%s || %s ? 0 : ($_isvoid($dsk_ns) ? 0 : $dsk_ns))' % (k(60), k(99), k(30)),
    '$dsk_ff = %s ? 1 : (%s ? 2 : (%s ? 0 : ($_isvoid($dsk_ff) ? 0 : $dsk_ff)))' % (k(61), k(62), k(30)),
    '$dsk_mis = %s ? 1 : (%s || %s ? 0 : ($_isvoid($dsk_mis) ? 0 : $dsk_mis))' % (k(42), k(99), k(30)),
    '$dsk_ru = %s ? 1 : (%s || %s ? 0 : ($_isvoid($dsk_ru) ? 0 : $dsk_ru))' % (k(45), k(99), k(30)),
    '$dsk_wt = %s ? 10 : (%s ? 8 : (%s || %s ? 0 : ($_isvoid($dsk_wt) ? 0 : $dsk_wt)))' % (k(46), k(47), k(99), k(30)),
    '$dsk_ex = %s ? 1 : (%s || %s ? 0 : ($_isvoid($dsk_ex) ? 0 : $dsk_ex))' % (k(48), k(99), k(30)),
    '$dsk_ow = %s ? 1 : (%s || %s ? 0 : ($_isvoid($dsk_ow) ? 0 : $dsk_ow))' % (k(49), k(99), k(30)),
    '$dsk_sb = %s ? 1 : (%s ? 2 : (%s ? 0 : ($_isvoid($dsk_sb) ? 0 : $dsk_sb)))' % (k(63), k(64), k(30)),
    '$dsk_sbo = %s ? -1 : ($_isvoid($dsk_sbo) ? -1 : $dsk_sbo)' % k(30)] + [
    put(m, v) for v, m in MEMBERS[1:-5]] + [
    put('$this->mGuestC04', '$dsk_c04', (42, 0), (43, 1)),
    put('$this->mBodyController.mPtr->mTimeScale', '$dsk_ts'),
    put('$this->mDive.mBounced', '$dsk_bnc', (40, 1)),
    put('$this->mDash.mStepsLeft', '$dsk_stp', (41, 1)),
    put('$this->mGuestD14', '$dsk_d14', (44, 1), (50, 1)),
    put('$this->mGuestD24', '$dsk_d24', (51, 0.5), (52, 1)),
    put('$this->mPhazonPhase', '$dsk_pp', (53, 1)),
    put('$this->mGuestDE8', '$dsk_de8', (54, 0)),
    put('$this->mGuestE04', '$dsk_e04', (55, '$this->mElapsedTime')),
    put('$this->mGuestE08', '$dsk_e08', (55, 10)),
    put('$this->mGuestE0C', '$dsk_e0c', (55, 1)),
    put('$this->mBoostBallUid.value', '$dsk_bb', (56, 0xffff)),
    put('$this->mGuestFC0', '$dsk_fc0', (57, 0.5))]
MISSILE = ['(!$_isvoid($dsk_mis) && $dsk_mis && $rdx) ? (((CWeapon*)$rdx)->mWeaponType = 6) : 0']
RU = '((unsigned short*)$this->mReflectedUids.mData)'
REFLECT = [
    '(!$_isvoid($dsk_ru) && $dsk_ru == 1 && $rdx && $this->mReflectedUids.mCount < 4) ? '
    '(%s[$this->mReflectedUids.mCount] = 0x7ffe, %s[$this->mReflectedUids.mCount + 1] = '
    '((CEntity*)$rdx)->mUniqueId.value, $this->mReflectedUids.mCount = $this->mReflectedUids.mCount + 2, '
    '$dsk_ru = 2) : 0' % (RU, RU),
    '(!$_isvoid($dsk_wt) && $dsk_wt && $rdx) ? '
    '(((CWeapon*)$rdx)->mCurDamageInfo.mWeaponMode.mWeaponType = $dsk_wt) : 0',
    '(!$_isvoid($dsk_ex) && $dsk_ex == 1 && $rdx) ? '
    '(((CEnergyProjectile*)$rdx)->mHasExploded = 1, $dsk_ex = 2) : 0',
    '(!$_isvoid($dsk_ow) && $dsk_ow == 1 && $rdx) ? '
    '(((CWeapon*)$rdx)->mOwnerId.value = $this->mUniqueId.value, $dsk_ow = 2) : 0']
STAGE = ['$this->mBattleStageUid.value = $dsk_ns == 1 ? 0xffff : $this->mBattleStageUid.value']
FLAG = '((CDarkSamusBattleStage*)$rdi)->mStageInfo.unknown_0x49b9936d'
STAGEFLAG = ['%s = $dsk_ff == 1 ? 1 : ($dsk_ff == 2 ? 0 : %s)' % (FLAG, FLAG),
             '$dsk_ff = $dsk_ff == 2 ? 0 : $dsk_ff']
SB = '((CDarkSamusBattleStage*)$rdi)->mStageInfo.unknown_0x53fdcb5b'
# The cast also runs on other entities: key 63 writes only the actor's stage (its uid, $dsk_uid).
ISST = '(!$_isvoid($dsk_sb) && $rdi && ((CEntity*)$rdi)->mUniqueId.value == $dsk_uid)'
STAGEFLAG += ['(%s && $dsk_sb == 1 && $dsk_sbo == -1) ? ($dsk_sbo = %s) : 0' % (ISST, SB),
              '(%s && $dsk_sb == 1) ? (%s = 1) : 0' % (ISST, SB),
              '(%s && $dsk_sb == 2 && $dsk_sbo != -1) ? (%s = $dsk_sbo, $dsk_sbo = -1, $dsk_sb = 0) : 0' % (ISST, SB)]
FORCE = [
    ('1BEFC entry', 'CDarkSamus::DoUserAnimEvent(', ['$xmm0.v4_float[0]', '(int)$rcx', '$this->mCurrentAttack'],
     SETS, 0, 'the attack by the dt key'),
    ('GetBattleStage', 'CDarkSamus::GetBattleStage(', ['$dsk_ns'], STAGE, 0, 'no stage after key 60'),
    ('stage cast', 'CDarkSamusBattleStage* TCastToPtr<CDarkSamusBattleStage>(CEntity*)', ['$dsk_ff', FLAG, '(int)' + SB],
     STAGEFLAG, 0, 'the fade-out flag after keys 61, 62'),
    ('CanReflectProjectile', 'CDarkSamus::CanReflectProjectile(', ['$dsk_mis', '(long)$rdx', '(int)$rcx'],
     MISSILE + REFLECT, 0, 'the projectile reads kWT_Missile after key 42 (45..49 the reflect arms)'),
    ('Render', 'CDarkSamus::Render(', ['$this->mGuestD14', '(int)$this->mFlags90C', '$this->mGuestD24',
     '$this->mGuestDE8', '(int)$this->mPhazonPhase', '(int)$this->mDarkVisorDED80', '(int)$this->mEchoVisorDED40', '(int)$this->mBoostBallUid.value', '(int)$dsk_sb'],
     [], 0, 'nothing (the render state seen)'),
    ('183E8', 'CDarkSamus::RenderInvulnerableModel(', ['$this->mGuestD24', '(int)$this->mPhazonPhase',
     '$this->mCurrentAttack', '(int)$this->mEchoVisorDED40', '(long)$rdi'], [], 0, 'nothing (the alpha state seen)'),
]
