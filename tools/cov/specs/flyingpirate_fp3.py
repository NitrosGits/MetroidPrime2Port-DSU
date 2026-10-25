# fp3: FlyingPirate (module 22) CoverFind 6040 and Think 15DC arms that no shipped fight reaches,
# forced on the coverage build by direct calls (CALLS: members written, the body called, members
# written back; a stub makes a callee return at once). `forced (<label>)` in the note's branch table.
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/flyingpirate_fp3.py <out> 1baa96c2,01_cliff_entrance \
#     --con tools/cov/specs/flyingpirate_fp3_cliff.con --fast --frames 4200
#   (the same spec with flyingpirate_fp3_casual.con on 08_sand_casualpirates, which has the water)
# $fp3_obj(mgr, list, n, 'Type') is the n-th object of that list whose dynamic type is Type (0 when
# none); $fp3_uid(obj) its unique id. Both walk CStateManager::mObjectLists (16 bytes per auto_ptr,
# mItem at +8).
import os
import gdb

CLASS = 'CFlyingPirate'
OUT = os.environ['COV_OUT']
LOGF = os.path.join(OUT, 'fp3.log')


def flog(msg):
    with open(LOGF, 'a') as fh:
        fh.write(msg + '\n')


class Obj(gdb.Function):
    def __init__(self):
        super().__init__('fp3_obj')

    def invoke(self, mgr, lid, n, ty):
        try:
            return self.find(mgr, int(lid), int(n), ty if isinstance(ty, str) else ty.string())
        except Exception as exc:  # a failed walk is "none", never an aborted scenario batch
            flog('fp3_obj failed: %s' % exc)
            return gdb.Value(0).cast(gdb.lookup_type('unsigned long'))

    def find(self, mgr, lid, n, ty):
        if lid < 0:  # any list
            for k in range(8):
                r = self.find(mgr, k, n, ty)
                if int(r):
                    return r
            return gdb.Value(0).cast(gdb.lookup_type('unsigned long'))
        base = int(mgr) + int(gdb.parse_and_eval('(long)&((CStateManager*)0)->mObjectLists.mData'))
        lst = int(gdb.parse_and_eval('*(unsigned long*)%d' % (base + 16 * lid + 8)))
        if not lst:
            return gdb.Value(0).cast(gdb.lookup_type('unsigned long'))
        first = int(gdb.parse_and_eval('((CObjectList*)%d)->mFirstId' % lst))
        i, seen = first, 0
        while 0 <= i < 1024:
            ent = gdb.parse_and_eval('((CObjectList*)%d)->mObjects[%d].mEntity' % (lst, i))
            if int(ent) and str(ent.dynamic_type).replace(' ', '').rstrip('*') == ty:
                if seen == n:
                    return gdb.Value(int(ent)).cast(gdb.lookup_type('unsigned long'))
                seen += 1
            i = int(gdb.parse_and_eval('((CObjectList*)%d)->mObjects[%d].mNext' % (lst, i)))
        return gdb.Value(0).cast(gdb.lookup_type('unsigned long'))


class Uid(gdb.Function):
    def __init__(self):
        super().__init__('fp3_uid')

    def invoke(self, obj):
        if not int(obj):
            return 65535
        return int(gdb.parse_and_eval('((CEntity*)%d)->mUniqueId.value' % int(obj)))


Obj()
Uid()

COVER = '$fp3_obj($mgr, 5, %d, "CScriptCoverPoint")'
WATER = '$fp3_obj($mgr, -1, 0, "CScriptWater")'
CF = '$this->CoverFind(*$mgr, *(CTriggerData*)$this)'
TH = '$this->Think(0.016f, *$mgr)'
LAND = ('CScriptCoverPoint::ShouldLandHere(', 1)
FREE = ('CScriptCoverPoint::GetInUse(', 0)
BUSY = ('CScriptCoverPoint::GetInUse(', 1)
BIG = ('$this->mData.mMaxCoverDistance', '1000000.0')


def cover(n):
    return '((CScriptCoverPoint*)%s)' % (COVER % n)


def water():
    return WATER


CALLS = [
    ('CoverFind arms', 'CFlyingPirate::Think(', '%s != 0' % (COVER % 0).replace('$mgr', '(CStateManager*)$rsi'), [
        # no cover point lands: the walk ends on `!ShouldLandHere`, then the release, null closest
        ([], CF),
        # lands but busy
        ([], CF, [LAND, BUSY]),
        # lands, free, another area (own area id + 1)
        ([('$this->mAreaId.value', '$this->mAreaId.value + 1')], CF, [LAND, FREE]),
        # the first cover point switched off
        ([('%s->mActive' % cover(0), '0')], CF, [LAND, FREE]),
        # lands, free, same area, a 0 search distance: `mag < closestMag` false
        ([('$this->mData.mMaxCoverDistance', '0.0')], CF, [LAND, FREE]),
        # lands, free, same area, any distance: a closest cover point, then the SetCoverPoint arm
        ([BIG], CF, [LAND, FREE]),
        # the last release: nothing lands, the cover point is let go
        ([], CF),
    ], 'CoverFind with ShouldLandHere / GetInUse stubbed'),
    ('Think arms', 'CFlyingPirate::Think(', '((CEntity*)$rdi)->mActive != 0', [
        ([('((CEntity*)$this)->mActive', '0')], TH),
        ([('$this->mPrevInCineCam', '0')], TH, [('CCameraManager::IsInCinematicCamera(', 1), ('CStateManager::IsMultiplayer(', 0)]),
        ([('$this->mPrevInCineCam', '1')], TH, [('CCameraManager::IsInCinematicCamera(', 1), ('CStateManager::IsMultiplayer(', 0)]),
        ([('$this->mPrevInCineCam', '1')], TH, [('CCameraManager::IsInCinematicCamera(', 0), ('CStateManager::IsMultiplayer(', 0)]),
        ([('$this->mPrevInCineCam', '0')], TH, [('CCameraManager::IsInCinematicCamera(', 1), ('CStateManager::IsMultiplayer(', 1)]),
        ([('$this->mShotAt', '1'), ('$this->mHitByPlayerProjectile', '1')], TH),
        ([], TH, [('CActor::InFluidId(', '$this->mUniqueId.value')]),
        ([('$this->mPendingDeath', '0'), ('$this->mTransform.m23', '-1000000.0')], TH,
         [('CActor::InFluidId(', '$fp3_uid(%s)' % water())]),
        ([('$this->mPendingDeath', '0'), ('$this->mTransform.m23', '1000000.0')], TH,
         [('CActor::InFluidId(', '$fp3_uid(%s)' % water())]),
        ([('$this->mIsAquaPirate', '1')], TH),
        ([('$this->mBodyController.mPtr->mFrozen', '1'), ('$this->mBodyController.mPtr->mFrozenDur', '4.0'),
          ('$this->mBodyController.mPtr->mTimeFrozen', '1.0')], TH),
    ], 'Think with cinematic camera / water / aqua / frozen forced'),
]
SPEC = []
