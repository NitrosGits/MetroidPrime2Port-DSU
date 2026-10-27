# Linux CI and packaging

Mirrors `../MetroidPrimePort` (its `docs/RELEASING.md`), Linux only. No game data is ever packaged:
the disc image is the user's, asked for on first launch.

| File | What it does |
|---|---|
| `.github/workflows/linux-build.yml` | push to `port`, PRs: Ubuntu 24.04, gcc-14, `-DMP_SDK_HEADERS_ONLY=OFF`, `.github/actions/build-and-test` (build, `ctest -L port`, Aurora `gx_fifo_tests`) |
| `.github/workflows/linux-release.yml` | manual: `almalinux:9` + gcc-toolset-14, x86_64 and aarch64; fails if the binary needs glibc > 2.34 or GLIBCXX > 3.4.29; artifacts `metroid_prime2_port-linux[-aarch64]` (AppImage + tarball) |
| `.github/workflows/cache-prune.yml` | keeps the newest cache entry per key family after each build workflow |
| `tools/make_appimage.sh`, `tools/collect_licenses.sh` | AppImage (no bundled libs) and the notices (`share/licenses/metroid-prime2-port/`); Dawn/nod texts in `packaging/licenses/` |
| `.github/workflows/build.yml.disable` | upstream's matching-decomp CI (needs the retail DOL); disabled as MP1 did |

Release: `gh workflow run linux-release.yml --ref port -R Odrannnn/MetroidPrime2Port`, then
`gh run download <id> -n metroid_prime2_port-linux`. The linker is the container's default (bfd); the
CMake needs only an ELF linker with RESCAN support. Checkout takes no submodules: `extern/musyx` is unused
by the port build (`extern/musyx-port` is vendored).

Not done: the Windows/macOS/Android/iOS workflows, Flatpak, Linux `--version` check (no such flag).
