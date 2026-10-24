# SwarmBasics (module 80) swb12: fn_80_1978 CSwarmBasics::AssignSeekerBoids, the taken-scan arms
# (1685/1686/1691) plus the geometry arms.  force.py, two fixtures, every write restored:
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swarmbasics_seeker.py <out> \
#     3dfd2249,0g_swamp_hall --driver "python3 tools/drivers/con.py {port} tools/drivers/swb12.con" \
#     --fast --frames 1200
#
# 1. `gun_fixture` at the entry of the real caller, CSwarmBasics::UpdateSeekerTargets: the swarm's own
#    five kMT_SeekerTarget actors (created in kSM_Create, uids in mSeekerTargets, mapped to boid
#    indices in mSeekerBoidIndices by the previous frame) give ONE real uid, and that uid is supplied
#    as the single element of CPlayerGun::mSeekerTargets (reserved_vector<pair<TUniqueId,float>,5>,
#    member near CPlayerGun.hpp:340) so the caller's own keep/lose loop puts the mapped boid index
#    into keptBoids and passes a non-empty `taken` to AssignSeekerBoids.  Written only when the gun's
#    vector is empty (never clobbers a real lock), element first and then mCount, restored at
#    UpdateSeekerTargets' return from the cached addresses: mCount back to its original, and element
#    0's exact bits (.first.value and .second) back to what they were.  The `taken`/`out` vectors are
#    never touched: the caller builds them.
# (the object-list lookup is CObjectList::GetObjectById's own two tests: mObjects[uid.Value()]
#  and the uid equality - no inline function is called from gdb.)
# 2. `assign_fixture` at AssignSeekerBoids' entry, one phase per call: ph0 a boid beyond the orbit
#    distance, ph1 a boid exactly at the camera, ph2 no move, ph3 the taken boid (and a second boid)
#    placed in front of the camera, so the candidate list holds one candidate whose index IS in
#    `taken` and one whose index is not - the equality and inequality sides of the scan.  Boid
#    translation and mActive are restored at AssignSeekerBoids' return.
# Every restore breakpoint is armed BEFORE the first write, and a fixture aborts without writing if
# arming fails (the swb11 version wrote first: corrected here).  phase 2 is deliberate: it is what
# leaves the run a natural swarm, not an observation that the gun locks by itself (swb11 measured
# that it does not).
CLASS = 'CSwarmBasics'

import os

import gdb  # this spec is exec'd by tools/cov/force.py inside gdb's python

_OUT = os.environ.get('COV_OUT', '/tmp')
_LOG = os.path.join(_OUT, 'swb12_geom.log')

_VEC_U = 'rstl::vector<unsigned int, rstl::rmemory_allocator>'
# the cast value is the auto_ptr array; index it with _OBJ_LIST0 % mgr wrapped in ((...))[0]
_OBJ_LIST0 = '(rstl::auto_ptr<CObjectList>*)&((CStateManager*)%d)->mObjectLists.mData[0]'
_GUN_EL = '(rstl::pair<TUniqueId, float>*)&((CPlayerGun*)%d)->mSeekerTargets.mData[0]'

_st = {
    'gunCalls': 0, 'gunWrites': 0, 'gunSkipCount': 0, 'gunSkipPick': 0, 'gunSkipNoGun': 0,
    'assignCalls': 0, 'apply': 0, 'err': 0, 'takenIdx': -1, 'takenUid': -1, 'takenSlot': -1,
}


def _log(msg):
    try:
        with open(_LOG, 'a') as fh:
            fh.write(msg + '\n')
    except Exception:
        pass


def _f(g, expr):
    return float(g.parse_and_eval(expr))


def _i(g, expr):
    return int(g.parse_and_eval(expr))


def _cam(g, mgr):
    """(eye, forward, right) of the first-person camera, the way the body reads it."""
    cam = _i(g, '(long)((CStateManager*)%d)->mCameraManagers[0]' % mgr)
    if not cam:
        return None
    fp = _i(g, '(long)((CCameraManager*)%d)->mFpCamera' % cam)
    if not fp:
        return None
    xf = '((CActor*)%d)->mTransform' % fp
    return ((_f(g, xf + '.m03'), _f(g, xf + '.m13'), _f(g, xf + '.m23')),
            (_f(g, xf + '.m01'), _f(g, xf + '.m11'), _f(g, xf + '.m21')),
            (_f(g, xf + '.m00'), _f(g, xf + '.m10'), _f(g, xf + '.m20')))


