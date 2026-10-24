# spl5 (Splinter, REL 74): the arms of IsNearWall / ScoreJump / FindJumpPoints / FindJump / JumpToPoint /
# FallFromCocoon / ApplyLaunchVelocity / ApplyAmbushVelocity the fights cannot reach, taken by gdb calls
# (the dtr9 way): at the SPL5_AT-th CSplinter::Think entry gdb writes the members each arm needs
# (a land-point connection list, a jump point and its waypoint moved next to the splinter, the player
# moved, the mega spit engaged, the ing-possession blend), calls the body, and writes every member back.
# IsNearWall's answer is forced for the calls made *inside* FindJumpPoints / JumpToPoint ($nwf: -1 none,
# 0 false, 1 true).  The results of every call go to <out>/spl5.log.
#   COV_GDB_SCRIPT=$PWD/tools/cov/specs/splinter_spl5_calls.py BUILD=$PWD/build-port-cov \
#     tools/cov/force.sh tools/cov/specs/splinter_spl5_calls.py <out> 3bfa3eff,02_temple_exploration \
#     --con tools/cov/specs/splinter_te_fight.con --fast --frames 1800
#   then tools/cov/gcov_report.py build-port-cov <out>/gcov --module Splinter
import os
import gdb

CLASS = 'CSplinter'
FORCE = []
OUT = os.environ['COV_OUT']
LOG = OUT + '/spl5.log'

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
    """One gdb command; its output is logged, an error too (the script goes on)."""
    try:
        out = gdb.execute(cmd, to_string=True)
        log('> %s\n%s' % (cmd, out.rstrip()))
        return out
    except gdb.error as e:
        log('> %s\n  ERROR %s' % (cmd, e))
        return ''


def ev(expr):
    return gdb.parse_and_eval(expr)


gdb.execute('set $busy = 0')
gdb.execute('set $done = 0')
gdb.execute('set $nwf = -1')


class NwRet(gdb.FinishBreakpoint):
    def stop(self):
        gdb.execute('set var $rax = $nwf')
        return False

    def out_of_scope(self):
        pass


class NwBp(gdb.Breakpoint):
    """IsNearWall called from FindJumpPoints / JumpToPoint answers $nwf when it is 0 or 1."""

    def stop(self):
        if int(gdb.convenience_variable('nwf')) >= 0:
            NwRet(gdb.newest_frame(), internal=True)
        return False


NwBp(addr('CSplinter::IsNearWall('), internal=True)


state = {'general': False, 'jump': False, 'tries': 0}


def tpos():
    return [float(ev('$t->mTransform.m%d3' % i)) for i in range(3)]


def put(var, rel, pos):
    """Write buf[var] = pos + rel (a CVector3f at $buf + 16 * var)."""
    for i in range(3):
        run('set var ((float*)$buf)[%d] = %r' % (4 * var + i, pos[i] + rel[i]))


def vec(var):
    return '*(CVector3f*)($buf + %d)' % (16 * var)


def setpos(obj, p):
    # GetTranslation() reads mPosition (CActor) or mTranslation (CPhysicsActor), not mTransform.
    for i in range(3):
        run('set var %s->mTransform.m%d3 = %r' % (obj, i, p[i]))
        for f in ('mPosition', 'mTranslation'):
            try:
                gdb.execute('set var %s->%s.%s = %r' % (obj, f, 'xyz'[i] if False else ('x', 'y', 'z')[i], p[i]), to_string=True)
            except gdb.error:
                try:
                    gdb.execute('set var ((float*)&%s->%s)[%d] = %r' % (obj, f, i, p[i]), to_string=True)
                except gdb.error:
                    pass


