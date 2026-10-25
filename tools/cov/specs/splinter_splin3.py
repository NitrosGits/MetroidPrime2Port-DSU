# Splinter (module 74) splin3 fixture: SSpitData copy ctor (fn_74_B9DC) with the engaged mVisorEffect token
# (the Alpha Splinter, 07_temple_mothlordsanctuary layer 2, eid 0x080200ff, the only Mega placement on the disc),
# plus ~CSplinter (fn_74_ABA0) chains with a non-empty mEvadeChecks.  COV_GDB_SCRIPT for force.sh (no FORCE list:
# nothing is written to the inferior).
#
#   COV_ROOT=$PWD COV_GDB_SCRIPT=tools/cov/specs/splinter_splin3.py MP2_ALLOW_PTRACE=1 tools/cov/force.sh \
#     tools/cov/specs/splinter_splin3.py <out> 863fcd72,07_temple_mothlordsanctuary \
#     --con tools/cov/specs/splinter_splin3.con --fast --frames 700
#
# Ownership as splin2: an invocation is the PHYSICAL frame of the entry (gdb Frame equality) plus (inferior,
# thread) plus the root ($rdi at entry); callee events count only when their caller, walked up through INLINE
# frames, is that frame; tail calls through the caller frame with $rdi == root.  PCs are locations / log fields.
#
# Copy ctor: every CSplinter ctor frame is owned while it runs.  Inside it `mSpit = SSpitData(...)` builds a
# temporary (real call, snapshotted on return: fields, and for an engaged visor the CObjectReference counts) and
# copy-constructs it into the disengaged optional (inline SSpitData(const SSpitData&)).  Retail 0xBA30/0xBA3C is
# the engaged visor arm: TLockedToken(const TLockedToken&) = CToken::CToken(const CToken&) (rdi = dst +0x48,
# rsi = temp +0x48) then mToken.Lock().  The host copy ctor ends in `jmp CToken::Lock` when the source holds its
# lock (0xf390e0+0x20), so that Lock shows the ctor frame as caller with the copy call's return address as pc
# ("tail"); the TLockedToken body's own mToken.Lock() is "outer" (a no-op when the tail Lock already locked).  At the temporary's first destruction event the destination is compared
# field by field (DWARF fields, engaged optionals as their payload; the visor mLockHeld must be 1 in the copy),
# and the visor reference must show mRefCount +1 and mLockCount +1 against the snapshot.
#
# Retail ~CSplinter (ABA0) 0xAC44-0xAC90 is the mEvadeChecks reserved_vector<SEvadeCheck,4> destroy loop (no calls,
# size at retail +0xa14, host +0xb04): a chain counts for it when mCount > 0 at entry.
import hashlib
import json
import os
import re
import struct

import gdb

OUT = os.environ.get('COV_OUT', '/tmp')
JSONL = os.path.join(OUT, 'splin3.jsonl')
SUMMARY = os.path.join(OUT, 'splin3_summary.txt')
ROOT_SRC = os.environ.get('COV_ROOT', '.')

# host LP64 CSplinter offsets (DWARF): mAlignment 0xa38 (mCache single_ptr +0x50), mSac 0xb7c,
# mStateMachine2 0xbd8 (valid +0x10), mParticle 0xbf0 (mEffect valid +0x18), mSpit 0xc40 (valid +0x90),
# mEvadeChecks 0xb04 (mCount +0)
SPL = dict(alignment=0xa38, cache=0xa88, sac=0xb7c, sm2=0xbd8, sm2_valid=0xbe8, part=0xbf0,
           part_valid=0xc08, spit=0xc40, spit_end=0xcd0, spit_valid=0xcd0, visor=0xc88, visor_valid=0xca0,
           weapon=0xc50, vuln=0x3e8, evade=0xb04)
SAC = dict(rel=0x220, vuln=0x1c8)
SPIT_SIZE = 144

state = {'binary': 'unchecked', 'installed': 0, 'failed': 0, 'owners': [], 'chains': [],
         'ctor': None, 'ctors': [], 'visor_roots': set(), 'next_id': 0}


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


# ---------------------------------------------------------------- constructors / copy ctor
def u32(addr):
    return struct.unpack('<I', bytes(mem().read_memory(addr, 4)))[0]