def _actor_of(g, mgr, uid):
    """CEntity* for uid out of the manager's kOL_All list, or 0 (CObjectList's own test, inline)."""
    try:
        list0 = _i(g, '((%s))[0].mItem' % (_OBJ_LIST0 % mgr))
        if not list0:
            return 0
        ent = _i(g, '((CObjectList*)%d)->mObjects[%d].mEntity' % (list0, uid & 0x3FF))
        if not ent:
            return 0
        if _i(g, '((CEntity*)%d)->mUniqueId.value' % ent) != uid:
            return 0
        return ent
    except Exception:
        return 0


class _RestoreBoids(gdb.FinishBreakpoint):
    def __init__(self, frame, items):
        super().__init__(frame, internal=True)
        self.items = items

    def stop(self):
        for it in self.items:
            try:
                gdb.execute('set var %s.mTransform.m03 = %r' % (it['b'], it['pos'][0]))
                gdb.execute('set var %s.mTransform.m13 = %r' % (it['b'], it['pos'][1]))
                gdb.execute('set var %s.mTransform.m23 = %r' % (it['b'], it['pos'][2]))
                gdb.execute('set var %s.mActive = %d' % (it['b'], it['active']))
                _log('RESTORE-BOID %s boid=0x%x idx=%d pos=(%.5f,%.5f,%.5f) active=%d'
                     % (it['tag'], it['addr'], it['idx'], it['pos'][0], it['pos'][1],
                        it['pos'][2], it['active']))
            except Exception as e:
                _log('RESTORE-BOID %s FAILED: %s' % (it['tag'], e))
        return False


class _RestoreGun(gdb.FinishBreakpoint):
    def __init__(self, frame, info):
        super().__init__(frame, internal=True)
        self.info = info

    def stop(self):
        info = self.info
        try:
            gun, elem = info['gun'], info['elem']
            gdb.execute('set var ((CPlayerGun*)%d)->mSeekerTargets.mCount = %d'
                        % (gun, info['count']))
            gdb.execute('set var (%s)[0].first.value = %d' % (elem, info['uid0']))
            gdb.execute('set var (%s)[0].second = %r' % (elem, info['lock0']))
            _log('RESTORE-GUN %s gun=0x%x mCount back to %d, element 0 back to uid=0x%x second=%r'
                 % (info['tag'], gun, info['count'], info['uid0'], info['lock0']))
        except Exception as e:
            _log('RESTORE-GUN %s FAILED: %s' % (info['tag'], e))
        return False


