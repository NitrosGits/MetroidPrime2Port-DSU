#!/usr/bin/env python3
"""Read-only GC-assumption grep queue for compiled, matching G2ME01 sources.

python3 tools/audit/upstream_gconly.py [--cat 1..7] [--json] [--no-headers]
Uses configure.py AST, not execution. By default follows quoted project includes
under include/ transitively, retaining the owning matching translation units.
Counts are distinct (category,path,line) grep candidates, NOT confirmed bugs.
Definitely inactive TARGET_PC arms and explicit positive TARGET_PC handling arms
are skipped; other preprocessor conditions remain unresolved. This is not a C++
parser: ctor hits require manual declaration/init/read-before-write tracing;
integer-cast hits require manual float/type/range analysis. SDK/system headers,
macro-generated bodies and indirect include paths are outside coverage.
"""
import argparse
import ast
import collections
import json
from pathlib import Path
import re

from upstream_units import definitions, masked, pc_lines, rank


ROOT = Path(__file__).resolve().parents[2]
PATTERNS = {
    1: r'(?<!\w)(?:(?:const|volatile)\s+)*char\s+(?:\*\s*)?\w+|\(\s*char\s*\)|static_cast\s*<\s*char\s*>',
    2: r'\b(?:uninitiali[sz]ed|not initialized|not initialised)\b',
    3: r'\b(?:memcpy|memmove|offsetof)\s*\(|\b(?:ReadBytes|Read)\s*\([^;]*sizeof|\bsizeof\s*\(|\b(?:Alloc|new)\b[^;]*(?:\*\s*4\b|\b4\s*\*)',
    4: r'\bunion\b|reinterpret_cast\s*<[^>]*(?:float|double|u32|uint|uchar|u8)[^>]*\*|\*\s*\(\s*(?:float|double|u32|uint|u8|uchar)\s*\*',
    5: r'(?:reinterpret_cast|static_cast)\s*<\s*(?:u32|s32|int|uint|unsigned(?:\s+int)?)\s*>|\(\s*(?:u32|s32|uint|int)\s*\)\s*(?:this|&|\w+)|\b(?:u32|s32|uint|int)\s+\w*[pP]tr\w*\b',
    6: r'\b(?:OSGetTime|OSGetTick|VIWaitForRetrace|GXDrawDone|GXReadDrawSync|GXGetDrawDone|OS_TIMER_CLOCK|OS_BUS_CLOCK)\b|0x[Cc][Cc]00|\b(?:486000000|162000000|40500000)\b|\bvolatile\s+[^;]*\*',
    7: r'(?:static_cast\s*<\s*(?:int|s32|u32|uint|unsigned(?:\s+int)?|short|ushort|char|uchar)\s*>|\(\s*(?:int|s32|u32|uint|short|ushort|char|uchar)\s*\))|(?:<<|>>)\s*\w+|\b(?:mSeed|seed)\b\s*=',
}


def units():
    text = (ROOT / 'configure.py').read_text()
    compiled = set(re.findall(r'\bsrc/[^\s;)]+\.(?:cpp|c)', (ROOT / 'files.cmake').read_text()))
    result = set()
    for n in ast.walk(ast.parse(text)):
        if not isinstance(n, ast.Call) or not isinstance(n.func, ast.Name) or n.func.id != 'Object' or len(n.args) < 2:
            continue
        status = ast.get_source_segment(text, n.args[0])
        matching = status == 'Matching'
        arg = n.args[0]
        if isinstance(arg, ast.Call) and isinstance(arg.func, ast.Name) and arg.func.id == 'MatchingFor':
            matching = any(isinstance(a, ast.Constant) and a.value == 'G2ME01' for a in arg.args)
        if not matching or not isinstance(n.args[1], ast.Constant):
            continue
        source = next((k.value.value for k in n.keywords if k.arg == 'source' and isinstance(k.value, ast.Constant)), n.args[1].value)
        path = 'src/' + source
        if path in compiled and not Path(path).name.startswith('Port') and (ROOT / path).is_file():
            result.add(path)
    return sorted(result)


