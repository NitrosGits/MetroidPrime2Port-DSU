# StoneToad (module 77, in-game Watchdrone) spec for tools/cov/run.sh; arms come from the coverage
# build (gcov_report.py), the breakpoints only count calls. Driver:
#   BUILD=$PWD/build-port-cov SECS=1500 tools/cov/run.sh tools/cov/specs/stonetoad.py <out> \
#     3dfd2249,09_swamp_catacombs_dark --con tools/cov/specs/stonetoad.con --frames 1500
CLASS = 'CStoneToad'
SPEC = [
    ('22F4 AcceptScriptMsg', 'CStoneToad::AcceptScriptMsg(', [], 'max=4000'),
    ('13CC Touch', 'CStoneToad::Touch(', [], 'max=4000'),
    ('2168 Sleep', 'CStoneToad::Sleep(', ['$rdx'], 'max=4000'),
    ('1EC0 WakeUp', 'CStoneToad::WakeUp(', ['$rdx'], 'max=4000'),
    ('1978 MoveAway', 'CStoneToad::MoveAway(', ['$rdx'], 'max=4000'),
    ('193C Wait', 'CStoneToad::Wait(', ['$rdx'], 'max=4000'),
    ('186C MoveBack', 'CStoneToad::MoveBack(', ['$rdx'], 'max=4000'),
    ('158C GoToSleep', 'CStoneToad::GoToSleep(', ['$rdx'], 'max=4000'),
    ('1C84 ReactToSafeZone', 'CStoneToad::ReactToSafeZone(', ['$rdx'], 'max=4000'),
    ('1A48 ReactToWeapon', 'CStoneToad::ReactToWeapon(', ['$rdx'], 'max=4000'),
    ('21A4 PreRender', 'CStoneToad::PreRender(', [], 'max=4'),
    ('2184 AddToRenderer', 'CStoneToad::AddToRenderer(', [], 'max=4'),
]
