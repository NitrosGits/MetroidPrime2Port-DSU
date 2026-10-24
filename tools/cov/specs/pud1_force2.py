# PuddleSpore (module 51) forcing spec, second window: fn_51_1574 Run's mAnimPhase switch.
# 0x15D8 switch (mAnimPhase): case 0 and case 1 have their own arms and any value >= 2 leaves
# through 0x15E4 bge .L_000016E8 (the switch's no-case arc), which Run's own phase 1 writes
# (0x1694 stw 0x2, 0x850(r31)) but a short run leaves the state before it is read back.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/pud1_force2.py <out> \
#     3dfd2249,cbc72d79 --con tools/cov/specs/puddlespore_pud1c.con --fast --frames 1900
_hits = [0]


def phases(gdb):
    # forced for the first Run window only, so the state machine frees the state afterwards
    _hits[0] += 1
    if 2 <= _hits[0] <= 6:
        gdb.execute('set var ((CPuddleSpore*)$rdi)->mAnimPhase = 1')
    elif 7 <= _hits[0] <= 10:
        gdb.execute('set var ((CPuddleSpore*)$rdi)->mAnimPhase = 2')


CLASS = 'CPuddleSpore'
FORCE = [
    ('1574 mAnimPhase 1 then 2', 'CPuddleSpore::Run(', [], [phases], 0,
     'mAnimPhase 1 (case 1) then 2 (the switch no-case arc) in the first Run window'),
]
