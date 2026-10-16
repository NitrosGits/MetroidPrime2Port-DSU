# Port layer

Carried over from the Metroid Prime port (`../MetroidPrimePort/platform`) as the
adaptation baseline for Echoes. `compat.h`, `include/dolphin/` and the three
sources in the `mp_platform` target are in use; the rest is listed with what it
waits for. See `../PORT_NOTES.md` for the measured SDK link gap.

## In use

- `compat.h` — force-included ahead of every C++ game translation unit. Restores
  the SDK macros Aurora omits (`AUTO*`, `nofralloc`, `__abs`) and maps the game's
  `triggerL/R` onto Aurora's `TARGET_PC` `PADStatus` members. C++-only.
- `include/dolphin/` — SDK headers Aurora omits: `arq.h`, `gba.h`, `PPCArch.h`,
  `thp/`, and `gx/GXShims.h` (GX entry points Aurora does not provide).
- `shims.cpp`, `sdk_stubs.cpp`, `glibc_compat.c` — GX/SI/PAD/PPCArch shims, stubs
  for the SDK entry points Aurora lacks (17 of the 20 the compiled game needs),
  and the glibc floor. Built as `mp_platform`, so they stay compiled and verified
  against Aurora's headers even while the game cannot link.
- `entry.cpp`, `include/port_entry.h` — which disc the port accepts and how it
  finds the image. Game-free, so it is covered by `port_entry_tests`.
- `main.cpp` — the process entry point: Aurora up, disc mounted and checked,
  `InvokeCMain` called, platform down. Compiled by `mp_port_entry`; it cannot link
  until upstream writes `COsContext` and `CMemorySys` (see `../PORT_NOTES.md`).
- `rel.cpp`, `include/port_rel.h` — the REL module runtime (arena, loader, linker,
  registry), tested against a module extracted from an owned disc.
- `port_events.cpp` — the game loop's per-iteration Aurora poll (built by
  `mp_port_audio`, since it needs SDL). It installs the port's keyboard map and
  drives pad 0 from `MP2_INPUT` scripts.
- `port_warp.cpp`, `port_harness.cpp` — the warp request slot and the test harness's
  run length (`MP2_FRAMES`), both built by `mp_port_audio` and served by the game in
  `CStateManager`; see "Warps and the debug overlay" and "Test runs: skipping the
  front end" below.

## Input

Aurora reads a real controller by itself, but its keyboard map is empty
(`g_defaultKeys` is all `PAD_KEY_INVALID`), so a machine with no pad reported
`PAD_ERR_NO_CONTROLLER` and the game got no input at all. `InstallDefaultKeyBindings`
in `port_events.cpp` installs the Prime port's layout once at startup:

| Key | Pad | Key | Pad |
| --- | --- | --- | --- |
| WASD | main stick | X / Z | A / B |
| IJKL | C-stick | C / V | X / Y |
| Q / E | L / R (also the analog triggers) | F | Z |
| arrows | D-pad | Enter | Start |

Keyboard state merges with a connected controller's and with `MP2_INPUT`'s virtual
pad, so neither replaces the other. There is no remap UI yet (see `debug_ui.cpp`
under "Waiting").

## Mouse look

`MP2_MOUSE_AIM=1` hands the aim to the mouse; `MP2_MOUSE_SENS` sets radians per
pixel (default 0.0035, as in the Prime port) and `MP2_MOUSE_INVERT_X`/`_Y` flip an
axis. The platform side is `port_mouse.cpp` (built by `mp_port_audio`, because it
needs SDL): it owns SDL's relative capture and the aim angles in
`include/port_mouse.h`. The game side is a `#ifdef TARGET_PC` hook in two files,
the same shape as the Prime port's:

- `CPlayer::UpdateMouseAim` turns the player body to the mouse yaw, so movement
  stays body-relative and the gun (which follows the camera) points where the
  crosshair is. It runs in `Think`, after `ProcessInput`, whose stick turn would
  otherwise win the frame. Nothing there sets `mInFreeLook`: that flag stops
  movement and changes gun animations, and the mouse should not pretend that R is
  held. `MouseAimAllowed` gates it - first person, playing, unmorphed, unfrozen,
  no lock-on, no cinematic - and while something else owns the view the aim
  tracks the camera direction instead, so re-engaging does not snap.
