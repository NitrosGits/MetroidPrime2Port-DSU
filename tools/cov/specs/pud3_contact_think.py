"""pud3: CPuddleSpore Think (fn_51_34E0) and UpdatePlayerContact (fn_51_23F0) forced arms.

Spec and COV_GDB_SCRIPT in one file (default force.py with FORCE = []):
  BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/pud3_contact_think.py \
    tools/cov/force.sh tools/cov/specs/pud3_contact_think.py <out> 3dfd2249,cbc72d79 \
    --con tools/cov/specs/pud3_puddlespore.con --fast --frames 2500

Three arms the fight cannot reach on its own are taken on REAL invocations only:
  think-cbn  Think 0x3670 `bl CanBeNormalized` / 0x3678 beq: the local direction vector (the `this` of the
             call) is zeroed for that one call, so the real CanBeNormalized returns false;
  upc-cbn    UpdatePlayerContact 0x2880 `bl CanBeNormalized` / 0x2888 beq: same;
  upc-gc     UpdatePlayerContact 0x27F0..0x2804 (HasMaterial kMT_GroundCollider, inlined in both builds):
             bit 0x25 of the player's CMaterialList is cleared at the invocation's first DoBoundsOverlap
             call and the whole u64 is restored at the invocation's return.  The oracle is the real
             RemoveMaterial/AddMaterial call sequence (0x2810/0x2820/0x2830 and 0x2924/0x2934/0x2944).
No return value is fabricated.  Ownership: (inferior, thread) context plus frame equality - a nested hook
acts only when its caller frame equals the owning window's frame.  Snapshots precede arming; arming
precedes every write; each write is read back; every restore is read back.  The summary fails closed.
"""
import gdb
import hashlib
import json
import os

CLASS = 'CPuddleSpore'
FORCE = []
NEED = 2                    # forced successes wanted per arm; one is enough to pass
GC_BIT = 0x25               # kMT_GroundCollider
PLAYER_BIT = 0x20           # kMT_Player
CBN_SITES = [('think', 'CPuddleSpore::Think(', 473), ('upc', 'CPuddleSpore::UpdatePlayerContact(', 955)]
OVERLAP_SITE = 706          # UpdatePlayerContact+706: the loop's first CAABox::DoBoundsOverlap call (0x26C8)
state = dict(active={}, failed=0, restores=0, ignored=0, problems=[], nat_think=0, nat_upc=0,
             think_ok=0, upc_cbn_ok=0, gc_ok=0, windows=0, upc_noeffect=0)


def log(event, **values):
    with open(os.path.join(os.environ['COV_OUT'], 'pud3.jsonl'), 'a') as f:
        f.write(json.dumps(dict(event=event, **values), sort_keys=True, default=str) + '\n')


def iv(expression):
    return int(gdb.parse_and_eval(expression))


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


def failure(text):
    state['failed'] += 1
    state['problems'].append(str(text))
    log('FAIL', reason=str(text))


def same_frame(a, b):
    check(a is not None and b is not None and a.is_valid() and b.is_valid(), 'invalid frame')
    return bool(a == b)


class Snapshot:
    def __init__(self, regions, memory):
        self.memory = memory
        self.saved = [(l, a, bytes(memory.read_memory(a, n))) for l, a, n in regions]

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


