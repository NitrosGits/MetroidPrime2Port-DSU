#!/usr/bin/env python3
"""Write docs/module-analysis/MinorIng_functions.tsv: one row per MinorIng REL function.

    python3 tools/drivers/minoring_class.py [--asm DIR] [--out FILE]

The row data comes from three places:

  * `tools/drivers/minoring_scan.py` walks the REL's asm and gives every `.fn` its offset, its
    size, the module functions that call it and the data objects that name it (the vtable, the
    FSM tables, the ctor/dtor lists).  That is the evidence for the `class` column.
  * the port's own source (`src/MetroidPrime/Enemies/PortCMinorIng.cpp`) gives the name of the
    marked bodies, so the `B` rows name the host method that carries the marker.
  * the table below is the hand-written classification of the functions the port does not mark.

The three classes (the rule is stated in docs/module-analysis/MinorIng.md, section "The
function classification"):

  H  the body is a copy of something the host already has: a compiler-emitted destructor, the
     crtexe REL runtime, upstream's generated loader code, the REL's own copy of a host type's
     member function, or an FSM2 node destructor the host's CState2 classes own.
  T  the body is a data/vtable thunk (it only returns a constant, a member address or a flag) or
     the function is dead in the REL: no module text calls it and no table names it.
  B  module behaviour: everything else.  These are the port's bodies.
"""
import argparse
import pathlib
import re
import subprocess
import sys

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
import minoring_scan as ms  # noqa: E402
import minoring_tables as mt  # noqa: E402

OUT = ROOT / "docs/module-analysis/MinorIng_functions.tsv"
PORT = ROOT / "src/MetroidPrime/Enemies/PortCMinorIng.cpp"
# fn_44_204C (the lure point on the nav mesh) is marked in the shared Ing header, which the Ing
# lane owns; the port's body for it lives there, not in PortCMinorIng.cpp.
PORT_SHARED = ROOT / "include/MetroidPrime/Enemies/PortIngShared.hpp"