def general(tp):
    """Calls that need only the splinter."""
    # --- ScoreJump: every return --------------------------------------------------------------
    t = tp
    cases = [
        ('jump > 6', (10, 0, 0), (20, 0, 0), (30, 0, 0), 0),
        ('nw=0 waypoint within 6', (1, 0, 0), (2, 0, 0), (30, 0, 0), 0),
        ('nw=0 landing < player distance', (1, 0, 0), (20, 0, 0), (60, 0, 0), 0),
        ('nw=0 score', (1, 0, 0), (20, 0, 0), (30, 0, 0), 0),
        ('nw=1 waypoint within 6', (1, 0, 0), (2, 0, 0), (30, 0, 0), 1),
        ('nw=1 landing < player distance', (1, 0, 0), (20, 0, 0), (60, 0, 0), 1),
    ]
    for label, jp, wp, pl, nw in cases:
        put(0, jp, t)
        put(1, wp, t)
        put(2, pl, t)
        log('# ScoreJump %s' % label)
        run('print $t->ScoreJump(%s, %s, %s, %d, 0.3f)' % (vec(0), vec(1), vec(2), nw))
    # --- IsNearWall: the mega arm and the vertical-forward arm ------------------------------------
    log('# IsNearWall mega')
    sv = int(ev('$t->mSpit.m_valid'))
    run('set var $t->mSpit.m_valid = 1')
    run('print $t->IsNearWall(*$m)')
    run('set var $t->mSpit.m_valid = %d' % sv)
    log('# IsNearWall forward vertical')
    fx, fy = float(ev('$t->mTransform.m01')), float(ev('$t->mTransform.m11'))
    run('set var $t->mTransform.m01 = 0')
    run('set var $t->mTransform.m11 = 0')
    run('print $t->IsNearWall(*$m)')
    run('set var $t->mTransform.m01 = %r' % fx)
    run('set var $t->mTransform.m11 = %r' % fy)
    log('# IsNearWall plain')
    run('print $t->IsNearWall(*$m)')
    # --- state saved for the jump bodies ----------------------------------------------------------
    run('set $sj = $t->mJump')
    run('set $sa = $t->mAnimationState')
    run('set $sv = $t->mVelocity')
    run('set $sic = $t->mInCocoon')
    run('set $salive = $t->mAlive')
    run('set $sblend = $t->mIngPossessionBlend')
    run('set $stgt = $t->mIngPossessionTarget')
    run('set $sconn = $t->mConnections')
    # --- JumpToPoint ---------------------------------------------------------------------------
    for nw in (0, 1):
        log('# JumpToPoint Activate nearWall=%d' % nw)
        run('set $nwf = %d' % nw)
        run('call $t->JumpToPoint(*$m, kStateMsg_Activate, 0.016f)')
        run('set $nwf = -1')
        run('set var $t->mJump = $sj')
    for los in (0, 1):
        for st, name in ((1, 'Ready'), (0, 'NotReady')):
            log('# JumpToPoint Update los=%d state=%s' % (los, name))
            run('set var $t->mJump = $sj')
            run('set var $t->mJump.mHasLineOfSight = %d' % los)
            run('set var $t->mAnimationState.mState = %d' % st)
            run('call $t->JumpToPoint(*$m, kStateMsg_Update, 0.016f)')
            run('print $t->mAnimationState.mState')
    log('# JumpToPoint Update animation over')
    run('set var $t->mAnimationState.mState = 3')
    run('call $t->JumpToPoint(*$m, kStateMsg_Update, 0.016f)')
    log('# JumpToPoint Deactivate / default')
    run('call $t->JumpToPoint(*$m, kStateMsg_Deactivate, 0.016f)')
    run('call $t->JumpToPoint(*$m, (EStateMsg)99, 0.016f)')
    run('set var $t->mJump = $sj')
    run('set var $t->mAnimationState = $sa')
    # --- FallFromCocoon ------------------------------------------------------------------------
    log('# FallFromCocoon no land points (dead)')
    run('set var $t->mAlive = 0')
    run('call $t->FallFromCocoon(*$m, kStateMsg_Activate, 0.016f)')
    run('set var $t->mAlive = $salive')
    run('set var $t->mJump = $sj')
    # a three-entry list [land point, wrong state, wrong message] and a twelve-entry list
    def conns(items):
        run('set $cbuf = (SConnection*)(((void*(*)(unsigned long))malloc)(12 * 16))')
        for n, (s, mg) in enumerate(items):
            run('set var $cbuf[%d].state = %s' % (n, s))
            run('set var $cbuf[%d].msg = %s' % (n, mg))
            run('set var $cbuf[%d].objId = $t->mEditorId' % n)
        run('set var $t->mConnections.mItems = $cbuf')
        run('set var $t->mConnections.mCount = %d' % len(items))
        run('set var $t->mConnections.mCapacity = 16')

    one = [('kSS_Attack', 'kSM_Next')]
    mixed = [('kSS_Arrived', 'kSM_Next'), ('kSS_Attack', 'kSM_Activate'), ('kSS_Attack', 'kSM_Next')]
    twelve = [('kSS_Attack', 'kSM_Next')] * 12
    for label, items in (('one', one), ('mixed', mixed), ('twelve', twelve)):
        log('# FallFromCocoon Activate land points: %s' % label)
        conns(items)
        run('call $t->FallFromCocoon(*$m, kStateMsg_Activate, 0.016f)')
        run('set var $t->mVelocity = $sv')
        run('set var $t->mInCocoon = $sic')
        run('set var $t->mJump = $sj')
    log('# FallFromCocoon Update / default / Deactivate')
    conns(one)
    for st in (1, 0):
        run('set var $t->mAnimationState.mState = %d' % st)
        run('call $t->FallFromCocoon(*$m, kStateMsg_Update, 0.016f)')
        run('set var $t->mJump = $sj')
    run('set var $t->mAnimationState.mState = 3')
    run('call $t->FallFromCocoon(*$m, kStateMsg_Update, 0.016f)')
    run('set var $t->mJump = $sj')
    run('call $t->FallFromCocoon(*$m, (EStateMsg)99, 0.016f)')
    for sp in (0, 1):
        run('set var $t->mSpit.m_valid = %d' % sp)
        run('call $t->FallFromCocoon(*$m, kStateMsg_Deactivate, 0.016f)')
    run('set var $t->mSpit.m_valid = %d' % sv)
    run('set var $t->mConnections = $sconn')
    run('set var $t->mAnimationState = $sa')
    run('set var $t->mJump = $sj')
    # --- ApplyLaunchVelocity / ApplyAmbushVelocity --------------------------------------------------
    log('# ApplyLaunchVelocity')
    run('set var $t->mJump.mIsHighJump = 1')
    run('call $t->ApplyLaunchVelocity()')
    run('set var $t->mJump.mIsHighJump = 0')
    run('set var $t->mJump.mHeight = 2.0f')
    for n, d in enumerate(((5, 0, 3), (-4, 3, -2))):
        for i, c in enumerate('XYZ'):
            run('set var $t->mJump.mStart.m%s = %r' % (c.lower() if False else c, tp[i] + d[i]))
        run('call $t->ApplyLaunchVelocity()')
        run('print $t->mVelocity')
        run('set var $t->mJump.mIsHighJump = 0')
    run('set var $t->mVelocity = $sv')
    run('set var $t->mJump = $sj')
    log('# ApplyAmbushVelocity')
    run('set var $t->mJump.mIsHighJump = 1')
    run('call $t->ApplyAmbushVelocity(0)')
    for far, label in ((6.0, 'far start'), (0.1, 'start at the splinter')):
        for h in (0.0, 1.0):
            for morph in (0, 1):
                for mega in (0, 1):
                    for ing in (0, 1):
                        log('# Ambush %s height=%r morph=%d mega=%d ing=%d' % (label, h, morph, mega, ing))
                        run('set var $t->mJump.mIsHighJump = 0')
                        run('set var $t->mJump.mHeight = %r' % h)
                        for i, c in enumerate('XYZ'):
                            run('set var $t->mJump.mStart.m%s = %r' % (c, tp[i] + (far if i == 0 else 0.0)))
                        run('set var $t->mSpit.m_valid = %d' % mega)
                        run('set var $t->mIngPossessionBlend = %r' % float(ing))
                        run('set var $t->mIngPossessionTarget = 0.0f')
                        run('call $t->ApplyAmbushVelocity(%d)' % morph)
                        run('print $t->mVelocity')
    run('set var $t->mSpit.m_valid = %d' % sv)
    run('set var $t->mIngPossessionBlend = $sblend')
    run('set var $t->mIngPossessionTarget = $stgt')
    run('set var $t->mVelocity = $sv')
    run('set var $t->mJump = $sj')
    run('set var $t->mAnimationState = $sa')


