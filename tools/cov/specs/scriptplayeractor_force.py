# ScriptPlayerActor (REL 61) forcing spec for tools/cov/force.sh, with the landing-site driver (merge
# the gcov dir with the natural runs' in gcov_report.py):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/scriptplayeractor_force.py <out> \
#     3bfa3eff,01_Temple_LandingSite --con tools/drivers/spa_landingsite.con --fast --frames 2000
CLASS = 'CScriptPlayerActor'
FORCE = [
    ('29F0 AcceptScriptMsg keep in manager', 'CScriptPlayerActor::AcceptScriptMsg(', ['((int*)$rdx)[2]'],
     ['$this->mFlags = $this->mFlags | 0x10'], 0,
     'mFlags 0x10 set (no shipped PLAC sets it): Deactivate leaves the actor in the player-actor list'),
]
