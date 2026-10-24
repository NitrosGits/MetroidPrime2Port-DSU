"""Offline srp2_render window/ownership/arithmetic tests. No inferior/game is launched."""
import importlib.util
import os
from pathlib import Path
import sys
import types
import unittest

class Frame:
    def __init__(self, stack, token):self.stack, self.token = stack, token
    def is_valid(self):return self.token in self.stack
    def level(self):return self.stack.index(self.token)
    def older(self):
        index = self.level() + 1
        return Frame(self.stack, self.stack[index]) if index < len(self.stack) else None
    def __eq__(self, other):return isinstance(other, Frame) and self.token is other.token

SITE = [dict(addr=0x10 + i, asm=a) for i, a in enumerate([
    'addss  xmm3,DWORD PTR [rip+0x1]        # a536b4 <.LC0>', 'pxor   xmm0,xmm0',
    'comiss xmm0,xmm3', 'jbe    d35468 <foo+0xdf8>', 'addss  xmm2,DWORD PTR [rip+0x2]',
    'pxor   xmm3,xmm3', 'movss  xmm1,DWORD PTR [rip+0x3]'])]

class Tests(unittest.TestCase):
    def setUp(self):
        self.stack = [object(), object(), object()]  # [0]=newest
        self.ctx = [1, 7]
        self.xmm = {'xmm2': 0.25, 'xmm3': -0.5}
        self.events = []
        test = self
        class Finish:
            def __init__(self, frame, internal=True):
                if test.arm_fail:raise RuntimeError('cannot arm return')
        self.arm_fail = False
        self.executed = []
        self.gdb = types.SimpleNamespace(Breakpoint=object, FinishBreakpoint=Finish,
            selected_inferior=lambda:types.SimpleNamespace(num=self.ctx[0]),
            selected_thread=lambda:types.SimpleNamespace(global_num=self.ctx[1]),
            newest_frame=lambda:Frame(self.stack, self.stack[self.top]),
            execute=self.execute, parse_and_eval=lambda e:0)
        self.top = 1  # newest frame == owner (stack[1]); stack[0] is a callee, stack[2] a caller
        sys.modules['gdb'] = self.gdb
        old = os.environ.get('SRP2_SELFTEST');os.environ['SRP2_SELFTEST'] = '1'
        try:
            spec = importlib.util.spec_from_file_location('srp2r', Path(__file__).with_name('srp2_render.py'))
            self.m = importlib.util.module_from_spec(spec);spec.loader.exec_module(self.m)
        finally:
            if old is None:os.environ.pop('SRP2_SELFTEST')
            else:os.environ['SRP2_SELFTEST'] = old
        self.m.log = lambda event, **kw:self.events.append((event, kw))
        self.m.xmm = lambda n:self.xmm[n]
        self.m.xmm_bits = lambda n:int(self.xmm[n] * 1000) & 0xffffffff
        self.m.viewport = lambda:(0, 0, 640, 448)

    def execute(self, command, to_string=False):
        self.executed.append(command)
        self.xmm['xmm2'] = float(command.rsplit(' = ', 1)[1])

    def window(self, case):
        return self.m.Window(0x1000, Frame(self.stack, self.stack[1]), tuple(self.ctx), case)

    def play(self, w, mutate=None):
        w.at_site()
        left, top, width, height = w.site['viewport']
        want = self.m.expected_outputs(w.site['min_x'], w.site['max_y'], width, height, left, top)
        w.copies.append(list(want['copy']));w.trans.append(want['trans'])
        if mutate:mutate(w)
        return want

    def test_find_site_unique_and_required(self):
        self.assertEqual(self.m.find_site(SITE), 0x10)
        with self.assertRaises(RuntimeError):self.m.find_site(SITE[1:])
        with self.assertRaises(RuntimeError):self.m.find_site(SITE + SITE)

    def test_arithmetic_arms(self):
        lo = self.m.expected_outputs(-0.5, -1.5, 640, 448, 0, 0)
        self.assertEqual((lo['arm'], lo['copy_y'], lo['trans'][1]), ('min', 448, -1.0))
        hi = self.m.expected_outputs(-0.5, 1.5, 640, 448, 0, 0)
        self.assertEqual((hi['arm'], hi['copy_y'], hi['trans'][1]), ('max', 0, 1.0))
        mid = self.m.expected_outputs(-0.5, 0.25, 640, 448, 3, 5)
        self.assertEqual(mid['arm'], 'mid')
        self.assertEqual(mid['copy'][2:], (320, 224))
        self.assertEqual(mid['copy'][0], (int(0.5 * 0.5 * 640) & ~1) + 3)

    def test_each_case_counts_and_forces_register_only(self):
        for case in self.m.CASES:
            w = self.window(case);self.play(w)
            self.assertFalse(w.stop())
        self.assertEqual((self.m.state['index'], self.m.state['failed']), (4, 0))
        self.assertEqual(len(self.executed), 2)  # only the two forcing cases write
        self.assertTrue(all('xmm2' in c for c in self.executed))
        forced = [kw for e, kw in self.events if e == 'APPLY' and kw['case'] in ('y-min', 'y-max')]
        self.assertTrue(all(kw['max_y'] == kw['max_y'] and kw['readback_bits'] != kw['original_bits']
                            for kw in forced))

    def test_early_return_is_not_progress(self):
        w = self.window(self.m.CASES[0])
        self.assertFalse(w.stop())
        self.assertEqual((self.m.state['index'], self.m.state['early'], self.m.state['failed']), (0, 1, 0))

    def test_wrong_output_fails(self):
        w = self.window(self.m.CASES[1])
        self.play(w, lambda w:w.copies.__setitem__(0, [0, 0, 1, 1]))
        self.assertTrue(w.stop());self.assertEqual(self.m.state['failed'], 1)
        self.assertEqual(self.m.state['index'], 0)

    def test_wrong_trans_and_missing_calls_fail(self):
        w = self.window(self.m.CASES[2])
        self.play(w, lambda w:w.trans.__setitem__(0, (0.0, 0.0)))
        self.assertTrue(w.stop())
        self.setUp();w = self.window(self.m.CASES[2]);w.at_site()
        self.assertTrue(w.stop())

    def test_forced_arm_must_match(self):
        w = self.window(('y-min', -1.5, 'max'))
        self.play(w)
        self.assertTrue(w.stop())

    def test_arm_failure_blocks_before_any_write(self):
        self.arm_fail = True
        with self.assertRaises(RuntimeError):self.window(self.m.CASES[1])
        self.assertFalse(self.executed);self.assertFalse(self.m.state['active'])

    def test_nested_entry_rejected(self):
        self.window(self.m.CASES[0])
        with self.assertRaises(RuntimeError):self.window(self.m.CASES[1])

    def test_foreign_frame_and_context_ignored(self):
        w = self.window(self.m.CASES[1])
        self.assertIs(self.m.owned(0), w)
        self.top = 2;self.assertIsNone(self.m.owned(0))  # the caller is not the owner
        self.top = 0;self.assertIs(self.m.owned(1), w)   # a callee one level deeper reaches it
        self.assertIsNone(self.m.owned(0))
        self.top = 1;self.ctx[1] = 8
        self.assertIsNone(self.m.owned(0))  # other thread has no window
        self.ctx[1] = 7;self.assertEqual(self.m.state['ignored'], 2)

    def test_site_in_foreign_frame_rejected_without_write(self):
        w = self.window(self.m.CASES[1]);self.top = 2
        with self.assertRaises(RuntimeError):w.at_site()
        self.assertFalse(self.executed)

    def test_unwind_fails_and_closes(self):
        w = self.window(self.m.CASES[1]);w.out_of_scope()
        self.assertEqual(self.m.state['failed'], 1);self.assertFalse(self.m.state['active'])
        w.out_of_scope();self.assertEqual(self.m.state['failed'], 2)
        self.assertTrue(w.stop())

    def test_summary_requires_all_cases(self):
        self.m.summary(None)
        self.assertFalse([kw for e, kw in self.events if e == 'EXIT'][0]['ok'])

if __name__ == '__main__':
    unittest.main()
