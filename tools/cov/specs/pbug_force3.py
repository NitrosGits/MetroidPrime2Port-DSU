# PillBug (module 48) CloseToPath RaySphere true arm (CPillBug.cpp:478-479, retail 0x28F4): the
# bug is stood on its own destination waypoint (mDestPos, CActor::mPosition is what GetTranslation()
# returns) for the duration of one call, so that waypoint's segment starts inside the sphere and
# RaySphereIntersection reports T = radius < mag. The position is put back when the call returns,
# so the rest of the frame (and the fight) sees the bug where it was. The first 400 calls are
# forced; the body's own random gate (Next() & 3) lets about one in four through.
#   BUILD=$PWD/build-port-link tools/cov/force.sh tools/cov/specs/pbug_force3.py <out> \
#     1baa96c2,06_cliff_spiderballguardian --con tools/cov/specs/pillbug_cliff.con --fast --frames 4500
import os

import gdb

THIS = '((CPillBug*)$rdi)'
_n = [0]


def _log(msg):
    try:
        with open(os.environ['COV_OUT'] + '/forced.log', 'a') as fh:
            fh.write(msg + '\n')
    except Exception:
        pass


class _Back(gdb.FinishBreakpoint):
    def __init__(self, frame, this, old, n):
        super().__init__(frame, internal=True)
        self.this, self.old, self.n = this, old, n

    def stop(self):
        try:
            ret = int(gdb.parse_and_eval('$rax')) & 0xff
            for axis, v in zip('XYZ', self.old):
                gdb.execute('set var ((CPillBug*)%d)->mPosition.m%s = %.9g' % (self.this, axis, v))
            _log('274C CloseToPath call %d returned %d (position put back)' % (self.n, ret))
        except gdb.error as e:
            _log('274C CloseToPath call %d restore failed: %s' % (self.n, str(e)[:60]))
        return False


def close_stand(gdb):
    _n[0] += 1
    if _n[0] > 400:
        return
    try:
        this = int(gdb.parse_and_eval('(long)$rdi'))
        old = [float(gdb.parse_and_eval('%s->mPosition.m%s' % (THIS, a))) for a in 'XYZ']
        dest = [float(gdb.parse_and_eval('%s->mDestPos.m%s' % (THIS, a))) for a in 'XYZ']
        for a, v in zip('XYZ', dest):
            gdb.execute('set var %s->mPosition.m%s = %.9g' % (THIS, a, v))
        _Back(gdb.newest_frame(), this, old, _n[0])
        _log('274C CloseToPath call %d: position %s -> destination %s' % (_n[0], old, dest))
    except (gdb.error, ValueError) as e:
        _log('274C CloseToPath call %d force failed: %s' % (_n[0], str(e)[:60]))


CLASS = 'CPillBug'
FORCE = [
    ('274C CloseToPath stand on destination', 'CPillBug::CloseToPath(', [], [close_stand], 0,
     'the bug stood on its destination waypoint for one call: the ray starts inside the sphere'),
    ('274C RaySphere call', 'loc:CPillBug.cpp:478', [], [], 0,
     'RaySphereIntersection reached (a segment passed the null/zero-length continues)'),
]
