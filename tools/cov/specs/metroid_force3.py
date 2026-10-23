# Metroid (module 40) forcing spec, third run: SelectTarget's Two state with a target id that is no
# object (the actor cast's false side; in play the target is a player or a pirate and exists until
# it is deleted, and the Delete path re-targets first), and GetGrowthStage's third return, forced
# through GetDamageMultiplier's entry because that body asks for the stage on every damage event
# (the growth energy itself cannot rise far: these metroids have 1 hp).
CLASS = 'CMetroid'

FORCE = [
    ('4E44 Two, target gone', 'CMetroid::SelectTarget(',
     ['$this->mState', '$this->mAttackTarget.value'],
     ['$this->mAttackTarget.value = $this->mState == 2 ? 0x0fff : $this->mAttackTarget.value'], 0,
     'the Two state target is an id no object has: the actor cast fails'),
]
