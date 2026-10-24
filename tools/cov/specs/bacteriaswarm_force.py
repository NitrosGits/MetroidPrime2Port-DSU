# BacteriaSwarm (module 6) forcing spec for tools/cov/force.sh, same area and driver as
# tools/cov/specs/bacteriaswarm.py (merge the two gcov dirs in gcov_report.py):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/bacteriaswarm_force.py <out> \
#     3bfa3eff,04_temple_unknownenemies_dark --con tools/cov/specs/bacteriaswarm_temple.con --fast --frames 3200
CLASS = 'CBacteriaSwarm'

# bac1: ApplySteeringBehaviors clears the boid's three flags on entry, so the `if (mInSafeZone ||
# mTouchingSafeZone) break` arm (0x4568/0x4570) can only be forced from inside the steering loop:
# ApplyBoundsAvoidance is case 3's callee, and its `boid` argument is $rsi (x86-64 SysV, arg 2).
# Set one of the two flags, alternating per hit so both break arms run. Forcing them also takes
# UpdateBoid's mInSafeZone arms (364, 373).
_steer_hits = [0]


def force_steer_flags(m):
    # the member names, not retail's +0x81: the host layout differs from retail (host mSpeed 0x80,
    # the bitfield unit 0x89), so the flag byte is written through the type.
    _steer_hits[0] += 1
    on = 1 if (_steer_hits[0] // 2) % 2 == 0 else 0
    m.execute('set var ((CBacteriaSwarm::CBoid*)$rsi)->mInSafeZone = %d' % on)
    m.execute('set var ((CBacteriaSwarm::CBoid*)$rsi)->mTouchingSafeZone = %d' % (1 - on))


FORCE = [
    ('1E08 steering break bits', 'CBacteriaSwarm::ApplyBoundsAvoidance(', [], [force_steer_flags],
     0, 'boid mInSafeZone (0x8) / mTouchingSafeZone (0x4) alternating: the ApplySteeringBehaviors '
        'break arms and UpdateBoid 364/373'),
    ('3570 Think create all', 'CBacteriaSwarm::Think(', [],
     ['$this->mMaxCreatedBoids = 0', '$this->mBoidGenCooldownTimer = -1000.0'], 600,
     'mMaxCreatedBoids 0 and the gen timer very negative: the creation loop runs to the "no '
     'inactive boid" arm (496 mMaxCreatedBoids==0, 499 skip, 508 empty pass)'),
    ('3A8 AcceptScriptMsg spawnInstantly', 'CBacteriaSwarm::AcceptScriptMsg(', [],
     ['$this->mSpawnInstantly = 1'], 0,
     'mSpawnInstantly set (every shipped BSWM has 0): the AreaLoaded spawn loop runs'),
    ('touch radius', 'CBacteriaSwarm::Touch(', [], ['$this->mPlayerTouchRadius = 0.0'], 0,
     'mPlayerTouchRadius 0: Touch falls back to mTouchRadius'),
    ('12E0 KillBoid chances', 'CBacteriaSwarm::KillBoid(', [],
     ['$xmm0.v4_float[0] = 0.0', '$xmm1.v4_float[0] = 0.0'], 0,
     'deathRattleChance and deadChance 0 (Touch passes 1.0): neither message is sent'),
    ('3570 Think particle gens', 'CBacteriaSwarm::Think(', [],
     ['$this->mDeathParticleGenerator.mPtr = 0', '$this->mBoidParticleGenerator.mPtr = 0'], 2600,
     'both particle generators null from the 2601st Think (leaked): the no-generator arms'),
]
