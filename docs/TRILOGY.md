# Metroid Prime Trilogy (experimental, milestone 1)

The port accepts the Wii Trilogy disc (USA `R3ME01`, PAL `R3MP01`) and mounts its Echoes data,
like PAL: USA 1.00 code compiled natively, the disc supplies only data (`DiscVersion::TrilogyUsa/TrilogyPal`).

- Mount: `aurora_dvd_set_root("MP2", "rs5mp2_p.dol")` (vendored Aurora, ported from MP1 575f54d28) makes the
  FST subdirectory `MP2/` the root and serves `MP2/../rs5mp2_p.dol` (a root-level file) as the main DOL.
- Images: `.iso .gcm .rvz .wia .gcz .ciso .wbfs` (nod reads them; the Trilogy RVZ is Zstd-19, 128 KiB blocks, so first
  reads are slow). Not hash-checked: `port_disc_verify` has no table row, so the disc shows as "unknown disc".
- Saves: card region follows the 4th game-code char (`R3ME` -> USA, `R3MP` -> `EUR/`), same folders as the GC discs.
- Tested image: `Metroid Prime Trilogy (USA).rvz` (3903324160 bytes, rev 0).

## Echoes paks (`MP2/` in the FST) vs the USA GC disc

Resource versions match for every type sampled (MLVL 0x17, MREA 0x19, CMDL v4, ANCS, ANIM, PART, TXTR, STRG, FRME,
FONT, SCAN, SAVW, PATH, DCLN, CSKR, CINF, EGMC, MAPA/MAPW, AGSC): same headers on the same ids; FONT `0x37ff312`
(NoARAM) differs in size only.

| pak | GC bytes | Trilogy bytes |
|---|---|---|
| FrontEnd | 5628864 (146 res) | 1082144 (139 res) |
| NoARAM | 699136 | 866464 |
| Metroid1..6 | 184.0M/42.2M/174.4M/140.8M/134.2M/22.0M | within +-0.8%, same ids |
| AudioGrp, GGuiSys, LogBook, MiscData, SamusGun | | within 1% |
| MidiData, SamGunFx(Low), SamusGunLow, TestAnim | | identical size |
| SamGunFxMulti, SlideShow | present | absent (multiplayer / slideshow cut) |
| RS5.PAK, Strings.pak | absent | extra (Trilogy menu data; `SamGunFxMulti`/`SlideShow` names appear upper-case) |

Missing paks read as empty bytes (guarded), so the absent ones don't fail the mount.

## Boot results (USA 1.00 code, Trilogy data)

- Mount and identify: OK. `MP2_SKIP_FRONTEND=1 MP2_NEW_GAME=1` reaches gameplay at 5.0 s (frame 121) in Landing
  Site (world 3bfa3eff); a 2500-frame run ends cleanly (no caught signals, no reach stubs, no crash).
- **First failure, normal front end**: SIGSEGV in `CScriptGuiFrontEndScreen::CollectWidgets`
  (`src/MetroidPrime/ScriptObjects/CScriptGuiScreen.cpp:146`, log `/tmp/opencode/tri_boot.log:252`): Trilogy's
  `FrontEnd.pak` (139 resources, 1/5 of the GC size) drops the multiplayer menu connections the GC front-end layout expects, so an
  `invertIds[i]` index runs out of range. Needs a guard (or a Trilogy-specific front end) in a later milestone.
- `PortReadDolResource` (`platform/disc.cpp`, fixed DOL addresses) has no callers in the tree today; if one is added,
  Trilogy's `rs5mp2_p.dol` is a different binary and needs its own address table. DOL-embedded data is otherwise
  unverified (the port compiles all code natively).
- Not checked: audio (`AudioGrp`/`Strings.pak` differences), PAL Trilogy, Wii-only features, RVZ load time.

## Regression (2026-10-26)

GC USA and PAL `MP2_SKIP_FRONTEND=1 MP2_NEW_GAME=1` boots (1500 frames) are unchanged: gameplay at frame 121,
exit 0, no caught signals. `python3 tools/gen_files_cmake.py --check` clean; no upstream `src/` file touched.

## Front end on Trilogy data (2026-10-26)