def find_jump_point():
    """(J, W) with J a CScriptAiJumpPoint of the splinter's area whose Arrived/Next connection is a CScriptWaypoint."""
    run('set $ol = *(CObjectList**)($m->mObjectLists.mData + 16 * (int)kOL_AiWaypoint + 8)')
    ents = []
    i = int(ev('$ol->mFirstId'))
    while i != -1:
        ents.append(ev('$ol->mObjects[%d].mEntity' % i))
        i = int(ev('$ol->mObjects[%d].mNext' % i))
    area = int(ev('$t->mAreaId.value'))
    if state['tries'] == 1:
        log('# waypoint list: %d entities, splinter area %d' % (len(ents), area))
        for e in ents:
            try:
                log('#   %s area %d' % (e.dereference().dynamic_type, int(e.cast(gdb.lookup_type('CEntity').pointer())['mAreaId']['value'])))
            except gdb.error as x:
                log('#   ? %s' % x)
    for e in ents:
        try:
            tn = str(e.dereference().dynamic_type)
        except gdb.error:
            continue
        if 'CScriptAiJumpPoint' not in tn:
            continue
        j = e.cast(gdb.lookup_type('CScriptAiJumpPoint').pointer())
        if int(j['mAreaId']['value']) != area:
            continue
        run('set $j = (CScriptAiJumpPoint*)%d' % int(j))
        run('set $uid = $j->CheckConnectedObject(*$m, kSS_Arrived, kSM_Next)')
        wid = int(ev('$uid.value'))
        if wid == 0xffff:
            continue
        run('set $w = (CScriptWaypoint*)$m->ObjectById($uid)')
        if int(ev('$w')) != 0 and 'CScriptWaypoint' in str(ev('$w').dereference().dynamic_type):
            return True
    return False


