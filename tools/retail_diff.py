#!/usr/bin/env python3
"""Static check of ported REL functions against their retail asm: dropped calls, lost constants,
shape and float-compare NaN arms. No build, no game run (whole port in a few seconds).

For each function marked `// @ fn_<mod>_<off>` (found as tools/port_progress.py finds markers) it
compares the port body with the retail body (asm and rodata read through tools/asmcheck.py):
  call   a retail `bl` target the port body never names: a ported module callee by its port name,
         an unported one by fn_<mod>_<off> or an x<OFF>_ name (in code or a comment), a DOL callee
         by its demangled base name (runtime helpers, rstl, ctors/dtors, operators skipped).
         Port helpers the body calls (any def of that name, the caller's class when several) are
         expanded 2 levels; DOL callees the port names another way (DOL_ALIAS: ObjectById,
         AsNormalized, SetDrawShadow...) count as named. Hidden unless --strict: callees missed at
         >= half of their (>= 3) call sites (port-model differences), unported module callees of a
         ctor or dtor (member ctors/dtors), unported rstl/string/allocation template instances,
         CToken::GetObj. A marker inside a body (a switch arm) checks that body (no shape check)
  const  a float constant retail loads from module rodata that appears nowhere in the port file
         as a literal (0 and the int->float conversion magic skipped)
  shape  conditional-branch count far from the port's if/?:/loops/&&/|| count, or a big retail body
         behind a tiny port body
  nan    fcmpo/fcmpu consumers: a plain branch (blt/bgt/bge/ble, NaN arm = the ordered `<`/`>`
         result) needs a strict `<`/`>` in C, negated or not; `cror eq,lt|gt,eq` (+ beq/bne/mfcr)
         needs `<=`/`>=`. Negating a C compare flips its NaN arm together with its sense, so
         `!(a <= b)` is NOT `a > b` on a NaN: it matches a cror+beq, never a plain ble. Flagged:
         the port has fewer strict (or non-strict) compares than retail has plain (cror) float
         compares, or more negated `!(a <= b)` forms than retail has crors. Counts only: leads.
Findings are hints for a human (inlined callees, constants named in another file show up); a clean
function is a good sign, not proof.

usage: retail_diff.py [--module M ...] [--fn fn_X ...] [--diff BASE] [--kinds call,const,shape,nan]
                      [--top N] [--strict]
  no selector: whole port, per-kind totals and the N (default 40) worst functions
  --diff BASE: only functions whose marked body is new or changed since BASE (lane review)
"""
import argparse
import os
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import asmcheck as ac  # noqa: E402
import port_progress  # noqa: E402
import rel_manifest as rm  # noqa: E402

MARKER_FILE = re.compile(r"//\s*@\s*fn_\d+_[0-9A-Fa-f]+\b")
FNID = re.compile(r"fn_(\d+)_([0-9A-Fa-f]+)$")
FLOAT_LIT = re.compile(r"(?<![\w.])(\d+\.\d*(?:[eE][-+]?\d+)?|\.\d+(?:[eE][-+]?\d+)?|\d+[eE][-+]?\d+)[fFlL]?")
INT_LIT = re.compile(r"(?<![\w.])(\d+)(?![\w.])")
MAGIC = {4503601774854144.0, 4503599627370496.0}
SMALL = 0x10  # callee size at or under which a missing call is not reported
CONDB = re.compile(r"^b(?:eq|ne|lt|le|gt|ge|nl|ng|so|ns|un|nu)(?:lr|ctr)?$")
PORT_COND = re.compile(r"\bif\b|\bwhile\b|\bfor\b|\?|&&|\|\||\bcase\b")
# DOL callees the port names another way (same behaviour): base name -> regex over the port body
DOL_ALIAS = {
    "GetObjectById": r"(?:Get)?ObjectById", "ObjectById": r"(?:Get)?ObjectById",
    "Normalize": r"Normalize\w*|AsNormalized|Normalized|CUnitVector3f",
    "AsNormalized": r"Normalize\w*|AsNormalized|Normalized|CUnitVector3f",
    "IsActorVisibleInPortals": r"IsActorVisible\w*",
    "AccumulateBounds": r"AccumulateBounds|Include",
    "DeliverScriptMsg": r"(?:Deliver|Send)ScriptMsgs?",
    "CreateShadow": r"CreateShadow|SetDrawShadow",
    "SetTransformAlt": r"SetTransform\w*|SetRotation",
    "BuildTransform4f": r"BuildTransform4f|SetRotation",
    "CheckConnectedObject_if": r"(?:Check|Find)ConnectedObject_if",
    "InitializeStateMachine": r"InitializeStateMachine|SetupStateMachine",
}
SKIP_DOL = ("GetObj",)
INBODY = set()  # CToken::GetObj: a token dereference (`*tok`, `tok->`), never a behaviour
PLUMB = re.compile(r"rstl|allocate|Free__7CMemory|string_l|internal_dereference|__ct__|__dt__|__nw|__dl")
SKIP_CALL = ("__ct__", "__dt__", "__nw", "__dl", "__as", "__pl", "__mi", "__ml", "__dv", "__eq", "__ne",
             "__lt", "__gt", "__le", "__ge", "__vc", "__cl", "__rf", "__apl", "__ami", "__aml", "__adv")


