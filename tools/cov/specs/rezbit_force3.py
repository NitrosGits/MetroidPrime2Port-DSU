# Rezbit (REL 53) rez5: direct-call forcing of the leaf bodies rez4 left (UpdateShield, LowerShields,
# BreakShield, UpdateSeparation, CheckVerticalClearance, JoinTeam, UpdatePatrolMovement,
# SetupLaserTargets, UpdateLaser, ...). gdb is the whole driver: at the REZ5_AT-th live
# CRezbit::Think entry it writes the members each arm needs, `call`s the body, and writes them back
# (the spl5 / dtr9 way); every section ends with PortCoverageFlush() (= __gcov_dump + reset), so a
# stalled or killed run keeps what ran. Each section's results go to <out>/rez5.log.
#   COV_GDB_SCRIPT=$PWD/tools/cov/specs/rezbit_force3.py BUILD=$PWD/build-port-cov MP2_CONSOLE_WAIT=120 \
#     SECS=600 tools/cov/force.sh tools/cov/specs/rezbit_force3.py <out> 1baa96c2,02_cliff_reactorcore \
#     --con tools/drivers/rezbit_forced4.con --fast --frames 1800
import os
import gdb

CLASS = 'CRezbit'
FORCE = []
OUT = os.environ['COV_OUT']
LOG = OUT + '/rez5.log'

syms = []
for ln in open(os.environ['COV_NM']):
    parts = ln.rstrip('\n').split('\t')
    if len(parts) == 2:
        f = parts[0].split()
        if len(f) == 3 and f[1] in 'tTW':
            syms.append((f[2], parts[1]))


def addr(prefix):
    hits = [m for m, d in syms if d.startswith(prefix) and '.cold' not in d and 'clone' not in d]
    if len(hits) != 1:
        raise gdb.GdbError('%s: %d symbols' % (prefix, len(hits)))
    return '*' + hits[0]


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def run(cmd):
    try:
        out = gdb.execute(cmd, to_string=True)
        log('> %s\n%s' % (cmd, out.rstrip()))
        return out
    except gdb.error as e:
        log('> %s\n  ERROR %s' % (cmd, e))
        return ''


def ev(expr):
    return gdb.parse_and_eval(expr)


def flush():
    run('call (void)PortCoverageFlush()')


gdb.execute('set $busy = 0')
gdb.execute('set $done = 0')
gdb.execute('set $fbsd = -99')   # CPatterned::FindBestStepDirection answer, -99 = real

# members (lvalues; `$t` is the Rezbit under test, `$m` the CStateManager)
BRK = '$t->mShieldBroken'
ALERT = '$t->mAlert'
RZ = '$t->mRezState'
AK = '$t->mAttackKind'
TMR = '$t->mShieldTimer'
SID = '$t->mShieldActorId.value'
SFX = '*(unsigned*)&$t->mShieldSfx'
AD = '$t->mBodyController.mPtr->mBodyStateInfo.mAdditiveState'
TID = '$t->mTargetId.value'
EFF = '$t->mShieldExplodeEffect.m_valid'
PLID = '$m->mPlayers[0]->mUniqueId.value'
FLOATS = (TMR,)


def snap(lvs):
    d = {}
    for lv in lvs:
        v = ev(lv)
        d[lv] = float(v) if lv in FLOATS else int(v)
    return d


def put(d):
    for lv, v in d.items():
        run('set var %s = %r' % (lv, v))


def w(lv, v):
    run('set var %s = %s' % (lv, v))


class FbsdRet(gdb.FinishBreakpoint):
    def stop(self):
        gdb.execute('set var $rax = $fbsd')
        return False

    def out_of_scope(self):
        pass


class FbsdBp(gdb.Breakpoint):
    def stop(self):
        if int(gdb.convenience_variable('fbsd')) != -99:
            FbsdRet(gdb.newest_frame(), internal=True)
        return False


FbsdBp(addr('CPatterned::FindBestStepDirection('), internal=True)

class GatRet(gdb.FinishBreakpoint):
    def stop(self):
        gdb.execute('set var $rax = $gat')
        return False

    def out_of_scope(self):
        pass


class GatBp(gdb.Breakpoint):
    def stop(self):
        if int(gdb.convenience_variable('gat')) != -99:
            GatRet(gdb.newest_frame(), internal=True)
        return False


gdb.execute('set $gat = -99')
GatBp(addr('CScriptTeamAiMgr::GetAssociatedTeamId('), internal=True)

SHIELD_LVS = [BRK, ALERT, RZ, AK, TMR, SID, SFX, AD]


