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
# Slice 11 (Think's 10A64/1D6CC/1D5C4/12DD0, PreRender's 10864, F298, 1890C):
#   23 the attack 0x8000 (1D5C4's first arm)    65 the reflect chance cache reads full (0x10A8 = 10)
#   66 / 67 the last hit times 0xC0C and 0xC20 at the elapsed time / 6 s before it (F298's windows)
#   68 the next RenderFlash sees no texture (its optional's valid byte; the call after puts it back)
#   69 the next UpdateHeadAim: no forward (its x, y zeroed) and the actor on the player's x, y;
#      71 / 72 the forward turned 90 degrees one way / the other (both put back at UpdateFade)
#   73 the next TwistHead sees the actor dead (put back at the next UpdateMeleePulseAndEffects)
#   74 the next TwistHead sees 0xD48 at the elapsed time    75 0xDEC (fading) at 1 until 99
#   76 dead from each UpdateFade to the UpdateTargetable after it, until 99
#   77 the stage's +0x33 reads 0 at the next lookups (it is 1 there naturally); 64 puts it back
#   78 the next PreRender sees a boost ball (its uid 0xE38 reads 1; put back at that PreRender's TwistHead)
# The trigger answers are the console's `trigcall ... = true/false` lines in the game log.
# Run: BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darksamus_uevent.py <out>
#      "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_uevent.con --fast --frames 1500
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
ATTACKS = [(23, 0x8000), (1, 1), (2, 2), (3, 3), (4, 4), (5, 0x40), (6, 0x80), (7, 0x100), (8, 0x20000), (9, 0x100000),
           (10, 0x800000), (11, 0x1000), (12, 0x40000), (13, 0x400000), (14, 0x80000), (15, 0x10000), (20, 0x20), (21, 8), (22, 0x10), (19, 0)]
