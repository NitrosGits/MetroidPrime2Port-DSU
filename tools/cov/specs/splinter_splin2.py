# Splinter (module 74) splin2 fixture: ~CSplinter (fn_74_ABA0), SSurfaceAlignment dtor (fn_74_AF50),
# SSpitData copy ctor (fn_74_B9DC), ~CSplinterAcidSac (fn_74_CCA8).  force.sh spec AND COV_GDB_SCRIPT.
#
#   COV_ROOT=$PWD COV_GDB_SCRIPT=tools/cov/specs/splinter_splin2.py MP2_ALLOW_PTRACE=1 tools/cov/force.sh \
#     tools/cov/specs/splinter_splin2.py <out> 3bfa3eff,0e_temple_hive_hall --con tools/cov/specs/splinter_splin2.con \
#     --fast --frames 1500
#
# Retail order (G2ME01 Splinter CSplinter.s): ABA0 stores the vtable, then 0xABD0 `bl fn_74_B144` (mSpit, r4=-1),
# the mParticle.mEffect token (valid byte +0xafc), the mStateMachine2 token (valid byte +0xaec), 0xAC4C
# `bl ~SSacData` (+0xa8c), the empty mEvadeChecks loop (+0xa14), 0xAC80 `bl fn_74_AF50` (+0x948), 0xAC8C
# `bl ~CPatterned` (r4=0), then the delete flag -> CMemory::Free.  CCA8: vtable, CRELFileToken (+0x1b8),
# CDamageVulnerability (+0x168), CActor (r4=0), then the delete flag -> Free.  Host LP64 offsets are below.
#
# Ownership: an invocation is the PHYSICAL frame of the D0/D2 entry (gdb Frame equality: stack address +
# function, stable while the pc advances) plus (inferior, thread) plus the root ($rdi at entry).  A member
# event is a callee-entry breakpoint whose caller, walked up through INLINE frames only, is that same owner
# frame; any NORMAL frame in between (a member's own out-of-line body) makes it a nested call and it is
# ignored.  PCs and frame levels are breakpoint locations / log fields only, never identity.  The host
# tail-calls (D0's final `jmp operator delete`, D2's `jmp ~CAi` / `jmp ~CActor`) leave no owner frame: they
# count only when the callee's caller frame equals the owner's recorded caller frame and $rdi == root.
# Each chain ends at a FinishBreakpoint on the owner frame (out_of_scope = failure).
#
# Forcing (first CSplinter load only, snapshot -> write -> readback, restored with readback when the
# constructor returns to the same LoadSplinter frame): isMegaSplinter := 1, stateMachine2 := stateMachine,
# pART := gibParticles (the line itself then clears stateMachine, also restored), so mSpit / mStateMachine2 /
# mParticle.mEffect are engaged on that root; the other four Splinters keep every arm disengaged.  The mega's
# spit asset is invalid (spl6): CPatterned::LaunchProjectile from DoUserAnimEvent answers null.
#
# Copy ctor: on the forced root the constructor's `mSpit = SSpitData(...)` builds a temporary (main ctor,
# real call) and copy-constructs it into the disengaged optional (inline SSpitData(const SSpitData&)).  The
# temporary's fields are snapshotted after its ctor returns; at its first destruction event (first owned
# CToken::~CToken inside the temporary, i.e. after the copy and before any temporary member dies) the
# temporary is re-read (unchanged) and the destination root+0xc40 is compared field by field (DWARF fields,
# no padding; engaged optionals reinterpreted as their payload).
import hashlib
import json
import os
import re
import struct

import gdb

OUT = os.environ.get('COV_OUT', '/tmp')
JSONL = os.path.join(OUT, 'splin2.jsonl')
SUMMARY = os.path.join(OUT, 'splin2_summary.txt')
ROOT_SRC = os.environ.get('COV_ROOT', '.')

# host LP64 CSplinter offsets (DWARF): mAlignment 0xa38 (mCache single_ptr +0x50), mSac 0xb7c,
# mStateMachine2 0xbd8 (valid +0x10), mParticle 0xbf0 (mEffect valid +0x18), mSpit 0xc40 (valid +0x90)
SPL = dict(alignment=0xa38, cache=0xa88, sac=0xb7c, sm2=0xbd8, sm2_valid=0xbe8, part=0xbf0,
           part_valid=0xc08, spit=0xc40, spit_end=0xcd0, spit_valid=0xcd0, visor=0xc88, visor_valid=0xca0,
           weapon=0xc50, vuln=0x3e8)
SAC = dict(rel=0x220, vuln=0x1c8)
SPIT_SIZE = 144

