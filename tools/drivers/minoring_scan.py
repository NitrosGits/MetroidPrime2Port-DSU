#!/usr/bin/env python3
"""Scan the MinorIng REL's asm for the function table the classification needs.

    python3 tools/drivers/minoring_scan.py [--asm DIR] [--tsv OUT.tsv]

Reads the dtk-style asm the port's `tools/asmcheck.py` reads (the module's
`build/G2ME01/<Module>/asm` directory) and prints, for every `.fn`, its offset,
size, the module functions it calls, and the data objects that name it (the
vtables, the FSM tables and the ctor/dtor lists).  That is the evidence the
H/T/B classification cites: a function no text and no table names is dead in
the REL.

The asm root defaults to $ASMCHECK_ASM_ROOT (the same default asmcheck uses).
"""
import argparse
import os
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
DEFAULT_ASM = os.environ.get(
    "ASMCHECK_ASM_ROOT",
    "/run/media/odran/Leo/projects/Restored-projects/Chatgpt/MetroidPrime2Port/build/G2ME01",
)

FN_RE = re.compile(r"^\.fn\s+(\S+?)(?:,\s*(\w+))?\s*$")
END_FN_RE = re.compile(r"^\.endfn\s+(\S+)")
OBJ_RE = re.compile(r"^\.obj\s+(\S+?)(?:,\s*(\w+))?\s*$")
END_OBJ_RE = re.compile(r"^\.endobj\s+(\S+)")
OFF_RE = re.compile(r"^#\s*\.text:0x[0-9A-Fa-f]+\s*\|\s*0x([0-9A-Fa-f]+)\s*\|\s*size:\s*0x([0-9A-Fa-f]+)")
DATA_OFF_RE = re.compile(r"^#\s*\.(data|rodata|bss):0x[0-9A-Fa-f]+\s*\|\s*0x([0-9A-Fa-f]+)\s*\|\s*size:\s*0x([0-9A-Fa-f]+)")
INSN_RE = re.compile(r"^/\*\s*([0-9A-Fa-f]{8})\s+(?:[0-9A-Fa-f]{8}\s+)?(?:[0-9A-Fa-f]{2}\s+){4}\*/\s*(.*?)\s*$")
BL_RE = re.compile(r"^bl\s+([A-Za-z_][\w.$]*)(?:@ha|@l)?", re.I)
LIS_RE = re.compile(r"^(?:lis|addis)\s+r\d+,\s*([A-Za-z_][\w.$]*)@(ha|l)", re.I)


def parse_dir(asm_dir):
    """-> (functions, dataobjs): {name: {...}}, {name: {...}}"""
    funcs = {}
    objs = {}
    for path in sorted(asm_dir.rglob("*.s")):
        cur_fn = None
        pend_fn = None
        cur_obj = None
        pend_obj = None
        fn_off = fn_size = 0
        obj_off = obj_size = 0
        obj_sec = "data"
        for raw in path.read_text(errors="replace").splitlines():
            line = raw.split("//")[0].rstrip()
            if not line.strip():
                continue
            om = OFF_RE.match(line)
            if om:
                pend_fn = (int(om.group(1), 16), int(om.group(2), 16))
                pend_obj = None
                continue
            dm = DATA_OFF_RE.match(line)
            if dm:
                pend_obj = (dm.group(1), int(dm.group(2), 16), int(dm.group(3), 16))
                continue
            m = FN_RE.match(line)
            if m:
                cur_fn = m.group(1)
                cur_obj = None
                funcs.setdefault(cur_fn, {"off": 0, "size": 0, "calls": [], "insns": [], "files": []})
                funcs[cur_fn]["files"].append(path.name)
                if pend_fn:
                    funcs[cur_fn]["off"], funcs[cur_fn]["size"] = pend_fn
                continue
            m = END_FN_RE.match(line)
            if m:
                cur_fn = None
                continue
            m = OBJ_RE.match(line)
            if m:
                cur_obj = m.group(1)
                objs.setdefault(cur_obj, {"off": 0, "size": 0, "sec": "data", "names": [], "files": []})
                objs[cur_obj]["files"].append(path.name)
                if pend_obj:
                    objs[cur_obj]["sec"] = pend_obj[0]
                    objs[cur_obj]["off"] = pend_obj[1]
                    objs[cur_obj]["size"] = pend_obj[2]
                continue
            m = END_OBJ_RE.match(line)
            if m:
                cur_obj = None
                continue
            if cur_fn is not None:
                im = INSN_RE.match(line)
                if im:
                    text = im.group(2)
                    funcs[cur_fn]["insns"].append((int(im.group(1), 16), text))
                    bm = BL_RE.match(text)
                    if bm:
                        funcs[cur_fn]["calls"].append(bm.group(1))
                    continue
            if cur_obj is not None:
                # a data word that names a function symbol
                for tok in re.findall(r"[A-Za-z_][\w.$]*", line):
                    if tok.startswith("fn_44_") or tok in ("RELMain", "RELExit", "_prolog", "_epilog",
                                                           "ModuleConstructors", "ModuleDestructors",
                                                           "__register_global_object", "__destroy_global_chain",
                                                           "_unresolved"):
                        objs[cur_obj]["names"].append(tok)
    return funcs, objs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--asm", default=None, help="module asm dir (default $ASMCHECK_ASM_ROOT/<Module>/asm)")
    ap.add_argument("--module", default="MinorIng")
    ap.add_argument("--tsv", default=None, help="write the raw table here")
    args = ap.parse_args()

    asm_dir = pathlib.Path(args.asm) if args.asm else pathlib.Path(DEFAULT_ASM) / args.module / "asm"
    if not asm_dir.exists():
        print("no asm dir:", asm_dir, file=sys.stderr)
        return 2
    funcs, objs = parse_dir(asm_dir)

    rows = []
    for name, f in sorted(funcs.items(), key=lambda kv: kv[1]["off"]):
        callers = [n for n, g in funcs.items() if name in g["calls"]]
        named_by = [o for o, g in objs.items() if name in g["names"]]
        rows.append((f["off"], f["size"], name, callers, named_by))
    if args.tsv:
        with open(args.tsv, "w") as fh:
            fh.write("offset\tsize\tname\tcallers\tnamed_by\n")
            for off, size, name, callers, named_by in rows:
                fh.write(f"0x{off:X}\t0x{size:X}\t{name}\t{' '.join(sorted(callers))}\t{' '.join(sorted(named_by))}\n")
    for off, size, name, callers, named_by in rows:
        print(f"0x{off:X}\t0x{size:X}\t{name}\tcallers={len(callers)}\tnamed_by={len(named_by)}"
              f"\t{' '.join(sorted(callers))}\t{' '.join(sorted(named_by))}")
    print(f"\n{len(funcs)} functions, {len(objs)} data objects", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
