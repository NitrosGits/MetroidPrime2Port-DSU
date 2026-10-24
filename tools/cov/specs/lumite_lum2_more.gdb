# lum2 more: Taunt's Deactivate with the nearest surface degenerate / not; Hop (no direction; wall-angle with a degenerate
# surface); TurnToHopPoint Update with a chosen hint (ChooseHop true and selected); BuildHopCandidates with a sunlight hint
# moved next to each safe zone (active, hurtful forced both ways); FindSafeZone with a manager id that is no zone; ChooseHop
# with a hint behind the floor (line of sight blocked); Dead: velocity zero, fade already running, body state dead.
set $t = (CLumite*)$rdi
set $m = (CStateManager*)$rsi
set $lm_hop = $t->mHop
set $lm_srf = $t->mSurface
set $lm_ltt = $t->mLastTauntTime
set $lm_tt = $t->mTauntType
set $lm_tsd = $t->mTauntStepDir
set $lm_ib = (unsigned short*)malloc(16)
set $lm_v = (float*)malloc(64)
set $lm_pl = (char*)malloc(64)
python
import os
def rd(n):
    try:
        return [int(x) for x in open(os.environ['LUM2_FILES'] + n).read().split()]
    except Exception:
        return []
zones, hints = rd('.zones'), rd('.hints')
selfid = int(gdb.parse_and_eval('$t->mUniqueId.value'))
def ptr(uid, ty):
    gdb.execute('set var $lm_ib[0] = %d' % uid)
    return '((%s*)%d)' % (ty, int(gdb.parse_and_eval('(long)$m->ObjectById(*(TUniqueId*)$lm_ib)')))
def run(c):
    try:
        gdb.execute(c)
    except gdb.error as e:
        print('ERR', c, str(e)[:80])
def restore():
    gdb.execute('set var $t->mHop = $lm_hop'); gdb.execute('set var $t->mSurface = $lm_srf')
    gdb.execute('set var $t->mLastTauntTime = $lm_ltt'); gdb.execute('set var $t->mTauntType = $lm_tt')
    gdb.execute('set var $t->mTauntStepDir = $lm_tsd')
# Taunt Deactivate on a floor surface
for dg in (-1, 1, 0):
    run('set var $t->mSurface.mKind = 0'); run('set $lm_rf_degen = %d' % dg)
    run('call $t->Taunt(*$m, kStateMsg_Deactivate, 0.016f)')
    run('set $lm_rf_degen = -1'); restore()
# Hop: no direction (target on the Lumite, not a sunlight landing), degenerate surface at the wall-angle test
run('set var $t->mHop.mLaunched = 1'); run('set var $t->mHop.mLanded = 0'); run('set var $t->mHop.mSunlightHop = 0')
for a in 'XYZ':
    run('set var $t->mHop.mTarget.m%s = $t->mPosition.m%s' % (a, a))
run('call $t->Hop(*$m, kStateMsg_Update, 0.016f)'); restore()
run('set var $t->mHop.mSunlightHop = 0')
for a in 'XYZ':
    run('set var $t->mHop.mPlane.mNormal.m%s = -$t->mSurface.mPlane.mNormal.m%s' % (a, a))
for dg in (1, 0):
    run('set $lm_rf_degen = %d' % dg)
    run('call $t->Hop(*$m, kStateMsg_Deactivate, 0.016f)')
    run('set $lm_rf_degen = -1'); restore()
# TurnToHopPoint Update: ChooseHop -> selected -> SelectHopTarget + TurnTowards(Activate)
def cand(uids):
    gdb.execute('set var $t->mHop.mCandidates.mCount = %d' % len(uids))
    for i, u in enumerate(uids):
        gdb.execute('set var ((unsigned short*)$t->mHop.mCandidates.mData)[%d] = %d' % (i, u))
for h in hints[:4]:
    cand([h]); run('set var $t->mHop.mChosenScore = -1.0f'); run('set var $t->mHop.mChosenHintId.value = 0xffff')
    run('set var $t->mHop.mHopSelected = 0'); run('set var $t->mHop.mHopFailed = 0')
    run('call $t->TurnToHopPoint(*$m, kStateMsg_Update, 0.016f)'); restore()
# BuildHopCandidates with sunlight hints next to each zone
if hints:
    hq = ptr(hints[0], 'CScriptAIHint')
    ht0 = int(gdb.parse_and_eval(hq + '->mHintType'))
    p0 = [float(gdb.parse_and_eval('%s->mPosition.m%s' % (hq, a))) for a in 'XYZ']
    gdb.execute('set var %s->mHintType = 2' % hq)
    for z in zones:
        zq = ptr(z, 'CScriptSafeZone')
        c = [float(gdb.parse_and_eval('%s->mPosition.m%s' % (zq, a))) for a in 'XYZ']
        for dx in (2.0, 5.0):
            for a, v in zip('XYZ', [c[0] + dx, c[1], c[2]]):
                gdb.execute('set var %s->mPosition.m%s = %r' % (hq, a, v))
            for hv in (-1, 0, 1):
                gdb.execute('set $lm_rf_hurt = %d' % hv)
                run('call $t->BuildHopCandidates(*$m)')
            gdb.execute('set $lm_rf_hurt = -1')
    for a, v in zip('XYZ', p0):
        gdb.execute('set var %s->mPosition.m%s = %r' % (hq, a, v))
    gdb.execute('set var %s->mHintType = %d' % (hq, ht0))
    restore()
    # ChooseHop with the hint far below the floor (blocked line of sight)
    gdb.execute('set var %s->mPosition.mZ = %r' % (hq, p0[2] - 5000.0))
    cand([hints[0]]); run('set var $t->mHop.mChosenScore = -1.0f')
    run('call $t->ChooseHop(*$m)')
    gdb.execute('set var %s->mPosition.mZ = %r' % (hq, p0[2]))
    restore()
# FindSafeZone: the manager answers an id that is no safe zone
run('set var $lm_v[0] = $t->mPosition.mX'); run('set var $lm_v[1] = $t->mPosition.mY'); run('set var $lm_v[2] = $t->mPosition.mZ')
run('set $lm_rf_szm = %d' % selfid)
run('call $t->FindSafeZone(*$m, *(CVector3f*)$lm_v)')
run('set $lm_rf_szm = -1')
print('more calls done')
end
set var $t->mHop = $lm_hop
set var $t->mSurface = $lm_srf
printf "more done\n"
