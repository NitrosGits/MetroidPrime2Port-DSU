#!/usr/bin/env python3
"""Flag suspicious `H` and `T` rows in docs/module-analysis/<Module>_functions.tsv.

Report-only: reads the tables and the retail asm, never writes them. The class column
is `B` (behaviour to reproduce), `H` (a host copy the base class already provides) or
`T` (dead or table-only); see docs/DECOMP_WORKFLOW.md "Progress". A wrong H or T label
hides real B work, so each flag below is a reason to look again, not a verdict.

T rows
  1. called by a B row's body (`bl`/`b fn_<mod>_<off>` in the same module)
  2. its address is in a data/rodata table that a B row's body installs
     (`lis rN, lbl_...@ha` ... `stw rN, 0x0(rM)`)
  3. body longer than a trivial stub (> 0x40 bytes) and containing calls

H rows
  4. body calls module-local functions (`bl fn_<mod>_...`) - a base-class/template
     copy should not call back into the module (split: callee is a B row = strong,
     callee is itself H/T = weak)
  5. body reads module rodata/data (`lbl_<mod>_...`) beyond the vtable pointer it
     installs
  6. size differs a lot (>= 2x and >= 0x20 bytes) from other copies of the same
     named function in other modules

Usage:
  tools/ht_audit.py                      # report on stdout
  tools/ht_audit.py --out /tmp/x.md      # report to a file
  tools/ht_audit.py --module Sandworm    # one module

Flags are grouped by kind, strongest first; within a kind they are sorted by module
and offset. Two exclusions keep the noise down, and both are stated in the report:
a size outlier whose body forwards to the base of the same name is kept as the
weakest kind, and module plumbing (static initialisers, crtexe, REL entry stubs)
is not compared against named base functions.
"""
import argparse
import glob
import os
import re
import statistics
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TABLE_DIR = os.path.join(ROOT, "docs", "module-analysis")
BUILD = os.path.join(ROOT, "build", "G2ME01")

SKIP_COLS = {
    "offset", "size", "class", "kind", "fn", "symbol", "callers", "marker",
    "port", "marked", "tables", "package", "owner", "callees", "state",
}

ASM_LINE = re.compile(
    r"^/\*\s*([0-9A-Fa-f]{8})\s+([0-9A-Fa-f]{8})\s+(?:(?:[0-9A-Fa-f]{2}\s+){4})?\*/\s*(.*)$")
FN_START = re.compile(r"^\.fn\s+(\w+)")
FN_END = re.compile(r"^\.endfn")
OBJ_START = re.compile(r"^\.obj\s+(\w+)")
FN_NAME = re.compile(r"\bfn_(\d+)_([0-9A-Fa-f]+)\b")
LBL_NAME = re.compile(r"\b(lbl_(\d+)_(?:data|rodata|bss)_([0-9A-Fa-f]+))\b")
QNAME = re.compile(r"\b([A-Z][A-Za-z0-9_]*(?:::[~A-Za-z0-9_]+)+)")
SIMPLE = re.compile(r"^[A-Za-z_~][A-Za-z0-9_]{2,}$")

# docs/DECOMP_WORKFLOW.md "Progress": the fallback when a table has no class column.
ASM_ROOTS_USED = []

KIND_CLASS = {
    "behaviour": "B", "host-copy": "H", "vtable": "T", "unreferenced": "T",
    "rel-runtime": "T", "rel-entry": "T",
}

WEIGHT = {  # lower = stronger; used for the sort only
    "t-called-by-b": 1,
    "t-large-with-calls": 2,
    "h-calls-b-local": 3,
    "t-in-vtable-installed-by-b": 4,
    "t-called-by-b-trivial": 5,
    "h-reads-module-data": 6,
    "h-size-outlier": 7,
    "t-in-vtable-trivial": 8,
    "h-calls-ht-local": 9,
    "h-size-outlier-forwarder": 10,
}

