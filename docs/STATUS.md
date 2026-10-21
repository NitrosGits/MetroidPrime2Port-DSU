# Session status (detail)

The Status section of AGENTS.md links here: this is the per-session detail, rewritten rather than
appended, dated as in the heading there.

## 2026-10-20 (port orchestrator): lanes in flight

Read a lane's result with the openchamber tool (`session.messages`, `lastAssistant`), review with
`tools/review_decomp_diff.sh <base>` from its worktree, accept, then send the same session its next slice.
Worktrees: `~/.local/share/opencode/worktree/3e7f972e5f2a67a4bf166b19e99a591913d8d4a4/wt-mp2-*`.

Batch 5 merged into port `3bb8177db` (via branch `batch5`, worktree `../wt-mp2-batch5`): playerturret
`c377344ed`, ing `8855b1674`, spacepirate-vt `c42d82fce`, emperoring `542d6c9b8`, emperoring3 `f786b983e`,
boostguardian `da45064c5`, digitalguardian `227e42fb1`, commandopirate `cf9eb1995`, splitter `b17910bb4`
(+ `e4aead7b6` drops DG's duplicate `CCollisionActor::SetExtendedTouchBounds`). Host/link/matching
green, smoke5 17/17 clean (`/tmp/opencode/smoke5/`), 1159 markers. Upstream build must run before the
port build in a fresh worktree (writes `build/G2ME01/include/*.inc`; needs `extern/musyx` checked out).

Batch 6 (2026-10-21, merged into port as `3b2effc06`, 1677 markers): merged emperoring3
`43eb8312e`, emperoring `5a594830e`, boostguardian `673fbcb56`, minoring `c8d342614`, spacepirate-vt
`33291f63c` (conflict in PortCSpacePirateStates.cpp: kept port's WarpIn fix `4d6c4d036`, dropped the
stale fn_72_7078 marker), wallcrawler2 `036ca13db`, commandopirate `6ea58f47a`, digitalguardian
`c56aff752`, splitter `29e4bf6aa`, darksamus2 `f0de7edab`. Matching 87 OK + sha1 OK, host + link green
(07:05). smoke6 17/17 clean (rc 0, 0 caught, 0 reach-stub; stand-ins down in boostg, digig, emp1,
emp3, ep). Batch 7 = lane commits after these bases, merged by hash.

Older list (superseded by batch 6):
spacepirate-vt `33291f63c` (8 combat marks); wallcrawler2 `036ca13db` (obstruction scan, 3 marks);
emperoring `bd4c2c25a` (7EA8, 7F8C cases, 1D38, Render 531C); splitter `b1f682ff7` (1188/58E8, F1C0,
654C, 1790, AC68, 8D50, AAC4); commandopirate `843ebf292` (4D84); boostguardian `999b51bdf` (D4F8 events,
C1B0/C05C, C850, puddle vulnerability 0x1178); digitalguardian `8826277c0` (8090, 7D8C, 5910, 5384,
7CB4, 5274/266C/2E60, 5838 marked); emperoring3 `e3971fcd3` (6904/67C0 beams, full-TUniqueId fix).

Port fix 2026-10-20 (scan-visor lag, after the OOM fix `1f1c5c9a3`): `DrawScanVisor` builds a new
CGraphicsPalette (new TLUT id) every frame, so Aurora made a render-scale palette texture every frame;
`resolve_dynamic_palette_texture` now takes over an older identity's texture for the same copy (desktop:
1 alloc on entry, then 0/120 frames). The two scan-visor EFB CPU read-backs (full render-scale frame
each, every frame) are dropped: Aurora draws both from the copy textures (A/B identical; only
`MP2_NO_COPY_TEX=1` uses the bytes). Desktop-verified; tablet not yet tried (ask before installing).

