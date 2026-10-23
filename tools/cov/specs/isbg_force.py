# IngSpiderballGuardian (module 35) forcing spec for tools/cov/force.sh: the guardian resists no
# player weapon the console can fire, so kSM_ResistedDamage from a CWeapon (fn_35_3214, the
# ApplyStunDamage call) never arrives. Every kSM_Damage it receives is turned into
# kSM_ResistedDamage at AcceptScriptMsg's entry; the sender stays the real projectile (`shoot`), so
# ApplyStunDamage, the Damaged/Stunned states, StunnedReaction and the stun timer run as retail.
#   BUILD=$PWD/build-port-cov SECS=1800 tools/cov/force.sh tools/cov/specs/isbg_force.py <out> \
#     1baa96c2,06_cliff_spiderballguardian --con tools/cov/specs/isbg_stun.con --fast --frames 8000
CLASS = 'CIngSpiderballGuardian'



def counter(name):
    return '%s = $_isvoid(%s) ? 1 : %s + 1' % (name, name, name)


MSG = '((CScriptMsg*)$rdx)->m_msg'

FORCE = [
    ('3214 AcceptScriptMsg: Damage -> ResistedDamage (forced)', 'CIngSpiderballGuardian::AcceptScriptMsg(',
     [MSG],
     ['%s = %s == kSM_Damage ? kSM_ResistedDamage : %s' % (MSG, MSG, MSG)], 0,
     'kSM_Damage rewritten to kSM_ResistedDamage (same sender)'),
    # fn_35_F8C: the stun health (40) outlasts what the beam's resisted damage takes off; on hits
    # 4 mod 10 it is set to 0.1 so the hit stuns (the mStunHealth <= 0 arm).
    ('0F8C ApplyStunDamage: stun health 0.1 (forced)', 'CIngSpiderballGuardian::ApplyStunDamage(',
     ['$this->mState', '$this->mStunHealth'],
     [counter('$isd_n'), '$this->mStunHealth = $isd_n % 10 == 4 ? 0.1 : $this->mStunHealth'], 0,
     'mStunHealth = 0.1 on hits 4 mod 10'),
]
