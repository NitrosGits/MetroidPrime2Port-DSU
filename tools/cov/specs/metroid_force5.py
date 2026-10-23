# Metroid (module 40) forcing spec, fifth run: GetGrowthStage's third return.  The body is a real
# call only from GetDamageMultiplier (the drain in SuckEnergyFromTarget inlines it), so the growth
# energy is written at GetDamageMultiplier's entry; the metroid drains the god-mode player, which
# asks for the multiplier every frame.
CLASS = 'CMetroid'

FORCE = [
    ('44EC growth stage 3 via multiplier', 'CMetroid::GetDamageMultiplier(',
     ['(int)$this->mGrowthEnergy'], ['$this->mGrowthEnergy = 1000000.f'], 0,
     'the growth energy is past the explosion threshold: GetGrowthStage third return'),
]
