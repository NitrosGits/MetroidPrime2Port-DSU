"""swb20: the rest of fn_80_220C CSwarmBasics::Touch.

    COV_GDB_SCRIPT=tools/cov/specs/swarmbasics_swb20.py BUILD=$PWD/build-port-cov \
      tools/cov/force.sh tools/cov/specs/swarmbasics_swb20.py <out> 42b935e4,01_sand_entrance \
      --con tools/drivers/swb20_touch.con --fast --frames 3400

Reuses swarmbasics_swb19.py's Touch logger, stand-in boid (glue) and freeze/screw/spawn windows
(its ctor fixture is not installed: that row is marked).  Adds, each tied to one Touch frame:

* return-site forces, at the instruction right after a call inside Touch's own body (so the stop
  is Touch's frame by construction), each logged with the real value and the written one:
  - `WeaponHits` result -> false (once, WH_AT-th projectile touch): Touch's `WeaponHits` false side;
  - `TCastToPtr<CEnergyProjectile>` result -> null (EN_COUNT overlaps from the EN_FROM-th): the
    non-energy path (`mHealth -=`, `KillBoid`), the `energy &&` false side;
  - `CEnergyProjectile::Explode` result -> false (once, EX_AT-th call): the reflect block;
  - `CLightComboProjectile::CreateRay` result -> kInvalidUniqueId (once, CR_AT-th call);
  - the projectile's `GetTouchBounds` optional -> empty (once, TB_AT-th call; line 1472).
* field writes snapshotted at Touch entry / a return site and restored (readback) when that same
  Touch frame returns (FinishBreakpoint): boid mHealth 0.001 on the KILL_AT-th null energy cast
  (KillBoid), and on PL_COUNT player Touch frames mPlayerTouchRadius -> PL_RADIUS (first one also
  freezes one boid) for the player-damage / cooldown / frozen-boid sides.
* `mPlayerTouchRadius` (all four sand-entrance swarms have 0) written to the swarm's own
  mTouchRadius for a window, snapshotted first and restored with a readback.
* read-only logs of the gun's charge-combo path (CAuxWeapon::Fire / FireLightCombo, the
  CLightComboProjectile Touch entries) so a real Sunburst can be shown to reach Touch.
"""
import os
os.environ['SWB19_SELFTEST'] = '1'
os.environ.setdefault('SWB19_GLUE_START', '8000')
_HERE = os.path.dirname(os.path.realpath(os.environ.get('COV_SPEC', __file__)))
exec(open(os.path.join(_HERE, 'swarmbasics_swb19.py')).read(), globals())

LOG = os.path.join(COV_OUT, 'swb20.jsonl')

WH_AT = int(os.environ.get('SWB20_WH_AT', '40'))
EN_FROM = int(os.environ.get('SWB20_EN_FROM', '4'))
EN_COUNT = int(os.environ.get('SWB20_EN_COUNT', '80'))
EX_AT = int(os.environ.get('SWB20_EX_AT', '6'))
CR_AT = int(os.environ.get('SWB20_CR_AT', '2'))
# Run 1's tick window (mPlayerTouchRadius := mTouchRadius); off by default since the per-frame
# player write below covers line 1527 too.
RAD_START = int(os.environ.get('SWB20_RAD_START', '0'))
RAD_END = int(os.environ.get('SWB20_RAD_END', '0'))
TB_AT = int(os.environ.get('SWB20_TB_AT', '50'))
KILL_AT = int(os.environ.get('SWB20_KILL_AT', '3'))
PL_START = int(os.environ.get('SWB20_PL_START', '9000'))
PL_COUNT = int(os.environ.get('SWB20_PL_COUNT', '12'))
PL_RADIUS = os.environ.get('SWB20_PL_RADIUS', '100.0')

state.update({'sites': {}, 'hits': {}, 'forced_sites': {}, 'radius': {}, 'radius_fail': 0,
              'aux': 0, 'combo': 0, 'radius_done': False, 'light_touch': 0, 'boids_logged': 0,
              'pl': 0, 'kill': 0, 'frame_writes': 0, 'frame_restores': 0, 'frame_fail': 0,
              'en_null': 0})

