#!/usr/bin/env python3
"""Sweep 2: per-render-frame state advanced without dt.

Usage: tools/audit/frame_rate.py [repo-root] > candidates.tsv

Finds functions that run once per rendered frame on the uncapped path (Draw*, PreRender, Render*,
AddToRenderer, CompleteRender, RenderGlow, ... and the kAM_FrameBegin handlers) and prints the
statements inside them that mutate member/static state: ++/--, +=/-=, assignments to m* / s* /
g* names, random draws (CRandom16/rand), and uses of CGraphics::GetFrameCounter. Output:
file:line<TAB>function<TAB>kind<TAB>statement. Candidates only; each needs a human read.
Skips extern/, build dirs, and *.inc. Read-only.
"""
import os, re, sys

root = sys.argv[1] if len(sys.argv) > 1 else "."
FN = re.compile(r'^[A-Za-z_][\w:<>\*&, ]*?\b(\w+::)?(~?\w+)\s*\([^;{]*\)\s*(const)?\s*\{\s*$')
FN_HEAD = re.compile(r'^[A-Za-z_][\w:<>\*&, ]*?\b(\w+::)(~?\w+)\s*\([^;{)]*$')
RENDER = re.compile(r'(Draw|PreRender|Render|AddToRenderer|CompleteRender|DrawFrame|Paint|BeginScene|EndScene|FrameBegin|OnMessage)')
PAT = [
    ('incdec', re.compile(r'(\+\+|--)\s*(m|s|g)[A-Z_]\w*|\b(m|s|g)[A-Z_]\w*(\.\w+)*\s*(\+\+|--)')),
    ('accum', re.compile(r'\b(m|s|g)[A-Z_]\w*\s*[-+*]=')),
    ('assign-dec', re.compile(r'\b(m|s|g)[A-Z_]\w*\s*=\s*\w*\s*[-+]\s*(1\b|\w*[Dd]t\b|1\.f?/60)')),
    ('random', re.compile(r'CRandom16|\brand\(|Float\(\)\s*\*|mRandom|GetRandom')),
    ('framectr', re.compile(r'GetFrameCounter|mFrameCounter|FramesDrawn|sCurrentFrameCount')),
    ('const60', re.compile(r'1\.f?\s*/\s*60|0\.016')),
]
for d, _, fs in os.walk(root):
    if any(x in d for x in ('/extern', '/build', '/.git', '/tools')):
        continue
    for f in fs:
        if not f.endswith(('.cpp', '.h', '.hpp')):
            continue
        p = os.path.join(d, f)
        try:
            L = open(p, errors='replace').read().split('\n')
        except OSError:
            continue
        i = 0
        while i < len(L):
            m = FN.match(L[i])
            if not m and FN_HEAD.match(L[i]):
                # multi-line signature: join up to the line that opens the body
                k = i
                while k < len(L) and k < i + 6 and '{' not in L[k] and ';' not in L[k]:
                    k += 1
                if k < len(L) and L[k].rstrip().endswith('{') and ';' not in ' '.join(L[i:k + 1]):
                    m = FN.match(' '.join(x.strip() for x in L[i:k + 1]))
                    if m:
                        L[i] = ' '.join(x.strip() for x in L[i:k + 1])
                        for q in range(i + 1, k + 1):
                            L[q] = ''
            if m and RENDER.search(m.group(2)) and m.group(2) not in ('if', 'for', 'while', 'switch'):
                name = (m.group(1) or '') + m.group(2)
                depth = L[i].count('{') - L[i].count('}')
                j = i + 1
                while j < len(L) and depth > 0:
                    s = L[j]
                    if not s.strip().startswith('//'):
                        for kind, rx in PAT:
                            if rx.search(s):
                                print(f"{os.path.relpath(p, root)}:{j+1}\t{name}\t{kind}\t{s.strip()[:110]}")
                                break
                    depth += s.count('{') - s.count('}')
                    j += 1
                i = j
            else:
                i += 1
