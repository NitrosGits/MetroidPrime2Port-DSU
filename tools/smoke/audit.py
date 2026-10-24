#!/usr/bin/env python3
"""Audit smoke transcripts and print a markdown table.

Usage: python3 tools/smoke/audit.py <smoke_dir> [family ...]
"""
import os
import re
import sys


def transcript_path(d, fam):
    con = os.path.join(d, "con_%s.out" % fam)
    if os.path.isfile(con):
        return con
    return None


def read_rc(d, fam):
    p = os.path.join(d, "%s.rc" % fam)
    if not os.path.isfile(p):
        return None
    try:
        return int(open(p).read().strip())
    except ValueError:
        return None


def hp_fall_evidence_line(txt):
    cur_uid = None
    prev_uid_hp = {}
    prev_tuple = None
    for raw in txt.splitlines():
        m = re.match(r"\s*(u[0-9a-f]{4})\b", raw)
        if m:
            cur_uid = m.group(1)
            continue
        # arrow form: hp A -> B
        for a, b in re.findall(r"hp\s+(-?[\d.]+)\s*->\s*(-?[\d.]+)", raw):
            if float(b) < float(a):
                return raw.strip()
        # obj read form: hp X/Y
        m = re.search(r"hp\s+(-?[\d.]+)/(-?[\d.]+)", raw)
        if m:
            try:
                val = float(m.group(1))
            except ValueError:
                val = None
            if val is not None:
                if cur_uid is not None and cur_uid in prev_uid_hp and val < prev_uid_hp[cur_uid]:
                    return raw.strip()
                if cur_uid is not None:
                    prev_uid_hp[cur_uid] = val
        # obj read form: hp (X, Y)
        m = re.search(r"hp\s*\(\s*(-?[\d.]+)\s*,\s*(-?[\d.]+)\s*\)", raw)
        if m:
            try:
                val = float(m.group(1))
            except ValueError:
                val = None
            if val is not None:
                if cur_uid is not None and cur_uid in prev_uid_hp and val < prev_uid_hp[cur_uid]:
                    return raw.strip()
                if cur_uid is not None:
                    prev_uid_hp[cur_uid] = val
                else:
                    if prev_tuple is not None and val < prev_tuple:
                        return raw.strip()
                    prev_tuple = val
        # driver form: -> hp (X, Y)
        m = re.search(r"->\s*hp\s*\(\s*(-?[\d.]+)\s*,\s*(-?[\d.]+)\s*\)", raw)
        if m:
            try:
                val = float(m.group(1))
            except ValueError:
                val = None
            if val is not None:
                if cur_uid is not None and cur_uid in prev_uid_hp and val < prev_uid_hp[cur_uid]:
                    return raw.strip()
                if cur_uid is not None:
                    prev_uid_hp[cur_uid] = val
                else:
                    if prev_tuple is not None and val < prev_tuple:
                        return raw.strip()
                    prev_tuple = val
    return None


def has_hp_fall(txt):
    return hp_fall_evidence_line(txt) is not None


def fsm_change_evidence_line(txt):
    for raw in txt.splitlines():
        m = re.search(r"fsm\b[^\n]*?(\S+)\s*->\s*(\S+)", raw)
        if m:
            a = m.group(1).strip("[]")
            b = m.group(2).strip("[]")
            if a != b:
                return raw.strip()
    return None


def has_fsm_change(txt):
    return fsm_change_evidence_line(txt) is not None


def kill_marker(txt):
    low = txt.lower()
    return ("killed" in low) or ("delete requested" in low)


def kill_evidence_line(txt):
    for raw in txt.splitlines():
        if "killed" in raw or "delete requested" in raw:
            return raw.strip()
    return None


def objs_counts(txt):
    current = None
    counts = {}
    for raw in txt.splitlines():
        m = re.search(r"objs\s+([A-Za-z0-9_]+)", raw)
        if m:
            current = m.group(1)
        m = re.search(r"\b(\d+)\s+objects\b", raw)
        if m:
            q = current if current is not None else "__all__"
            counts.setdefault(q, []).append(int(m.group(1)))
    return counts


