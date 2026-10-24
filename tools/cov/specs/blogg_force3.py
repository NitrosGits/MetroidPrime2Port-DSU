# Blogg (REL 7) rez6: direct-call and per-call-queue forcing of the unmarked bodies (the rez5 way, helpers in
# force_lib.py). gdb is the driver: at the BLG_AT-th live CBlogg::Think the `misc` group calls the leaf bodies with
# the members each arm needs (written, called, restored), flushes the gcov counters, and arms the queues that the
# console commands of the driver then drive (statecall / uevent / send / trigcall).
#   COV_GDB_SCRIPT=$PWD/tools/cov/specs/blogg_force3.py BUILD=$PWD/build-port-cov SECS=600 MP2_CONSOLE_WAIT=120 \
#     tools/cov/force.sh tools/cov/specs/blogg_force3.py <out> 3dfd2249,07_swamp_shaft \
#     --con tools/drivers/blogg_forced3.con --fast --frames 1900          (env BLG_GROUPS=misc,queues)
import os
import gdb

CLASS = 'CBlogg'
FORCE = []
LOG = os.environ['COV_OUT'] + '/blogg6.log'
GATE_SYMS = {
    'team': 'CScriptTeamAiMgr::CanStartAttack(',
    'ipf': 'CSfxManager::IsPlaying(',
    'cam': 'CCameraManager::IsInCinematicCamera(',
    'onpath': 'CPathFindSearch::OnPath(',
    'start': 'CScriptTeamAiMgr::StartAttack(',
}
AFTER_SYMS = {'base_asm': 'CPatterned::AcceptScriptMsg('}
exec(compile(open(os.path.join(os.path.dirname(os.environ['COV_SPEC']), 'force_lib.py')).read(), 'force_lib.py', 'exec'))

T = '$t->'
PLID = '$m->mPlayers[0]->mUniqueId.value'
STATE = {}


def snap(lvs):
    out = {}
    for lv in lvs:
        v = ev(lv)
        out[lv] = float(v) if v.type.code == gdb.TYPE_CODE_FLT else int(v)
    return out


def put(d):
    for lv, v in d.items():
        w(lv, repr(v))


def hints():
    """[(ptr, id)] of the CScriptAIHint objects of the waypoint list."""
    run('set $wl = *(CObjectList**)($m->mObjectLists.mData + 16 * (int)kOL_AiWaypoint + 8)')
    out = []
    i = int(ev('$wl->mFirstId'))
    while i != -1:
        e = ev('$wl->mObjects[%d].mEntity' % i)
        try:
            tn = str(e.dereference().dynamic_type)
        except gdb.error:
            tn = ''
        if 'CScriptAIHint' in tn:
            out.append((int(e), int(ev('((CEntity*)%d)->mUniqueId.value' % int(e)))))
        i = int(ev('$wl->mObjects[%d].mNext' % i))
    return out


def vec(slot, x, y, z):
    for k, v in enumerate((x, y, z)):
        w('((float*)($buf + %d))[%d]' % (16 * slot, k), v)
    return '*(CVector3f*)($buf + %d)' % (16 * slot)


