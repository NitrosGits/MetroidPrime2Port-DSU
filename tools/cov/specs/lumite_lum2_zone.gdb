# lum2 zone: every safe zone of the area (the driver's <prefix>.zones, active and inactive, hurtful or not) and the
# Lumite itself (not a zone: the null cast) as the surface's zone id, with the safe-zone surface kind:
# DamageSafeZoneOccupant, SpawnZoneImpact, ReadyToMove (the null / inactive zone arm).
set $t = (CLumite*)$rdi
set $m = (CStateManager*)$rsi
set $lm_kind = $t->mSurface.mKind
set $lm_zid = $t->mSurface.mZoneId.value
set $lm_v = (float*)malloc(64)
set var $lm_v[0] = $t->mTransform.m03
set var $lm_v[1] = $t->mTransform.m13
set var $lm_v[2] = $t->mTransform.m23
python
import os
zs = []
try:
    zs = [int(x) for x in open(os.environ['LUM2_FILES'] + '.zones').read().split()]
except Exception as e:
    print('no zones file', e)
zs.append(int(gdb.parse_and_eval('$t->mUniqueId.value')))
for z in zs:
    gdb.execute('set var $t->mSurface.mKind = 2')
    gdb.execute('set var $t->mSurface.mZoneId.value = %d' % z)
    gdb.execute('call $t->DamageSafeZoneOccupant(*$m, 0.016f)')
    gdb.execute('call $t->ReadyToMove(*$m, *(CTriggerData*)$lm_v)')
    gdb.execute('set var $t->mSurface.mZoneId.value = %d' % z)
    gdb.execute('call $t->SpawnZoneImpact(*$m, *(CVector3f*)$lm_v, 1.0f)')
print('zone calls', len(zs))
end
set var $t->mSurface.mKind = $lm_kind
set var $t->mSurface.mZoneId.value = $lm_zid
printf "zone done\n"
