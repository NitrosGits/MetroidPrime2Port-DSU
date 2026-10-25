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
         'touch_log': 0, 'keys': {}, 'err': '', 'pending': None, 'done': False,
         'ticks': 0, 'vuln': [], 'repair': None, 'glue': {}, 'glue_applied': 0,
         'glue_restored': 0, 'glue_fail': 0, 'mgr': 0, 'freeze': None, 'screw': None,
         'spawn': None}
# Tick numbers are CSwarmBasics::UpdatePartition calls, about four per game frame (one per swarm),
# so 8000..11600 is roughly frames 2000..2900 - the driver's warped/morphed phases.
GLUE_START = int(os.environ.get('SWB19_GLUE_START', '8000'))
GLUE_END = int(os.environ.get('SWB19_GLUE_END', '11600'))
FREEZE_START = int(os.environ.get('SWB19_FREEZE_START', '8400'))
FREEZE_END = int(os.environ.get('SWB19_FREEZE_END', '8800'))
SCREW_START = int(os.environ.get('SWB19_SCREW_START', '10600'))
SCREW_END = int(os.environ.get('SWB19_SCREW_END', '11000'))
SPAWN_START = int(os.environ.get('SWB19_SPAWN_START', '11200'))
SPAWN_END = int(os.environ.get('SWB19_SPAWN_END', '11600'))

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

def quote(name):
    return "'" + name + "'"

def ctor_symbol():
    """The CSwarmBasics constructor.  The build has both the complete-object (C1) and base-object
    (C2) symbols at ONE address, so accept several names only when they resolve to the same
    function, and prefer C1 (what the factories call)."""
    names = []
    for line in open(os.environ['COV_NM']):
        raw, dem = line.rstrip('\n').split('\t')
        parts = raw.split()
        if len(parts) == 3 and parts[1] in 'tTW' and dem.startswith('CSwarmBasics::CSwarmBasics(') \
                and 'clone' not in dem and '.cold' not in dem:
            names.append(parts[2])
    check(bool(names), 'the CSwarmBasics constructor symbol')
    addresses = {}
    for name in names:
        addresses.setdefault(iv('(long)&' + quote(name)), []).append(name)
    check(len(addresses) == 1, 'one constructor address: %s' % sorted(addresses))
    same = list(addresses.values())[0]
    log('CTOR-SYMBOLS', names=same, address=list(addresses)[0])
    for name in same:
        if 'C1E' in name:
            return name
    return same[0]

def vulnerability(this, swarm_count):
    """The swarm's damage-vulnerability table: mWeaponIndices[kWT_Max] index into mVulnerabilities,
    plus the charged and combo override words.  Read once per swarm so the report can say which
    weapon types are kE_Reflect / kE_Immune (and therefore which of Touch's arms are data-dead)."""
    rows = []
    try:
        base = '((CSwarmBasics*)%d)->mDamageVulnerability' % this
        indices = [iv(base + '.mWeaponIndices[%d]' % i) for i in range(21)]
        count = iv(base + '.mVulnerabilities.mCount')
        entries = []
        for i in range(min(count, 8)):
            entry = base + '.mVulnerabilities.mItems[%d]' % i
            entries.append([float(ev(entry + '.mDamageMultiplier')), iv(entry + '.mEffect'),
                            iv(entry + '.mIgnoreRadius')])
        rows = dict(swarm=this, indices=indices, count=count, entries=entries,
                    charged=iv(base + '.mChargedBeamWord'), combo=iv(base + '.mComboWord'))
        log('VULN', **rows)
    except gdb.error as error:
        log('VULN-ERR', swarm=this, error=str(error)[:120])

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
        animated = iv('((CSwarmBasics*)%d)->mAnimated' % self.swarm)
        if models == 0 and animated:
            # The invalid arm's null model then reaches AllocateSkinnedModels (kSM_Create), whose
            # `mModelData->PickAnimatedModel` has no null test in the port (retail's model is by
            # value): run 2 crashed there (SIGSEGV, this=0x0).  A real non-animated swarm skips
            # that body (`if (mAnimated)`), so clear the flag for this actor only, log it, and put
            # it back later.  The arm itself has already run and been counted.
            write('((CSwarmBasics*)%d)->mAnimated' % self.swarm, '0')
            state['repair'] = dict(swarm=self.swarm, until=900, animated=animated,
                                   readback=iv('((CSwarmBasics*)%d)->mAnimated' % self.swarm))
            log('REPAIR', swarm=self.swarm, animated_was=animated,
                animated_now=state['repair']['readback'],
                why='null model + mAnimated would deref a null CModelData in AllocateSkinnedModels')
        log('RESTORE', site=self.site, address=self.address, original=self.original.hex(),
            readback=bytes(inf.read_memory(self.address, len(self.original))).hex(),
            errors=errors, swarm=self.swarm, model_datas=models, editor_id=editor, uid=uid,
            animated=animated)
        check(not errors, 'ctor fixture restore: ' + str(errors))
        return False

    def out_of_scope(self):
        state['err'] = 'ctor frame discarded before the fixture restored'
        log('FAIL', label=state['err'])