def rel(p):
    p = Path(p)
    return p.relative_to(ROOT) if p.is_relative_to(ROOT) else p


def f32(x):
    try:
        return struct.unpack(">f", struct.pack(">f", x))[0]
    except (OverflowError, struct.error):
        return x


def fid(mid, off):
    return f"fn_{mid}_{off:X}"


# ---------------------------------------------------------------- retail side

def load_retail(mods):
    """module -> (asmcheck Module, {offset: Func}), module id -> name, (mid, off) -> size."""
    out, ids, sizes = {}, {}, {}
    for m in mods:
        if not os.path.isdir(os.path.join(ac.ASM_ROOT, m, "asm")):
            continue
        mid = rm.module_id(m)
        mod = ac.load_module(m)
        out[m] = (mod, {f.start: f for f in mod.funcs.values() if f.ins})
        ids[mid] = m
        for off, size in rm.read_symbols(m):
            sizes[(mid, off)] = size
    return out, ids, sizes


def dol_symbols():
    """DOL address -> (current symbol name, size). The REL asm names a DOL callee as the symbols
    of the day it was split did (fn_8016C6CC, fn_80036F10__13CStateManagerCFv); the address in that
    name finds today's name (Update__8CIkChainFf)."""
    out, byname = {}, {}
    pat = re.compile(r"^(\S+) = \.\w+:0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)")
    for line in (ROOT / "config/G2ME01/symbols.txt").read_text(errors="replace").splitlines():
        m = pat.match(line)
        if m:
            out[int(m.group(2), 16)] = (m.group(1), int(m.group(3), 16))
            byname[m.group(1)] = int(m.group(2), 16)
    return out, byname


def dol_callee(c, dol):
    """(name to look for in the port, size) of a DOL call target."""
    syms, byname = dol
    m = re.match(r"fn_(8[0-9A-Fa-f]{7})", c)
    addr = int(m.group(1), 16) if m else byname.get(c)
    name, size = syms.get(addr, (c, 0x100))
    return name, size


