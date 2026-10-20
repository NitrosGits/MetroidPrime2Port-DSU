# Module decomp workflow (parallel sessions)

How a module session and its lanes work so they don't redo each other's work. Lanes get a
pointer to this file plus their slice, not a restatement of it.

## Ownership

- One session per module family, one branch `decomp/<family>`, one worktree. It owns its
  `src/MetroidPrime/Enemies/Port<Class>*.cpp`, its headers' `TARGET_PC` blocks, and
  `docs/module-analysis/<Module>.md`. Nothing else.
- Shared files (`tools/modules_port.txt`, `src/MetroidPrime/PortModuleStubs.cpp`, `files.cmake`,
  `docs/modules.tsv`, any upstream file another family also includes): change only the lines
  for your own module, and say so in the report. The orchestrator resolves the merge.
- Shared tooling lives on `port` and is not rebuilt per session. Need a tool change? Ask the
  orchestrator; don't fork a copy. Current tools:
  - `tools/rel_pseudo/` - REL plf -> ET_EXEC for REA/Ghidra, split per function, `relsim.py`
    REL similarity. (Supersedes `tools/rel_plf2exec.py`; don't add another linker.)
  - m2c pseudo-C over the whole REL; REA `batch_decompile`.
- One orchestrator (the port session) runs every family and reviews every lane patch; there are
  no per-family orchestrator sessions. Lanes run only on MiniMax free
  (`opencode-go/minimax-m3.1-flash-free`, passed explicitly). Never DeepSeek, `haiku`, `sonnet`
  or `claude` lanes, not even to escalate. A lane that fails twice gets a narrower brief, or the
  orchestrator does the piece itself.
- Each lane gets an exclusive list of functions (by `fn_<id>_<off>`) and files. Two lanes never
  hold the same file at once; run them in sequence, or split the file.

## Baseline

- A lane starts from a named commit on the session branch and commits on top of it. No
  rebases: if the branch moved, the session merges. A patch with unrelated or reversed hunks
  is rejected and redone, not repaired by hand.
- Sessions merge `port` only when the orchestrator announces a merge point, not ad hoc.

## Don't re-investigate

- Decompile a module once: write the pseudo-C to `/tmp/opencode/pseudo/<Module>/` (one `.c`
  per function) and have lanes `grep`/read ranges there. Do not re-run REA per lane.
- Every verified fact (layout offsets, vtable slots, state/trigger tables, constants, anim
  ids, spawn area + layer that works) goes into `docs/module-analysis/<Module>.md` in a
  terse table the moment it is verified. A lane reads that note first and does not
  re-derive what is there. Wrong entries are deleted, not annotated.
- Briefs give paths and line ranges, never file contents.

## Done means verified

A lane reports `STATUS: DONE` only with all of these pasted (one line each):
host build `0 FAILED`; `gen_files_cmake.py --check` rc 0; for upstream-file edits, the
TARGET_PC strip-diff CLEAN and the matching build's `fuzzy/matched` percentages; the runtime
area, frame count and `grep -c '[port] caught\|reach-stub'` = 0. Anything skipped makes it
`STATUS: INCOMPLETE - <what>`; markers (`// @ fn_<id>_<off>`) go only on functions the run
exercised.

## Review every lane patch

MiniMax lanes pass the gate first time but slip in the same places. Before committing a lane's
patch, run `tools/review_decomp_diff.sh <lane-base>` and answer every hit:

- **NULL** - null guards retail doesn't have. Keep one only if the asm has the test or a named
  port limit requires it (say which in the comment); otherwise delete it.
- **MASK** - re-derive each bit test from the asm `rlwinm`/`andi.` yourself (`rlwinm rD,rS,0,21,21`
  is `& 0x400`, not `& 0x20`: PowerPC bit N is `1u << (31-N)`, never `1u << N`). A selftest written from the same reading proves nothing.
- **ADDR / STUB** - no thunk that compares against a retail address and otherwise calls a stub;
  no stand-in left behind a `// @`.
- **MARK** - a marker means the body is retail's and ran. A function that mostly calls unported
  functions is not ported: unmark it or port the callees. A body another lane already ported is
  called, not copied (`grep -rn 'fn_<id>_<off>' src/`).
- Lanes never run `git reset --hard`, `git clean`, `git stash`, `git checkout -- <path>` or `rm -rf`
  on a path held in a variable; reject a patch made that way and re-brief.

## Context

- Sessions hand off (AGENTS.md "Session length and handoffs") at a committed slice rather than
  ride through compactions; the module note is what makes the handoff cheap.
- Bulky output (builds, logs, pseudo-C dumps) stays in files; read the lines that matter.
