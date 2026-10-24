# sjg2: shipped loader replay and nested readers, without skipped real callees.
# force_ret.py; 42b935e4,02_sand_sandfalls_dark; tools/drivers/sjg2.con;1000 frames.
import gdb
import os
import struct

CLASS = 'CIngSpaceJumpGuardian'
captured = None
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


def exact(name):
    return '($sj2_bias+%d)' % addresses[name]


def log(text):
    with open(os.path.join(os.environ['COV_OUT'], 'forced.log'), 'a') as out:
        out.write(text + '\n')


def val(expr):
    return gdb.parse_and_eval(expr)


def assign(expr, value):
    gdb.execute('set var %s=%s' % (expr, value))


def prop(pid, data):
    return struct.pack('>IH', pid, len(data)) + data


class Capture(gdb.Breakpoint):
    def stop(self):
        global captured
        if captured is None:
            pos = int(val('((CInputStream*)$rsi)->mPtr'))
            inferior = gdb.selected_inferior()
            count = struct.unpack('>H', inferior.read_memory(pos, 2))[0]
            end = pos + 2
            for _ in range(count):
                _, size = struct.unpack('>IH', inferior.read_memory(end, 6))
                end += 6 + size
            captured = (bytes(inferior.read_memory(pos, end - pos)),
                        bytes(inferior.read_memory(int(val('$rdx')), gdb.lookup_type('CEntityInfo').sizeof)))
            log('170 captured %d top-level properties, %d bytes' % (count, end - pos))
        return False


Capture('*' + symbol('LoadIngSpaceJumpGuardian('), internal=True)


def augment(data, depth, missing_effect):
    count = struct.unpack('>H', data[:2])[0]
    cursor = 2
    result = []
    nested = {0xf9837ab3, 0x5e1d1931, 0x6b08e2e5, 0xf223aa76, 0xd0db5f7a}
    for _ in range(count):
        pid, size = struct.unpack('>IH', data[cursor:cursor + 6])
        payload = data[cursor + 6:cursor + 6 + size]
        if pid in nested:
            payload = augment(payload, depth + 1, missing_effect)
        if pid == 0xa926f8a8 and missing_effect:
            payload = b'\xff' * 4  # 6004..6010: absent optional token.
        result.append(prop(pid, payload))
        cursor += 6 + size
    if depth == 1:
        # Reader D18 also sees count0, preserving previously read fields.
        result.append(prop(0x5e1d1931, b'\0\0'))
    result.append(prop(0x12345678, b'xyz'))
    return struct.pack('>H', len(result)) + b''.join(result)


class Prepare(gdb.Function):
    def __init__(self):
        super().__init__('sj2_prepare')

    def invoke(self, buffer, info, which):
        data, original_info = captured
        which = int(which)
        if which == 1:
            # Reader864 count0, top-level default, naturally invalid model.
            data = struct.pack('>H', 2) + prop(0xf9837ab3, b'\0\0') + prop(0x12345678, b'xyz')
        elif which == 3:
            data = b'\0\0'  # Top-level property loop count0.
        else:
            data = augment(data, 0, which == 2)
        inferior = gdb.selected_inferior()
        inferior.write_memory(int(buffer), data)
        inferior.write_memory(int(info), original_info)
        for field in ('mCount', 'mCapacity', 'mItems'):
            assign('((CEntityInfo*)%d)->mConnections.%s' % (int(info), field), 0)
        log('170 replay%d %d bytes' % (which, len(data)))
        return len(data)


class Check(gdb.Function):
    def __init__(self):
        super().__init__('sj2_check')

    def invoke(self, which, good):
        log('sjg2 #%d %s' % (int(which), 'PASS' if bool(good) else 'FAIL'))
        if not bool(good):
            raise gdb.GdbError('Jump Guardian differs from retail')
        return 1


Prepare()
Check()
scenarios = []
for which in range(4):
    cleanup = '0' if which in (1, 3) else '((void(*)(void*))%s)($sj2_result)' % exact(symbol('CIngSpaceJumpGuardian::~CIngSpaceJumpGuardian()').replace('D1', 'D0').replace('D2', 'D0'))
    expr = ('($sj2_buffer=(char*)malloc(65536),$sj2_info=(CEntityInfo*)malloc(sizeof(CEntityInfo)),'
            '$sj2_stream=(CInputStream*)malloc(sizeof(CInputStream)),'
            '$sj2_length=$sj2_prepare($sj2_buffer,$sj2_info,%d),'
            '((void(*)(void*,void*,unsigned long))%s)($sj2_stream,$sj2_buffer,$sj2_length),'
            '$sj2_result=LoadIngSpaceJumpGuardian(*$mgr,*$sj2_stream,*$sj2_info),'
            '$sj2_check(%d,($sj2_result!=0)==%d),%s,'
            '((void(*)(void*))%s)($sj2_stream),'
            '(void)free($sj2_stream),(void)free($sj2_buffer),(void)free($sj2_info),1)') % (
                which, symbol('CInputStream::CInputStream(void const*, unsigned long)'),
                which, int(which not in (1, 3)), cleanup,
                exact(symbol('CInputStream::~CInputStream()').replace('D0', 'D2').replace('D1', 'D2')))
    scenarios.append(([], expr))
scenarios.append(([], '$sj2_check(4,$this->IsListening() && $this->GetGravityConstant()==60.f && $this->GetSearchPath()==&$this->mPathFindSearch)'))
for index, cls in enumerate(('SLdrIngSpaceJumpGuardianData', 'SLdrIngSpaceJumpGuardian'), 5):
    ctor = exact(symbol(cls + '::' + cls + '()'))
    dtor = exact(symbol(cls + '::~' + cls + '()').replace('D1', 'D2'))
    scenarios.append(([], '($sj2_heap=1,$sj2_fixture=(void*)malloc(sizeof(%s)),((void(*)(void*))%s)($sj2_fixture),'
                      '((void(*)(void*))%s)($sj2_fixture),(void)free($sj2_fixture),$sj2_heap=0,$sj2_check(%d,1))' %
                      (cls, ctor, dtor, index)))
