"""srp2: CScriptRiftPortal::AdvanceModel (fn_64_23F8) particle-POI loop, owned fixture calls.

Use default force.py (FORCE = []). The window is one natural AdvanceModel call, owned by guarded
frame equality + inferior/thread. After AdvanceParticles returns inside that frame, the static POI
array (CAnimData::mParticlePOINodes), the animation data's passed-particle count (and, for the null
arm, the CModelData animation pointer) are written; every region is snapshotted first and restored
with readbacks at the AdvanceModel return. Actual SetParticleEffectState calls are recorded.
No production probes.
"""
import gdb
import hashlib
import json
import os

CLASS = 'CScriptRiftPortal'
FORCE = []
# name, passed count, nodes [(hash, charIdx kind)], null animation pointer, expected hashes
# kinds: any = -1 (retail 0x24A4 cmpwi -1), same = animData mCharIdx (0x24B4 cmpw eq), other = +1 / +7
CASES = [
    ('mixed4', 4, [(0xA1, 'any'), (0xA2, 'other'), (0xA3, 'same'), (0xA4, 'other7')], False, [0xA1, 0xA3]),
    ('all-skipped', 2, [(0xB1, 'other'), (0xB2, 'other7')], False, []),
    ('zero', 0, [(0xC1, 'any')], False, []),
    ('negative', -3, [(0xD1, 'any'), (0xD2, 'same')], False, []),
    ('single-any', 1, [(0xE1, 'any'), (0xE2, 'any')], False, [0xE1]),
    ('last-only', 3, [(0xF1, 'other'), (0xF2, 'other7'), (0xF3, 'same')], False, [0xF3]),
    ('null-anim', 2, [(0x91, 'any'), (0x92, 'same')], True, []),
]
NODES = 4
state = dict(index=0, active={}, failed=0, restores=0, ignored=0)

def log(event, **values):
    with open(os.path.join(os.environ['COV_OUT'], 'srp2.jsonl'), 'a') as f:
        f.write(json.dumps(dict(event=event, **values), sort_keys=True) + '\n')

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

def failure(text):
    state['failed'] += 1
    log('FAIL', reason=str(text))

def same_frame(a, b):
    check(a is not None and b is not None and a.is_valid() and b.is_valid(), 'invalid frame')
    return bool(a == b)

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

def node_type():
    return gdb.lookup_type('CParticlePOINode')

def node_addr(index):
    base = int(ev('(long)&CAnimData::mParticlePOINodes.mData[0]'))
    return base + index * node_type().sizeof

def regions(model):
    result = [('POI count', int(ev('(long)&CAnimData::mParticlePOINodes.mCount')), 4)]
    result.append(('POI nodes', node_addr(0), NODES * node_type().sizeof))
    anim = iv('((CModelData*)%d)->mAnimData.mItem' % model)
    check(anim != 0, 'real animation data')
    result.append(('passed count', anim + iv('(long)&((CAnimData*)0)->mPassedParticleCount'), 4))
    result.append(('animation pointer', int(ev('(long)&((CModelData*)%d)->mAnimData.mItem' % model)), 8))
    return result

class Window(gdb.FinishBreakpoint):
    def __init__(self, this, model, mgr, case, frame, ctx, snapshot):
        self.this, self.model, self.mgr, self.case = this, model, mgr, case
        self.owner, self.ctx, self.snapshot = frame, ctx, snapshot
        self.closed = False
        self.anim = iv('((CModelData*)%d)->mAnimData.mItem' % model)
        self.charidx = iv('((CAnimData*)%d)->mCharIdx' % self.anim)
        self.db = self.anim + iv('(long)&((CAnimData*)0)->mParticleDB')
        self.parts = 0
        self.armed = False
        self.calls = []
        super().__init__(frame, internal=True)  # MUST succeed before mutation
        check(ctx not in state['active'], 'nested owned AdvanceModel')
        state['active'][ctx] = self
        log('ARM', case=case[0], frame_level=frame.level(), context=ctx, this=this, model=model,
            anim=self.anim, char_idx=self.charidx, originals=snapshot.originals())

    def restore(self):
        check(context() == self.ctx, 'restore context mismatch: no unsafe writes')
        check(not self.closed and state['active'].get(self.ctx) is self,
              'restore ownership mismatch: no stale writes')
        rows, errors = self.snapshot.restore()
        log('RESTORE', case=self.case[0], fields=rows, errors=errors)
        state['restores'] += 1
        check(not errors, 'restore failed ' + str(errors))

    def apply(self):
        """Called in the owned frame right after AdvanceParticles returned (retail 0x2468)."""
        name, count, nodes, null_anim, expected = self.case
        check(same_frame(gdb.newest_frame(), self.owner), 'apply frame')
        writes = [('((CAnimData*)%d)->mPassedParticleCount' % self.anim, count)]
        writes.append(('CAnimData::mParticlePOINodes.mCount', max(count, 0)))
        for i, (hashv, kind) in enumerate(nodes):
            idx = {'any': -1, 'same': self.charidx, 'other': self.charidx + 1,
                   'other7': self.charidx + 7}[kind]
            node = '((CParticlePOINode*)%d)->' % node_addr(i)
            writes += [(node + 'mNameHash', hashv), (node + 'mCharIdx', idx)]
        if null_anim:
            writes.append(('((CModelData*)%d)->mAnimData.mItem' % self.model, 0))
        try:
            for expression, value in writes:
                self.armed = True  # execute can change memory and then raise
                gdb.execute('set var %s = %d' % (expression, value), to_string=True)
                check(iv(expression) == value, 'forced readback mismatch ' + expression)
        except Exception:
            self.restore()
            raise
        log('APPLY', case=name, writes=[[e, v] for e, v in writes])

    def stop(self):
        if self.closed:
            failure('duplicate finish; no stale writes')
            return True
        good = False
        try:
            check(state['active'].get(self.ctx) is self, 'finish window ownership')
            check(self.parts == 1 and self.armed, 'AdvanceParticles completed once and writes applied')
            expected = self.case[4]
            got = [c['name'] for c in self.calls]
            check(got == expected, 'POI effect calls %s != %s' % (got, expected))
            for c in self.calls:
                check(c['this'] == self.db and c['active'] == 1 and c['mgr'] == self.mgr,
                      'call arguments ' + str(c))
            log('RESULT', case=self.case[0], calls=self.calls, expected=expected)
            good = True
        except Exception as error:
            failure(error)
        finally:
            try:
                self.restore()
            except Exception as error:
                failure(error)
                good = False
            self.closed = True
            if state['active'].get(self.ctx) is self:
                state['active'].pop(self.ctx)
        if good:
            state['index'] += 1
        return not good

    def out_of_scope(self):
        if self.closed:
            failure('late unwind; no writes')
            return
        failure('AdvanceModel unwind ' + self.case[0])
        try:
            self.restore()
        except Exception as error:
            failure(error)
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
    failure(error)
    for w in list(state['active'].values()):
        try:
            w.restore()
        except Exception as restore_error:
            failure(restore_error)
    return True

