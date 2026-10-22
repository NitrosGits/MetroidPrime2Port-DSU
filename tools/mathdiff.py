#!/usr/bin/env python3
"""Differential test: retail (main.dol under ppc_run's emulator) vs port on the same random inputs.

The port side is tools/mathdiff/harness.cpp (one Case per line: retail address + signature + adapter),
built with the game's own compile flags (taken from build-port-link) against src/Kyoto/Math/*.cpp,
unresolved symbols ignored. Each case runs N random input sets through both; outputs must agree to
within a relative/absolute 2e-6 (NaN == NaN; ints exact). About a fifth of the sets get one NaN,
+-inf, +-0 or denormal-range value injected, so NaN/inf arms are compared too (`nospecial` in a
signature turns that off).

usage: mathdiff.py [ADDR|#CASE ...] [--n 50] [--seed 1] [--verbose] [--rebuild] [--strict]
  no ADDR: every case in the harness. Prints one line per case: `ok n/n` or MISMATCH with the first
  failing set. Two soft classes are counted but not failed (--strict fails them; --verbose shows an
  example of each): `inf-edge` (an input is inf or >1e30) and `rounding` (within 1e-3 of the
  largest magnitude involved: fused multiply-add cancellation).
Add a case: a line in harness.cpp's kCases (see its header for the signature tokens); include the
header that declares the port function and, if its .cpp is outside Kyoto/Math, add it to EXTRA_SOURCES.
"""
import concurrent.futures
import math
import os
import random
import re
import shlex
import subprocess
import sys
from pathlib import Path

VENV_PY = Path.home() / ".cache/mp3-orch/m2c-venv/bin/python"
try:
    import unicorn  # noqa: F401
except ImportError:
    if VENV_PY.exists() and Path(sys.executable) != VENV_PY:
        os.execv(str(VENV_PY), [str(VENV_PY), *sys.argv])
    sys.exit("unicorn missing: ~/.cache/mp3-orch/m2c-venv/bin/pip install unicorn")

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import ppc_run  # noqa: E402

HARNESS = ROOT / "tools/mathdiff/harness.cpp"
OUT = Path.home() / ".cache/mp2-orch/tmp/mathdiff" / ROOT.name
EXTRA_SOURCES = ["src/MetroidPrime/CAnimData.cpp"]  # port .cpp files outside Kyoto/Math that cases need (CQuaternion::BuildInverted)
TOL = 2e-6
SPECIALS = [math.nan, math.inf, -math.inf, 0.0, -0.0, 1e-30, -1e-30, 3e38]
NINJA = ROOT.parent / "MetroidPrimePort/build/review-tools/bin/ninja"


def flags():
    """The game's compile flags for a Kyoto/Math unit (defines, includes, -include compat.h)."""
    try:
        cmd = subprocess.run([str(NINJA), "-C", str(ROOT / "build-port-link"), "-t", "commands", "metroid_prime2_port"],
                             capture_output=True, text=True).stdout
        line = next(l for l in cmd.splitlines() if "Kyoto/Math/CTransform4f.cpp" in l)
    except (StopIteration, OSError):
        sys.exit("no build-port-link compile command for Kyoto/Math (configure build-port-link first)")
    args = shlex.split(line)
    keep, i = [], 0
    while i < len(args):
        a = args[i]
        if a in ("-isystem", "-include"):
            keep += [a, args[i + 1]]
            i += 2
            continue
        if a.startswith(("-D", "-I", "-include", "-std=", "-O", "-fsigned-char")):
            keep.append(a)
        i += 1
    return keep


