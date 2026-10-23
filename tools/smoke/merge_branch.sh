#!/bin/bash
# usage: tools/smoke/merge_branch.sh <branch-sha> <label>
# Merges a smoke-family branch: tables via merge_family.py, RUNNING.md by union (dedupe it by hand after).
set -u
b=$1; lab=$2
git merge --no-commit --no-ff "$b" >/dev/null 2>&1
for f in tools/smoke/run.sh tools/smoke/compare.py; do git show HEAD:$f > $f; done
if git ls-files -u docs/RUNNING.md | grep -q .; then
  git show :1:docs/RUNNING.md > /tmp/opencode/rm.base; git show :2:docs/RUNNING.md > /tmp/opencode/rm.ours; git show :3:docs/RUNNING.md > /tmp/opencode/rm.theirs
  git merge-file --union -p /tmp/opencode/rm.ours /tmp/opencode/rm.base /tmp/opencode/rm.theirs > docs/RUNNING.md
fi
python3 tools/smoke/merge_family.py "$b" || exit 1
bash -n tools/smoke/run.sh || exit 1
git add tools/smoke docs/RUNNING.md
if git ls-files -u | grep -q .; then echo "UNRESOLVED:"; git ls-files -u; exit 1; fi
git commit -qm "Merge smoke $lab ($b) into batch33" && echo "ok $lab"
