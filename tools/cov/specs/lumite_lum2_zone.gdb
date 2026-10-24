# lum2 zone: every safe zone of the area (the driver's <prefix>.zones, hurtful or not) with its active flag as shipped and
# forced off, IsHurtful forced false, and the Lumite itself (not a zone: the null cast) as the surface's zone id, with the
# safe-zone surface kind: DamageSafeZoneOccupant, SpawnZoneImpact, ReadyToMove (the null / inactive zone arm).
set $t = (CLumite*)$rdi
set $m = (CStateManager*)$rsi
set $lm_kind = $t->mSurface.mKind
set $lm_zid = $t->mSurface.mZoneId.value
set $lm_v = (float*)malloc(64)
set $lm_ib = (unsigned short*)malloc(16)
set var $lm_v[0] = $t->mPosition.mX
set var $lm_v[1] = $t->mPosition.mY
set var $lm_v[2] = $t->mPosition.mZ
python
import os
zs = []
try:
    zs = [int(x) for x in open(os.environ['LUM2_FILES'] + '.zones').read().split()]
except Exception as e:
    print('no zones file', e)
def calls(z):
    gdb.execute('set var $t->mSurface.mKind = 2')
    gdb.execute('set var $t->mSurface.mZoneId.value = %d' % z)
    gdb.execute('call $t->DamageSafeZoneOccupant(*$m, 0.016f)')
    gdb.execute('call $t->ReadyToMove(*$m, *(CTriggerData*)$lm_v)')
    gdb.execute('set var $t->mSurface.mZoneId.value = %d' % z)
    gdb.execute('call $t->SpawnZoneImpact(*$m, *(CVector3f*)$lm_v, 1.0f)')
for z in zs:
    gdb.execute('set var $lm_ib[0] = %d' % z)
    zp = gdb.parse_and_eval('(CScriptSafeZone*)$m->ObjectById(*(TUniqueId*)$lm_ib)')
    zq = '((CScriptSafeZone*)%d)' % int(zp)
    a0 = int(zp['mActive'])
    calls(z)
    for hv in (0, 1):
        gdb.execute('set $lm_rf_hurt = %d' % hv)
        calls(z)
    gdb.execute('set $lm_rf_hurt = -1')
    gdb.execute('set var %s->mActive = 0' % zq)
    calls(z)
    gdb.execute('set var %s->mActive = %d' % (zq, a0))
calls(int(gdb.parse_and_eval('$t->mUniqueId.value')))
print('zone calls', len(zs))
end
set var $t->mSurface.mKind = $lm_kind
set var $t->mSurface.mZoneId.value = $lm_zid
printf "zone done\n"
