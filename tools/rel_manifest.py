#!/usr/bin/env python3
"""Write docs/modules.tsv and docs/module_spawns.tsv: one row per Echoes REL module.

    python3 tools/rel_manifest.py [--iso PATH]

Everything that is not a plain repo lookup is read from the user's own disc image (MP2_DISC, or the
default ISO path in tools/echoes_disc.py): MLVLs give each area's per-layer REL list, MREA script
layers give every object's script type and its SCAN reference, and SCAN -> STRG gives the in-game
text. With no disc the name/spawn columns are `?`. See docs/MODULES.md.
"""
import argparse
import collections
import pathlib
import re
import struct
import sys

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(HERE))
import echoes_disc as ed  # noqa: E402

RELS = ROOT / "config/G2ME01/rels"
OUT = ROOT / "docs/modules.tsv"
OUT_SPAWNS = ROOT / "docs/module_spawns.tsv"
FILES_CMAKE = ROOT / "files.cmake"
LOADERS = ROOT / "src/MetroidPrime/ScriptLoader.cpp"
RELPROD = ROOT / "orig/G2ME01/files/RelProd"
RELPROD_FALLBACK = ROOT / "../MetroidPrime2Port/orig/G2ME01/files/RelProd"

SYM_RE = re.compile(r"^(\S+) = \.text:0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)")
LOADER_RE = re.compile(r"MakeNamedScriptLoader\('(\w{4})',\s*&REL_ENTRY\(Load(\w+)\)\)")
# Loader names that do not start or end like their module. An alias is only a candidate: the pair is
# accepted when every layer that spawns the type also lists the module (map_types_to_modules).
ALIASES = {"Ings": "Ing", "CommandPirate": "CommandoPirate", "MetroidAlpha": "Metroid",
           "CannonBall": "ScriptCannonBall", "Coin": "ScriptCoin",
           "FrontEndDataNetwork": "ScriptFrontEndDataNetwork", "GuiMenu": "ScriptGui",
           "GuiPlayerJoinManager": "ScriptGui", "GuiScreen": "ScriptGui", "GuiSlider": "ScriptGui",
           "GuiWidget": "ScriptGui", "PlayerActor": "ScriptPlayerActor",
           "PlayerController": "ScriptPlayerProxy", "PlayerTurret": "ScriptPlayerTurret",
           "RiftPortal": "ScriptRiftPortal", "RsfAudio": "ScriptRsfAudio",
           "SafeZone": "ScriptSafeZone", "SafeZoneCrystal": "ScriptSafeZone",
           "StreamedMovie": "ScriptStreamedMovie", "GunTurretBase": "GunTurret",
           "GunTurretTop": "GunTurret", "DigitalGuardianHead": "DigitalGuardian",
           "SplitterCommandModule": "Splitter", "SplitterMainChassis": "Splitter",
           "FishCloudModifier": "FishCloud"}
CAP = 5
# Scans that say nothing about the creature ("Bioscan complete.", "Object scan complete.").
GENERIC_SCAN = re.compile(r"scan complete", re.I)


# ---------------------------------------------------------------------------- repo side
def read_symbols(mod):
    funcs = []
    for line in (RELS / mod / "symbols.txt").read_text().splitlines():
        m = SYM_RE.match(line)
        if m:
            funcs.append((int(m.group(2), 16), int(m.group(3), 16)))
    return funcs


def module_id(mod):
    for d in (RELPROD, RELPROD_FALLBACK):
        f = d / f"{mod}.rel"
        if f.exists():
            return struct.unpack(">I", f.read_bytes()[:4])[0]
    m = re.search(r"\bfn_(\d+)_", (RELS / mod / "symbols.txt").read_text())
    return int(m.group(1)) if m else -1


def module_sources():
    """module -> source paths, from the generated files.cmake (the build's own list)."""
    out = {}
    for m in re.finditer(r"set\(MP_MODULE_(\w+)_SOURCES\n(.*?)\n\)", FILES_CMAKE.read_text(), re.S):
        out[m.group(1)] = [ln.strip() for ln in m.group(2).splitlines() if ln.strip()]
    return out


def status_of(srcs):
    names = [pathlib.PurePath(s).name for s in srcs]
    if not srcs or all(n == "PortModuleStubs.cpp" for n in names):
        return "entry-stub"
    if any(not n.startswith("Port") for n in names):
        return "upstream"
    return "port-body"


