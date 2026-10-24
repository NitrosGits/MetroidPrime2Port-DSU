# PillBug (module 48) forcing spec for tools/cov/force.sh, same area and driver as pb1's coverage
# runs (merge the gcov dirs in gcov_report.py, this one's included):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/pbug_force.py <out> \
#     42b935e4,0b_sand_hall --con tools/cov/specs/pillbug_sandhall.con --fast --frames 3000
_hits = [0]


def pause_phase2(gdb):
    # fn_48_BD8 0xD28 switch (mAnimSubState): case 0 at 0xD44, case 1 at 0xE10 and any value >= 2
    # leaves through 0xD34 bge .L_00000EF8 (the switch's no-case edge). The state writes 2 itself
    # (0xE58 li r0,0x2 / 0xE5C stw), but the body is still in kAS_Step when the state ends, so the
    # next Update never reads it back. Forced after a few natural Updates.
    _hits[0] += 1
    if _hits[0] > 4:
        gdb.execute('set var ((CPillBug*)$rdi)->mAnimSubState = 2')


CLASS = 'CPillBug'
FORCE = [
    ('BD8 Pause substate 2', 'CPillBug::Pause(', [], [pause_phase2], 0,
     'mAnimSubState 2 on the Update arm: the mAnimSubState switch no-case edge'),
]
