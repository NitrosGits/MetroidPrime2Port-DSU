#!/usr/bin/env python3
"""Scanner fixtures and isolated actual-source probes; no game build required.

python3 tools/audit/test_upstream_gconly.py
Uses CXX (default c++) and UBSan; generated stubs replace stream/SDK plumbing,
not the implementation under test. This does not execute the game or assets.
"""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import upstream_gconly as audit


class ScanTests(unittest.TestCase):
    def test_pc_branches(self):
        text = 'int shared;\n#ifdef TARGET_PC\nint handled;\n#else\nint retail;\n#endif\nint tail;\n'
        active = audit.allowed(text)
        self.assertIn(1, active)
        self.assertNotIn(3, active)
        self.assertNotIn(5, active)
        self.assertIn(7, active)

    def test_inventory_and_headers(self):
        with tempfile.TemporaryDirectory(prefix='gconly-', dir='/tmp/opencode') as tmp:
            root = Path(tmp)
            files = {
                'configure.py': 'Object(MatchingFor("G2ME01"), "a.cpp")\n'
                                'Object(Matching, "b.cpp")\n'
                                'Object(NonMatching, "c.cpp")\n'
                                'Object(MatchingFor("G2MP01"), "d.cpp")\n'
                                'Object(Matching, "PortE.cpp")\n',
                'files.cmake': 'src/a.cpp src/b.cpp src/c.cpp src/d.cpp src/PortE.cpp',
                'src/a.cpp': '#include "shared.hpp"\nint f(float x) { return (int)x; }\n',
                'src/b.cpp': '#include "shared.hpp"\n',
                'src/c.cpp': 'char ignored;\n',
                'src/d.cpp': 'char ignored;\n',
                'src/PortE.cpp': 'char ignored;\n',
                'include/shared.hpp': '#include "shared.hpp"\nchar sentinel;\n'
                                      '// int not_code = (int)x;\n',
            }
            for name, text in files.items():
                p = root / name
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_text(text)
            with patch.object(audit, 'ROOT', root):
                self.assertEqual(audit.units(), ['src/a.cpp', 'src/b.cpp'])
                result = audit.scan()
                self.assertEqual(result['files_scanned'], 3)
                h = next(h for h in result['hits'] if h['category'] == 1)
                self.assertEqual(h['owners'], ['src/a.cpp', 'src/b.cpp'])
                self.assertFalse(any(h['line'] == 3 and h['path'].endswith('.hpp')
                                     for h in result['hits']))
                self.assertEqual(audit.scan(False)['files_scanned'], 2)

    def test_actual_bitreader_and_casts(self):
        with tempfile.TemporaryDirectory(prefix='gconly-probe-', dir='/tmp/opencode') as tmp:
            root = Path(tmp)
            stubs = {
                'types.h': '#pragma once\nusing uint=unsigned; using uchar=unsigned char; '
                           'using ushort=unsigned short; using s64=long long;\n'
                           '#define CHECK_SIZEOF(...)\n',
                'Kyoto/Alloc/CMemory.hpp': '',
                'dolphin/os/OSFastCast.h': '',
                'Kyoto/Streams/CInputStream.hpp': '#pragma once\n#include <cstring>\n'
                    'class CInputStream { const unsigned char* p; public:\n'
                    'explicit CInputStream(const unsigned char* p):p(p) {}\n'
                    'void Get(void* dst, unsigned n) { std::memcpy(dst,p,n);p+=n; } };\n',
                'probe.cpp': '#include <cstdio>\n#include <climits>\n'
                    '#include "Kyoto/Streams/CInputStream.hpp"\n'
                    '#include "Kyoto/Streams/CBitStreamReader.hpp"\n'
                    '#include "Kyoto/Basics/CCast.hpp"\n'
                    '#include "Kyoto/Audio/CSfxPitchBend.hpp"\n'
                    'int main() {\n'
                    'const unsigned char bytes[]={0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef};\n'
                    'CInputStream in(bytes); CBitStreamReader reader(in);\n'
                    'unsigned a=reader.ReadBits(32), b=reader.ReadBits(32);\n'
                    'volatile float large=3e9f, narrow=40000.f;\n'
                    'int converted=CCast::ToInt32(large);\n'
                    'short s=CCast::FtoS(narrow);\n'
                    'CSfxPitchBend bend(CSfxHandle(),8192,16383,0.005f); bend.Update(0.025f);\n'
                    'std::printf("words=%08x,%08x int=%d short=%d\\n",a,b,converted,s);\n'
                    'std::printf("pitch=%u (retail saturating step reaches target 16383)\\n",'
                    'unsigned(bend.GetPitch()));\n'
                    'return a!=0x12345678u || b!=0x90abcdefu; }\n',
            }
            for name, text in stubs.items():
                p = root / name
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_text(text)
            exe = root / 'probe'
            cmd = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-DTARGET_PC',
                   '-fsigned-char', '-fsanitize=undefined,float-cast-overflow',
                   '-I' + str(root), '-I' + str(audit.ROOT / 'include'),
                   str(root / 'probe.cpp'),
                   str(audit.ROOT / 'src/Kyoto/Streams/CBitStreamReader.cpp'),
                   str(audit.ROOT / 'src/Kyoto/Audio/CSfxPitchBend.cpp'), '-o', str(exe)]
            subprocess.run(cmd, check=True, capture_output=True, text=True)
            run = subprocess.run([str(exe)], capture_output=True, text=True,
                                 env={**os.environ, 'UBSAN_OPTIONS': 'halt_on_error=0'})
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
            self.assertIn('shift exponent 32', run.stderr)
            self.assertIn('outside the range', run.stderr)
            print('\nActual-source probe (synthetic values; UBSan recovery):\n' +
                  run.stdout + run.stderr)


if __name__ == '__main__':
    unittest.main()