| lane | session | branch @ base | task |
|---|---|---|---|
| BoostGuardian (Opus) | — (done) | boostguardian @ `673fbcb56` (204/318; 103 cited, 2 folded; loaders hand-checked, isEchoEmitter + 19 sound ids -1 defaults; 12AB0 compares correct) | merge in batch 6; 9 fns between tsv 309 and 318 unexplained |
| EmperorIng3 (Opus) | — (done) | emperoring3 @ `43eb8312e` (182/270; 88 cited, 0 todo; matching build OK) | module complete: merge in batch 6 |
| DigitalGuardian (Opus) | `ses_ea312bc93ffdb6liLXfqe2O49p` | sandboss @ `c56aff752` (SelectRetreatPlatform 13C6C ported; 12818 unmarked; 41 markers; exit 13 did not recur in 2 runs) | why beams never hit the root collider u0574 (likely port bug), XRDG damage arm, retreat → 12818/15284 platform arms, then slice-3 stand-ins |
| Splitter (Opus) | `ses_ea312bc7fffemfIq7YQeApP3b6` | elitepirate @ `5c3bbaa70` (accepted for batch 7: frame-900 drop was a fall-through, fixed by head material filter `89974267a`; A4E4 legs are OBBs 0.8 + every actor reflect-vulnerable, marked A4E4, A37C; 8EC8 ported unmarked) | drive 8EC8 radius arms, 986C 0.5/kSD_Left/Right, ACEC non-LookAt; then next 4 unmarked |
| CommandoPirate (worker) | `ses_ea328681cfferVtp6Dt5onjmgs` (retry; first lane died silently) | commandopirate @ `6ea58f47a` (57A0 LOS fix, 3090, C1F4; Render C598 ported unmarked) | reached: C798, BF44, 1C0C, 3E1C, B250 |
| SpacePirate (worker) | `ses_ea3206eb3ffeIXL6w8JDRAddQp` | spacepirate @ `1c654bc31` (72/262) | 5 most-called unmarked fns |
| SwampBossStage1 (worker) | `ses_ea3206eafffeTqAzGkbdyimVXR` | swampboss @ `1c654bc31` (108/251) | 5 most-called unmarked fns |
| SwampBossStage2 (worker) | `ses_ea3206eaeffeRdrP5aHU5wZwue` | swampboss-s2 @ `1c654bc31` (76/243) | 5 most-called unmarked fns |
| WallCrawler (worker) | `ses_ea3206eacffe4RdcKDUk7WwTq0` | wallcrawler @ `e7a84a386` (accepted for batch 7: 1310 AlignToFloor fixed — bogus obstruction scan removed, best starts at radius; flag bits renamed to retail masks; 32 unmarked all cited as REL copies) | 1310 constrained arm (x834≠0): shipped placements set it? keep/unmark marker; then module done |
| EmperorIng1 (worker) | — (done) | emperoring @ `5a594830e` (91/241; 140 cited; 4954 needs FSM1 code-node dispatch, 7EA8 no tentacle sound event; 8 fns between survey 233 and port_progress 241 unexplained) | merge in batch 6 |
| MinorIng (worker) | — (done) | minoring @ `c8d342614` (128/217; 28 dead, rest cited; 204C query-success arm never ran: path query returns kR_NoSourcePoint in area 13, check it is not a port bug) | merge in batch 6 |
| Inglet fix (claude) | `ses_ea4b8efdcffeJYrVuvmgGX4q6a` | merged to port as `2294724f1` (CFoldySurface byte-swap) + `67f337a57` (Aurora dstAlpha alpha pass) | done; the Aurora pass touches every blended dstAlpha draw: include in the next smoke. Puddles are unscannable in retail too |
| DarkSamus (Opus) | `ses_ea312bc93ffecMAU928oeEeY7P` | darksamus2 @ `c391309aa` (34 markers; slice 9 accepted: Dash chain ported, marked 12C2C 12CE0 14C50 14E5C EB90 EDE0; 150DC/154FC/14A00/14D14/14FE0/16048 ported unmarked, need a >1.2 s stuck run) | null-test cites at Fsm 1302/2558/2746; 19B64 tree: 16418+164C0, 16600+166AC+16ADC, 12B38, then 19B64 |