Trilogy's FrontEnd.pak has no per-player invert menus (`kSS_*` list empty, rumble 4, join 8 entries) and only 3
text panes in the save-slot list (GC: 9), so upstream's `CollectWidgets`/`UpdateSaveSlots` indexed past the lists.
TARGET_PC fix in `CScriptGuiScreen.cpp`: `PortIdAt` turns a missing list entry into `kInvalidUniqueId` (null
widget), and the slot-pane `SetText`s plus the multiplayer invert/rumble users null-check. The non-PC path is
byte-identical (strip-diff clean). Result: health warning -> main menu (Single Player / Options / Multiplayer) ->
file select -> new game -> opening cinematic -> first room, no `MP2_SKIP_FRONTEND`
(`MP2_INPUT=A@1300+3,A@2900+3,A@3400+3,A@3900+3,START@4300+3,...`, ~30 updates/s). Multiplayer and used slots: see "Multiplayer and used save slot" below.
Screenshots from the run: `/tmp/opencode/trilogy/shots/`. GC USA still reaches title and gameplay.

## Disc picker (Android)

`SupportedHeader` accepts a plain GC header (G2ME01/G2MP01/R3ME01/R3MP01) or a container magic
(RVZ/WIA/CISO/WBFS/GCZ, `port::entry::IsPickableDiscHeader`); nod verifies the disc after open. The picker
filter lists iso/gcm/rvz/wia/gcz/ciso/wbfs. Unit tests: `tests/port_entry_tests.cpp`.

## Area sweep sample (USA-code-on-Trilogy, fast mode, 6 slots per world, 2026-10-26)

`MP2_DISC=<rvz> MP2_LIST_DISC=<GC USA iso>` (`tools/world_areas.py` cannot read an rvz; `MP2_LIST_DISC` points
the slot lister at a plain ISO), `MP2_NEW_GAME=1`, `SHARDS=1 COUNT=6`, WORLD = each campaign world.
Result: 30/30 slots OK (Temple Grounds, Agon, Torvus, Sanctuary, Great Temple, slots 0..5), no crash, no stubs.

## Multiplayer and used save slot (2026-10-26)

Multiplayer (Trilogy USA, desktop): main menu, two held `SDOWN@f+40` steps select Multiplayer. Pads 1-3 are driven
with `MP2_INPUT` prefixes (`p1.A@f+3`, docs/RUNNING.md "Driving pads 1-3"); two pads join and the start works from
pad input alone. The earlier "single pad cannot start" was the pad-count gate (`UpdateControllerCount`), not a
missing MAXR/ZERO message (inference refuted; no SetToZero workaround needed). Arena is picked naturally
(`SRIGHT` on the arena screen: world 7e19ed26 instead of the first, a50a80cc); the `mTeleporters[0]` fallback is
removed. Split-screen 2-player deathmatch: `/tmp/opencode/trilogy/shots/mp2p_deathmatch.png`.
Real crash found by the trace: Trilogy FRME v5 text panes carry 9 more bytes (2 ints + 1 bool, behaviour unknown)
after `scaleToViewport`; `CGuiTextPane::Create` skips them for `version >= 5` (TARGET_PC).
TARGET_PC guards: `CScriptGuiScreen.cpp` `StartMultiplayerGame`/`UpdateUnlocks` (null switches/menus/teleporters,
`[mp] start:` stderr line), `CWorldLayerState::GetLayerCount` (arena worlds have no SAVW layer table).
GC USA 2-player start was not reproduced through the GC front-end wheel (stick-direction selection, would not
reach Multiplayer in 5 attempts); GC FRME is v4 so the text-pane change does not touch it.

Used slot: save via `MP2_SKIP_FRONTEND=1 MP2_NEW_GAME=1` + `STATION@300,A@500+3` (card:
`USA/Card A/01-R3ME-MetroidPrime2.gci`, region follows the 4th game-code char). Trilogy's file select then shows
all three panes: "Samus 1" (title), "00:00 elapsed" (play time), "Temple Grounds 00%" (world + completion).
Nothing seen missing; the other six GC panes do not exist in Trilogy's pak. Loading it reaches Temple Grounds
gameplay (world 3bfa3eff, area 0, 6500 frames). The GC USA side-by-side was not done (the GC ISO uses G2ME and
rejects the R3ME gci). Shots: `/tmp/opencode/trilogy/shots/mp_*.png`, `slot_*.png`.
Gotcha: launch under Xvfb with `WAYLAND_DISPLAY=` empty, else it hangs at "[aurora::gpu] Requesting adapter".

## Native desktop beam / visor selectors (opt-in)

