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
# watcher with the visor-mask, team-bit and other-visor arms. Slice ppx1: H = M06_ShootingGallery natural PLCTs
# + `gameevent` (spp_m06.con), I = D with a fake CGMCoin for Think's SetCanRespawn arm (spp_force_I.con).
# Launch 1 of ppx1 EXECUTED run H (spp_m06.con, tools/cov/force_ret.py, build-port-cov binary sha256 9b8ae971...ed834) and hung
# at frame 232 (the persistent per-Think `1C44 hp back` stop on 21 proxies). The three files were last written 01:54:22 before
# the run's forced.log (01:54:58) and have not changed since; their sha256 at commit 5a89fc961 (= the executed bytes by mtime,
# no copy was taken then): this spec 6ea20c1a...cecde2d2, spp_m06.con 919c7728...271cb, spp_force_I.con (not executed) 0099eda7...cfcd.
# Slice ppx3, launch 1: run M (spp_m06_M.con) = run L plus the Think coin/wrap/visor arms, follower on/off, inactive proxy;
# it crashed at Think call 100 (entry order, see the M block). Launch 2 runs M with its entries before the COIN entry.
# Launch 3: run N (same con) = M plus OnGameEvent's coin/relay arms launch 2 missed (the N block).
# Launch 4 (ppx2, run L): restore through the address, COIN forced only while the entry installed a force (launch 3 = run K crashed: IDLE turned true mid-call).
# Launch 3 (slice ppx2) runs J again with the fake vtable word fixed to `**(long**)GM` (launch 2 crashed at pc 0 writing GM there).
# Launch 2 runs J (spp_m06_J.con, tools/cov/force_ret_spp.py): M06 with the Think forces restricted by ONLY to the CoinCollector
# e080000a8 and no persistent writes; the fixture and its sha256 are copied into the run dir.
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
elif RUN == 'H':
    # M06_ShootingGallery layers 0/2/3 (natural PLCTs: CoinCollector, PlayerFollower with model, relays, DOT);
    # `gameevent` drives OnGameEvent on them. The coin arm reads CGMCoin::GetCoinLimit/IsNearTimeLimit through a
    # CGMSinglePlayer (harmless reads); both answers and the overkill test are forced per call.
    OG = 'CScriptPlayerProxy::OnGameEvent('
    FORCE_RET = [
        ('2644 overkill', OG, 2,
         [(0, [('!' + HP, '-50.0'), ('$this->mFloatParameter2', '10.0')]),
          (1, [('!' + HP, '99.0'), ('$this->mFloatParameter2', '0.0')])],
         'HP -50 and overkill threshold 10 on odd calls; HP 99 and threshold 0 on even calls'),
        ('1C44 hp back', THINK, 1, [(0, [('!' + HP, '%s < 0.0 ? 99.0 : %s' % (HP, HP))])], 'HP back to 99'),
    ]
    FORCE_INNER = [
        ('2644 coin limit', OG, 'CGMCoin::GetCoinLimit(', 1,
         {0: [('$rax', '$this->mCoinsToDrop % 2 == 0 ? 0 : 100')]}, 'limit 0 when mCoinsToDrop even, else 100'),
        ('2644 near time limit', OG, 'CGMCoin::IsNearTimeLimit(', 2, {0: [('$rax', '1')], 1: [('$rax', '0')]},
         'near the time limit on alternate calls'),
        ('2644 counter', OG, GIA, 2, {0: [('$rax', '500')], 1: [('$rax', '0')]},
         'item amounts 500 / 0 on alternate calls'),
    ]
