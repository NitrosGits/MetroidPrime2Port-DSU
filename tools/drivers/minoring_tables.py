#!/usr/bin/env python3
"""Check the MinorIng FSM tables in the port against the REL's own tables.

    python3 tools/drivers/minoring_tables.py [--asm DIR] [--port FILE]

The REL's module constructor (fn_44_ABCC, the .ctors entry) builds the three FSM tables it
runs on: data_1A8 (the triggers), data_47C (the states) and data_5A8 (the code functions).
The port installs its own tables in CMinorIng::SetupStateMachine, so the retail's table is the
evidence for the host's.  This reads both and prints the comparison: the names, their order and
the function each name stands for.

Exit code 0 when the three tables match entry for entry, 1 when they do not.
"""
import argparse
import pathlib
import re
import sys

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
import minoring_scan as ms  # noqa: E402

DEFAULT_PORT = ROOT / "src/MetroidPrime/Enemies/PortCMinorIng.cpp"

# The retail's tables: the data object, the rodata label of the first entry (the name pointer).
RETAIL = {
    "triggers": ("lbl_44_data_1A8", 4),
    "states": ("lbl_44_data_47C", 4),
    "code functions": ("lbl_44_data_5A8", 4),
}
PORT_ARRAY = {
    "triggers": "skTriggers",
    "states": "skStates",
    "code functions": "skCodeFuncs",
}


def read_rodata(asm_dir):
    """rodata label -> the string it holds."""
    out = {}
    blobs = {}
    for path in sorted(asm_dir.rglob("*rodata*.s")):
        cur = None
        for line in path.read_text(errors="replace").splitlines():
            m = re.match(r"^\.obj\s+(\S+?)(?:,\s*\w+)?\s*$", line)
            if m:
                cur = m.group(1)
                out.setdefault(cur, "")
                blobs.setdefault(cur, b"")
                continue
            if line.startswith(".endobj"):
                cur = None
                continue
            if cur is not None:
                m = re.match(r'^\s*\.string\s+"(.*)"\s*$', line)
                if m:
                    out[cur] = m.group(1)
                    continue
                # a blob of .4byte words is a run of NUL-terminated strings; the label names the
                # first one (the others are reached by address).
                m = re.match(r"^\s*\.4byte\s+0x([0-9A-Fa-f]{8})\s*$", line)
                if m:
                    blobs[cur] += bytes.fromhex(m.group(1))
    for label, blob in blobs.items():
        if not out.get(label) and blob:
            out[label] = blob.split(b"\0")[0].decode("latin-1")
    return out


def read_data(asm_dir):
    """data object -> the list of the .4byte words it holds."""
    out = {}
    for path in sorted(asm_dir.rglob("*data*.s")):
        cur = None
        for line in path.read_text(errors="replace").splitlines():
            m = re.match(r"^\.obj\s+(\S+?)(?:,\s*\w+)?\s*$", line)
            if m:
                cur = m.group(1)
                out.setdefault(cur, [])
                continue
            if line.startswith(".endobj"):
                cur = None
                continue
            if cur is not None:
                m = re.match(r"^\s*\.4byte\s+(\S+)", line)
                if m:
                    out[cur].append(m.group(1))
    return out


def read_port(port_path):
    """the port's table arrays: name -> the method the entry points at."""
    text = port_path.read_text(errors="replace")
    out = {}
    for key, array in PORT_ARRAY.items():
        m = re.search(r"\b" + array + r"\[\]\s*=\s*\{(.*?)\n\};", text, re.S)
        if not m:
            out[key] = []
            continue
        entries = []
        for em in re.finditer(r"\{\s*\"([^\"]+)\"\s*,\s*(.*?)\}\s*,", m.group(1), re.S):
            name = em.group(1)
            body = em.group(2)
            mm = re.search(r"&CMinorIng::(\w+)", body)
            entries.append((name, mm.group(1) if mm else body.strip()[:40]))
        out[key] = entries
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--asm", default=None)
    ap.add_argument("--port", default=str(DEFAULT_PORT))
    args = ap.parse_args()

    asm_dir = pathlib.Path(args.asm) if args.asm else pathlib.Path(ms.DEFAULT_ASM) / "MinorIng" / "asm"
    rodata = read_rodata(asm_dir)
    data = read_data(asm_dir)
    port = read_port(pathlib.Path(args.port))

    bad = 0
    for key, (label, stride) in RETAIL.items():
        words = data.get(label, [])
        names = [rodata.get(w, w) for w in words[::stride]]
        host = port.get(key, [])
        print(f"== {key}: the REL's {label} has {len(names)} entries, the host's "
              f"{PORT_ARRAY[key]} has {len(host)}")
        for i in range(max(len(names), len(host))):
            rn = names[i] if i < len(names) else "-"
            hn, hf = host[i] if i < len(host) else ("-", "-")
            ok = rn == hn
            if not ok:
                bad += 1
            print(f"  {i:2d}  {rn:45s} {hf:50s} {'ok' if ok else 'MISMATCH'}")
    print("MISMATCHES:", bad)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
