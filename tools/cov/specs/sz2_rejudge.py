"""OFFLINE re-judgement of an sz2 run log: the executed fixture compared the natural dt of the Think call
(about 1/60) with the case's dt=0.25 (it never wrote xmm0), so every case "failed" on dt alone.  This
reruns the same model with dt taken from the logged base_think call and compares fields and calls.
Not an executed verification of the fixture: only evidence about what the 17 windows that ran observed.
Run: python3 -I tools/cov/specs/sz2_rejudge.py <sz2.jsonl>"""
import importlib.util
import json
import os
import sys
import types
from pathlib import Path

sys.modules['gdb'] = types.SimpleNamespace(Breakpoint=object, FinishBreakpoint=object)
os.environ['SZ2_SELFTEST'] = '1'
spec = importlib.util.spec_from_file_location('sz2', Path(__file__).with_name('sz2_think.py'))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
XF = ([1, 0, 0, 3.5, 0, 1, 0, -2.25, 0, 0, 1, 7.0], (0.5, 1.0, -0.125))
bad = 0
for line in open(sys.argv[1]):
    e = json.loads(line)
    if e['event'] != 'RESULT':
        continue
    c = dict(m.D)
    c.update(dict(m.CASES)[e['case']])
    c['dt'] = e['calls'][0][1]
    final, calls, accept = m.model(c, 0, (0, 0, 0), XF)
    got = e['fields']
    probs = []
    for k, v in got.items():
        if not (v in accept[k] if k in accept else v == final[k]):
            probs.append('%s got %r expected %r' % (k, v, accept.get(k, final[k])))
    log = [tuple(x) for x in e['calls']]
    if log != calls:
        probs.append('calls got %r expected %r' % (log, calls))
    bad += bool(probs)
    print(e['case'], 'OK' if not probs else 'MISMATCH ' + '; '.join(probs), 'dt=%r fields=%r' % (c['dt'], got))
print('mismatches', bad)
sys.exit(1 if bad else 0)
