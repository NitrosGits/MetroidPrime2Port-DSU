# PillBug (module 48) state-machine forcing spec (Pass 1). The path-building and loader arms live in
# pbug_force2.py: forcing those changes the bug's mPath, which keeps it out of Wander/Fall/ReturnToPath.
# tools/cov/force.sh, same areas and drivers as pb1's coverage runs (merge the gcov dirs in
# gcov_report.py, this one's included):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/pbug_force.py <out> \
#     42b935e4,0b_sand_hall --con tools/cov/specs/pillbug_sandhall.con --fast --frames 3000
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/pbug_force.py <out> \
#     1baa96c2,06_cliff_spiderballguardian --con tools/cov/specs/pillbug_cliff.con --fast --frames 4500
#
# Every member write is one call: the value the member had is put back at the next entry of the
# same body, so the fight after the forced call is the fight the driver meant to have. A write that
# gdb cannot make (a local optimised into a register) is logged to forced.log, never fatal.
import os

import gdb  # the spec's own namespace; force.py passes gdb to every set callable

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



_pause = [0]


def pause_phase2(gdb):
    # 0xD28 switch (mAnimSubState): case 0 at 0xD44, case 1 at 0xE10, any value >= 2 leaves
    # through 0xD34 bge (the switch's no-case edge). 0xE58/0xE5C write 2 themselves; the state
    # ends before the next Update reads it back.
    _pause[0] += 1
    if _pause[0] == 5:
        _set(gdb, '%s->mAnimSubState = 2' % THIS)


# ------------------------------------------------------- fn_48_2210 PatrolImpl
_patrol = {'n': 0, 'restore': []}


def patrol_arms(gdb):
    for member, old in _patrol['restore']:
        _set(gdb, '%s->%s = %s' % (THIS, member, old))
    _patrol['restore'] = []
    _patrol['n'] += 1
    n = _patrol['n']
    if n == 40:
        # 0x2494 cmpwi r0,0 / beq .L_000025C8: mPlanarConstraint == 0 skips the arrival command.
        _patrol['restore'].append(
            ('mPlanarConstraint', str(gdb.parse_and_eval(THIS + '->mPlanarConstraint'))))
        _set(gdb, '%s->mPlanarConstraint = 0' % THIS)
    elif n == 60:
        # 0x26B4 TCastToPtr<CScriptWaypoint>(ObjectById(mDestObj)): the bug's own id is not a
        # waypoint, so the cast's null arm at 0x26BC runs. mDestObj is put back next entry.
        _patrol['restore'].append(
            ('mDestObj.value', str(gdb.parse_and_eval(THIS + '->mDestObj.value'))))
        _set(gdb, '%s->mDestObj.value = %d' % (THIS, _int(THIS + '->mUniqueId.value')))


# ------------------------------------------------------ fn_48_1A58 ReturnToPath
_rtp = {'n': 0, 'act': 0, 'restore': []}


