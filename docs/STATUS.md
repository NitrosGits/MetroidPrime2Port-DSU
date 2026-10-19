# Session status (detail)

The Status section of AGENTS.md links here: this is the per-session detail, rewritten rather than
appended, dated as in the heading there.

## 2026-10-19 (twenty-eighth session): widescreen HUD spread, the MP1 way

- **`6ae31083e`, merged `f209d2f97`:** MP1's widescreen HUD. A first attempt spread widgets by their
  x position and detached the energy number from its bar; that was thrown away. MP1's mechanism is a
  per-camera opt-out (`SetAspectMatch(match, spread)`, default spreadable) plus a per-widget move in
  `CGuiFrame::Draw` after `CGuiCamera` widens the projection. `CSamusHud` marks the HUD and helmet
  cameras spreadable; the pause and message screens keep the game's proportions. Setting
  `MP2_HUD_WIDE` (key `hud_wide`, F1 > Video > Display, default on).
- Measured before/after: MP2's HUD never spread before this (the pause frame spans x 197-1088 of 1280
  at 16:9, which is MP1 with HudWide off). Now with it on the radar and energy readout sit at the left
  edge and the ammo/missile cluster at the right; with it off the HUD is 4:3 centred; the pause screen
  is identical either way. Screenshots `/tmp/opencode/hw2_{hud,pause}_{on,off}.png`.
- Merged-tree gates: host build 0 FAILED (binary 15:39), `gen_files_cmake.py --check` rc 0, every
  touched upstream file strips CLEAN, and the matching build still reads 94.14% fuzzy / 63.29% matched.
- Not covered: the selected-visor HUD frames are not marked spreadable (`CSamusHud` exposes only the
  HUD and helmet cameras), MP1's explicit anchor clusters are not ported, and the spread ignores a HUD
  scale below 100%. Nothing was walked in gameplay, per usual: the screenshots are of a standing player.

## 2026-10-19 (twenty-seventh session): every world swept

- **Method.** I used a scratch copy of `tools/sweep_areas.sh` that sets `MP2_WORLD="<world>,<first area's MREA>"` instead of
  `MP2_TELEPORT`, because the teleport only indexes the save's own world and wins when both are set. Settings: `NEXTAREA` walk,
  `GODMODE=1`, `SHARDS=4 DWELL=900 TAIL=900`, save `/tmp/opencode/ss_user`, and the binary from 15:39 (`c334bc9ae`).
  The logs are in `/tmp/opencode/wsw/log_w_<world>_<shard>.log`.
- **Worlds.** The console's `worlds` command lists five campaign worlds, FrontEnd04 (`69802220`) and six 1-area
  multiplayer maps. All of the 1-area worlds load, and boots of `233e42be` and `0x69802220` exit 0.

  | World | Id | Areas | Reached | Verdicts |
  |---|---|---|---|---|
  | Temple Grounds | 3bfa3eff | 63 | 0-53, 56, 57 | shard 3 STALL at 57->58; 54/55 not in the walk; 59-62 not covered |
  | Agon Wastes | 42b935e4 | 72 | 72 | all exit=0; reach stubs at 7 and 37 |
  | Torvus Bog | 3dfd2249 | 70 | 70 | all exit=0; reach stub at 46 |
  | Sanctuary Fortress | 1baa96c2 | 67 | 67 | all exit=0; area 16 POOL |
  | Great Temple | 863fcd72 | 12 | 12 | all exit=0 |

- **Totals across every log.** There were no `[port] caught`, `[player] killed`, missing-resource, `[model]`, assert or `[end]`
  lines. The only `[warning]` is the shutdown `Device lost: Device was destroyed`.
- **Temple Grounds 58 is a harness trap, not a game wall.** The log reads `[warp] area 58: its validation was held by a dock;
  releasing it` and then `[harness] no game update for 30.0 s (6322 updates so far, limit 13500)`, exit=124. A lone
  `WORLD=3bfa3eff,85499aa7` run reproduces it after 122 updates. The screenshot shows "Save progress to Memory Card in
  Slot A? Yes/No": the arrival spawn is inside a save station, and the sweep's `B` does not answer that prompt. Under gdb
  the main thread was idle in the frame limiter (`CGraphics::EndScene`).
