# Android APK for the MP2 port: plan

Plan written 2026-10-17 (orchestrator session, after the Temple Grounds sweep). Nothing here is
built yet: this tree has **no** Android integration. The document exists so a fresh session can
start the APK without re-doing the reconnaissance.

## 1. What the user asked for

An installable APK to play on their phone. Decisions taken:

- **arm64-v8a, Android 13+** (minSdk can stay 28 to match MP1's app if that is simpler).
- **On-screen touch controls** (not just a Bluetooth pad).
- **In-app file picker (SAF)** for the ISO.
- **First deliverable is a boot-and-play tech demo**, not MP1-app parity: it boots the save/disc on
  the phone and plays the areas that work today. The game is not finished (see `AGENTS.md` Status:
  actor families are still stand-ins, and the Temple Grounds sweep found walls), so "playable
  end to end" is not on the table yet.

## 2. What already exists (measured, in `../MetroidPrimePort`)

MP1's port ships this pipeline; it is the template.

- **Driver**: `tools/android_apk.sh` resolves SDK/NDK/Gradle, writes `android/local.properties`,
  then `gradle --project-dir android :app:assembleRelease`.
  NDK pinned **29.0.14206865** (r29 is a hard floor); installed at
  `~/android-ndk-cache/android-ndk-r29` and `~/android/sdk/ndk/29.0.14206865`. Gradle 8.7 at
  `~/.gradle/wrapper/dists/gradle-8.7-bin/*/gradle-8.7/bin/gradle`; AGP 8.5.2.
  Keystore: `tools/make_android_keystore.sh`, `android/keystore.properties` +
  `android/metroid-prime-port-release.jks`; `-PmpDebugSigning=true` sideloads with a debug key.
  Warm APK build: `BUILD SUCCESSFUL in 1m 15s` (51 tasks, ccache and `.cxx` caches warm).
- **App shell**: single `:app` module, native build through AGP `externalNativeBuild` (CMake),
  `applicationId org.metroidprime.port`, `arm64-v8a` only, `jniLibs.useLegacyPackaging` (so the
  224 MB unstripped `.so` gets stripped), assets for textures and a pipeline-cache seed.
- **CMake side** (`MetroidPrimePort/CMakeLists.txt`): ANDROID blocks at ~40, 47-49, 54, 87-92
  (nod stub), 270-272, 287-303 (SHARED lib + `mediandk` + `-Wl,-u,SDL_main` + adrenotools hooks),
  307-333 (dev tools out), 361-364 (OpenSSL), 386-392. Aurora defines `TARGET_PC` itself.
  Aurora's Android support is already vendored here: `extern/aurora/cmake/AuroraSDL3Provider.cmake`
  (:164-176, needs `AURORA_SDL3_PROVIDER=vendor`), `AuroraDawnProvider.cmake` (:83-90 prebuilt
  `android-aarch64` Dawn, `AURORA_DAWN_PROVIDER=auto`).
- **Touch controls** (all Java + JNI, no assets): `TouchControlsView.java` (3148 lines, drawn in
  code) over the SDL surface; JNI glue `platform/debug_ui/android_jni.inc` -> a real SDL virtual
  gamepad (`platform/touch_pad.cpp`) plus the port's own input layer; layouts persisted as
  `touch_layout`; a physical pad hides the overlay.
- **Device paths**: user dir via `MP_USER_PATH` -> `data_folder.txt` marker ->
  `SDL_GetPrefPath(nullptr, "Metroid Prime")` (`port_paths.h:244-262`); data folder move with
  `MANAGE_EXTERNAL_STORAGE`; cache defaults to the private folder.
- **Disc**: SAF picker via `SDL_ShowFileDialogWithProperties` -> the `content://` result is copied
  to `<user>/disc.iso` (`.part` + rename) so no grant is needed later; precedence argv > `MP_DISC`
  > remembered. This is the mechanism to copy for MP2 (`aurora_dvd_open()` in this tree only takes
  a plain path - `extern/aurora/lib/dolphin/dvd/dvd.cpp:658` -> `SDL_IOFromFile` - so *copying* the
  pick to local storage is the cheap route, not reading the URI in place).

## 3. What this tree lacks for Android

1. **No `android/` gradle project, no Java, no JNI, no CMake ANDROID blocks.** `platform/android_probe/`
   holds only a `nod` stub (`nod_stub.c`, `include/nod.h`) - the probe's other files are gone.
2. **Desktop-only host code** (from an inventory of this tree):
   - `platform/main.cpp:26,88-108`: `execinfo.h` backtrace + SIGSEGV/BUS/ILL/FPE handlers. bionic has
     no `execinfo.h`; needs a `__ANDROID__` path (logcat-only, or a tombstone).
   - `platform/glibc_compat.c`: glibc-only, self-excludes on bionic.
   - Env-var config everywhere (`MP2_DISC`/`MP2_USER_PATH`/`MP2_CACHE_PATH`/`MP2_SKIP_FRONTEND`/
     `MP2_INPUT`/`MP2_WORLD`/`MP2_FRAMES`/`MP2_DEBUG_IDS`): a JNI launcher can set env before
     `main`, or a config file + intent extras; MP1 solved this with `port_paths.h` + a marker file.
   - Keyboard/mouse bindings, F1-F12 overlay hotkeys: meaningless with touch; the touch overlay has
     to feed the port's virtual pad instead.
   - Dev/harness only: `tools/*.sh` (Xvfb, gdb, x11grab), `-rdynamic`, CTest targets, host binutils
     (`ld.bfd`/`objcopy`/`nm` in `CMakeLists.txt:162-164`) and the `link_check`/REL-stub ladder.
3. **Architecture-sensitive spots** (little-endian LP64, so most LP64 notes carry over unchanged):
   - `platform/rel.cpp:322-366` `GuestArena`: the 32-bit guest address requirement. On arm64 there
     is no `MAP_32BIT`, and the `#elif defined(MAP_FIXED_NOREPLACE)` branch *uses* `MAP_32BIT`
     unconditionally, which bionic does not define - a compile error to fix. **Fixed at M0** (a
     `/proc/self/maps` gap scan below 4 GiB, `MAP_FIXED_NOREPLACE`, then a hint at `0x10000000`,
     then a refusal that names the size) and **measured on the phone**, where a 32 MiB arena lands
     at `0x12000000` and the console's `0x81000000` module region is free and honoured exactly:
     see section 4a.
   - No x86 intrinsics or asm anywhere in the port's own code (LZO is plain C), so NEON is not a
     gate.
   - Signed `char` is already forced (`CMakeLists.txt:48-52`, `platform/compat.h:60`).
4. **`MP_SDK_HEADERS_ONLY=ON` is the only *verified* configure** in this tree; `build-port-link` is
   the full build. Whichever Android uses, the CMake dev-tool steps have to be excluded.

## 4. Milestones (each one verifiable without assuming a working device)

**M0 - arm64 cross-build links.** Add a toolchain/ANDROID path to `CMakeLists.txt` (exclude the dev
tools, host-binutils steps, tests; pick the Aurora Android providers; keep the SDK stubs the way
`MP_SDK_HEADERS_ONLY` does or build the real SDK), fix the bionic blockers (`execinfo`, `MAP_32BIT`,
any glibc shim), and produce `libmetroid_prime2_port.so` for arm64. **Done - see the M0 detail
below.**
*Verified when*: `$NDK/.../llvm-readelf -h libmetroid_prime2_port.so` says `AArch64` and the link
has no undefined symbols (`llvm-nm -u`), i.e. the same acceptance the desktop `link_check.sh` gives.
No device needed. This is the gate: if the REL arena cannot be placed, stop and report.

### M0 detail: what landed (2026-10-17)

`tools/android_probe.sh` cross-builds this tree for `arm64-v8a` into
`build/android-aarch64/libmetroid_prime2_port.so` (134 MB); `llvm-readelf -h` says `ELF64`, `DYN`,
`AArch64`, and it exports `SDL_main` (the entry SDL's Java activity looks up) and `aurora_main`.
`llvm-nm -u` lists 465 symbols and **every one of them is a platform symbol** - libc, libm, liblog,
libandroid, zlib, GLES, OpenSLES - resolved by the 9 `NEEDED` libraries (`liblog libandroid libz
libm libOpenSLES libdl libGLESv1_CM libGLESv2 libc`), with no game or port symbol among them; the
NDK links with `--no-undefined`, so a green link *is* resolution. No network: Dawn comes from the
prebuilt `android-aarch64` package and SDL from the 3.4.10 sources, both copied into
`build/android-deps/` from `../MetroidPrimePort` (`build/` is git-ignored; the script says where to
get them when the copy is missing).

The four things section 3 predicted, as they actually landed:

- **`CMakeLists.txt`**: the module-scoping step uses the NDK's `ld.lld -r --force-group-allocation`
  / `llvm-objcopy` / `llvm-nm`, because the host binutils cannot read an AArch64 object at all;
  `metroid_prime2_port` becomes a `SHARED` library with `-Wl,-u,SDL_main`, links `log android z`,
  and drops `-rdynamic`; `MP_ANDROID_NOD_STUB` substitutes `platform/android_probe/nod_stub.c` for
  nod (Aurora skips its own nod provider when a `nod::nod` target already exists). The desktop
  build, the `-rdynamic` link and the matching build are untouched.
- **`platform/main.cpp`**: bionic has no `execinfo.h`, so the crash handler logs the signal and the
  fault address to logcat and re-raises - the tombstone is the stack Android gets. The file also
  now includes `<aurora/main.h>` explicitly: that header's `main` -> `aurora_main` rename is what
  gives the shared library its entry point (the desktop links the same way `../MetroidPrimePort`
  does, and still boots).
- **`platform/rel.cpp`**: the arena's 32-bit-address fallback, see below.
- **`platform/glibc_compat.c`** needed nothing: it self-excludes on bionic, as predicted.

No GRADLE, Java, JNI, SAF picker or touch controls at M0, and **nod is the stub**: this library
links, it cannot read a disc yet. Real nod is a Rust cross-build (Corrosion,
`Rust_CARGO_TARGET=aarch64-linux-android`, the vendored rustup under
`../MetroidPrimePort/build/android-rust` has that target installed) and is M1 work.

### The REL arena gate: passed, on the phone

bionic defines `MAP_FIXED_NOREPLACE` but not `MAP_32BIT`, so `GuestArena`'s fallback now scans
`/proc/self/maps` for the lowest free 1 MiB-aligned gap that fits below 4 GiB and maps it with
`MAP_FIXED_NOREPLACE`; failing that it asks for a plain hint at `0x10000000` and takes it only if
the result is below 4 GiB; failing that it stays invalid and prints one line naming the size and
the reason. The existing `< 4 GiB` check is the single accept/reject gate.
`-DMP_ARENA_FORCE_PORTABLE` selects this same path on a host that *does* have `MAP_32BIT`, so
`port_arena_placement_tests` covers it on Linux without a phone.

The phone measurement: a throwaway debug APK (`org.metroidprime2.arenaprobe`, built in `/tmp` with
its own CMakeLists, whose only native source is this tree's `platform/rel.cpp` compiled for arm64
with **no arena macro at all**), installed on the user's Xiaomi 25102PCBEG, Android 16, arm64-v8a.
6 of 6 runs identical:

    maps: below4g_mappings=26 mapped_bytes=780750848 free_bytes=3514216448 (of 4294967296)
    maps: gap[0] 0x0-0x2000000 (32768 KiB) ... gap[4] 0x759d8000-0xebad6000 (1934328 KiB)
    arena_a: valid=1 base=0x12000000 size=33554432 below4g=1     # no preferred base
    arena_b: valid=1 base=0x81000000 preferred_hit=1             # the console module region
    arena_c: valid=1 base=0x100000 size=1048576 below4g=1
    alloc:   ptr=0x12000000 aligned=1 inside=1 below4g=1
    RESULT: PASS

So: a 32 MiB arena is placed below 4 GiB without `MAP_32BIT`, and `0x81000000` - the base the REL
tests use because a real module's `REL24` branches need the console's module region - is free and
honoured exactly in a real Android 16 app process. Caveat worth keeping: that process was fresh, so
SDL/Vulkan/Dawn were not mapped yet; a later run inside the real app would be the confirmation, and
the port does not construct a `GuestArena` at boot today (only the REL tests do).

The probe project is not kept in the tree (the durable part is
`tests/port_arena_placement.cpp`); remove the package with
`adb uninstall org.metroidprime2.arenaprobe`.

**M1 - minimal app that boots.** New `android/` gradle project modelled on MP1's (app module,
manifest, `MetroidPrimeActivity` + Aurora surface, `jniLibs`, debug signing), with paths for the
disc/user/cache dirs (fixed app-internal paths + a `data_folder.txt`-style marker) and logcat
mirroring of the port's `[warp]`/`[ids]`/`[harness]` lines. **Done - see the M1 detail below.**
*Verified when*: `adb install` + launch reaches the game's first update on the phone with a
Bluetooth pad, and `adb logcat` shows the same boot lines the desktop log shows.

### M1 detail: what landed (2026-10-17)

`android/` holds the Gradle project (AGP 8.5.2, `org.metroidprime2.port`,
`org.metroidprime2.port/.MetroidPrime2Activity`, arm64-v8a, `minSdk 28`/`targetSdk 34`, no
permissions, debug signing), driven by `tools/android_apk.sh` (SDK/NDK/Gradle resolution like
MP1's, `-PmpDebugSigning` for a sideload). Java is SDL's own `org/libsdl/app/**` (copied from the
version-matched SDL 3.4.10 source, which is why they are tracked) plus `AuroraSurface` and a
75-line activity: it names `metroid_prime2_port` in `getLibraries()`, and writes
`files/port_paths.txt` (`user=`, `cache=`, `disc=`) *before* `super.onCreate` loads the library, so
a pushed copy wins and the picker has somewhere to write.

Native side, both in `platform/main.cpp` and both `#if defined(__ANDROID__)`:

- **The log.** Android discards fd 1 and 2, so the entry repoints them at a pipe and a detached
  thread turns the bytes back into `mp2port` logcat lines (unbuffered stderr, line-buffered
  stdout, drained on all four exits). Without it a run that stops at the disc is indistinguishable
  from one that never started - which is what the first stub-APK run looked like.
- **The directories.** `MP2_USER_PATH`/`MP2_CACHE_PATH`/`MP2_DISC` still win when set, and the
  env-less app falls back to `SDL_GetPrefPath(nullptr, "Metroid Prime 2: Echoes")` - on Android
  the app's own files directory - with `user/`, `cache/` and `disc.iso` beneath it, overridden by
  `port_paths.txt`. It prints the three it chose, the way MP1 prints its memory-card path.

**nod is now the real one.** `tools/android_apk.sh :app:assembleDebug -PandroidNodStub=false`
builds it (Corrosion + the vendored Rust toolchain: this tree's `build/android-rust`, falling back
to `../MetroidPrimePort/build/android-rust`, `CARGO_NET_OFFLINE=true`, the offline
`aurora_nod`/`corrosion` FetchContent caches). Verified on the packaged, stripped library:
19 `T nod_*` symbols including `nod_disc_open_stream`, 522 undefined (all platform), zero
`nod_stub.c` strings; the stub build has 9 and 3.

**On the phone** (Xiaomi 25102PCBEG, Android 16/arm64, the user's own disc pushed to
`files/disc.iso` after `run-as`; 1.46 GB in ~25 s at 55 MB/s): the run reaches

    mp2port : metroid_prime2_port: disc mounted: /data/user/0/org.metroidprime2.port/files/disc.iso
    mp2port : metroid_prime2_port: init compiled module WallCrawler … (86 modules)
    mp2port : [info] [aurora::card] Loaded GC Card Image: …/files/user/USA/Card A
    mp2port : Initializing renderer...   then   Low latency audio enabled
    (AAudio 44100/2ch fed for the whole 35 s; the process is alive at the end, no ANR)

So the app installs, loads the library, mounts the user's disc, accepts `G2ME01`, brings Aurora and
the renderer up and runs the game loop. Not proven: the front end *visually* - the phone was
locked, so `screencap` returned the lock screen - and "reaches the game's first update" in the
strict sense, because the port's harness counters are driven by environment variables an installed
app cannot set; the env bridge (below) is what will put `[ids] frame …` in a device log.

Known issues, in order of how likely they are to be hit:

- **One SIGABRT at startup with the screen locked**: `FORTIFY: pthread_mutex_lock called on a
  destroyed mutex`, signal 6, `SI_QUEUE`, through `android::Surface::hook_perform` →
  `libvulkan CreateAndroidSurfaceKHR` → Dawn `CreateVulkanSurface` → `SwapChain::Initialize` →
  `aurora::webgpu::initialize` → `aurora_main` ← `SDLActivity.nativeRunMain`. That is Dawn
  configuring a surface from an `ANativeWindow` SDL has already destroyed, i.e. an activity
  lifecycle race, not a game bug; three relaunches with the same locked screen did not reproduce
  it. Reported here so the next device session does not re-discover it.
- **The debug variant cannot be built at `-O0`**: musyx's C99 `inline u_seqPlaySong`
  (`extern/musyx-port/src/musyx/runtime/s_data.c:524`, called at :597) has no out-of-line copy
  there, so the link fails. `android/app/build.gradle` builds the debug variant's native code as
  RelWithDebInfo instead of touching vendored musyx; `static inline` there is the real fix, for
  whoever owns that snapshot.
- **A locked phone is an awkward test target**: `adb exec-out screencap` returns the lock screen,
  so a device session needs the phone unlocked to prove anything visually.

Next: **M2** - the SAF picker, plus an environment bridge so a device run can set the harness
variables (`MP2_DEBUG_IDS`, `MP2_FRAMES`, `MP2_INPUT`) from `port_paths.txt`; that is what makes
"the game reached its first update" provable on the phone instead of inferred from audio.

**M2 - SAF picker.** Port MP1's picker + copy-to-`<user>/disc.iso` (`main.cpp:513-560` there), with
MP2's game-id check (this port expects `G2ME01`).
*Verified when*: picking an ISO once makes later launches boot with no picker.

**M3 - touch controls.** Port `TouchControlsView.java` + `touch_pad.cpp` + the JNI bridge, and map
them into this port's input layer (`platform/port_events.cpp`'s virtual pad, `platform/port_mouse.cpp`
for the look-stick).
*Verified when*: a run on the phone moves the player and fires with only touch.
MP2-specific work: MP2 has no `port_input_settings.cpp`/`debug_ui` module split yet, so the layout
settings have somewhere to live only after a small refactor - see MP1's `platform/debug_ui/controls.inc`
if the module split is copied.

**M4 - tech-demo polish.** Stripped release APK with a debug-ish signing story, versionName, an
in-app line with the build's git hash, and a short `docs/ANDROID.md` "how to install" section.

## 5. Risks, worst first

- **REL arena placement** (M0): the thing that could have made the whole plan collapse. **Resolved**
  - see the M0 detail: it was a compile error (`MAP_32BIT`) plus a placement strategy, and the
  phone measurement says a 32 MiB arena lands below 4 GiB with the console's `0x81000000` region
  free. What remains unmeasured is placement in the *real* app's process, which has SDL, Vulkan and
  Dawn mapped by then (see the caveat there).
- **Tombstones instead of backtraces**: without `execinfo`, a crash on device is only a tombstone;
  `adb logcat` + `ndk-stack` is the workflow to write down before debugging on the phone.
- **Touch mapping into MP2's input layer** (M3): MP2's platform layer is smaller than MP1's (no
  input-settings module, no touch pad), so expect a small refactor, and expect the look-stick to be
  the fiddly part.
- **Performance unknown**: MP2 on a phone GPU is untested; the port's software-Vulkan present path
  is a desktop workaround and must not be copied - Android needs the real SDL/Aurora surface.
- **The game itself is incomplete** (see `AGENTS.md` Status): the APK will hit stubs, missing
  modules and the sweep's walls. That is expected for a tech demo, and the phone is actually a
  useful second front for that work - *if* the crash reporting is in place first (logcat + tombstone).
