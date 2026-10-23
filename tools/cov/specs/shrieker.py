# Shrieker (REL 69) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Torvus Bog 02_swamp_greatbridge layer 2 holds Shriekers;
# tools/drivers/shrieker_greatbridge.con wakes one, walks the state table, damages and kills it:
#   BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/shrieker.py <out> 3dfd2249,02_swamp_greatbridge \
#     --con tools/drivers/shrieker_greatbridge.con --fast --frames <n>
#   then tools/cov/gcov_report.py build-port-cov <out>/gcov --module Shrieker
CLASS = 'CShrieker'
SPEC = [
    ('51E0 Think', 'CShrieker::Think(', [], 'max=4000'),
    ('5988 AcceptScriptMsg', 'CShrieker::AcceptScriptMsg(', [], 'max=4000'),
    ('333C DoUserAnimEvent', 'CShrieker::DoUserAnimEvent(', ['$rdx'], 'max=400'),
    ('1C2C DeflectMissiles', 'CShrieker::DeflectMissiles(', [], 'max=400'),
    ('24F4 ApplyMeleeDamage', 'CShrieker::ApplyMeleeDamage(', [], 'max=400'),
    ('2A50 InMaxRange', 'CShrieker::InMaxRange(', [], 'ret max=400'),
    ('2B20 ShouldDodge', 'CShrieker::ShouldDodge(', [], 'ret max=400'),
    ('2D64 ShouldMelee', 'CShrieker::ShouldMelee(', [], 'ret max=400'),
    ('2F08 ShouldFire', 'CShrieker::ShouldFire(', [], 'ret max=400'),
    ('4DA8 KnockBack', 'CShrieker::KnockBack(', [], 'max=400'),
    ('5554 ReactToDamage', 'CShrieker::ReactToDamage(', [], 'max=400'),
]
