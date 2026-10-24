# ScriptGui REL (module 60): audit of the upstream body

Current: **80/80 behaviour bodies** (gui1 completion below). Historical survey
counts below describe the original runs, not the current inventory.

## gui1 completion (base c4b67e741, 2026-10-24)

Six markers only; no executable fixes. Fixtures:
`tools/cov/specs/gui1_force.py`, `gui1_slideshow.py`, drivers
`tools/drivers/gui1.con`, `gui1_slideshow.con`.

| body | final post-marker native coverage / oracle |
|---|---|
| 4B8 PlayerJoinManager::Think | **38/38 lines,42/42 edges**. Direct calls on the real join manager, all five join states, A/Y/B/Start/no button, all controller slots, present/disconnected paths, back with/without joined players and Start with/without enough players. Transition field oracles PASS. Input and state writes restored per call. |
| 4544 FrontEndScreen::Think | **25/25,22/22**. Natural open/load/reset plus forced completion results Normal/RemoveIOWin/RemoveIOWinAndExit/Exit, busy/ready and failed paths. PumpLoad result forced only for controlled completion calls; CloseSaveGameScreen suppressed in repeated failure cases to retain the real screen. Final Exit runs the **actual delete**, asserts mgr save pointer null and mGameStarted true, exercising existing PC lifetime early return. Outgoing connections removed for this one call to avoid starting a game without a selected world; no freed field restored. |
| 2B78 UpdateSaveSlots | **51/51,63/63**. Constructed POD GameFileStateInfo metadata, GetGameData returns0/1/2/3 valid fixture slots; normal/hard, light/dark world names, missing-name fallback, unlocked/locked normal completion. No card file writes; native text/switch logic runs. Name-return null forced in the fallback case. |
| 4714 CollectWidgets | **182/199,153/153 non-EH edges**. Real FrontEnd graph; repeat collection resets counts of non-owning/trivial vectors, then restores counts. Coin/DeathMatch previous results with one player and differing invert/rumble values; field oracle verifies the existing PC menu-swap fix. Temporary typed PowerBombDamage/None connection to a **real CScriptSound** covers reserve/push/volume loop, then original graph restored and temporary allocation freed. Remaining uncovered compiler cleanup lines are exception-only vector unwind code, not normal branches. |
| CEC ShowSlideShow | **7/7,1/1**. Existing environment-variable values forced PercentScans100, NormalModeCompleted1, HardModeCompleted1; native GetGalleriesUnlocked returns63 and native list build creates6 galleries/141 slides. Actual draw hits and B closes the modal; normal exit. No mask-return override, empty-gallery guard, or claim that all141 slides were displayed. |
| 95FC Slider::Think | **46/47,35/36**. Real brightness slider, analog left/right, dpad left/right, idle, clamps, no request, target changes, save-screen absent/ready/busy, real sound allocation followed by a second moving tick (existing-handle path) and stop. Only missing callback invocation is **writer-dead**: mCallback default-constructs null; repository-wide search finds no calls to SetEventCallback and no assignments to mCallback. Its unused setter is the only non-constructor writer; no fake callback bridge/method pointer introduced. |

Final queue **72/72 successful calls**, field oracles PASS; both final runs exit
normally with zero caught/reach-stub, failed calls/writes/restores or console
errors. A diagnostic Exit run originally followed Arrived into an unselected
game world; the final isolated-connection fixture avoids that invalid setup.

Static audit retains the original retail comparisons described below. Additional
retail_diff findings resolved: D74 is CArchitectureQueue::Push H copy,5EB0 the
SSaveSlot reserved_vector::push_back H copy, KillSaveGameInterface the retail
name of DeleteSaveGameScreen. Slider's eight PPC float comparisons include
inlined ALALeft/ALARight sign extraction (9678/9694/96CC/96E8), not eight source
conditions; remaining comparisons are the two input-positive checks and the
inline min/max clamps. Hardware/final input axes are finite, bounded inputs;
no NaN-driven native change is justified. Existing Think PC return is separately
native-tested above, not justified by upstream's100% match alone.

Gates: host/coverage links0 FAILED; manifest rc0; matching configure+ninja and
checksum **87 files OK**; source with marker comments removed identical to base
on both PC/non-PC paths; whitespace clean. Review base c4b67e741:6 MARK.
No unchanged-family sweep. Evidence `/tmp/opencode/reports/gui1-evidence/`.

Lane gui1 audited this module on 2026-10-23.

- **Body:** upstream's `src/MetroidPrime/ScriptObjects/CScriptGui{Menu,PlayerJoinManager,Screen,Slider,Widget}.cpp` and `ScriptGuiRel.cpp` (script types GWIG, GMNU, GSLD, GSCR, GPJN). The port builds them with the two fixes below.
- **Sources:** the retail asm `build/G2ME01/ScriptGui/asm/MetroidPrime/ScriptObjects/*.s`, objdiff of upstream's matching build against it, and coverage runs in the FrontEnd area.
- **Per-function table:** `ScriptGui_functions.tsv` (152 rows: 80 B, 65 H, 7 T). All `fn_60_X` are REL `.text` offsets.

## 1. Fixes (both `#ifdef TARGET_PC`; the matching build is unchanged)

