# ScriptSafeZone (module 66) sz1 lane, force.py `loc:` writes into the loaders' locals (every shipped zone has no dark-visor spot texture, every
# crystal has all four models):
#   LoadSafeZone: darkVisorSpotTexture := shell1Texture for the three attribute sets of every zone (RenderDarkVisorSpot's body then runs under the dark visor);
#   LoadSafeZoneCrystal: on successive crystals normalCrystal / entangledCrystal / hurtfulCrystal / echoCrystal := invalid (the four null-model arms).
#   COV_ROOT=$PWD BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 tools/cov/force.sh tools/cov/specs/safezone_force1.py <out> 42b935e4,02_sand_sandfalls_dark \
#     --con tools/cov/specs/safezone_sand2.con --fast --frames 2150
import os
import re
CLASS = 'CScriptSafeZone'
_root = os.environ.get('COV_ROOT', '.')
_z = open(os.path.join(_root, 'src/MetroidPrime/ScriptObjects/CScriptSafeZone.cpp')).read().split('\n')
_c = open(os.path.join(_root, 'src/MetroidPrime/ScriptObjects/CScriptSafeZoneCrystal.cpp')).read().split('\n')
_ZL = [i + 1 for i, l in enumerate(_z) if re.search(r'^  const CVector3f forceField =', l)][0]
_CL = [i + 1 for i, l in enumerate(_c) if re.search(r'^  const TUniqueId uid = mgr\.AllocateUniqueId\(\);', l)][0]


def _seq(*steps):
    st = {'n': 0}

    def f(gdb):
        i = st['n']
        st['n'] += 1
        if i < len(steps):
            for e in steps[i]:
                gdb.execute('set var ' + e)
    return f


def _spots(gdb):
    for a in ('normalAttributes', 'hurtfulAttributes', 'echoAttributes'):
        gdb.execute('set var sldrThis.%s.darkVisorSpotTexture = sldrThis.%s.shell1Texture' % (a, a))


FORCE = [
    ('zone spot texture', 'loc:CScriptSafeZone.cpp:%d' % _ZL, [], [_spots], 0, 'a valid dark-visor spot texture on every zone'),
    ('crystal models', 'loc:CScriptSafeZoneCrystal.cpp:%d' % _CL, [],
     [_seq(['sldrThis.normalCrystal = 4294967295'], ['sldrThis.entangledCrystal = 4294967295'], ['sldrThis.hurtfulCrystal = 4294967295'],
           ['sldrThis.echoCrystal = 4294967295'])], 0, 'the first four crystals lose one model each'),
]
