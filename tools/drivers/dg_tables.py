#!/usr/bin/env python3
"""Check the DigitalGuardian FSM tables (and the other statics) the REL's static initialisers build.

    python3 tools/drivers/dg_tables.py [--asm DIR] [--full DIR]

The module has two .ctors entries: fn_14_D904 (the body's tables lbl_14_data_9C / _204 / _380,
the GX colours bss_1C/bss_30 and the toe bounds bss_24) and fn_14_1AD2C (the head's tables
lbl_14_data_808 / _A14 / _C80, the echo colour bss_34, bss_38 and data_E50). Both are straight
copies (no branch): the three-word member-pointer records {0, -1, fn} in .data are copied into
the 16-byte table entries {name, 0, -1, fn}. This runs each initialiser symbolically (lis/addi/
lwz/lwzu/stw/stwu/lfs/stfs/fmr/mr and the CColor ctor), so every table entry resolves to its
name and the retail function it calls, and then compares that with the port's arrays: the port
entry's member function must carry the marker `// @ fn_14_<off>` of the same retail function.

Exit code 0 when every entry of the six tables matches (name, order, and function), 1 otherwise.
"""
import argparse
import pathlib
import re
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
DEFAULT_ASM = ROOT.parent / "wt-mp2-port/build/G2ME01/DigitalGuardian/asm"
DEFAULT_FULL = pathlib.Path("/tmp/opencode/dg32/full")
SRC = ROOT / "src/MetroidPrime/Enemies"

# initialiser -> [(table label, port file, port array, the class)]
TABLES = {
    "D904": [("lbl_14_data_9C", "PortCDigitalGuardian.cpp", "skTriggers", "CDigitalGuardian"),
             ("lbl_14_data_204", "PortCDigitalGuardian.cpp", "skStates", "CDigitalGuardian"),
             ("lbl_14_data_380", "PortCDigitalGuardian.cpp", "skCodes", "CDigitalGuardian")],
    "1AD2C": [("lbl_14_data_808", "PortCDigitalGuardianHead.cpp", "skTriggers",
               "CDigitalGuardianHead"),
              ("lbl_14_data_A14", "PortCDigitalGuardianHead.cpp", "skStates",
               "CDigitalGuardianHead"),
              ("lbl_14_data_C80", "PortCDigitalGuardianHead.cpp", "skCodes",
               "CDigitalGuardianHead")],
}


def sym_addr(sym):
    m = re.match(r"lbl_14_(data|rodata|bss)_([0-9A-Fa-f]+)$", sym)
    return (m.group(1), int(m.group(2), 16)) if m else None


def read_section(asm_dir, sect):
    """(sect, offset) -> word/value, from the section dump(s)."""
    mem = {}
    for path in sorted(asm_dir.glob(f"auto_*_{sect}.s")):
        off = None
        for line in path.read_text(errors="replace").splitlines():
            m = re.match(r"^\.obj\s+(lbl_14_\w+?)(?:,\s*\w+)?\s*$", line)
            if m:
                a = sym_addr(m.group(1))
                off = a[1] if a else None
                continue
            if off is None:
                continue
            s = line.strip()
            m = re.match(r"\.4byte\s+(\S+)", s)
            if m:
                v = m.group(1)
                mem[(sect, off)] = int(v, 16) if v.startswith("0x") else v
                off += 4
                continue
            m = re.match(r"\.float\s+(\S+)", s)
            if m:
                mem[(sect, off)] = float(m.group(1))
                off += 4
                continue
            m = re.match(r"\.byte\s+(.*)", s)
            if m:
                off += len(m.group(1).split(","))
                continue
            m = re.match(r"\.2byte\s+(.*)", s)
            if m:
                off += 2 * len(m.group(1).split(","))
                continue
            m = re.match(r'\.string\s+"(.*)"', s)
            if m:
                mem[(sect, off)] = ("str", m.group(1))
                off += len(m.group(1)) + 1
    return mem


def read_strings(asm_dir):
    """rodata label -> string (labels whose object is a .string, or a NUL-run of .4byte)."""
    out = {}
    for path in sorted(asm_dir.glob("auto_*_rodata.s")):
        cur, blob = None, b""
        for line in path.read_text(errors="replace").splitlines():
            m = re.match(r"^\.obj\s+(\S+?)(?:,\s*\w+)?\s*$", line)
            if m:
                cur, blob = m.group(1), b""
                continue
            if line.startswith(".endobj"):
                if cur and cur not in out and blob:
                    out[cur] = blob.split(b"\0")[0].decode("latin-1")
                cur = None
                continue
            if cur is None:
                continue
            m = re.match(r'^\s*\.string\s+"(.*)"', line)
            if m and cur not in out:
                out[cur] = m.group(1)
            m = re.match(r"^\s*\.4byte\s+0x([0-9A-Fa-f]{8})", line)
            if m:
                blob += bytes.fromhex(m.group(1))
            m = re.match(r"^\s*\.byte\s+(.*)", line)
            if m:
                blob += bytes(int(x, 16) for x in m.group(1).split(","))
    return out


