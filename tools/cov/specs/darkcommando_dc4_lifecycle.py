# dc4 lifecycle/previously unentered state arms. Entry inputs restore at return;
# injected dependency outputs and real lifecycle effects can persist for this run.
# Compiler coverage is the arm oracle. Use tools/drivers/darkcommando_dc4_lifecycle.con.
import os
import gdb

CLASS = 'CDarkCommando'
FORCE = []
OUT = os.environ['COV_OUT']
COUNTS = {}
ACTIVE_RAG = None
WATER = None
FREEZE_THINK = None
SFX = None


def ev(expr):
    return gdb.parse_and_eval(expr)


def put(expr, value):
    gdb.execute('set var %s = %s' % (expr, value), to_string=True)


def log(label, detail):
    with open(os.path.join(OUT, 'forced.log'), 'a') as out:
        out.write('%s: %s\n' % (label, detail))


def count(name):
    n = COUNTS.get(name, 0)
    COUNTS[name] = n + 1
    return n


class Done(gdb.FinishBreakpoint):
    def __init__(self, callback):
        super().__init__(gdb.newest_frame(), internal=True)
        self.callback = callback

    def stop(self):
        self.callback()
        return False


def temporary(prefix, values, label):
    saved = []
    for field, value in values.items():
        expr = prefix + field
        old = ev(expr)
        old = str(int(old)) if old.type.strip_typedefs().code in (
            gdb.TYPE_CODE_INT, gdb.TYPE_CODE_CHAR, gdb.TYPE_CODE_ENUM, gdb.TYPE_CODE_BOOL) else str(old)
        saved.append((expr, old))
        put(expr, value)

    def restore():
        for expr, value in saved:
            put(expr, value)
        log(label, 'returned; inputs restored')
    Done(restore)


