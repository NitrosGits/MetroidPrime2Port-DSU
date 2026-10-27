# Metroid Prime 2: Echoes — native port

**Looking for the first game? See the [Metroid Prime native port](https://github.com/Odrannnn/MetroidPrimePort).**

[![Linux build](https://github.com/Odrannnn/MetroidPrime2Port/actions/workflows/linux-build.yml/badge.svg?branch=port)](https://github.com/Odrannnn/MetroidPrime2Port/actions/workflows/linux-build.yml)

A native build of **Metroid Prime 2: Echoes** for Linux, Windows, macOS, Android and (experimentally) iOS, compiled from
the [PrimeDecomp/echoes](https://github.com/PrimeDecomp/echoes) decompilation and
rendered through Aurora (SDL3/WebGPU). There's no emulator involved. Supported
discs: the USA GameCube release (`G2ME01`), the PAL release (`G2MP01`, experimental) and
the Wii's Metroid Prime Trilogy (`R3ME01` USA, `R3MP01` PAL; its Metroid Prime 2, experimental).
This is a pre-release: expect rough edges, and please report what you hit.

**No game content is included.** You need your own copy of the game: the port
reads it from a disc image and never ships any of its assets. This repository
ships no disc image, and neither does any package or release built from it.

## What it adds over the console release

- **Widescreen** with a widescreen-aware HUD, **render scale**, vsync, a frame
  cap you can lift, and **frame interpolation**
- **Graphics:** first-person FOV, MSAA, anisotropic filtering, dynamic resolution,
  HUD scale, arm cannon offset, toggles to hide the helmet and visor effects, and a
  choice of renderer (Vulkan or OpenGL on desktop Linux)
- **Controls:** mouse and twin-stick aim, gyro aim, lock-on free aim, pointer aim,
  full rebinding with presets
- **Optional tweaks:** Fast Morph, Spring Ball, Toggle Lock-On, Sticky Charge,
  walk speed, scan speed, hint delay, elevator ride; an **Original experience**
  switch puts the retail behaviour back
- **Skippable cutscenes**, boot logos and the world-name card
- **Language** switching on PAL and Trilogy discs
- **Save states** and **quick resume** after quitting
- **Saves:** import and export the memory card file, with Dolphin and PrimeHack
  card folders, backed up before every write
- **Metroid Prime Trilogy:** run Echoes from a Wii disc, with Trilogy's HUD frames,
  reticles and, opt-in, its radial beam and visor menus (mouse, pad and touch)
- **Suit colours** for the Varia, Dark and Light suits
- **Mods:** drop-in mod folders that replace disc data or single resources, and
  an HD texture pack ([docs/MODS.md](docs/MODS.md))
- **Photo mode** with a free camera and high-resolution capture
- **Speedrun tools:** an in-game time display and LiveSplit
- **Discord Rich Presence** on desktop, and a **touch overlay** with a layout
  editor on Android
- A **debug console** over TCP, and the **F1 overlay**, which has every setting,
  with search

## Downloads

Releases are published on the
[GitHub releases page](https://github.com/Odrannnn/MetroidPrime2Port/releases).
Every build needs your own disc image; none of them contains one.

| Platform | Asset |
|---|---|
| Linux x86_64 / aarch64 | `metroid-prime2-port-X-linux-{x86_64,aarch64}.tar.gz`, or the AppImage |
| Windows x86_64 / arm64 | `metroid-prime2-port-X-windows-{x86_64,arm64}.zip` |
| macOS (Apple silicon) | `metroid-prime2-port-X-macos-arm64.zip` |
| Android (arm64) | `metroid-prime2-port-X-android-arm64.apk` |
| iPhone / iPad (experimental) | `MetroidPrime2Port.ipa`, unsigned: sign it yourself and sideload it ([docs/IOS.md](docs/IOS.md)) |

**Quick start:** download the build for your platform, start it, and pick your disc
image when it asks (Android shows a file picker and copies the image into the
app). Supported discs are the GameCube USA (`G2ME01`) and PAL (`G2MP01`)
releases and the Wii Metroid Prime Trilogy (USA `R3ME01`, PAL `R3MP01`), as
`.iso`, `.gcm`, `.rvz`, `.wia`, `.gcz`, `.ciso` or `.wbfs`. See [docs/PAL.md](docs/PAL.md)
and [docs/TRILOGY.md](docs/TRILOGY.md) for what is verified on each. Press **F1**
for every setting.

On Linux the AppImage bundles no libraries: FreeType, libpng and a Vulkan
driver, X11 or Wayland come from the system.

## Running

```sh
./metroid_prime2_port "/path/to/Metroid Prime 2 - Echoes.iso"
```

The `MP2_DISC` environment variable works too. Headless and scripted runs,
the gameplay harness and the area sweep are in [docs/RUNNING.md](docs/RUNNING.md).

## Building

CMake 3.25+ and Ninja with a C++20 toolchain. A host build needs your disc only at run
time, never to compile. The port configuration is `-DMP_SDK_HEADERS_ONLY=OFF`,
which adds `extern/aurora` (it fetches its own SDL3 and Dawn) and builds the
`metroid_prime2_port` executable:

```sh
cmake -S . -B build-port-link -G Ninja -DMP_SDK_HEADERS_ONLY=OFF
cmake --build build-port-link --target metroid_prime2_port
```

[docs/RUNNING.md](docs/RUNNING.md) has the run, test and coverage commands,
[docs/RELEASING.md](docs/RELEASING.md) what CI builds, [docs/ANDROID.md](docs/ANDROID.md) the
Android build, and [PORT_NOTES.md](PORT_NOTES.md) how the port works (the REL
runtime, `TARGET_PC` rules, SDK shims). The upstream matching build
(`python3 configure.py && ninja`, see [docs/DECOMP_README.md](docs/DECOMP_README.md)) is kept
intact; every port change to an upstream file sits in a `#ifdef TARGET_PC` block.

## Troubleshooting

- **Where are the logs?** `metroid_prime2_port.log` in the user folder (F1 > System
  > Log); the previous run's is `metroid_prime2_port.old.log`. Attach it to a bug report.
- **"unsupported disc"**: the image is not one of the releases above. Re-dump it,
  or check you picked the right disc.
- **Settings in a bad state**: F1 > System > Settings > **Reset configuration...**
  backs up and resets every setting and mapping; saves are not touched.

## Credits and licensing

This is a fork of the [PrimeDecomp/echoes](https://github.com/PrimeDecomp/echoes)
decompilation, which the game logic is built from. The port layer follows
[MetroidPrimePort](https://github.com/Odrannnn/MetroidPrimePort), the same
approach for Metroid Prime. Aurora (`extern/aurora`) is a vendored snapshot, and MusyX
(`extern/musyx-port`) provides the audio engine; third-party licences are kept
alongside them and copied into release packages.

`LICENSE` (MIT) covers this project's own work — the platform layer, the build and
packaging scripts, and the Android integration. It does **not** cover the game code
under `src/` and `include/`, which comes from the decompilation and remains
Nintendo's. See `NOTICE` for the exact scope and the third-party terms.
`LICENSE-decomp` is upstream PrimeDecomp's CC0 dedication, kept as it was.

Metroid Prime 2: Echoes is a trademark of Nintendo. This project is unaffiliated
with Nintendo and Retro Studios, and ships no game data.

## Documentation

| Document | What it covers |
|---|---|
| `docs/SETTINGS.md` | Every control, setting and environment variable |
| `docs/QOL.md` | Quality-of-life features: what is in, what is next |
| `docs/RUNNING.md` | Running the port: harness, console scripts, sweeps, coverage |
| `docs/PAL.md` | The PAL disc: what is verified |
| `docs/TRILOGY.md` | Metroid Prime Trilogy: mount, HUD, radial menus |
| `docs/MODS.md` | The mods folder, suit colours, texture pack |
| `docs/CONSOLE.md` | The debug console (`MP2_CONSOLE`) |
| `docs/ANDROID.md` | The Android port's status and build |
| `docs/RELEASING.md` | What a release contains and must carry |
| `docs/DECOMP_README.md` | Upstream PrimeDecomp/echoes README (decomp build) |
| `PORT_NOTES.md` | How the port works and non-obvious facts |

## AI assistance

Parts of this port were written with AI coding assistants, under my oversight.
