# sp14 (2026-10-23): the SpacePirate ragdoll arms shipped play does not reach, forced one call at
# a time (COV_GDB_SCRIPT=tools/cov/force_ret.py: every write is restored when the body returns,
# except the '!' ones the body itself clears). The arms come from the coverage build's gcov
# profile (tools/cov/gcov_report.py), merged with the natural runs of spacepirate_ragdoll.py.
# Run (BUILD=build-port-cov):
#   COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh tools/cov/specs/spacepirate_ragdoll_force.py
#     <out> 42b935e4,08_sand_casualpirates --driver 'SPR_WATER_OFF=1 python3
#     tools/drivers/spacepirate_ragdoll_drive.py {port} - 30' --fast --frames 14000
#   (the driver deactivates the area's waters once the corpses float: Think's inactive-water arm),
#   with SPR_FORCE=water; and SPR_FORCE=getragdoll in 42b935e4,07_sand_alamoportalobservation
#   (driver layer 3, SPR_SHOTS=10), where the shots reach the corpses (GetRagDoll's arms).
import os

CLASS = 'CSpacePirate'
# SPR_FORCE=water keeps the ragdolls primed (no GetRagDoll forcing) for the casualpirates water
# run; SPR_FORCE=getragdoll keeps only GetRagDoll's forcing.
PART = os.environ.get('SPR_FORCE', 'all')
RD = '$this->xb30_ragDoll.mPtr'
NOAI = '$this->x8f5_31_ragdollNoAiCollision'
TRIG_ACTIVE = '((CEntity*)$rsi)->mActive'
FORCE_RET = [
    # Think: a ragdoll on a frozen body (0x1D4C), the delay running out with a ragdoll already
    # there (0x1F34), the creation flag 4 (0x8f5 0x01, no shipped placement has it), the
    # cinematic exit with that flag (0x1A64) and a pirate in a projectile's path (0x1B0C).
    ('1980 Think', 'CSpacePirate::Think(', 6,
     [(p, [('$this->mBodyController.mPtr->mFrozen', '%s != 0 ? 1 : $this->mBodyController.mPtr->mFrozen' % RD) if p in (1, 4) else
           ('$this->xb2c_ragdollDelayTimer', '%s != 0 ? 0.0001 : $this->xb2c_ragdollDelayTimer' % RD),
           (NOAI, '1' if p == 5 else '(%s == 0 && $this->xb2c_ragdollDelayTimer > 0) ? 1 : %s' % (RD, NOAI)),
           ('!$this->x8f7_27_inProjectilePath', '1' if p == 3 else '$this->x8f7_27_inProjectilePath')])
      for p in range(6)],
     'frozen body with a ragdoll (1, 4), delay with a ragdoll (0, 2, 3, 5), flag 4 at creation, '
     '0x8f5 0x01 on 5, projectile path on 3'),
    # PreRenderAllViewports (the first of the pirate's render passes) leaves the ragdoll unprimed
    # (calls 1 mod 4) or without valid render bounds (calls 3 mod 4) until the next Think primes it
    # again (0x1D74) or its Update rebuilds the bounds: its own and PreRender's unprimed arms
    # (0x3A3C, 0x3914), and GetRagDoll's (0x17B4, 0x17BC) from the projectiles that test the
    # corpse before that Think (CGameProjectile::CanCollideWithGameObject calls it twice, so a
    # write inside one GetRagDoll call would split the pair).
    ('3A0C PreRenderAllViewports', 'CSpacePirate::PreRenderAllViewports(', 4,
     [(1, [('!' + RD + '->mPrimed', '0')]), (3, [('!' + RD + '->mRenderBoundsValid', '0')])],
     'mPrimed = 0 on calls 1 mod 4, mRenderBoundsValid = 0 on calls 3 mod 4, until the next Think'),
    # Touch: a water touching a primed ragdoll goes inactive and stays so (0xE428); the corpse
    # keeps it as its fluid, so Think's inactive-water arm (0x1E38) follows. ($rsi is gone at the
    # return, so this write cannot be restored.)
    ('E3D8 Touch', 'CSpacePirate::Touch(', 1,
     [(0, [('!' + TRIG_ACTIVE, '(%s != 0 && %s->mPrimed && *(long*)$rsi == (long)&\'vtable for CScriptWater\' + 16) ? 0 : %s'
            % (RD, RD, TRIG_ACTIVE))])],
     'a water touching a primed ragdoll deactivated'),
]
FORCE_INNER = [
    # Think's head: multiplayer (0x1A28), the cinematic camera (0x1A40), a fully frozen body
    # (0x1C78, 0x1CEC) and a fluid id that is no water (0x1E24).
    ('1980 multiplayer', 'CSpacePirate::Think(', 'CStateManager::IsMultiplayer(', 7,
     {3: [('$rax', '1')]}, 'IsMultiplayer true on calls 3 mod 7'),
    ('1980 cinematic', 'CSpacePirate::Think(', 'CCameraManager::IsInCinematicCamera(', 5,
     {1: [('$rax', '1')], 2: [('$rax', '1')]}, 'IsInCinematicCamera true on calls 1, 2 mod 5'),
    ('1980 frozen', 'CSpacePirate::Think(', 'CBodyController::GetPercentageFrozen(', 9,
     {4: [('$xmm0.v4_float[0]', '1.0')]}, 'GetPercentageFrozen 1.0 on calls 4 mod 9'),
    # 0x0443 is a water of 08_sand_casualpirates the driver deactivates (SPR_WATER_OFF; the
    # deactivation lets the inhabitants go, so the corpse's own fluid id never names it).
    ('1980 fluid', 'CSpacePirate::Think(', 'CActor::InFluidId(', 4,
     {2: [('$rax', '$this->mUniqueId.value')], 3: [('$rax', '0x0443')]},
     'InFluidId = the pirate itself on calls 2 mod 4, the deactivated water 0x0443 on 3 mod 4'),
    # GetRagDoll's unprimed (0x17B4) and invalid-bounds (0x17BC) arms, for the projectiles that test
    # the corpse after its Think: the ragdolls of the pirates with an even id are left unprimed
    # after every Prime (so every Think primes them again), the others lose their render bounds
    # after every second Update.
    ('17A0 unprimed', 'CSpacePirate::Think(', 'CPirateRagDoll::Prime(', 1,
     {0: [(RD + '->mPrimed', '($this->mUniqueId.value & 1) == 0 ? 0 : %s->mPrimed' % RD)]},
     'the ragdoll unprimed after its Prime (pirates with an even id)'),
    ('17A0 bounds', 'CSpacePirate::Think(', 'CPirateRagDoll::Update(', 2,
     {1: [(RD + '->mRenderBoundsValid', '0')]}, 'the render bounds invalid after every second Update'),
]
if PART == 'water':
    FORCE_INNER = [f for f in FORCE_INNER if not f[0].startswith('17A0')]
elif PART == 'getragdoll':
    # only GetRagDoll's two: the other breakpoints slow the game so much that the shots reach
    # the corpses before their ragdolls are built.
    FORCE_RET = []
    FORCE_INNER = [f for f in FORCE_INNER if f[0].startswith('17A0')]
