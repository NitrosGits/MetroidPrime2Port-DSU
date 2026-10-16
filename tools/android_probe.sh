#!/bin/sh
# Android cross-build probe for this tree: a device-free arm64-v8a build that
# links `libmetroid_prime2_port.so` (milestones M0/M1 of docs/ANDROID.md).
#
# Two nod modes, chosen by the *first* argument:
#
#   tools/android_probe.sh              --nod-rust (the default): the real Rust
#       nod, built from build/android-deps/aurora_nod by Corrosion. This is the
#       mode a bootable APK needs - aurora_dvd_open() reads the disc through nod
#       (extern/aurora/lib/dolphin/dvd/dvd.cpp) - and it is configured with
#       -DMP_ANDROID_NOD_STUB=OFF (stated, not defaulted, so that switching back
#       from --nod-stub in the same build directory really drops the stub).
#   tools/android_probe.sh --nod-stub    -DMP_ANDROID_NOD_STUB=ON: links
#       platform/android_probe/nod_stub.c instead, which needs no Rust
#       toolchain but answers every entry point with a failure, so a build made
#       this way cannot mount a disc. M0 gated this mode.
#
# (`--real-nod` is accepted as a synonym of `--nod-rust`; any further arguments
# are passed through to CMake, e.g. `tools/android_probe.sh -DMP_BUILD_TESTS=ON`.
# Both modes configure the same build directory; CMake just reconfigures.)
#
# It configures with the NDK's CMake toolchain (r29 floor: Aurora needs C++20
# `std::jthread`, which r29's libc++ is the first to ship) and builds the same
# target the desktop build produces, only as a shared library. Everything comes
# from local caches - there is no network here:
#
#   build/android-deps/dawn_prebuilt   prebuilt aarch64 Dawn package
#                                      (lib/libwebgpu_dawn.a), consumed through
#                                      FETCHCONTENT_SOURCE_DIR_DAWN_PREBUILT
#   build/android-deps/sdl             SDL3 3.4.10 sources matching
#                                      AURORA_SDL3_VERSION, consumed through
#                                      FETCHCONTENT_SOURCE_DIR_SDL. It must be a
#                                      *copy*: Aurora's FetchContent
#                                      PATCH_COMMANDs rewrite files in the SDL
#                                      source directory.
#   build/android-deps/aurora_nod      nod 2.0.0-alpha.12 source plus the crates
#                                      its Cargo.lock needs (--nod-rust only):
#                                      Aurora's FetchContent_Declare(aurora_nod)
#                                      is redirected through
#                                      FETCHCONTENT_SOURCE_DIR_AURORA_NOD, so
#                                      neither the git clone nor the crate
#                                      fetches happen.
#   build/android-deps/corrosion       Corrosion v0.6.1 (the CMake/Rust bridge
#                                      nod's own CMakeLists.txt fetches; the
#                                      package name there is `Corrosion`),
#                                      redirected through
#                                      FETCHCONTENT_SOURCE_DIR_CORROSION
#                                      (--nod-rust only).
#
# None of the four is in git (`build/` is ignored); missing ones are copied from
# ../MetroidPrimePort when that checkout still has them, and the script says how
# to supply them otherwise.
#
# --nod-rust also needs a Rust cross-toolchain with the aarch64-linux-android
# target. One is vendored at ../MetroidPrimePort/build/android-rust (a cargo +
# rustup pair, ~2 GB, so it is referenced rather than copied); ANDROID_RUST_DIR
# points the script at another one. Cargo runs with CARGO_NET_OFFLINE=true: the
# registry cache inside that tree is the only crate source.
#
# Usage: tools/android_probe.sh [--nod-stub|--nod-rust] [-DEXTRA=...].
# ANDROID_BUILD_DIR and ANDROID_DEPS_DIR override the two build directories,
# ANDROID_RUST_DIR the Rust toolchain, ANDROID_API the API level (28),
# ANDROID_NOD_MODE the mode when no flag is given, MP_REVIEW_TOOLS the
# cmake/ninja prefix.
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mp1_dir=$root_dir/../MetroidPrimePort

nod_mode=${ANDROID_NOD_MODE:-rust}
while [ $# -gt 0 ]; do
    case $1 in
        --nod-stub) nod_mode=stub; shift ;;
        --nod-rust|--real-nod) nod_mode=rust; shift ;;
        *) break ;;
    esac
done
case $nod_mode in
    rust|stub) ;;
    *)
        echo "error: ANDROID_NOD_MODE must be rust or stub (got $nod_mode)" >&2
        exit 1
        ;;
esac

# This machine has no system cmake/ninja; the review-tools prefix (see AGENTS.md) does.
review_bin=${MP_REVIEW_TOOLS:-"$mp1_dir/build/review-tools/bin"}
cmake="$review_bin/cmake"
ninja="$review_bin/ninja"
build_dir=${ANDROID_BUILD_DIR:-$root_dir/build/android-aarch64}
deps_dir=${ANDROID_DEPS_DIR:-$root_dir/build/android-deps}
home=${HOME:-/nonexistent}