def gun_fixture(g):
    """Supply ONE real swarm seeker-target uid in the player's gun for this call."""
    st = _st
    st['gunCalls'] += 1
    try:
        this = _i(g, '$rdi')
        mgr = _i(g, '$rsi')
        ntargets = _i(g, '((CSwarmBasics*)%d)->mSeekerTargets.mCount' % this)
        nboids = _i(g, '((CSwarmBasics*)%d)->mBoids.mCount' % this)
    except Exception as e:
        if st['err'] < 4:
            _log('gun entry read failed: %s' % e)
        st['err'] += 1
        return
    if ntargets <= 0 or nboids <= 0:
        if st['gunSkipPick'] < 3:
            _log('no targets/boids yet (targets=%d boids=%d)' % (ntargets, nboids))
        st['gunSkipPick'] += 1
        return
    try:
        player = _i(g, '(long)((CStateManager*)%d)->mPlayers[0]' % mgr)
        gun = _i(g, '(long)((CPlayer*)%d)->mGun.mPtr' % player) if player else 0
    except Exception as e:
        if st['err'] < 4:
            _log('gun chain failed: %s' % e)
        st['err'] += 1
        return
    if not player or not gun:
        if st['gunSkipNoGun'] < 3:
            _log('no player/gun yet (player=0x%x gun=0x%x)' % (player, gun))
        st['gunSkipNoGun'] += 1
        return
    try:
        gcount = _i(g, '((CPlayerGun*)%d)->mSeekerTargets.mCount' % gun)
    except Exception as e:
        if st['err'] < 4:
            _log('gun vector read failed: %s' % e)
        st['err'] += 1
        return
    if gcount != 0:
        if st['gunSkipCount'] < 3:
            _log('gun already has %d seeker target(s) - left alone' % gcount)
        st['gunSkipCount'] += 1
        return
    pick = None
    for k in range(ntargets if ntargets < 5 else 5):
        try:
            uid = _i(g, '((CSwarmBasics*)%d)->mSeekerTargets.mItems[%d].value' % (this, k))
            idx = _i(g, '((CSwarmBasics*)%d)->mSeekerBoidIndices.mItems[%d]' % (this, k))
        except Exception:
            continue
        ent = _actor_of(g, mgr, uid) if uid != 0xFFFF else 0
        if st['gunCalls'] <= 2:
            _log('slot %d uid=0x%x idx=%d actor=0x%x (boids=%d)' % (k, uid, idx, ent, nboids))
        if uid == 0xFFFF or idx < 0 or idx >= nboids or not ent:
            continue
        pick = (k, uid, idx, ent)
        break
    if pick is None:
        if st['gunSkipPick'] < 5:
            _log('no slot with a real actor and a valid boid index (targets=%d boids=%d)'
                 % (ntargets, nboids))
        st['gunSkipPick'] += 1
        return
    slot, uid, idx, ent = pick
    elem = _GUN_EL % gun
    try:
        uid0 = _i(g, '(%s)[0].first.value' % elem)
        lock0 = _f(g, '(%s)[0].second' % elem)
    except Exception as e:
        _log('gun element snapshot failed: %s' % e)
        return
    tag = 'swb12-gun-%d' % (st['gunWrites'] + 1)
    try:
        _RestoreGun(g.newest_frame(), {'gun': gun, 'elem': elem, 'count': gcount,
                                       'uid0': uid0, 'lock0': lock0, 'tag': tag})
    except Exception as e:
        _log('%s: restore could not be armed (%s) - no writes' % (tag, e))
        return
    try:
        setv = lambda e, v: g.execute('set var %s = %s' % (e, v))
        setv('(%s)[0].first.value' % elem, str(uid))
        setv('(%s)[0].second' % elem, '0.5')
        setv('((CPlayerGun*)%d)->mSeekerTargets.mCount' % gun, '1')
    except Exception as e:
        _log('%s write failed: %s' % (tag, e))
        return
    st['gunWrites'] += 1
    st['takenIdx'], st['takenUid'], st['takenSlot'] = idx, uid, slot
    _log('%s gun=0x%x player=0x%x swarm=0x%x slot=%d uid=0x%x actor=0x%x boid_idx=%d of %d'
         ' mCount %d->1 (slot bits were uid=0x%x second=%r)'
         % (tag, gun, player, this, slot, uid, ent, idx, nboids, gcount, uid0, lock0))


