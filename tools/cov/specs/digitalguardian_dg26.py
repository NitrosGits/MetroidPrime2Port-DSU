# DigitalGuardian (module 14), dg26: the head's triggers IsBurstOver (fn_14_173B8), UseMachineGun
# (17370), UseLockOnMissiles (1735C), UseAnnihilatorCharge (17338), AllSplittersDead (17298),
# IsAntennaTransmitting (17244), AllEchoTargetsDestroyed (171D8), BombPitDestroyed (17198) with
# their results, and the helpers RetractEchoTargets (14DA0), fn_14_11FEC, 12B04, F2B8, 13248.
# Run: BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/digitalguardian_dg26.py <out>
#   1baa96c2,09_cliff_temple_dark --driver 'python3 tools/drivers/dg26_slots.py {port} a' --fast --frames 2400
CLASS = 'CDigitalGuardianHead'
SPEC = [
    ('173B8 IsBurstOver', 'CDigitalGuardianHead::IsBurstOver(', ['((CDigitalGuardianHead*)$rdi)->x10D8_'], 'ret'),
    ('17370 UseMachineGun', 'CDigitalGuardianHead::UseMachineGun(', [], 'ret'),
    ('1735C UseLockOnMissiles', 'CDigitalGuardianHead::UseLockOnMissiles(', [], 'ret'),
    ('17338 UseAnnihilatorCharge', 'CDigitalGuardianHead::UseAnnihilatorCharge(', [], 'ret'),
    ('17298 AllSplittersDead', 'CDigitalGuardianHead::AllSplittersDead(', [], 'ret calls'),
    ('17244 IsAntennaTransmitting', 'CDigitalGuardianHead::IsAntennaTransmitting(', [], 'ret calls'),
    ('171D8 AllEchoTargetsDestroyed', 'CDigitalGuardianHead::AllEchoTargetsDestroyed(',
     ['((CDigitalGuardianHead*)$rdi)->xFE0_'], 'ret'),
    ('17198 BombPitDestroyed', 'CDigitalGuardianHead::BombPitDestroyed(', [], 'ret'),
    ('14DA0 RetractEchoTargets', 'CDigitalGuardianHead::RetractEchoTargets(', [], 'calls'),
    ('11FEC echo target destroyed', 'CDigitalGuardianHead::x11FEC_(', ['((CDigitalGuardianHead*)$rdi)->xFE0_'], 'calls'),
    ('12B04 bomb slot destroyed', 'CDigitalGuardianHead::x12B04_(', ['$rdx'], 'calls'),
    ('F2B8 waypoint', 'CDigitalGuardianHead::xF2B8_(', [], 'calls'),
    ('13248 ball index', 'CDigitalGuardianHead::x13248_(', ['$rsi & 0xffff'], ''),
]
