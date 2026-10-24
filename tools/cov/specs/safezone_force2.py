# ScriptSafeZone (module 66) sz1 lane: force_ret.py runs on the natural drivers (safezone_sand2.con, sandfalls_dark, 2150 frames; run it together with
# safezone_force1.py's run for the spot texture):
#   FORCE_RET (writes for one natural call, restored at the return): ApplyRenderEffect with huge / negative flash and inside alpha (the alpha clamps) and
#   mMobile both ways, RenderDarkVisorSpot with the zone moved onto the camera (toSpot cannot be normalised), HandleProjectile with the shape forced
#   to cylinder / an unknown value, UpdatePlayerInside with mIgnoreCinematicCamera both ways;
#   RayCylinderIntersection (inlined into HandleProjectile, no shipped zone is a cylinder) is driven by the console var `sz.cyl` (CScriptSafeZone::PortConsoleVar).
#   COV_ROOT=$PWD COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 tools/cov/force.sh tools/cov/specs/safezone_force2.py <out> \
#     42b935e4,02_sand_sandfalls_dark --con tools/cov/specs/safezone_force2.con --fast --frames 2150
import os
import re
import gdb

# the zones need a dark-visor spot texture for RenderDarkVisorSpot's body to run (as safezone_force1.py): the same write, made here by a gdb breakpoint
_src = open(os.path.join(os.environ.get('COV_ROOT', '.'), 'src/MetroidPrime/ScriptObjects/CScriptSafeZone.cpp')).read().split('\n')
_ZL = [i + 1 for i, l in enumerate(_src) if re.search(r'^  const CVector3f forceField =', l)][0]


class _Spots(gdb.Breakpoint):
    def stop(self):
        for a in ('normalAttributes', 'hurtfulAttributes', 'echoAttributes'):
            gdb.execute('set var sldrThis.%s.darkVisorSpotTexture = sldrThis.%s.shell1Texture' % (a, a))
        return False


_Spots('CScriptSafeZone.cpp:%d' % _ZL, internal=True)


gdb.execute('set $sz_c = 0')


class _CineCast(gdb.Breakpoint):
    # $sz_c 1: TCastToPtr<CCinematicCamera>(CEntity*) answers its argument (the camera is a cinematic camera) for the call
    def stop(self):
        if int(gdb.parse_and_eval('$sz_c')):
            gdb.execute('set $rax = $rdi')
            gdb.execute('set $rip = *(unsigned long*)$rsp')
            gdb.execute('set $rsp = $rsp + 8')
        return False


for _ln in open(os.environ['COV_NM']):
    _p = _ln.rstrip('\n').split('\t')
    if len(_p) == 2 and _p[1].startswith('CCinematicCamera* TCastToPtr<CCinematicCamera>('):
        _f = _p[0].split()
        if len(_f) == 3 and _f[1] in 'tTW':
            _CineCast('*' + _f[2], internal=True)

CLASS = 'CScriptSafeZone'
_FL = '$this->mFlashTime', '1.0'
FORCE_RET = [
    ('ApplyRenderEffect alpha clamps', 'CScriptSafeZone::ApplyRenderEffect(', 4,
     [(0, [_FL, ('$this->mFlashTimer', '1000000.0')]), (1, [_FL, ('$this->mFlashTimer', '-1000000.0')]),
      (2, [('$this->mInsideAlpha', '1000000.0')]), (3, [('$this->mInsideAlpha', '-1000000.0')])], 'pulse / inside alpha far above and below the byte range'),
    ('ApplyRenderEffect mobile', 'CScriptSafeZone::ApplyRenderEffect(', 2,
     [(0, [('$this->mMobile', '1')]), (1, [('$this->mMobile', '0')])], 'mobile zone spot size'),
    ('RenderDarkVisorSpot on the camera', 'CScriptSafeZone::RenderDarkVisorSpot(', 1,
     [(0, [('CGraphics::mViewMatrix.m03', '$this->mTransform.m03'), ('CGraphics::mViewMatrix.m13', '$this->mTransform.m13'),
           ('CGraphics::mViewMatrix.m23', '$this->mTransform.m23')])], 'toSpot is zero'),
    ('HandleProjectile shape', 'CScriptSafeZone::HandleProjectile(', 3,
     [(0, [('*(int*)&$this->mShape', '1')]), (1, [('*(int*)&$this->mShape', '7')])], 'cylinder zone / unknown shape'),
    ('UpdatePlayerInside cinematic flag', 'CScriptSafeZone::UpdatePlayerInside(', 3,
     [(0, [('$this->mIgnoreCinematicCamera', '1')]), (1, [('$this->mIgnoreCinematicCamera', '0')]),
      (2, [('$this->mIgnoreCinematicCamera', '1'), ('$sz_c', '1')])], 'ignore-cinematic both ways; the camera answers as a cinematic one'),
]
CALLS = []