def has_count_drop(txt):
    counts = objs_counts(txt)
    for vals in counts.values():
        if any(vals[i] < vals[j] for j in range(len(vals)) for i in range(j + 1, len(vals))):
            return True
    return False


def objs_evidence_line(txt):
    for raw in txt.splitlines():
        if re.search(r"\b\d+\s+objects\b", raw):
            return raw.strip()
    return None


def classify(fam, d):
    rc = read_rc(d, fam)
    if rc is None:
        # no rc -> treat as skipped (handled by caller); but if forced, mark FAIL
        pass
    tpath = transcript_path(d, fam)
    if tpath is None:
        return None, None
    txt = open(tpath, errors="replace").read()
    if rc is not None and rc != 0:
        return "FAIL", "rc=%d" % rc
    hp_line = hp_fall_evidence_line(txt)
    fsm_line = fsm_change_evidence_line(txt)
    km = kill_marker(txt)
    cd = has_count_drop(txt)
    hp = hp_line is not None
    fsm = fsm_line is not None
    kill_ev = km and cd
    missing = []
    if not hp:
        missing.append("no hp fall")
    if not fsm:
        missing.append("no fsm change")
    if not km:
        missing.append("no delete/kill")
    elif not cd:
        missing.append("no objs count drop")
    if hp and fsm and kill_ev:
        cls = "STRONG"
    else:
        cls = "WEAK"
    if hp_line is not None:
        tell = hp_line
    elif fsm_line is not None:
        tell = fsm_line
    elif km:
        tell = kill_evidence_line(txt)
    else:
        ol = objs_evidence_line(txt)
        if ol is not None:
            tell = ol
        else:
            tell = ""
            for raw in txt.splitlines():
                s = raw.strip()
                if not s:
                    continue
                low = s.lower()
                if "gone" in low or "error" in low or "broken" in low:
                    tell = s
                    break
            if tell == "":
                for raw in txt.splitlines():
                    s = raw.strip()
                    if s:
                        tell = s
                        break
    reason = "; ".join(missing)
    return cls, reason, tell


def main(argv):
    if len(argv) < 2:
        print("usage: %s <smoke_dir> [family ...]" % argv[0], file=sys.stderr)
        return 2
    d = argv[1]
    if len(argv) > 2:
        fams = argv[2:]
    else:
        fams = []
        for fn in os.listdir(d):
            if fn.endswith(".rc"):
                fams.append(fn[:-3])
        fams = sorted(set(fams))
    # discover any con/log families not in rc list
    for fn in os.listdir(d):
        if fn.startswith("con_") and fn.endswith(".out"):
            fams.append(fn[4:-4])
        elif fn.startswith("log_") and fn.endswith(".log"):
            fams.append(fn[4:-4])
    fams = sorted(set(fams))

    audited = []
    skipped = []
    for fam in fams:
        tpath = transcript_path(d, fam)
        rc = read_rc(d, fam)
        if tpath is None or rc is None:
            skipped.append(fam)
            continue
        audited.append(fam)

    dirname = os.path.basename(os.path.normpath(d))
    print("# Smoke transcript audit (%s)" % dirname)
    print()
    if skipped:
        print("Families audited: %d (skipped %s)" % (len(audited), ", ".join(skipped)))
    else:
        print("Families audited: %d (skipped none)" % len(audited))
    print()
    print("| family | class | missing/reason | telling line |")
    print("|---|---|---|---|")
    for fam in audited:
        res = classify(fam, d)
        if res[0] == "FAIL":
            cls, reason = res[0], res[1]
            tell = ""
        else:
            cls, reason, tell = res
        tell_cell = "`%s`" % tell if tell != "" else "``"
        print("| %s | %s | %s | %s |" % (fam, cls, reason, tell_cell))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