def ctor_site():
    """The inlined GetModelDataForAnimRes test inside the CSwarmBasics constructor, found in the
    ctor's own disassembly: `cmpl $0xffffffff,(%rsi)` on the animRes asset id.  A file:line
    breakpoint on line 188 does not bind in this build ("No compiled code for line 188"), so the
    site is located by that instruction and set as symbol+offset, which relocates with the PIE."""
    ctor = ctor_symbol()
    text = gdb.execute('disassemble ' + quote(ctor), to_string=True)
    hits = [line for line in text.splitlines() if 'cmpl' in line and '0xffffffff' in line
            and '(%rsi)' in line]
    check(len(hits) == 1, 'one kInvalidAssetId test in the ctor: %d found' % len(hits))
    address = int(hits[0].split('<')[0].strip().rstrip(':'), 16)
    start = int(gdb.parse_and_eval('(long)&' + quote(ctor)))
    log('SITE', instruction=hits[0].strip(), address=address, ctor_start=start,
        offset=address - start)
    return ctor, address - start


class CtorSite(gdb.Breakpoint):
    """The inlined GetModelDataForAnimRes call site inside the real CSwarmBasics constructor."""

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

# ------------------------------------------------------------------- boid position sampling

def write(lv, rhs):
    gdb.execute('set var %s = %s' % (lv, rhs), to_string=True)

def player_position(mgr):
    try:
        player = iv('((CStateManager*)%d)->mPlayers[0]' % mgr)
        if player:
            base = '((CActor*)%d)->mPosition' % player
            return tuple(float(ev(base + '.m' + axis)) for axis in 'XYZ'), player
    except gdb.error:
        pass
    return None, 0

