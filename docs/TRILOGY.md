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
