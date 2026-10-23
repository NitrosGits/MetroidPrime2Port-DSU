# DarkCommando (module 3) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Temple Grounds 06_temple_sacredpath_dark holds Dark Commandos;
# tools/drivers/darkcommando_states.py walks the state table, damages and kills them:
#   BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/darkcommando.py <out> 3bfa3eff,06_temple_sacredpath_dark \
#     --driver 'python3 tools/drivers/darkcommando_states.py {port} <out>/states.log' --fast --frames 6000
CLASS = 'CDarkCommando'
SPEC = [
    ('68B4 Think', 'CDarkCommando::Think(', [], 'max=4000'),
    ('6A48 AcceptScriptMsg', 'CDarkCommando::AcceptScriptMsg(', [], 'max=4000'),
]
