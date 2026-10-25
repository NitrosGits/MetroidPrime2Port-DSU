"""sz2: CScriptSafeZone::Think (fn_66_3A40) forced arms on natural zone calls, owned and rolled back.

Use default force.py (this file defines CLASS/FORCE only for it).  Run (see sz2_think.con):
  SZ2_SRC=$PWD/src/MetroidPrime/ScriptObjects/CScriptSafeZone.cpp tools/cov/force.sh \
    tools/cov/specs/sz2_think.py <out> 42b935e4,02_sand_sandfalls_dark --con tools/cov/specs/sz2_think.con --fast --frames 2400

At the entry of a natural Think of a real zone (empty projectile / impact lists, so no list node is
ever freed) the fields in a CASE are written (snapshotted first, readback-checked, rolled back at
the return and read back again).  The window is owned by the Think frame (frame equality, same
inferior and thread).  Every callee Think makes on the owned frame is skipped at its entry (the
return register edited, rsp popped) after its arguments are logged, so nothing destructive runs
(DeleteObjectRequest, RemoveSafeZone, SendScriptMsg(s), ...).  The log of skipped calls and the
final fields are compared with MODEL, written from the retail asm (module 66, 0x3A40), not from the
port source.  Echo emitter and game light are scratch memory (one mmap'd page), never a game object.
Line counters on Think's source lines count natural and forced hits separately.
"""
import gdb
import faulthandler
import hashlib
import json
import os
import struct
import threading
import time

if os.environ.get('SZ2_DEBUG_INFRUN'):
    gdb.execute('set debug infrun 1')  # diagnosis only: gdb's own resume/stop decisions go to its log
from fractions import Fraction

CLASS = 'CScriptSafeZone'
FORCE = []
SPACING = int(os.environ.get('SZ2_SPACING', '5'))  # eligible natural calls between two windows

D = dict(active=1, dark=1, act=0.5, aT=2.0, dT=4.0, shell=0.0, life=0.0, lt=0.0, zt=0, mob=0, dirty=0,
         obst=-1, gen=0, ci=0, pci=0, ign=0, cine=0, fog=0, emit=0, light=1, dt=0.25)
CASES = [
    ('dark-grow', dict()),
    ('dark-grow-clamp', dict(act=0.9, aT=0.1)),
    ('dark-full-shell-decay', dict(act=1.0, shell=0.5)),
    ('dark-full-shell-clamp', dict(act=1.0, shell=0.05)),
    ('dark-full-noshell', dict(act=1.0)),
    ('dark-over-full', dict(act=1.5, shell=0.5)),
    ('light-off-clears', dict(dark=0)),
    ('light-off-zero', dict(dark=0, act=0.0)),
    ('life-tick', dict(life=5.0, lt=1.0)),
    ('life-expired-active-act', dict(life=1.0, lt=2.0)),
    ('life-expired-active-zero', dict(life=1.0, lt=2.0, dark=0, act=0.0)),
    ('life-expired-inactive-act', dict(life=1.0, lt=2.0, active=0)),
    ('inactive-fade-end', dict(active=0, act=0.03, life=1.0, lt=2.0)),
    ('inactive-fade-end-echo', dict(active=0, act=0.0625, zt=2)),
    ('inactive-fade', dict(active=0, act=0.5)),
    ('inactive-zero', dict(active=0, act=0.0)),
    ('inactive-mobile-dirty', dict(active=0, act=0.0, mob=1, dirty=1, obst=3, gen=1)),
    ('moved-obst-light', dict(mob=1, dirty=1, obst=3, gen=1, light=1)),
    ('moved-plain', dict(mob=1, dirty=1)),
    ('moved-nolight-obj', dict(mob=1, dirty=1, gen=1, light=0)),
    ('moved-wrongtype', dict(mob=1, dirty=1, gen=1, light=2)),
    ('mobile-notdirty', dict(mob=1, dirty=0, obst=3, gen=1)),
    ('notmobile-dirty', dict(mob=0, dirty=1, obst=3, gen=1)),
    ('echo-zone', dict(zt=2)),
    ('hurtful-zone', dict(zt=1)),
    ('emitter-live', dict(emit=1)),
    ('emitter-zero', dict(emit=1, dark=0, act=0.0)),
    ('emitter-inactive-fade', dict(emit=1, active=0, act=0.03)),
    ('inside-enter', dict(ci=1, pci=0)),
    ('inside-leave', dict(ci=0, pci=1)),
    ('inside-enter-ign-cine', dict(ci=1, pci=0, ign=1, cine=1)),
    ('inside-enter-ign-nocine', dict(ci=1, pci=0, ign=1, cine=0)),
    ('inside-leave-ign-nocine', dict(ci=0, pci=1, ign=1, cine=0)),
    ('fog-dirty-inside', dict(ci=1, pci=1, fog=1)),
    ('fog-dirty-outside', dict(ci=0, pci=0, fog=1)),
    ('enter-blocked-fog', dict(ci=1, pci=0, ign=1, cine=1, fog=1)),
    ('enter-fog', dict(ci=1, pci=0, fog=1)),
]
state = dict(index=0, active={}, failed=0, infra=0, restores=0, ignored=0, eligible=0, skipped_nonempty=0, skipped_emitter=0,
             scratch=0, results=[])
lines = dict(natural={}, forced={})