if [ ! -x "$cmake" ] || [ ! -x "$ninja" ]; then
    echo "error: $cmake and $ninja are required (set MP_REVIEW_TOOLS)" >&2
    exit 1
fi

# The host interpreter: CMake's FindPython3 cannot use the NDK sysroot's python even
# when there is one, and the module-scoping step *runs* tools/scope_module.py.
python3=${MP_HOST_PYTHON3:-$(command -v python3 || true)}
if [ -z "$python3" ] || [ ! -x "$python3" ]; then
    echo "error: no host python3 found (set MP_HOST_PYTHON3)" >&2
    exit 1
fi

ndk=${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}
if [ -z "$ndk" ]; then
    for sdk in "${ANDROID_SDK_ROOT:-}" "${ANDROID_HOME:-}" "$home/Android/Sdk" "$home/android/sdk" /opt/android-sdk; do
        [ -n "$sdk" ] || continue
        for candidate in "$sdk"/ndk/*; do
            if [ -f "$candidate/build/cmake/android.toolchain.cmake" ]; then
                ndk=$candidate
            fi
        done
    done
    for candidate in "$home"/android-ndk-cache/android-ndk-*; do
        if [ -f "$candidate/build/cmake/android.toolchain.cmake" ]; then
            ndk=$candidate
        fi
    done
fi

toolchain=${ndk:+$ndk/build/cmake/android.toolchain.cmake}
if [ -z "$ndk" ] || [ ! -f "$toolchain" ]; then
    echo "error: Android NDK not found; set ANDROID_NDK_HOME" >&2
    exit 1
fi

ndk_revision=
while IFS='=' read -r key value; do
    case $key in
        "Pkg.Revision "*) ndk_revision=${value# } ;;
    esac
done < "$ndk/source.properties"
ndk_major=${ndk_revision%%.*}
if [ -z "$ndk_major" ] || [ "$ndk_major" -lt 29 ]; then
    echo "error: Android NDK r29 or newer is required (found ${ndk_revision:-unknown})" >&2
    exit 1
fi

# The offline dependency caches, copied in place when they are missing.
if [ ! -f "$deps_dir/dawn_prebuilt/lib/cmake/Dawn/DawnConfig.cmake" ]; then
    if [ -d "$mp1_dir/build/android-deps/dawn_prebuilt" ]; then
        echo "android_probe: copying the prebuilt aarch64 Dawn cache into $deps_dir"
        mkdir -p "$deps_dir"
        cp -a "$mp1_dir/build/android-deps/dawn_prebuilt" "$deps_dir/dawn_prebuilt"
    else
        echo "error: $deps_dir/dawn_prebuilt is missing and there is no copy at" >&2
        echo "       $mp1_dir/build/android-deps/dawn_prebuilt" >&2
        exit 1
    fi
fi
if [ ! -f "$deps_dir/sdl/CMakeLists.txt" ]; then
    mp1_sdl=$mp1_dir/android/app/.cxx/RelWithDebInfo/645t3x2r/arm64-v8a/_deps/sdl-src
    if [ -d "$mp1_sdl" ]; then
        echo "android_probe: copying the SDL3 source cache into $deps_dir"
        mkdir -p "$deps_dir"
        cp -a "$mp1_sdl" "$deps_dir/sdl"
    else
        echo "error: $deps_dir/sdl is missing and there is no copy at $mp1_sdl" >&2
        echo "       (any source tree of the SDL3 version Aurora pins works)" >&2
        exit 1
    fi
fi

nod_args=
if [ "$nod_mode" = rust ]; then
    # FETCHCONTENT_SOURCE_DIR_AURORA_NOD / _CORROSION: the two FetchContent
    # packages the vendor nod build would otherwise clone from GitHub. The
    # package names are the ones Aurora declares - `aurora_nod` in
    # extern/aurora/cmake/AuroraNodProvider.cmake and `Corrosion` in nod's own
    # CMakeLists.txt - upper-cased for the variable, exactly as
    # android/app/build.gradle maps them.
    for pair in "aurora_nod:Cargo.toml" "corrosion:CMakeLists.txt"; do
        dep=${pair%%:*}
        marker=$deps_dir/$dep/${pair#*:}
        if [ ! -f "$marker" ]; then
            if [ -d "$mp1_dir/build/android-deps/$dep" ]; then
                echo "android_probe: copying the $dep cache into $deps_dir"
                mkdir -p "$deps_dir"
                cp -a "$mp1_dir/build/android-deps/$dep" "$deps_dir/$dep"
            else
                echo "error: $deps_dir/$dep is missing and there is no copy at" >&2
                echo "       $mp1_dir/build/android-deps/$dep" >&2
                echo "       (fetchCorrosion/nod once with network, or use --nod-stub)" >&2
                exit 1
            fi
        fi
    done

    # -DRust_CARGO_TARGET: Corrosion would otherwise build nod for the host
    # triple. The rest is the environment cargo needs to link for Android (the
    # same set tools/android_apk.sh exports).
    rust_dir=${ANDROID_RUST_DIR:-}
    if [ -z "$rust_dir" ]; then
        for candidate in "$root_dir/build/android-rust" "$mp1_dir/build/android-rust"; do
            if [ -x "$candidate/cargo/bin/cargo" ] && [ -d "$candidate/rustup/toolchains" ]; then
                rust_dir=$candidate
            fi
        done
    fi
    if [ -z "$rust_dir" ]; then
        echo "error: no vendored Rust toolchain found (set ANDROID_RUST_DIR, or use --nod-stub)" >&2
        echo "       looked in $root_dir/build/android-rust and $mp1_dir/build/android-rust" >&2
        exit 1
    fi
    if ! CARGO_HOME="$rust_dir/cargo" RUSTUP_HOME="$rust_dir/rustup" \
        "$rust_dir/cargo/bin/cargo" --version >/dev/null 2>&1; then
        echo "error: $rust_dir/cargo/bin/cargo does not run (set ANDROID_RUST_DIR)" >&2
        exit 1
    fi

    ndk_host=linux-x86_64
    case $(uname -s) in
        Darwin) ndk_host=darwin-x86_64 ;;
    esac
    ndk_bin=$ndk/toolchains/llvm/prebuilt/$ndk_host/bin
    cargo_linker=$ndk_bin/aarch64-linux-android${ANDROID_API:-28}-clang

    # CARGO_NET_OFFLINE: cargo must not try to update the registry index or
    # fetch a crate; the caches in $rust_dir/cargo are the whole crate source.
    CARGO_HOME="$rust_dir/cargo"
    RUSTUP_HOME="$rust_dir/rustup"
    PATH="$rust_dir/cargo/bin:$PATH"
    CARGO_NET_OFFLINE=true
    CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER="$cargo_linker"
    CC_aarch64_linux_android="$cargo_linker"
    CXX_aarch64_linux_android="$ndk_bin/aarch64-linux-android${ANDROID_API:-28}-clang++"
    AR_aarch64_linux_android="$ndk_bin/llvm-ar"
    RANLIB_aarch64_linux_android="$ndk_bin/llvm-ranlib"
    export CARGO_HOME RUSTUP_HOME PATH CARGO_NET_OFFLINE
    export CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER
    export CC_aarch64_linux_android CXX_aarch64_linux_android
    export AR_aarch64_linux_android RANLIB_aarch64_linux_android

    # MP_ANDROID_NOD_STUB=OFF explicitly: switching back from --nod-stub in the
    # same build directory must clear the cached ON, or Aurora would keep using
    # the stub it finds as `nod::nod` ("aurora: Using existing nod").
    nod_args="-DMP_ANDROID_NOD_STUB=OFF
    -DRust_CARGO_TARGET=aarch64-linux-android
    -DFETCHCONTENT_SOURCE_DIR_AURORA_NOD=$deps_dir/aurora_nod
    -DFETCHCONTENT_SOURCE_DIR_CORROSION=$deps_dir/corrosion"
    echo "android_probe: nod mode: rust ($rust_dir, cargo --offline)"
else
    nod_args=-DMP_ANDROID_NOD_STUB=ON
    echo "android_probe: nod mode: stub (platform/android_probe/nod_stub.c)"
fi

launcher_args=
if command -v ccache >/dev/null 2>&1; then
    launcher_args="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
fi

# -DPython3_EXECUTABLE: FindPython3 would otherwise look in the sysroot.
# -DMP_SDK_HEADERS_ONLY=OFF: the real link, which is what M0 gates.
# $nod_args: MP_ANDROID_NOD_STUB=ON for the stub mode, or the nod/corrosion
#   source-directory pair plus the Rust target for the real nod.
# AURORA_*_PROVIDER=vendor/package + the FETCHCONTENT_SOURCE_DIR_* pair: every
# dependency comes from the caches above, never from the network.
# shellcheck disable=SC2086
"$cmake" -S "$root_dir" -B "$build_dir" -G Ninja \
    -DCMAKE_MAKE_PROGRAM="$ninja" \
    -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-${ANDROID_API:-28} \
    -DANDROID_STL=c++_static \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    $launcher_args \
    -DMP_SDK_HEADERS_ONLY=OFF \
    $nod_args \
    -DPython3_EXECUTABLE="$python3" \
    -DAURORA_SDL3_PROVIDER=vendor \
    -DAURORA_SDL3_LINKAGE=static \
    -DAURORA_DAWN_PROVIDER=package \
    -DAURORA_DAWN_LINKAGE=static \
    -DAURORA_NOD_PROVIDER=vendor \
    -DAURORA_NOD_LINKAGE=static \
    -DFETCHCONTENT_SOURCE_DIR_DAWN_PREBUILT="$deps_dir/dawn_prebuilt" \
    -DFETCHCONTENT_SOURCE_DIR_SDL="$deps_dir/sdl" \
    "$@"

exec "$cmake" --build "$build_dir" --target metroid_prime2_port --parallel
