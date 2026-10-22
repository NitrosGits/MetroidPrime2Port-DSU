# Quality-of-life features, against MetroidPrimePort

MetroidPrimePort (the Metroid Prime 1 port) is the reference for what "QoL" means here. This is the
inventory: what this tree already has, what to port next, and what is out of scope for now. It is a
plan, not a promise of order - the list is reordered as the port's own walls move.

"MP2" below is this tree; "MP1" is `../MetroidPrimePort`.

## Already in MP2 (inherited from the MP1 port's shapes)

| Feature | Where |
| --- | --- |
| F1 debug overlay: skip cinematics, god mode, mouse/twin-stick aim, teleports, first-person FOV, hide helmet / hide visor effects, arm-cannon offset, MSAA / anisotropy, HUD scale | `platform/port_overlay.cpp`; rows are saved in `<user>/settings.cfg` (`platform/port_overlay_settings.cpp`), an `MP2_*` environment variable wins for one run. The user folder is `$XDG_DATA_HOME/Metroid Prime 2: Echoes` (SDL's preference path) unless `MP2_USER_PATH` names another |
| F1 debug menu, the Prime 1 port's Cheats page (items then Teleport, as there): a filterable room picker (every MLVL area of the world, by name), a world picker built from the card's world table (front-end names, GuiSys markup stripped), Reload; and an items page with Echoes' abilities and keys, missiles / power bombs / dark+light ammo / energy tanks, Full health and Grant everything | `platform/port_overlay.cpp` (Cheats page); the grants are the console `give`/`take` path (`CScriptPickup`'s: capacity and amount together) and the warps are the `PortWarp` requests the console and `MP2_TELEPORT` use |
| F1 pages, in the Prime 1 port's set where the feature exists here: Game (state, the area/world cycle buttons, Speedrun, Cutscenes), Controls, Video, Cheats (items then Teleport), **Save states**, **Randomizer** (status and Generate from the dump log), System, **Debug** (free camera and photo, the live state, the console) | `platform/port_overlay.cpp` (page list in `PortOverlaySearch::kPages`); the Randomizer page reads `platform/port_randomizer.cpp` |
| Aspect ratio, widescreen HUD, render scale, vsync, 60 fps cap, frame interpolation | F1 > Video/Render (`platform/port_overlay.cpp`: `aspect`, `hud_wide`, `render_scale`, `vsync`, `frame_limit`, `smooth_frames`), both in `settings.cfg` and as `MP2_*` for one run; `platform/main.cpp` still sets the viewport policy (`AURORA_VIEWPORT_FIT`) |
| Skip / fast-forward cutscenes, cutscene speed, `MP2_SKIP_CINEMATICS`, `MP2_SKIP_INTRO` (level-load text + cinematic scene) | `platform/port_harness.cpp`, `CMFGame`, `CMFGameLoader` |
| Boot logos (F1 > Game > Cutscenes, `boot_logos`, `MP2_BOOT_LOGOS=show\|skip\|off`, as the Prime 1 port's): Skip (default) ends the health warning or a Nintendo/Retro/Dolby logo with its 0.5 s fade on A or Start, no 2 s wait on the warning; Off goes from the progressive check straight to the front end. The log prints `[splash] <n> ended at <s>` per screen | `src/MetroidPrime/CSplashScreen.cpp` (`TARGET_PC`), `platform/port_overlay.cpp` |
| Mouse aim, twin-stick aim, gyro aim, full keyboard bindings | `platform/port_mouse.cpp`, `port_events.cpp`, `debug_ui.cpp` |
| God mode, heal/kill, teleport to doorway/door/save station/elevator, area and world warps, layer activation | `platform/port_warp.cpp`, `port_overlay.cpp` |
| `MP2_INPUT` scripted input with warp tokens (the sweep's driver) | `platform/port_events.cpp` |
| Widescreen-aware HUD (`MP_HUD_WIDE`), camera bob used by the game itself | `platform/debug_ui.cpp` |
| Touch overlay (Android) with a layout editor | `platform/port_touch.cpp`, `touch_pad.cpp` |
| HD texture set hook (`PortTextures::DeviceName`) | `platform/port_textures.cpp` |
| **Debug command console over TCP** (`MP2_CONSOLE=<port>`, `tools/mp2con.py`) | `platform/port_console.cpp` - new here |
| Console: `objs` / `obj` / `send` (object inspection and script messages), `visor` / `beam` switching, `tp`, `items` | `platform/port_console.cpp`, names from `tools/gen_console_names.py` |
| Console: `face`, `give`/`take`, `press`/`stick` (pad buttons and the main stick held on the virtual pad), `screenshot` | `platform/port_console.cpp`, `platform/port_events.cpp` (`port_input.h`) |
| Console: `world <id> [area]` (a world warp), `worlds` (the memory card's world ids and name tables), `strg <id|name> [index]` (a string table) | `platform/port_console.cpp` |
| Console: `gunoffset`, `msaa`, `aniso`, `hudscale` (the graphics settings, drivable from a headless run) | `platform/port_console.cpp` |
| Input remapping: MP1's remap UI (keyboard and pad rows, contexts, chords) and presets (`classic`, `mouse`, `gamecube`, `remastered`, `modern`, `southpaw`) in the F1 Controls page; bindings in `controls.toml` | `platform/input/port_input_remap_ui.cpp`, `platform/port_controls.cpp`, `platform/port_overlay.cpp` |
| Graphics: first-person FOV, hide helmet, hide visor effects, arm-cannon offset, MSAA, anisotropic filtering, HUD scale | `port_overlay.cpp` + hooks in `CCameraManager`, `CInGameGuiManager`, `CPlayerVisor`, `CPlayer`, `CSamusHud`/`CGuiCamera`, and `aurora_set_graphics_quality` (vendored Aurora) |
| Gameplay tweaks: fast morph, spring ball, toggle lock-on, sticky charge (F1 > Gameplay, `MP2_FAST_MORPH`/`MP2_SPRING_BALL`/`MP2_LOCK_ON_TOGGLE`/`MP2_STICKY_CHARGE`, console `fastmorph`/`springball`/`lockon`/`sticky`) | `platform/port_overlay.cpp`, `platform/include/port_hold_toggle.h` (+ its unit test), hooks in `CPlayer::UpdateMorphBallState`, `CMorphBall::ComputeBallMovement`, `CStateManager::ProcessInput` and `CFinalInput`'s `PortSetL`/`PortSetA` |
| Log file: `metroid_prime2_port.log` next to the card (`.old.log` rotation, `MP2_LOG_FILE=0`, F1 > Testing, console `logfile`) | `platform/port_logfile.cpp`, fed by the Android logcat pump and by a forked copier on the desktop |
| F1 window on the phone: a MENU pill opens it, touches go to SDL as a mouse, two fingers scroll, a drag scrolls | `android/.../TouchControlsView.java`, `platform/port_touch.cpp` |
| Save tools: read/export/import the card's save file, back it up before every write (`save [info|export|import|backup|backups]`, `MP2_CARD_BACKUP`, F1 > Saves, console `cardbackup`) | `platform/port_save.cpp`, hook in `CMemoryCardSys::CCardFileInfo::WriteFile` |

## Next, cheapest and most useful first

1. **Console, what is left of MP1's** - each with the reason it waits: `memo <text>`
   (`ShowPausedHUDMemo` takes a `CAssetId`, so a free-text message needs a port-side string table;
   the one-off text a debugging session wants is usually a `strg` read anyway), `wait <n>` (the
   console answers one command per game update, so a blocking wait buys nothing) and `cstick`/`gyro`
   (the sweep's `MP2_INPUT` tokens already cover both). `warp <world>` is `world <id> [area]` here,
   with `worlds` to name the ids.
2. **Graphics toggles**: done - first-person FOV (`MP2_FOV=<deg>`, F1 > Render), hide helmet /
   hide visor effects (`MP2_NO_HELMET`, `MP2_NO_VISOR_FX`, F1 > HUD), arm cannon offset
   (`MP2_GUN_OFFSET`), MSAA / anisotropy (`MP2_MSAA`, `MP2_ANISO`) and HUD scale (`MP2_HUD_SCALE`,
   50-100%, scaled about the screen centre). Note that `CPlayerVisor`'s echo and dark effects are
   empty stubs here, so `MP2_NO_VISOR_FX` currently only suppresses what the scan visor draws (its
   frame/edges; the scan window itself is the HUD's own scan interface and stays), and that MSAA is
   the one setting whose cost is the whole framebuffer: 4x is for a desktop, not a phone.
3. **Input settings**: the *engine* import is queued (MP1's `platform/input/`: bindings with
   `controls.toml`, device profiles, chords/contexts, its remap UI - 4,100 lines) as its own slice,
   replacing the 22-row Bindings table; it is what "input bindings like Prime" needs. Stage C is
   in: MP1's remap UI and presets are the F1 Controls page (`platform/input/port_input_remap_ui.cpp`,
   `platform/port_controls.cpp`), and `bind`/`binds` and `keybinds.cfg` are gone. What is left is
   export/import of the settings file.
4. **Gameplay tweaks**: done - fast morph, spring ball, toggle lock-on and sticky charge, verified on
   the desktop (see `docs/STATUS.md`). What MP1 has beside them and this tree does not: rapid charge,
   damage/ammo readouts on the HUD, and the morph-ball "hold to charge" variants; each needs a hook in
   `CPlayerGun` that MP2 does not expose as a getter today.
5. **Save tools**: the card file is done - `save info|export|import|backup|backups` on the console,
   `MP2_CARD_BACKUP` (F1 > Saves) and a hook that copies the card before every write (detail in
   `docs/STATUS.md`). What is left of MP1's set does not fit this tree yet: **save states** are done
   (F5/F9, `platform/port_savestate*.cpp`, room state included). **Quick resume** is not: MP2 resumes
   from the card's own save (`MP2_SKIP_FRONTEND`); **region conversion** is refused by design (MP2's save
   carries per-release `SAVW`/area data); **slot-level copy/erase** would have to reproduce the front
   end's option shuffling; raw-image cards are **import only**, and MP1's Dolphin/Trilogy folder
   discovery has no MP2 counterpart.
6. **Photo mode / free camera**: done (`platform/port_photo.cpp`, `platform/port_freecam.cpp`).
7. **Map and tracker** (MP1's `port_map_marks.cpp`, `port_tracker.cpp`): reveal the map, item and scan
   tracking. MP2's map is a different format (`CMapWorld`/`CMapArea`), so this is a rewrite.
8. **Remastered assets** (MP1's `port_remastered_*`): import models/effects from a Metroid Prime
   Remastered dump. Very large; MP2's asset formats are its own.
9. **Discord Rich Presence** (`port_discord.cpp`, needs the RPC library) and **Archipelago** - MP1
   features with no MP2 design yet. LiveSplit and the randomizer are in tree (`docs/RANDOMIZER.md`,
   `platform/port_livesplit.cpp`).
10. **Texture packs / mods** (`port_mods.cpp`, `port_texture_replacement`): Aurora already has texture
    replacement; what is missing is the pack loader and the settings UI.

## Rules that apply to every one of these

- Port-side game changes go inside `#ifdef TARGET_PC`, with the non-PC text byte-identical; the
  upstream matching build must stay at `94.14% fuzzy, 63.29% matched` after each.
- A new setting is one field in `platform/port_overlay.cpp` (with its `MP2_*` env var, which is what a
  headless run uses), one row in the F1 overlay, and a getter in `platform/include/port_overlay.h`.
  `platform/debug_ui.cpp` is in no CMake target - a setting added there compiles nowhere.
- Anything a test must drive also needs an `MP2_INPUT` token or a console command, because the sweep
  and the harness cannot press keys.
