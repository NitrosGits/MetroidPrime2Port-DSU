#!/usr/bin/env python3
"""Behaviour survey of the upstream-source REL modules (docs/modules.tsv status `upstream`).

Upstream's matching decomp compiles these modules, so their logic is retail's; what can go wrong
is the port layer under them. Per module this boots the first spawn area modules.tsv names
(activating the layer it lists with `layer <n>`), finds the module's objects with `objs`, and
drives them through their life over the console:

  natural    let them run (an inactive one gets `send <id> Activate`)
  place      the first one is put 8 units in front of the player and a screenshot is taken
  hurt       `damage <id> 1 [weapon]` until a weapon takes hp off (took damage?)
  fsm        (not with --no-fsm) every state `fsm <id> ?` lists is entered in turn, 40 updates each,
             on a second object when there is one (a crash here is a forced state, see the phase)
  death      `damage <id> 99999 <weapon>` on every object, then `obj` (gone or hp 0 = died);
             a survivor gets `kill <id>`

and records crash / caught / reach-stub / stand-in / stall / NaN position / missing asset from
the log and the replies. With the coverage build (build-port-cov, -DMP_PORT_COVERAGE=ON, which
also instruments tools/modules_upstream.txt) each module gets its line/branch coverage from
tools/cov/gcov_report.py --summary.

    tools/upstream_survey.py [--build build-port-cov] [--out /tmp/opencode/up1] [--jobs 2]
                             [--frames 3600] [--no-fsm] [--area <world>/<area>[Ln]] [module ...]
    tools/upstream_survey.py --report <out>      # the table from the results of earlier runs
    tools/upstream_survey.py --write-cov-list    # regenerate tools/modules_upstream.txt

Per module <out>/<Module>/ holds log.txt, con.txt (console transcript), result.json, shot.png
and the card/cache copies (gcov counters under c/gcov). Every game takes a machine-wide slot
(tools/games_running.sh --wait) and its own display and console port.
"""
import argparse
import concurrent.futures
import json
import math
import os
import pathlib
import re
import shutil
import socket
import subprocess
import sys
import threading
import time

ROOT = pathlib.Path(__file__).resolve().parent.parent
TSV = ROOT / "docs" / "modules.tsv"
WORLDS = {"Agon Wastes": "42b935e4", "Torvus Bog": "3dfd2249", "Sanctuary Fortress": "1baa96c2",
          "Temple Grounds": "3bfa3eff", "Great Temple": "863fcd72"}
# Modules with no object of their own: their code runs inside other modules' actors (the swarm
# base, the pirate rag doll, the blob collision trackers) or at boot (the tweaks).
NO_OBJECT = {"SwarmBasics", "PirateRagDoll", "GeomBlobV2", "Tweaks"}
# Class-name filters (`objs` matches a substring of the demangled class name) where the source
# file names are not the class names.
FILTERS = {"ScriptGui": ["CScriptGui"], "Metroid": ["CMetroid", "CBabyMetroid"],
           "GunTurret": ["CGunTurret"], "Sporb": ["CSporb"], "Grenchler": ["CGrenchler"],
           "ScriptSafeZone": ["CScriptSafeZone"]}
WEAPONS = ["power", "dark", "light", "annihilator", "missile", "powerbomb", "bomb", "boostball",
           "screwattack", "sonicboom"]
OBJ_RE = re.compile(r"^\s*u([0-9a-f]{4}) ([0-9a-f]{8})\s+(\S+)\s+a(-?\d+)(?: \(([^)]*)\))?(\s+inactive)?")
HP_RE = re.compile(r"hp (-?[\d.]+|nan|-?inf) -> (\(gone\)|-?[\d.]+|nan|-?inf)")
LOCK = threading.Lock()


def modules():
    rows = []
    lines = TSV.read_text().splitlines()
    head = lines[0].split("\t")
    for line in lines[1:]:
        r = dict(zip(head, line.split("\t")))
        if r.get("status") == "upstream":
            rows.append(r)
    return rows


def write_cov_list():
    out = ["# Upstream-source REL modules (docs/modules.tsv status 'upstream'): the coverage build",
           "# (MP_PORT_COVERAGE) instruments these too, for the behaviour survey (tools/upstream_survey.py).",
           "# Same format as tools/modules_port.txt; regenerate with tools/upstream_survey.py --write-cov-list."]
    for r in modules():
        for src in r["source"].split(", "):
            out.append(f"{r['module']};{src}")
    (ROOT / "tools" / "modules_upstream.txt").write_text("\n".join(out) + "\n")


