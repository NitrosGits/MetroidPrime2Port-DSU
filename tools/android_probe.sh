#!/bin/sh
# Milestone M0 of docs/ANDROID.md: a device-free arm64 cross-build of this tree
# that links `libmetroid_prime2_port.so`.
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
#
# Neither is in git (`build/` is ignored); missing ones are copied from
# ../MetroidPrimePort when that checkout still has them, and the script says how
# to supply them otherwise.
#
# Usage: tools/android_probe.sh [-DEXTRA=...]. ANDROID_BUILD_DIR and ANDROID_DEPS_DIR
# override the two build directories, MP_REVIEW_TOOLS the cmake/ninja prefix.
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
# This machine has no system cmake/ninja; the review-tools prefix (see AGENTS.md) does.
review_bin=${MP_REVIEW_TOOLS:-"$root_dir/../MetroidPrimePort/build/review-tools/bin"}
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

# The two offline dependency caches, copied in place when they are missing.
mp1_dir=$root_dir/../MetroidPrimePort
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

launcher_args=
if command -v ccache >/dev/null 2>&1; then
    launcher_args="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
fi

# -DPython3_EXECUTABLE: FindPython3 would otherwise look in the sysroot.
# -DMP_SDK_HEADERS_ONLY=OFF: the real link, which is what M0 gates.
# -DMP_ANDROID_NOD_STUB=ON: Aurora reads the disc through nod; the Rust build is not
#   reachable offline, and platform/android_probe/nod_stub.c is the stream API it needs.
# AURORA_*_PROVIDER=vendor/package + the FETCHCONTENT_SOURCE_DIR_* pair: every
# dependency comes from the two caches above, never from the network.
"$cmake" -S "$root_dir" -B "$build_dir" -G Ninja \
    -DCMAKE_MAKE_PROGRAM="$ninja" \
    -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-28 \
    -DANDROID_STL=c++_static \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    $launcher_args \
    -DMP_SDK_HEADERS_ONLY=OFF \
    -DMP_ANDROID_NOD_STUB=ON \
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
