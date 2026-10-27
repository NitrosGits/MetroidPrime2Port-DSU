#!/usr/bin/env python3
"""Compile the hand-written randomizer rules (tools/rando_rules/*.rules) into
platform/include/port_rando_logic.inc.

Format (docs/RANDOMIZER.md, "Hand rules (format)"): one file per world, named after the MLVL internal
name (`01_Temple_Hub_20041018.rules`), blocks of

    area 0a_temple_hall                       # MLVL internal area name
      dock0 - dock1: any                      # '-' both ways, '>' one way
      dock1 > loc0x00020034: SpaceJumpBoots | (MorphBall & MorphBallBombs)
      @ledge - dock2: GrappleBeam             # @name: an in-room node
      tel0x0C06013A - dock0: any              # a teleporter (editor id)
      covers 0x00020041 0x00020042            # obstacles these rules account for
      ignore 0x00020050: decorative webbing   # obstacles that block nothing, with a reason

Requirements: `any`, `never`, item names as spelled in platform/port_randomizer.cpp's table,
`Name>=N`, `&`, `|`, parentheses. A bare `Name` means at least one. They are compiled to DNF (an OR
of AND-terms of (item, count)), which is what the .inc stores.

Checked against the disc data of tools/rando_world.py (collect()): the area exists in the world the
file names, every dockN / locX / telX exists in that area, every obstacle of the area is in exactly
one covers/ignore line. Nothing else is checked: an area with no block simply connects nothing.

    tools/rando_rules.py                       check, print per-world coverage
    tools/rando_rules.py --write               also write port_rando_logic.inc
    tools/rando_rules.py --rules-dir DIR       read DIR instead of tools/rando_rules
    tools/rando_rules.py --out FILE            write FILE instead of the .inc (with --write)
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import rando_world as rw  # noqa: E402

RULES_DIR = ROOT / "tools" / "rando_rules"
OUT = ROOT / "platform" / "include" / "port_rando_logic.inc"
ITEM_SRC = ROOT / "platform" / "port_randomizer.cpp"
MAX_TERMS = 256
KINDS = {"dock": "Dock", "loc": "Location", "tel": "Teleporter", "@": "Named"}


class RuleError(Exception):
    pass


def item_table():
    """name -> EItemType value, from PortRandomizer's kItems table in the source."""
    src = ITEM_SRC.read_text()
    body = src[src.index("kItems[] = {"):]
    body = body[:body.index("};")]
    items = {m.group(1): int(m.group(2)) for m in re.finditer(r'\{"(\w+)",\s*(\d+)\}', body)}
    if not items:
        sys.exit(f"[rando_rules] no item table found in {ITEM_SRC}")
    return items


# ---- requirements: DNF as a tuple of terms, a term a tuple of (item, count) sorted by item --------

def norm_term(lits):
    best = {}
    for item, count in lits:
        best[item] = max(best.get(item, 0), count)
    return tuple(sorted(best.items()))


def norm_dnf(terms):
    terms = sorted({norm_term(t) for t in terms}, key=lambda t: (len(t), t))
    out = []
    for t in terms:
        # drop a term that a smaller-or-equal one already implies (its literals all hold in t)
        if any(all(dict(t).get(i, 0) >= c for i, c in o) for o in out):
            continue
        out.append(t)
    return tuple(out)


def dnf_and(a, b):
    return norm_dnf([x + y for x in a for y in b])


def dnf_or(a, b):
    return norm_dnf(list(a) + list(b))


class ReqParser:
    def __init__(self, text, items):
        self.toks = re.findall(r">=|\d+|\w+|[()&|]|\S", text)
        self.pos = 0
        self.items = items

    def peek(self):
        return self.toks[self.pos] if self.pos < len(self.toks) else None

    def take(self, want=None):
        tok = self.peek()
        if tok is None or (want is not None and tok != want):
            raise RuleError(f"expected {want or 'a requirement'}, found {tok or 'end of line'}")
        self.pos += 1
        return tok

    def parse(self):
        dnf = self.parse_or()
        if self.peek() is not None:
            raise RuleError(f"unexpected '{self.peek()}'")
        if len(dnf) > MAX_TERMS:
            raise RuleError(f"requirement expands to {len(dnf)} terms (limit {MAX_TERMS})")
        return dnf

    def parse_or(self):
        dnf = self.parse_and()
        while self.peek() == "|":
            self.take()
            dnf = dnf_or(dnf, self.parse_and())
        return dnf

    def parse_and(self):
        dnf = self.parse_atom()
        while self.peek() == "&":
            self.take()
            dnf = dnf_and(dnf, self.parse_atom())
            if len(dnf) > MAX_TERMS:
                raise RuleError(f"requirement expands to more than {MAX_TERMS} terms")
        return dnf

    def parse_atom(self):
        tok = self.take()
        if tok == "(":
            dnf = self.parse_or()
            self.take(")")
            return dnf
        if tok == "any":
            return ((),)
        if tok == "never":
            return ()
        if tok not in self.items:
            raise RuleError(f"unknown item or keyword '{tok}'")
        count = 1
        if self.peek() == ">=":
            self.take()
            num = self.take()
            if not num.isdigit() or int(num) < 1:
                raise RuleError(f"'{tok}>=' needs a count of at least 1, found '{num}'")
            count = int(num)
        return (((self.items[tok], count),),)