elif RUN in ('J', 'M', 'N'):
    # M06_ShootingGallery natural PLCTs. Think's COIN arm on the natural CoinCollector e080000a8 (the only object the
    # ONLY filter lets the Think entries touch: idle, mCoinsToDrop 0): even calls mask 0 (invalid first player),
    # odd calls a fake CGMCoin (built in the proxy's spare bytes at +0x230, vtable of the real mode, restored at the
    # return) so SetCanRespawn's inline write lands in scratch. OnGameEvent forces (|all) are per call, restored.
    ONLY = '(($this->mEditorId.value & 0x3ffffff) == 0xa8)'
    OG = 'CScriptPlayerProxy::OnGameEvent('
    MH = '$mgr->mPlayerStates[0]->mHealth.mHealth'
    IDLE = '($this->mCoinsToDrop == 0 && $this->mPendingCoins == 0 && $this->mProxyType == 1)'
    FAKE = '((char*)$this + 0x230)'
    GM = '*(long*)((char*)gpGameState + 0x260)'
    VT = '**(long**)((char*)gpGameState + 0x260)'  # the real mode's vtable word (run J's first try wrote GM, the mode pointer itself)
    FORCE_RET = [
        ('2644 overkill|all', OG, 2,
         [(0, [(MH, '-50.0'), ('$this->mFloatParameter2', '10.0')]),
          (1, [(MH, '99.0'), ('$this->mFloatParameter2', '0.0')])],
         'HP -50 and overkill threshold 10 on odd calls; HP 99 and threshold 0 on even calls (restored at the return)'),
        ('1C44 COIN arm', THINK, 2,
         [(0, [(MASK, '%s ? 0 : %s' % (IDLE, MASK))]),
          (1, [('*(long*)%s' % FAKE, '%s ? %s : *(long*)%s' % (IDLE, VT, FAKE)),
               ('*(long*)(%s + 0x68)' % FAKE, '%s ? (long)(%s + 0x10) : *(long*)(%s + 0x68)' % (IDLE, FAKE, FAKE)),
               ('*(char*)(%s + 0x10)' % FAKE, '*(char*)(%s + 0x10)' % FAKE),
               (GM, '%s ? (long)%s : %s' % (IDLE, FAKE, GM))])],
         'idle: mask 0 (even calls) | fake COIN mode with a valid first player (odd calls)'),
    ]
    FORCE_INNER = [
        ('2644 coin limit|all', OG, 'CGMCoin::GetCoinLimit(', 1,
         {0: [('$rax', '$this->mCoinsToDrop % 2 == 0 ? 0 : 100')]}, 'limit 0 when mCoinsToDrop even, else 100'),
        ('2644 near time limit|all', OG, 'CGMCoin::IsNearTimeLimit(', 2, {0: [('$rax', '1')], 1: [('$rax', '0')]},
         'near the time limit on alternate calls'),
        ('2644 counter|all', OG, GIA, 2, {0: [('$rax', '500')], 1: [('$rax', '0')]},
         'item amounts 500 / 0 on alternate calls'),
        ('1C44 COIN mode', THINK, 'CGMSinglePlayer::GetGameModeType(', 1,
         # only while the entry installed a force: the mask is 0 (even calls) or the fake mode is in place (odd calls); IDLE can turn
         # true mid-call (the drop branch empties mCoinsToDrop) and run K crashed on the real mode taken for a CGMCoin
         {0: [('$rax', '(%s == 0 || %s == (long)%s) ? 0x434f494e : $rax' % (MASK, GM, FAKE))]}, "'COIN' while the entry forced the mask or the fake mode"),
    ]
    if RUN in ('M', 'N'):
        # slice ppx3 (spp_m06_M.con): run L plus Think's other live arms on e080000a8, call numbers counted over its own
        # Thinks: the 900 wrap (50), pending coins with HP 0 (100, 101; paid at 102), coins to drop 1500 (clamp, chunks
        # 100/50/10, 150), 75 (400) and 7 (amount < chunk, 550), the visor-watcher case as type 8 with mask 0 (701, 703:
        # odd calls, where the COIN entry leaves the mask alone; CheckVisors' loop sends nothing). OnGameEvent's coin arm
        # gets coins per kill 10 and damage per coin 1.0 on e080000a8 (restored). These entries come first: gdb runs the
        # entries of one site in breakpoint order, so the COIN entry's IDLE test sees these writes (launch 1 put them after it:
        # call 100 got mask 0 from the COIN entry, then 5 pending, and Think read PlayerState(-1)).
        FORCE_RET[:0] = [
            ('1C44 coins', THINK, 1000000,
             [(50, [('!$this->mTime', '899.99')]), (100, [('!$this->mPendingCoins', '5')]),
              (150, [('!$this->mCoinsToDrop', '1500')]), (400, [('!$this->mCoinsToDrop', '75')]),
              (550, [('!$this->mCoinsToDrop', '7')])],
             'mTime 899.99 (call 50), 5 pending (100), coins to drop 1500 (150), 75 (400), 7 (550)'),
            ('1C44 dead player', THINK, 1000000, [(p, [(MH, '0.0')]) for p in (100, 101)], 'HP 0 on calls 100, 101'),
            ('1C44 visor case', THINK, 1000000, [(p, [('$this->mProxyType', '8'), (MASK, '0')]) for p in (701, 703)],
             'type 8 (echo visor watcher) with mask 0 on calls 701, 703'),
            ('2644 coin params', OG, 1, [(0, [('$this->mIntParameter2', '10'), ('$this->mFloatParameter1', '1.0')])],
             'coins per kill 10, damage per coin 1.0'),
        ]
        FORCE_INNER += [
            ('1C44 amplifier', THINK, GIA, 1,
             {0: [('$rax', '$this->mCoinsToDrop < 700 && $this->mCoinsToDrop > 300 && $rax == 0 ? 1 : $rax')]},
             'amplifier (and counter) 1 while 300 < coins to drop < 700'),
        ]
    if RUN == 'N':
        # launch 3: run M plus OnGameEvent's arms run M2 missed, on e080000a8 only. Each gameevent line of spp_m06_M.con is
        # one call on it (37 in run M2), so call 5 = kill f560, 6 = damage f575, 11 = kill as 0 f650, 23 = kill f830, 24 =
        # damage f845. These come after '2644 overkill|all' and '2644 coin params', which save and restore the same
        # lvalues, so these writes are not restored on their own ('!').
        FORCE_RET += [
            ('2644 thresholds', OG, 1000000,
             [(5, [('!$this->mFloatParameter2', '0.0')]),
              (11, [('!$this->mFloatParameter2', '10.0'), ('!' + MH, '99.0')]),
              (23, [('!$this->mFloatParameter2', '10.0'), ('!' + MH, '99.0')]),
              (24, [('!$this->mFloatParameter1', '1000.0')])],
             'no overkill threshold (5); threshold 10, HP 99 (11 as relay, 23); damage per coin 1000 (24)'),
        ]
        FORCE_INNER += [
            ('2644 limit 100', OG, 'CGMCoin::GetCoinLimit(', 1000000, {1: [('$rax', '100')], 2: [('$rax', '100')]},
             'coin limit 100 on its first call (kill f560)'),
            ('2644 owned 5000', OG, GIA, 1000000, {1: [('$rax', '5000')]}, '5000 coins owned on its first call (kill f560)'),
        ]