KIND_NOTE = {
    "t-called-by-b": "A T row that a B row's body calls is reachable behaviour, not dead code.",
    "t-called-by-b-trivial": "Called by a B row, but the body is a single-instruction accessor "
                             "(still reachable; likely harmless either way).",
    "t-large-with-calls": "A T row whose body is longer than a stub and calls out is not dead code.",
    "t-in-vtable-installed-by-b": "Its address sits in a table a B row installs. Expected for a class's "
                                  "own vtable; suspicious when the body is not a trivial thunk, because "
                                  "then the module really runs it.",
    "t-in-vtable-trivial": "In a table a B row installs, body is a trivial thunk (the normal case for "
                           "an inherited base virtual).",
    "h-calls-b-local": "An H row that calls a B row is not a self-contained base/template copy.",
    "h-calls-ht-local": "An H row that calls other module-local rows (themselves H/T). Weaker: a copied "
                        "helper often calls another copy, but a real base-copy should not call into the module.",
    "h-reads-module-data": "An H row that reads module rodata/data other than the vtable pointer it installs "
                           "is module-specific code, not a base-class copy.",
    "h-size-outlier": "Size differs a lot from the same named function's copies in other modules.",
    "h-size-outlier-forwarder": "Size differs a lot from the copies, but the body only forwards to the "
                                "base of the same name, so H is probably right; listed for completeness.",
}

TRIVIAL = 0x10


class Insn:
    __slots__ = ("line", "addr", "instr", "ops", "raw")

    def __init__(self, line, addr, instr, ops, raw):
        self.line, self.addr, self.instr, self.ops, self.raw = line, addr, instr, ops, raw


class Body:
    def __init__(self, name, path, line):
        self.name, self.path, self.line = name, path, line
        self.insns = []
        self.size = 0

    def finish(self):
        if self.insns:
            self.size = self.insns[-1].addr + 4 - self.insns[0].addr

    def calls(self):
        """[(target, insn)] for every bl/b with a symbol target."""
        out = []
        for i in self.insns:
            if i.instr in ("bl", "b", "bcl"):
                m = re.match(r"([A-Za-z_][\w.]*)", i.ops)
                if m:
                    out.append((m.group(1), i))
        return out

    def label_refs(self):
        out = []
        for i in self.insns:
            for m in LBL_NAME.finditer(i.ops):
                out.append((m.group(1), i))
        return out

    def fingerprint(self):
        toks = []
        for i in self.insns:
            ops = re.sub(r"\bfn_\d+_[0-9A-Fa-f]+\b", "<localfn>", i.ops)
            ops = re.sub(r"\blbl_\d+_\w+", "<lbl>", ops)
            ops = re.sub(r"\b[A-Za-z_]\w*__\w+", lambda m: m.group(0), ops)
            # branch displacements differ between copies of the same body at
            # different addresses; call/jump targets keep their (DOL) name.
            if i.instr in ("b", "bl", "bc", "bcl", "beq", "bne", "bge", "ble", "bgt",
                           "blt", "bdnz", "bdz", "bso", "bns"):
                ops = re.sub(r"0x[0-9A-Fa-f]+", "<disp>", ops)
            toks.append((i.instr, ops))
        return tuple(toks)


class Module:
    def __init__(self, name, asm_dir, modid):
        self.name, self.asm_dir, self.modid = name, asm_dir, modid
        self.bodies = {}          # fn name -> Body
        self.tables = {}          # data label -> {"file","line","entries":[(line,text)]}
        self.rows = []

    def body(self, fnname):
        return self.bodies.get(fnname)


