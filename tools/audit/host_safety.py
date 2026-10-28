#!/usr/bin/env python3
"""Sweep 3 host-safety heuristics (grep level). Usage:
  tools/audit/host_safety.py [--cat N] [--list] [--root DIR]
Categories: 1 ctor-uninit (ctors with init list missing members), 2 LP64 casts,
3 raw reads without swap, 4 Alloc/new constant size, 5 divisors, 6 indexed arrays.
Default prints counts; --list prints file:line candidates (optionally one --cat)."""
import re, sys, os, argparse
ap = argparse.ArgumentParser()
ap.add_argument('--cat', type=int)
ap.add_argument('--list', action='store_true')
ap.add_argument('--root', default='.')
a = ap.parse_args()
CAT = {
 2: [r'\(\s*(u32|s32|int|unsigned|unsigned int)\s*\)\s*(&|this\b|\w+Ptr\b|\w*->|p\w+\b)', r'\b(u32|s32)\s+\w*[pP]tr\w*\s*[;=]', r'\(\s*(u32|s32)\s*\)\s*\w*(data|buf|mem|ptr)\w*'],
 3: [r'\bDVDRead\w*\(', r'\bfread\s*\(', r'\*\s*\(\s*(u32|s32|u16|s16|float|f32)\s*\*\s*\)\s*\w*(buf|data|mem|ptr|raw)\w*'],
 4: [r'\bAlloc\s*\(\s*(0x[0-9a-fA-F]+|\d+)\s*\)', r'\boperator new\s*\(\s*(0x[0-9a-fA-F]+|\d+)', r'\b(OSAlloc|CMemory::Alloc|Alloc)\s*\(\s*(0x[0-9a-fA-F]+|\d+)\s*[,)]'],
 5: [r'[^/]/\s*\(?\s*\w*(Size|Count|Length|Num|Width|Height|size|count|length|num|width|height|Stride|stride)\w*\)?\s*[;,)]', r'\s%\s*\w*(Size|Count|Num|size|count|num)\w*'],
 6: [r'\w+\[\s*\w*(Idx|Index|index|idx|id|Id|ID)\w*\s*\]', r'\w+\[\s*(in|r|stream)\w*\.Read\w*\(\)\s*\]'],
}
def files():
    for d in ('src','platform','include'):
        for r,_,fs in os.walk(os.path.join(a.root,d)):
            for f in fs:
                if f.endswith(('.cpp','.hpp','.h','.c')): yield os.path.join(r,f)
counts = {c:0 for c in range(1,7)}
out = []
for p in files():
    try: txt = open(p,errors='replace').read()
    except: continue
    lines = txt.split('\n')
    pc = ('platform' in p) or os.path.basename(p).startswith('Port') or 'Port' in os.path.basename(p)
    for c,pats in CAT.items():
        if a.cat and a.cat!=c: continue
        for i,l in enumerate(lines,1):
            if l.lstrip().startswith('//'): continue
            if any(re.search(x,l) for x in pats):
                counts[c]+=1; out.append((c,p,i,l.strip()))
    if (not a.cat or a.cat==1) and pc:
        # ctor with init list but body/class members: flag Port ctors with no ': ' init list on class holding raw members
        for i,l in enumerate(lines,1):
            m = re.match(r'^\s*(\w+)::\1\s*\([^)]*\)\s*(\{|$)', l)
            if m:
                counts[1]+=1; out.append((1,p,i,'ctor without init list: '+l.strip()))
for c in range(1,7):
    if not a.cat or a.cat==c: print(f'cat {c}: {counts[c]}')
if a.list:
    for c,p,i,l in out: print(f'{c}\t{p}:{i}\t{l[:140]}')