def shield_group():
    base = snap(SHIELD_LVS)
    sid0 = base[SID]

    def us(label, brk, rz, alert, ak, tmr, ad, sid=None, sfx=0):
        log('# UpdateShield %s' % label)
        put({BRK: brk, RZ: rz, ALERT: alert, AK: ak, TMR: tmr, AD: ad, SID: sid0 if sid is None else sid, SFX: sfx})
        run('call $t->UpdateShield(*$m, 0.016f)')
        put(base)

    for ad in (28, 0):
        us('broken ad=%d' % ad, 1, 0, 1, 0, 1e9, ad)
        us('rez1 ad=%d' % ad, 0, 1, 1, 0, 1e9, ad)
        us('no alert ad=%d' % ad, 0, 0, 0, 0, 1e9, ad)
        us('attack kind 2 ad=%d' % ad, 0, 0, 1, 2, 1e9, ad)
        us('timer running ad=%d' % ad, 0, 0, 1, 0, 1e9, ad)
        us('timer elapsed ad=%d' % ad, 0, 0, 1, 0, -1.0, ad)
        us('ak 5 timer elapsed ad=%d' % ad, 0, 0, 1, 5, -1.0, ad)
        us('ak -1 timer elapsed ad=%d' % ad, 0, 0, 1, -1, -1.0, ad)
        us('no shield actor ad=%d' % ad, 0, 0, 1, 0, 1e9, ad, sid=0xffff)
        us('sfx live ad=%d' % ad, 0, 0, 1, 0, 1e9, ad, sfx=0x1234)
        us('sfx live no shield ad=%d' % ad, 0, 0, 1, 0, 1e9, ad, sid=0xffff, sfx=0x1234)
    flush()

    def ls(label, brk, ad, sid=None, sfx=0):
        log('# LowerShields %s' % label)
        put({BRK: brk, AD: ad, SID: sid0 if sid is None else sid, SFX: sfx})
        run('call $t->LowerShields(*$m)')
        put(base)

    ls('broken', 1, 28)
    for ad in (28, 0):
        for sid in (None, 0xffff):
            for sfx in (0, 0x1234):
                ls('ad=%d sid=%s sfx=%#x' % (ad, sid, sfx), 0, ad, sid, sfx)
    flush()

    def bs(label, brk, ad, sid=None, sfx=0, eff=None):
        log('# BreakShield %s' % label)
        e0 = int(ev(EFF))
        put({BRK: brk, AD: ad, SID: sid0 if sid is None else sid, SFX: sfx})
        if eff is not None:
            w(EFF, eff)
        run('call $t->BreakShield(*$m)')
        w(EFF, e0)
        put(base)

    bs('already broken', 1, 28)
    bs('loop reaction, sfx live, effect valid', 0, 28, sfx=0x1234)
    bs('idle, no sfx, effect valid', 0, 0)
    bs('effect invalid, sfx live', 0, 0, sfx=0x1234, eff=0)
    bs('effect invalid, no sfx', 0, 0, eff=0)
    bs('no shield actor', 0, 28, sid=0xffff)
    flush()