def read_asm(module):
    """Parse every .s under the module's asm dir into bodies and data tables."""
    if not module.asm_dir:
        return
    for path in sorted(glob.glob(os.path.join(module.asm_dir, "*.s"))):
        base = os.path.basename(path)
        cur_body = None
        cur_obj = None
        try:
            fh = open(path, errors="replace")
        except OSError:
            continue
        with fh:
            for lineno, line in enumerate(fh, 1):
                line = line.rstrip("\n")
                m = FN_START.match(line)
                if m:
                    cur_body = Body(m.group(1), base, lineno)
                    module.bodies.setdefault(cur_body.name, cur_body)
                    continue
                if FN_END.match(line):
                    if cur_body is not None:
                        cur_body.finish()
                    cur_body = None
                    continue
                m = OBJ_START.match(line)
                if m:
                    cur_obj = m.group(1)
                    module.tables[cur_obj] = {"file": base, "line": lineno, "entries": []}
                    continue
                if line.startswith(".endobj"):
                    cur_obj = None
                    continue
                if cur_obj is not None and line.strip().startswith(".4byte"):
                    module.tables[cur_obj]["entries"].append((lineno, line.strip()))
                    continue
                if cur_body is not None:
                    am = ASM_LINE.match(line)
                    if am:
                        addr = int(am.group(1), 16)
                        text = am.group(3).strip()
                        parts = text.split(None, 1)
                        instr = parts[0] if parts else ""
                        ops = parts[1] if len(parts) > 1 else ""
                        # drop trailing comments the parser sometimes emits
                        cur_body.insns.append(Insn(lineno, addr, instr, ops, text))
        for b in module.bodies.values():
            pass


def detect_modid(asm_dir):
    counts = {}
    for path in sorted(glob.glob(os.path.join(asm_dir, "*.s")))[:6]:
        with open(path, errors="replace") as fh:
            for line in fh:
                m = FN_START.match(line)
                if m:
                    fm = FN_NAME.match(m.group(1))
                    if fm:
                        counts[fm.group(1)] = counts.get(fm.group(1), 0) + 1
        if counts:
            break
    if not counts:  # no asm: fall back to any fn_ id in the table
        return None
    return max(counts, key=counts.get)


def parse_offset(value, modid):
    v = (value or "").strip().lower()
    if v.startswith("0x"):
        v = v[2:]
    if not re.fullmatch(r"[0-9a-f]+", v or "-"):
        return None
    return int(v, 16)


def derived_fn(modid, off):
    return "fn_%s_%X" % (modid, off)


def load_table(module):
    path = os.path.join(TABLE_DIR, module.name + "_functions.tsv")
    if not os.path.exists(path):
        return False
    rows = []
    with open(path, errors="replace") as fh:
        header = fh.readline().rstrip("\n").split("\t")
        for lineno, line in enumerate(fh, 2):
            line = line.rstrip("\n")
            if not line.strip():
                continue
            fields = line.split("\t")
            row = dict(zip(header, fields))
            row["_line"] = lineno
            row["_file"] = os.path.basename(path)
            off = parse_offset(row.get("offset"), module.modid)
            row["_off"] = off
            cls = (row.get("class") or "").strip().upper()
            if cls not in ("B", "H", "T"):
                cls = KIND_CLASS.get((row.get("kind") or "").strip().lower(), "B")
            row["_class"] = cls
            fn = None
            for col in ("fn", "symbol", "name"):
                m = FN_NAME.search(row.get(col) or "")
                if m and m.group(1) == str(module.modid):
                    fn = m.group(0)
                    break
            if fn is None and off is not None:
                fn = derived_fn(module.modid, off)
            row["_fn"] = fn
            rows.append(row)
    module.rows = rows
    return True


def row_label(row):
    return "%s %s" % (row.get("_fn") or "?", row.get("offset", "?"))


# ---------------------------------------------------------------- checks

