# DigitalGuardian (module 14), dg25: the head codes DestroyActiveSplitters (fn_14_14C24) with the
# Splitter's fn_75_5A54 / fn_75_E660, EnableValidTarget (14604), DisableValidTarget (145A4),
# ResetGravityRepulsorHP (14248), EjectPlayerFromBombPit (14058), StartVocalRecoverReaction (13C1C).
# Run: tools/cov/run.sh tools/cov/specs/digitalguardian_dg25.py <out> 1baa96c2,09_cliff_temple_dark
#   --con tools/cov/specs/digitalguardian_dg25.con --fast --frames 450
CLASS = 'CDigitalGuardianHead'
SPEC = [
    ('14C24 DestroyActiveSplitters', 'CDigitalGuardianHead::DestroyActiveSplitters(', [], 'calls'),
    ('75_5A54 PortAutoDestruct', 'CSplitterCommandModule::PortAutoDestruct(', ['$xmm0.v4_float[0]'], ''),
    ('75_E660 PortStartDeathTimer', 'CSplitterMainChassis::PortStartDeathTimer(',
     ['$xmm0.v4_float[0]', '$rdi->mFlagsD41 & 1'], ''),
    ('14058 EjectPlayerFromBombPit', 'CDigitalGuardianHead::EjectPlayerFromBombPit(', [], 'calls'),
    ('14604 EnableValidTarget', 'CDigitalGuardianHead::EnableValidTarget(', [], ''),
    ('145A4 DisableValidTarget', 'CDigitalGuardianHead::DisableValidTarget(', [], ''),
    ('14248 ResetGravityRepulsorHP', 'CDigitalGuardianHead::ResetGravityRepulsorHP(', [], ''),
    ('13C1C StartVocalRecoverReaction', 'CDigitalGuardianHead::StartVocalRecoverReaction(', [], 'calls'),
]