SETS = [keep(v, m) for v, m in MEMBERS] + [
    # The forced attack (key 30 forgets it): 99 puts the attack back only while it is still the forced
    # one, so an attack the state machine started during a held window is not overwritten (its
    # Deactivate would push the saved 0xFFFF into the history 0x998).
    '$dsk_fa = ' + ''.join('%s ? %d : (' % (k(v), val) for v, val in ATTACKS) +
    '%s ? 0x7fffffff : ($_isvoid($dsk_fa) ? 0x7fffffff : $dsk_fa)' % k(30) + ')' * len(ATTACKS),
    put('$this->mCurrentAttack', '($this->mCurrentAttack == $dsk_fa ? %s : $this->mCurrentAttack)'
        % V['$this->mCurrentAttack'], *ATTACKS),
    '$dsk_ns = %s ? 1 : (%s || %s ? 0 : ($_isvoid($dsk_ns) ? 0 : $dsk_ns))' % (k(60), k(99), k(30)),
    '$dsk_ff = %s ? 1 : (%s ? 2 : (%s ? 0 : ($_isvoid($dsk_ff) ? 0 : $dsk_ff)))' % (k(61), k(62), k(30)),
    '$dsk_mis = %s ? 1 : (%s || %s ? 0 : ($_isvoid($dsk_mis) ? 0 : $dsk_mis))' % (k(42), k(99), k(30)),
    '$dsk_ru = %s ? 1 : (%s || %s ? 0 : ($_isvoid($dsk_ru) ? 0 : $dsk_ru))' % (k(45), k(99), k(30)),
    '$dsk_wt = %s ? 10 : (%s ? 8 : (%s || %s ? 0 : ($_isvoid($dsk_wt) ? 0 : $dsk_wt)))' % (k(46), k(47), k(99), k(30)),
    '$dsk_ex = %s ? 1 : (%s || %s ? 0 : ($_isvoid($dsk_ex) ? 0 : $dsk_ex))' % (k(48), k(99), k(30)),
    '$dsk_ow = %s ? 1 : (%s || %s ? 0 : ($_isvoid($dsk_ow) ? 0 : $dsk_ow))' % (k(49), k(99), k(30)),
    '$dsk_sb = %s ? 1 : (%s ? 3 : (%s ? 2 : (%s ? 0 : ($_isvoid($dsk_sb) ? 0 : $dsk_sb))))' % (k(63), k(77), k(64), k(30)),
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
    put('$this->mGuestFC0', '$dsk_fc0', (57, 0.5)),
    keep('$dsk_c0c', '$this->mGuestC0C'), keep('$dsk_c20', '$this->mGuestC20'),
    keep('$dsk_rcc', '$this->mReflectChanceCount'),
    put('$this->mGuestC0C', '$dsk_c0c', (66, '$this->mElapsedTime'), (67, '$this->mElapsedTime - 6')),
    put('$this->mGuestC20', '$dsk_c20', (66, '$this->mElapsedTime'), (67, '$this->mElapsedTime - 6')),
    put('$this->mReflectChanceCount', '$dsk_rcc', (65, 10)),
    '$this->mFadeOut = %s ? 1 : $this->mFadeOut' % k(75),
    '$dsk_nf = %s ? 1 : ($_isvoid($dsk_nf) ? 0 : $dsk_nf)' % k(68),
    '(%s && !$_isvoid($dsk_nf) && $dsk_nf == 2) ? ($this->mFlashTexture.m_valid = 1, $dsk_nf = 0) : 0' % k(99),
    '$dsk_ha = %s ? 1 : (%s ? 3 : (%s ? 4 : ($_isvoid($dsk_ha) ? 0 : $dsk_ha)))' % (k(69), k(71), k(72)),
    '$dsk_al = %s ? 1 : ($_isvoid($dsk_al) ? 0 : $dsk_al)' % k(73),
    '$dsk_tt = %s ? 1 : ($_isvoid($dsk_tt) ? 0 : $dsk_tt)' % k(74),
    '$dsk_bbf = %s ? 1 : ($_isvoid($dsk_bbf) ? 0 : $dsk_bbf)' % k(78),
    '$dsk_dead = %s ? 1 : (%s || %s ? 0 : ($_isvoid($dsk_dead) ? 0 : $dsk_dead))' % (k(76), k(99), k(30))]
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
SB = '((CDarkSamusBattleStage*)$rdi)->mStageInfo.unknown_0x53fdcb5b'
# The cast also runs on other entities: keys 61..64 write only the actor's stage (its uid, $dsk_uid).
ISST = '(!$_isvoid($dsk_uid) && $rdi && ((CEntity*)$rdi)->mUniqueId.value == $dsk_uid)'
ISFF = '(%s && !$_isvoid($dsk_ff))' % ISST
ISSB = '(%s && !$_isvoid($dsk_sb))' % ISST
STAGEFLAG = ['(%s && $dsk_ff == 1) ? (%s = 1) : 0' % (ISFF, FLAG),
             '(%s && $dsk_ff == 2) ? (%s = 0, $dsk_ff = 0) : 0' % (ISFF, FLAG),
             '(%s && ($dsk_sb == 1 || $dsk_sb == 3) && $dsk_sbo == -1) ? ($dsk_sbo = %s) : 0' % (ISSB, SB),
             '(%s && $dsk_sb == 1) ? (%s = 1) : 0' % (ISSB, SB),
             '(%s && $dsk_sb == 3) ? (%s = 0) : 0' % (ISSB, SB),
             '(%s && $dsk_sb == 2 && $dsk_sbo != -1) ? (%s = $dsk_sbo, $dsk_sbo = -1, $dsk_sb = 0) : 0' % (ISSB, SB)]
HEADAIM = [
    '(!$_isvoid($dsk_ha) && $dsk_ha >= 1 && $dsk_ha <= 4) ? ($dsk_m01 = $this->mTransform.m01, $dsk_m11 = $this->mTransform.m11, '
    '$dsk_px = $this->mPosition.mX, $dsk_py = $this->mPosition.mY) : 0',
    '(!$_isvoid($dsk_ha) && $dsk_ha == 1) ? ($this->mTransform.m01 = 0, $this->mTransform.m11 = 0, $this->mPosition.mX = ((CActor*)((CStateManager*)$rsi)->mPlayers[0])->mPosition.mX, '
    '$this->mPosition.mY = ((CActor*)((CStateManager*)$rsi)->mPlayers[0])->mPosition.mY, $dsk_ha = 9) : 0',
    '(!$_isvoid($dsk_ha) && $dsk_ha == 3) ? ($this->mTransform.m01 = $dsk_m11, $this->mTransform.m11 = -$dsk_m01, $dsk_ha = 9) : 0',
    '(!$_isvoid($dsk_ha) && $dsk_ha == 4) ? ($this->mTransform.m01 = -$dsk_m11, $this->mTransform.m11 = $dsk_m01, $dsk_ha = 9) : 0']