def misc_group():
    run('set $buf = (unsigned char*)malloc(512)')
    run('set $trg = (CTriggerData*)malloc(16)')
    w('*(int*)$trg', 0)
    TRG = '*$trg'
    # --- GetNextPositionIndex ------------------------------------------------------------------------
    base = snap([T + 'mPositionHistory.mCount', T + 'xb14_', T + 'xb18_'])
    for cnt, b14, b18 in ((0, 1, 0), (3, 1, 0), (3, 1, 1), (3, 0, 0), (3, 0, 2), (3, 0, 5), (3, 2, 0), (3, -1, 0)):
        log('# GetNextPositionIndex count=%d xb14=%d xb18=%d' % (cnt, b14, b18))
        put({T + 'mPositionHistory.mCount': cnt, T + 'xb14_': b14, T + 'xb18_': b18})
        run('print (int)$t->GetNextPositionIndex()')
    put(base)
    flush()
    # --- CanRangedAttack / CanMeleeAttack -----------------------------------------------------------------
    names = ['mMeleePursuitEnded', 'mChargeOver', 'mIsMegaBlogg', 'xbc5_26_', 'xbc5_27_', 'xbc5_30_', 'xbc4_25_']
    lv = [T + n for n in names] + [T + 'x951_', T + 'mUnknown_0xa19d5f62', T + 'mTeamManagerId.value']
    base = snap(lv)
    mgrid = base[T + 'mTeamManagerId.value']

    def flags(**kw):
        d = dict((T + n, 0) for n in names)
        d[T + 'x951_'] = 0
        d[T + 'mUnknown_0xa19d5f62'] = 5
        d[T + 'mTeamManagerId.value'] = 0xffff
        for k, v in kw.items():
            d[T + k] = v
        return d
    cases = [('plain', {}, None), ('pursuit ended', dict(mMeleePursuitEnded=1), None), ('charge over', dict(mChargeOver=1), None),
             ('mega, no flags', dict(mIsMegaBlogg=1), None), ('mega, 26', dict(mIsMegaBlogg=1, xbc5_26_=1), None),
             ('mega, 27', dict(mIsMegaBlogg=1, xbc5_27_=1), None), ('mega, 30', dict(mIsMegaBlogg=1, xbc5_30_=1), None),
             ('ranged: count at limit', dict(x951_=5), None), ('ranged: 25 set', dict(xbc4_25_=1), None),
             ('team: both allow', dict(), (1, [])), ('team: melee refuses', dict(), (0, [])),
             ('team: projectile refuses', dict(), (None, [1, 0])),
             ('team refuses, 26', dict(xbc5_26_=1), (0, [])), ('team refuses, 30', dict(xbc5_30_=1), (0, [])),
             ('team refuses, none', dict(), (0, []))]
    for lab, kw, gt in cases:
        for fn in ('CanMeleeAttack', 'CanRangedAttack'):
            d = flags(**kw)
            if gt is not None:
                d[T + 'mTeamManagerId.value'] = '$t->mUniqueId.value'
            put({k: v for k, v in d.items() if not isinstance(v, str)})
            if gt is not None:
                w(T + 'mTeamManagerId.value', '$t->mUniqueId.value')
                gates_on({'team': gt})
            log('# %s %s' % (fn, lab))
            run('print $t->%s(*$m, %s)' % (fn, TRG))
            gates_off(['team'])
    put(base)
    flush()
    # --- FindNearestHint ----------------------------------------------------------------------------------
    hs = hints()
    log('# hints in the area: %d' % len(hs))
    run('set $ids = (unsigned short*)malloc(64)')
    old = (int(ev(T + 'mHintIds.mCount')), int(ev(T + 'mHintIds.mItems')))
    px, py, pz = [float(ev('$t->mPosition.m' + c)) for c in 'XYZ']
    sets = [[], [0xffff], [h[1] for h in hs[:1]], [h[1] for h in hs[:3]], [0xffff] + [h[1] for h in hs[:2]]]
    for sidx, ids in enumerate(sets):
        for k, v in enumerate(ids):
            w('$ids[%d]' % k, v)
        w(T + 'mHintIds.mItems', '(unsigned short*)$ids')
        w(T + 'mHintIds.mCount', len(ids))
        for los in (0, 1):
            for off in ((0, 0, 0), (5, 0, 0), (-30, 40, 5), (300, 300, 300)):
                pos = vec(0, px + off[0], py + off[1], pz + off[2])
                log('# FindNearestHint set %d los %d off %r' % (sidx, los, off))
                run('print $t->FindNearestHint(*$m, %s, %d)' % (pos, los))
        if ids and hs:
            # an inactive hint, an in-use hint
            h0 = hs[0][0]
            for lab, lv_, val in (('inactive', 'mActive', 0), ('in use', 'mInUse', 1)):
                o = int(ev('((CScriptAIHint*)%d)->%s' % (h0, lv_)))
                w('((CScriptAIHint*)%d)->%s' % (h0, lv_), val)
                log('# FindNearestHint set %d first hint %s' % (sidx, lab))
                run('print $t->FindNearestHint(*$m, %s, 0)' % vec(0, px, py, pz))
                w('((CScriptAIHint*)%d)->%s' % (h0, lv_), o)
    w(T + 'mHintIds.mItems', old[1])
    w(T + 'mHintIds.mCount', old[0])
    flush()
    STATE['hints'] = hs


