# gdb Python side of tools/cov/run.sh: coverage of port functions without probes or a rebuild.
#
# Every spec entry becomes a breakpoint that never stops the game: on each hit it evaluates the
# entry's expressions (on the entry registers) and, for `ret` entries, the return value, and counts
# the resulting "arm key". When the game exits the counts are written as <out>/cov.json and as a
# per-arm table, <out>/cov.txt; both are also rewritten every 15 s of hits, so a killed run keeps
# its counts. See docs/LANE_BRIEF.md, "Coverage without probes".
#
# Env: COV_SPEC (the spec file), COV_NM (`nm` dump: "<addr> <type> <mangled>\t<demangled>"),
#      COV_OUT (output dir), COV_MAX (hits recorded per breakpoint, default 400; then it disables).
#
# A spec is a Python file defining
#   CLASS = 'CSandworm'          # what `$this` in an expression means: ((CLASS*)$rdi)
#   SPEC = [(label, where, [expressions], flags), ...]
#     where:  a demangled-name prefix ('CSandworm::DeleteHeldBomb(') - every matching function,
#             or 'loc:<gdb location>' ('loc:PortCSandwormBombs.cpp:256') for a line inside a body
#             (optimised code: check the line really maps to the arm; registers are not the args)
#     exprs:  gdb expressions; `$this` = the object, `$rsi`/`$rdx`/`$rcx` the next integer args
#             (a state's EStateMsg is `$rdx`), `$xmm0.v4_float[0]` the first float argument
#     flags:  'ret'  record the low byte of the return value (bools)
#             'retp' record whether the returned pointer is non-null
#             'caller=<s>' count only hits whose caller frame's name contains <s> (tail calls have
#             no frame of their own: a callee reached by `jmp` never matches)
#             'calls' also count every call/jmp site in the body by its callee, as '-> <callee>'
#             keys under the same label: an arm that ends in a call (tail calls too) shows as its
#             callee; only for a symbol `where`
#             'max=<n>' per-breakpoint cap instead of COV_MAX
import json
import os
import re
import time

import gdb

ns = {}
exec(compile(open(os.environ['COV_SPEC']).read(), os.environ['COV_SPEC'], 'exec'), ns)
CLASS = ns.get('CLASS', '')
SPEC = ns['SPEC']
MAX = int(os.environ.get('COV_MAX', '400'))
OUT = os.environ['COV_OUT']
results = {}
last_dump = [time.time()]


def record(label, key, hit=False):
    # `hit` is the entry breakpoint only: the call-site and the return-value records are arms of
    # that entry, not new hits, so the table's hit count is the number of times the body ran.
    r = results.setdefault(label, {'hits': 0, 'arms': {}})
    if hit:
        r['hits'] += 1
    k = ', '.join(str(v) for v in key) if key else '(entry)'
    r['arms'][k] = r['arms'].get(k, 0) + 1
    if time.time() - last_dump[0] > 15:  # a killed run still leaves its table
        dump(None)


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr.replace('$this', '((%s*)$rdi)' % CLASS)))
    except gdb.error as e:
        return 'err:' + str(e)[:48]


class RetBp(gdb.FinishBreakpoint):
    def __init__(self, frame, label, key, kind):
        super().__init__(frame, internal=True)
        self.label, self.key, self.kind = label, key, kind

    def stop(self):
        try:
            rax = int(gdb.parse_and_eval('$rax'))
            rv = (rax & 0xff) if self.kind == 'ret' else int(rax != 0)
        except gdb.error:
            rv = '?'
        record(self.label, self.key + ('%s=%s' % (self.kind, rv),))
        return False

    def out_of_scope(self):
        record(self.label, self.key + ('%s=lost' % self.kind,))


class EntryBp(gdb.Breakpoint):
    def __init__(self, spec, label, exprs, flags):
        super().__init__(spec, internal=False)
        self.label, self.exprs, self.n = label, exprs, 0
        fl = flags.split()
        self.ret = 'ret' if 'ret' in fl else ('retp' if 'retp' in fl else None)
        self.caller = next((f[7:] for f in fl if f.startswith('caller=')), None)
        self.max = next((int(f[4:]) for f in fl if f.startswith('max=')), MAX)

    def stop(self):
        if self.caller is not None:
            older = gdb.newest_frame().older()
            if older is None or self.caller not in (older.name() or ''):
                return False
        self.n += 1
        if self.n > self.max:
            self.enabled = False
            return False
        key = tuple(ev(e) for e in self.exprs)
        record(self.label, key, hit=True)
        if self.ret:
            try:
                RetBp(gdb.newest_frame(), self.label, key, self.ret)
            except (gdb.error, ValueError):
                record(self.label, key + ('%s=nofinish' % self.ret,))
        return False


