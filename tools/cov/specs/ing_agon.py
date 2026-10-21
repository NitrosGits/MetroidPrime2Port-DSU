# Ing (module 29) proof spec for tools/cov/run.sh; the labels are the retail offsets.
# Run: the Agon Wastes 01_sand_entrance_dark (the world 42b935e4, the area 0x3f5cdf39) with
# tools/cov/specs/ing_agon.con.
CLASS = 'CIng'
SPEC = [
    # fn_29_B890 (TakeDamage, 0x30): the early return in the forms 0, 1 and 5 (asm 0x0000B894-
    # 0x0000B8AC) and the base class's store in the other forms (asm 0x0000B8B0) - the form is
    # the key.
    ('B890 TakeDamage', 'CIng::TakeDamage(', ['$this->mForm'], 'max=400'),
    # fn_29_B534 (CollidedWith, 0x250): the form (the form 4 is the body projectile, asm
    # 0x0000B558) and the call sites of the body (the 'calls' flag).
    ('B534 CollidedWith', 'CIng::CollidedWith(', ['$this->mForm'], 'calls max=400'),
    # fn_29_4C88 (the create handler, 0x90): the data flag byte 0x9d0's bit 7 (asm 0x00004CAC)
    # picks the form - the key is the flag word; the 'calls' flag shows the create branch (its
    # own call).
    ('4C88 the create handler', 'CIng::AcceptScriptMsg(', ['$this->mData.mFlags210'],
     'calls max=3000'),
]
