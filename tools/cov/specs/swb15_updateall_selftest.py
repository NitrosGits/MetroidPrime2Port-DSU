"""Offline GDB model: real changing levels, fresh wrappers, byte-exact memory.

No GDB inferior and no game launch. Logs are appended, never remove old evidence.
Run: python3 tools/cov/specs/swb15_updateall_selftest.py
"""
import os
from pathlib import Path
import re
import struct
import sys
import tempfile
import types
import unittest

BODY = 'CSwarmBasics::UpdateAllBoidMovement(CStateManager&, float)'
MOVE = 'CSwarmBasics::MoveBoid(CStateManager&, CSwarmBasics::CBoid&, CVector3f const&, float)'
THINK = 'CSwarmBasics::Think(float, CStateManager&)'
A, B = 0x100001000, 0x100009000  # deliberately above 32 bits
OUT = os.environ.get('COV_OUT') or tempfile.mkdtemp(prefix='swb15-sol-offline-', dir='/tmp/opencode')
Path(OUT).mkdir(parents=True, exist_ok=True)


class Activation:
    def __init__(self, name):
        self.name = name
        self.invalid = False
        self.error = None


class Frame:
    def __init__(self, harness, activation):
        self.h, self.activation = harness, activation

    def _error(self, method):
        if self.activation.error == method:
            raise RuntimeError('unreadable ' + method)

    def is_valid(self):
        self._error('valid')
        return not self.activation.invalid and self.activation in self.h.stack

    def level(self):
        self._error('level')
        return self.h.stack.index(self.activation)  # NOT an entry-time constant

    def name(self):
        self._error('name')
        return self.activation.name

    def older(self):
        self._error('older')
        index = self.level() + 1
        return Frame(self.h, self.h.stack[index]) if index < len(self.h.stack) else None

    def __eq__(self, other):
        self._error('equal')
        if isinstance(other, Frame):
            other._error('equal')
            return self.h is other.h and self.activation is other.activation
        return False


