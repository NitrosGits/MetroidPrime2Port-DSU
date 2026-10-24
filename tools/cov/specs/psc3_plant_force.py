# PlantScarabSwarm (module 49) psc3 targeted force for tools/cov/force.sh (force.py, persistent
# writes), picking up where psc1/psc2 stopped.
#
# Measured first (read-only probe, same driver, 0g_swamp_hall): the ctor does build the shared attack
# model (attackerCount 3, attackModels 3, modelDatas 4, sharedModel/sharedState/sharedDeltas 1,
# mSharedAttackBit 7, mAttackModelBitBase 4) and the draw mask carries the shared bit on every one of
# 1914 PreRenderBoid calls, so the gate psc1/psc2 blamed (the bit) is never the missing one.  What
# never happens is an attacking boid with mAttackSlot == -1 *at draw time*: retail reaches it only in
# the one tick between UpdateBoid releasing a finished slot (cpp:196-199, mAttackSlot = -1, mAttacking
# still set) and the next tick's kill, so it needs a render tick inside a one-tick window.
#
# The write below is the real boid argument ($rsi in PreRenderBoid; rdx is the draw mask) and only its
# two attack fields: no mask write at all (`uint drawMask = -1` in CSwarmBasics::PreRender, cpp:1027,
# so the shared bit is already set; the arm's own clear acts on that stack local).  The write is
# guarded (shared model and skinned state non-null, boid active, not attacking, slot -1) and made at
# most twice: once while the mask still carries mAttackModelBitBase (cpp:218's true arm) and once
# after an earlier attack boid in the same pass cleared it (cpp:218's false arm).  Each forced boid
# goes through retail's own next-tick path: UpdateBoid's `else if (boid.mAttacking)` -> KillBoid.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/psc3_plant_force.py <out> \
#     3dfd2249,ok_swamp_hall --con tools/cov/specs/psc3_plant_ok.con --fast --frames 1200
import os

CLASS = 'CPlantScarabSwarm'
P = '((CPlantScarabSwarm*)$rdi)'
B = '((CSwarmBasics::CBoid*)$rsi)'
DIAG = os.path.join(os.environ.get('COV_OUT', '/tmp'), 'diag.log')
seen = []
wrote = {'set': 0, 'clear': 0}


def log(msg):
    with open(DIAG, 'a') as fh:
        fh.write(msg + '\n')


def ev(expr, gdb):
    try:
        return int(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return 'err:' + str(e)[:40]


def think_diag(gdb):
    # what the ctor built, once per object (keyed on its own bit numbers and model counts)
    key = (ev(P + '->mSharedAttackBit', gdb), ev(P + '->mAttackModels.mCount', gdb),
           ev(P + '->mModelDatas.mCount', gdb))
    if ('think',) + key in seen:
        return
    seen.append(('think',) + key)
    log('Think model: attackerCount=%s attackModels=%s modelDatas=%s sharedModel=%s '
        'sharedState=%s sharedDeltas=%s sharedBit=%s bitBase=%s' %
        (ev(P + '->mAttackerCount', gdb), ev(P + '->mAttackModels.mCount', gdb),
         ev(P + '->mModelDatas.mCount', gdb), ev(P + '->mSharedAttackModel.mPtr != 0', gdb),
         ev(P + '->mSharedAttackState.mPtr != 0', gdb),
         ev(P + '->mSharedAttackDeltas.mPtr != 0', gdb), ev(P + '->mSharedAttackBit', gdb),
         ev(P + '->mAttackModelBitBase', gdb)))


def prerender_arm(gdb):
    active = ev(B + '->mActive', gdb)
    attacking = ev(B + '->mAttacking', gdb)
    slot = ev(B + '->mAttackSlot', gdb)
    if attacking == 1:
        key = ('attacking', slot,
               ev('(*(unsigned int*)$rdx & (1u << ((CPlantScarabSwarm*)$rdi)->mSharedAttackBit)) != 0', gdb),
               ev('(*(unsigned int*)$rdx & (1u << ((CPlantScarabSwarm*)$rdi)->mAttackModelBitBase)) != 0', gdb))
        if key not in seen:
            seen.append(key)
            log('PreRenderBoid attacking boid: active=%s attackSlot=%s maskSharedBit=%s maskBitBase=%s' %
                (active, slot, key[2], key[3]))
    if active != 1 or attacking != 0 or slot != -1:
        return
    model = ev(P + '->mSharedAttackModel.mPtr != 0', gdb)
    state = ev(P + '->mSharedAttackState.mPtr != 0', gdb)
    if model != 1 or state != 1:
        if ('skip',) not in seen:
            seen.append(('skip',))
            log('slot -1 arm: not forced, shared model/state missing (model=%s state=%s)' % (model, state))
        return
    bit = ev(P + '->mAttackModelBitBase', gdb)
    which = 'set' if ev('(*(unsigned int*)$rdx & (1u << ((CPlantScarabSwarm*)$rdi)->mAttackModelBitBase)) != 0', gdb) else 'clear'
    if wrote[which]:
        return
    wrote[which] += 1
    log('slot -1 arm: boid $rsi active, not attacking, slot -1; shared model+state present; '
        'mAttackModelBitBase bit in the mask = %s -> forcing this call (mask untouched)' % (bit,))
    try:
        gdb.execute('set var ((CSwarmBasics::CBoid*)$rsi)->mAttacking = 1')
        gdb.execute('set var ((CSwarmBasics::CBoid*)$rsi)->mAttackSlot = -1')
        log('slot -1 arm: wrote attacking=1 attackSlot=-1 (%s pass)' % which)
    except gdb.error as e:
        log('slot -1 arm: write failed: %s' % str(e)[:80])


FORCE = [
    ('1B64 Think model', 'CPlantScarabSwarm::Think(', [], [think_diag], 0,
     'read-only: the ctor-built attack model state, once per object'),
    ('12AC PreRenderBoid slot -1', 'CPlantScarabSwarm::PreRenderBoid(', [], [prerender_arm], 0,
     'read-only record + guarded boid state write for cpp:218/219/223 (both arms of the bit test)'),
]