def crbit(s):
    s = s.strip()
    m = re.fullmatch(r"(?:4\*cr(\d)\+)?(lt|gt|eq|so|un)", s)
    if m:
        return int(m.group(1) or 0), {"un": "so"}.get(m.group(2), m.group(2))
    m = re.fullmatch(r"(\d+)", s)
    return (int(s) // 4, ac.COND_NAMES[int(s) % 4]) if m else (None, s)


def cr_field(a):
    m = re.fullmatch(r"cr(\d)", a.strip())
    return int(m.group(1)) if m else None


def retail_facts(mod, fn):
    """calls, float constants, conditional-branch count, float compare sites [(off, kind)]."""
    calls, consts, conds, fcmps = [], [], 0, []
    base = {}          # reg -> rodata label from `addi rD, rS, lbl@l`
    live = {}          # cr field -> (fcmp offset, has cror)
    for i in fn.ins:
        a = i.args
        if i.op == "bl" and a:
            calls.append(a[0].strip('"'))
        elif CONDB.match(i.op) and not i.op.startswith("bdnz"):
            conds += 1
        if i.op in ("fcmpo", "fcmpu") and a:
            cf = cr_field(a[0]) if len(a) == 3 else 0
            live[cf] = [i.off, False, i.op]
            continue
        if i.op in ("cror", "crnor", "crorc") and len(a) == 3:
            (df, db), (s1f, s1), (s2f, s2) = crbit(a[0]), crbit(a[1]), crbit(a[2])
            if s1f in live and db == "eq" and {s1, s2} in ({"lt", "eq"}, {"gt", "eq"}):
                live[s1f][1] = True
            continue
        if i.op == "mfcr" and live:
            for cf, (off, cr, op) in live.items():
                fcmps.append((off, "nonstrict" if cr else "strict", "mfcr"))
            live = {}
            continue
        m = re.match(r"^b(lt|le|gt|ge|nl|ng|eq|ne)(?:lr|ctr)?$", i.op)
        if m:
            cf = cr_field(a[0]) if a and re.fullmatch(r"cr\d", a[0].strip()) else 0
            if cf in live:
                off, cr, op = live.pop(cf)
                if cr:
                    fcmps.append((off, "nonstrict", "cror+" + i.op))
                elif m.group(1) in ("eq", "ne"):
                    pass          # == / != : NaN arm is C's either way
                else:
                    fcmps.append((off, "strict", i.op))
        if i.op in ("b", "blr", "bctr"):
            live = {}
        # rodata float loads
        if i.op == "addi" and len(a) == 3:
            mm = re.match(r"(lbl_\w+?)(?:\+0x([0-9A-Fa-f]+))?@l$", a[2])
            if mm:
                base[a[0]] = (mm.group(1), int(mm.group(2) or "0", 16))
                continue
        if i.op in ("lfs", "lfsu", "lfd", "lfdu") and len(a) == 2:
            kind = "d" if i.op.startswith("lfd") else "f"
            mm = re.match(r"(lbl_\w+?)(?:\+0x([0-9A-Fa-f]+))?@(?:l|sda21)\((r\d+)\)$", a[1])
            if mm:
                v = ac.rod_value(mod, mm.group(1), int(mm.group(2) or "0", 16), kind)
            else:
                mm = re.match(r"(-?(?:0x[0-9A-Fa-f]+|\d+))\((r\d+)\)$", a[1])
                v = None
                if mm and mm.group(2) in base:
                    lbl, d = base[mm.group(2)]
                    v = ac.rod_value(mod, lbl, d + int(mm.group(1), 0), kind)
            if isinstance(v, float):
                consts.append(v)
        if a and a[0] in base and i.op not in ("lfs", "lfd", "lfsu", "lfdu", "stfs", "stfd", "lwz", "lbz",
                                                "lhz", "lha", "stw", "stb", "sth", "cmpwi", "cmplwi"):
            base.pop(a[0], None)
    return calls, consts, conds, fcmps


# ---------------------------------------------------------------- port side

def marker_files():
    """Files with a `// @ fn_` marker, plus the port-only sources (Port*.cpp/.hpp: their helpers)."""
    out = []
    for src in port_progress.SOURCES:
        for p in src.rglob("*"):
            if p.suffix in (".c", ".cpp", ".h", ".hpp", ".inc"):
                try:
                    if p.name.startswith("Port") or MARKER_FILE.search(p.read_text(errors="replace")):
                        out.append(p)
                except OSError:
                    pass
    return sorted(out)


def norm_id(s):
    m = re.fullmatch(r"fn_(\d+)_([0-9A-Fa-f]+)", s, re.I)
    return (int(m.group(1)), int(m.group(2), 16)) if m else None


def port_functions(paths, display=None):
    """(mid, off) -> CppFunc for every marked body; also helpers (name -> [code]) and file texts.

    A body is found the way asmcheck finds it (`// @ fn_X` in the comment block above it); a marker
    that sits elsewhere (after the body, in a class) falls back to a def citing the id first or
    named x<OFF>_ in the same file. In-class bodies asmcheck does not parse are not checked.
    """
    funcs, helpers, texts = {}, {}, {}
    for p in paths:
        try:
            defs = ac.parse_cpp(str(p))
            text = Path(p).read_text(errors="replace")
        except OSError:
            continue
        shown = display.get(str(p), p) if display else p
        texts[shown] = text
        mk = [(norm_id(m.group(0)[m.group(0).index("fn_"):]), text.count("\n", 0, m.start()) + 1)
              for m in MARKER_FILE.finditer(text)]
        want = {k for k, _ in mk}
        for f in defs:
            f.file = shown
            if f.marked:
                for x in f.ids:
                    k = norm_id(x)
                    if k:
                        funcs.setdefault(k, f)
            # every def, marked or not, is a helper a caller may have inlined
            helpers.setdefault(f.short, []).append((f.cls, f.code))
        for k, ln in mk:   # a marker inside a body (a switch arm) is that body's
            if k is None or k in funcs:
                continue
            for f in defs:
                if f.line < ln <= f.end_line:
                    funcs[k] = f
                    INBODY.add(k)   # the enclosing body has more code than the retail fn: no shape
                    break
        for k in want - set(funcs):
            if k is None:
                continue
            for f in defs:
                if (f.ids and norm_id(f.ids[0]) == k) or re.fullmatch(rf"x0*{k[1]:X}_\w*", f.short or "", re.I):
                    funcs[k] = f
                    break
    return funcs, helpers, texts


def expand(code, helpers, depth=2, cls=None):
    seen, todo, out = set(), [code], [code]
    for _ in range(depth):
        nxt = []
        for b in todo:
            for name in set(re.findall(r"(~?\w+)\s*\(", b)):
                if name not in helpers or name in seen:
                    continue
                cands = helpers[name]
                if len(cands) > 1:
                    cands = [c for c in cands if c[0] == cls]
                if len(cands) == 1 and cands[0][1] != code:
                    seen.add(name)
                    nxt.append(cands[0][1])
        out += nxt
        todo = nxt
    return "\n".join(out)


def strip_strings(s):
    return re.sub(r'"(?:\\.|[^"\\])*"', '""', s)


def port_compares(code):
    """(strict, nonstrict, negated strict, negated nonstrict) relational compares in the code."""
    toks = ac.drop_templates(ac.lex(strip_strings(code), 1))
    t = [x[0] for x in toks]
    s = n = ns = nn = 0
    for i, x in enumerate(t):
        if x in ("<", ">"):
            s += 1
        elif x in ("<=", ">="):
            n += 1
        else:
            continue
        # negated: the compare sits directly inside `!( ... )`
        d, j = 0, i - 1
        while j >= 0:
            if t[j] == ")":
                d += 1
            elif t[j] == "(":
                if d == 0:
                    break
                d -= 1
            elif d == 0 and t[j] in ("&&", "||", "?", ":", ",", ";", "{", "}", "="):
                j = -1
                break
            j -= 1
        if j >= 1 and t[j - 1] == "!":
            if x in ("<", ">"):
                ns += 1
            else:
                nn += 1
    return s, n, ns, nn


# ---------------------------------------------------------------- compare

def plumbing(ck, ctx, depth=0):
    """An unported module function whose calls are all rstl/string/allocation plumbing (or more
    such functions), at least one: a template instance (vector reserve, string copy)."""
    if ck in ctx["plumb"]:
        return ctx["plumb"][ck]
    ctx["plumb"][ck] = False
    m = ctx["ids"].get(ck[0])
    fn = ctx["retail"][m][1].get(ck[1]) if m else None
    ok = False
    if fn is not None and depth < 3:
        calls = [i.args[0].strip('"') for i in fn.ins if i.op == "bl" and i.args]
        ok = bool(calls)
        for c in calls:
            mm = FNID.match(c)
            if mm:
                sub = (int(mm.group(1)), int(mm.group(2), 16))
                if sub in ctx["pnames"] or not plumbing(sub, ctx, depth + 1):
                    ok = False
            elif not (PLUMB.search(c) or ac.is_runtime(c)):
                ok = False
            if not ok:
                break
    ctx["plumb"][ck] = ok
    return ok


def check(key, f, retail, ctx):
    mod, fn = retail
    calls, consts, conds, fcmps = retail_facts(mod, fn)
    code = f.code
    body = expand(code, ctx["helpers"], cls=f.cls)
    comments = "\n".join(t for _, t in f.comment_lines)
    names_text = body + "\n" + comments
    out = []
    seen = set()
    mid = key[0]
    for c in calls:
        if c in seen or ac.is_runtime(c) or c.startswith(SKIP_CALL) or "rstl" in c or "basic_string" in c:
            continue
        seen.add(c)
        m = FNID.match(c)
        if m:
            ck = (int(m.group(1)), int(m.group(2), 16))
            size = ctx["sizes"].get(ck, 0x100)
        else:
            ck = c
            dname, size = dol_callee(c, ctx["dol"])
        ctx["sites"][ck] = ctx["sites"].get(ck, 0) + 1
        if size <= SMALL:
            continue
        if m:
            off = ck[1]
            if re.search(rf"fn_{ck[0]}_0*{off:X}\b|\bx0*{off:X}_|\b0x0*{off:X}\b", names_text, re.I):
                continue
            if ck not in ctx["pnames"] and plumbing(ck, ctx) and not ctx["strict"]:
                continue    # an rstl/string/allocation template instance the port's containers are
            if ck in ctx["pnames"]:
                pn = ctx["pnames"][ck]
                cf = ctx["funcs"][ck]
                if pn.startswith(("operator", "~")) or pn == cf.cls:  # a ctor: initializer lists name the type
                    continue
                if re.search(rf"\b{re.escape(pn)}\b", body):
                    continue
                out.append(("call", f"{c} {pn} (ported) not called", ck))
            elif not (f.short.startswith("~") or f.short == f.cls) or ctx["strict"]:
                # in a ctor/dtor an unported module callee is mostly a member's ctor/dtor
                out.append(("call", f"{c} (unported) not named", ck))
        else:
            short = ac.base_name(dname)
            if short.startswith("TCastTo"):
                short = r"TCastTo(?:Const)?Ptr"
            if ac.is_runtime(dname) or dname.startswith(SKIP_CALL):
                continue
            if short in SKIP_DOL and not ctx["strict"]:
                continue
            pat = short if short.startswith("TCastTo") else DOL_ALIAS.get(short, re.escape(short))
            if not short or re.search(rf"\b{pat}\b", names_text):
                continue
            out.append(("call", f"{dname[:60]} not named", ck))
    if consts:
        if f.file not in ctx["lits"]:
            text = ac.strip_comments_keep(ctx["texts"][f.file])
            ctx["lits"][f.file] = {f32(float(x)) for x in FLOAT_LIT.findall(text)}
        lits = ctx["lits"][f.file]
        lits = lits | {f32(1.0 / x) for x in lits if x} | {float(x) for x in INT_LIT.findall(body)} | {f32(float(x)) for x in FLOAT_LIT.findall(body)}
        missing = []
        for v in consts:
            if v != v or v == 0.0 or v in MAGIC or abs(v) > 1e30:
                continue
            fv = f32(v)
            if any(abs(fv - lt) <= 1e-6 * max(1.0, abs(fv)) or abs(-fv - lt) <= 1e-6 * max(1.0, abs(fv)) for lt in lits):
                continue
            missing.append(f"{v:.9g}")
        if missing:
            out.append(("const", "retail constant(s) not in the port file: " + ", ".join(sorted(set(missing))[:6])))
    nonblank = [x for x in code.split("\n") if x.strip()]
    shape_body = code if len(nonblank) > 5 else body
    pc = len(PORT_COND.findall(strip_strings(shape_body)))
    pc_own = len(PORT_COND.findall(strip_strings(code)))   # too many: the body's own, not its helpers'
    plines = len([x for x in shape_body.split("\n") if x.strip()])
    marked_ids = [norm_id(x) for x in f.ids if norm_id(x)]
    if f.short.startswith("~") or key in INBODY or (len(marked_ids) > 1 and key != marked_ids[0]):
        pass        # also a body that carries several ids: its shape is the first one's        # member dtors inline into retail's; an arm marked inside a bigger body
    elif conds >= 4 and pc == 0 and f.short != f.cls:   # a ctor's branches are its members' ctors
        out.append(("shape", f"{conds} retail conditional branches, no port conditions"))
    elif pc_own > 2 * conds + 4:
        pc = pc_own
        out.append(("shape", f"{pc} port conditions vs {conds} retail conditional branches"))
    if len(fn.ins) >= 80 and plines <= 6 and not (f.short.startswith("~") or f.short == f.cls):
        out.append(("shape", f"{len(fn.ins)} retail instructions behind a {plines}-line port body"))
    if fcmps:
        s, n, ns, nn = port_compares(body)
        rs = [x for x in fcmps if x[1] == "strict"]
        rn = [x for x in fcmps if x[1] == "nonstrict"]
        sites = lambda xs: " ".join(f"{o:X}:{w}" for o, _, w in xs[:6])  # noqa: E731
        if len(rs) > s:
            out.append(("nan", f"{len(rs)} retail plain float branches, port has {s} `<`/`>` "
                               f"(a `!(a <= b)`/`>=` where retail has b<cond> flips the NaN arm): {sites(rs)}"))
        if len(rn) > n:
            out.append(("nan", f"{len(rn)} retail cror float compares, port has {n} `<=`/`>=` "
                               f"(a `<`/`>` or `!(a < b)` there flips the NaN arm): {sites(rn)}"))
        if nn > len(rn):
            out.append(("nan", f"port has {nn} negated `!(a <= b)` forms, retail {len(rn)} cror float "
                               f"compares (a negated <=/>= is NaN-true; retail plain b<cond> is not): {sites(rs)}"))
    return out


# ---------------------------------------------------------------- --diff

def changed_since(base, cur_funcs):
    """Marked ids whose body is new or differs from BASE's."""
    r = subprocess.run(["git", "-C", str(ROOT), "diff", "--name-only", base, "--", "src", "platform", "include"],
                       capture_output=True, text=True, check=True)
    files = [ROOT / x for x in r.stdout.split() if x.endswith((".c", ".cpp", ".h", ".hpp", ".inc"))]
    files = set(files)
    with tempfile.TemporaryDirectory(prefix="rdiff-") as td:
        old_paths, display = [], {}
        for i, p in enumerate(sorted(files)):
            r = subprocess.run(["git", "-C", str(ROOT), "show", f"{base}:{rel(p)}"], capture_output=True)
            if r.returncode:
                continue
            q = Path(td) / f"{i}_{p.name}"
            q.write_bytes(r.stdout)
            old_paths.append(q)
            display[str(q)] = p
        old, _, _ = port_functions(old_paths, display)
    norm = lambda s: re.sub(r"\s+", " ", s).strip()  # noqa: E731
    return {k for k, f in cur_funcs.items()
            if Path(f.file) in files and (k not in old or norm(old[k].code) != norm(f.code))}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--module", action="append", default=[], help="only this REL module (repeatable)")
    ap.add_argument("--fn", action="append", default=[], help="only fn_<mod>_<off> (repeatable)")
    ap.add_argument("--diff", metavar="BASE", help="only bodies new or changed since BASE")
    ap.add_argument("--kinds", help="comma list of call,const,shape,nan")
    ap.add_argument("--top", type=int, default=40, help="whole-port listing: N worst functions")
    ap.add_argument("--strict", action="store_true", help="also show calls to port-model callees")
    args = ap.parse_args()

    mods = sorted(p.name for p in rm.RELS.iterdir() if p.is_dir())
    retail, ids, sizes = load_retail(mods)
    funcs, helpers, texts = port_functions(marker_files())
    ctx = {"helpers": helpers, "texts": texts, "sizes": sizes, "dol": dol_symbols(), "lits": {}, "sites": {},
           "pnames": {k: f.short for k, f in funcs.items()}, "funcs": funcs, "strict": args.strict,
           "retail": retail, "ids": ids, "plumb": {}}
    marks = port_progress.markers()
    found_by, nobody = {}, []
    for k in sorted(marks):
        m = ids.get(k[0])
        if m is None or k[1] not in retail[m][1]:
            continue
        if k not in funcs:
            nobody.append(k)
            continue
        found_by[k] = check(k, funcs[k], (retail[m][0], retail[m][1][k[1]]), ctx)
    missed = {}
    for fs in found_by.values():
        for x in fs:
            if x[0] == "call":
                missed[x[2]] = missed.get(x[2], 0) + 1
    model = {c for c, n in missed.items() if ctx["sites"].get(c, 0) >= 3 and n * 2 >= ctx["sites"][c]}

    sel = set(found_by)
    whole = not (args.module or args.fn or args.diff)
    if args.module:
        want = {rm.module_id(m) for m in args.module}
        sel = {k for k in sel if k[0] in want}
    if args.fn:
        want = {norm_id(x) for x in args.fn}
        for x in want - set(found_by):
            print(f"{fid(*x)}: no marked port body found" if x else "bad --fn")
        sel &= want
    if args.diff:
        sel &= changed_since(args.diff, funcs)
    kinds = set(args.kinds.split(",")) if args.kinds else None
    totals, rows, hidden = {}, [], 0
    for k in sorted(sel):
        found = []
        for x in found_by[k]:
            if x[0] == "call" and x[2] in model and not args.strict:
                hidden += 1
                continue
            if not kinds or x[0] in kinds:
                found.append(x)
        for x in found:
            totals[x[0]] = totals.get(x[0], 0) + 1
        if found:
            rows.append((k, found))
    print(f"# {len(sel)} marked function(s) checked ({len(nobody)} markers without a parsed body); findings: "
          + (", ".join(f"{a} {b}" for a, b in sorted(totals.items())) or "none")
          + f"; {hidden} calls to {len(model)} port-model callees hidden (--strict shows them)")
    if whole:
        rows.sort(key=lambda r: -len(r[1]))
        rows = rows[:args.top]
    for k, found in rows:
        f = funcs[k]
        print(f"{fid(*k)} {ids[k[0]]} {f.name} ({rel(f.file)}:{f.line})")
        for x in found:
            print(f"  {x[0]:5} {x[1]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
