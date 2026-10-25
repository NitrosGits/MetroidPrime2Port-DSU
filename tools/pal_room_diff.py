#!/usr/bin/env python3
"""Diff the script layers (SCLY) of every MREA between the USA and PAL Echoes discs.

usage: python3 -I tools/pal_room_diff.py [--usa ISO] [--pal ISO] [--json] [--world Metroid1]
                                          [--no-raw]

Read-only, over the user's own discs (tools/echoes_disc.py's readers). Rooms are matched by
(pak, MREA asset id). Per room it prints the script objects added, removed and changed (object
FourCC, editor/instance id, name if the object has one), which property ids changed (nested
property structs are walked when their bytes parse exactly; the path is id/id/...) and the
connections (state, message, target) added or removed. A property whose byte size differs
between the discs is marked `size a->b`, which is what a USA-layout reader cannot cope with.
Rooms whose MREA bytes differ but whose script layers are identical are only counted
(--no-raw skips that hash pass). MLVL differences (area list, MREA ids) are listed per world.

Property layout (generated readers): u32 0xFFFFFFFF, u16 size, u16 count, then per property
u32 id, u16 size, data; a nested struct is u16 count + the same property records.
"""
import argparse
import hashlib
import json
import os
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

USA = ed.DEFAULT_ISO
PAL = "/run/media/odran/Leo/mp2-discs/eu/G2MP01.iso"
XFORM_EPS = 0.05  # position deltas below this (world units) are float noise
NAME_PROP = 0x255A4580  # CEditorProperties name (first property of every object block)


def parse_props(data, count, p=0):
    """Parse `count` property records at data[p:]; return ([(id, bytes)], end) or None."""
    out = []
    for _ in range(count):
        if p + 6 > len(data):
            return None
        pid, size = struct.unpack_from(">IH", data, p)
        if p + 6 + size > len(data):
            return None
        out.append((pid, data[p + 6:p + 6 + size]))
        p += 6 + size
    return out, p


def top_props(body):
    if len(body) < 8 or struct.unpack_from(">I", body, 0)[0] != 0xFFFFFFFF:
        return None
    _magic, _size, count = struct.unpack_from(">IHH", body, 0)
    r = parse_props(body, count, 8)
    return r[0] if r else None


def sub_props(data):
    """A nested struct (u16 count + records) that consumes `data` exactly, else None."""
    if len(data) < 2:
        return None
    (count,) = struct.unpack_from(">H", data, 0)
    if count == 0 or count > 256:
        return None
    r = parse_props(data, count, 2)
    if r and r[1] == len(data):
        return r[0]
    return None


def cstr(b):
    return b.split(b"\0", 1)[0].decode("latin1", "replace")


def obj_name(props):
    # editor properties: struct 0x255A4580 holds the name string (id 0x494E414D... varies); take
    # the first printable C string found in the first property
    if not props:
        return ""
    pid, data = props[0]
    if pid != NAME_PROP:
        return ""
    sub = sub_props(data)
    if sub:
        for _i, d in sub:
            s = cstr(d)
            if len(s) >= 2 and s.isprintable() and d.endswith(b"\0"):
                return s
    return ""


def diff_props(a, b, path, out):
    da, db = dict(a), dict(b)
    for pid in sorted(set(da) | set(db)):
        name = path + "%08x" % pid
        if pid not in da:
            out.append(f"+{name} ({len(db[pid])}B)")
        elif pid not in db:
            out.append(f"-{name} ({len(da[pid])}B)")
        elif da[pid] != db[pid]:
            x, y = da[pid], db[pid]
            sx, sy = sub_props(x), sub_props(y)
            if sx is not None and sy is not None:
                diff_props(sx, sy, name + "/", out)
            elif len(x) != len(y):
                out.append(f"~{name} size {len(x)}->{len(y)}")
            elif pid == 0x5846524D and len(x) == 36:
                fa_, fb_ = struct.unpack(">9f", x), struct.unpack(">9f", y)
                dpos = max(abs(u - v) for u, v in zip(fa_[:3], fb_[:3]))
                drs = max(abs(u - v) for u, v in zip(fa_[3:], fb_[3:]))
                if dpos > XFORM_EPS:
                    out.append(f"~xform moved {dpos:.2f}u {tuple(round(v, 1) for v in fa_[:3])}"
                               f"->{tuple(round(v, 1) for v in fb_[:3])}")
                elif drs > XFORM_EPS * 10:
                    out.append(f"~xform rot/scale d={drs:.2f}")
                # smaller deltas are re-export float noise: not reported
            elif len(x) == 4:
                out.append(f"~{name} {x.hex()}->{y.hex()} (f {struct.unpack('>f', x)[0]:.4g}"
                           f"->{struct.unpack('>f', y)[0]:.4g})")
            else:
                out.append(f"~{name} {len(x)}B")


