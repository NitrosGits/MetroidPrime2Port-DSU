# DigitalGuardian (module 14), dg32: the head without its armour model. The one head's data
# names an armour (data +0x0C), so fn_14_137A8's -1 arm (addis/cmplwi 0xffff at 0x137C0/0x137C4)
# and the empty-armour arms of fn_14_17A64 (0x17AAC) and fn_14_174F4 (0x17594) are forced: the id
# is set to -1 at fn_14_137A8's entry. Also the null arms the data never gives: fn_14_9DBC's
# 0xE68 token (beq at 0x9DDC; the area's data names a crippled scan) from the second call on,
# and the dtors' deletes of null pointers (fn_14_AB98: 0x123C at 0xAC2C, 0xE64 at 0xAE18, 0xE60
# at 0xAE2C; fn_14_17F64: 0xD70 at 0x18208), written at their entry (the objects leak). CLASS is
# the head's, so the body's sets cast $rdi themselves.
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg32_noarmour.py \
#        <out> 1baa96c2,09_cliff_temple_dark --driver 'python3 tools/drivers/dg32_head.py {port}' \
#        --fast --frames 900
CLASS = 'CDigitalGuardianHead'
SPEC = []
FORCE = [
    ('137A8 no armour', 'CDigitalGuardianHead::x137A8_(', [],
     ['$this->mData.headArmor = 0xffffffff'], 0, 'armour id -1'),
    ('9DBC no token', 'CDigitalGuardian::GetScannableObjectInfo(', [],
     ['((CDigitalGuardian*)$rdi)->xE68_.mPtr = 0'], 1, '0xE68 token null'),
    ('AB98 null pointers', 'CDigitalGuardian::~CDigitalGuardian(', [],
     ['((CDigitalGuardian*)$rdi)->x123C_ = 0', '((CDigitalGuardian*)$rdi)->xE60_ = 0', '((CDigitalGuardian*)$rdi)->xE64_ = 0'], 0, 'deletes of null'),
    ('17F64 null manager', 'CDigitalGuardianHead::~CDigitalGuardianHead(', [],
     ['$this->xD70_ = 0'], 0, 'delete of null'),
]
