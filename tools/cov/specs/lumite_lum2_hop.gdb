# lum2 hop: the hop planning arms, by calls with the members written (the whole hop plan and surface are saved and put
# back): FindSafeZone / TrySunlightHop at each safe zone (centre, just outside, inside, on the sphere surface, far),
# BuildHopCandidates (chosen hint valid / not a hint, other area, other hint type, in use, nonzero leash position,
# out-of-range, sunlight hints against active / inactive / hurtful zones), ChooseHop (candidate not a hint, inactive,
# in use, hint on the start point, previous chosen hint, nothing left twice), TurnToHopPoint (every message arm, hint not a
# hint, hop failed -> abort), Hop (Update: sunlight landing, no direction; Deactivate: sunlight hop, wall angle, no random)
# and SelectHopTarget (not a hint, sunlight hint, far hint, the three speed-scale arms).
set $t = (CLumite*)$rdi
set $m = (CStateManager*)$rsi
set $lm_hop = $t->mHop
set $lm_srf = $t->mSurface
set $lm_ra = $m->mRandomAvailable
set $lm_ib = (unsigned short*)malloc(16)
set $lm_v = (float*)malloc(64)
set $lm_pl = (char*)malloc(64)
set $lm_td = (float*)malloc(64)
python
import os, struct
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
def setv(x, y, z):
    gdb.execute('set var $lm_v[0] = %r' % x); gdb.execute('set var $lm_v[1] = %r' % y); gdb.execute('set var $lm_v[2] = %r' % z)
def restore():
    gdb.execute('set var $t->mHop = $lm_hop'); gdb.execute('set var $t->mSurface = $lm_srf')
def run(c):
    try:
        gdb.execute(c)
    except gdb.error as e:
        print('ERR', c, str(e)[:80])
# ---- FindSafeZone / TrySunlightHop
setv(1e6, 1e6, 1e6)
run('call $t->FindSafeZone(*$m, *(CVector3f*)$lm_v)')
run('call $t->TrySunlightHop(*$m, *(CVector3f*)$lm_v, *(CPlane*)$lm_pl)')
for z in zones:
    zq = ptr(z, 'CScriptSafeZone')
    cx, cy, cz = [float(gdb.parse_and_eval('%s->mPosition.m%s' % (zq, a))) for a in 'XYZ']
    try:
        sx, sy, sz = [float(gdb.parse_and_eval('%s->mScale.m%s' % (zq, a))) for a in 'XYZ']
    except gdb.error:
        sx = sy = sz = 3.0
    r = (sx + sy + sz) / 3.0
    a0 = int(gdb.parse_and_eval('%s->mActive' % zq))
    for dx in (0.0, r * 0.3, r, r + 1.5):
        setv(cx + dx, cy, cz)
        run('call $t->FindSafeZone(*$m, *(CVector3f*)$lm_v)')
        run('call $t->TrySunlightHop(*$m, *(CVector3f*)$lm_v, *(CPlane*)$lm_pl)')
        restore()
    setv(cx + r + 1.5, cy, cz)
    gdb.execute('set var %s->mActive = 0' % zq)
    run('call $t->FindSafeZone(*$m, *(CVector3f*)$lm_v)')
    gdb.execute('set var %s->mActive = %d' % (zq, a0))
    # a zone whose scale is below 1 (the FindSafeZone scale operand)
    try:
        olds = [gdb.parse_and_eval('%s->mScale.m%s' % (zq, a)) for a in 'XYZ']
        for a in 'XYZ':
            gdb.execute('set var %s->mScale.m%s = 0.1f' % (zq, a))
        run('call $t->FindSafeZone(*$m, *(CVector3f*)$lm_v)')
        for a, o in zip('XYZ', olds):
            gdb.execute('set var %s->mScale.m%s = %r' % (zq, a, float(o)))
    except gdb.error as e:
        print('no scale member', e)
print('zones done', len(zones))
# ---- BuildHopCandidates
HT = '->mHintType'
saved = {h: int(gdb.parse_and_eval(ptr(h, 'CScriptAIHint') + HT)) for h in hints}
def hint_all(v):
    for h in hints:
        gdb.execute('set var %s%s = %d' % (ptr(h, 'CScriptAIHint'), HT, v))
def hint_restore():
    for h, v in saved.items():
        gdb.execute('set var %s%s = %d' % (ptr(h, 'CScriptAIHint'), HT, v))
