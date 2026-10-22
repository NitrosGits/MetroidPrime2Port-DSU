#!/usr/bin/env python3
"""SwampBossStage2 (module 79): run the code function fn_79_72FC (`ResetAttackPhase`) once.

The code node is in the module's code list, lbl_79_data_3F4 (.data 0x3F4 = `{0, -1,
fn_79_72FC}`), which fn_79_D60C installs at 0xDAE4, but no state's transition list names it, so
the fight never runs it. The console's `codecall` runs it through the port's own code table
(CGenericFSM2State::PortFindCodeFunction -> CSwampBossStage2::ResetAttackPhase), the same body
the FSM2 would call on a transition, so the coverage breakpoint on that body sees it. The note
records this arm as `forced (codecall <id> ResetAttackPhase)`.

usage: swampboss2_codecall.py <port>
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import swampboss2_ing as ing


def main():
    con = ing.Console(int(sys.argv[1]))
    # the layer request needs a loaded area, or the layer load never finishes.
    con.wait_frame(1200)
    lines, end = con.cmd("layer 6")
    print("[sb2] layer 6 -> %s %s" % (end, lines), flush=True)
    boss = ing.find_boss(con)
    if boss is None:
        print("[sb2] no boss in the world", flush=True)
        return 1
    print("[sb2] the boss is %s" % boss, flush=True)
    lines, end = con.cmd("codecall %s ResetAttackPhase" % boss)
    print("[sb2] codecall %s ResetAttackPhase -> %s %s" % (boss, end, lines), flush=True)
    con.wait_frame(con.frame() + 5)
    print("[sb2] frame %d" % con.frame(), flush=True)
    con.cmd("covdump")
    con.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
