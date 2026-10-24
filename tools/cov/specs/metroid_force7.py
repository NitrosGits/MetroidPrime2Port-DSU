# Metroid (module 40), slice met5: the arms of IsPirateValidTarget (fn_40_470C), IsPlayerInFluid
# (fn_40_15A8) and FindNearestTarget (fn_40_1170) that no fight reaches, by direct calls (force_ret.py
# CALLS, hooked on CMetroid::Think once a pirate was seen) with members written for one call and stubs.
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov \
#     tools/cov/force.sh tools/cov/specs/metroid_force7.py <out> 42b935e4,08_sand_casualpirates \
#     --con tools/cov/specs/metroid_force6.con --fast --frames 1200
CLASS = 'CMetroid'
try:  # the captured objects stay 0 until the first sighting
    import gdb
    gdb.execute('set $pir = 0')
    gdb.execute('set $sa = 0')
except ImportError:  # read outside gdb
    pass

MT = '$this->mAttackTarget.value'
PL = '$P->mUniqueId.value'
PIRU = '((CSpacePirate*)$pir)->mUniqueId.value'
SAU = '((CScriptActor*)$sa)->mUniqueId.value'
SELFU = '$this->mUniqueId.value'
STATE = '$this->mAttackState'
BODY = '((CPatterned*)$pir)->mBodyController.mPtr->mBodyStateInfo'
DIE = '((CBSDie*)%s.mStates.mItems[4])->mIsDead' % BODY

FORCE_RET = [
    ('capture pirate|once', 'CSpacePirate::AcceptScriptMsg(', 1, [(0, [('!$pir', '$rdi')])], 'a pirate'),
    ('capture scriptactor|once', 'CScriptActor::AcceptScriptMsg(', 1, [(0, [('!$sa', '$rdi')])], 'a script actor'),
]


PIRVALID = 'CMetroid::IsPirateValidTarget('
GHI = 'CSpacePirate::GetHealthInfo('
PA = '((CSpacePirate*)$pir)->mAttachedActor.value'
PH = '((CHealthInfo*)((CSpacePirate*)$pir)->HealthInfo())->mHealth'
FL0 = '*(unsigned short*)&$P->mFluidIds.mData[0]'
CNT = '$P->mFluidIds.mCount'
IPV = '$this->IsPirateValidTarget(*(CSpacePirate*)$pir)'
IPF = '$this->IsPlayerInFluid(*$P, *$mgr)'
FNT = '$this->FindNearestTarget(*$mgr, %s)'
INV = 'kInvalidUniqueId'

CALLS = [('met5 direct calls', 'CMetroid::Think(', '$pir != 0', [
    # IsPirateValidTarget (0x47D0 attached id, vtable 0x3c GetHealthInfo, 0x4818 hp > 0)
    ([(PA, '0xffff'), (PH, '5.0')], IPV),
    ([(PA, '1'), (PH, '5.0')], IPV),
    ([(PA, '0xffff')], IPV, [(GHI, 0)]),
    ([(PA, '0xffff'), (PH, '0.0')], IPV),
    ([(PA, '0xffff'), (PH, '-1.0')], IPV),
    ([], IPV),
    # IsPlayerInFluid (0x1690 count, 0x16A0 id, 0x1708 cast, 0x1720 z < max)
    ([], IPF),
    ([(CNT, '1'), (FL0, '0xffff')], IPF),
    ([(CNT, '1'), (FL0, PIRU)], IPF),
    ([(CNT, '1'), (FL0, '0x0fff')], IPF),
    # the water cast succeeds: u0443 / u043c are CScriptWater in this area (`objs Water`)
    ([(CNT, '1'), (FL0, '0x0443')], IPF),
    ([(CNT, '1'), (FL0, '0x043c')], IPF),
    # FindNearestTarget (0x12A0 self, 0x12AC ignore, 0x12E4 dist, 0x12F0 pirate, 0x1338 fluid)
    ([], FNT % INV),
    ([], FNT % '$P->mUniqueId'),
    ([], FNT % '((CSpacePirate*)$pir)->mUniqueId'),
    ([], FNT % INV, [(PIRVALID, 0)]),
    ([(CNT, '1'), (FL0, '0xffff')], FNT % INV),
    ([(CNT, '1'), (FL0, '0xffff')], FNT % INV, [(PIRVALID, 0)]),
    ([(CNT, '1'), (FL0, '0xffff')], FNT % '((CSpacePirate*)$pir)->mUniqueId'),
], 'direct calls with the forced scenario members')]
