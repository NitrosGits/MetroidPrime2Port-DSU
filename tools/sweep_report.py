#!/usr/bin/env python3
"""Digest a `tools/sweep_areas.sh` run into a per-area verdict table.

    python3 tools/sweep_report.py '/tmp/opencode/log_tg_*.log'

The sweep's logs are the only record of what each area did, and a raw one is ~500 lines of
`[aram]`/`[ids]` noise per shard. This reads them and answers the two questions a sweep is for:

  * **coverage** - which area slots of the world were actually played, in what order, and which
    were missed (a `NEXTAREA` token that never fired, or a hop that never arrived);
  * **walls** - what stopped each area that did not simply play.

Everything is keyed on two line families the port already prints: `[area] N` marks each arrival
(`CStateManager::Update` prints it whenever the current area changes), and `[ids] frame N: ...` is
the 300-frame id-pool sample that says how much of the area was live. A "phase" is the block of log
between one `[area] N` and the next, so the id high-water, the missing resources and the warp route
are all attributed to the area they happened in. A hop's shape is read from its `[warp] area N:`
lines: "streaming (N of 1024 object ids free)" is the port's own direct route, "handing the hop to
the game's own transition" is the hand-over.

Verdicts, worst first:

    CRASH    the fault handler (`[port] caught`) fired in this phase, or the run's exit code is a
             signal (134 abort, 139 segfault).
    ABORT    an id-pool exhaustion or an assert stopped the run in this phase.
    STALL    no game update for the harness's 30 s window: the game stopped updating here.
    POOL     the id pool came within 64 ids of full in this phase - legal in itself, but it is the
             headroom the next hop needs, which is what the port's pre-stream drop exists to keep.
    OK       the area loaded, played, and nothing above happened.

`--verbose` also prints each phase's notable lines (everything that is not a known-benign line),
which is how a wall's cause is read out without opening the log.
"""

from __future__ import annotations

import argparse
import glob
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

AREA_RE = re.compile(r"^\[area\] (\d+)$")
IDS_RE = re.compile(r"^\[ids\] frame (\d+): (\d+)/(\d+) ids allocated, (\d+) objects$")
WORLD_RE = re.compile(r"^\[warp\] world ([0-9a-f]{8}) areas:")
EXIT_RE = re.compile(r"^exit=(\d+) at ")
HOP_RE = re.compile(r"^\[warp\] area (\d+): (.*)$")
MOVED_RE = re.compile(r"^\[warp\] (?:moved to|the game's own transition arrived in) area (\d+)")
RES_RE = re.compile(r"^\[res\] (.*)$")
FATAL = ("[port] caught",)
ABORT = ("Object list full", "assert", "Assert", "ASSERT")
# Lines that are ordinary in every area and say nothing about the area being tested.
BENIGN = (
    "[aram]",
    "[save]",
    "[input]",
    "[warp] healed to",
    "[map] no map area",
    "[ids]",
    "[skip]",
    "[reach-stub",
    "metroid_prime2_port:",
)


@dataclass
class Phase:
    area: int
    shard: int
    first_frame: int | None = None
    last_frame: int | None = None
    max_ids: int = 0
    pool_size: int = 0
    misses: list[str] = field(default_factory=list)
    route: str = ""
    fatal: list[str] = field(default_factory=list)
    abort: list[str] = field(default_factory=list)
    stall: bool = False
    stalling_index: int = -1
    fatal_index: int = -1
    stubs: int = 0
    notable: list[str] = field(default_factory=list)

    @property
    def updates(self) -> int:
        if self.first_frame is None or self.last_frame is None:
            return 0
        return self.last_frame - self.first_frame

    @property
    def sampled(self) -> bool:
        """The fault handler fired *after* the stall report: someone sampled the stalled process with
        an external signal (the port prints a backtrace for one), so the phase is a stall, not a
        crash. A real crash prints no stall report before it."""
        return self.stall and self.fatal_index > self.stalling_index >= 0

    @property
    def verdict(self) -> str:
        if self.stall:
            return "STALL"
        if self.fatal:
            return "CRASH"
        if self.abort:
            return "ABORT"
        if self.pool_size and self.max_ids >= self.pool_size - 64:
            return "POOL"
        return "OK"

    @property
    def evidence(self) -> list[str]:
        return self.fatal + self.abort + self.misses + self.notable