# ---- file parsing ---------------------------------------------------------------------------------

def parse_node(tok, ctx):
    m = re.fullmatch(r"(dock|loc|tel)(0[xX][0-9A-Fa-f]+|\d+)|@(\w+)", tok)
    if not m:
        raise RuleError(f"bad node '{tok}' (dockN, locEDITORID, telEDITORID or @name)")
    if m.group(3):
        return ("@", m.group(3))
    return (m.group(1), int(m.group(2), 0))


class Block:
    def __init__(self, name, line):
        self.name, self.line = name, line
        self.edges = []        # (line, from node, to node, dnf, both ways)
        self.covered = {}      # obstacle id -> line


def parse_file(path, items):
    blocks, cur = [], None

    def fail(n, msg):
        raise RuleError(f"{path}:{n}: {msg}")

    for n, raw in enumerate(path.read_text().splitlines(), 1):
        line = raw.split("#", 1)[0].rstrip()
        if not line.strip():
            continue
        try:
            m = re.fullmatch(r"area\s+(\S+)", line)
            if m and not line[0].isspace():
                cur = Block(m.group(1), n)
                blocks.append(cur)
                continue
            if cur is None:
                fail(n, "a rule before any 'area' line")
            line = line.strip()
            m = re.fullmatch(r"(covers)\s+(.+)", line) or re.fullmatch(r"(ignore)\s+(\S+?)\s*:\s*(\S.*)", line)
            if m and m.group(1) == "covers":
                for tok in m.group(2).split():
                    if not re.fullmatch(r"0[xX][0-9A-Fa-f]+", tok):
                        fail(n, f"bad obstacle id '{tok}'")
                    oid = int(tok, 16)
                    if oid in cur.covered:
                        fail(n, f"obstacle 0x{oid:08X} already listed at line {cur.covered[oid]}")
                    cur.covered[oid] = n
                continue
            if m:
                if not re.fullmatch(r"0[xX][0-9A-Fa-f]+", m.group(2)):
                    fail(n, f"bad obstacle id '{m.group(2)}'")
                oid = int(m.group(2), 16)
                if oid in cur.covered:
                    fail(n, f"obstacle 0x{oid:08X} already listed at line {cur.covered[oid]}")
                cur.covered[oid] = n
                continue
            m = re.fullmatch(r"([@\w]+)\s*([->])\s*([@\w]+)\s*:\s*(.+)", line)
            if not m:
                fail(n, f"cannot parse '{line}'")
            a, b = parse_node(m.group(1), None), parse_node(m.group(3), None)
            cur.edges.append((n, a, b, ReqParser(m.group(4), items).parse(), m.group(2) == "-"))
        except RuleError as e:
            if str(e).startswith(str(path)):
                raise
            fail(n, str(e))
    return blocks


# ---- validation and emit --------------------------------------------------------------------------

def validate(path, blocks, world_idx, data):
    worlds, areas, docks, teleporters, locations, dock_locks, obstacles = data
    by_name = {}
    for gi, (w, _m, _d, name) in enumerate(areas):
        if w == world_idx:
            by_name.setdefault(name, []).append(gi)
    dock_nums = {}
    for (a, d) in dock_locks:
        dock_nums.setdefault(a, set()).add(d)
    locs, tels, obst = {}, {}, {}
    for a, _l, eid, *_ in locations:
        locs.setdefault(a, set()).add(eid)
    for a, _l, eid, *_ in teleporters:
        tels.setdefault(a, set()).add(eid)
    for o in obstacles:
        obst.setdefault(o[0], set()).add(o[3])
    seen, out = {}, []
    for b in blocks:
        def fail(n, msg):
            raise RuleError(f"{path}:{n}: {msg}")
        if b.name not in by_name:
            fail(b.line, f"no area '{b.name}' in world {worlds[world_idx][1]}")
        if len(by_name[b.name]) > 1:
            fail(b.line, f"area name '{b.name}' is not unique in the world")
        gi = by_name[b.name][0]
        if gi in seen:
            fail(b.line, f"area '{b.name}' already has a block at line {seen[gi]}")
        seen[gi] = b.line
        named = {}
        for n, a, c, _dnf, _both in b.edges:
            for kind, val in (a, c):
                if kind == "dock" and val not in dock_nums.get(gi, ()):
                    fail(n, f"area {b.name} has no dock {val}")
                if kind == "loc" and val not in locs.get(gi, ()):
                    fail(n, f"area {b.name} has no location 0x{val:08X}")
                if kind == "tel" and val not in tels.get(gi, ()):
                    fail(n, f"area {b.name} has no teleporter 0x{val:08X}")
                if kind == "@":
                    named.setdefault(val, len(named))
        have = obst.get(gi, set())
        for oid, n in sorted(b.covered.items()):
            if oid not in have:
                fail(n, f"area {b.name} has no obstacle 0x{oid:08X}")
        missing = sorted(have - set(b.covered))
        if missing:
            fail(b.line, f"area {b.name}: obstacles not in covers/ignore: " +
                 " ".join(f"0x{o:08X}" for o in missing))
        out.append((gi, b, named))
    return out


