#!/usr/bin/env python3
"""Print every FlyingPirate placement's data type, and what else its area holds, from the disc.

usage: tools/scan_flyingpirate_props.py

Read-only, over the user's own disc (tools/echoes_disc.py's readers). SLdrFlyingPirate's type
word is the property 0x20daf45e (an int32 that becomes CFlyingPirateData::mType: bit 0 =
mIsFlyingPirate, bit 1 = mIsAquaPirate, bit 1 also picks the path-find type 4 over 3 in the
CFlyingPirate ctor). For every area with a FPRT placement it counts, over all layers of the
area (a cover point on any layer is in the object list once the layer is on): COVR cover
points, CINE cinematic cameras, WATR water and, per layer, the FPRT placements, plus each placement's
animationInformation ANCS id and the kind of pak resource it names. Printed last:
the paks/MLVLs/areas covered (and any unreadable), the three landing-cloud particle ids
(mParticleGen1/2/3, kInvalidAssetId when absent), the set of types seen and the number of areas with cover points / cinematic cameras / water.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

TYPE_ID = 0x20DAF45E
PATTERNED_ID = 0xB3774750
ANIM_ID = 0xE25FB08C
WATER = ("WATR", "SWTR")
CAMERA = ("CINE", "CAMH", "CAMP", "CAMF", "CAMI")


def type_of(body):
    """(type, found) of one FPRT property block (the default is 0 when the id is absent)."""
    try:
        magic, _size, count = struct.unpack_from(">IHH", body, 0)
        if magic != 0xFFFFFFFF:
            return None
        p = 8
        for _ in range(count):
            pid, size = struct.unpack_from(">IH", body, p)
            data = body[p + 6:p + 6 + size]
            p += 6 + size
            if pid == TYPE_ID and size >= 4:
                return struct.unpack_from(">i", data, 0)[0]
    except struct.error:
        return None
    return 0


def ancs_of(body):
    """The animationInformation.ancs resource id of one FPRT block (None when absent)."""
    try:
        _magic, _size, count = struct.unpack_from(">IHH", body, 0)
        p = 8
        for _ in range(count):
            pid, size = struct.unpack_from(">IH", body, p)
            data = body[p + 6:p + 6 + size]
            p += 6 + size
            if pid != PATTERNED_ID:
                continue
            n = struct.unpack_from(">H", data, 0)[0]
            q = 2
            for _ in range(n):
                qid, qsize = struct.unpack_from(">IH", data, q)
                if qid == ANIM_ID:
                    return struct.unpack_from(">I", data, q + 6)[0]
                q += 6 + qsize
    except struct.error:
        return None
    return None


GENS = (("landingCloudDirt", 0x317212AB), ("landingCloudDust", 0xBC113D7B),
        ("landingCloudSnow", 0x738BBBAA))


def gens_of(body):
    """(dirt, dust, snow) CAssetIds of one FPRT block; an absent id keeps the loader default
    (kInvalidAssetId = 0xffffffff, SLdrFlyingPirate.hpp:67-69). None when unparsable."""
    got = {pid: 0xFFFFFFFF for _n, pid in GENS}
    try:
        _magic, _size, count = struct.unpack_from(">IHH", body, 0)
        p = 8
        for _ in range(count):
            pid, size = struct.unpack_from(">IH", body, p)
            data = body[p + 6:p + 6 + size]
            p += 6 + size
            if pid in got and size >= 4:
                got[pid] = struct.unpack_from(">I", data, 0)[0]
    except struct.error:
        return None
    return tuple(got[pid] for _n, pid in GENS)


def main():
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    types = {}
    ancs_kinds = {}
    areas = cover_areas = cine_areas = water_areas = 0
    paks = mlvls = scanned = failed = orphan = 0
    gen_sets = {}
    for pk in ed.load_paks(iso):
        paks += 1
        in_mlvl = 0
        for nm, (t, mid) in pk.names.items():
            if t != "MLVL":
                continue
            mlvls += 1
            try:
                w = ed.parse_mlvl(pk.get(mid))
                in_mlvl += len(w["areas"])
            except Exception as e:
                failed += 1
                print(f"!! {nm}: MLVL unreadable ({e})")
                continue
            for a in w["areas"]:
                iname = a["iname"] or f"area{a['index']}"
                try:
                    layers = ed.mrea_layers(pk.get(a["mrea"]))
                except Exception as e:
                    failed += 1
                    print(f"!! {nm}/{iname}: MREA unreadable ({e})")
                    continue
                scanned += 1
                per = []
                counts = {"COVR": 0, "CINE": 0, "WATR": 0}
                for li, sec in enumerate(layers):
                    for t4, eid, body in ed.scly_objects(sec):
                        if t4 == "FPRT":
                            per.append((li, eid, type_of(body), ancs_of(body), gens_of(body)))
                        elif t4 == "COVR":
                            counts["COVR"] += 1
                        elif t4 in CAMERA:
                            counts["CINE"] += 1
                        elif t4 in WATER:
                            counts["WATR"] += 1
                if not per:
                    continue
                areas += 1
                cover_areas += counts["COVR"] > 0
                cine_areas += counts["CINE"] > 0
                water_areas += counts["WATR"] > 0
                print(f"{nm}/{iname}: COVR={counts['COVR']} cameras={counts['CINE']} "
                      f"water={counts['WATR']}")
                for li, eid, ty, ancs, gens in per:
                    types[ty] = types.get(ty, 0) + 1
                    gen_sets[gens] = gen_sets.get(gens, 0) + 1
                    kind = pk.res[ancs][0] if ancs in pk.res else None
                    ancs_kinds[kind] = ancs_kinds.get(kind, 0) + 1
                    print(f"    L{li} FPRT eid=0x{eid:08x} type={ty} ancs="
                          f"{'none' if ancs is None else f'0x{ancs:08x}'}:{kind} gens="
                          f"{'?' if gens is None else '/'.join(f'{g:08x}' for g in gens)}")
        orphan += sum(1 for v in pk.res.values() if v[0] == "MREA") - in_mlvl
    print(f"MREA resources no MLVL names (an FPRT there could never load): {orphan}")
    print(f"coverage: paks {paks}, MLVLs {mlvls}, areas scanned {scanned} (every layer of each), "
          f"unreadable {failed}")
    print("particle gens (dirt,dust,snow) -> placements: " + ", ".join(
        f"{'?' if g is None else '/'.join(f'{x:08x}' for x in g)}:{n}"
        for g, n in sorted(gen_sets.items(), key=lambda kv: str(kv[0]))))
    print(f"types seen: {sorted(types.items(), key=lambda kv: str(kv[0]))}")
    print(f"ancs resource kinds (in the area's own pak): {ancs_kinds}")
    print(f"areas with FPRT {areas}; with COVR {cover_areas}; with cameras {cine_areas}; "
          f"with water {water_areas}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
