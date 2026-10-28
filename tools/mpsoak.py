#!/usr/bin/env python3
"""Pre-release crash soak on the harness (`tools/run_gameplay.sh`).

Drives the shipping matrix -- the user's mods folder on and off, Dawn validation/robustness
on and off, MSAA 1 and 4 -- through a fixed tour of areas across the five campaign worlds,
capturing one screenshot per area, and fails on anything that killed the game: a crash, a
WebGPU error, a device loss, a vertex-array overrun or an aurora FATAL.

    python3 tools/mpsoak.py --evidence-dir build/s-release-<date>/soak
    python3 tools/mpsoak.py --dry-run --evidence-dir /tmp/soak-dry    # print the commands

MP1's `mpsoak.py` rides `tools/mprig.py`, a rig manager that keeps one game alive across a
tour of rooms; this tree has no such tool. The soak is therefore built on the harness it does
have: one `tools/run_gameplay.sh` game per (mods, toggles), booted straight into the tour's
first area (`WORLD=<world>,<area>`, docs/RUNNING.md "Booting into a world and an area"), the
rest of the tour reached with the console's `world <world> <area>` and `warp <area>`
(`tools/mp2con.py`) and MSAA switched inside that game with `msaa N`, so one process covers
both sample counts. Screenshots come from the display the run's own Xvfb is on
(`tools/shot.sh`). A dead game is detected from the harness process leaving, the log's fault
tried to be named, and the run restarted; the tour then carries on with the next area.

`--rooms` names the tour as `world:area` (the world half is what `tools/world_areas.py`
resolves -- `agon`, `torvus`, an MLVL id, a part of its name; the area half is a slot index,
an MREA id or (part of) the area's name in that world's MLVL). Both halves become ids before
they reach the game, so nothing depends on name resolution at warp time. Screenshots go to
`<evidence>/<mode>-<toggles>-msaa<N>/<NN>-<area>.png`, one row per (mode, toggles, msaa, area)
to `<evidence>/summary.tsv`, and the exit status is 1 when any row FAILs.
"""

import argparse
import atexit
import difflib
import os
import re
import shlex
import signal
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUN = ROOT / "tools" / "run_gameplay.sh"
SHOT = ROOT / "tools" / "shot.sh"
CON = ROOT / "tools" / "mp2con.py"
AREAS = ROOT / "tools" / "world_areas.py"
GAMES = ROOT / "tools" / "games_running.sh"
BINARY = "metroid_prime2_port"
BOOT_TIMEOUT = 600  # seconds from launch to `[harness] gameplay reached`; a busy machine waits for a slot
WARP_TIMEOUT = 90  # seconds for one area warp to land (a cold area is ~10 s)
CON_TIMEOUT = 60  # seconds for one console command and its reply
STOP_TIMEOUT = 30  # seconds for a stopped run to leave on SIGTERM

# The default tour: two or three areas per world, so every world is drawn and both a light and a
# dark area of each are seen (Dark Aether's atmosphere and its safe-zone volumes are what the
# MSAA/depth-copy path is for). Every entry boots and plays in the smoke suite or the sweep, so a
# FAIL here is something the shipping matrix did, not an area no run has ever reached. Areas whose
# warp lands on a save station are kept out: the save prompt pauses the game, and a paused game
# performs no game update, so every later warp would time out as "never landed".
DEFAULT_ROOMS = [
    "temple grounds:02_temple_exploration",
    "temple grounds:0b_temple_hall",
    "temple grounds:0t_temple_hall_dark",
    "agon:02_sand_sandfalls",
    "agon:0b_sand_hall",
    "agon:01_sand_entrance_dark",
    "torvus:01_swamp_entrance",
    "torvus:09_swamp_catacombs_dark",
    "sanctuary:0b_cliff_hall",
    "sanctuary:0f_cliff_hall",
    "sanctuary:0p_cliff_hall",
    "great temple:0i_temple_hall",
]

