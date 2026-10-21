# tools/cov/force.sh spec for the arms no fight reaches (the core driver with `msgs`): the SPEC of
# specs/commandopirate.py with its own FORCE. The writes alternate on the timer xb60_ (fractions of a
# second), so each body sees both sides of the forced test in one run.
import os
exec(open(os.path.join(os.path.dirname(os.environ['COV_SPEC']), 'commandopirate.py')).read())

T2 = '((int)($this->xb60_ * 2) & 1)'
K5 = '(((int)($this->xb60_ * 10)) % 5)'
# per-body hit counters for the rarely called bodies (gdb convenience variables)
def CNT(n):
    return '$%s = $_isvoid($%s) ? 1 : $%s + 1' % (n, n, n)
FORCE = [
    ('662C hit/counter', 'CCommandoPirate::x662C_UpdateTimers(', ['$this->mHitByPlayerProjectile'],
     ['$this->mHitByPlayerProjectile = ' + T2, '$this->xc50_ = %s ? 1000.0 : $this->xc50_' % T2], 600,
     'mHitByPlayerProjectile = 1 and xc50_ = 1000 (above the shield maximum) on alternate half seconds from the 601st call'),
    ('662C shield data', 'CCommandoPirate::x662C_UpdateTimers(', [],
     ['$this->mData.mShield.mUnknownC3938663 = 0.0', '$this->mData.mShield.mUnknown6CB0DA5A = 0.0',
      '$this->xc64_ = $this->xc64_ & 0x7f'], 6000,
     'the shield block\'s +0xC3938663/+0x6CB0DA5A = 0 and xc64_ loses 0x80 from the 6001st call'),
    ('F58C invalid id', 'CCommandoPirate::xF58C_CreateEffect(', ['$rdi == 0xffffffff'], ['$rdi = 0xffffffff'], 0,
     'id = kInvalidAssetId on every call (both shield effects null: C598 and 1C0C take their null arms)'),
    ('C798 event type', 'CCommandoPirate::DoUserAnimEvent(', ['$rcx'],
     [CNT('c798'), '$rcx = ($c798 %% 2) ? $rcx : (%s == 0) ? 13 : (%s == 1) ? 22 : (%s == 2) ? 29 : (%s == 3) ? 30 : 11' % (K5, K5, K5, K5),
      '$this->xc65_ = ($c798 % 2) ? $this->xc65_ : ($this->xc65_ & 0xf3) | ((((int)($this->xb60_ * 7)) & 3) << 2)'], 20,
     'on every other call: type = TakeOff/BreakLockOn/Activate/Deactivate/DeGenerate and xc65_ bits 0x04/0x08 by the timer from the 21st call'),
    ('1C0C state/scale', 'CCommandoPirate::x1C0C_UpdateRenderables(', ['$this->xc4c_'],
     [CNT('c1c0c'), '$this->xc4c_ = (int)$this->xb60_ & 1',
      '$this->xc66_ = (%s || $c1c0c %% 8 == 1) ? ($this->xc66_ | 0x20) : ($this->xc66_ & 0xdf)' % T2,
      '$this->mBodyController.mPtr->mFrozen = ($c1c0c % 8 == 1)',
      '$xmm0.v4_float[0] = ($c1c0c % 8 == 5) ? 0.6 : $xmm0.v4_float[0]',
      '$this->xc48_ = ((int)($this->xb60_ * 4) & 1) ? 1.0 : 0.0',
      '$this->xc44_ = ((int)($this->xb60_ * 4) & 2) ? 0.05 : $this->xc44_'], 600,
     'xc4c_ = 0/1, xc66_ 0x20, xc48_ = 0/1 and xc44_ = 0.05 by the timer, the body frozen (one call) and dt = 0.6 on calls 1/5 mod 8, from the 601st call'),
    ('6DBC Cover alt', 'CCommandoPirate::Cover(', ['$rdx'], ['$this->xbd2_.value = %s ? 0x5c8 : 0xffff' % T2], 2,
     'xbd2_ = u05c8 / kInvalidUniqueId on alternate half seconds from the 3rd call'),
    ('6C68 CoverAttack alt', 'CCommandoPirate::CoverAttack(', ['$rdx'], ['$this->xbd2_.value = %s ? 0x5c8 : 0xffff' % T2], 2,
     'xbd2_ = u05c8 / kInvalidUniqueId on alternate half seconds from the 3rd call'),
    ('3480 no target', 'CCommandoPirate::x3480_PickMeleeAttack(', ['$this->xbca_.value != 0xffff'],
     [CNT('c3480'), '$this->xbca_.value = ($c3480 % 3 == 0) ? 0xffff : $this->xbca_.value'], 0,
     'xbca_ = kInvalidUniqueId (no target: fn_9_3CBC fails here and in MeleeAttack) on every 3rd call'),
    ('5410 no target', 'CCommandoPirate::x5410_CreateEGrenade(', ['$this->xbca_.value != 0xffff'],
     [CNT('c5410'), '$this->xbca_.value = ($c5410 % 3 == 1) ? 0xffff : $this->xbca_.value'], 0,
     'xbca_ = kInvalidUniqueId (no target) on calls 1, 4, 7...'),
    ('52BC not a player', 'CCommandoPirate::x52BC_GetGrenadeTarget(', [],
     [CNT('c52bc'), '$rdx = ($c52bc % 2 == 1) ? $rdi : $rdx'], 0,
     'the target = the commando itself (not a CPlayer) on the odd calls'),
    ('5008 geometry', 'CCommandoPirate::x5008_SolveGrenadeLaunch(', [],
     [CNT('c5008'),
      '((float*)$rsi)[2] = ($c5008 % 4 == 1) ? ((float*)$rdx)[2] - 5.0 : ($c5008 % 4 == 2) ? ((float*)$rdx)[2] + 200.0 : ((float*)$rsi)[2]',
      '((float*)$rsi)[0] = ($c5008 % 4 == 3) ? ((float*)$rdx)[0] : ((float*)$rsi)[0]',
      '((float*)$rsi)[1] = ($c5008 % 4 == 3) ? ((float*)$rdx)[1] : ((float*)$rsi)[1]'], 0,
     'the aim point (5410\'s own vector) 5 below / 200 above / straight over the origin on calls 1/2/3 mod 4'),
    ('F0F8 no EMP', '(anonymous namespace)::CCommandoPirateEGrenade::Think(', [],
     [CNT('cf0f8'), "(('(anonymous namespace)::CCommandoPirateEGrenade'*)$rdi)->mData.mEMPDuration = ($cf0f8 % 2) ? (('(anonymous namespace)::CCommandoPirateEGrenade'*)$rdi)->mData.mEMPDuration : 0.0"], 0,
     'the grenade\'s EMP duration = 0 on every 2nd call'),
    ('CB40 flags', 'CCommandoPirate::AcceptScriptMsg(', [],
     ['$this->mData.mFlags1 = $this->mData.mFlags1 ^ 0x80', '$this->mData.mFlags0 = $this->mData.mFlags0 ^ 0x01',
      '$this->xc66_ = $this->xc66_ ^ 0x40', '$this->xc64_ = $this->xc64_ ^ 0x10'], 2000,
     'the data flags +0x91d 0x80 / +0x91c 0x01 and xc66_ 0x40 / xc64_ 0x10 flip on every call from the 2001st'),
]
