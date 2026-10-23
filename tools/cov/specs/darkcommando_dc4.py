# dc4: execute the real KnockBack body with controlled inputs and base-call results.
# The base call itself runs normally. Its result is forced only after it returns;
# this isolates fn_3_62D0's decisions from the DOL RULE evaluator. See the module note.
import os
import gdb

CLASS = 'CDarkCommando'
FORCE = []
OUT = os.environ['COV_OUT']
STATE = {'next': 0, 'active': None, 'checks': {}, 'seen': {}}
# label, alive, knocked, dashing, reaction, followup, ragdoll early return
CASES = [
    ('flinch', 1, 0, 0, 1, 0, False),
    ('already knocked', 1, 1, 0, 1, 0, False),
    ('live dash', 1, 0, 1, 1, 0, False),
    ('freeze', 1, 0, 0, 1, 2, False),
    ('knock', 1, 0, 0, 2, 0, False),
    ('hurl', 1, 0, 0, 3, 0, False),
    ('dead non-hurl', 0, 0, 0, 2, 0, False),
    ('dead lagged burn', 0, 0, 1, 3, 10, False),
    ('dead burn', 0, 0, 0, 3, 9, False),
    ('dead explode', 0, 0, 0, 3, 7, False),
    ('dead ice', 0, 0, 0, 3, 8, False),
    ('dead sound', 0, 0, 0, 3, 6, False),
    ('ragdoll guard', 1, 0, 0, 1, 0, True),
]


def ev(expr):
    return gdb.parse_and_eval(expr)


def put(expr, value):
    gdb.execute('set var %s = %s' % (expr, value), to_string=True)


def check(label, passed, detail=''):
    STATE['checks'][label] = STATE['checks'].get(label, True) and passed
    with open(os.path.join(OUT, 'forced.log'), 'a') as out:
        out.write('%s %s: %s\n' % ('PASS' if passed else 'FAIL', label, detail))
    with open(os.path.join(OUT, 'dc4_checks.txt'), 'w') as out:
        for name, ok in sorted(STATE['checks'].items()):
            out.write('%s %s\n' % ('PASS' if ok else 'FAIL', name))
        out.write('KnockBack cases: %d/%d\n' % (STATE['next'], len(CASES)))


def scalar(expr):
    value = ev(expr)
    if value.type.code == gdb.TYPE_CODE_PTR:
        return '(void*)%d' % int(value)
    if value.type.code in (gdb.TYPE_CODE_ENUM, gdb.TYPE_CODE_BOOL, gdb.TYPE_CODE_INT):
        return str(int(value))
    return str(value)


class Done(gdb.FinishBreakpoint):
    def __init__(self, callback):
        super().__init__(gdb.newest_frame(), internal=True)
        self.callback = callback

    def stop(self):
        self.callback()
        return False


