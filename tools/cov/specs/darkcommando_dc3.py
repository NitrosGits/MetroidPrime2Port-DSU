# Focused regression for fn_3_3C44's whole-body cloak gate and fn_3_9754's eager token load.
# tools/cov/force.sh tools/cov/specs/darkcommando_dc3.py <out> \
#   3bfa3eff,06_temple_sacredpath_dark --con tools/cov/specs/darkcommando_fight.con \
#   --frames 6000 --fast
# Unchanged AI arms are asm-audited in the module note; this spec only forces the changed arms.
import os
import gdb

CLASS = 'CDarkCommando'
FORCE = []
OUT = os.environ['COV_OUT']
CASES = [
    ('visible', '1.0', '0.0', 0, False, True),
    ('already cloaked', '0.0', '0.0', 0, False, False),
    ('negative alphas', '-1.0', '-1.0', 0, False, False),
    ('unordered alphas', '$dc3_nan', '$dc3_nan', 0, False, False),
    ('target visible', '0.0', '1.0', 0, False, True),
    ('hurled', '1.0', '1.0', 1, False, False),
    ('invalid effect', '1.0', '0.0', 0, True, True),
    ('unordered alpha, visible target', '$dc3_nan', '1.0', 0, False, True),
    ('visible alpha, unordered target', '1.0', '$dc3_nan', 0, False, True),
]
state = {'next': 0, 'active': None, 'checks': {}}


def check(label, passed, detail):
    state['checks'][label] = state['checks'].get(label, True) and passed
    with open(os.path.join(OUT, 'forced.log'), 'a') as out:
        out.write('%s %s: %s\n' % ('PASS' if passed else 'FAIL', label, detail))
    with open(os.path.join(OUT, 'dc3_checks.txt'), 'w') as out:
        for name, ok in sorted(state['checks'].items()):
            out.write('%s %s\n' % ('PASS' if ok else 'FAIL', name))
        out.write('cases completed: %d/%d\n' % (state['next'], len(CASES)))


class CloakDone(gdb.FinishBreakpoint):
    def __init__(self, actor, case, saved):
        super().__init__(gdb.newest_frame(), internal=True)
        self.actor, self.case, self.saved = actor, case, saved
        self.before = int(gdb.parse_and_eval(actor + 'mDecoyId.value'))

    def stop(self):
        name, alpha, target, hurled, invalid_fx, spawn = self.case
        active = state['active']
        after = int(gdb.parse_and_eval(self.actor + 'mDecoyId.value'))
        ok = active['ctors'] == int(spawn) and ((after != self.before) == spawn)
        check(name, ok, 'decoy %04x -> %04x; constructors=%d' %
              (self.before, after, active['ctors']))
        if spawn:
            check(name + ' effect', active['effects'] == int(not invalid_fx),
                  'effects=%d' % active['effects'])
        # Preserve the real decoy result, but restore artificial alpha/flag/resource inputs.
        for field, value in self.saved.items():
            gdb.execute('set var %s%s = %s' % (self.actor, field, value))
        state['active'] = None
        return False


class CloakEntry(gdb.Breakpoint):
    def stop(self):
        if state['next'] >= len(CASES):
            return False
        case = CASES[state['next']]
        state['next'] += 1
        actor = '((CDarkCommando*)%d)->' % int(gdb.parse_and_eval('$rdi'))
        fields = ['mCloakAlpha', 'mCloakTargetAlpha', 'mCloakRate', 'mCloakFading',
                  'mHurled', 'mData.shadowDashInfo.shadowDecoyFx']
        saved = {f: str(gdb.parse_and_eval(actor + f)) for f in fields}
        CloakDone(actor, case, saved)
        state['active'] = {'ctors': 0, 'effects': 0}
        gdb.execute('set $dc3_nan = (float)0.0 / (float)0.0')
        for field, value in zip(fields[:2] + ['mHurled'], case[1:4]):
            gdb.execute('set var %s%s = %s' % (actor, field, value))
        if case[4]:
            gdb.execute('set var %smData.shadowDashInfo.shadowDecoyFx = 0xffffffff' % actor)
        return False


class DecoyEntry(gdb.Breakpoint):
    def stop(self):
        if state['active'] is not None:
            state['active']['ctors'] += 1
        return False


class EffectEntry(gdb.Breakpoint):
    def stop(self):
        if state['active'] is None:
            return False
        caller = gdb.newest_frame().older()
        # CreateShadowDecoyEffect is inlined into the decoy ctor in the host build.
        if caller and any(s in (caller.name() or '') for s in
                          ('CShadowDecoy::CShadowDecoy', 'CreateShadowDecoyEffect')):
            state['active']['effects'] += 1
            locked = bool(gdb.parse_and_eval('((CToken*)$rsi)->mLockHeld'))
            check('effect description locked before generator ctor', locked, 'lock=%s' % locked)
        return False


seen = set()
for line in open(os.environ['COV_NM']):
    raw, _, demangled = line.rstrip().partition('\t')
    parts = raw.split()
    if len(parts) != 3 or '.cold' in demangled or 'clone' in demangled or parts[0] in seen:
        continue
    for prefix, bp in [('CDarkCommando::ActivateCloak(', CloakEntry),
                       ('CShadowDecoy::CShadowDecoy(', DecoyEntry),
                       ('CElementGen::CElementGen(', EffectEntry)]:
        if demangled.startswith(prefix):
            bp('*' + parts[2], internal=True)
            seen.add(parts[0])
