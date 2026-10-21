#!/bin/bash
# tools/new_worktree.sh <dir> <branch> [start=port]
# A ready-to-build lane worktree: `git worktree add`, AGENTS.md/CLAUDE.md symlinks, orig/G2ME01 disc
# symlinks, the extern/musyx submodule, the generated DefaultFont*.inc (copied from this tree instead
# of a full upstream build), and build-port-link configured for the game binary. Run from wt-mp2-port.
set -euo pipefail
DIR=$1 BRANCH=$2 START=${3:-port}
SRC=$(git rev-parse --show-toplevel)
T=$SRC/../MetroidPrimePort/build/review-tools/bin
git worktree add -q "$DIR" -b "$BRANCH" "$START"
DIR=$(cd "$DIR" && pwd)
ln -sf "$SRC/AGENTS.md" "$DIR/AGENTS.md"
ln -sf "$SRC/CLAUDE.md" "$DIR/CLAUDE.md"
for f in "$SRC"/orig/G2ME01/*; do
  [ -L "$f" ] && ln -sfn "$(readlink "$f")" "$DIR/orig/G2ME01/$(basename "$f")"
done
git -C "$DIR" submodule update -q --init extern/musyx
mkdir -p "$DIR/build/G2ME01/include/MetroidPrime"
cp "$SRC"/build/G2ME01/include/MetroidPrime/DefaultFont*.inc "$DIR/build/G2ME01/include/MetroidPrime/"
"$T/cmake" -S "$DIR" -B "$DIR/build-port-link" -G Ninja -DCMAKE_MAKE_PROGRAM="$T/ninja" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMP_SDK_HEADERS_ONLY=OFF \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_LINKER_TYPE=MOLD \
  > "$DIR/build-port-link.configure.log" 2>&1 || { echo "configure failed: $DIR/build-port-link.configure.log"; exit 1; }
echo "$DIR ready: \$T/ninja -C $DIR/build-port-link metroid_prime2_port (python3 configure.py && \$T/ninja for the asm/matching build)"
