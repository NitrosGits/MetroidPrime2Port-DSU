# DigitalGuardian (module 14) scan path: fn_14_9CA4 GetScanVisorRenderBounds (CScanDisplay::
# PrepareScanDisplay) and fn_14_9ABC ScanVisorRender (CScanDisplay::Draw), reached with the
# console `scan` (tools/cov/specs/digitalguardian_scan.con).
# Run: tools/cov/run.sh tools/cov/specs/digitalguardian_scan.py <out> 1baa96c2,09_cliff_temple_dark
#       --con tools/cov/specs/digitalguardian_scan.con
CLASS = 'CDigitalGuardian'
SPEC = [
    ('SCAN state', 'CPlayer::SetScanningState(', ['$rsi'], ''),
    ('SCAN prepare', 'CScanDisplay::PrepareScanDisplay(', [], ''),
    ('9CA4 GetScanVisorRenderBounds', 'CDigitalGuardian::GetScanVisorRenderBounds(', [], 'calls'),
    ('9ABC ScanVisorRender', 'CDigitalGuardian::ScanVisorRender(', [
        '($this->x128C_ & 8) != 0', '$this->x1264_ > 0'], 'calls max=2000'),
    ('1570 the model box', 'CDigitalGuardian::x1570_(', [], ''),
    ('HEAD 9CA4 bounds', 'CDigitalGuardianHead::GetScanVisorRenderBounds(', [], ''),
    ('HEAD 9ABC render', 'CDigitalGuardianHead::ScanVisorRender(', [], 'max=2000'),
]