state = {'binary': 'unchecked', 'installed': 0, 'failed': 0, 'owners': [], 'chains': [],
         'force': None, 'ctor': None, 'copy': None, 'launch_stub': 0, 'next_id': 0}


def log(kind, **kw):
    kw['kind'] = kind
    with open(JSONL, 'a') as fh:
        fh.write(json.dumps(kw, sort_keys=True, default=str) + '\n')


def failure(text, **kw):
    state['failed'] += 1
    log('FAIL', reason=str(text), **kw)


def check(cond, text):
    if not cond:
        raise RuntimeError(text)


def context():
    thread = gdb.selected_thread()
    inferior = gdb.selected_inferior()
    check(thread is not None and inferior is not None and thread.global_num is not None,
          'unreadable inferior/thread identity')
    return int(inferior.num), int(thread.global_num)


def same_frame(a, b):
    check(a is not None and b is not None and a.is_valid() and b.is_valid(), 'invalid frame')
    return bool(a == b)


def physical(frame):
    """Walk up through INLINE frames; returns (physical frame, inline names innermost first)."""
    names = []
    while frame is not None and frame.type() == gdb.INLINE_FRAME:
        names.append(frame.name() or '?')
        frame = frame.older()
    return frame, names


def reg(name):
    return int(gdb.parse_and_eval('$' + name)) & 0xffffffffffffffff


def mem():
    return gdb.selected_inferior()


def u8(addr):
    return bytes(mem().read_memory(addr, 1))[0]


def u64(addr):
    return struct.unpack('<Q', bytes(mem().read_memory(addr, 8)))[0]


def nm_syms(prefix):
    out = []
    for ln in open(os.environ['COV_NM']):
        parts = ln.rstrip('\n').split('\t')
        if len(parts) == 2 and parts[1].startswith(prefix) and '.cold' not in parts[1] \
                and 'clone' not in parts[1]:
            f = parts[0].split()
            if len(f) == 3 and f[1] in 'tTW':
                out.append((int(f[0], 16), f[2]))
    return out


# PIE: a constant '*0x...' never relocates, and '*<mangled>' resolves D2/C2 aliases to the D0/C1 symbol, so every
# address breakpoint is '*((char*)main + delta)' (gdb re-evaluates the expression after relocation).
def _main_addr():
    found = set()
    for ln in open(os.environ['COV_NM']):
        raw = ln.split('\t')[0].split()
        if len(raw) == 3 and raw[2] == 'main' and raw[1] == 'T':
            found.add(int(raw[0], 16))
    check(len(found) == 1, 'main symbol')
    return found.pop()


def at(addr):
    return '*((char*)main + %d)' % (addr - MAIN)


def nm_addr(mangled):
    found = set()
    for ln in open(os.environ['COV_NM']):
        f = ln.split('\t')[0].split()
        if len(f) == 3 and f[2] == mangled:
            found.add(int(f[0], 16))
    check(len(found) == 1, 'symbol %s: %d addresses' % (mangled, len(found)))
    return at(found.pop())


def nm_unique(prefix):
    syms = nm_syms(prefix)
    addrs = {a for a, _m in syms}
    check(len(addrs) == 1, 'symbol %r: %d addresses' % (prefix, len(addrs)))
    return at(addrs.pop())


# ---------------------------------------------------------------- field flattening / snapshots
def flatten(val, path, out):
    t = val.type.strip_typedefs()
    code = t.code
    if code in (gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION):
        if str(t).startswith('rstl::optional_object<'):
            valid = int(val['m_valid'])
            out[path + '.m_valid'] = valid
            if valid:
                inner_t = t.template_argument(0)
                inner = val['m_data'].address.cast(inner_t.pointer()).dereference()
                flatten(inner, path + '.value', out)
            return
        for f in t.fields():
            if f.is_base_class:
                flatten(val.cast(f.type), path + '.<' + str(f.type) + '>', out)
            elif f.name and hasattr(f, 'bitpos'):
                flatten(val[f.name], path + '.' + f.name, out)
        return
    if code == gdb.TYPE_CODE_ARRAY:
        lo, hi = t.range()
        for i in range(lo, hi + 1):
            flatten(val[i], '%s[%d]' % (path, i), out)
        return
    if code == gdb.TYPE_CODE_FLT:
        out[path] = struct.pack('<f' if t.sizeof == 4 else '<d', float(val)).hex()
        return
    if code == gdb.TYPE_CODE_PTR:
        out[path] = '0x%x' % (int(val) & 0xffffffffffffffff)
        return
    out[path] = int(val)