def vtable_installs(body):
    """{label: Insn} for tables/pointers a body installs with stw into 0(rN).

    The MWCC shape is `lis rA, lbl@ha` / `addi rB, rA, lbl@l` / `stw rB, 0x0(rC)`
    (src and dst register differ often, e.g. `addi r0, r3, lbl_10_data_40@l`).
    Operands arrive without the mnemonic, so each pattern starts at the register.
    """
    reg = {}
    out = {}
    for i in body.insns:
        m = re.match(r"r(\d+),\s*(\w+)@ha", i.ops)
        if m and i.instr == "lis":
            reg[int(m.group(1))] = m.group(2)
            continue
        m = re.match(r"r(\d+),\s*r\d+,\s*(\w+)@l", i.ops)
        if m and i.instr in ("addi", "addis", "la"):
            reg[int(m.group(1))] = m.group(2)
            continue
        m = re.match(r"r(\d+),\s*(\w+)@l", i.ops)
        if m and i.instr == "la":
            reg[int(m.group(1))] = m.group(2)
            continue
        m = re.match(r"r(\d+),\s*(?:0x0|0)\(r\d+\)", i.ops)
        if m and i.instr in ("stw", "stwu"):
            src = int(m.group(1))
            if src in reg:
                out[reg[src]] = i
    return out


def is_local_fn(name, modid):
    m = FN_NAME.match(name or "")
    return bool(m and m.group(1) == str(modid))


def table_refs(module):
    """{fn name: [(label, line, text)]} for fn pointers stored in data tables."""
    out = {}
    for label, info in module.tables.items():
        for lineno, text in info["entries"]:
            for m in FN_NAME.finditer(text):
                if m.group(1) != str(module.modid):
                    continue
                out.setdefault(m.group(0), []).append((label, lineno, text, info))
    return out


STOP_WORDS = {"None", "none", "Null", "null", "The", "the", "REL", "DOL", "PC",
              "TARGET", "asm", "data", "rodata"}


def name_keys(row):
    """{short function name: provenance} for this row's copy of a named function.

    Only the `name`/`symbol` column and qualified `Class::Name` citations count, so
    "a CPatterned body" or a destructor `~CPatterned` never becomes a key.
    """
    keys = {}
    nm = (row.get("name") or row.get("symbol") or "").strip()
    nm = re.sub(r"^fn_\d+_[0-9A-Fa-f]+", "", nm).strip()
    nm = re.sub(r"\(.*", "", nm).strip()
    if (SIMPLE.match(nm) and nm not in STOP_WORDS and not nm.startswith("~")
            and not nm.startswith(("lbl_", "data_"))):
        keys[nm] = "name column"
    for col, val in row.items():
        if col.startswith("_") or col in SKIP_COLS or not val:
            continue
        for m in QNAME.finditer(val):
            full = m.group(1)
            last = full.split("::")[-1]
            if not last or last.startswith("~") or last in STOP_WORDS:
                continue
            keys.setdefault(last, "cited as %s" % full)
    return keys


