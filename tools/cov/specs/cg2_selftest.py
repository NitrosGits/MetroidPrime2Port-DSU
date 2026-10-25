"""Offline cg2 ownership/rollback tests. No inferior/game is launched."""
import importlib.util
import os
from pathlib import Path
import sys
import types
import unittest

class Memory:
    def __init__(self):
        self.raw=bytearray(range(128))
        self.calls=[]
        self.write_error=None
        self.read_error=None
    def read_memory(self,address,size):
        if self.read_error:self.read_error(address,size)
        return bytes(self.raw[address:address+size])
    def write_memory(self,address,raw):
        self.calls.append(address)
        if self.write_error:self.write_error('before',address)
        self.raw[address:address+len(raw)]=raw
        if self.write_error:self.write_error('after',address)

class Frame:
    def __init__(self,stack,token):self.stack,self.token=stack,token
    def is_valid(self):return self.token in self.stack
    def level(self):return self.stack.index(self.token)
    def older(self):
        index=self.level()+1
        return Frame(self.stack,self.stack[index]) if index<len(self.stack) else None
    def __eq__(self,other):return isinstance(other,Frame) and self.token is other.token

class Tests(unittest.TestCase):
    def setUp(self):
        self.mem=Memory();self.original=bytes(self.mem.raw)
        self.stack=[object()];self.ctx=[1,7]
        self.arm_fail=False;self.write_fail_at=None;self.setters=[];self.events=[]
        test=self
        class Finish:
            def __init__(self,frame,internal=True):
                if test.arm_fail:raise RuntimeError('cannot arm return')
        self.gdb=types.SimpleNamespace(Breakpoint=object,FinishBreakpoint=Finish,
            selected_inferior=lambda:types.SimpleNamespace(num=self.ctx[0]),
            selected_thread=lambda:types.SimpleNamespace(global_num=self.ctx[1]),
            newest_frame=lambda:Frame(self.stack,self.stack[0]),
            execute=self.execute,parse_and_eval=self.evaluate)
        sys.modules['gdb']=self.gdb
        old=os.environ.get('CG2_SELFTEST');os.environ['CG2_SELFTEST']='1'
        try:
            spec=importlib.util.spec_from_file_location('cg2',Path(__file__).with_name('cg2_prerender.py'))
            self.m=importlib.util.module_from_spec(spec);spec.loader.exec_module(self.m)
        finally:
            if old is None:os.environ.pop('CG2_SELFTEST')
            else:os.environ['CG2_SELFTEST']=old
        self.m.log=lambda event,**kw:self.events.append((event,kw))

    def evaluate(self,expression):
        if expression=='S.mAlive':return (self.mem.raw[8]>>1)&1
        if expression=='P.mCurrentVisor':return int.from_bytes(self.mem.raw[24:28],'little')
        if expression.startswith('S.mColor.m'):
            return self.mem.raw[16+'RGBA'.index(expression[-1])]
        raise RuntimeError('unmodelled '+expression)

    def execute(self,command,**kw):
        expression,value=command.removeprefix('set var ').split(' = ');value=int(value)
        self.setters.append(expression)
        if expression=='S.mAlive':self.mem.raw[8]=(self.mem.raw[8]&~2)|(value<<1)
        elif expression=='P.mCurrentVisor':self.mem.raw[24:28]=value.to_bytes(4,'little')
        else:self.mem.raw[16+'RGBA'.index(expression[-1])]=value
        if self.write_fail_at==len(self.setters):raise RuntimeError('raised after write')

    def window(self,case=None):
        snapshot=self.m.Snapshot([('flags',8,3),('color',16,4),('visor',24,4),('output',32,12)],self.mem)
        return self.m.Window(0x100001000,0x100008000,case or self.m.CASES[2],
            'S.','P.',Frame(self.stack,self.stack[0]),tuple(self.ctx),snapshot)

    def ready(self,w):
        w.base=w.bone=1;w.visor=[w.case[4],w.case[4]]
        self.m.visual=lambda _w:{'checked':'mock output; runtime separately verifies actual values'}

    def restored(self):self.assertEqual(bytes(self.mem.raw),self.original)

    def test_raw_snapshot_before_arm_and_normal_return(self):
        w=self.window();self.restored();self.assertFalse(self.setters)
        w.apply();self.ready(w)
        self.assertFalse(w.stop());self.restored()
        self.assertEqual((self.m.state['index'],self.m.state['restores'],self.m.state['failed']),(1,1,0))
        record=[kw for event,kw in self.events if event=='RESTORE'][0]
        self.assertTrue(all(f['original']==f['readback'] for f in record['fields']))

    def test_missing_return_arm_permits_no_mutation(self):
        self.arm_fail=True
        with self.assertRaises(RuntimeError):self.window()
        self.restored();self.assertFalse(self.setters);self.assertFalse(self.m.state['active'])

    def test_each_attempted_write_error_restores_exact_originals(self):
        for index in range(1,7):
            with self.subTest(index=index):
                self.setUp();w=self.window();self.write_fail_at=index
                with self.assertRaises(RuntimeError):w.apply()
                self.assertTrue(w.attempted);self.restored()
                self.assertEqual(self.m.state['restores'],1)

    def test_failed_output_oracle_finally_restores(self):
        w=self.window();w.apply()
        self.m.visual=lambda _w:(_ for _ in ()).throw(RuntimeError('oracle mismatch'))
        self.assertTrue(w.stop());self.restored()
        self.assertEqual(self.m.state['index'],0);self.assertEqual(self.m.state['failed'],1)

    def test_unwind_restores_and_fails(self):
        w=self.window();w.apply();w.out_of_scope();self.restored()
        self.assertEqual((self.m.state['index'],self.m.state['failed'],self.m.state['restores']),(0,1,1))

    def test_restore_error_attempts_all_other_regions(self):
        w=self.window();w.apply();self.ready(w)
        def error(when,address):
            if when=='before' and address==24:raise RuntimeError('visor rollback failed')
        self.mem.write_error=error
        self.assertTrue(w.stop());self.assertEqual(self.m.state['index'],0)
        self.assertEqual(self.mem.calls,[32,24,16,8])
        self.assertEqual(bytes(self.mem.raw[8:20]),self.original[8:20])
        self.assertNotEqual(bytes(self.mem.raw[24:28]),self.original[24:28])
        self.assertEqual(self.m.state['failed'],1)

    def test_post_restore_write_exception_not_false_ok(self):
        w=self.window();w.apply();self.ready(w)
        def error(when,address):
            if when=='after' and address==24:raise RuntimeError('restore raised after landing')
        self.mem.write_error=error
        self.assertTrue(w.stop());self.restored();self.assertEqual(self.m.state['failed'],1)

    def test_restore_read_failure_not_false_ok(self):
        w=self.window();w.apply();self.ready(w)
        self.mem.read_error=lambda *_:(_ for _ in ()).throw(RuntimeError('readback failed'))
        self.assertTrue(w.stop());self.restored();self.assertEqual(self.m.state['failed'],1)

    def test_silent_restore_mismatch_not_false_ok(self):
        w=self.window();w.apply();self.ready(w)
        real=self.mem.write_memory
        self.mem.write_memory=lambda a,b:None if a==24 else real(a,b)
        self.assertTrue(w.stop());self.assertEqual(self.m.state['failed'],1)
        self.assertNotEqual(bytes(self.mem.raw),self.original)

    def test_wrong_context_permits_no_restore_writes(self):
        w=self.window();w.apply();self.ready(w);self.ctx[1]=9
        before=bytes(self.mem.raw)
        self.assertTrue(w.stop());self.assertFalse(self.mem.calls)
        self.assertEqual(bytes(self.mem.raw),before);self.assertEqual(self.m.state['index'],0)

    def test_stale_finish_cannot_overwrite_another_owner(self):
        w=self.window();w.apply();self.ready(w)
        other=object();self.m.state['active'][tuple(self.ctx)]=other
        before=bytes(self.mem.raw)
        self.assertTrue(w.stop());self.assertFalse(self.mem.calls)
        self.assertEqual(bytes(self.mem.raw),before)
        self.assertIs(self.m.state['active'][tuple(self.ctx)],other)

    def test_duplicate_return_or_unwind_never_replays_snapshot(self):
        w=self.window();w.apply();self.ready(w);w.stop();self.restored()
        before=list(self.mem.calls);self.mem.raw[16]^=0x80
        self.assertTrue(w.stop());w.out_of_scope()
        self.assertEqual(self.mem.calls,before);self.assertEqual(self.m.state['index'],1)

    def test_real_changing_level_ownership_and_wrong_caller(self):
        w=self.window();self.assertEqual(w.owner.level(),0)
        callee=object();self.stack.insert(0,callee)
        self.assertEqual(w.owner.level(),1)
        self.assertIs(self.m.owned(),w)
        self.stack.insert(1,object())
        self.assertIsNone(self.m.owned());self.assertEqual(self.m.state['ignored'],1)
        self.restored()

    def test_invalid_frame_ownership_rejected(self):
        w=self.window();self.stack[:]=[object()]
        with self.assertRaises(RuntimeError):self.m.owned()
        self.restored()

    def test_recursive_entry_refused_before_new_writes(self):
        w=self.window();w.apply();before=bytes(self.mem.raw);writes=list(self.setters)
        self.gdb.parse_and_eval=lambda expr:0x100001000
        self.stack.insert(0,object())
        self.assertTrue(self.m.Entry().stop())
        self.assertEqual(self.setters,writes)
        self.restored()  # abort restores the existing owned fixture, not a new snapshot
        self.assertNotEqual(before,self.original)
        self.assertGreater(self.m.state['failed'],0)

    def test_expected_rgba_independent_boundary_table(self):
        answers=[(0,(255,255,255,255)),(5,(255,255,255,64)),(5,(255,127,127,255)),
                 (5,(255,1,1,128)),(5,(255,0,0,255)),(5,(255,0,0,0)),(5,(255,253,253,254))]
        self.assertEqual([self.m.expected(c) for c in self.m.CASES],answers)

    def test_bitfield_storage_inheritance_and_ambiguity(self):
        class Type:
            def __init__(self,fields):self._fields=fields
            def fields(self):return self._fields
            def strip_typedefs(self):return self
        field=lambda name,pos,bits=1,base=False,typ=None:types.SimpleNamespace(
            name=name,bitpos=pos,bitsize=bits,is_base_class=base,type=typ)
        parent=Type([field('flag',17)])
        child=Type([field('parent',64,0,True,parent)])
        self.assertEqual(self.m.bit_storage(child,'flag'),(10,1))
        with self.assertRaises(LookupError):self.m.bit_storage(child,'absent')
        ambiguous=Type([field('flag',17),field('parent',64,0,True,parent)])
        with self.assertRaises(RuntimeError):self.m.bit_storage(ambiguous,'flag')

    def test_summary_rejects_empty_or_pending(self):
        self.m.summary(None);self.assertEqual(self.m.state['failed'],1)
        self.window();self.m.summary(None)
        exits=[kw for event,kw in self.events if event=='EXIT']
        self.assertFalse(exits[0]['ok']);self.assertEqual(exits[1]['pending'],1)

if __name__=='__main__':unittest.main(verbosity=2)
