#!/usr/bin/env python3
"""Run one retail function from MP2 main.dol (G2ME01) under a PowerPC emulator: a ground-truth oracle.

Use it to get expected values for selftests (cite them as `ppc_run fn_X <inputs>`), to settle what
a body computes, or to compare a port function's answer with retail on the same inputs.

The whole DOL is loaded (text, data, rodata, zeroed bss), so callees run as retail too. Static
constructors and the OS are NOT run: globals hold their link-time values; set what the body needs
with mem:. Gekko paired-single instructions (psq_l/psq_st/ps_*) are emulated in Python (ps1 kept in
a shadow; a non-PS write to an FPR refills ps1 from ps0, as lfs/single ops do). `operator new`
/`new[]` return zeroed scratch memory and delete is a no-op. Hardware (0xCC/0xCD) is unmapped:
a body that touches it stops with the faulting address.

usage: ppc_run.py FN [ARG ...] [--dump SPEC ...] [--max N] [--calls]
  FN                     fn_X | 0xADDR | symbol
  r3=V .. r10=V          integer args; V = 0x.. | decimal | @name[+off] (a scratch block's address)
  f1=X .. f8=X           float args
  alloc:NAME=SIZE        a zeroed scratch block (MEM2), referenced as @NAME
  mem:ADDR=TYPE:v,v,..   write values; ADDR = 0x.. | @name[+off] | lbl_X[+off]; TYPE u8 u16 u32 s8 s16 s32 f32 f64 hex
  --dump ADDR:TYPE:N     print N values after the run (same ADDR/TYPE forms)
  --max N                instruction cap (default 5,000,000)
  --calls                list each function entered (slower)
Prints r3/r4 (hex and signed), f1/f2, the instruction count and the dumps.
venv: ~/.cache/mp3-orch/m2c-venv (pip install unicorn); re-execs itself under it when needed.
"""
import math
import os
import re
import struct
import sys
from pathlib import Path

VENV_PY = Path.home() / ".cache/mp3-orch/m2c-venv/bin/python"
try:
    from unicorn import Uc, UcError, UC_ARCH_PPC, UC_MODE_PPC32, UC_MODE_BIG_ENDIAN, UC_HOOK_INTR, UC_HOOK_BLOCK
    from unicorn import UC_HOOK_MEM_UNMAPPED
    import unicorn.ppc_const as P
except ImportError:
    if VENV_PY.exists() and Path(sys.executable) != VENV_PY:
        os.execv(str(VENV_PY), [str(VENV_PY), *sys.argv])
    sys.exit("unicorn missing: ~/.cache/mp3-orch/m2c-venv/bin/pip install unicorn")

ROOT = Path(__file__).resolve().parent.parent


def _find_dol():
    # orig/ is only populated in the main checkout; lane worktrees fall back to it
    for base in (ROOT, ROOT.parent / "wt-mp2-port", ROOT.parent / "MetroidPrime2Port"):
        p = base / "orig/G2ME01/sys/main.dol"
        if p.exists():
            return p
    return ROOT / "orig/G2ME01/sys/main.dol"


DOL = Path(os.environ.get("MP2_DOL") or _find_dol())
SYMBOLS = ROOT / "config/G2ME01/symbols.txt"
SYM_RE = re.compile(r"^(\S+) = \.(?:init|text):0x([0-9A-Fa-f]{8}); // type:function size:0x([0-9A-Fa-f]+)")


def load_symbols():
    """addr -> (name, size) for every function in config/G2ME01/symbols.txt."""
    out = {}
    for line in open(SYMBOLS):
        m = SYM_RE.match(line)
        if m:
            out[int(m.group(2), 16)] = (m.group(1), int(m.group(3), 16))
    return out
MEM1, MEM1_SIZE = 0x80000000, 0x01800000
MEM2, MEM2_SIZE = 0x90000000, 0x04000000
STACK_TOP = MEM2 + MEM2_SIZE - 0x1000
SENTINEL = MEM1 + MEM1_SIZE - 0x100      # return address: emulation stops here
TRAP = 0x00000000                        # illegal: raises a program interrupt we emulate
FPR = [getattr(P, f"UC_PPC_REG_FPR{i}") for i in range(32)]
GPR = [getattr(P, f"UC_PPC_REG_{i}") for i in range(32)]
TYPES = {"u8": ">B", "u16": ">H", "u32": ">I", "s8": ">b", "s16": ">h", "s32": ">i", "f32": ">f", "f64": ">d"}


