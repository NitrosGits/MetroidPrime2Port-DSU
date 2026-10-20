# Session status (detail)

The Status section of AGENTS.md links here: this is the per-session detail, rewritten rather than
appended, dated as in the heading there.

## 2026-10-20 (port orchestrator): lanes in flight

Read a lane's result with the openchamber tool (`session.messages`, `lastAssistant`), review with
`tools/review_decomp_diff.sh <base>` from its worktree, accept, then send the same session its next slice.
Worktrees: `~/.local/share/opencode/worktree/3e7f972e5f2a67a4bf166b19e99a591913d8d4a4/wt-mp2-*`.

Merged into port (eb3252ed4, smoke clean on all 8, 951 markers, `60a51e2d8`): swampboss, sandboss,
wallcrawler, boostguardian, digitalguardian, commandopirate, emperoring, spacepirate. Smoke logs:
`/tmp/opencode/smoke/`. Smoke notes: EmperorIngStage1's actor is now `u04e3` (docs say u0687);
DG needs `send u054d Activate`; IBBG runs with `send u052b Activate`.

| lane | session | branch / base | task |
|---|---|---|---|
| Ing (worker) | `ses_ea67c8a48ffeGHDnEm7Su7uOCv` (subagent) | decomp/ing @ `618c7e8f5` (rework accepted) | reached stand-ins <=0x100 |
| CommandoPirate (worker) | `ses_ea68aa796ffeYgAi8HQZWbsMMM (subagent)` | decomp/commandopirate @ `317c2f449` | fn_9_662C timer + stand-ins 0x100-0x200; run with the player on the linked nav region |
| EmperorIngStage1 (worker) | `ses_ea68aa795ffe7vd2PvE2JI3Mwo (subagent)` | decomp/emperoring @ `e1f34df29` | area-load stand-ins fn_16_2B68 / 1E6C / 3984; fn_16_3EAC (0x794, collision managers) reserved for Opus |
| EmperorIngStage3 (Opus) | `ses_ea61d5880ffejeb3NNt1B4Zpl5` | decomp/emperoring3 @ `ca6b8278a` | body-command classes (data_744/750/78C) + state update paths, fn_18_3BB0 heart holder, onward |
| SwampBoss marking (worker, ex-EmperorIng3 lane) | `ses_ea68aa793ffdPJMI0HqWQM1mSC` (subagent) | decomp/swampboss @ `59637cee5` | marking pass with port's asmcheck; list mismatches, no code changes |
| DigitalGuardian (worker) | `ses_ea68aa793ffeDxOmT3IprQYLiP (subagent)` | decomp/digitalguardian @ `344f8e7d0` | fn_14_6864 model build, ctor locator ids, renderer slot 0x44 |
| DarkSamus (worker) | `ses_ea67ca29affeP6zi3urmpO5e6C` (subagent) | decomp/darksamus on `232823309` | milestone 1 (old lane did nothing) |
| ScriptPlayerTurret (worker) | `ses_ea67c9e47ffeXetyJZpm95C0H6` (subagent) | decomp/playerturret @ `1402494b4` | XCRT/XDMG/STOP cases, ProcessInput 1134; Think 23C0 (0x968) parked for Opus |
| EmperorIngStage1 (Opus) | `ses_ea6700b83ffeiYYMVx9wazyiAs` | decomp/emperoring @ `046b710dc` (TrackPlayer/Attack/heart states in; walk ends at ProjectileAttack stand-in) | tentacle motion 26B8/2524/2070, ProjectileAttack 68F4, FireBeam CD0, onward to the heart |
| IngBoostBallGuardian (Opus, ex-Agon lane) | `ses_ea66ff706ffemgWrJxsGl8iAxE` | decomp/boostguardian @ `9787cc26a` (Think/timers in; fight cycles, takes damage, phases) | data +0x08/+0x14 layout + IngSpotPathFind, 0x5E0 class + fn_30_3478 (BoostAttack), retreat 2F9C |