# ---------------------------------------------------------------------------- disc side
def scan_disc(iso_path):
    iso = ed.Iso(iso_path)
    paks = ed.load_paks(iso)
    owner = {}
    for pk in paks:
        for rid, (t, *_rest) in pk.res.items():
            owner.setdefault((t, rid), pk)
    scan_ids = {rid for (t, rid) in owner if t == "SCAN"}

    def first_line(strg_id):
        try:
            strings = ed.strg_strings(owner[("STRG", strg_id)].get(strg_id))
        except Exception:
            return None
        for s in strings:
            line = re.sub(r"&[^;&]*;", "", s).strip().split("\n")[0].strip()
            if line and not GENERIC_SCAN.search(line):
                return line
        return None

    scan_name = {}

    def name_of_scan(sid):
        if sid not in scan_name:
            b = owner[("SCAN", sid)].get(sid)
            name = None
            for k in range(0, len(b) - 3):
                v = struct.unpack_from(">I", b, k)[0]
                if ("STRG", v) in owner:
                    name = first_line(v)
                    if name:
                        break
            scan_name[sid] = name
        return scan_name[sid]

    type_layers = collections.defaultdict(set)   # fourcc -> {(world, area, layer)}
    type_scans = collections.defaultdict(collections.Counter)
    mod_layers = collections.defaultdict(set)    # module -> {(world, area, layer)}
    for pk in paks:
        for nm, (t, mid) in pk.names.items():
            if t != "MLVL":
                continue
            try:
                w = ed.parse_mlvl(pk.get(mid))
            except Exception as e:  # multiplayer / demo worlds
                print(f"warning: {pk.path} {nm}: {e}", file=sys.stderr)
                continue
            wname = (first_line(w["strg"]) if ("STRG", w["strg"]) in owner else None) or nm
            for a in w["areas"]:
                key = (wname, a["iname"] or f"area{a['index']}")
                for layer in range(len(a["offs"]) // 2):
                    for mname in a["mods"][a["offs"][2 * layer]:a["offs"][2 * layer + 1]]:
                        mod_layers[mname[:-4] if mname.endswith(".rel") else mname].add(key + (layer,))
                try:
                    layers = ed.mrea_layers(pk.get(a["mrea"]))
                except Exception as e:
                    print(f"warning: {key}: {e}", file=sys.stderr)
                    continue
                for layer, sec in enumerate(layers):
                    for t4, _eid, body in ed.scly_objects(sec):
                        type_layers[t4].add(key + (layer,))
                        for k in range(0, max(0, len(body) - 3)):
                            v = struct.unpack_from(">I", body, k)[0]
                            if v in scan_ids:
                                type_scans[t4][v] += 1
    return {"type_layers": type_layers, "type_scans": type_scans, "mod_layers": mod_layers,
            "name_of_scan": name_of_scan}


def map_types_to_modules(loaders, modules, data):
    """fourcc -> module. A candidate comes from the loader's name; it is accepted only if every layer
    that spawns the type also lists the module (so the type is proven to live there). A type that is
    never spawned is mapped by name alone and flagged (spawned=False)."""
    result = {}
    for cc, lname in loaders:
        base = ALIASES.get(lname, lname)
        cands = [m for m in modules if m == base or m == lname
                 or (len(m) > 3 and (lname.startswith(m) or m.startswith(lname)))]
        cands.sort(key=lambda m: (m != base, -len(m)))
        layers = data["type_layers"].get(cc)
        if layers:
            cands = [m for m in cands if layers <= data["mod_layers"].get(m, set())]
        if cands:
            result[cc] = (cands[0], bool(layers))
    return result


def fmt_spawns(layers):
    byarea = collections.defaultdict(set)
    for w, a, l in layers:
        byarea[(w, a)].add(l)
    items = []
    for (w, a), ls in sorted(byarea.items()):
        tag = "" if ls == {0} else "[L" + ",".join(map(str, sorted(ls))) + "]"
        items.append(f"{w}/{a}{tag}")
    if not items:
        return "-", 0
    head = "; ".join(items[:CAP])
    return head + (f" (+{len(items) - CAP} more)" if len(items) > CAP else ""), len(items)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--iso")
    args = ap.parse_args()

    modules = sorted(p.name for p in RELS.iterdir() if p.is_dir())
    srcs = module_sources()
    loaders = LOADER_RE.findall(LOADERS.read_text())

    iso = args.iso or ed.find_iso()
    data = scan_disc(iso) if iso else None
    if data is None:
        print("warning: no disc image (set MP2_DISC); name and spawn columns are `?`", file=sys.stderr)
    t2m = map_types_to_modules(loaders, modules, data) if data else {}
    m2t = collections.defaultdict(list)
    for cc, (m, spawned) in t2m.items():
        m2t[m].append((cc, spawned))

    rows, spawn_rows = [], []
    for mod in modules:
        funcs = read_symbols(mod)
        sl = srcs.get(mod, [])
        name, spawn, nspawn, types = "?", "?", "?", "-"
        if data:
            ts = sorted(m2t.get(mod, []))
            types = ",".join(c + ("" if sp else "*") for c, sp in ts) or "-"
            counter = collections.Counter()
            for cc, _ in ts:
                for sid, n in data["type_scans"].get(cc, {}).items():
                    nm = data["name_of_scan"](sid)
                    if nm:
                        counter[nm] += n
            # Only "Category: Name" scans count; a bare sentence describes a part or plant, not the enemy.
            tagged = {n: c for n, c in counter.items() if re.match(r"\w+: ", n)}
            names = [n for n, _ in collections.Counter(tagged).most_common(3)]
            name = " | ".join(names) if names else "?"
            lay = data["mod_layers"].get(mod, set())
            spawn, nspawn = fmt_spawns(lay)
            spawn_rows += [(mod, w, a, l) for w, a, l in sorted(lay)]
        rows.append((mod, module_id(mod), sum(s for _, s in funcs), len(funcs), status_of(sl),
                     ", ".join(sl) or "-", types, name, nspawn, spawn))

    hdr = ["module", "id", "text_bytes", "functions", "status", "source", "script_types",
           "in_game_name", "areas", "spawns"]
    OUT.write_text("\t".join(hdr) + "\n" + "\n".join("\t".join(map(str, r)) for r in rows) + "\n")
    if spawn_rows:
        OUT_SPAWNS.write_text("module\tworld\tarea\tlayer\n" + "\n".join(
            "\t".join(map(str, r)) for r in spawn_rows) + "\n")
    named = sum(1 for r in rows if r[7] != "?")
    print(f"{len(rows)} modules -> {OUT.relative_to(ROOT)}; {named} with a proven in-game name")
    for st in ("upstream", "port-body", "entry-stub"):
        sel = [r for r in rows if r[4] == st]
        print(f"  {st:10s} {len(sel):3d} modules {sum(r[2] for r in sel):8d} bytes {sum(r[3] for r in sel):5d} fns")


if __name__ == "__main__":
    main()