def jump(tp):
    """FindJumpPoints / FindJump with a jump point and its waypoint moved around the splinter."""
    jp = [float(ev('$j->mTransform.m%d3' % i)) for i in range(3)]
    wp = [float(ev('$w->mTransform.m%d3' % i)) for i in range(3)]
    pp = [float(ev('$m->mPlayers[0]->mTransform.m%d3' % i)) for i in range(3)]
    run('set $sj = $t->mJump')
    run('set $sconn = $j->mConnections')
    run('set $sact = $j->mActive')
    run('set $suse = $j->mInUse')
    run('set $strem = $j->mTimeRemaining')
    run('print $j->mActive')
    run('print $j->mInUse')
    run('print $j->mTimeRemaining')
    run('set var $j->mInUse = 0')
    run('set var $j->mTimeRemaining = 0')
    run('set $suse = 0')
    run('set $sarea = $j->mAreaId.value')

    def place(t_rel, w_rel, p_rel):
        setpos('$t', [jp[i] + t_rel[i] for i in range(3)])
        setpos('$w', [jp[i] + w_rel[i] for i in range(3)])
        setpos('$m->mPlayers[0]', [jp[i] + p_rel[i] for i in range(3)])

    def scenario(label, nw, t_rel, w_rel, p_rel, extra=None, undo=None, fj=False):
        log('# FindJumpPoints %s' % label)
        place(t_rel, w_rel, p_rel)
        run('set $nwf = %d' % nw)
        if extra:
            for c in extra:
                c() if callable(c) else run(c)
        if label == 'nw=0 best':
            for c in ('print $j->mActive', 'print $j->mInUse', 'print $j->mTimeRemaining', 'print $j->mOccupant',
                      'print $j->mAreaId', 'print $t->mAreaId', 'print $j->GetInUse(kInvalidUniqueId)',
                      'print $j->mTransform.m03', 'print $j->CheckConnectedObject(*$m, kSS_Arrived, kSM_Next)',
                      'print $t->mTransform.m03', 'print $w->mTransform.m03',
                      'print $m->mPlayers[0]->mTransform.m03', 'print $w->mAreaId',
                      'print $t->ScoreJump($j->GetTranslation(), $w->GetTranslation(), $m->mPlayers[0]->GetTranslation(), 0, 0.3f)'):
                run(c)
        run('print/x $t->FindJumpPoints(*$m)')
        if fj:
            run('print $t->FindJump(*$m)')
            run('set var $t->mJump = $sj')
        if undo:
            for c in undo:
                run(c)
        run('set $nwf = -1')

    def selfconn():
        for n in range(int(ev('$j->mConnections.mCount'))):
            c = ev('$j->mConnections.mItems[%d]' % n)
            if str(c['state']) == 'kSS_Arrived' and str(c['msg']) == 'kSM_Next':
                run('set var $j->mConnections.mItems[%d].objId = $j->mEditorId' % n)

    T, W = (-1, 0, 0), (19, 0, 0)
    scenario('nw=0 best', 0, T, W, (35, 0, 0), fj=True)
    scenario('nw=1 best', 1, T, W, (35, 0, 0), fj=True)
    scenario('nw=0 score > player distance', 0, T, W, (9, 0, 0))
    scenario('nw=0 landing < player distance', 0, T, W, (-6, 0, 0))
    scenario('nw=1 not better than best (player on the splinter)', 1, T, W, T)
    pass  # nw=0 with the player on the splinter is always -1 (|jp-wp|=19 < 20)
    scenario('jump point > 6 away', 0, (-10, 0, 0), W, (35, 0, 0))
    scenario('inactive', 0, T, W, (35, 0, 0), ['set var $j->mActive = 0'], ['set var $j->mActive = $sact'])
    scenario('in use', 0, T, W, (35, 0, 0), ['set var $j->mInUse = 1'], ['set var $j->mInUse = $suse'])
    scenario('other area', 0, T, W, (35, 0, 0), ['set var $j->mAreaId.value = $sarea + 1'],
             ['set var $j->mAreaId.value = $sarea'])
    scenario('no connection', 0, T, W, (35, 0, 0), ['set var $j->mConnections.mCount = 0'],
             ['set var $j->mConnections = $sconn'])
    # the Arrived/Next connection points at the jump point itself: not a waypoint
    scenario('connection is not a waypoint', 0, T, W, (35, 0, 0),
             [selfconn],
             ['set var $j->mConnections = $sconn'])
    # restore
    setpos('$t', tp)
    setpos('$w', wp)
    setpos('$m->mPlayers[0]', pp)
    run('set var $j->mConnections = $sconn')
    run('set var $j->mTimeRemaining = $strem')
    run('set var $t->mJump = $sj')


class ThinkBp(gdb.Breakpoint):
    pass


def spl5_hook():
    gdb.execute('set $busy = 1')
    try:
        run('set $t = (CSplinter*)$rdi')
        run('set $m = (CStateManager*)$rsi')
        run('set $buf = ((void*(*)(unsigned long))malloc)(256)')
        tp = tpos()
        if not state['general']:
            state['general'] = True
            log('# general: splinter %s at %r' % (ev('$t'), tp))
            general(tp)
        if not state['jump']:
            state['tries'] += 1
            if find_jump_point():
                state['jump'] = True
                log('# jump: j=%s w=%s' % (ev('$j'), ev('$w')))
                jump(tp)
            elif state['tries'] > 400:
                state['jump'] = True
        if state['general'] and state['jump']:
            gdb.execute('set $done = 1')
            log('# done')
    except Exception as e:
        log('EXC %r' % (e,))
    finally:
        gdb.execute('set $busy = 0')


bp = gdb.Breakpoint(addr('CSplinter::Think('))
bp.condition = '$busy == 0 && $done == 0 && ((CSplinter*)$rdi)->mAlive'
bp.ignore_count = int(os.environ.get('SPL5_AT', '1500'))
bp.commands = 'python spl5_hook()\ncontinue'
