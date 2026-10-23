# ScriptRiftPortal (module 64) spec for tools/cov/run.sh; arms come from the coverage build
# (gcov_report.py), the breakpoints only count calls. Driver:
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/riftportal.py <out> \
#     3bfa3eff,12_temple_finalgate --con tools/cov/specs/riftportal.con --frames 1600
CLASS = 'CScriptRiftPortal'
SPEC = [
    ('1CE4 AcceptScriptMsg', 'CScriptRiftPortal::AcceptScriptMsg(', [], 'max=4000'),
    ('2194 Think', 'CScriptRiftPortal::Think(', [], 'max=4'),
    ('2500 UpdateProjectiles', 'CScriptRiftPortal::UpdateProjectiles(', [], 'max=4'),
    ('0C88 Render', 'CScriptRiftPortal::Render(', [], 'max=4'),
    ('1C38 AddToRenderer', 'CScriptRiftPortal::AddToRenderer(', [], 'max=4'),
]
