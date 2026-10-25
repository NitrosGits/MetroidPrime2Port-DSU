# SwarmBasics swb16: register-only branch forcing for fn_80_50B8 CSwarmBasics::Think and fn_80_5024
# CSwarmBasics::UpdateAllBoidMovement (both out-of-line in the coverage build), loaded by tools/cov/force.py:
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swb16_think.py <out> 42b935e4,01_sand_entrance \
#     --con tools/cov/specs/swarmbasics_sand.con --fast --frames 3000
# Offsets are for the frozen coverage binary (sha256 9df97547...7e03, docs/module-analysis/SwarmBasics.md swb16);
# at load every site is disassembled and checked (mnemonic + target), any mismatch disables all forcing.
#
# Forcing writes only $eflags at a conditional jump whose flags are dead at both successors (the next
# executed instruction writes flags or is a call/ret). The original $eflags is restored at the successor
# breakpoint, before that instruction runs, and every write is read back. The two members the forced paths
# write naturally (mOccludedTimer 0x1d4 at line 769, mBoidGenCooldownTimer 0x63c at line 808) are saved as
# raw bytes when the plan starts and written back (and read back) before the activation leaves the path.
#
# Activation ownership: Think's body rsp is fixed (6 pushes + 0x2528), so CFA = body rsp + 0x2558. The entry
# breakpoint at Think+0x83 (after GetActive) opens an activation (thread, seq, CFA, this); every site must
# match thread, $rsp+0x2558 == CFA and the site's this-register == this; the epilogue Think+0x51 closes it
# (and fails the run if anything is still pending). UpdateAllBoidMovement is owned only when entered from the
# call at Think+0x595 (return address Think+0x59b) with rsp+8+0x2558 == CFA and rdi == this.
# No inferior calls in stop(): only registers, symbol addresses and raw memory reads/writes.
FORCE = []

import os
import re
import gdb

_OUT = os.environ.get('COV_OUT', '/tmp/opencode')
_LOG = os.path.join(_OUT, 'swb16.log')
_T = 'CSwarmBasics::Think(float, CStateManager&)'
_U = 'CSwarmBasics::UpdateAllBoidMovement(CStateManager&, float)'
_FN = {'T': _T, 'U': _U}
CF, ZF, SF, OF = 0x1, 0x40, 0x80, 0x800