def misc_group():
    # --- UpdateSeparation --------------------------------------------------------------------------
    log('# UpdateSeparation')
    run('call $t->UpdateSeparation(*$m)')
    a0 = int(ev('$t->mAreaId.value'))
    w('$t->mAreaId.value', a0 + 1)
    run('call $t->UpdateSeparation(*$m)')
    w('$t->mAreaId.value', a0)
    run('set $ol = *(CObjectList**)($m->mObjectLists.mData + 16 * (int)kOL_ListeningAi + 8)')
    i = int(ev('$ol->mFirstId'))
    others = []
    while i != -1:
        e = ev('$ol->mObjects[%d].mEntity' % i)
        if int(e) != int(ev('$t')):
            others.append((i, int(e)))
        i = int(ev('$ol->mObjects[%d].mNext' % i))
    log('# listening AI: %d others' % len(others))
    px = [float(ev('$t->mPosition.m' + c)) for c in 'XYZ']
    for n, (idx, ptr) in enumerate(others[:3]):
        log('# UpdateSeparation next to listener %d (%#x)' % (n, ptr))
        for j, c in enumerate('XYZ'):
            ov = float(ev('((CActor*)%d)->mPosition.m%s' % (ptr, c)))
            w('$t->mPosition.m' + c, ov + (0.5 if j == 0 else 0.0))
        run('call $t->UpdateSeparation(*$m)')
    for j, c in enumerate('XYZ'):
        w('$t->mPosition.m' + c, px[j])
    flush()
    # --- JoinTeam ------------------------------------------------------------------------------------
    t0 = int(ev('$t->mTeamAiMgrId.value'))
    for label, val in (('invalid', 0xffff), ('stale id', 0xfffe), ('self (not a team)', int(ev('$t->mUniqueId.value'))),
                       ('as found', t0)):
        log('# JoinTeam %s' % label)
        w('$t->mTeamAiMgrId.value', val)
        run('call $t->JoinTeam(*$m)')
    log('# JoinTeam lookup finds no team')
    w('$t->mTeamAiMgrId.value', 0xffff)
    run('set $gat = 65535')
    run('call $t->JoinTeam(*$m)')
    run('set $gat = -99')
    w('$t->mTeamAiMgrId.value', t0)
    flush()
    # --- CheckVerticalClearance -----------------------------------------------------------------------
    run('set $av = (unsigned char*)malloc(16)')
    tid0 = int(ev(TID))
    sd0 = float(ev('$t->mStrafeDistance'))
    posz = float(ev('$t->mPosition.mZ'))

    def setz(z):
        w('$t->mPosition.mZ', z)

    def cvc(label):
        for i in range(4):
            w('$av[%d]' % (4 + i), 1)
        w('*(int*)$av', 4)
        log('# CheckVerticalClearance %s' % label)
        run('call $t->CheckVerticalClearance(*$m, *(rstl::reserved_vector<bool,4>*)$av)')

    w(TID, 0xffff)
    cvc('no target')
    w(TID, PLID)
    pz = float(ev('$m->mPlayers[0]->mPosition.mZ'))
    for dz in (30.0, 10.0, 5.0, 0.0, -5.0, -100.0, 1000.0):
        setz(pz + dz)
        for sd in (-100.0, 0.0, 5.0, 10.0, 100.0):
            w('$t->mStrafeDistance', sd)
            cvc('dz=%s sd=%s' % (dz, sd))
    w('$t->mPosition.mZ', posz)
    w('$t->mStrafeDistance', sd0)
    w(TID, tid0)
    flush()
    # --- UpdatePatrolMovement --------------------------------------------------------------------------
    mv = '$t->mBodyController.mPtr->mCmdMgr.mMove'
    old = [float(ev(mv + '.m' + c)) for c in 'XYZ']
    for label, vec in (('safe', (1.0, 2.0, 0.5)), ('unsafe', (0.0, 0.0, 0.0))):
        for fb in (-99, 0, 1, 2, 3, 4, -1):
            for i, c in enumerate('XYZ'):
                w(mv + '.m' + c, vec[i])
            run('set $fbsd = %d' % fb)
            log('# UpdatePatrolMovement %s dir=%d' % (label, fb))
            run('call $t->UpdatePatrolMovement()')
            run('set $fbsd = -99')
    for i, c in enumerate('XYZ'):
        w(mv + '.m' + c, old[i])
    flush()
    # --- SetupLaserTargets / SpawnLaser / UpdateLaser / DeleteLaser -----------------------------------
    ld0 = float(ev('$t->mLaserDuration'))
    lid0 = int(ev('$t->mLaserId.value'))
    w('$t->mLaserDuration', 2.0)
    w(TID, 0xffff)
    log('# SetupLaserTargets no target')
    run('call $t->SetupLaserTargets(*$m)')
    w(TID, int(ev('$t->mUniqueId.value')))
    log('# SetupLaserTargets target is itself')
    run('call $t->SetupLaserTargets(*$m)')
    # unsafe perpendicular: this stands where the laser centre is (xy), the target is the player, duration 0
    w(TID, PLID)
    w('$t->mLaserDuration', 0.0)
    ax = float(ev('$m->mPlayers[0]->GetAimPosition(*$m, 0.0f).mX'))
    ay = float(ev('$m->mPlayers[0]->GetAimPosition(*$m, 0.0f).mY'))
    tx = float(ev('$m->mPlayers[0]->mPosition.mX'))
    ty = float(ev('$m->mPlayers[0]->mPosition.mY'))
    px0, py0 = float(ev('$t->mPosition.mX')), float(ev('$t->mPosition.mY'))
    w('$t->mPosition.mX', 0.5 * (tx + ax))
    w('$t->mPosition.mY', 0.5 * (ty + ay))
    log('# SetupLaserTargets perpendicular unsafe')
    run('call $t->SetupLaserTargets(*$m)')
    w('$t->mPosition.mX', px0)
    w('$t->mPosition.mY', py0)
    w('$t->mLaserDuration', 2.0)
    log('# SetupLaserTargets normal')
    run('call $t->SetupLaserTargets(*$m)')
    w('$t->mLaserId.value', 0xffff)
    log('# UpdateLaser no laser')
    run('call $t->UpdateLaser(*$m, 0.016f)')
    log('# SpawnLaser + UpdateLaser found')
    run('call $t->SpawnLaser(*$m, 0.016f)')
    run('call $t->UpdateLaser(*$m, 0.016f)')
    run('call $t->UpdateLaser(*$m, 0.016f)')
    run('call $t->DeleteLaser(*$m)')
    w('$t->mLaserDuration', ld0)
    w('$t->mLaserId.value', lid0)
    w(TID, tid0)
    flush()


def hook():
    gdb.execute('set $busy = 1')
    try:
        run('set $t = (CRezbit*)$rdi')
        run('set $m = (CStateManager*)$rsi')
        log('# rezbit %s' % ev('$t'))
        which = os.environ.get('REZ5_GROUPS', 'shield,misc').split(',')
        if 'shield' in which:
            shield_group()
        if 'misc' in which:
            misc_group()
        gdb.execute('set $done = 1')
        log('# done')
    except Exception as e:
        log('EXC %r' % (e,))
        gdb.execute('set $done = 1')
    finally:
        gdb.execute('set $busy = 0')


bp = gdb.Breakpoint(addr('CRezbit::Think('))
bp.condition = '$busy == 0 && $done == 0 && ((CRezbit*)$rdi)->mAlive && ((CRezbit*)$rdi)->mActive'
bp.ignore_count = int(os.environ.get('REZ5_AT', '300'))
bp.commands = 'python hook()\ncontinue'
