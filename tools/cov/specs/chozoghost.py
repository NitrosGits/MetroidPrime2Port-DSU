# ChozoGhost (REL 8) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Shipped data has no CHOG placement and no ChozoGhost state
# machine, so every run is forced: the drivers `spawn CHOG` with a borrowed SpacePirate body and
# AFSM in Agon Wastes 08_sand_casualpirates and drive the states with statecall/trigcall/send.
# The spawn needs a knockbackRules write, so runs use chozoghost_force.py (which loads this SPEC);
# its header has the command line (docs/module-analysis/ChozoGhost.md, "Coverage").
CLASS = 'CChozoGhost'
SPEC = [
    ('3D58 AcceptScriptMsg', 'CChozoGhost::AcceptScriptMsg(', [], 'max=4000'),
    ('3C68 Touch', 'CChozoGhost::Touch(', [], 'max=4000'),
    ('4468 PreRender', 'CChozoGhost::PreRender(', [], 'max=400'),
    ('3E28 FindBestAnchor', 'CChozoGhost::FindBestAnchor(', [], 'max=400'),
    ('15D8 DoUserAnimEvent', 'CChozoGhost::DoUserAnimEvent(', ['$rdx'], 'max=400'),
    ('15D0 KnockBack', 'CChozoGhost::KnockBack(', [], 'max=400'),
]
