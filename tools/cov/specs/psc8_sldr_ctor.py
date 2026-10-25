# PlantScarabSwarm (module 49) psc8 value oracle for the inline SLdrPlantScarabSwarm constructor
# (fn_49_858, include/MetroidPrime/ScriptLoader/SLdrPlantScarabSwarm.hpp).  This file is BOTH the
# force.sh spec argument and COV_GDB_SCRIPT.  It writes no game state.
#
# The constructor is inlined into LoadPlantScarabSwarm (no out-of-line symbol; only the gcov record
# __gcov0._ZN20SLdrPlantScarabSwarmC2Ev and the destructor exist), so the oracle breaks in the caller at
# the first line of the generated property loop - SLdrPlantScarabSwarm.inc:3, whose first code address is
# LoadPlantScarabSwarm+0x194 (0xf4faf0 + 0x194 = 0xf4fc84 from objdump --dwarf=decodedline) - which is
# after the LAST default store of the inline body (the packed ambient colour at 0xf4fc75; visorFlags 0xf
# at 0xf4fc6b comes just before it) and before any serialized property is read.
#
# The breakpoint offset and the frame-base fallback are numbers of ONE build, so they are bound to that
# build's binary: the fixture hashes the binary gdb is about to run and refuses to install itself unless
# it is the archived evidence binary (sha256 below).  It also fails closed (see summary) on a missing
# binary path, a mismatch, an unresolved object address, an unreadable field, a caught exception, or no
# hit/object at all - a run whose oracle did not actually verify anything reports status=FAIL with the
# reason, never a silent fields_bad=0.
#
# Object address: the DWARF local `sldrThis` when gdb can resolve it, else the caller's frame base minus
# OBJECT_RBP_DELTA (derived from the same disassembly for the same binary: active, host offset 0xcc, is
# stored at rbp-0x384 and visorFlags, host offset 0xac, at rbp-0x3a4, so the base is rbp-0x450).
#
# Every field is compared BIT-EXACTLY against the value the source assigns (the TARGET_PC overrides
# included), so the evidence is a value oracle, not just "the loader was entered".
#
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/psc8_sldr_ctor.py tools/cov/force.sh \
#     tools/cov/specs/psc8_sldr_ctor.py /tmp/opencode/psc8_rN 3dfd2249,0g_swamp_hall \
#     --con tools/cov/specs/psc6_grenade_0g.con --fast --frames 1200
import hashlib
import os

import gdb

OUT = os.environ.get('COV_OUT', '/tmp')
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
# The archived evidence binary of the accepted psc8 run (see /tmp/opencode/psc8_evidence/sha256-r2.txt).
EVIDENCE_SHA256 = 'bac9be367bf8b38dfed82bd117419b063cfef9105402e1a0e8d632abc914046f'
LOADER = '_Z20LoadPlantScarabSwarmR13CStateManagerR12CInputStreamR11CEntityInfo'
POST_CTOR_OFFSET = 0x194   # verified for EVIDENCE_SHA256 only
OBJECT_RBP_DELTA = 0x450   # verified for EVIDENCE_SHA256 only
# (field expression suffix, width b|w, expected bits) - expected = the source's assignment, bit-exact
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
state = {'installed': False, 'binary': 'unchecked', 'hits': 0, 'objects': 0, 'fields_ok': 0,
         'fields_bad': 0, 'unread': [], 'problems': [], 'done': 0}
seen = []


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def problem(msg):
    state['problems'].append(msg)
    log('psc8 FAIL: ' + msg)


