# Splitter (module 75) split26 lane, forced: the arms of the beam effect's PreRender (fn_75_10F1C)
# split26's natural run (tools/cov/specs/splitter_split26.py) left, at the return of a callee
# while PreRender runs (force_ret.py FORCE_INNER): a null near actor (0x11440; the near list
# only holds live actors), an actor without projected shadows (0x1145C; every CanDrawStatic
# actor near the beam in 914f1381 takes them, so one in 150 looked up loses the flag for good)
# and a box in the frustum that misses the near box (0x11488).  MakeAreaSet / GetVisSet count
# whether any loaded area has a PVS (the 0x1114C arm).
#
#   COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py BUILD=$PWD/build-port-cov SECS=2400 \
#     tools/cov/force.sh tools/cov/specs/splitter_split26_force.py <out> 1baa96c2,914f1381 \
#     --con tools/cov/specs/splitter_split23.con --fast --frames 4000
CLASS = 'CSplitterBeamEffect'
_PR = 'CSplitterBeamEffect::PreRender('
FORCE_RET = []
FORCE_INNER = [
    ('11440 null actor', _PR, 'CStateManager::GetObjectById(', 500, {3: [('$rax', '0')]},
     'the near actor looked up is null'),
    ('1145C no projected shadow', _PR, 'CStateManager::GetObjectById(', 150,
     {7: [('((CActor*)$rax)->mTakesProjectedShadow', '0')]}, 'the actor takes no projected shadow'),
    ('11488 misses the near box', _PR, 'CAABox::DoBoundsOverlap(', 5, {2: [('$rax', '0')]},
     'the other bounds miss the near box'),
]
SPEC = [
    ('PVS MakeAreaSet', 'CPVSAreaSet::MakeAreaSet(', [], 'max=400'),
    ('PVS GetVisSet', 'CPVSVisOctree::GetVisSet(', [], 'max=400'),
]