Merged into port 2026-10-20 (`171940cc5`, host+link+upstream green, smoke clean 15 runs `/tmp/opencode/smoke2/`, 954 markers `2015f13d5`; Ing now takes 300->260 from a beam shot in form 3 vs 300->180 before - check when Ing's lane reports): ing `618c7e8f5`, playerturret `1402494b4`,
emperoring `87f405931`, commandopirate `b53f44229`, spacepirate `a9ba3f157`, emperoring3 `528919371`, swampboss `59637cee5`,
digitalguardian `344f8e7d0`, fix/agon-black `3e6ac419c` (Aurora partial copy-clear, upstream aurora 9c0bf66f1e; keep on re-vendor).
Accepted, not yet merged: emperoring3 `ca6b8278a`, emperoring `046b710dc`, boostguardian `9787cc26a`, commandopirate `317c2f449`.
Merge by accepted hash, not branch tip: `/tmp/opencode/merge/merge_hash.sh <hash>...`.
Lane-review lessons: bytes inside the loader data block are mData fields; a guest offset a module only
reads, below its own fields, is a base-class member; a stand-in returning false fakes FSM progress; a raw script-message constant must be re-read from the lis/addi pair (0x41544348 kSM_Attach was misread); `FastCosR(const)` then `fcmpo dot, f1` means dot > cos(const), not cos(dot) > const; guest 0x6B4 (stores of 0/1/3) is CPatterned::mAnimationState, 0x158 mMass, 0x1C0 mConstantForce.


Later in the day (port orchestrator, Opus): the five MiniMax family orchestrators were stopped and the
watchdog `8dfd61bf` disabled; the port session runs every family. Two Opus bit-decoding audits found
and fixed: SpacePirate `Leash` 52F8 (cr0 GT, `8a9fbb50c`), `Ambushing` 7630 -> `x8f4_25_ceilingAmbush`
and `CheckBlade` CC80 -> `x8f8_25_appliedBladeDamage` (`c412a6e5f`); Ing `AcceptScriptMsg` C9A4
(`8efb0e851`), PreRender C010 / Render C0B4 unmarked (`08c486cbf`, bodies incomplete);
IngBoostBallGuardian's data byte 0x320 packing + every reader (`c417f150f`, follow-ups in its report:
locomotion types look shifted by one, IngSpotPathFind speed selection, `fn_30_30E4/3190/3068` calls).
ElitePirate and Sandworm audited clean (396 fns / 626 bit ops). DigitalGuardian+Splitter milestone 1
(`ec8f933af`, loaders + Think, reviewed). D800's `xa78_ < 0.5f` is correct (data +0xc4 = rodata B44).