def simulate(path, mem):
    """Run the initialiser; returns the stores into .data/.bss: (sect, off) -> value."""
    R = {"r1": ("stack", 0)}
    Fr = {}
    stack = {}
    stores = {}

    def load(addr):
        if addr[0] == "stack":
            return stack.get(addr[1], ("?stack", addr[1]))
        if addr in stores:
            return stores[addr]
        return mem.get(addr, ("?", addr))

    def ea(off, base):
        b = R.get(base)
        m = re.match(r"(lbl_14_\w+)@l$", off)
        if m:
            a = sym_addr(m.group(1))
            assert b == ("ha", m.group(1)), (off, base, b)
            return a
        o = int(off, 0)
        assert isinstance(b, tuple) and b[0] in ("stack", "data", "rodata", "bss"), (off, base, b)
        return (b[0], b[1] + o)

    for line in path.read_text().splitlines():
        m = re.match(r"^([0-9A-F]+)\s+(\S+)\s*(.*)$", line)
        if not m:
            continue
        op, args = m.group(2), [a.strip() for a in m.group(3).split(",")] if m.group(3) else []
        def memop(a):
            mm = re.match(r"(.*)\((r\d+)\)$", a)
            return mm.group(1), mm.group(2)
        if op == "lis":
            mm = re.match(r"(lbl_14_\w+)@ha$", args[1])
            R[args[0]] = ("ha", mm.group(1)) if mm else ("imm", int(args[1], 0) << 16)
        elif op == "addi":
            mm = re.match(r"(lbl_14_\w+)@l$", args[2])
            if mm:
                assert R[args[1]] == ("ha", mm.group(1))
                R[args[0]] = sym_addr(mm.group(1))
            else:
                b = R[args[1]]
                R[args[0]] = (b[0], b[1] + int(args[2], 0))
        elif op == "li":
            R[args[0]] = ("imm", int(args[1], 0))
        elif op == "mr":
            R[args[0]] = R[args[1]]
        elif op == "fmr":
            Fr[args[0]] = Fr[args[1]]
        elif op in ("lwz", "lwzu"):
            o, b = memop(args[1])
            a = ea(o, b)
            R[args[0]] = load(a)
            if op == "lwzu":
                R[b] = a
        elif op in ("lfs", "lfsu"):
            o, b = memop(args[1])
            Fr[args[0]] = load(ea(o, b))
        elif op in ("stw", "stwu", "stfs", "stfsu"):
            o, b = memop(args[1])
            a = ea(o, b)
            v = Fr[args[0]] if op.startswith("stf") else R.get(args[0], ("?reg", args[0]))
            if a[0] == "stack":
                stack[a[1]] = v
            else:
                stores[a] = v
            if op in ("stwu", "stfsu"):
                R[b] = a
        elif op == "bl" and args[0] == "__ct__6CColorFffff":
            a = R["r3"]
            v = ("CColor", Fr["f1"], Fr["f2"], Fr["f3"], Fr["f4"])
            if a[0] == "stack":
                stack[a[1]] = v
            else:
                stores[a] = v
        elif op in ("mflr", "mtlr", "stmw", "lmw", "blr"):
            pass
        else:
            raise SystemExit(f"unhandled: {line}")
    return stores


def port_entries(fname, array, cls):
    text = (SRC / fname).read_text(errors="replace")
    m = re.search(r"\b" + array + r"\[\]\s*=\s*\{(.*?)\n\};", text, re.S)
    names = re.findall(r"DG\w*_(?:TRIGGER|STATE|CODE)\((\w+)\)", m.group(1))
    out = []
    for n in names:
        # the definition and the marker above it (the member may be defined in any DG file)
        fn = None
        for f in sorted(SRC.glob("PortCDigitalGuardian*.cpp")):
            lines = f.read_text(errors="replace").splitlines()
            for i, l in enumerate(lines):
                if re.match(r"^\S.*\b" + cls + r"::" + n + r"\(", l):
                    for j in range(i - 1, max(i - 40, 0), -1):
                        mm = re.match(r"^// @ (fn_14_[0-9A-F]+)", lines[j])
                        if mm:
                            fn = mm.group(1)
                            break
                        if lines[j].startswith("}"):
                            break
                    break
            if fn:
                break
        out.append((n, fn))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--asm", default=str(DEFAULT_ASM))
    ap.add_argument("--full", default=str(DEFAULT_FULL))
    args = ap.parse_args()
    asm = pathlib.Path(args.asm)
    mem = {}
    for s in ("data", "rodata", "bss"):
        mem.update(read_section(asm, s))
    strings = read_strings(asm)
    bad = 0
    for init, tables in TABLES.items():
        stores = simulate(pathlib.Path(args.full) / f"{init}.s", mem)
        print(f"#### fn_14_{init}: {len(stores)} stores")
        covered = set()
        for label, fname, array, cls in tables:
            base = sym_addr(label)
            host = port_entries(fname, array, cls)
            rows = []
            off = base[1]
            while True:
                name = mem.get(("data", off))
                if not isinstance(name, str) or not name.startswith("lbl_14_rodata_"):
                    break
                words = [stores.get(("data", off + k), mem.get(("data", off + k))) for k in (4, 8, 12)]
                for k in (4, 8, 12):
                    covered.add(("data", off + k))
                rows.append((strings.get(name, name), words))
                off += 16
            print(f"== {label} ({array}): retail {len(rows)}, port {len(host)}")
            for i in range(max(len(rows), len(host))):
                rn, rw = rows[i] if i < len(rows) else ("-", ["-"] * 3)
                hn, hf = host[i] if i < len(host) else ("-", None)
                ok = rn == hn and rw[0] == 0 and rw[1] == 0xFFFFFFFF and rw[2] == hf
                bad += not ok
                print(f"  {i:2d} {rn:28s} {str(rw[2]):12s} {hn:28s} {str(hf):12s} "
                      f"{'ok' if ok else 'MISMATCH ' + str(rw)}")
        print("  other stores:")
        for a in sorted(set(stores) - covered):
            print(f"    {a[0]} 0x{a[1]:X} = {stores[a]}")
    print("MISMATCHES:", bad)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