class Window(gdb.FinishBreakpoint):
    """One owned Think / UpdatePlayerContact invocation."""

    def __init__(self, kind, this, mgr, frame, ctx):
        self.kind, self.this, self.mgr, self.owner, self.ctx = kind, this, mgr, frame, ctx
        self.closed = False
        self.mat = []                 # RemoveMaterial/AddMaterial calls: ('R'|'A', type)
        self.cbn = []                 # CanBeNormalized results, ('nat'|'forced', rax)
        self.emitter = 0              # CSfxManager::UpdateEmitter calls
        self.forced = None            # 'cbn' | 'gc' | None
        self.entry_gc = None          # player had kMT_GroundCollider on entry (UPC only)
        self.cbn_pending = None       # (forced, snapshot) between the call site and the instruction after it
        self.gc_snapshot = None
        super().__init__(frame, internal=True)   # MUST succeed before any write
        check(ctx not in state['active'] or kind not in state['active'][ctx], 'nested owned ' + kind)
        state['active'].setdefault(ctx, {})[kind] = self
        state['windows'] += 1
        if kind == 'upc' and iv('((CStateManager*)%d)->mNumPlayers' % mgr) >= 1:
            player = iv('(unsigned long)((CStateManager*)%d)->mPlayers[0]' % mgr)
            mask = iv('*(unsigned long*)&((CActor*)%d)->mMaterial.mValue' % player)
            self.entry_gc = bool(mask & (1 << GC_BIT))

    def restore_gc(self):
        if self.gc_snapshot is None:
            return
        rows, errors = self.gc_snapshot.restore()
        state['restores'] += 1
        log('RESTORE_GC', fields=rows, errors=errors)
        self.gc_snapshot = None
        check(not errors, 'gc restore failed ' + str(errors))

    def close(self):
        self.closed = True
        if self.cbn_pending is not None:
            failure('%s closed with a CanBeNormalized call pending' % self.kind)
            if self.cbn_pending[1] is not None:
                rows, errors = self.cbn_pending[1].restore()
                log('RESTORE_CBN_UNWIND', fields=rows, errors=errors)
            self.cbn_pending = None
        table = state['active'].get(self.ctx, {})
        if table.get(self.kind) is self:
            table.pop(self.kind)

    def stop(self):
        if self.closed:
            failure('duplicate finish; no stale writes')
            return True
        good = True
        try:
            check(state['active'].get(self.ctx, {}).get(self.kind) is self, 'finish window ownership')
            self.restore_gc()
            self.judge()
        except Exception as error:
            failure(error)
            good = False
        finally:
            self.close()
        return not good

    def out_of_scope(self):
        if self.closed:
            failure('late unwind; no writes')
            return
        failure('%s unwound before return (forced=%s)' % (self.kind, self.forced))
        try:
            self.restore_gc()
        except Exception as error:
            failure(error)
        self.close()

    def judge(self):
        if self.kind == 'think':
            if self.forced == 'cbn':
                check([r for k, r in self.cbn if k == 'forced'] == [0], 'forced Think CanBeNormalized not false')
                check(self.emitter == 0, 'UpdateEmitter ran on the false arm')
                state['think_ok'] += 1
                log('RESULT', fn='Think', arm='cbn-false', emitter=self.emitter, cbn=self.cbn)
            elif self.cbn:
                check(self.cbn[0] == ('nat', 1) and self.emitter == 1, 'natural Think arm shape')
                state['nat_think'] += 1
                log('RESULT', fn='Think', arm='cbn-true', emitter=self.emitter, cbn=self.cbn)
            return
        push = bool(self.cbn)           # CanBeNormalized is only reached on the push path
        if self.forced == 'gc':
            if not push:
                state['upc_noeffect'] += 1
                log('RESULT', fn='UPC', arm='gc-noeffect', mat=self.mat)
                return
            check(self.mat == [('R', PLAYER_BIT), ('A', PLAYER_BIT)], 'gc-false sequence ' + str(self.mat))
            state['gc_ok'] += 1
            log('RESULT', fn='UPC', arm='gc-false', mat=self.mat, cbn=self.cbn)
        elif self.forced == 'cbn':
            if not push:
                state['upc_noeffect'] += 1
                log('RESULT', fn='UPC', arm='cbn-noeffect', mat=self.mat)
                return
            check([r for k, r in self.cbn if k == 'forced'] == [0], 'forced UPC CanBeNormalized not false')
            want = ([('R', GC_BIT), ('R', PLAYER_BIT), ('A', GC_BIT), ('A', PLAYER_BIT)] if self.entry_gc
                    else [('R', PLAYER_BIT), ('A', PLAYER_BIT)])
            check(self.mat == want, 'cbn-false sequence %s for entry_gc=%s' % (self.mat, self.entry_gc))
            state['upc_cbn_ok'] += 1
            log('RESULT', fn='UPC', arm='cbn-false', mat=self.mat, cbn=self.cbn)
        elif push:
            # the sequence must follow the GroundCollider bit the player had on entry (0x2804 / 0x2924 gate)
            want = ([('R', GC_BIT), ('R', PLAYER_BIT), ('A', GC_BIT), ('A', PLAYER_BIT)] if self.entry_gc
                    else [('R', PLAYER_BIT), ('A', PLAYER_BIT)])
            check(self.mat == want, 'natural push sequence %s for entry_gc=%s' % (self.mat, self.entry_gc))
            state['nat_gc_set' if self.entry_gc else 'nat_gc_clear'] = state.get('nat_gc_set' if self.entry_gc else 'nat_gc_clear', 0) + 1
            check(self.cbn == [('nat', 1)], 'natural push CanBeNormalized ' + str(self.cbn))
            state['nat_upc'] += 1
            log('RESULT', fn='UPC', arm='natural-push', mat=self.mat, cbn=self.cbn)


def owned(kind):
    """The window of `kind` whose frame is the direct caller of the current hook, or None."""
    ctx = context()
    w = state['active'].get(ctx, {}).get(kind)
    if w is None:
        return None
    caller = gdb.newest_frame().older()
    if not same_frame(w.owner, caller):
        state['ignored'] += 1
        return None
    return w