def first_spawn(spawns):
    """(world, area, layers) of the first spawn of a modules.tsv `spawns` cell in a single-player
    world (the front end only when there is no other), or None."""
    frontend = None
    for item in spawns.split("; "):
        item = re.sub(r" \(\+\d+ more\)$", "", item).strip()
        if not item or item == "-":
            continue
        m = re.fullmatch(r"([^/]+)/([^\[]+)(?:\[L([\d,]+)\])?", item)
        if not m:
            continue
        world, area, layers = m.group(1), m.group(2), m.group(3)
        spawn = world, area, [int(x) for x in layers.split(",")] if layers else [0]
        if world == "FrontEnd":
            frontend = frontend or spawn
        elif world in WORLDS:  # not the multiplayer stations
            return spawn
    return frontend


def filters_of(r):
    if r["module"] in FILTERS:
        return FILTERS[r["module"]]
    return [pathlib.Path(s).stem for s in r["source"].split(", ")]


class Console:
    def __init__(self, port, transcript):
        self.port, self.tr, self.s, self.f = port, transcript, None, None

    def connect(self, game, limit=900):
        t0 = time.time()
        while time.time() - t0 < limit:
            if game.poll() is not None:
                return False
            try:
                self.s = socket.create_connection(("127.0.0.1", self.port), timeout=5)
                self.f = self.s.makefile("rw")
                return True
            except OSError:
                time.sleep(0.5)
        return False

    def cmd(self, c, timeout=60):
        self.s.settimeout(timeout)
        self.f.write(c + "\n")
        self.f.flush()
        lines, pending, end = [], c.count(";") + 1, ""
        while True:
            line = self.f.readline()
            if not line:
                end = "[con] eof"
                break
            line = line.rstrip()
            if line.startswith("=> "):
                pending -= 1
                end = line if not end.startswith("=> err") else end
                if pending == 0:
                    break
                continue
            lines.append(line)
        if c != "frame":
            self.tr.write(f"> {c}\n" + "".join(f"   {x}\n" for x in lines) + f"   {end}\n")
            self.tr.flush()
        if end == "[con] eof":
            raise EOFError(c)
        return lines, end

    def frame(self):
        lines, _ = self.cmd("frame")
        for x in lines:
            if x.startswith("frame "):
                return int(x.split()[1])
        return -1

    def wait_frame(self, n, game):
        while game.poll() is None:
            try:
                f = self.frame()
            except (OSError, socket.timeout):
                f = -1
            if f >= n:
                return f
            time.sleep(0.05)
        raise EOFError("game ended")


def parse_objs(lines):
    out = []
    for x in lines:
        m = OBJ_RE.match(x)
        if m:
            pos = m.group(5)
            out.append({"uid": "u" + m.group(1), "eid": m.group(2), "cls": m.group(3),
                        "area": int(m.group(4)), "pos": pos, "inactive": bool(m.group(6)),
                        "nan": bool(pos and re.search(r"nan|inf", pos, re.I))})
    return out


def obj_info(con, uid):
    lines, end = con.cmd(f"obj {uid}")
    info = {"exists": not end.startswith("=> err"), "hp": None, "fsm": None}
    for x in lines:
        x = x.strip()
        if x.startswith("hp "):
            try:
                info["hp"] = float(x.split()[1].split("/")[0])
            except ValueError:
                info["hp"] = float("nan")
        elif x.startswith("fsm "):
            info["fsm"] = x[4:]
    return info


def wait_world(con, game, world_hex):
    """Wait until the boot warp has put the game in the target world, then 120 updates more."""
    con.wait_frame(60, game)
    for _ in range(200):
        lines, _ = con.cmd("status")
        if not world_hex or any(x.startswith(f"world {world_hex}") for x in lines):
            break
        con.wait_frame(con.frame() + 30, game)
    else:
        raise EOFError(f"never reached world {world_hex}")
    con.wait_frame(con.frame() + 120, game)


