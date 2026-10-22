# ds39: the bodied, unmarked rows (natural run; the arms come from the gcov report).
# Run: BUILD=$PWD/build-port-cov MP2_INPUT=LAYER2@400 tools/cov/run.sh tools/cov/specs/darksamus_ds39.py
#      <out> "1baa96c2,10_cliff_rooftop" --con tools/drivers/darksamus_ds39.con --fast --frames 5700
CLASS = 'CDarkSamus'
SPEC = [
    ('6E38 EnragedTravel', 'CDarkSamus::EnragedTravel(', ['$rdx'], 'calls max=2000'),
    ('6DAC UpdateShieldFromHealth', 'CDarkSamus::UpdateShieldFromHealth(', ['$rsi'], 'max=2000'),
]
