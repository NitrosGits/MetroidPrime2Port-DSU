# Lane brief: the standing part

Every decomp lane brief says "read docs/LANE_BRIEF.md first" and then only gives the slice. The rules
behind this are in `docs/DECOMP_WORKFLOW.md` ("Done means verified", "Lane briefs"); this is the checklist.

## Start

1. `git status --short` clean, then `git merge port` (unless the brief names another base). Note the
   slice base (`git rev-parse HEAD`) for the report and the review.
2. Read your module note (`docs/module-analysis/<Module>.md`) before re-deriving anything.

## Work

- Every bit test, mask, compare, store and rodata constant carries its asm offset (`// 0x1234`).
- Mark (`// @ fn_<mod>_<off>`) only a body whose every live arm ran; a dead arm needs proof (the
  only writer, the shipped data scan). Forced arms count, labelled `forced (<cmd>)` in the note's branch table.
- Don't edit shared code (`platform/`, `CGenericFSM2*`, DOL classes, other modules' headers). Report a
  shared bug with file:line and evidence; ask for missing console commands/accessors in the report.
- Probes are temporary: never committed; `git diff <base>` must show none.

## Runs

- Foreground with `timeout`; fast mode `MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8` (use 1 for
  wall-timed plans) and `tools/drivers/con.py` `@f<n>` scripts; drivers go in `tools/drivers/`.
- Console (`help` lists all): `fsm <id> <state>` (reaches FSM2 sub-machines), `place <id> player [d]`,
  `damage <id> <n> [weapon|powerbomb|annihilator|darkburst|sunburst|sonicboom]`, `send <id> <msg>`,
  `shoot`, `grapple <id> [d]`, `objs <type>`, `status`. `a; b; c` runs in one tick. `kill` takes no
  argument (it kills the player).
- Kill only pids you launched (bracketed `pgrep -f` names) and any Xvfb you started. Delete your run
  outputs from `/tmp` (a 31 GB tmpfs).

## Gates (one line each in the report)

Build `$T/ninja -C build-port-link metroid_prime2_port` 0 FAILED; `python3 tools/gen_files_cmake.py
--check` rc 0; `tools/review_decomp_diff.sh <base>` output (every MARK/ASM line explained); the run:
area, frames, `[port] caught` / `reach-stub` count 0; upstream-file edits: TARGET_PC strip-diff clean
and `python3 configure.py && $T/ninja` "87 files OK".

## Report

Write `/tmp/opencode/reports/<lane>.md` (overwrite) and reply with it, under ~400 tokens in the reply:
first line `STATUS: DONE` or `STATUS: INCOMPLETE — <reason>`; commits with base; each function: marked
or not and why (arm table in the note, not the reply); files with line numbers; gates; asks; caveats.
