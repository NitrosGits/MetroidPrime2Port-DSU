# The six Sandworm bodies that are ported but unmarked (the note's section g.3 "no marker" rows,
# in the classification's order): the spec watches their entries, so a run that never enters one
# proves the "dead in the retail" claim at runtime, not only in the static scan.
# Run: Agon 03_sand_temple layer 2 with tools/cov/specs/sandworm_temple.con.
CLASS = 'CSandworm'
SPEC = [
    # fn_56_2210: a straight copy of the eye's position (no arms at all)
    ('2210 GetX2E8', 'CSandwormEye::GetX2E8(', [], 'ret'),
    # fn_56_534C: the GRNT/NEXT connection test and the CActor cast
    ('534C CountScriptActors', 'CSandworm::CountScriptActors(', [], 'ret'),
    # fn_56_6324: the CONN/FOLW test
    ('6324 Unreferenced6324', 'CSandworm::Unreferenced6324(', [], 'ret'),
    # fn_56_C350: the two compares of the attack range
    ('C350 InAttackRange', 'CSandworm::InAttackRange(', [], 'ret'),
    # fn_56_C3D0: the actor's active flag
    ('C3D0 Activated', 'CSandworm::Activated(', [], 'ret'),
    # fn_56_10110: the joint-list search
    ('10110 UnreferencedMath10110', 'CSandworm::UnreferencedMath10110(', [], ''),
]