class KnockEntry(gdb.Breakpoint):
    def stop(self):
        if STATE['next'] == len(CASES):
            return False
        case = CASES[STATE['next']]
        STATE['next'] += 1
        actor = '((CDarkCommando*)%d)->' % int(ev('$rdi'))
        fields = ['mAlive', 'mKnockedBack', 'mShadowDashing', 'mHurled',
                  'mRagDoll.mPtr', 'mCloakAlpha', 'mCloakTargetAlpha',
                  'mCloakRate', 'mCloakFading']
        saved = {f: scalar(actor + f) for f in fields}
        active = {'actor': actor, 'case': case, 'saved': saved, 'enable': [],
                  'base': 0, 'sound': 0, 'cloak': 0, 'visor': 0, 'shadow': 0}
        STATE['active'] = active
        for f, val in zip(fields[:4], case[1:4] + (0,)):
            put(actor + f, val)
        if case[6]:
            # 0x62F4..0x62FC only checks this pointer, then returns; never dereferences it.
            put(actor + 'mRagDoll.mPtr', '(CPirateRagDoll*)%d' % int(ev('$rdi')))

        def finish():
            name, alive, knocked, dash, reaction, followup, rag = case
            expected = [] if rag else [(1, 1), (2, int(not knocked and not dash)),
                                      (3, int(not (alive and dash)))]
            check('62D0 ' + name + ' enable', active['enable'] == expected,
                  repr(active['enable']))
            check('62D0 ' + name + ' base', active['base'] == int(not rag))
            want_knocked = knocked or (alive and reaction != 1 and not rag)
            want_hurled = alive and reaction == 3 and not rag
            check('62D0 ' + name + ' flags',
                  bool(ev(actor + 'mKnockedBack')) == bool(want_knocked) and
                  bool(ev(actor + 'mHurled')) == bool(want_hurled))
            freeze = int(alive and followup == 2 and not rag)
            check('62D0 ' + name + ' freeze calls',
                  (active['cloak'], active['visor'], active['shadow']) == (freeze,) * 3)
            sound = int(not alive and reaction == 3 and followup not in (10, 9, 7, 8) and not rag)
            check('62D0 ' + name + ' sound', active['sound'] == sound)
            for f, value in saved.items():
                put(actor + f, value)
            for f, value in active.get('base_result', {}).items():
                put(actor + f, value)
            STATE['active'] = None

        Done(finish)
        return False


class BaseEntry(gdb.Breakpoint):
    def stop(self):
        a = STATE['active']
        if a is None or a['actor'] != '((CDarkCommando*)%d)->' % int(ev('$rdi')):
            return False
        a['base'] += 1
        # The forced dead bit above tests only EnableAnimReaction's input gate.
        put(a['actor'] + 'mAlive', a['saved']['mAlive'])

        def finish():
            actor, case = a['actor'], a['case']
            result_fields = ['mKnockBackController.mActiveParameters.mReaction',
                             'mKnockBackController.mActiveParameters.mFollowUp']
            a['base_result'] = {f: scalar(actor + f) for f in result_fields}
            put(actor + 'mAlive', case[1])
            put(actor + 'mKnockBackController.mActiveParameters.mReaction', case[4])
            put(actor + 'mKnockBackController.mActiveParameters.mFollowUp', case[5])
        Done(finish)
        return False


class Observe(gdb.Breakpoint):
    def __init__(self, spec, kind):
        super().__init__(spec, internal=True)
        self.kind = kind

    def stop(self):
        a = STATE['active']
        if a is None:
            return False
        caller = gdb.newest_frame().older()
        if not caller or 'CDarkCommando::KnockBack' not in (caller.name() or ''):
            return False
        if self.kind == 'enable':
            a['enable'].append((int(ev('$esi')), int(ev('$edx'))))
        else:
            a[self.kind] += 1
        return False


seen = set()
for line in open(os.environ['COV_NM']):
    raw, _, demangled = line.rstrip().partition('\t')
    parts = raw.split()
    if len(parts) != 3 or '.cold' in demangled or 'clone' in demangled or parts[0] in seen:
        continue
    prefix = demangled.split('(')[0]
    symbol = '*' + parts[2]
    if prefix == 'CDarkCommando::KnockBack':
        KnockEntry(symbol, internal=True)
    elif prefix == 'CPatterned::KnockBack':
        BaseEntry(symbol, internal=True)
    elif prefix in {'CKnockBackMgr::EnableAnimReaction', 'CActor::PlayCustomSound',
                    'CDarkCommando::SetCloakTarget', 'CActor::SetVisorOrbitableFlags',
                    'CActor::SetDrawShadow'}:
        kind = {'CKnockBackMgr::EnableAnimReaction': 'enable', 'CActor::PlayCustomSound': 'sound',
                'CDarkCommando::SetCloakTarget': 'cloak', 'CActor::SetVisorOrbitableFlags': 'visor',
                'CActor::SetDrawShadow': 'shadow'}[prefix]
        Observe(symbol, kind)
    else:
        continue
    seen.add(parts[0])
