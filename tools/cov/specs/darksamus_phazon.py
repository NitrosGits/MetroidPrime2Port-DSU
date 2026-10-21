# DarkSamus phazon enrage helpers (fn_10_7F98 tree, fn_10_452C), a forcing run on the coverage
# build: the phazon states run once each by the console's statecall (darksamus_phazon.con), and
# two arms the fight cannot reach are written at a state's entry, keyed by the statecall's dt of
# 0.0123 (0.0124 below): the absorbed phazon shots at the gun tweak's maximum (fn_10_7F98's 0xFCC drain), restored
# at the next call, and an emitter handle in 0xF60 for fn_10_452C to remove.
# Run: BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darksamus_phazon.py <out>
#      "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_phazon.con --fast --frames 1500
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DarkSamus
CLASS = 'CDarkSamus'
KEY = '($xmm0.v4_float[0] > 0.0122 && $xmm0.v4_float[0] < 0.0124)'
AB = '((CStateManager*)$rsi)->mPlayers[0]->mGun.mPtr->mAbsorbedPhazonShots'
AB_SET = ('({k} ? ($_isvoid($ab) ? ($ab = {a}, {a} = 100000) : 0) : '
          '(($_isvoid($ab) || !$_isvoid($abr)) ? 0 : ({a} = $ab, $abr = 1)))').format(k=KEY, a=AB)
# 0.0124: gpTweakPlayerGun null for one PhazonMissiles Update (fn_10_7F98's tweak check), put back
# at RunMeleeAttackCmd, the state's next call.
KEY2 = '($xmm0.v4_float[0] > 0.01235 && $xmm0.v4_float[0] < 0.01245)'
TG_SET = '({k} ? ($_isvoid($tg) ? ($tg = gpTweakPlayerGun, gpTweakPlayerGun = 0) : 0) : 0)'.format(k=KEY2)
TG_RESTORE = '(($_isvoid($tg) || !$_isvoid($tgr)) ? 0 : (gpTweakPlayerGun = $tg, $tgr = 1))'
SFX_SET = '({k} ? ($this->mGuestF60.mID = 0x7fffffff) : 0)'.format(k=KEY)
FORCE = [
    ('7F98 absorbed', 'CDarkSamus::UpdatePhazonEnrage(', ['$rdx'], [AB_SET], 0,
     'absorbed phazon shots 100000 for the dt 0.0123 call'),
    ('81D4 sfx', 'CDarkSamus::PhazonAttack(', ['$rdx'], [SFX_SET], 0,
     '0xF60 a handle no sound has, for the dt 0.0123 Activate'),
    ('7F98 tweak', 'CDarkSamus::PhazonMissiles(', ['$rdx'], [TG_SET], 0,
     'gpTweakPlayerGun null for the dt 0.0124 Update'),
    ('7F98 tweak back', 'CDarkSamus::RunMeleeAttackCmd(', [], [TG_RESTORE], 0,
     'gpTweakPlayerGun put back'),
]
