#!/usr/bin/env python3
"""CIng fn_29_3684 (LeaveTeam) arms driver (lane tail7): the team the Ing joins, and the leaves.

The area is Sand World `02_sand_sandfalls_dark` (area 25) with layer 5 ('AFTER') on: its two ing
spots (INGS editor ids 0x14190138 and 0x14190139) each carry an `ACTV`/`PLAY` connection to the
CScriptTeamAiMgr 0x14190161 (disc scan; no other Ing area loads a team, IngFamily.md A.33). The
first Ing in the area joins that team on its own, so `var <ing> ing.team` reads a live team id to
force with. `codecall <ing> SetTargetDest` runs fn_29_5B60, whose JoinTeam (fn_29_372C) resolves
the connection into `mTeamId` at 0xd6a; `send <ing> Deactivate` runs the kSM_Deactivate handler
(asm 0xCCB8) -> LeaveTeam; `send <ing> Delete` runs the kSM_Delete handler (asm 0xCC28).

  * arm 0x36B0 (the id at 0xd6a is kInvalidUniqueId): `var ing.team 65535` + Deactivate;
  * arm 0x36D0 (ObjectById/TCastToPtr give no CScriptTeamAiMgr): `var ing.team` = a live object
    that is not a team (the other Ing), and = 1 (an id the area does not load), + Deactivate;
  * arm 0x36EC (a real team that does not count the Ing): `var ing.team` = the team id without
    joining it, + Deactivate;
  * the body (QuitTeam 0x3704 + the id reset 0x3710): SetTargetDest (join), `var ing.team` must
    become the team id, Deactivate, `var ing.team` must be 65535 again.

Each forced value and the action share one tick (`a; b`), so the Ing's own Think cannot rewrite
0xd6a in between. Every step prints the id before and after. The counter side is
tools/cov/specs/ing_team.py. usage: ing_team.py <port> <game log>
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1])
LOGPATH = sys.argv[2]
INVALID = 0xFFFF


def cmd(c):
    want = c.count(";") + 1
    s = None
    for _ in range(200):
        try:
            s = socket.create_connection(("127.0.0.1", PORT), timeout=10)
            break
        except OSError:
            time.sleep(1)
    if s is None:
        return ""
    try:
        s.sendall((c + "\n").encode())
        buf = b""
        while buf.count(b"=> ") < want:
            d = s.recv(65536)
            if not d:
                break
            buf += d
    except OSError:
        return ""
    finally:
        s.close()
    return buf.decode(errors="replace").strip()


def run(c):
    out = cmd(c)
    print(f"> {c}\n{out}", flush=True)
    return out


def objs(cls):
    """`objs` lines: 'u051f 0807007d  CIng                       a25 (x, y, z)' (a script object has
    no position). Returns (uid, editor id, class)."""
    found = []
    for line in cmd(f"objs {cls}").splitlines():
        m = re.match(r"\s*u([0-9a-f]{4})\s+([0-9a-f]{8})\s+(\S+)", line)
        if m and m.group(3) == cls:
            found.append(("u" + m.group(1), int(m.group(2), 16), m.group(3)))
    return found


def packed(uid):
    return int(uid[1:], 16)


def var(uid, name, value=None):
    out = run(f"var {uid} {name}" + ("" if value is None else f" {value}"))
    m = re.search(r"=\s*(-?\d+)", out)
    return int(m.group(1)) if m else None


def force(uid, name, value, *cmds):
    """Set a field and run the console commands in the same tick."""
    return run("; ".join([f"var {uid} {name} {value}"] + list(cmds)))


for _ in range(400):
    try:
        if "gameplay reached" in open(LOGPATH, errors="replace").read():
            break
    except OSError:
        pass
    time.sleep(1)
time.sleep(3)

run("layer 5")  # the ing spots and the team live in layer 5 ('AFTER') of this area
time.sleep(4)
ings = objs("CIng")
teams = objs("CScriptTeamAiMgr")
print("== ings", ings, "\n== teams", teams, flush=True)
if not ings:
    print("!! no ing: the arms cannot run", flush=True)
    sys.exit(0)
for i in ings:
    run(f"send {i[0]} Activate")
time.sleep(2)

ing = ings[0][0]
other = ings[1][0] if len(ings) > 1 else None
team = teams[0][0] if teams else None
print("== ing", ing, "other", other, "team", team, flush=True)

# The id the Ing joined on its own (or one it joins now): the team to force with.
natural = var(ing, "ing.team")
print("   team at start", natural, flush=True)
if natural in (None, INVALID):
    run(f"codecall {ing} SetTargetDest")
    natural = var(ing, "ing.team")
    print("   team after a join", natural, flush=True)
if team is None and natural not in (None, INVALID):
    team = "u%04x" % natural
    print("   team from the join", team, flush=True)

# Arm 0x36B0: the id at 0xd6a is kInvalidUniqueId.
force(ing, "ing.team", INVALID, f"send {ing} Deactivate")
print("   after the invalid-id leave", var(ing, "ing.team"), flush=True)
run(f"send {ing} Activate")
time.sleep(1)

# Arm 0x36D0 with an id that names nothing the area loads (ObjectById null).
force(ing, "ing.team", 1, f"send {ing} Deactivate")
print("   after the unloaded-id leave", var(ing, "ing.team"), flush=True)
run(f"send {ing} Activate")
time.sleep(1)

# Arm 0x36D0 with a live object that is no CScriptTeamAiMgr.
if other:
    force(ing, "ing.team", packed(other), f"send {ing} Deactivate")
    print("   after the non-team leave", var(ing, "ing.team"), flush=True)
    run(f"send {ing} Activate")
    time.sleep(1)

# The member arm on its own tick: join (SetTargetDest -> JoinTeam fn_29_372C), then Deactivate
# leaves the team - QuitTeam (asm 0x3704) and the id back to invalid (asm 0x3710).
force(ing, "ing.team", INVALID, f"codecall {ing} SetTargetDest")
print("   after the join", var(ing, "ing.team"), flush=True)
run(f"send {ing} Deactivate")
print("   after the member leave", var(ing, "ing.team"), flush=True)
run(f"send {ing} Activate")
time.sleep(1)

# The body and the 0x36EC arm in ONE tick: the Ing re-joins the team in its own Think (its state
# runs SetTargetDest), so a leave that is not followed by the not-a-member leave inside the same
# tick would find the role restored. `a; b; c` runs every command in one tick.
#   1. id invalid, SetTargetDest (fn_29_5B60 -> JoinTeam fn_29_372C), Deactivate:
#      the member leave - QuitTeam (asm 0x3704) then the id reset (asm 0x3710), the role erased;
#   2. Activate (the Deactivate above left it inactive, and the handler needs wasActive),
#      the team id forced, Deactivate: IsPartOfTeam is now false (asm 0x36E8/0x36EC).
run(f"var {ing} ing.team {INVALID}; codecall {ing} SetTargetDest; send {ing} Deactivate; "
    f"send {ing} Activate; var {ing} ing.team {packed(team)}; send {ing} Deactivate")
print("   after the member leave and the not-a-member leave, team", var(ing, "ing.team"), flush=True)
run(f"send {ing} Activate")
time.sleep(1)

# The member arm once more through the kSM_Delete call site (asm 0xCC28), on the second Ing.
if other and team:
    run(f"send {other} Activate")
    time.sleep(1)
    force(other, "ing.team", packed(team), f"send {other} Delete")
    var(other, "ing.team")
print("== 3684 arms done", flush=True)