@dataclass
class Shard:
    path: Path
    index: int
    world: str = ""
    world_areas: list[int] = field(default_factory=list)
    exit_code: int | None = None
    head: str = ""
    tail: list[str] = field(default_factory=list)
    requested: list[int] = field(default_factory=list)
    arrived: list[int] = field(default_factory=list)
    # From the sweep's manifest: the slice this shard was asked to walk.
    expected_first: int | None = None
    expected_count: int | None = None


def read_manifest(logs: list[Path]) -> dict[int, tuple[int, int]]:
    """`sweep_areas.sh` writes `sweep_<tag>.txt` next to the logs: shard -> (first area, count)."""
    slices: dict[int, tuple[int, int]] = {}
    tags = set()
    for path in logs:
        m = re.match(r"log_(.+)_(\d+)\.log$", path.name)
        if m:
            tags.add(m.group(1))
    for tag in tags:
        for manifest in glob.glob(str(log_dir(logs) / f"sweep_{tag}.txt")):
            for line in Path(manifest).read_text(errors="replace").splitlines():
                fields = dict(zip(line.split()[0::2], line.split()[1::2]))
                if "shard" in fields and "first" in fields:
                    slices[int(fields["shard"])] = (int(fields["first"]),
                                                    int(fields.get("count", 0)))
    return slices


def log_dir(logs: list[Path]) -> Path:
    for path in logs:
        if path.parent.exists():
            return path.parent
    return Path("/tmp/opencode")


def shard_index(path: Path) -> int:
    m = re.search(r"_(\d+)\.log$", path.name)
    return int(m.group(1)) if m else 0


def parse(path: Path) -> tuple[Shard, list[Phase]]:
    shard = Shard(path=path, index=shard_index(path))
    info = path.with_suffix(".log.info")
    if info.exists():
        shard.head = info.read_text(errors="replace").strip()
    phases: list[Phase] = []
    current: Phase | None = None
    for index, line in enumerate(path.read_text(errors="replace").splitlines()):
        if (m := EXIT_RE.match(line)):
            shard.exit_code = int(m.group(1))
        elif (m := WORLD_RE.match(line)):
            if not shard.world:      # the run's first world line is the save's own world
                shard.world = m.group(1)
                shard.world_areas = len(line.split("areas:", 1)[1].split())
        elif (m := AREA_RE.match(line)):
            current = Phase(area=int(m.group(1)), shard=shard.index)
            phases.append(current)
            shard.arrived.append(current.area)
        else:
            if (m := IDS_RE.match(line)) and current is not None:
                frame, live, size = int(m.group(1)), int(m.group(2)), int(m.group(3))
                if current.first_frame is None:
                    current.first_frame = frame
                current.last_frame = frame
                current.max_ids = max(current.max_ids, live)
                current.pool_size = size
            elif (m := HOP_RE.match(line)) and current is not None:
                text = m.group(2)
                shard.requested.append(int(m.group(1)))
                if "streaming" in text:
                    current.route = "direct"
                elif "handing the hop" in text:
                    current.route = "handed"
                if "loading" in text or "holding" in text or "dropped the areas around" in text \
                        or "does not look like" in text:
                    pass                # the ordinary shape of every hop
                else:
                    current.notable.append(line.strip())
            elif (m := MOVED_RE.match(line)) and current is not None:
                pass
            elif current is None:
                continue
            elif (m := RES_RE.match(line)):
                current.misses.append(line.strip())
            elif "no game update for" in line:
                current.stall = True
                current.stalling_index = index
                current.notable.append(line.strip())
            elif "reach-stub" in line:
                current.stubs += 1
            elif any(p in line for p in FATAL):
                current.fatal.append(line.strip())
                if current.fatal_index < 0:
                    current.fatal_index = index
            elif any(p in line for p in ABORT):
                current.abort.append(line.strip())
            elif not any(b in line for b in BENIGN):
                current.notable.append(line.strip())
    shard.tail = path.read_text(errors="replace").splitlines()[-14:]
    return shard, phases