def spit_fields(addr):
    t = gdb.lookup_type('CSplinter::SSpitData')
    check(t.sizeof == SPIT_SIZE, 'SSpitData sizeof %d' % t.sizeof)
    out = {}
    flatten(gdb.Value(addr).cast(t.pointer()).dereference(), 'SSpitData', out)
    return out


class Snapshot:
    def __init__(self, regions):
        self.saved = [(label, addr, bytes(mem().read_memory(addr, size))) for label, addr, size in regions]

    def rows(self):
        return [dict(label=l, address='0x%x' % a, original=b.hex()) for l, a, b in self.saved]

    def restore(self):
        errors, rows = [], []
        for label, addr, raw in reversed(self.saved):
            try:
                mem().write_memory(addr, raw)
            except Exception as e:
                errors.append(label + ' write: ' + str(e))
        for label, addr, raw in self.saved:
            try:
                got = bytes(mem().read_memory(addr, len(raw)))
                rows.append(dict(label=label, address='0x%x' % addr, original=raw.hex(), readback=got.hex()))
                if got != raw:
                    errors.append(label + ' readback mismatch')
            except Exception as e:
                errors.append(label + ' read: ' + str(e))
        return rows, errors


# ---------------------------------------------------------------- forcing (first load)
_src = open(os.path.join(ROOT_SRC, 'src/MetroidPrime/Enemies/CSplinter.cpp')).read().split('\n')
FORCE_LINE = [i + 1 for i, l in enumerate(_src)
              if re.search(r'^  if \(sldrThis\.patterned\.stateMachine2 != kInvalidAssetId\) \{', l)]
check(len(FORCE_LINE) == 1, 'force line')
FORCE_FIELDS = ['sldrThis.isMegaSplinter', 'sldrThis.patterned.stateMachine',
                'sldrThis.patterned.stateMachine2', 'sldrThis.pART']


class ForceBp(gdb.Breakpoint):
    def stop(self):
        try:
            if state['force'] is None:
                self._force()
        except Exception as e:
            failure('force: %r' % e)
        return False

    def _force(self):
        regions = []
        for expr in FORCE_FIELDS:
            v = gdb.parse_and_eval(expr)
            check(v.address is not None, 'addressable ' + expr)
            regions.append((expr, int(v.address), v.type.sizeof))
        snap = Snapshot(regions)
        gib = int(gdb.parse_and_eval('sldrThis.patterned.gibParticles'))
        sm = int(gdb.parse_and_eval('sldrThis.patterned.stateMachine'))
        gdb.execute('set var sldrThis.isMegaSplinter = 1')
        gdb.execute('set var sldrThis.patterned.stateMachine2 = sldrThis.patterned.stateMachine')
        gdb.execute('set var sldrThis.pART = sldrThis.patterned.gibParticles')
        rb = {e: int(gdb.parse_and_eval(e)) for e in FORCE_FIELDS}
        ok = rb['sldrThis.isMegaSplinter'] == 1 and rb['sldrThis.patterned.stateMachine2'] == sm \
            and rb['sldrThis.pART'] == gib
        loader, names = physical(gdb.newest_frame())
        state['force'] = dict(snap=snap, loader=loader, ctx=context(), restored=False)
        log('force', line=FORCE_LINE[0], originals=snap.rows(), gib=gib, sm=sm, readback=rb, ok=ok,
            loader=loader.name(), inline=names)
        if not ok:
            failure('force readback mismatch', readback=rb)


class CtorBp(gdb.Breakpoint):
    """CSplinter ctor entry: owns the forced root's ctor frame (called straight from the forced loader frame)."""

    def stop(self):
        try:
            f = state['force']
            if f is None or state['ctor'] is not None or f['restored']:
                return False
            caller, names = physical(gdb.newest_frame().older())
            if context() != f['ctx'] or not same_frame(caller, f['loader']):
                return False
            frame = gdb.newest_frame()
            root = reg('rdi')
            state['ctor'] = dict(frame=frame, root=root, ctx=context(), temp=None)
            log('ctor_enter', root='0x%x' % root, caller=caller.name(), inline=names)
            CtorFinish(frame)
        except Exception as e:
            failure('ctor entry: %r' % e)
        return False


