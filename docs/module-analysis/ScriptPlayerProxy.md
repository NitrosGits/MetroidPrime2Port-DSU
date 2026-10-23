# ScriptPlayerProxy REL (module 62): audit of the upstream body

Lane spp1 audited this module on 2026-10-23.

- **Body:** upstream's `src/MetroidPrime/ScriptObjects/CScriptPlayerProxy.cpp` (`CScriptPlayerProxy`, script type `PLCT`). The port builds it unchanged.
- **Sources:**
  - the retail asm `build/G2ME01/ScriptPlayerProxy/asm/MetroidPrime/ScriptObjects/CScriptPlayerProxy.s`;
  - objdiff of upstream's matching build against that asm;
  - the shipped PLCT data from all worlds (177 placements), scanned with `tools/scan_playerproxy_props.py`.
- **Per-function table:** `ScriptPlayerProxy_functions.tsv`.
- All `fn_62_X` are REL `.text` offsets.

## 1. Fix

None. Every function either matches the asm or differs only in code generation. The six handlers that objdiff leaves unpaired were checked against the asm by hand.

One retail quirk is kept on purpose. In `OnGameEvent`, the PersistentCounterReset pick loop increments `seen` before it compares, so pick 0 never clears an item. The asm does the same thing.

## 2. Equivalences (objdiff below 100%, no behaviour difference)

- `LoadPlayerController` (99.6%), `CheckVisors` (95.3%), `BuildPlayerMask` (97.8%): only register allocation differs.
- `UpdateTransform` (98.8%), `DropCoins` (95.7%): only instruction scheduling differs. The `DropCoins` state ids BIDG, BXDG, IDMG and XDMG are equal on both sides.
- `NotifyProxies` (98.8%): the bool return at 0x12FC is a different idiom.
- `Think` (98.9%): the bool argument uses a `cntlzw` idiom.
- `OnGameEvent` (98.8%): the equality test at 0x2A68 is a different idiom.
- Handlers left unpaired by objdiff, because upstream's signatures take more arguments than retail: `HandleDamageOverTimeMessage`, `HandleVisorMessage`, `HandleMessage`, `HandleRespawnMessage`, `HandleFollowerMessage`, `HandleArchenemyMessage`. The asm walk found the same loads, calls and arms:
  - `HandleMessage`: the GiveScore and EndGame vtable calls, and a ZERO `ApplyDamage` of 1e6.
  - `HandleFollowerMessage`: bit 0x40 at +0x230.
- `CModelData`, `optional_object`, `TToken`, `rstl::string`/`wstring`, `SLdrPlayerController` and `reserved_vector` copies (class H): template code that the host emits itself.

## 3. Shipped data (PLCT proxyType, prop 0xCA56A18A)

| proxyType | placements | where |
|---|---:|---|
| EchoVisorWatcher | 29 | single player (Agon `05_portalpuzzle` and others) |
| DarkVisorWatcher | 20 | single player (Temple `01_temple_landingsite_dark` and others) |
| EventRelay | 48 | multiplayer maps M01–M06 only |
| CoinCollector | 24 | multiplayer only |
| PlayerFollower | 24 | multiplayer only |
| DamageOverTime | 24 | multiplayer only |
| PlayerMessageRelay | 8 | multiplayer only |
| ArchenemyMarker, PersistentCounterReset, CombatVisorWatcher, ScanVisorWatcher | 0 | not shipped |

Single player therefore reaches only the visor-watcher paths. Everything else needs the multiplayer game mode, which the port does not boot. The Combat and Scan arms of `SetVisorFromProxyType` are data-dead.

## 4. Coverage (build-port-cov)

Lane spp1 (2026-10-23) ran three natural visor-watcher runs (`tools/cov/specs/scriptplayerproxy.py` with
`spp_echo_portalpuzzle.con`, `spp_dark_landingsite.con`, `spp_dark_arealoaded.con`). Lane sppx1 (2026-10-24)
added seven forced runs, `SPP_RUN=A..G` of `tools/cov/specs/scriptplayerproxy_force.py`, each with its
`spp_force_<run>.con` (Agon 42b935e4 `05_portalpuzzle`, 2000 frames, `--fast`; command line in the spec header).
Each run retypes the one EchoVisorWatcher there (u0523, editor 00120402) into another proxyType at its first
AreaLoaded/Create, so single player reaches the multiplayer arms:

| run | forced (`SPP_RUN=`) | arms |
|---|---|---|
| A | PlayerFollower (3) | Think follower, UpdateTransform both arms (vector param on/off), AddToRenderer draw, HandleFollowerMessage |
| B | ArchenemyMarker (2), render index 0/1 | Set/GetArchenemyIndex, HandleArchenemyMessage, UpdateTransform archenemy, AddToRenderer index arms |
| C | EventRelay (0), then type 11 on Activate and -1 on Reset | HandleMessage (GiveScore/EndGame/ZERO), AcceptScriptMsg range arms |
| D | CoinCollector (1), HP 0 for the first Thinks | Think pending-coins and coin arms, DropCoins (amplified both ways, all four counts) |
| E | DamageOverTime (10), list emptied on the 20th hit | UpdateDamageOverTime, HandleDamageOverTimeMessage |
| F | PersistentCounterReset (4) | HandleRespawnMessage |
| G | DarkVisorWatcher (7), mask 0 on CheckVisors calls 2/4/6 and InitVisors call 5, team bit 4 on odd ctors | CheckVisors/InitVisors not-in-mask and both visor arms, BuildPlayerMask team bit |