TOUCH = None

def touch_sites():
    """Return addresses inside Touch, found in its own disassembly: the instruction after each
    call.  Offsets relocate with the PIE (set as symbol+offset)."""
    text = gdb.execute('disassemble ' + quote(TOUCH), to_string=True).splitlines()
    rows = []
    for line in text:
        if '<+' not in line:
            continue
        offset = int(line.split('<+')[1].split('>')[0])
        rows.append((offset, line.split(':', 1)[1].strip()))
    wanted = {'WH': 'CDamageVulnerability10WeaponHits', 'EN': 'TCastToPtrI17CEnergyProjectile',
              'CR': 'CLightComboProjectile9CreateRay', 'EX': 'call   *%rax'}
    found = {}
    # The projectile's GetTouchBounds (virtual, sret) is the indirect call whose result's valid
    # byte is tested next (`cmpb $0x0,<off>(%rbp)`, line 1472); the player's has no such test.
    tb = [i for i, (_o, ins) in enumerate(rows)
          if ins.startswith('call') and '*0x88(%rax)' in ins and rows[i + 2][1].startswith('cmpb   $0x0,')]
    check(len(tb) == 1, 'one projectile GetTouchBounds call in Touch: %d' % len(tb))
    i = tb[0]
    found['TB'] = dict(call=rows[i][0], ret=rows[i + 1][0], next=rows[i + 1][1],
                       after=[r[1] for r in rows[i + 1:i + 4]],
                       valid=rows[i + 2][1].split(',')[1].split('(')[0])
    for key, needle in wanted.items():
        hits = [i for i, (_o, ins) in enumerate(rows) if ins.startswith('call') and needle in ins]
        check(len(hits) == 1, 'one %s call in Touch: %d' % (key, len(hits)))
        i = hits[0]
        found[key] = dict(call=rows[i][0], ret=rows[i + 1][0], next=rows[i + 1][1],
                          after=[r[1] for r in rows[i + 1:i + 4]])
    # The Explode site is the only indirect call through %rax; its result test must follow.
    check('test   %al,%al' in found['EX']['after'][0], 'Explode result test after the call')
    check('test   %al,%al' in ' '.join(found['WH']['after']), 'WeaponHits result test')
    check('test   %rax,%rax' in found['EN']['after'][0], 'energy cast null test')
    check('cmp    %ax,' in ' '.join(found['CR']['after']), 'CreateRay id compare')
    log('SITES', **found)
    return found

def invalid_uid():
    return iv('kInvalidUniqueId.value')

