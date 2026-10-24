# Run through force.sh + cov_force.py, build-port-cov, Ing area 1baa96c2,2735236b.
# Disposable stack copies exercise helper arms without calling the inferior
# allocator while stopped. The scratch is 16 KiB below entry SP, beyond these
# helpers' stack frames, and never escapes the call.
import os
import gdb

CLASS = 'CCollisionTracker'
SPEC = []
FORCE = [
    ('27DC emission', 'CCollisionTracker::Think(', [],
     ['$this->mParticleEmissionRateScalar = 1.0'], 0, 'emit live Ing strands'),
]

if not hasattr(gdb, '_gb3_helpers_installed'):
    gdb._gb3_helpers_installed = True
    symbols = []
    for line in open(os.environ['COV_NM']):
        parts = line.rstrip().split('\t')
        if len(parts) == 2:
            fields = parts[0].split()
            if len(fields) == 3 and fields[1] in 'tTW':
                symbols.append((fields[2], parts[1]))

    def symbol(prefix):
        hits = [m for m, d in symbols if d.startswith(prefix) and 'clone' not in d]
        if not hits:
            raise RuntimeError('missing symbol: ' + prefix)
        return hits[0]

    def command(text):
        gdb.execute(text, to_string=True)

    def copy(dst, src, typename):
        inf = gdb.selected_inferior()
        inf.write_memory(int(gdb.parse_and_eval(dst)),
                         inf.read_memory(int(gdb.parse_and_eval(src)), gdb.lookup_type(typename).sizeof))

    def log(text):
        with open(os.path.join(os.environ['COV_OUT'], 'helpers.log'), 'a') as f:
            f.write(text + '\n')

    class Triangle(gdb.Breakpoint):
        n = 0

        def stop(self):
            self.n += 1
            try:
                command('set $gb3_a = (STriangleSlot*)($rsp - 16384)')
                command('set $gb3_b = $gb3_a + 1')
                copy('$gb3_a', '$rsi', 'STriangleSlot')
                copy('$gb3_b', '$gb3_a', 'STriangleSlot')
                command('set $rdi = $gb3_a')
                command('set $rsi = $gb3_b')
                if self.n == 1:
                    command('set $rdi = 0')
                elif self.n == 2:
                    command('set $rsi = 0')
                elif self.n == 3:
                    command('set $gb3_b->mTriangle.mIndex ^= 1')
                elif self.n in (4, 5, 6):
                    command('set ((float*)$gb3_b)[%d] += 10.0' % (3 * (self.n - 4)))
                log('2074 forced case %d (null A/B, index, vertices 0/1/2, equal)' % self.n)
            except Exception as exc:
                log('2074 FAILED: ' + str(exc))
            if self.n >= 7:
                self.enabled = False
            return False

    class Simplify(gdb.Breakpoint):
        n = 0

        def stop(self):
            self.n += 1
            try:
                command('set $gb3_original = (CSurfacePath*)$rdi')
                command('set $gb3_path = (CSurfacePath*)($rsp - 16384)')
                command('set $gb3_segments = (SSurfacePathSegment*)($rsp - 16000)')
                copy('$gb3_path', '$gb3_original', 'CSurfacePath')
                for i in range(3):
                    copy('$gb3_segments + %d' % i,
                         '$gb3_original->mSegments.mItems', 'SSurfacePathSegment')
                command('set $gb3_path->mSegments.mItems = $gb3_segments')
                command('set $gb3_path->mSegments.mCapacity = 3')
                command('set $gb3_path->mSegments.mCount = %d' % min(self.n - 1, 3))
                if self.n == 5:
                    command('set $gb3_segments[1].mDirection.mX = -$gb3_segments[0].mDirection.mX')
                    command('set $gb3_segments[1].mDirection.mY = -$gb3_segments[0].mDirection.mY')
                    command('set $gb3_segments[1].mDirection.mZ = -$gb3_segments[0].mDirection.mZ')
                elif self.n == 6:
                    command('set ((float*)&$gb3_segments[1].mOrientation)[0] += 2.0')
                command('set $rdi = $gb3_path')
                log('0 forced case %d (0/1/2 segments, merge, direction mismatch, orientation mismatch)' % self.n)
            except Exception as exc:
                log('0 FAILED: ' + str(exc))
            if self.n >= 6:
                self.enabled = False
            return False

    class TraceMiss(gdb.Breakpoint):
        def stop(self):
            self.enabled = False
            try:
                command('set $gb3_start = (CVector3f*)($rsp - 16384)')
                for axis in ('mX', 'mY', 'mZ'):
                    command('set $gb3_start->%s = 1000000.0' % axis)
                command('set $rsi = $gb3_start')
                log('1FC forced initial nearest-triangle miss (start outside cached bounds)')
            except Exception as exc:
                log('1FC FAILED: ' + str(exc))
            return False

    class Local(gdb.Breakpoint):
        def __init__(self, source_text, expression, label):
            source = os.path.join(os.path.dirname(os.environ['COV_SPEC']),
                                  '../../../src/MetroidPrime/Enemies/CCollisionTracker.cpp')
            line = next(i for i, text in enumerate(open(source), 1) if source_text in text)
            super().__init__('CCollisionTracker.cpp:%d' % line, internal=True)
            self.write_expr, self.label = expression, label

        def stop(self):
            self.enabled = False
            try:
                command('set var ' + self.write_expr)
                log(self.label + ' forced: ' + self.write_expr)
            except Exception as exc:
                log(self.label + ' FAILED: ' + str(exc))
            return False

    Triangle('*' + symbol('IsSameTriangle('), internal=True)
    Simplify('*' + symbol('CSurfacePath::Simplify('), internal=True)
    TraceMiss('*' + symbol('CSurfacePath::Trace('), internal=True)
    Local('if (neighbor == 0xffff', 'neighbor = triangle->mTriangle.mIndex', '1FC self-neighbor')
    Local('if (sharedEdge == neighborEdges[0])', 'sharedEdge = 65535', '1220 missing edge')
