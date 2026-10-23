# ScriptPlayerProxy (module 62), slice sppx1: forced arms on Agon 05_portalpuzzle's
# EchoVisorWatcher e00120402 (the only proxy type single player ships near the start). Every run
# turns that one proxy into another proxyType (a persistent write at the AcceptScriptMsg entry of
# the message named below, so the dispatch of that message already sees the new type) and drives
# it with tools/cov/specs/spp_force_<run>.con. The run is chosen by SPP_RUN (A..G):
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py SPP_RUN=A tools/cov/force.sh
#     tools/cov/specs/scriptplayerproxy_force.py <out> 42b935e4,05_portalpuzzle
#     --con tools/cov/specs/spp_force_A.con --fast --frames 2000
# A PlayerFollower (3), B ArchenemyMarker (2), C EventRelay (0) plus out-of-range types,
# D CoinCollector (1), E DamageOverTime (10), F PersistentCounterReset (4), G the native echo
# watcher with the visor-mask, team-bit and other-visor arms.
import os

CLASS = 'CScriptPlayerProxy'
RUN = os.environ.get('SPP_RUN', 'A')
ME = '(($this->mEditorId.value & 0x3ffffff) == 0x120402)'
MSG = '((CScriptMsg*)$rdx)->m_msg'
AREALOADED, CREATE, RESET = '0x58414c44', '0x58435254', '0x52534554'
ACCEPT = 'CScriptPlayerProxy::AcceptScriptMsg('
THINK = 'CScriptPlayerProxy::Think('
GIA = 'CPlayerState::GetItemAmount('
FIRST = 'CPlayerListener::GetFirstPlayer('
MASK = '$this->mListener.mPlayerMask'
HP = 'mgr.mPlayerStates[0]->mHealth.mHealth'
DOT = '((CScriptPlayerProxy::SDamageOverTime*)&$this->mDamageOverTime.mData[0])'


def retype(msg, new):
    # proxyType := new on this proxy's first `msg`, while it is still the echo watcher (8)
    return ('retype %s' % new, ACCEPT, 1,
            [(0, [('!$this->mProxyType', '%s && %s == %s && $this->mProxyType == 8 ? %s : $this->mProxyType'
                   % (ME, MSG, msg, new))])], 'proxyType %s at %s' % (new, msg))


def visor_as(t):
    # SetVisorFromProxyType's switch sees type t on its one call (Create), put back at the return
    return ('1BDC visor type %s' % t, 'CScriptPlayerProxy::SetVisorFromProxyType(', 1,
            [(0, [('$this->mProxyType', '%s ? %s : $this->mProxyType' % (ME, t))])], 'type %s' % t)


FORCE_RET, FORCE_INNER = [], []
if RUN == 'A':
    FORCE_RET = [
        retype(AREALOADED, 3), visor_as(5),
        # UpdateTransform (fn_62_1448): follower spin (vector nonzero), offset-only (zero), invalid index
        ('1448 transform', 'CScriptPlayerProxy::UpdateTransform(', 3,
         [(0, [('$this->mVectorParameter1.mZ', '30.0'), ('$this->mVectorParameter1.mY', '10.0')]),
          (1, [('$this->mVectorParameter1.mX', '0.0'), ('$this->mVectorParameter1.mY', '0.0'),
               ('$this->mVectorParameter1.mZ', '0.0')]),
          (2, [('$rdx', '0xffffffff')])], 'vector nonzero | zero | index invalid'),
        ('1C44 time wrap', THINK, 500, [(250, [('!$this->mTime', '%s ? 899.99 : $this->mTime' % ME)])],
         'mTime 899.99 once (the 900 wrap)'),
    ]
    FORCE_INNER = [
        ('131C invisible', 'CScriptPlayerProxy::AddToRenderer(', GIA, 2, {1: [('$rax', '1')]},
         'Invisibility 1 on every second render'),
    ]
elif RUN == 'B':
    FORCE_RET = [
        retype(CREATE, 2),
        ('16B4 no first player', 'CScriptPlayerProxy::SetArchenemyIndex(', 2, [(0, [(MASK, '0')])], 'mask 0'),
        ('1718 no first player', 'CScriptPlayerProxy::GetArchenemyIndex(', 7, [(3, [(MASK, '0')])], 'mask 0'),
        ('131C other render player', 'CScriptPlayerProxy::AddToRenderer(', 3,
         # persistent (a restore at the return cannot name mgr): 1 on every third render, else 0
         [(1, [('!mgr.mCurrentRenderPlayerIndex', '1')]), (2, [('!mgr.mCurrentRenderPlayerIndex', '0')])],
         'render index 1'),
    ]
    FORCE_INNER = [
        # v21 (multiplayer archenemy mode) true on the first Create only: a later `send Create` deletes
        ('23D4 archenemy kept', ACCEPT, 'CGMSinglePlayer::v21(', 1000, {1: [('$rax', '1')]}, 'v21 true once'),
        ('222C index invalid', 'CScriptPlayerProxy::HandleArchenemyMessage(',
         'CScriptPlayerProxy::GetArchenemyIndex(', 1000, {2: [('$rax', '0xffffffff')], 3: [('$rax', '0xffffffff')]},
         'archenemy index invalid on the 2nd and 3rd Action'),
        ('222C pick 1', 'CScriptPlayerProxy::HandleArchenemyMessage(', 'CRandom16::Range(int', 2,
         {0: [('$rax', '1')]}, 'Range answers 1 (not the first player) on every second pick'),
    ]