class ReturnForce(gdb.Breakpoint):
    def __init__(self, key, offset, valid=None):
        super().__init__('*(%s + %d)' % (quote(TOUCH), offset), internal=True)
        self.key = key
        self.valid = valid

    def stop(self):
        key = self.key
        n = state['hits'][key] = state['hits'].get(key, 0) + 1
        try:
            frame = gdb.newest_frame()
            if 'CSwarmBasics::Touch' not in (frame.name() or ''):
                log('SKIP', key=key, frame=frame.name())
                return False
            rax = iv('$rax') & 0xffffffffffffffff
            if key == 'WH' and n == WH_AT:
                self.force(n, rax, rax & ~0xff, 0xff, 0)
            elif key == 'TB' and n == TB_AT:
                # optional_object<CAABox> valid byte in Touch's own sret slot -> false.
                address = iv('(long)$rbp') + int(self.valid, 16) if not self.valid.startswith('-') \
                    else iv('(long)$rbp') - int(self.valid[1:], 16)
                inf = gdb.selected_inferior()
                real = bytes(inf.read_memory(address, 1))
                inf.write_memory(address, b'\x00')
                readback = bytes(inf.read_memory(address, 1))
                check(readback == b'\x00', 'TB valid byte read back')
                state['forced_sites']['TB'] = state['forced_sites'].get('TB', 0) + 1
                log('FORCE-RET', key='TB', n=n, real=real.hex(), written='00', readback=readback.hex(),
                    address=address, swarm=state.get('touch_this', 0),
                    actor=state.get('touch_actor', 0), actor_type=state.get('touch_type', '?'))
            elif key == 'EN' and EN_FROM <= n < EN_FROM + EN_COUNT and rax:
                self.force(n, rax, 0, 0xffffffffffffffff, 0)
                if state['boids_logged'] < 40:
                    state['boids_logged'] += 1
                    self.boids()
                self.en_null()
            elif key == 'EN' and not rax:
                log('NATURAL', key=key, n=n, rax=hex(rax))
                self.en_null()
            elif key == 'EX' and n == EX_AT:
                self.force(n, rax, rax & ~0xff, 0xff, 0)
            elif key == 'CR' and n == CR_AT:
                self.force(n, rax, (rax & ~0xffff) | invalid_uid(), 0xffff, invalid_uid())
            elif n <= 3 or n % 500 == 0:
                log('NATURAL', key=key, n=n, rax=hex(rax))
        except Exception as error:
            log('FORCE-ERR', key=key, n=n, error='%s: %s' % (type(error).__name__, error))
        return False

    def boids(self):
        swarm = state.get('touch_this', 0)
        try:
            count = iv('((CSwarmBasics*)%d)->mBoids.mCount' % swarm)
            health = [round(float(ev('((CSwarmBasics*)%d)->mBoids.mItems[%d].mHealth' % (swarm, i))), 3)
                      for i in range(min(count, 8))]
            log('BOID-HEALTH', swarm=swarm, health=health)
        except gdb.error as error:
            log('BOID-HEALTH-ERR', error=str(error)[:100])
        return 0

    def en_null(self):
        # KILL_AT-th null energy cast (non-energy path): every active boid of this swarm gets
        # mHealth 0.001 for this Touch frame only, so the `mHealth <= 0` arm (KillBoid) runs on
        # the overlapping ones; all are put back when this Touch returns, with a readback.
        state['en_null'] += 1
        if state['en_null'] != KILL_AT:
            return
        swarm = state.get('touch_this', 0)
        count = iv('((CSwarmBasics*)%d)->mBoids.mCount' % swarm)
        records = []
        for i in range(count):
            boid = '((CSwarmBasics*)%d)->mBoids.mItems[%d]' % (swarm, i)
            if iv('(int)%s.mActive' % boid):
                records.append(snapshot_write('%s.mHealth' % boid, '0.001'))
        state['kill'] = len(records)
        log('KILL-HEALTH', swarm=swarm, boids=len(records),
            original=[r['original'].hex() for r in records])
        FrameRestore('KILL', records, swarm)

    def force(self, n, real, value, mask, expect):
        write('$rax', str(value if value < (1 << 63) else value - (1 << 64)))
        readback = iv('$rax') & mask
        check(readback == expect, '%s forced result read back: %x' % (self.key, readback))
        state['forced_sites'][self.key] = state['forced_sites'].get(self.key, 0) + 1
        log('FORCE-RET', key=self.key, n=n, real=hex(real), written=hex(value),
            readback=hex(readback), swarm=state.get('touch_this', 0),
            actor=state.get('touch_actor', 0), actor_type=state.get('touch_type', '?'))

def snapshot_write(lvalue, value):
    inf = gdb.selected_inferior()
    address = iv('(long)&(%s)' % lvalue)
    original = bytes(inf.read_memory(address, 4))
    write(lvalue, value)
    state['frame_writes'] += 1
    return dict(lvalue=lvalue, address=address, original=original,
                written=float(ev(lvalue)))

