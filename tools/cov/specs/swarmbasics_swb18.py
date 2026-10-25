"""swb18: the two SwarmBasics bodies no natural run can finish, by forced native calls.

Default force.py (not force_ret.py): this spec installs its own breakpoints and does the
work in one batch command that gdb runs outside stop() (`hook-backtrace` -> `swb18-run`),
so no inferior call happens inside a breakpoint's stop().

    BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swarmbasics_swb18.py <out> \
      3dfd2249,0g_swamp_hall --con tools/cov/specs/swarmbasics_swamp.con --fast --frames 1200

Bodies (owner: this lane; nothing else is touched):

* fn_80_10C0 FindClosestCell - no call site anywhere in the module (0 `bl`, no vtable slot), so
  the only way to run it is a native call on a live swarm.  Five scenarios on a forced partition
  list and forced box geometry: empty list (result stays zero), a later cell closer (update), the
  first cell already closest (strict `<` keeps it), an exact midpoint tie (first wins) and a NaN
  query (every comparison unordered -> `fcmpo; bge` -> no update, zero returned).
* fn_80_7E4 UpdateEffects - called by UpdateSwarmAnimations every frame, but the shipped swarms'
  animation data never sends a sound node through every filter.  Eight calls on a real CAnimData:
  zero count, wrong node type, weight miss, character mismatch, the high sound-id bit, character
  -1, character == animData's own index, and two nodes.  Every emitted 3D emitter is read at
  CSfxManager::AddEmitter and released through the native RemoveEmitter.

Everything is written through members (`$this`-relative) or a cached element address inside a
Transaction that snapshots the original bytes before the first write and restores them with a
readback.  Nothing is written in stop(): the trigger only reads.
"""
CLASS = 'CSwarmBasics'
FORCE = []

import gdb
import hashlib
import json
import math
import os

state = {'pending': None, 'done': False, 'scenario': None, 'sound': [], 'busy': False,
         'expect_sound': None, 'expect_bounds': None, 'active': [], 'trigger': None,
         'applies': 0, 'restores': 0, 'restore_fail': 0, 'release_fail': 0, 'sound_calls': 0,
         'closest': 0, 'effects': 0, 'soft_fail': 0, 'err': ''}

def log(event, **data):
    with open(os.path.join(os.environ['COV_OUT'], 'swb18.jsonl'), 'a') as f:
        f.write(json.dumps(dict(event=event, **data), sort_keys=True) + '\n')

def ev(s):
    return gdb.parse_and_eval(s)

def iv(s):
    return int(ev(s))

def check(value, label):
    if not value:
        log('FAIL', label=label)
        raise gdb.GdbError(label)

def verify(value, label):
    """An oracle.  In SWB18_SOFT=1 runs a mismatch is logged and the fixture continues, so one
    diagnostic launch shows every scenario; a run without SWB18_SOFT fails on the first one."""
    if value:
        return
    if os.environ.get('SWB18_SOFT') == '1':
        state['soft_fail'] += 1
        log('SOFT-FAIL', label=label)
        return
    check(False, label)

def write(lv, rhs):
    gdb.execute('set var %s = %s' % (lv, rhs), to_string=True)

def quote(name):
    return "'" + name + "'"

def nan_bits():
    import struct
    return struct.pack('<I', 0x7fc00000)

class Transaction:
    """Snapshot the original bytes of every lvalue before the first write, restore them all
    afterwards (reverse order, every one attempted) and read each restore back."""

    def __init__(self, label, lvalues):
        self.label = label
        self.saved = []
        inf = gdb.selected_inferior()
        for lv in lvalues:
            value = ev(lv)
            check(value.address is not None, 'addressable snapshot: ' + lv)
            address, size = int(value.address), value.type.sizeof
            self.saved.append((lv, address, bytes(inf.read_memory(address, size))))

    def __enter__(self):
        state['applies'] += 1
        log('ARM', label=self.label, n=state['applies'],
            fields=[dict(lvalue=lv, address=a, original=b.hex()) for lv, a, b in self.saved])
        return self

    def __exit__(self, typ, value, tb):
        inf = gdb.selected_inferior()
        errors, rows = [], []
        for lv, address, original in reversed(self.saved):
            try:
                inf.write_memory(address, original)
                actual = bytes(inf.read_memory(address, len(original)))
                rows.append(dict(lvalue=lv, address=address, ok=actual == original,
                                 original=original.hex(), readback=actual.hex()))
                if actual != original:
                    errors.append(lv)
            except Exception as error:
                errors.append(lv + ': ' + str(error))
        state['restores'] += 1
        if errors:
            state['restore_fail'] += 1
        log('RESTORE', label=self.label, fields=rows, errors=errors,
            exception=None if typ is None else typ.__name__)
        check(not errors, 'restore mismatch ' + self.label + ': ' + str(errors))
        return False