class CtorFinish(gdb.FinishBreakpoint):
    def __init__(self, frame):
        super().__init__(frame, internal=True)

    def stop(self):
        try:
            c, f = state['ctor'], state['force']
            root = c['root']
            arms = dict(spit_valid=u8(root + SPL['spit_valid']), visor_valid=u8(root + SPL['visor_valid']),
                        part_valid=u8(root + SPL['part_valid']), sm2_valid=u8(root + SPL['sm2_valid']))
            rows, errors = f['snap'].restore()
            f['restored'] = True
            try:
                uid = '0x%04x' % int(gdb.parse_and_eval('((CEntity*)0x%x)->mUniqueId.value' % root))
            except Exception as e:
                uid = 'unreadable: %s' % str(e)[:60]
            log('ctor_return_restore', root='0x%x' % root, uid=uid, arms=arms, rows=rows,
                errors=errors, back_in=same_frame(gdb.newest_frame(), f['loader']))
            c['uid'], c['arms'] = uid, arms
            if errors:
                failure('restore', errors=errors)
            if state['copy'] is None or state['copy'].get('result') is None:
                failure('copy ctor fixture never compared', copy=str(state['copy'])[:200])
        except Exception as e:
            failure('ctor finish: %r' % e)
        state['ctor'] = dict(state['ctor'], frame=None, done=True)
        copy_tok.enabled = False
        return False

    def out_of_scope(self):
        failure('ctor finish out of scope')


class SpitCtorBp(gdb.Breakpoint):
    """SSpitData main ctor (temporary) called straight from the owned CSplinter ctor frame."""

    def stop(self):
        try:
            c = state['ctor']
            if c is None or c.get('frame') is None or state['copy'] is not None:
                return False
            caller, names = physical(gdb.newest_frame().older())
            if context() != c['ctx'] or not same_frame(caller, c['frame']):
                return False
            temp = reg('rdi')
            state['copy'] = dict(temp=temp, src=None, inline_hits=[], result=None)
            log('spit_temp_ctor', temp='0x%x' % temp, inline=names)
            SpitCtorFinish(gdb.newest_frame())
        except Exception as e:
            failure('spit ctor: %r' % e)
        return False


class SpitCtorFinish(gdb.FinishBreakpoint):
    def __init__(self, frame):
        super().__init__(frame, internal=True)

    def stop(self):
        try:
            cp = state['copy']
            cp['src'] = spit_fields(cp['temp'])
            log('spit_temp_snapshot', temp='0x%x' % cp['temp'], fields=len(cp['src']), src=cp['src'])
            copy_tok.enabled = True
        except Exception as e:
            failure('spit snapshot: %r' % e)
        return False

    def out_of_scope(self):
        failure('spit ctor finish out of scope')


class CopyInlineBp(gdb.Breakpoint):
    """The inline SSpitData(const SSpitData&) location inside the owned ctor frame."""

    def stop(self):
        try:
            c = state['ctor']
            if c is None or c.get('frame') is None:
                return False
            frame = gdb.newest_frame()
            phys, names = physical(frame)
            if context() != c['ctx'] or not same_frame(phys, c['frame']):
                return False
            this = None
            fr = frame
            while fr is not None and fr.type() == gdb.INLINE_FRAME:
                if 'SSpitData::SSpitData' in (fr.name() or ''):
                    try:
                        this = int(fr.read_var('this'))
                    except Exception as e:
                        this = 'unreadable: %s' % str(e)[:60]
                    break
                fr = fr.older()
            hit = dict(pc='0x%x' % int(frame.pc()), inline=names,
                       this=('0x%x' % this) if isinstance(this, int) else this,
                       dst_expected='0x%x' % (c['root'] + SPL['spit']),
                       dst_valid_before=u8(c['root'] + SPL['spit_valid']))
            if state['copy'] is not None:
                state['copy']['inline_hits'].append(hit)
            log('copy_inline_enter', **hit)
        except Exception as e:
            failure('copy inline: %r' % e)
        return False


class CopyTempTokenBp(gdb.Breakpoint):
    """First owned CToken::~CToken inside the temporary = start of its destruction, copy already done."""

    def stop(self):
        try:
            cp, c = state['copy'], state['ctor']
            if cp is None or cp['src'] is None or cp['result'] is not None or c.get('frame') is None:
                return False
            rdi = reg('rdi')
            if not (cp['temp'] <= rdi < cp['temp'] + SPIT_SIZE):
                return False
            caller, names = physical(gdb.newest_frame().older())
            if context() != c['ctx'] or not same_frame(caller, c['frame']):
                return False
            src_now = spit_fields(cp['temp'])
            dst = spit_fields(c['root'] + SPL['spit'])
            dst_valid = u8(c['root'] + SPL['spit_valid'])
            rows, bad = [], []
            for k in sorted(cp['src']):
                r = dict(field=k, snap=cp['src'][k], src=src_now.get(k), dst=dst.get(k))
                rows.append(r)
                if not (r['snap'] == r['src'] == r['dst']):
                    bad.append(k)
            extra = sorted(set(dst) ^ set(cp['src']))
            ok = not bad and not extra and dst_valid == 1 and len(cp['inline_hits']) == 1
            cp['result'] = 'OK' if ok else 'FAIL'
            copy_tok.enabled = False
            log('copy_compare', result=cp['result'], token_this='0x%x' % rdi, temp_offset=rdi - cp['temp'],
                dst='0x%x' % (c['root'] + SPL['spit']), dst_valid=dst_valid, fields=len(rows),
                mismatched=bad, key_set_diff=extra, inline_hits=len(cp['inline_hits']), rows=rows,
                inline=names)
            if not ok:
                failure('copy compare', mismatched=bad, extra=extra)
        except Exception as e:
            failure('copy compare: %r' % e)
        return False