FADE = [
    '(!$_isvoid($dsk_ha) && $dsk_ha == 9) ? ($this->mTransform.m01 = $dsk_m01, $this->mTransform.m11 = $dsk_m11, '
    '$this->mPosition.mX = $dsk_px, $this->mPosition.mY = $dsk_py, $dsk_ha = 0) : 0',
    '(!$_isvoid($dsk_dead) && $dsk_dead == 1) ? ($dsk_alv = $this->mAlive, $this->mAlive = 0, $dsk_dead = 2) : 0']
TARGETABLE = ['(!$_isvoid($dsk_dead) && $dsk_dead == 2) ? ($this->mAlive = $dsk_alv, $dsk_dead = 1) : 0']
TWIST = ['(!$_isvoid($dsk_al) && $dsk_al == 1) ? ($dsk_alv2 = $this->mAlive, $this->mAlive = 0, $dsk_al = 2) : 0',
         '(!$_isvoid($dsk_tt) && $dsk_tt == 1) ? ($this->mHeadTwistTime = $this->mElapsedTime, $dsk_tt = 0) : 0',
         '(!$_isvoid($dsk_bbf) && $dsk_bbf == 2) ? ($this->mBoostBallUid.value = $dsk_bbs, $dsk_bbf = 0) : 0']
PRERENDER = ['(!$_isvoid($dsk_bbf) && $dsk_bbf == 1) ? ($dsk_bbs = $this->mBoostBallUid.value, $this->mBoostBallUid.value = 1, $dsk_bbf = 2) : 0']
PULSE = ['(!$_isvoid($dsk_al) && $dsk_al == 2) ? ($this->mAlive = $dsk_alv2, $dsk_al = 0) : 0']
FLASH = ['(!$_isvoid($dsk_nf) && $dsk_nf == 2) ? ($this->mFlashTexture.m_valid = 1, $dsk_nf = 0) : '
         '((!$_isvoid($dsk_nf) && $dsk_nf == 1) ? ($this->mFlashTexture.m_valid = 0, $dsk_nf = 2) : 0)']
