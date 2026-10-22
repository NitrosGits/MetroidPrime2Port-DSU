# split24 forced calls on the beam effect, run once at a CSplitterBeamEffect::Think entry
# (tools/cov/specs/splitter_split24.py).  Every member written here is written back after the call.
import os
import gdb

_LOG = os.path.join(os.environ['COV_OUT'], 'split24_calls.log')


def log(msg):
    with open(_LOG, 'a') as fh:
        fh.write(msg + '\n')


def run(cmd):
    try:
        out = gdb.execute(cmd, to_string=True)
        log('%s -> %s' % (cmd, out.strip()[:160]))
    except gdb.error as e:
        log('%s FAILED %s' % (cmd, str(e)[:160]))


def ev(expr):
    return gdb.parse_and_eval(expr)


gdb.execute('disable %d' % int(gdb.convenience_variable('s24_beambp')))
gdb.execute('set $s24b = $rdi')
gdb.execute('set $s24bm = (CStateManager*)$rsi')
B = '((CSplitterBeamEffect*)$s24b)'
M = '*$s24bm'
log('split24 beam: %s uid %s' % (ev('$s24b'), ev(B + '->mUniqueId.value')))
BS = ['((CEntity*)%s)->mActive' % B, B + '->mMaxAngle', B + '->mOpenFraction', B + '->mAngle',
      B + '->mOpening', B + '->mOutOfFrustum', B + '->mRenderBoundsDirty']
old = []
for f in BS:
    v = ev(f)
    old.append((f, repr(float(v)) if v.type.strip_typedefs().code == gdb.TYPE_CODE_FLT else str(int(v))))
try:
    # fn_75_10DC4: inactive; a negative full angle (the wrap's below-zero arm).
    run('set var ((CEntity*)%s)->mActive = 0' % B)
    run('call %s->Think(0.016, %s)' % (B, M))
    run('set var ((CEntity*)%s)->mActive = 1' % B)
    run('set var %s->mMaxAngle = -3' % B)
    run('set var %s->mOpening = 1' % B)
    run('set var %s->mOpenFraction = 1' % B)
    run('call %s->Think(0.016, %s)' % (B, M))
    log('beam angle %s' % ev(B + '->mAngle'))
    # fn_75_10EF0: clipped.  fn_75_1167C: render bounds dirty and clean.
    run('set var %s->mOutOfFrustum = 1' % B)
    run('call %s->AddToRenderer(%s)' % (B, M))
    run('set var %s->mRenderBoundsDirty = 1' % B)
    run('call %s->PreRenderAllViewports(%s)' % (B, M))
    run('set var %s->mRenderBoundsDirty = 0' % B)
    run('call %s->PreRenderAllViewports(%s)' % (B, M))
finally:
    for f, v in old:
        run('set var %s = %s' % (f, v))
log('split24 beam: done')
