#!/usr/bin/env python3
"""Per-branch counts of source lines from the gcov data of one or more runs (what gcov_report.py's `line:k/m`
names mean): branch_lines.py <build dir> <source suffix> <first>-<last> <prefix dir>...
Prints, per line in the range, its count and every branch edge (count, `throw` for exception edges)."""
import pathlib
import sys
import tempfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import gcov_report as g  # noqa: E402

build = pathlib.Path(sys.argv[1]).resolve()
suffix = sys.argv[2]
lo, hi = [int(x) for x in sys.argv[3].split('-')]
prefixes = [pathlib.Path(p).resolve() for p in sys.argv[4:]]
with tempfile.TemporaryDirectory(prefix='gcov-merge-') as tmp:
    merged = g.merge_prefixes(prefixes, pathlib.Path(tmp))
    docs = g.run_gcov(build, [(merged, g.find_gcda(merged))])
for doc in docs:
    for entry in doc.get('files', []):
        if not entry.get('file', '').endswith(suffix):
            continue
        for line in entry.get('lines', []):
            n = line.get('line_number', 0)
            if lo <= n <= hi and line.get('branches'):
                print('%d count=%s fn=%s' % (n, line.get('count'), line.get('function_name', '')[:40]))
                for k, b in enumerate(line['branches']):
                    print('   edge %d count=%s%s%s' % (k, b.get('count'), ' throw' if b.get('throw') else '',
                                                        ' fallthrough' if b.get('fallthrough') else ''))
