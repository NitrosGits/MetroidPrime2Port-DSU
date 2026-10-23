# ds47: the arms of the CDarkSamus constructor (fn_10_1FA14) that the shipped data does not take.
# Used as both the spec and the gdb script of tools/cov/force.sh: at the constructor's entry it
# logs the loader's data (the parameter `data`) and, with DS47_FORCE=1, writes every sound id -1, every optional asset invalid
# and startsInTheAir 1; at the return it logs what the constructor built from them and writes the
# original values back into mData (the members built from the forced values stay as built, so
# keep the run short: the actor is never activated). DS47_FORCE=2 instead gives the two sweep beam
# sounds valid ids and the alternate scannable info the actor's own SCAN. DS47_FORCE=0 only logs.
#   COV_GDB_SCRIPT=$PWD/tools/cov/specs/darksamus_ds47_ctor.py BUILD=$PWD/build-port-cov \
#     DS47_FORCE=1 MP2_INPUT=LAYER2@400 tools/cov/force.sh tools/cov/specs/darksamus_ds47_ctor.py \
#     <out> "1baa96c2,10_cliff_rooftop" --fast --frames 900
import os
import gdb

CLASS = 'CDarkSamus'
FORCE = []
OUT = os.environ.get('COV_OUT', '/tmp')
LOG = os.path.join(OUT, 'forced.log')
FORCE_MODE = os.environ.get('DS47_FORCE', '0')
DO_FORCE = FORCE_MODE == '1'

SOUNDS = ['mGlideSound', 'mMissileRicochetSound', 'mBoostBallRollSound', 'mBoostBallHitPlayerSound',
          'mSweepBeamSound', 'mUnknown0EF8DC15']
ASSETS = ['mScatterShotProjectile', 'mScatterShotProjectile2', 'mMeleeAttackFX', 'mPart',
          'mNormalMissileProjectile', 'mSuperMissileProjectile', 'mFreezeBeamProjectile', 'mTXTR',
          'mPhazonProjectile', 'mPhazonSuperMissileProjectile', 'mAlternateScannableInfo']
BUILT = ['mFlags90C', 'mGlideSfxId', 'mRicochetSfxId', 'mBoostBallRollSfxId',
         'mBoostBallHitPlayerSfxId', 'mSweepBeamSfxId', 'mSweepBeamEndSfxId', 'mSavedSpeed',
         'mContactTime', 'mLastWallHitTime', 'mDamageDecay', 'mDamageThreshold', 'mGuest1058',
         'mHitDirection.mX', 'mHitDirection.mY', 'mHitDirection.mZ', 'mHitDirectionFlat.mX',
         'mHitDirectionFlat.mY', 'mHitDirectionFlat.mZ', 'mAltScanInfo.mPtr',
         'mScatterShotInfo.m_valid', 'mScatterShotInfo2.m_valid', 'mMeleeAttackFXToken.m_valid',
         'mMeleePartToken.m_valid', 'mMissileInfo.m_valid', 'mSuperMissileInfo.m_valid',
         'mFreezeBeamToken.m_valid', 'mFlashTexture.m_valid', 'mPhazonShotInfo.m_valid',
         'mPhazonSuperMissileInfo.m_valid']


def log(s):
    with open(LOG, 'a') as f:
        f.write(s + '\n')


def ev(expr):
    try:
        return str(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return '<%s>' % e


syms = []
for ln in open(os.environ['COV_NM']):
    parts = ln.rstrip('\n').split('\t')
    if len(parts) == 2:
        f = parts[0].split()
        if len(f) == 3 and f[1] in 'tTW' and parts[1].startswith('CDarkSamus::CDarkSamus('):
            syms.append(f[0])


class Done(gdb.FinishBreakpoint):
    def __init__(self, frame, this, saved):
        super().__init__(frame, internal=True)
        self.this, self.saved = this, saved

    def stop(self):
        t = '((CDarkSamus*)%d)' % self.this
        log('built: ' + ', '.join('%s=%s' % (m, ev('%s->%s' % (t, m))) for m in BUILT))
        for m, v in self.saved:
            gdb.execute('set var %s->mData.%s = %s' % (t, m, v))
        log('mData restored (%d fields)' % len(self.saved))
        return False

    def out_of_scope(self):
        log('ctor frame out of scope')


class Entry(gdb.Breakpoint):
    def stop(self):
        this = int(gdb.parse_and_eval('(long)this'))
        d = '((CDarkSamusData*)%d)' % int(gdb.parse_and_eval('(long)&data'))
        log('ctor this=%#x force=%d threshold=%s f317=%s starts=%s' % (
            this, DO_FORCE, ev(d + '->mDamageInterruptThreshold'), ev(d + '->mUnknownF317F4D5'),
            ev('(int)' + d + '->mStartsInTheAir')))
        log('data: ' + ', '.join('%s=%s' % (m, ev('(int)%s->%s' % (d, m))) for m in SOUNDS + ASSETS))
        saved = []
        if DO_FORCE:
            for m in SOUNDS + ASSETS:
                saved.append((m, ev('(int)%s->%s' % (d, m))))
                gdb.execute('set var %s->%s = -1' % (d, m))
            saved.append(('mStartsInTheAir', ev('(int)%s->mStartsInTheAir' % d)))
            gdb.execute('set var %s->mStartsInTheAir = 1' % d)
            log('forced: sounds -1, assets invalid, startsInTheAir 1')
        elif FORCE_MODE == '2':
            # The arms the shipped data leaves -1: the two sweep beam sounds and the
            # alternate scannable info (the actor's own SCAN id, a real asset).
            scan = int(gdb.parse_and_eval('(int)actParms.mScannable.mScanId'))
            for m, v in (('mSweepBeamSound', 8076), ('mUnknown0EF8DC15', 303),
                         ('mAlternateScannableInfo', scan)):
                saved.append((m, ev('(int)%s->%s' % (d, m))))
                gdb.execute('set var %s->%s = %d' % (d, m, v))
            log('forced: sweep sounds 8076/303, alternate scan %#x' % (scan & 0xffffffff))
        Done(gdb.newest_frame(), this, saved)
        return False


# By name (the binary is PIE: nm's addresses are not the run's); gdb puts a location on the
# complete and the base constructor, after their prologue, where `this` and `data` are named.
Entry('CDarkSamus::CDarkSamus', internal=True)
log('ctor symbols: %d' % len(set(syms)))