# name: (fn, site off, jcc, taken off, fallthrough off, this register, forced outcome)
# forced outcome: True = take the jump, False = fall through, 'inv' = the opposite of the natural outcome,
# 'adapt' = the arm not yet seen (natural otherwise).
SITES = {
    'S1': ('T', 0xe1, 'je', 0x8bc, 0xe7, 'rbp', False),     # e37321 767 IsLoaded false -> occluded path (e37327 movss/pxor/comiss)
    'S2': ('T', 0x8d3, 'jne', 0xe7, 0x8d9, 'rbp', True),    # e37b13 767 occlusion state != Visible -> e37327
    'S3': ('T', 0xf6, 'jbe', 0x10f, 0xf8, 'rbp', 'inv'),    # e37336 768 mOccludedTimer > 0 (e3734f pxor+comiss / e37338 lock add)
    'S4': ('T', 0x116, 'jae', 0xa23, 0x11c, 'rbp', 'inv'),  # e37356 771 timer <= 0 return (e37c63 / e3735c lock add)
    'S5': ('T', 0x128, 'jne', 0xa31, 0x12e, 'rbp', 'adapt'),  # e37368 774 mThinkCounter & 2 (e37c71 lock add / e3736e call path)
    'S6': ('T', 0x1e9, 'jne', 0x61e, 0x1ef, 'rbp', False),  # e37429 787 !x4f1_25_ -> RemoveMaterial (e3742f mov/call)
    'S7': ('T', 0x918, 'je', 0x92f, 0x91a, 'rbp', True),    # e37b58 795 mMaxCreatedBoids == 0 (e37b6f pxor+comiss)
    'S7b': ('T', 0x93a, 'jb', 0xa8d, 0x940, 'rbp', True),   # e37b7a 796 leave the loop (e37ccd lock add), only after S7
    'S8': ('T', 0x9ab, 'jne', 0x980, 0x9ad, 'rbp', True),   # e37beb 799 boid treated active -> 807/808 (e37bc0 lock add)
    'S9': ('T', 0x86a, 'je', 0x725, 0x870, 'r12', False),   # e37aaa 835 ShouldBuild true -> 836 (e37ab0 jmp -> lock add)
    'S10': ('T', 0x760, 'jne', 0x810, 0x766, 'r12', False),  # e379a0 840 boid inactive (e379a6 mov/xor)
    'S11': ('T', 0x788, 'je', 0x7b7, 0x78a, 'r12', True),   # e379c8 843 !mActive (e379f7 and)
    'S12': ('T', 0x79f, 'ja', 0x7c5, 0x7a1, 'r12', False),  # e379df 844 mFreezeTimer >= 0.1 (e379e1 lock add)
    'S13': ('T', 0x7ba, 'je', 0x740, 0x7bc, 'r12', True),   # e379fa 845 !mLaunched -> skip UpdateBoid (e37980 lock add)
    'S14': ('T', 0x419, 'jne', 0x875, 0x41f, 'rbp', False),  # e37659 855 outlier inactive (e3765f mov/xor)
    'S15': ('T', 0x43e, 'je', 0x467, 0x440, 'rbp', True),   # e3767e 858 !mActive (e376a7 test)
    'S16': ('T', 0x455, 'ja', 0x46b, 0x457, 'rbp', False),  # e37695 858 mFreezeTimer >= 0.1 (e37697 lock add)
    'S17': ('T', 0x469, 'je', 0x3f8, 0x46b, 'rbp', True),   # e376a9 859 !mLaunched -> skip (e37638 lock add)
    'S18': ('T', 0x29b, 'jle', 0xa9b, 0x2a1, 'rbp', False),  # e374db 818 --x52c_ > 0: keep creating (e374e1 lock add)
    'U1': ('U', 0x14, 'je', 0xc0, 0x1a, 'rdi', True),       # e31424 883 !mAnimated -> ret (e314d0 ret)
    'U2': ('U', 0x3b, 'je', 0xa7, 0x3d, 'rdi', True),       # e3144b 885 count == 0 -> epilogue (e314b7 lock add)
}
OCC = [('S1',), ('S2',), ('S1', 'S3'), ('S2', 'S3'), ('S1', 'S4'), ('S2', 'S4'), ('S1', 'S3', 'S4'), ('S2', 'S3', 'S4')]
OCC_SEQ = {600 + 97 * k: OCC[k % len(OCC)] for k in range(2 * len(OCC))}
ROT = ['S6', 'S7', 'S9', 'S10', 'S11', 'S12', 'S13', 'S14', 'S15', 'S16', 'S17', 'U1', 'U2']
BUDGET, GAP = 3, 40
OCC_RESTORE = (0x12e, 0xa23, 0xa31)   # e3736e (774 continue), e37c63 (771 return), e37c71 (774 return)
# EXIT T+0x5a (e3729a): the GetActive-false path (T+0x51) and the main return (jmp at T+0x619) both pass it
ENTRY, EXIT, S8ARM, S8DONE, URET = 0x83, 0x5a, 0x940, 0xa1e, 0x59b
S18DONE = 0x2bf      # e374ff: both exits of the 814 create loop (T+0x2b6 end, T+0xa9b break) join here

_st = dict(seq=0, opens=0, closes=0, unattributed=0, forced={}, natural={}, flagRestored=0, memRestored=0,
           budget={k: BUDGET for k in SITES}, last={k: -10 ** 9 for k in SITES}, rot=0, s8last=-10 ** 9,
           fail=[], s5seen=set(), uOwned=0, uForeign=0, disabled=False)
_acts = {}      # thread global_num -> open activation
_pend = {}      # thread global_num -> pending eflags restore
_base = {}


def _log(msg):
    with open(_LOG, 'a') as fh:
        fh.write(msg + '\n')


def _fail(reason):
    _st['fail'].append(reason)
    _log('FAIL ' + reason)


def _reg(name):
    return int(gdb.parse_and_eval('$' + name)) & 0xffffffffffffffff


def _addr(fn):
    if fn not in _base:
        _base[fn] = int(gdb.parse_and_eval("(long)&'%s'" % _FN[fn]))
    return _base[fn]


