# Ing lane ing30 forcing spec (tools/cov/force.sh, gcov build): the arms of fn_29_51D0, 5714,
# 8FE8 and B534 that the Agon Wastes area cannot produce. Driver: tools/drivers/ing30.py.
#
# 51D0/5714 cycle four modes per call of the body ($ing30_c % 4, counted at each body entry):
#   0 natural; 1 the first FindClosestPhysicalPoint gets a filter flag no point has (0x80000000),
#   so `from` stays -1; 2 the second query (5714's hint point) the same, so `to` stays -1;
#   3 CPathFindPointSearch::Search gets a null source, which its PointPathExists(ptr, ptr)
#   answers false (kR_NoConnection): the search's failure arm. Every point the area has is
#   connected (PointPathExists is symmetric, FindClosestPhysicalPoint keeps the nearest point
#   whatever the distance), so these three arms need a point graph the shipped area lacks.
# 8FE8: Activate (msg 0) alternately on the wall triangle (normal +x: cross(normal, up) is
#   magnitude-safe, the transform is set) and on a floor triangle (normal +z: not magnitude-safe,
#   asm 0x90CC), Update (msg 1) on a wall triangle with normal +x (the driver places the target
#   10 units along +x: inside the 45-degree cone, asm 0x9358 ble taken).
# D540: every second OptionalPart call of the ctor gets the invalid asset id (asm 0xD688 cmplwi
#   r0,0xffff / 0xD68C beq and the same test before each of the seven effect tokens).
# B534: the flying body projectile's (form 4) first contact gets an empty material list, so the
#   loop takes the not-a-splat-surface arm (asm 0xB734 beq) and goes on to the next contact.
CLASS = 'CIng'
FORCE = [
    ('init counters', 'CIng::CIng(', [], ['$ing30_c = 0', '$ing30_k = 0', '$ing30_a = 0', '$ing30_p = 0'], 0, 'the mode counters'),
    ('51D0 mode', 'CIng::SetLuredPointDest(', ['$ing30_c % 4'], ['$ing30_c = $ing30_c + 1', '$ing30_k = 0'], 0, 'next mode'),
    ('5714 mode', 'CIng::SetWallPointCoverDest(', ['$ing30_c % 4'], ['$ing30_c = $ing30_c + 1', '$ing30_k = 0'], 0, 'next mode'),
    ('point query', 'CPathFindPointSearch::FindClosestPhysicalPoint(', ['$ing30_c % 4', '$ing30_k'],
     ['$ing30_k = $ing30_k + 1',
      '((CPathFindPointSearchFilter*)$rcx)->mFlags = ($ing30_c % 4 == $ing30_k) ? 0x80000000 : ((CPathFindPointSearchFilter*)$rcx)->mFlags'],
     0, 'mode 1/2: the query finds no point'),
    ('point search', 'CPathFindPointSearch::Search(', ['$ing30_c % 4'],
     ['$rsi = ($ing30_c % 4 == 3) ? 0 : $rsi'], 0, 'mode 3: the search fails'),
    ('8FE8 surface', 'CIng::BecomeWallProjectile(', ['(int)$edx', '$ing30_a'], [
        '$ing30_a = $ing30_a + ((int)$edx == 0)',
        '$ing30_w = ((int)$edx == 1) || ((int)$edx == 0 && $ing30_a % 2 == 1)',
        '$ing30_f = ((int)$edx == 0 && $ing30_a % 2 == 0)',
        '$this->mSurfaceAlignment.mSurface.mA.mX = $ing30_f ? 0.0 : ($ing30_w ? 0.0 : $this->mSurfaceAlignment.mSurface.mA.mX)',
        '$this->mSurfaceAlignment.mSurface.mA.mY = $ing30_f ? 0.0 : ($ing30_w ? 0.0 : $this->mSurfaceAlignment.mSurface.mA.mY)',
        '$this->mSurfaceAlignment.mSurface.mA.mZ = $ing30_f ? 0.0 : ($ing30_w ? 0.0 : $this->mSurfaceAlignment.mSurface.mA.mZ)',
        '$this->mSurfaceAlignment.mSurface.mB.mX = $ing30_f ? 1.0 : ($ing30_w ? 0.0 : $this->mSurfaceAlignment.mSurface.mB.mX)',
        '$this->mSurfaceAlignment.mSurface.mB.mY = $ing30_f ? 0.0 : ($ing30_w ? 0.0 : $this->mSurfaceAlignment.mSurface.mB.mY)',
        '$this->mSurfaceAlignment.mSurface.mB.mZ = $ing30_f ? 0.0 : ($ing30_w ? 1.0 : $this->mSurfaceAlignment.mSurface.mB.mZ)',
        '$this->mSurfaceAlignment.mSurface.mC.mX = $ing30_f ? 0.0 : ($ing30_w ? 0.0 : $this->mSurfaceAlignment.mSurface.mC.mX)',
        '$this->mSurfaceAlignment.mSurface.mC.mY = $ing30_f ? 1.0 : ($ing30_w ? -1.0 : $this->mSurfaceAlignment.mSurface.mC.mY)',
        '$this->mSurfaceAlignment.mSurface.mC.mZ = $ing30_f ? 0.0 : ($ing30_w ? 0.0 : $this->mSurfaceAlignment.mSurface.mC.mZ)',
    ], 0, 'msg 0: wall (odd Activates) or floor triangle; msg 1: wall triangle, normal +x'),
    ('D540 part', 'loc:PortCIng.cpp:406', ['$ing30_p'],
     ['$ing30_p = $ing30_p + 1', 'id = ($ing30_p % 2 == 0) ? 0xffffffff : id'], 0,
     'every second effect id of the ctor reads as no asset'),
    ('B534 contact', 'CIng::CollidedWith(', ['(int)$this->mForm', '*(int*)$rdx'],
     ['*(unsigned long long*)((char*)$rdx + 8 + 0x30) = ((int)$this->mForm == 4 && *(int*)$rdx > 0) ? 0 : *(unsigned long long*)((char*)$rdx + 8 + 0x30)'],
     0, 'form 4: the first contact has no material'),
]
