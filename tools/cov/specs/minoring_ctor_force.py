# MinorIng lane ing31: both arms of the three tests of the constructor fn_44_9D58 (tools/cov/force.sh
# with COV_GDB_SCRIPT=tools/cov/cov_force.py, any build-port-link binary):
#   0x9E00 cmplwi r0,0xffff / 0x9E04 beq   the normal hit effect's id (0x814) is set
#   0x9F7C cmplwi r0,0xffff / 0x9F80 beq   the heavy hit effect's id (0x818) is set
#   0xA3F0 cmplwi r0,0xff  / 0xA3F4 beq    the lock-on locator was found
# The host's OptionalPart (PortCMinorIng.cpp:288) is both id tests, called normal then heavy per
# constructor; its call count $mi31_p forces the normal id of odd constructors (p % 4 == 1) and the
# heavy id of even ones (p % 4 == 0) to no asset. The lock-on test (line 408) is forced to "not
# found" on every second hit ($mi31_l even), so the odd hits show the natural value. (A break at
# the constructor's entry fires twice per object, the complete and the base constructor, so the
# counts are kept at the tested lines.) Lines are the built source's.
CLASS = 'CMinorIng'
SPEC = [
    ('9D58 hit effect id unset (0x9E04/0x9F80)', 'loc:PortCMinorIng.cpp:288', ['id == 0xffffffff'], ''),
    ('9D58 lock-on locator unset (0xA3F4)', 'loc:PortCMinorIng.cpp:408',
     ['this->mLockOnLocator.mId == 0xff'], ''),
]
FORCE = [
    ('9D58 part', 'loc:PortCMinorIng.cpp:288', ['$mi31_p'],
     ['$mi31_p = $_isvoid($mi31_p) ? 1 : $mi31_p + 1',
      'id = ($mi31_p % 4 == 1 || $mi31_p % 4 == 0) ? 0xffffffff : id'], 0,
     'odd constructors: the normal hit effect has no asset; even ones: the heavy one'),
    ('9D58 lock-on', 'loc:PortCMinorIng.cpp:408', ['$mi31_l'],
     ['$mi31_l = $_isvoid($mi31_l) ? 1 : $mi31_l + 1',
      'this->mLockOnLocator.mId = ($mi31_l % 2 == 0) ? 0xff : this->mLockOnLocator.mId'], 0,
     'every second constructor: no lock-on locator'),
]