def audit(modules):
    # global body index for cross-module checks
    by_fn = {}          # (module, fn) -> row
    bodies = {}
    for mod in modules:
        for row in mod.rows:
            if row["_fn"]:
                by_fn[(mod.name, row["_fn"])] = row
        for fn, body in mod.bodies.items():
            bodies[(mod.name, fn)] = body

    # fingerprint groups for the structural-twin view
    fp_groups = {}
    for (mname, fn), body in bodies.items():
        fp_groups.setdefault(body.fingerprint(), []).append((mname, fn))

    # named groups (only rows that name their counterpart)
    name_groups = {}
    for mod in modules:
        for row in mod.rows:
            for k in name_keys(row):
                name_groups.setdefault(k, []).append((mod.name, row))

    flags = []
    for mod in modules:
        row_of = {r["_fn"]: r for r in mod.rows if r["_fn"]}
        trefs = table_refs(mod) if mod.asm_dir else {}
        callers_of = {}   # local fn -> [(caller_fn, caller_row, insn, kind)]
        lblloads = {}     # data label -> [(fn, row, insn)]
        for fn, body in mod.bodies.items():
            caller_row = row_of.get(fn)
            for target, insn in body.calls():
                if is_local_fn(target, mod.modid):
                    callers_of.setdefault(target, []).append((fn, caller_row, insn))
            for label, insn in body.label_refs():
                lblloads.setdefault(label, []).append((fn, caller_row, insn, body))

        for row in mod.rows:
            cls = row["_class"]
            fn = row["_fn"]
            body = mod.bodies.get(fn) if fn else None
            size = body.size if body and body.size else parse_offset(row.get("size"), mod.modid) or 0
            base = {"module": mod.name, "row": row, "fn": fn, "cls": cls}

            if cls == "T" and body is not None:
                # 1. called by a B row
                for caller_fn, caller_row, insn in callers_of.get(fn, []):
                    if caller_row is not None and caller_row["_class"] == "B":
                        kind = "t-called-by-b" if (body.size or 0) > 0x8 else "t-called-by-b-trivial"
                        flags.append(dict(base, kind=kind, weight=WEIGHT[kind],
                                          reason="called from B row %s (body 0x%X)" % (
                                              caller_fn, body.size or 0),
                                          evidence=["%s:%d  %s" % (body.path, insn.line, insn.raw)],
                                          extra=["call site in %s (%s:%d)" % (
                                              caller_fn, body.path, insn.line),
                                              "callee body: %s:%d" % (body.path, body.line)]))
                        break
                # 2. in a table a B row installs
                for label, tline, ttext, tinfo in trefs.get(fn, []):
                    inst = None
                    for lfn, lrow, insn, lbody in lblloads.get(label, []):
                        if lrow is not None and lrow["_class"] == "B" and label in vtable_installs(lbody):
                            inst = (lfn, lrow, lbody)
                            break
                    if inst:
                        lfn, lrow, lbody = inst
                        iinsn = vtable_installs(lbody)[label]
                        kind = ("t-in-vtable-installed-by-b" if (body.size or 0) > TRIVIAL
                                else "t-in-vtable-trivial")
                        flags.append(dict(base, kind=kind, weight=WEIGHT[kind],
                                          reason="address in table %s installed by B row %s (body 0x%X)" % (
                                              label, lfn, body.size or 0),
                                          evidence=["%s:%d  %s" % (tinfo["file"], tline, ttext),
                                                    "%s:%d  %s" % (lbody.path, iinsn.line, iinsn.raw)],
                                          extra=["table %s at %s:%d" % (label, tinfo["file"], tinfo["line"])]))
                        break
                # 3. not a trivial stub
                if size > 0x40 and body.calls():
                    tgt, insn = body.calls()[0]
                    flags.append(dict(base, kind="t-large-with-calls", weight=WEIGHT["t-large-with-calls"],
                                      reason="body 0x%X with %d call(s), first to %s" % (
                                          size, len(body.calls()), tgt),
                                      evidence=["%s:%d  %s" % (body.path, insn.line, insn.raw)],
                                      extra=["body starts %s:%d" % (body.path, body.line)]))

            if cls == "H" and body is not None:
                local = [(t, insn) for t, insn in body.calls() if is_local_fn(t, mod.modid)]
                if local:
                    bcallee = [(t, insn) for t, insn in local
                               if row_of.get(t) is not None and row_of[t]["_class"] == "B"]
                    pick = bcallee[0] if bcallee else local[0]
                    kind = "h-calls-b-local" if bcallee else "h-calls-ht-local"
                    flags.append(dict(base, kind=kind, weight=WEIGHT[kind],
                                      reason="calls module-local %s%s" % (
                                          ", ".join(sorted({t for t, _ in local})),
                                          " (a B row)" if bcallee else " (itself H/T)"),
                                      evidence=["%s:%d  %s" % (body.path, pick[1].line, pick[1].raw)],
                                      extra=["body starts %s:%d" % (body.path, body.line)]))
                installed = vtable_installs(body)
                reads = [(lbl, insn) for lbl, insn in body.label_refs() if lbl not in installed]
                if reads:
                    lbl, insn = reads[0]
                    flags.append(dict(base, kind="h-reads-module-data", weight=WEIGHT["h-reads-module-data"],
                                      reason="reads module data %s%s" % (
                                          ", ".join(sorted({l for l, _ in reads})[:3]),
                                          "" if installed else " (and installs no vtable)"),
                                      evidence=["%s:%d  %s" % (body.path, insn.line, insn.raw)],
                                      extra=["body starts %s:%d" % (body.path, body.line)]))

        # cross-module size outliers, per module
        for row in mod.rows:
            if row["_class"] != "H":
                continue
            fn = row["_fn"]
            body = mod.bodies.get(fn) if fn else None
            if body is None or not body.size:
                continue
            text = " ".join(v for k, v in row.items() if not k.startswith("_") and v)
            if PLUMBING.search(text):
                continue
            size = body.size
            keys = name_keys(row)
            twins = []      # (module, fn, size, why)
            fp = body.fingerprint()
            grp = [t for t in fp_groups.get(fp, []) if t[0] != mod.name]
            for tm, tf in grp:
                tb = bodies[(tm, tf)]
                twins.append((tm, tf, tb.size, "same body shape"))
            for k, provenance in keys.items():
                for tm, trow in name_groups.get(k, []):
                    if tm == mod.name:
                        continue
                    tb = bodies.get((tm, trow["_fn"]))
                    if tb is None or not tb.size:
                        continue
                    twins.append((tm, trow["_fn"], tb.size, "both %s: %s" % (provenance, k)))
            if not twins:
                continue
            # A copy that forwards to the base of the same name (PreRenderAllViewports
            # -> PreRenderAllViewports__10CPatterned) is smaller by design and is H: keep
            # it, but as the weakest kind, so the reason is visible.
            forwarder = None
            for k in keys:
                forwarder = forwarded_to_base(body.calls(), k)
                if forwarder:
                    break
            sizes = [t[2] for t in twins]
            med = statistics.median(sizes)
            diff = abs(size - med)
            if diff >= 0x20 and (size >= 2 * med or med >= 2 * size):
                tm, tf, tsize, why = min(twins, key=lambda t: abs(t[2] - med))
                tb = bodies[(tm, tf)]
                kind = "h-size-outlier-forwarder" if forwarder else "h-size-outlier"
                flags.append({"module": mod.name, "row": row, "fn": fn, "cls": "H",
                              "kind": kind, "weight": WEIGHT[kind],
                              "reason": "0x%X vs 0x%X median of %d cross-module copies (%s)" % (
                                  size, int(med), len(twins), why)
                              + ("" if not forwarder else "; body forwards to %s" % forwarder),
                              "evidence": ["%s:%d  %s" % (body.path, body.line, first_insn_text(body)),
                                           "%s:%d  %s" % (tb.path, tb.line, first_insn_text(tb))],
                              "extra": ["twin: %s %s (0x%X)" % (tm, tf, tsize)]})
    flags.sort(key=lambda f: (f["weight"], f["module"], f["row"]["_off"] if f["row"]["_off"] is not None else 0))
    return flags


