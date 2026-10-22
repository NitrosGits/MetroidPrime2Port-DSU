# Coverage spec: fn_79_4F88 (SendDecrement).
#
# Nothing in the module calls the retail's fn_79_4F88 (the only reference to 0x4F88 in the
# retail's .text is its own definition), so the body is forced from gdb: at the boss's Think,
# once the area-loaded message has filled mLightChain (the light chain actor's uid), the port's
# own SendDecrement is called on the live boss - once with the chain there (the retail's
# "found" side) and once with mLightChain forced to kInvalidUniqueId (its "not found" side).
import gdb

CLASS = 'CSwampBossStage2'
SPEC = [
    ('4F88', 'CSwampBossStage2::SendDecrement(', ['$this->mLightChain.value'], 'max=50'),
]


class Force(gdb.Breakpoint):
    """Flip the light chain's uid so the body's other arm runs (no gdb call: that hangs the
    game)."""

    def __init__(self, where):
        super().__init__(where, internal=True)
        self.n = 0

    def stop(self):
        try:
            chain = int(gdb.parse_and_eval('this->mLightChain.value'))
        except gdb.error as e:
            if self.n < 3:
                print('[force] mLightChain unreadable: %s' % e)
            self.n += 1
            return False
        if chain == 0xffff:
            return False  # the area-loaded message has not filled mLightChain yet
        self.n += 1
        if self.n == 50:
            print('[force] mLightChain -> 0xffff')
            gdb.execute('set var this->mLightChain.value = 0xffff')
        elif self.n == 100:
            print('[force] mLightChain -> %d' % chain)
            gdb.execute('set var this->mLightChain.value = %d' % chain)
        return False


Force('CSwampBossStage2::Think')
