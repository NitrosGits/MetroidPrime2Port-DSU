#!/usr/bin/env python3
"""Replay a REL's FSM-table static initialiser and print each table entry's name and function.

usage: tools/fsm_ctor_tables.py <asm dir> <fn> <table label>[:<count>] ...
  e.g. tools/fsm_ctor_tables.py build/G2ME01/Splitter/asm fn_75_7C80 \
         lbl_75_data_CC:17 lbl_75_data_284:14 lbl_75_data_3B8:7

The initialisers (Splitter fn_75_7C80 / fn_75_FED0) are straight-line lis/addi/lwz/lwzu/stw code
that copies the three-word records {0, 0xFFFFFFFF, fn} of .data into the 16-byte entries
{name, 0, 0xFFFFFFFF, fn} of the tables. This replays that code symbolically (a register holds a
.data address or a loaded word) over the .data dump and prints, per entry, the name string and
the words the body stored at +4, +8 and +0xC. Any other instruction stops it with an error.
"""
import re
import sys
import pathlib


def load_data(asm_dir):
    words, labels, strings = {}, {}, {}
    for f in sorted(pathlib.Path(asm_dir).glob('auto_*_data.s')) + \
            sorted(pathlib.Path(asm_dir).glob('auto_*_rodata.s')):
        rodata = 'rodata' in f.name
        off = None
        cur = None
        for line in f.read_text().splitlines():
            m = re.match(r'# \.(ro)?data:0x([0-9A-Fa-f]+) \|', line)
            if m:
                off = int(m.group(2), 16)
                continue
            m = re.match(r'\.obj (\w+),', line)
            if m:
                cur = m.group(1)
                labels[cur] = off
                continue
            if cur is None:
                continue
            m = re.match(r'\s+\.string "(.*)"', line)
            if m and rodata:
                strings[cur] = m.group(1)
                continue
            m = re.match(r'\s+\.4byte (\S+)', line)
            if m and not rodata:
                words[off] = m.group(1)
                off += 4
    return words, labels, strings


def run(asm_dir, fn):
    words, labels, strings = load_data(asm_dir)
    text = []
    for f in sorted(pathlib.Path(asm_dir).glob('auto_*_text.s')):
        body, on = [], False
        for line in f.read_text().splitlines():
            if line.startswith('.fn %s,' % fn):
                on = True
            elif on and line.startswith('.endfn'):
                break
            elif on:
                m = re.match(r'/\* ([0-9A-F]{8}) .*\*/\t(\S+)\s*(.*)', line)
                if m:
                    body.append((m.group(1), m.group(2), [a.strip() for a in m.group(3).split(',')]))
        if body:
            text = body
            break
    if not text:
        sys.exit('no body for %s' % fn)
    reg = {'r1': ('stack', 0)}
    mem = {}

    def addr(base, d):
        kind, v = base
        if kind == 'stack':
            return ('stack', v + d)
        if kind != 'addr':
            raise ValueError('not an address: %r' % (base,))
        return ('data', v + d)

    def load(a):
        if a in mem:
            return mem[a]
        if a[0] == 'data':
            return ('word', words.get(a[1], '?'))
        raise ValueError('stack read before write %r' % (a,))

    def parse_mem(op):
        m = re.match(r'(-?0x[0-9a-fA-F]+|-?\d+|\w+@l)\((r\d+)\)', op)
        d, r = m.group(1), m.group(2)
        if d.endswith('@l'):
            return reg[r], 0
        return reg[r], int(d, 0)

    for pc, ins, ops in text:
        if ins in ('stmw', 'lmw', 'blr', 'stwu') or (ins in ('stw', 'lwz') and ops[1].endswith('(r1)') and ops[0] in ('r0',) and False):
            if ins == 'stwu':
                continue
            continue
        if ins == 'lis':
            lab = ops[1].split('@')[0]
            reg[ops[0]] = ('addr', labels[lab])
        elif ins == 'addi':
            if ops[2].endswith('@l'):
                lab = ops[2].split('@')[0]
                reg[ops[0]] = ('addr', labels[lab])
            else:
                k, v = reg[ops[1]]
                reg[ops[0]] = (k, v + int(ops[2], 0))
        elif ins in ('lwz', 'lwzu'):
            base, d = parse_mem(ops[1])
            a = addr(base, d)
            reg[ops[0]] = load(a)
            if ins == 'lwzu':
                rr = re.search(r'\((r\d+)\)', ops[1]).group(1)
                reg[rr] = ('addr', a[1])
        elif ins == 'stw':
            base, d = parse_mem(ops[1])
            mem[addr(base, d)] = reg[ops[0]]
        else:
            raise SystemExit('%s: unhandled %s %s' % (pc, ins, ops))
    return words, labels, strings, mem


def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    asm_dir, fn = sys.argv[1], sys.argv[2]
    words, labels, strings, mem = run(asm_dir, fn)
    bad = 0
    for spec in sys.argv[3:]:
        lab, _, cnt = spec.partition(':')
        base = labels[lab]
        n = int(cnt) if cnt else 0
        print('== %s (.data 0x%X)' % (lab, base))
        for i in range(n):
            e = base + 16 * i
            name = words.get(e, '?')
            got = [mem.get(('data', e + k)) for k in (4, 8, 12)]
            vals = [g[1] if g else 'unset' for g in got]
            ok = vals[0] == '0x00000000' and vals[1] == '0xFFFFFFFF' and vals[2].startswith('fn_')
            bad += not ok
            print('%2d %-24s %s %s %s%s' % (i, strings.get(name, name), vals[0], vals[1], vals[2],
                                           '' if ok else '  <-- ?'))
    print('== %d stores, %d bad entries' % (len(mem), bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