# ---------------------------------------------------------------- destructor chains
def splinter_stage(label, rdi, o, names):
    r = o['root']
    off = rdi - r
    joined = ' | '.join(names)
    if label == 'CToken::~CToken':
        if SPL['spit'] <= off < SPL['spit_end']:
            return 0, 'mSpit +0x%x' % off
        if off == SPL['part']:
            return 1, 'mParticle.mEffect'
        if off == SPL['sm2']:
            return 2, 'mStateMachine2'
        if 0 <= off < SPL['alignment']:
            return 5, 'CPatterned token +0x%x' % off
    if label == 'SSacData::~SSacData' and off == SPL['sac']:
        return 3, 'mSac'
    if label in ('GetLockedCacheAllocationBase', 'FreeLockedCache', 'CMemory::Free') \
            and 'SSurfaceAlignment' in joined:
        return 4, 'mAlignment cache vector'
    if label in ('GetLockedCacheAllocationBase', 'FreeLockedCache', 'CMemory::Free') and o['cache'] \
            and 'CPatterned' not in joined and o['events'] and o['events'][-1]['stage'] in (3, 4):
        # inline chain lost the alignment name: between mSac and mCache's delete, labelled separately
        return 4, 'mAlignment cache vector (by order, no inline name)'
    if label == 'operator delete':
        if o['cache'] and rdi == o['cache']:
            return 4, 'mAlignment.mCache'
        if 'CPatterned::~CPatterned' in joined:
            return 5, 'CPatterned body controller'
    if label in ('SLdrIngPossessionData::~SLdrIngPossessionData', 'CBodyStateInfo::~CBodyStateInfo',
                 'CBodyStateCmdMgr::~CBodyStateCmdMgr'):
        return 5, 'CPatterned ' + label
    if label == 'CDamageVulnerability::~CDamageVulnerability' and off == SPL['vuln']:
        return 5, 'CPatterned mDamageVulnerability'
    if label == 'CAi::~CAi' and off == 0:
        return 6, 'CAi base'
    return None, 'foreign %s +0x%x' % (label, off)


def sac_stage(label, rdi, o, names):
    off = rdi - o['root']
    if label == 'CRELFileToken::~CRELFileToken' and off == SAC['rel']:
        return 0, 'mRelToken'
    if label == 'CDamageVulnerability::~CDamageVulnerability' and off == SAC['vuln']:
        return 1, 'mDamageVulnerability'
    if label == 'CActor::~CActor' and off == 0:
        return 2, 'CActor base'
    return None, 'foreign %s +0x%x' % (label, off)


TAIL = {('splinter', 'D0'): ('operator delete', 7), ('splinter', 'D2'): ('CAi::~CAi', 6),
        ('sac', 'D0'): ('operator delete', 3), ('sac', 'D2'): ('CActor::~CActor', 2)}


class EntryBp(gdb.Breakpoint):
    def __init__(self, spec, kind, variant):
        super().__init__(spec, internal=True)
        self.kind, self.variant = kind, variant

    def stop(self):
        try:
            frame = gdb.newest_frame()
            phys, names = physical(frame)
            root = reg('rdi')
            o = dict(id=state['next_id'], kind=self.kind, variant=self.variant, frame=phys,
                     caller=phys.older(), ctx=context(), root=root, events=[], marks=[], cache=0)
            state['next_id'] += 1
            if self.kind == 'splinter':
                o['cache'] = u64(root + SPL['cache'])
                o['arms'] = dict(spit=u8(root + SPL['spit_valid']), part=u8(root + SPL['part_valid']),
                                 sm2=u8(root + SPL['sm2_valid']), cache='0x%x' % o['cache'])
                o['arms']['visor'] = u8(root + SPL['visor_valid']) if o['arms']['spit'] else 0
                o['arms']['cache_data'] = '0x%x' % u64(o['cache'] + 0x28) if o['cache'] else '0x0'
            state['owners'].append(o)
            set_members(True)
            log('dtor_enter', id=o['id'], dtor=self.kind, variant=self.variant, root='0x%x' % root,
                ctx=o['ctx'], caller=o['caller'].name() if o['caller'] else None, arms=o.get('arms'),
                forced_root=(state['ctor'] or {}).get('root') == root)
            OwnerFinish(phys, o)
        except Exception as e:
            failure('dtor entry %s %s: %r' % (self.kind, self.variant, e))
        return False


