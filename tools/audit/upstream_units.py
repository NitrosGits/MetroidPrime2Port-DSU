#!/usr/bin/env python3
"""Inventory untrusted upstream units/functions without executing configure.py.

Usage: python3 tools/audit/upstream_units.py [--format json|markdown] [--root DIR]
Default version is G2ME01, without configure.py's --non-matching flag. Category a
includes missing-source units: a retail object is linked instead of their C++.
Category b is a lexical review queue, NOT proof that a trivial body is incorrect.
The balanced-delimiter scanner handles ordinary definitions/ctor initializer lists,
not arbitrary C++ macros, templates or preprocessing. Unknown linking expressions
are conservatively classified not-linked. Both preprocessor arms are inventoried;
pc_active distinguishes inactive TARGET_PC arms (other conditions stay unknown).
Port companions are identified by shared qualified definition names; exact
Port<Unit>/unitPortStub filenames are separately labelled unverified candidates.
This is evidence of a companion/alternate body, not proof it wins the
link: overloads, module namespaces and conditional compilation require review.
Ranking is a static gameplay-frequency heuristic, NOT measured execution coverage.
"""
import argparse
import ast
import collections
import json
from pathlib import Path
import re


def masked(text):
    """Preserve byte offsets/newlines while hiding comments and string contents."""
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                  lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]),
                  text, flags=re.S)


def close(code, start, left='(', right=')'):
    depth = 0
    for i in range(start, len(code)):
        if code[i] == left:
            depth += 1
        elif code[i] == right:
            depth -= 1
            if depth == 0:
                return i
    return None


def definitions(text):
    code = masked(text)
    out = []
    occupied = -1
    pattern = r'(?<![\w:])((?:[A-Za-z_]\w*::)*~?[A-Za-z_]\w*)\s*\('
    for m in re.finditer(pattern, code):
        if m.start() < occupied or m[1] in {'if', 'for', 'while', 'switch', 'catch', 'sizeof'}:
            continue
        endargs = close(code, m.end() - 1)
        if endargs is None:
            continue
        j = endargs + 1
        qualifiers = re.match(r'\s*(?:(?:const|volatile|override|final|noexcept)\s*)*', code[j:])
        j += qualifiers.end()
        # Only constructors may have an initializer list. In particular, a call
        # inside if (Type::Function()) must not absorb the following block.
        if j < len(code) and code[j] == ':':
            while j < len(code) and code[j] not in '{;=}':
                if code[j] == '(':
                    end = close(code, j)
                    if end is None:
                        break
                    j = end
                j += 1
        if j >= len(code) or code[j] != '{':
            continue
        # A function declarator must start a declaration, not be a call in an expression.
        prev = max(code.rfind(c, 0, m.start()) for c in ';{}\n')
        prefix = code[prev + 1:m.start()].strip()
        if '::' not in m[1] and not re.fullmatch(r'[\w\s:*&<>]+', prefix):
            continue
        end = close(code, j, '{', '}')
        if end is None:
            continue
        out.append({'name': m[1], 'start': text.count('\n', 0, m.start()) + 1,
                    'end': text.count('\n', 0, end) + 1, 'body': code[j + 1:end]})
        occupied = end + 1
    return out


def pc_lines(text):
    """False only for definitely inactive TARGET_PC branches; None for unknown."""
    stack, states = [], {}
    for n, line in enumerate(text.splitlines(), 1):
        m = re.match(r'\s*#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)', line)
        if m:
            kw, expr = m.groups()
            if kw in ('if', 'ifdef', 'ifndef'):
                val = None
                if re.fullmatch(r'\s*TARGET_PC\s*', expr) and kw != 'if':
                    val = kw == 'ifdef'
                elif kw == 'if' and re.fullmatch(r'\s*!?\s*defined\s*\(?\s*TARGET_PC\s*\)?\s*', expr):
                    val = '!' not in expr
                stack.append(val)
            elif kw == 'endif' and stack:
                stack.pop()
            elif kw == 'else' and stack and stack[-1] is not None:
                stack[-1] = not stack[-1]
            elif kw == 'elif' and stack:
                stack[-1] = None
        states[n] = False if False in stack else (None if None in stack else True)
    return states


