"""amt3: forced arms of CAIMannedTurret UpdateAim/Attack/Fire (REL 1), the gdb side and the spec.

    COV_GDB_SCRIPT=$PWD/tools/cov/specs/amt3_force.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
      tools/cov/specs/amt3_force.py <out> 42b935e4,06_sand_alamofront \
      --con tools/drivers/aimannedturret_alamofront.con --fast --frames 14000
    static dry run (no inferior): AMT3_DRY=1 COV_NM=<nm.txt> gdb -batch -nx <binary> -ex 'source <this>'

Arms the alamofront fight never takes (natural run /tmp/opencode/amt3/run_nat), forced on the one
turret AI the fight drives (the first `this` UpdateAim sees), inside the frame armed at the
body's entry (gdb.newest_frame() equality); originals are recorded, every write is read back,
and the touched members are written back (and read back) when that frame returns. No inferior
calls: every stop() only reads and writes registers and memory.

  UpdateAim (fn_1_1F28)
    snap   entry $rdx (bool snap, copied to r13d at +50) := 1     -> 429-431 LookAt(2pi), 463 !snap
    dot1   +353 comiss |dot|,1e-4: xmm0 (dot) and xmm1 (|dot|) := 0 -> 421-423 false arm (aim
           perpendicular to the turret's forward: heading stays the turret's forward)
    dot2   +543 the same for the heading/base dot                  -> 436-438 false arm (no SetTransform)
    limits entry mData.maxHorizRotationLeft/Right := 0             -> 441/445 angle > limit
    elow   entry mElevation := -1000                               -> 470 Clamp -180 arm, 477 low clamp
    ehigh  entry mElevation := 1000                                -> 472 Clamp 180 arm, 477 high clamp
    over   entry mElevation := 170, mData.vertSpeed := 1e6         -> 472 Clamp target arm (overshoot)
  Attack (fn_1_2C6C)
    limit  +896 comiss |dot|,1.0: xmm0 := copysign(1.0000001, dot), xmm1 := 1.0000001
           (the Dot of two unit vectors rounding past 1)           -> 297 Limit |v| > 1 arm
  Fire (fn_1_CF0)
    unloaded +82 je after cmpq $0,0x20(token obj): ZF := 1 (the token's object not loaded) -> 604
    noanim   +480 je after test anim data: ZF := 1 (the turret has no anim data)          -> 615
"""
import gdb
import json
import os
import struct

CLASS = 'CAIMannedTurret'
FORCE = []

UPDATE = '_ZN15CAIMannedTurret9UpdateAimER13CStateManagerfb'
ATTACK = '_ZN15CAIMannedTurret6AttackER13CStateManager9EStateMsgf'
FIRE = '_ZN15CAIMannedTurret4FireER13CStateManager'
# (symbol, offset, expected instruction text): checked statically before the run and at install.
SITES = [
    (UPDATE, 0, 'endbr64'),
    (UPDATE, 8, 'mov    %rdi,%r14'),
    (UPDATE, 50, 'mov    %edx,%r13d'),
    (UPDATE, 353, 'comiss -0x478adc(%rip),%xmm1'),
    (UPDATE, 360, 'jae'),
    (UPDATE, 543, 'comiss -0x478b9a(%rip),%xmm1'),
    (UPDATE, 550, 'jae'),
    (ATTACK, 0, 'endbr64'),
    (ATTACK, 773, 'mov    0x8(%rsp),%rcx'),
    (ATTACK, 896, 'comiss -0x47d79b(%rip),%xmm1'),
    (ATTACK, 903, 'jbe'),
    (FIRE, 0, 'endbr64'),
    (FIRE, 77, 'cmpq   $0x0,0x20(%rax)'),
    (FIRE, 82, 'je'),
    (FIRE, 477, 'test   %rcx,%rcx'),
    (FIRE, 480, 'je'),
]
UPDATE_PLAN = ['snap', 'dot1', 'dot2', 'limits', 'elow', 'ehigh', 'over'] * 3 + ['limits'] * 3
UPDATE_START, UPDATE_EVERY = 600, 150
ATTACK_PLAN = ['limit'] * 3
FIRE_PLAN = [None, None, 'unloaded', None, 'unloaded', None, 'noanim', None, 'noanim']

state = {'this': None, 'calls': {'U': 0, 'A': 0, 'F': 0}, 'plan': {'U': 0, 'A': 0, 'F': 0},
         'armed': {}, 'watch': 0}