## 2026-10-20 (thirtieth session): the scan visor's highlights flashed - one shared copy destination

The report: "the scan visor overlay colours still are not perfectly right"; asked which part, "highlights
on scannables"; asked how, "they flash".

- **What flashed.** The visor's palette-index map (an A8 viewport copy sampled as a C8 texture through
  the scan palette) alternated between the real indices and the *scan window pane's* RGB565 bytes, so
  the highlight tint on scannables changed from frame to frame. Measured with a new
  `MP2_DEBUG_SCAN_PALETTE=1` check - every populated index is `paletteIndex << 2`, so any index that is
  not a multiple of four is foreign data: **~34,500 of 356,608 bytes on roughly half the frames** while
  the visor was up; 0 on the others.
- **Three causes, each fixed:**
  1. `PortEfbCopy`'s staging slots were keyed by the destination pointer alone and had their geometry
     overwritten by every `Request`, so the visor's `Fill` could consume the pane's completed read-back
     (two copies into the same buffer per frame: `796x448 tx=1` and `208x92 tx=2`, both logged).
     A slot is now the whole copy identity - shared pointer, size, texel size - with its own staging
     buffer, and `Fill` takes the texel size too (from the destination format in
     `LoadDolphinSpareTexture`).
  2. **Aurora resolves a copy into a texture keyed by the destination pointer.** Both consumers copied
     into `CGraphics::GetDolphinSpareBuffer()`, so whichever copy was recorded last owned the resolved
     texture for both - the reason the pane showed the visor's indices as noise and vice versa.
     `PortEfbCopy::Destination(shared, w, h, texelBytes)` now hands each consumer a stable private
     destination: the scan visor's A8 viewport copy (`CCubeRenderer::DrawScanVisor`) and the window
     pane's RGB565 copy (`CPlayerVisor::DrawScanEffect`) `GXCopyTex` into it, `Request`/`Fill` it, and
     bind their texture from it.
  3. A first attempt bound the texture to a *shadow copy* while the copy still landed in the shared
     buffer; Aurora then read the linear guest bytes as tiled C8 and the overlay became diagonal
     stripes (`/tmp/opencode/final_22.png`). That is why the fix is a per-copy *destination*, not a
     shadow of the bytes.
- **Verified on the desktop** (`Xvfb :86`, `tools/run_gameplay.sh`, `MP2_INPUT=LEFT@600+4`, plus a
  turning-camera and a holding-`L` run): `bad=0` on every logged frame, no dropped slots, no torn
  reads; screenshots show the pane's magnified scene clean (`/tmp/opencode/vf_30.png` - it had white
  streaks before) and the highlight stripes steady in colour. Gates: `python3 tools/gen_files_cmake.py
  --check` rc=0, and the upstream matching build after the three upstream-file edits -
  `main.dol` sha1 `6ef9b491d0cc08bc81a124fdedb8bfaec34d0010`, Game Code 94.14% fuzzy / 63.29% matched
  (unchanged), every edit inside `#ifdef TARGET_PC` with the non-PC statements preserved verbatim.
- **Still open, same class:** the camera filters, `CPlayerGun` and the particles also copy into the
  shared spare buffer without a `Destination`, so they can still take each other's resolved texture
  (unchanged by this session, and no longer able to reach the visor's or the pane's copies). Fold them
  into `Destination` the same way when one of them shows the symptom.

## 2026-10-20 (twenty-ninth session): the F1 Teleport and items pages, Prime 1's shape