`trilogy_radial_selectors=1` in `settings.cfg` (or `MP2_TRILOGY_RADIAL_SELECTORS=1`) enables
the native Echoes RS5 selector frames. Default **off**. F1 > Controls > Options > Aiming exposes
**Native Trilogy radial selectors** only with a Trilogy disc. RS5 is registered only on Trilogy;
the existing combat/helmet/passive HUD is not replaced. Missing frames, declared textures,
required widgets or incomplete loading disable capture and log `[radial] ... unavailable`.

Bind **Beam menu (hold)** and **Visor menu (hold)** in F1 > Controls > Remap, under Beams/Visors,
for keyboard or gamepad. They are deliberately unbound by default: no custom binding is replaced.
Use the normal Press (while-held) trigger, not Tap/Toggle. Choose an unused key/button, or explicitly
remove its conflicting gameplay binding yourself. Example additive `controls.toml` profile:

```toml
[[profile]]
name = "Radial menus"
match = ""
[[profile.bind]]
action = "beam_menu"
inputs = ["key:F6"]
contexts = ["gameplay"]
[[profile.bind]]
action = "visor_menu"
inputs = ["key:F7"]
contexts = ["gameplay"]
[[profile.bind]]
action = "beam_menu"
inputs = ["pad:rightstick"]
contexts = ["gameplay"]
[[profile.bind]]
action = "visor_menu"
inputs = ["pad:leftstick"]
contexts = ["gameplay"]
```

Add the binding entries to your existing base profile rather than adding a second empty-match
profile if you already have one. Mouse aim must be enabled to provide captured relative motion;
the mapped C-stick/right-stick vector works with or without twin-stick aim. Hold a menu, point,
then release to request the owned item through the existing retryable beam/visor path. Returning
the selection stick to neutral selects the center. After release a still-deflected stick is drained
until neutral, so it cannot also trigger a GC beam change or turn the view.

| Position | Beam | Visor |
|---|---|---|
| Center | Power | Combat |
| Upper | Annihilator | Scan |
| Lower left | Dark | Dark |
| Lower right | Light | Echo |

These are the actual FRME icon positions, not Prime 1's item order. Center **selects**, it does
not cancel. Releasing on the current item is a no-op; unowned icons/highlights are hidden and their
regions cannot commit. An already-open wheel wins conflicts; Beam wins simultaneous first presses.
Relative mouse travel is normalized at 200 counts/radius, center radius 0.35. Fade/highlight timings
are donor-inspired port choices, not proven Echoes retail timing/colors. GUI proportions use the
existing MP2 camera projection policy without horizontal text/icon scaling.

Only player 0 in single-player, alive, unmorphed first-person running gameplay can capture
(not in a turret or photo mode).
Pause/map, F1, cinematic, death, morph, world teardown, disabling the setting and unavailable assets
cancel without committing. Cancellation requires a fresh menu press before reopening. Normal
simulation continues; no global pause is added. Mouse tick movement, pointer edge-turn, pending
view interpolation, normal C-stick and direct/cycle item actions are suppressed during capture.

### Deterministic console harness

With the setting enabled and `MP2_CONSOLE=<port>`:

* `radial` reports capture, active kind (0 beam / 1 visor / -1 none), vector and fade fractions.
* `radial beam <x> <y>` / `radial visor <x> <y>` holds a normalized selection (X right / Y up).
* `radial release` releases that hold; `radial cancel` tears it down without selection.
  Cancel also invalidates released-but-unaccepted radial retries and queued gameplay requests;
  it never rolls back a selection the game already applied. Disabling the setting invalidates
  these requests immediately, even when re-enabled before the next input poll. Ordinary touch,
  direct and cycle selection requests are not withdrawn by radial cancellation.
* `radial mouse <dx> <dy>` injects relative travel through the existing captured mouse path.
* `radial pad beam <x> <y>` / `radial pad visor <x> <y>` simulates raw right-stick vector plus
  right/left stick click **through the input engine**. Requires the example pad bindings above.
  `radial padrelease` releases the click but keeps the stick vector; `radial padoff` then returns
  the simulated pad to neutral. This is an explicit test override, not a physical device install.

Use `at <frame> <command>` / `tools/drivers/con.py` for deterministic sequences; leave several
updates after release for the game's ordinary switch transition. `give` can grant test inventory.
Do not combine forced normalized selection with physical holds when testing arbitration.

Desktop only in this slice. The normalized transport is exposed for later touch integration;
existing Java/C++ touch wheels and touch controls are unchanged. No new cursor model is drawn.