FORCE = [
    ('1BEFC entry', 'CDarkSamus::DoUserAnimEvent(', ['$xmm0.v4_float[0]', '(int)$rcx', '$this->mCurrentAttack'],
     SETS, 0, 'the attack by the dt key'),
    ('GetBattleStage', 'CDarkSamus::GetBattleStage(', ['$dsk_ns'], STAGE, 0, 'no stage after key 60'),
    ('stage cast', 'CDarkSamusBattleStage* TCastToPtr<CDarkSamusBattleStage>(CEntity*)', ['$dsk_ff', '%s ? (int)%s : -1' % (ISST, FLAG), '%s ? (int)%s : -1' % (ISST, SB)],
     STAGEFLAG, 0, 'the fade-out flag after keys 61, 62'),
    ('CanReflectProjectile', 'CDarkSamus::CanReflectProjectile(', ['$dsk_mis', '(long)$rdx', '(int)$rcx'],
     MISSILE + REFLECT, 0, 'the projectile reads kWT_Missile after key 42 (45..49 the reflect arms)'),
    ('Render', 'CDarkSamus::Render(', ['$this->mGuestD14', '(int)$this->mFlags90C', '$this->mGuestD24',
     '$this->mGuestDE8', '(int)$this->mPhazonPhase', '(int)$this->mDarkVisorDED80', '(int)$this->mEchoVisorDED40', '(int)$this->mBoostBallUid.value', '(int)$dsk_sb'],
     [], 0, 'nothing (the render state seen)'),
    ('183E8', 'CDarkSamus::RenderInvulnerableModel(', ['$this->mGuestD24', '(int)$this->mPhazonPhase',
     '$this->mCurrentAttack', '(int)$this->mEchoVisorDED40', '(long)$rdi'], [], 0, 'nothing (the alpha state seen)'),
    ('10A64', 'CDarkSamus::UpdateHeadAim(', ['(int)$dsk_ha', '$this->mHeadYaw'], HEADAIM, 0, 'the forward/position after 69, 71, 72'),
    ('1D6CC', 'CDarkSamus::UpdateFade(', ['$this->mGuestDE8', '(int)$this->mFadeOut', '(int)$dsk_dead'], FADE, 0, 'put back 69..72; dead after 76'),
    ('1D5C4', 'CDarkSamus::UpdateTargetable(', ['$this->mCurrentAttack', '$this->mGuestDE8', '(int)$this->mDarkVisorDED80', '(int)$this->mEchoVisorDED40'], TARGETABLE, 0, 'alive again after 76'),
    ('18AB8', 'CDarkSamus::PreRender(', ['(int)$this->mBoostBallUid.value', '(int)$dsk_bbf'], PRERENDER, 0, 'a boost ball after 78'),
    ('10864', 'CDarkSamus::TwistHead(', ['(int)$this->mAlive', '$this->mElapsedTime', '$this->mHeadTwistTime'], TWIST, 0, 'dead after 73, 0xD48 after 74'),
    ('12DD0', 'CDarkSamus::UpdateMeleePulseAndEffects(', ['(int)$this->mMeleeContact.mHitUid.value'], PULSE, 0, 'alive again after 73'),
    ('F298', 'CDarkSamus::RollReflectChance(', ['(int)$this->mReflectChanceCount', '$this->mGuestC0C', '$this->mElapsedTime'], [], 0, 'nothing (the cache and the windows seen)'),
    ('1890C', 'CDarkSamus::RenderFlash(', ['(int)$this->mFlashTexture.m_valid', '(int)$dsk_nf'], FLASH, 0, 'no texture after 68'),
]


# Slice 12 (Think fn_10_1B1FC and its calls 1ACA0, F710, F654, 1AE08, 1B0E0, 1B154, 13868):
#   80 the next Think: the actor on the player's x, y (no flat direction; put back at UpdateFade)
#   81 the next Think: dead after CPatterned::Think (the alive test; alive again at UpdateFade)
#   82 0xC00 at 1 before each Think's UpdateMeleeHit, and the contact's hit byte 0x9DE at 0 at its
#      entry, until 99      83 the hit byte at 1 at UpdateMeleeHit's entry until 99
#   84 the next Think's unsafe test: mSafePos 20 above the actor (and kept, 0x928 = 1); 85 the same
#      and 0x928 read 2 after IsInUnsafeSpot's height arm (both put back at UpdateFade)
#   86 the dive launched (0xB20 bit 0x40) at each UpdateDiveContact until 99
#   87 the next UpdateDiveContact sees no touch bounds (its optional's valid byte)
#   88 the stage's +0x30 (unknown_0x3ff87a8c) reads 1 at the next lookups; 89 puts it back
#   90 0xD59 (stage changed) at 1 before each Think's UpdateSilhouetteFade until 99
#   91 / 92 0xCD0 at 1 and 0xD18 = (1, 0, 0) / zero before each Think's UpdateInvulnerableFade
#      until 99
#   93 the next CheckHealthThreshold: threshold 0x10A0 = 1e6, not sent; 94 threshold 0.5, not sent
#   96 each UpdateDiveContact until 99: the dive has hit the player and not bounced
#   95 the next PushPlayerAfterBounce sees a morphed player (put back after its call)
# -O2 puts a body's line breakpoints before the line's own code, so 85 and 87 break on the
# instruction after the call instead (after_call; the actor is kept in $dsk_ds at Think's entry).
import os as _os
_EN = _os.path.join(_os.path.dirname(_os.path.realpath(_os.environ['COV_SPEC'])), '..', '..', '..',
                    'src', 'MetroidPrime', 'Enemies')