def read_objects(sec):
    """[(fourcc, eid, [(state, msg, target)], props-bytes)] of one SCLY section."""
    count = struct.unpack_from(">I", sec, 10)[0]
    p = 14
    out = []
    for _ in range(count):
        t = sec[p:p + 4].decode("latin1")
        (ln,) = struct.unpack_from(">H", sec, p + 4)
        body = sec[p + 6:p + 6 + ln]
        eid, nc = struct.unpack_from(">IH", body, 0)
        conns = [struct.unpack_from(">III", body, 6 + 12 * i) for i in range(nc)]
        out.append((t, eid, conns, body[6 + 12 * nc:]))
        p += 6 + ln
    return out


def fc(v):
    s = struct.pack(">I", v)
    return s.decode("latin1") if s.isalpha() else "%08x" % v


def collect(path, raw):
    iso = ed.Iso(path)
    worlds = {}
    for pk in ed.load_paks(iso):
        base = os.path.basename(pk.path)[:-4]
        for nm, (t, mid) in pk.names.items():
            if t != "MLVL":
                continue
            w = ed.parse_mlvl(pk.get(mid))
            wd = {"pak": base, "mlvl": mid, "areas": {}, "order": []}
            wd["mlvl_hash"] = hashlib.md5(pk.get(mid)).hexdigest()
            for a in w["areas"]:
                try:
                    nmstr = ed.strg_strings(pk.get(a["strg"]))[0]
                except Exception:
                    nmstr = ""
                mrea = a["mrea"]
                ent = {"index": a["index"], "iname": a["iname"], "name": nmstr, "mrea": mrea}
                wd["order"].append(mrea)
                try:
                    secs = ed.mrea_layers(pk.get(mrea))
                    ent["layers"] = [read_objects(s) for s in secs]
                except Exception as e:  # noqa: BLE001
                    ent["layers"] = None
                    ent["error"] = repr(e)
                if raw:
                    t_, fl, sz, off = pk.res[mrea]
                    ent["rawhash"] = hashlib.md5(iso.read(pk.path, off, sz)).hexdigest()
                wd["areas"][mrea] = ent
            worlds[base] = wd
    return worlds


def flat(layers):
    d = {}
    for li, objs in enumerate(layers):
        for t, eid, conns, props in objs:
            d[eid] = (li, t, conns, props)
    return d