- **`CollectWidgets` (fn_60_4714), multiplayer-results restore:** upstream swaps the invert and rumble menus. The asm loads `lbz r3,0xc(r29)` (`SPlayerResult::xc_`) and passes it to `lwz r3,0x4(r30)` (`SPlayerSetup::mInvertMenu`); `lbz r4,0xd(r29)` (`mRumbleEnabled`) goes to `lwz r3,0x0(r30)` (`mRumbleMenu`).
- **`SLdrGuiSlider` ctor (inlined in `LoadGuiSlider`, fn_60_8E90):** the default `slideSound` is -1, not 0: `li r6,-0x1` … `stw r6,0x7c(r1)` (0x7c is `slideSound`, 0x80 is the volume, `li r0,0x7f`). With 0, a slider whose data has no sound would play sfx 0.

An older port change is kept: `CScriptGuiFrontEndScreen::Think` (fn_60_4544) returns right after a state change that can free the screen (use-after-free at 0x4674 on the host).

## 2. Equivalences (objdiff below 100%, no behaviour difference)

- `Render` 4220 (81.8%), `Slider::Think` 95FC (96.3%), Menu ctor 8CE0 (95.1%), `UpdateSaveSlots` 2B78 (98.5%): only scheduling or registers differ.
- `LoadGuiPlayerJoinManager` B8 (96.0%): one redundant store of `unknown=3`.
- `PlayerJoinManager::Think` 4B8 (97.7%): `cmplwi` against `cmpwi` on a value that is never negative.
- `LoadGuiMenu` 7D20, `LoadGuiWidget` 6FF0 (99.7%): one register.
- `CollectWidgets` (97.8%) and `LoadGuiSlider` (93.9%): the fixes above; the objdiff figure is upstream's non-PC path.
- Class H: rstl `vector`/`list`/`string` methods, the `CGameState` member-dtor chain fn_60_3900…6F20, `CModelDataNull`, `~CGameOptions`. This is template and DOL code that the host emits itself.

## 3. Coverage (build-port-cov, `tools/cov/specs/scriptgui.py`)

Area FrontEnd: world 69802220, area cee3af0d, `--env MP2_SKIP_FRONTEND=0`. The drivers are in `tools/cov/specs/`:

- two plain boots;
- `gui_setzero.con`;
- `gui_saves_options.con` (forced (send): save-slot and option InternalMessages, slider steps);
- `gui_menu.con`;
- `gui_menu_slider.con` (forced (send): menu Alert/Open/Close/Lock, presses, slider Increment/Decrement/Delete).

Each run ran 700–1200 frames with 0 `[port] caught`/reach-stub.

- The area boots at about 1008/1024 object ids, so a driver must activate only a few objects. Mass activation exhausts the pool.
- `send e0 Unlock` must come first: without it `StartSelectedGame` faults on a null save screen.
- Sending `Restore` before `Store` reads the INT_MIN sentinel. The drivers send Alert first.

**Marked (`// @ fn_60_`, 28):**

- 0, B8, 980, BD0, EB4, 1234, 2908, 312C, 4220, 424C;
- 6060, 62D0, 6F70, 6FF0, 7190, 721C, 7BE0, 7CB0, 7D20, 85D8;
- 8770, 8CE0, 8E30, 8E90, 95C8, 95E0, 9B10, 9C68.

**Open (52):** the TSV gives each one's line and branch counts.

- Menu Select*/Think/ProcessMenuInput: wrap and locked-item arms.
- Widget Accept/Think/ProcessInput, and the Slider message, value and platform arms.
- The option Store/Compare/Restore/Reset/Apply widgets: per-option arms.
- `ResetOptionPage` (not reached) and `ShowSlideShow`. IM13 crashes in the DOL `CSlideShow` with an empty gallery.
- The PlayerJoinManager functions, `StartMultiplayerGame` and `UpdateControllerCount`: multiplayer or multi-controller arms. These are live but not run.

**asmcheck:** `tools/asmcheck.py check ScriptGui` on the marked functions reports only tool artifacts:

- **Loaders:** calls and branches come from the inline `SLdr*` ctors, `LoadTypedefEditorProperties` and `ReadBytes`.
- **Dtors:** compiler-generated member and base dtors, and `Free`.
- **CALL fn_60_8D8C/6560/71C8/CA0:** these are H helpers.
- **`DeliverScriptMsg`:** called inside the inline `SendScriptMsg`.
- **9C68:** retail `ScriptGUI_SetPtrs` is upstream's `SetSGuiWidget_FuncPtrs`.
- **Bits:** the bitfields now carry position comments (`// 0x368 bit 0x80`…, `// 0x90 bit 0x80`), and the BIT findings are clean.
- **Crash:** asmcheck crashes with a crbit `TypeError` on EB4, 312C, 427C, 4714, 2B78, 2954, 28A4 and 14A4. EB4 and 312C (marked) were checked by hand; both match objdiff at 100%.

## 4. Smoke family (proposed)

- **`scriptgui`:** FrontEnd 69802220,cee3af0d, driver `tools/cov/specs/gui_menu_slider.con`, 700 frames, `--env MP2_SKIP_FRONTEND=0`. It covers the FE screen ctor/Think/CollectWidgets, the Menu, Widget and Slider, and the loaders.
