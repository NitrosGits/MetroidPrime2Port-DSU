# gdb coverage and forcing spec for the player turret module (ScriptPlayerTurret, module 63).
#
# SPEC: one entry per arm - a line in src/MetroidPrime/ScriptObjects/PortCScriptPlayerTurret.cpp;
# the hit count of the line is the number of times that arm ran.  The lines are found by their
# text, so the table follows the source.  Used with tools/cov/run.sh (with
# COV_GDB_SCRIPT=tools/cov/cov_force.py it counts the arms as well as forces them).
#
# FORCE: the arms the game's flow never reaches, forced by writing the state the guest's test
# reads (tools/cov/force.sh).  The three forces are switched by the environment, so each run
# covers one of them:
#   PT_FORCE_MP=1     the game mode type is 'DM' while the turret asks: the guest's two game-mode
#                     tests (the shot's sound at 0xA88 and the damage message's sound at 0x225C).
#                     The port cannot start a match (the harness installs CGMSinglePlayer), so
#                     this is the only way to reach them.
#   PT_FORCE_DEAD=1   the player's health is -1 while the turret's kSM_Activate case reads it:
#                     the guest's test at 0x1DE4 is (hp == 0) | (hp < 0).
#   PT_FORCE_NOHPI=1  the kSM_Damage message's sender is the turret itself, whose HealthInfo() is
#                     null (CActor::HealthInfo returns nullptr): the guest's test at 0x231C.
import os

CLASS = 'CScriptPlayerTurret'
SRC = ('/home/odran/.local/share/opencode/worktree/'
       '3e7f972e5f2a67a4bf166b19e99a591913d8d4a4/wt-mp2-playerturret/src/MetroidPrime/'
       'ScriptObjects/PortCScriptPlayerTurret.cpp')

_lines = open(SRC).read().split('\n')


def line_of(text, start=1, nth=1):
    """The line number of the nth line holding `text`, from `start` on."""
    n = 0
    for i in range(start - 1, len(_lines)):
        if text in _lines[i]:
            n += 1
            if n == nth:
                return i + 1
    raise SystemExit('playerturret_spec: not found: %r' % text)


SHOT = line_of('void CScriptPlayerTurret::FireTurretShot(')
MSG = line_of('void CScriptPlayerTurret::AcceptScriptMsg(')


def loc(text, start=1, nth=1, off=0):
    return 'loc:%s:%d' % (SRC, line_of(text, start, nth) + off)


