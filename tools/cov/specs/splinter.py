# Splinter (REL 74) on the coverage build: the gcov profile is the evidence; these breakpoints only
# count a few entries. Drivers: tools/cov/specs/splinter_*.con.
# Run: BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/splinter.py <out> <world,area>
#      --con tools/cov/specs/splinter_<x>.con --fast --frames <n>
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module Splinter
CLASS = 'CSplinter'
SPEC = [
    ('DoUserAnimEvent 1600', 'CSplinter::DoUserAnimEvent(', ['(int)$rcx'], ''),
    ('ApplyAmbushVelocity A9B0', 'CSplinter::ApplyAmbushVelocity(', ['(int)$rsi'], ''),
    ('Pounce', 'CSplinter::Pounce(', ['(int)$rdx'], ''),
    ('Jump', 'CSplinter::Jump(', ['(int)$rdx'], ''),
    ('ExplodeSac', 'CSplinter::ExplodeSac(', [], ''),
    ('CreateSac', 'CSplinter::CreateSac(', [], ''),
]