def vector(value):
    return tuple(float(value['m' + axis]) for axis in 'XYZ')

def translation(expr):
    """A CTransform4f's translation column (m03, m13, m23)."""
    return tuple(float(ev(expr + '.m%d3' % row)) for row in range(3))

def near(a, b):
    return all(abs(x - y) <= 0.0003 * max(1., abs(y)) for x, y in zip(a, b))

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

def swarm_anim(this):
    """The swarm's own CAnimData - the exact object the natural caller
    (UpdateSwarmAnimations: UpdateEffects(mgr, *mModelDatas[i].AnimationData(), ...)) passes."""
    S = '((CSwarmBasics*)%d)->' % this
    check(iv(S + 'mModelDatas.mCount') >= 1, 'swarm has a model data entry')
    anim = iv(S + 'mModelDatas.mItems[0].mAnimData.mItem')
    check(anim != 0, 'swarm model data owns a real CAnimData')
    log('REAL_ANIM', swarm=this, anim=anim)
    return anim

def cells(bounds_min, bounds_max, margin):
    """BoxForPosition's cell centres from the box GetBoundingBox reports, computed here (not by
    calling BoxForPosition or FindClosestCell): size = extent/5, centre = min - margin +
    (idx + .5) * size.  The margin shifts min and max alike, so it moves the centre by -margin."""
    size = [(hi - lo) / 5. for lo, hi in zip(bounds_min, bounds_max)]
    return {idx: tuple(bounds_min[axis] - margin + (idx[axis] + .5) * size[axis]
                       for axis in range(3))
            for idx in ((x, y, z) for x in range(5) for y in range(5) for z in range(5))}

def slot(idx):
    return idx[0] + 5 * idx[1] + 25 * idx[2]

def nearest(points, pos):
    """Strict `<` keeps the first equal candidate, exactly as retail's `fcmpo; bge` skip, and a
    NaN query never updates (unordered -> bge taken)."""
    best, distance = (0., 0., 0.), 3.4028234663852886e38
    for point in points:
        new_distance = sum((a - b) ** 2 for a, b in zip(point, pos))
        if new_distance < distance:
            best, distance = point, new_distance
    return best

class Return(gdb.FinishBreakpoint):
    def __init__(self, callback):
        super().__init__(gdb.newest_frame(), internal=True)
        self.callback = callback
    def stop(self):
        self.callback()
        return False
    def out_of_scope(self):
        state['err'] = 'observed native callee unwound'
        log('FAIL', label=state['err'])

class Sound(gdb.Breakpoint):
    def stop(self):
        expected = state['expect_sound']
        if expected is None:
            return False
        params = ev('((CAudioSys::C3DEmitterParmData*)%d)' % iv('$rdi')).dereference()
        actual = dict(pos=vector(params['mPos']), direction=vector(params['mDir']),
                      maxdist=float(params['mMaxDist']), falloff=float(params['mDistComp']),
                      flags=int(params['mFlags']), sfx=int(params['mSfxId']),
                      maxvol=int(params['mMaxVol']), minvol=int(params['mMinVol']),
                      area=iv('$esi'), acoustics=iv('$edx') & 255, looped=iv('$ecx') & 255)
        hit = [name for name in ('direction', 'maxdist', 'falloff', 'flags', 'sfx', 'maxvol',
                                 'minvol', 'area', 'acoustics', 'looped')
               if actual[name] != expected[name]]
        verify(not hit, 'sound parameter ' + ','.join(hit) + ' in ' + str(state['scenario']))
        verify(any(all(abs(a - b) <= 1e-6 for a, b in zip(actual['pos'], p))
                   for p in state['active']),
               'emitted position is an active boid translation in ' + str(state['scenario']))
        state['sound_calls'] += 1
        record = dict(scenario=state['scenario'], params=actual)
        state['sound'].append(record)
        def returned():
            record['handle'] = iv('$eax') & 0xffffffff
            log('SOUND_RESULT', **record)
        Return(returned)
        return False

