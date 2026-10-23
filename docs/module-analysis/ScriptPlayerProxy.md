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

## 4. Coverage (build-port-cov, `tools/cov/specs/scriptplayerproxy.py`)

Three runs were merged. The drivers are in `tools/cov/specs/`.

- `spp_echo_portalpuzzle.con`: Agon 42b935e4 `05_portalpuzzle`, EchoVisorWatcher e00120402. It cycles visors and sends Deactivate/Activate.
- `spp_dark_landingsite.con`: Temple 3bfa3eff `01_temple_landingsite_dark`, DarkVisorWatcher e00230084. The same sequence with the dark visor.
- `spp_dark_arealoaded.con` (forced): sends Activate, switches to the dark visor, sends `send e00230084 AreaLoaded` with the visor on and off, then sends Delete.

| fn | result | untaken |
|---|---|---|
| ctor 2CA8 | all arms | — |
| HandleVisorMessage 1F0C | all arms | — |
| SendStateToPlayer 13D8 | 4/4 lines | — |
| CheckVisors 1778 | 9/10 branches | 185 player not in mask (one player) |
| InitVisors 1868 | 5/6, in-visor arm forced (`send AreaLoaded`) | 205 player not in mask |
| AcceptScriptMsg 23D4 | 12/23 | Create → Archenemy/default, range-check false arms, non-visor handler switch (multiplayer) |
| Think 1C44 | visor arm only | DOT, coin, follower and archenemy arms (multiplayer) |
| UpdateTransform 1448 | 8/19, else arm | Follower/Archenemy arms |
| LoadPlayerController 188 | 8/12 | model present, proxyType out of range |
| BuildPlayerMask 2C58 | — | team-bit arm |
| AddToRenderer 131C | ran, no draw | draw arms (Follower/Archenemy) |
| DOT, NotifyProxies, DropCoins, Archenemy, HandleMessage/Respawn/Follower/Archenemy/DOT, OnGameEvent | not run | multiplayer only |

Marked (`// @ fn_62_`): 13D8, 1F0C, 2CA8.

- **CheckVisors and InitVisors:** their single untaken arm needs a second player (multiplayer), so it is live but not run; they stay unmarked.

`tools/asmcheck.py check ScriptPlayerProxy` reports only tool artifacts:

- **13D8:** a CALL to the unmarked `UpdateTransform` (fn_62_1448), plus a BRANCH count that the merged helper inflates.
- **2CA8:** a CALL to the unmarked `BuildPlayerMask` (fn_62_2C58), plus CALLS 5 vs 0, because init-list calls are not counted.

The ctor bitfields carry header position comments (`// 0x230 0x80` / `0x40`).

## 5. Smoke family (proposed)

- **`scriptplayerproxy`:** Temple Grounds 3bfa3eff `01_temple_landingsite_dark`, driver `tools/cov/specs/spp_dark_landingsite.con`, about 1100 frames with `--fast`. It covers ctor, AcceptScriptMsg, Think, InitVisors/CheckVisors and SendStateToPlayer.
- **Alternative:** Agon `05_portalpuzzle` with `spp_echo_portalpuzzle.con`.
