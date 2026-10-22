# DigitalGuardian (module 14), dg31: the echo targets without their emitter effect (x784_) and
# echo emitter (CActor's single_ptr): both are data-present for the three area-53 targets
# (echoTargetEmitterFx set, mIsEchoEmitter set), so fn_14_1A1F8 (0x1A3D8), fn_14_1A430 (0x1A5C0),
# fn_14_1A168 and fn_14_19B70 (0x19B9C) never see them null.  Also fn_14_1A430's kill without a sound.  After 300 Thinks (about 100 frames
# each) both are nulled and CActor's echo-emitter bit cleared (forced; CActor::Think
# reads the emitter under that bit; the objects leak, the run is a coverage run).
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg31_nulls.py \
#        <out> 1baa96c2,09_cliff_temple_dark --driver 'python3 tools/drivers/dg31_echo.py {port}' \
#        --fast --frames 800
CLASS = 'CDigitalGuardianEchoTarget'
SPEC = []
FORCE = [
    ('1A1F8 nulls', 'CDigitalGuardianEchoTarget::Think(', [],
     ['$this->x784_ = 0', '$this->mEchoEmitter.mPtr = 0', '$this->mEchoEmitterEnabled = 0'], 300, 'effect and echo emitter null'),
    # fn_14_1A430's kill with no transmit sound (0x1A504's skip): every target that a kill reaches
    # has its sound playing (the driver's kills land on extended targets), so the handle is cleared
    # at each message (forced; the sound's emitter leaks until the run ends)
    ('1A430 no sound', 'CDigitalGuardianEchoTarget::AcceptScriptMsg(', [],
     ['$this->x788_.mID = 0'], 0, 'transmit handle null'),
]
