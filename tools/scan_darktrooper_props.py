#!/usr/bin/env python3
"""Print every DarkTrooper (DKTR) placement with the flags the module's flag-gated arms test.

usage: tools/scan_darktrooper_props.py

Read-only, like tools/scan_wallwalker_props.py (tools/echoes_disc.py's readers, the user's own
disc image). SLdrDarkTrooper properties (include/MetroidPrime/ScriptLoader/SLdrDarkTrooper.inc):
0xc1d1e465 flotsam (CDarkTrooper ragdoll holder +0x70 0x80), 0xebafeb27 avoidDownFrames
(mAvoidDownFrames, byte 0x928 0x10), 0xc9fb6a85 firesMissiles (mFiresMissiles, 0x928 0x40),
0x2dca199d the attack cooldown, 0xaf38bbd0-class state machine ids are not read. A property
absent from a placement keeps the loader's default (false). The layer index is the SCLY layer.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

DKTR_PROPS = {
    0xC1D1E465: ("flotsam", ">B"),
    0xEBAFEB27: ("avoidDownFrames", ">B"),
    0xC9FB6A85: ("firesMissiles", ">B"),
    0x2DCA199D: ("cooldown", ">f"),
}
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
        elif pid in props and len(payload) == struct.calcsize(props[pid][1]):
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
                for li, sec in enumerate(layers):
                    for t4, eid, conns, body in objects(sec):
                        if t4 != "DKTR":
                            continue
                        out = {}
                        if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                            walk(body[6:], DKTR_PROPS, out)
                        rows += 1
                        print(f"{nm}/{iname} (mlvl {mid:08x}) L{li} eid=0x{eid:08x} "
                              + " ".join(f"{k}={v}" for k, v in out.items()))
    print(f"== {rows} DKTR placements")
    return 0


if __name__ == "__main__":
    sys.exit(main())