def rtp_arms(gdb):
    for member, old in _rtp['restore']:
        _set(gdb, '%s->%s = %s' % (THIS, member, old))
    _rtp['restore'] = []
    _rtp['n'] += 1
    n = _rtp['n']
    if n == 6:
        # 0x2324 bl UpdateWPDestination then its bool: a huge advance radius makes the
        # "arrived" test (asm 0xF78-0xFA8, (pos - GetTranslation()).MagSquared() < r*r) true, so
        # ReturnToPath takes SetConstraint at 587.
        _rtp['restore'].append(
            ('mAdvanceWpRadius', str(gdb.parse_and_eval(THIS + '->mAdvanceWpRadius'))))
        _set(gdb, '%s->mAdvanceWpRadius = 1.0e6' % THIS)
    elif n == 10:
        # ReturnToPath's Dot(GetUp(), dir) > 0.5f: mDestPos 100 units behind the bug along its
        # own down axis makes the dot -1.
        tx, ty, tz = (_f32('(char*)$rdi + 0x54'), _f32('(char*)$rdi + 0x58'),
                      _f32('(char*)$rdi + 0x5c'))
        ux, uy, uz = (_f32('(char*)$rdi + 0x2c'), _f32('(char*)$rdi + 0x3c'),
                      _f32('(char*)$rdi + 0x4c'))
        for member, value in (('mDestPos.mX', tx - 100.0 * ux), ('mDestPos.mY', ty - 100.0 * uy),
                              ('mDestPos.mZ', tz - 100.0 * uz)):
            _rtp['restore'].append((member, str(gdb.parse_and_eval(THIS + '->' + member))))
            _set(gdb, '%s->%s = %.9g' % (THIS, member, value))
    elif n == 14:
        # ReturnToPath's `if (mPlanarConstraint != 0)`: 0 skips the constraint-plane block.
        _rtp['restore'].append(
            ('mPlanarConstraint', str(gdb.parse_and_eval(THIS + '->mPlanarConstraint'))))
        _set(gdb, '%s->mPlanarConstraint = 0' % THIS)
    elif n == 18:
        # ReturnToPath's `if (mPlanarConstraint == 1)` false arm (a planar-2 bug).
        _rtp['restore'].append(
            ('mPlanarConstraint', str(gdb.parse_and_eval(THIS + '->mPlanarConstraint'))))
        _set(gdb, '%s->mPlanarConstraint = 2' % THIS)
    elif _int('$rdx') == 0:
        # ReturnToPath's Activate `if (!mPath.empty())`: an empty path takes the false arm.
        _rtp['act'] += 1
        if _rtp['act'] == 2:
            _rtp['restore'].append(
                ('mPath.mCount', str(gdb.parse_and_eval(THIS + '->mPath.mCount'))))
            _set(gdb, '%s->mPath.mCount = 0' % THIS)


# -------------------------------------------------------- fn_48_274C CloseToPath
_close = {'n': 0, 'restore': [], 'pos': None}


def close_arms(gdb):
    for member, old in _close['restore']:
        _set(gdb, '%s->%s = %s' % (THIS, member, old))
    _close['restore'] = []
    if _close['pos'] is not None:
        for off, old in _close['pos']:
            _fset(gdb, off, old)
        _close['pos'] = None
    _close['n'] += 1
    n = _close['n']
    if n in (20, 60, 100, 140, 180, 220, 300, 400):
        # 0x28F4 RaySphereIntersection(sphere, start, dir, mag, t, point) with the sphere centred
        # on GetTranslation(): standing the bug on its own destination waypoint (mDestPos, set by
        # UpdateWPDestination) puts the ray's start inside the sphere. Translation put back next.
        for member, value in (('mTransform.m03', 'mDestPos.mX'), ('mTransform.m13', 'mDestPos.mY'),
                              ('mTransform.m23', 'mDestPos.mZ')):
            _close['restore'].append(
                (member, str(gdb.parse_and_eval(THIS + '->' + member))))
            _set(gdb, '%s->%s = %s' % (THIS, member,
                                       str(gdb.parse_and_eval(THIS + '->' + value))))
        # 0x277C-0x27B4: the sphere is 1.25f * mColSphere.GetSphere().GetRadius(); a 20-unit
        # radius is entered from outside by the other segments, so the ray's t is positive.
        # 0x288C..0x2890: T = dirDot + sqrt(radSq) must be < mag, so the radius has to be small:
        # with the bug on a waypoint (the ray's start) T is exactly the radius.
        _close['restore'].append(
            ('mColSphere.mSphere.mRadius',
             str(gdb.parse_and_eval(THIS + '->mColSphere.mSphere.mRadius'))))
        _set(gdb, '%s->mColSphere.mSphere.mRadius = 0.5' % THIS)
    elif n == 45:
        # 0x27CC lbz/extrwi xa34 bit 0x8 then 0x27D4 beq: mPathLoops false takes the continue at
        # 457 on the segment that wraps to mPath[0].
        _close['restore'].append(
            ('mPathLoops', str(gdb.parse_and_eval(THIS + '->mPathLoops'))))
        _set(gdb, '%s->mPathLoops = 0' % THIS)
    elif n == 60:
        # 0x2820/0x2828 (to/from null) continue at 465: an invalid id is no CActor.
        _close['restore'].append(
            ('mPath.mItems[0].value', str(gdb.parse_and_eval(THIS + '->mPath.mItems[0].value'))))
        _set(gdb, '%s->mPath.mItems[0].value = 0xffff' % THIS)
    elif n in (40, 80, 120, 160, 240, 320, 480):
        # 0x2890 blt (close_enough(mag, 0.f)) continue at 472: two equal entries make mag 0.
        _close['restore'].append(
            ('mPath.mItems[1].value', str(gdb.parse_and_eval(THIS + '->mPath.mItems[1].value'))))
        _set(gdb, '%s->mPath.mItems[1].value = %d'
             % (THIS, _int(THIS + '->mPath.mItems[0].value')))
    if n in (20, 40, 60, 80, 100, 120, 140, 160, 180, 220, 240, 300, 320, 400, 480):
        _log('274C CloseToPath n=%d forced' % n)