def d2q(x):
    return struct.unpack(">Q", struct.pack(">d", x))[0]


def q2d(q):
    return struct.unpack(">d", struct.pack(">Q", q & 0xFFFFFFFFFFFFFFFF))[0]


def f32(x):
    """Round to single precision (inf on overflow, as the hardware does)."""
    try:
        return struct.unpack(">f", struct.pack(">f", x))[0]
    except OverflowError:
        return math.copysign(math.inf, x)


def fdiv(a, b):
    """IEEE a / b (Python raises on a zero divisor)."""
    if b == 0.0:
        if a != a or a == 0.0:
            return math.nan
        return math.copysign(math.inf, a) * math.copysign(1.0, b)
    return a / b


def frsqrt(b):
    if b != b or b < 0.0:
        return math.nan
    if b == 0.0:
        return math.copysign(math.inf, b)
    return 1.0 / math.sqrt(b)


def sext(v, bits):
    return v - (1 << bits) if v & (1 << (bits - 1)) else v


class Machine:
    def __init__(self, calls=False):
        self.uc = uc = Uc(UC_ARCH_PPC, UC_MODE_PPC32 | UC_MODE_BIG_ENDIAN)
        uc.ctl_set_cpu_model(P.UC_CPU_PPC32_750CL_V2_0)
        uc.mem_map(MEM1, MEM1_SIZE)
        uc.mem_map(MEM2, MEM2_SIZE)
        self.funcs = load_symbols()
        self.byname = {v[0]: k for k, v in self.funcs.items()}
        self.text = []
        self.load_dol()
        self.ps1 = {}          # fpr -> (ps0 bits when ps1 was set, ps1 value)
        self.gqr = [0] * 8
        self.patched = {}      # addr -> original word
        self.alloc_next = MEM2 + 0x100000
        self.blocks = {}
        self.patch_ps()
        self.hooks = {}
        for name, a in self.byname.items():
            if name.startswith(("__nw__", "__nwa__")):
                self.hook_fn(a, self.op_new)
            elif name.startswith(("__dl__", "__dla__")):
                self.hook_fn(a, lambda: None)
        self.msr = uc.reg_read(P.UC_PPC_REG_MSR) | 0x2000  # FP on
        uc.reg_write(P.UC_PPC_REG_MSR, self.msr)
        uc.hook_add(UC_HOOK_INTR, self.on_intr)
        uc.hook_add(UC_HOOK_MEM_UNMAPPED, self.on_unmapped)
        self.fault = None
        self.entered = []
        if calls:
            uc.hook_add(UC_HOOK_BLOCK, self.on_block)
        self.sda()

    # -------------------------------------------------------- setup
    def load_dol(self):
        dol = DOL.read_bytes()
        offs = struct.unpack(">18I", dol[0:0x48])
        addrs = struct.unpack(">18I", dol[0x48:0x90])
        sizes = struct.unpack(">18I", dol[0x90:0xD8])
        for i, (o, a, s) in enumerate(zip(offs, addrs, sizes)):
            if s:
                self.uc.mem_write(a, dol[o:o + s])
                if i < 7:
                    self.text.append((a, s))

    def patch_ps(self):
        for a, s in self.text:
            data = self.uc.mem_read(a, s)
            for off in range(0, s, 4):
                w = struct.unpack_from(">I", data, off)[0]
                op = w >> 26
                ps = op in (56, 57, 60, 61) or (op == 4 and w != 0)
                if op == 31 and ((w >> 1) & 0x3FF) in (339, 467):  # mfspr/mtspr GQR
                    spr = ((w >> 16) & 0x1F) | (((w >> 11) & 0x1F) << 5)
                    ps = 912 <= spr <= 919
                if ps:
                    self.patched[a + off] = w
                    self.uc.mem_write(a + off, struct.pack(">I", TRAP))

    def hook_fn(self, addr, fn):
        self.hooks[addr] = fn
        if addr not in self.patched:
            self.patched[addr] = struct.unpack(">I", self.uc.mem_read(addr, 4))[0]
        self.uc.mem_write(addr, struct.pack(">I", TRAP))

    def sda(self):
        """r2/r13 as __init_registers sets them (lis rN,hi; ori rN,rN,lo)."""
        a = self.byname.get("__init_registers")
        if a is None:
            return
        words = struct.unpack(">36I", self.uc.mem_read(a, 0x90))
        hi = {}
        for w in words:
            op, d, ra, imm = w >> 26, (w >> 21) & 0x1F, (w >> 16) & 0x1F, w & 0xFFFF
            if op == 15 and ra == 0:          # lis
                hi[d] = imm << 16
            elif op == 24 and d == ra and ra in hi and ra in (2, 13):  # ori
                self.uc.reg_write(GPR[ra], hi[ra] | imm)

    # -------------------------------------------------------- memory helpers
    def alloc(self, size, name=None):
        a = (self.alloc_next + 0x1F) & ~0x1F
        self.alloc_next = a + max(size, 4)
        if self.alloc_next >= STACK_TOP - 0x100000:
            raise RuntimeError("scratch heap exhausted")
        if name:
            self.blocks[name] = a
        return a

    def addr(self, text):
        m = re.fullmatch(r"(@\w+|lbl_[0-9A-Fa-f]{8}|fn_[0-9A-Fa-f]{8}|0x[0-9A-Fa-f]+|\d+)(?:\+(0x[0-9A-Fa-f]+|\d+))?", text)
        if not m:
            raise ValueError(f"bad address {text}")
        base, off = m.group(1), int(m.group(2) or "0", 0)
        if base.startswith("@"):
            return self.blocks[base[1:]] + off
        if base.startswith(("lbl_", "fn_")):
            return int(base.split("_")[1], 16) + off
        return int(base, 0) + off

    def write(self, addr, typ, values):
        if typ == "hex":
            self.uc.mem_write(addr, bytes.fromhex(values))
            return
        fmt = TYPES[typ]
        for i, v in enumerate(values.split(",")):
            v = v.strip()
            val = float(v) if typ.startswith("f") else self.addr(v) if v.startswith(("@", "lbl_", "fn_")) else int(v, 0)
            if not typ.startswith("f"):
                val &= (1 << (8 * struct.calcsize(fmt))) - 1
                fmt2 = fmt.upper() if fmt[1].islower() else fmt
            else:
                fmt2 = fmt
            self.uc.mem_write(addr + i * struct.calcsize(fmt), struct.pack(fmt2, val))

    def read(self, addr, typ, n):
        fmt = TYPES[typ]
        size = struct.calcsize(fmt)
        data = self.uc.mem_read(addr, size * n)
        return [struct.unpack_from(fmt, data, i * size)[0] for i in range(n)]

    # -------------------------------------------------------- FPR / paired singles
    def fget(self, r):
        return q2d(self.uc.reg_read(FPR[r]))

    def ps_get(self, r):
        bits = self.uc.reg_read(FPR[r])
        ps0 = q2d(bits)
        sh = self.ps1.get(r)
        ps1 = sh[1] if sh and sh[0] == bits else f32(ps0)
        return ps0, ps1

    def ps_set(self, r, a, b):
        bits = d2q(a)
        self.uc.reg_write(FPR[r], bits)
        self.ps1[r] = (bits, b)

    def dequant(self, raw, typ, scale):
        if typ == 0:
            return struct.unpack(">f", struct.pack(">I", raw))[0]
        sc = 2.0 ** -sext(scale, 6)
        val = {4: raw & 0xFF, 5: raw & 0xFFFF, 6: sext(raw & 0xFF, 8), 7: sext(raw & 0xFFFF, 16)}[typ]
        return val * sc

    def quant(self, x, typ, scale):
        if typ == 0:
            return struct.unpack(">I", struct.pack(">f", f32(x)))[0]
        v = x * 2.0 ** sext(scale, 6)
        lo, hi, mask = {4: (0, 255, 0xFF), 5: (0, 65535, 0xFFFF), 6: (-128, 127, 0xFF), 7: (-32768, 32767, 0xFFFF)}[typ]
        v = 0 if v != v else int(max(lo, min(hi, v)))
        return v & mask

    def psq_load(self, frD, ea, w, i):
        g = self.gqr[i]
        typ, scale = (g >> 16) & 7, (g >> 24) & 0x3F
        size = {0: 4, 4: 1, 5: 2, 6: 1, 7: 2}.get(typ, 4)
        fmt = {4: ">I", 2: ">H", 1: ">B"}[size]
        raw0 = struct.unpack(fmt, self.uc.mem_read(ea, size))[0]
        a = self.dequant(raw0, typ, scale)
        b = 1.0 if w else self.dequant(struct.unpack(fmt, self.uc.mem_read(ea + size, size))[0], typ, scale)
        self.ps_set(frD, a, b)

    def psq_store(self, frS, ea, w, i):
        g = self.gqr[i]
        typ, scale = g & 7, (g >> 8) & 0x3F
        size = {0: 4, 4: 1, 5: 2, 6: 1, 7: 2}.get(typ, 4)
        fmt = {4: ">I", 2: ">H", 1: ">B"}[size]
        a, b = self.ps_get(frS)
        self.uc.mem_write(ea, struct.pack(fmt, self.quant(a, typ, scale)))
        if not w:
            self.uc.mem_write(ea + size, struct.pack(fmt, self.quant(b, typ, scale)))

    def set_cr_field(self, field, bits):
        cr = self.uc.reg_read(P.UC_PPC_REG_CR)
        sh = 28 - 4 * field
        self.uc.reg_write(P.UC_PPC_REG_CR, (cr & ~(0xF << sh)) | (bits << sh))

    @staticmethod
    def fcmp(a, b):
        if math.isnan(a) or math.isnan(b):
            return 1
        return 8 if a < b else 4 if a > b else 2

    def emulate(self, pc, w):
        g = lambda r: self.uc.reg_read(GPR[r])
        op = w >> 26
        d, a_, b_, c_ = (w >> 21) & 0x1F, (w >> 16) & 0x1F, (w >> 11) & 0x1F, (w >> 6) & 0x1F
        if op in (56, 57, 60, 61):
            wbit, i = (w >> 15) & 1, (w >> 12) & 7
            ea = ((g(a_) if a_ or op in (57, 61) else 0) + sext(w & 0xFFF, 12)) & 0xFFFFFFFF
            (self.psq_load if op < 60 else self.psq_store)(d, ea, wbit, i)
            if op in (57, 61):
                self.uc.reg_write(GPR[a_], ea)
            return
        if op == 31:  # mtspr/mfspr GQR
            spr = ((w >> 16) & 0x1F) | (((w >> 11) & 0x1F) << 5)
            if (w >> 1) & 0x3FF == 467:
                self.gqr[spr - 912] = g(d)
            else:
                self.uc.reg_write(GPR[d], self.gqr[spr - 912])
            return
        xo10, xo5, xo6 = (w >> 1) & 0x3FF, (w >> 1) & 0x1F, (w >> 1) & 0x3F
        if xo6 in (6, 7, 38, 39):  # psq_lx/stx/lux/stux
            wbit, i = (w >> 10) & 1, (w >> 7) & 7
            ea = ((g(a_) if a_ or xo6 in (38, 39) else 0) + g(b_)) & 0xFFFFFFFF
            (self.psq_load if xo6 in (6, 38) else self.psq_store)(d, ea, wbit, i)
            if xo6 in (38, 39):
                self.uc.reg_write(GPR[a_], ea)
            return
        A, B, C = self.ps_get(a_), self.ps_get(b_), self.ps_get(c_)
        two = lambda f: self.ps_set(d, f32(f(0)), f32(f(1)))
        if xo5 in range(10, 32) and xo10 not in (40, 72, 136, 264, 528, 560, 592, 624):
            ops = {
                10: lambda: self.ps_set(d, f32(A[0] + B[1]), C[1]),            # ps_sum0
                11: lambda: self.ps_set(d, C[0], f32(A[0] + B[1])),            # ps_sum1
                12: lambda: two(lambda k: A[k] * C[0]),                        # ps_muls0
                13: lambda: two(lambda k: A[k] * C[1]),                        # ps_muls1
                14: lambda: two(lambda k: A[k] * C[0] + B[k]),                 # ps_madds0
                15: lambda: two(lambda k: A[k] * C[1] + B[k]),                 # ps_madds1
                18: lambda: two(lambda k: fdiv(A[k], B[k])),
                20: lambda: two(lambda k: A[k] - B[k]),
                21: lambda: two(lambda k: A[k] + B[k]),
                23: lambda: two(lambda k: C[k] if A[k] >= 0 else B[k]),        # ps_sel
                24: lambda: two(lambda k: fdiv(1.0, B[k])),                   # ps_res (exact, not the estimate)
                25: lambda: two(lambda k: A[k] * C[k]),
                26: lambda: two(lambda k: frsqrt(B[k])),                     # ps_rsqrte (exact)
                28: lambda: two(lambda k: A[k] * C[k] - B[k]),
                29: lambda: two(lambda k: A[k] * C[k] + B[k]),
                30: lambda: two(lambda k: -(A[k] * C[k] - B[k])),
                31: lambda: two(lambda k: -(A[k] * C[k] + B[k])),
            }
            if xo5 in ops:
                ops[xo5]()
                return
        simple = {
            40: lambda: self.ps_set(d, -B[0], -B[1]),
            72: lambda: self.ps_set(d, B[0], B[1]),
            136: lambda: self.ps_set(d, -abs(B[0]), -abs(B[1])),
            264: lambda: self.ps_set(d, abs(B[0]), abs(B[1])),
            528: lambda: self.ps_set(d, f32(A[0]), f32(B[0])),
            560: lambda: self.ps_set(d, f32(A[0]), B[1]),
            592: lambda: self.ps_set(d, A[1], f32(B[0])),
            624: lambda: self.ps_set(d, A[1], B[1]),
        }
        if xo10 in simple:
            simple[xo10]()
            return
        if xo10 in (0, 32, 64, 96):  # ps_cmpu0/o0/u1/o1
            k = 1 if xo10 >= 64 else 0
            self.set_cr_field(d >> 2, self.fcmp(A[k], B[k]))
            return
        if xo10 == 1014:  # dcbz_l
            ea = ((g(a_) if a_ else 0) + g(b_)) & ~0x1F
            self.uc.mem_write(ea, bytes(32))
            return
        raise RuntimeError(f"unemulated paired-single word 0x{w:08X} at 0x{pc:08X}")

    # -------------------------------------------------------- hooks
    def op_new(self):
        size = self.uc.reg_read(GPR[3])
        self.uc.reg_write(GPR[3], self.alloc(size))
        self.uc.reg_write(P.UC_PPC_REG_PC, self.uc.reg_read(P.UC_PPC_REG_LR))

    def on_intr(self, uc, intno, _):
        uc.reg_write(P.UC_PPC_REG_MSR, self.msr)  # exception entry clears MSR[FP]
        pc = uc.reg_read(P.UC_PPC_REG_PC) - 4    # unicorn reports the word after the trap
        if pc in self.hooks:
            self.hooks[pc]()
            if uc.reg_read(P.UC_PPC_REG_PC) == pc:
                uc.reg_write(P.UC_PPC_REG_PC, uc.reg_read(P.UC_PPC_REG_LR))
            return
        w = self.patched.get(pc)
        if w is None:
            self.fault = f"interrupt {intno} at 0x{pc:08X} (word 0x{struct.unpack('>I', uc.mem_read(pc, 4))[0]:08X})"
            uc.emu_stop()
            return
        try:
            self.emulate(pc, w)
        except Exception as e:  # noqa: BLE001
            self.fault = str(e)
            uc.emu_stop()
            return
        uc.reg_write(P.UC_PPC_REG_PC, pc + 4)

    def on_unmapped(self, uc, access, address, size, value, _):
        self.fault = f"unmapped access 0x{address:08X} (size {size}) at pc 0x{uc.reg_read(P.UC_PPC_REG_PC):08X}"
        return False

    def on_block(self, uc, address, size, _):
        if address in self.funcs:
            self.entered.append(address)

    # -------------------------------------------------------- run
    def run(self, entry, max_insns):
        uc = self.uc
        uc.reg_write(GPR[1], STACK_TOP)
        uc.reg_write(P.UC_PPC_REG_LR, SENTINEL)
        try:
            uc.emu_start(entry, SENTINEL, count=max_insns)
        except UcError as e:
            self.fault = self.fault or f"{e} at pc 0x{uc.reg_read(P.UC_PPC_REG_PC):08X}"
        pc = uc.reg_read(P.UC_PPC_REG_PC)
        if not self.fault and pc != SENTINEL:
            self.fault = f"stopped at 0x{pc:08X} (instruction cap?)"