Merged (`tools/cov/gcov_report.py build-port-cov /tmp/opencode/sppx1_{A..G}/gcov --module ScriptPlayerProxy`):
16 of 23 functions with branches have every arm. All ten runs: 0 `[port] caught`, 0 `reach-stub`.

| fn | result | untaken |
|---|---|---|
| UpdateTransform 1448 | 19/19 lines, 7/8 | 150:1/2 is a component edge of the inlined `CVector3f::IsNonZero` (the run's vector had X = 0); both if bodies ran |
| DropCoins 190C | 13/16 | `denominationMode != 1` (237/246): dead, the only caller is `bl fn_62_190C` at 0x1DE4 in Think after `li r8, 0x1` (0x1DE0), and nothing else in the module's asm names fn_62_190C |
| Think 1C44 | 37/38 | 366 true arm: the COIN game mode with a first player calls `CGMCoin::SetCanRespawn`; on the port's CGMSinglePlayer that cast corrupts memory, so it is live but unforceable |
| LoadPlayerController 188 | 8/12 | model present (0x4BC `bne`), proxyType < 0 / > 10 (0x4E8/0x4F0): no shipped PLCT has a type outside 0..10 (§3); the model arm is live data |
| OnGameEvent 2644, NotifyProxies 11C4, AddDamageOverTime DF8, ClearDamageOverTime D94, CListener::OnGameEvent, GetPlayerMask 1314 | not in the gcov runs | only the multiplayer game modes post game events; CGMSinglePlayer has no listener dispatch. Now driven by the port console `gameevent` command (§6) |

Marked (`// @ fn_62_`, 25): 0, D94, DF8, 11C4, 1314, 2F70, EA4, 131C, 13D8, 1448, 16B4, 1718, 1778, 1868, 190C, 1BDC, 1ED8, 1F0C,
1F44, 2118, 21DC, 222C, 23D4, 2C58, 2CA8.

`tools/asmcheck.py check ScriptPlayerProxy src/MetroidPrime/ScriptObjects/CScriptPlayerProxy.cpp` leaves 7
findings, all compiler-emitted calls: the dtor (0) has implicit member dtors, the `Free` of the deleting dtor
and their conditions; EA4 calls the `reserved_vector::erase` template (fn_62_113C, class H); 222C has a
`rstl::wstring` temporary dtor; 2CA8 has init-list calls (CALLS 5 vs 0).

The ctor bitfields carry header position comments (`// 0x230 0x80` / `0x40`).

## 5. Smoke family `plct` (extension implemented)

`tools/smoke/cmd_plct.txt` drives u0523 on Agon `05_portalpuzzle` (editor 00120402, the EchoVisorWatcher,
proxyType 8; the header said PlayerMessageRelay before). Steps f420-f740 give the echo visor, switch
echo/combat/echo/combat with `visor` read-backs, send `AreaLoaded` twice (InitVisors/CheckVisors arms),
then `gameevent spawn`/`generic` on the natural type (no-op arms, the dispatch itself). A broken visor path
fails on the `visor` read-backs (combat 0 / echo 1) and on any `[port] caught`/`reach-stub`; the last check
stays @f2000 (F=2200).

## 6. `gameevent` console command (OnGameEvent family)

`gameevent <score|kill|generic|scan|damage|stop|spawn> [src] [tgt] [value] [as <proxyType>]` calls
`CScriptPlayerProxy::PortDeliverGameEvent`, which mimics `CGMMultiplayer::NotifyListeners` (single player's
`AddListener` is a no-op): every proxy whose player mask has the target bit gets `CListener::OnGameEvent`.
`as N` retypes the proxy for the call only (not 1, CoinCollector: its arm casts the game mode to CGMCoin).
Asm walk of OnGameEvent 2644 (all 11 jump-table slots: 0/4 relay+counter reset, 2 archenemy, 9 message
relay, 10 DOT, 1 coin, 3/5-8 none), NotifyProxies, Add/ClearDamageOverTime, CListener::OnGameEvent and
GetPlayerMask matches the source line by line. Marked: D94, DF8, 11C4, 1314, 2F70. OnGameEvent is not
marked: the CoinCollector arm was compared by reading only, it cannot run on CGMSinglePlayer.
Runs (plct on Agon, 0 caught, 0 reach-stub): every event on the natural type, `as 0/2/4/9/10` with the
events each arm handles, `as 9 generic 0x454e5452` (NotifyProxies' state match), out-of-range index errors.