PLUMBING = re.compile(
    r"static.init|initialiser|initializer|\.ctors|REL runtime|crtexe|"
    r"global.chain|unresolved|_prolog|_epilog|ModuleConstructors|ModuleDestructors")


def forwarded_to_base(calls, key):
    """The call target that is the DOL/base body of the same name (`Key__10CPatternedFv`), else None."""
    for target, _insn in calls:
        if target.split("__")[0] == key:
            return target
    return None


def first_insn_text(body):
    if body.insns:
        return body.insns[0].raw
    return ".fn %s" % body.name


def asm_roots():
    """Candidate build/G2ME01 roots, best first.

    The asm is a build artefact, so a lane worktree normally has none: fall back to
    $MP_ASM_ROOT and to the other worktrees of this repo. The caller scores them by
    module coverage; ties keep this order, which puts a `port` checkout before the
    archived matching-decomp tree (that one leaves DOL calls as `fn_802xxxxx`).
    """
    roots = []
    env = os.environ.get("MP_ASM_ROOT")
    if env:
        roots.append(env)
    roots.append(BUILD)
    seen = set()
    trees = worktrees()
    trees.sort(key=lambda t: (t[1] != "port",))
    for path, _branch in trees:
        cand = os.path.join(path, "build", "G2ME01")
        if os.path.isdir(cand) and os.path.realpath(cand) not in seen:
            seen.add(os.path.realpath(cand))
            roots.append(cand)
    for cand in sorted(glob.glob(os.path.join(os.path.dirname(ROOT), "*", "build", "G2ME01"))):
        if os.path.isdir(cand) and os.path.realpath(cand) not in seen:
            seen.add(os.path.realpath(cand))
            roots.append(cand)
    return roots


