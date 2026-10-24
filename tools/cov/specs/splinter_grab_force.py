# Splinter (REL 74) spl2 lane, forced: the arms that need mSac.mGrabbedId, which only FindConnectedId
# writes (a mSacOnCable splinter with a kSS_Connect connection) and no spawn has one.  The force writes
# mGrabbedId (and mHoldingSac) at the entry of Rise / GrabSac / ExplodeSac, so IsGrabbingPlayer,
# DetachMorphBall, ExplodeSac and the GrabSac / Rise holding arms run.  MBallMoved's
# inactive-player and other-area arms are forced at its entry (SPL_MB); SPL_TOP=1 makes ReachedTop true (the Rise arm that skips ExplodeSac), SPL_TOP=low false (Rise calls ExplodeSac).
# Ids are the hive_hall (3bfa3eff,0e_temple_hive_hall, layers 3 4) values, `value` not Value():
# player u0400, the splinter u048a is a live actor, 0x7f0 is no object.
#   SPL_GRAB=0x400 player | 0x48a another actor | 0x7f0 nothing ; SPL_HOLD=1 sets mHoldingSac
#   SPL_MB=inactive (player mActive=0) | area (the splinter's area id made different)
#   COV_GDB_SCRIPT=$PWD/tools/cov/force.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/splinter_grab_force.py <out> 3bfa3eff,0e_temple_hive_hall \
#     --con tools/cov/specs/splinter_grab.con --fast --frames 1300
import os
CLASS = 'CSplinter'
_G = int(os.environ.get('SPL_GRAB', '0x400'), 0)
_H = int(os.environ.get('SPL_HOLD', '1'), 0)
_MB = os.environ.get('SPL_MB', '')
_TOP = os.environ.get('SPL_TOP', '')
_SETS = ['$this->mSac.mGrabbedId.value = %d' % _G, '$this->mSac.mHoldingSac = %d' % _H]
_CARRY = os.environ.get('SPL_CARRY', '')  # SPL_CARRY=0|1 forces mCarriesSac (0: IsSacMissing's grabbed-null arm)
if _CARRY:
    _SETS.append('$this->mSac.mCarriesSac = %s' % _CARRY)
if _TOP:  # SPL_TOP=1: the splinter has not left the top (ReachedTop true: z > descendStartZ - 0.1)
    _SETS.append('$this->mSac.mDescendStartZ = %d' % (100000 if _TOP == 'low' else -100000))
FORCE = []
if os.environ.get('SPL_FGINV'):  # SPL_FGINV=1 (with SPL_GRAB=0x48a): FindGrabbed's mGrabbedId == invalid arm, forced at its entry
    FORCE += [('FindGrabbed invalid', 'CSplinter::FindGrabbed(', [], ['$this->mSac.mGrabbedId.value = 0xffff'], 0, 'mGrabbedId = kInvalidUniqueId')]
if not _MB:
    FORCE += [
        ('Rise grabbed', 'CSplinter::Rise(', ['(int)$rdx'], _SETS, 0, 'mGrabbedId=%#x holding=%d' % (_G, _H)),
        ('GrabSac grabbed', 'CSplinter::GrabSac(', ['(int)$rdx'], _SETS, 0, 'mGrabbedId=%#x holding=%d' % (_G, _H)),
        ('ExplodeSac grabbed', 'CSplinter::ExplodeSac(', ['(int)$this->mParticle.mEffect.m_valid'], _SETS, 0, 'mGrabbedId=%#x holding=%d' % (_G, _H)),
        # the player is attached to this splinter: DetachMorphBall's DetachActorFromPlayer arm
        ('DetachMorphBall attached', 'CSplinter::DetachMorphBall(', [],
         ['((CStateManager*)$rsi)->mPlayers[0]->mAttachedActor.value = $this->mUniqueId.value'], 0,
         'the player is attached to the splinter'),
    ]
elif _MB == 'inactive':
    FORCE += [('MBallMoved inactive', 'CSplinter::MBallMoved(', [],
               ['((CStateManager*)$rsi)->mPlayers[0]->mActive = 0'], 0, 'the player is inactive')]
elif _MB == 'same':  # the morph ball has not moved since the sac was set: Magnitude() > 0.1f false
    FORCE += [('MBallMoved same pos', 'CSplinter::MBallMoved(', [],
               ['$this->mSac.mMorphBallPos.mX = ((CStateManager*)$rsi)->mPlayers[0]->mTransform.m03', '$this->mSac.mMorphBallPos.mY = ((CStateManager*)$rsi)->mPlayers[0]->mTransform.m13', '$this->mSac.mMorphBallPos.mZ = ((CStateManager*)$rsi)->mPlayers[0]->mTransform.m23'], 0,
               'the morph ball has not moved')]
elif _MB == 'area':
    FORCE += [('MBallMoved other area', 'CSplinter::MBallMoved(', [],
               ['$this->mAreaId.value = ((CStateManager*)$rsi)->mPlayers[0]->mAreaId.value + 1'], 0,
               'the player is in another area'),
              # undo it at the next Think (a splinter in another area crashes PreRender)
              ('MBallMoved area restore', 'CSplinter::Think(', [],
               ['$this->mAreaId.value = $this->mAreaId.value == ((CStateManager*)$rsi)->mPlayers[0]->mAreaId.value + 1 ? '
                '((CStateManager*)$rsi)->mPlayers[0]->mAreaId.value : $this->mAreaId.value'], 0, 'restores the area')]
SPEC = [
    ('HasUnpoppedSac', 'CSplinter::HasUnpoppedSac(', [], 'ret'),
    ('GetSac', 'CSplinter::GetSac(', [], 'retp'),
    ('CreateSac', 'CSplinter::CreateSac(', ['$this->mSac.mSacAncs', '$this->mSac.mSacId.value'], ''),
    ('PopSac', 'CSplinter::PopSac(', [], ''),
    ('Activate', 'CSplinter::AcceptScriptMsg(', ['(int)$rdx', '$this->mSac.mSacOnCable'], ''),
]
