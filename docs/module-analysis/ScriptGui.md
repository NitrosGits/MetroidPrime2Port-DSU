# ScriptGui REL (module 60): audit of the upstream body

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
