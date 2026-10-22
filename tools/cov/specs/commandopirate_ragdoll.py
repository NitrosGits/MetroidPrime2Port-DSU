# CommandoPirate (module 9) forcing spec for tools/cov/force.sh: the rag-doll arms of fn_9_3E1C
# (and fn_9_C944 / fn_9_C300) that a fight on land does not reach. The labels are retail offsets.
# Run (Torvus Bog: the commandos u0508 / u0500 in a24, the waters u058b active and u05bd inactive
# in the neighbouring a30 - the driver waits for a30 before it kills anything):
#   tools/cov/force.sh tools/cov/specs/commandopirate_ragdoll.py <out> 'swamp,05_swamp_timber!' --fast \
#     --frames 9000 --driver "python3 tools/drivers/commandopirate_ragdoll.py {port} <driver log>"
# force.py logs a FORCE entry's exprs as the hit's key; it does not gate on them, and the sets run
# on every hit past the skip. So every set below carries its own condition (a ternary).
CLASS = 'CCommandoPirate'
RD = '$this->xbfc_ragDoll.mPtr'
PRIMED = '(' + RD + ' != 0 && ' + RD + '->mPrimed)'
UID = '$this->mUniqueId.value'
FORCE = [
    ('4090 rag doll flags 3 (forced)', 'CCommandoPirate::x3E1C_UpdateRagDoll(',
     [RD + ' != 0', '$this->mData.mFlags0 & 1'],
     ['$this->mData.mFlags0 = $this->mData.mFlags0 | 1'], 0,
     "every call: the block's flag byte at guest this+0x91C gets its 0x01 bit, so the rag doll is "
     "created with the flags 3 (0x000040A0..0x000040C0) - it keeps making small movements "
     "(0x00003F90) and the KnockBack override pushes its torso (0x0000C324..0x0000C3AC)"),
    ('4020 timer runs out with a rag doll (forced)', 'CCommandoPirate::x3E1C_UpdateRagDoll(',
     [RD + ' != 0', '$this->xc00_ > 0'],
     ['$this->xc00_ = (' + RD + ' != 0 && $this->xc00_ > 0 ? 0.001 : $this->xc00_)'], 0,
     "the module's timer at guest this+0xC00 is put just above zero while the rag doll is there, so "
     "the creation is skipped (0x00004028) and the timer cleared"),
    ('3ED4 fluid: u0508 in the active water, u0500 in the inactive one (forced)',
     'CCommandoPirate::x3E1C_UpdateRagDoll(',
     [PRIMED, UID],
     ['$this->mFluidIds.mCount = (' + PRIMED + ' ? 1 : $this->mFluidIds.mCount)',
      '*(unsigned short*)&$this->mFluidIds.mData[0] = (' + PRIMED + ' ? (' + UID +
      ' == 0x0508 ? 0x058b : 0x05bd) : *(unsigned short*)&$this->mFluidIds.mData[0])'], 0,
     "a primed rag doll's fluid id is a30's water: u058b (active) for u0508, u05bd (inactive, the "
     "entity flag byte's 0x80 bit clear, 0x00003F1C) for u0500 (InFluidId at 0x00003ED4, the "
     "water's top at 0x00003F2C)"),
    ('CA74 primed rag doll, frozen body (forced)', 'CCommandoPirate::Think(',
     [RD + ' != 0'],
     ['$this->mBodyController.mPtr->mFrozen = (' + RD + ' != 0 ? 1 : $this->mBodyController.mPtr->mFrozen)'], 0,
     "the body controller is frozen while the rag doll is there, so the primed branch unfreezes it "
     "(0x0000CA74..0x0000CA78)"),
]