SPEC = [
    # fn_63_70 (the REL's setup - the port's RELMain)
    ('fn_63_70', loc('SetSPlayerTurret_FuncPtrs(&REL_loader_PlayerTurret);'), [], ''),
    # fn_63_0 (the vtable's slot 15)
    ('fn_63_0', loc('return const_cast< CScriptPlayerTurret* >(this)->HealthInfo();'), [], ''),
    # fn_63_1134 (the input path)
    ('1134_fire_pressed', loc('if (player->FireBeamPressed(input)) {'), [], ''),
    ('1134_reload_test', loc('if (mUnknown1B8 <= 0.f) {'), [], ''),
    # fn_63_84C (the shot)
    ('84C_player_null', loc('return;', SHOT, 1), [], ''),
    ('84C_token_not_loaded', loc('return;', SHOT, 2), [], ''),
    ('84C_projectile_null', loc('return;', SHOT, 3), [], ''),
    ('84C_base_anim_ok', loc('animData->EnableLooping(false);', SHOT), [], ''),
    ('84C_mp_sound', loc('CSfxManager::SfxStart(mSfxMultiPlayerProjectile', SHOT), [], ''),
    ('84C_sp_sound', loc('CSfxManager::SfxStart(mSfxSinglePlayerProjectile', SHOT), [], ''),
    # fn_63_1BE8 (the messages)
    ('1BE8_create', loc('const CAssetId effect =', MSG), [], ''),
    ('1BE8_create_token', loc('mToken = rs_new CToken(', MSG), [], ''),
    ('1BE8_activate', loc('mPlayerId = msg.GetOriginator();', MSG), [], ''),
    ('1BE8_activate_dead_player', loc('return;', MSG, 1), [], ''),
    ('1BE8_activate_player', loc('player->StartTurret(', MSG), [], ''),
    ('1BE8_activate_anim', loc('AddAdditiveAnimation(mAnimId1', MSG, 1), [], ''),
    ('1BE8_deactivate', loc('CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mPlayerId));', MSG, 1), [], ''),
    ('1BE8_deactivate_player', loc('player->SetTurretState(CPlayer::kTS_None, mgr);', MSG), [], ''),
    ('1BE8_start', loc('if (!GetActive()) {', MSG, 1), [], ''),
    ('1BE8_start_player', loc('player->SetTurretState(CPlayer::kTS_Active, mgr);', MSG), [], ''),
    ('1BE8_start_solid', loc('AddSolidToTopActor(mgr);', MSG), [], ''),
    ('1BE8_start_orbit', loc('AddMaterial(kMT_Orbit, mgr);', MSG), [], ''),
    ('1BE8_stop', loc('if (!GetActive()) {', MSG, 2), [], ''),
    ('1BE8_stop_model_flags', loc('base->SetModelFlags(mModelFlags);', MSG), [], ''),
    ('1BE8_stop_no_player', loc('SetActive(false);', MSG, 1), [], ''),
    ('1BE8_stop_eject', loc('player->EjectFromTurret(', MSG), [], ''),
    ('1BE8_stop_kill', loc('mgr.KillPlayer(', MSG), [], ''),
    ('1BE8_stop_anim', loc('CAnimPlaybackParms(mUnknown1AC', MSG), [], ''),
    ('1BE8_stop_deactivate', loc('SetActive(false);', MSG, 2), [], ''),
    ('1BE8_damage_flash', loc('mUnknown20C = 0.15f;', MSG), [], ''),
    ('1BE8_damage_mp_sound', loc('CSfxManager::SfxStart(mSfxMultiPlayerImpact', MSG), [], ''),
    ('1BE8_damage_sp_emitter', loc('CSfxManager::AddEmitter(mSfxSinglePlayerImpact', MSG), [], ''),
    ('1BE8_damage_sender', loc('mgr.GetObjectById(msg.GetSenderId())', MSG), [], ''),
    ('1BE8_damage_owner', loc('mHullActorId = health->GetLastDamageOwner();', MSG), [], ''),
    ('1BE8_damage_originator', loc('mHullActorId = msg.GetOriginator();', MSG), [], ''),
    ('1BE8_default', loc('CActor::AcceptScriptMsg(mgr, msg);', MSG), [], ''),
]

# The turret's own code, in the demangled and the mangled spelling (gdb's $_caller_matches takes
# a regexp, and the name it compares can be either).
TURRET_SHOT = '(_ZN19CScriptPlayerTurret15FireTurretShot|CScriptPlayerTurret::FireTurretShot)'
TURRET_MSG = '(_ZN19CScriptPlayerTurret15AcceptScriptMsg|CScriptPlayerTurret::AcceptScriptMsg)'

FORCE = []
if os.environ.get('PT_FORCE_MP'):
    # The port's game mode object answers 'SNGL' for good (CGMSinglePlayer::GetGameModeType), so
    # the mode's type cannot be written: the force makes CStateManager::IsMultiplayer() *return*
    # true for the turret's own calls (the 'ret=1' set writes the return value when the function
    # returns, and only when the caller is the module's own code).
    FORCE.append(
        ('turret mp', 'CStateManager::IsMultiplayer(', [], ['ret=1'], 0,
         "CStateManager::IsMultiplayer() returns true while the turret's own code asks"))
if os.environ.get('PT_FORCE_DEAD'):
    FORCE.append(
        ('dead player', 'CPlayer::HealthInfo(', [],
         # gdb's $_caller_matches takes a basic regexp, so no alternation here.
         ['$dead = $_caller_matches("CScriptPlayerTurret") ? -1.0 : 0.0',
          '((CPlayerState*)((CPlayer*)$rdi)->mPlayerState)->mHealth.mHealth = '
          '$dead ? -1.0 : ((CPlayerState*)((CPlayer*)$rdi)->mPlayerState)->mHealth.mHealth'], 0,
         "the player's health is -1 while the turret's kSM_Activate case reads it (the guest's "
         'test at 0x1DE4 is (hp == 0) | (hp < 0)), so the case takes the dead-player arm'))
if os.environ.get('PT_FORCE_NOHPI'):
    FORCE.append(
        ('damage without health info', 'CScriptPlayerTurret::AcceptScriptMsg(', [],
         ['((CScriptMsg*)$rdx)->mSenderId = ((CScriptMsg*)$rdx)->m_id'], 0,
         "the kSM_Damage message's sender is the turret itself, whose HealthInfo() is null "
         "(CActor::HealthInfo returns nullptr), so the case takes the 'no health info' arm"))
