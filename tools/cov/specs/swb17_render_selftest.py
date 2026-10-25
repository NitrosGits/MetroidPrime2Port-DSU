# Offline test of the swb17 oracle (no gdb): python3 -I tools/cov/specs/swb17_render_selftest.py
import sys, types, unittest, os
sys.modules['gdb'] = types.SimpleNamespace(events=types.SimpleNamespace(exited=types.SimpleNamespace(connect=lambda f: None)),
                                           FinishBreakpoint=object, Breakpoint=object)
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import swb17_render as S


class Oracle(unittest.TestCase):
    def test_lighting_off(self):
        self.assertEqual(S.expected([0, 5], [(1, 1)], 0, 0, 0), (0, 0, 0))

    def test_hardware(self):
        self.assertEqual(S.expected([0, 5, 9], [(1, 1), (0, 1), (1, 0), (1, 1)], 1, 0, 0), (5, 0, 0))

    def test_software_match(self):
        # cells 0,5,9 -> &3 = 0,1,1 ; think 1 matches two cells; outlier idx 1 and 5 match
        self.assertEqual(S.expected([0, 5, 9], [(1, 1), (1, 1), (1, 1), (1, 1), (1, 1)], 1, 1, 1), (0, 4, 2))

    def test_software_miss(self):
        self.assertEqual(S.expected([4], [(1, 1)], 1, 1, 0), (0, 1, 1))

    def test_invisible_head(self):
        self.assertEqual(S.expected([], [(0, 1), (1, 0)], 1, 0, 0), (0, 0, 0))

    def test_phases_cover_all(self):
        self.assertEqual([p[0] for p in S.PHASES], list('ABCDEGF'))
        self.assertEqual(S.PERIOD % 1, 0)


if __name__ == '__main__':
    unittest.main()
