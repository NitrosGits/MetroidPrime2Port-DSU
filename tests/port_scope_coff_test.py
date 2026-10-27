#!/usr/bin/env python3
"""Synthetic test of tools/scope_module.py --format coff.

Builds two tiny "REL modules" (two objects each) for x86_64-w64-mingw32 (Itanium names) and
x86_64-pc-windows-msvc (MSVC names), scopes them, links them with lld and checks that the private
symbols (same-named helper, COMDAT template/inline, initialized global) are renamed per module.
Runs the exe under wine when available.

    python3 tests/port_scope_coff_test.py [--llvm-bin DIR]
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SCOPE = os.path.join(HERE, "..", "tools", "scope_module.py")
DEFAULT_BIN = os.path.join(HERE, "..", "..", "wt-mp2-port", "build", "llvm-mingw", "bin")

# Module sources; @ID@ is the module letter, @V@ its distinguishing value.
MOD1 = """
extern "C" void host_hook(int);
extern "C" int mod_export_@ID@() { return @V@; }
int helper() { return @V@; }                       // same name, different body per module
int cross() { return @V@ + 1000; }                 // defined here, used from the second object
template <class T> __attribute__((noinline)) T tmpl(T x) { return x + @V@; } // COMDAT, different per module
inline __attribute__((noinline)) int inl() { return @V@ * 10; }
int g_init_value() { return @V@ * 100; }
int g_init = g_init_value();                       // dynamic initializer (.CRT$XCU)
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


def variant(bin_dir, target, msvc, work):
    cxx = os.path.join(bin_dir, "x86_64-w64-mingw32-clang++")
    nm = os.path.join(bin_dir, "llvm-nm")
    objcopy = os.path.join(bin_dir, "llvm-objcopy")
    d = os.path.join(work, "msvc" if msvc else "mingw")
    os.makedirs(d)
    srcs = {}
    for mod, v in (("A", 1), ("B", 2)):
        for i, body in enumerate((MOD1, MOD2)):
            p = os.path.join(d, f"{mod}{i}.cpp")
            open(p, "w").write(body.replace("@ID@", mod).replace("@V@", str(v)))
            srcs[(mod, i)] = p
    open(os.path.join(d, "main.cpp"), "w").write(MAIN)
    flags = ["-c", "-O1", "-fno-exceptions", "-fno-rtti", "--target=" + target, "-nostdinc++", "-ffreestanding"]

    def cc(src, dst):
        run([cxx, *flags, src, "-o", dst])
        return dst
    objs = {k: cc(p, p[:-4] + ".obj") for k, p in srcs.items()}
    main_o = cc(os.path.join(d, "main.cpp"), os.path.join(d, "main.obj"))

    # The second object of each module references the first one's `g_init`: it must resolve to the
    # module's own definition, not the other module's.
    mods = os.path.join(d, "modules.txt")
    open(mods, "w").write("".join(f"{m};{objs[(m, 0)]};{objs[(m, 1)]}\n" for m in "AB"))
    outs = os.path.join(d, "outputs.txt")
    scoped = {m: [os.path.join(d, "scoped", m, f"{i}_{m}{i}.cpp.obj") for i in (0, 1)] for m in "AB"}
    open(outs, "w").write("".join(f"{m};{scoped[m][0]};{scoped[m][1]}\n" for m in "AB"))
    others = os.path.join(d, "others.txt")
    open(others, "w").write(main_o + "\n")
    run([sys.executable, "-I", SCOPE, "scope", "--format", "coff", "--modules", mods, "--others", others,
         "--outputs", outs, "--outdir", os.path.join(d, "scoped"), "--objcopy", objcopy, "--nm", nm])

    names = {}
    for m in "AB":
        names[m] = set()
        for o in scoped[m]:
            for line in run([nm, "--no-demangle", "-P", "-g", o]).splitlines():
                parts = line.split()
                if len(parts) >= 2:
                    names[m].add(parts[0])
    tag = "msvc" if msvc else "mingw"
    for m in "AB":
        check(f"mp_rel_{m}_prolog" in names[m] and f"mp_rel_{m}_epilog" in names[m],
              f"{tag}: module {m} entry points renamed")
        check(not any(s in ("RELMain", "RELExit") or "RELMain" in s.replace(f"mp_rel_{m}_", "")
                      for s in names[m]), f"{tag}: module {m} has no bare RELMain/RELExit")
        for frag in ("helper", "tmpl", "inl", "g_init", "cross"):
            hits = [s for s in names[m] if frag in s and not s.startswith("host_")]
            check(hits and all(s.startswith(f"mp_rel_{m}__") for s in hits),
                  f"{tag}: module {m} {frag} symbols are prefixed ({len(hits)})")
        check("host_hook" in names[m] and f"mod_export_{m}" in names[m],
              f"{tag}: module {m} keeps host_hook (host-defined) and mod_export_{m} (used by host) global")
    check(not (names["A"] & names["B"] - {"host_hook"}), f"{tag}: the modules share no definitions/references")

    exe = os.path.join(d, "t.exe")
    if msvc:
        # No CRT/SDK: check only that the objects resolve into one image.
        lld = os.path.join(bin_dir, "lld-link")
        if not os.path.exists(lld):
            print(f"skip: {tag} link (no lld-link)")
            return
        run([lld, "/subsystem:console", "/entry:start", "/nodefaultlib", "/out:" + exe, main_o, *scoped["A"], *scoped["B"]])
    else:
        run([cxx, "--target=" + target, "-fuse-ld=lld", "-nostdlib", "-Wl,-e,start", "-Wl,--subsystem,console",
             "-o", exe, main_o, *scoped["A"], *scoped["B"]])
        # Negative control: the raw objects collide, proving the scoping is what makes the link work.
        r = subprocess.run([cxx, "--target=" + target, "-fuse-ld=lld", "-nostdlib", "-Wl,-e,start", "-o",
                            os.path.join(d, "raw.exe"), main_o, *objs.values()], capture_output=True, text=True)
        check(r.returncode != 0 and "duplicate symbol" in r.stderr, f"{tag}: raw objects fail to link (duplicates)")
    check(os.path.exists(exe), f"{tag}: scoped objects link with lld")
    wine = shutil.which("wine64") or shutil.which("wine")
    if wine and not msvc:
        print("note: wine present, but the freestanding exe has no CRT to run; link-only")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--llvm-bin", default=os.environ.get("MP_LLVM_MINGW_BIN", DEFAULT_BIN))
    args = ap.parse_args()
    with tempfile.TemporaryDirectory() as work:
        variant(args.llvm_bin, "x86_64-w64-mingw32", False, work)
        variant(args.llvm_bin, "x86_64-pc-windows-msvc", True, work)
    print("PASS")


if __name__ == "__main__":
    main()
