# MediumIng (module 41) slice 1 spec for tools/cov/run.sh; labels are the retail offsets.
# The Ings are in Sky Temple Grounds area 46 (3bfa3eff,02_temple_exploration_dark: u0417, u0454 and
# eight inactive tentacles); the driver is tools/cov/specs/mediuming.con:
#   tools/cov/run.sh tools/cov/specs/mediuming.py <out> 3bfa3eff,02_temple_exploration_dark \
#     --con tools/cov/specs/mediuming.con --fast --frames 1000
CLASS = 'CMediumIng'
SPEC = [
    ('7804 Think', 'CMediumIng::Think(', ['$this->mC1', '$this->mC0', '$this->mMistState'], 'max=4000'),
    ('3274 UpdateTimers', 'CMediumIng::UpdateTimers(', ['$this->mC2'], 'max=4000'),
    ('4644 ScanProjectileThreats', 'CMediumIng::ScanProjectileThreats(', ['$this->mMistState'], 'max=4000'),
    ('6078 UpdateMistState', 'CMediumIng::UpdateMistState(', ['$this->mMistState', '$this->mC5'], 'max=4000'),
    ('5DC8 IsMistDecayed', 'CMediumIng::IsMistDecayed(', [], 'ret max=4000'),
    ('49CC SetMistCollision', 'CMediumIng::SetMistCollision(', ['$rsi & 0xff'], ''),
    ('5E0C IsBlocked', 'CMediumIng::IsBlocked(', [], 'ret'),
    ('1680 UpdateMistEffect', 'CMediumIng::UpdateMistEffect(', ['$this->mC0'], 'max=4000'),
    ('198C CreateMistEffect', 'CMediumIng::CreateMistEffect(', [], ''),
    ('155C UpdateCurrentSafeZone', 'CMediumIng::UpdateCurrentSafeZone(', [], ''),
    ('7478 Listen', 'CMediumIng::Listen(', ['$rcx', '$this->mAlive'], 'ret'),
    ('2024 CalculateSeparation', 'CMediumIng::CalculateSeparation(', [], ''),
    ('79C8 AcceptScriptMsg', 'CMediumIng::AcceptScriptMsg(', ['((CScriptMsg*)$rdx)->m_msg'], 'max=400'),
    ('75A4 PreRender', 'CMediumIng::PreRender(', ['$this->mC1', '$this->mC3'], 'max=4000'),
    ('74F8 Render', 'CMediumIng::Render(', [], 'max=4000'),
    ('761C Touch', 'CMediumIng::Touch(', ['$this->mC5', '$this->mD2', '$this->mD3'], 'max=4000'),
    ('1B18 Death', 'CMediumIng::Death(', [], ''),
    ('1B04 SetFlagE1', 'CMediumIng::SetFlagE1(', [], ''),
    ('A4 CanBeShot', 'CMediumIng::CanBeShot(', [], 'ret max=400'),
    ('T 9F84 Think', 'CMediumIngTentacle::Think(', ['((CMediumIngTentacle*)$rdi)->mExtending', '((CMediumIngTentacle*)$rdi)->mRetracting'], 'max=4000'),
    ('T 94C8 PreRender', 'CMediumIngTentacle::PreRender(', ['((CMediumIngTentacle*)$rdi)->mExtending'], 'max=4000'),
    ('T 93FC GetTouchBounds', 'CMediumIngTentacle::GetTouchBounds(', [], 'max=4000'),
    ('T 94A4 GetProgress', 'CMediumIngTentacle::GetProgress(', [], 'max=4000'),
    ('T 91B0 Touch', 'CMediumIngTentacle::Touch(', ['((CMediumIngTentacle*)$rdi)->mCanDamage'], ''),
    ('T 9A34 Extend', 'CMediumIngTentacle::Extend(', [], ''),
    ('T 9CE0 Retract', 'CMediumIngTentacle::Retract(', [], ''),
    ('T 9ED0 UpdateShake', 'CMediumIngTentacle::UpdateShake(', [], ''),
]