def summarize():
    """status=OK only when the oracle really verified every field on a real object."""
    problems = list(state['problems'])
    if state['binary'] != EVIDENCE_SHA256 and not state['installed']:
        pass  # the mismatch/problem is already recorded
    if state['hits'] == 0:
        problems.append('no breakpoint hit (binary/offset mismatch or the loader never ran)')
    if state['objects'] == 0:
        problems.append('no object read')
    if state['unread']:
        problems.append('unreadable fields: ' + ','.join(state['unread']))
    if state['fields_bad']:
        problems.append('%d field mismatch(es)' % state['fields_bad'])
    if state['fields_ok'] != len(ORACLE):
        problems.append('only %d/%d fields verified' % (state['fields_ok'], len(ORACLE)))
    status = 'OK' if not problems else 'FAIL'
    line = ('status=%s binary=%s installed=%d hits=%d objects=%d fields_ok=%d/%d fields_bad=%d '
            'unread=%d reason=%s\n' % (status, state['binary'], int(state['installed']), state['hits'],
                                       state['objects'], state['fields_ok'], len(ORACLE),
                                       state['fields_bad'], len(state['unread']),
                                       '; '.join(problems) if problems else '-'))
    with open(TABLE, 'a') as fh:
        fh.write(line)
    log('psc8 summary: ' + line.strip())
    return status, problems


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def binary_sha():
    """(sha256, path) of the binary gdb is about to run, or (None, why not)."""
    try:
        ps = gdb.current_progspace()
        fn = ps.filename if ps is not None else None
    except gdb.error as e:
        return None, 'progspace error: %s' % str(e)[:60]
    if not fn:
        return None, 'no binary path in the progspace (started without --args?)'
    path = fn if os.path.isabs(fn) else os.path.join(os.getcwd(), fn)
    try:
        return sha256_file(path), path
    except OSError as e:
        return None, 'cannot read %s: %s' % (path, str(e)[:60])


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return 'err:' + str(e)[:48]
    except Exception as e:  # a broken expression must not abort the run
        return 'exc:' + repr(e)[:48]


def fmt(got):
    return '0x%x' % got if isinstance(got, int) else str(got)


def read_field(obj, suffix, width):
    """The field's raw bits, or None when the address or the memory read is unusable."""
    addr = ev('(long)&((SLdrPlantScarabSwarm*)0x%x)->%s' % (obj, suffix))
    if not isinstance(addr, int) or addr == 0:
        log('  %-42s UNREADABLE address (%s)' % (suffix, addr))
        return None
    cast = 'unsigned char' if width == 'b' else 'unsigned int'
    got = ev('*(%s*)0x%x' % (cast, addr))
    if not isinstance(got, int):
        log('  %-42s UNREADABLE memory at 0x%x (%s)' % (suffix, addr, got))
        return None
    return got


class Bp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*%s+0x%x' % (LOADER, POST_CTOR_OFFSET), internal=True)

    def stop(self):
        try:
            return self._stop()
        except Exception as e:  # never abort the run from the fixture; report it instead
            problem('fixture exception on hit %d: %r' % (state['hits'], e))
            return False

    def _stop(self):
        state['hits'] += 1
        obj = ev('(long)&sldrThis')
        how = 'DWARF local sldrThis'
        if not isinstance(obj, int) or obj == 0:
            rbp = ev('$rbp')
            obj = rbp - OBJECT_RBP_DELTA if isinstance(rbp, int) else None
            how = 'frame base - 0x%x' % OBJECT_RBP_DELTA
        if not isinstance(obj, int) or obj == 0:
            problem('object address unresolved on hit %d (sldrThis and $rbp both unusable)'
                    % state['hits'])
            return False
        if obj in seen:
            return False
        seen.append(obj)
        state['objects'] += 1
        log('C858: object 0x%x via %s (hit %d); comparing %d fields bit-exactly'
            % (obj, how, state['hits'], len(ORACLE)))
        for suffix, width, want in ORACLE:
            got = read_field(obj, suffix, width)
            if got is None:
                state['unread'].append(suffix)
                continue
            if got == want:
                state['fields_ok'] += 1
                log('  %-42s %s  OK' % (suffix, fmt(got)))
            else:
                state['fields_bad'] += 1
                log('  %-42s %s  MISMATCH want 0x%x' % (suffix, fmt(got), want))
                problem('field %s = %s, want 0x%x' % (suffix, fmt(got), want))
        if not state['unread'] and not state['fields_bad']:
            log('C858: all %d fields match the source defaults (value oracle PASS) on 0x%x'
                % (len(ORACLE), obj))
        return False


def on_exit(event):
    summarize()


gdb.events.exited.connect(on_exit)
_sha, _why = binary_sha()
if _sha == EVIDENCE_SHA256:
    state['binary'] = _sha
    Bp()
    state['installed'] = True
    log('psc8: value oracle installed at %s+0x%x on binary %s (%s)'
        % (LOADER, POST_CTOR_OFFSET, _sha[:12], _why))
else:
    state['binary'] = _sha or ('unavailable: ' + str(_why))
    problem('BINARY SHA MISMATCH: expected %s, got %s (%s) - the offset 0x%x and the frame delta 0x%x '
            'are bound to the archived evidence binary, so the fixture is NOT installed'
            % (EVIDENCE_SHA256, _sha, _why, POST_CTOR_OFFSET, OBJECT_RBP_DELTA))