def main() -> int:
    ap = argparse.ArgumentParser(description="per-area verdict table for a sweep_areas.sh run")
    ap.add_argument("logs", nargs="+", help="log paths or globs (quote a glob)")
    ap.add_argument("--verbose", action="store_true", help="print each phase's notable lines")
    args = ap.parse_args()

    paths: list[Path] = []
    for pattern in args.logs:
        hits = sorted(glob.glob(pattern))
        if not hits:
            print(f"[report] no log matched {pattern}", file=sys.stderr)
        paths.extend(Path(h) for h in hits)
    if not paths:
        return 2

    shards: list[Shard] = []
    phases: list[Phase] = []
    for path in paths:
        shard, shard_phases = parse(path)
        shards.append(shard)
        phases.extend(shard_phases)

    print("== shards")
    slices = read_manifest(paths)
    for shard in sorted(shards, key=lambda s: s.index):
        end = "still running" if shard.exit_code is None else f"exit={shard.exit_code}"
        bad = [p for p in phases if p.shard == shard.index and p.verdict != "OK"]
        played = sorted({p.area for p in phases if p.shard == shard.index})
        slice_text = ""
        if shard.index in slices:
            first, count = slices[shard.index]
            if first not in played:
                slice_text = f", asked for {first}..{first + count - 1}, reached none of it"
            else:
                last = max(played)
                slice_text = (f", asked for {first}..{first + count - 1}, "
                              f"reached {first}..{last}")
        print(f"  {shard.path.name}: {end}, world {shard.world or '?'} "
              f"({shard.world_areas} areas), played {len(played)}{slice_text}, "
              f"{len(bad)} area(s) not OK")
        if shard.head:
            print(f"    {shard.head}")

    print("\n== areas: slot shard verdict updates id-high-water res route stubs")
    for phase in sorted(phases, key=lambda p: (p.area, p.shard)):
        pool = f"{phase.max_ids}/{phase.pool_size}" if phase.pool_size else "?"
        note = " (fault sampled after the stall)" if phase.sampled else ""
        print(f"  {phase.area:>3}  shard{phase.shard}  {phase.verdict:<6} {phase.updates:>5}  "
              f"{pool:>9}  {len(phase.misses):>2}  {phase.route or '-':<6}  {phase.stubs}{note}")

    # Coverage against the manifest: a shard's slice should have been walked in order, so the first
    # area it did not reach says where it stopped. An area inside the walked range that no shard
    # played is the interesting one - a token that never fired, or a hop that never arrived.
    played = {p.area for p in phases}
    requested = {a for shard in shards for a in shard.requested}
    if slices:
        print("\n== coverage")
        for index in sorted(slices):
            first, count = slices[index]
            want = list(range(first, first + count))
            missing = [a for a in want if a not in played]
            if not missing:
                continue
            shard = next((s for s in shards if s.index == index), None)
            end = "unknown" if shard is None or shard.exit_code is None else str(shard.exit_code)
            wall = [p for p in phases if p.shard == index and p.verdict != "OK"]
            inside = [a for a in missing if a in requested]
            if wall:
                reason = f"the run stopped in area {wall[-1].area} ({wall[-1].verdict})"
            elif inside:
                reason = "the run played to its frame limit, so a hop's stream never completed"
            else:
                reason = "the run ended before the slice was walked (exit %s)" % end
            print(f"  shard {index}: reached {len(want) - len(missing)}/{len(want)} of "
                  f"{first}..{first + count - 1}; first missing {missing[0]} - {reason}")
            if inside:
                print(f"     asked for but never arrived: {inside}")

    walls = [p for p in phases if p.verdict != "OK"]
    if walls:
        print("\n== walls")
        for phase in sorted(walls, key=lambda p: (p.verdict != "CRASH", p.shard, p.area)):
            print(f"  -- area {phase.area} (shard {phase.shard}): {phase.verdict}")
            for line in phase.evidence[:10]:
                print(f"       {line}")
    elif args.verbose:
        print("\n== walls: none")

    if args.verbose:
        print("\n== every phase's notable lines")
        for phase in sorted(phases, key=lambda p: (p.shard, p.area)):
            if phase.notable:
                print(f"  area {phase.area} (shard {phase.shard}):")
                for line in phase.notable:
                    print(f"       {line}")

    misses = sorted({(p.area, line) for p in phases for line in p.misses})
    if misses:
        print("\n== missing resources by area")
        for area, line in misses:
            print(f"  area {area}: {line}")

    bad_exits = [s for s in shards if s.exit_code not in (None, 0)]
    if bad_exits:
        print("\n== abnormal exits")
        for shard in bad_exits:
            print(f"  {shard.path.name}: exit={shard.exit_code}")
            for line in shard.tail:
                print(f"       {line}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