def find_owner(site_phys, ctx, rdi, label):
    """(owner, direct|tail) for an event whose caller physical frame is site_phys."""
    for o in reversed(state['owners']):
        if o['ctx'] != ctx:
            continue
        if o['frame'].is_valid() and site_phys is not None and site_phys.is_valid() \
                and o['frame'] == site_phys:
            return o, 'direct'
    tail_owner = None
    for o in reversed(state['owners']):
        t = TAIL[(o['kind'], o['variant'])]
        if o['ctx'] == ctx and label == t[0] and rdi == o['root'] and o['caller'] is not None \
                and site_phys is not None and o['caller'].is_valid() and site_phys.is_valid() \
                and o['caller'] == site_phys and not o['frame'].is_valid():
            check(tail_owner is None, 'ambiguous tail owner')
            tail_owner = o
    return (tail_owner, 'tail') if tail_owner else (None, None)


class MemberBp(gdb.Breakpoint):
    def __init__(self, spec, label):
        super().__init__(spec, internal=True)
        self.label = label
        self.enabled = False

    def stop(self):
        try:
            if not state['owners']:
                return False
            rdi = reg('rdi')
            site = gdb.newest_frame().older()
            site_phys, names = physical(site)
            o, how = find_owner(site_phys, context(), rdi, self.label)
            if o is None:
                return False
            if how == 'tail':
                stage, what = TAIL[(o['kind'], o['variant'])][1], 'tail %s(root)' % self.label
            else:
                stage, what = (splinter_stage if o['kind'] == 'splinter' else sac_stage)(
                    self.label, rdi, o, names)
            ev = dict(stage=stage, what=what, label=self.label, rdi='0x%x' % rdi, how=how,
                      off=rdi - o['root'], site_pc='0x%x' % int(site.pc()), inline=names)
            o['events'].append(ev)
            log('dtor_event', id=o['id'], **ev)
        except Exception as e:
            failure('member %s: %r' % (self.label, e))
        return False


class MarkBp(gdb.Breakpoint):
    """Inline member-dtor entry inside an owned ~CSplinter (logs entry even when the arm is disengaged)."""

    def __init__(self, spec, label, stage, offset):
        super().__init__(spec, internal=True)
        self.label, self.stage, self.offset = label, stage, offset

    def stop(self):
        try:
            if not state['owners']:
                return False
            frame = gdb.newest_frame()
            phys, names = physical(frame)
            ctx = context()
            for o in reversed(state['owners']):
                if o['kind'] == 'splinter' and o['ctx'] == ctx and o['frame'].is_valid() and o['frame'] == phys:
                    this_s, this_ok = 'no inline frame', None
                    if frame.type() == gdb.INLINE_FRAME and ('optional_object' if self.stage == 0 else 'SSurfaceAlignment') \
                            in (frame.name() or ''):
                        try:
                            this = int(frame.read_var('this'))
                            this_s, this_ok = '0x%x' % this, this == o['root'] + self.offset
                        except Exception as e:
                            this_s = 'unreadable: %s' % str(e)[:50]
                    ev = dict(stage=self.stage, what='enter ' + self.label, label=self.label, how='inline-entry',
                              this=this_s, this_ok=this_ok, pc='0x%x' % int(frame.pc()), inline=names)
                    o['marks'].append(ev)
                    log('dtor_mark', id=o['id'], **ev)
                    break
        except Exception as e:
            failure('mark %s: %r' % (self.label, e))
        return False


def expected_splinter(o):
    a, v = o['arms'], o['variant']
    exp = []
    if a['spit']:
        if a['visor']:
            exp.append((0, 'CToken::~CToken', SPL['visor']))
        exp.append((0, 'CToken::~CToken', SPL['weapon']))
    if a['part']:
        exp.append((1, 'CToken::~CToken', SPL['part']))
    if a['sm2']:
        exp.append((2, 'CToken::~CToken', SPL['sm2']))
    exp.append((3, 'SSacData::~SSacData', SPL['sac']))
    return exp


