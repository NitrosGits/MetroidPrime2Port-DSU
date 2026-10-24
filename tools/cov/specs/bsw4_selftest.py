"""Mock-gdb transaction/selection tests; never claimed as native arm evidence."""
import importlib.util
import os
from pathlib import Path
import sys
import types
import unittest

class Tests(unittest.TestCase):
    def setUp(self):
        self.memory=bytearray(range(64))
        self.original=bytes(self.memory)
        self.events=[]
        self.fields={'a':(8,4),'b':(16,8)}
        test=self
        class Value:
            def __init__(self,name):
                self.address,size=test.fields[name]
                self.type=types.SimpleNamespace(sizeof=size)
        class Inferior:
            def read_memory(self,address,size):return bytes(test.memory[address:address+size])
            def write_memory(self,address,raw):test.memory[address:address+len(raw)]=raw
        gdb=types.SimpleNamespace(Breakpoint=object,FinishBreakpoint=object,Command=object,
            GdbError=RuntimeError,error=RuntimeError,
            selected_inferior=lambda:Inferior(),parse_and_eval=lambda expr:Value(expr))
        sys.modules['gdb']=gdb
        old=os.environ.get('BSW4_SELFTEST')
        os.environ['BSW4_SELFTEST']='1'
        try:
            spec=importlib.util.spec_from_file_location('bsw4',Path(__file__).with_name('bsw4_verify.py'))
            self.module=importlib.util.module_from_spec(spec);spec.loader.exec_module(self.module)
        finally:
            if old is None:os.environ.pop('BSW4_SELFTEST')
            else:os.environ['BSW4_SELFTEST']=old
        self.module.log=lambda event,**data:self.events.append((event,data))

    def check_restored(self):
        self.assertEqual(bytes(self.memory),self.original)
        event,data=self.events[-1]
        self.assertEqual(event,'RESTORE')
        self.assertFalse(data['errors'])
        for field in data['fields']:self.assertEqual(field['original'],field['readback'])

    def test_normal(self):
        tx=self.module.Transaction('native return',['a','b'])
        self.assertEqual(bytes(self.memory),self.original)
        with tx:
            self.assertEqual(self.events[-1][0],'ARM')
            self.memory[8:12]=b'abcd';self.memory[16:24]=b'abcdefgh'
        self.check_restored()

    def test_mutation_error(self):
        with self.assertRaises(ValueError):
            with self.module.Transaction('mutation error',['a','b']):
                self.memory[8:12]=b'abcd'
                raise ValueError('write failed')
        self.check_restored()

    def test_inferior_unwind(self):
        with self.assertRaises(RuntimeError):
            with self.module.Transaction('inferior unwind',['a','b']):
                self.memory[8:12]=b'abcd';self.memory[16:24]=b'abcdefgh'
                raise RuntimeError('GDB inferior-call unwind')
        self.check_restored()

    def test_oracle_error(self):
        with self.assertRaises(RuntimeError):
            with self.module.Transaction('failed oracle',['a','b']):
                self.memory[8:12]=b'abcd'
                self.module.check(False,'failed output oracle')
        self.check_restored()

    def test_nested(self):
        with self.module.Transaction('outer',['a']):
            self.memory[8:12]=b'abcd'
            with self.assertRaises(RuntimeError):
                with self.module.Transaction('inner',['a','b']):
                    self.memory[8:12]=b'wxyz';self.memory[16:24]=b'abcdefgh'
                    raise RuntimeError('nested unwind')
            self.assertEqual(bytes(self.memory[8:12]),b'abcd')
        self.check_restored()

    def test_closest(self):
        nearest=self.module.nearest
        p=[(0.,0.,0.),(2.,0.,0.)]
        self.assertEqual(nearest([],p[0]),(0.,0.,0.))
        self.assertEqual(nearest(p,(2.,0.,0.)),p[1])
        self.assertEqual(nearest(p,(0.,0.,0.)),p[0])
        self.assertEqual(nearest(p,(1.,0.,0.)),p[0])
        self.assertEqual(nearest(p,(float('nan'),0.,0.)),(0.,0.,0.))

    def test_graphics_unwind_replays_after_cpu_restore(self):
        replay=[]
        def observe():
            self.assertEqual(bytes(self.memory),self.original)
            replay.append(True)
        self.module.replay_graphics=observe
        with self.assertRaises(RuntimeError):
            with self.module.GraphicsTransaction('graphics unwind',['a']):
                self.memory[8:12]=b'abcd'
                raise RuntimeError('native light call unwound')
        self.assertEqual(replay,[True])
        self.check_restored()

    def test_release_actual_returned_handles_only(self):
        self.module.state['sound']=[{'handle':11},{'handle':0},{},{'handle':0x1005}]
        calls=[]
        self.module.iv=lambda expr:0x1005
        self.module.gdb.execute=lambda expr,**kw:calls.append(expr)
        self.module.release_sounds(1)
        self.assertEqual(len(calls),1)
        self.assertIn('RemoveEmitter',calls[0])
        self.assertIn('[5]',calls[0])

if __name__=='__main__':unittest.main()
