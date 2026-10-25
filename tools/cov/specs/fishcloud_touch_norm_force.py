# FishCloud (module 20) Touch (22EC) CanBeNormalized arms, forcing spec for tools/cov/force.sh (fish2b, 2026-10-25).
# The ball contact loop tests `flat.CanBeNormalized() && vel.CanBeNormalized()` (the build's line 740) and
# `delta.CanBeNormalized() && vel.CanBeNormalized()` (750; two lines further down in the source after the fn_20_22EC
# marker was added). The vector is a stack temporary, so the write is made at the entry of the out-of-line
# CVector3f::CanBeNormalized (this = rdi) when its caller is CFishCloud::Touch: per arm line, the first test is
# zeroed on every third contact (flat/delta false) and the second (velocity) on every second one it reaches.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/fishcloud_touch_norm_force.py <out> \
#     3bfa3eff,06_temple_sacredpath --con tools/cov/specs/fishcloud_sacredpath.con --fast --frames 2600
CLASS = 'CVector3f'
_st = {}


def cbn(gdb):
    older = gdb.newest_frame().older()
    if older is None or older.name() is None or 'CFishCloud::Touch' not in older.name():
        return
    line = older.find_sal().line
    s = _st.setdefault(line, {'next': 1, 'k1': 0, 'k2': 0})
    zero = False
    if s['next'] == 1:
        zero = s['k1'] % 3 == 0  # flat / delta false
        s['k1'] += 1
        s['next'] = 1 if zero else 2
    else:
        zero = s['k2'] % 2 == 0  # velocity false
        s['k2'] += 1
        s['next'] = 1
    if zero:
        for c in ('mX', 'mY', 'mZ'):
            gdb.execute('set var ((CVector3f*)$rdi)->%s = 0' % c)


FORCE = [
    ('22EC CanBeNormalized', 'CVector3f::CanBeNormalized(', [], [cbn], 0,
     'ball contact in Touch: flat/delta zero (every third), velocity zero (every second)'),
]