# MP2 ships no remastered asset set, so the asset axis is the mods folder instead: `mods` runs the
# user's own (texture packs included, which is what a player has installed), `none` runs a shipping
# build with nothing installed (`MP2_MODS` at an empty directory, so the card's mods stay untouched).
MODES = {"mods": {}, "none": "no-mods"}
# Dawn validation and robustness back on: any WebGPU error is then fatal in game. A release build
# already turns both off (`NDEBUG` in aurora's toggle list), which is the `release` cell.
TOGGLES = {
    "validate": {"MP2_DAWN_DISABLE": "skip_validation,disable_robustness"},
    "release": {},
}
# Every cell also checks the GX array bounds: a draw that would read past its vertex arrays is
# dropped and logged, which is how the 807013b1e-style overrun is caught by a run instead of a crash.
ALWAYS = {"MP_GX_CHECK_ARRAYS": "1"}

# The port logs `port: crashed: <signal>` with a stack, aurora logs `[fatal] [module] message`
# (platform/main.cpp's logCallback, the same shape as MP1's). A `Device lost` line is how a lost
# device appears mid-run; the shutdown one (`Device lost: Device was destroyed`) is the software
# device going away at exit, which every run prints, so it is a warn and not a FAIL.
FAIL_TEXT = ("port: crashed:", "[port] caught", "WebGPU error", "Device lost:", "[fatal]")
WARN_TEXT = ("not supported; skipped", "[reach-stub", "is a port stand-in")
WARN_TEXT_EXCEPTIONS = ("Device lost: Device was destroyed",)

SLOT_RE = re.compile(r"^(\d+) ([0-9a-fA-F]{8}) (light|dark) (\S+)$")
WORLD_RE = re.compile(r"^world ([0-9a-fA-F]{8}) (\S+) (\d+) (.*)$")
# An area warp lands when the game's current area changes (`[area] N`, CStateManager::Update) or
# when the warp's own target handler reports the move.
ARRIVED_RE = re.compile(r"^\[area\] \d+$|^\[warp\] moved to area \d+$", re.M)


def die(msg):
    print(f"mpsoak: {msg}", file=sys.stderr)
    sys.exit(1)


def slug(text):
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-") or "area"


def last_line(text):
    lines = [l for l in text.splitlines() if l.strip()]
    return lines[-1].strip() if lines else "(no output)"


def render(args):
    return shlex.join([str(a) for a in args])


def classify(text):
    """(first fail line, first warn line) of a log chunk."""
    fail = warn = None
    for line in text.splitlines():
        if any(ok in line for ok in WARN_TEXT_EXCEPTIONS):
            warn = warn or line.strip()
            continue
        if any(f in line for f in FAIL_TEXT):
            fail = fail or line.strip()
            continue
        if any(w in line for w in WARN_TEXT):
            warn = warn or line.strip()
    return fail, warn


# ---------------------------------------------------------------- the tour

class RoomError(Exception):
    pass


class Room:
    def __init__(self, world, world_id, slot, area, name, dark):
        self.world = world  # the text the caller / the tour used
        self.world_id = world_id
        self.slot = slot
        self.area = area  # MREA id, the hex token the game's `warp` and MP2_WORLD take
        self.name = name
        self.dark = dark == "dark"

    @property
    def label(self):
        return f"{self.world}:{self.name} {self.area}"

    @property
    def key(self):
        return (self.world_id, self.area)