def worktrees():
    """[(path, branch)] from `git worktree list --porcelain` (branch short name, "" if none)."""
    try:
        out = subprocess.run(["git", "-C", ROOT, "worktree", "list", "--porcelain"],
                             capture_output=True, text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError):
        return []
    trees, path, branch = [], None, ""
    for line in out.splitlines() + [""]:
        if line.startswith("worktree "):
            path, branch = line[len("worktree "):], ""
        elif line.startswith("branch "):
            branch = line[len("branch refs/heads/"):]
        elif not line.strip() and path:
            trees.append((path, branch))
            path = None
    return trees


def pick_asm_root(names):
    """The candidate root with the most of our modules' asm; ties keep the earlier one."""
    best, best_key = None, None
    for idx, root in enumerate(asm_roots()):
        score = sum(1 for n in names if os.path.isdir(os.path.join(root, n, "asm")))
        key = (-score, idx)
        if best_key is None or key < best_key:
            best, best_key = root, key
    return best


def find_asm_dir(name, roots):
    for root in roots:
        path = os.path.join(root, name, "asm")
        if os.path.isdir(path):
            return path, root
    return None, None


def load_modules(only=None):
    modules = []
    roots = asm_roots()
    used_roots = set()
    all_names = [os.path.basename(p)[: -len("_functions.tsv")]
                 for p in sorted(glob.glob(os.path.join(TABLE_DIR, "*_functions.tsv")))]
    # score the asm roots over every table, so --module picks the same root as a full run
    preferred = pick_asm_root(all_names)
    if preferred:
        roots = [preferred] + [r for r in roots if r != preferred]
    for path in sorted(glob.glob(os.path.join(TABLE_DIR, "*_functions.tsv"))):
        name = os.path.basename(path)[: -len("_functions.tsv")]
        if only and name not in only:
            continue
        asm_dir, root = find_asm_dir(name, roots)
        if root:
            used_roots.add(root)
        mod = Module(name, asm_dir, None)
        mod.modid = detect_modid(mod.asm_dir) if mod.asm_dir else None
        if mod.modid is None:
            # no asm: derive the id from the table's own fn_ ids
            mod.modid = "?"
            with open(path, errors="replace") as fh:
                fh.readline()
                for line in fh:
                    m = FN_NAME.search(line)
                    if m:
                        mod.modid = m.group(1)
                        break
        read_asm(mod)
        load_table(mod)
        modules.append(mod)
    global ASM_ROOTS_USED
    ASM_ROOTS_USED = sorted(used_roots)
    return modules


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", help="write the markdown report here instead of stdout")
    ap.add_argument("--module", action="append", help="restrict to a module (repeatable)")
    args = ap.parse_args()

    modules = load_modules(set(args.module) if args.module else None)
    if not modules:
        print("ht_audit: no *_functions.tsv tables found", file=sys.stderr)
        return 1
    flags = audit(modules)

    out = []
    tot = {c: 0 for c in "BHT"}
    for m in modules:
        for r in m.rows:
            tot[r["_class"]] += 1
    audited = 0
    for m in modules:
        for r in m.rows:
            if r["_fn"] and m.bodies.get(r["_fn"]) is not None:
                audited += 1
    by_mod = {}
    for f in flags:
        by_mod[f["module"]] = by_mod.get(f["module"], 0) + 1

    out.append("# H/T class audit")
    out.append("")
    out.append("Report-only: `tools/ht_audit.py` reads the tables and the retail asm, it never changes "
               "a class column. Each flag is a reason to look again, not a verdict. Evidence is "
               "`asm file:line` plus the raw line, in the module's `build/G2ME01/<Module>/asm/`.")
    out.append("")
    out.append("Tables: %d modules, B %d, H %d, T %d; %d row(s) flagged." % (
        len(modules), tot["B"], tot["H"], tot["T"], len(flags)))
    if ASM_ROOTS_USED:
        out.append("")
        out.append("Asm roots used: %s" % ", ".join("`%s`" % r for r in ASM_ROOTS_USED))
    out.append("")
    out.append("## Per module")
    out.append("")
    out.append("| module | H total | T total | flagged | asm |")
    out.append("|---|---|---|---|---|")
    for m in modules:
        h = sum(1 for r in m.rows if r["_class"] == "H")
        t = sum(1 for r in m.rows if r["_class"] == "T")
        out.append("| %s | %d | %d | %d | %s |" % (
            m.name, h, t, by_mod.get(m.name, 0),
            ("%d bodies" % len(m.bodies)) if m.asm_dir
            else "missing (no build/G2ME01/%s/asm)" % m.name))
    out.append("| **TOTAL** | **%d** | **%d** | **%d** | |" % (tot["H"], tot["T"], len(flags)))
    out.append("")
    out.append("## Checks and how many rows they flagged")
    out.append("")
    out.append("| check | rows |")
    out.append("|---|---|")
    for kind in sorted(WEIGHT, key=lambda k: WEIGHT[k]):
        n = sum(1 for f in flags if f["kind"] == kind)
        out.append("| `%s` | %d |" % (kind, n))
    out.append("")
    out.append("Coverage: %d of %d rows had a parsed asm body and were checked; the rest "
               "have no body in the split (`-` offsets, rows past `.text`) or sit in a module "
               "whose asm is not built here. The cross-module size check only compares rows that "
               "name the copied function in their table." % (audited, tot["B"] + tot["H"] + tot["T"]))
    out.append("")
    if not flags:
        out.append("No flags.")
    else:
        out.append("## Flags, strongest reason first")
        out.append("")
        kinds = []
        for f in flags:
            if f["kind"] not in kinds:
                kinds.append(f["kind"])
        kinds.sort(key=lambda k: WEIGHT.get(k, 99))
        for kind in kinds:
            group = [f for f in flags if f["kind"] == kind]
            out.append("### `%s` (%d)" % (kind, len(group)))
            out.append("")
            out.append(KIND_NOTE.get(kind, ""))
            out.append("")
            for f in group:
                row = f["row"]
                out.append("#### %s %s — %s" % (f["module"], f["fn"] or "?", f["cls"]))
                out.append("")
                out.append("- row `%s:%d`: offset %s, size %s" % (
                    row["_file"], row["_line"], row.get("offset", "?"), row.get("size", "?")))
                out.append("- reason: %s" % f["reason"])
                for e in f["evidence"]:
                    out.append("- asm `%s`" % e)
                for e in f.get("extra", []):
                    out.append("- %s" % e)
                out.append("")
    text = "\n".join(out) + "\n"
    if args.out:
        os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
        with open(args.out, "w") as fh:
            fh.write(text)
        print("wrote %s (%d flags over %d modules)" % (args.out, len(flags), len(modules)))
    else:
        sys.stdout.write(text)
        sys.stdout.flush()  # raise BrokenPipeError here, inside main(), not at exit
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except BrokenPipeError:  # `| head` closed the pipe; silence the exit flush
        os.dup2(os.open(os.devnull, os.O_WRONLY), sys.stdout.fileno())
        sys.exit(0)