class SiteBp(gdb.Breakpoint):
    def __init__(self, where, label, key, cap):
        super().__init__(where, internal=True)
        self.label, self.key, self.n, self.max = label, key, 0, cap

    def stop(self):
        self.n += 1
        if self.n > self.max:
            self.enabled = False
        else:
            record(self.label, (self.key,))
        return False


SITE = re.compile(r'\b(call|jmp)\s+(?:q\s+)?0x([0-9a-f]+)\s+<([^>]*)>')


def call_sites(mangled, label, cap, extent=None):
    start = int(gdb.parse_and_eval('(long)&\'%s\'' % mangled))
    block = gdb.block_for_pc(start)
    while block is not None and block.function is None:
        block = block.superblock
    if block is None:
        return
    end = block.end - 1
    # The DWARF's end of a function can reach past its own code (the .cold part and the next
    # function share the section), and a site outside the body would count another function's
    # calls: clamp the range to the symbol's own extent (the next symbol's address).
    if extent is not None:
        end = min(end, extent - 1)
    arch = gdb.selected_inferior().architecture()
    for insn in arch.disassemble(start, end):
        m = SITE.search(insn['asm'])
        if not m:
            continue
        target = int(m.group(2), 16)
        if block.start <= target < block.end:  # a branch inside the body
            continue
        name = re.sub(r'\+\d+$', '', m.group(3))
        # symbol-relative, so gdb re-resolves it once the PIE is relocated
        SiteBp('*%s+%d' % (mangled, insn['addr'] - start), label, '-> ' + name.split('(')[0], cap)


def install():
    gdb.execute('set print asm-demangle on')
    syms = []
    addrs = []
    for ln in open(os.environ['COV_NM']):
        parts = ln.rstrip('\n').split('\t')
        if len(parts) == 2:
            f = parts[0].split()
            if len(f) == 3 and f[1] in 'tTW':
                syms.append((f[2], parts[1]))
                addrs.append(int(f[0], 16))
    # the symbol table, sorted by address: a symbol's extent ends where the next one starts
    order = sorted(range(len(syms)), key=lambda i: addrs[i])

    def extent_of(mangled):
        for i in order:
            if syms[i][0] == mangled:
                return addrs[order[i + 1]] if i + 1 < len(order) else None
        return None
    for label, where, exprs, flags in SPEC:
        results[label] = {'hits': 0, 'arms': {}, 'where': where, 'exprs': exprs}
        if where.startswith('loc:'):
            EntryBp(where[4:], label, exprs, flags)
            continue
        # the .cold part of a function has the same demangled name (only the mangled one has the
        # '.cold'), so it has to be filtered on the mangled name too: its code is a separate
        # region and its call sites are not the body's.
        hits = [m for m, d in syms
                if d.startswith(where) and '.cold' not in d and 'clone' not in d
                and '.cold' not in m and 'clone' not in m]
        if not hits:
            results[label]['note'] = 'no symbol'
        for m in hits:
            bp = EntryBp('*' + m, label, exprs, flags)
            if 'calls' in flags.split():
                call_sites(m, label, bp.max, extent_of(m))


def dump(code):
    last_dump[0] = time.time()
    with open(os.path.join(OUT, 'cov.json'), 'w') as fh:
        json.dump({'exit_code': code, 'entries': results}, fh, indent=1)
    with open(os.path.join(OUT, 'cov.txt'), 'w') as fh:
        fh.write('game exit code %s\n' % (code if code is not None else 'unknown (running, killed or crashed)'))
        for label, _w, exprs, _f in SPEC:
            r = results[label]
            head = '%-40s %6d hits' % (label, r['hits'])
            if r.get('note'):
                head += '  (' + r['note'] + ')'
            if exprs:
                head += '   key: ' + ', '.join(exprs)
            fh.write(head + '\n')
            for k, n in sorted(r['arms'].items(), key=lambda kv: -kv[1]):
                fh.write('    %-60s %6d\n' % (k, n))


def on_exit(event):
    dump(getattr(event, 'exit_code', None))


gdb.events.exited.connect(on_exit)
install()
