# CommandoPirate (module 9) forcing spec, slice cp4: the constructor's (fn_9_D904) split-out
# helpers, the shield's CollidedWith (fn_9_BF44) outside a shield charge, and the
# AcceptScriptMsg (fn_9_CB40) arms the driver's messages cannot reach on their own. Every write
# lasts one call (tools/cov/force_ret.py); a '!' register write is not restored (the register is
# dead after the entry).
#
# Run (the coverage build; put the forced profiles first in a gcov-tool merge):
#   BUILD=build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh
#     tools/cov/specs/commandopirate_cp4.py <out> 3bfa3eff,05_temple_federationsite_dark --fast
#     --frames 9000 --driver 'python3 tools/drivers/commandopirate_cp4.py {port} <out>/drv.txt'
#   (and the same in 'swamp,05_swamp_timber!')
import os

CLASS = 'CCommandoPirate'
# CP4_MODE: 'col' forces only CollidedWith, 'msg' only AcceptScriptMsg (both: the ctor helpers);
# one breakpoint per contact and per message makes a run with both too slow under gdb.
MODE = os.environ.get('CP4_MODE', 'msg')
C63, C64, C65, C66 = '$this->xc63_', '$this->xc64_', '$this->xc65_', '$this->xc66_'
F0, F1 = '$this->mData.mFlags0', '$this->mData.mFlags1'
# AcceptScriptMsg(this, mgr, msg): the message at entry. A value is evaluated at the entry, so the
# message may pick it; an lvalue through $rdx/$rsi would be re-read at the return, so those writes
# are '!' (the console's message and the collision's id/list are the caller's temporaries).
MSG = '((CScriptMsg*)$rdx)->m_msg'


SHIELD, BLADE = '$this->mCollisionShield.mPtr', '$this->mCollisionBlade.mPtr'
NULLED = ['kSM_Deactivate', 'kSM_AIUpdateDisabled']
REPAIR = ('(!(%s == kSM_Deactivate || %s == kSM_AIUpdateDisabled) && !$_isvoid($cp4o) && '
          '$cp4o == (long)$this && %%s == 0) ? (CCollisionActorManager*)%%s : %%s' % (MSG, MSG))


def on(msgs, lv, val):
    test = ' || '.join('%s == %s' % (MSG, m) for m in msgs)
    return (lv, '(%s) ? (%s) : (%s)' % (test, val, lv.lstrip('!')))



def ent(label, fn, period, phases, what):
    return (label, 'CCommandoPirate::%s(' % fn, period, phases, what)