def _tid():
    th = gdb.selected_thread()
    if th is None:
        raise RuntimeError('no thread')
    return th.global_num


def _mem(addr, n):
    return bytes(gdb.selected_inferior().read_memory(addr, n))


def _put(addr, data, what):
    gdb.selected_inferior().write_memory(addr, data)
    if _mem(addr, len(data)) != data:
        _fail('readback ' + what)
        return False
    _st['memRestored'] += 1
    return True


def _taken(jcc, fl):
    c, z = bool(fl & CF), bool(fl & ZF)
    if jcc == 'jle':
        return z or bool(fl & SF) != bool(fl & OF)
    return {'je': z, 'jne': not z, 'jbe': c or z, 'jae': not c, 'ja': not c and not z, 'jb': c}[jcc]


def _flags_for(jcc, fl, want):
    # smallest change of CF/ZF that gives outcome `want`
    if _taken(jcc, fl) == want:
        return fl
    for cand in (fl ^ ZF, fl ^ CF, fl ^ CF ^ ZF, fl & ~(CF | ZF), fl | CF, fl | ZF, fl | CF | ZF,
                 fl & ~(ZF | SF | OF)):
        if _taken(jcc, cand) == want:
            return cand
    raise RuntimeError('no flags for ' + jcc)


def _owned(name):
    fn, off, _, _, _, reg, _ = SITES[name] if name in SITES else (None,) * 7
    tid = _tid()
    act = _acts.get(tid)
    if act is None:
        return None
    rsp = _reg('rsp')
    if fn == 'U':
        u = act.get('u')
        want = None if u is None else (u if off == 0x14 else u - 0x38)
        if want is None or rsp != want or _reg('rdi') != act['this']:
            return None
        return act
    if rsp + 0x2558 != act['cfa'] or _reg(reg or 'rbp') != act['this']:
        return None
    return act


def _count(table, key):
    table[key] = table.get(key, 0) + 1


def _force(name, act):
    fn, off, jcc, toff, foff, reg, want = SITES[name]
    fl = _reg('eflags')
    nat = _taken(jcc, fl)
    if want == 'inv':
        want = not nat
    elif want == 'adapt':
        want = (not nat) if (nat in _st['s5seen'] and (not nat) not in _st['s5seen']) else nat
    new = _flags_for(jcc, fl, want)
    if new != fl:
        gdb.execute('set var $eflags = %d' % new)
        rb = _reg('eflags')
        if (rb & (CF | ZF)) != (new & (CF | ZF)):
            _fail('%s eflags readback %#x != %#x' % (name, rb, new))
            gdb.execute('set var $eflags = %d' % fl)
            return
        _pend[_tid()] = dict(name=name, fn=fn, expect=toff if want else foff, old=fl, seq=act['seq'])
        _count(_st['forced'], name)
    else:
        _count(_st['natural'], name + ('=taken' if nat else '=fall'))
    if name == 'S5':
        _st['s5seen'].add(want)
    _log('seq %d %s %s+%#x %s natural=%s forced=%s eflags %#x->%#x' % (
        act['seq'], name, fn, off, jcc, nat, want, fl, new))


def _restore_flags(fn, off):
    p = _pend.get(_tid())
    if p is None:
        return
    del _pend[_tid()]
    if p['fn'] != fn or p['expect'] != off:
        _fail('seq %d %s: next stop %s+%#x, expected %s+%#x' % (p['seq'], p['name'], fn, off, p['fn'], p['expect']))
    gdb.execute('set var $eflags = %d' % p['old'])
    rb = _reg('eflags')
    if (rb & 0xcd5) != (p['old'] & 0xcd5):
        _fail('%s eflags restore readback %#x != %#x' % (p['name'], rb, p['old']))
    else:
        _st['flagRestored'] += 1


def _plan(seq):
    if seq in OCC_SEQ:
        return list(OCC_SEQ[seq]) + ['S5']
    if seq >= 200 and seq % 7 == 0:
        for i in range(len(ROT)):
            k = ROT[(_st['rot'] + i) % len(ROT)]
            if _st['budget'][k] > 0 and seq - _st['last'][k] >= GAP:
                _st['rot'] = (_st['rot'] + i + 1) % len(ROT)
                return [k] + (['S7b'] if k == 'S7' else [])
    return []


