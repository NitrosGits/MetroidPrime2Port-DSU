# Rezbit (REL 53) on the coverage build: the gcov profile is the evidence; these breakpoints only
# count a few entries. Natural-play drivers: tools/drivers/tail21_rezbit*.txt, tail24_rezbit_missile.con,
# rezbit_*.con (02_cliff_reactorcore, layers 3+4).
# Run: BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/rezbit.py <out> 1baa96c2,02_cliff_reactorcore \
#        --con tools/drivers/<driver> --fast --frames <n>
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module Rezbit
CLASS = 'CRezbit'
SPEC = [
    ('Think rezState', 'CRezbit::Think(', ['$this->mRezState'], 'max=4000'),
    ('AcceptScriptMsg', 'CRezbit::AcceptScriptMsg(', [], 'max=4000'),
    ('FireEnergyBolt', 'CRezbit::FireEnergyBolt(', [], 'max=400'),
    ('SpawnLaser', 'CRezbit::SpawnLaser(', [], 'max=400'),
    ('StartVirusAttack', 'CRezbit::StartVirusAttack(', [], 'max=400'),
]
