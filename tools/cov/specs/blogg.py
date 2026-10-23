# Blogg (module 7) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Torvus Bog 07_swamp_shaft layers 2/3/5 and
# 08_swamp_grappleguardian layers 2/4/7 hold Bloggs:
#   BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/blogg.py <out> 3dfd2249,07_swamp_shaft \
#     --con tools/cov/specs/blogg_shaft.con --fast --frames 3000
CLASS = 'CBlogg'
SPEC = [
    ('87C8 Think', 'CBlogg::Think(', [], 'max=4000'),
    ('8F7C AcceptScriptMsg', 'CBlogg::AcceptScriptMsg(', [], 'max=4000'),
]
