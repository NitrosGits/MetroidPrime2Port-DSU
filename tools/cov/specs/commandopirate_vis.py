# CommandoPirate (module 9) spec: the CVisualKnowledge at guest this+0xAB0 (Think's Update at
# 0xC9F8, SelectTarget's SetVisualTarget at 0xAB68) and its two readers (HasLineOfSight, 0xBAEC;
# SetTargetDest, 0xAA94) and the shot's clear (0x5B70). No forcing.
#
# Run: tools/cov/run.sh tools/cov/specs/commandopirate_vis.py <out>
#      3bfa3eff,05_temple_federationsite_dark --driver 'python3 tools/drivers/commandopirate_core.py {port}'
CLASS = 'CCommandoPirate'
L = 'loc:PortCCommandoPirate.cpp:'
SPEC = [
    ('0xC944 Think', 'CCommandoPirate::Think(',
     ['$this->mCollisionShield.mPtr == 0', '$this->mCollisionBlade.mPtr == 0',
      '$this->xbfc_ragDoll.mPtr == 0'], 'calls max=20000'),
    ('0xC9F8 Think: VK Update', L + '528',
     ['$this->mVisualKnowledge.x20_target', '$this->mVisualKnowledge.x38_visible'], 'max=20000'),
    ('0xAB08 SelectTarget', 'CCommandoPirate::SelectTarget(', ['$rdx'], 'calls max=3000'),
    ('0xBAEC HasLineOfSight', 'CCommandoPirate::HasLineOfSight(', [], 'ret max=3000'),
    ('0xAA20 SetTargetDest', 'CCommandoPirate::SetTargetDest(',
     ['$rdx', '$this->mVisualKnowledge.x38_visible', '($this->xc64_ & 0x40) != 0'], 'calls max=3000'),
    ('0x5B70 shot: VK clear', L + '857', [], 'max=3000'),
]