def loc(fname, start, text):
    lines = open(_os.path.join(_EN, fname)).read().split('\n')
    i = next(n for n, l in enumerate(lines) if start in l)
    j = next(n for n in range(i, len(lines)) if text in lines[n])
    return 'loc:%s:%d' % (fname, j + 1)


def flag(var, *pairs):
    expr = '(%s || %s ? 0 : ($_isvoid(%s) ? 0 : %s))' % (k(99), k(30), var, var)
    for v, val in reversed(pairs):
        expr = '(%s ? %s : %s)' % (k(v), val, expr)
    return '%s = %s' % (var, expr)


PLX = '((CActor*)((CStateManager*)$rsi)->mPlayers[0])->mPosition'
SETS.extend([
    flag('$dsk_t80', (80, 1)), flag('$dsk_t81', (81, 1)), flag('$dsk_t82', (82, 1)),
    flag('$dsk_t83', (83, 1)), flag('$dsk_t84', (84, 1), (85, 3)), flag('$dsk_t86', (86, 1)),
    flag('$dsk_t87', (87, 1)), flag('$dsk_t90', (90, 1)), flag('$dsk_t91', (91, 1), (92, 2)),
    flag('$dsk_t93', (93, 1), (94, 2)), flag('$dsk_t95', (95, 1)), flag('$dsk_t96', (96, 1)),
    '$dsk_s30 = %s ? 1 : (%s ? 2 : (%s ? 0 : ($_isvoid($dsk_s30) ? 0 : $dsk_s30)))' % (k(88), k(89), k(30)),
    '$dsk_s30o = %s ? -1 : ($_isvoid($dsk_s30o) ? -1 : $dsk_s30o)' % k(30),
    keep('$dsk_thr', '$this->mData.mUnknown72EDEB7D'), keep('$dsk_hts', '$this->mHealthThresholdSent'),
    keep('$dsk_cd0', '$this->mGuestCD0'), keep('$dsk_d18x', '$this->mGuestD18.mX'),
    keep('$dsk_d18y', '$this->mGuestD18.mY'), keep('$dsk_d18z', '$this->mGuestD18.mZ'),
    keep('$dsk_lau', '$this->mDive.mLaunched'), keep('$dsk_mhit', '$this->mMeleeContact.mHit'),
    put('$this->mData.mUnknown72EDEB7D', '$dsk_thr'), put('$this->mHealthThresholdSent', '$dsk_hts'),
    put('$this->mGuestCD0', '$dsk_cd0'), put('$this->mGuestD18.mX', '$dsk_d18x'),
    put('$this->mGuestD18.mY', '$dsk_d18y'), put('$this->mGuestD18.mZ', '$dsk_d18z'),
    put('$this->mDive.mLaunched', '$dsk_lau'), put('$this->mMeleeContact.mHit', '$dsk_mhit')])
S30 = '((CDarkSamusBattleStage*)$rdi)->mStageInfo.unknown_0x3ff87a8c'
IS30 = '(%s && !$_isvoid($dsk_s30))' % ISST
STAGEFLAG.extend([
    '(%s && $dsk_s30 == 1 && $dsk_s30o == -1) ? ($dsk_s30o = %s) : 0' % (IS30, S30),
    '(%s && $dsk_s30 == 1) ? (%s = 1) : 0' % (IS30, S30),
    '(%s && $dsk_s30 == 2 && $dsk_s30o != -1) ? (%s = $dsk_s30o, $dsk_s30o = -1, $dsk_s30 = 0) : 0' % (IS30, S30)])
FADE.extend([
    '(!$_isvoid($dsk_t80) && $dsk_t80 == 2) ? ($this->mPosition.mX = $dsk_tpx, $this->mPosition.mY = $dsk_tpy, $dsk_t80 = 0) : 0',
    '(!$_isvoid($dsk_t81) && $dsk_t81 == 2) ? ($this->mAlive = $dsk_alv3, $dsk_t81 = 0) : 0',
    '(!$_isvoid($dsk_t84) && $dsk_t84 >= 2 && $dsk_t84 != 3) ? ($this->mSafePos.mZ = $dsk_sz, $this->mHasSafePos = $dsk_hsp, $dsk_t84 = 0) : 0'])
