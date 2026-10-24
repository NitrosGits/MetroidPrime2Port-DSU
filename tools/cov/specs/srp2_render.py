"""srp2b: CScriptRiftPortal::Render (fn_64_C88) screen-space clamp arms, owned fixture calls.

OFFLINE ONLY, never executed in this form: the one srp2b launch (/tmp/opencode/srp2-b) ran the
previous version, whose absolute PIE breakpoint failed to insert (0 cases). The Site at line ~303
is now symbol+offset, checked only by a static gdb dry run and the offline selftests.

Use default force.py (FORCE = []). A window is one natural Render call, owned by guarded frame
equality + inferior/thread, armed with a FinishBreakpoint before anything is written. Right after
the corner loop (host `addss xmm3,1.0; pxor xmm0,xmm0; comiss xmm0,xmm3; jbe`, retail 0xFF8) the
projected maxY lives in xmm2; for the forcing cases only that register is written (no memory is
written at all, so nothing needs rollback; the original bits and a readback are logged). The
clamp result is checked against GXSetTexCopySrc (copy window) and PSMTX44Trans (screen offsets)
called from inside the owned frame. A Render that returns before the loop (guard arm) is
ignored, never counted. No production probes.
"""
import gdb
import hashlib
import json
import os
import struct

CLASS = 'CScriptRiftPortal'
FORCE = []
# name, forced maxY (None = natural projection), expected clamp arm
CASES = [
    ('natural-a', None, 'mid-or-natural'),
    ('y-min', -1.5, 'min'),
    ('y-max', 1.5, 'max'),
    ('natural-b', None, 'mid-or-natural'),
]
PATTERN = [('addss', 'xmm3'), ('pxor', 'xmm0,xmm0'), ('comiss', 'xmm0,xmm3'), ('jbe', None),
           ('addss', 'xmm2'), ('pxor', 'xmm3,xmm3')]
state = dict(index=0, active={}, failed=0, ignored=0, early=0)

def log(event, **values):
    with open(os.path.join(os.environ['COV_OUT'], 'srp2b.jsonl'), 'a') as f:
        f.write(json.dumps(dict(event=event, **values), sort_keys=True) + '\n')

def ev(expression):
    return gdb.parse_and_eval(expression)

def iv(expression):
    return int(ev(expression))

def check(condition, text):
    if not condition:
        raise RuntimeError(text)

def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]

def context():
    thread = gdb.selected_thread()
    check(thread is not None, 'missing thread')
    inferior = gdb.selected_inferior()
    check(inferior is not None and inferior.num is not None and thread.global_num is not None,
          'unreadable inferior/thread identity')
    return inferior.num, thread.global_num

def failure(text):
    state['failed'] += 1
    log('FAIL', reason=str(text))

def same_frame(a, b):
    check(a is not None and b is not None and a.is_valid() and b.is_valid(), 'invalid frame')
    return bool(a == b)

def clamp(low, value, high):
    return low if value < low else high if value > high else value

