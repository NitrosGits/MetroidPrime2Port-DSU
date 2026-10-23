# Remaining caller-side team, null-target, mold and decoy arms. Team permissions
# are produced by the real team manager with temporary capacity/time inputs.
import os
import gdb

CLASS = 'CDarkCommando'
FORCE = []
OUT = os.environ['COV_OUT']
COUNTS = {}
TEAM = None
ATTACK = None
BEAM = None
DECOY = None
VISOR = None


def ev(expr):
    return gdb.parse_and_eval(expr)


def put(expr, value):
    gdb.execute('set var %s = %s' % (expr, value), to_string=True)


def log(label, detail):
    with open(os.path.join(OUT, 'forced.log'), 'a') as out:
        out.write('%s: %s\n' % (label, detail))


def count(key):
    n = COUNTS.get(key, 0)
    COUNTS[key] = n + 1
    return n


class Done(gdb.FinishBreakpoint):
    def __init__(self, callback):
        super().__init__(gdb.newest_frame(), internal=True)
        self.callback = callback

    def stop(self):
        self.callback()
        return False


def force(fields):
    saved = []
    for field, value in fields.items():
        old = ev(field)
        code = old.type.strip_typedefs().code
        if code == gdb.TYPE_CODE_PTR:
            old = '(void*)%d' % int(old)
        elif code in (gdb.TYPE_CODE_ENUM, gdb.TYPE_CODE_BOOL, gdb.TYPE_CODE_INT, gdb.TYPE_CODE_CHAR):
            old = str(int(old))
        else:
            old = str(old)
        saved.append((field, old))
        put(field, value)
    Done(lambda: [put(field, value) for field, value in saved])


class TeamThink(gdb.Breakpoint):
    def stop(self):
        global TEAM
        TEAM = int(ev('$rdi'))
        return False


class State(gdb.Breakpoint):
    def __init__(self, spec, name):
        super().__init__(spec, internal=True)
        self.name = name

    def stop(self):
        global ATTACK
        caller = gdb.newest_frame().older()
        if not caller or 'CmdStateCall' not in (caller.name() or ''):
            return False
        a = '((CDarkCommando*)%d)->' % int(ev('$rdi'))
        msg = int(ev('$edx'))
        n = count((self.name, msg))
        label = self.name + ' msg%d case%d' % (msg, n)
        if msg == 0 and n < 3 and TEAM:
            t = '((CScriptTeamAiMgr*)%d)->' % TEAM
            teamid = 65535 if n == 0 else int(ev(t + 'mUniqueId.value'))
            force({a + 'mTeamAiMgrId.value': teamid,
                   t + 'mData.mMaxMeleeAttackerCount': 0 if n == 2 else 20,
                   t + 'mData.mMaxProjectileAttackerCount': 0 if n == 2 else 20,
                   t + 'mTimeSinceMelee': 10000, t + 'mTimeSinceProjectile': 10000})
            ATTACK = (a, self.name, n)

            def finish():
                global ATTACK
                ready = int(ev(a + 'mAnimationState.mState')) == int(ev('CAnimationState::kAS_Ready'))
                log(label, '%s ready=%s' % ('PASS' if ready == (n != 2) else 'FAIL', ready))
                ATTACK = None
            Done(finish)
        elif msg == 1 and n < 5:
            fields = {a + 'mAttackState': 0, a + 'mAnimationState.mState':
                      'CAnimationState::kAS_' + ('NotReady' if n == 1 else 'Ready')}
            if n < 4:
                fields[a + 'mTargetId.value'] = 65535
            if self.name == 'MeleeAttack' and n < 3:
                fields[a + 'mAnimationState.mState'] = 'CAnimationState::kAS_NotReady'
            if self.name == 'FireEMP':
                fields[a + 'mData.eMPGrenadeAttackInfo.preFireIdleTime'] = -1 if n < 2 else 100000
                fields[a + 'mKnockedBack'] = int(n == 2)
            force(fields)
            Done(lambda: log(label, 'returned from forced target/timer/command inputs'))
        elif msg == 2 and n < 3:
            force({a + 'mTeamAiMgrId.value': 65535 if n == 0 else
                   (int(ev('((CScriptTeamAiMgr*)%d)->mUniqueId.value' % TEAM)) if TEAM else 65535)})
            Done(lambda: log(label, 'returned from EndAttack permutation'))
        return False


class Variant(gdb.Breakpoint):
    def stop(self):
        if ATTACK and ATTACK[1] == 'MeleeAttack':
            Done(lambda: put('$eax', 0))
        return False


