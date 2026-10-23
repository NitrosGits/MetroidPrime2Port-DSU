# Splitter (module 75) split26 lane: the beam effect's PreRender (fn_75_10F1C) and Render
# (fn_75_10778) where the heads scan and open the beam.  The arms are line breakpoints in the
# built source: Render's early return (fewer than two copies), PreRender's PVS arm and each
# near-actor gate's `continue`, and the flat draw.
#
#   tools/cov/run.sh tools/cov/specs/splitter_split26.py <out> 1baa96c2,914f1381 \
#     --con tools/cov/specs/splitter_split23.con --fast --frames 4000
# (the line numbers are those of commit "Splitter split26: separate near-actor gates")
CLASS = 'CSplitterBeamEffect'
_F = 'loc:PortCSplitterCommandModule.cpp:'
SPEC = [
    ('10778 Render', 'CSplitterBeamEffect::Render(', ['$this->mUnknown184 < 2'], 'max=4000'),
    ('107AC Render early return', _F + '603', [], 'max=4000'),
    ('10D10 Render fan row', _F + '697', [], 'max=200000'),
    ('10F1C PreRender', 'CSplitterBeamEffect::PreRender(', ['$this->mUnknown184 < 1'], 'max=4000'),
    ('1114C PreRender PVS', _F + '496', [], 'max=4000'),
    ('11440 no actor', _F + '536', [], 'max=4000'),
    ('1144C !CanDrawStatic', _F + '539', [], 'max=4000'),
    ('1145C no projected shadow', _F + '542', [], 'max=4000'),
    ('11474 outside the frustum', _F + '545', [], 'max=4000'),
    ('11488 misses the near box', _F + '548', [], 'max=4000'),
    ('1148C draw flat', _F + '552', [], 'max=4000'),
]
