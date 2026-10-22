# Coverage spec: fn_79_5D40 (GetIngSnatchingPoint) and fn_79_5E3C (GetIngSnatchingNormal).
#
# The two bodies are reached the way the game reaches them: the boss is ing-possessed (the
# head-mode anim event 0x0e, which the retail's anim-event handler turns into
# SetIngPossessed(true, 15.f)), and while the possession blend is between 0 and 1 the module's
# AddToRenderer (fn_79_A59C) runs the ing-snatch transition, which asks for the normal and the
# point through the vtable (the slots +0x138 and +0x13C).  The driver
# (tools/drivers/swampboss2_ing.py) sends the anim event; this spec only tallies the two calls.
CLASS = 'CSwampBossStage2'
SPEC = [
    ('5D40', 'CSwampBossStage2::GetIngSnatchingPoint(', [], 'max=2000'),
    ('5E3C', 'CSwampBossStage2::GetIngSnatchingNormal(', [], 'max=2000'),
]