def allowed(text):
    states = pc_lines(text)
    handling, stack = set(), []
    for n, line in enumerate(text.splitlines(), 1):
        m = re.match(r'\s*#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)', line)
        if m:
            kw, expr = m.groups()
            if kw in ('ifdef', 'ifndef', 'if'):
                direct = 'TARGET_PC' in expr and '!' not in expr and kw != 'ifndef'
                stack.append(direct)
            elif kw == 'endif' and stack:
                stack.pop()
            elif kw in ('else', 'elif') and stack:
                stack[-1] = False
        if any(stack):
            handling.add(n)
    return {n for n, value in states.items() if value is not False and n not in handling}


def scan(headers=True):
    sources = units()
    owners = collections.defaultdict(set)
    texts = {}
    includes = {}
    for source in sources:
        todo, seen = [source], set()
        while todo:
            path = todo.pop()
            if path in seen:
                continue
            seen.add(path)
            owners[path].add(source)
            if path not in texts:
                texts[path] = (ROOT / path).read_text(errors='replace')
            text = texts[path]
            if headers and path not in includes:
                includes[path] = []
                active = allowed(text)
                for n, line in enumerate(text.splitlines(), 1):
                    m = re.match(r'\s*#\s*include\s*"([^"]+)"', line)
                    if not m or n not in active:
                        continue
                    for candidate in (Path(path).parent / m[1], Path('include') / m[1]):
                        if (ROOT / candidate).is_file() and candidate.parts[0] in ('src', 'include'):
                            includes[path].append(candidate.as_posix())
                            break
            if headers:
                todo.extend(includes[path])
    hits = []
    for path, text in sorted(texts.items()):
        active = allowed(text)
        code = masked(text).splitlines()
        lines = text.splitlines()
        found = set()
        for cat, pattern in PATTERNS.items():
            for n, line in enumerate(lines, 1):
                if n not in active:
                    continue
                # Comments matter only for explicit initialization warnings.
                test = line if cat == 2 else code[n - 1]
                if cat == 1 and re.search(r'\b(?:unsigned|signed)\s+char\b', test):
                    test = re.sub(r'\b(?:unsigned|signed)\s+char\b', '', test)
                if re.search(pattern, test):
                    found.add((cat, n, 'grep'))
        if path in sources:
            for f in definitions(text):
                parts = f['name'].split('::')
                if len(parts) > 1 and parts[-1] == parts[-2] and f['start'] in active:
                    found.add((2, f['start'], 'ctor: manually compare declaration/init/first use'))
        for cat, n, reason in sorted(found):
            hits.append({'category': cat, 'path': path, 'line': n, 'text': lines[n - 1].strip(),
                         'reason': reason, 'owners': sorted(owners[path]),
                         'rank': min(rank(p) for p in owners[path])})
    hits.sort(key=lambda x: (x['category'], x['rank'], x['path'], x['line']))
    return {'matching_compiled_units': len(sources), 'files_scanned': len(texts),
            'header_files': len(set(texts) - set(sources)),
            'counts': {str(c): sum(h['category'] == c for h in hits) for c in range(1, 8)},
            'hits': hits}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cat', type=int, choices=range(1, 8))
    ap.add_argument('--json', action='store_true')
    ap.add_argument('--no-headers', action='store_true')
    args = ap.parse_args()
    result = scan(not args.no_headers)
    if args.cat:
        result['hits'] = [h for h in result['hits'] if h['category'] == args.cat]
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print(json.dumps({k: v for k, v in result.items() if k != 'hits'}, sort_keys=True))
        for h in result['hits']:
            print(f"{h['category']}\t{h['path']}:{h['line']}\t{h['text']}")


if __name__ == '__main__':
    main()