elif RUN == 'C':
    FORCE_RET = [
        retype(CREATE, 0),
        ('23D4 type range', ACCEPT, 1,
         [(0, [('$this->mProxyType',
                '%s && %s == 0x41435456 ? 11 : (%s && %s == %s ? -1 : $this->mProxyType)'
                % (ME, MSG, ME, MSG, RESET))])], 'type 11 on Activate, -1 on Reset'),
    ]
elif RUN == 'D':
    FORCE_RET = [
        retype(AREALOADED, 1), visor_as(1),
        ('1C44 coins', ACCEPT, 1,
         [(0, [('!$this->mCoinsToDrop',
                '%s && %s == %s ? 1500 : (%s && %s == %s ? 7 : $this->mCoinsToDrop)'
                % (ME, MSG, AREALOADED, ME, MSG, RESET)),
               ('!$this->mPendingCoins', '%s && %s == %s ? 5 : $this->mPendingCoins' % (ME, MSG, AREALOADED))])],
         'coins to drop 1500 at AreaLoaded, 7 at Reset; 5 pending'),
        # persistent (a restore at the return cannot name mgr): HP 0, then 99 once the first Thinks pass
        ('1C44 dead player', THINK, 1,
         [(0, [('!' + HP, '%s && $this->mTime < 0.2 ? 0.0 : (%s && %s == 0.0 ? 99.0 : %s)' % (ME, ME, HP, HP))])],
         'HP 0 for the first Thinks (pending coins wait), then 99'),
        # 'COIN' mode needs an invalid first player (SetCanRespawn would read CGMCoin's player vector):
        # mask 0 only once nothing is left to drop or hand out
        ('1C44 no player when idle', THINK, 1,
         [(0, [(MASK, '%s && $this->mCoinsToDrop == 0 && $this->mPendingCoins == 0 ? 0 : %s' % (ME, MASK))])],
         'mask 0 while idle'),
    ]
    FORCE_INNER = [
        ('1C44 COIN mode', THINK, 'CGMSinglePlayer::GetGameModeType(', 1,
         {0: [('$rax', '%s == 0 ? 0x434f494e : $rax' % MASK)]}, "'COIN' while the mask is 0"),
        ('190C coins held', 'CScriptPlayerProxy::DropCoins(', GIA, 3, {1: [('$rax', '500')], 2: [('$rax', '500')]},
         'coin counter 500 on two of three drops'),
        ('1C44 amplifier', THINK, GIA, 1,
         {0: [('$rax', '%s && $this->mCoinsToDrop < 700 && $this->mCoinsToDrop > 300 && $rax == 0 ? 1 : $rax' % ME)]},
         'amplifier (and counter) 1 while 300 < coins to drop < 700'),
    ]
elif RUN == 'E':
    FORCE_RET = [
        retype(AREALOADED, 10), visor_as(6),
        ('EA4 entries', 'CScriptPlayerProxy::UpdateDamageOverTime(', 1000,
         [(p, [('!$this->mDamageOverTime.mCount', '%s ? 2 : $this->mDamageOverTime.mCount' % ME),
               ('!%s[0].mTimeRemaining' % DOT, '1.0'), ('!%s[0].mDamagePerSecond' % DOT, '5.0'),
               ('!%s[0].mAttacker.value' % DOT, '0xffff'),
               ('!%s[1].mTimeRemaining' % DOT, '0.0'), ('!%s[1].mDamagePerSecond' % DOT, '5.0'),
               ('!%s[1].mAttacker.value' % DOT, '0xffff')]) for p in (30, 100)] +
         [(400, [('!$this->mDamageOverTime.mCount', '%s ? 0 : $this->mDamageOverTime.mCount' % ME)])],
         'two entries (live, spent) at calls 30 and 100; none at 400'),
        ('EA4 no first player', 'CScriptPlayerProxy::UpdateDamageOverTime(', 1000,
         [(p, [(MASK, '0')]) for p in (10, 500)], 'mask 0 on calls 10 and 500'),
    ]
    FORCE_INNER = [
        ('EA4 list emptied', 'CScriptPlayerProxy::UpdateDamageOverTime(', 'CStateManager::ApplyDamage(', 1000,
         {20: [('$this->mDamageOverTime.mCount', '0')]}, 'the list empties during the 20th hit'),
    ]
elif RUN == 'F':
    FORCE_RET = [retype(AREALOADED, 4)]
elif RUN == 'G':
    FORCE_RET = [
        visor_as(7),
        ('1778 not in mask', 'CScriptPlayerProxy::CheckVisors(', 2, [(0, [(MASK, '%s ? 0 : %s' % (ME, MASK))])],
         'mask 0 on every second call'),
        # 5th call only (f780, combat): calls 1-4 keep the mask, f420/f660 run with the dark visor on
        ('1868 not in mask', 'CScriptPlayerProxy::InitVisors(', 5, [(0, [(MASK, '%s ? 0 : %s' % (ME, MASK))])],
         'mask 0 on the fifth call'),
        # BuildPlayerMask is inlined into the ctor: its mask is the first stack argument
        ('2C58 team bit', 'CScriptPlayerProxy::CScriptPlayerProxy(', 2,
         [(1, [('!*(unsigned*)($rsp+8)', '*(unsigned*)($rsp+8) | 0x10')])], 'mask bit 4 (team 0) on odd ctors'),
    ]

SPEC = [
    ('Think', THINK, [], 'max=400'),
    ('AcceptScriptMsg', ACCEPT, [MSG], 'max=400'),
]
