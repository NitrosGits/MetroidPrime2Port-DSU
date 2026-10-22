# DarkSamus boost ball actor (dsball): the ball class CDarkSamusBoostBall (vtable lbl_10_data_104)
# and its spawn fn_10_9E00 / the roll state fn_10_A3B0 that reads it.
#   COV_GDB_SCRIPT=tools/cov/cov_force.py MP2_INPUT=LAYER2@400 SECS=1500 tools/cov/force.sh \
#     tools/cov/specs/darksamus_ball.py <out> "1baa96c2,10_cliff_rooftop" \
#     --con tools/drivers/darksamus_ball.con --fast --frames 2600
# (tools/cov/run.sh with the same arguments, plus --env MP2_INPUT=LAYER2@400, runs it unforced.)
# The `calls` flag counts each callee, which is how the arms that end in a call show. `$this` is
# the ball, so the Dark Samus's own bodies cast $rdi. The driver's console steps take the ball's
# message arms: `damage` on the ball's collision actor (u0703, the uid of the first ball's in
# every run so far) sends kSM_Damage through it (a dark burst: the reflect vulnerability takes only
# the charged and combo shots), `send` to the ball (u0702) a kSM_HitObject and a
# kSM_Damage from no sender (the null-cast arms). With 3000 the DS drops to 1 hp (the
# "damage >= hp - 1" arm of fn_10_A3B0; a run with 50 instead took the "< hp - 1" arm, 1500 ->
# 1450), and `visor scan` at f1700 runs the silhouette pre-render with a non-combat visor.
# FORCE: the 1st spawn (fn_10_9E00) sees 0xD14 = 0.5 (the fn_10_3BC4 silhouette ball, its
# render / pre-render arms), and the first kSM_Delete finds 0x354 null (the arm a ball deleted
# before its first Think takes; the manager is leaked for the rest of the run).
CLASS = 'CDarkSamusBoostBall'
SPEC = [
    ('9E00 SpawnBoostBall', 'CDarkSamus::SpawnBoostBall(', ['((CDarkSamus*)$rdi)->mGuestD14 == 0'], 'calls'),
    ('A3B0 BoostBallRoll', 'CDarkSamus::BoostBallRoll(', ['$rdx', '((CDarkSamus*)$rdi)->mBoostBallFlag20'], 'calls max=3000'),
    ('3FD8/3BC4 ctor', 'CDarkSamusBoostBall::CDarkSamusBoostBall(', [], 'calls'),
    ('2E64 dtor', 'CDarkSamusBoostBall::~CDarkSamusBoostBall(', [], 'calls'),
    ('2EFC GetDamageTaken', 'CDarkSamusBoostBall::GetDamageTaken(', [], 'calls'),
    ('2F04 HasBeenDamaged', 'CDarkSamusBoostBall::HasBeenDamaged(', [], 'ret max=3000'),
    ('2F10 ApplyTouchDamage', 'CDarkSamusBoostBall::ApplyTouchDamage(', ['$this->mHitCooldown > 0'], 'calls'),
    ('3008 Render', 'CDarkSamusBoostBall::Render(', ['$this->mSilhouette'], 'calls max=3000'),
    ('3174 PreRender', 'CDarkSamusBoostBall::PreRender(', ['$this->mSilhouette'], 'calls max=3000'),
    ('31E4 CreateCollisionActors', 'CDarkSamusBoostBall::CreateCollisionActors(', ['$this->mCollisionManager == 0'], 'calls'),
    ('3B08 Think', 'CDarkSamusBoostBall::Think(', ['$this->mCollisionManager == 0', '$this->mHitCooldown > 0'], 'calls max=20000'),
    ('3998 AcceptScriptMsg', 'CDarkSamusBoostBall::AcceptScriptMsg(', ['((CScriptMsg*)$rdx)->m_msg', '$this->mCollisionManager == 0'], 'calls max=20000'),
    ('3934 GetOrbitPosition', 'CDarkSamusBoostBall::GetOrbitPosition(', [], 'calls max=3000'),
    ('38D0 GetAimPosition', 'CDarkSamusBoostBall::GetAimPosition(', [], 'calls max=3000'),
]

MSG = '((CScriptMsg*)$rdx)->m_msg'
FORCE = [
    ('9E00 silhouette', 'CDarkSamus::SpawnBoostBall(', ['((CDarkSamus*)$rdi)->mGuestD14 == 0'],
     ['$dsb_n = $_isvoid($dsb_n) ? 1 : $dsb_n + 1',
      '((CDarkSamus*)$rdi)->mGuestD14 = ($dsb_n == 1) ? 0.5 : ((CDarkSamus*)$rdi)->mGuestD14'],
     0, '0xD14 = 0.5 on the 1st spawn'),
    ('3998 delete null', 'CDarkSamusBoostBall::AcceptScriptMsg(', [MSG],
     ['$dsb_del = $_isvoid($dsb_del) ? 0 : $dsb_del',
      '$this->mCollisionManager = (%s == 0x5844454c && $dsb_del == 0) ? (CCollisionActorManager*)0 : $this->mCollisionManager' % MSG,
      '$dsb_del = (%s == 0x5844454c) ? $dsb_del + 1 : $dsb_del' % MSG],
     0, 'the first kSM_Delete with 0x354 null'),
]