def build(rebuild=False):
    srcs = [HARNESS, *sorted((ROOT / "src/Kyoto/Math").glob("*.cpp")), *(ROOT / s for s in EXTRA_SOURCES)]
    deps = srcs + [ROOT / "tools/mathdiff.py"] + list((ROOT / "include/Kyoto/Math").glob("*.hpp")) + [ROOT / "platform/compat.h"]
    exe = OUT / "harness"
    if not rebuild and exe.exists() and exe.stat().st_mtime > max(p.stat().st_mtime for p in deps if p.exists()):
        return exe
    OUT.mkdir(parents=True, exist_ok=True)
    fl = flags()

    def cc(s):
        o = OUT / (s.stem + ".o")
        r = subprocess.run(["nice", "-n5", "c++", *fl, "-w", "-c", str(s), "-o", str(o)], capture_output=True, text=True)
        return s, o, r

    objs, errs = [], []
    with concurrent.futures.ThreadPoolExecutor(4) as ex:
        for s, o, r in ex.map(cc, srcs):
            if r.returncode:
                if s == HARNESS:
                    sys.exit(r.stderr[-3000:])
                errs.append(s.name)
                continue
            objs.append(str(o))
    if errs:
        print(f"# skipped (did not compile standalone): {', '.join(errs)}", file=sys.stderr)
    r = subprocess.run(["c++", "-no-pie", "-o", str(exe), *objs, "-Wl,--unresolved-symbols=ignore-all", "-lm"],
                       capture_output=True, text=True)
    if r.returncode:
        sys.exit(r.stderr[-3000:])
    return exe


def parse_sig(sig):
    """[(parts, (lo, hi, step))]: parts = reg, kind[, n[, flag]] split on ':'."""
    toks = []
    for t in sig.split():
        if t == "nospecial":
            continue
        rng = (-10.0, 10.0, 0.0)
        if "@" in t:
            t, r = t.split("@")
            v = [float(x) for x in r.split(",")]
            rng = (v[0], v[1], v[2] if len(v) > 2 else 0.0)
        toks.append((t.split(":"), rng))
    return toks


def is_buf(parts):
    return len(parts) >= 3 and parts[1] in ("in", "io", "out")


def buf_kind(parts):
    return parts[3] if len(parts) > 3 else "f"


def rand_val(rnd, lo, hi, step, kind):
    if kind == "i":
        return float(rnd.randint(int(lo), int(hi)))
    v = rnd.uniform(lo, hi)
    if step:
        v = lo + round((v - lo) / step) * step
    return v if kind == "d" else float(ppc_run.f32(v))


