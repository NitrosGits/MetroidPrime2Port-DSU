"""swb19: GetModelDataForAnimRes' inlined arm (forced invalid asset id at the call site) and a
per-Touch-call diagnostic for fn_80_220C.

    BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swarmbasics_swb19.py <out> \
      42b935e4,01_sand_entrance --con tools/drivers/swb19_touch.con --fast --frames 3000

Two independent jobs, both read-only except inside the ctor fixture:

1. GetModelDataForAnimRes (fn_80_A1BC) is a `static` free function the compiler inlines into its
   only caller, the CSwarmBasics constructor (`GetModelDataForAnimRes(animRes)` is a base-class
   initialiser argument). It has no symbol to call, so the arm is forced at the inlined call site:
   the host ctor's copy lives on source line 188 (`disassemble`: `0xe39734 cmpl $0xffffffff,(%rsi)`
   is the start of line 188, `0xe39763` is the inlined CAnimRes::GetId at CAnimRes.hpp:30,
   `0xe39772` line 188 again, then `0xe39783 call CModelData::CModelData(CAnimRes const&)`; the
   `je` at `0xe39763` skips to the null model). A one-shot breakpoint on that line writes
   `animRes.mAncsId = kInvalidAssetId` through the reference, the test then takes the invalid arm,
   and the original word is restored and read back at the constructor's return. The other
   constructors in the area are untouched, so the valid arm runs naturally in the same run.
2. Every `CSwarmBasics::Touch` entry is logged (reads only, never a call) so the coverage build's
   remaining Touch edges can be attributed: the actor's dynamic type, its weapon-mode bits, the
   swarm's cooldown timer and the boid states the arms read.
"""
CLASS = 'CSwarmBasics'
FORCE = []

import gdb
import hashlib
import json
import os

state = {'ctor': 0, 'forced': 0, 'restored': 0, 'restore_fail': 0, 'touch': 0,
         'touch_log': 0, 'keys': {}, 'err': '', 'pending': None, 'done': False}

COV_OUT = os.environ.get('COV_OUT', '/var/tmp/swb19')
LOG = os.path.join(COV_OUT, 'swb19.jsonl')

def log(event, **data):
    with open(LOG, 'a') as f:
        f.write(json.dumps(dict(event=event, **data), sort_keys=True) + '\n')

def ev(s):
    return gdb.parse_and_eval(s)

def iv(s):
    return int(ev(s))

def check(value, label):
    if not value:
        log('FAIL', label=label)
        raise gdb.GdbError(label)

def symbol(prefix):
    matches = []
    for line in open(os.environ['COV_NM']):
        raw, dem = line.rstrip('\n').split('\t')
        parts = raw.split()
        if len(parts) == 3 and parts[1] in 'tTW' and dem.startswith(prefix) \
                and 'clone' not in dem and '.cold' not in dem:
            matches.append(parts[2])
    check(len(matches) == 1, 'one native symbol: ' + prefix)
    return matches[0]

def dyn(addr):
    """The actor's dynamic type from its vtable (no inferior call)."""
    try:
        return str(ev('((CActor*)%d)' % addr).dynamic_type)
    except gdb.error:
        return '?'

# ---------------------------------------------------------------------------- the ctor fixture

class CtorReturn(gdb.FinishBreakpoint):
    def __init__(self, site, address, original, swarm):
        super().__init__(gdb.newest_frame(), internal=True)
        self.site, self.address, self.original, self.swarm = site, address, original, swarm

    def stop(self):
        inf = gdb.selected_inferior()
        errors = []
        try:
            inf.write_memory(self.address, self.original)
            actual = bytes(inf.read_memory(self.address, len(self.original)))
            if actual != self.original:
                errors.append('mismatch %s != %s' % (actual.hex(), self.original.hex()))
        except Exception as error:
            errors.append(str(error))
        state['restored'] += 1
        if errors:
            state['restore_fail'] += 1
        models = iv('((CSwarmBasics*)%d)->mModelDatas.mCount' % self.swarm)
        editor = iv('((CEntity*)%d)->mEditorId.value' % self.swarm)
        uid = iv('((CEntity*)%d)->mUniqueId.value' % self.swarm)
        log('RESTORE', site=self.site, address=self.address, original=self.original.hex(),
            readback=bytes(inf.read_memory(self.address, len(self.original))).hex(),
            errors=errors, swarm=self.swarm, model_datas=models, editor_id=editor, uid=uid)
        check(not errors, 'ctor fixture restore: ' + str(errors))
        return False

    def out_of_scope(self):
        state['err'] = 'ctor frame discarded before the fixture restored'
        log('FAIL', label=state['err'])

