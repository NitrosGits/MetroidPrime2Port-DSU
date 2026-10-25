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
        za = int(gdb.parse_and_eval(zq + '->mActive'))
        gdb.execute('set var %s->mActive = 1' % zq)
        print('lum4 zone', z, 'active as shipped', za)
        run('print %s->GetScale().MagSquared()' % zq)
        c = [getf('%s->mPosition.m%s' % (zq, a)) for a in 'XYZ']
        for dx in (0.0, 1.0, 2.0):
            setpos(hq, [c[0] + dx, c[1], c[2]])
            gdb.execute('set var $t->mHop.mMinDistance = 0.0f'); gdb.execute('set var $t->mHop.mMaxDistance = 1e9f')
            gdb.execute('set var $lm_v[0] = %r' % (c[0] + dx)); gdb.execute('set var $lm_v[1] = %r' % c[1]); gdb.execute('set var $lm_v[2] = %r' % c[2])
            print('lum4 find zone', z, 'dx', dx, int(gdb.parse_and_eval('(long)$t->FindSafeZone(*$m, *(CVector3f*)$lm_v)')) == int(zq.strip('()').split(')')[-1]))
            for hv in (-1, 0, 1):
                gdb.execute('set $lm_rf_hurt = %d' % hv)
                run('call $t->BuildHopCandidates(*$m)')
                print('lum4 build zone', z, 'dx', dx, 'hurt', hv, 'candidates', int(gdb.parse_and_eval('$t->mHop.mCandidates.mCount')))
                gdb.execute('set var $t->mHop.mMinDistance = 0.0f'); gdb.execute('set var $t->mHop.mMaxDistance = 1e9f')
            gdb.execute('set $lm_rf_hurt = -1')
        gdb.execute('set var %s->mActive = %d' % (zq, za))
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

# Dead: the switch's third arm (Deactivate: no case matches), and the IsDead-true arm for real: the body state info's current
# state is switched to the death state (pas::kAS_Death, a CBSDie) with its mIsDead set, so Dead's `GetCurrentState()->IsDead()`
# (slot 0x18, not CBodyState::IsDead) takes the indirect call into CBSDie::IsDead (true) and the fade starts.
set $lm_fd = $t->mFadeToDeath
set $lm_pg = $t->mPrevOnGround
set $lm_ad = $t->mAlphaDelta
python
run('call $t->Dead(*$m, kStateMsg_Deactivate, 0.016f)')
gdb.execute('set $lm_bc = *(long*)((char*)$t + 0x530)')
gdb.execute('set $lm_bsi = (CBodyStateInfo*)((char*)$lm_bc + 0x4c8)')
gdb.execute('set $lm_ms = $lm_bsi->mState')
gdb.execute('set $lm_die = ((CBSDie**)$lm_bsi->mStates.mData)[(int)pas::kAS_Death]')
print('lum4 die state', int(gdb.parse_and_eval('(long)$lm_die')), 'cur', int(gdb.parse_and_eval('$lm_ms')))
gdb.execute('set $lm_dd = $lm_die->mIsDead')
gdb.execute('set $lm_z = (float*)malloc(16)')
for i in range(3):
    gdb.execute('set var $lm_z[%d] = 0.0f' % i)
gdb.execute('call $t->SetVelocityWR(*(CVector3f*)$lm_z)')
gdb.execute('set var $t->mPrevOnGround = 0')
gdb.execute('set var $t->mFadeToDeath = 0')
gdb.execute('set var $lm_die->mIsDead = 1')
gdb.execute('set var $lm_bsi->mState = pas::kAS_Death')
run('call $t->Dead(*$m, kStateMsg_Update, 0.016f)')
print('lum4 dead fade', int(gdb.parse_and_eval('$t->mFadeToDeath')), 'alphaDelta', getf('$t->mAlphaDelta'))
gdb.execute('set var $lm_bsi->mState = $lm_ms')
gdb.execute('set var $lm_die->mIsDead = $lm_dd')
gdb.execute('set var $t->mFadeToDeath = $lm_fd')
gdb.execute('set var $t->mPrevOnGround = $lm_pg')
gdb.execute('set var $t->mAlphaDelta = $lm_ad')
end
set var $t->mHop = $lm_hop
set var $t->mSurface = $lm_srf
printf "lum4 done\n"