class FrameRestore(gdb.FinishBreakpoint):
    """Put back snapshotted fields when the owning Touch frame (the newest one) returns."""
    def __init__(self, label, records, swarm):
        super().__init__(gdb.newest_frame(), internal=True)
        self.label, self.records, self.swarm = label, records, swarm

    def restore(self, how):
        inf = gdb.selected_inferior()
        ok = True
        for record in self.records:
            inf.write_memory(record['address'], record['original'])
            if bytes(inf.read_memory(record['address'], 4)) != record['original']:
                ok = False
        state['frame_restores'] += 1
        if not ok:
            state['frame_fail'] += 1
        log('FRAME-RESTORE', label=self.label, how=how, ok=ok, swarm=self.swarm,
            fields=len(self.records))

    def stop(self):
        try:
            extra = {}
            if self.label.startswith('PL'):
                extra = dict(cooldown_timer=float(ev('((CSwarmBasics*)%d)->mDamageCooldownTimer' % self.swarm)))
            elif self.label == 'KILL':
                count = iv('((CSwarmBasics*)%d)->mBoids.mCount' % self.swarm)
                extra = dict(active=[iv('(int)((CSwarmBasics*)%d)->mBoids.mItems[%d].mActive' % (self.swarm, i))
                                     for i in range(count)])
            log('FRAME-END', label=self.label, **extra)
            self.restore('return')
        except Exception as error:
            state['frame_fail'] += 1
            log('FRAME-ERR', label=self.label, error='%s: %s' % (type(error).__name__, error))
        return False

    def out_of_scope(self):
        try:
            self.restore('out_of_scope')
        except Exception as error:
            state['frame_fail'] += 1
            log('FRAME-ERR', label=self.label, error=str(error)[:100])

def player_frame(this):
    # PL_COUNT player Touch frames from tick PL_START: mPlayerTouchRadius (data 0) -> PL_RADIUS so
    # the player's bounds overlap a boid (lines 1547-1556: ApplyDamage when not ball-damaging,
    # then the cooldown-still-running side on the next frames); the first one also freezes the
    # first active boid (mFreezeTimer 1.0, line 1543's frozen side).  Restored at Touch return.
    if state['ticks'] < PL_START or state['pl'] >= PL_COUNT:
        return
    state['pl'] += 1
    swarm = '((CSwarmBasics*)%d)' % this
    records = [snapshot_write('%s->mPlayerTouchRadius' % swarm, PL_RADIUS)]
    frozen = None
    if state['pl'] == 1:
        for i in range(iv('%s->mBoids.mCount' % swarm)):
            boid = '%s->mBoids.mItems[%d]' % (swarm, i)
            if iv('(int)%s.mActive' % boid):
                records.append(snapshot_write('%s.mFreezeTimer' % boid, '1.0'))
                frozen = i
                break
    log('PL-FRAME', n=state['pl'], swarm=this, tick=state['ticks'], frozen=frozen,
        written=[r['written'] for r in records], original=[r['original'].hex() for r in records],
        cooldown=float(ev('%s->mDamageCooldown' % swarm)),
        cooldown_timer=float(ev('%s->mDamageCooldownTimer' % swarm)))
    FrameRestore('PL%d' % state['pl'], records, this)

class TouchLog20(TouchLog):
    """swb19's logger, plus the current Touch frame's swarm/actor for the force logs."""
    def stop(self):
        state['touch_this'] = iv('$rdi')
        state['touch_actor'] = iv('$rsi')
        name = dyn(state['touch_actor'])
        state['touch_type'] = name
        if name == 'CPlayer *':
            try:
                player_frame(state['touch_this'])
            except Exception as error:
                state['frame_fail'] += 1
                log('PL-ERR', error='%s: %s' % (type(error).__name__, error))
        if 'LightCombo' in name or 'Plasma' in name or 'Beam' in name:
            state['light_touch'] += 1
            if state['light_touch'] <= 40:
                log('LIGHT-TOUCH', n=state['light_touch'], swarm=state['touch_this'],
                    actor=state['touch_actor'], type=name)
        return TouchLog.stop(self)

