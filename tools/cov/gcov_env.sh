#!/bin/bash
# The gcov environment of a coverage run. Sourced by the run scripts; it defines one function.
#
# A coverage build (CMake option MP_PORT_COVERAGE) writes its .gcda files next to the objects
# unless GCOV_PREFIX says otherwise, and two runs of the same build would then write over each
# other - the lanes run many at once. GCOV_PREFIX moves the .gcda files into the run's own
# directory, and GCOV_PREFIX_STRIP drops the build directory's own path components, so the tree
# under the prefix mirrors the build tree: the report tool finds the .gcno of a .gcda that way
# (tools/cov/gcov_report.py).
#
#   gcov_env <build dir> <output dir>
#
# Exports GCOV_PREFIX and GCOV_PREFIX_STRIP when the binary in the build directory is a coverage
# build, and does nothing otherwise: a normal build is never touched. Set MP2_COVERAGE=1 or 0 to
# decide by hand (a build without symbols, for instance).
#
# See docs/LANE_BRIEF.md, "Coverage without probes", and docs/RUNNING.md.

# The number of path components of a directory: the .gcda path is the object's path, and the
# object's path starts with the build directory.
gcov_strip_depth() {
  printf '%s' "$1" | awk -F/ '{n=0; for (i=1;i<=NF;i++) if ($i != "") n++; print n}'
}

gcov_env() {
  local build=$1 out=$2 bin coverage=""
  bin=$build/metroid_prime2_port
  [ -x "$bin" ] || return 0
  if [ -n "${MP2_COVERAGE:-}" ]; then
    coverage=$MP2_COVERAGE
  elif nm -g "$bin" 2>/dev/null | grep -q '__gcov_dump'; then
    # libgcov is in the binary only in a coverage build (__gcov_dump is its marker).
    coverage=1
  fi
  [ "$coverage" = "1" ] || return 0
  local abs
  abs=$(cd "$build" && pwd) || return 0
  export GCOV_PREFIX="$out/gcov"
  export GCOV_PREFIX_STRIP=$(gcov_strip_depth "$abs")
  echo "[gcov] coverage build: GCOV_PREFIX=$GCOV_PREFIX GCOV_PREFIX_STRIP=$GCOV_PREFIX_STRIP" >&2
}
