# ScriptPlayerProxy (module 62) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Single player places only the visor watchers (tools/scan_playerproxy_props.py):
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/scriptplayerproxy.py <out> \
#     42b935e4,05_portalpuzzle --con tools/cov/specs/spp_echo_portalpuzzle.con --fast --frames 1100
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/scriptplayerproxy.py <out> \
#     3bfa3eff,01_temple_landingsite_dark --con tools/cov/specs/spp_dark_landingsite.con --fast --frames 1100
CLASS = 'CScriptPlayerProxy'
SPEC = [
    ('1C44 Think', 'CScriptPlayerProxy::Think(', [], 'max=4000'),
    ('1778 CheckVisors', 'CScriptPlayerProxy::CheckVisors(', [], 'max=4000'),
    ('1868 InitVisors', 'CScriptPlayerProxy::InitVisors(', [], 'max=400'),
    ('13D8 SendStateToPlayer', 'CScriptPlayerProxy::SendStateToPlayer(', [], 'max=400'),
    ('23D4 AcceptScriptMsg', 'CScriptPlayerProxy::AcceptScriptMsg(', [], 'max=400'),
]
