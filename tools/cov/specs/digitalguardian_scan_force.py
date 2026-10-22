# DigitalGuardian (module 14) forcing spec for tools/cov/force.sh (run with the coverage build:
# the arms are counted by gcov, tools/cov/gcov_report.py). fn_14_9ABC's beacon test at 0x9BEC /
# 0x9BF8 has two arms the area 53 fight does not take while the scan display is up:
#   * 0x128C bit 0x08 set: only "CLOS" sets it, and CLOS also deletes the guardian;
#   * no beacon model (0x11EC empty): fn_14_6864 fills it whenever the data block's
#     transmission beacon (mTransmissionBeacon) is a valid id, and the one guardian's data names one.
# Run: tools/cov/force.sh tools/cov/specs/digitalguardian_scan_force.py <out> 1baa96c2,09_cliff_temple_dark
#       --con tools/cov/specs/digitalguardian_scan_force.con --fast --frames 750
CLASS = 'CDigitalGuardian'
FORCE = [
    ('9ABC ScanVisorRender: no beacon model (forced)', 'CDigitalGuardian::ScanVisorRender(',
     [], ['$this->x11EC_.m_valid = 0'], 4,
     "the beacon model at 0x11EC is empty, so the test at 0x9BF8 skips the beacon"),
    ('9ABC ScanVisorRender: 0x128C bit 0x08 (forced)', 'CDigitalGuardian::ScanVisorRender(',
     [], ['$this->x128C_ |= 8'], 10,
     "0x128C bit 0x08 is set (CLOS), so the test at 0x9BEC skips the beacon"),
]
