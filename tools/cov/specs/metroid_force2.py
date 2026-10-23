# Metroid (module 40) forcing spec, second run (tools/cov/force.sh + COV_GDB_SCRIPT=cov_force.py,
# BUILD=build-port-cov): SelectTarget's Two state with no target, and GetGrowthStage's third
# return.  In Two the body reads mAttackTarget (0x4E44); the metroid's target is otherwise only a
# player or a pirate, so "no target" needs the member written while mState is kAiState_Two (2).
# mGrowthEnergy cannot pass the explosion threshold in play: the shipped metroids have 1 hp and
# die at the first hit.
CLASS = 'CMetroid'

FORCE = [
    ('4E44 Two, no target', 'CMetroid::SelectTarget(',
     ['$this->mState', '$this->mAttackTarget.value'],
     ['$this->mAttackTarget.value = $this->mState == 2 ? 0xffff : $this->mAttackTarget.value'], 0,
     'the Two state has no target: the SetTargetVector arm is skipped'),
]