def actors(tname, lst='kOL_Actor'):
    """[(ptr, id)] of the entities of an object list whose dynamic type name contains tname."""
    run('set $acl = *(CObjectList**)($m->mObjectLists.mData + 16 * (int)%s + 8)' % lst)
    out = []
    i = int(ev('$acl->mFirstId'))
    while i != -1:
        e = ev('$acl->mObjects[%d].mEntity' % i)
        try:
            tn = str(e.dereference().dynamic_type)
        except gdb.error:
            tn = ''
        if tname in tn:
            out.append((int(e), int(ev('((CEntity*)%d)->mUniqueId.value' % int(e)))))
        i = int(ev('$acl->mObjects[%d].mNext' % i))
    return out


def attack_group():
    run('set $buf = (unsigned char*)malloc(512)')
    px, py, pz = [float(ev('$m->mPlayers[0]->mPosition.m' + c)) for c in 'XYZ']
    bx, by, bz = [float(ev('$t->mPosition.m' + c)) for c in 'XYZ']
    mp = T + 'mPositionHistory.mCount'
    waters = actors('CScriptWater')
    log('# waters: %r' % (waters,))
    log('# teams: %r, this team id %#x' % (actors('CScriptTeamAiMgr', 'kOL_All'), int(ev(T + 'mTeamManagerId.value'))))
    fl0 = int(ev(T + 'mFluidIds.mCount'))
    fl_id0 = int(ev('((unsigned short*)$t->mFluidIds.mData)[0]'))
    team0 = int(ev(T + 'mTeamManagerId.value'))
    # --- FindAttackPositions: path answers, fluid, roles -----------------------------------------------------
    for fluid in ([0] + ([1] if waters else [])):
        if fluid:
            w(T + 'mFluidIds.mCount', 1)
            w('((unsigned short*)$t->mFluidIds.mData)[0]', waters[0][1])
        else:
            w(T + 'mFluidIds.mCount', 0)
        for onpath in (None, 0, 1):
            for dist in (4.0, 20.0):
                for dx, dy in ((1.0, 0.0), (0.0, 1.0)):
                    w(mp, 0)
                    if onpath is not None:
                        gates_on({'onpath': (onpath, [])})
                    d = vec(1, dx, dy, 0.0)
                    log('# FindAttackPositions fluid=%d onpath=%s dist=%s' % (fluid, onpath, dist))
                    run('call $t->FindAttackPositions(*$m, %s, %s, %s, %rf)' % (vec(0, px, py, pz), vec(2, bx, by, bz), d, dist))
                    gates_off(['onpath'])
    if waters:
        wp = waters[0][0]
        try:
            lo = [float(ev('((CScriptWater*)%d)->GetTriggerBoundsWR().min.m%s' % (wp, c))) for c in 'XYZ']
            hi = [float(ev('((CScriptWater*)%d)->GetTriggerBoundsWR().max.m%s' % (wp, c))) for c in 'XYZ']
            ctr = [(a + b) / 2 for a, b in zip(lo, hi)]
            log('# water bounds %r %r' % (lo, hi))
            w(T + 'mFluidIds.mCount', 1)
            w('((unsigned short*)$t->mFluidIds.mData)[0]', waters[0][1])
            for onpath in (0,):
                for dist in (2.0, 6.0):
                    w(mp, 0)
                    gates_on({'onpath': (onpath, [])})
                    log('# FindAttackPositions inside the water dist=%s' % dist)
                    run('call $t->FindAttackPositions(*$m, %s, %s, %s, %rf)' % (vec(0, ctr[0], ctr[1], ctr[2]), vec(2, bx, by, bz), vec(1, 1.0, 0.0, 0.0), dist))
                    run('print $t->mPositionHistory.mCount')
                    gates_off(['onpath'])
        except gdb.error as e:
            log('water bounds: %s' % e)
    w(T + 'mFluidIds.mCount', fl0)
    flush()
    # --- FindAttackPositions with a team manager and its roles -----------------------------------------------
    teams = actors('CScriptTeamAiMgr', 'kOL_All')
    bloggs = actors('CBlogg', 'kOL_All')
    if teams and len(bloggs) > 1:
        tptr, tid = teams[0]
        for bptr, bid in bloggs[:4]:
            run('call ((CScriptTeamAiMgr*)%d)->JoinTeam(*(CAi*)((CBlogg*)%d), (CTeamAiRole::ETeamAiRole)1, (CTeamAiRole::ETeamAiRole)2, (CTeamAiRole::ETeamAiRole)-1)' % (tptr, bptr))
        w(T + 'mTeamManagerId.value', tid)
        w(T + 'mFluidIds.mCount', 0)
        run('print ((CScriptTeamAiMgr*)%d)->mRoles.mCount' % tptr)
        run('print $m->GetObjectById($t->mTeamManagerId) == (CEntity*)%d' % tptr)
        others = [b for b in bloggs[:4] if b[0] != int(ev('(long)$t'))]
        cand = (px + 4.0, py, pz)          # the first candidate of a 4.0 attack distance along +x
        o0 = [float(ev('((CBlogg*)%d)->mAttackPosition.m%s' % (others[0][0], c))) for c in 'XYZ']
        for lab, pos in (('other blogg has no position', (0.0, 0.0, 0.0)), ('other blogg on the candidate', cand),
                         ('other blogg far away', (px + 500.0, py, pz))):
            for c, v in zip('XYZ', pos):
                w('((CBlogg*)%d)->mAttackPosition.m%s' % (others[0][0], c), v)
            for onpath, dist in ((0, 4.0), (0, 20.0)):
                w(mp, 0)
                gates_on({'onpath': (onpath, [])})
                log('# FindAttackPositions team: %s dist=%s' % (lab, dist))
                run('call $t->FindAttackPositions(*$m, %s, %s, %s, %rf)' % (vec(0, px, py, pz), vec(2, bx, by, bz), vec(1, 1.0, 0.0, 0.0), dist))
                run('print $t->mPositionHistory.mCount')
                gates_off(['onpath'])
        # a role whose owner is not a Blogg
        o_id = int(ev('((CScriptTeamAiMgr*)%d)->mRoles.mItems[0].mOwnerId.value' % tptr))
        w('((CScriptTeamAiMgr*)%d)->mRoles.mItems[0].mOwnerId.value' % tptr, 0xffff)
        w(mp, 0)
        gates_on({'onpath': (0, [])})
        log('# FindAttackPositions team: role owner invalid')
        run('call $t->FindAttackPositions(*$m, %s, %s, %s, 4.0f)' % (vec(0, px, py, pz), vec(2, bx, by, bz), vec(1, 1.0, 0.0, 0.0)))
        gates_off(['onpath'])
        w('((CScriptTeamAiMgr*)%d)->mRoles.mItems[0].mOwnerId.value' % tptr, o_id)
        for c, v in zip('XYZ', o0):
            w('((CBlogg*)%d)->mAttackPosition.m%s' % (others[0][0], c), v)
        w(T + 'mTeamManagerId.value', team0)
        w(T + 'mFluidIds.mCount', fl0)
        flush()
    # --- ComputeAttackPositions ----------------------------------------------------------------------------
    w(T + 'mFluidIds.mCount', 0)
    hs = hints()
    run('set $ids = (unsigned short*)malloc(64)')
    old = (int(ev(T + 'mHintIds.mCount')), int(ev(T + 'mHintIds.mItems')))
    pid = int(ev(T + 'mPlayerId.value'))
    saved = snap([T + 'mAttackPosition.mX', T + 'mAttackPosition.mY', T + 'mAttackPosition.mZ', T + 'mHintId.value', T + 'mCurrentAttackAngle',
                  T + 'mCurrentAttackRange'])
    for lab, ids in (('empty list', []), ('real hints', [h[1] for h in hs[:3]]), ('stale hints', [0xffff, 0xfffe])):
        for k, v in enumerate(ids):
            w('$ids[%d]' % k, v)
        w(T + 'mHintIds.mItems', '(unsigned short*)$ids')
        w(T + 'mHintIds.mCount', len(ids))
        for onpath in (None, 0, 1):
            for plid in (int(ev(PLID)), 0xffff):
                w(T + 'mPlayerId.value', plid)
                if onpath is not None:
                    gates_on({'onpath': (onpath, [])})
                log('# ComputeAttackPositions %s onpath=%s player=%#x' % (lab, onpath, plid))
                run('call $t->ComputeAttackPositions(*$m, 0.016f)')
                gates_off(['onpath'])
    # the three direction loops one after the other: count the OnPath calls of an all-failing run (fixed range),
    # then let the first loop, the first two loops fail and the next succeed
    w(T + 'mFluidIds.mCount', 0)
    w(T + 'mHintIds.mCount', 0)
    w(T + 'mPlayerId.value', int(ev(PLID)))
    mr0 = snap([T + 'mMinAttackRange', T + 'mMaxAttackRange'])
    w(T + 'mMinAttackRange', 12.0)
    w(T + 'mMaxAttackRange', 12.0)
    gates_on({'onpath': (1, [])})
    GATES['onpath'].calls = 0
    log('# ComputeAttackPositions all loops fail (counting OnPath calls)')
    run('call $t->ComputeAttackPositions(*$m, 0.016f)')
    total = GATES['onpath'].calls
    gates_off(['onpath'])
    log('# OnPath calls in the three loops: %d' % total)
    for lab, n in (('first loop fails, second succeeds', total // 3), ('two loops fail, mirrored succeeds', 2 * (total // 3))):
        gates_on({'onpath': (0, [1] * n)})
        log('# ComputeAttackPositions %s' % lab)
        run('call $t->ComputeAttackPositions(*$m, 0.016f)')
        gates_off(['onpath'])
    put(mr0)
    # the blogg on the player's aim position: zero direction
    ax, ay, az = [float(ev('$m->mPlayers[0]->GetAimPosition(*$m, 0.0f).m' + c)) for c in 'XYZ']
    w(T + 'mHintIds.mCount', 0)
    w(T + 'mPlayerId.value', pid)
    for c, v in zip('XYZ', (ax, ay, az)):
        w('$t->mPosition.m' + c, v)
    log('# ComputeAttackPositions blogg on the aim position')
    run('call $t->ComputeAttackPositions(*$m, 0.016f)')
    for c, v in zip('XYZ', (bx, by, bz)):
        w('$t->mPosition.m' + c, v)
    w(T + 'mHintIds.mItems', old[1])
    w(T + 'mHintIds.mCount', old[0])
    w(T + 'mPlayerId.value', pid)
    put(saved)
    w(T + 'mFluidIds.mCount', fl0)
    flush()


def state_group():
    """leaf getters, GrabBall (all arms), Dead (last: it changes the Blogg for good)."""
    run('set $buf = (unsigned char*)malloc(512)')
    run('print (long)$t->GetSearchPath() - (long)$t')
    run('print (long)$t->ProjectileInfo() - (long)$t')
    run('print $t->GetContactDamage()')
    teams = actors('CScriptTeamAiMgr', 'kOL_All')
    tid = teams[0][1] if teams else 0xffff
    pid = int(ev(PLID))
    lv = [T + n for n in ('mBallGrabTime', 'mBallGrabbed', 'mAbortBallGrab', 'mCanBite', 'x951_', 'mMeleeDelayTimer',
                          'mTeamManagerId.value', 'mPlayerId.value', 'mState')]
    base = snap(lv)
    P = '$m->mPlayers[0]->'
    ppos = [float(ev(P + 'mPosition.m' + c)) for c in 'XYZ']

    def restore():
        put(base)
        run('call $m->mPlayers[0]->SetTranslation(%s)' % vec(7, *ppos))
    for team in (0xffff, tid):
        for start in ((None,) if team == 0xffff else (1, 0)):
            for plid in (pid, 0xffff):
                w(T + 'mTeamManagerId.value', team)
                w(T + 'mPlayerId.value', plid)
                if start is not None:
                    gates_on({'start': (start, [])})
                log('# GrabBall Activate team=%#x start=%s player=%#x' % (team, start, plid))
                run('call $t->GrabBall(*$m, (EStateMsg)0, 0.016f)')
                gates_off(['start'])
                restore()
    w(T + 'mTeamManagerId.value', tid)
    for plid in (pid, 0xffff):
        for dt, g0 in ((0.016, 0.0), (1.0, 0.0), (0.016, 0.2)):
            w(T + 'mPlayerId.value', plid)
            w(T + 'mBallGrabTime', g0)
            log('# GrabBall Update player=%#x dt=%s grab time=%s' % (plid, dt, g0))
            run('call $t->GrabBall(*$m, (EStateMsg)1, %rf)' % dt)
            restore()
        w(T + 'mPlayerId.value', plid)
        log('# GrabBall Deactivate player=%#x' % plid)
        run('call $t->GrabBall(*$m, (EStateMsg)2, 0.016f)')
        restore()
    flush()
    # Dead: Update with fluid / fade combinations, then the effective one, Activate and Deactivate
    fl0 = int(ev(T + 'mFluidIds.mCount'))
    run('call $t->Dead(*$m, (EStateMsg)2, 0.016f)')
    for fl, fade in ((1, 0), (0, 1), (1, 1)):
        w(T + 'mFluidIds.mCount', fl)
        w(T + 'mFadeToDeath', fade)
        log('# Dead Update fluid=%d fade=%d' % (fl, fade))
        run('call $t->Dead(*$m, (EStateMsg)1, 0.016f)')
    w(T + 'mFluidIds.mCount', 0)
    w(T + 'mFadeToDeath', 0)
    run('call $t->Dead(*$m, (EStateMsg)0, 0.016f)')
    log('# Dead Update effective')
    run('call $t->Dead(*$m, (EStateMsg)1, 0.016f)')
    run('print $t->mAlphaDelta')
    w(T + 'mFluidIds.mCount', fl0)
    flush()


STATE_MSG = {'A': 0, 'U': 1, 'D': 2}


def hook():
    gdb.execute('set $busy = 1')
    try:
        run('set $t = (CBlogg*)$rdi')
        run('set $m = (CStateManager*)$rsi')
        log('# blogg %s' % ev('$t'))
        which = os.environ.get('BLG_GROUPS', 'misc').split(',')
        if 'misc' in which:
            misc_group()
        if 'attack' in which:
            attack_group()
        if 'state' in which:
            state_group()
        gdb.execute('set $tvalid = 1')
        log('# done')
    except Exception as e:
        log('EXC %r' % (e,))
    finally:
        flush()
        gdb.execute('set $busy = 0')
        gdb.execute('set $done = 1')


gdb.execute('set $done = 0')
bp = gdb.Breakpoint(addr('CBlogg::Think('))
bp.condition = '$busy == 0 && $done == 0 && ((CBlogg*)$rdi)->mAlive && ((CBlogg*)$rdi)->mActive'
bp.ignore_count = int(os.environ.get('BLG_AT', '400'))
bp.commands = 'python hook()\ncontinue'