def main():
    argv = sys.argv[1:]
    if not argv:
        sys.exit(__doc__)
    calls = "--calls" in argv
    argv = [a for a in argv if a != "--calls"]
    max_insns, dumps, rest = 5_000_000, [], []
    i = 0
    while i < len(argv):
        if argv[i] == "--max":
            max_insns = int(argv[i + 1], 0)
            i += 2
        elif argv[i] == "--dump":
            dumps.append(argv[i + 1])
            i += 2
        else:
            rest.append(argv[i])
            i += 1
    m = Machine(calls)
    fn = rest[0]
    if re.fullmatch(r"(0x)?[0-9A-Fa-f]{8}", fn):
        entry = int(fn, 16)
    elif re.fullmatch(r"fn_[0-9A-Fa-f]{8}", fn):
        entry = int(fn[3:], 16)
    elif fn in m.byname:
        entry = m.byname[fn]
    else:
        sys.exit(f"{fn}: not in symbols.txt")
    for a in rest[1:]:
        if a.startswith("alloc:"):
            name, size = a[6:].split("=")
            m.alloc(int(size, 0), name)
    for a in rest[1:]:
        if a.startswith("alloc:"):
            continue
        if a.startswith("mem:"):
            target, spec = a[4:].split("=", 1)
            typ, vals = spec.split(":", 1)
            m.write(m.addr(target), typ, vals)
            continue
        reg, val = a.split("=", 1)
        if re.fullmatch(r"r([3-9]|10)", reg):
            m.uc.reg_write(GPR[int(reg[1:])], m.addr(val) & 0xFFFFFFFF if not val.startswith("-") else int(val, 0) & 0xFFFFFFFF)
        elif re.fullmatch(r"f[1-8]", reg):
            x = float(val)
            m.uc.reg_write(FPR[int(reg[1:])], d2q(x))
        else:
            sys.exit(f"bad argument {a}")
    m.run(entry, max_insns)
    uc = m.uc
    name = m.funcs.get(entry, (f"fn_{entry:08X}",))[0]
    print(f"# ppc_run {name} @ 0x{entry:08X}" + (f"  FAULT: {m.fault}" if m.fault else "  returned"))
    for r in (3, 4):
        v = uc.reg_read(GPR[r])
        print(f"r{r} = 0x{v:08X} ({sext(v, 32)})")
    for r in (1, 2):
        x = m.fget(r)
        print(f"f{r} = {x!r} (f32 {f32(x)!r})")
    for spec in dumps:
        target, typ, n = spec.rsplit(":", 2)
        a = m.addr(target)
        vals = m.read(a, typ, int(n, 0))
        print(f"{target} ({typ}) = " + ", ".join(f"{v:.9g}" if isinstance(v, float) else f"0x{v & 0xFFFFFFFF:X}" for v in vals))
    if calls:
        seen = []
        for a in m.entered:
            if a not in seen:
                seen.append(a)
        print("entered: " + ", ".join(f"0x{a:08X} {m.funcs[a][0][:40]}" for a in seen))
    return 1 if m.fault else 0


if __name__ == "__main__":
    sys.exit(main())
