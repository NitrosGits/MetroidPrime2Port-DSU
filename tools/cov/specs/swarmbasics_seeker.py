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
# arming fails (the swb11 version wrote first).  phase 2 is deliberate: it is what leaves the run a
# natural swarm, not an observation that the gun locks by itself (swb11 measured that it does not).
#
# CORRECTION (swb12 rework, after the run): the run that produced 26/26 lines / 27/28 branches used a
# version of this fixture whose plan stored the TARGET position in the field the restore wrote, so its
# "RESTORE-BOID" lines rewrote the target and the boids were never restored - the run's phase-2 calls
# were therefore not natural either.  Its numbers are counts only and no transient-restore claim rests
# on them.  Now the plan keeps `target` and the ORIGINAL translation (read before any write), the
# restore writes the original through the cached &mBoids[idx]; the gun element is snapshotted and
# restored as raw 32-bit words at the cached element address, not through a float %r; and phase 3
# takes the index from THIS call's taken vector (guarded, in range) rather than a global from an
# earlier call or another swarm.  Verified statically with an emulated gdb over a model of the boid
# fields, the caller's vectors, the camera and the gun element: unchanged log files are the record of
# the old run and must not be relabelled.
CLASS = 'CSwarmBasics'

import os

import gdb  # this spec is exec'd by tools/cov/force.py inside gdb's python

_OUT = os.environ.get('COV_OUT', '/tmp')
_LOG = os.path.join(_OUT, 'swb13_geom.log')

_VEC_U = 'rstl::vector<unsigned int, rstl::rmemory_allocator>'
# the cast value is the auto_ptr array; index it with _OBJ_LIST0 % mgr wrapped in ((...))[0]
_OBJ_LIST0 = '(rstl::auto_ptr<CObjectList>*)&((CStateManager*)%d)->mObjectLists.mData[0]'
_GUN_EL = '(rstl::pair<TUniqueId, float>*)&((CPlayerGun*)%d)->mSeekerTargets.mData[0]'