def judge(o):
    problems = []
    ev = o['events']
    stages = [e['stage'] for e in ev]
    if None in stages:
        problems.append('foreign direct events: %s' % [e['what'] for e in ev if e['stage'] is None])
    seq = [s for s in stages if s is not None]
    if seq != sorted(seq):
        problems.append('stage order %s' % seq)
    if o['kind'] == 'splinter':
        got = [(e['stage'], e['label'], e['off']) for e in ev if e['stage'] in (0, 1, 2, 3)]
        if got != expected_splinter(o):
            problems.append('members 0-3 %s != expected %s' % (got, expected_splinter(o)))
        s4 = [e for e in ev if e['stage'] == 4]
        if o['cache']:
            dels = [e for e in s4 if e['label'] == 'operator delete' and int(e['rdi'], 16) == o['cache']]
            if len(dels) != 1:
                problems.append('mCache delete count %d' % len(dels))
            if int(o['arms']['cache_data'], 16) and not [e for e in s4 if e['label'] == 'GetLockedCacheAllocationBase']:
                problems.append('cache vector storage not released')
        elif s4:
            problems.append('stage 4 events without a cache')
        if len([e for e in ev if e['what'] == 'CPatterned mDamageVulnerability']) != 1:
            problems.append('CPatterned mDamageVulnerability count')
        if len([e for e in ev if e['stage'] == 6]) != 1:
            problems.append('CAi base count')
        if o['variant'] == 'D0' and len([e for e in ev if e['stage'] == 7]) != 1:
            problems.append('final delete(root) count')
        marks = [m['stage'] for m in o['marks']]
        if marks != [0, 4]:
            problems.append('inline entries %s != [0 mSpit optional, 4 SSurfaceAlignment]' % marks)
        if any(m['this_ok'] is False for m in o['marks']):
            problems.append('inline entry this mismatch')
        # interleave: mark 0 precedes every stage>=0 event, mark 4 precedes stage 4 and follows stage 3
    else:
        want = [0, 1, 2] + ([3] if o['variant'] == 'D0' else [])
        if seq != want:
            problems.append('sac stages %s != %s' % (seq, want))
    return problems


class OwnerFinish(gdb.FinishBreakpoint):
    def __init__(self, frame, o):
        super().__init__(frame, internal=True)
        self.o = o

    def _close(self, how):
        o = self.o
        if o in state['owners']:
            state['owners'].remove(o)
        if not state['owners']:
            set_members(False)
        problems = judge(o) if how == 'returned' else ['finish %s' % how]
        status = 'OK' if not problems else 'FAIL'
        rec = dict(id=o['id'], dtor=o['kind'], variant=o['variant'], root='0x%x' % o['root'],
                   arms=o.get('arms'), status=status, problems=problems,
                   stages=[e['stage'] for e in o['events']], whats=[e['what'] for e in o['events']],
                   marks=[m['what'] for m in o['marks']],
                   forced_root=(state['ctor'] or {}).get('root') == o['root'])
        state['chains'].append(rec)
        log('chain', **rec)
        if problems:
            failure('chain %d %s' % (o['id'], problems))

    def stop(self):
        try:
            self._close('returned')
        except Exception as e:
            failure('owner finish: %r' % e)
        return False

    def out_of_scope(self):
        try:
            self._close('out_of_scope')
        except Exception as e:
            failure('owner out_of_scope: %r' % e)


MEMBER_BPS = []


def set_members(on):
    for b in MEMBER_BPS:
        b.enabled = on


class LaunchStub(gdb.Breakpoint):
    def stop(self):
        caller = gdb.newest_frame().older()
        if caller is not None and 'DoUserAnimEvent' in (caller.name() or ''):
            state['launch_stub'] += 1
            gdb.execute('set $rax = 0')
            gdb.execute('set $rip = *(unsigned long*)$rsp')
            gdb.execute('set $rsp = $rsp + 8')
        return False


