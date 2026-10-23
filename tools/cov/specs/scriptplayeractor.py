# ScriptPlayerActor (REL 61) spec for tools/cov/run.sh; arms come from the coverage build
# (gcov_report.py), the breakpoints only count calls and the message ids. Drivers:
# tools/drivers/spa_*.con (areas and layers in their headers):
#   BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/scriptplayeractor.py <out> <world,area> \
#     --con tools/drivers/spa_<area>.con --fast --frames <n>
#   then tools/cov/gcov_report.py build-port-cov <out>/gcov --module ScriptPlayerActor
CLASS = 'CScriptPlayerActor'
SPEC = [
    ('AcceptScriptMsg fn_61_29F0', 'CScriptPlayerActor::AcceptScriptMsg(', ['((int*)$rdx)[2]'], 'max=4000'),
    ('Think fn_61_2630', 'CScriptPlayerActor::Think(', [], 'max=400'),
    ('LoadSuit fn_61_1A20', 'CScriptPlayerActor::LoadSuit(', ['$rsi'], 'max=400'),
    ('LoadBeam fn_61_1914', 'CScriptPlayerActor::LoadBeam(', ['$rsi'], 'max=400'),
    ('UsePlayerBeamModel fn_61_13A4', 'CScriptPlayerActor::UsePlayerBeamModel(', [], 'max=400'),
    ('SetIntoStateManager fn_61_970', 'CScriptPlayerActor::SetIntoStateManager(', ['$rdx'], 'max=400'),
    ('SetupEnvFx fn_61_AB8', 'CScriptPlayerActor::SetupEnvFx(', ['$rdx'], 'max=400'),
    ('SetupOfflineModelData fn_61_1C28', 'CScriptPlayerActor::SetupOfflineModelData(', [], 'max=400'),
    ('SetupOnlineModelData fn_61_1DC0', 'CScriptPlayerActor::SetupOnlineModelData(', [], 'max=400'),
    ('TouchModels fn_61_2238', 'CScriptPlayerActor::TouchModels(', [], 'max=400'),
    ('LoadPlayerActor fn_61_C0', 'LoadPlayerActor(', [], 'max=400'),
]
