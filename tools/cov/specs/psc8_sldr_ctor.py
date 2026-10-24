# PlantScarabSwarm (module 49) psc8 value oracle for the inline SLdrPlantScarabSwarm constructor
# (fn_49_858, include/MetroidPrime/ScriptLoader/SLdrPlantScarabSwarm.hpp).  This file is BOTH the
# force.sh spec argument and COV_GDB_SCRIPT.  It writes nothing.
#
# The constructor is inlined into LoadPlantScarabSwarm (no out-of-line symbol; only the gcov record
# __gcov0._ZN20SLdrPlantScarabSwarmC2Ev and the destructor exist), so the oracle breaks in the caller at
# the first line of the generated property loop - SLdrPlantScarabSwarm.inc:3, which the decoded line
# table places at 0xf4fc84, after every default store of the inline body (the last is visorFlags 0xf at
# 0xf4fc6b) and before any serialized property is read - and reads the constructed object.
#
# Object address: the DWARF local `sldrThis` when gdb can resolve it, else the caller's frame base minus
# 0x450, which is derived statically from the same disassembly (active, host offset 0xcc, is stored at
# rbp-0x384; visorFlags, host offset 0xa8+4, at rbp-0x3a4: base = rbp-0x450).  Both paths are logged.
#
# Every field is compared BIT-EXACTLY against the value the source assigns (the TARGET_PC overrides
# included), so the log is a value oracle, not just "the loader was entered".
#
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/psc8_sldr_ctor.py tools/cov/force.sh \
#     tools/cov/specs/psc8_sldr_ctor.py /tmp/opencode/psc8_r1 3dfd2249,0g_swamp_hall \
#     --con tools/cov/specs/psc6_grenade_0g.con --fast --frames 600
import os

import gdb

OUT = os.environ['COV_OUT']
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
# SLdrPlantScarabSwarm.inc:3's first code address is 0xf4fc84, i.e. LoadPlantScarabSwarm+0x194
# (0xf4faf0 + 0x194, from objdump --dwarf=decodedline): symbol-relative so gdb relocates the PIE.
LOADER = '_Z20LoadPlantScarabSwarmR13CStateManagerR12CInputStreamR11CEntityInfo'
WHERE = 'SLdrPlantScarabSwarm.inc:3 (LoadPlantScarabSwarm+0x194)'
ABASE = '((SLdrPlantScarabSwarm*)0x%x)'
# (field expression suffix, width b|w|d, expected bits) - expected = the source's assignment, bit-exact
ORACLE = [
    ('active', 'b', 1),
    ('intoAttackAnimation', 'w', 0xffffffff),
    ('attackAnimation', 'w', 0xffffffff),
    ('maxAttackAngle', 'w', 0x41f00000),          # 30.0f
    ('intoAttackSpeed', 'w', 0x3f800000),         # 1.0f
    ('attackSpeed', 'w', 0x3f800000),             # 1.0f
    ('grenadeMass', 'w', 0x3f800000),             # 1.0f
    ('grenadeLaunchSpeed', 'w', 0x3f800000),      # 1.0f
    ('unknown_0xed086ce0', 'w', 0x3f000000),      # 0.5f
    ('grenadeNumBounces', 'w', 3),
    ('grenadeExplosionProximity', 'w', 0x3f000000),  # 0.5f
    ('grenadeExplosionEffect', 'w', 0xffffffff),
    ('grenadeExplosionXRayEffect', 'w', 0xffffffff),
    ('grenadeTrailEffect', 'w', 0xffffffff),
    ('grenadeEffect', 'w', 0xffffffff),
    ('grenadeBounceSound', 'w', 0xffffffff),      # TARGET_PC override (upstream 0)
    ('grenadeBounceSoundFallOff', 'w', 0x00000000),  # 0.0f
    ('grenadeBounceMaxAudibleDistance', 'w', 0x42c80000),  # 100.0f
    ('grenadeExplosionSound', 'w', 0xffffffff),   # TARGET_PC override (upstream 0)
    ('grenadeExplosionSoundFallOff', 'w', 0x00000000),  # 0.0f
    ('grenadeExplosionMaxAudibleDistance', 'w', 0x42c80000),  # 100.0f
    ('actorInformation.lighting.ambientColor', 'w', 0xffffffff),  # CColor(1,1,1,1) packed
    ('actorInformation.visor.visorFlags', 'w', 0x0000000f),  # TARGET_PC override
]
counts = {'hits': 0, 'fields_ok': 0, 'fields_bad': 0, 'objects': 0}
seen = []


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return 'err:' + str(e)[:40]


def bits(addr, width):
    cast = {'b': 'unsigned char', 'w': 'unsigned int'}[width]
    return ev('*(%s*)0x%x' % (cast, addr))


class Bp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*%s+0x194' % LOADER, internal=True)

    def stop(self):
        try:
            return self._stop()
        except Exception as e:  # never abort the run from the fixture
            log('C858: FIXTURE EXCEPTION on hit %d: %r' % (counts['hits'], e))
            return False

    def _stop(self):
        counts['hits'] += 1
        obj = ev('(long)&sldrThis')
        how = 'DWARF local sldrThis'
        if not isinstance(obj, int) or obj == 0:
            rbp = ev('$rbp')
            obj = rbp - 0x450 if isinstance(rbp, int) else None
            how = 'frame base - 0x450 (static derivation)'
        if not isinstance(obj, int) or obj == 0:
            log('C858: hit %d, object address unresolved' % counts['hits'])
            return False
        if obj in seen:
            return False
        seen.append(obj)
        counts['objects'] += 1
        log('C858: object 0x%x via %s (hit %d); comparing %d fields bit-exactly'
            % (obj, how, counts['hits'], len(ORACLE)))
        bad = []
        for suffix, width, want in ORACLE:
            addr = ev('(long)&%s->%s' % (ABASE % obj, suffix))
            if not isinstance(addr, int) or addr == 0:
                bad.append((suffix, 'addr?', want))
                continue
            got = bits(addr, width)
            if got == want:
                counts['fields_ok'] += 1
            else:
                counts['fields_bad'] += 1
                bad.append((suffix, got, want))
            log('  %-42s 0x%x/%s  %s' % (suffix, got, width,
                                         'OK' if got == want else 'MISMATCH want 0x%x' % want))
        if bad:
            log('C858: MISMATCHES on 0x%x: %s' % (obj, bad))
        else:
            log('C858: all %d fields match the source defaults (value oracle PASS) on 0x%x'
                % (len(ORACLE), obj))
        return False


def on_exit(event):
    with open(TABLE, 'a') as fh:
        fh.write('hits=%d objects=%d fields_ok=%d fields_bad=%d\n' %
                 (counts['hits'], counts['objects'], counts['fields_ok'], counts['fields_bad']))


gdb.events.exited.connect(on_exit)
Bp()
log('psc8: value oracle installed at %s' % WHERE)