def radius_window(this):
    tick = state['ticks']
    inf = gdb.selected_inferior()
    if RAD_START <= tick <= RAD_END and not state['radius_done'] and this not in state['radius']:
        try:
            address = iv('(long)&((CSwarmBasics*)%d)->mPlayerTouchRadius' % this)
            original = bytes(inf.read_memory(address, 4))
            value = gdb.parse_and_eval('((CSwarmBasics*)%d)->mTouchRadius' % this)
            state['radius'][this] = dict(address=address, original=original)
            inf.write_memory(address, bytes(inf.read_memory(
                iv('(long)&((CSwarmBasics*)%d)->mTouchRadius' % this), 4)))
            log('RADIUS', swarm=this, original=original.hex(), written=float(value),
                readback=float(ev('((CSwarmBasics*)%d)->mPlayerTouchRadius' % this)), tick=tick)
        except gdb.error as error:
            log('RADIUS-ERR', error=str(error)[:100])
    if tick > RAD_END and state['radius'] and not state['radius_done']:
        for swarm, record in state['radius'].items():
            inf.write_memory(record['address'], record['original'])
            ok = bytes(inf.read_memory(record['address'], 4)) == record['original']
            if not ok:
                state['radius_fail'] += 1
            log('RADIUS-UNDO', swarm=swarm, ok=ok,
                readback=bytes(inf.read_memory(record['address'], 4)).hex())
        state['radius_done'] = True

class PartitionTick20(PartitionTick):
    def stop(self):
        result = PartitionTick.stop(self)
        try:
            if RAD_START < RAD_END:
                radius_window(iv('$rdi'))
        except Exception as error:
            log('RADIUS-ERR', error='%s: %s' % (type(error).__name__, error))
        return result

class AuxFire(gdb.Breakpoint):
    def stop(self):
        state['aux'] += 1
        try:
            if state['aux'] <= 60:
                log('AUX-FIRE', n=state['aux'], beam=iv('$edx'), charge=iv('$ecx'),
                    loaded=iv('((CAuxWeapon*)$rdi)->mIsLoaded'))
        except gdb.error as error:
            log('AUX-ERR', error=str(error)[:100])
        return False

class LightCombo(gdb.Breakpoint):
    def stop(self):
        state['combo'] += 1
        log('FIRE-LIGHT-COMBO', n=state['combo'])
        return False

def finished20(event):
    summary = ('SUMMARY exit: touch=%d hits=%s forced=%s light_touch=%d aux=%d combo=%d '
               'glue_applied=%d glue_fail=%d radius_fail=%d freeze=%s screw=%s spawn=%s err=%s '
               'pl=%d kill=%d frame_writes=%d frame_restores=%d frame_fail=%d en_null=%d'
               % (state['touch'], state['hits'], state['forced_sites'], state['light_touch'],
                  state['aux'], state['combo'], state['glue_applied'], state['glue_fail'],
                  state['radius_fail'], state['freeze'], state['screw'], state['spawn'],
                  state['err'] or '-', state['pl'], state['kill'], state['frame_writes'],
                  state['frame_restores'], state['frame_fail'], state['en_null']))
    log('EXIT', summary=summary)
    with open(os.path.join(COV_OUT, 'swb20_summary.txt'), 'w') as f:
        f.write(summary + '\n')
        for key in sorted(state['keys']):
            f.write('  %-46s %d\n' % (key, state['keys'][key]))

def install20():
    global TOUCH
    binary = gdb.current_progspace().filename
    check(bool(binary), 'GDB executable identity')
    with open(binary, 'rb') as f:
        digest = hashlib.file_digest(f, 'sha256').hexdigest()
    log('BINARY', path=binary, sha256=digest)
    TOUCH = symbol('CSwarmBasics::Touch(')
    for key, site in touch_sites().items():
        ReturnForce(key, site['ret'], site.get('valid'))
    TouchLog20('*' + TOUCH, internal=True)
    PartitionTick20('*' + symbol('CSwarmBasics::UpdatePartition('), internal=True)
    AuxFire('*' + symbol('CAuxWeapon::Fire('), internal=True)
    LightCombo('*' + symbol('CAuxWeapon::FireLightCombo('), internal=True)
    gdb.events.exited.connect(finished20)

if os.environ.get('SWB20_SELFTEST') != '1':
    install20()
