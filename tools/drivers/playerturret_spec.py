# gdb coverage spec for the player turret module (ScriptPlayerTurret, module 63).
#
# Every entry is a line in src/MetroidPrime/ScriptObjects/PortCScriptPlayerTurret.cpp: the hit
# count of the line is the number of times that arm of the module ran.  The lines are the arm's
# own line (the statement only that arm reaches), so a run per phase (tools/drivers/
# playerturret_run.sh PHASE=...) gives each arm its own count.
#
#   tools/cov/run.sh <spec> <out> sand_world,06_sand_alamofront --driver '...' --fast
#
CLASS = 'CScriptPlayerTurret'
SRC = ('/home/odran/.local/share/opencode/worktree/'
       '3e7f972e5f2a67a4bf166b19e99a591913d8d4a4/wt-mp2-playerturret/src/MetroidPrime/'
       'ScriptObjects/PortCScriptPlayerTurret.cpp')


def loc(line):
    return 'loc:%s:%d' % (SRC, line)


SPEC = [
    # fn_63_84C (the shot)
    ('84C_player_null', loc(974), [], ''),
    ('84C_token_not_loaded', loc(982), [], ''),
    ('84C_projectile_null', loc(1012), [], ''),
    ('84C_base_anim_ok', loc(1032), [], ''),
    ('84C_mp_sound', loc(1049), [], ''),
    ('84C_sp_sound', loc(1054), [], ''),
    # fn_63_1BE8 (the messages)
    ('1BE8_create', loc(448), [], ''),
    ('1BE8_create_token', loc(453), [], ''),
    ('1BE8_activate', loc(459), [], ''),
    ('1BE8_activate_dead_player', loc(469), [], ''),
    ('1BE8_activate_player', loc(473), [], ''),
    ('1BE8_activate_anim', loc(498), [], ''),
    ('1BE8_deactivate', loc(513), [], ''),
    ('1BE8_deactivate_player', loc(516), [], ''),
    ('1BE8_start', loc(527), [], ''),
    ('1BE8_start_player', loc(534), [], ''),
    ('1BE8_start_solid', loc(537), [], ''),
    ('1BE8_start_orbit', loc(539), [], ''),
    ('1BE8_stop', loc(545), [], ''),
    ('1BE8_stop_model_flags', loc(555), [], ''),
    ('1BE8_stop_no_player', loc(561), [], ''),
    ('1BE8_stop_eject', loc(565), [], ''),
    ('1BE8_stop_kill', loc(571), [], ''),
    ('1BE8_stop_anim', loc(605), [], ''),
    ('1BE8_stop_deactivate', loc(608), [], ''),
    ('1BE8_damage_flash', loc(625), [], ''),
    ('1BE8_damage_mp_sound', loc(633), [], ''),
    ('1BE8_damage_sp_emitter', loc(640), [], ''),
    ('1BE8_damage_sender', loc(645), [], ''),
    ('1BE8_damage_owner', loc(650), [], ''),
    ('1BE8_damage_originator', loc(653), [], ''),
    ('1BE8_default', loc(659), [], ''),
]