def drive(con, game, res, mod, filters, layers, no_fsm, world_hex):
    res["phase"] = "boot"
    wait_world(con, game, world_hex)
    found = []
    tried = []
    for layer in layers:
        if layer != 0:
            con.cmd(f"layer {layer}")
            con.wait_frame(con.frame() + 240, game)
        tried.append(layer)
        found = []
        for flt in filters:
            lines, _ = con.cmd(f"objs {flt}")
            found += [o for o in parse_objs(lines) if o not in found]
        if found:
            break
    res["layers_tried"] = tried
    res["objects"] = len(found)
    res["classes"] = sorted({o["cls"] for o in found})
    res["spawned"] = bool(found)
    if not found:
        return
    targets = found[:4]
    res["phase"] = "natural"
    for o in targets:
        if o["inactive"]:
            con.cmd(f"send {o['uid']} Activate")
    for o in targets:
        o.update(obj_info(con, o["uid"]))
    res["initial"] = [{k: o[k] for k in ("uid", "cls", "inactive", "hp", "fsm", "pos")} for o in targets]
    con.wait_frame(con.frame() + 300, game)
    nan = []
    for flt in filters:
        lines, _ = con.cmd(f"objs {flt}")
        nan += [o["uid"] for o in parse_objs(lines) if o["nan"]]
    a = targets[0]
    res["phase"] = "place"
    con.cmd(f"place {a['uid']} player 8")
    con.wait_frame(con.frame() + 24, game)
    con.cmd("screenshot")
    con.wait_frame(con.frame() + 24, game)
    # hurt: the first weapon that takes hp off
    res["phase"] = "hurt"
    res["took_damage"] = None
    res["weapon"] = None
    for o in targets:
        if o.get("hp") is None:
            continue
        res["took_damage"] = False
        for w in WEAPONS:
            lines, end = con.cmd(f"damage {o['uid']} 1 {w}")
            m = next((HP_RE.search(x) for x in lines if HP_RE.search(x)), None)
            if m and (m.group(2) == "(gone)" or float(m.group(2)) < float(m.group(1))):
                res["took_damage"], res["weapon"] = True, w
                break
            con.wait_frame(con.frame() + 6, game)
        if res["took_damage"]:
            break
    con.wait_frame(con.frame() + 120, game)
    # fsm walk
    if not no_fsm:
        res["phase"] = "fsm"
        f = targets[1] if len(targets) > 1 and targets[1].get("fsm") else targets[0]
        lines, end = con.cmd(f"fsm {f['uid']} ?")
        text = " ".join(lines) + " " + end
        names = []
        m = re.search(r"valid names:\s*(.*)", text)
        if m:
            names = [n.strip(" ,;") for n in re.split(r"[\s,]+", m.group(1)) if n.strip(" ,;")]
            names = [n for n in names if re.fullmatch(r"[A-Za-z_][\w:]*", n)]
        res["fsm_states"] = len(names)
        entered = []
        for n in names:
            res["phase"] = f"fsm {n}"
            if not obj_info(con, f["uid"])["exists"]:
                break
            _, end = con.cmd(f"fsm {f['uid']} {n}")
            if not end.startswith("=> err"):
                entered.append(n)
            con.wait_frame(con.frame() + 40, game)
        res["fsm_entered"] = len(entered)
    # death
    res["phase"] = "death"
    weapon = res["weapon"] or "power"
    died = []
    for o in targets:
        if o.get("hp") is None:
            continue
        for _ in range(3):
            if not obj_info(con, o["uid"])["exists"]:
                break
            con.cmd(f"damage {o['uid']} 99999 {weapon}")
            con.wait_frame(con.frame() + 60, game)
    con.wait_frame(con.frame() + 300, game)
    killed = []
    for o in targets:
        if o.get("hp") is None:
            continue
        info = obj_info(con, o["uid"])
        dead = not info["exists"] or (info["hp"] is not None and info["hp"] <= 0)
        died.append(dead)
        if not dead:
            con.cmd(f"kill {o['uid']}")
            killed.append(o["uid"])
    con.wait_frame(con.frame() + 200, game)
    res["died"] = (all(died) if died else None)
    res["died_count"] = f"{sum(died)}/{len(died)}" if died else "-"
    res["killed_by_cmd"] = killed
    for flt in filters:
        lines, _ = con.cmd(f"objs {flt}")
        nan += [o["uid"] for o in parse_objs(lines) if o["nan"]]
    res["nan"] = sorted(set(nan))
    res["phase"] = "done"


def scan_log(log, res):
    text = log.read_text(errors="replace") if log.exists() else ""
    res["caught"] = len(re.findall(r"\[port\] caught", text))
    res["reach_stub"] = len(re.findall(r"reach-stub", text))
    res["standin"] = len(re.findall(r"is a port stand-in", text))
    res["stall"] = "no game update for" in text
    res["gameplay"] = bool(re.search(r"gameplay reached", text))
    miss = re.findall(r"^.*(?:missing|not found|no such resource|could not load|failed to load).*$",
                      text, re.I | re.M)
    res["missing"] = sorted(set(m.strip()[:160] for m in miss))[:12]
    res["stub_lines"] = sorted(set(re.findall(r"^.*(?:reach-stub|is a port stand-in).*$", text, re.M)))[:10]
    m = re.search(r"\[port\] caught .*?(?:\n.*){0,14}", text)
    res["backtrace"] = m.group(0)[:1500] if m else ""
    m = re.search(r"Screenshot saved to (\S+)", text)
    res["shot_src"] = m.group(1) if m else None


