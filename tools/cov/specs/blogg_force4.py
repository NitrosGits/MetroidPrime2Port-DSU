# Blogg (REL 7) rez6: the loader arms no natural load reaches.
#   - LoadBlogg: the first call has projectileBlurRadius's property id (0x2dd3662e) replaced by 0xfeedbeef (the
#     `default:` skip arm of the generated reader) and isMegaBlogg (the 1-byte value) set to 1, so the first CBlogg is
#     built through the mega arm of its ctor (phase list); at the return the bytes are restored and the returned
#     object's mIsMegaBlogg is cleared (the object stays a plain Blogg for the rest of the run).
#   - LoadTypedefBloggStruct: the first call has `min` (0x3e505ddb) replaced by 0xfeedbeef; bytes and field restored.
# Driver tools/drivers/blogg_load.con (07_swamp_shaft).
#   COV_GDB_SCRIPT=$PWD/tools/cov/specs/blogg_force4.py BUILD=$PWD/build-port-cov SECS=600 \
#     tools/cov/force.sh tools/cov/specs/blogg_force4.py <out> 3dfd2249,07_swamp_shaft \
#     --con tools/drivers/blogg_load.con --fast --frames 1200
import os
import struct
import gdb

CLASS = 'CBlogg'
FORCE = []
OUT = os.environ['COV_OUT']
LOG = OUT + '/blogg6l.log'

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
    """First call: every (id, value-bytes) of `patches` is written into the property list (restored at the return)."""

    def __init__(self, sym, tag, patches, typ=None, field=None, mega=False):
        super().__init__(sym, internal=True)
        self.tag, self.patches, self.typ, self.field, self.mega, self.done = tag, patches, typ, field, mega, False

    def stop(self):
        if self.done:
            return False
        self.done = True
        try:
            stream = '((CInputStream*)%d)->' % int(ev('$rsi'))
            start, data, plist, pos = props(stream)
            saved = []
            dest = None
            if self.field:
                dest = '((%s*)%d)->%s' % (self.typ, int(ev('$rdi')), self.field)
            value = None
            for pid, new in self.patches:
                off = [p for p in plist if p[1] == pid][0][0]
                if self.field and new == b'\xfe\xed\xbe\xef':
                    value = struct.unpack_from('>i', data, off + 6)[0]
                n = len(new)
                saved.append((off, n, data[off:off + n] if new == b'\xfe\xed\xbe\xef' else data[off + 6:off + 6 + n]))
                if new == b'\xfe\xed\xbe\xef':
                    gdb.selected_inferior().write_memory(start + off, new)
                else:
                    gdb.selected_inferior().write_memory(start + off + 6, new)

            def finish():
                reached = int(ev(stream + 'mPtr'))
                log('%s patched: %s consumed=%d expected=%d' %
                    (self.tag, 'PASS' if reached == start + pos else 'FAIL', reached - start, pos))
                for (pid, new), (off, n, old) in zip(self.patches, saved):
                    base = start + off if new == b'\xfe\xed\xbe\xef' else start + off + 6
                    gdb.selected_inferior().write_memory(base, old)
                if dest:
                    gdb.execute('set var %s = %r' % (dest, value))
                if self.mega:
                    obj = int(ev('$rax'))
                    log('%s returned %#x, mIsMegaBlogg=%s' % (self.tag, obj, ev('((CBlogg*)%d)->mIsMegaBlogg' % obj)))
                    gdb.execute('set var ((CBlogg*)%d)->mIsMegaBlogg = 0' % obj)
            Done(finish)
        except Exception as e:
            log('LOADER EXC %s %r' % (self.tag, e))
        return False


FEED = b'\xfe\xed\xbe\xef'
for a in addrs('LoadBlogg(CStateManager&, CInputStream&, CEntityInfo&)')[:1]:
    Loader(a, 'LoadBlogg', [(0x2dd3662e, FEED), (0x4fc9dfe4, b'\x01')], mega=True)
for a in addrs('LoadTypedefBloggStruct('):
    Loader(a, 'LoadTypedefBloggStruct', [(0x3e505ddb, FEED)], 'SLdrBloggStruct', 'min_________________________')