# A stall diagnosis aid: every stop() after window TRACE_FROM writes one line (kind, thread, rsp, pc) to
# sz2.trace, and faulthandler dumps all Python threads to faulthandler.log every 45 s (it runs on its own
# C thread, so it answers while gdb is stuck in a callback).
TRACE_FROM = int(os.environ.get('SZ2_TRACE_FROM', '14'))
trace = dict(file=None, lines=0)
stall = dict(last=time.monotonic(), pid=0, want=False, fired=False, this=None, restore=None)
STALL_IDLE = float(os.environ.get('SZ2_STALL_IDLE', '40'))  # seconds without any breakpoint activity


def traced(kind):
    def wrap(fn):
        def inner(self):
            stall['last'] = time.monotonic()
            stall['pid'] = gdb.selected_inferior().pid
            try:
                if state['index'] >= TRACE_FROM and trace['lines'] < 40000:
                    if trace['file'] is None:
                        trace['file'] = open(os.path.join(os.environ['COV_OUT'], 'sz2.trace'), 'a', buffering=1)
                    trace['lines'] += 1
                    trace['file'].write('%.3f %s %s idx=%d active=%d rsp=%#x\n' % (
                        time.monotonic(), kind, getattr(self, 'kind', ''), state['index'], len(state['active']),
                        int(gdb.parse_and_eval('$rsp'))))
            except Exception:
                pass
            return fn(self)
        return inner
    return wrap


def log(event, **values):
    with open(os.path.join(os.environ['COV_OUT'], 'sz2.jsonl'), 'a') as f:
        f.write(json.dumps(dict(event=event, t=round(time.monotonic(), 3), **values), sort_keys=True) + '\n')


def ev(expression):
    return gdb.parse_and_eval(expression)


def iv(expression):
    return int(ev(expression))


def check(condition, text):
    if not condition:
        raise RuntimeError(text)


def context():
    thread = gdb.selected_thread()
    check(thread is not None, 'missing thread')
    inferior = gdb.selected_inferior()
    check(inferior is not None and inferior.num is not None and thread.global_num is not None,
          'unreadable inferior/thread identity')
    return inferior.num, thread.global_num


def failure(text, infra=False):
    state['failed'] += 1
    if infra:
        state['infra'] += 1
    log('FAIL', reason=str(text), infra=infra)


def same_frame(a, b):
    check(a is not None and b is not None and a.is_valid() and b.is_valid(), 'invalid frame')
    return bool(a == b)


def f32(x):
    return struct.unpack('<f', struct.pack('<f', x))[0]


def fused(exact):
    """one rounding of an exact Fraction to float32"""
    return f32(float(exact))