def run_module(r, args, idx, area_override=None):
    mod = r["module"]
    out = pathlib.Path(args.out) / mod
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    spawn = area_override or first_spawn(r["spawns"])
    res = {"module": mod, "area": None, "spawned": False, "phase": "start"}
    if spawn is None:
        res["phase"] = "no spawn area"
        (out / "result.json").write_text(json.dumps(res, indent=1))
        return res
    world, area, layers = spawn
    res["area"] = f"{world}/{area}" + (f"[L{','.join(map(str, layers))}]" if layers != [0] else "")
    shutil.copytree(args.save, out / "u")
    port = args.port_base + idx
    disp = f":{args.display_base + idx}"
    env = dict(os.environ)
    env.update({"GODMODE": "1", "DISPLAY_NUM": disp, "FRAMES": str(args.frames), "SECS": str(args.secs),
                "RUNTAG": f"up1_{mod}", "CACHEDIR": str(out / "c"), "LOG": str(out / "log.txt"),
                "USERDIR": str(out / "u"), "BUILD": str(pathlib.Path(args.build).resolve()),
                "MP2_CONSOLE": str(port), "MP2_FRAME_LIMIT": "0",
                "MP2_TICKS_PER_FRAME": str(args.ticks)})
    if world in WORLDS:
        env["WORLD"] = f"{WORLDS[world]},{area}"
    elif world == "FrontEnd":
        env["SKIP_FRONTEND"] = "0"
    no_obj = mod in NO_OBJECT
    subprocess.run([str(ROOT / "tools" / "games_running.sh"), "--wait"])
    with LOCK:
        print(f"[survey] {mod} start {res['area']} display {disp} port {port}", flush=True)
    t0 = time.time()
    game = subprocess.Popen([str(ROOT / "tools" / "run_gameplay.sh")], env=env, cwd=str(ROOT),
                            stdout=open(out / "run.txt", "w"), stderr=subprocess.STDOUT,
                            start_new_session=True)
    with open(out / "con.txt", "w") as tr:
        con = Console(port, tr)
        try:
            if world == "FrontEnd":
                # The console answers only in game: let the title and the front end run, then end it.
                time.sleep(args.frontend_secs)
                res["phase"] = "front end (no console)"
                res["driver_done"] = True
            elif con.connect(game):
                if no_obj:
                    wait_world(con, game, WORLDS.get(world))
                    con.wait_frame(con.frame() + 1200, game)
                    res["phase"] = "done (no object of its own)"
                else:
                    drive(con, game, res, mod, filters_of(r), layers, args.no_fsm, WORLDS.get(world))
                res["driver_done"] = True
            else:
                res["phase"] = "no console"
        except (EOFError, OSError, socket.timeout) as e:
            res["error"] = f"{type(e).__name__}: {e}"
        try:
            if con.s:
                con.s.close()
        except OSError:
            pass
    if res.get("driver_done"):
        # The run is over: end the game itself (SIGTERM flushes a coverage build's counters), so
        # it does not idle on to MP2_FRAMES.
        time.sleep(1)
        for pid in game_pids(game.pid):
            try:
                os.kill(pid, 15)
            except OSError:
                pass
    try:
        rc = game.wait(timeout=args.secs + 60)
    except subprocess.TimeoutExpired:
        os.killpg(game.pid, 15)
        rc = game.wait()
    res["rc"] = rc
    res["secs"] = round(time.time() - t0, 1)
    scan_log(out / "log.txt", res)
    shot = res.get("shot_src") and pathlib.Path(args.build).resolve() / res["shot_src"]
    if shot and shot.exists():
        shutil.move(str(shot), out / ("shot" + shot.suffix))
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", str(out / ("shot" + shot.suffix)),
                        "-vf", "scale=480:-1", str(out / "shot.png")])
        if shot.suffix != ".png":
            (out / ("shot" + shot.suffix)).unlink()
        res["shot"] = str(out / "shot.png")
    res["crash"] = (rc not in (0, 124, 143) and not (res.get("driver_done") and rc in (-15, 143))) \
        or res["caught"] > 0
    (out / "result.json").write_text(json.dumps(res, indent=1))
    with LOCK:
        print(f"[survey] {mod} rc={rc} phase={res['phase']} spawned={res['spawned']} "
              f"died={res.get('died')} caught={res['caught']} {res['secs']}s", flush=True)
    return res