def slot_table(world):
    """(world id, [(slot, MREA id, dark, name)]) from `tools/world_areas.py` (the disc's MLVL)."""
    r = subprocess.run([sys.executable, str(AREAS), world], cwd=ROOT, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if r.returncode:
        die(f"`tools/world_areas.py {world}` failed: {last_line(r.stdout)}")
    world_id, slots = None, []
    for line in r.stdout.splitlines():
        head = WORLD_RE.match(line.strip())
        if head:
            world_id = head.group(1).lower()
            continue
        slot = SLOT_RE.match(line.strip())
        if slot:
            slots.append((int(slot.group(1)), slot.group(2).lower(), slot.group(3), slot.group(4)))
    if world_id is None or not slots:
        die(f"`tools/world_areas.py {world}` printed no slots")
    return world_id, slots


def near_matches(text, slots, limit=8):
    hits = []
    words = [w for w in re.split(r"[^a-z0-9]+", text.lower()) if len(w) > 2]
    for slot, area, dark, name in slots:
        if any(w in name.lower() for w in words):
            hits.append((slot, area, dark, name))
    names = [name for _, _, _, name in slots]
    for guess in difflib.get_close_matches(text, names, n=limit, cutoff=0.4):
        hits.extend(s for s in slots if s[3] == guess)
    seen, out = set(), []
    for entry in hits:
        if entry not in seen:
            seen.add(entry)
            out.append(entry)
    return out[:limit]


def resolve(text, world_id, slots):
    """`world:area` -> Room; the area half by slot index, MREA id, name or name part."""
    text = text.strip()
    if ":" not in text:
        raise RoomError(f"'{text}' is not `world:area` (see: tools/world_areas.py --list)")
    world_text, _, area_text = text.partition(":")
    world_text, area_text = world_text.strip(), area_text.strip()
    if not world_text or not area_text:
        raise RoomError(f"'{text}' names no world or no area")
    want = area_text.lower()
    if want.isdigit():
        hit = [s for s in slots if s[0] == int(want)]
        if len(hit) == 1:
            return Room(world_text, world_id, hit[0][0], hit[0][1], hit[0][3], hit[0][2])
        raise RoomError(f"{world_text} has no area slot {want} (slots 0..{len(slots) - 1})")
    if re.fullmatch(r"(0x)?[0-9a-f]{8}", want):
        want_hex = want[2:] if want.startswith("0x") else want
        hit = [s for s in slots if s[1] == want_hex]
        if len(hit) == 1:
            return Room(world_text, world_id, hit[0][0], hit[0][1], hit[0][3], hit[0][2])
        raise RoomError(f"no area of {world_text} has MREA id {want_hex}")
    picks = (
        lambda n: n.lower() == want,
        lambda n: n.lower().startswith(want),
        lambda n: want in n.lower(),
    )
    for pick in picks:
        hit = [s for s in slots if pick(s[3])]
        if len(hit) == 1:
            return Room(world_text, world_id, hit[0][0], hit[0][1], hit[0][3], hit[0][2])
        if hit:
            if len({s[1] for s in hit}) == 1:
                return Room(world_text, world_id, hit[0][0], hit[0][1], hit[0][3], hit[0][2])
            raise RoomError(f"area '{area_text}' of {world_text} is ambiguous: "
                            + "; ".join(f"{s[3]} slot {s[0]} {s[1]}" for s in hit[:8]))
    near = near_matches(area_text, slots)
    raise RoomError(f"no area named '{area_text}' in {world_text}" + (
        ": closest are " + "; ".join(f"{s[3]} slot {s[0]} {s[1]}" for s in near) if near
        else f" (see: tools/world_areas.py {world_text})"))


def split_axis(values, choices, label):
    if values is None:
        return list(choices)
    out = []
    for v in str(values).split(","):
        v = v.strip()
        if not v:
            continue
        if v not in choices:
            die(f"--{label} '{v}' is not one of: {', '.join(choices)}")
        if v not in out:
            out.append(v)
    if not out:
        die(f"--{label} is empty")
    return out


def split_msaa(values):
    if values is None:
        return [1, 4]
    out = []
    for v in str(values).replace(",", " ").split():
        try:
            n = int(v)
        except ValueError:
            die(f"--msaa '{v}' is not a number")
        if n not in (1, 4):
            die(f"--msaa {n}: the console takes 1 (off) or 4")
        if n not in out:
            out.append(n)
    if not out:
        die("--msaa is empty")
    return out


def resolve_tour(texts):
    tables = {}  # world text -> (world id, its slots), one `world_areas.py` read per world
    rooms, problems = [], []
    for text in texts:
        world_text = text.partition(":")[0].strip()
        try:
            world_id, slots = tables.setdefault(world_text.lower(), slot_table(world_text))
            rooms.append(resolve(text, world_id, slots))
        except RoomError as e:
            problems.append(str(e))
    if problems:
        die("\n".join(problems))
    return rooms


# ---------------------------------------------------------------- processes

def wait_for_slot():
    """Block until fewer than MP2's 3 game slots are taken (LANE_BRIEF "Game slots")."""
    if not GAMES.is_file():
        return
    subprocess.run([str(GAMES), "--wait"], cwd=ROOT)


def game_pid(ancestor):
    """The game process under `ancestor` (the harness shell), or None.

    Only this soak's own game is found: the search walks up each candidate's parents, so another
    lane's `metroid_prime2_port` is never touched (never `pkill` by name).
    """
    for entry in Path("/proc").iterdir():
        if not entry.name.isdigit():
            continue
        try:
            if os.path.basename(os.readlink(entry / "exe")) != BINARY:
                continue
        except OSError:
            continue
        pid, hops = int(entry.name), 0
        while pid > 1 and hops < 8:
            pid, hops = ppid(pid), hops + 1
            if pid == ancestor:
                return int(entry.name)
    return None


def ppid(pid):
    try:
        with open(f"/proc/{pid}/status", encoding="utf-8", errors="replace") as f:
            for line in f:
                if line.startswith("PPid:"):
                    return int(line.split()[1])
    except OSError:
        pass
    return 0


# ---------------------------------------------------------------- the runs

class Run:
    """One game of the matrix: `tools/run_gameplay.sh` with a console and a display of its own."""

    def __init__(self, soak, mode, toggle):
        self.soak = soak
        self.mode, self.toggle = mode, toggle
        self.name = f"soak-{mode}-{toggle}"
        self.log = soak.evidence / f"{self.name}.log"
        self.harness = None
        self.proc = None
        self.landed = None  # the room the game is in
        self.exit_code = None

    # -- lifecycle

    def env(self):
        env = os.environ.copy()
        env.update(ALWAYS)
        env.update(TOGGLES[self.toggle])
        mods = MODES[self.mode]
        if mods == "no-mods":
            env["MP2_MODS"] = str(self.soak.no_mods)
        else:
            env.pop("MP2_MODS", None)
        env.update({
            "USERDIR": str(self.soak.user),
            "BUILD": str(self.soak.build),
            "LOG": str(self.log),
            "CACHEDIR": str(self.soak.cache),
            "RUNTAG": self.name,
            "DISPLAY_NUM": f":{self.soak.args.display}",
            "FRAMES": str(self.soak.args.frames),
            "SECS": str(self.soak.args.secs),
            "GODMODE": "1",  # a dead player looks like a stall (docs/GOTCHAS.md)
            "MP2_CONSOLE": str(self.soak.args.port),
        })
        if self.soak.disc:
            env["MP2_DISC"] = str(self.soak.disc)
        first = self.soak.rooms[0]
        env["WORLD"] = f"{first.world_id},{first.area}"  # the direct boot
        return env

    def start(self):
        wait_for_slot()
        self.soak.evidence.mkdir(parents=True, exist_ok=True)
        self.harness = open(self.soak.evidence / f"{self.name}-harness.txt", "w", encoding="utf-8")
        self.harness.write("$ " + render([RUN]) + "\n" + "".join(
            f"    {k}={v}\n" for k, v in sorted(self.env().items())
            if k.startswith(("MP_", "MP2_")) or k in ("LOG", "WORLD", "FRAMES", "GODMODE", "SECS")))
        self.harness.flush()
        self.proc = subprocess.Popen([str(RUN)], cwd=ROOT, env=self.env(), stdout=self.harness,
                                     stderr=subprocess.STDOUT, start_new_session=False)
        self.landed = None
        return self.wait_for_gameplay()

    def wait_for_gameplay(self):
        """Wait for the harness's `gameplay reached`; the console answers from then on."""
        deadline = time.monotonic() + BOOT_TIMEOUT
        while time.monotonic() < deadline:
            if not self.alive():
                return f"the run ended before gameplay ({self.exit_detail()})"
            text = self.log_text()
            fail, _ = classify(text)
            if fail:
                return f"the boot logged: {fail}"
            if "[harness] gameplay reached" in text:
                # The console answers only in game, and only once its listener is up.
                rc, out = self.console("frame")
                if rc == 0:
                    self.landed = self.soak.rooms[0]
                    return None
                if "[console] cannot listen" in text or "is not a port" in text:
                    return f"the console did not start: {last_line(out)}"
            time.sleep(0.25)
        return f"no gameplay after {BOOT_TIMEOUT}s"

    def stop(self):
        if self.proc is None:
            return
        pid = game_pid(self.proc.pid)
        if pid is not None:
            try:
                os.kill(pid, signal.SIGTERM)  # the port's own clean-exit path (port_coverage)
            except OSError:
                pass
        try:
            self.proc.wait(timeout=STOP_TIMEOUT)
        except subprocess.TimeoutExpired:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait(timeout=10)
        self.exit_code = self.proc.returncode
        if self.harness:
            self.harness.close()
            self.harness = None
        self.proc = None

    def alive(self):
        return self.proc is not None and self.proc.poll() is None

    def exit_detail(self):
        if self.proc is not None:
            return "still running"
        if self.exit_code == 0:
            return "exit 0: a clean quit (the run's MP2_FRAMES limit, or the game quit itself)"
        return f"exit {self.exit_code}: a signal (the bare number with uutils timeout, 128+it with GNU's)"

    # -- the log

    def log_text(self):
        try:
            return self.log.read_text(encoding="utf-8", errors="replace")
        except OSError:
            return ""

    def log_size(self):
        try:
            return self.log.stat().st_size
        except OSError:
            return 0

    def log_since(self, pos):
        try:
            if self.log.stat().st_size < pos:
                pos = 0
            with open(self.log, "rb") as f:
                f.seek(pos)
                return f.read().decode("utf-8", "replace")
        except OSError:
            return ""

    # -- driving it

    def console(self, command):
        """One console command through the tree's own client; (rc, its output)."""
        try:
            r = subprocess.run([sys.executable, str(CON), "-p", str(self.soak.args.port), command],
                               cwd=ROOT, text=True, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, timeout=CON_TIMEOUT)
        except subprocess.TimeoutExpired:
            return 1, f"no reply to `{command}` in {CON_TIMEOUT}s"
        return r.returncode, r.stdout

    def frame(self):
        rc, out = self.console("frame")
        if rc:
            return None
        m = re.search(r"^frame (\d+)$", out, re.M)
        return int(m.group(1)) if m else None

    def settle(self, updates):
        """Wait `updates` game updates (the counter `MP2_FRAMES` and `@f` use)."""
        want = self.frame()
        if want is None:
            return
        want += updates
        deadline = time.monotonic() + max(30.0, updates / 20.0)
        while time.monotonic() < deadline:
            now = self.frame()
            if now is not None and now >= want:
                return
            if not self.alive():
                return
            time.sleep(0.1)

    def go(self, room):
        """Put the game in `room`; None, or a failure detail."""
        if self.landed is not None and room.key == self.landed.key:
            return None  # the run booted (or was last warped) here
        if room.world_id == (self.landed.world_id if self.landed else room.world_id):
            cmd = f"warp {room.area}"
        else:
            cmd = f"world {room.world_id} {room.area}"
        pos = self.log_size()
        rc, out = self.console(cmd)
        if rc:
            return f"`{cmd}` refused: {last_line(out)}"
        deadline = time.monotonic() + WARP_TIMEOUT
        while time.monotonic() < deadline:
            chunk = self.log_since(pos)
            if ARRIVED_RE.search(chunk):
                self.landed = room
                return None
            fail, _ = classify(chunk)
            if fail:
                return fail
            if not self.alive():
                return f"the game died on the way to {room.name}"
            time.sleep(0.25)
        chunk = self.log_since(pos)
        if "[harness] no game update for" in chunk:
            return (f"the game stopped updating after `{cmd}`: a paused game performs no update "
                    f"(a save prompt or a pause menu - keep such areas out of the tour)")
        return f"no area change after `{cmd}` in {WARP_TIMEOUT}s"

    def msaa(self, samples):
        rc, out = self.console(f"msaa {samples}")
        return None if rc == 0 else f"`msaa {samples}` refused: {last_line(out)}"

    def shoot(self, out):
        r = subprocess.run([str(SHOT), str(self.soak.args.display), str(out)],
                           cwd=ROOT, text=True, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=120)
        return r.returncode, r.stdout

    def crash_report(self, msaa, room, detail):
        out = self.soak.evidence / f"{self.name}-msaa{msaa}-{slug(room.name)}-crash.txt"
        text = [f"$ {self.name} msaa {msaa} {room.label}", f"# {detail}",
                f"# harness {self.exit_detail()}", ""]
        text.append(self.log_text())
        out.write_text("\n".join(text), encoding="utf-8")
        return out


# ---------------------------------------------------------------- soak

class Soak:
    def __init__(self, args):
        self.args = args
        self.evidence = Path(args.evidence_dir).resolve()
        self.build = Path(args.build).resolve()
        self.user = Path(args.user_dir).resolve()
        self.cache = Path(args.cache_dir).resolve()
        self.disc = Path(args.disc).resolve() if args.disc else None
        self.no_mods = self.evidence / "no-mods"
        self.modes = split_axis(args.modes, list(MODES), "modes")
        self.toggles = split_axis(args.toggles, list(TOGGLES), "toggles")
        self.msaa = split_msaa(args.msaa)
        self.rooms = resolve_tour(args.rooms.split(",") if args.rooms else DEFAULT_ROOMS)
        self.rows, self.runs, self.summary = [], [], None

    # -- output

    def open_summary(self):
        self.evidence.mkdir(parents=True, exist_ok=True)
        self.summary = open(self.evidence / "summary.tsv", "w", encoding="utf-8")
        self.summary.write("mode\ttoggles\tmsaa\troom\tresult\tfirst matching line\n")
        self.summary.flush()

    def row(self, mode, toggle, msaa, room, result, detail=""):
        self.rows.append((mode, toggle, str(msaa), room, result, detail))
        detail = " ".join(detail.split())
        print(f"{mode:<9} {toggle:<9} {msaa:<4} {room:<56} {result:<4} {detail[:100]}", flush=True)
        if self.summary:
            self.summary.write(f"{mode}\t{toggle}\t{msaa}\t{room}\t{result}\t{detail}\n")
            self.summary.flush()

    def note(self, text):
        print(f"mpsoak: {text}", file=sys.stderr, flush=True)

    # -- plan

    def describe(self):
        """The commands of one cell, for --dry-run and the run's own notes."""
        first = self.rooms[0]
        lines = [f"# {len(self.rooms)} rooms, {len(self.modes)} modes x {len(self.toggles)} toggles "
                 f"x {len(self.msaa)} msaa"]
        for room in self.rooms:
            lines.append(f"#   {room.label} ({'dark' if room.dark else 'light'}, slot {room.slot})")
        for mode in self.modes:
            for toggle in self.toggles:
                name = f"soak-{mode}-{toggle}"
                envs = dict(ALWAYS)
                envs.update(TOGGLES[toggle])
                mods = MODES[mode]
                if mods == "no-mods":
                    envs["MP2_MODS"] = str(self.no_mods)
                shown = dict(envs, MP2_CONSOLE=str(self.args.port), GODMODE="1",
                             WORLD=f"{first.world_id},{first.area}", USERDIR=str(self.user),
                             BUILD=str(self.build), DISPLAY_NUM=f":{self.args.display}",
                             FRAMES=str(self.args.frames), SECS=str(self.args.secs))
                lines.append(f"\n# {mode} + {toggle}: one game, {len(self.msaa)} msaa x "
                             f"{len(self.rooms)} rooms")
                lines.append(render(["env"] + [f"{k}={v}" for k, v in sorted(shown.items())]
                                    + ["tools/run_gameplay.sh"]))
                landed = first
                for samples in self.msaa:
                    lines.append(render([CON, "-p", self.args.port, f"msaa {samples}",
                                         f"# {name}"]))
                    for i, room in enumerate(self.rooms, 1):
                        if room.key != landed.key:
                            cmd = (f"warp {room.area}" if room.world_id == landed.world_id
                                   else f"world {room.world_id} {room.area}")
                            lines.append(render([CON, "-p", self.args.port, cmd]))
                            landed = room
                        lines.append(render([CON, "-p", self.args.port, "heal"]))
                        lines.append(f"# {self.args.settle} game updates, then:")
                        lines.append(render([SHOT, self.args.display,
                                             self.out_path(mode, toggle, samples, i, room)]))
                    lines.append(f"# kill {BINARY} (SIGTERM) when the cell is done")
        lines.append(f"\n# rows -> {self.evidence / 'summary.tsv'}")
        return "\n".join(lines)

    def out_path(self, mode, toggle, msaa, index, room):
        return (self.evidence / f"{mode}-{toggle}-msaa{msaa}"
                / f"{index:02d}-{slug(room.name)}.png")

    # -- run

    def soak(self):
        self.evidence.mkdir(parents=True, exist_ok=True)
        if "none" in self.modes:
            self.no_mods.mkdir(parents=True, exist_ok=True)  # the empty mods folder of `--modes none`
        self.cache.mkdir(parents=True, exist_ok=True)
        self.open_summary()
        (self.evidence / "plan.txt").write_text(self.describe() + "\n", encoding="utf-8")
        self.note(f"{len(self.rooms)} rooms x {len(self.modes)} modes x {len(self.toggles)} "
                  f"toggles x {len(self.msaa)} msaa -> {self.evidence / 'summary.tsv'}")
        for mode in self.modes:
            for toggle in self.toggles:
                self.cell(mode, toggle)
        return self.finish()

    def start_run(self, run):
        detail = run.start()
        if detail:
            self.note(f"{run.name}: {detail}")
        return detail

    def cell(self, mode, toggle):
        run = Run(self, mode, toggle)
        self.runs.append(run)
        self.note(f"{run.name}: {len(self.msaa)} msaa x {len(self.rooms)} rooms")
        t0 = time.monotonic()
        dead = self.start_run(run)
        for samples in self.msaa:
            if dead is None:
                dead = run.msaa(samples)
            for i, room in enumerate(self.rooms, 1):
                label = room.label
                if dead is None and not run.alive():
                    dead = f"the run ended ({run.exit_detail()})"
                if dead is None and run.landed is None:
                    dead = self.start_run(run) or run.msaa(samples)
                if dead is None:
                    dead = run.go(room)
                if dead is None:
                    run.console("heal")  # a hazard should not end the tour
                    run.settle(self.args.settle)
                    time.sleep(self.args.shot_delay)  # a presented frame of this area on the X display
                    if not run.alive():
                        dead = f"the game died in {room.name}"
                if dead is not None:
                    self.row(mode, toggle, samples, label, "FAIL", dead)
                    if not run.alive():
                        self.note(f"{run.name} died in {label} (msaa {samples}): "
                                  f"{run.crash_report(samples, room, dead)}")
                        run.stop()
                        dead = self.start_run(run)
                    continue
                out = self.out_path(mode, toggle, samples, i, room)
                out.parent.mkdir(parents=True, exist_ok=True)
                pos = run.log_size()
                rc, text = run.shoot(out)
                fail, warn = classify(run.log_since(pos))
                if fail is None and not run.alive():
                    fail = f"the game process died ({run.exit_detail()})"
                if fail is None and rc:
                    fail = f"the screenshot failed: {last_line(text)}"
                if fail is None and not out.is_file():
                    fail = "no screenshot was written"
                if fail:
                    self.row(mode, toggle, samples, label, "FAIL", fail)
                    if not run.alive():
                        self.note(f"{run.name} died in {label} (msaa {samples}): "
                                  f"{run.crash_report(samples, room, fail)}")
                        run.stop()
                        dead = self.start_run(run)
                    continue
                self.row(mode, toggle, samples, label, "warn" if warn else "ok", warn or "")
        run.stop()
        self.note(f"{run.name}: {time.monotonic() - t0:.0f}s, exit {run.exit_code}")

    def finish(self):
        if self.summary:
            self.summary.close()
            self.summary = None
        counts = {}
        for *_, result, _detail in self.rows:
            counts[result] = counts.get(result, 0) + 1
        total = len(self.rows)
        print(f"\n{total} rows: " + ", ".join(f"{k}={counts[k]}" for k in sorted(counts))
              + f" -> {self.evidence / 'summary.tsv'}")
        return 1 if counts.get("FAIL") else 0

    def stop_all(self):
        for run in self.runs:
            if run.alive():
                run.stop()


def main():
    p = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--evidence-dir", required=True,
                   help="where summary.tsv, the logs and the screenshots go (created)")
    p.add_argument("--build", default=str(ROOT / "build-port-link"),
                   help="game build directory (default: build-port-link)")
    p.add_argument("--disc", help="disc image (default: MP2_DISC / tools/run_gameplay.sh's own)")
    p.add_argument("--user-dir", default=str(Path(os.environ.get("MP2_SOAK_USER")
                                                  or "/tmp/opencode/m8u")),
                   help="the directory that holds the memory card (default: $MP2_SOAK_USER, "
                        "else /tmp/opencode/m8u)")
    p.add_argument("--cache-dir", default="/tmp/opencode/mpsoak-cache",
                   help="shader/package cache shared by the cells (default: "
                        "/tmp/opencode/mpsoak-cache)")
    p.add_argument("--rooms", help="comma-separated `world:area` (default: the 12-area tour)")
    p.add_argument("--modes", help="comma-separated: mods, none (default: both)")
    p.add_argument("--toggles", help="comma-separated: validate, release (default: both)")
    p.add_argument("--msaa", help="comma-separated MSAA values: 1, 4 (default: both)")
    p.add_argument("--settle", type=int, default=120,
                   help="game updates to wait in an area before its screenshot (default: 120)")
    p.add_argument("--shot-delay", type=float, default=0.5,
                   help="seconds between the last settle frame and the screenshot (default: 0.5)")
    p.add_argument("--frames", type=int, default=30000,
                   help="MP2_FRAMES for one cell's run, its own ceiling (default: 30000, about 8 min "
                        "of game updates where a 12-area tour needs ~4500)")
    p.add_argument("--secs", type=int, default=900,
                   help="SECS backstop for one cell's run (default: 900)")
    p.add_argument("--port", type=int, default=4791,
                   help="MP2_CONSOLE port for the soak's game (default: 4791)")
    p.add_argument("--display", type=int, default=97,
                   help="X display number for the soak's game and its screenshots (default: 97)")
    p.add_argument("--keep", action="store_true", help="leave the last cell's game running at exit")
    p.add_argument("--dry-run", action="store_true",
                   help="print the commands (only the disc's area tables are read)")
    args = p.parse_args()

    if not Path(args.build).is_dir():
        die(f"--build {args.build} is not a directory")
    if not (Path(args.user_dir) / "USA").is_dir():
        die(f"--user-dir {args.user_dir} has no USA/: it must be the directory that holds the "
            f"memory card (docs/RUNNING.md)")
    card = Path(args.user_dir) / "USA" / "Card A"
    if card.is_dir() and not any(card.glob("*G2ME*.gci")):
        # Without a save every cell sits on the front end until its SECS backstop ("ended before
        # gameplay"), which reads like 48 crashes.
        die(f"--user-dir {args.user_dir}: {card} holds no G2ME save (.gci); the soak boots into a save")
    soak = Soak(args)
    if args.dry_run:
        print(soak.describe())
        return 0
    if not args.keep:
        atexit.register(soak.stop_all)
    try:
        return soak.soak()
    finally:
        if args.keep:
            print("mpsoak: leaving the runs going (--keep): "
                  + ", ".join(f"{r.name} on :{args.display} (console {args.port})"
                              for r in soak.runs), file=sys.stderr)
        else:
            soak.stop_all()
            atexit.unregister(soak.stop_all)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
