"""Offline self-test of swarmbasics_swb18.py's pure geometry/oracle helpers (no gdb, no game).

    python3 tools/cov/specs/swarmbasics_swb18_selftest.py     # prints SELFTEST PASS

The fixture is loaded with a stub gdb module, so only cells(), nearest(), near() and slot() are
exercised: the cell-centre arithmetic (the 0.1 margin cancels out of the centre - run 5 measured
20.0 where an uncancelled margin would give 19.9), the strict `<` rule that keeps the first of two
equal candidates, the NaN query that never updates, and the x + 5y + 25z slot order.
"""
import os, sys, types, math
gdb = types.ModuleType('gdb')
class GdbError(Exception): pass
gdb.GdbError = GdbError; gdb.error = GdbError
gdb.parse_and_eval = lambda s: (_ for _ in ()).throw(GdbError('no inferior'))
gdb.execute = lambda *a, **k: None
gdb.FinishBreakpoint = type('FinishBreakpoint', (object,), {'__init__': lambda self, *a, **k: None})
gdb.Breakpoint = type('Breakpoint', (object,), {'__init__': lambda self, *a, **k: None})
gdb.Command = type('Command', (object,), {'__init__': lambda self, *a, **k: None})
gdb.COMMAND_USER = 1
gdb.current_progspace = lambda: None
gdb.events = types.SimpleNamespace(exited=types.SimpleNamespace(connect=lambda f: None))
sys.modules['gdb'] = gdb
os.environ['SWB18_SELFTEST'] = '1'
os.environ.setdefault('COV_OUT', '/var/tmp/swb18/selftest-out')
os.environ.setdefault('COV_NM', '/var/tmp/swb18/nm.txt')
os.makedirs(os.environ['COV_OUT'], exist_ok=True)
ns = {'__name__': 'spec'}
exec(compile(open(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                  'swarmbasics_swb18.py')).read(),
             'tools/cov/specs/swarmbasics_swb18.py', 'exec'), ns)
cells, nearest, slot = ns['cells'], ns['nearest'], ns['slot']

c = cells((-25., -25., -25.), (25., 25., 25.))
assert c[(0, 0, 0)] == (-20., -20., -20.), c[(0, 0, 0)]
assert c[(4, 4, 4)] == (20., 20., 20.), c[(4, 4, 4)]
assert c[(2, 2, 2)] == (0., 0., 0.), c[(2, 2, 2)]
assert c[(1,0,0)] == (-10., -20., -20.)
assert slot((0, 0, 0)) == 0 and slot((4, 0, 0)) == 4 and slot((0, 1, 0)) == 5
assert slot((0, 0, 1)) == 25 and slot((4, 4, 4)) == 124
# nearest: strict < keeps the first of two equal candidates, NaN never updates
pts = [c[(0, 0, 0)], c[(4, 0, 0)]]
assert nearest(pts, c[(4, 0, 0)]) == c[(4, 0, 0)]
assert nearest(pts, c[(0, 0, 0)]) == c[(0, 0, 0)]
assert nearest([], c[(2, 2, 2)]) == (0., 0., 0.)
assert nearest(pts, (float('nan'), 0., 0.)) == (0., 0., 0.)
assert nearest(pts, (1e6, 1e6, 1e6)) == c[(4, 0, 0)]   # second is nearer at 1e6
# near() tolerance
assert ns['near']((-20.0000001, 0., 0.), (-20., 0., 0.))
assert nearest([c[(0,0,0)], c[(1,0,0)]], (-15., -20., -20.)) == c[(0,0,0)]
assert not ns['near']((1., 0., 0.), (2., 0., 0.))
# a NaN query written as float32 NaN, read back as the same expression
print('SELFTEST PASS')
