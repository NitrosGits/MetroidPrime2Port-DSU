# PillBug (module 48) path/loader forcing spec (Pass 2): the HasInnerPathLoop walk arms and the
# loader's null-model arm. Forcing these changes the path BuildPath builds, so they are in their own
# run: the game after them is not the fight pbug_force.py's arms need.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/pbug_force2.py <out> \
#     42b935e4,0b_sand_hall --con tools/cov/specs/pillbug_sandhall.con --fast --frames 3000
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/pbug_force2.py <out> \
#     1baa96c2,06_cliff_spiderballguardian --con tools/cov/specs/pillbug_cliff.con --fast --frames 4500
#
# Every member write is one call: the value the member had is put back at the next entry of the
# same body, so the fight after the forced call is the fight the driver meant to have. A write that
# gdb cannot make (a local optimised into a register) is logged to forced.log, never fatal.
import os

import gdb

THIS = '((CPillBug*)$rdi)'


def _log(msg):
    try:
        with open(os.environ['COV_OUT'] + '/forced.log', 'a') as fh:
            fh.write(msg + '\n')
    except Exception:
        pass


def _set(gdb, expr):
    try:
        gdb.execute('set var ' + expr)
        return True
    except Exception as e:  # noqa: BLE001 - gdb raises several types here
        _log('set failed (%s): %s' % (expr, str(e)[:60]))
        return False


def _fset(gdb, off, value):
    try:
        gdb.execute('set *(float*)((char*)$rdi + %d) = %.9g' % (off, value))
        return True
    except Exception as e:  # noqa: BLE001
        _log('fset failed (%d): %s' % (off, str(e)[:60]))
        return False


def _int(expr):
    return int(gdb.parse_and_eval(expr)) & 0xffffffff


def _f32(expr):
    return float(gdb.parse_and_eval('*(float*)(%s)' % expr))


# -------------------------------------------------- fn_48_2FE4 HasInnerPathLoop
_hp = {'current': 0, 'anchor': 0, 'null': 0, 'loop': 0, 'anchor_null': 0}


def hp_exit_current(gdb):
    # 0x310C beq .L_0000311C: the walk ends on an invalid current id.
    _hp['current'] += 1
    if _hp['current'] == 9:
        _set(gdb, 'current.value = 0xffff')


def hp_exit_anchor(gdb):
    # 0x3118 bne .L_00003038: the walk ends on an invalid anchor id.
    _hp['anchor'] += 1
    if _hp['anchor'] == 11:
        _set(gdb, 'anchor.value = 0xffff')


def hp_null_waypoint(gdb):
    # 0x305C bne: a chain id that is not a waypoint returns false at 316.
    _hp['null'] += 1
    if _hp['null'] == 3:
        _set(gdb, 'waypoint = 0')


def hp_loop_start(gdb):
    # 0x30A0 bne .L_000030AC else li r3,1: current == loopStart returns true at 323.
    _hp['loop'] += 1
    if _hp['loop'] == 4:
        _set(gdb, 'current.value = anchor.value')


def hp_anchor_null(gdb):
    # 0x30D4 bne: the anchor lookup at 327 returns false at 329.
    _hp['anchor_null'] += 1
    if _hp['anchor_null'] == 6:
        _set(gdb, 'waypoint = 0')


# ------------------------------------------------------- fn_48_130 LoadPillBug
_load = [0]


def load_null_model(gdb):
    # 0x66C cmplwi/beq (the optional's valid byte at 0x1c4): the modelData local cleared, so the
    # loader returns nullptr at 808. Only the first spawn, so the driver's other bugs live.
    _load[0] += 1
    if _load[0] == 1:
        _set(gdb, 'modelData.m_valid = 0')



CLASS = 'CPillBug'
FORCE = [
    ('2FE4 walk ends on current', 'loc:CPillBug.cpp:310', [], [hp_exit_current], 0,
     'current = kInvalidUniqueId: the walk leaves through the first while operand'),
    ('2FE4 walk ends on anchor', 'loc:CPillBug.cpp:310', [], [hp_exit_anchor], 0,
     'anchor = kInvalidUniqueId: the walk leaves through the second while operand'),
    ('2FE4 non-waypoint in the chain', 'loc:CPillBug.cpp:315', [], [hp_null_waypoint], 0,
     'the chain id is not a CScriptWaypoint: return false at 316'),
    ('2FE4 back to the loop start', 'loc:CPillBug.cpp:322', [], [hp_loop_start], 0,
     'current == loopStart: return true at 323'),
    ('2FE4 anchor is not a waypoint', 'loc:CPillBug.cpp:328', [], [hp_anchor_null], 0,
     'the anchor id is not a CScriptWaypoint: return false at 329'),
    ('130 no model', 'loc:CPillBug.cpp:809', [], [load_null_model], 0,
     'modelData cleared: the loader returns nullptr at 808'),
]
