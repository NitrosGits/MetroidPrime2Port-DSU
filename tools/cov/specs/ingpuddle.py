# IngPuddle (module 32) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Agon Wastes 06_sand_alamofront_dark layer 2 holds the disc's
# seven IngPuddles (e08260040/44/c1/c2/13f/140/142):
#   BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/ingpuddle.py <out> \
#     42b935e4,06_sand_alamofront_dark --con tools/cov/specs/ingpuddle.con --fast --frames 4000
CLASS = 'CIngPuddle'
SPEC = [
    ('1B14 Think', 'CIngPuddle::Think(', [], 'max=8000'),
    ('182C AcceptScriptMsg', 'CIngPuddle::AcceptScriptMsg(', [], 'max=4000'),
    ('688 HandleDamage', 'CIngPuddle::HandleDamage(', [], 'max=400'),
    ('1154 Patrol', 'CIngPuddle::Patrol(', ['$rdx'], 'max=8000'),
    ('1628 Idle', 'CIngPuddle::Idle(', ['$rdx'], 'max=8000'),
    ('1490 Dead', 'CIngPuddle::Dead(', ['$rdx'], 'max=400'),
]
