#!/usr/bin/env python3
"""Compare two batch smoke output dirs (tools/smoke/run.sh writes one).

usage: compare.py <new out dir> <old out dir>

One row per family: the run's exit code, the caught and reach-stub counts, the port stand-in lines
and the seconds until gameplay was first reached. Exits 1 when a family regressed: the new run
failed where the old one passed, or the caught or reach-stub count went up.

Both the current layout (<out>/log_<family>.log) and the old one (the smoke7 suite kept some
families in <family>.log) are read.
"""
import os
import re
import sys

FAMILIES = ["swamp1", "swamp2", "sandboss", "sandworm", "boostg", "boostg_fsm", "digig",
            "cmdpirate", "emp1", "spacepirate", "ing", "ep", "plain", "emp3", "turret", "agon54",
            "darksamus", "splitter", "grenchler", "grenguard", "flyingpirate", "mediuming",
            "metroid", "sporb", "blogg", "swarm", "gunturret", "rezbit", "parasite", "lumite",
            "shrieker", "ingswarm", "darktrooper", "bacteriaswarm", "tryclops", "wisptentacle",
            "octapede", "amt", "dbar", "sjguard", "fishcloud", "spa", "wallwalker", "isbg", "spp",
            "mysf", "spor", "scriptgui", "kral", "gbug", "pssm", "stod", "pbug", "szone",
            "splinter", "rptl", "chog", "drkc", "plac", "plct", "ming", "snak", "eyeb", "kroc",
            "atma", "atmb", "shrd", "fsw", "iss", "ibs", "mree", "puff", "rubk", "emp2", "cball",
            "fobj", "skyr", "dsbs", "isg"]
# the old suite named some logs after the family alone
ALIASES = {"ing": "ing.log", "ep": "ep.log", "plain": "plain.log"}


def read(path):
    try:
        with open(path, "rb") as f:
            return f.read().decode("utf-8", "replace")
    except OSError:
        return None


def log_of(out, tag):
    """The game's log for a family, or None when the family did not run."""
    for name in ("log_%s.log" % tag, ALIASES.get(tag, "")):
        if not name:
            continue
        path = os.path.join(out, name)
        if os.path.isfile(path):
            return read(path)
    return None


def stats(text):
    if text is None:
        return None
    exits = re.findall(r"^exit=(\d+)", text, re.M)
    game = re.findall(r"gameplay reached at ([0-9.]+) s", text)
    return {
        "exit": exits[-1] if exits else "?",
        "caught": len(re.findall(r"\[port\] caught", text)),
        "stub": len(re.findall(r"reach-stub", text)),
        "standin": len(re.findall(r"is a port stand-in", text)),
        "gs": game[-1] if game else "?",
        "standin_names": set(re.findall(r"\[([a-z0-9]+)\] ([^\n]*?) is a port stand-in", text)),
    }


def cell(s):
    if s is None:
        return "(no log)"
    return "exit=%s caught=%d stub=%d standin=%d gs=%s" % (
        s["exit"], s["caught"], s["stub"], s["standin"], s["gs"])


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    new_dir, old_dir = argv[1], argv[2]
    print("%-12s %-46s %s" % ("run", "new (%s)" % os.path.basename(os.path.normpath(new_dir)),
                               "old (%s)" % os.path.basename(os.path.normpath(old_dir))))
    regressions = []
    for tag in FAMILIES:
        n = stats(log_of(new_dir, tag))
        o = stats(log_of(old_dir, tag))
        if n is None and o is None:
            continue
        print("%-12s %-46s %s" % (tag, cell(n), cell(o)))
        if n is None or o is None:
            continue
        if o["exit"] == "0" and n["exit"] != "0":
            regressions.append("%s: exit %s (old 0)" % (tag, n["exit"]))
        if n["caught"] > o["caught"]:
            regressions.append("%s: caught %d (old %d)" % (tag, n["caught"], o["caught"]))
        if n["stub"] > o["stub"]:
            regressions.append("%s: reach-stub %d (old %d)" % (tag, n["stub"], o["stub"]))
        if n["standin"] > o["standin"]:
            print("    stand-ins went up: %d (old %d)" % (n["standin"], o["standin"]))
        added = n["standin_names"] - o["standin_names"]
        if added:
            print("    NEW stand-ins: %s" % "; ".join(sorted(added)[:10]))
    if regressions:
        print("\nREGRESSIONS:")
        for r in regressions:
            print("  " + r)
        return 1
    print("\nno regressions")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