class Entry(gdb.Breakpoint):
    def __init__(self, spec, name):
        super().__init__(spec, internal=True)
        self.name = name

    def stop(self):
        global FREEZE_THINK
        a = '((CDarkCommando*)%d)->' % int(ev('$rdi'))
        name = self.name
        if name == 'GetDamageVulnerability':
            n = count(name)
            cases = [(1, 0, 0), (0, 1, 0), (0, 0, 1), (0, 0, 0)]
            if n < len(cases):
                temporary(a, dict(zip(['mWarpingIn', 'mWarpingOut', 'mShadowDashing'], cases[n])),
                          '6284 vulnerability %d' % n)
        elif name in ('SelectMeleeVariant', 'ShouldMeleeAttack', 'SetTargetFaceVect',
                      'SelectDashTarget', 'BuildActionChoices', 'LaunchGrenade',
                      'FireChargeBeamShot', 'UpdateAdditiveAim'):
            if count(name) == 0:
                temporary(a, {'mTargetId.value': 65535}, 'null target ' + name)
        elif name == 'GetUp':
            msg = int(ev('$edx'))
            if msg == 1:
                n = count(name)
                if n < 2:
                    temporary(a, {'mAnimationState.mState': 'CAnimationState::kAS_' +
                                 ('Ready' if n == 0 else 'NotReady')}, '46E4 command %d' % n)
            log('46E4 message', str(msg))
        elif name == 'WarpOut':
            msg = int(ev('$edx'))
            log('5B6C message', str(msg))
            if msg == 1:
                n = count(name)
                # Last case really requests deletion. Earlier cases restore the state-id
                # before the body controller sees it, so no fake PAS state is updated.
                cases = [(0, 'Locomotion', 0, 0), (0, 'Locomotion', 1, 1),
                         (0, 'Locomotion', 1, 0), (0, 'Generate', 1, 0),
                         (1, 'Generate', 1, 0), (1, 'Locomotion', 1, 0)]
                if n < len(cases):
                    warp, body, alpha, frozen = cases[n]
                    temporary(a, {'mWarpingOut': warp, 'mCloakAlpha': alpha,
                                  'mBodyController.mPtr->mBodyStateInfo.mState': 'pas::kAS_' + body,
                                  'mBodyController.mPtr->mFrozen': frozen}, '5B6C update %d' % n)
        elif name == 'AcceptScriptMsg':
            msg = int(ev('((CScriptMsg*)$rdx)->m_msg'))
            # Enum names avoid assumptions about script-message ordinals.
            if msg == int(ev('kSM_OffGround')):
                n = count('OffGround')
                if n < 2:
                    temporary(a, {'mBodyController.mPtr->mFrozen': n}, '6A48 frozen %d' % n)
            elif msg == int(ev('kSM_Escape')):
                temporary(a, {'mWarpOutRequested': 0}, '6A48 Escape')
        elif name == 'DoUserAnimEvent':
            event = int(ev('$ecx'))
            if event in (12, 13):
                n = count('event%d' % event)
                if n < 2:
                    temporary(a, {'mShadowDashing': n}, '6748 event %d dash %d' % (event, n))
            elif event == 0:
                n = count('projectile')
                cases = [(0, 0), (1, 0), (0, 1)]
                if n < len(cases):
                    temporary(a, dict(zip(['mFiringEMP', 'mFiringChargeBeam'], cases[n])),
                              '6748 projectile %d' % n)
        elif name == 'Think':
            rag = int(ev(a + 'mRagDoll.mPtr'))
            if rag and bool(ev(a + 'mRagDoll.mPtr->mPrimed')):
                n = count('Think-primed')
                if n < 2:
                    temporary(a, {'mBodyController.mPtr->mFrozen': n}, '68B4 frozen %d' % n)
            elif bool(ev(a + 'mActive')) and count('Think-live') == 2:
                temporary(a, {'mBodyController.mPtr->mFrozen': 1}, '68B4 live frozen')
                FREEZE_THINK = a
                def clear_freeze():
                    global FREEZE_THINK
                    FREEZE_THINK = None
                Done(clear_freeze)
        elif name == 'PreRender':
            n = count(name)
            cases = [(0, 0, 0), (0, 1, 0), (0, 0, 1), (1, 0, 0)]
            if n < len(cases):
                temporary(a, dict(zip(['mFadeToDeath', 'mWarpingIn', 'mWarpingOut'], cases[n])),
                          '6528 visibility %d' % n)
            if int(ev(a + 'mRagDoll.mPtr')) and count('PreRender-rag') == 0:
                temporary(a, {'mRagDoll.mPtr->mPrimed': 0}, '6528 unprimed rag')
        elif name == 'Render':
            n = count(name)
            cases = [(1, 1, 0), (1, 1, 128), (1, 0, 255), (0, 1, 255)]
            if n < len(cases):
                alive, fade, alpha = cases[n]
                temporary(a, {'mAlive': alive, 'mCloakFading': fade, 'mColor.mA': alpha},
                          '6664 render %d' % n)
        return False


class WaterEntry(gdb.Breakpoint):
    def stop(self):
        global WATER
        WATER = int(ev('$rdi'))
        return False


class BaseThink(gdb.Breakpoint):
    def stop(self):
        if FREEZE_THINK and FREEZE_THINK == '((CDarkCommando*)%d)->' % int(ev('$rdi')):
            a = FREEZE_THINK
            def finish():
                put(a + 'mBodyController.mPtr->mFrozen', 1)
                log('68B4 base Think return', 'forced frozen before the bone-tracking gate')
            Done(finish)
        return False


class SfxEntry(gdb.Breakpoint):
    def stop(self):
        global SFX
        n = count('sfx')
        if n >= 4:
            return False
        a = '((CDarkCommando*)%d)->' % int(ev('$rdi'))
        old = int(ev(a + 'mSfxHandle.mID'))
        put(a + 'mSfxHandle.mID', 0 if n == 0 else '0xffffffff')
        SFX = n
        def finish():
            global SFX
            handle = int(ev(a + 'mSfxHandle.mID'))
            expected = 0 if n in (0, 3) else 0xffffffff
            log('16FC case %d' % n, '%s handle=%x' % ('PASS' if handle == expected else 'FAIL', handle))
            put(a + 'mSfxHandle.mID', old)
            SFX = None
        Done(finish)
        return False


class SfxQuery(gdb.Breakpoint):
    def __init__(self, spec, playing):
        super().__init__(spec, internal=True)
        self.playing = playing

    def stop(self):
        if SFX is not None:
            value = int(SFX == (1 if self.playing else 2))
            Done(lambda: put('$eax', value))
        return False


