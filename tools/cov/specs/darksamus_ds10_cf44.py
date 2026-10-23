# ds10: the loader's null-model arm (fn_10_CF44, PortLoadDarkSamus; `lbz 0x36c` at 0x0000DA3C), the
# one arm the shipped DRKS placements never take: every LdrToModelData the loader calls comes back
# empty (force_ret.py FORCE_INNER), so the boss is not built and the loader returns null.
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_INPUT=LAYER2@400 \
#     tools/cov/force.sh tools/cov/specs/darksamus_ds10_cf44.py <out> "1baa96c2,10_cliff_rooftop" \
#     --fast --frames 900
CLASS = 'CDarkSamus'
FORCE_RET = []
FORCE_INNER = [
    ('CF44 null model (forced)', 'PortLoadDarkSamus(', 'LdrToModelData(', 1,
     {0: [('((rstl::optional_object<CModelData>*)$rax)->m_valid', '0')]},
     'the model data comes back empty'),
]