# ---------------------------------------------------------------- summary / binary
def summarize(_event=None):
    ch = state['chains']
    ok = [c for c in ch if c['status'] == 'OK']

    def have(pred):
        return any(pred(c) for c in ok)

    need = {
        'splinter D0 engaged spit+visor?+part+sm2 (forced root)': have(
            lambda c: c['dtor'] == 'splinter' and c['variant'] == 'D0' and c['forced_root']
            and c['arms']['spit'] and c['arms']['part'] and c['arms']['sm2']),
        'splinter D0 all arms disengaged': have(
            lambda c: c['dtor'] == 'splinter' and c['variant'] == 'D0' and not c['arms']['spit']
            and not c['arms']['part'] and not c['arms']['sm2']),
        'sac D0': have(lambda c: c['dtor'] == 'sac' and c['variant'] == 'D0'),
        'copy ctor compared OK': (state['copy'] or {}).get('result') == 'OK',
        'force restored': bool(state['force'] and state['force']['restored']),
    }
    pending = [o['id'] for o in state['owners']]
    status = 'OK' if state['failed'] == 0 and all(need.values()) and not pending else 'FAIL'
    rec = dict(status=status, cache_chains=[c['id'] for c in ok if c['dtor'] == 'splinter'
                                            and c['arms']['cache'] != '0x0'], binary=state['binary'], installed=state['installed'], failed=state['failed'],
               chains=len(ch), chains_ok=len(ok), pending=pending, need=need,
               launch_stub=state['launch_stub'],
               chain_table=[(c['id'], c['dtor'], c['variant'], c['root'], c['status'], c['stages'])
                            for c in ch])
    log('summary', **rec)
    with open(SUMMARY, 'w') as fh:
        fh.write(json.dumps(rec, sort_keys=True, indent=1, default=str) + '\n')


def binary_sha():
    fn = gdb.current_progspace().filename
    h = hashlib.sha256()
    with open(fn, 'rb') as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b''):
            h.update(chunk)
    return '%s %s' % (h.hexdigest(), fn)


gdb.events.exited.connect(summarize)
try:
    MAIN = _main_addr()
    state['binary'] = binary_sha()
except Exception as e:
    state['binary'] = 'unavailable: %r' % e
try:
    ForceBp('CSplinter.cpp:%d' % FORCE_LINE[0], internal=True)
    CtorBp(nm_unique('CSplinter::CSplinter(TUniqueId'), internal=True)
    SpitCtorBp(nm_unique('CSplinter::SSpitData::SSpitData(float, unsigned int, float'), internal=True)
    CopyInlineBp("'CSplinter::SSpitData::SSpitData(CSplinter::SSpitData const&)'", internal=True)
    copy_tok = CopyTempTokenBp(nm_unique('CToken::~CToken()'), internal=True)
    copy_tok.enabled = False
    EntryBp(nm_addr('_ZN9CSplinterD0Ev'), 'splinter', 'D0')
    EntryBp(nm_addr('_ZN9CSplinterD2Ev'), 'splinter', 'D2')
    EntryBp(nm_addr('_ZN16CSplinterAcidSacD0Ev'), 'sac', 'D0')
    EntryBp(nm_addr('_ZN16CSplinterAcidSacD2Ev'), 'sac', 'D2')
    for prefix, label in [('CToken::~CToken()', 'CToken::~CToken'),
                          ('CSplinter::SSacData::~SSacData()', 'SSacData::~SSacData'),
                          ('GetLockedCacheAllocationBase()', 'GetLockedCacheAllocationBase'),
                          ('FreeLockedCache(void*)', 'FreeLockedCache'),
                          ('CMemory::Free(void const*)', 'CMemory::Free'),
                          ('operator delete(void*, unsigned long)', 'operator delete'),
                          ('SLdrIngPossessionData::~SLdrIngPossessionData()',
                           'SLdrIngPossessionData::~SLdrIngPossessionData'),
                          ('CBodyStateInfo::~CBodyStateInfo()', 'CBodyStateInfo::~CBodyStateInfo'),
                          ('CBodyStateCmdMgr::~CBodyStateCmdMgr()', 'CBodyStateCmdMgr::~CBodyStateCmdMgr'),
                          ('CDamageVulnerability::~CDamageVulnerability()',
                           'CDamageVulnerability::~CDamageVulnerability'),

                          ('CRELFileToken::~CRELFileToken()', 'CRELFileToken::~CRELFileToken'),
                          ]:
        MEMBER_BPS.append(MemberBp(nm_unique(prefix), label))
    # complete-object (D2) destructors: the D0 deleting ones share the demangled name
    MEMBER_BPS.append(MemberBp(nm_addr('_ZN3CAiD2Ev'), 'CAi::~CAi'))
    MEMBER_BPS.append(MemberBp(nm_addr('_ZN6CActorD2Ev'), 'CActor::~CActor'))
    MarkBp("'rstl::optional_object<CSplinter::SSpitData>::~optional_object'", 'mSpit optional dtor', 0, SPL['spit'])
    MarkBp('CSplinter::SSurfaceAlignment::~SSurfaceAlignment', 'SSurfaceAlignment dtor', 4, SPL['alignment'])
    for _a, _m in nm_syms('CPatterned::LaunchProjectile('):
        LaunchStub(at(_a), internal=True)
    state['installed'] = 1
    log('installed', binary=state['binary'], force_line=FORCE_LINE[0], members=len(MEMBER_BPS))
except Exception as e:
    failure('install: %r' % e)