class RagEntry(gdb.Breakpoint):
    def stop(self):
        global ACTIVE_RAG
        a = '((CDarkCommando*)%d)->' % int(ev('$rdi'))
        rag = int(ev(a + 'mRagDoll.mPtr'))
        if not rag or not bool(ev(a + 'mRagDoll.mPtr->mPrimed')):
            return False
        n = count('rag')
        if n >= 7:
            return False
        ACTIVE_RAG = (a, n)
        vals = {'mRagDollTimer': '0.001' if n == 0 else '0.0',
                'mFadeToDeath': int(n == 2), 'mCloakAlpha': '0.5' if n == 3 else '1.0'}
        temporary(a, vals, '17A0 conditions %d' % n)
        # InFluidId returns the last entry. The foreign-id arm is safely forced with
        # the commando itself; an observed live water supplies active/inactive arms.
        fluid = {'mFluidIds.mCount': 1 if n in (1, 4, 5) else 0}
        temporary(a, fluid, '17A0 fluid count %d' % n)
        slot = '*(unsigned short*)&' + a + 'mFluidIds.mData[0]'
        saved_id = int(ev(slot))
        water_id = int(ev(a + 'mUniqueId.value'))
        if n in (4, 5) and WATER:
            w = '((CScriptWater*)%d)->' % WATER
            water_id = int(ev(w + 'mUniqueId.value'))
            temporary(w, {'mActive': int(n == 5)}, '17A0 water active %d' % n)
        put(slot, water_id)
        Done(lambda: put(slot, saved_id))

        def finish():
            global ACTIVE_RAG
            log('17A0 rag %d' % n, 'fade=%d alphaDelta=%s water=%s' %
                (int(ev(a + 'mFadeToDeath')), ev(a + 'mAlphaDelta'), WATER))
            ACTIVE_RAG = None
        Done(finish)
        return False


class RagUpdate(gdb.Breakpoint):
    def stop(self):
        if ACTIVE_RAG is None:
            return False
        a, n = ACTIVE_RAG
        rag = '((CPirateRagDoll*)%d)->' % int(ev('$rdi'))

        def finish():
            # Set the outcome AFTER the real physics update, which writes mOver.
            put(rag + 'mOver', int(n != 0))
            put(rag + 'mContinueSmallMovements', int(n == 1))
            log('17A0 physics return %d' % n, 'forced over/continue flags')
        Done(finish)
        return False


seen = set()
names = {'GetDamageVulnerability', 'SelectMeleeVariant', 'ShouldMeleeAttack', 'SetTargetFaceVect',
         'SelectDashTarget', 'BuildActionChoices', 'LaunchGrenade', 'FireChargeBeamShot',
         'UpdateAdditiveAim', 'GetUp', 'WarpOut', 'AcceptScriptMsg', 'DoUserAnimEvent',
         'Think', 'PreRender', 'Render'}
for line in open(os.environ['COV_NM']):
    raw, _, demangled = line.rstrip().partition('\t')
    parts = raw.split()
    if len(parts) != 3 or '.cold' in demangled or 'clone' in demangled or parts[0] in seen:
        continue
    prefix = demangled.split('(')[0]
    symbol = '*' + parts[2]
    if prefix.startswith('CDarkCommando::') and prefix.split('::')[1] in names:
        Entry(symbol, prefix.split('::')[1])
    elif prefix == 'CDarkCommando::ThinkRagDoll':
        RagEntry(symbol, internal=True)
    elif prefix == 'CPirateRagDoll::Update':
        RagUpdate(symbol, internal=True)
    elif prefix == 'CScriptWater::Think':
        WaterEntry(symbol, internal=True)
    elif prefix == 'CScriptWater::CScriptWater':
        WaterEntry(symbol, internal=True)
    elif prefix == 'CPatterned::Think':
        BaseThink(symbol, internal=True)
    elif prefix == 'CDarkCommando::UpdateSfx':
        SfxEntry(symbol, internal=True)
    elif prefix in ('CSfxManager::IsPlaying', 'CSfxManager::IsQueued'):
        SfxQuery(symbol, prefix.endswith('IsPlaying'))
    else:
        continue
    seen.add(parts[0])