class CtorSite(gdb.Breakpoint):
    """The inlined GetModelDataForAnimRes call site: source line 188 inside CSwarmBasics::CSwarmBasics."""

    def stop(self):
        state['ctor'] += 1
        if state['forced'] or state['restore_fail'] or state['err']:
            return False
        try:
            frame = gdb.newest_frame()
            name = frame.name() or ''
            if 'CSwarmBasics::CSwarmBasics' not in name:
                log('SKIP', label='line 188 outside the swarm ctor', frame=name)
                return False
            this = iv('$rdi')
            try:
                address = iv('&animRes')
                source = 'animRes'
            except gdb.error:
                address = iv('$rsi')      # the instruction at this line reads (%rsi)
                source = '$rsi'
            inf = gdb.selected_inferior()
            original = bytes(inf.read_memory(address, 4))
            before = int.from_bytes(original, 'little')
            if before == 0xFFFFFFFF:
                log('SKIP', label='this ctor already has an invalid asset id', swarm=this)
                return False
            # Arm the restore before the first write.
            CtorReturn('line188', address, original, this)
            inf.write_memory(address, b'\xff\xff\xff\xff')
            written = int.from_bytes(bytes(inf.read_memory(address, 4)), 'little')
            check(written == 0xFFFFFFFF, 'the forced asset id read back')
            state['forced'] += 1
            log('FORCE', site='CSwarmBasics.cpp:188', swarm=this, address=address,
                source=source, anim_id_before=before, written=written, pc=frame.pc())
        except Exception as error:
            log('FAIL', label='ctor site: %s: %s' % (type(error).__name__, error))
        return False

# ---------------------------------------------------------------------------- the Touch logger

def ball_state(player):
    try:
        ball = iv('((CPlayer*)%d)->mMorphBall.mItem' % player)
        return iv('((CMorphBall*)%d)->mBallState' % ball) if ball else -1
    except gdb.error:
        return -2

class TouchLog(gdb.Breakpoint):
    def stop(self):
        try:
            this, mgr = iv('$rdi'), iv('$rdx')
            actor = iv('$rsi')
            name = dyn(actor)
            state['touch'] += 1
            weapon = -1
            if 'Projectile' in name:
                try:
                    raw = iv('((CGameProjectile*)%d)->mCurDamageInfo.mWeaponMode.mWeaponType'
                             % actor)
                    charged = iv('((CGameProjectile*)%d)->mCurDamageInfo.mWeaponMode.mCharged'
                                 % actor)
                    comboed = iv('((CGameProjectile*)%d)->mCurDamageInfo.mWeaponMode.mComboed'
                                 % actor)
                    weapon = raw | (charged << 16) | (comboed << 17)
                except gdb.error:
                    weapon = -2
            player = -1
            try:
                player = iv('((CStateManager*)%d)->mPlayers[0]' % mgr)
            except gdb.error:
                player = -1
            ball = ball_state(player) if player > 0 else -2
            key = '%s w=%d ball=%d' % (name, weapon, ball)
            state['keys'][key] = state['keys'].get(key, 0) + 1
            if state['touch_log'] < 120:
                state['touch_log'] += 1
                boid_state = [-1, -1, -1, -1]
                try:
                    boids = iv('((CSwarmBasics*)%d)->mBoids.mCount' % this)
                    for i in range(min(boids, 1)):
                        base = '((CSwarmBasics*)%d)->mBoids.mItems[%d]' % (this, i)
                        boid_state = [boids, iv(base + '.mActive'),
                                      iv(base + '.mFreezeTimer') > 0,
                                      int(float(ev(base + '.mHealth')))]
                except gdb.error:
                    pass
                log('TOUCH', n=state['touch'], swarm=this, actor=actor, type=name,
                    weapon=weapon, player=player, ball=ball,
                    morph=iv('((CPlayer*)%d)->mMorphBallState' % player) if player > 0 else -1,
                    spawned=iv('((CPlayer*)%d)->mSpawnedMorphBallState' % player)
                    if player > 0 else -1,
                    cooldown=float(ev('((CSwarmBasics*)%d)->mDamageCooldownTimer' % this)),
                    touch_radius=float(ev('((CSwarmBasics*)%d)->mTouchRadius' % this)),
                    player_radius=float(ev('((CSwarmBasics*)%d)->mPlayerTouchRadius' % this)),
                    boid=boid_state)
        except Exception as error:
            log('TOUCH-ERR', error='%s: %s' % (type(error).__name__, error))
        return False

# ---------------------------------------------------------------------------- install / summary

def finished(event):
    summary = ('SUMMARY exit: ctor_sites=%d forced=%d restored=%d restore_fail=%d touch=%d err=%s'
               % (state['ctor'], state['forced'], state['restored'], state['restore_fail'],
                  state['touch'], state['err'] or '-'))
    log('EXIT', summary=summary)
    with open(os.path.join(COV_OUT, 'swb19_summary.txt'), 'w') as f:
        f.write(summary + '\n')
        for key in sorted(state['keys']):
            f.write('  %-46s %d\n' % (key, state['keys'][key]))
    if state['forced'] < 1 or state['restore_fail']:
        log('FAIL', label='ctor fixture incomplete')

def install():
    binary = gdb.current_progspace().filename
    check(bool(binary), 'GDB executable identity')
    with open(binary, 'rb') as f:
        digest = hashlib.file_digest(f, 'sha256').hexdigest()
    log('BINARY', path=binary, sha256=digest)
    CtorSite('CSwarmBasics.cpp:188', internal=True)
    TouchLog('*' + symbol('CSwarmBasics::Touch('), internal=True)
    gdb.events.exited.connect(finished)

if os.environ.get('SWB19_SELFTEST') != '1':
    install()