THINK = ['$dsk_ds = (long)$rdi', '(!$_isvoid($dsk_t80) && $dsk_t80 == 1) ? ($dsk_tpx = $this->mPosition.mX, $dsk_tpy = $this->mPosition.mY, '
         '$this->mPosition.mX = %s.mX, $this->mPosition.mY = %s.mY, $dsk_t80 = 2) : 0' % (PLX, PLX)]


def after_call(func, callee, nth=1):
    # symbol + offset, so gdb places it again once the PIE binary is relocated
    try:
        import gdb as _g
        out = _g.execute("disassemble '%s'" % func, to_string=True).split('\n')
        mangled = next(l.split('\t')[0].split()[2] for l in open(_os.environ['COV_NM'])
                       if '\t' in l and l.rstrip('\n').split('\t')[1] == func)
    except Exception:
        return 'loc:*0'
    hits = [i for i, l in enumerate(out) if 'call' in l and callee in l]
    off = int(out[hits[nth - 1] + 1].split('<+')[1].split('>')[0])
    return 'loc:*((char*)&%s + %d)' % (mangled, off)


DS = '((CDarkSamus*)$dsk_ds)'
PL = '((CPlayer*)((CStateManager*)$rsi)->mPlayers[0])'
T = 'PortCDarkSamus.cpp'
H = 'PortCDarkSamusHelpers.cpp'
TS = 'void CDarkSamus::Think('
FORCE.extend([
    ('1B1FC', 'CDarkSamus::Think(', ['(int)$dsk_t80', '(int)$this->mAlive'], THINK, 0, 'the actor on the player after 80'),
    ('1B1FC alive', loc(T, TS, 'mElapsedTime += dt;'), ['(int)this->mAlive'],
     ['(!$_isvoid($dsk_t81) && $dsk_t81 == 1) ? ($dsk_alv3 = this->mAlive, this->mAlive = 0, $dsk_t81 = 2) : 0'], 0, 'dead after 81'),
    ('1B1FC F710', loc(T, TS, 'UpdateSilhouetteFade(mgr, dt);'), ['(int)this->mStageChanged'],
     ['(!$_isvoid($dsk_t90) && $dsk_t90 == 1) ? (this->mStageChanged = 1) : 0'], 0, '0xD59 after 90'),
    ('1B1FC F654', loc(T, TS, 'UpdateInvulnerableFade(dt);'), ['(int)this->mGuestCD0', 'this->mGuestD24'],
     ['(!$_isvoid($dsk_t91) && $dsk_t91 == 1) ? (this->mGuestCD0 = 1, this->mGuestD18.mX = 1, this->mGuestD18.mY = 0, this->mGuestD18.mZ = 0) : 0',
      '(!$_isvoid($dsk_t91) && $dsk_t91 == 2) ? (this->mGuestCD0 = 1, this->mGuestD18.mX = 0, this->mGuestD18.mY = 0, this->mGuestD18.mZ = 0) : 0'],
     0, '0xCD0/0xD18 after 91, 92'),
    ('1B1FC 1AE08', loc(T, TS, 'UpdateMeleeHit(mgr);'), ['this->mGuestC00'],
     ['(!$_isvoid($dsk_t82) && $dsk_t82 == 1) ? (this->mGuestC00 = 1) : 0'], 0, '0xC00 after 82'),
    ('1B1FC unsafe', loc(T, TS, 'UpdateStageFlash(mgr);'), ['(int)this->mHasSafePos'],
     ['(!$_isvoid($dsk_t84) && ($dsk_t84 == 1 || $dsk_t84 == 3)) ? ($dsk_sz = this->mSafePos.mZ, $dsk_hsp = this->mHasSafePos, '
      'this->mSafePos.mZ = this->mPosition.mZ + 20, this->mHasSafePos = 1, $dsk_t84 = $dsk_t84 + 1) : 0'], 0, 'unsafe after 84, 85'),
    ('EFC8 height', after_call('CDarkSamus::Think(float, CStateManager&)', 'IsInUnsafeSpot'), ['(int)$rax & 0xff'],
     ['($dsk_t84 == 4 && ($rax & 0xff) == 1) ? (%s->mHasSafePos = 2, $dsk_t84 = 5) : 0' % DS], 0, '0x928 = 2 after 85'),
    ('1AE08', 'CDarkSamus::UpdateMeleeHit(', ['$this->mGuestC00', '(int)$this->mMeleeContact.mHit', '$this->mCurrentAttack'],
     ['(!$_isvoid($dsk_t83) && $dsk_t83 == 1) ? ($this->mMeleeContact.mHit = 1) : ((!$_isvoid($dsk_t82) && $dsk_t82 == 1) ? ($this->mMeleeContact.mHit = 0) : 0)'],
     0, 'the hit byte after 82, 83'),
    ('1ACA0', 'CDarkSamus::UpdateDiveContact(', ['(int)$this->mDive.mLaunched', '(int)$this->mDive.mHitPlayer', '(int)$this->mDive.mBounced'],
     ['(!$_isvoid($dsk_t86) && $dsk_t86 == 1) ? ($this->mDive.mLaunched = 1) : 0',
      '$dsk_t96 == 1 ? ($this->mDive.mHitPlayer = 1, $this->mDive.mBounced = 0) : 0'], 0, 'launched after 86; hit, not bounced after 96'),
    ('1ACA0 bounds', after_call('CDarkSamus::UpdateDiveContact(CStateManager&)', '*0x88'),
     ['(int)((rstl::optional_object<CAABox>*)$rax)->m_valid'],
     ['$dsk_t87 == 1 ? (((rstl::optional_object<CAABox>*)$rax)->m_valid = 0, $dsk_t87 = 0) : 0'], 0, 'no bounds after 87'),
    ('13868', 'CDarkSamus::PushPlayerAfterBounce(', ['(int)%s->mMorphBallState' % PL],
     ['$dsk_t95 == 1 ? ($dsk_pl = (long)%s, $dsk_mbs = (int)%s->mMorphBallState, %s->mMorphBallState = CPlayer::kMS_Morphed, $dsk_t95 = 2) : 0' % (PL, PL, PL)],
     0, 'a morphed player after 95'),
    ('1ACA0 13868', after_call('CDarkSamus::UpdateDiveContact(CStateManager&)', 'PushPlayerAfterBounce'), ['$dsk_t95'],
     ['$dsk_t95 == 2 ? (((CPlayer*)$dsk_pl)->mMorphBallState = (CPlayer::EPlayerMorphBallState)$dsk_mbs, $dsk_t95 = 0) : 0'], 0, 'unmorphed again after 95'),
    ('1B154', 'CDarkSamus::CheckHealthThreshold(', ['$this->mData.mUnknown72EDEB7D', '(int)$this->mHealthThresholdSent'],
     ['(!$_isvoid($dsk_t93) && $dsk_t93 == 1) ? ($this->mData.mUnknown72EDEB7D = 1000000, $this->mHealthThresholdSent = 0, $dsk_t93 = 0) : '
      '((!$_isvoid($dsk_t93) && $dsk_t93 == 2) ? ($this->mData.mUnknown72EDEB7D = 0.5, $this->mHealthThresholdSent = 0, $dsk_t93 = 0) : 0)'],
     0, 'the threshold after 93, 94'),
])

# gdb evaluates both sides of `!$_isvoid($v) && $v == 1`, so every set failed (and was logged) until
# key 30 first wrote its variables: start them all at their "nothing forced" values instead.
try:
    import gdb as _gdb
    import re as _re
    _INIT = {'$dsk_fa': 0x7fffffff, '$dsk_sbo': -1, '$dsk_s30o': -1, '$dsk_uid': -1}
    _names = set()
    for _f in FORCE:
        for _e in list(_f[2]) + list(_f[3]):
            _names.update(_re.findall(r'\$dsk_\w+', _e))
    for _n in sorted(_names):
        _gdb.execute('set var %s = %d' % (_n, _INIT.get(_n, 0)))
except ImportError:
    pass