def release_sounds(before):
    for record in state['sound'][before:]:
        handle = record.get('handle', 0)
        if not handle:
            continue
        try:
            soundptr = ('((CSfxManager::CBaseSfxWrapper**)CSfxManager::mChannels'
                        '[CSfxManager::mCurrentChannel].mSounds.mData)[%d]' % (handle & 0xfff))
            check(iv(soundptr + '->mHandle.mID') == handle, 'native emitter handle identity')
            gdb.execute('call CSfxManager::RemoveEmitter(' + soundptr + '->mHandle)',
                        to_string=True)
            log('SOUND_RELEASE', handle=handle)
        except (gdb.error, gdb.GdbError) as error:
            state['release_fail'] += 1
            log('SOUND_RELEASE_FAIL', handle=handle, error=str(error)[:120])

class Trigger(gdb.Breakpoint):
    def stop(self):
        if state['done'] or state['busy'] or state['pending'] is not None:
            return False
        this, mgr = iv('$rdi'), iv('$rsi')
        S = '((CSwarmBasics*)%d)->' % this
        if iv('((CStateManager*)%d)->mUpdateFrameIdx' % mgr) < 500:
            return False
        if iv(S + 'mBoids.mCount') < 2 or iv(S + 'mActiveBoidIndices.mCount') < 1:
            return False
        if iv(S + 'mModelDatas.mCount') < 1:
            return False
        if iv(S + 'mModelDatas.mItems[0].mAnimData.mItem') == 0:
            return False
        state['pending'] = (this, mgr)
        state['trigger'] = dict(thread=gdb.selected_thread().global_num,
                                pc=gdb.newest_frame().pc(), level=gdb.newest_frame().level())
        self.enabled = False
        return True

