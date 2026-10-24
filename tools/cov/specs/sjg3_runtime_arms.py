# Explicit live inputs; no skipped/stubbed callees. Run with force_ret.py.
CLASS = 'CIngSpaceJumpGuardian'
SPEC = []
scenarios = []


def call(name, args='', writes=()):
    scenarios.append((list(writes), '$this->%s(*$mgr%s)' % (name, ', ' + args if args else '')))


# All fixtures run before the first active Think, when collision/light setup is complete.
for alive, light in ((1, None), (0, None), (1, 0xffff)):
    writes = [('$this->mAlive', str(alive))]
    if light is not None:
        writes.append(('$this->mLightId.value', str(light)))
    scenarios.append((writes, '$this->UpdateLight(0.25f, *$mgr)'))
for intensity in (0., 2.):
    scenarios.append(([('$this->mLightIntensity', str(intensity))], '$this->UpdateLight(0.25f, *$mgr)'))

descs = '$this->mCollisionManager.mPtr->mJointDescriptions'
for name, args in (('UpdateCollisionVulnerabilities', ''),
                   ('SetCollisionVulnerability', 'CDamageVulnerability::ReflectVulnerabilty()')):
    call(name, args)
    call(name, args, [(descs + '.mItems[0].mActorId.value', '65535')])
    call(name, args, [(descs + '.mCount', '0')])

for dt in ('0.f', '0.25f'):
    call('GetAimPosition', dt)
    call('GetAimPosition', dt, [('$this->mCollarSeg.mId', '255')])

for jumping, target in ((1, 0xffff), (0, 0xffff), (0, 0xfffe), (0, None)):
    target = '$P->mUniqueId.value' if target is None else str(target)
    call('ComputeJumpVelocity', writes=[('$this->mJumping', str(jumping)),
                                      ('$this->mJumpTarget.value', target),
                                      ('$this->mJumpApexHeight', '10.f')])
for height in (-5, 5):
    call('ComputeJumpVelocity', writes=[('$this->mJumping', '0'),
         ('$this->mJumpTarget.value', '$P->mUniqueId.value'),
         ('$this->mJumpApexHeight', '10.f'),
         ('$this->mTransform.m23', '$P->mTransform.m23 + %d' % height)])

for msg in (0, 2, 3):
    call('SteerToDest', '(EStateMsg)%d, 0.016f' % msg)
call('SteerToDest', '(EStateMsg)1, 0.016f', [('$this->mJumpTarget.value', '65535')])
for sign in (0, 1, -1):
    writes = [('$this->mJumpTarget.value', '$P->mUniqueId.value')]
    for coord, translation, forward in (('X', '03', '01'), ('Y', '13', '11'), ('Z', '23', '21')):
        writes.append(('$this->mPathFindNavigation.mDestinationPosition.m' + coord,
                       '$this->mTransform.m%s + %d * $this->mTransform.m%s' % (translation, sign, forward)))
    call('SteerToDest', '(EStateMsg)1, 0.016f', writes)

for msg in (0, 1, 2, 3):
    for path in (0, 1):
        call('PathFind', '(EStateMsg)%d, 0.016f' % msg,
             [('$this->mHasJumpPath', str(path))])
for magnitude in ('1.f', '-1.f', '-1.e-20f'):
    writes = [('$this->mHasJumpPath', '0'),
              ('$this->mPathFindSearch.mWaypoints.mCount', '2'),
              ('$this->mPathFindSearch.mCurWaypoint', '0')]
    for coord, forward in (('X', '01'), ('Y', '11'), ('Z', '21')):
        writes.append(('$this->mBodyController.mPtr->mCmdMgr.mMove.m' + coord,
                       magnitude + ' * $this->mTransform.m' + forward))
    call('PathFind', '(EStateMsg)2, 0.016f', writes)

call('FollowIntroPattern', '(EStateMsg)0, 0.016f')
call('FollowIntroPattern', '(EStateMsg)0, 0.016f',
     [('$this->mTransform.m%s' % slot, value) for slot, value in
      (('00', '1'), ('01', '0'), ('02', '0'), ('10', '0'), ('11', '1'), ('12', '0'),
       ('20', '0'), ('21', '0'), ('22', '1'), ('03', '0'), ('13', '1000000'), ('23', '0'))])
for sign in (-1, 1):
    writes = []
    for translation, forward in (('03', '01'), ('13', '11'), ('23', '21')):
        writes.append(('$this->mTransform.m' + translation,
                       '((CActor*)$mgr->GetObjectById($this->mIntroWaypoint))->mTransform.m%s + %d * $this->mTransform.m%s' % (translation, sign, forward)))
    call('FollowIntroPattern', '(EStateMsg)0, 0.016f', writes)
call('FollowIntroPattern', '(EStateMsg)0, 0.016f', [('$this->mIntroWaypoint.value', '$P->mUniqueId.value')])
call('FollowIntroPattern', '(EStateMsg)2, 0.016f')
call('FollowIntroPattern', '(EStateMsg)1, 0.016f')

# Activation creates a real blob, allowing both emission arms, plus missing-id input.
call('Dead', '(EStateMsg)0, 0.016f')
for active in (0, 1):
    call('UpdateBlob', '0.016f', [('$this->mBlobActive', str(active))])
call('UpdateBlob', '0.016f', [('$this->mBlobId.value', '65535')])

body = '$this->mBodyController.mPtr->mBodyStateInfo.mState'
scale = '$this->mModelData.mPtr->mScale.mZ'
call('Dead', '(EStateMsg)2, 0.016f')
call('Dead', '(EStateMsg)1, 0.016f', [(body, 'pas::kAS_Locomotion')])
for boss, z, timer in ((True, 1., 2.), (False, 0., 2.), (False, 0., 0.)):
    call('Dead', '(EStateMsg)1, 0.016f', [(body, 'pas::kAS_Death'),
         ('$mgr->mBossId.value', '$this->mUniqueId.value' if boss else '65535'),
         (scale, str(z)), ('$this->mDeathTimer', str(timer)),
         ('$this->mRender', '1'), ('$this->mBlobActive', '1')])

CALLS = [('sjg3 runtime fixtures', 'CIngSpaceJumpGuardian::Think(',
          '((CIngSpaceJumpGuardian*)$rdi)->mActive', scenarios,
          'real light/collision/aim/jump/blob/death calls; restore input members')]
