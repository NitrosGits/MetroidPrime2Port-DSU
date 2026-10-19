#!/usr/bin/env python3
"""Per-function similarity of REL functions against named reference objects/ELFs.

Reuses the MP3 project's tokenizer (tools/similarity_match.py: register-canonicalised,
immediate-wildcarded tokens; Dice over 1/2/3-gram shingle sets).

usage: relsim.py OUT.tsv QUERY.elf[,QUERY2.elf] REF1 [REF2 ...]
REF is an ELF path or a directory (every *.o below it). Symbols come from `nm -S`.
"""
import os, subprocess, sys
from collections import defaultdict
sys.path.insert(0, "/run/media/odran/Leo/projects/Restored-projects/Chatgpt/MetroidPrime3Port/tools")
from similarity_match import tokenize, gram_keys  # noqa: E402

NM = "/run/media/odran/Leo/projects/Restored-projects/Chatgpt/MetroidPrime2Port/build/binutils/powerpc-eabi-nm"
OBJCOPY = NM.replace("-nm", "-objcopy")
OBJDUMP = NM.replace("-nm", "-objdump")


def sections(path):
    """name -> (file offset, size, vma) from objdump -h."""
    out = {}
    lines = subprocess.run([OBJDUMP, "-h", path], capture_output=True, text=True).stdout.splitlines()
    for ln in lines:
        p = ln.split()
        if len(p) >= 7 and p[0].isdigit():
            out[p[1]] = (int(p[5], 16), int(p[2], 16), int(p[3], 16))
    return out


def functions(path):
    data = open(path, "rb").read()
    secs = sections(path)
    text = [s for s in secs if s.startswith(".text")]
    out = []
    # nm -S: value size type name; for .o the value is section-relative (one .text assumed)
    res = subprocess.run([NM, "-S", "--defined-only", path], capture_output=True, text=True).stdout
    is_obj = path.endswith(".o") or path.endswith(".plf")
    for ln in res.splitlines():
        p = ln.split()
        if len(p) != 4 or p[2] not in "Tt":
            continue
        val, size, name = int(p[0], 16), int(p[1], 16), p[3]
        if size < 8:
            continue
        for s in text:
            off, ssz, vma = secs[s]
            base = 0 if is_obj else vma
            if base <= val < base + ssz:
                code = data[off + val - base: off + val - base + size]
                out.append((name, os.path.basename(path), val, code))
                break
    return out


def main():
    outp, queries, refs = sys.argv[1], sys.argv[2].split(","), sys.argv[3:]
    q = []
    for path in queries:
        q += functions(path)
    r = []
    for ref in refs:
        if os.path.isdir(ref):
            for root, _, files in os.walk(ref):
                for f in files:
                    if f.endswith(".o"):
                        r += functions(os.path.join(root, f))
        else:
            r += functions(ref)
    tokid = {}
    base = 1 << 20

    def fp(code):
        toks = tokenize(code)
        return len(toks), gram_keys([tokid.setdefault(t, len(tokid)) for t in toks], base)

    rfp = [(n, src, v, *fp(c)) for n, src, v, c in r]
    by_len = defaultdict(list)
    for i, e in enumerate(rfp):
        by_len[e[3]].append(i)
    rows = []
    for name, src, val, code in q:
        n, g = fp(code)
        best = (0.0, None)
        win = max(2, n // 4)
        for L in range(n - win, n + win + 1):
            for i in by_len.get(L, ()):
                rn, rsrc, rv, rl, rg = rfp[i]
                if rsrc == src and rn == name:
                    continue
                s = 2 * len(g & rg) / (len(g) + len(rg))
                if s > best[0]:
                    best = (s, i)
        if best[1] is not None:
            rn, rsrc, rv, rl, _ = rfp[best[1]]
            rows.append((best[0], name, n, rn, rsrc, rl))
        else:
            rows.append((0.0, name, n, "-", "-", 0))
    rows.sort(key=lambda x: (-x[0] * x[2]))
    with open(outp, "w") as f:
        f.write("sim\tfunc\tninstr\tref\tref_src\tref_ninstr\n")
        for s, nm, n, rn, rs, rl in rows:
            f.write(f"{s:.3f}\t{nm}\t{n}\t{rn}\t{rs}\t{rl}\n")
    print(outp, len(q), "queries", len(r), "refs")


if __name__ == "__main__":
    main()
