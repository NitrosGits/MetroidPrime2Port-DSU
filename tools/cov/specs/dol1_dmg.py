# dol1: DOL damage holes (CBloggProjectile::ApplyDamageToOneActor fn_80213508,
# CDamageInfo::WithNoImmunity fn_800B5F8C). Drivers: tools/drivers/dol1_blogg.con (Torvus
# 07_swamp_shaft), tools/drivers/dol1_parasite.con (Agon 01_sand_entrance).
CLASS = 'CBloggProjectile'
SPEC = [
    ('80213508 Blogg ApplyDamageToOneActor', 'CBloggProjectile::ApplyDamageToOneActor(', [], ''),
    ('CGameProjectile ApplyDamageToOneActor', 'CGameProjectile::ApplyDamageToOneActor(', ['$rcx'], ''),
    ('800B5F8C WithNoImmunity', 'CDamageInfo::WithNoImmunity(', ['$rsi->mDamage'], 'max=100'),
    ('CStateManager ApplyDamage', 'CStateManager::ApplyDamage(', [], 'max=100'),
]
