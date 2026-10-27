#!/usr/bin/env python3
"""Synthetic test of tools/scope_module.py --format macho.

Builds two tiny "REL modules" (two objects each) for arm64-apple-macos12, scopes them with
llvm-objcopy/llvm-nm and checks the renamed symbols: private ones get mp_rel_<Name>__ (keeping
the Mach-O leading underscore), RELMain/RELExit become _mp_rel_<Name>_prolog/_epilog (C names, as
the generated registry declares them), and symbols the host defines or needs stay global. There is
no Mach-O linker here, so the objects are not linked. Exits 77 (ctest skip) without the tools.

    python3 tests/port_scope_macho_test.py [--llvm-bin DIR]
"""
import argparse
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SCOPE = os.path.join(HERE, "..", "tools", "scope_module.py")
DEFAULT_BIN = os.path.join(HERE, "..", "..", "wt-mp2-port", "build", "llvm-mingw", "bin")

MOD1 = """
extern "C" void host_hook(int);
extern "C" int mod_export_@ID@() { return @V@; }
int helper() { return @V@; }
int cross() { return @V@ + 1000; }
template <class T> __attribute__((noinline)) T tmpl(T x) { return x + @V@; }
inline __attribute__((noinline)) int inl() { return @V@ * 10; }
int g_init_value() { return @V@ * 100; }
int g_init = g_init_value();
void RELMain() { host_hook(helper() + tmpl<int>(1) + inl() + cross()); }
"""
MOD2 = """
extern "C" void host_hook(int);
int cross();
extern int g_init;
void RELExit() { host_hook(cross() + g_init); }
"""
MAIN = """
extern "C" {
void mp_rel_A_prolog(); void mp_rel_A_epilog(); void mp_rel_B_prolog(); void mp_rel_B_epilog();
int mod_export_A(); int mod_export_B();
int g_sink;
void host_hook(int v) { g_sink += v; }
}
extern "C" int start() {
    mp_rel_A_prolog(); mp_rel_B_prolog(); mp_rel_A_epilog(); mp_rel_B_epilog();
    return mod_export_A() + mod_export_B() == 3 ? 0 : 1;
}
"""


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if r.returncode:
        sys.exit(f"FAIL: {' '.join(cmd)}\n{r.stdout}{r.stderr}")
    return r.stdout


def check(cond, msg):
    if not cond:
        sys.exit(f"FAIL: {msg}")
    print("ok:", msg)


def symbols(nm, obj, *flags):
    out = {}
    for line in run([nm, "--no-demangle", "-P", *flags, obj]).splitlines():
        parts = line.split()
        if len(parts) >= 2:
            out[parts[0]] = parts[1]
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--llvm-bin", default=os.environ.get("MP_LLVM_MINGW_BIN", DEFAULT_BIN))
    args = ap.parse_args()
    clang, nm, objcopy = (os.path.join(args.llvm_bin, t) for t in ("clang", "llvm-nm", "llvm-objcopy"))
    if not all(os.path.exists(t) for t in (clang, nm, objcopy)):
        print(f"skip: no clang/llvm-nm/llvm-objcopy in {args.llvm_bin}")
        return 77
    with tempfile.TemporaryDirectory() as d:
        flags = ["-c", "-O1", "-fno-exceptions", "-fno-rtti", "-target", "arm64-apple-macos12",
                 "-nostdinc++", "-ffreestanding", "-x", "c++"]

        def cc(src, dst):
            run([clang, *flags, src, "-o", dst])
            return dst
        objs = {}
        for mod, v in (("A", 1), ("B", 2)):
            for i, body in enumerate((MOD1, MOD2)):
                p = os.path.join(d, f"{mod}{i}.cpp")
                open(p, "w").write(body.replace("@ID@", mod).replace("@V@", str(v)))
                objs[(mod, i)] = cc(p, p[:-4] + ".o")
        open(os.path.join(d, "main.cpp"), "w").write(MAIN)
        main_o = cc(os.path.join(d, "main.cpp"), os.path.join(d, "main.o"))

        mods = os.path.join(d, "modules.txt")
        open(mods, "w").write("".join(f"{m};{objs[(m, 0)]};{objs[(m, 1)]}\n" for m in "AB"))
        scoped = {m: [os.path.join(d, "scoped", m, f"{i}_{m}{i}.cpp.o") for i in (0, 1)] for m in "AB"}
        outs = os.path.join(d, "outputs.txt")
        open(outs, "w").write("".join(f"{m};{scoped[m][0]};{scoped[m][1]}\n" for m in "AB"))
        others = os.path.join(d, "others.txt")
        open(others, "w").write(main_o + "\n")

        # The registry reads the raw objects' entry points (underscore forms) first.
        inc = os.path.join(d, "reg.inc")
        run([sys.executable, "-I", SCOPE, "registry", "--modules", mods, "--out", inc, "--nm", nm])
        reg = open(inc).read()
        check(all(f"void mp_rel_{m}_prolog();" in reg and f"void mp_rel_{m}_epilog();" in reg for m in "AB"),
              "registry finds Mach-O RELMain/RELExit")

        run([sys.executable, "-I", SCOPE, "scope", "--format", "macho", "--modules", mods, "--others", others,
             "--outputs", outs, "--outdir", os.path.join(d, "scoped"), "--objcopy", objcopy, "--nm", nm])

        names = {}
        for m in "AB":
            names[m] = {}
            for o in scoped[m]:
                names[m].update(symbols(nm, o, "-g"))
        for m in "AB":
            check(f"_mp_rel_{m}_prolog" in names[m] and f"_mp_rel_{m}_epilog" in names[m],
                  f"module {m} entry points renamed (C names mp_rel_{m}_prolog/_epilog)")
            check(not any("RELMain" in s.replace(f"mp_rel_{m}_", "") or "RELExit" in s.replace(f"mp_rel_{m}_", "")
                          for s in names[m]), f"module {m} has no bare RELMain/RELExit")
            for frag in ("helper", "tmpl", "inl", "g_init", "cross"):
                hits = [s for s in names[m] if frag in s]
                check(hits and all(s.startswith(f"_mp_rel_{m}__") for s in hits),
                      f"module {m} {frag} symbols are prefixed, underscore kept ({len(hits)})")
            check("_host_hook" in names[m] and f"_mod_export_{m}" in names[m],
                  f"module {m} keeps _host_hook (host-defined) and _mod_export_{m} (used by host) global")
        check(not (names["A"].keys() & names["B"].keys() - {"_host_hook"}),
              "the modules share no definitions/references")
        # Defined symbols keep their section type (a rename must not change what they are).
        raw = symbols(nm, objs[("A", 0)], "-g", "--defined-only")
        new = symbols(nm, scoped["A"][0], "-g", "--defined-only")
        check(sorted(raw.values()) == sorted(new.values()), "symbol types are unchanged by the rename")
    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
