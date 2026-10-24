# lum2 trig: IsTouchingSunTrigger with the Lumite moved onto each plain CScriptTrigger of the area (the driver's
# <prefix>.trigs) and its flags / active written: LumiteSunlight set (overlap true), LumiteSunlight clear, inactive.
set $t = (CLumite*)$rdi
set $m = (CStateManager*)$rsi
set $lm_ib = (unsigned short*)malloc(16)
set $lm_px = $t->mPosition.mX
set $lm_py = $t->mPosition.mY
set $lm_pz = $t->mPosition.mZ
python
import os
ts = []
try:
    ts = [int(x) for x in open(os.environ['LUM2_FILES'] + '.trigs').read().split()]
except Exception as e:
    print('no trigs file', e)
for u in ts:
    gdb.execute('set var $lm_ib[0] = %d' % u)
    tp = gdb.parse_and_eval('(CScriptTrigger*)$m->ObjectById(*(TUniqueId*)$lm_ib)')
    tq = '((CScriptTrigger*)%d)' % int(tp)
    f0, a0 = int(tp['mFlags']), int(tp['mActive'])
    mn, mx, pos = tp['mBounds']['min'], tp['mBounds']['max'], tp['mPosition']
    for axis in ('X', 'Y', 'Z'):
        c = (float(mn['m' + axis]) + float(mx['m' + axis])) * 0.5 + float(pos['m' + axis])
        gdb.execute('set var $t->mPosition.m%s = %r' % (axis, c))
    sun = int(gdb.parse_and_eval('kTFL_LumiteSunlight'))
    for flags, act in ((f0 | sun, 1), (f0 & ~sun, 1), (f0 | sun, 0)):
        gdb.execute('set var %s->mFlags = %d' % (tq, flags))
        gdb.execute('set var %s->mActive = %d' % (tq, act))
        r = gdb.parse_and_eval('$t->IsTouchingSunTrigger(*$m)')
        print('trigger %d flags %x active %d -> %s' % (u, flags, act, r))
        if act == 1 and flags & sun:
            gdb.execute('set $lm_rf_ovl = 0')
            gdb.parse_and_eval('$t->IsTouchingSunTrigger(*$m)')
            gdb.execute('set $lm_rf_ovl = -1')
    gdb.execute('set var %s->mFlags = %d' % (tq, f0))
    gdb.execute('set var %s->mActive = %d' % (tq, a0))
print('trig calls', len(ts))
end
set var $t->mPosition.mX = $lm_px
set var $t->mPosition.mY = $lm_py
set var $t->mPosition.mZ = $lm_pz
printf "trig done\n"