- Scan visor highlights no longer flash: each EFB copy gets its own destination (`PortEfbCopy::Destination`) because Aurora keys a resolved copy texture by the destination pointer, and the staging slots are keyed by the whole copy. Remaining same-class gap: camera filters/gun/particles still share the spare buffer. Detail: `docs/STATUS.md`, `docs/GOTCHAS.md`.
- `port` at `e6833bcfc`: all worlds load; WallCrawler is retail (22/57 marked); warp by area name. History: `docs/STATUS.md`.
- Decomp families (port orchestrator + MiniMax `worker` lanes, own worktree + `decomp/<family>` branch): SpacePirate (main tree; the trigger work is complete and committed - port commits `6c8e91bcb` `0af474a2f` `33a4f2ae1` `e2194e6b5` `fbd2b95fc`, doc `58dd3a5b8`; **no stand-in is left in the trigger or state files** - all 39 triggers and all 37 states are retail's; 27/262 marked. 14 vtable-override stand-ins are still in `PortCSpacePirate.cpp` (Attach/Detach, PreRender, Render, GetDamageVulnerability x2, SetAttackTarget, ...). The last 12 trigger stand-ins went in on 2026-10-20 (`6c8e91bcb` `0af474a2f` `33a4f2ae1` `e2194e6b5`): `ADC0` `ABC0` `AA64` `A988` `A8D0` `99A4` `7970` `767C` `602C` `55B8` `558C` `52F8`, of which `ADC0`/`7970`/`55B8` are marked (the 208 cycle evaluates them, always false there). Retail readings that differ from MP1: `ABC0`/`AA64` test `GetTargetPos()` (not the player), `A988` casts the *attack target* to CPlayer, `7970` uses `MakeExclude(kMT_Solid)` and breaks at the first targeting point, `602C` scores `distSq + |mDestPos-wp|^2 + 9*dz^2` and breaks at the first candidate that reaches the destination; `ADC0` *pushes* its id into the module's charge list when it is not in it. Details: SpacePirate.md §6/§7. Next: nothing open in this module - the 14 vtable-override stand-ins in `PortCSpacePirate.cpp` are the only leftovers and are not required by the live cycle; the port orchestrator (this session) now runs every family itself, so pick the next family from the list), ElitePirate/Ingsmasher (wt-mp2-elitepirate, `decomp/elitepirate`; stands/fights, 97/241 marked; next: docs/module-analysis/ElitePirate.md §5 "Next"), Ing+MinorIng (wt-mp2-ing, `decomp/ing` at `526ca2795`; CIng arm swipe (`fn_29_6A88`/`3E1C`/`421C`/`3D74`), suck `76B8`, splat `4F14`, the whole exit host, BodyProjectileFlight `7E30` + GrappleBall `6578`/`4D18` and the five path functions are ported and marked. Probe recipes are what unlock the FSM: area-57 `aim <yaw> <pitch>` for the puddle, and Agon `01_sand_entrance_dark` with `layer 2`/`layer 3`, `press x` to morph (the morphIntoBall tweak is function-list 17, not B) and `tp` onto the puddle for the grapple. Two retail-shaped bugs this found: the cover scan `fn_29_6DA0` filtered hint types 4/5 while retail wants 3/4, which kept every puddle out of `IngSpotPathFind`, and the ball triggers `A2D8`/`A100` need both 0x1144-adjacent player fields (spawned state 0x390 then transition 0x38c). **101/274** since 2026-10-20 (`526ca2795`): the fail-safe question is answered - `FailSafeMode`'s teleport *does* move the `0233` Ing (5.38 units to point-graph node 329 of 470) and it stays there, so the `Lurk -> SetRecoverDest -> FailSafeMode` loop is the level's, not the port's (that node is off the ground mesh and has no graph links); the nine remaining minimal states and the eleven triggers are read against the asm and marked (locomotion type 0 = `kLT_Crouch`, `SafeZoneReaction`'s emitter volume/priority, `Patrol`'s walk/run speed ratio, `Dead`'s base-class call + the two burn flags, per-axis model-scale boxes), and `SetRecoverDest`'s second point query now uses the first query's index as its connected point (asm 0x00005A2C). A new script (`/tmp/opencode/ing_triggers.py`) computes which of the machine's 51 triggers a run evaluates: 43 did, the 8 that never fired are listed in IngFamily.md §A.15. Killing an Ing needs three `damage <uid> 100` hits, not one big one, or `CPatterned::Death` takes the massive-death path. Next: the two Ing classes that are still unported - `CIngMiniPortalAttack` (`11570` aim, `11840` fire, `11CA4` Think, `11A7C`) and `CIngExitHostEffect` (`1105C` ctor, the bezier path `10154`/`1046C`/`1065C`/`1077C`, the particles `10944`/`10DEC`/`10EB4`, the Think `10F58`); see IngFamily.md §A.14-A.17), Sandworm (wt-mp2-wallcrawler, `decomp/wallcrawler` at `c837737b1`, do not merge; **262/283 marked and the other 20 all carry a reason**: 4 data-gated bomb bodies, `C350`/`C3D0` never asked by either prefab, `C828` blocked (u048f, the only worm with the underground bit clear, never leaves Start/Prime/Snatch, so `SetUnderground` never runs), and 13 dead slots - no caller (`14AE4` `14B34` `14B3C` `14A6C` `2210` `10A20` `534C` `10A68` `6324`), no jump/hurled body state (`14B10`), the worm pattern is never in a projectile's near list so `GetRagDoll`/`CanBeShot` (`14AEC` `14B24`) never run, and the knock-back rules never pick kFU_ExplodeDeath so `GetDeathExplosionParticle` (`14B1C`) never runs. Bombs are CWeapon, not CGameProjectile; a bomb kill works. Nothing open: pick another module - Sandworm.md Port status is the evidence index), and Sandworm is finished), IngBoostBallGuardian (wt-mp2-boostguardian, `decomp/boostguardian` @ `d6d1c6265`, do not merge; **milestone 1 is in** (`1e753b70e`, 2026-10-20): the REL's loader `fn_30_130`, constructor `fn_30_EC6C` and the one-line vtable accessors are host code in `PortCIngBoostBallGuardian.cpp`, every AI override still forwards to `CPatterned` behind a once-only `[ibbg]` stand-in log. In area 28 (`Torvus Bog/06_swamp_boostballguardian_dark`, layer 5 'Boss Battle' already active at load) the actor registers, takes `kSM_Create` and runs PreThink/Think each frame; `[port] caught|reach-stub` 0, host build green and the matching build still 87 files OK. Shape: 0x11F0, `CPatterned` character 0x19 (the only free DOL tag, 117 = `kET_IngBoostBallGuardian`), one `CModelData`, both blocks taken straight from the already-generated `SLdrIngBoostBallGuardian` (the guardian block is a verbatim copy of `SLdrIngBoostBallGuardianData`), 49 triggers / 28 states / 9 codes copied into `.bss` by a `.ctors` function with the transitions in the FSM2 asset, and an unidentified 0x5E0 `CPhysicsActor` class that `fn_30_3478` spawns. Next: the `MP2_BG_FSM` probe over `CGenericFSM2` to dump the transitions, then the Ing half of the states from `PortCIng.cpp`/`IngFamily.md`; `docs/module-analysis/BoostGuardian.md` sections i and k are the index), SandBoss, SwampBoss (`decomp/swampboss` @ `f97561271`: Stage1+Stage2 bodies committed; Stage1 105/251,
Stage2 74/243). Stage1: phase 0 only asks for the splash cycle, so drop it below 600 HP (`damage`
caps at 580/call) and park the player on the arena `CScriptAIHint` - that walks Beach/Bubbles/
Wait/Rise, Dive, PlatformReady, TongueLoop/Pull, Vomit, StopBeaching, Null, ResetPlatform, Dead.
Stage2: the FSM2 probe must print transition *targets*; `Lurk` tests `PlayerInWater` first and
that edge goes to `Taunt`, so keep the player dry and Lurk -> Level -> Swoop -> Circle runs (all
marked). **The beam was never broken - the harness was aiming wrong** (2026-10-20, ~100 sampled
shots): the projectile follows the camera's aim exactly and a shot that touches a joint is never
rejected (6/6 hits produced a `[sb1dmg]` line), but the joints swim with the larva at 22-28 u/s
and the muzzle is still 2-11 u/s off the camera's aim line right after a teleport, so every shot's
line passed 1.8-4.4 units from the joint and only ~1 in 100 landed. Recipe: aim at the joint's
*predicted* position (`p1 + v*T`, `T = 0.60 + 0.12 + D/120`), wait 0.60 s for the gun's follow
transform to settle, then `beam power` + `press a 60` (that fires a 2-damage shot at once and a
50-damage one on the release), and correct the aim by the measured line error. A phase-1 run with
that recipe landed 5 joint hits (14-15 damage each) and took the belly accumulator to 16.6 of the
50 it needs, so `ShouldExposeBelly` now runs - ExposeBelly and the two hit reacts still need
~3 more charged hits, and Stage 2's growth hit needs the shot's direction within 90 deg of the
boss's facing. Stage1's belly threshold is 50 (Ldr `unknown_0x78755da3`), a heavy hit is 15 and
adds 180. Console `aim <yaw> <pitch>` came from `decomp/ing` `6ad804a12`, committed here as
`4d1864f8e` - merge `decomp/ing` once, not twice - SwampBoss.md).
- SandBoss (`decomp/sandboss` @ `f81d88dcc`, do not merge): 155 `// @`, no stand-ins, whole FSM walked
  in area 36 (`LAYER5@400`): stampede -> attach -> `Attack` (SnapJaws, RepeaterAttack, DarkBeamAttack,
  Double/TripleCharge) -> `Damaged` (SuckAir/SuckBall/BallInMouth) -> destroy/jump/dead chain
  (DestroySphere B2D8 + helpers, JumpOffSphere B06C, Dead BDAC, Deactivate BD30) with the round chain
  (`kSM_Increment`), plus `1D74`, `8F94`, a real bomb/beam hit on the armour (8258 7E60 8008).
  `MP2_SB_ROUND=<0..2>` decides how many worms attach; the probe scripts are
  `/tmp/opencode/sb7_run.sh` (all knobs; exports a knob only when non-empty — an empty
  `MP2_SB_HP=` used to read as on and faked a stampede-HP bug). Open: the scan-visor render path
  (CB30/CC84 + 89E8/88D0/8740) never runs — the worms are scannable but never become the scan
  target (SandBoss.md §5 slice 20); C1CC is unreferenced by the FSM asset. Next: a real-play sanity
  pass of the fight without probe knobs, then another boss.
- Lane lesson: m2c offsets scale with the declared pointer type (`int*` `+ 0x597` = 0x165C) but are bytes for a scalar `int`; `0xD` is kBSC_AbortScripted, NextState `0xC`. `// @` only after a traced run; matching build untouched (94.14%/63.29%). `build/G2ME01/<Module>/asm/auto_*_text.s` is a readable disassembly when the pseudo-C is ambiguous.
- Next bosses: IngBoostBallGuardian, DigitalGuardian+Splitter, DarkSamus, EmperorIng; also CommandoPirate. Pseudo-C: `/tmp/opencode/pseudo/<Module>/` (`tools/rel_pseudo/m2c_module.sh`; jump-table fns fail in m2c; IngSwarm/WallCrawlerSwarm have no asm).
- `upstream` remote URL 404s; repoint before the next sync. Phone APK is stale (rebuild, re-sign, install).

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