CALLS = [('sjg2 loader/getter suite', 'CIngSpaceJumpGuardian::Think(',
          '$sj2_ready', scenarios, 'actual loaders, nested readers, constructors/destructors, inline getters')]
gdb.execute('set $sj2_ready=0')
gdb.execute('set $sj2_heap=0')


class Ready(gdb.Breakpoint):
    def stop(self):
        assign('$sj2_bias', '(long)&LoadIngSpaceJumpGuardian-%d' % addresses[symbol('LoadIngSpaceJumpGuardian(')])
        assign('$sj2_ready', int(float(val('((CIngSpaceJumpGuardian*)$rdi)->mLightIntensity')) > 0.05))
        return False


Ready('*' + symbol('CIngSpaceJumpGuardian::Think('), internal=True)


class CtorDone(gdb.FinishBreakpoint):
    def __init__(self, frame, actor, expected_effect):
        super().__init__(frame, internal=True)
        self.actor, self.expected_effect = actor, expected_effect

    def stop(self):
        actor = self.actor
        obj = val(actor).dereference()
        flags = [bool(obj[n]) for n in ('mJumping', 'mHasJumpPath', 'mHeardNoise',
                 'mLastJumpToWaypoint', 'mDamageable', 'mBlobActive', 'mRender')]
        ids = [int(obj[n]['value']) for n in ('mLightId', 'mPlayerId', 'mFaceTarget',
               'mIntroWaypoint', 'mLastJumpPoint', 'mBlobId', 'mJumpTarget')]
        good = flags == [False, False, False, False, True, False, True] and ids == [65535] * 7
        good &= int(obj['mAttackAction']) == -1 and int(obj['mJumpMode']) == -1
        good &= int(obj['mPortalCount']) == 0 and int(obj['mPortalIndex']) == -1
        good &= float(obj['mDeathTimer']) == 2. and float(obj['mLightIntensity']) == 0.
        good &= int(obj['mPathFindSearch']['mFlags']) == 0x301
        good &= bool(obj['mMiniPortalEffect']['m_valid']) == self.expected_effect
        log('5E58 constructor defaults %s effect=%s' % ('PASS' if good else 'FAIL', self.expected_effect))
        if not good:
            raise gdb.GdbError('constructor differs from retail')
        return False


class Ctor(gdb.Breakpoint):
    def stop(self):
        actor = '((CIngSpaceJumpGuardian*)%d)' % int(val('$rdi'))
        data = gdb.newest_frame().read_var('data').referenced_value()
        CtorDone(gdb.newest_frame(), actor, int(data['miniPortalEffect']) != 0xffffffff)
        return False


Ctor('*' + symbol('CIngSpaceJumpGuardian::CIngSpaceJumpGuardian('), internal=True)


class DataDone(gdb.FinishBreakpoint):
    def __init__(self, frame, address):
        super().__init__(frame, internal=True)
        self.address = address

    def stop(self):
        data = val('((SLdrIngSpaceJumpGuardianData*)%d)' % self.address).dereference()
        good = int(data['ingSpotBlobEffect']) == 0xffffffff and int(data['miniPortalEffect']) == 0xffffffff
        good &= int(data['sound']) == -1 and int(data['sound_MiniPortal']) == -1
        good &= float(data['lightAttenuation']) == 5.
        for name in ('ingSpaceJumpGuardianStruct', 'ingSpaceJumpGuardianStruct_0x6b08e2e5',
                     'ingSpaceJumpGuardianStruct_0xf223aa76', 'ingSpaceJumpGuardianStruct_0xd0db5f7a'):
            for field, expected in (('tauntChance', 10.), ('attackChance', 90.), ('unknown_0x03698c10', 33.3),
                                    ('locomotionSpeed', 1.), ('unknown_0x3e370622', 1500.)):
                good &= abs(float(data[name][field]) - expected) < 1.e-4
        beam = data['miniPortalBeamInfo']
        for field, expected in (('length', 500.), ('expansionSpeed', 4.), ('lifeTime', 1.),
                                ('pulseSpeed', 20.), ('shutdownTime', .25), ('pulseEffectScale', 2.)):
            good &= float(beam[field]) == expected
        for color, expected in ((data['lightColor'], (255, 255, 255, 255)),
                                (beam['innerColor'], (127, 127, 127, 127)),
                                (beam['outerColor'], (153, 153, 0, 127))):
            good &= tuple(int(color['m' + channel]) for channel in 'RGBA') == expected
        damage = data['miniPortalProjectileDamage']
        good &= int(damage['dI_WeaponType']) == 11 and float(damage['dI_Damage']) == 20.
        good &= float(damage['dI_KnockBackPower']) == 10.
        log('B7C/E84 default-data oracle %s' % ('PASS' if good else 'FAIL'))
        if not good:
            raise gdb.GdbError('loader defaults differ from retail')
        return False


class DataCtor(gdb.Breakpoint):
    def stop(self):
        # Direct calls from GDB can unwind the C1/C2 alias to its dummy frame
        # before the prologue. Natural loader calls already assert these defaults.
        if not int(val('$sj2_heap')):
            DataDone(gdb.newest_frame(), int(val('$rdi')))
        return False


DataCtor('*' + symbol('SLdrIngSpaceJumpGuardianData::SLdrIngSpaceJumpGuardianData()'), internal=True)
SPEC = []