def abort(error):
    failure(error)
    for table in list(state['active'].values()):
        for w in list(table.values()):
            try:
                w.restore_gc()
            except Exception as restore_error:
                failure(restore_error)
    return True


class Entry(gdb.Breakpoint):
    def __init__(self, symbol, kind):
        super().__init__('*' + symbol, internal=True)
        self.kind = kind

    def stop(self):
        if state['failed']:
            return True
        try:
            ctx = context()
            frame = gdb.newest_frame()
            check(frame is not None and frame.is_valid(), 'entry frame')
            check(self.kind not in state['active'].get(ctx, {}), 'recursive entry rejected before writes')
            Window(self.kind, iv('$rdi'), iv('$rsi'), frame, ctx)
            return False
        except Exception as error:
            return abort(error)


def owned_here(kind):
    """The window of `kind` whose frame is the frame that stopped at the current call site, or None."""
    w = state['active'].get(context(), {}).get(kind)
    if w is None or not same_frame(w.owner, gdb.newest_frame()):
        state['ignored'] += 1
        return None
    return w


class CbnAfter(gdb.Breakpoint):
    """The instruction after the CanBeNormalized call in the owner frame: read the result, restore."""

    def __init__(self, address, kind):
        super().__init__('*' + address, internal=True)
        self.kind = kind

    def stop(self):
        if state['failed']:
            return True
        try:
            w = owned_here(self.kind)
            if w is None or w.cbn_pending is None:
                return False
            forced, snapshot = w.cbn_pending
            w.cbn_pending = None
            rax = iv('$rax') & 0xff
            if snapshot is not None:
                rows, errors = snapshot.restore()
                state['restores'] += 1
                log('RESTORE_CBN', kind=w.kind, rax=rax, fields=rows, errors=errors)
                check(not errors, 'cbn restore failed ' + str(errors))
            check(rax in (0, 1), 'unreadable CanBeNormalized result')
            w.cbn.append(('forced' if forced else 'nat', rax))
            return False
        except Exception as error:
            return abort(error)


class Cbn(gdb.Breakpoint):
    """The CanBeNormalized call site inside Think / UpdatePlayerContact (the function itself is hot in collision)."""

    def __init__(self, address, kind):
        super().__init__('*' + address, internal=True)
        self.kind = kind

    def stop(self):
        if state['failed']:
            return True
        try:
            w = owned_here(self.kind)
            if w is None:
                return False
            check(w.cbn_pending is None, 'cbn call pending')
            this = iv('$rdi')
            want = False
            if w.forced is None:
                if w.kind == 'think':
                    want = state['nat_think'] >= 1 and state['think_ok'] < NEED
                else:
                    want = (state['nat_upc'] >= 1 and state['upc_cbn_ok'] < NEED
                            and state['gc_ok'] > state['upc_cbn_ok'])
            snap = None
            if want:
                snap = Snapshot([('direction vector', this, 12)], gdb.selected_inferior())
                w.cbn_pending = (True, snap)        # armed BEFORE the write
                w.forced = 'cbn'
                log('ARM_CBN', kind=w.kind, originals=snap.originals())
                gdb.selected_inferior().write_memory(this, bytes(12))
                back = bytes(gdb.selected_inferior().read_memory(this, 12))
                check(back == bytes(12), 'forced vector readback')
            else:
                w.cbn_pending = (False, None)
            return False
        except Exception as error:
            return abort(error)


class Overlap(gdb.Breakpoint):
    """The first DoBoundsOverlap call site of an UpdatePlayerContact invocation (this frame is the owner)."""

    def stop(self):
        if state['failed']:
            return True
        try:
            ctx = context()
            w = state['active'].get(ctx, {}).get('upc')
            if w is None or not same_frame(w.owner, gdb.newest_frame()):
                state['ignored'] += 1
                return False
            if w.forced is not None or w.gc_snapshot is not None:
                return False
            if not (state['nat_upc'] >= 1 and state['gc_ok'] < NEED and state['gc_ok'] <= state['upc_cbn_ok']):
                return False
            if getattr(w, 'gc_tried', False):
                return False
            w.gc_tried = True
            check(iv('((CStateManager*)%d)->mNumPlayers' % w.mgr) >= 1, 'a player exists')
            player = iv('(unsigned long)((CStateManager*)%d)->mPlayers[0]' % w.mgr)
            check(player != 0, 'player pointer')
            address = iv('(unsigned long)&((CActor*)%d)->mMaterial.mValue' % player)
            snap = Snapshot([('player mMaterial.mValue', address, 8)], gdb.selected_inferior())
            original = int.from_bytes(snap.saved[0][2], 'little')
            if not original & (1 << GC_BIT):
                log('GC_SKIP', reason='ground collider bit already clear', value=original)
                return False
            w.gc_snapshot = snap                      # restore at the window finish, armed before the write
            w.forced = 'gc'
            log('ARM_GC', originals=snap.originals())
            forced = original & ~(1 << GC_BIT)
            gdb.selected_inferior().write_memory(address, forced.to_bytes(8, 'little'))
            back = int.from_bytes(bytes(gdb.selected_inferior().read_memory(address, 8)), 'little')
            check(back == forced, 'forced material readback')
            return False
        except Exception as error:
            return abort(error)


