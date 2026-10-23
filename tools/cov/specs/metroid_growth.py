# Metroid (module 40) coverage: ApplyDamageGrowth (fn_40_3FC0) and PreDamageSpacePirate (fn_40_4300).
# Run: tools/cov/run.sh tools/cov/specs/metroid_growth.py <out> 42b935e4,08_sand_casualpirates
#      --con tools/cov/specs/metroid_casualpirates.con --fast
CLASS = 'CMetroid'
SPEC = [
    ('3FC0 ApplyDamageGrowth', 'CMetroid::ApplyDamageGrowth(', [], 'calls'),
    ('4300 PreDamageSpacePirate', 'CMetroid::PreDamageSpacePirate(', [], 'ret calls'),
    ('AttachToTarget', 'CMetroid::AttachToTarget(', [], 'ret'),
    ('ApplyGrowth', 'CMetroid::ApplyGrowth(', ['$this->mGrowthEnergy'], ''),
    ('MassiveDeath', 'CPatterned::MassiveDeath(', [], ''),
]
SPEC += [
    ('E0D8 pirate attach', 'CSpacePirate::PortAttachActor(',
     ['$this->mXDamageThreshold', '$this->mHealthInfo.mHealth'], 'ret'),
    ('ApplyDamage', 'CStateManager::ApplyDamage(', ['$rsi & 0xffff', '$rdx & 0xffff'], 'max=4000'),
]