def force_windows(this):
    """Two real game states no shipped sand-entrance swarm reaches on its own, each written to the
    field the body reads and restored with a readback:

    * a frozen boid (mFreezeTimer > 0: FreezeBoids is a CHomingBlob path) for Touch's
      `it->mFreezeTimer <= 0.f` false side;
    * the player in screw-attack ball mode (CMorphBall::kBS_ScrewAttack) with no cannon ball, for
      `!cannonBall && !InScrewAttackMode()`;
    * the player SPAWNED morphed (kMS_Morphed: a save made in ball form), for the
      GetSpawnedMorphballState ternary's non-unmorphed side.
    """
    tick = state['ticks']
    inf = gdb.selected_inferior()
    if FREEZE_START <= tick <= FREEZE_END and state['freeze'] is None:
        try:
            if iv('((CSwarmBasics*)%d)->mBoids.mCount' % this) > 1:
                address = iv('(long)&((CSwarmBasics*)%d)->mBoids.mItems[1].mFreezeTimer' % this)
                original = inf.read_memory(address, 4)
                inf.write_memory(address, gdb.parse_and_eval('(float)1.0').bytes)
                state['freeze'] = dict(swarm=this, address=address, original=original.tobytes() if hasattr(original, 'tobytes') else bytes(original))
                log('FREEZE', swarm=this, address=address, original=state['freeze']['original'].hex(),
                    readback=bytes(inf.read_memory(address, 4)).hex(), tick=tick)
        except gdb.error as error:
            log('FREEZE-ERR', error=str(error)[:100])
    if tick > FREEZE_END and state['freeze'] is not None:
        record = state['freeze']
        inf.write_memory(record['address'], record['original'])
        log('FREEZE-UNDO', swarm=record['swarm'],
            readback=bytes(inf.read_memory(record['address'], 4)).hex(),
            ok=bytes(inf.read_memory(record['address'], 4)) == record['original'])
        state['freeze'] = None
    if tick == SCREW_START and state['screw'] is None and state['mgr']:
        try:
            player = iv('((CStateManager*)%d)->mPlayers[0]' % state['mgr'])
            ball = iv('((CPlayer*)%d)->mMorphBall.mPtr' % player)
            if ball:
                address = iv('(long)&((CMorphBall*)%d)->mBallState' % ball)
                original = bytes(inf.read_memory(address, 4))
                inf.write_memory(address, (4).to_bytes(4, 'little'))     # kBS_ScrewAttack
                state['screw'] = dict(address=address, original=original,
                                      ball=ball, player=player)
                log('SCREW', player=player, ball=ball, original=original.hex(),
                    readback=bytes(inf.read_memory(address, 4)).hex(), tick=tick)
        except gdb.error as error:
            log('SCREW-ERR', error=str(error)[:100])
    if tick > SCREW_END and state['screw'] is not None:
        record = state['screw']
        inf.write_memory(record['address'], record['original'])
        log('SCREW-UNDO', readback=bytes(inf.read_memory(record['address'], 4)).hex(),
            ok=bytes(inf.read_memory(record['address'], 4)) == record['original'])
        state['screw'] = None
    if tick == SPAWN_START and state['spawn'] is None and state['mgr']:
        try:
            player = iv('((CStateManager*)%d)->mPlayers[0]' % state['mgr'])
            address = iv('(long)&((CPlayer*)%d)->mSpawnedMorphBallState' % player)
            original = bytes(inf.read_memory(address, 4))
            inf.write_memory(address, (1).to_bytes(4, 'little'))         # CPlayer::kMS_Morphed
            state['spawn'] = dict(address=address, original=original, player=player)
            log('SPAWN', player=player, original=original.hex(),
                readback=bytes(inf.read_memory(address, 4)).hex(), tick=tick)
        except gdb.error as error:
            log('SPAWN-ERR', error=str(error)[:100])
    if tick > SPAWN_END and state['spawn'] is not None:
        record = state['spawn']
        inf.write_memory(record['address'], record['original'])
        log('SPAWN-UNDO', readback=bytes(inf.read_memory(record['address'], 4)).hex(),
            ok=bytes(inf.read_memory(record['address'], 4)) == record['original'])
        state['spawn'] = None


def glue(this):
    """Stand-in-boid: keep one live boid of every swarm in the area on the player during the
    window, so the real collision system's player-vs-swarm Touch reaches Touch's own per-boid box
    test (run 1: the player was inside the swarm's box 3215 times and never inside a boid's).  The
    boid is a real live object, its translation is snapshotted before the first write and restored
    at the window's end with a readback; the player is never written."""
    if state['ticks'] < GLUE_START or state['ticks'] > GLUE_END:
        if state['ticks'] > GLUE_END and state['glue']:
            restore_glue()
        if state['repair'] and state['ticks'] >= state['repair']['until']:
            write('((CSwarmBasics*)%d)->mAnimated' % state['repair']['swarm'], '1')
            log('REPAIR-UNDO', swarm=state['repair']['swarm'],
                readback=iv('((CSwarmBasics*)%d)->mAnimated' % state['repair']['swarm']))
            state['repair'] = None
        return
    pos, _player = player_position(state['mgr'])
    if pos is None:
        return
    record = state['glue'].get(this)
    if record is None:
        try:
            base = '((CSwarmBasics*)%d)->mBoids.mItems[0]' % this
            if iv('((CSwarmBasics*)%d)->mBoids.mCount' % this) < 1:
                return
            inf = gdb.selected_inferior()
            addresses = [iv('(long)&' + base + '.mTransform.m%d3' % axis) for axis in range(3)]
            original = [bytes(inf.read_memory(a, 4)) for a in addresses]
            record = dict(addresses=addresses, original=original)
            state['glue'][this] = record
            log('GLUE-ARM', swarm=this, n=state['ticks'],
                original=[b.hex() for b in original], boid=0)
        except gdb.error as error:
            log('GLUE-ERR', swarm=this, error=str(error)[:100])
            return
    try:
        for axis in range(3):
            write('((CSwarmBasics*)%d)->mBoids.mItems[0].mTransform.m%d3' % (this, axis),
                  repr(pos[axis]))
        state['glue_applied'] += 1
        if state['glue_applied'] % 200 == 1:
            log('GLUE', swarm=this, n=state['ticks'], position=pos)
    except gdb.error as error:
        log('GLUE-ERR', swarm=this, error=str(error)[:100])