def expected_outputs(min_x, max_y, width, height, left, top):
    """Retail/ported arithmetic of Render lines 299-306 and the PSMTX44Trans clamps."""
    clamped_x = clamp(0.0, f32(1.0 + min_x), 2.0)
    clamped_y = clamp(0.0, f32(1.0 + max_y), 2.0)
    copy_x = int(f32(f32(0.5 * clamped_x) * width)) & ~1
    copy_y = int(f32(f32(1.0 - f32(0.5 * clamped_y)) * height)) & ~1
    offset_y = f32(f32(2.0 * f32(1.0 - f32(f32(copy_y) / height))) - 1.0)
    offset_x = f32(f32(2.0 * f32(f32(copy_x) / width)) - 1.0)
    arm = 'min' if f32(1.0 + max_y) < 0.0 else 'max' if f32(1.0 + max_y) > 2.0 else 'mid'
    return dict(copy=(copy_x + left, copy_y + top, width // 2, height // 2), arm=arm,
                trans=(clamp(-1.0, offset_x, 1.0), clamp(-1.0, offset_y, 1.0)), copy_y=copy_y)

def find_site(instructions):
    """Address of the post-loop clamp (first instruction of PATTERN) in a disassembly list."""
    hits = []
    for i in range(len(instructions) - len(PATTERN) + 1):
        ok = True
        for (mnemonic, operand), item in zip(PATTERN, instructions[i:i + len(PATTERN)]):
            text = item['asm'].split()
            if text[0] != mnemonic or (operand is not None and not text[1].startswith(operand)):
                ok = False
                break
        if ok:
            hits.append(instructions[i]['addr'])
    check(len(hits) == 1, 'exactly one post-loop clamp site, got %d' % len(hits))
    return hits[0]

def disassemble(start, count):
    """[{'addr', 'asm'}] from gdb `x/Ni` (works on the unrelocated, not yet running binary)."""
    gdb.execute('set disassembly-flavor intel', to_string=True)
    text = gdb.execute('x/%di 0x%x' % (count, start), to_string=True)
    rows = []
    for line in text.splitlines():
        head, _, asm = line.partition(':\t')
        if asm:
            rows.append(dict(addr=int(head.replace('=>', '').split()[0], 16), asm=asm.strip()))
    return rows

def viewport():
    base = int(ev('(long)&CGraphics::mViewport'))
    raw = bytes(gdb.selected_inferior().read_memory(base, 16))
    left, top, width, height = struct.unpack('<4i', raw)
    check(width > 0 and height > 0, 'viewport readable')
    return left, top, width, height

def xmm(name):
    return float(ev('$%s.v4_float[0]' % name))

def xmm_bits(name):
    return int(ev('$%s.v4_int32[0]' % name)) & 0xffffffff

class Window(gdb.FinishBreakpoint):
    def __init__(self, this, frame, ctx, case):
        self.this, self.owner, self.ctx, self.case = this, frame, ctx, case
        self.closed = False
        self.site = None
        self.copies = []
        self.trans = []
        super().__init__(frame, internal=True)  # MUST succeed before any mutation
        check(ctx not in state['active'], 'nested owned Render')
        state['active'][ctx] = self
        log('ARM', case=case[0], frame_level=frame.level(), context=ctx, this=this)

    def at_site(self):
        """In the owned frame after the corner loop: optionally force maxY (register only)."""
        name, forced, _ = self.case
        check(same_frame(gdb.newest_frame(), self.owner), 'site frame')
        check(self.site is None, 'site reached twice')
        min_x, natural = xmm('xmm3'), xmm('xmm2')
        original = xmm_bits('xmm2')
        if forced is not None:
            gdb.execute('set $xmm2.v4_float[0] = %r' % forced, to_string=True)
            check(f32(xmm('xmm2')) == f32(forced), 'forced readback mismatch')
        self.site = dict(min_x=min_x, natural_max_y=natural, max_y=xmm('xmm2'),
                         original_bits=original, readback_bits=xmm_bits('xmm2'),
                         viewport=viewport())
        log('APPLY', case=name, **self.site)

    def stop(self):
        if self.closed:
            failure('duplicate finish; no stale writes')
            return True
        good = False
        counted = True
        try:
            check(state['active'].get(self.ctx) is self, 'finish window ownership')
            if self.site is None:
                counted = False  # guard-arm early return: not evidence, not progress
                state['early'] += 1
                log('EARLY', case=self.case[0])
            else:
                s = self.site
                left, top, width, height = s['viewport']
                want = expected_outputs(s['min_x'], s['max_y'], width, height, left, top)
                check(want['arm'] == self.case[2] or self.case[1] is None,
                      'forced arm %s' % want['arm'])
                check(len(self.copies) == 1, 'one GXSetTexCopySrc, got %d' % len(self.copies))
                check(tuple(self.copies[0]) == want['copy'],
                      'copy %s != %s' % (self.copies[0], want['copy']))
                check(len(self.trans) >= 1, 'PSMTX44Trans reached')
                x, y = self.trans[0]
                check(abs(x - want['trans'][0]) < 1e-5 and abs(y - want['trans'][1]) < 1e-5,
                      'trans %s != %s' % (self.trans[0], want['trans']))
                log('RESULT', case=self.case[0], arm=want['arm'], copies=self.copies,
                    trans=self.trans, expected=want, site=s)
            good = True
        except Exception as error:
            failure(error)
        finally:
            self.closed = True
            if state['active'].get(self.ctx) is self:
                state['active'].pop(self.ctx)
        if good and counted:
            state['index'] += 1
        return not good

    def out_of_scope(self):
        if self.closed:
            failure('late unwind; no writes')
            return
        failure('Render unwind ' + self.case[0])
        self.closed = True
        if state['active'].get(self.ctx) is self:
            state['active'].pop(self.ctx)

def owned(depth):
    ctx = context()
    w = state['active'].get(ctx)
    if w is None:
        return None
    frame = gdb.newest_frame()
    for _ in range(depth):
        frame = frame.older() if frame is not None else None
    if frame is None or not same_frame(w.owner, frame):
        state['ignored'] += 1
        return None
    return w

def abort(error):
    failure(error)
    for w in list(state['active'].values()):
        w.closed = True
    state['active'].clear()
    return True

class Entry(gdb.Breakpoint):
    def stop(self):
        if state['failed']:
            return True
        if state['index'] == len(CASES):
            return False
        try:
            this = iv('$rdi')
            ctx = context()
            frame = gdb.newest_frame()
            check(frame is not None and frame.is_valid(), 'entry frame')
            check(ctx not in state['active'], 'recursive entry rejected before writes')
            dynamic = str(ev('((CEntity*)%d)' % this).dynamic_type)
            check(dynamic == 'CScriptRiftPortal *', 'concrete portal dynamic type ' + dynamic)
            Window(this, frame, ctx, CASES[state['index']])
            return False
        except Exception as error:
            return abort(error)

class Site(gdb.Breakpoint):
    def stop(self):
        try:
            w = owned(0)
            if w is None:
                return False
            w.at_site()
            return False
        except Exception as error:
            return abort(error)

class Copy(gdb.Breakpoint):
    def stop(self):
        try:
            w = owned(2)  # Render -> CopyFrameBufferRegion -> GXSetTexCopySrc
            if w is None:
                return False
            check(w.site is not None, 'copy only after the clamp site')
            w.copies.append([iv('$rdi') & 0xffff, iv('$rsi') & 0xffff, iv('$rdx') & 0xffff,
                             iv('$rcx') & 0xffff])
            return False
        except Exception as error:
            return abort(error)

class Trans(gdb.Breakpoint):
    def stop(self):
        try:
            w = owned(1)
            if w is None:
                return False
            check(w.site is not None, 'translation only after the clamp site')
            w.trans.append((xmm('xmm0'), xmm('xmm1')))
            return False
        except Exception as error:
            return abort(error)

def symbol(prefix):
    matches = []
    for line in open(os.environ['COV_NM']):
        raw, dem = line.rstrip('\n').split('\t')
        parts = raw.split()
        if len(parts) == 3 and parts[1] in 'tTW' and dem.startswith(prefix) and 'clone' not in dem:
            matches.append(parts[2])
    check(len(matches) == 1, 'one native symbol ' + prefix)
    return matches[0]

def symbol_address(name):
    for line in open(os.environ['COV_NM']):
        raw, _dem = line.rstrip('\n').split('\t')
        parts = raw.split()
        if len(parts) == 3 and parts[2] == name and parts[1] in 'tTW':
            return int(parts[0], 16)
    raise RuntimeError('no address for ' + name)

def summary(event):
    ok = state['index'] == len(CASES) and not state['active'] and not state['failed']
    log('EXIT', ok=ok, completed=state['index'], pending=len(state['active']),
        failed=state['failed'], ignored_nested=state['ignored'], early_returns=state['early'])
    if not ok:
        failure('incomplete srp2b run')

def install():
    binary = gdb.current_progspace().filename
    with open(binary, 'rb') as f:
        digest = hashlib.file_digest(f, 'sha256').hexdigest()
    log('BINARY', path=binary, sha256=digest)
    render = symbol('CScriptRiftPortal::Render(')
    start = symbol_address(render)
    instructions = disassemble(start, 700)
    site = find_site(instructions)
    log('SITE', render=start, site=site, offset=site - start)
    Entry('*' + render, internal=True)
    Site('*%s+%d' % (render, site - start), internal=True)  # PIE: relocated at run, so symbol+offset
    Copy('*' + symbol('GXSetTexCopySrc'), internal=True)
    Trans('*' + symbol('PSMTX44Trans'), internal=True)
    gdb.events.exited.connect(summary)

if os.environ.get('SRP2_SELFTEST') != '1':
    install()