run('set var $t->mHop.mChosenHintId.value = %d' % (hints[0] if hints else selfid))
run('call $t->BuildHopCandidates(*$m)')
run('set var $t->mHop.mChosenHintId.value = %d' % selfid)
run('call $t->BuildHopCandidates(*$m)')
sz = int(gdb.parse_and_eval('sizeof($t->mAreaId)'))
a0 = int(gdb.parse_and_eval('*(int*)&$t->mAreaId')) & ((1 << (8 * sz)) - 1)
gdb.execute('set var *(char*)&$t->mAreaId = %d' % ((a0 + 1) & 0x7f))
run('call $t->BuildHopCandidates(*$m)')
gdb.execute('set var *(char*)&$t->mAreaId = %d' % (a0 & 0xff))
restore()
hint_all(0)
run('call $t->BuildHopCandidates(*$m)')
hint_all(2)
for zact in (None, 0):
    if zact is not None:
        for z in zones:
            gdb.execute('set var %s->mActive = %d' % (ptr(z, 'CScriptSafeZone'), zact))
    for hurt in (-1, 0, 1):
        gdb.execute('set $lm_rf_hurt = %d' % hurt)
        run('call $t->BuildHopCandidates(*$m)')
    gdb.execute('set $lm_rf_hurt = -1')
    if zact is not None:
        for z in zones:
            gdb.execute('set var %s->mActive = 1' % ptr(z, 'CScriptSafeZone'))
hint_restore()
if hints:
    hq = ptr(hints[0], 'CScriptAIHint')
    gdb.execute('call %s->SetInUse(1)' % hq)
    run('call $t->BuildHopCandidates(*$m)')
    gdb.execute('call %s->SetInUse(0)' % hq)
run('set var $t->mLatestLeashPosition.mX = 1.0f')
run('call $t->BuildHopCandidates(*$m)')
run('set var $t->mLatestLeashPosition.mX = 0.0f')
run('set var $t->mHop.mMinDistance = 1e9f')
run('call $t->BuildHopCandidates(*$m)')
restore()
print('build done')
# ---- ChooseHop: candidates = [uid], count and data of the reserved vector
def cand(uids):
    gdb.execute('set var $t->mHop.mCandidates.mCount = %d' % len(uids))
    for i, u in enumerate(uids):
        gdb.execute('set var ((unsigned short*)$t->mHop.mCandidates.mData)[%d] = %d' % (i, u))
if hints:
    for h in hints[:6]:
        for pre in (0, 1):
            cand([h])
            run('set var $t->mHop.mChosenScore = -1.0f')
            run('set var $t->mHop.mChosenHintId.value = %d' % (hints[-1] if pre else 0xffff))
            run('call $t->ChooseHop(*$m)')
            restore()
    cand([selfid]); run('call $t->ChooseHop(*$m)'); restore()
    hq = ptr(hints[0], 'CScriptAIHint')
    a0 = int(gdb.parse_and_eval('%s->mActive' % hq))
    gdb.execute('set var %s->mActive = 0' % hq)
    cand([hints[0]]); run('call $t->ChooseHop(*$m)')
    gdb.execute('set var %s->mActive = %d' % (hq, a0)); restore()
    gdb.execute('call %s->SetInUse(1)' % hq)
    cand([hints[0]]); run('call $t->ChooseHop(*$m)')
    gdb.execute('call %s->SetInUse(0)' % hq); restore()
    px = [float(gdb.parse_and_eval('%s->mPosition.m%s' % (hq, a))) for a in 'XYZ']
    ox = [float(gdb.parse_and_eval('$t->mPosition.m%s' % a)) for a in 'XYZ']
    ux = [float(gdb.parse_and_eval('$t->mTransform.m%d2' % i)) for i in range(3)]
    for a, v in zip('XYZ', [ox[i] + 0.5 * ux[i] for i in range(3)]):
        gdb.execute('set var %s->mPosition.m%s = %r' % (hq, a, v))
    run('set var $t->mHop.mChosenScore = -1.0f')
    cand([hints[0]]); run('call $t->ChooseHop(*$m)')
    for a, v in zip('XYZ', px):
        gdb.execute('set var %s->mPosition.m%s = %r' % (hq, a, v))
    restore()
