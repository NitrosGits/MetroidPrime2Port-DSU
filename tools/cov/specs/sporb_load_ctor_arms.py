# sporb7: real loader replays, natural constructors, and tendril ABI variants.
# force_ret.py; hall 1200 frames. SPORB7_CASE=0..3 selects ctor boundaries.
import gdb
import math
import os
import struct

CLASS = 'CSporbBase'
selected = int(os.environ.get('SPORB7_CASE', '0'))
captured = {}
base_replays = 0
active = None
symbols = []
addresses = {}
for line in open(os.environ['COV_NM']):
    raw, _, name = line.rstrip().partition('\t')
    words = raw.split()
    if len(words) == 3 and words[1] in 'tTW' and '.cold' not in name:
        symbols.append((words[2], name))
        addresses[words[2]] = int(words[0], 16)


def symbol(prefix):
    return next(m for m, d in symbols if d.startswith(prefix))


def exact(mangled):
    # GDB's DWARF lookup conflates destructor ABI aliases; select nm's address.
    return '($sp7_bias+%d)' % addresses[mangled]


def log(text):
    with open(os.path.join(os.environ['COV_OUT'], 'forced.log'), 'a') as out:
        out.write(text + '\n')


def val(expr):
    return gdb.parse_and_eval(expr)


def assign(expr, value):
    if isinstance(value, float) and math.isnan(value):
        gdb.set_convenience_variable('sp7_nan', gdb.Value(value))
        value = '$sp7_nan'
    gdb.execute('set var %s = %s' % (expr, value))


class Capture(gdb.Breakpoint):
    def __init__(self, name):
        super().__init__('*' + symbol('LoadSporb' + name + '('), internal=True)
        self.name = name

    def stop(self):
        if self.name not in captured:
            stream = val('(CInputStream*)$rsi')
            pos = int(stream['mPtr'])
            inferior = gdb.selected_inferior()
            count = struct.unpack('>H', inferior.read_memory(pos, 2))[0]
            end = pos + 2
            ids = []
            for _ in range(count):
                pid, size = struct.unpack('>IH', inferior.read_memory(end, 6))
                ids.append('%08x' % pid)
                end += 6 + size
            data = bytes(inferior.read_memory(pos, end - pos))
            info = bytes(inferior.read_memory(int(val('$rdx')), gdb.lookup_type('CEntityInfo').sizeof))
            captured[self.name] = (data, info)
            log('capture %s %d bytes %s' % (self.name, len(data), ','.join(ids)))
        return False


for name in ('Base', 'Top', 'Projectile', 'Needle'):
    Capture(name)


