# Quality-of-life features, against MetroidPrimePort

MetroidPrimePort (the Metroid Prime 1 port) is the reference for what "QoL" means here. This is the
inventory: what this tree already has, what to port next, and what is out of scope for now. It is a
plan, not a promise of order - the list is reordered as the port's own walls move.

"MP2" below is this tree; "MP1" is `../MetroidPrimePort`.

## Already in MP2 (inherited from the MP1 port's shapes)

| Feature | Where |
| --- | --- |
| F1 debug overlay with settings, saved to `port_settings.ini` + `MP_*` env overrides | `platform/debug_ui.cpp` |
| Aspect ratio (4:3 / 16:9 / follow window), widescreen HUD, render scale, vsync, 60 fps cap, sim-rate (fixed/adaptive) | `platform/debug_ui.cpp`, `platform/port_*.cpp` |
| Skip / fast-forward cutscenes, cutscene speed, `MP2_SKIP_CINEMATICS`, `MP2_SKIP_INTRO` (level-load text + cinematic scene) | `platform/port_harness.cpp`, `CMFGame`, `CMFGameLoader` |
| Mouse aim, twin-stick aim, gyro aim, full keyboard bindings | `platform/port_mouse.cpp`, `port_events.cpp`, `debug_ui.cpp` |
| God mode, heal/kill, teleport to doorway/door/save station/elevator, area and world warps, layer activation | `platform/port_warp.cpp`, `port_overlay.cpp` |
| `MP2_INPUT` scripted input with warp tokens (the sweep's driver) | `platform/port_events.cpp` |
| Widescreen-aware HUD (`MP_HUD_WIDE`), camera bob used by the game itself | `platform/debug_ui.cpp` |
| Touch overlay (Android) with a layout editor | `platform/port_touch.cpp`, `touch_pad.cpp` |
| HD texture set hook (`PortTextures::DeviceName`) | `platform/port_textures.cpp` |
| **Debug command console over TCP** (`MP2_CONSOLE=<port>`, `tools/mp2con.py`) | `platform/port_console.cpp` - new here |
| Console: `objs` / `obj` / `send` (object inspection and script messages), `visor` / `beam` switching, `tp`, `items` | `platform/port_console.cpp`, names from `tools/gen_console_names.py` |
| Console: `face`, `give`/`take`, `press`/`stick` (pad buttons and the main stick held on the virtual pad), `screenshot` | `platform/port_console.cpp`, `platform/port_events.cpp` (`port_input.h`) |

## Next, cheapest and most useful first

1. **Console, fourth wave** - what is left of MP1's console, and why each waits:
   `warp <world> [area]` (a world warp; `PortWarp::World` exists, the console's `warp` is area-only),
   `strg <id>` (a STRG table by asset id; needs the string-table loader, `gpStringTable` only serves
   the game's own tables), `memo <text>` (`ShowPausedHUDMemo` takes a `CAssetId`, so a free-text
   message needs a table entry or a port-side one), `wait <n>` (the console answers one command per
   game update, so a blocking wait buys nothing) and `cstick`/`gyro` (the sweep's `MP2_INPUT` tokens
   already cover both).
2. **Graphics toggles**: FOV slider (MP1: `PortDebug::FirstPersonFov`, hooked at
   `CCameraManager::GetDefaultFirstPersonVerticalFOV` - here the FPS camera takes its FOV from
   `CCameraManager::mFirstPersonFov`, so a live change needs the FP camera refreshed), hide helmet
   (`CInGameGuiManager::Draw`'s `mHelmetVisMode` gate), hide visor effects
   (`CPlayerVisor::Draw`), arm-cannon offset, MSAA, anisotropic filtering, HUD scale.
3. **Input settings**: rebinding UI and presets (MP1's `port_input_settings.cpp`), plus the settings
   file export/import. The bindings themselves already exist; the editor does not.
4. **Gameplay tweaks** (MP1 names): fast morph, spring ball, toggle lock-on, sticky charge. Each is a
   small game-side change behind a setting; MP2's equivalents live in `CPlayer`/`CPlayerGun`.
5. **Save tools**: quick resume after quitting, save import/export (MP1's `port_gci.cpp`), save
   states (`port_savestate.cpp`, the largest of these).
6. **Photo mode / free camera** (MP1's `port_photo.cpp`, `port_freecam.cpp`): needs a free camera and
   a high-resolution capture path.
7. **Map and tracker** (MP1's `port_map_marks.cpp`, `port_tracker.cpp`): reveal the map, item and scan
   tracking. MP2's map is a different format (`CMapWorld`/`CMapArea`), so this is a rewrite.
8. **Remastered assets** (MP1's `port_remastered_*`): import models/effects from a Metroid Prime
   Remastered dump. Very large; MP2's asset formats are its own.
9. **Discord Rich Presence** (`port_discord.cpp`, needs the RPC library), **Livesplit** integration,
   **randomizer** and **Archipelago** - all MP1 features with no MP2 design yet.
10. **Texture packs / mods** (`port_mods.cpp`, `port_texture_replacement`): Aurora already has texture
    replacement; what is missing is the pack loader and the settings UI.

## Rules that apply to every one of these

- Port-side game changes go inside `#ifdef TARGET_PC`, with the non-PC text byte-identical; the
  upstream matching build must stay at `94.14% fuzzy, 63.29% matched` after each.
- A new setting is three lines in `platform/debug_ui.cpp` (field, `ApplySetting`, save line), one row
  in the overlay, and a getter in `platform/include/port_overlay.h`; the `MP_*`/`MP2_*` env override
  is what a headless run uses.
- Anything a test must drive also needs an `MP2_INPUT` token or a console command, because the sweep
  and the harness cannot press keys.
