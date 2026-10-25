# lum4: the two arms the lum3 battery missed, by calls with the members written (hop plan restored after each):
#  - BuildHopCandidates, the IsHurtful-true arm: a sunlight hint (type 2) next to each safe zone with the hop range opened
#    (mMinDistance 0, mMaxDistance 1e9: the lum2 `more` step moved the hint but left the range, so the distance test dropped it
#    before FindSafeZone), IsHurtful forced -1/0/1 (the real virtual call: slot 0x120 of the zone's vtable);
#  - ChooseHop, the previous chosen hint (a real hint / the Lumite, which is no hint) replaced by a new one: the hint put on the
#    Lumite's up axis at 1.5 (length 0.5: rejected), 3 and 6 above the start point (accepted when the line of sight is clear).
set $t = (CLumite*)$rdi
set $m = (CStateManager*)$rsi
set $lm_hop = $t->mHop
set $lm_srf = $t->mSurface
set $lm_ib = (unsigned short*)malloc(16)
set $lm_v = (float*)malloc(64)
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
def restore():
    gdb.execute('set var $t->mHop = $lm_hop'); gdb.execute('set var $t->mSurface = $lm_srf')
def run(c):
    try:
        gdb.execute(c)
    except gdb.error as e:
        print('ERR', c, str(e)[:80])
def setpos(hq, p):
    for a, v in zip('XYZ', p):
        gdb.execute('set var %s->mPosition.m%s = %r' % (hq, a, v))
def getf(e):
    return float(gdb.parse_and_eval(e))
print('lum4 zones', zones, 'hints', hints, 'self', selfid)
if hints:
    hq = ptr(hints[0], 'CScriptAIHint')
    ht0 = int(gdb.parse_and_eval(hq + '->mHintType'))
    p0 = [getf('%s->mPosition.m%s' % (hq, a)) for a in 'XYZ']
    gdb.execute('set var %s->mHintType = 2' % hq)
    for z in zones:
        zq = ptr(z, 'CScriptSafeZone')
        c = [getf('%s->mPosition.m%s' % (zq, a)) for a in 'XYZ']
        for dx in (0.0, 1.0, 2.0):
            setpos(hq, [c[0] + dx, c[1], c[2]])
            gdb.execute('set var $t->mHop.mMinDistance = 0.0f'); gdb.execute('set var $t->mHop.mMaxDistance = 1e9f')
            gdb.execute('set var $lm_v[0] = %r' % (c[0] + dx)); gdb.execute('set var $lm_v[1] = %r' % c[1]); gdb.execute('set var $lm_v[2] = %r' % c[2])
            run('print (long)$t->FindSafeZone(*$m, *(CVector3f*)$lm_v)')
            for hv in (-1, 0, 1):
                gdb.execute('set $lm_rf_hurt = %d' % hv)
                run('call $t->BuildHopCandidates(*$m)')
                print('lum4 build zone', z, 'dx', dx, 'hurt', hv, 'candidates', int(gdb.parse_and_eval('$t->mHop.mCandidates.mCount')))
                gdb.execute('set var $t->mHop.mMinDistance = 0.0f'); gdb.execute('set var $t->mHop.mMaxDistance = 1e9f')
            gdb.execute('set $lm_rf_hurt = -1')
    setpos(hq, p0)
    gdb.execute('set var %s->mHintType = %d' % (hq, ht0))
    restore()
    # ChooseHop: hint above the Lumite on its up axis
    ox = [getf('$t->mPosition.m%s' % a) for a in 'XYZ']
    ux = [getf('$t->mTransform.m%d2' % i) for i in range(3)]
    def cand(uids):
        gdb.execute('set var $t->mHop.mCandidates.mCount = %d' % len(uids))
        for i, u in enumerate(uids):
            gdb.execute('set var ((unsigned short*)$t->mHop.mCandidates.mData)[%d] = %d' % (i, u))
    other = hints[-1] if hints[-1] != hints[0] else (hints[1] if len(hints) > 1 else selfid)
    for d in (0.5, 1.5, 3.0, 6.0):
        for prev in (0xffff, other, selfid):
            setpos(hq, [ox[i] + (0.5 + d) * ux[i] for i in range(3)])
            cand([hints[0]])
            gdb.execute('set var $t->mHop.mChosenScore = -1.0f')
            gdb.execute('set var $t->mHop.mChosenHintId.value = %d' % prev)
            run('call $t->ChooseHop(*$m)')
            print('lum4 choose d', d, 'prev', prev, 'chosen', int(gdb.parse_and_eval('$t->mHop.mChosenHintId.value')))
            gdb.execute('call %s->SetInUse(0)' % hq)
            if other != selfid:
                gdb.execute('call %s->SetInUse(0)' % ptr(other, 'CScriptAIHint'))
            restore()
    setpos(hq, p0)
print('lum4 calls done')
end
set var $t->mHop = $lm_hop
set var $t->mSurface = $lm_srf
printf "lum4 done\n"