def restore_glue():
    inf = gdb.selected_inferior()
    for swarm, record in state['glue'].items():
        errors = []
        for address, original in zip(record['addresses'], record['original']):
            try:
                inf.write_memory(address, original)
                if bytes(inf.read_memory(address, 4)) != original:
                    errors.append(address)
            except Exception as error:
                errors.append(str(error))
        state['glue_restored'] += 1
        if errors:
            state['glue_fail'] += 1
        log('GLUE-RESTORE', swarm=swarm, errors=errors,
            readback=[bytes(inf.read_memory(a, 4)).hex() for a in record['addresses']])
    state['glue'] = {}


class PartitionTick(gdb.Breakpoint):
    """Every 250th UpdatePartition of a swarm: log its live boid translations, so a later driver can
    `tp` the player onto a real boid (the player's box overlap is what the Touch arms need)."""

    def stop(self):
        state['ticks'] += 1
        this = iv('$rdi')
        if this not in state['vuln']:
            state['vuln'].append(this)
            vulnerability(this, len(state['vuln']))
        force_windows(this)
        glue(this)
        if state['ticks'] % 250:
            return False
        try:
            editor = iv('((CEntity*)%d)->mEditorId.value' % this)
            count = iv('((CSwarmBasics*)%d)->mBoids.mCount' % this)
            rows = []
            for i in range(min(count, 8)):
                base = '((CSwarmBasics*)%d)->mBoids.mItems[%d]' % (this, i)
                if not iv(base + '.mActive'):
                    continue
                rows.append([round(float(ev(base + '.mTransform.m%d3' % axis)), 3)
                             for axis in range(3)])
            log('BOIDS', n=state['ticks'], swarm=this, editor_id=editor, count=count,
                touch_radius=float(ev('((CSwarmBasics*)%d)->mTouchRadius' % this)),
                positions=rows)
        except gdb.error as error:
            log('BOIDS-ERR', error=str(error)[:120])
        return False

# ---------------------------------------------------------------------------- the Touch logger

def ball_state(player):
    try:
        ball = iv('((CPlayer*)%d)->mMorphBall.mPtr' % player)
        return iv('((CMorphBall*)%d)->mBallState' % ball) if ball else -1
    except gdb.error:
        return -2

class TouchLog(gdb.Breakpoint):
    def stop(self):
        try:
            this, mgr = iv('$rdi'), iv('$rdx')
            state['mgr'] = mgr
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
    summary = ('SUMMARY exit: ctor_sites=%d forced=%d restored=%d restore_fail=%d touch=%d '
               'glue_applied=%d glue_restored=%d glue_fail=%d err=%s'
               % (state['ctor'], state['forced'], state['restored'], state['restore_fail'],
                  state['touch'], state['glue_applied'], state['glue_restored'],
                  state['glue_fail'], state['err'] or '-'))
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
    ctor, offset = ctor_site()
    CtorSite('*(%s + %d)' % (quote(ctor), offset), internal=True)
    TouchLog('*' + symbol('CSwarmBasics::Touch('), internal=True)
    PartitionTick('*' + symbol('CSwarmBasics::UpdatePartition('), internal=True)
    gdb.events.exited.connect(finished)

if os.environ.get('SWB19_SELFTEST') != '1':
    install()