# ------------------------------------------------------------- fn_48_1768 Fall
_fall = {'n': 0, 'act': 0, 'restore': [], 'up': None}


def fall_arms(gdb):
    for member, old in _fall['restore']:
        _set(gdb, '%s->%s = %s' % (THIS, member, old))
    _fall['restore'] = []
    msg = _int('$rdx')
    if msg == 1:  # kStateMsg_Update
        _fall['n'] += 1
        if _fall['n'] == 1:
            # 0x19B8 cmpwi r0,0xa / beq: the body already in kAS_LoopReaction skips the command.
            _fall['restore'].append(
                ('mBodyController.mPtr->mBodyStateInfo.mState',
                 str(gdb.parse_and_eval(THIS + '->mBodyController.mPtr->mBodyStateInfo.mState'))))
            _set(gdb, '%s->mBodyController.mPtr->mBodyStateInfo.mState = 10' % THIS)
    elif msg == 0:  # kStateMsg_Activate
        _fall['act'] += 1
        if _fall['act'] in (1, 3):
            _log('1768 Fall Activate %d: up column flipped' % _fall['act'])
            # 0x18BC fcmpo ... -0.998f / bge: an upside-down bug (up = -world up) takes the arm.
            for member, value in (('mTransform.m02', '0.0'), ('mTransform.m12', '0.0'),
                                  ('mTransform.m22', '-1.0')):
                _fall['restore'].append(
                    (member, str(gdb.parse_and_eval(THIS + '->' + member))))
                _set(gdb, '%s->%s = %s' % (THIS, member, value))



# ------------------------------------------------- fn_48_274C the RaySphere test
_cm = [0]


def close_mag(gdb):
    # 0x288C..0x2898: T must be < mag for RaySphereIntersection to report a hit. mag is the
    # segment length; a huge one makes the test true as soon as the discriminant is positive.
    _cm[0] += 1
    if _cm[0] == 30:
        _set(gdb, 'mag = 1.0e9')


CLASS = 'CPillBug'
FORCE = [
    ('BD8 Pause substate 2', 'CPillBug::Pause(', [], [pause_phase2], 0,
     'mAnimSubState 2 on one Update: the mAnimSubState switch no-case edge'),
    ('2210 PatrolImpl planar 0', 'CPillBug::PatrolImpl(', [], [patrol_arms], 0,
     'mPlanarConstraint 0 on one call (the arrival command arm), then mDestObj = the bug id on '
     'another (the CScriptWaypoint cast arm)'),
    ('1A58 ReturnToPath advance', 'CPillBug::ReturnToPath(', [], [rtp_arms], 0,
     'a huge advance radius on one call (UpdateWPDestination true) and mDestPos behind the bug on '
     'another (up dot <= 0.5)'),
    ('274C CloseToPath geometry', 'CPillBug::CloseToPath(', [], [close_arms], 0,
     'the bug on its destination waypoint (RaySphere hit), mPathLoops false, an invalid path id '
     'and two equal entries, one call each'),
    ('274C RaySphere mag', 'loc:CPillBug.cpp:478', [], [close_mag], 0,
     'mag (the segment length) huge at the RaySphereIntersection call: T < mag'),
    ('1768 Fall body state', 'CPillBug::Fall(', [], [fall_arms], 0,
     'mBodyStateInfo.mState = kAS_LoopReaction on one Update (the command arm)'),
]
