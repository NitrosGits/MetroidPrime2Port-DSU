#!/usr/bin/env python3
"""List every CommandoPirate placement and the attack pattern it would follow.

usage: tools/scan_commando_pattern.py [--all]

Read-only, over the user's own disc (tools/echoes_disc.py's readers: ISO, PAK, MLVL, MREA, SCLY).
For every area and script layer it prints the CMDO (CommandoPirate) placements with their script
connections and the AI waypoints (AIWP) of the layer with their flag word, so the module's attack
pattern can be found in the shipped data:

  * the module's FollowAttackPattern state (fn_9_961C) takes the commando's waypoint from the
    connection in the "ATTK" state (0x4154544b) with the "FOLW" message (0x464f4c57) - the
    fourCCs it builds at 0x0000969C..0x000096B4;
  * the ShouldBoost trigger (fn_9_B1E0, the rlwinm at 0x0000B22C) wants the waypoint's flag word
    (the word at the waypoint's +0x164, the property 0xc6705a00) to have its 0x20 bit, and the
    FollowAttackPattern state's own flag test (0x00009788, 0x00009790) wants its 0x02 or 0x04
    bit.

With --all every area and layer is printed; without it only the layers that have a commando with
an "ATTK"/"FOLW" connection, or an AI waypoint with one of those flag bits, are printed.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

# The CommandoPirate's script object and the AI waypoint's.
CMDO = "CMDO"
AIWP = "AIWP"
# The module's fourCCs: the "ATTK" state and the "FOLW" message.
ATTK = 0x4154544B
FOLW = 0x464F4C57
# The chain: the "ARRV" state and the "NEXT" message (the host's kSS_Arrived / kSM_Next).
ARRV = 0x41525256
NEXT = 0x4E455854
# The AI waypoint's properties (SLdrAIWaypoint).
AIWP_SPEED = 0x6392404E
AIWP_PAUSE = 0x80F7E605
AIWP_FLAGS = 0xC6705A00
AIWP_LOCATOR = 0xA790C6A9
AIWP_PREF = 0x166979D4
# The editor properties' transform position.
POSITION = 0x5846524D


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
                for li, sec in enumerate(layers):
                    if sec[:4] != b"SCLY":
                        continue
                    try:
                        objs = scly_objects(sec)
                    except Exception:
                        continue
                    cmdos = [o for o in objs if o[0] == CMDO]
                    wayps = [o for o in objs if o[0] == AIWP]
                    if not cmdos:
                        continue
                    interesting = False
                    for t4, eid, conns, props in cmdos:
                        for state, msg, obj in conns:
                            if state == ATTK and msg == FOLW:
                                interesting = True
                    for t4, eid, conns, props in wayps:
                        flags = u32(props, AIWP_FLAGS) or 0
                        if flags & 0x26:
                            interesting = True
                    if not interesting and not show_all:
                        continue
                    print(f"{nm}/{iname} L{li}: CMDO={len(cmdos)} AIWP={len(wayps)}")
                    for t4, eid, conns, props in cmdos:
                        pos = position(props)
                        print(f"    CMDO eid=0x{eid:04x} pos={pos}")
                        for state, msg, obj in conns:
                            tag = ""
                            if state == ATTK and msg == FOLW:
                                tag = "  <- the attack pattern (ATTK/FOLW)"
                            print(f"        on {fcc(state)} send {fcc(msg)} -> 0x{obj:04x}{tag}")
                    for t4, eid, conns, props in wayps:
                        flags = u32(props, AIWP_FLAGS) or 0
                        print(f"    AIWP eid=0x{eid:04x} flags=0x{flags:08x} "
                              f"boost={(flags >> 5) & 1} jump={(flags >> 1) & 1} "
                              f"jump2={(flags >> 2) & 1} "
                              f"speed={u32(props, AIWP_SPEED)} pos={position(props)}")
                        for state, msg, obj in conns:
                            if state == ARRV and msg == NEXT:
                                print(f"        on {fcc(state)} send {fcc(msg)} -> 0x{obj:04x}"
                                      "  (the chain's next waypoint)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
