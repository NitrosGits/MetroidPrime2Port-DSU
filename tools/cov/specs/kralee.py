# Kralee (module 37) spec for tools/cov/run.sh; arms come from the coverage build
# (gcov_report.py), the breakpoints only count calls and a few entry values. Driver:
#   BUILD=$PWD/build-port-cov SECS=1500 tools/cov/run.sh tools/cov/specs/kralee.py <out> \
#     3bfa3eff,0b_temple_hall --con tools/cov/specs/kralee.con --frames 1300
# (no --fast: at 8 ticks a frame the killed Kralee is deleted before its PreRender)
CLASS = 'CKralee'
SPEC = [
    ('1BA0 AcceptScriptMsg', 'CKralee::AcceptScriptMsg(', [], 'max=4000'),
    ('20DC Patrol', 'CKralee::Patrol(', ['$rdx'], 'max=4000'),
    ('1DB8 WarpOut', 'CKralee::WarpOut(', ['$rdx'], 'max=4000'),
    ('0E18 WarpOutExit', 'CKralee::WarpOutExit(', [], 'max=4000'),
    ('117C WarpIn', 'CKralee::WarpIn(', [], 'max=4000'),
    ('1080 PreRender', 'CKralee::PreRender(', [], 'max=4'),
    ('1018 AddToRenderer', 'CKralee::AddToRenderer(', [], 'max=4'),
]