VISOR_IN_SPIT = SPL['visor'] - SPL['spit']           # 0x48: optional<TLockedToken<CGenDescription>>
VISOR_VALID_IN_SPIT = SPL['visor_valid'] - SPL['spit']  # 0x60
TOK_HELD = 8                                          # CToken: mObjRef +0, mLockHeld +8
REF_COUNT, LOCK_COUNT = 0, 4                          # CObjectReference: mRefCount +0, mLockCount +4


def ref_counts(ref):
    return dict(ref='0x%x' % ref, refcount=u32(ref + REF_COUNT), lockcount=u32(ref + LOCK_COUNT))


def copy_bps(on):
    for b in COPY_BPS:
        b.enabled = on


def active_ctor():
    c = state['ctor']
    if c is None or c.get('frame') is None:
        return None
    return c


class CtorBp(gdb.Breakpoint):
    """CSplinter ctor entry: owns that ctor frame until it returns (FinishBreakpoint)."""

    def stop(self):
        try:
            if active_ctor() is not None:
                failure('nested CSplinter ctor while %d is active' % state['ctor']['id'])
                return False
            frame = gdb.newest_frame()
            root = reg('rdi')
            c = dict(id=len(state['ctors']), frame=frame, root=root, ctx=context(), copy=None)
            state['ctor'] = c
            state['ctors'].append(c)
            caller, names = physical(frame.older())
            log('ctor_enter', id=c['id'], root='0x%x' % root, caller=caller.name() if caller else None,
                inline=names)
            CtorFinish(frame, c)
        except Exception as e:
            failure('ctor entry: %r' % e)
        return False


class CtorFinish(gdb.FinishBreakpoint):
    def __init__(self, frame, c):
        super().__init__(frame, internal=True)
        self.c = c

    def stop(self):
        c = self.c
        try:
            root = c['root']
            arms = dict(spit_valid=u8(root + SPL['spit_valid']), visor_valid=u8(root + SPL['visor_valid']),
                        part_valid=u8(root + SPL['part_valid']), sm2_valid=u8(root + SPL['sm2_valid']))
            uid = '0x%04x' % int(gdb.parse_and_eval('((CEntity*)0x%x)->mUniqueId.value' % root))
            eid = '0x%08x' % int(gdb.parse_and_eval('((CEntity*)0x%x)->mEditorId.value' % root))
            cp = c['copy']
            c.update(uid=uid, eid=eid, arms=arms)
            if arms['visor_valid'] and not arms['spit_valid']:
                failure('ctor %d: visor valid without spit' % c['id'])
            if arms['spit_valid']:
                if cp is None or cp.get('result') is None:
                    failure('ctor %d: engaged spit never compared' % c['id'], copy=str(cp)[:200])
                elif bool(arms['visor_valid']) != bool(cp.get('visor_src')):
                    failure('ctor %d: visor arm differs from the snapshot' % c['id'])
            if arms['visor_valid']:
                state['visor_roots'].add(root)
            log('ctor_return', id=c['id'], root='0x%x' % root, uid=uid, eid=eid, arms=arms,
                copy=(cp or {}).get('result'), visor_copy=(cp or {}).get('visor_result'))
        except Exception as e:
            failure('ctor finish: %r' % e)
        c['frame'] = None
        copy_bps(False)
        return False

    def out_of_scope(self):
        self.c['frame'] = None
        copy_bps(False)
        failure('ctor %d finish out of scope' % self.c['id'])


class SpitCtorBp(gdb.Breakpoint):
    """SSpitData main ctor (the temporary) called straight from the owned CSplinter ctor frame."""

    def stop(self):
        try:
            c = active_ctor()
            if c is None:
                return False
            caller, names = physical(gdb.newest_frame().older())
            if context() != c['ctx'] or not same_frame(caller, c['frame']):
                return False
            if c['copy'] is not None:
                failure('ctor %d: second SSpitData temporary' % c['id'])
                return False
            temp = reg('rdi')
            c['copy'] = dict(temp=temp, src=None, inline_hits=[], tok_hits=[], lock_hits=[], result=None,
                             visor_src=None, visor_result=None)
            log('spit_temp_ctor', id=c['id'], temp='0x%x' % temp, inline=names)
            SpitCtorFinish(gdb.newest_frame(), c)
        except Exception as e:
            failure('spit ctor: %r' % e)
        return False