- **Top blockers, earliest first:**
  1. Agon 7: `CParasite::ProjectVectorToPlane` reach stub (the parasites' plane projection is a no-op).
  2. Agon 37: reach stub for `TStateMachineStateBase<CPatterned>::~TStateMachineStateBase()`.
  3. Torvus 46: the `CParasite::ProjectVectorToPlane` stub again.
  4. Sanctuary 16: the id pool holds at `[ids] frame 14700: 982/1024 ids allocated, 982 objects` until frame 15000. That
     follows `[warp] dropped the areas around 16 (650 object ids free)`, so one more spawn burst there could exhaust the pool.
  5. Tooling: the save prompt at TG 58 (above) stops the sweep. `ResolveWarpAsset` reads an all-digit hex id as decimal
     (`MP2_WORLD=69802220` → `0x042918ec`, "neither the save nor the disc's MLVL names an area"; `0x69802220` works).
     The warp walk also never rolls credits, because slot 53 logged no `[end]`.
  `DebugDrawAABox` also hits its stub (Sanctuary 0, TG 44/48, Agon 47/59), which is harmless.
- **Coverage limit.** This sweep tests arrival plus about 15 s of standing gameplay under god mode. It does not test
  traversal, doors, elevators, bosses or the ending. Next: real `ProjectVectorToPlane` and state-dtor bodies, a `B`/`Yes`
  answer for save prompts in `sweep_areas.sh`, a hex-always parse in `ResolveWarpAsset`, and TG 59-62.

## 2026-10-19 (twenty-sixth session): interpolation phase 6, save-state resume and door locks

- **Phase 6** (`9b651429a`, MP1's section 3 "swooshes, electric effects and beam weapons"):
  `CParticleSwoosh` draws trails between the last two ticks (segments matched by age, spin the short
  way, the actor's shift dropped while it blends), `CParticleElectric` blends its line transform, and
  `CPlasmaProjectile` blends transform, length, width and angle (its Echoes `Render` did not fit MP1's
  edit, so the PC version is duplicated ahead of the original under `#else`). Beams opt out of the actor
  blend through `CActor::PortSetOwnPresentation`. `CWaveBuster`/`CFlameThrower` have no Echoes
  counterpart. `platform/include/port_tick_pair.h` and `port_tick_pair_tests` come from MP1. Uses the
  existing Smooth effects setting, so there is no new flag. Not seen on screen: both runs idled in area 0.
- **Quick resume, door locks, a patient restore** (`65a05ac34`, merged `f117ba308`): the resume state
  (`<user>/savestates/resume.mp2ss`) is written on a clean exit when `MP2_RESUME=1` and loaded at the next
  launch when the front end is skipped and no `MP2_WORLD`/`MP2_TELEPORT` was asked for; it is deleted once
  used and refused if its world is not on the disc. Doors keep their lock state in the room capture
  (applied with `kSM_Lock`/`kSM_Unlock`; one friend line in `CScriptDoor.hpp`). `TickRoom` now gives up only
  after 1,200 *idle* frames (a pending layer, a loading area or a progress change resets the count), with a
  hard cap. Verified end to end: `resume: wrote ... (Area 0)` then `loading it instead of the card's save`
  and `Samus is back in Area 0; the resume state is used up`, both runs exit 0, 0 unmatched. The door-lock
  apply and the real window-close path were not exercised.
- Merged-tree gates: host build 0 FAILED (binary 15:14), matching build "87 files OK",
  94.14% fuzzy / 63.29% matched. Every touched upstream file strips CLEAN.

## 2026-10-19 (twenty-fifth session): the glyph keys were right all along

- **Echoes uses Prime 1's button textures.** Opening the pause screen (START) and diffing its texture
  dumps against a gameplay run leaves 19 textures, and four are the hashes already in `kKeys`:
  `32x32 bb21e8755f36b2f0` (`AButtonIn`), `32x32 e6dbfd18d4666ee7` (`BButtonIn`), and the two `64x80`
  `AButtonOut`/`BButtonOut` sets. Keys are content hashes (`XXH64` of the decoded bytes,
  `extern/aurora/lib/gfx/texture_replacement.cpp:428`), and the art is shared between the games.
- So the earlier "the binding icons replace nothing in Echoes" was wrong: every run up to then
  dismissed the HUD memo (`MP2_SKIP_INTRO=1`) and never drew a prompt, so the textures were never
  loaded. Evidence: `/tmp/opencode/pause.png`, `/tmp/opencode/pause_only.txt`.
- The `MP2_DEBUG_PROMPTS` hook (`36775d730`) and its constructor variant were reverted: only one
  `CGuiModel` is ever built in a run (id `ffffffff`), so Echoes' prompt widget is not a `CGuiModel`
  and the hook told us nothing about it.
- **Left to do:** run `tools/make_prompt_glyphs.py` to produce a `bindings/` set, point `MP2_TEXTURES`
  at it, and confirm the pause screen's bottom bar shows keyboard glyphs instead of A/B/C/Y.

## 2026-10-19 (twenty-fourth session): the texture dump path, and where Echoes' prompts are

- **`f558e5b3f`:** a texture dump dropped every file. `dump_editable_texture_dds` wrote into
  `<cache>/texture_dumps`, but only `load_replacement_directory` ever created that folder, so a run
  with no pack installed - the one way to author a pack - had nowhere to write. The dump now creates
  the folder, and a dump skipped because the format did not convert logs why (once per format/size).
  Verified: `MP2_DUMP_TEXTURES=1` writes 147 `tex1_*.dds` into the run's own cache folder
  (`/tmp/opencode/c_run/texture_dumps`, because `tools/run_gameplay.sh` sets `MP2_CACHE_PATH` from
  `CACHEDIR` - the `MP2_CACHE_PATH` a caller passes is overridden, which is why the first attempts
  looked empty).
- **Echoes' button prompts are not 32x32 textures.** Of the nine 32x32 dumps from a Temple Grounds
  boot (`/tmp/opencode/glyphs_sheet.png`), none is a button: the game draws prompts from GUI *models*
  (`model_abutton` in `CSamusHud.cpp:157`, `CHudDecoInterfaceScan.cpp:108`), whose texture is not one
  of them. Building Echoes' `kKeys` therefore needs a dump taken while a prompt is up, plus the model's
  own texture size/hash - still open.


Older sessions: `docs/STATUS-history.md`.