_st = {
    'gunCalls': 0, 'gunWrites': 0, 'gunSkipCount': 0, 'gunSkipPick': 0, 'gunSkipNoGun': 0,
    'assignCalls': 0, 'apply': 0, 'err': 0, 'takenIdx': -1, 'takenUid': -1, 'takenSlot': -1,
    'restoreFail': 0,
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


_BOID = '((CSwarmBasics::CBoid*)%d)'


def _off(g):
    """Byte offsets of the three translation floats inside CBoid, from the debug info (once)."""
    if 'off' not in _st:
        _st['off'] = {
            k: _i(g, '(long)&((CSwarmBasics::CBoid*)0)->mTransform.%s' % k)
            for k in ('m03', 'm13', 'm23')
        }
        _log('BOID field offsets: %s' % _st['off'])
    return _st['off']


def _word(g, addr, off):
    return _i(g, '*(int*)((char*)(%s) + %d)' % (_BOID % addr, off))


def _put_word(g, addr, off, word):
    g.execute('set {int}((char*)(%s) + %d) = %d' % (_BOID % addr, off, word))


class _RestoreBoids(gdb.FinishBreakpoint):
    def __init__(self, frame, items):
        super().__init__(frame, internal=True)
        self.items = items

    def stop(self):
        off = _off(gdb)
        for it in self.items:
            addr = it['addr']
            try:
                # write the ORIGINAL raw words back through the CACHED boid address - no re-resolving
                # of $this->mBoids.mItems[idx], no float formatting
                for k in ('m03', 'm13', 'm23'):
                    _put_word(gdb, addr, off[k], it['w'][k])
                gdb.execute('set var (%s)->mActive = %d' % (_BOID % addr, it['active']))
                # READ BACK: a successful gdb execute is not proof that the value is the original
                back = {k: _word(gdb, addr, off[k]) for k in ('m03', 'm13', 'm23')}
                back_active = _i(gdb, '(%s)->mActive' % (_BOID % addr))
                same = back == it['w'] and back_active == it['active']
                if not same:
                    _st['restoreFail'] += 1
                _log('RESTORE-BOID %s %s boid=0x%x idx=%d target=(%.5f,%.5f,%.5f)'
                     ' original_words=[%d,%d,%d] readback_words=[%d,%d,%d] active %d/%d'
                     % (it['tag'], 'OK' if same else 'MISMATCH', addr, it['idx'],
                        it['target'][0], it['target'][1], it['target'][2],
                        it['w']['m03'], it['w']['m13'], it['w']['m23'],
                        back['m03'], back['m13'], back['m23'], it['active'], back_active))
            except Exception as e:
                _st['restoreFail'] += 1
                _log('RESTORE-BOID %s FAILED boid=0x%x idx=%d: %s'
                     % (it['tag'], addr, it['idx'], e))
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
            # the element's two 32-bit words, byte for byte (no float round-trip: a NaN payload or a
            # signed zero would not survive %r -> parse)
            gdb.execute('set {int}((char*)(%s) + 0) = %d' % (elem, info['w0']))
            gdb.execute('set {int}((char*)(%s) + 4) = %d' % (elem, info['w1']))
            # READ BACK the words and the count: the execute succeeding is not proof
            r0 = _i(gdb, '*(int*)((char*)(%s) + 0)' % elem)
            r1 = _i(gdb, '*(int*)((char*)(%s) + 4)' % elem)
            rcount = _i(gdb, '((CPlayerGun*)%d)->mSeekerTargets.mCount' % gun)
            same = (r0 == info['w0'] and r1 == info['w1'] and rcount == info['count'])
            if not same:
                _st['restoreFail'] += 1
            _log('RESTORE-GUN %s %s gun=0x%x mCount back to %d (readback %d), element 0 words'
                 ' 0x%08x 0x%08x (readback 0x%08x 0x%08x)'
                 % (info['tag'], 'OK' if same else 'MISMATCH', gun, info['count'], rcount,
                    info['w0'] & 0xFFFFFFFF, info['w1'] & 0xFFFFFFFF,
                    r0 & 0xFFFFFFFF, r1 & 0xFFFFFFFF))
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
        # raw words of element 0: word 0 is the TUniqueId plus its padding, word 1 the float's bits
        w0 = _i(g, '*(int*)((char*)(%s) + 0)' % elem)
        w1 = _i(g, '*(int*)((char*)(%s) + 4)' % elem)
    except Exception as e:
        _log('gun element snapshot failed: %s' % e)
        return
    tag = 'swb12-gun-%d' % (st['gunWrites'] + 1)
    try:
        _RestoreGun(g.newest_frame(), {'gun': gun, 'elem': elem, 'count': gcount,
                                       'w0': w0, 'w1': w1, 'tag': tag})
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
         ' mCount %d->1 (element 0 words were 0x%08x 0x%08x)'
         % (tag, gun, player, this, slot, uid, ent, idx, nboids, gcount,
            w0 & 0xFFFFFFFF, w1 & 0xFFFFFFFF))


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
    # the index to place in front comes from THIS call's taken vector, never from a global left by
    # an earlier call (or by another swarm's call): guarded read of its single entry
    taken_first = -1
    if tcount > 0:
        try:
            v = _i(g, '((%s*)%d)->mItems[0]' % (_VEC_U, taken))
            taken_first = v if 0 <= v < count else -1
        except Exception:
            taken_first = -1
    phase = st['assignCalls'] % 4
    cam = _cam(g, mgr) if phase in (0, 1, 3) else None
    moves = []
    if phase == 0 and cam:
        moves.append((0, (cam[0][0] + 1.0e6, cam[0][1], cam[0][2])))
    elif phase == 1 and cam:
        moves.append((0, cam[0]))
    elif phase == 3 and cam and taken_first >= 0:
        # swb12 measured 27/28 with only two boids moved: 1682:1/4 (the `out.size() < numNeeded`
        # false exit) needs `out` to actually reach numNeeded, i.e. numNeeded candidates that are
        # NOT in `taken`.  So move min(count, numNeeded + 1) boids into the cone: the taken one
        # first (index equality) and the rest as distinct non-taken indices (inequality), and the
        # caller's newBoids has capacity numLost == numNeeded, so out stops exactly at the limit.
        # NOTE: this wider cone is DERIVED from that uncovered arm, not yet measured.
        eye, fwd, right = cam
        ahead = (eye[0] + 2.0 * fwd[0], eye[1] + 2.0 * fwd[1], eye[2] + 2.0 * fwd[2])
        # never ask for more than the caller's out vector can hold
        ocap = -1
        try:
            if out:
                ocap = _i(g, '((%s*)%d)->mCapacity' % (_VEC_U, out))
        except Exception:
            ocap = -1
        if ocap >= 0 and ocap < needed:
            _log('phase3 skipped: out capacity %d < numNeeded %d' % (ocap, needed))
            return
        want = min(count, (needed if needed > 0 else 1) + 1)
        moves.append((taken_first, ahead))
        for k in range(1, want):
            idx = (taken_first + k) % count
            if idx == taken_first:
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
    off = _off(g)
    for idx, target in moves:
        b = '((CSwarmBasics*)%d)->mBoids.mItems[%d]' % (this, idx)
        try:
            addr = _i(g, '&' + b)          # the absolute boid pointer, cached for the restore
            # the ORIGINAL translation as raw words, read before anything is written to this boid
            words = {k: _word(g, addr, off[k]) for k in ('m03', 'm13', 'm23')}
            plan.append({'addr': addr, 'idx': idx, 'target': target, 'w': words,
                         'active': _i(g, '(%s)->mActive' % (_BOID % addr)),
                         'tag': 'swb13-%d-ph%d' % (st['apply'] + 1, phase)})
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
            b = _BOID % it['addr']
            g.execute('set var %s->mTransform.m03 = %r' % (b, it['target'][0]))
            g.execute('set var %s->mTransform.m13 = %r' % (b, it['target'][1]))
            g.execute('set var %s->mTransform.m23 = %r' % (b, it['target'][2]))
            g.execute('set var %s->mActive = 1' % b)
            wrote.append(it)
        except Exception as e:
            _log('APPLY %s write failed on idx %d: %s' % (it['tag'], it['idx'], e))
            return
    if wrote:
        st['apply'] += 1
        _log('APPLY %s swarm=0x%x boids=%d taken=%d needed=%d out=%d moved=%s was_active=%s'
             ' targets=(%s) originals=(%s) cam=(%.5f,%.5f,%.5f) taken_first=%d'
             % (wrote[0]['tag'], this, count, tcount, needed, ocount,
                ','.join(str(it['idx']) for it in wrote),
                ','.join(str(it['active']) for it in wrote),
                ';'.join('(%.4f,%.4f,%.4f)' % it['target'] for it in wrote),
                ';'.join('words[%d,%d,%d]' % (it['w']['m03'], it['w']['m13'], it['w']['m23'])
                         for it in wrote),
                cam[0][0] if cam else 0.0, cam[0][1] if cam else 0.0, cam[0][2] if cam else 0.0,
                taken_first))


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