def rotation(rnd, n, lo, hi):
    w, x, y, z = (rnd.gauss(0, 1) for _ in range(4))
    k = math.sqrt(w * w + x * x + y * y + z * z) or 1.0
    w, x, y, z = w / k, x / k, y / k, z / k
    m = [[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
         [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
         [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]]
    out = []
    for row in m:
        out += [float(ppc_run.f32(v)) for v in row]
        if n == 12:
            out.append(float(ppc_run.f32(rnd.uniform(lo, hi))))
    return out


def gen_inputs(toks, rnd, special):
    xs, floats = [], []
    for parts, (lo, hi, step) in toks:
        if parts[0] == "ret":
            continue
        if is_buf(parts):
            if parts[1] == "out":
                continue
            kind, n = buf_kind(parts), int(parts[2])
            if kind == "rot":  # a random rotation, row-major 3x3 (9) or 3x4 with translation (12)
                vals = rotation(rnd, n, lo, hi)
                floats += range(len(xs), len(xs) + n)
                xs += vals
                continue
            vals = [rand_val(rnd, lo, hi, step, "f" if kind == "u" else kind) for _ in range(n)]
            if kind == "u":
                mag = math.sqrt(sum(v * v for v in vals)) or 1.0
                vals = [float(ppc_run.f32(v / mag)) for v in vals]
            if kind != "i":
                floats += range(len(xs), len(xs) + n)
            xs += vals
        elif parts[0].startswith("f"):
            floats.append(len(xs))
            xs.append(rand_val(rnd, lo, hi, step, "d" if parts[1:] == ["d"] else "f"))
        else:
            xs.append(rand_val(rnd, lo, hi, step, "i"))
    if special and floats and rnd.random() < 0.2:
        xs[rnd.choice(floats)] = rnd.choice(SPECIALS)
    return xs


def nouts(toks):
    return sum(int(p[2]) for p, _ in toks if is_buf(p) and p[1] in ("io", "out")) + sum(1 for p, _ in toks if p[0] == "ret")


BUF_TYPE = {"f": ("f32", 4), "u": ("f32", 4), "rot": ("f32", 4), "i": ("s32", 4), "d": ("f64", 8)}


def retail(m, addr, toks, xs):
    m.alloc_next = ppc_run.MEM2 + 0x100000
    uc, k, reads, ret = m.uc, 0, [], None
    for parts, _ in toks:
        if parts[0] == "ret":
            ret = parts[1]
            continue
        reg = parts[0]
        if is_buf(parts):
            n = int(parts[2])
            typ, size = BUF_TYPE[buf_kind(parts)]
            a = m.alloc(size * n)
            m.uc.mem_write(a, bytes(size * n))
            if parts[1] in ("in", "io"):
                vals = xs[k:k + n]
                m.write(a, typ, ",".join(repr(v) if typ != "s32" else str(int(v)) for v in vals))
                k += n
            uc.reg_write(ppc_run.GPR[int(reg[1:])], a)
            if parts[1] in ("io", "out"):
                reads.append((a, typ, n))
        elif reg.startswith("f"):
            uc.reg_write(ppc_run.FPR[int(reg[1:])], ppc_run.d2q(xs[k]))
            k += 1
        else:
            uc.reg_write(ppc_run.GPR[int(reg[1:])], int(xs[k]) & 0xFFFFFFFF)
            k += 1
    m.fault = None
    m.run(addr, 2_000_000)
    if m.fault:
        return None, m.fault
    ys = []
    for a, typ, n in reads:
        ys += [float(v) for v in m.read(a, typ, n)]
    if ret == "f":
        ys.append(m.fget(1))
    elif ret == "i":
        ys.append(float(ppc_run.sext(uc.reg_read(ppc_run.GPR[3]), 32)))
    elif ret == "b":
        ys.append(float(uc.reg_read(ppc_run.GPR[3]) & 0xFF))
    return ys, None


def run_sinits(m):
    """Retail's static initializers for the math units (statics like CQuaternion's axis refs)."""
    for a, (name, _) in sorted(m.funcs.items()):
        if name.startswith("__sinit_") and (0x802C60EC <= a < 0x802CE098 or 0x802F74C4 <= a < 0x802F8D90):
            m.fault = None
            m.run(a, 1_000_000)
            if m.fault:
                print(f"# {name}: {m.fault}", file=sys.stderr)


def same(a, b, tol=TOL, scale=1.0):
    if a != a and b != b:
        return True
    if math.isinf(a) or math.isinf(b):
        return a == b
    return abs(a - b) <= tol * max(scale, abs(a), abs(b))


# Hard mismatches that are understood and not port bugs: printed as `known`, not counted as bad.
KNOWN = {
    0x802CBCA0: "fused discriminant: retail fmadds f1,f24,f24,f3 @0x802CBD68 then fcmpo/bge vs 0.0; "
                "an exact repeated root is -3e-9 fused, 0 unfused (3 roots vs 1)",
    0x802CB918: "calls SolveCubic: same fmadds knife edge (0x802CBD68)",
}


def classify(xs, ry, py):
    """'' when equal; else 'inf-edge' (an input is inf or >1e30: retail's frsqrte-based sqrt gives
    NaN for inf, the port's sqrt gives inf; huge float->int conversions saturate differently),
    'rounding' (every output within 1e-3 of the largest input/output magnitude: cancellation with
    retail's fused multiply-adds), or 'MISMATCH'."""
    if len(py) == len(ry) and all(same(a, b) for a, b in zip(ry, py)):
        return ""
    if any(math.isinf(v) or abs(v) > 1e30 for v in xs):
        return "inf-edge"
    fin = [abs(v) for v in xs + ry + py if v == v and not math.isinf(v)]
    big = max(fin + [1.0])
    if len(py) == len(ry) and all(same(a, b, 1e-3, big) for a, b in zip(ry, py)):
        return "rounding"
    return "MISMATCH"


def main():
    args = sys.argv[1:]
    n, seed = 50, 1
    verbose, rebuild, strict = "--verbose" in args, "--rebuild" in args, "--strict" in args
    args = [a for a in args if a not in ("--verbose", "--rebuild", "--strict")]
    for flag in ("--n", "--seed"):
        if flag in args:
            i = args.index(flag)
            v = int(args[i + 1])
            n, seed = (v, seed) if flag == "--n" else (n, v)
            del args[i:i + 2]
    exe = build(rebuild)
    cases = []
    for l in subprocess.run([str(exe), "--list"], capture_output=True, text=True).stdout.splitlines():
        idx, addr_s, sig = l.split(" ", 2)
        cases.append((int(idx), int(addr_s, 16), sig))
    want_addr = {int(a, 16) for a in args if not a.startswith("#")}
    want_idx = {int(a[1:]) for a in args if a.startswith("#")}
    m = ppc_run.Machine()
    run_sinits(m)
    bad = total = 0
    for idx, addr, sig in cases:
        if (want_addr or want_idx) and addr not in want_addr and idx not in want_idx:
            continue
        total += 1
        toks = parse_sig(sig)
        rnd = random.Random(seed * 1000003 ^ addr ^ (idx << 20))
        sets = [gen_inputs(toks, rnd, "nospecial" not in sig) for _ in range(n)]
        nout = nouts(toks)
        feed = "".join(f"{idx} {len(xs)} {nout} " + " ".join(repr(v) for v in xs) + "\n" for xs in sets)
        try:
            pr = subprocess.run([str(exe)], input=feed, capture_output=True, text=True, timeout=30)
            port_out, prc = pr.stdout.splitlines(), pr.returncode
        except subprocess.TimeoutExpired:
            port_out, prc = [], "timeout"
        name = m.funcs.get(addr, (f"fn_{addr:08X}",))[0]
        ok, first, nnan, soft = 0, None, 0, {}
        for j, xs in enumerate(sets):
            if j >= len(port_out):
                first = first or f"port died (rc {prc}) on in {[f'{v:.6g}' for v in xs]}"
                break
            py = [float.fromhex(v) for v in port_out[j].split()]
            ry, fault = retail(m, addr, toks, xs)
            if fault:
                first = first or f"retail fault: {fault}"
                continue
            nnan += any(v != v for v in xs)
            c = classify(xs, ry, py)
            if not c:
                ok += 1
                continue
            msg = f"in {[f'{v:.6g}' for v in xs]}\n    retail {[f'{v:.9g}' for v in ry]}\n    port   {[f'{v:.9g}' for v in py]}"
            if c == "MISMATCH" or strict:
                first = first or msg
            else:
                soft.setdefault(c, [0, msg])[0] += 1
        hard = first is not None
        known = hard and addr in KNOWN and not strict
        bad += hard and not known
        status = f"known ({KNOWN[addr]})" if known else "MISMATCH" if hard else "ok"
        extra = "".join(f", {k} {v[0]}" for k, v in sorted(soft.items()))
        print(f"#{idx:<3} 0x{addr:08X} {name[:64]}: {status} {ok}/{len(sets)}{extra}"
              + (f" (nan sets {nnan})" if verbose else ""))
        if first and (not known or verbose):
            print("    " + first)
        if verbose:
            for k, v in sorted(soft.items()):
                print(f"    [{k}] {v[1]}")
    print(f"# {total - bad}/{total} cases agree")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