- **This commit:** the F1 overlay gets MP1's two debug pages, in this game's terms.
  - **Cheats page, Teleport block** (where MP1 keeps it, below the items; the Game page keeps
    this port's own area/world cycle buttons). A room combo (`IGetAreaAlways` names, so unloaded areas are
    named too; slots with no MREA are hidden) with a filter field, `Reload` (a world warp to the same
    world and room - the area warp deliberately ignores a request for the room it is already in),
    and a world combo built from the memory card's world table (`CSaveWorldMemory::GetFrontEndName`,
    with the GuiSys markup and the hand-placed line break stripped: the entry is
    `&line-spacing=75;Temple\nGrounds`).
  - **Cheats page, items.** Echoes' abilities and keys as checkbox grids, missiles / power bombs /
    dark and light ammo / energy tanks sliders bounded by `GetPowerUpMaxValue`, `Full health` and
    `Grant everything`. A grant sets the capacity to the game's own maximum for a counter (1 for an
    ability, 0 to remove it) and the amount to the slider's value: `AddPowerUp` for the capacity
    difference, then `SetItemAmount` for the amount. That is MP1's `SetPowerUp`/`SetPickup` shape.
    A first version moved both fields by the amount's delta, and a review caught what that does:
    sliding missiles from 255 to 55 lowered the capacity to 55 and left the amount at 255, and
    unticking an item whose amount was already 0 did nothing.
- **The first version kept a `CWorld*` and segfaulted on a world warp from the menu** - the
  outgoing world is freed while the overlay still draws the frames before the restart. The menu's
  own world warp is what found it (`[warp] 3bfa3eff: the outgoing world's pak stays readable`, then
  a fault); the console's `world` with the overlay closed survived, which is why it looked flaky.
  The room list is now a snapshot copied in `SetGameState`, keyed on `CWorld::IGetWorldAssetId()` -
  not on `gpGameState`'s id, which names the destination on the frame the warp is served while the
  outgoing world is still the live one (that key left Torvus Bog's page showing Temple Grounds'
  rooms).
- **Measured** (run on Xvfb :441, `MP2_SKIP_FRONTEND=1`, a copy of the `/tmp/opencode/locktest`
  card): the room picker lists `0  01_Temple_LandingSite` … 62 areas of Temple Grounds, and picking
  one warped (`[warp] area 9: loading` -> `[area] 9` -> `the game's own transition arrived in area
  9`); the world picker warped to `3dfd2249` and the page then read `0  00_Elev_Swamp_Temple_B`;
  `Grant everything` moved health to 1499/1499, missiles/power bombs/dark ammo to 255/10/250 with
  every box ticked; clicking the missiles slider at a quarter then put `55 / 255` on the HUD, and
  unticking and re-ticking Power Beam followed in the grid. Screenshots `/tmp/opencode/final_game.png`,
  `final_cheats.png`, `p2_torvus.png`, `r3_hud.png`.
- **Pages.** The F1 window now has the Prime 1 port's page set, as far as this port has the
  features behind each: Game (state and the area/world cycle buttons, Speedrun, Cutscenes),
  Controls, Video, Cheats (items then Teleport), Save states, Randomizer, System, Debug. MP1's
  Remastered, Mods, Archipelago and Tracker have no feature here, so no empty pages. `Debug` holds
  the free camera and photo (moved off Game), the world/area and game-update readout, and the
  console's commands; `Save states` is the old Game block on a page of its own; `Randomizer` reports
  the loaded seed and writes one from the dump log (`PortRandomizer::SeedFilePath`,
  `DumpLogFilePath`, `GenerateSeedFromDumpLog`).
- **The randomizer's user directory was the Prime 1 port's** (`MP_USER_PATH`, else SDL's pref path
  for "Metroid Prime"), so a dump or seed went somewhere the port's card and settings never look.
  It now reads `MP2_USER_PATH` first; the Randomizer page showed `/tmp/opencode/ov_user/...` under a
  run with that variable.
- **Layout.** The first version split the two: the warp pickers on the Game page and the items on
  Cheats. MP1 has both on Cheats, in the order health/grants, abilities, counters, Teleport, and
  this now does too (the port's `Previous/Next area/world` buttons stay on Game).
- Not covered: the Android page layout, and the items page in the front end beyond its
  "Waiting for gameplay..." guard.

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

The openchamber tool rejects every non-string argument (2026-10-20), so lanes run as subagents now; old lane sessions can't be resumed from here.
