#!/bin/sh
# Builds this tree's Android APK. Modelled on ../MetroidPrimePort/tools/android_apk.sh, which
# is the same pipeline: resolve the SDK, the pinned NDK and a Gradle 8.x, write
# android/local.properties so AGP knows where the SDK is, then hand the rest to Gradle
# (android/app/build.gradle drives CMake through externalNativeBuild).
#
# Usage: tools/android_apk.sh :app:assembleDebug [gradle args...]
#   --strict-signing   refuse to sign a release with the shared debug key instead of
#                      falling back to it (see below)
#
# Milestone M1 of docs/ANDROID.md builds :app:assembleDebug; the release variant and its key
# are wired up here too so M4 does not have to revisit it. android/app/build.gradle still
# defaults -PandroidNodStub to true, so the plain command builds the nod stub (launches,
# stops at the disc); an APK that can read a disc needs -PandroidNodStub=false, which also
# needs the vendored Rust toolchain this script puts on PATH.
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
home=${HOME:-/nonexistent}
sdk=${ANDROID_SDK_ROOT:-${ANDROID_HOME:-$home/android/sdk}}
ndk=${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}

if [ ! -d "$sdk" ]; then
    echo "error: Android SDK not found; set ANDROID_SDK_ROOT" >&2
    exit 1
fi
# The NDK app/build.gradle pins (ndkVersion) first: ANDROID_NDK_HOME set here becomes Gradle's
# ndkPath, which overrides ndkVersion.
pinned_ndk=$(sed -n 's/^[[:space:]]*ndkVersion[[:space:]]*"\([^"]*\)".*/\1/p' "$root_dir/android/app/build.gradle")
if [ -z "$ndk" ] && [ -n "$pinned_ndk" ] && [ -f "$sdk/ndk/$pinned_ndk/source.properties" ]; then
    ndk="$sdk/ndk/$pinned_ndk"
fi
if [ -z "$ndk" ]; then
    for candidate in "$sdk"/ndk/* "$home"/android-ndk-cache/android-ndk-*; do
        if [ -f "$candidate/source.properties" ]; then
            ndk=$candidate
        fi
    done
    if [ -n "$ndk" ]; then
        echo "note: NDK $pinned_ndk (app/build.gradle) is not installed; using $ndk" >&2
    fi
fi
if [ -z "$ndk" ] || [ ! -f "$ndk/source.properties" ]; then
    echo "error: Android NDK not found; set ANDROID_NDK_HOME" >&2
    exit 1
fi
ANDROID_NDK_HOME=$ndk
export ANDROID_NDK_HOME

gradle=${GRADLE:-}
if [ -z "$gradle" ]; then
    gradle=$(command -v gradle || true)
fi
if [ -z "$gradle" ]; then
    for candidate in "$home"/.gradle/wrapper/dists/gradle-8.*/*/gradle-8.*/bin/gradle; do
        if [ -x "$candidate" ]; then
            gradle=$candidate
        fi
    done
fi
if [ -z "$gradle" ]; then
    echo "error: Gradle 8.x not found; set GRADLE" >&2
    exit 1
fi

# The host interpreter CMake runs tools/scope_module.py with; build.gradle defaults to
# /usr/bin/python3, so this only has to say so when that is not where python3 is.
if [ -z "${MP_HOST_PYTHON3:-}" ]; then
    MP_HOST_PYTHON3=$(command -v python3 || true)
    export MP_HOST_PYTHON3
fi

# A Rust cross-toolchain for the real nod (-PandroidNodStub=false). Same discovery as
# tools/android_probe.sh: this tree's vendored copy first, then the MP1 checkout's
# (referenced, not copied - it is ~2 GB), or ANDROID_RUST_DIR to point at another. Harmless
# while nod is the stub.
mp1_dir=$root_dir/../MetroidPrimePort
rust_dir=
for candidate in "${ANDROID_RUST_DIR:-}" "$root_dir/build/android-rust" "$mp1_dir/build/android-rust"; do
    [ -n "$candidate" ] || continue
    if [ -x "$candidate/cargo/bin/cargo" ] && [ -d "$candidate/rustup/toolchains" ]; then
        rust_dir=$candidate
        break
    fi