class Material(gdb.Breakpoint):
    def __init__(self, symbol, kind):
        super().__init__('*' + symbol, internal=True)
        self.kind = kind

    def stop(self):
        if state['failed']:
            return True
        try:
            w = owned('upc')
            if w is not None:
                w.mat.append((self.kind, iv('$esi') & 0xffffffff))
            return False
        except Exception as error:
            return abort(error)


class Emitter(gdb.Breakpoint):
    def stop(self):
        if state['failed']:
            return True
        try:
            w = owned('think')
            if w is not None:
                w.emitter += 1
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
    check(len(matches) == 1, 'one native symbol ' + prefix + ' got ' + str(matches))
    return matches[0]


def site_address(kind, name, offset):
    """Hook the CanBeNormalized call at name+offset (checked to be that call) and the instruction after it."""
    site = '*%s+%d' % (symbol(name), offset)
    lines = gdb.execute('x/2i ' + site, to_string=True).strip().splitlines()
    check('call' in lines[0] and 'CanBeNormalized' in lines[0], 'cbn call site mismatch: ' + lines[0])
    length = int(lines[1].split()[0], 16) - int(lines[0].split()[0], 16)
    check(0 < length < 16, 'call length')
    after = '%s+%d' % (symbol(name), offset + length)      # symbol-relative: the binary is PIE
    log('SITE', site=site, insn=lines[0].strip(), after=after)
    Cbn(site[1:], kind)
    CbnAfter(after, kind)


def summary(event=None):
    pending = sum(len(t) for t in state['active'].values())
    ok = (not state['failed'] and not pending and state['nat_think'] >= 1 and state['think_ok'] >= 1
          and state['nat_upc'] >= 1 and state['gc_ok'] >= 1 and state['upc_cbn_ok'] >= 1)
    log('EXIT', ok=ok, pending=pending, failed=state['failed'], restores=state['restores'],
        ignored_nested=state['ignored'], windows=state['windows'], nat_think=state['nat_think'],
        think_ok=state['think_ok'], nat_upc=state['nat_upc'], gc_ok=state['gc_ok'],
        upc_cbn_ok=state['upc_cbn_ok'], upc_noeffect=state['upc_noeffect'], problems=state['problems'])
    with open(os.path.join(os.environ['COV_OUT'], 'pud3.summary.txt'), 'w') as f:
        f.write(('PASS' if ok else 'FAIL') + ' ' + json.dumps(
            {k: v for k, v in state.items() if k != 'active'}, sort_keys=True) + '\n')


def install():
    binary = gdb.current_progspace().filename
    with open(binary, 'rb') as f:
        digest = hashlib.file_digest(f, 'sha256').hexdigest()
    log('BINARY', path=binary, sha256=digest)
    Entry(symbol('CPuddleSpore::Think('), 'think')
    Entry(symbol('CPuddleSpore::UpdatePlayerContact('), 'upc')
    # DoBoundsOverlap is called tens of thousands of times by the collision code (the first pud3 launch
    # was stalled by a breakpoint on the function itself): hook the one call site inside
    # UpdatePlayerContact, symbol + offset, and refuse to install unless it disassembles to that call.
    upc = symbol('CPuddleSpore::UpdatePlayerContact(')
    site = '*%s+%d' % (upc, OVERLAP_SITE)
    text = gdb.execute('x/i ' + site, to_string=True)
    check('call' in text and 'DoBoundsOverlap' in text, 'overlap call site mismatch: ' + text.strip())
    log('SITE', site=site, insn=text.strip())
    Overlap(site, internal=True)
    for kind, name, offset in CBN_SITES:
        site_address(kind, name, offset)
    Material(symbol('CActor::RemoveMaterial(EMaterialTypes, CStateManager&)'), 'R')
    Material(symbol('CActor::AddMaterial(EMaterialTypes, CStateManager&)'), 'A')
    Emitter('*' + symbol('CSfxManager::UpdateEmitter('), internal=True)
    gdb.events.exited.connect(summary)


if os.environ.get('PUD3_SELFTEST') != '1':
    install()
