#!/usr/bin/env python3
"""Print the loader inputs that gate the two DarkTrooper bodies dct1 has open, per placement.

usage: tools/scan_darktrooper_loader.py

Read-only, like tools/scan_pillbug_paths.py (tools/echoes_disc.py's readers, the user's own disc
image). Columns:

  ancs / anim   SLdrPatternedAITypedef.animationInformation (property 0xe25fb08c, SLdrAnimationSet:
                ancs, character_index, initial_anim in that order). LoadDarkTrooper
                (src/MetroidPrime/Enemies/CDarkTrooper.cpp:1158-1163) passes kInvalidAssetId as the
                model and this ancs to LdrToModelData (ScriptLoader.cpp:446-459), which returns an
                empty optional only when BOTH resource types are zero, i.e. when the ancs id is not
                in any pak. `type` is the FourCC the pak reports for that id: 'ANCS' takes the
                CAnimRes path (a real model), any other present type takes the CStaticRes path with
                an invalid model id (a different, non-empty optional), and only NOWHERE is the
                null-model return reachable.
  scan          scannableInfoWhenAttacking (property 0x8b6f9b43): absent keeps the loader's
                kInvalidAssetId, so CDarkTrooper's `mAttackScanInfo` allocation
                (CDarkTrooper.cpp:161-164) is skipped.
  flotsam       property 0xc1d1e465 -> the ctor's SetDrawShadow/SetHighlightedInDarkVisor arm
                (CDarkTrooper.cpp:166-169).
  adf/fire      avoidDownFrames 0xebafeb27 / firesMissiles 0xc9fb6a85 (context only).

An arm is data-dead only when no placement at any layer of any area takes it; a property absent
from a placement keeps the loader default (false / kInvalidAssetId).
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

ANIM = 0xE25FB08C
FLOTSAM = 0xC1D1E465
ADF = 0xEBAFEB27
FIRE = 0xC9FB6A85
SCAN = 0x8B6F9B43
NAME = 0x494E414D


def items(buf):
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


def props(buf, want, out, depth=0):
    its = items(buf)
    if its is None or depth > 6:
        return
    for pid, payload in its:
        if pid == NAME:
            out.setdefault("name", payload.rstrip(b"\0").decode("latin1"))
        elif pid == ANIM and len(payload) == 12:
            out["ancs"], out["char"], out["anim"] = struct.unpack_from(">3I", payload, 0)
        elif pid in want and len(payload) == 1:
            out[want[pid]] = payload[0]
        elif pid in want and len(payload) == 4:
            out[want[pid]] = struct.unpack_from(">i", payload, 0)[0]
        else:
            props(payload, want, out, depth + 1)


WANT = {FLOTSAM: "flotsam", ADF: "adf", FIRE: "fire", SCAN: "scan"}


def objects(sec):
    count = struct.unpack_from(">I", sec, 10)[0]
    p = 14
    out = []
    for _ in range(count):
        t = sec[p:p + 4].decode("latin1")
        (ln,) = struct.unpack_from(">H", sec, p + 4)
        body = sec[p + 6:p + 6 + ln]
        eid, nc = struct.unpack_from(">IH", body, 0)
        out.append((t, eid, body[6 + 12 * nc:]))
        p += 6 + ln
    return out


def main():
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    paks = ed.load_paks(iso)
    rows = 0
    arms = {"ancs NOWHERE": 0, "ancs ANCS": 0, "ancs other": 0,
            "scan present": 0, "scan absent": 0,
            "flotsam 1": 0, "flotsam 0": 0}
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
                seen = False
                for li, sec in enumerate(layers):
                    for t4, eid, body in objects(sec):
                        if t4 != "DKTR":
                            continue
                        d = {}
                        if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                            props(body[6:], WANT, d)
                        ancs = d.get("ancs")
                        where = []
                        if ancs is not None:
                            where = [f"{q.path.rsplit('/', 1)[-1]}:{q.res[ancs][0]}"
                                     for q in paks if ancs in q.res]
                        kind = where[0].split(":")[1] if where else "NOWHERE"
                        if not seen:
                            print(f"== {nm}/{iname} (mlvl {mid:08x})")
                            seen = True
                        rows += 1
                        arms["ancs " + ("ANCS" if kind == "ANCS" else kind if where else "NOWHERE")] += 1
                        arms["scan present" if "scan" in d else "scan absent"] += 1
                        arms["flotsam 1" if d.get("flotsam") else "flotsam 0"] += 1
                        print(f"DKTR L{li} eid=0x{eid:08x} {d.get('name', '?')[:24]:24} "
                              f"ancs={'-' if ancs is None else '0x%08x' % ancs} "
                              f"type={kind} char={d.get('char', '-')} anim={d.get('anim', '-')} "
                              f"scan={'-' if 'scan' not in d else '0x%08x' % d['scan']} "
                              f"flotsam={d.get('flotsam', 0)} adf={d.get('adf', 0)} fire={d.get('fire', 0)}")
    print(f"== {rows} DKTR placements: " + ", ".join(f"{k}={v}" for k, v in arms.items()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
