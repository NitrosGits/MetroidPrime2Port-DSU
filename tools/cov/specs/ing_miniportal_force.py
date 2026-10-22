# tools/cov/force.sh spec for the mini portal's arms no fight reaches (CIngMiniPortalAttack,
# src/MetroidPrime/Enemies/PortCIngMiniPortalAttack.cpp), on a coverage build (gcov counts the arms).
# Run: the Agon Wastes 01_sand_entrance_dark with tools/drivers/ing_miniportal.py, e.g.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/ing_miniportal_force.py <out> \
#     42b935e4,01_sand_entrance_dark --frames 40000 \
#     --driver "python3 $PWD/tools/drivers/ing_miniportal.py {port} <out>/game.log 8"
CLASS = 'CIngMiniPortalAttack'


def CNT(n):
    return '$%s = $_isvoid($%s) ? 1 : $%s + 1' % (n, n, n)


# The breakpoints sit on the first instruction, so *$rsp is the return address: a call made from
# AimAtTarget's own body (it is under 0x800 bytes) returns into it.
_AIM = "(unsigned long)&'CIngMiniPortalAttack::AimAtTarget(CStateManager&, CActor const&)'"
FROM_AIM = '(*(unsigned long*)$rsp > %s && *(unsigned long*)$rsp < %s + 0x800)' % (_AIM, _AIM)

FORCE = [
    # fn_29_11840 @0x11874: the target lookup fails - no beam; the portal's next Think then finds
    # no beam (fn_29_11CA4 @0x11CF4) and deletes itself (@0x11E3C).
    ('11840 no target', 'CIngMiniPortalAttack::Fire(', ['$this->mInfo.mTarget.value'],
     [CNT('mpf'), '$this->mInfo.mTarget.value = ($mpf % 3 == 0) ? 0xffff : $this->mInfo.mTarget.value'], 0,
     'mInfo.mTarget = kInvalidUniqueId on every 3rd Fire'),
    # fn_29_11CA4 @0x11D80: at the aim time the target is gone (only the portals whose beam id is
    # 2 mod 4, so the others still aim at the aim time and reach the forced aims below).
    ('11CA4 target gone', 'CIngMiniPortalAttack::Think(', ['$this->mFlags & 0x80', '$this->mTime >= 0.95'],
     ['$this->mInfo.mTarget.value = ($this->mTime >= 0.95 && ($this->mFlags & 0x80) == 0 && $this->mProjectileId.value % 4 == 2) ? 0xffff : $this->mInfo.mTarget.value'],
     0, 'mInfo.mTarget = kInvalidUniqueId at the aim time for the portals whose beam id is 2 mod 4'),
    # fn_29_11570, only the Think's aim (mTime > 0.5, after the beam exists): @0x1168C the beam's
    # travel speed is not above zero (every 4th call).
    ('11570 speed 0', 'CIngMiniPortalAttack::AimAtTarget(',
     ['$this->mInfo.mBeamInfo.travelSpeed > 0'],
     [CNT('mpa'),
      '$this->mInfo.mBeamInfo.travelSpeed = ($this->mTime > 0.5 && $mpa % 4 == 0) ? 0.0 : $this->mInfo.mBeamInfo.travelSpeed'],
     0, 'travelSpeed = 0 (calls 0 mod 4), Think aims only'),
    # fn_29_11570 @0x11734: the aim delta is too short to normalise. Aiming the portal at itself
    # did not take the arm (the blend presumably misses the translation by a rounding step, above
    # the 1e-28 squared-length floor), so the delta handed to IsMagnitudeSafe is zeroed, every 2nd
    # call made from AimAtTarget.
    ('11570 zero delta', 'CVector3f::IsMagnitudeSafe(', [FROM_AIM],
     ['$mpz = %s ? ($_isvoid($mpz) ? 1 : $mpz + 1) : ($_isvoid($mpz) ? 0 : $mpz)' % FROM_AIM]
     + ['((float*)$rdi)[%d] = (%s && $mpz %% 2 == 0) ? 0.0 : ((float*)$rdi)[%d]' % (i, FROM_AIM, i)
        for i in range(3)],
     0, 'the delta = 0 on every 2nd IsMagnitudeSafe call from AimAtTarget'),
]