def rank(path):
    core = {'CPlayer', 'CStateManager', 'CActor', 'CPhysicsActor', 'CWorld',
            'CGameArea', 'CGameCollision', 'CAnimData', 'CMorphBall', 'CRezbitEffect'}
    if Path(path).stem in core:
        return 0
    if any(s in path for s in ('/Enemies/', '/Player/', '/Collision/', '/Weapons/',
                                '/ScriptObjects/', '/Cameras/', '/Graphics/', '/MetaRender/')):
        return 1
    if path.startswith('src/MetroidPrime/'):
        return 2
    if path.startswith('src/Kyoto/'):
        return 3
    return 4


def inventory(root, version):
    text = (root / 'configure.py').read_text()
    tree = ast.parse(text)
    compiled = set(re.findall(r'\bsrc/[^\s;)]+\.(?:cpp|c|s)', (root / 'files.cmake').read_text()))
    port_defs = collections.defaultdict(set)
    port_files = collections.defaultdict(set)
    for p in sorted((root / 'src').rglob('*.cpp')):
        if p.name.startswith('Port') or p.name.endswith('PortStub.cpp'):
            port_files[p.stem].add(p.relative_to(root).as_posix())
            for f in definitions(p.read_text(errors='replace')):
                if '::' in f['name']:
                    port_defs[f['name']].add(p.relative_to(root).as_posix())
    units, items = [], []
    # AST prevents Object calls with nested commas/keyword source overrides being lost.
    calls = sorted((n for n in ast.walk(tree) if isinstance(n, ast.Call)
                    and isinstance(n.func, ast.Name) and n.func.id == 'Object'),
                   key=lambda n: n.lineno)
    for node in calls:
        if len(node.args) < 2 or not isinstance(node.args[1], ast.Constant):
            continue
        status = ast.get_source_segment(text, node.args[0])
        linked = status == 'Matching'
        if isinstance(node.args[0], ast.Call) and isinstance(node.args[0].func, ast.Name) and node.args[0].func.id == 'MatchingFor':
            linked = version in [a.value for a in node.args[0].args if isinstance(a, ast.Constant)]
        source = next((k.value.value for k in node.keywords if k.arg == 'source'
                       and isinstance(k.value, ast.Constant)), node.args[1].value)
        path = 'src/' + source
        p = root / path
        source_text = p.read_text(errors='replace') if p.is_file() else ''
        funcs = definitions(source_text)
        states = pc_lines(source_text)
        def companions(fs):
            paths = sorted(set().union(*(port_defs[f['name']] for f in fs))) if fs else []
            return [{'path': x, 'compiled': x in compiled,
                     'relationship': 'companion/alternate definitions; review guards and overloads'} for x in paths]
        unit = {'path': path, 'start': 1, 'end': max(1, len(source_text.splitlines())),
                'configure_line': node.lineno, 'link_status': status, 'linked': linked,
                'source_exists': p.is_file(), 'compiled': path in compiled,
                'port_replacements': companions(funcs), 'rank': rank(path)}
        seen = {p['path'] for p in unit['port_replacements']}
        for candidate in sorted(port_files['Port' + p.stem] | port_files[p.stem + 'PortStub']):
            if candidate not in seen:
                unit['port_replacements'].append({'path': candidate, 'compiled': candidate in compiled,
                    'relationship': 'filename candidate only; no shared qualified definition detected'})
        units.append(unit)
        if not linked:
            items.append(dict(unit, category='a', reasons=['not linked in default ' + version]))
            continue
        nonmatching, stack = set(), []
        for n, line in enumerate(source_text.splitlines(), 1):
            m = re.match(r'\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)', line)
            if m:
                kw, expr = m.groups()
                if kw in ('if', 'ifdef', 'ifndef'):
                    stack.append('NONMATCHING' in expr)
                elif kw == 'endif' and stack:
                    stack.pop()
                elif kw == 'elif' and stack:
                    stack[-1] = 'NONMATCHING' in expr
                elif kw == 'else' and stack:
                    stack[-1] = False
            if any(stack):
                nonmatching.add(n)
        lines = source_text.splitlines()
        claimed = set()
        for f in funcs:
            reasons = []
            # Include the immediately preceding comment, but not a previous function.
            lo = f['start'] - 1
            while lo > 0 and (not lines[lo - 1].strip() or lines[lo - 1].lstrip().startswith('//')):
                lo -= 1
            snippet = '\n'.join(lines[lo:f['end']])
            if any(n in nonmatching for n in range(f['start'], f['end'] + 1)):
                reasons.append('NONMATCHING conditional')
            if re.search(r'//[^\n]*\bTODO\b', snippet):
                reasons.append('// TODO')
            if re.search(r'\b[Gg]uessed\b', snippet):
                reasons.append('guessed comment')
            body = re.sub(r'^\s*#.*$', '', f['body'], flags=re.M).strip()
            if not body or re.fullmatch(r'return\s*(?:[-+]?\d+(?:\.\d+f?)?|true|false|nullptr|NULL)?\s*;', body):
                reasons.append('stub-shaped body (may be legitimate)')
            if reasons:
                claimed.update(range(lo + 1, f['end'] + 1))
                items.append(dict(unit, category='b', start=f['start'], end=f['end'],
                                  function=f['name'], reasons=reasons,
                                  pc_active=states.get(f['start']), port_replacements=companions([f])))
        # Keep annotations on classes/globals rather than silently losing guessed names.
        for n, line in enumerate(lines, 1):
            reasons = []
            if n not in claimed:
                if re.search(r'//[^\n]*\bTODO\b', line):
                    reasons.append('// TODO')
                if re.search(r'\b[Gg]uessed\b', line) and ('//' in line or '/*' in line):
                    reasons.append('guessed comment')
                if n in nonmatching and not line.lstrip().startswith('#') and line.strip():
                    reasons.append('NONMATCHING conditional')
            if reasons:
                items.append(dict(unit, category='b-annotation', start=n, end=n,
                                  reasons=reasons, pc_active=states.get(n)))
    items.sort(key=lambda x: (not x['compiled'], x['rank'], x['path'], x['start'], x['category']))
    return {'version': version, 'counts': {'configured_units': len(units),
            'not_linked_units': sum(not u['linked'] for u in units),
            'not_linked_with_source': sum(not u['linked'] and u['source_exists'] for u in units),
            'not_linked_port_compiled': sum(not u['linked'] and u['compiled'] for u in units),
            'matching_flagged_functions': sum(x['category'] == 'b' for x in items),
            'matching_flagged_annotations': sum(x['category'] == 'b-annotation' for x in items),
            'reasons': dict(collections.Counter(r for x in items for r in x['reasons']))}, 'items': items}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--version', default='G2ME01')
    parser.add_argument('--format', choices=['json', 'markdown'], default='json')
    args = parser.parse_args()
    result = inventory(args.root, args.version)
    if args.format == 'json':
        print(json.dumps(result, indent=2))
    else:
        print('# Untrusted upstream inventory\n\nStatic heuristic ranking; not execution coverage.\n')
        print('Counts: `' + json.dumps(result['counts'], sort_keys=True) + '`\n')
        print('| Tier | Source:lines | Compiled | PC active | Reason | Port companion/alternate |')
        print('|---|---|---|---|---|---|')
        for x in result['items']:
            ports = ', '.join(p['path'] + ('' if p['compiled'] else ' (not built)') for p in x['port_replacements']) or 'none detected'
            print(f"| {x['category']} | {x['path']}:{x['start']}-{x['end']} | {x['compiled']} | {x.get('pc_active', 'unit')} | {'; '.join(x['reasons'])} | {ports} |")


if __name__ == '__main__':
    main()
