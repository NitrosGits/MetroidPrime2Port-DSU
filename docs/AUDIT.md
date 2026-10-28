# Code audit ledger (port branch)

Plan: Project knowledge "Full code audit (port branch)". Started 2026-10-28.

Lanes write findings to `/tmp/opencode/reports/audit-<slice>.md` in the row format below; the
orchestrator verifies each
(upstream PrimeDecomp code included: it is not trusted, see Review 5b) against code evidence (pseudo-C, MP1 port code or data, disc data) and
merges it here. A run only confirms or reproduces a crash.

- **class**: `asset-name`, `frame-rate`, `host-safety`, `target-pc`, `logic`, `render`, `platform`, `upstream` (decomp code wrong vs retail asm)
- **severity**: `crash` / `gameplay` / `visual` / `minor`
- **status**: `open` / `confirmed` / `fixed <sha>` / `not-a-bug`
- Ids: `A<sweep/review>-<n>` (A1-3 = sweep 1, third finding; A5b-2 = Review 5b).

Scripts: `tools/audit/` (see its README).

## Findings

| id | area | file:line | class | severity | evidence | status |
|----|------|-----------|-------|----------|----------|--------|
| A0-1 | CommandoPirate | src/MetroidPrime/Enemies/PortCCommandoPirate.cpp | asset-name | gameplay | E-grenades launched from a misread locator (burst at the feet) | fixed 051efd4c6 |
| A0-2 | Rezbit | src/MetroidPrime/CRezbitEffect.cpp | frame-rate | gameplay | hack freeze counted per frame, too short uncapped | fixed 4cd2ec2b9 |

## Sweep log

| sweep | lane report | base | date | result |
|-------|-------------|------|------|--------|