class SpitCtorFinish(gdb.FinishBreakpoint):
    def __init__(self, frame, c):
        super().__init__(frame, internal=True)
        self.c = c

    def stop(self):
        try:
            cp = self.c['copy']
            temp = cp['temp']
            cp['src'] = spit_fields(temp)
            if u8(temp + VISOR_VALID_IN_SPIT):
                ref = u64(temp + VISOR_IN_SPIT)
                cp['visor_src'] = dict(ref=ref, held=u8(temp + VISOR_IN_SPIT + TOK_HELD),
                                       counts=ref_counts(ref))
            log('spit_temp_snapshot', id=self.c['id'], temp='0x%x' % temp, fields=len(cp['src']), src=cp['src'],
                visor_src=cp['visor_src'] and dict(cp['visor_src'], ref='0x%x' % cp['visor_src']['ref']))
            copy_bps(True)
        except Exception as e:
            failure('spit snapshot: %r' % e)
        return False

    def out_of_scope(self):
        failure('spit ctor finish out of scope')


class CopyInlineBp(gdb.Breakpoint):
    """The inline SSpitData(const SSpitData&) location inside an owned ctor frame."""

    def stop(self):
        try:
            c = active_ctor()
            if c is None:
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
            if c['copy'] is not None:
                c['copy']['inline_hits'].append(hit)
            log('copy_inline_enter', id=c['id'], **hit)
        except Exception as e:
            failure('copy inline: %r' % e)
        return False


def copy_window():
    c = active_ctor()
    if c is None or c['copy'] is None or c['copy']['src'] is None or c['copy']['result'] is not None:
        return None, None
    return c, c['copy']


class TokCopyBp(gdb.Breakpoint):
    """CToken::CToken(const CToken&) into the destination mSpit during the copy window."""

    def stop(self):
        try:
            c, cp = copy_window()
            if c is None:
                return False
            rdi, rsi = reg('rdi'), reg('rsi')
            dst = c['root'] + SPL['spit']
            if not (dst <= rdi < dst + SPIT_SIZE):
                return False
            caller = gdb.newest_frame().older()
            phys, names = physical(caller)
            hit = dict(dst_off=rdi - dst, src_off=rsi - cp['temp'], rsi_ok=(rsi - cp['temp']) == (rdi - dst),
                       owned=context() == c['ctx'] and same_frame(phys, c['frame']), ret_pc=int(caller.pc()),
                       inline=names)
            cp['tok_hits'].append(hit)
            log('copy_token_ctor', id=c['id'], **hit)
        except Exception as e:
            failure('token copy: %r' % e)
        return False


class LockBp(gdb.Breakpoint):
    """CToken::Lock on the destination visor token during the copy window: "tail" (the copy ctor's
    `jmp CToken::Lock`: caller = the owned ctor frame at the visor copy call's return address, before any other
    Lock) or "outer" (the TLockedToken copy body's mToken.Lock(), caller = the owned ctor frame)."""

    def stop(self):
        try:
            c, cp = copy_window()
            if c is None:
                return False
            rdi = reg('rdi')
            if rdi != c['root'] + SPL['visor']:
                return False
            caller = gdb.newest_frame().older()
            phys, names = physical(caller)
            where = 'other'
            vtok = [h for h in cp['tok_hits'] if h['dst_off'] == VISOR_IN_SPIT]
            if context() == c['ctx'] and same_frame(phys, c['frame']):
                where = 'outer'
                if len(vtok) == 1 and not cp['lock_hits'] and int(caller.pc()) == vtok[0]['ret_pc']:
                    where = 'tail'
            hit = dict(where=where, held_before=u8(rdi + TOK_HELD), caller=phys.name(), pc='0x%x' % int(caller.pc()),
                       copy_ret_pc=('0x%x' % vtok[0]['ret_pc']) if len(vtok) == 1 else None, inline=names)
            cp['lock_hits'].append(hit)
            log('copy_token_lock', id=c['id'], **hit)
        except Exception as e:
            failure('token lock: %r' % e)
        return False