def game_pids(pgid):
    """The game binaries in a run's process group."""
    out = []
    for d in pathlib.Path("/proc").iterdir():
        if not d.name.isdigit():
            continue
        try:
            if os.getpgid(int(d.name)) != pgid:
                continue
            if os.readlink(d / "exe").endswith("/metroid_prime2_port"):
                out.append(int(d.name))
        except OSError:
            continue
    return out


def coverage(build, out, mod):
    gdir = pathlib.Path(out) / mod / "c" / "gcov"
    if not gdir.exists():
        return None
    p = subprocess.run([sys.executable, str(ROOT / "tools" / "cov" / "gcov_report.py"), str(build),
                        str(gdir), "--module", mod, "--summary"], capture_output=True, text=True)
    for line in p.stdout.splitlines():
        f = line.split()
        if f and f[0] == mod and len(f) >= 6:
            return {"functions": f[1], "lines": f[2], "line_pct": float(f[3]),
                    "branches": f[4], "br_pct": float(f[5])}
    return None


def report(args):
    rows = []
    for r in modules():
        p = pathlib.Path(args.out) / r["module"] / "result.json"
        res = json.loads(p.read_text()) if p.exists() else {"module": r["module"], "phase": "not run"}
        res["cov"] = coverage(pathlib.Path(args.build).resolve(), args.out, r["module"])
        rows.append(res)
    print("| module | area | spawned | took dmg | died | line% | br% | findings |")
    print("|---|---|---|---|---|---|---|---|")
    for res in rows:
        cov = res.get("cov")
        find = []
        if res.get("crash"):
            find.append(f"CRASH rc={res.get('rc')} in {res.get('phase')}")
        if res.get("stall"):
            find.append("stall")
        if res.get("nan"):
            find.append("NaN pos " + ",".join(res["nan"]))
        if res.get("reach_stub"):
            find.append(f"reach-stub x{res['reach_stub']}")
        if res.get("standin"):
            find.append(f"stand-in x{res['standin']}")
        if res.get("error") and not res.get("crash"):
            find.append(res["error"][:60])
        if res.get("killed_by_cmd"):
            find.append("survived 99999 dmg")
        if res.get("fsm_states") is not None:
            find.append(f"fsm {res.get('fsm_entered')}/{res['fsm_states']}")
        if res.get("classes"):
            find.append(",".join(res["classes"]))
        print(f"| {res['module']} | {res.get('area') or '-'} | {yn(res.get('spawned'))} | "
              f"{yn(res.get('took_damage'))} | {res.get('died_count', '-')} | "
              f"{cov['line_pct'] if cov else '-'} | {cov['br_pct'] if cov else '-'} | "
              f"{'; '.join(find) or res.get('phase', '')} |")


def yn(v):
    return "-" if v is None else ("yes" if v else "no")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("modules", nargs="*")
    ap.add_argument("--build", default=str(ROOT / "build-port-cov"))
    ap.add_argument("--out", default="/tmp/opencode/up1")
    ap.add_argument("--save", default="/tmp/opencode/mi_u")
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--frames", type=int, default=30000)
    ap.add_argument("--secs", type=int, default=420)
    ap.add_argument("--ticks", type=int, default=4)
    ap.add_argument("--port-base", type=int, default=49930)
    ap.add_argument("--display-base", type=int, default=930)
    ap.add_argument("--no-fsm", action="store_true")
    ap.add_argument("--frontend-secs", type=int, default=90)
    ap.add_argument("--area", help="override the spawn: <world>/<area>[Ln,..]")
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--write-cov-list", action="store_true")
    args = ap.parse_args()
    if args.write_cov_list:
        write_cov_list()
        return
    if args.report:
        report(args)
        return
    rows = [r for r in modules() if not args.modules or r["module"] in args.modules]
    override = first_spawn(args.area.replace("/", "/", 1)) if args.area else None
    pathlib.Path(args.out).mkdir(parents=True, exist_ok=True)
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as ex:
        futs = [ex.submit(run_module, r, args, i % 16, override) for i, r in enumerate(rows)]
        for f in futs:
            f.result()


if __name__ == "__main__":
    main()
