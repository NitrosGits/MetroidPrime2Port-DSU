#!/usr/bin/env python3
"""List every Ing placement with an attack pattern (the "ATTK"/"FOLW" connection) and every
safe zone, so the module's triggers that need them can be run.

usage: tools/scan_ing_pattern.py [--all]

Read-only, over the user's own disc (tools/echoes_disc.py's readers: ISO, PAK, MLVL, MREA, SCLY).
For every area and script layer it prints:

  * the INGS placements (the Ing, src/MetroidPrime/ScriptLoader.cpp:145) with their script
    connections - the module's FollowAttackPattern state (fn_29_82D4) takes the attack pattern
    from the connection in the "ATTK" state (0x4154544B) with the "FOLW" message (0x464f4c57);
  * the AI waypoints (AIWP) of the layer with their flag word (the property 0xc6705a00): the
    Ing's own form change (fn_29_82D4's update, asm 0x00008410) reads the waypoint's flags -
    0x40 or 0x80 make the Ing an ing spot (the form 3), 0x100 makes it corporeal (the form 2) -
    and the waypoint is also the destination the AttackPatternOver trigger (fn_29_A340) tests
    against kInvalidUniqueId;
  * the safe zones (SAFE) of the layer: the Ing's lure (fn_29_1DF4) and the safe zone count
    (the byte at 0xe2c, the ShouldFleeSafeZone trigger fn_29_AA94) need them.

With --all every area and layer with an Ing or a safe zone is printed; without it only the
layers that have an Ing with an "ATTK"/"FOLW" connection, a waypoint with one of the Ing's form
bits, or a safe zone.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

# The Ing's script object, the AI waypoint's and the safe zone's.
INGS = "INGS"
AIWP = "AIWP"
SAFE = "SAFE"
# The module's fourCCs: the "ATTK" state and the "FOLW" message.
ATTK = 0x4154544B
FOLW = 0x464F4C57
# The AI waypoint's flags (SLdrAIWaypoint).
AIWP_FLAGS = 0xC6705A00
AIWP_SPEED = 0x6392404E
# The editor properties' transform position.
POSITION = 0x5846524D
# The Ing's own form bits in the waypoint's flag word (fn_29_82D4, asm 0x00008410).
ING_FORM_BITS = 0x40 | 0x80 | 0x100


def properties(body):
    """The properties of one SCLY property block (u32 magic, u16 size, u16 count, ...)."""
    out = {}
    try:
        magic, _size, count = struct.unpack_from(">IHH", body, 0)
        if magic != 0xFFFFFFFF:
            return out
        p = 8
        for _ in range(count):
            pid, size = struct.unpack_from(">IH", body, p)
            data = body[p + 6:p + 6 + size]
            p += 6 + size
            out[pid] = data
    except struct.error:
        pass
    return out


def scly_objects(sec):
    """[(fourcc, editor_id, [(state, msg, obj_id)], properties)] of one script layer."""
    assert sec[:4] == b"SCLY", sec[:8]
    count = struct.unpack_from(">I", sec, 10)[0]
    p = 14
    out = []
    for _ in range(count):
        t = sec[p:p + 4].decode("latin1")
        (ln,) = struct.unpack_from(">H", sec, p + 4)
        body = sec[p + 6:p + 6 + ln]
        eid, nc = struct.unpack_from(">IH", body, 0)
        conns = []
        q = 6
        for _ in range(nc):
            state, msg, obj = struct.unpack_from(">III", body, q)
            conns.append((state, msg, obj))
            q += 12
        out.append((t, eid, conns, properties(body[q:])))
        p += 6 + ln
    return out


def fcc(v):
    """A fourCC as text (the printable one), or its hex."""
    b = struct.pack(">I", v)
    return b.decode("latin1") if all(32 <= c < 127 for c in b) else "0x%08x" % v


def u32(props, pid):
    d = props.get(pid)
    if d is None or len(d) < 4:
        return None
    return struct.unpack_from(">I", d, 0)[0]


def position(props):
    d = props.get(POSITION)
    if d is None or len(d) < 36:
        return None
    return struct.unpack_from(">3f", d, 0)


def main():
    show_all = "--all" in sys.argv
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    hits = 0
    for pk in ed.load_paks(iso):
        for nm, (t, mid) in sorted(pk.names.items()):
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
                # Every object of the area, by editor id, so the Ing's attack pattern target
                # (the "ATTK"/"FOLW" connection) can be shown with its own type and flags.
                by_id = {}
                area_objs = []
                for sec in layers:
                    if sec[:4] != b"SCLY":
                        continue
                    try:
                        objs = scly_objects(sec)
                    except Exception:
                        continue
                    area_objs.extend(objs)
                    for o in objs:
                        by_id[o[1]] = o
                for li, sec in enumerate(layers):
                    if sec[:4] != b"SCLY":
                        continue
                    try:
                        objs = scly_objects(sec)
                    except Exception:
                        continue
                    ings = [o for o in objs if o[0] == INGS]
                    wayps = [o for o in objs if o[0] == AIWP]
                    safes = [o for o in objs if o[0] == SAFE]
                    if not ings and not safes:
                        continue
                    interesting = bool(safes)
                    for t4, eid, conns, props in ings:
                        for state, msg, obj in conns:
                            if state == ATTK and msg == FOLW:
                                interesting = True
                    for t4, eid, conns, props in wayps:
                        flags = u32(props, AIWP_FLAGS) or 0
                        if flags & ING_FORM_BITS:
                            interesting = True
                    if not interesting and not show_all:
                        continue
                    hits += 1
                    print(f"{nm}/{iname} L{li}: INGS={len(ings)} AIWP={len(wayps)} "
                          f"SAFE={len(safes)}  (world 0x{mid:08x}, mrea 0x{a['mrea']:08x})")
                    for t4, eid, conns, props in ings:
                        print(f"    INGS eid=0x{eid:04x} pos={position(props)}")
                        for state, msg, obj in conns:
                            tag = ""
                            if state == ATTK and msg == FOLW:
                                tag = "  <- the attack pattern (ATTK/FOLW)"
                                target = by_id.get(obj)
                                if target is None:
                                    tag += " (the target is not in this area)"
                                else:
                                    tname, teid, _tc, tprops = target
                                    tflags = u32(tprops, AIWP_FLAGS)
                                    tag += (f" (the target is a {tname}"
                                            + (f", flags=0x{tflags:08x}" if tflags is not None
                                               else "")
                                            + (", an Ing form bit" if (tflags or 0) & ING_FORM_BITS
                                               else "") + ")")
                            print(f"        on {fcc(state)} send {fcc(msg)} -> 0x{obj:04x}{tag}")
                    for t4, eid, conns, props in wayps:
                        flags = u32(props, AIWP_FLAGS) or 0
                        mark = "  <- an Ing form bit" if flags & ING_FORM_BITS else ""
                        print(f"    AIWP eid=0x{eid:04x} flags=0x{flags:08x} "
                              f"spot={(flags >> 6) & 1} spot2={(flags >> 7) & 1} "
                              f"corp={(flags >> 8) & 1} speed={u32(props, AIWP_SPEED)} "
                              f"pos={position(props)}{mark}")
                    for t4, eid, conns, props in safes:
                        print(f"    SAFE eid=0x{eid:04x} pos={position(props)}")
    if hits == 0:
        print("no Ing pattern and no safe zone in the shipped data")
    return 0


if __name__ == "__main__":
    sys.exit(main())