# empty candidates: nothing chosen -> ignore-in-use, then failed
run('set var $t->mHop.mMinDistance = 1e9f'); run('set var $t->mHop.mMaxDistance = -1.0f')
cand([]); run('call $t->ChooseHop(*$m)'); run('call $t->ChooseHop(*$m)')
restore()
print('choose done')
# ---- TurnToHopPoint
def tth(msg):
    run('call $t->TurnToHopPoint(*$m, %s, 0.016f)' % msg)
run('set var $t->mHop.mChosenHintId.value = %d' % selfid); tth('kStateMsg_Activate'); restore()
if hints:
    run('set var $t->mHop.mChosenHintId.value = %d' % hints[0]); tth('kStateMsg_Activate'); restore()
tth('kStateMsg_Deactivate'); restore()
run('set var $t->mHop.mHopSelected = 1'); tth('kStateMsg_Update'); restore()
run('set var $t->mHop.mHopFailed = 1'); tth('kStateMsg_Update'); restore()
run('set var $t->mHop.mMinDistance = 1e9f'); run('set var $t->mHop.mMaxDistance = -1.0f'); run('set var $t->mHop.mIgnoreInUse = 1')
cand([]); run('set var $t->mHop.mChosenHintId.value = 0xffff'); tth('kStateMsg_Update'); restore()
tth('(EStateMsg)9'); restore()
print('tth done')
# ---- Hop
def hop(msg):
    run('call $t->Hop(*$m, %s, 0.016f)' % msg)
run('set var $t->mHop.mLaunched = 1'); run('set var $t->mHop.mLanded = 0'); run('set var $t->mHop.mSunlightHop = 1')
for a in 'XYZ':
    run('set var $t->mHop.mTarget.m%s = $t->mPosition.m%s' % (a, a))
hop('kStateMsg_Update'); restore()
run('set var $t->mHop.mLaunched = 1'); run('set var $t->mHop.mLanded = 0'); run('set var $t->mHop.mSunlightHop = 0')
hop('kStateMsg_Update'); restore()
run('set var $t->mHop.mLaunched = 1'); run('set var $t->mHop.mLanded = 0'); run('set var $t->mHop.mSunlightHop = 0')
run('set var $t->mHop.mTarget.mX = $t->mPosition.mX + 20.0f')
hop('kStateMsg_Update')
run('set var $t->mAnimationState.mState = CAnimationState::kAS_Over'); hop('kStateMsg_Update')
run('set var $t->mAnimationState.mState = CAnimationState::kAS_Ready'); restore()
run('set var $t->mHop.mSunlightHop = 1'); hop('kStateMsg_Deactivate'); restore()
run('set var $t->mHop.mSunlightHop = 0'); hop('kStateMsg_Deactivate')
run('set var $t->mHop.mPlane.mNormal.mX = -$t->mSurface.mPlane.mNormal.mX')
run('set var $t->mHop.mPlane.mNormal.mY = -$t->mSurface.mPlane.mNormal.mY')
run('set var $t->mHop.mPlane.mNormal.mZ = -$t->mSurface.mPlane.mNormal.mZ')
hop('kStateMsg_Deactivate'); restore()
run('set var $t->mHop.mSunlightHop = 0'); run('set var $m->mRandomAvailable = 0'); hop('kStateMsg_Deactivate')
run('set var $m->mRandomAvailable = %d' % int(gdb.parse_and_eval('$lm_ra'))); restore()
hop('(EStateMsg)9'); restore()
print('hop done')
# ---- SelectHopTarget
def sel(uid):
    run('call $t->SelectHopTarget(*$m, *(TUniqueId*)$lm_ib)')
for uid in [selfid] + hints:
    gdb.execute('set var $lm_ib[0] = %d' % uid)
    for dist in (5.0, 30.0, 100.0):
        run('set var $t->mHop.mDistance = %r' % dist)
        sel(uid); restore()
    run('set var $t->mHop.mHintId.value = %d' % selfid)
    sel(uid); restore()
if hints:
    hq = ptr(hints[0], 'CScriptAIHint')
    px = [float(gdb.parse_and_eval('%s->mPosition.m%s' % (hq, a))) for a in 'XYZ']
    gdb.execute('set var %s->mPosition.mZ = 100000.0f' % hq)
    gdb.execute('set var $lm_ib[0] = %d' % hints[0]); sel(hints[0]); restore()
    gdb.execute('set var %s->mPosition.mZ = %r' % (hq, px[2]))
print('select done')
end
set var $t->mHop = $lm_hop
set var $t->mSurface = $lm_srf
set var $m->mRandomAvailable = $lm_ra
printf "hop done\n"