FORCE_RET = [
    # static, sret: the hidden result pointer is $rdi, the asset id $esi.
    ent('D904 LockEffect', 'xD904_LockEffect', 2, [(1, [('!$rsi', '0xffffffff')])],
        'the asset id kInvalidAssetId (the id tests in 0x0000DC24..0x0000E080)'),
    ent('D904 SetChargeTime', 'xD904_SetChargeTime', 2,
        [(1, [('!$xmm0.v4_float[0]', '0.0')])],
        'the back-up speed 0 (fcmpo/ble at 0x0000E660..0x0000E664 skips the store)'),
    # every other collision of the commando runs with the shield-charge bit: its walking contacts
    # give the floor, steep and wall-material arms of the list walk.
    # CollidedWith(this, &id, list, mgr), period 4, every phase with the +0xC65 0x04 bit
    # (0x0000BF74..0x0000BF7C): 1 the natural contacts; 2 the id the player's (the CPlayer cast
    # at 0x0000BFA4 succeeds, 0x0000BFA8 skips the damage); 3 the actor's up axis turned over (the
    # transform's m22 negated: every contact is steeper than 0.707, 0x0000C170..0x0000C174);
    # 0 the first contact's left materials without the floor/ceiling/wall bits (0x0000C1A8..C1C8
    # falls through).
    ent('BF44 CollidedWith', 'CollidedWith', 4,
        [(1, [(C65, C65 + ' | 0x04')]),
         (2, [(C65, C65 + ' | 0x04'),
              ('!*(unsigned short*)$rsi', '((CStateManager*)$rcx)->mPlayers[0]->mUniqueId.value')]),
         (3, [(C65, C65 + ' | 0x04'), ('$this->mTransform.m22', '-$this->mTransform.m22')]),
         (0, [(C65, C65 + ' | 0x04'),
              ('!((CCollisionInfo*)((CCollisionInfoList*)$rdx)->mList.mData)[0].mMaterialLeft.mValue',
               '((CCollisionInfo*)((CCollisionInfoList*)$rdx)->mList.mData)[0].mMaterialLeft.mValue'
               ' & ~0xE0000000ULL')])],
        'the +0xC65 0x04 bit outside a shield charge; the player id / the up axis turned over / '
        'no floor bit'),
    # AcceptScriptMsg, period 4. Phase 1: the two managers null for DCTV/XAUD (the null tests at
    # 0x0000CD90/0x0000CDA8 and 0x0000CDD0/0x0000CDE8); the block's +0x91D 0x80 bit for ACTV
    # (0x0000CD3C); +0xC65 0x04 for XDMG/XRDG (0x0000D0A0, 0x0000D168), +0xC66 0x40 for XDMG
    # (0x0000D0D0), +0xC64 0x10 for XDMG/XRDG (0x0000D150, 0x0000D248); the body state Hurled for
    # XLAU (0x0000CFAC); the XRDG sender the shield actor (0x0000D184). Phase 3: +0xC65 0x04 and
    # the shield actor's id kInvalidUniqueId for XRDG (the cast at 0x0000D1A4 fails); +0xC63 0x20
    # for ESCP/ZERO (0x0000CF3C, 0x0000CF08). Phases 1 and 3: the block's +0x91C 0x01 bit for an
    # XALD after the first (the waypoint +0xBCE is set: the driver's own late XALD; 0x0000CE04).
    ent('CB40 AcceptScriptMsg', 'AcceptScriptMsg', 4,
        [(1, [on(NULLED, SHIELD, '($cp4o = (long)$this, $cp4s = (long)%s, 0)' % SHIELD),
              on(NULLED, BLADE, '($cp4b = (long)%s, 0)' % BLADE),
              on(['kSM_Activate'], F1, F1 + ' | 0x80'),
              on(['kSM_Damage', 'kSM_ResistedDamage'], C65, C65 + ' | 0x04'),
              on(['kSM_Damage'], C66, C66 + ' | 0x40'),
              on(['kSM_Damage', 'kSM_ResistedDamage'], C64, C64 + ' | 0x10'),
              on(['kSM_Launching'], '$this->mBodyController.mPtr->mBodyStateInfo.mState',
                 '(pas::EAnimationState)14'),
              on(['kSM_ResistedDamage'], '!((CScriptMsg*)$rdx)->mSenderId.value',
                 '$this->xbc6_.value'),
              on(['kSM_AreaLoaded'], F0, '$this->xbce_.value != 0xffff ? %s | 0x01 : %s' % (F0, F0))]),
         (3, [on(['kSM_ResistedDamage'], C65, C65 + ' | 0x04'),
              on(['kSM_ResistedDamage'], '$this->xbc6_.value', '0xffff'),
              on(['kSM_Escape', 'kSM_SetToZero'], C63, C63 + ' | 0x20'),
              on(['kSM_AreaLoaded'], F0, '$this->xbce_.value != 0xffff ? %s | 0x01 : %s' % (F0, F0))])],
        'the message arms (see the comment)'),
    # A null manager whose restore was lost (gdb's "left without a return") is put back at the
    # commando's next message, before an XDEL's Destroy can see it.
    ent('CB40 repair', 'AcceptScriptMsg', 1, [(0, [('!' + SHIELD, REPAIR % (SHIELD, '$cp4s', SHIELD)),
                                                  ('!' + BLADE, REPAIR % (BLADE, '$cp4b', BLADE))])],
        'a lost restore of a forced null manager'),
]
FORCE_RET = [e for e in FORCE_RET
             if e[0].startswith('D904') or (MODE == 'col') == e[0].startswith('BF44')]

SPEC = []