# ---------------------------------------------------------------- the hand-written table
# name -> (owner, what it is, kind, class, the host's part)
TABLE = {
    # ---- the crtexe REL runtime -------------------------------------------------------------
    "RELExit": ("module", "RELExit: the module's exit", "rel-entry", "H", "the host's REL runtime"),
    "RELMain": ("module", "RELMain: the module's entry", "rel-entry", "H", "the host's REL runtime"),
    "_epilog": ("module", "_epilog: module exit; ModuleDestructors then RELExit", "runtime", "H",
                "crtexe module epilog"),
    "_prolog": ("module", "_prolog: module entry; ModuleConstructors then RELMain", "runtime", "H",
                "crtexe module prolog"),
    "ModuleConstructors": ("module", "ModuleConstructors: runs the module's static constructor list",
                           "runtime", "H", "crtexe module constructor table"),
    "ModuleDestructors": ("module", "ModuleDestructors: runs the module's static destructor list",
                          "runtime", "H", "crtexe module destructor table"),
    "__register_global_object": ("REL runtime",
                                 "__register_global_object: pushes (object, dtor, cookie) on the "
                                 "global destructor chain", "runtime", "H",
                                 "crtexe global object registry"),
    "__destroy_global_chain": ("REL runtime",
                               "__destroy_global_chain: walks the global destructor chain, calling "
                               "each registered dtor", "runtime", "H",
                               "crtexe global destructor chain"),
    "_unresolved": ("REL runtime",
                    "_unresolved: unresolved-symbol trap; OSReport + stack walk, never returns",
                    "runtime", "H", "crtexe unresolved-symbol handler"),
    # ---- compiler-emitted destructors (no module caller, no table) --------------------------
    "fn_44_954": ("(weak)", "delete of a CDamageVulnerability holder (the delete operator)", "weak-dtor",
                  "H", "the host's own delete"),
    "fn_44_E64": ("(weak)", "delete of a heap block with a pointer at +0x0c", "weak-dtor", "H",
                  "the host's own delete"),
    "fn_44_107C": ("(weak)", "delete of a CDamageVulnerability holder", "weak-dtor", "H",
                   "the host's own delete"),
    "fn_44_1298": ("(weak)", "delete of a CToken holder", "weak-dtor", "H", "the host's own delete"),
    "fn_44_1304": ("(weak)", "delete of a CToken holder", "weak-dtor", "H", "the host's own delete"),
    "fn_44_1358": ("(weak)", "delete of a CAnimData holder", "weak-dtor", "H", "the host's own delete"),
    "fn_44_13BC": ("(weak)", "delete of a CModelData holder", "weak-dtor", "H", "the host's own delete"),
    "fn_44_141C": ("(weak)", "delete of an SLdr block (SLdrDamageInfo, SLdrActorParameters, ...)",
                   "weak-dtor", "H", "the host's own delete"),
    "fn_44_1660": ("(weak)", "delete of an SLdrDamageInfo block", "weak-dtor", "H",
                   "the host's own delete"),
    "fn_44_17CC": ("(weak)", "delete of an SLdrDamageInfo block", "weak-dtor", "H",
                   "the host's own delete"),
    "fn_44_2984": ("(weak)", "delete of an rstl::string (live: the module's static init registers it "
                   "as a global destructor)", "weak-dtor", "H", "rstl::string's own destructor"),
    "fn_44_2C9C": ("(weak)", "delete of a heap block", "weak-dtor", "H", "the host's own delete"),
    "fn_44_51D8": ("(weak)", "delete of an array of 8-byte elements", "weak-dtor", "H",
                   "the host's own delete"),
    "fn_44_67C8": ("(weak)", "delete of a CToken holder", "weak-dtor", "H", "the host's own delete"),
    "fn_44_6834": ("(weak)", "delete of a CToken holder", "weak-dtor", "H", "the host's own delete"),
    "fn_44_6888": ("(weak)", "delete of a heap block", "weak-dtor", "H", "the host's own delete"),
    "fn_44_68C4": ("(weak)", "delete of a heap block", "weak-dtor", "H", "the host's own delete"),
    "fn_44_8AFC": ("(weak)", "delete of a heap block", "weak-dtor", "H", "the host's own delete"),
    "fn_44_9700": ("(weak)", "delete of a CToken holder", "weak-dtor", "H", "the host's own delete"),
    "fn_44_A644": ("(weak)", "delete of a heap block", "weak-dtor", "H", "the host's own delete"),
    "fn_44_A804": ("(weak)", "delete of a heap block", "weak-dtor", "H", "the host's own delete"),
    "fn_44_A840": ("(weak)", "delete of an array of 8-byte elements", "weak-dtor", "H",
                   "the host's own delete"),
    "fn_44_A8CC": ("(weak)", "delete of a heap block", "weak-dtor", "H", "the host's own delete"),
    "fn_44_A908": ("(weak)", "delete of a heap block", "weak-dtor", "H", "the host's own delete"),
    "fn_44_A944": ("(weak)", "delete of a heap block", "weak-dtor", "H", "the host's own delete"),
    "fn_44_A980": ("(weak)", "delete of a CToken holder", "weak-dtor", "H", "the host's own delete"),
    # ---- the REL's own copy of a host type's member function -------------------------------
    "fn_44_10D4": ("(host copy)", "LdrToPatternedInfo: the patterned info the loader builds",
                   "inline-dol", "H", "the host's generated LdrToPatternedInfo"),
    "fn_44_1114": ("(host copy)", "the patterned info's own copy", "inline-dol", "H",
                   "the host's generated LdrToPatternedInfo"),
    "fn_44_1134": ("CModelData", "the REL's copy of CModelData (the null-checked wrapper)",
                   "inline-dol", "H", "the host's CModelData owns its copy"),
    "fn_44_115C": ("CModelData", "the REL's copy of CModelData (the three model slots, the scale, the "
                   "id; the source's byte at +0x0c cleared)", "inline-dol", "H",
                   "the host's CModelData owns its copy"),
    "fn_44_28D8": ("CEntityInfo", "the REL's copy of CEntityInfo's destructor", "inline-dol", "H",
                   "the host's CEntityInfo owns its destruction"),
    "fn_44_2930": ("(host copy)", "the REL's copy of a member's destructor (CMemory::Free of the "
                   "pointer at +0x0c, then of the block)", "inline-dol", "H",
                   "the host's CEntityInfo owns its destruction"),
    "fn_44_2EF4": ("CDamageInfo", "the REL's copy of CDamageInfo's assignment", "inline-dol", "H",
                   "the host's CDamageInfo owns its copy"),
    "fn_44_2F24": ("CDamageInfo", "the REL's copy of CDamageInfo (the mode, the four floats, the three "
                   "ids; the flag byte at +0x1c)", "inline-dol", "H",
                   "the host's CDamageInfo owns its copy"),
    "fn_44_2FB0": ("CDamageInfo", "the REL's copy of CDamageInfo (the null-guarded body of fn_44_2F24)",
                   "inline-dol", "H", "the host's CDamageInfo owns its copy"),
    "fn_44_76AC": ("CModelData", "CModelDataNull: the empty model data the inglet gets",
                   "inline-dol", "H", "CModelData::CModelDataNull"),
    "fn_44_9940": ("CPatterned", "the REL's copy of CPatterned's destructor", "inline-dol", "H",
                   "the host's CPatterned owns its destruction"),
    "fn_44_9B24": ("(host copy)", "the REL's copy of a member's destructor (the vtable restore, the "
                   "CToken at +0x04)", "inline-dol", "H", "the host's members own their destruction"),
    "fn_44_9BB8": ("(host copy)", "the REL's copy of a member's destructor (the member at +0x00)",
                   "inline-dol", "H", "the host's members own their destruction"),
    "fn_44_9C10": ("(host copy)", "the REL's copy of a member's destructor (the member at +0x18)",
                   "inline-dol", "H", "the host's members own their destruction"),
    "fn_44_9C68": ("(host copy)", "the REL's copy of a member's destructor (the member at +0x30)",
                   "inline-dol", "H", "the host's members own their destruction"),
    "fn_44_9CC0": ("(host copy)", "the REL's copy of a member's destructor (the two-bit counter at "
                   "+0x00, its bit 5 the alive flag, the release of the pointer at +0x0c)",
                   "inline-dol", "H", "the host's members own their destruction"),
    "fn_44_A5D0": ("rstl::vector", "the REL's own vector resize (two zero ints)", "template", "H",
                   "the host's rstl::vector does it"),
    "fn_44_A680": ("CPatternedInfo", "the REL's own vector of float pairs for the two animation states",
                   "template", "H", "the host's CPatternedInfo holds them"),
    "fn_44_A9DC": ("CMinorIng", "the REL's own copy of the data block into the actor", "inline-dol", "H",
                   "the host's CMinorIngData is its own shape, so the copy is the member's own"),
    "fn_44_E08": ("CIngSpotData", "the REL's own copy of CIngSpotData (the null-guarded body)",
                   "inline-dol", "H", "the host's CIngSpotData owns its copy"),
    "fn_44_EB8": ("CIngSpotData", "the REL's own copy of the ing spot's vector member", "inline-dol",
                   "H", "the host's CIngSpotData owns its copy"),
    # ---- upstream's generated loader code ----------------------------------------------------
    "fn_44_14D8": ("SLdrMinorIng", "SLdrMinorIng::SLdrMinorIng (the default constructor)",
                   "inline-hdr", "H", "upstream's generated SLdrMinorIng"),
    "fn_44_16C4": ("SLdrMinorIng", "the sub-block's default constructor", "inline-hdr", "H",
                   "upstream's generated SLdrMinorIng"),
    "fn_44_1830": ("SLdrMinorIng", "the sub-block's default constructor", "inline-hdr", "H",
                   "upstream's generated SLdrMinorIng"),
    "fn_44_194C": ("SLdrMinorIng", "LoadTypedefMinorIngIngSpot (the ing spot reader)", "inline-hdr",
                   "H", "upstream's generated loader"),
    "fn_44_1CE0": ("SLdrMinorIng", "LoadTypedefUnknownStruct34 (the inglet's block reader)",
                   "inline-hdr", "H", "upstream's generated loader"),
    # ---- the FSM2 node destructors the host's CState2 classes own ---------------------------
    "fn_44_78BC": ("(FSM node)", "the FSM node's destructor (data_790)", "weak-dtor", "H",
                   "the host's CState2 node classes own theirs"),
    "fn_44_7918": ("(FSM node)", "the FSM node's destructor (data_79C)", "weak-dtor", "H",
                   "the host's CState2 node classes own theirs"),
    "fn_44_8404": ("(FSM node)", "the FSM node's destructor (data_784)", "weak-dtor", "H",
                   "the host's CState2 node classes own theirs"),
    "fn_44_87E0": ("(FSM node)", "the FSM node's destructor (data_778)", "weak-dtor", "H",
                   "the host's CState2 node classes own theirs"),
    # ---- the REL's own copy of CImpactVisorEffect ------------------------------------------
    "fn_44_64AC": ("CImpactVisorEffect", "the delete of the effect the projectile gets",
                   "inline-dol", "H", "the host's CImpactVisorEffect::None()"),
    "fn_44_6500": ("CImpactVisorEffect", "the effect's destructor (the flag at +0x14)", "inline-dol",
                   "H", "the host's CImpactVisorEffect::None()"),
    "fn_44_655C": ("CImpactVisorEffect", "the effect's member destructor", "inline-dol", "H",
                   "the host's CImpactVisorEffect::None()"),
    "fn_44_657C": ("CToken", "the effect's CToken destructor", "inline-dol", "H",
                   "the host's CImpactVisorEffect::None()"),
    "fn_44_65C4": ("CImpactVisorEffect", "the effect the projectile is launched with (three empty "
                   "parts, the invalid forced visor)", "inline-dol", "H",
                   "the host's CImpactVisorEffect::None()"),
    "fn_44_660C": ("CImpactVisorEffect", "the effect's copy (the optional members, the position, the "
                   "direction)", "inline-dol", "H", "the host's CImpactVisorEffect::None()"),
    "fn_44_66C0": ("CImpactVisorEffect", "the effect's assignment", "inline-dol", "H",
                   "the host's CImpactVisorEffect::None()"),
    "fn_44_6700": ("CImpactVisorEffect", "the effect's member destructor", "inline-dol", "H",
                   "the host's CImpactVisorEffect::None()"),
    "fn_44_6720": ("CImpactVisorEffect", "the effect's CToken destructor", "inline-dol", "H",
                   "the host's CImpactVisorEffect::None()"),
    "fn_44_6748": ("CToken", "the effect's CToken copy (the id, the asset id, the lock)", "inline-dol",
                   "H", "the host's CImpactVisorEffect::None()"),
    # ---- the data / vtable thunks ------------------------------------------------------------
    "fn_44_18": ("CMinorIng", "the hit-time slot: mDamageHitTime = skDamageHitTime", "virtual", "T",
                 "CPatterned's own slot"),
    "fn_44_28": ("CMinorIng", "the slot that returns the byte at +0x44f", "virtual", "T",
                 "CPatterned's own slot"),
    "fn_44_30": ("CMinorIng", "the slot that returns 0", "virtual", "T", "CPatterned's own slot"),
    "fn_44_38": ("CMinorIng", "the slot that returns kInvalidUniqueId", "virtual", "T",
                 "CPatterned's own slot"),
    "fn_44_48": ("CMinorIng", "the slot that tests the bit 0x10 of the byte at +0x34c", "virtual", "T",
                 "CPatterned's own slot"),
    "fn_44_54": ("CMinorIng", "the slot that returns the rodata float lbl_8041B758", "virtual", "T",
                 "CPatterned's own slot"),
    "fn_44_60": ("CMinorIng", "the slot that returns &this->0x754", "virtual", "T",
                 "CPatterned's own slot"),
    "fn_44_68": ("CMinorIng", "the slot that returns true", "virtual", "T", "CPatterned's own slot"),
    "fn_44_59E8": ("CMinorIng", "the slot that copies a CVector3f", "virtual", "T",
                   "CPatterned's own slot"),
    # ---- dead in the REL ---------------------------------------------------------------------
    "fn_44_24C0": ("CMinorIng", "dead: sets the bit 0x02 of the byte at 0xd3e (mShouldEvaporate)",
                   "unreferenced", "T", "no host body (the retail never calls it)"),
    "fn_44_57C4": ("CMinorIng", "dead: the team manager's member target (the team id at 0xccc, the "
                   "target id at 0xcce)", "unreferenced", "T",
                   "no host body (the retail never calls it)"),
    "fn_44_5A04": ("CMinorIng", "dead: faces the target (the direction from the target at 0xcce, "
                   "FaceDirection)", "unreferenced", "T", "no host body (the retail never calls it)"),
    "fn_44_6F98": ("CMinorIng", "dead: the slot that returns &this->0x254", "unreferenced", "T",
                   "no host body (the retail never calls it)"),
    # ---- the marked helpers (the port's bodies that are not CMinorIng methods) -------------------
    "fn_44_3614": ("(port helper)", "CopyRayCastResult: the copy of the ray's result", "method", "B",
                   "the host's CopyRayCastResult (marked)"),
    "fn_44_AB78": ("(port helper)", "FindBestAnimationId: the best animation for a parm", "method",
                   "B", "the host's FindBestAnimationId (marked)"),
    "fn_44_D00": ("CHealthInfo", "CopyHealthInfo: the copy of the health info", "method", "B",
                  "the host's CopyHealthInfo (marked)"),
    "fn_44_FE8": ("CBouncyGrenadeData", "BuildGrenadeData: the inglet's grenade data", "method", "B",
                  "the host's BuildGrenadeData (marked)"),
    "fn_44_C44": ("CBouncyGrenadeData", "CopyGrenadeData: the copy of the grenade data", "method", "B",
                  "the host's CopyGrenadeData (marked)"),
    "fn_44_110": ("module", "the loader: the property loop over the REL's own binary format",
                  "loader", "B", "the host's generated SLdrMinorIng reader (marked)"),
    "fn_44_9D58": ("CMinorIng", "the constructor (the actor and the members)", "ctor", "B",
                   "the host's CMinorIng constructor (marked)"),
    "fn_44_97E4": ("CMinorIng", "the destructor", "dtor", "B", "the host's ~CMinorIng (marked)"),
    "fn_44_9B4": ("CMinorIng", "BuildMinorIngData: the data build", "method", "B",
                  "the host's BuildMinorIngData (marked)"),
    "fn_44_D54": ("CMinorIng", "the copy of the ing spot into the data", "method", "B",
                  "the host's copy of the ing spot (marked)"),
    "fn_44_E0": ("module", "RegisterMinorIngModule: the module's registration", "rel-entry", "B",
                 "the host's RegisterMinorIngModule (marked)"),
    "fn_44_204C": ("CMinorIng", "the lure point on the nav mesh (the search's flags 3 for the height "
                   "query, 0x301 after it)", "method", "B",
                   "the host's PortIngLuredMeshPoint (marked in PortIngShared.hpp, the Ing lane's "
                   "file)"),
    # ---- the module's static constructor --------------------------------------------------------
    "fn_44_ABCC": ("module", "the module's static constructor (the .ctors entry): it copies the FSM "
                   "trigger and state tables into .data and registers the two global strings' "
                   "destructors", "static-init", "B",
                   "the port's own FSM tables (skTriggers, skStates, skCodeFuncs) carry the marker; "
                   "tools/drivers/minoring_tables.py checks them entry for entry"),
}