def visor_held_key(k):
    return 'mVisorEffect' in k and k.endswith('.mLockHeld')


class CopyTempTokenBp(gdb.Breakpoint):
    """First owned CToken::~CToken inside the temporary = start of its destruction, copy already done."""

    def stop(self):
        try:
            c, cp = copy_window()
            if c is None:
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
                if visor_held_key(k):
                    if not (r['snap'] == r['src'] and r['dst'] == 1):
                        bad.append(k)
                elif not (r['snap'] == r['src'] == r['dst']):
                    bad.append(k)
            extra = sorted(set(dst) ^ set(cp['src']))
            ok = not bad and not extra and dst_valid == 1 and len(cp['inline_hits']) == 1
            vrec = None
            if cp['visor_src'] is not None:
                vs = cp['visor_src']
                d = c['root'] + SPL['visor']
                now = ref_counts(vs['ref'])
                vtok = [h for h in cp['tok_hits'] if h['dst_off'] == VISOR_IN_SPIT]
                where = [h['where'] for h in cp['lock_hits']]
                held_before = [h['held_before'] for h in cp['lock_hits']]
                vrec = dict(dst_valid=u8(c['root'] + SPL['visor_valid']), dst_ref='0x%x' % u64(d),
                            src_ref='0x%x' % vs['ref'], dst_held=u8(d + TOK_HELD), src_held=vs['held'],
                            src_held_now=u8(cp['temp'] + VISOR_IN_SPIT + TOK_HELD),
                            counts_snap=vs['counts'], counts_now=now, tok_hits=vtok, lock_where=where)
                vproblems = []
                if vrec['dst_valid'] != 1:
                    vproblems.append('dst visor not engaged')
                if u64(d) != vs['ref']:
                    vproblems.append('dst mObjRef != src mObjRef')
                if vrec['dst_held'] != 1:
                    vproblems.append('dst mLockHeld != 1')
                if vrec['src_held_now'] != vs['held']:
                    vproblems.append('src mLockHeld changed')
                if now['refcount'] != vs['counts']['refcount'] + 1:
                    vproblems.append('mRefCount %d -> %d (want +1)' % (vs['counts']['refcount'], now['refcount']))
                if now['lockcount'] != vs['counts']['lockcount'] + 1:
                    vproblems.append('mLockCount %d -> %d (want +1)' % (vs['counts']['lockcount'], now['lockcount']))
                if len(vtok) != 1 or not vtok[0]['rsi_ok'] or not vtok[0]['owned']:
                    vproblems.append('visor CToken copy hits %s' % vtok)
                want = (['tail', 'outer'], [0, 1]) if vs['held'] else (['outer'], [0])
                if (where, held_before) != want:
                    vproblems.append('Lock hits %s held_before %s != %s (src held %d)'
                                     % (where, held_before, want, vs['held']))
                vrec['problems'] = vproblems
                cp['visor_result'] = 'OK' if not vproblems else 'FAIL'
                vrec['result'] = cp['visor_result']
                ok = ok and not vproblems
            cp['result'] = 'OK' if ok else 'FAIL'
            copy_bps(False)
            log('copy_compare', id=c['id'], result=cp['result'], token_this='0x%x' % rdi,
                temp_offset=rdi - cp['temp'], dst='0x%x' % (c['root'] + SPL['spit']), dst_valid=dst_valid,
                fields=len(rows), mismatched=bad, key_set_diff=extra, inline_hits=len(cp['inline_hits']),
                rows=rows, visor=vrec, inline=names)
            if not ok:
                failure('copy compare', mismatched=bad, extra=extra, visor=vrec)
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
        if not (0 <= off < SPL_SIZE) and 'single_ptr' in joined and 'CPatterned::~CPatterned' in joined:
            # the heap pointee of CPatterned::mIngScanInfo (single_ptr<TLockedToken<CScannableObjectInfo>>);
            # judge() requires the next event to free this same address
            return 5, 'CPatterned mIngScanInfo pointee token'
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
                o['arms']['evade'] = u32(root + SPL['evade'])
            state['owners'].append(o)
            set_members(True)
            log('dtor_enter', id=o['id'], dtor=self.kind, variant=self.variant, root='0x%x' % root,
                ctx=o['ctx'], caller=o['caller'].name() if o['caller'] else None, arms=o.get('arms'),
                visor_root=root in state['visor_roots'])
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
                              this=this_s, this_ok=this_ok, pc='0x%x' % int(frame.pc()), inline=names,
                              evade=u32(o['root'] + SPL['evade']))
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
        for i, e in enumerate(ev):
            if e['what'] == 'CPatterned mIngScanInfo pointee token':
                nxt = ev[i + 1] if i + 1 < len(ev) else None
                if nxt is None or nxt['label'] != 'operator delete' or nxt['rdi'] != e['rdi']:
                    problems.append('pointee token %s not freed next' % e['rdi'])
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
                   visor_root=o['root'] in state['visor_roots'])
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
COPY_BPS = []


