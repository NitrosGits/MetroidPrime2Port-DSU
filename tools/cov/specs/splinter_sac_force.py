# Splinter (REL 74) spl3 lane, forced: CreateSac's mSacAncs == kInvalidAssetId arm (0x2664 addis/cmplwi 0xffff; beq 0x27A0).
# No shipped splinter on a cable lacks the sac asset, so the force writes mSacAncs = -1 at CreateSac's entry.
#   COV_GDB_SCRIPT=$PWD/tools/cov/force.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/splinter_sac_force.py <out> 3bfa3eff,0e_temple_hive_hall \
#     --con tools/cov/specs/splinter_grab.con --fast --frames 1300
CLASS = 'CSplinter'
FORCE = [
    ('CreateSac no ancs', 'CSplinter::CreateSac(', ['$this->mSac.mSacAncs', '$this->mSac.mSacId.value'],
     ['$this->mSac.mSacAncs = -1'], 0, 'mSacAncs = kInvalidAssetId'),
]
SPEC = []