MARK_RE = re.compile(r"^\s*//\s*@\s*fn_44_([0-9A-Fa-f]+)\s*$")


def port_bodies():
    """fn id -> the host definition the marker sits above."""
    out = {}
    lines = (PORT.read_text(errors="replace") + "\n" +
             PORT_SHARED.read_text(errors="replace")).splitlines()
    for i, line in enumerate(lines):
        m = MARK_RE.match(line)
        if not m:
            continue
        for j in range(i + 1, min(i + 40, len(lines))):
            s = lines[j].strip()
            if s and not s.startswith("//"):
                name = s
                for sep in (" CMinorIng::", " static ", " void ", " bool ", " int ", " CVector3f ",
                            " CTransform4f ", " rstl::optional_object< CAABox > ",
                            " const CDamageVulnerability* ", " const CHealthInfo* "):
                    if sep in name:
                        name = name.split(sep)[-1]
                        break
                out["fn_44_" + m.group(1).upper()] = (name.split("(")[0].strip(), s)
                break
    return out


def kind_for(name, owner, callers, named_by, port, fsm):
    """The retail's own FSM tables say which entries are triggers, states and code functions."""
    if name in port:
        member = port[name][0]
        if member in fsm["triggers"]:
            return "fsm-trigger"
        if member in fsm["states"]:
            return "fsm-state"
        if member in fsm["code functions"]:
            return "method"
        if "lbl_44_data_620" in named_by:
            return "virtual"
        return "method"
    if name in TABLE:
        return TABLE[name][2]
    if "lbl_44_data_620" in named_by:
        return "virtual"
    return "method"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--asm", default=None)
    ap.add_argument("--out", default=str(OUT))
    args = ap.parse_args()

    asm_dir = pathlib.Path(args.asm) if args.asm else pathlib.Path(ms.DEFAULT_ASM) / "MinorIng" / "asm"
    funcs, objs = ms.parse_dir(asm_dir)
    port = port_bodies()
    rodata = mt.read_rodata(asm_dir)
    data = mt.read_data(asm_dir)
    fsm = {}
    for key, (label, stride) in mt.RETAIL.items():
        words = data.get(label, [])
        fsm[key] = {rodata.get(w, w) for w in words[::stride]}
    marked = set()
    for base in ("src", "include", "platform"):
        for f in (ROOT / base).rglob("*"):
            if f.suffix in (".c", ".cpp", ".h", ".hpp", ".inc"):
                marked |= set(re.findall(r"@\s*fn_44_([0-9A-Fa-f]+)",
                                         f.read_text(errors="replace")))
    marked = {"fn_44_" + m.upper() for m in marked}

    rows = []
    for name, f in sorted(funcs.items(), key=lambda kv: kv[1]["off"]):
        callers = sorted(n for n, g in funcs.items() if name in g["calls"])
        named_by = sorted(o for o, g in objs.items() if name in g["names"])
        if name in TABLE:
            owner, what, kind, cls, host = TABLE[name]
        else:
            host, sig = port.get(name, ("?", ""))
            owner = "CMinorIng"
            what = host + " (the host's body carries the marker)"
            kind = kind_for(name, owner, callers, named_by, port, fsm)
            cls = "B"
        rows.append((f"0x{f['off']:X}", f"0x{f['size']:X}", owner, what, kind, cls,
                     " ".join(c.replace("fn_44_", "") for c in callers),
                     host, "marked" if name in marked else "no marker"))

    out = pathlib.Path(args.out)
    out.write_text("offset\tsize\towner\tname\tkind\tclass\tcallers\tdol_counterpart\tport\n" +
                   "".join("\t".join(str(c) for c in r) + "\n" for r in rows))
    counts = {}
    for r in rows:
        counts[r[5]] = counts.get(r[5], 0) + 1
    print(f"{len(rows)} rows -> {out}")
    print("counts:", counts)
    nmarked = sum(1 for r in rows if r[8] == "marked")
    print("marked:", nmarked, " unmarked:", len(rows) - nmarked)
    return 0


if __name__ == "__main__":
    sys.exit(main())