- `CFirstPersonCamera::UpdateTransform` uses the aim for the view direction,
  bypassing the free-look clamps (retail limits how far the C-stick travel may
  go), skips the jump/fall pitch-down, and rebuilds the gun-follow basis directly
  from `lookDir` so pixel deltas get no controller-style easing.

Verified on Xvfb by injecting relative X pointer motion (`/tmp/xkey mouse 400 0`):
the view turned, pitch followed the mouse down, and walking afterwards went in the
new facing. It is opt-in until it has had more play time; a pad or the keyboard map
stays the default.

## Warps and the debug overlay

`port_warp.cpp` (with `include/port_warp.h`) is a one-slot request queue; the game
serves one request per update in `CStateManager::PortWarpTick` (`#ifdef TARGET_PC`),
because only the game can read the resource factory, the world and the memory
card's world table:

- an **area warp** does what an area load does - `TravelToArea(kATT_SkipAdjacent)`,
  the `WorldLoaded` script messages, the first active spawn point through
  `SpawnPlayer`, `ResetCameras` - so it lands the player where the area expects;
- a **world warp** is `CScriptWorldTeleporter`'s restart: pause the outgoing world's
  loading, `gpGameState->SetCurrentWorldId` + `SetDesiredAreaAssetId`,
  `gpMain->SetRestartMode(kRM_None)`, `QuitGame()`, and CMainFlow loads the new world.