def bit_field(typ, name):
    """(byte offset, bit in byte, bit size) from DWARF, through base classes."""
    found = []
    for field in typ.strip_typedefs().fields():
        if field.name == name:
            check(field.bitpos is not None and field.bitsize > 0, 'not a bitfield ' + name)
            found.append((field.bitpos // 8, field.bitpos % 8, field.bitsize))
        elif field.is_base_class:
            try:
                o, b, s = bit_field(field.type, name)
            except LookupError:
                continue
            check(field.bitpos is not None and field.bitpos % 8 == 0, 'unsupported base layout')
            found.append((field.bitpos // 8 + o, b, s))
    if not found:
        raise LookupError(name)
    check(len(found) == 1, 'ambiguous bitfield ' + name)
    return found[0]


class Snapshot:
    def __init__(self, regions, memory):
        self.memory = memory
        self.saved = []
        seen = set()
        for label, address, size in regions:
            if (address, size) not in seen:
                self.saved.append((label, address, bytes(memory.read_memory(address, size))))
                seen.add((address, size))

    def originals(self):
        return [dict(label=l, address=a, original=b.hex()) for l, a, b in self.saved]

    def restore(self):
        errors = []
        for label, address, raw in reversed(self.saved):
            try:
                self.memory.write_memory(address, raw)
            except Exception as error:
                errors.append(label + ' write: ' + str(error))
        rows = []
        for label, address, raw in self.saved:
            try:
                actual = bytes(self.memory.read_memory(address, len(raw)))
                rows.append(dict(label=label, address=address, original=raw.hex(), readback=actual.hex()))
                if actual != raw:
                    errors.append(label + ' readback mismatch')
            except Exception as error:
                errors.append(label + ' read: ' + str(error))
        return rows, errors


# ---------------------------------------------------------------- the model (retail asm, 0x3A40)
def model(c, uid, trans, light_xform):
    """expected (final fields, ordered skipped calls) for a case; trans = zone (x, y, z); light_xform =
    the zone's CTransform4f (12 floats) and the light offset (3 floats) for the mobile-light matrix."""
    dt = f32(c['dt'])
    act, shell, lt = f32(c['act']), f32(c['shell']), f32(c['lt'])
    aT, dT, life = f32(c['aT']), f32(c['dT']), f32(c['life'])
    dirty, pci, fog = c['dirty'], c['pci'], c['fog']
    calls = [('base_think', dt)]
    accept = {}  # field -> list of acceptable float values (fused and unfused)
    changed = False
    moved = bool(c['mob'] and c['dirty'])
    if c['active']:
        if c['dark']:
            if act < 1.0:
                inv = f32(1.0 / aT)
                step = f32(inv * dt)
                cand = [f32(step + act), fused(Fraction(inv) * Fraction(dt) + Fraction(act))]
                # min_val(new, 1.0): (1 < new) ? 1 : new  (retail: fcmpo f0(1.0), f1(new); bge -> new)
                accept['act'] = [1.0 if 1.0 < v else v for v in cand]
                act = accept['act'][0]
                changed = True
            elif shell > 0.0:
                inv = f32(1.0 / aT)
                step = f32(inv * dt)
                cand = [f32(shell - step), fused(Fraction(shell) - Fraction(inv) * Fraction(dt))]
                accept['shell'] = [0.0 if v < 0.0 else v for v in cand]
                shell = accept['shell'][0]
        elif act != 0.0:
            act = 0.0
            changed = True
        if life > 0.0:
            lt = f32(lt + dt)
            accept['lt'] = [lt]
        calls += [('loopsound', dt), ('insidealpha', dt), ('flash', dt)]
        if moved:
            dirty = 0
            if c['obst'] != -1:
                calls += [('obstruction', 0), ('obstruction', 1)]
            if c['gen']:
                calls.append(('objbyid', 0x123))
                if c['light'] == 1:
                    calls.append(('cast_light', 1))
                    m = list(light_xform[0])
                    off = light_xform[1]
                    m[3], m[7], m[11] = f32(m[3] + off[0]), f32(m[7] + off[1]), f32(m[11] + off[2])
                    calls.append(('set_transform', tuple(m)))
                else:
                    calls.append(('cast_light', 0 if c['light'] == 2 else None))
        if c['zt'] == 2:
            calls.append(('inform', tuple(trans), 6))
    elif act > 0.0:
        inv = f32(1.0 / dT)
        step = f32(inv * dt)
        cand = [f32(act - step), fused(Fraction(act) - Fraction(inv) * Fraction(dt))]
        accept['act'] = cand
        act = cand[0]
        if act <= 0.0:
            accept['act'] = [0.0]
            act = 0.0
            calls += [('set_zone_type', 0), ('lowpass', 0)]
        changed = True
    if changed:
        calls.append(('update_manager',))
    if c['emit']:
        if act == 0.0:
            calls.append(('emitter_off',))
        else:
            calls.append(('update_emitter', dt))
    if life > 0.0 and lt > life:
        if c['active']:
            calls.append(('send_deactivate',))
        if act == 0.0:
            calls += [('remove_zone',), ('delete_object',)]
    if pci != c['ci']:
        if (not c['ign']) or (not c['cine']):
            if c['ign']:
                calls.append(('cine_query', 0))
            calls += [('send_msgs', 'max' if c['ci'] else 'zero'), ('fog', 'in' if c['ci'] else 'out'),
                      ('inside_filter',), ('lowpass', c['ci'])]
            fog = 0
        elif c['ign']:
            calls.append(('cine_query', 1))
        pci = c['ci']
    if fog:
        calls += [('fog', 'in' if c['ci'] else 'out'), ('lowpass', c['ci'])]
        fog = 0
    final = dict(act=act, shell=shell, lt=lt, dirty=dirty, pci=pci, fog=fog)
    return final, calls, accept


# ---------------------------------------------------------------- the window
def zone_expr(this, member):
    return '((CScriptSafeZone*)%d)->%s' % (this, member)


def write_float(mem, this, member, value):
    address = int(ev('&' + zone_expr(this, member)))
    mem.write_memory(address, struct.pack('<f', value))
    check(float(ev(zone_expr(this, member))) == f32(value), 'forced readback ' + member)


def write_int(mem, this, member, value, size=4, signed=True):
    address = int(ev('&' + zone_expr(this, member)))
    mem.write_memory(address, int(value).to_bytes(size, 'little', signed=signed))
    check(iv(zone_expr(this, member)) == value, 'forced readback ' + member)


def write_bit(mem, base, typ, owner_expr, name, value):
    byte, bit, size = bit_field(typ, name)
    address = base + byte
    cur = bytearray(mem.read_memory(address, 1))
    cur[0] = (cur[0] & ~(1 << bit) & 0xFF) | ((1 if value else 0) << bit)
    mem.write_memory(address, bytes(cur))
    check(iv(owner_expr + name) == (1 if value else 0), 'forced readback ' + name)


def write_dt(value):
    """Think's dt arrives in xmm0 (SysV, float): write the case dt at the entry, read it back."""
    before = float(ev('$xmm0.v4_float[0]'))
    gdb.execute('set var $xmm0.v4_float[0] = %r' % f32(value))
    after = float(ev('$xmm0.v4_float[0]'))
    check(after == f32(value), 'forced readback dt %r != %r' % (after, f32(value)))
    return before, after


def read_bit(base, typ, name):
    byte, bit, size = bit_field(typ, name)
    return (bytes(gdb.selected_inferior().read_memory(base + byte, 1))[0] >> bit) & 1


class Window(gdb.FinishBreakpoint):
    def __init__(self, this, mgr, case, ctx, frame, snapshot, extra):
        self.this, self.mgr, self.case, self.ctx = this, mgr, case, ctx
        self.owner, self.snapshot, self.extra = frame, snapshot, extra
        self.closed = False
        self.calls = []
        self.armed_writes = False
        super().__init__(frame, internal=True)  # MUST succeed before any mutation
        check(ctx not in state['active'], 'nested owned Think')
        state['active'][ctx] = self
        self.dt_before = self.dt_written = None
        log('ARM', case=case[0], context=ctx, this=this, originals=snapshot.originals())

    def restore(self):
        check(context() == self.ctx, 'restore context mismatch: no unsafe writes')
        check(not self.closed and state['active'].get(self.ctx) is self, 'restore ownership mismatch')
        rows, errors = self.snapshot.restore()
        log('RESTORE', case=self.case[0], fields=rows, errors=errors)
        stall['this'], stall['restore'] = self.this, (self.case[0], rows, errors)
        state['restores'] += 1
        check(not errors, 'restore failed ' + str(errors))

    def apply(self):
        c = self.case[1]
        mem = gdb.selected_inferior()
        T = self.this
        ztype = gdb.lookup_type('CScriptSafeZone')
        mtype = gdb.lookup_type('CStateManager')
        etype = gdb.lookup_type('CEchoEmitter')
        try:
            self.armed_writes = True  # execute can change memory and then raise
            for member, key in (('mActivation', 'act'), ('mActivationTime', 'aT'), ('mDeactivationTime', 'dT'),
                                ('mShellPulse', 'shell'), ('mLifetime', 'life'), ('mLifeTimer', 'lt')):
                write_float(mem, T, member, c[key])
            write_int(mem, T, 'mZoneType', c['zt'], 4, False)
            write_int(mem, T, 'mObstructionType', c['obst'])
            write_int(mem, T, 'mLightId.value', self.extra['light_id'], 2, False)
            zx = zone_expr(T, '')
            for name, key in (('mCameraInside', 'ci'), ('mPrevCameraInside', 'pci'), ('mIgnoreCinematicCamera', 'ign'),
                              ('mFogDirty', 'fog'), ('mMobile', 'mob'), ('mGenerateMobileLight', 'gen')):
                write_bit(mem, T, ztype, zx, name, c[key])
            write_bit(mem, T, ztype, zx, 'mActive', c['active'])
            write_bit(mem, T, ztype, zx, 'mTransformDirty', c['dirty'])
            write_bit(mem, self.mgr, mtype, '((CStateManager*)%d)->' % self.mgr, 'mIsDarkWorld', c['dark'])
            if c['emit']:
                page = state['scratch']
                mem.write_memory(page, bytes(512))
                write_bit(mem, page, etype, '((CEchoEmitter*)%d)->' % page, 'mActive', 1)
                ptr = int(ev('&' + zone_expr(T, 'mEchoEmitter.mPtr')))
                mem.write_memory(ptr, page.to_bytes(8, 'little'))
                check(iv(zone_expr(T, 'mEchoEmitter.mPtr')) == page, 'forced readback emitter')
            else:
                ptr = int(ev('&' + zone_expr(T, 'mEchoEmitter.mPtr')))
                mem.write_memory(ptr, bytes(8))  # natural zones all own an emitter: hide it for this window
                check(iv(zone_expr(T, 'mEchoEmitter.mPtr')) == 0, 'forced readback no emitter')
            self.dt_before, self.dt_written = write_dt(c['dt'])
        except Exception:
            self.restore()
            raise

    def judge(self):
        c = self.case[1]
        T = self.this
        mem = gdb.selected_inferior()
        final, calls, accept = model(c, self.extra['uid'], self.extra['trans'], self.extra['xform'])
        problems = []
        got = dict(act=float(ev(zone_expr(T, 'mActivation'))), shell=float(ev(zone_expr(T, 'mShellPulse'))),
                   lt=float(ev(zone_expr(T, 'mLifeTimer'))), dirty=iv(zone_expr(T, 'mTransformDirty')),
                   pci=iv(zone_expr(T, 'mPrevCameraInside')), fog=iv(zone_expr(T, 'mFogDirty')))
        for key, value in got.items():
            ok = got[key] in accept[key] if key in accept else got[key] == final[key]
            if not ok:
                problems.append('%s got %r expected %r' % (key, got[key], accept.get(key, final[key])))
        if c['emit']:
            bit = read_bit(state['scratch'], gdb.lookup_type('CEchoEmitter'), 'mActive')
            want = 0 if any(x[0] == 'emitter_off' for x in calls) else 1
            if bit != want:
                problems.append('emitter bit %d expected %d' % (bit, want))
        if self.calls != calls:
            # the matrix floats and the dt/act values compare exactly; show both sequences
            problems.append('calls got %r expected %r' % (self.calls, calls))
        return got, calls, problems

    @traced('finish')
    def stop(self):
        if self.closed:
            failure('duplicate finish; no stale writes', True)
            return True
        good = False
        try:
            check(state['active'].get(self.ctx) is self, 'finish window ownership')
            got, expected, problems = self.judge()
            log('RESULT', case=self.case[0], fields=got, calls=self.calls, expected=expected,
                problems=problems, dt_natural=self.dt_before, dt_written=self.dt_written)
            state['results'].append((self.case[0], not problems))
            if problems:
                failure('%s: %s' % (self.case[0], '; '.join(problems)))
            good = True
        except Exception as error:
            failure(error, True)
        finally:
            try:
                self.restore()
            except Exception as error:
                failure(error, True)
                good = False
            self.closed = True
            self.retire()
            if state['active'].get(self.ctx) is self:
                state['active'].pop(self.ctx)
        if good:
            state['index'] += 1
        return not good

    def retire(self):
        """a finish breakpoint whose stop() returned False stays armed at the caller's return address:
        disable it so the next Think returns do not re-enter it"""
        try:
            self.enabled = False
        except Exception as error:
            log('RETIRE_FAIL', case=self.case[0], error=str(error))

    def out_of_scope(self):
        if self.closed:
            failure('late unwind; no writes', True)
            return
        failure('Think unwind ' + self.case[0], True)
        try:
            self.restore()
        except Exception as error:
            failure(error, True)
        self.closed = True
        if state['active'].get(self.ctx) is self:
            state['active'].pop(self.ctx)


def owned():
    ctx = context()
    w = state['active'].get(ctx)
    if w is None:
        return None
    caller = gdb.newest_frame().older()
    if not same_frame(w.owner, caller):
        state['ignored'] += 1
        return None
    return w


def abort(error):
    failure(error, True)
    for w in list(state['active'].values()):
        try:
            w.restore()
        except Exception as restore_error:
            failure(restore_error, True)
    return True


def make_scratch():
    """one page for the fake echo emitter (offset 0) and the fake light (offset 1024): the unused low end
    of another thread's mapped stack, zero-filled and far below its stack pointer.  Not an inferior call:
    one made inside a breakpoint's stop() leaves every other thread stopped (the Aurora FIFO worker
    starves, sz2e)."""
    if state['scratch']:
        return
    inferior = gdb.selected_inferior()
    maps = []
    with open('/proc/%d/maps' % inferior.pid) as f:
        for line in f:
            fields = line.split()
            lo, hi = (int(x, 16) for x in fields[0].split('-'))
            if fields[1].startswith('rw') and len(fields) < 6:
                maps.append((lo, hi))
    page = 0
    # other threads are still running inside stop(): gdb cannot read their registers, but a thread
    # sleeping in a syscall has its stack pointer in /proc/<pid>/task/<tid>/syscall ("nr args... sp pc")
    for thread in inferior.threads():
        if not (thread.name or '').startswith(('MusyX', 'SDLAudio')):
            continue
        try:
            with open('/proc/%d/task/%d/syscall' % (inferior.pid, thread.ptid[1])) as f:
                fields = f.read().split()
            sp = int(fields[-2], 16)
        except (OSError, ValueError, IndexError):
            continue
        cand = (sp - 4 * 1024 * 1024) & ~4095  # default 8 MiB thread stack, sp near its top
        if any(lo <= cand and cand + 4096 <= hi for lo, hi in maps):
            page = cand
            break
    check(page != 0, 'scratch page (idle thread stack)')
    state['scratch'] = page
    log('SCRATCH', page=page)


class Entry(gdb.Breakpoint):
    @traced('entry')
    def stop(self):
        if state['infra']:
            return True
        try:
            if state['index'] >= len(CASES):
                return False
            this, mgr = iv('$rdi'), iv('$rsi')
            ctx = context()
            frame = gdb.newest_frame()
            check(frame is not None and frame.is_valid(), 'entry frame')
            if ctx in state['active']:
                return False
            dynamic = str(ev('((CEntity*)%d)' % this).dynamic_type)
            if dynamic != 'CScriptSafeZone *':
                return False
            if iv(zone_expr(this, 'mProjectiles.mCount')) != 0 or iv(zone_expr(this, 'mImpactGens.mCount')) != 0:
                state['skipped_nonempty'] += 1
                return False
            if iv(zone_expr(this, 'mEchoEmitter.mPtr')) != 0:
                state['skipped_emitter'] += 1  # counted only: the natural emitter is never written, the
                # pointer is replaced (fake page or null) for the window and restored from the snapshot
            state['eligible'] += 1
            if state['eligible'] <= 400:
                log('ELIGIBLE', n=state['eligible'], this=this, ctx=ctx, index=state['index'])
            if state['eligible'] % SPACING != 1:
                return False
            make_scratch()
            ztype = gdb.lookup_type('CScriptSafeZone')
            mtype = gdb.lookup_type('CStateManager')
            etype = gdb.lookup_type('CEchoEmitter')
            case = CASES[state['index']]
            c = dict(D)
            c.update(case[1])
            case = (case[0], c)
            if state['index'] == 0:
                log('TARGET', this=this, dynamic_type=dynamic, uid=iv(zone_expr(this, 'mUniqueId.value')))
            mem = gdb.selected_inferior()
            regions = []
            for member in ('mActivation', 'mActivationTime', 'mDeactivationTime', 'mShellPulse', 'mLifetime',
                           'mLifeTimer', 'mZoneType', 'mObstructionType', 'mLightId.value', 'mEchoEmitter.mPtr'):
                value = ev(zone_expr(this, member))
                regions.append((member, int(value.address), value.type.sizeof))
            for name in ('mCameraInside', 'mPrevCameraInside', 'mIgnoreCinematicCamera', 'mFogDirty', 'mMobile',
                         'mGenerateMobileLight', 'mActive', 'mTransformDirty'):
                byte, bit, size = bit_field(ztype, name)
                regions.append((name + ' storage', this + byte, 1))
            byte, bit, size = bit_field(mtype, 'mIsDarkWorld')
            regions.append(('mIsDarkWorld storage', mgr + byte, 1))
            if c['emit']:
                regions.append(('scratch page', state['scratch'], 1024))
            xf = [float(ev(zone_expr(this, 'mTransform.m%d%d' % (r, k)))) for r in range(3) for k in range(4)]
            off = [float(ev(zone_expr(this, 'mMobileLightOffset.m' + a))) for a in 'XYZ']
            uid = iv(zone_expr(this, 'mUniqueId.value'))
            trans = (xf[3], xf[7], xf[11])
            snap = Snapshot(regions, mem)
            extra = dict(uid=uid, trans=trans, xform=(xf, off), light_id=0x123)
            w = Window(this, mgr, case, ctx, frame, snap, extra)
            w.apply()
            return False
        except Exception as error:
            return abort(error)


# ---------------------------------------------------------------- the skipped callees
def skip(ret=None):
    gdb.execute('set $rip = *(unsigned long*)$rsp')
    gdb.execute('set $rsp = $rsp + 8')
    if ret is not None:
        gdb.execute('set $rax = %d' % ret)


def reg(name):
    return iv('$' + name)


class Mock(gdb.Breakpoint):
    def __init__(self, symbol, kind):
        super().__init__('*' + symbol, internal=True)
        self.kind = kind

    @traced('mock')
    def stop(self):
        try:
            w = owned()
            if w is None:
                return False
            c, T, mgr = w.case[1], w.this, w.mgr
            k = self.kind
            rdi, rsi, rdx, rcx, r8 = reg('rdi'), reg('rsi'), reg('rdx'), reg('rcx'), reg('r8')
            xmm0 = float(ev('$xmm0.v4_float[0]'))
            ret = None
            # an argument that does not match the expected owner is recorded as ('BAD', ...) so the
            # judge reports it; the call is still skipped
            if k == 'base_think':
                rec = ('base_think', xmm0) if rdi == T and rsi == mgr else ('BAD', k, rdi, rsi)
            elif k in ('loopsound', 'insidealpha', 'flash'):
                rec = (k, xmm0) if rdi == T and rsi == mgr else ('BAD', k, rdi, rsi)
            elif k == 'obstruction':
                rec = ('obstruction', rdx & 1) if rdi == T and rsi == mgr else ('BAD', k, rdi, rsi)
            elif k == 'objbyid':
                rec = ('objbyid', rsi & 0xFFFF) if rdi == mgr else ('BAD', k, rdi, rsi)
                ret = 0 if c['light'] == 0 else state['scratch'] + 1024
            elif k == 'cast_light':
                fake = state['scratch'] + 1024
                if c['light'] == 1:
                    rec = ('cast_light', 1) if rdi == fake else ('BAD', k, rdi)
                    ret = fake
                else:
                    rec = ('cast_light', 0 if c['light'] == 2 else None)
                    ret = 0
                    if c['light'] == 0 and rdi != 0:
                        rec = ('BAD', k, rdi)
            elif k == 'set_transform':
                fake = state['scratch'] + 1024
                m = tuple(float(ev('((CTransform4f*)%d)->m%d%d' % (rsi, r, q))) for r in range(3) for q in range(4))
                rec = ('set_transform', m) if rdi == fake else ('BAD', k, rdi)
            elif k == 'inform':
                v = tuple(float(ev('((CVector3f*)%d)->m%s' % (rsi, a))) for a in 'XYZ')
                rec = ('inform', v, rdx & 0xFFFFFFFF) if rdi == mgr else ('BAD', k, rdi)
            elif k == 'set_zone_type':
                rec = ('set_zone_type', rsi & 0xFFFFFFFF) if rdi == T else ('BAD', k, rdi)
            elif k == 'lowpass':
                rec = ('lowpass', rsi & 1) if rdi == T else ('BAD', k, rdi)
            elif k == 'update_manager':
                rec = ('update_manager',) if rdi == T and rsi == mgr else ('BAD', k, rdi, rsi)
            elif k == 'update_emitter':
                rec = ('update_emitter', xmm0) if rdi == T and rsi == mgr else ('BAD', k, rdi, rsi)
            elif k == 'cine_query':
                rec = ('cine_query', c['cine']) if rdi == iv('((CStateManager*)%d)->mCameraManagers[0]' % mgr) and rdi else ('BAD', k, rdi)
                ret = c['cine']
            elif k == 'send_msgs':
                invalid = iv('kInvalidUniqueId.value')
                want = iv('(int)kSS_MaxReached') if c['ci'] else iv('(int)kSS_Zero')
                ok = rdi == T and (rsi & 0xFFFFFFFF) == (want & 0xFFFFFFFF) and rdx == mgr and (rcx & 0xFFFF) == invalid \
                    and (r8 & 0xFFFFFFFF) == (iv('(int)kSM_None') & 0xFFFFFFFF)
                rec = ('send_msgs', 'max' if c['ci'] else 'zero') if ok else ('BAD', k, rdi, rsi, rdx, rcx, r8)
            elif k == 'fog':
                inside = int(ev('&' + zone_expr(T, 'mInsideFog')))
                outside = int(ev('&' + zone_expr(T, 'mOutsideFog')))
                ok = rdi == T and rsi == mgr and rdx in (inside, outside)
                rec = ('fog', 'in' if rdx == inside else 'out') if ok else ('BAD', k, rdi, rsi, rdx)
            elif k == 'inside_filter':
                rec = ('inside_filter',) if rdi == T and rsi == mgr else ('BAD', k, rdi, rsi)
            elif k == 'send_msg':
                invalid = iv('kInvalidUniqueId.value')
                ok = rdi == mgr and rsi == T and (rdx & 0xFFFF) == w.extra['uid'] and (rcx & 0xFFFFFFFF) == (iv('(int)kSM_Deactivate') & 0xFFFFFFFF) \
                    and (r8 & 0xFFFF) == invalid
                rec = ('send_deactivate',) if ok else ('BAD', k, rdi, rsi, rdx, rcx, r8)
            elif k == 'remove_zone':
                ok = rdi == iv('((CStateManager*)%d)->mSafeZoneManager' % mgr) and \
                    int.from_bytes(gdb.selected_inferior().read_memory(rsi, 2), 'little') == w.extra['uid']
                rec = ('remove_zone',) if ok else ('BAD', k, rdi, rsi)
            elif k == 'delete_object':
                rec = ('delete_object',) if rdi == mgr and (rsi & 0xFFFF) == w.extra['uid'] else ('BAD', k, rdi, rsi)
            else:
                raise RuntimeError('mock kind ' + k)
            # the call that reads the emitter-off bit is inline; nothing to skip for it
            w.calls.append(rec)
            skip(ret)
            return False
        except Exception as error:
            return abort(error)


def symbol(prefix, exact=False):
    matches = []
    for line in open(os.environ['COV_NM']):
        raw, dem = line.rstrip('\n').split('\t')
        parts = raw.split()
        if len(parts) == 3 and parts[1] in 'tTW' and (dem == prefix if exact else dem.startswith(prefix)) \
                and 'clone' not in dem and '.cold' not in dem:
            matches.append(parts[2])
    check(len(matches) == 1, 'one native symbol %s (%d)' % (prefix, len(matches)))
    return matches[0]


def count_lines(think):
    src = os.environ.get('SZ2_SRC')
    if not src:
        return
    text = open(src).read().split('\n')
    first = [i + 1 for i, l in enumerate(text) if l.startswith('void CScriptSafeZone::Think(')][0]
    last = [i + 1 for i, l in enumerate(text) if i + 1 > first and l == '}'][0]
    seen = set()
    start = int(gdb.parse_and_eval('(unsigned long)&' + think))  # unrelocated: the breakpoints are symbol+offset
    for n in range(first, last + 1):
        try:
            _, sals = gdb.decode_line('CScriptSafeZone.cpp:%d' % n)
        except gdb.error:
            continue
        for s in sals or []:
            if s.line == n and s.pc not in seen:
                seen.add(s.pc)
                LineCount('*%s+%d' % (think, s.pc - start), n)
    log('LINES', first=first, last=last, resolved=len(seen))


class LineCount(gdb.Breakpoint):
    def __init__(self, spec, line):
        super().__init__(spec, internal=True)
        self.line = line

    @traced('line')
    def stop(self):
        try:
            w = state['active'].get(context())
            forced = w is not None and same_frame(gdb.newest_frame(), w.owner)
            table = lines['forced' if forced else 'natural']
            table[self.line] = table.get(self.line, 0) + 1
        except Exception:
            pass
        return False


def summary(event):
    faulthandler.cancel_dump_traceback_later()
    ok = state['index'] == len(CASES) and not state['active'] and not state['failed']
    log('EXIT', ok=ok, completed=state['index'], pending=len(state['active']), failed=state['failed'],
        restores=state['restores'], ignored_nested=state['ignored'], eligible=state['eligible'],
        skipped_nonempty=state['skipped_nonempty'], skipped_emitter=state['skipped_emitter'], results=state['results'])
    log('LINE_HITS', natural=lines['natural'], forced=lines['forced'])
    if not ok:
        failure('incomplete sz2 run')


# the Aurora FIFO and draw-sync state a render-side spin (GPUMemory::EnsureAllocation) waits on
FIFO_STATE = ['aurora::gx::fifo::detail::sInDisplayList',
              'aurora::gx::fifo::detail::sBufferSize',
              'aurora::gx::fifo::(anonymous namespace)::sFrameActive',
              'aurora::gx::fifo::(anonymous namespace)::sPublished',
              'aurora::gx::fifo::(anonymous namespace)::sProcessed',
              'aurora::gx::fifo::(anonymous namespace)::sStreamBase',
              'aurora::gx::(anonymous namespace)::sDrawSyncToken',
              'aurora::gx::fifo::(anonymous namespace)::sWorkerWake']


def stall_dump(event):
    """gdb-side stall report: runs in the stop handler after the watchdog's `interrupt`"""
    if not stall['want']:
        return
    stall['want'] = False
    out = []
    def run(cmd, limit=60000):
        try:
            out.append('### ' + cmd + '\n' + gdb.execute(cmd, to_string=True)[:limit])
        except Exception as error:
            out.append('### %s FAILED %s' % (cmd, error))
    run('thread apply all bt 30')
    run('info registers rip rsp')
    run('x/6i $pc')
    for thread in gdb.selected_inferior().threads():  # the FIFO worker's loop locals (event, processed, published)
        if (thread.name or '').startswith('Aurora FIFO'):
            thread.switch()
            run('frame function aurora::gx::fifo::(anonymous namespace)::worker_main')
            run('info locals')
    pid = gdb.selected_inferior().pid  # per-task scheduler view: is the worker runnable, did it get CPU
    for task in sorted(os.listdir('/proc/%d/task' % pid)):
        try:
            base = '/proc/%d/task/%s/' % (pid, task)
            out.append('### task %s %s | wchan %s | schedstat %s' % (task, open(base + 'stat').read().split(')')[1].split()[0],
                       open(base + 'wchan').read(), open(base + 'schedstat').read().strip()))
        except Exception as error:
            out.append('### task %s FAILED %s' % (task, error))
    for name in FIFO_STATE:
        run("print '%s'" % name)
    this = stall['this']
    out.append('### last restore ' + json.dumps(stall['restore'], default=str))
    out.append('### state index=%d eligible=%d restores=%d active=%d' % (
        state['index'], state['eligible'], state['restores'], len(state['active'])))
    if this:
        run('print *(CScriptSafeZone*)%d' % this, 20000)
        run('print ((CScriptSafeZone*)%d)->mProjectiles' % this)
        run('print ((CScriptSafeZone*)%d)->mImpactGens' % this)
    with open(os.path.join(os.environ['COV_OUT'], 'stall.txt'), 'w') as f:
        f.write('\n'.join(out))
    log('STALL', index=state['index'], eligible=state['eligible'], file='stall.txt')
    gdb.execute('kill')


def stall_interrupt():
    if not stall['fired']:
        stall['fired'] = True
        stall['want'] = True
        gdb.execute('interrupt')


def task_states(pid):
    """one line per task: tid, state letter (t = stopped by the debugger), wchan, schedstat (ns run, ns wait, slices)"""
    lines = []
    for task in sorted(os.listdir('/proc/%d/task' % pid)):
        try:
            base = '/proc/%d/task/%s/' % (pid, task)
            lines.append('%s %s %s %s' % (task, open(base + 'stat').read().split(')')[1].split()[0],
                                          open(base + 'wchan').read(), open(base + 'schedstat').read().strip()))
        except OSError as error:
            lines.append('%s FAILED %s' % (task, error))
    return lines


def stall_sample(pid):
    """before the interrupt (so the debugger has not stopped anything yet): two samples of every task, 1 s apart"""
    with open(os.path.join(os.environ['COV_OUT'], 'stall_pre.txt'), 'w') as f:
        for n in range(2):
            f.write('### sample %d t=%.3f\n' % (n, time.monotonic()) + '\n'.join(task_states(pid)) + '\n')
            f.flush()
            time.sleep(1)


def stall_watch():
    while True:
        time.sleep(2)
        if stall['fired']:
            return
        if state['eligible'] and time.monotonic() - stall['last'] > STALL_IDLE:
            try:
                stall_sample(stall['pid'])
            except Exception as error:
                log('STALL_SAMPLE_FAILED', error=str(error))
            gdb.post_event(stall_interrupt)
            return


def install():
    binary = gdb.current_progspace().filename
    with open(binary, 'rb') as f:
        digest = hashlib.file_digest(f, 'sha256').hexdigest()
    log('BINARY', path=binary, sha256=digest)
    faulthandler.dump_traceback_later(45, repeat=True, file=open(os.path.join(os.environ['COV_OUT'], 'faulthandler.log'), 'w'))
    think = symbol('CScriptSafeZone::Think(')
    Entry('*' + think, internal=True)
    for prefix, kind in (
            ('CScriptTriggerEllipsoid::Think(', 'base_think'),
            ('CScriptSafeZone::UpdateLoopSound(', 'loopsound'),
            ('CScriptSafeZone::UpdateInsideAlpha(', 'insidealpha'),
            ('CScriptSafeZone::UpdateFlash(', 'flash'),
            ('CScriptSafeZone::UpdateObstruction(', 'obstruction'),
            ('CStateManager::ObjectById(', 'objbyid'),
            ('CGameLight* TCastToPtr<CGameLight>(CEntity*)', 'cast_light'),
            ('CActor::SetTransform(CTransform4f const&)', 'set_transform'),
            ('CStateManager::InformListeners(', 'inform'),
            ('CScriptSafeZone::SetZoneType(', 'set_zone_type'),
            ('CScriptSafeZone::SetLowPassFilter(', 'lowpass'),
            ('CScriptSafeZone::UpdateSafeZoneManager(', 'update_manager'),
            ('CScriptSafeZone::UpdateEchoEmitter(', 'update_emitter'),
            ('CCameraManager::IsInCinematicCamera(', 'cine_query'),
            ('CEntity::SendScriptMsgs(', 'send_msgs'),
            ('CScriptSafeZone::ApplyFog(', 'fog'),
            ('CScriptSafeZone::ApplyInsideFilter(', 'inside_filter'),
            ('CStateManager::SendScriptMsg(CEntity*', 'send_msg'),
            ('CSafeZoneManager::RemoveSafeZone(', 'remove_zone'),
            ('CStateManager::DeleteObjectRequest(', 'delete_object')):
        Mock(symbol(prefix), kind)
    count_lines(think)
    gdb.events.exited.connect(summary)
    gdb.events.stop.connect(stall_dump)
    threading.Thread(target=stall_watch, daemon=True).start()


if os.environ.get('SZ2_SELFTEST') != '1':
    install()