class Emit:
    def __init__(self):
        self.lits, self.terms, self.reqs = [], [], []
        self.req_ix = {}
        self.blocks, self.edges = [], []

    def req(self, dnf):
        # the terms of one requirement are contiguous; identical requirements share one row
        if dnf not in self.req_ix:
            first = len(self.terms)
            for t in dnf:
                self.terms.append((len(self.lits), len(t)))
                self.lits.extend(t)
            self.req_ix[dnf] = len(self.reqs)
            self.reqs.append((first, len(dnf)))
        return self.req_ix[dnf]

    def node(self, n, named):
        kind, val = n
        return KINDS[kind], (named[val] if kind == "@" else val)


def render(validated):
    em = Emit()
    for gi, b, named in validated:
        em.blocks.append((gi, b.name, len(b.edges)))
        for n, a, c, dnf, both in b.edges:
            r = em.req(dnf)
            fk, fi = em.node(a, named)
            tk, ti = em.node(c, named)
            em.edges.append((gi, fk, fi, tk, ti, r, f"{b.name} line {n}"))
            if both:
                em.edges.append((gi, tk, ti, fk, fi, r, f"{b.name} line {n} (reverse)"))
    lines = [
        "// Generated by tools/rando_rules.py from tools/rando_rules/*.rules.",
        "// Do not edit by hand.",
        "#ifdef PORT_RANDO_LIT",
        "// PORT_RANDO_LIT(item, count)",
    ]
    lines += [f"PORT_RANDO_LIT({i}, {c})" for i, c in em.lits]
    lines += ["#endif", "#ifdef PORT_RANDO_TERM",
              "// PORT_RANDO_TERM(first literal, literal count): an AND; zero literals = always true"]
    lines += [f"PORT_RANDO_TERM({s}, {n})" for s, n in em.terms]
    lines += ["#endif", "#ifdef PORT_RANDO_REQ",
              "// PORT_RANDO_REQ(first term, term count): an OR; zero terms = never"]
    lines += [f"PORT_RANDO_REQ({s}, {n})" for s, n in em.reqs]
    lines += ["#endif", "#ifdef PORT_RANDO_BLOCK",
              "// PORT_RANDO_BLOCK(area, name, edge count): the areas that have rules"]
    lines += [f'PORT_RANDO_BLOCK({a}, "{n}", {e})' for a, n, e in em.blocks]
    lines += ["#endif", "#ifdef PORT_RANDO_EDGE",
              "// PORT_RANDO_EDGE(area, from kind, from id, to kind, to id, req); kind is a NodeKind,",
              "//   id the dock number, location/teleporter editor id, or the area's @name index"]
    for a, fk, fi, tk, ti, r, why in em.edges:
        lines.append(f"PORT_RANDO_EDGE({a}, {fk}, {fi}, {tk}, {ti}, {r})  // {why}")
    lines.append("#endif")
    return "\n".join(lines) + "\n"


def main():
    args = sys.argv[1:]

    def opt(name, default):
        return pathlib.Path(args[args.index(name) + 1]).resolve() if name in args else default

    rules_dir, out = opt("--rules-dir", RULES_DIR), opt("--out", OUT)
    items = item_table()
    worlds, areas, docks, teleporters, locations, dock_locks, obstacles, _dp, _lp = rw.collect()
    data = (worlds, areas, docks, teleporters, locations, dock_locks, obstacles)
    by_world = {name: i for i, (_m, name) in enumerate(worlds)}
    validated, stats = [], {}
    try:
        for path in sorted(rules_dir.glob("*.rules")):
            if path.stem not in by_world:
                raise RuleError(f"{path}: no world named '{path.stem}' (have: {', '.join(by_world)})")
            wi = by_world[path.stem]
            v = validate(path, parse_file(path, items), wi, data)
            validated += v
            stats[wi] = v
    except RuleError as e:
        sys.exit(f"[rando_rules] {e}")
    validated.sort(key=lambda t: t[0])
    print("rule coverage:")
    for wi, (_m, name) in enumerate(worlds):
        v = stats.get(wi, [])
        total_areas = sum(1 for a in areas if a[0] == wi)
        covered = sum(len(b.covered) for _g, b, _n in v)
        total_obst = sum(1 for o in obstacles if areas[o[0]][0] == wi)
        total_obst_blocked = sum(1 for g, _b, _n in v for o in obstacles if o[0] == g)
        print(f"  {name}: areas {len(v)}/{total_areas}, obstacles covered {covered}/{total_obst}"
              f" ({total_obst_blocked} in areas with a block), edges {sum(len(b.edges) for _g, b, _n in v)}")
    if "--write" in args:
        out.write_text(render(validated))
        print(f"wrote {out}")


if __name__ == "__main__":
    main()