def log(event, **data):
    with open(os.path.join(os.environ['COV_OUT'], 'amt3.jsonl'), 'a') as f:
        f.write(json.dumps(dict(event=event, **data), sort_keys=True) + '\n')

def ev(s):
    return gdb.parse_and_eval(s)

def fail(label):
    log('FAIL', label=label)
    raise gdb.GdbError(label)

def check_sites():
    for sym, off, text in SITES:
        line = gdb.execute('x/i %s+%d' % (sym, off), to_string=True)
        ok = text in line
        log('SITE', symbol=sym, offset=off, expect=text, insn=line.strip().split(':', 1)[-1].strip(),
            ok=ok)
        if not ok:
            fail('site %s+%d' % (sym, off))
    # host offsets as the disassembly uses them (0x3a8/0x3ac at UpdateAim+1945/+1698, 0x3c0 at +778)
    for member, expect in [('mElevation', 0x578), ('mTargetElevation', 0x574),
                           ('mData.maxHorizRotationLeft', 0x3a8),
                           ('mData.maxHorizRotationRight', 0x3ac), ('mData.vertSpeed', 0x3c0)]:
        off = int(ev('(long)&((%s*)0)->%s' % (CLASS, member)))
        log('MEMBER', member=member, offset=off, expect=expect)
        if off != expect:
            fail('member offset ' + member)

def f32(x):
    return struct.unpack('<f', struct.pack('<f', x))[0]

class Frame(gdb.FinishBreakpoint):
    """Restores the members written in this frame when the armed body returns (or unwinds)."""
    def __init__(self, key, this, scenario, saved):
        super().__init__(gdb.newest_frame(), internal=True)
        self.key, self.this, self.scenario, self.saved = key, this, scenario, saved

    def restore(self, how):
        inf = gdb.selected_inferior()
        rows, errors = [], []
        for lv, address, original in reversed(self.saved):
            inf.write_memory(address, original)
            back = bytes(inf.read_memory(address, len(original)))
            rows.append(dict(lvalue=lv, original=original.hex(), readback=back.hex()))
            if back != original:
                errors.append(lv)
        if self.scenario == 'watch' and not rows:
            state['watch'] += 1  # Attack call that did not reach +896: nothing written, no log line
        else:
            log('RESTORE', body=self.key, scenario=self.scenario, how=how, fields=rows,
                errors=errors)
        state['armed'].pop(self.key, None)
        if errors:
            fail('restore mismatch ' + str(errors))

    def stop(self):
        self.restore('return')
        return False

    def out_of_scope(self):
        self.restore('unwind')

def write_member(this, member, value, saved):
    lv = '((%s*)%d)->%s' % (CLASS, this, member)
    v = ev(lv)
    address, size = int(v.address), v.type.sizeof
    inf = gdb.selected_inferior()
    original = bytes(inf.read_memory(address, size))
    saved.append((lv, address, original))
    inf.write_memory(address, struct.pack('<f', value))
    back = struct.unpack('<f', bytes(inf.read_memory(address, size)))[0]
    log('WRITE', lvalue=lv, original=struct.unpack('<f', original)[0], value=value, readback=back)
    if back != f32(value):
        fail('readback ' + lv)

def write_reg(reg, value, scenario):
    original = ev(reg)
    if reg.startswith('$xmm'):
        gdb.execute('set var %s = %r' % (reg, value), to_string=True)
        back = float(ev(reg))
        ok = back == f32(value)
        original = float(original)
    else:
        gdb.execute('set var %s = %d' % (reg, value), to_string=True)
        back = int(ev(reg))
        ok = back == value
        original = int(original)
    log('REG', scenario=scenario, reg=reg, original=original, value=value, readback=back)
    if not ok:
        fail('readback ' + reg)

def outer_frame():
    # Attack+896 is inside inlined CMath::Limit: compare the real (non-inline) frame.
    f = gdb.newest_frame()
    while f is not None and f.type() == gdb.INLINE_FRAME:
        f = f.older()
    return f

def owner(key):
    armed = state['armed'].get(key)
    return armed if armed and armed.is_valid() and outer_frame() == armed.frame_ else None