class Entry(gdb.Breakpoint):
    def stop(self):
        if state['failed']:
            return True
        if state['index'] == len(CASES):
            return False
        try:
            this, model, mgr = iv('$rdi'), iv('$rsi'), iv('$rdx')
            ctx = context()
            frame = gdb.newest_frame()
            check(frame is not None and frame.is_valid(), 'entry frame')
            check(ctx not in state['active'], 'recursive entry rejected before writes')
            dynamic = str(ev('((CEntity*)%d)' % this).dynamic_type)
            check(dynamic == 'CScriptRiftPortal *', 'concrete portal dynamic type ' + dynamic)
            if iv('((CModelData*)%d)->mAnimData.mItem' % model) == 0:
                return False
            if state['index'] == 0:
                log('TARGET', this=this, dynamic_type=dynamic,
                    editor=iv('((CEntity*)%d)->mEditorId.value' % this))
            snap = Snapshot(regions(model), gdb.selected_inferior())
            Window(this, model, mgr, CASES[state['index']], frame, ctx, snap)
            return False
        except Exception as error:
            return abort(error)

class ParticlesReturn(gdb.FinishBreakpoint):
    def __init__(self, w):
        super().__init__(gdb.newest_frame(), internal=True)
        self.w = w

    def stop(self):
        try:
            w = self.w
            check(state['active'].get(context()) is w and not w.closed, 'particles ownership')
            check(same_frame(gdb.newest_frame(), w.owner), 'particles return owner')
            w.parts += 1
            w.apply()
            return False
        except Exception as error:
            return abort(error)

    def out_of_scope(self):
        failure('AdvanceParticles unwound')

class Trace(gdb.Breakpoint):
    def __init__(self, symbol, kind):
        super().__init__('*' + symbol, internal=True)
        self.kind = kind

    def stop(self):
        try:
            w = owned()
            if w is None:
                return False
            if self.kind == 'particles':
                # rdi this, rsi transform, xmm0 dt, rdx manager
                check(iv('$rdi') == w.model, 'AdvanceParticles model')
                check(iv('$rdx') == w.mgr, 'AdvanceParticles manager')
                ParticlesReturn(w)
            else:
                check(w.armed, 'effect call only after the fixture writes')
                w.calls.append(dict(name=iv('$rsi') & 0xffffffff, this=iv('$rdi'),
                                    active=iv('$rdx') & 0xff, mgr=iv('$rcx')))
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

def summary(event):
    ok = state['index'] == len(CASES) and not state['active'] and not state['failed']
    log('EXIT', ok=ok, completed=state['index'], pending=len(state['active']),
        failed=state['failed'], restores=state['restores'], ignored_nested=state['ignored'])
    if not ok:
        failure('incomplete srp2 run')

def install():
    binary = gdb.current_progspace().filename
    with open(binary, 'rb') as f:
        digest = hashlib.file_digest(f, 'sha256').hexdigest()
    log('BINARY', path=binary, sha256=digest)
    Entry('*' + symbol('CScriptRiftPortal::AdvanceModel('), internal=True)
    Trace(symbol('CModelData::AdvanceParticles('), 'particles')
    Trace(symbol('CParticleDatabase::SetParticleEffectState(unsigned int'), 'effect')
    gdb.events.exited.connect(summary)

if os.environ.get('SRP2_SELFTEST') != '1':
    install()
