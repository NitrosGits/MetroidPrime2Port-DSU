# dol1: CDamageInfo::WithNoImmunity (DOL fn_800B5F8C) through CParasite::Think's contact damage
# (x944_ > 0, set only for kPT_IceZoomer && mVulnerable). Driver tools/drivers/dol1_parasite.con in
# 42b935e4,01_sand_entrance_dark (layers 1-3). The area's Parasites are mType 10, not IceZoomers
# (forcing mType 3 crashes: no collision actors), so tools/cov/force.sh with
# COV_GDB_SCRIPT=tools/cov/cov_force.py sets x944_ (the timer that arm sets) directly.
CLASS = 'CParasite'
D = '((CDamageInfo*)$r8)'
S = '((CDamageInfo*)$rsi)'
SPEC = [
    ('CParasite Think', 'CParasite::Think(', ['$this->mType', '$this->mVulnerable', '$this->x944_ > 0'], 'max=4000'),
    ('800B5F8C WithNoImmunity', 'CDamageInfo::WithNoImmunity(',
     # the source (x928_): no-immunity, radius flag, damage, radius damage, sfx id
     [S + '->mNoImmunity', S + '->mApplyRadiusDamage', '(int)(%s->mDamage*100)' % S,
      '(int)(%s->mRadiusDamageAmount*100)' % S, S + '->mDamageSfxId'], 'max=200'),
    # damager, damagee, owner, damage info (r8): no-immunity, radius flag, damage, radius damage
    ('CStateManager ApplyDamage', 'CStateManager::ApplyDamage(',
     ['$rdx', D + '->mNoImmunity', D + '->mApplyRadiusDamage', '(int)(%s->mDamage*100)' % D,
      '(int)(%s->mRadiusDamageAmount*100)' % D], 'max=400'),
]
FORCE = [
    ('CParasite Think: x944_', 'CParasite::Think(', ['$this->x944_ > 0'],
     ['$this->x944_ = 0.5'], 300, 'x944_ = 0.5 (the IceZoomer contact timer): WithNoImmunity arm'),
]