Requests come from `MP2_WORLD="<world>[,<area>]"` and `MP2_TELEPORT="<area>"` at
start-up, the F5/F6 (area) and F7/F8 (world) hotkeys, the `MP2_INPUT` tokens
`NEXTAREA`/`PREVAREA`/`NEXTWORLD`/`PREVWORLD`, and the F1 overlay's buttons. World
and area text is an asset name or a hex asset id. The same slot also carries the gameplay-test
tokens `KILL` (the state manager's own kill, so a run reaches death and respawn), `HEAL`
(refill the player's energy, so a sweep survives a damaging atmosphere) and `LAYER<layer>`
(activate that layer of the current area, which is how a module whose actors live in a layer
other than 0 - every one of ElitePirate's five areas, for instance - becomes visible at all:
`ActivateLayerDynamic` runs the layer's script objects, and the layer's data has to stream in
first, so the request is finished over the following frames).

### What a transition has to get right

Walking through a doorway works like this in the game, and a warp has to follow it:

1. The player touches a `CScriptDock` (`Touch`), which starts the destination area
   loading and moves the dock's state machine to `kDS_EnterNextArea`.
2. When the player crosses the dock's plane **and the destination area reports
   `IsLoaded()`**, the dock calls `SetCurrentAreaId(nextArea)`. That also moves
   `mNextAreaId`, which is what the world update travels to every frame.
3. `CStateManager::Update` ends with `TravelToArea(mNextAreaId, kATT_LoadAdjacent)`,
   so the destination becomes the current area and the previous one is queued for
   unloading.

A warp that changes the area id *before* the area is loaded breaks two ways: the
update calls `Area(GetNextAreaId())->UpdateDocks`, which segfaults on an area that is
still streaming, and the per-frame travel then drags the world back to the old area
while the warp drags it forward, which makes the loop crawl. So `PortWarpToArea`
starts the load (`ScheduleAreaToLoad` + `StartStreamIn` every frame until
`ScheduleAreaToLoad` reports done, exactly like `CStateManager::InitializeState`)
and `PortWarpSettle` moves the player only once `IsLoaded()`.

The area's script objects allocate their object ids *as* the stream runs, so the free-id
floor `PortWarpSettle` applies only guards **starting** a stream: a warp that withholds
`StartStreamIn` from a stream already under way stalls the load forever and keeps the ids
the area has already taken (a walk of world `1baa96c2` stopped there with 600 of the 1024
ids held by a half-loaded area). Once a stream has started, the only thing that gives it
room is dropping the areas the warp left behind, so `PortWarpSettle` does that while the
stream runs rather than only before it starts.

Two more things the warp exposed, both fixed in the game files:

- `CAutoMapper::GetDesiredMiniMapCameraDistance` dereferenced `GetMapArea()` results
  that are null until an area's map data is resident; a warp can arrive first, so the
  port skips areas whose map is not there yet.
- An area slot with an invalid MREA is not a real area (a world can have many more
  slots than areas); `CycleArea` skips them, and `PortWarpTick` prints the world's
  area list once so a sweep can see which slots are real.

`ATDOOR` (F9) stands the player in the nearest doorway facing through it, and `DOOR`
stands them in front of the nearest door: with `SUP` to walk and `A` to shoot, those
two tokens exercise the real door-then-dock path instead of the warp's shortcut.

`port_overlay.cpp` is the F1 window (ImGui, which Aurora sets up; `mp_port_audio`
links `aurora::core` for its headers). It draws inside the open Aurora frame from
`DolphinCGraphics.cpp`, and every control in it is wired to something that exists:
the warp buttons, mouse aim with inversion and sensitivity, the keyboard map, and
the current world/area, which `CStateManager::Update` publishes every update. While
it is open the cursor belongs to the window (`PortMouse::SetUiActive`).

## Test runs: skipping the front end

`port_harness.cpp` (with `include/port_harness.h`) is the test harness's run length,
and the game-side half of it lives in `src/MetroidPrime/PortSkipFrontEnd.cpp`:

- **`MP2_SKIP_FRONTEND=1`** boots straight into the memory card's save instead of
  playing the front end's menu. It reads the save itself through
  `CMemoryCardDriver` (the same read the file select panel performs through
  `CSaveGameScreen::StartGame`) and leaves it where `CMain::StreamNewGameState`
  expects it, then starts the game the way retail's single-player start does.
  `MP2_SLOT=<0|1|2>` picks the slot; without it the slot the card's own header
  points at is used, falling back to the first slot that has a save. The log's
  `[skip]` lines name every slot and say where the run is. Any failure - no card, no
  file, an empty slot, a read that never settles - falls back to the front end with
  a line saying so, so a skipped run is never a stuck one.
  It also dismisses the boot's splash chain, which otherwise blocks the flow until a
  button is pressed or the health-warning screen times out: measured, the flow
  reached `CPreFrontEnd` at 56.6 s without a button press and at 1.1 s with the
  splash dismissed, and the save's first area was loaded and reached at ~15-20 s
  where the menu path needs ~65 s.
- **`MP2_FRAMES=<n>`** ends a run cleanly once n *game* updates (`CStateManager::Update`)
  have happened: the game loop takes its own quit path, Aurora shuts down and the
  process exits 0, so a log's last line is the run's verdict instead of the script's
  `timeout` (which stays in place as the backstop for a run that never gets there).
  `[harness]` lines report the limit and the world/area the run ended in.

A load test is the two together, e.g.

    MP2_SKIP_FRONTEND=1 MP2_WORLD=1baa96c2,b666b655 MP2_FRAMES=800
    MP2_INPUT="LAYER3@4200+4" ./metroid_prime2_port

which now costs tens of seconds rather than a menu's worth of scripted button
presses. `[harness]`/`[skip]` lines carry the wall seconds since the loop's first
iteration, so a log says how long each phase took.

## Reworked for Echoes

- `sdk_stubs.cpp` — the carried-over `OSLink`/`OSUnlink` no-op stubs were removed
  (`port::rel::LinkModule`/`UnlinkModule` own that job now), and the three OS
  context entry points Echoes references were added.
- `main.cpp` — rewritten against Echoes' bootstrap: it drives the decompilation's
  `InvokeCMain` seam instead of Metroid Prime's `metroid_main`, and builds the OS
  context and memory system that seam takes.

## Waiting

- `disc.cpp`, `include/port_disc.h` — reads embedded DOL resources by console
  address (Metroid Prime's font fetch). Echoes' resources are LZO/zlib-compressed
  and split across `main.dol` and the REL modules, so this needs the game's
  resource model, not just the DOL section walk.
- `ai_dma.cpp` — the AI DMA bridge; needs SDL3, which arrives with Aurora's build.
- `debug_ui.cpp` — the F1 overlay. Reads `CStateManager`, `CWorld`, `CPlayerState`,
  `CGameState` and `CMemoryCard` and the MP1 debug hooks (`port_debug.h`).
- `port_textures.cpp`, `port_prompts.cpp`, `port_randomizer.cpp`, `smoke.cpp` —
  HD textures, button prompts, randomizer and the opt-in smoke driver. All
  Metroid-Prime-specific; see the Prime port's `docs/NATIVE_PORT.md` for what each
  does, and expect to rewrite them against Echoes' assets and UI.