class Harness:
    def __init__(self):
        self.stack = [Activation(THINK)]
        self.memory, self.models, self.addresses = {}, {}, {}
        self.regs, self.writes, self.breakpoints = {}, [], []
        self.inferior, self.thread = 1, 10
        self.arm_fail = False
        self.write_hook = None
        self.read_hook = None
        self.ignore_write = None
        h = self

        class Finish:
            def __init__(self, frame, internal=True):
                if h.arm_fail:
                    raise RuntimeError('cannot arm finish breakpoint')
                self.frame, self.enabled = frame, True
                h.breakpoints.append(self)

        self.gdb = types.ModuleType('gdb')
        self.gdb.FinishBreakpoint = Finish
        self.gdb.newest_frame = lambda: Frame(self, self.stack[0]) if self.stack else None
        self.gdb.selected_inferior = lambda: types.SimpleNamespace(num=self.inferior)
        self.gdb.selected_thread = lambda: types.SimpleNamespace(global_num=self.thread)
        self.gdb.parse_and_eval = self.evaluate
        self.gdb.execute = self.execute
        self.gdb.events = types.SimpleNamespace(exited=types.SimpleNamespace(connect=lambda fn: None))
        sys.modules['gdb'] = self.gdb
        self.ns = {'__name__': 'swb15_offline'}
        old = os.environ.get('COV_OUT')
        os.environ['COV_OUT'] = OUT
        try:
            path = Path(__file__).with_name('swb15_updateall.py')
            exec(compile(path.read_text(), str(path), 'exec'), self.ns)
        finally:
            if old is None:
                os.environ.pop('COV_OUT')
            else:
                os.environ['COV_OUT'] = old
        self.st = self.ns['_st']

    def add(self, swarm=A, count=3, animated=1):
        raw = bytearray(range(16))
        raw[0] = 0xA1 | (animated << 2)  # retain unrelated flag bits exactly
        struct.pack_into('<i', raw, 4, count)
        self.memory[swarm] = raw
        self.models[swarm] = dict(cap=50, models=4, adv=4)
        for offset, field in ((0x100, 'boids'), (0x200, 'models'), (0x300, 'adv')):
            self.addresses[swarm + offset] = (swarm, field)

    def original(self):
        return {address: bytes(raw) for address, raw in self.memory.items()}

    def value(self, swarm, field):
        raw = self.memory[swarm]
        return (raw[0] >> 2) & 1 if field == 'anim' else struct.unpack_from('<i', raw, 4)[0]

    def _location(self, expression):
        if 'mAnimated' in expression:
            swarm = int(re.search(r'CSwarmBasics\*\)(\d+)', expression).group(1))
            return swarm, 'anim'
        address = int(re.search(r'\*\)(\d+)\)->', expression).group(1))
        return self.addresses[address][0], 'count'

    def evaluate(self, expression):
        if self.read_hook:
            self.read_hook(expression)
        if expression in self.regs:
            return self.regs[expression]
        if expression == '$xmm0.v4_float[0]':
            return 1. / 60.
        if 'sizeof(CSwarmBasics::CBoid)' in expression:
            return 192
        if 'sizeof(CAdvancementDeltas)' in expression:
            return 28
        if expression.startswith('&((CSwarmBasics*)'):
            swarm = int(re.search(r'CSwarmBasics\*\)(\d+)', expression).group(1))
            return swarm + (0x100 if 'mBoids' in expression else
                            0x200 if 'mModelDatas' in expression else 0x300)
        if 'mAnimated' in expression:
            return self.value(*self._location(expression))
        address = int(re.search(r'\*\)(\d+)\)->', expression).group(1))
        swarm, kind = self.addresses[address]
        if expression.endswith('mItems'):
            return swarm + (0x1000 if kind == 'boids' else 0x3000)
        if expression.endswith('mCapacity'):
            return self.models[swarm]['cap']
        if expression.endswith('mCount'):
            return self.value(swarm, 'count') if kind == 'boids' else self.models[swarm][kind]
        raise RuntimeError('unmodelled expression: ' + expression)

    def execute(self, command):
        expression, text = command.removeprefix('set var ').rsplit(' = ', 1)
        swarm, field = self._location(expression)
        value = int(text)
        self.writes.append((swarm, field, value))
        if self.write_hook:
            self.write_hook('before', swarm, field, value)
        if self.ignore_write and self.ignore_write(swarm, field, value):
            pass
        elif field == 'anim':
            self.memory[swarm][0] = (self.memory[swarm][0] & ~4) | ((value & 1) << 2)
        else:
            struct.pack_into('<i', self.memory[swarm], 4, value)
        if self.write_hook:
            self.write_hook('after', swarm, field, value)

    def enter(self, swarm=A, phase=None):
        if phase is not None:
            self.st['calls'] = (phase - 1) % 3
        activation = Activation(BODY)
        self.stack.insert(0, activation)
        self.regs.update({'$rdi': swarm, '$rsi': 0x2000})
        before = len(self.breakpoints)
        self.ns['body_fixture'](self.gdb)
        return self.breakpoints[-1] if len(self.breakpoints) > before else None

    def hit(self, swarm=A, index=0, error=None, bad_mgr=False):
        self.regs.update({'$rdi': swarm, '$rsi': 99 if bad_mgr else 0x2000,
                          '$rdx': swarm + 0x1000 + index * 192,
                          '$rcx': swarm + 0x3000 + (index & 3) * 28})
        move = Activation(MOVE)
        move.error = error
        self.stack.insert(0, move)
        try:
            self.ns['moveboid_oracle'](self.gdb)
        finally:
            self.stack.remove(move)

    def finish(self, bp, unwind=False):
        # At a return breakpoint the owning frame has already left the stack.
        if bp.frame.activation in self.stack:
            self.stack.remove(bp.frame.activation)
        return bp.out_of_scope() if unwind else bp.stop()


