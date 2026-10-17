#!/bin/sh
# Creates this project's own release signing key: android/keystore.jks plus the
# android/keystore.properties that android/app/build.gradle reads (both git-ignored). Until one
# exists, tools/android_apk.sh signs a release with the shared debug key when asked
# (-PmpDebugSigning=true), which is fine for sideloading and wrong for anything else.
#
#   tools/make_android_keystore.sh [-f] [-a <alias>]
#
# Read this before running it on a phone that already has the port installed: Android identifies an
# app by its package name *and* signing certificate, so a build signed with this key cannot be
# installed over one signed with the debug key - `adb install -r` fails with
# INSTALL_FAILED_UPDATE_INCOMPATIBLE, and the only way forward is `adb uninstall org.metroidprime2.port`
# first, which deletes the app's data directory: the picked disc image (files/disc.iso, a 1.4 GB copy),
# the memory card it was playing (files/user/USA), the touch layout (files/touch.cfg) and
# files/port_paths.txt. On the phone that means re-picking the ISO and copying the save back before
# the next run. The key itself must be kept: losing it loses the ability to update an installed copy.
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
android_dir=$root_dir/android
keystore=$android_dir/keystore.jks
properties=$android_dir/keystore.properties
alias=metroidprime2
force=0

while [ $# -gt 0 ]; do
    case $1 in
        -f|--force) force=1 ;;
        -a|--alias) shift; [ $# -gt 0 ] || { echo "error: -a needs an alias" >&2; exit 2; }; alias=$1 ;;
        -h|--help) sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "error: unknown argument '$1'" >&2; exit 2 ;;
    esac
    shift
done

if [ -e "$keystore" ] && [ "$force" -ne 1 ]; then
    echo "error: $keystore already exists; -f replaces it (and any installed build with the old key" >&2
    echo "       can then never be updated in place again)" >&2
    exit 1
fi

# The SDK's keytool, so this does not depend on a system JDK; $JAVA_HOME is the fallback.
sdk=${ANDROID_SDK_ROOT:-${ANDROID_HOME:-${HOME:-/nonexistent}/android/sdk}}
keytool=
for candidate in "$sdk"/build-tools/*/keytool "$sdk"/build-tools/*/apksigner; do
    [ -f "$candidate" ] || continue
    case $candidate in */keytool) keytool=$candidate ;; esac
done
if [ -z "$keytool" ] && [ -n "${JAVA_HOME:-}" ] && [ -x "$JAVA_HOME/bin/keytool" ]; then
    keytool=$JAVA_HOME/bin/keytool
fi
if [ -z "$keytool" ] && command -v keytool >/dev/null 2>&1; then
    keytool=$(command -v keytool)
fi
if [ -z "$keytool" ]; then
    echo "error: no keytool found (looked in $sdk/build-tools, \$JAVA_HOME/bin, \$PATH)" >&2
    exit 1
fi

# A random password, because it is stored next to the key anyway: the value of this file is that the
# key is this project's own, not that the password is a secret from whoever has the checkout.
password=$(od -An -N24 -tx1 /dev/urandom | tr -d ' \n')
[ -n "$password" ] || { echo "error: could not read /dev/urandom" >&2; exit 1; }

rm -f "$keystore"
"$keytool" -genkeypair -noprompt \
    -keystore "$keystore" \
    -storetype PKCS12 \
    -storepass "$password" \
    -keypass "$password" \
    -alias "$alias" \
    -keyalg RSA -keysize 4096 -validity 10000 \
    -dname "CN=Metroid Prime 2 Port, O=local, C=US" >/dev/null

umask 077
cat > "$properties" <<EOF
# Written by tools/make_android_keystore.sh; read by android/app/build.gradle. Keep both files:
# an installed app can only be updated by a package signed with the same key.
storeFile=keystore.jks
storePassword=$password
keyAlias=$alias
keyPassword=$password
EOF
chmod 600 "$keystore" "$properties"

echo "wrote $keystore and $properties"
echo "next: tools/android_apk.sh :app:assembleRelease, then"
echo "  adb uninstall org.metroidprime2.port   # one time only: the debug-key build cannot be replaced in place"
echo "  adb install android/app/build/outputs/apk/release/app-release.apk"
echo "the uninstall deletes files/disc.iso, files/user/USA, files/touch.cfg and files/port_paths.txt."
