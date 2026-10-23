# DestructibleBarrier (REL 13) spec for tools/cov/run.sh; arms come from the coverage build
# (gcov_report.py), the breakpoints only count calls. No shipped area places a DBAR: every run spawns
# one with the console (tools/drivers/destructiblebarrier_forced.con):
#   BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/destructiblebarrier.py <out> \
#     42b935e4,08_sand_casualpirates --con tools/drivers/destructiblebarrier_forced.con --frames <n>
#   then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DestructibleBarrier
CLASS = 'CScriptDestructibleBarrier'
SPEC = [
    ('Think', 'CScriptDestructibleBarrier::Think(', [], 'max=4000'),
    ('AcceptScriptMsg', 'CScriptDestructibleBarrier::AcceptScriptMsg(', [], 'max=4000'),
    ('TakeDamage', 'CScriptDestructibleBarrier::TakeDamage(', [], 'max=400'),
    ('OnChunkDestroyed', 'CScriptDestructibleBarrier::OnChunkDestroyed(', [], 'max=400'),
    ('SplitOffTop', 'CBarrierChunkGrid::SplitOffTop(', [], 'max=400'),
]
