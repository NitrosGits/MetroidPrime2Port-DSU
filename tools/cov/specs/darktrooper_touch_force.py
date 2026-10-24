# dtr7: Touch (`trigger->GetActive() && (flags & DetectAI) && force magnitude > 0`). The hive_hall
# triggers are never active together with DetectAI and a force, so at Touch's entry the trigger
# passed in (`$rsi`, the actor) gets its own fields written, by uid (0e_temple_hive_hall, the cov
# build; see tools/drivers/darktrooper_touch.con):
#   u0482 active: flags |= DetectAI (0x8000), force magnitude 1   -> every operand true, impulse added
#   u0465 active: flags |= DetectAI, force magnitude 0             -> the magnitude operand false
#   u044c active: flags &= ~DetectAI                               -> the flag operand false
# u0490 (inactive) is natural: the active operand false.
CLASS = 'CDarkTrooper'


def _write(gdb):
    uid = int(gdb.parse_and_eval('((CEntity*)$rsi)->mUniqueId.value')) & 0xffff
    if uid not in (0x482, 0x465, 0x44c):
        return
    t = '((CScriptTrigger*)$rsi)'
    flags = int(gdb.parse_and_eval(t + '->mFlags')) & 0xffffffff
    flags = flags & ~0x8000 if uid == 0x44c else flags | 0x8000
    gdb.execute('set var %s->mFlags = %d' % (t, flags))
    if uid != 0x44c:
        gdb.execute('set var %s->mForceMagnitude = %s' % (t, '1.0f' if uid == 0x482 else '0.0f'))


FORCE = [
  ('TouchTrigger', 'CDarkTrooper::Touch(', [], [_write], 0, 'forced: trigger flags / force magnitude by trigger id'),
]
