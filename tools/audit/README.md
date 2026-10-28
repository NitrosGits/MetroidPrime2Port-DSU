# tools/audit

Scripts for the code audit's mechanical sweeps (ledger: `docs/AUDIT.md`). Each script is read-only
over the source tree and the disc, prints findings as ledger-ready rows, and documents its own
usage in its header.

| sweep | script | what it checks |
|-------|--------|----------------|
| 1 | `asset_names.py` | locator/bone/particle/sound/anim name literals vs the disc (CINF, ANCS/CHAR) |
| 2 | `frame_rate.py` | per-frame state (counters, cooldowns) on render/frame-begin paths |
| 3 | `host_safety.py` | ctor init gaps, LP64 casts, raw disc reads, `new(retail size)`, data-fed divisors and indices |
| 5b | `upstream_units.py` | NonMatching/unlinked units and NONMATCHING/TODO/guessed functions, ranked for review |
