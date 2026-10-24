# Rezbit (REL 53) rez6: the loader / ctor / dtor arms no natural load reaches.
#   - LoadRezbit and LoadTypedefRezbitData: the first call has one property id overwritten in the input
#     buffer by 0xfeedbeef (the `default:` skip arm of the generated reader); bytes and the skipped
#     scalar are restored at the return, the consumed length is checked.
#   - CRezbit::CRezbit: the first object is built with SLdrRezbitData.shieldExplodeEffect and
#     .derezModel invalid (the empty-optional arms of the ctor and, when it is deleted, of the dtor);
#     the data record is restored at the return.
# Driver tools/drivers/rezbit_forced6.con (`spawn REZB` without ancs: LoadRezbit's model-null return).
#   COV_GDB_SCRIPT=$PWD/tools/cov/specs/rezbit_force4.py BUILD=$PWD/build-port-cov SECS=600 \
#     tools/cov/force.sh tools/cov/specs/rezbit_force4.py <out> 1baa96c2,02_cliff_reactorcore \
#     --con tools/drivers/rezbit_forced6.con --fast --frames 1800
import os
import struct
import gdb

CLASS = 'CRezbit'
FORCE = []
OUT = os.environ['COV_OUT']
LOG = OUT + '/rez6.log'

syms = []
for ln in open(os.environ['COV_NM']):
    parts = ln.rstrip('\n').split('\t')
    if len(parts) == 2:
        f = parts[0].split()
        if len(f) == 3 and f[1] in 'tTW':
            syms.append((f[2], parts[1]))


def addrs(prefix):
    hits = [m for m, d in syms if d.startswith(prefix) and '.cold' not in d and 'clone' not in d]
    if not hits:
        raise gdb.GdbError('%s: no symbol' % prefix)
    return ['*' + h for h in sorted(set(hits))]


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def ev(expr):
    return gdb.parse_and_eval(expr)


class Done(gdb.FinishBreakpoint):
    def __init__(self, fn):
        super().__init__(gdb.newest_frame(), internal=True)
        self.fn = fn

    def stop(self):
        try:
            self.fn()
        except Exception as e:
            log('FINISH EXC %r' % (e,))
        return False

    def out_of_scope(self):
        pass


def props(stream):
    """[(offset from mPtr, id, length)] of the property list at the stream's current position."""
    start = int(ev(stream + 'mPtr'))
    end = int(ev(stream + 'mBuffer')) + int(ev(stream + 'mBufferSize'))
    data = bytes(gdb.selected_inferior().read_memory(start, end - start))
    n = int.from_bytes(data[:2], 'big')
    pos, out = 2, []
    for _ in range(n):
        pid, length = struct.unpack_from('>IH', data, pos)
        out.append((pos, pid, length))
        pos += 6 + length
    return start, data, out, pos


class Loader(gdb.Breakpoint):
    """First call of a property reader: `wanted` (a property id of its list) is replaced by an unknown id."""

    def __init__(self, sym, tag, wanted, typ=None, field=None):
        super().__init__(sym, internal=True)
        self.tag, self.wanted, self.typ, self.field, self.done = tag, wanted, typ, field, False

    def stop(self):
        if self.done:
            return False
        self.done = True
        try:
            stream = '((CInputStream*)%d)->' % int(ev('$rsi'))
            start, data, plist, pos = props(stream)
            off = [p for p in plist if p[1] == self.wanted][0][0]
            dest = None
            if self.field:
                dest = '((%s*)%d)->%s' % (self.typ, int(ev('$rdi')), self.field)
            value = struct.unpack_from('>f', data, off + 6)[0] if self.field else None
            gdb.selected_inferior().write_memory(start + off, b'\xfe\xed\xbe\xef')

            def finish():
                reached = int(ev(stream + 'mPtr'))
                log('%s unknown property: %s consumed=%d expected=%d' %
                    (self.tag, 'PASS' if reached == start + pos else 'FAIL', reached - start, pos))
                gdb.selected_inferior().write_memory(start + off, data[off:off + 4])
                if dest:
                    gdb.execute('set var %s = %r' % (dest, value))
            Done(finish)
        except Exception as e:
            log('LOADER EXC %s %r' % (self.tag, e))
        return False


for a in addrs('LoadRezbit(CStateManager&, CInputStream&, CEntityInfo&)')[:1]:
    Loader(a, 'LoadRezbit', 0x7e397fed)
for a in addrs('LoadTypedefRezbitData('):
    Loader(a, 'LoadTypedefRezbitData', 0xed69488f, 'SLdrRezbitData', 'hearingRadius')


class Ctor(gdb.Breakpoint):
    def __init__(self, sym):
        super().__init__(sym, internal=True)
        self.n = 0

    def stop(self):
        self.n += 1
        if self.n != 1:
            return False
        try:
            data = '((SLdrRezbitData*)*(unsigned long*)($rsp+24))->'
            old = [int(ev(data + f)) for f in ('shieldExplodeEffect', 'derezModel')]
            for f in ('shieldExplodeEffect', 'derezModel'):
                gdb.execute('set var %s%s = 0xffffffff' % (data, f))
            ptr = int(ev('*(unsigned long*)($rsp+24)'))

            def finish():
                for f, v in zip(('shieldExplodeEffect', 'derezModel'), old):
                    gdb.execute('set var ((SLdrRezbitData*)%d)->%s = %d' % (ptr, f, v))
                log('ctor with invalid effect/derez model done, restored %r' % (old,))
            Done(finish)
        except Exception as e:
            log('CTOR EXC %r' % (e,))
        return False


# the C1/C2 aliases: only the one that runs stops; the counter is per breakpoint
for a in addrs('CRezbit::CRezbit('):
    Ctor(a)