elif RUN == 'I':
    # Agon 05_portalpuzzle EchoVisorWatcher as CoinCollector (as D), COIN mode with a valid first player on
    # idle Thinks: a fake CGMCoin (vtable of the real mode, mPlayers.mItems at +0x68 -> scratch) sits in the
    # proxy's own spare bytes at +0x230 for that one Think call (restored at the return), so SetCanRespawn's
    # inline write lands on scratch. The invalid-first-player arm runs on the alternate idle Thinks.
    IDLE = '(%s && $this->mCoinsToDrop == 0 && $this->mPendingCoins == 0 && $this->mProxyType == 1)' % ME
    FAKE = '((char*)$this + 0x230)'
    GM = '*(long*)((char*)gpGameState + 0x260)'
    FORCE_RET = [
        retype(AREALOADED, 1), visor_as(1),
        ('1C44 coins', ACCEPT, 1,
         [(0, [('!$this->mCoinsToDrop',
                '%s && %s == %s ? 1500 : (%s && %s == %s ? 7 : $this->mCoinsToDrop)'
                % (ME, MSG, AREALOADED, ME, MSG, RESET)),
               ('!$this->mPendingCoins', '%s && %s == %s ? 5 : $this->mPendingCoins' % (ME, MSG, AREALOADED))])],
         'coins to drop 1500 at AreaLoaded, 7 at Reset; 5 pending'),
        ('1C44 dead player', THINK, 1,
         [(0, [('!' + HP, '%s && $this->mTime < 0.2 ? 0.0 : (%s && %s == 0.0 ? 99.0 : %s)' % (ME, ME, HP, HP))])],
         'HP 0 for the first Thinks (pending coins wait), then 99'),
        ('1C44 COIN arm', THINK, 2,
         [(0, [(MASK, '%s ? 0 : %s' % (IDLE, MASK))]),
          (1, [('*(long*)%s' % FAKE, '%s ? %s : *(long*)%s' % (IDLE, GM, FAKE)),
               ('*(long*)(%s + 0x68)' % FAKE, '%s ? (long)(%s + 0x10) : *(long*)(%s + 0x68)' % (IDLE, FAKE, FAKE)),
               ('*(char*)(%s + 0x10)' % FAKE, '*(char*)(%s + 0x10)' % FAKE),
               (GM, '%s ? (long)%s : %s' % (IDLE, FAKE, GM))])],
         'idle: mask 0 (even calls) | fake COIN mode with a valid first player (odd calls)'),
    ]
    FORCE_INNER = [
        ('1C44 COIN mode', THINK, 'CGMSinglePlayer::GetGameModeType(', 1,
         {0: [('$rax', '%s ? 0x434f494e : $rax' % IDLE)]}, "'COIN' while idle"),
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
    ('OnGameEvent', 'CScriptPlayerProxy::OnGameEvent(', [], 'max=400'),
    ('2F10 ~CListener', 'CScriptPlayerProxy::CListener::~CListener(', [], 'max=400'),
]
if RUN in ('M', 'N'):
    SPEC += [
        ('188 LoadPlayerController', 'LoadPlayerController(', [], 'max=400'),
        ('0 ~CScriptPlayerProxy (2F10 inline)', 'CScriptPlayerProxy::~CScriptPlayerProxy(', [], 'max=400'),
    ]