done
if [ -n "$rust_dir" ]; then
    CARGO_HOME="$rust_dir/cargo"
    RUSTUP_HOME="$rust_dir/rustup"
    PATH="$rust_dir/cargo/bin:$PATH"
    # As in tools/android_probe.sh: the registry cache in that tree is the only crate source,
    # so cargo must not try to update the index or fetch anything.
    CARGO_NET_OFFLINE=true
    export CARGO_HOME RUSTUP_HOME PATH CARGO_NET_OFFLINE
    echo "note: Rust cross-toolchain: $rust_dir (cargo --offline)" >&2
else
    # Nothing to do for the stub (the default), but an explicit request for the real nod
    # cannot be configured without it, so say so here rather than in CMake's FindRust.
    case " $* " in
        *androidNodStub=false*|*androidNodStub=FALSE*)
            echo "error: androidNodStub=false needs a Rust cross-toolchain; none found in" >&2
            echo "       $root_dir/build/android-rust or $mp1_dir/build/android-rust" >&2
            echo "       (set ANDROID_RUST_DIR to point at one)" >&2
            exit 1
            ;;
    esac
fi

# The NDK names its host directory darwin-x86_64 on Apple Silicon as well.
case "$(uname -s)" in
    Darwin) ndk_host=darwin-x86_64 ;;
    *) ndk_host=linux-x86_64 ;;
esac
ndk_bin="$ndk/toolchains/llvm/prebuilt/$ndk_host/bin"
CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER="$ndk_bin/aarch64-linux-android28-clang"
CC_aarch64_linux_android=$CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER
CXX_aarch64_linux_android="$ndk_bin/aarch64-linux-android28-clang++"
AR_aarch64_linux_android="$ndk_bin/llvm-ar"
RANLIB_aarch64_linux_android="$ndk_bin/llvm-ranlib"
export CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER CC_aarch64_linux_android
export CXX_aarch64_linux_android AR_aarch64_linux_android RANLIB_aarch64_linux_android

escape_property() {
    printf '%s' "$1" | sed 's/\\/\\\\/g; s/:/\\:/g; s/ /\\ /g'
}

# Only sdk.dir is ours; anything else in local.properties is kept.
properties="$root_dir/android/local.properties"
{
    printf 'sdk.dir=%s\n' "$(escape_property "$sdk")"
    if [ -f "$properties" ]; then
        grep -v '^sdk\.dir=' "$properties" || true
    fi
} > "$properties.tmp"
mv "$properties.tmp" "$properties"

# A release is normally signed with this project's own key, configured in
# android/keystore.properties (see tools/make_android_keystore.sh). When there is no key,
# :app:assembleRelease stops rather than quietly signing with the shared debug key, so this
# script opts in on the caller's behalf for a build that is only going to be sideloaded - and
# says so, because a debug-signed package cannot be replaced by a properly signed one without
# uninstalling it first. A build that must not produce a debug-signed package passes
# --strict-signing and gets the refusal instead. assembleDebug ignores all of it.
strict=0
# Consumed here rather than forwarded: Gradle rejects an option it does not know, so leaving
# --strict-signing in the argument list fails the build with a usage dump instead of doing what
# it says. Rotating the positional parameters is the POSIX way to drop one without an array.
remaining_count=$#
index=0
while [ "$index" -lt "$remaining_count" ]; do
    argument=$1
    shift
    if [ "$argument" = "--strict-signing" ]; then
        strict=1
    else
        set -- "$@" "$argument"
    fi
    index=$((index + 1))
done

key_configured=0
[ -f "$root_dir/android/keystore.properties" ] && key_configured=1
[ -n "${MP_APK_KEYSTORE:-}" ] && key_configured=1
if [ "$key_configured" -eq 0 ]; then
    if [ "$strict" -eq 1 ]; then
        echo "error: --strict-signing and no release key configured." >&2
        echo "       run tools/make_android_keystore.sh first." >&2
        exit 1
    fi
    echo "note: no release key configured; a release build is signed with the shared debug key." >&2
    echo "      This build is only good for sideloading. To sign it properly, run" >&2
    echo "      tools/make_android_keystore.sh. Use --strict-signing to refuse instead." >&2
    set -- "$@" -PmpDebugSigning=true
fi

exec "$gradle" --project-dir "$root_dir/android" --no-daemon "$@"
