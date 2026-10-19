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
- Sessions and lanes run only on DeepSeek (`opencode-go/deepseek-v4.1-flash`); never Opus, `haiku`, `sonnet` or
  `claude`, not even to escalate. A lane that fails twice gets a narrower brief, or the
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

## Context

- Sessions hand off (AGENTS.md "Session length and handoffs") at a committed slice rather than
  ride through compactions; the module note is what makes the handoff cheap.
- Bulky output (builds, logs, pseudo-C dumps) stays in files; read the lines that matter.
