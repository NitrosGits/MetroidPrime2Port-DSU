# Metroid (module 40) forcing spec for tools/cov/force.sh: the arms no run reaches.  Run with
# COV_GDB_SCRIPT=tools/cov/cov_force.py BUILD=build-port-cov, so the forced hits land in the
# coverage profile tools/cov/gcov_report.py reads, merged with the natural runs' counters.
#
# PreDamageSpacePirate (fn_40_4300): the metroid's target is always a player or a pirate
# (FindNearestTarget only ever returns those), so the "there is no target" and "the target is not
# a pirate" returns need the member written.  Two hits: the first writes kInvalidUniqueId, the
# later ones an id no object has.
#
# GetGrowthStage's third return is not here: hooking its own entry never fired (the drain inlines
# it), so it went through GetDamageMultiplier in metroid_force5.py.
#
# SwarmRemove (fn_40_3E88): its three early-outs need mTeamAiManagerId invalid, an id no object
# has, and another team's manager that does not hold this metroid (u0554; the metroids send Play
# to u053e).  One hit each, then the natural runs' true arms complete the set.
CLASS = 'CMetroid'

FORCE = [
    ('4300 no target', 'CMetroid::PreDamageSpacePirate(',
     ['$this->mAttackTarget.value'], ['$this->mAttackTarget.value = 0xffff'], 0,
     'the metroid has no attack target: the body returns false at once'),
    ('4300 not a pirate', 'CMetroid::PreDamageSpacePirate(',
     ['$this->mAttackTarget.value'], ['$this->mAttackTarget.value = 0x0fff'], 2,
     'the target is an id no object has: the pirate cast fails'),
    ('3E88 no team', 'CMetroid::SwarmRemove(',
     ['$this->mTeamAiManagerId.value'], ['$this->mTeamAiManagerId.value = 0xffff'], 0,
     'there is no team manager: the body does nothing'),
    ('3E88 no manager', 'CMetroid::SwarmRemove(',
     ['$this->mTeamAiManagerId.value'], ['$this->mTeamAiManagerId.value = 0x0fff'], 3,
     'the team id has no object: the cast fails'),
    ('3E88 not a member', 'CMetroid::SwarmRemove(',
     ['$this->mTeamAiManagerId.value'], ['$this->mTeamAiManagerId.value = 0x0554'], 6,
     'the other team does not hold this metroid: it is not asked to quit'),
]
