#!/usr/bin/env python3
"""Print every WallWalker (WLWK, Mekenobite) placement: name, position, connections, the
SLdrWallWalker properties it carries, and the AI waypoints (AIWP) it can reach with their flags.

usage: tools/scan_wallwalker_props.py

Read-only, like tools/scan_splitter_props.py: tools/echoes_disc.py's readers and the user's own
disc image. A property absent from a placement keeps the loader's default (SLdrWallWalker.hpp).
CWallWalker::Patrol enters its Generate arm on a waypoint whose flags (AIWP 0xc6705a00, the
CScriptAIWaypoint ctor's `flags`) have 0x200 set.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

WLWK_PROPS = {
    0x733BD27C: ("waypointApproachDistance", ">f"),
    0x8E4F7B29: ("floorTurnSpeed", ">f"),
    0xD5C25506: ("stickyReach", ">f"),
    0xA72530E8: ("visibleDistance", ">f"),
    0x9A6BB47F: ("grenadeMass", ">f"),
    0xED086CE0: ("unknown_0xed086ce0", ">f"),
    0x454F16B1: ("grenadeNumBounces", ">i"),
    0x7F1613B7: ("grenadeSoundBounce", ">i"),
    0x7050D866: ("grenadeSoundExplode", ">i"),
    0xD4903C98: ("projectileInterval", ">f"),
    0x723542BB: ("projectileStopHomingRange", ">f"),
    0xEF485DB9: ("projectile", ">I"),
    0x68DC4D11: ("projectileExplosionSpash", ">I"),
    0x8DB77EE4: ("detectionRange", ">f"),
    0x55744160: ("stateMachine", ">I"),
}
AIWP_FLAGS = 0xC6705A00
NAME = 0x494E414D
XFRM = 0x5846524D
ACTIVE = 0x41435456


def items(buf):
    """Read buf as a property list; [(id, payload)] when it parses exactly, else None."""
    if len(buf) < 2:
        return None
    count = struct.unpack_from(">H", buf, 0)[0]
    p = 2
    out = []
    for _ in range(count):
        if p + 6 > len(buf):
            return None
        pid, size = struct.unpack_from(">IH", buf, p)
        out.append((pid, buf[p + 6:p + 6 + size]))
        p += 6 + size
    return out if p == len(buf) else None


def walk(buf, props, out, depth=0):
    its = items(buf)
    if its is None or depth > 6:
        return
    for pid, payload in its:
        if pid == NAME:
            out["name"] = payload.rstrip(b"\0").decode("latin1")
        elif pid == XFRM and len(payload) == 36:
            out["pos"] = "%.1f %.1f %.1f" % struct.unpack_from(">3f", payload, 0)
        elif pid == ACTIVE and len(payload) == 1:
            out["active"] = payload[0]
        elif pid in props and len(payload) == 4:
            name, fmt = props[pid]
            v = struct.unpack(fmt, payload)[0]
            out[name] = ("0x%x" % v) if fmt == ">I" else v
        else:
            walk(payload, props, out, depth + 1)


def objects(sec):
    """[(fourcc, eid, connections, props)] of one SCLY section (connections as (state, msg, target))."""
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
        out.append((t, eid, conns, body[6 + 12 * nc:]))
        p += 6 + ln
    return out


def main():
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    rows = 0
    for pk in ed.load_paks(iso):
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
                walkers = [o for o in objs.values() if o[1] == "WLWK"]
                if not walkers:
                    continue
                print(f"== {nm}/{iname} (mlvl {mid:08x})")
                for li, t4, eid, conns, body in walkers:
                    out = {}
                    if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                        walk(body[6:], WLWK_PROPS, out)
                    rows += 1
                    print(f"WLWK L{li} eid=0x{eid:08x} " + " ".join(f"{k}={v}" for k, v in out.items()))
                    for s, m, tg in conns:
                        tgt = objs.get(tg & 0x3FFFFFF)
                        print(f"    {s} {m} -> 0x{tg:08x} {tgt[1] if tgt else '?'}")
                    # AIWPs reachable through Next/Follow connections from this walker.
                    seen, todo = set(), [tg for _, _, tg in conns]
                    while todo:
                        tg = todo.pop() & 0x3FFFFFF
                        if tg in seen or tg not in objs or objs[tg][1] != "AIWP":
                            continue
                        seen.add(tg)
                        wl, _, weid, wconns, wbody = objs[tg]
                        wout = {}
                        if len(wbody) >= 8 and struct.unpack_from(">I", wbody, 0)[0] == 0xFFFFFFFF:
                            walk(wbody[6:], {AIWP_FLAGS: ("flags", ">I")}, wout)
                        print(f"    AIWP L{wl} 0x{weid:08x} flags={wout.get('flags', 'default')} "
                              f"pos={wout.get('pos')}")
                        todo += [t for _, _, t in wconns]
    print(f"== {rows} WLWK placements")
    return 0


if __name__ == "__main__":
    sys.exit(main())