class Run(gdb.Command):
    def __init__(self):
        super().__init__('swb18-run', gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        check(state['pending'] is not None, 'unexpected debugger stop')
        check(not state['busy'], 'fixture re-entry')
        frame = gdb.newest_frame()
        check(gdb.selected_thread().global_num == state['trigger']['thread'],
              'frame ownership: same thread')
        check(frame.pc() == state['trigger']['pc'] and frame.level() == state['trigger']['level'],
              'frame ownership: same activation')
        this, mgr = state['pending']
        state['pending'] = None
        state['busy'] = True
        try:
            self.fixture(this, mgr)
        except Exception as error:
            state['err'] = '%s: %s' % (type(error).__name__, error)
            log('FAIL', label='fixture error: ' + state['err'][:200])
        finally:
            state['busy'] = False
            state['scenario'] = None

    def fixture(self, this, mgr):
        S = '((CSwarmBasics*)%d)->' % this
        M = '((CStateManager*)%d)->' % mgr
        anim = swarm_anim(this)
        A = '((CAnimData*)%d)->' % anim
        check(iv('CAnimData::mSoundPOINodes.mCount') == 48, '48 constructed real POI slots')
        char_index = iv(A + 'mCharIdx')
        max_volume, min_volume = iv(S + 'mMaxVolume'), iv(S + 'mMinVolume')
        area = iv('((CEntity*)%d)->mAreaId.value' % this)
        count = iv(S + 'mBoids.mCount')
        active = []
        for slot_index in range(iv(S + 'mActiveBoidIndices.mCount')):
            boid = iv(S + 'mActiveBoidIndices.mItems[%d]' % slot_index)
            check(0 <= boid < count, 'active boid index in range')
            active.append(translation(S + 'mBoids.mItems[%d].mTransform' % boid))
        state['active'] = active
        log('GEOMETRY', boids=count, active=len(active), char_index=char_index,
            max_volume=max_volume, min_volume=min_volume, area=area)

        # ---- fn_80_10C0 FindClosestCell: forced partition list + forced box geometry ----------
        list_lv = S + 'mPartitionedBoidLists'
        data = S + 'mPartitionedBoidLists.mData'
        boid_size = gdb.lookup_type('CSwarmBasics::CBoid').pointer().sizeof
        query = S + 'mBoids.mItems[0].mVelocity'
        with Transaction('closest geometry', [list_lv, S + 'mBoids.mItems[0]', S + 'mTransform',
                                             S + 'mBoundingBoxExtent']):
            inf = gdb.selected_inferior()
            for row in range(3):
                for col in range(3):
                    write(S + 'mTransform.m%d%d' % (row, col), str(int(row == col)))
                write(S + 'mTransform.m%d3' % row, '0')
                write(S + 'mBoundingBoxExtent.m' + 'XYZ'[row], '50.f')
            extent_x = float(ev(S + 'mBoundingBoxExtent.mX'))
            check(extent_x == 50., 'forced box extent')
            centres = cells((-25., -25., -25.), (25., 25., 25.), 0.1)
            inf.write_memory(int(ev(data).address), bytes(125 * boid_size))
            write(list_lv + '.mCount', '125')
            heads = {'first': (0, 0, 0), 'second': (4, 0, 0)}

            def set_heads(names):
                inf.write_memory(int(ev(data).address), bytes(125 * boid_size))
                for i, name in enumerate(names):
                    write('((CSwarmBasics::CBoid**)&' + data + ')[%d]' % slot(heads[name]),
                          '&' + S + 'mBoids.mItems[%d]' % i)

            def ask(pos):
                for axis, value in zip('XYZ', pos):
                    if math.isnan(value):
                        inf.write_memory(int(ev(query + '.m' + axis).address),
                                         nan_bits())
                    else:
                        write(query + '.m' + axis, repr(value))
                return vector(ev('((CSwarmBasics*)%d)->FindClosestCell(%s)' % (this, query)))

            scenarios = (
                ('empty', [], centres[(2, 2, 2)]),
                ('second wins', ['first', 'second'], centres[heads['second']]),
                ('first stays', ['first', 'second'], centres[heads['first']]),
                ('far query', ['first', 'second'], (1.e6, 1.e6, 1.e6)),
                ('unordered query', ['first', 'second'], (float('nan'), 0., 0.)),
            )
            for label, names, pos in scenarios:
                state['scenario'] = 'closest ' + label
                set_heads(names)
                result = ask(pos)
                expected = nearest([centres[heads[name]] for name in names], pos)
                nans = any(math.isnan(v) for v in pos)
                if nans:
                    ok = result == (0., 0., 0.)
                else:
                    ok = near(result, expected)
                state['closest'] += 1
                log('CLOSEST_RESULT', scenario=label, actual=result, expected=expected, ok=ok,
                    query=pos,
                    heads=[iv('(long)((CSwarmBasics::CBoid**)&' + data + ')[%d]' % slot(name))
                           for name in heads.values()],
                    extent=[float(ev(S + 'mBoundingBoxExtent.m' + axis)) for axis in 'XYZ'],
                    translation=list(translation(S + 'mTransform')))
                verify(ok, 'closest output ' + label)
            set_heads(['first', 'second'])

        # ---- fn_80_7E4 UpdateEffects: real CAnimData, controlled nodes -------------------------
        node = '((CSoundPOINode*)CAnimData::mSoundPOINodes.mData)[0]'
        node2 = '((CSoundPOINode*)CAnimData::mSoundPOINodes.mData)[1]'
        statics = []
        for suffix in ('maxDistance', 'falloff'):
            for guard in (False, True):
                prefix = (('guard variable for ' if guard else '')
                          + 'CSwarmBasics::UpdateEffects(CStateManager&, CAnimData&, int)::' + suffix)
                found = []
                for line in open(os.environ['COV_NM']):
                    raw, dem = line.rstrip('\n').split('\t')
                    if dem == prefix:
                        found.append(raw.split()[-1])
                check(len(found) == 1, 'static identity ' + prefix)
                statics.append(('*(%s*)&%s' % ('unsigned long long' if guard else 'float',
                                               quote(found[0])), guard))
        node_sfx = iv(node + '.mSfxId')
        loco_sfx = iv(S + 'mLocomotionLoopedSound')
        sfx = node_sfx if 0 < node_sfx < 0xffff else loco_sfx
        log('SOUND_ID', chosen=sfx, node_original=node_sfx, locomotion=loco_sfx)
        cases = (
            # label, count, type, character, weight, sfx, maxdist, falloff, emits
            ('zero count', 0, 8, -1, 2.0, sfx, 0., 0., 0),
            ('wrong type', 1, 6, -1, 2.0, sfx, 0., 0., 0),
            ('weight miss', 1, 8, -1, -1.0, sfx, 0., 0., 0),
            ('character mismatch', 1, 8, char_index + 1, 2.0, sfx, 0., 0., 0),
            ('high sound bit', 1, 8, -1, 2.0, sfx | 0x80000000, 0., 0., 0),
            ('character -1', 1, 8, -1, 2.0, sfx, 17., .25, 1),
            ('character match', 1, 8, char_index, 2.0, sfx, 31., .75, 1),
            ('two nodes', 2, 8, -1, 2.0, sfx, 45., .5, 1),
        )
        with Transaction('effects nodes', [A + 'mPassedSoundCount', node, node2,
                                           S + 'mMaxVolume', S + 'mMinVolume']):
            with Transaction('effects statics', [lv for lv, guard in statics]):
                for lv, guard in statics:
                    write(lv, '0')
                for (label, count_c, kind, char, weight, sound, maxdist, falloff,
                     expected_calls) in cases:
                    state['scenario'] = 'effects ' + label
                    write(A + 'mPassedSoundCount', str(count_c))
                    for target, fields in ((node, dict(mType=kind, mCharIdx=char,
                                                      mWeight=weight, mSfxId=sound,
                                                      mMaxDist=maxdist, mFalloff=falloff)),
                                           (node2, dict(mType=6, mCharIdx=-1, mWeight=weight,
                                                        mSfxId=sound, mMaxDist=0.,
                                                        mFalloff=0.))):
                        for field, value in fields.items():
                            write(target + '.' + field, repr(value) if isinstance(value, float)
                                  else str(value))
                    # The first emitting case initialises the two statics (17/.25); the later
                    # emitting cases change the node but must keep emitting 17/.25.
                    sticky = (17., .25) if expected_calls else None
                    state['expect_sound'] = dict(direction=(0., 0., 0.), flags=1,
                                                 sfx=sound & 0xffff, maxvol=max_volume,
                                                 minvol=min_volume, area=area, acoustics=1,
                                                 looped=0, **({'maxdist': sticky[0],
                                                               'falloff': sticky[1]}
                                                              if sticky else
                                                              {'maxdist': 31., 'falloff': 0.75}))
                    before = len(state['sound'])
                    try:
                        gdb.execute('call ((CSwarmBasics*)%d)->UpdateEffects(*((CStateManager*)%d),'
                                    '*((CAnimData*)%d),%d)' % (this, mgr, anim, max_volume),
                                    to_string=True)
                        verify(len(state['sound']) - before == expected_calls,
                               'emitter count ' + label)
                        state['effects'] += 1
                        log('EFFECTS_RESULT', scenario=label, emitter_calls=expected_calls)
                    finally:
                        state['expect_sound'] = None
                        release_sounds(before)
        state['done'] = True
        log('DONE', closest=state['closest'], effects=state['effects'],
            sound_calls=state['sound_calls'], applies=state['applies'],
            restores=state['restores'], restore_fail=state['restore_fail'],
            release_fail=state['release_fail'])

def finished(event):
    summary = ('SUMMARY exit: closest=%d effects=%d sound_calls=%d applies=%d restores=%d '
               'restore_fail=%d release_fail=%d soft_fail=%d done=%s err=%s'
               % (state['closest'], state['effects'], state['sound_calls'], state['applies'],
                  state['restores'], state['restore_fail'], state['release_fail'],
                  state['soft_fail'], state['done'], state['err'] or '-'))
    log('EXIT', summary=summary)
    with open(os.path.join(os.environ['COV_OUT'], 'swb18_summary.txt'), 'w') as f:
        f.write(summary + '\n')
    if not state['done']:
        log('FAIL', label='fixture did not complete')

def install():
    binary = gdb.current_progspace().filename
    check(bool(binary), 'GDB executable identity')
    with open(binary, 'rb') as f:
        digest = hashlib.file_digest(f, 'sha256').hexdigest()
    log('BINARY', path=binary, sha256=digest)
    Sound('*' + symbol('CSfxManager::AddEmitter(CAudioSys::C3DEmitterParmData&'), internal=True)
    Trigger('*' + symbol('CSwarmBasics::UpdatePartition('), internal=True)
    Run()
    gdb.execute('define hook-backtrace\nswb18-run\ncontinue\nend')
    gdb.events.exited.connect(finished)

if os.environ.get('SWB18_SELFTEST') != '1':
    install()