class SafeZone(gdb.Breakpoint):
    def stop(self):
        caller = gdb.newest_frame().older()
        name = caller.name() if caller else ''
        if name and 'CDarkCommandoChargeBeam::' in name:
            n = count('safe ' + name)
            if n < 2:
                def finish():
                    put('$eax', int(n == 1))
                    log('safe zone ' + name, 'forced %d' % int(n == 1))
                Done(finish)
        return False


class BeamThink(gdb.Breakpoint):
    def stop(self):
        b = '((CDarkCommandoChargeBeam*)%d)->' % int(ev('$rdi'))
        if int(ev(b + 'mBillboardId.value')) == 65535:
            return False
        n = count('beam')
        if n < 5:
            fields = {b + 'mTimer': 6 if n == 2 else 0}
            if n == 3:
                fields[b + 'mPlayerId.value'] = 65535
            elif n == 4:
                fields[b + 'mBillboardId.value'] = 0xfffe
            force(fields)
            Done(lambda: log('8598 mold case%d' % n, 'returned'))
        return False


class DecoyThink(gdb.Breakpoint):
    def stop(self):
        global DECOY
        n = count('decoy')
        if n >= 4:
            return False
        d = '((CShadowDecoy*)%d)->' % int(ev('$rdi'))
        fields = {}
        if n == 0:
            fields[d + 'mEffect.mPtr'] = '(CElementGen*)0'
            fields[d + 'mSfxHandle.mID'] = '0xffffffff'
        elif n == 1:
            fields[d + 'mHealth'] = 0
        force(fields)
        DECOY = n
        def finish():
            global DECOY
            log('92A4 decoy case%d' % n, 'returned')
            DECOY = None
        Done(finish)
        return False


class Deletable(gdb.Breakpoint):
    def stop(self):
        if DECOY == 2:
            Done(lambda: put('$eax', 1))
        return False


class DecoyRender(gdb.Breakpoint):
    def stop(self):
        global VISOR
        n = count('decoy render')
        if n >= 4:
            return False
        d = '((CShadowDecoy*)%d)->' % int(ev('$rdi'))
        if n == 0:
            force({d + 'mEffect.mPtr': '(CElementGen*)0'})
        VISOR = ['kPV_Combat', 'kPV_Dark', 'kPV_Echo', 'kPV_Combat'][n]
        def finish():
            global VISOR
            log('9218 render case%d' % n, 'returned')
            VISOR = None
        Done(finish)
        return False


class Visor(gdb.Breakpoint):
    def stop(self):
        caller = gdb.newest_frame().older()
        if VISOR and caller and 'CShadowDecoy::AddToRenderer' in (caller.name() or ''):
            value = int(ev('CPlayerState::' + VISOR))
            Done(lambda: put('$eax', value))
        return False


class DecoyMessage(gdb.Breakpoint):
    def stop(self):
        if int(ev('((CScriptMsg*)$rdx)->m_msg')) == int(ev('kSM_Delete')) and count('decoy delete') == 0:
            d = '((CShadowDecoy*)%d)->' % int(ev('$rdi'))
            force({d + 'mSfxHandle.mID': '0xffffffff'})
            Done(lambda: log('93A4 delete', 'forced nonzero stale emitter handle'))
        return False


seen = set()
for line in open(os.environ['COV_NM']):
    raw, _, name = line.rstrip().partition('\t')
    parts = raw.split()
    if len(parts) != 3 or parts[1] not in 'tTW' or '.cold' in name or 'clone' in name or parts[0] in seen:
        continue
    prefix, symbol = name.split('(')[0], '*' + parts[2]
    if prefix in ('CDarkCommando::FireEMP', 'CDarkCommando::FireChargeBeam', 'CDarkCommando::MeleeAttack'):
        State(symbol, prefix.split('::')[1])
    elif prefix == 'CScriptTeamAiMgr::Think':
        TeamThink(symbol, internal=True)
    elif prefix == 'CDarkCommando::SelectMeleeVariant':
        Variant(symbol, internal=True)
    elif prefix == 'CSafeZoneManager::IsObjectInSafeZone':
        SafeZone(symbol, internal=True)
    elif prefix == 'CDarkCommandoChargeBeam::Think':
        BeamThink(symbol, internal=True)
    elif prefix == 'CShadowDecoy::Think':
        DecoyThink(symbol, internal=True)
    elif prefix == 'CElementGen::IsSystemDeletable':
        Deletable(symbol, internal=True)
    elif prefix == 'CShadowDecoy::AddToRenderer':
        DecoyRender(symbol, internal=True)
    elif prefix == 'CPlayerState::GetActiveVisor':
        Visor(symbol, internal=True)
    elif prefix == 'CShadowDecoy::AcceptScriptMsg':
        DecoyMessage(symbol, internal=True)
    else:
        continue
    seen.add(parts[0])
