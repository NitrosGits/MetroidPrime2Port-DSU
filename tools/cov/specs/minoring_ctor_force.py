# MinorIng lane ing31: both arms of the three tests of the constructor fn_44_9D58 (tools/cov/force.sh
# with COV_GDB_SCRIPT=tools/cov/cov_force.py, any build-port-link binary):
#   0x9E00 cmplwi r0,0xffff / 0x9E04 beq   the normal hit effect's id (0x814) is set
#   0x9F7C cmplwi r0,0xffff / 0x9F80 beq   the heavy hit effect's id (0x818) is set
#   0xA3F0 cmplwi r0,0xff  / 0xA3F4 beq    the lock-on locator was found
# The host's OptionalPart (PortCMinorIng.cpp:288) is both id tests; the constructor count $mi31_c
# and the call count $mi31_k pick which of the two ids reads as no asset ((c + k) even), so odd
# constructors force the normal one and even ones the heavy one. Even constructors also see no
# lock-on locator (line 408). Lines are the built source's.
CLASS = 'CMinorIng'
SPEC = [
    ('9D58 hit effect id unset (0x9E04/0x9F80)', 'loc:PortCMinorIng.cpp:288', ['id == 0xffffffff'], ''),
    ('9D58 lock-on locator unset (0xA3F4)', 'loc:PortCMinorIng.cpp:408',
     ['this->mLockOnLocator.mId == 0xff'], ''),
]
FORCE = [
    ('9D58 entry', 'CMinorIng::CMinorIng(', [], ['$mi31_c = $_isvoid($mi31_c) ? 1 : $mi31_c + 1', '$mi31_k = 0'], 0,
     'the constructor count'),
    ('9D58 part', 'loc:PortCMinorIng.cpp:288', ['$mi31_c', '$mi31_k'],
     ['$mi31_k = $mi31_k + 1', 'id = (($mi31_c + $mi31_k) % 2 == 0) ? 0xffffffff : id'], 0,
     'odd constructors: the normal hit effect has no asset; even ones: the heavy one'),
    ('9D58 lock-on', 'loc:PortCMinorIng.cpp:408', ['$mi31_c'],
     ['this->mLockOnLocator.mId = ($mi31_c % 2 == 0) ? 0xff : this->mLockOnLocator.mId'], 0,
     'even constructors: no lock-on locator'),
]
