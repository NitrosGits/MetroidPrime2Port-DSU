# Shredder (module 68) forcing spec for tools/cov/force.sh, same area as tools/cov/specs/shredder.py
# (merge the gcov dirs in gcov_report.py):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/shredder_force.py <out> \
#     3dfd2249,01_swamp_entrance --con tools/cov/specs/shredder_force.con --frames 800
# Every shipped Shredder of the two Torvus rooms has start state 1 or 0 and wakes only on Alert, a
# shot or a nearby explosion; the forces are per object (u04e6 = A, u04e3 = B of the driver).
CLASS = 'CShredder'
FORCE = [
    ('1728 Generate start states', 'CShredder::Generate(', ['$rdx', '$this->mStartState'],
     ['$this->mStartState = $this->mUniqueId.value == 0x4e3 ? 2 : '
      '($this->mUniqueId.value == 0x4e6 ? 0 : $this->mStartState)'], 0,
     'start state 2 for B, 0 for A: the kGType_Four and kGType_Zero generate commands'),
    ('12E8 Think crouch lurk', 'CShredder::Think(', ['$this->mStartState'],
     ['$this->mStartState = $this->mUniqueId.value == 0x4e3 ? 2 : $this->mStartState'], 0,
     'start state 2 for B, which lurks in the pool: the kLT_Crouch locomotion'),
    ('0F80 ShouldWakeUp detection', 'CShredder::ShouldWakeUp(', [],
     ['$this->mDetectionRange = $this->mUniqueId.value == 0x4e6 ? 1000.0 : $this->mDetectionRange',
      '$this->mDetectionHeightRange = $this->mUniqueId.value == 0x4e6 ? 1000.0 : $this->mDetectionHeightRange',
      '$this->mDetectionAngle = $this->mUniqueId.value == 0x4e6 ? -1.0 : $this->mDetectionAngle'], 0,
     'A detects the player (range, height 1000, any angle): ShouldWakeUp by InDetectionRange'),
    ('14F0 AttackPlayer desired distance', 'CShredder::AttackPlayer(', ['$rdx', '$this->mDesiredDistance'],
     ['$this->mDesiredDistance = $this->mDesiredDistance == 1000.0 ? 6.5 : 1000.0'], 0,
     'desired distance 1000 and 6.5 in turn, A parked 6 from the player: back off, and hold'),
]