class Entry(gdb.Breakpoint):
    def __init__(self, key, sym):
        super().__init__('*' + sym, internal=True)
        self.key = key

    def stop(self):
        this = int(ev('$rdi'))
        if state['this'] is None and self.key == 'U':
            state['this'] = this
            log('FILTER', this=this)
        if this != state['this']:
            return False
        state['calls'][self.key] += 1
        n = state['calls'][self.key]
        scenario = None
        if self.key == 'U' and n >= UPDATE_START and (n - UPDATE_START) % UPDATE_EVERY == 0:
            i = state['plan']['U']
            if i < len(UPDATE_PLAN):
                scenario, state['plan']['U'] = UPDATE_PLAN[i], i + 1
        elif self.key == 'A' and state['plan']['A'] < len(ATTACK_PLAN):
            scenario = 'watch'
        elif self.key == 'F':
            i = state['plan']['F']
            if i < len(FIRE_PLAN):
                scenario, state['plan']['F'] = FIRE_PLAN[i], i + 1
        if scenario is None:
            return False
        saved = []
        if scenario != 'watch':
            log('ARM', body=self.key, call=n, scenario=scenario, this=this)
        if scenario == 'snap':
            write_reg('$rdx', 1, scenario)
        elif scenario == 'limits':
            write_member(this, 'mData.maxHorizRotationLeft', 0.0, saved)
            write_member(this, 'mData.maxHorizRotationRight', 0.0, saved)
        elif scenario == 'elow':
            write_member(this, 'mElevation', -1000.0, saved)
        elif scenario == 'ehigh':
            write_member(this, 'mElevation', 1000.0, saved)
        elif scenario == 'over':
            write_member(this, 'mElevation', 170.0, saved)
            write_member(this, 'mData.vertSpeed', 1e6, saved)
        # Elevation state is always written back at return, also for the register scenarios.
        if self.key == 'U':
            for member in ['mElevation', 'mTargetElevation']:
                if not any(lv.endswith('->' + member) for lv, _, _ in saved):
                    v = ev('((%s*)%d)->%s' % (CLASS, this, member))
                    saved.append(('((%s*)%d)->%s' % (CLASS, this, member), int(v.address),
                                  bytes(gdb.selected_inferior().read_memory(int(v.address), 4))))
        fb = Frame(self.key, this, scenario, saved)
        fb.frame_ = outer_frame()
        state['armed'][self.key] = fb
        return False

class Mid(gdb.Breakpoint):
    def __init__(self, key, sym, off, scenario, regs_this):
        super().__init__('*%s+%d' % (sym, off), internal=True)
        self.key, self.off, self.scenario, self.regs_this = key, off, scenario, regs_this

    def stop(self):
        armed = owner(self.key)
        if armed is None or int(ev(self.regs_this)) != armed.this:
            return False
        if self.key == 'A':
            i = state['plan']['A']
            if i >= len(ATTACK_PLAN):
                return False
            state['plan']['A'] = i + 1
            armed.scenario = ATTACK_PLAN[i]
        elif armed.scenario != self.scenario:
            return False
        log('FORCE', body=self.key, offset=self.off, scenario=self.scenario, this=armed.this)
        if self.scenario in ('dot1', 'dot2'):
            write_reg('$xmm0.v4_float[0]', 0.0, self.scenario)
            write_reg('$xmm1.v4_float[0]', 0.0, self.scenario)
        elif self.scenario == 'limit':
            dot = float(ev('$xmm0.v4_float[0]'))
            write_reg('$xmm0.v4_float[0]', 1.0000001 if dot >= 0 else -1.0000001, self.scenario)
            write_reg('$xmm1.v4_float[0]', 1.0000001, self.scenario)
        elif self.scenario in ('unloaded', 'noanim'):
            write_reg('$eflags', int(ev('$eflags')) | 0x40, self.scenario)
        return False

def finished(event):
    log('EXIT', calls=state['calls'], plan=state['plan'], watch_unforced=state['watch'])

def install():
    check_sites()
    if os.environ.get('AMT3_DRY'):
        log('DRY', sites=len(SITES))
        return
    Entry('U', UPDATE)
    Entry('A', ATTACK)
    Entry('F', FIRE)
    Mid('U', UPDATE, 353, 'dot1', '$r14')
    Mid('U', UPDATE, 543, 'dot2', '$r14')
    Mid('A', ATTACK, 896, 'limit', '$rcx')
    Mid('F', FIRE, 82, 'unloaded', '$rbx')
    Mid('F', FIRE, 480, 'noanim', '$rbx')
    gdb.events.exited.connect(finished)

install()
