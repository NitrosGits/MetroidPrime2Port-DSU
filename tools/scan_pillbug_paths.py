#!/usr/bin/env python3
"""Walk every PillBug (PILB) placement's waypoint chain the way CPillBug::HasInnerPathLoop
(fn_48_2FE4, src/MetroidPrime/Enemies/CPillBug.cpp:306) and BuildPath (fn_48_3134) do, and count
which arm of each loop every step takes.

usage: tools/scan_pillbug_paths.py

Read-only, like tools/scan_wallwalker_props.py: tools/echoes_disc.py's readers and the user's own
disc image. The start is the PILB's first Patrol/Follow connection (PTRL/FOLW, CheckConnectedObject
at 307); a waypoint's next is its first Arrived/Next connection (ARRV/NEXT) whose target exists
(an editor id with an object on any layer: the most permissive reading, so a layer that is off at
run time can only remove links); a chain id that is not a WAYP is the `!waypoint` arm (315/328).
Arms counted (line numbers of CPillBug.cpp):
  310 head       the while's two exits (current or anchor invalid)
  315 notwp      GetObjectById is not a CScriptWaypoint (inner walk)
  319 cur==start the walk returned to its start (returns false)
  322 cur==loop  current == loopStart (returns true)
  328 anchor!=wp the anchor lookup is not a CScriptWaypoint
LoadPillBug's null-model arm (fn_48_130, CPillBug.cpp:809) needs LdrToModelData to see neither a
model (kInvalidAssetId) nor an ANCS: each PILB's animationInformation ancs (property 0xe25fb08c)
is looked up in the paks (`ancs=<id> in <pak>:<type>`); a type 'ANCS' there makes the arm dead.
Every shipped chain should end in `cur==start` (a pure cycle through its start) for the other
arms to be data-dead.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

INVALID = None


ANIM = 0xE25FB08C  # SLdrPatternedAITypedef.animationInformation: ANCS, character, animation


def objects(sec):
    count = struct.unpack_from(">I", sec, 10)[0]
    p = 14
    out = []
    for _ in range(count):
        t = sec[p:p + 4].decode("latin1")
        (ln,) = struct.unpack_from(">H", sec, p + 4)
        body = sec[p + 6:p + 6 + ln]
        eid, nc = struct.unpack_from(">IH", body, 0)
        conns = [struct.unpack_from(">4s4sI", body, 6 + 12 * i) for i in range(nc)]
        conns = [(s.decode("latin1"), m.decode("latin1"), tg) for s, m, tg in conns]
        out.append((t, eid, conns, body))
        p += 6 + ln
    return out


def connected(objs, o, state, msg):
    """CEntity::CheckConnectedObject: the first matching connection whose target exists."""
    for s, m, tg in o[3]:
        if s == state and m == msg and (tg & 0x3FFFFFF) in objs:
            return tg & 0x3FFFFFF
    return INVALID


def walk(objs, start, arms):
    """HasInnerPathLoop (CPillBug.cpp:306-337); returns its result, counting arms."""
    def wp(i):
        o = objs.get(i)
        return o if o and o[1] == "WAYP" else None

    current = anchor = start
    while True:
        if current is INVALID or anchor is INVALID:
            arms["310 head exit"] += 1
            return False
        loop_start = anchor
        for _ in range(2):
            w = wp(current)
            if not w:
                arms["315 notwp"] += 1
                return False
            current = connected(objs, w, "ARRV", "NEXT")
            if current == start:
                arms["319 cur==start"] += 1
                return False
            if current == loop_start:
                arms["322 cur==loop"] += 1
                return True
        w = wp(anchor)
        if not w:
            arms["328 anchor!=wp"] += 1
            return False
        anchor = connected(objs, w, "ARRV", "NEXT")
        arms["(round ok)"] += 1


def main():
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    total = {}
    rows = 0
    paks = ed.load_paks(iso)
    for pk in paks:
        for nm, (t, mid) in pk.names.items():
            if t != "MLVL":
                continue
            try:
                w = ed.parse_mlvl(pk.get(mid))
            except Exception:
                continue
            for a in w["areas"]:
                iname = a["iname"] or f"area{a['index']}"
                try:
                    layers = ed.mrea_layers(pk.get(a["mrea"]))
                except Exception:
                    continue
                objs = {}
                for li, sec in enumerate(layers):
                    for t4, eid, conns, body in objects(sec):
                        objs[eid & 0x3FFFFFF] = (li, t4, eid, conns, body)
                bugs = [o for o in objs.values() if o[1] == "PILB"]
                if not bugs:
                    continue
                print(f"== {nm}/{iname} (mlvl {mid:08x})")
                for li, t4, eid, conns, body in bugs:
                    arms = {k: 0 for k in ("310 head exit", "315 notwp", "319 cur==start",
                                           "322 cur==loop", "328 anchor!=wp", "(round ok)")}
                    start = connected(objs, (li, t4, eid, conns, body), "PTRL", "FOLW")
                    # the chain length (BuildPath's count loop, 266-277) and whether it loops
                    n, cur, loops = 0, start, False
                    seen = set()
                    while cur is not INVALID and cur in objs and objs[cur][1] == "WAYP":
                        if cur in seen:
                            break
                        seen.add(cur)
                        n += 1
                        cur = connected(objs, objs[cur], "ARRV", "NEXT")
                        if cur == start:
                            loops = True
                            break
                    ancs = None
                    i = body.find(struct.pack(">I", ANIM))
                    if i >= 0:
                        ancs = struct.unpack_from(">I", body, i + 6)[0]
                    where = [f"{pk2.path.rsplit('/', 1)[-1]}:{pk2.res[ancs][0]}"
                             for pk2 in paks if ancs in pk2.res]
                    res = walk(objs, start, arms) if start is not INVALID else "no-start"
                    rows += 1
                    for k, v in arms.items():
                        total[k] = total.get(k, 0) + v
                    print(f"PILB L{li} eid=0x{eid:08x} start={'-' if start is INVALID else '0x%08x' % start} "
                          f"ancs={'-' if ancs is None else '0x%08x' % ancs} in {','.join(where) or 'NOWHERE'} "
                          f"chain={n} loops={loops} HasInnerPathLoop={res} "
                          + " ".join(f"{k}={v}" for k, v in arms.items() if v))
    print(f"== {rows} PILB placements; arm totals: " + ", ".join(f"{k}={v}" for k, v in total.items()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
