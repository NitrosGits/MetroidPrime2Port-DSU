# Sandworm (module 56) proof spec for tools/cov/run.sh; labels are the retail offsets.
# Run: Agon 03_sand_temple layer 2 with tools/cov/specs/sandworm_temple.con.
CLASS = 'CSandworm'
SPEC = [
    # two arms: a held bomb (x1998_ valid) is deleted, or nothing happens
    ('95E0 DeleteHeldBomb', 'CSandworm::DeleteHeldBomb(', ['$this->x1998_.value != 0xffff'], ''),
    ('95E0 DeleteHeldBomb: delete arm', 'loc:PortCSandwormBombs.cpp:256', [], ''),
    ('9590 GetHeldBomb', 'CSandworm::GetHeldBomb(', ['$this->x1998_.value != 0xffff'], 'retp'),
    # first early-out: Bomb Guardian form (x8C0 bit 0x10) that is not Ing-possessed
    ('59A8 CanAttack', 'CSandworm::CanAttack(', ['($this->x8C0_flags & 0x10) != 0'], 'ret'),
]