def set_members(on):
    for b in MEMBER_BPS:
        b.enabled = on


# ---------------------------------------------------------------- summary / binary
def summarize(_event=None):
    ch = state['chains']
    ok = [c for c in ch if c['status'] == 'OK']

    def have(pred):
        return any(pred(c) for c in ok)

    ctors = [dict(id=c['id'], root='0x%x' % c['root'], uid=c.get('uid'), eid=c.get('eid'), arms=c.get('arms'),
                  copy=(c['copy'] or {}).get('result'), visor_copy=(c['copy'] or {}).get('visor_result'))
             for c in state['ctors']]
    need = {
        'copy ctor with engaged mVisorEffect compared OK (+refcount/lock readbacks)': any(
            c['copy'] == 'OK' and c['visor_copy'] == 'OK' for c in ctors),
        'splinter D0 with spit+visor engaged (visor root)': have(
            lambda c: c['dtor'] == 'splinter' and c['variant'] == 'D0' and c['visor_root']
            and c['arms']['spit'] and c['arms']['visor']),
        'splinter D0 with mEvadeChecks.mCount > 0': have(
            lambda c: c['dtor'] == 'splinter' and c['variant'] == 'D0' and c['arms']['evade'] > 0),
        'splinter D0 with spit disengaged': have(
            lambda c: c['dtor'] == 'splinter' and c['variant'] == 'D0' and not c['arms']['spit']),
    }
    pending = [o['id'] for o in state['owners']]
    ctor_open = [c['id'] for c in state['ctors'] if c.get('frame') is not None]
    status = 'OK' if state['failed'] == 0 and all(need.values()) and not pending and not ctor_open else 'FAIL'
    rec = dict(status=status, binary=state['binary'], installed=state['installed'], failed=state['failed'],
               ctors=ctors, chains=len(ch), chains_ok=len(ok), pending=pending, ctor_open=ctor_open, need=need,
               cache_chains=[c['id'] for c in ok if c['dtor'] == 'splinter' and c['arms']['cache'] != '0x0'],
               chain_table=[(c['id'], c['dtor'], c['variant'], c['root'], c['status'], c['arms'], c['stages'])
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
    SPL_SIZE = gdb.lookup_type('CSplinter').sizeof
    check(gdb.lookup_type('CSplinter::SSpitData').sizeof == SPIT_SIZE, 'SSpitData size')
    CtorBp(nm_unique('CSplinter::CSplinter(TUniqueId'), internal=True)
    SpitCtorBp(nm_unique('CSplinter::SSpitData::SSpitData(float, unsigned int, float'), internal=True)
    CopyInlineBp("'CSplinter::SSpitData::SSpitData(CSplinter::SSpitData const&)'", internal=True)
    COPY_BPS.append(CopyTempTokenBp(nm_unique('CToken::~CToken()'), internal=True))
    COPY_BPS.append(TokCopyBp(nm_unique('CToken::CToken(CToken const&)'), internal=True))
    COPY_BPS.append(LockBp(nm_unique('CToken::Lock()'), internal=True))
    copy_bps(False)
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
    state['installed'] = 1
    log('installed', binary=state['binary'], members=len(MEMBER_BPS), copy_bps=len(COPY_BPS), spl_size=SPL_SIZE)
except Exception as e:
    failure('install: %r' % e)
