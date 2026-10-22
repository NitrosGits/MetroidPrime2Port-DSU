# CommandoPirate (module 9), slice cp8: the shield's sound/scale event helpers (fn_9_1F88 on
# kUE_Activate, fn_9_1EF8 on kUE_Deactivate and from the charge/ArmShield states) and the two sound
# emitters' update (fn_9_4BC0, from the Think), driven by tools/drivers/commandopirate_cp8.py in
# the Swamp (05_swamp_timber!).
#   COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh tools/cov/specs/commandopirate_cp8.py
#     <out> 'swamp,05_swamp_timber!' --fast --frames 6000
#     --driver 'python3 tools/drivers/commandopirate_cp8.py {port} <out>/drv.txt'
# Forced (each write lasts one call, put back at the return; the bodies do not write 0xC64): the
# 0x01 bit of guest this+0xC64 on every third call of 1F88 and 1EF8 (their early return; no
# shipped animation event comes while the bit is set in a short run).
CLASS = 'CCommandoPirate'
L = 'loc:PortCCommandoPirate.cpp:'
H = lambda m: '(*(unsigned*)&$this->%s != 0)' % m
FORCE_RET = [
    ('1F88 c64 0x01', 'CCommandoPirate::x1F88_EventStart(', 3, [(1, [('$this->xc64_', '$this->xc64_ | 1')])],
     'the 0x01 bit of 0xC64 on calls 1 mod 3'),
    ('1EF8 c64 0x01', 'CCommandoPirate::x1EF8_Activate(', 3, [(1, [('$this->xc64_', '$this->xc64_ | 1')])],
     'the 0x01 bit of 0xC64 on calls 1 mod 3'),
]
SPEC = [
    ('1F88 entry: c64&1, c38 set, c65&0xc', 'CCommandoPirate::x1F88_EventStart(',
     ['$this->xc64_ & 1', H('xc38_'), '$this->xc65_ & 0xc'], 'max=400'),
    ('1EF8 entry: c64&1, c38 set', 'CCommandoPirate::x1EF8_Activate(',
     ['$this->xc64_ & 1', H('xc38_')], 'max=400'),
    ('4BC0 entry: c04 set, c38 set', 'CCommandoPirate::x4BC0_UpdateEmitters(',
     [H('xc04_'), H('xc38_')], 'calls max=100000'),
    ('4BC0 0x4C40 update c04', L + '613', [], 'max=100000'),
    ('4BC0 0x4C48 clear c04', L + '616', [], 'max=100000'),
    ('4BC0 0x4C8C update c38', L + '620', [], 'max=100000'),
]
# The loader's sound defaults (fn_9_E30's -1 ids, xE30_SoundDefaults): what each commando's block
# holds once loaded (0xffff = the default or the data's -1). Plain run, no driver needed:
#   tools/cov/run.sh tools/cov/specs/commandopirate_cp8.py <out> 'swamp,05_swamp_timber!' --fast --frames 1500
SPEC.append(('E30 sounds: impact hurled death projectile bounce explode shieldexplode on off',
             'CCommandoPirate::x4BC0_UpdateEmitters(',
             ['$this->mData.mSoundImpact', '$this->mData.mSoundHurled', '$this->mData.mSoundDeath',
              '$this->mData.mSoundProjectile', '$this->mData.mGrenade.mSoundGrenadeBounce',
              '$this->mData.mGrenade.mSoundGrenadeExplode', '$this->mData.mShield.mSoundExplode',
              '$this->mData.mShield.mSoundTurnOn', '$this->mData.mShield.mSoundTurnOff'], 'max=20'))
