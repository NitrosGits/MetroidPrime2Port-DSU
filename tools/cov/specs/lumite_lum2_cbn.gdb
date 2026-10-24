# lum2 cbn: arms behind `CanBeNormalized()` false that real geometry cannot reach, by forcing that one call's result
# (CbnForce: the call made from the source line of the test): TrySunlightHop's two `return false` tests, SelectHopTarget's
# cross-product loop (entered with the product degenerate; left because no random is available; the redrawn direction
# degenerate) and Hop's direction test.
set $t = (CLumite*)$rdi
set $m = (CStateManager*)$rsi
set $lm_hop = $t->mHop
set $lm_srf = $t->mSurface
set $lm_ra = $m->mRandomAvailable
set $lm_ib = (unsigned short*)malloc(16)
set $lm_pl = (char*)malloc(64)
set $lm_v = (float*)malloc(64)
python
import os
def rd(n):
    try:
        return [int(x) for x in open(os.environ['LUM2_FILES'] + n).read().split()]
    except Exception:
        return []
zones, hints = rd('.zones'), rd('.hints')
def run(c):
    try:
        gdb.execute(c)
    except gdb.error as e:
        print('ERR', c, str(e)[:80])
def restore():
    gdb.execute('set var $t->mHop = $lm_hop'); gdb.execute('set var $t->mSurface = $lm_srf')
def ptr(uid, ty):
    gdb.execute('set var $lm_ib[0] = %d' % uid)
    return '((%s*)%d)' % (ty, int(gdb.parse_and_eval('(long)$m->ObjectById(*(TUniqueId*)$lm_ib)')))
def arm(k, name, cnt=1):
    gdb.execute('set $lm_cbn_line_%d = $lm_L_%s' % (k, name)); gdb.execute('set $lm_cbn_cnt_%d = %d' % (k, cnt))
def disarm():
    for k in (1, 2, 3):
        gdb.execute('set $lm_cbn_cnt_%d = 0' % k)
# TrySunlightHop at a position next to each zone (FindSafeZone finds it), both tests forced
for z in zones:
    zq = ptr(z, 'CScriptSafeZone')
    c = [float(gdb.parse_and_eval('%s->mPosition.m%s' % (zq, a))) for a in 'XYZ']
    for a, v in zip('XYZ', [c[0] + 6.0, c[1], c[2]]):
        gdb.execute('set var $lm_v[%d] = %r' % ('XYZ'.index(a), v))
    for nm in ('tozone', 'normal'):
        arm(1, nm)
        run('call $t->TrySunlightHop(*$m, *(CVector3f*)$lm_v, *(CPlane*)$lm_pl)')
        disarm(); restore()
# SelectHopTarget: the degenerate-product loop for every hint uid (the first that reaches it takes the forced calls)
for mode in ('plain', 'norandom', 'inner'):
    for h in hints:
        gdb.execute('set var $lm_ib[0] = %d' % h)
        arm(1, 'while')
        if mode == 'norandom':
            gdb.execute('set var $m->mRandomAvailable = 0')
        if mode == 'inner':
            arm(2, 'inner')
        run('call $t->SelectHopTarget(*$m, *(TUniqueId*)$lm_ib)')
        gdb.execute('set var $m->mRandomAvailable = $lm_ra')
        left = int(gdb.convenience_variable('lm_cbn_cnt_1')) + int(gdb.convenience_variable('lm_cbn_cnt_2'))
        disarm(); restore()
        if left == 0:
            break
# Hop Update: a launched, not landed, non-sunlight hop whose direction is forced degenerate
run('set var $t->mHop.mAborted = 0'); run('set var $t->mHop.mLaunched = 1'); run('set var $t->mHop.mLanded = 0'); run('set var $t->mHop.mSunlightHop = 0')
arm(1, 'hopdir')
run('call $t->Hop(*$m, kStateMsg_Update, 0.016f)')
disarm(); restore()
print('cbn done')
end
set var $t->mHop = $lm_hop
set var $t->mSurface = $lm_srf
set var $m->mRandomAvailable = $lm_ra
printf "cbn done\n"