class Prepare(gdb.Function):
    def __init__(self):
        super().__init__('sp7_prepare')

    def invoke(self, buffer, info, index):
        global base_replays
        names = ('Base', 'Top', 'Projectile', 'Needle')
        name = names[int(index) // 2]
        data, original_info = captured[name]
        if int(index) % 2:
            # An unknown property, then an empty stream: invalid default model.
            data = struct.pack('>HIH', 1, 0x12345678, 3) + b'xyz'
        else:
            count = struct.unpack('>H', data[:2])[0]
            data = struct.pack('>H', count + 1) + data[2:] + struct.pack('>IH', 0x12345678, 3) + b'xyz'
            if name == 'Base':
                base_replays += 1
                data = bytearray(data)
                cursor = 2
                for _ in range(count):
                    pid, size = struct.unpack('>IH', data[cursor:cursor + 6])
                    if pid == 0xb628855a:
                        data[cursor + 6] = 1  # 3220/322C: build all four stages.
                    if pid == 0x33868c8f and base_replays == 3:
                        data[cursor + 6:cursor + 10] = b'\xff' * 4
                    cursor += 6 + size
        inferior = gdb.selected_inferior()
        inferior.write_memory(int(buffer), data)
        inferior.write_memory(int(info), original_info)
        # Borrow no expired vector storage from the natural loader's stack.
        assign('((CEntityInfo*)%d)->mConnections.mCount' % int(info), 0)
        assign('((CEntityInfo*)%d)->mConnections.mCapacity' % int(info), 0)
        assign('((CEntityInfo*)%d)->mConnections.mItems' % int(info), 0)
        return len(data)


class Check(gdb.Function):
    def __init__(self):
        super().__init__('sp7_check')

    def invoke(self, index, passed):
        log('loader case%d %s' % (int(index), 'PASS' if bool(passed) else 'FAIL'))
        if not bool(passed):
            raise gdb.GdbError('loader differs from retail')
        return 1


Prepare()
Check()
scenarios = []
for index in range(8):
    name = ('Base', 'Top', 'Projectile', 'Needle')[index // 2]
    ordinary_cleanup = '0' if index % 2 else '((void(*)(void*))%s)($sp7_result)' % exact(symbol('CSporb%s::~CSporb%s()' % (name, name)).replace('D1', 'D0').replace('D2', 'D0'))
    cleanup = ordinary_cleanup
    if index == 0:
        # The actual base ctor made this real tendril. Exercise D2, release its
        # storage, and detach before the base's normal deleting destruction.
        cleanup = ('($sp7_gen=((CSporbBase*)$sp7_result)->mTendrilGen.mPtr,'
                   '((void(*)(void*))%s)($sp7_gen),(void)free($sp7_gen),'
                   '((CSporbBase*)$sp7_result)->mTendrilGen.mPtr=0,%s)') % (
                       exact(symbol('CTendrilGen::~CTendrilGen()').replace('D0', 'D2').replace('D1', 'D2')), cleanup)
    expr = ('($sp7_buffer=(char*)malloc(65536),$sp7_info=(CEntityInfo*)malloc(sizeof(CEntityInfo)),'
            '$sp7_stream=(CInputStream*)malloc(sizeof(CInputStream)),'
            '$sp7_length=$sp7_prepare($sp7_buffer,$sp7_info,%d),'
            '((void(*)(void*,void*,unsigned long))%s)($sp7_stream,$sp7_buffer,$sp7_length),'
            '$sp7_result=LoadSporb%s(*$mgr,*$sp7_stream,*$sp7_info),'
            '$sp7_check(%d,($sp7_result!=0)==%d),%s,'
            '((void(*)(void*))%s)($sp7_stream),'
            '(void)free($sp7_stream),(void)free($sp7_buffer),(void)free($sp7_info),1)') % (
                index, symbol('CInputStream::CInputStream(void const*, unsigned long)'), name,
                index, 0 if index % 2 else 1, cleanup,
                exact(symbol('CInputStream::~CInputStream()').replace('D0', 'D2').replace('D1', 'D2')))
    scenarios.append(([], expr))
    if index == 0:
        deleting = cleanup.replace(exact(symbol('CTendrilGen::~CTendrilGen()').replace('D0', 'D2').replace('D1', 'D2')),
                                   exact(symbol('CTendrilGen::~CTendrilGen()').replace('D1', 'D0').replace('D2', 'D0')))
        deleting = deleting.replace('(void)free($sp7_gen),', '')
        scenarios.append(([], expr.replace(cleanup, deleting)))
        scenarios.append(([], expr.replace(cleanup, ordinary_cleanup)))

CALLS = [('sporb7 loader suite', 'CSporbBase::Think(',
          '$sp7_ready', scenarios, 'valid shipped property replays, unknown/default and invalid-model arms')]
gdb.execute('set $sp7_ready=0')


class Ready(gdb.Breakpoint):
    def stop(self):
        assign('$sp7_bias', '(long)&LoadSporbBase-%d' % addresses[symbol('LoadSporbBase(')])
        assign('$sp7_ready', int(len(captured) == 4))
        return False


Ready('*' + symbol('CSporbBase::Think('), internal=True)


def check(label, good, detail):
    log('%s case%d %s %s' % (label, selected, 'PASS' if good else 'FAIL', detail))
    if not good:
        raise gdb.GdbError(label + ' differs from retail')


class CtorDone(gdb.FinishBreakpoint):
    def __init__(self, frame, state):
        super().__init__(frame, internal=True)
        self.state = state

    def stop(self):
        global active
        state = self.state
        actor = state['actor']
        if 'ranges' in state:
            got = [float(val(actor + '->' + field)) for field in ('mMinGrabRange', 'mMaxGrabRange')]
            expected = [(5., 30.), (10., 20.), (float('nan'), float('nan')), (1., 30.)][selected]
            good = all((math.isnan(a) and math.isnan(b)) or a == b for a, b in zip(got, expected))
            check('C130 ranges', good, str(got))
            old = state['ranges']
            # Keep the ordinary actor's natural values for gameplay after the oracle.
            old[0] = old[2] if old[0] < old[2] else old[0]
            old[1] = old[3] if old[1] > old[3] else old[1]
            old[1] = 30. if old[1] > 30. else old[1]
            for field, value in zip(state['fields'], old):
                assign(actor + '->' + field, value)
        for field, anim_id, score, natural in state['animations']:
            got = int(val(actor + '->' + field))
            check('C130 ' + field, got == (anim_id if score > 2**-23 else 0), str(got))
            assign(actor + '->' + field, natural)
        guardian = bool(val(actor + '->mIsPowerBombGuardian'))
        got = float(val(actor + '->mMaxTimerScale'))
        check('C130 guardian scale', got == (state['timer_scale'] if guardian else 1.), str(got))
        if 'flail' in state:
            expected = state['flail'] / 2. if selected in (0, 2) else 0.
            got = float(val(actor + '->mFlailDamage.mDamage'))
            check('C130 damage', abs(got - expected) < 1.e-5, str(got))
            natural = state['flail'] / state['events'] if state['events'] else 0.
            assign(actor + '->mFlailDamage.mDamage', natural)
        active = None
        return False


class Ctor(gdb.Breakpoint):
    def stop(self):
        global active
        actor = '((CSporbBase*)%d)' % int(val('$rdi'))
        active = {'actor': actor, 'animations': [],
                  'flail': float(gdb.newest_frame().read_var('flailDamage')),
                  'timer_scale': float(gdb.newest_frame().read_var('maxTimerScale'))}
        CtorDone(gdb.newest_frame(), active)
        return False


class AnimDone(gdb.FinishBreakpoint):
    def stop(self):
        if active:
            packed = int(val('$rax')) & ((1 << 64) - 1)
            anim_id = (packed >> 32) & 0xffffffff
            if anim_id >= 2**31:
                anim_id -= 2**32
            score = [1., 0., 2**-23, float('nan')][selected]
            original_score = struct.unpack('<f', struct.pack('<I', packed & 0xffffffff))[0]
            natural = anim_id if original_score > 2**-23 else 0
            fields = ['mForwardLeanAnim', 'mBackLeanAnim', 'mLeftLeanAnim', 'mRightLeanAnim', 'mFlinchAnim']
            active['animations'].append((fields[len(active['animations'])], anim_id, score, natural))
            bits = struct.unpack('<I', struct.pack('<f', score))[0]
            assign('$rax', (packed & 0xffffffff00000000) | bits)
        return False


class Anim(gdb.Breakpoint):
    def stop(self):
        caller = gdb.newest_frame().older()
        if active and caller and (caller.name() or '').startswith('CSporbBase::CSporbBase'):
            AnimDone(gdb.newest_frame(), internal=True)
        return False


class Shadow(gdb.Breakpoint):
    def stop(self):
        caller = gdb.newest_frame().older()
        if active and int(val('$rdi')) == int(val(active['actor'])) and caller and (caller.name() or '').startswith('CSporbBase::CSporbBase'):
            fields = ['mMinGrabRange', 'mMaxGrabRange', 'mMinAttackDistance', 'mMaxAttackDistance']
            active['fields'] = fields
            active['ranges'] = [float(val(active['actor'] + '->' + f)) for f in fields]
            values = [(1., 40., 5., 50.), (10., 40., 5., 20.),
                      (float('nan'), float('nan'), 5., 20.), (1., 40., float('nan'), float('nan'))][selected]
            for field, value in zip(fields, values):
                assign(active['actor'] + '->' + field, value)
        return False


class EventsDone(gdb.FinishBreakpoint):
    def stop(self):
        if active:
            active['events'] = int(val('$eax'))
            assign('$rax', 2 if selected in (0, 2) else 0)
        return False


class Events(gdb.Breakpoint):
    def stop(self):
        caller = gdb.newest_frame().older()
        if active and caller and (caller.name() or '').startswith('CSporbBase::CSporbBase'):
            EventsDone(gdb.newest_frame(), internal=True)
        return False


Ctor('*' + symbol('CSporbBase::CSporbBase('), internal=True)
Anim('*' + symbol('CPASDatabase::FindBestAnimation(CPASAnimParmData const&, int) const'), internal=True)
Shadow('*' + symbol('CActor::SetDrawShadow('), internal=True)
Events('*' + symbol('CPatterned::GetNumUserEventsForAnimation('), internal=True)


class TendrilDtor(gdb.Breakpoint):
    def __init__(self, variant):
        name = symbol('CTendrilGen::~CTendrilGen()')
        name = name.replace('D0', variant).replace('D1', variant).replace('D2', variant)
        delta = addresses[name] - addresses[symbol('LoadSporbBase(')]
        super().__init__('*LoadSporbBase%+d' % delta, internal=True)
        self.variant = variant

    def stop(self):
        log('26A8 %s real tendril=%#x' % (self.variant, int(val('$rdi'))))
        return False


TendrilDtor('D0')
TendrilDtor('D2')
SPEC = []
