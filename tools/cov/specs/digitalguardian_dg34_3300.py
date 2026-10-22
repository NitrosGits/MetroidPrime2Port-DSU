# DigitalGuardian (module 14), dg34: fn_14_3300's span arms (both events found: fcmpo at 0x3430, then the
# time test at 0x3470). Data-dead: only A_into_vortex_fast (meta 30, char 29) has a TakeOff and a Landing
# (0.767, 1.233), and char 29 is in no PAS state (it plays only as the transition into the vortex), so
# the body controller's current anim (0x58C) is never 29. Forced: the current anim set to 29 at entry
# (every hit), so the lookups find both events (span 0.466 > 0); from the second hit on the span is also
# negative (the landing time written as 0), so the max_val(0, span) gives 0 and the zero-drift arm (0x34A4) runs.
# Run: BUILD=<cov build> tools/cov/force.sh tools/cov/specs/digitalguardian_dg34_3300.py <out>
#        1baa96c2,09_cliff_temple_dark --con tools/cov/specs/digitalguardian_dg34_force.con --fast --frames 900
CLASS = 'CDigitalGuardian'
SPEC = []
FORCE = [
    ('3300 anim 29', 'CDigitalGuardian::x3300_(', [], ['$this->mBodyController.mPtr->mCurAnim = 29'], 0,
     'the current anim A_into_vortex_fast (char 29)'),
    ('3300 landing 0', 'loc:PortCDigitalGuardian.cpp:844', [], ['landing.mTime = 0'], 1,
     'the landing time 0, below the take-off'),
]
