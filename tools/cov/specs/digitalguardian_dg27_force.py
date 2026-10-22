# DigitalGuardian (module 14), dg27: the head arms dg26 left unforced.
# - fn_14_14470 SelectDeathHeadAttack with firstShotType 4 (shipped 0): x10DC_ set and the type 4
#   at every entry, so the first-shot test fails into the weighted pick.
# - fn_14_13248 / 12B04: the first bomb-slot kill runs with no effect token (xF14_ unset at entry,
#   set back at the sound after the effect block); the second hit's id becomes 0xffff (no ball),
#   so 13248 answers -1 and 12B04 returns at once. No per-frame breakpoint: dg26's Think and
#   AcceptScriptMsg ones slowed the run until the ball left its slot.
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg27_force.py \
#        <out> 1baa96c2,09_cliff_temple_dark --driver 'python3 tools/drivers/dg26_slots.py {port} a' \
#        --fast --frames 2400
CLASS = 'CDigitalGuardianHead'
SPEC = []
FORCE = [
    ('14470 firstShotType 4', 'CDigitalGuardianHead::SelectDeathHeadAttack(', [],
     ['$this->x10DC_ = 1', '$this->mData.digitalGuardianHeadStruct_0x48b46e55.firstShotType = 4'], 0,
     'x10DC_ 1, firstShotType 4'),
    ('12B04 count', 'CDigitalGuardianHead::SetupStateMachine(', [], ['$dg27_n = 0'], 0,
     'zeroes the gdb hit count'),
    ('12B04 no effect token', 'CDigitalGuardianHead::x12B04_(', [],
     ['$dg27_n = $dg27_n + 1', '$dg27_h = $rdi',
      '$this->xF14_.m_valid = $dg27_n == 1 ? 0 : $this->xF14_.m_valid'], 0,
     'clears xF14_.m_valid on the first call'),
    ('12B04 token back', 'loc:PortCDigitalGuardianHead.cpp:2870', [],
     ['((CDigitalGuardianHead*)$dg27_h)->xF14_.m_valid = 1'], 0, 'sets xF14_.m_valid'),
    ('13248 no ball', 'CDigitalGuardianHead::x13248_(', [],
     ['$rsi = $dg27_n >= 1 ? 0xffff : $rsi'], 0, 'the id (si) becomes 0xffff after the first kill'),
]