def _on_entry():
    tid = _tid()
    if tid in _acts:
        _fail('seq %d: activation still open at a new entry' % _acts[tid]['seq'])
    _st['seq'] += 1
    seq = _st['seq']
    plan = _plan(seq)
    s8 = ('S7' not in plan and seq not in OCC_SEQ and seq >= 200 and
          _st['budget']['S8'] > 0 and seq - _st['s8last'] >= GAP)
    if seq % 2000 == 0:
        _summary()
    _acts[tid] = dict(seq=seq, cfa=_reg('rsp') + 0x2558, this=_reg('rdi'), plan=set(plan), done=set(),
                      s8=s8, s8armed=False, occ=None, cool=None, u=None, x52c=None)
    _st['opens'] += 1
    if plan:
        _log('seq %d open this=%#x cfa=%#x plan=%s' % (seq, _reg('rdi'), _reg('rsp') + 0x2558, plan))


def _on_exit():
    tid = _tid()
    act = _acts.get(tid)
    if act is None or _reg('rsp') + 0x2558 != act['cfa']:
        return  # GetActive() false: no activation was opened
    del _acts[tid]
    _st['closes'] += 1
    if act['occ'] is not None or act['cool'] is not None or act['x52c'] is not None:
        _fail('seq %d closed with a pending member restore' % act['seq'])
    if tid in _pend:
        _fail('seq %d closed with a pending eflags restore' % act['seq'])
    if act['plan']:
        _log('seq %d close done=%s' % (act['seq'], sorted(act['done'])))


def _on_site(name):
    act = _owned(name)
    if act is None:
        if _tid() in _acts and SITES[name][0] == 'T':
            _st['unattributed'] += 1
            _fail('%s: Think activation open but ownership mismatch' % name)
        return
    if name == 'S8':
        if act['s8armed']:
            _force(name, act)
        return
    if name == 'S18':
        if _st['budget']['S18'] > 0 and act['x52c'] is None and _taken('jle', _reg('eflags')):
            # 818 just stored the decremented x52c_ (0x7f8) and counted the boid (mCreatedBoids 0x7a4)
            act['x52c'] = (_mem(act['this'] + 0x7f8, 4), _mem(act['this'] + 0x7a4, 4))
            _force(name, act)
            _st['budget']['S18'] -= 1
            act['done'].add(name)
        return
    if name not in act['plan'] or name in act['done']:
        return
    if name == 'S7b' and 'S7' not in act['done']:
        return
    if name in ('S1', 'S2'):
        act['occ'] = _mem(act['this'] + 0x1d4, 4)
    if name in ('S3', 'S4', 'S5') and act['occ'] is None:
        return  # only on the forced occluded path
    _force(name, act)
    act['done'].add(name)
    if name in ROT:
        _st['budget'][name] -= 1
        _st['last'][name] = act['seq']


def _on_occ_restore(off):
    act = _owned('S1')
    if act is None or act['occ'] is None:
        return
    cur = _mem(act['this'] + 0x1d4, 4)
    if _put(act['this'] + 0x1d4, act['occ'], 'mOccludedTimer'):
        _log('seq %d mOccludedTimer %s -> %s at T+%#x' % (act['seq'], cur.hex(), act['occ'].hex(), off))
    act['occ'] = None


def _on_s8arm():
    act = _owned('S8')
    if act is None or not act['s8'] or act['s8armed']:
        return
    act['s8armed'] = True
    act['cool'] = _mem(act['this'] + 0x63c, 4)
    _st['budget']['S8'] -= 1
    _st['s8last'] = act['seq']
    _log('seq %d S8 armed mBoidGenCooldownTimer=%s count=%d' % (
        act['seq'], act['cool'].hex(), int.from_bytes(_mem(act['this'] + 0x1dc, 4), 'little')))


def _on_s8done():
    act = _owned('S8')
    if act is None or act['cool'] is None:
        return
    cur = _mem(act['this'] + 0x63c, 4)
    if _put(act['this'] + 0x63c, act['cool'], 'mBoidGenCooldownTimer'):
        _log('seq %d mBoidGenCooldownTimer %s -> %s at T+%#x' % (act['seq'], cur.hex(), act['cool'].hex(), S8DONE))
    act['cool'] = None
    act['done'].add('S8')
    act['s8armed'] = False


