# Grenchler (module 27) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Torvus Bog 01_swamp_entrance layers 4 and 8 hold Grenchlers:
#   tools/cov/run.sh tools/cov/specs/grenchler.py <out> 3dfd2249,01_swamp_entrance \
#     --con tools/cov/specs/grenchler_entrance.con --fast --frames 3000
CLASS = 'CGrenchler'
SPEC = [
    ('D7E0 Think', 'CGrenchler::Think(', [], 'max=4000'),
    ('FD2C AcceptScriptMsg', 'CGrenchler::AcceptScriptMsg(', [], 'max=4000'),
]
