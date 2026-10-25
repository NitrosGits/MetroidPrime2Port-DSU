# FishCloud (module 20) AddSelf (68D4) forcing spec for tools/cov/force.sh (fish2, 2026-10-25). Every shipped FSHM has exactly one
# MDFY/FOLW connection to a FISH, so AddSelf's skip/invalid/cast-fail arms never run. At the entry of AddSelf (this=rdi, a
# CFishCloudModifier) its first connection is reset to the shipped values and then, by call number, forced:
#   call 1 state != Modify (89:1), call 2 msg != Follow (89:2), call 3 objId unknown (93:0), call 4 objId = the modifier's own
#   editor id, an object that is no CFishCloud (96:3), later calls untouched.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/fishcloud_addself_force.py <out> \
#     3bfa3eff,01_Temple_LandingSite --con tools/cov/specs/fishcloud_landingsite.con --fast --frames 1800
CLASS = 'CFishCloudModifier'
_st = {'n': 0, 'orig': {}}


def addself(gdb):
    this = int(gdb.parse_and_eval('$rdi'))
    c = '((CFishCloudModifier*)$rdi)->mConnections.mItems[0]'

    def ev(e):
        return int(gdb.parse_and_eval(e))

    def put(f, v):
        gdb.execute('set var %s.%s = %d' % (c, f, v))
    if ev('((CFishCloudModifier*)$rdi)->mConnections.mCount') < 1:
        return
    if this not in _st['orig']:
        _st['orig'][this] = tuple(ev('(long)%s.%s' % (c, f)) for f in ('state', 'msg', 'objId.value'))
    for f, v in zip(('state', 'msg', 'objId.value'), _st['orig'][this]):
        put(f, v)
    n = _st['n']
    _st['n'] += 1
    if n == 0:
        put('state', 0)
    elif n == 1:
        put('msg', 0)
    elif n == 2:
        put('objId.value', 0x7ffffff0)
    elif n == 3:
        put('objId.value', ev('((CEntity*)$rdi)->mEditorId.value'))


FORCE = [
    ('68D4 AddSelf', 'CFishCloudModifier::AddSelf(', [], [addself], 0,
     'connection forced by call: state, msg, unknown objId, non-cloud objId'),
]
