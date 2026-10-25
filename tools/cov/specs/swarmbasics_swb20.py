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
  - `CLightComboProjectile::CreateRay` result -> kInvalidUniqueId (once, CR_AT-th call).
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
EN_FROM = int(os.environ.get('SWB20_EN_FROM', '30'))
EN_COUNT = int(os.environ.get('SWB20_EN_COUNT', '80'))
EX_AT = int(os.environ.get('SWB20_EX_AT', '6'))
CR_AT = int(os.environ.get('SWB20_CR_AT', '2'))
RAD_START = int(os.environ.get('SWB20_RAD_START', '9000'))
RAD_END = int(os.environ.get('SWB20_RAD_END', '9600'))

state.update({'sites': {}, 'hits': {}, 'forced_sites': {}, 'radius': {}, 'radius_fail': 0,
              'aux': 0, 'combo': 0, 'radius_done': False, 'light_touch': 0, 'boids_logged': 0})

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
    def __init__(self, key, offset):
        super().__init__('*(%s + %d)' % (quote(TOUCH), offset), internal=True)
        self.key = key

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
            elif key == 'EN' and EN_FROM <= n < EN_FROM + EN_COUNT and rax:
                self.force(n, rax, 0, 0xffffffffffffffff, 0)
                if state['boids_logged'] < 40:
                    state['boids_logged'] += 1
                    self.boids()
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

    def force(self, n, real, value, mask, expect):
        write('$rax', str(value if value < (1 << 63) else value - (1 << 64)))
        readback = iv('$rax') & mask
        check(readback == expect, '%s forced result read back: %x' % (self.key, readback))
        state['forced_sites'][self.key] = state['forced_sites'].get(self.key, 0) + 1
        log('FORCE-RET', key=self.key, n=n, real=hex(real), written=hex(value),
            readback=hex(readback), swarm=state.get('touch_this', 0),
            actor=state.get('touch_actor', 0), actor_type=state.get('touch_type', '?'))

class TouchLog20(TouchLog):
    """swb19's logger, plus the current Touch frame's swarm/actor for the force logs."""
    def stop(self):
        state['touch_this'] = iv('$rdi')
        state['touch_actor'] = iv('$rsi')
        name = dyn(state['touch_actor'])
        state['touch_type'] = name
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
               'glue_applied=%d glue_fail=%d radius_fail=%d freeze=%s screw=%s spawn=%s err=%s'
               % (state['touch'], state['hits'], state['forced_sites'], state['light_touch'],
                  state['aux'], state['combo'], state['glue_applied'], state['glue_fail'],
                  state['radius_fail'], state['freeze'], state['screw'], state['spawn'],
                  state['err'] or '-'))
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
        ReturnForce(key, site['ret'])
    TouchLog20('*' + TOUCH, internal=True)
    PartitionTick20('*' + symbol('CSwarmBasics::UpdatePartition('), internal=True)
    AuxFire('*' + symbol('CAuxWeapon::Fire('), internal=True)
    LightCombo('*' + symbol('CAuxWeapon::FireLightCombo('), internal=True)
    gdb.events.exited.connect(finished20)

if os.environ.get('SWB20_SELFTEST') != '1':
    install20()
