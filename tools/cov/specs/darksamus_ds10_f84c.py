# DarkSamus fn_10_F84C (GetCollisionResponseType, vtable slot 24) under real player shots.
# Run: Sanctuary 10_cliff_rooftop LAYER2@400 with tools/drivers/darksamus_ds10_shots.con; the key
# is the 0xCD0 shield byte the response reads (0 -> 0x2E, 1 -> 0x6A); the second entry names
# the actor each projectile hit (the boss is u04f7 = 1271).
CLASS = 'CDarkSamus'
SPEC = [
    ('F84C GetCollisionResponseType', 'CDarkSamus::GetCollisionResponseType(',
     ['$this->mGuestCD0'], 'max=4000'),
    ('ResolveCollisionWithActor (any)', 'CEnergyProjectile::ResolveCollisionWithActor(',
     ['((CEntity*)$rdx)->mUniqueId.value', '((CEntity*)$rdi)->mUniqueId.value'], 'max=4000'),
]