def diff_area(ea, eb):
    res = {"added": [], "removed": [], "changed": []}
    fa, fb = flat(ea["layers"]), flat(eb["layers"])
    for eid in sorted(fb.keys() - fa.keys()):
        li, t, conns, props = fb[eid]
        res["added"].append({"id": eid, "type": t, "layer": li, "name": obj_name(top_props(props)),
                             "conns": len(conns), "bytes": len(props)})
    for eid in sorted(fa.keys() - fb.keys()):
        li, t, conns, props = fa[eid]
        res["removed"].append({"id": eid, "type": t, "layer": li, "name": obj_name(top_props(props)),
                               "conns": len(conns), "bytes": len(props)})
    for eid in sorted(fa.keys() & fb.keys()):
        la, ta, ca, pa = fa[eid]
        lb, tb, cb, pb = fb[eid]
        ch = []
        if ta != tb:
            ch.append(f"type {ta}->{tb}")
        if la != lb:
            ch.append(f"layer {la}->{lb}")
        if pa != pb:
            xa, xb = top_props(pa), top_props(pb)
            if xa is None or xb is None:
                ch.append("props unparsed %dB->%dB" % (len(pa), len(pb)))
            else:
                diff_props(xa, xb, "", ch)
                if len(pa) != len(pb) and not any("size" in c for c in ch):
                    ch.append("struct %dB->%dB" % (len(pa), len(pb)))
        sa, sb = set(ca), set(cb)
        for c in sorted(sb - sa):
            ch.append("+conn %s %s ->%04x" % (fc(c[0]), fc(c[1]), c[2] & 0xFFFFFFFF))
        for c in sorted(sa - sb):
            ch.append("-conn %s %s ->%04x" % (fc(c[0]), fc(c[1]), c[2] & 0xFFFFFFFF))
        if ch:
            res["changed"].append({"id": eid, "type": tb, "name": obj_name(top_props(pb)),
                                   "changes": ch})
    return res


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--usa", default=os.environ.get("MP2_DISC") or USA)
    ap.add_argument("--pal", default=PAL)
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--world")
    ap.add_argument("--no-raw", action="store_true")
    args = ap.parse_args()
    for p in (args.usa, args.pal):
        if not os.path.exists(p):
            print("missing disc image:", p, file=sys.stderr)
            return 1
    raw = not args.no_raw
    wu, wp = collect(args.usa, raw), collect(args.pal, raw)
    report = {"worlds": {}}
    for base in sorted(wu.keys() | wp.keys()):
        if args.world and base != args.world:
            continue
        a, b = wu.get(base), wp.get(base)
        if not a or not b:
            report["worlds"][base] = {"only_in": "usa" if a else "pal"}
            continue
        wr = {"mlvl_changed": a["mlvl_hash"] != b["mlvl_hash"],
              "area_order_changed": a["order"] != b["order"], "rooms": [],
              "raw_only": [], "areas_added": [], "areas_removed": []}
        for m in b["areas"].keys() - a["areas"].keys():
            wr["areas_added"].append("%08x" % m)
        for m in a["areas"].keys() - b["areas"].keys():
            wr["areas_removed"].append("%08x" % m)
        for mrea in b["order"]:
            ea, eb = a["areas"].get(mrea), b["areas"][mrea]
            if not ea:
                continue
            label = {"mrea": "%08x" % mrea, "index": eb["index"], "iname": eb["iname"],
                     "name": eb["name"]}
            if ea["layers"] is None or eb["layers"] is None:
                label["error"] = ea.get("error") or eb.get("error")
                wr["rooms"].append(label)
                continue
            d = diff_area(ea, eb)
            lay = (len(ea["layers"]), len(eb["layers"]))
            if lay[0] != lay[1]:
                d["layers"] = list(lay)
            if d["added"] or d["removed"] or d["changed"] or lay[0] != lay[1]:
                d.update(label)
                wr["rooms"].append(d)
            elif raw and ea["rawhash"] != eb["rawhash"]:
                wr["raw_only"].append(label)
        report["worlds"][base] = wr
    if args.json:
        json.dump(report, sys.stdout, indent=1)
        print()
        return 0
    tot = [0, 0, 0, 0]
    for base, wr in report["worlds"].items():
        if "only_in" in wr:
            print(f"== {base}: only in {wr['only_in']}")
            continue
        print(f"== {base}: script-changed rooms {len(wr['rooms'])}, "
              f"mesh/other-only {len(wr['raw_only'])}, MLVL "
              f"{'CHANGED' if wr['mlvl_changed'] else 'same'}"
              f"{', area order CHANGED' if wr['area_order_changed'] else ''}"
              f"{', areas +' + ','.join(wr['areas_added']) if wr['areas_added'] else ''}"
              f"{', areas -' + ','.join(wr['areas_removed']) if wr['areas_removed'] else ''}")
        for r in wr["rooms"]:
            print(f"-- {base}/{r['name'] or r['iname']} [{r['iname']}] area#{r['index']} "
                  f"MREA {r['mrea']}"
                  + (f" layers {r['layers'][0]}->{r['layers'][1]}" if "layers" in r else ""))
            if "error" in r:
                print("   ERROR", r["error"])
                continue
            for k, sym in (("added", "+"), ("removed", "-")):
                for o in r[k]:
                    print(f"   {sym}{o['type']} {o['id']:08x} L{o['layer']} {o['name']!r} "
                          f"conns={o['conns']} {o['bytes']}B")
                    tot[0 if sym == "+" else 1] += 1
            for o in r["changed"]:
                print(f"   ~{o['type']} {o['id']:08x} {o['name']!r}: " + "; ".join(o["changes"]))
                tot[2] += 1
        for r in wr["raw_only"]:
            print(f"   (mesh/other only) {r['name'] or r['iname']} [{r['iname']}] MREA {r['mrea']}")
            tot[3] += 1
    print(f"TOTAL objects added {tot[0]} removed {tot[1]} changed {tot[2]}; "
          f"rooms with only non-script MREA changes {tot[3]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