def _on_s18done():
    act = _owned('S18')
    if act is None or act['x52c'] is None:
        return
    cur = (_mem(act['this'] + 0x7f8, 4), _mem(act['this'] + 0x7a4, 4))
    ok = _put(act['this'] + 0x7f8, act['x52c'][0], 'x52c_')
    ok &= _put(act['this'] + 0x7a4, act['x52c'][1], 'mCreatedBoids')
    if ok:
        _log('seq %d x52c_/mCreatedBoids %s/%s -> %s/%s at T+%#x' % (
            act['seq'], cur[0].hex(), cur[1].hex(), act['x52c'][0].hex(), act['x52c'][1].hex(), S18DONE))
    act['x52c'] = None


def _on_uentry():
    act = _acts.get(_tid())
    rsp = _reg('rsp')
    ret = int.from_bytes(_mem(rsp, 8), 'little')
    if act is None or ret != _addr('T') + URET or rsp + 8 + 0x2558 != act['cfa'] or _reg('rdi') != act['this']:
        _st['uForeign'] += 1
        return
    act['u'] = rsp
    _st['uOwned'] += 1


class _Bp(gdb.Breakpoint):
    def __init__(self, fn, off):
        super().__init__("*'%s'+%d" % (_FN[fn], off), internal=True)
        self.fn, self.off, self.handlers = fn, off, []

    def stop(self):
        if _st['disabled']:
            return False
        try:
            pc = _reg('pc')
            if pc != _addr(self.fn) + self.off:
                _fail('pc %#x != %s+%#x' % (pc, self.fn, self.off))
                return False
            _restore_flags(self.fn, self.off)
            for h in self.handlers:
                h()
        except Exception as e:  # any uncertainty fails the run summary
            _fail('%s+%#x: %r' % (self.fn, self.off, e))
        return False


_bps = {}


def _at(fn, off, h):
    if (fn, off) not in _bps:
        _bps[(fn, off)] = _Bp(fn, off)
    if h is not None:
        _bps[(fn, off)].handlers.append(h)


def _verify():
    ok = True
    for name, (fn, off, jcc, toff, foff, _, _) in sorted(SITES.items()):
        base = _addr(fn)
        txt = gdb.execute('x/2i %d' % (base + off), to_string=True).splitlines()
        f = txt[0].split(':\t', 1)[1].split()
        nxt = int(re.search(r'0x[0-9a-f]+', txt[1]).group(0), 16)
        good = (int(re.search(r'0x[0-9a-f]+', txt[0]).group(0), 16) == base + off and f[0] == jcc and
                int(f[1], 16) == base + toff and nxt == base + foff)
        _log('verify %-4s %s+%#x %s %s -> %s fall %#x %s' % (name, fn, off, f[0], f[1], hex(base + toff),
                                                             nxt, 'OK' if good else 'MISMATCH'))
        ok &= good
    if not ok:
        _st['disabled'] = True
        _fail('static site verification failed: forcing disabled')
    _base.clear()  # PIE: recompute after relocation


def _summary(_ev=None):
    s = dict(_st)
    s['s5seen'] = sorted(s['s5seen'])
    s['open'] = len(_acts)
    s['pending'] = len(_pend)
    _log('SUMMARY ' + repr(s))
    _log('STATUS ' + ('FAIL' if _st['fail'] or _st['disabled'] else 'PASS'))


_verify()
_at('T', ENTRY, _on_entry)
_at('T', EXIT, _on_exit)
for _n, (_fn, _off, _j, _toff, _foff, _r, _w) in SITES.items():
    _at(_fn, _off, (lambda n: lambda: _on_site(n))(_n))
    _at(_fn, _toff, None)
    _at(_fn, _foff, None)
for _o in OCC_RESTORE:
    _at('T', _o, (lambda o: lambda: _on_occ_restore(o))(_o))
_at('T', S8ARM, _on_s8arm)
_at('T', S8DONE, _on_s8done)
_at('T', S18DONE, _on_s18done)
_at('U', 0, _on_uentry)
gdb.events.exited.connect(_summary)
_log('loaded: %d breakpoints, occlusion plans at %s' % (len(_bps), sorted(OCC_SEQ)))
