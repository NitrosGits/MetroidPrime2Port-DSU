#!/usr/bin/env python3
"""Link a module's partially-linked ELF (build/G2ME01/<Module>/<Module>.plf) against the DOL's
main.elf into one ET_EXEC, so a decompiler that only takes executables (REA's Ghidra lane rejects
relocatable objects) can read the REL with every DOL call and constant resolved.

The DOL's allocated sections are copied at their own addresses; the module's sections are placed
from --base (default 0x81000000) in file order and their relocations applied (ADDR32, ADDR16_HA/LO/HI,
REL24, REL14). Undefined module symbols resolve by name against main.elf, then as fn_/lbl_<addr>.
The output's e_flags is 0 (Ghidra refuses Dolphin's EF_PPC_EMB 0x80000000).

usage: rel_plf2exec.py MODULE.plf MAIN.elf OUT.elf [--base 0x81000000]
"""
import argparse
import re
import struct

SHT_PROGBITS, SHT_SYMTAB, SHT_STRTAB, SHT_RELA, SHT_NOBITS = 1, 2, 3, 4, 8
SHF_WRITE, SHF_ALLOC, SHF_EXEC = 1, 2, 4


class Elf:
    def __init__(self, path):
        self.d = d = open(path, "rb").read()
        (self.type, _, _, self.entry, _, shoff, _, _, _, _, shentsize, shnum,
         shstrndx) = struct.unpack_from(">HHIIIIIHHHHHH", d, 0x10)
        self.sh = []
        for i in range(shnum):
            self.sh.append(struct.unpack_from(">IIIIIIIIII", d, shoff + i * shentsize))
        so = self.sh[shstrndx][4]
        self.names = [self.cstr(so + s[0]) for s in self.sh]

    def cstr(self, o):
        return self.d[o:self.d.index(b"\0", o)].decode("latin1")

    def data(self, i):
        s = self.sh[i]
        return self.d[s[4]:s[4] + s[5]] if s[1] != SHT_NOBITS else bytes(s[5])

    def symbols(self):
        for i, s in enumerate(self.sh):
            if s[1] != SHT_SYMTAB:
                continue
            stro = self.sh[s[6]][4]
            for o in range(s[4], s[4] + s[5], 16):
                name, value, size, info, other, shndx = struct.unpack_from(">IIIBBH", self.d, o)
                yield self.cstr(stro + name), value, size, info, shndx


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("plf")
    ap.add_argument("dol")
    ap.add_argument("out")
    ap.add_argument("--base", type=lambda s: int(s, 0), default=0x81000000)
    a = ap.parse_args()
    rel, dol = Elf(a.plf), Elf(a.dol)

    dolsyms = {}
    out_syms = []  # (name, addr, size, info)
    for name, value, size, info, shndx in dol.symbols():
        if name and shndx != 0 and info & 0xF in (1, 2):
            dolsyms.setdefault(name, value)
            out_syms.append((name, value, size, info & 0xF))

    # Place the module's allocated sections.
    place, addr = {}, a.base
    for i, s in enumerate(rel.sh):
        if s[2] & SHF_ALLOC and s[5]:
            align = max(s[8], 4)
            addr = (addr + align - 1) & ~(align - 1)
            place[i] = addr
            addr += s[5]
    image = {i: bytearray(rel.data(i)) for i in place}

    symaddr = []
    unresolved = set()
    for name, value, size, info, shndx in rel.symbols():
        if shndx == 0:
            v = dolsyms.get(name)
            if v is None:
                m = re.match(r"(?:fn|lbl|func|jumptable)_([0-9A-Fa-f]{8})$", name)
                v = int(m.group(1), 16) if m else None
            if v is None and name:
                unresolved.add(name)
            symaddr.append(v or 0)
        elif shndx == 0xFFF1:
            symaddr.append(value)
        elif shndx in place:
            symaddr.append(place[shndx] + value)
            if name and info & 0xF in (1, 2):
                out_syms.append((name, place[shndx] + value, size, info & 0xF))
        else:
            symaddr.append(0)

    for i, s in enumerate(rel.sh):
        if s[1] != SHT_RELA or s[7] not in place:
            continue
        tgt, base = image[s[7]], place[s[7]]
        for o in range(s[4], s[4] + s[5], 12):
            off, info, addend = struct.unpack_from(">IIi", rel.d, o)
            S = symaddr[info >> 8] + addend
            t = info & 0xFF
            P = base + off
            if t == 1:  # ADDR32
                struct.pack_into(">I", tgt, off, S & 0xFFFFFFFF)
            elif t == 4:  # ADDR16_LO
                struct.pack_into(">H", tgt, off, S & 0xFFFF)
            elif t == 5:  # ADDR16_HI
                struct.pack_into(">H", tgt, off, (S >> 16) & 0xFFFF)
            elif t == 6:  # ADDR16_HA
                struct.pack_into(">H", tgt, off, ((S + 0x8000) >> 16) & 0xFFFF)
            elif t == 10:  # REL24
                ins = struct.unpack_from(">I", tgt, off)[0]
                struct.pack_into(">I", tgt, off, (ins & 0xFC000003) | ((S - P) & 0x03FFFFFC))
            elif t == 11:  # REL14
                ins = struct.unpack_from(">I", tgt, off)[0]
                struct.pack_into(">I", tgt, off, (ins & 0xFFFF0003) | ((S - P) & 0xFFFC))
            elif t == 0:
                pass
            else:
                raise SystemExit(f"unhandled relocation type {t} at {P:#x}")

    # Ghidra's PowerPC language has no Gekko paired-single instructions, and the decompiler stops at
    # the first one ("bad instruction data"). The compiler only uses psq_l/psq_st to move a vector's
    # first two floats as one 8-byte unit, so lfd/stfd with the same 12-bit displacement keeps the
    # data flow readable.
    for i in place:
        if rel.sh[i][2] & SHF_EXEC:
            t = image[i]
            for off in range(0, len(t) - 3, 4):
                ins = struct.unpack_from(">I", t, off)[0]
                op = ins >> 26
                if op in (56, 57, 60, 61):  # psq_l, psq_lu, psq_st, psq_stu
                    d = ins & 0xFFF
                    if d & 0x800:
                        d -= 0x1000
                    new = {56: 50, 57: 51, 60: 54, 61: 55}[op]
                    struct.pack_into(">I", t, off, (new << 26) | (ins & 0x03FF0000) | (d & 0xFFFF))

    # Sections to emit: the DOL's allocated ones (its code as uninitialised memory, so its symbols
    # name every call without Ghidra analysing the whole DOL), then the module's.
    secs = []  # (name, type, flags, addr, bytes, size)
    for i, s in enumerate(dol.sh):
        if s[2] & SHF_ALLOC and s[5]:
            typ = SHT_NOBITS if s[2] & SHF_EXEC else s[1]
            secs.append((dol.names[i], typ, s[2], s[3], dol.data(i) if typ != SHT_NOBITS else b"", s[5]))
    for i in place:
        s = rel.sh[i]
        secs.append(("rel" + rel.names[i], s[1], s[2], place[i],
                     bytes(image[i]) if s[1] != SHT_NOBITS else b"", s[5]))

    shstr = bytearray(b"\0")
    strtab = bytearray(b"\0")

    def addstr(tab, s):
        o = len(tab)
        tab += s.encode("latin1") + b"\0"
        return o

    nsec = len(secs)
    body = bytearray()
    ehsize, phentsize = 0x34, 0x20
    loads = [s for s in secs if s[1] != SHT_NOBITS]
    data_off = ehsize + phentsize * len(secs)
    shdrs = [(0,) * 10]
    phdrs = []
    for name, typ, flags, vaddr, blob, size in secs:
        off = data_off + len(body)
        if typ != SHT_NOBITS:
            body += blob
            while len(body) % 4:
                body += b"\0"
        shdrs.append((addstr(shstr, name), typ, flags, vaddr, off, size, 0, 0, 4, 0))
        pflags = 4 | (1 if flags & SHF_EXEC else 0) | (2 if flags & SHF_WRITE else 0)
        phdrs.append((1, off if typ != SHT_NOBITS else 0, vaddr, vaddr,
                      size if typ != SHT_NOBITS else 0, size, pflags, 4))

    symtab = bytearray(bytes(16))
    for name, v, size, typ in out_syms:
        sec = 0
        for k, (_, _, _, va, _, sz) in enumerate(secs):
            if va <= v < va + max(sz, 1):
                sec = k + 1
                break
        symtab += struct.pack(">IIIBBH", addstr(strtab, name), v, size, 0x10 | typ, 0,
                              sec if sec else 0xFFF1)
    for blob_name, blob, typ in ((".symtab", symtab, SHT_SYMTAB), (".strtab", strtab, SHT_STRTAB)):
        off = data_off + len(body)
        body += blob
        while len(body) % 4:
            body += b"\0"
        link = len(shdrs) + 1 if typ == SHT_SYMTAB else 0
        shdrs.append((addstr(shstr, blob_name), typ, 0, 0, off, len(blob), link, 1, 4,
                      16 if typ == SHT_SYMTAB else 0))
    shstr_name = addstr(shstr, ".shstrtab")
    off = data_off + len(body)
    body += shstr
    while len(body) % 4:
        body += b"\0"
    shdrs.append((shstr_name, SHT_STRTAB, 0, 0, off, len(shstr), 0, 0, 1, 0))
    shoff = data_off + len(body)

    out = bytearray()
    out += b"\x7fELF\x01\x02\x01" + bytes(9)
    out += struct.pack(">HHIIIIIHHHHHH", 2, 20, 1, dol.entry, ehsize, shoff, 0, ehsize,
                       phentsize, len(phdrs), 0x28, len(shdrs), len(shdrs) - 1)
    for p in phdrs:
        out += struct.pack(">IIIIIIII", *p)
    out += body
    for s in shdrs:
        out += struct.pack(">IIIIIIIIII", *s)
    open(a.out, "wb").write(out)
    print(f"{a.out}: {len(secs)} sections, {len(out_syms)} symbols, module at {a.base:#x}"
          f"{', unresolved: ' + ' '.join(sorted(unresolved)) if unresolved else ''}")
    for i in place:
        print(f"  {rel.names[i]:8} {place[i]:#x} size {rel.sh[i][5]:#x}")


if __name__ == "__main__":
    main()