def assign_fixture(g):
    """One boid geometry per call, restored at the call's return."""
    st = _st
    st['assignCalls'] += 1
    try:
        this = _i(g, '$rdi')
        mgr = _i(g, '$rsi')
        taken = _i(g, '$rdx')
        needed = _i(g, '$rcx')
        out = _i(g, '$r8')
        count = _i(g, '((CSwarmBasics*)%d)->mBoids.mCount' % this)
        items = _i(g, '((CSwarmBasics*)%d)->mBoids.mItems' % this)
    except Exception as e:
        if st['err'] < 4:
            _log('assign entry read failed: %s' % e)
        st['err'] += 1
        return
    tcount = ocount = -1
    try:
        if taken:
            tcount = _i(g, '((%s*)%d)->mCount' % (_VEC_U, taken))
    except Exception:
        tcount = -2
    try:
        if out:
            ocount = _i(g, '((%s*)%d)->mCount' % (_VEC_U, out))
    except Exception:
        ocount = -2
    if count <= 0 or items <= 0 or not taken:
        return
    phase = st['assignCalls'] % 4
    cam = _cam(g, mgr) if phase in (0, 1, 3) else None
    moves = []
    if phase == 0 and cam:
        moves.append((0, (cam[0][0] + 1.0e6, cam[0][1], cam[0][2])))
    elif phase == 1 and cam:
        moves.append((0, cam[0]))
    elif phase == 3 and cam and 0 <= st['takenIdx'] < count:
        # swb12 measured 27/28 with only two boids moved: 1682:1/4 (the `out.size() < numNeeded`
        # false exit) needs `out` to actually reach numNeeded, i.e. numNeeded candidates that are
        # NOT in `taken`.  So move min(count, numNeeded + 1) boids into the cone: the taken one
        # first (index equality) and the rest as distinct non-taken indices (inequality), and the
        # caller's newBoids has capacity numLost == numNeeded, so out stops exactly at the limit.
        # NOTE: this wider cone is DERIVED from that uncovered arm, not yet measured.
        eye, fwd, right = cam
        ahead = (eye[0] + 2.0 * fwd[0], eye[1] + 2.0 * fwd[1], eye[2] + 2.0 * fwd[2])
        want = min(count, (needed if needed > 0 else 1) + 1)
        moves.append((st['takenIdx'], ahead))
        for k in range(1, want):
            idx = (st['takenIdx'] + k) % count
            if idx == st['takenIdx']:
                break
            lateral = 0.05 * k
            moves.append((idx, (ahead[0] + lateral * right[0], ahead[1] + lateral * right[1],
                                ahead[2] + lateral * right[2])))
    if not moves:
        if phase == 2 and st['assignCalls'] % 100 == 0:
            _log('CALL %d phase=2 (no move) taken=%d needed=%d out=%d boids=%d'
                 % (st['assignCalls'], tcount, needed, ocount, count))
        return
    plan = []
    for idx, pos in moves:
        b = '((CSwarmBasics*)%d)->mBoids.mItems[%d]' % (this, idx)
        try:
            plan.append({'b': b, 'idx': idx, 'pos': pos,
                         'active': _i(g, b + '.mActive'),
                         'addr': _i(g, '&' + b),
                         'tag': 'swb12-%d-ph%d' % (st['apply'] + 1, phase)})
        except Exception as e:
            _log('boid %d read failed: %s' % (idx, e))
            return
    try:
        _RestoreBoids(g.newest_frame(), plan)
    except Exception as e:
        _log('boid restores could not be armed (%s) - no writes' % e)
        return
    wrote = []
    for it in plan:
        try:
            g.execute('set var %s.mTransform.m03 = %r' % (it['b'], it['pos'][0]))
            g.execute('set var %s.mTransform.m13 = %r' % (it['b'], it['pos'][1]))
            g.execute('set var %s.mTransform.m23 = %r' % (it['b'], it['pos'][2]))
            g.execute('set var %s.mActive = 1' % it['b'])
            wrote.append(it)
        except Exception as e:
            _log('APPLY %s write failed on idx %d: %s' % (it['tag'], it['idx'], e))
            return
    if wrote:
        st['apply'] += 1
        _log('APPLY %s swarm=0x%x boids=%d taken=%d needed=%d out=%d moved=%s was_active=%s'
             ' targets=(%s) cam=(%.5f,%.5f,%.5f) takenIdx=%d'
             % (wrote[0]['tag'], this, count, tcount, needed, ocount,
                ','.join(str(it['idx']) for it in wrote),
                ','.join(str(it['active']) for it in wrote),
                ';'.join('(%.4f,%.4f,%.4f)' % it['pos'] for it in wrote),
                cam[0][0] if cam else 0.0, cam[0][1] if cam else 0.0, cam[0][2] if cam else 0.0,
                st['takenIdx']))


FORCE = [
    ('1978 caller gun target', 'CSwarmBasics::UpdateSeekerTargets(', [], [gun_fixture], 0,
     "one real swarm seeker-target uid (an existing kMT_SeekerTarget actor with a valid boid index)"
     " is supplied as the gun's only target so the caller itself builds a non-empty taken; the gun"
     ' vector and the element bits are restored when the call returns'),
    ('1978 seeker geometry', 'CSwarmBasics::AssignSeekerBoids(', [], [assign_fixture], 0,
     'ph0 beyond the orbit distance, ph1 exactly at the camera, ph2 natural, ph3 the taken boid and'
     ' a second boid in front of the camera (equality and inequality of the taken scan); restored at'
     " the call's return"),
]