class Tests(unittest.TestCase):
    def setUp(self):
        self.h = Harness()
        self.h.add()
        self.h.add(B, count=7)
        self.original = self.h.original()

    def restored(self):
        self.assertEqual(self.h.original(), self.original)

    def counters(self, **expected):
        for key, value in expected.items():
            self.assertEqual(self.h.st[key], value, key)

    def test_real_zero_to_one_levels_and_new_wrappers(self):
        h = self.h
        bp = h.enter()
        self.assertEqual(bp.win['entry_level'], 0)
        alias = Frame(h, bp.frame.activation)
        self.assertIsNot(alias, bp.frame)
        self.assertEqual(alias, bp.frame)
        move = Activation(MOVE)
        h.stack.insert(0, move)
        self.assertEqual(bp.frame.level(), 1)
        h.stack.remove(move)
        for i in range(3):
            h.hit(index=i)
        self.assertFalse(h.finish(bp))
        self.counters(opens=1, closed=1, mvHits=3, unattributed=0, ownershipFail=0)
        self.assertTrue(h.ns['_summary']())
        self.restored()

    def test_all_phases_exact_memory(self):
        for phase in (0, 1, 2):
            with self.subTest(phase=phase):
                h = Harness(); h.add(count=5)
                original = h.original()
                bp = h.enter(phase=phase)
                self.assertEqual(h.value(A, 'anim'), phase != 0)
                self.assertEqual(h.value(A, 'count'), 0 if phase == 2 else 5)
                if phase == 1:
                    for i in range(5): h.hit(index=i)
                self.assertFalse(h.finish(bp))
                self.assertEqual(h.original(), original)
                self.assertEqual((h.st['opens'], h.st['closed'], h.st['restoreFail']), (1, 1, 0))
                self.assertTrue(h.ns['_summary']())

    def test_recursive_same_this_inner_attribution_and_lifo_restore(self):
        h = self.h
        outer = h.enter(phase=0)  # animated1 -> 0, next call is ph1, not duplicate
        inner = h.enter()        # same-this recursive activation animated0 -> 1
        self.assertEqual((outer.frame.level(), inner.frame.level()), (1, 0))
        for i in range(3): h.hit(index=i)
        self.assertEqual((outer.win['mv'], inner.win['mv']), (0, 3))
        self.assertFalse(h.finish(inner))
        self.assertEqual(h.value(A, 'anim'), 0)
        self.assertFalse(h.finish(outer))
        self.counters(opens=2, closed=2, mvHits=3, unattributed=0, dupEntry=0)
        self.assertTrue(h.ns['_summary']())
        self.restored()

    def test_nested_count_snapshot_resume_outer(self):
        h = self.h
        outer = h.enter()  # ph1
        inner = h.enter()  # ph2: count lowered to zero
        self.assertEqual(h.value(A, 'count'), 0)
        self.assertFalse(h.finish(inner))
        self.assertEqual(h.value(A, 'count'), 3)
        for i in range(3): h.hit(index=i)
        self.assertFalse(h.finish(outer))
        self.assertTrue(h.ns['_summary']())
        self.restored()

    def test_duplicate_entry_same_real_frame_no_new_writes(self):
        h = self.h; bp = h.enter(phase=0)
        before = list(h.writes)
        h.ns['body_fixture'](h.gdb)  # fresh wrapper, same actual activation
        self.assertEqual(h.writes, before)
        self.counters(opens=1, dupEntry=1)
        h.finish(bp); self.restored()
        self.assertFalse(h.ns['_summary']())

    def test_same_this_same_name_wrong_activation_rejected(self):
        h = self.h; bp = h.enter()
        h.stack.insert(0, Activation(BODY))  # same function, immediate caller isn't owner
        h.hit()
        self.counters(mvHits=0, unattributed=1, ownershipFail=0)
        self.assertIn('move-no-owner', h.st['status'])
        h.stack.pop(0); h.finish(bp); self.restored()

    def test_thunk_not_immediate_owner_rejected(self):
        h = self.h; bp = h.enter()
        h.stack.insert(0, Activation('thunk'))
        h.hit(); self.counters(mvHits=0, unattributed=1)
        h.stack.pop(0); h.finish(bp); self.restored()

    def test_missing_and_unreadable_frames(self):
        for error in ('missing', 'name', 'level', 'valid', 'equal'):
            with self.subTest(error=error):
                h = Harness(); h.add(); bp = h.enter()
                original = h.original()
                saved = list(h.stack)
                if error == 'missing': h.stack.clear()
                else: bp.frame.activation.error = error
                h.hit()
                self.assertEqual((h.st['mvHits'], h.st['unattributed'], h.st['ownershipFail']), (0, 1, 1))
                h.stack[:] = saved; bp.frame.activation.error = None
                h.finish(bp)
                self.assertEqual(h.original(), original)
                self.assertFalse(h.ns['_summary']())

    def test_invalid_saved_frame_rejected(self):
        h = self.h; bp = h.enter(); bp.frame.activation.invalid = True
        h.hit(); self.counters(mvHits=0, unattributed=1, ownershipFail=1)
        h.finish(bp); self.restored()

    def test_unknown_thread_rejected(self):
        h = self.h; bp = h.enter(); h.thread = None
        h.hit(); self.counters(mvHits=0, unattributed=1, ownershipFail=1)
        h.thread = 10; h.finish(bp); self.restored()

    def test_wrong_thread_or_inferior_rejected(self):
        for field in ('thread', 'inferior'):
            with self.subTest(field=field):
                h = Harness(); h.add(); bp = h.enter()
                original = h.original()
                old = getattr(h, field); setattr(h, field, old + 1)
                h.hit()
                self.assertEqual((h.st['mvHits'], h.st['unattributed']), (0, 1))
                setattr(h, field, old); h.finish(bp)
                self.assertEqual(h.original(), original)

    def test_ambiguous_owners_rejected_no_counters_folded(self):
        h = self.h; bp = h.enter()
        shadow = dict(bp.win); h.st['order'].append(shadow); h.st['open'][id(shadow)] = shadow
        h.hit()
        self.counters(mvHits=0, unattributed=1)
        self.assertEqual((bp.win['mv'], shadow['mv']), (0, 0))
        self.assertIn('move-ambiguous', h.st['status'])
        h.st['order'].pop(); h.st['open'].pop(id(shadow))
        h.finish(bp); self.restored()

    def test_out_of_order_same_this_cancel_then_restore(self):
        h = self.h; outer = h.enter(phase=0); inner = h.enter()
        self.assertTrue(h.finish(outer))
        self.restored()
        self.counters(opens=2, closed=1, unwound=1, orderFail=1, restoreFail=0)
        before = list(h.writes)
        self.assertTrue(h.finish(inner))
        self.assertEqual(h.writes, before)
        self.counters(lateFinish=1, closed=1, unwound=1)
        self.assertFalse(h.ns['_summary']())

    def test_distinct_swarm_restore_uses_record_not_register(self):
        h = self.h; outer = h.enter(phase=2); inner = h.enter(B)
        self.assertEqual(h.regs['$rdi'], B)
        self.assertTrue(h.finish(outer))  # deliberately out of order; restore B then A
        self.restored(); self.counters(orderFail=1, restoreFail=0)
        h.finish(inner); self.restored()

    def test_duplicate_finish_does_not_rewrite_later_memory(self):
        h = self.h; bp = h.enter(phase=0); h.finish(bp); self.restored()
        h.memory[A][0] ^= 0x20
        before, writes = h.original(), list(h.writes)
        self.assertTrue(bp.stop())
        self.assertEqual(h.original(), before); self.assertEqual(h.writes, writes)
        self.counters(closed=1, lateFinish=1)

    def test_unwind_restores_forced_memory_once(self):
        h = self.h; bp = h.enter(phase=2)
        h.finish(bp, unwind=True); self.restored()
        self.counters(closed=0, unwound=1, restoreFail=0)
        self.assertFalse(h.ns['_summary']())
        before = list(h.writes); bp.out_of_scope()
        self.assertEqual(h.writes, before); self.counters(lateFinish=1)

    def test_write_raises_after_mutating_rollback_exact(self):
        h = self.h
        def error(when, swarm, field, value):
            if when == 'after' and len(h.writes) == 2: raise RuntimeError('post-write failure')
        h.write_hook = error
        bp = h.enter(phase=2)
        self.restored()
        self.assertEqual(bp.win['attempted'], ['anim', 'count'])
        self.counters(err=1, rollbackFail=0, apply=0)
        before = list(h.writes); h.finish(bp)
        self.assertEqual(h.writes, before); self.restored()
        self.assertFalse(h.ns['_summary']())

    def test_forced_read_failure_rolls_back_both_fields(self):
        h = self.h; failed = []
        def error(expr):
            if h.writes and not failed and 'mAnimated' in expr:
                failed.append(True); raise RuntimeError('forced read failed')
        h.read_hook = error
        bp = h.enter(phase=2)
        self.restored(); self.counters(err=1, rollbackFail=0)
        h.finish(bp); self.assertFalse(h.ns['_summary']())

    def test_rollback_failure_attempts_remaining_field_and_poison(self):
        h = self.h
        h.memory[A][0] &= ~4  # force a real animation change as well as count change
        self.original = h.original()
        def error(when, swarm, field, value):
            if when == 'after' and len(h.writes) == 2: raise RuntimeError('write landed then failed')
            if when == 'before' and field == 'anim' and len(h.writes) >= 3:
                raise RuntimeError('anim rollback fails')
        h.write_hook = error
        bp = h.enter(phase=2)
        self.counters(err=1, rollbackFail=1, apply=0)
        self.assertTrue(h.st['poisoned'])
        self.assertEqual((h.value(A, 'anim'), h.value(A, 'count')), (1, 3))
        expected = dict(self.original)
        changed = bytearray(expected[A]); changed[0] |= 4; expected[A] = bytes(changed)
        self.assertEqual(h.original(), expected)
        before = list(h.writes)
        self.assertIsNone(h.enter(B)); self.assertEqual(h.writes, before)
        self.assertTrue(h.finish(bp)); self.counters(restoreFail=1, refused=1)
        self.assertEqual(h.original(), expected)
        self.assertEqual(h.writes, [(A, 'anim', 1), (A, 'count', 0), (A, 'anim', 0),
                                    (A, 'count', 3), (A, 'anim', 0), (A, 'count', 3)])
        self.assertFalse(h.ns['_summary']())

    def test_restore_exception_after_land_is_still_failure(self):
        h = self.h; bp = h.enter(phase=2)
        def error(when, swarm, field, value):
            if when == 'after' and field == 'anim': raise RuntimeError('raised after restore landed')
        h.write_hook = error
        self.assertTrue(h.finish(bp)); self.restored()
        self.counters(restoreFail=1)
        self.assertFalse(h.ns['_summary']())

    def test_restore_readback_failure_not_false_ok(self):
        h = self.h; bp = h.enter(phase=0)
        h.read_hook = lambda expr: (_ for _ in ()).throw(RuntimeError('readback unavailable'))
        self.assertTrue(h.finish(bp)); self.restored()
        self.counters(restoreFail=1)
        self.assertFalse(h.ns['_summary']())

    def test_wrong_restore_context_no_unsafe_writes(self):
        h = self.h; bp = h.enter(phase=0); before = list(h.writes)
        h.inferior = 2
        self.assertTrue(h.finish(bp)); self.assertEqual(h.writes, before)
        self.counters(restoreFail=1)
        expected = dict(self.original)
        changed = bytearray(expected[A]); changed[0] &= ~4; expected[A] = bytes(changed)
        self.assertEqual(h.original(), expected)
        self.assertFalse(h.ns['_summary']())

    def test_missing_return_breakpoint_no_mutation(self):
        h = self.h; h.arm_fail = True
        self.assertIsNone(h.enter(phase=0)); self.assertFalse(h.writes); self.restored()
        self.counters(opens=0, noRestore=1)
        self.assertFalse(h.ns['_summary']())

    def test_missing_entry_frame_no_writes(self):
        h = self.h; h.stack.clear()
        h.ns['body_fixture'](h.gdb)
        self.assertFalse(h.writes); self.restored(); self.counters(opens=0, err=1)
        self.assertFalse(h.ns['_summary']())

    def test_pending_summary_idempotent_and_no_false_restore(self):
        h = self.h; bp = h.enter(phase=2)
        before = h.original()
        self.assertFalse(h.ns['_summary']()); self.assertFalse(h.ns['_summary']())
        self.counters(pending=1, opens=1, closed=0)
        self.assertEqual(h.original(), before)
        self.assertEqual(sum(s.startswith('pending:') for s in h.st['status']), 1)
        h.finish(bp); self.restored()

    def test_no_windows_summary_is_failure(self):
        self.assertFalse(self.h.ns['_summary']())
        self.counters(opens=0, closed=0)
        self.restored()

    def test_bad_arguments_attributed_but_not_accepted_as_correct(self):
        h = self.h; bp = h.enter(); h.hit(bad_mgr=True)
        self.assertEqual((bp.win['mv'], bp.win['bad']), (1, 1))
        self.counters(mvHits=1, unattributed=0)
        self.assertTrue(h.finish(bp)); self.restored()
        self.assertFalse(h.ns['_summary']())

    def test_unreadable_immediate_older_frame(self):
        h = self.h; bp = h.enter()
        h.hit(error='older')
        self.counters(mvHits=0, unattributed=1, ownershipFail=1)
        h.finish(bp); self.restored()

    def test_bad_outer_frame_does_not_fall_back_to_matching_inner(self):
        h = self.h; outer = h.enter(phase=0); inner = h.enter()
        outer.frame.activation.error = 'equal'
        h.hit()
        self.counters(mvHits=0, unattributed=1, ownershipFail=1)
        self.assertEqual((outer.win['mv'], inner.win['mv']), (0, 0))
        outer.frame.activation.error = None
        h.finish(inner); h.finish(outer); self.restored()

    def test_silent_forced_write_mismatch_rolls_back(self):
        h = self.h; h.ignore_write = lambda swarm, field, value: field == 'count' and value == 0
        bp = h.enter(phase=2)
        self.restored(); self.counters(err=1, rollbackFail=0, apply=0)
        h.finish(bp); self.assertFalse(h.ns['_summary']())

    def test_silent_restore_mismatch_not_claimed_restored(self):
        h = self.h; bp = h.enter(phase=2)
        h.ignore_write = lambda swarm, field, value: field == 'count' and value == 3
        self.assertTrue(h.finish(bp)); self.counters(restoreFail=1)
        expected = dict(self.original)
        altered = bytearray(expected[A]); struct.pack_into('<i', altered, 4, 0)
        expected[A] = bytes(altered)
        self.assertEqual(h.original(), expected)
        self.assertFalse(h.ns['_summary']())

    def test_ambiguous_finish_order_does_not_guess_snapshot(self):
        h = self.h; bp = h.enter(phase=0)
        h.st['order'].append(bp.win)
        before, writes = h.original(), list(h.writes)
        self.assertTrue(h.finish(bp))
        self.assertEqual(h.original(), before); self.assertEqual(h.writes, writes)
        self.counters(orderFail=1, restoreFail=1, unwound=1)
        self.assertFalse(h.ns['_summary']())

    def test_summary_checks_balance_not_just_empty_pending(self):
        h = self.h; bp = h.enter(phase=0); h.finish(bp); self.restored()
        h.st['closed'] += 1
        self.assertFalse(h.ns['_summary']())
        self.assertIn('window-accounting', h.st['status'])


if __name__ == '__main__':
    print('Offline log directory:', OUT, flush=True)
    unittest.main(verbosity=2)
