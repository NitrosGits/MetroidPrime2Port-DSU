# tail24: does `struggle 1` reach CTryclops::Think:212's right arm?
# Run: tools/cov/run.sh tools/cov/specs/tail24_struggle.py <out> 1baa96c2,42c85ad9 \
#        --con tools/drivers/tail24_struggle.con --fast --frames 2600 --env GODMODE=1
# Result: AttachActorToPlayer 14, Think -> GetAttachedActorStruggle 380, BallCloseToCollision 8.
CLASS = 'CTryclops'
SPEC = [
    ('212 Think', 'CTryclops::Think(', [], 'calls max=20000'),
    ('900 BallCloseToCollision', 'CTryclops::BallCloseToCollision(', [], 'calls max=20000'),
    ('AttachActorToPlayer', 'CPlayer::AttachActorToPlayer(', [], 'max=200'),
    ('Dizzy', 'CTryclops::Dizzy(', [], 'max=20000'),
]
