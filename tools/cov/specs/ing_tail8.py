# Ing small-row arm coverage, tools/cov/run.sh spec (lane tail8): fn_29_1DF4, 53E0, 5714, 8CF4,
# 10D50, 10E64. Driver: tools/drivers/ing_tail8.py over the Agon Wastes 01_sand_entrance_dark.
#
# The two lure helpers are `inline` in include/MetroidPrime/Enemies/PortIngShared.hpp and are inlined
# into their callers, and most of their statements carry no line record in the optimised build (gdb
# answers "No compiled code for line N" and leaves the breakpoint pending, counting nothing). So the
# arms are counted at the branch instruction the compiler emitted, symbol-relative (gdb re-resolves
# them after the PIE relocates), the way tools/cov/specs/ing_team.py does it. The offsets are from
# `objdump -d -C build-port-link/metroid_prime2_port`:
#
#  _ZN4CIng6ListenER13CStateManagerRK9CVector3f16EListenNoiseType      (fn_29_B294 + fn_29_1DF4)
#    +0xc7  jb      comiss r^2, distSq: CF=1 -> out of the hearing radius (exit; a NaN too)
#    +0x107 je      the held lure zone is kInvalidUniqueId -> take the noise's zone
#    +0x136 je      the held id is no CScriptSafeZone -> take the noise's zone
#    +0x16f jbe     the held zone is not farther than the noise -> keep it (CF|ZF)
#  _ZN4CIng12SetLuredDestER13CStateManagerf                            (fn_29_53E0 + fn_29_1C98)
#    +0x5d  je      no CScriptSafeZone at 0xd78 (0x1CD4/0x1CD8)
#    +0xd3  jne     GetHeightOfPointAboveMesh failed (0x1D?? -> no mesh point)
#    +0x13c jne     PathExists != 0 (asm 0x5470/0x5474): ZF=1 = the path exists -> dest = lure
#  _ZN4CIng21SetWallPointCoverDestER13CStateManagerf                   (fn_29_5714)
#    +0x54  je      no hint (asm 0x5744/0x5748)
#    +0x62  je      no host path area (the port's guard)
#    +0xad  je      no closest point to the Ing (asm 0x5784/0x5788)
#    +0xed  jne     a point for the hint exists (asm 0x57C4/0x57C8); ZF=1 = none
#    +0x144 jne     the point search failed (asm 0x57F0/0x57F4); ZF=1 = success -> the flag store
#  _ZN4CIng16SafeZoneReactionER13CStateManager9EStateMsgf              (fn_29_8CF4)
#    +0xd   je      msg 0 (Activate)  (the test is at +0xb; a breakpoint there reads the wrong flags)
#    +0x12  je      msg 1 (Update)
#    +0x17  jne     msg is not 2 -> the unknown-message return (asm 0x8D1C/0x8D24)
#    +0x19  (store 0) msg 2 (Deactivate)
#    +0x37  jne     the hurtful zone bit -> knock-back (asm 0x8D64)
#    +0x44  jne     the evaporating bit -> knock-back (asm 0x8D70)
#    +0x71  jbe     the state time is not over a second (asm 0x8E68 ble; a NaN too)
#    +0x73  (store 3) the time arm
#    +0x104 je      CanIssueCommand said no (asm 0x8D84/0x8D8C) - the fall-through delivers
#  _ZN12_GLOBAL__N_1L15ConstructEffectER6CTokenRK9CVector3fS4_          (fn_29_10D50)
#    +0x6   je      the host's HasReference guard (retail has no test: 0x10D50-0x10DC8)
#  _ZN18CIngExitHostEffect5RenderERK13CStateManager / ...10AddToRendererERK13CStateManager
#    whole bodies; Render must stay at 0 hits (dead on the host, nothing queues it) while
#    AddToRenderer runs.
CLASS = 'CIng'
LISTEN = '_ZN4CIng6ListenER13CStateManagerRK9CVector3f16EListenNoiseType'
LURED = '_ZN4CIng12SetLuredDestER13CStateManagerf'
COVER = '_ZN4CIng21SetWallPointCoverDestER13CStateManagerf'
REACT = '_ZN4CIng16SafeZoneReactionER13CStateManager9EStateMsgf'
EFFECT = '_ZN12_GLOBAL__N_1L15ConstructEffectER6CTokenRK9CVector3fS4_'
SPEC = [
    ('1DF4 @0x1E58 outside the hearing radius', 'loc:*%s+0xc7' % LISTEN, ['$eflags & 0x1'], 'max=400'),
    ('1DF4 @0x1E84 unset takes the zone', 'loc:*%s+0x107' % LISTEN, ['$eflags & 0x40'], 'max=400'),
    ('1DF4 @0x1EF4 held id is no zone', 'loc:*%s+0x136' % LISTEN, ['$eflags & 0x40'], 'max=400'),
    ('1DF4 @0x1EE8 keeps the held zone', 'loc:*%s+0x16f' % LISTEN, ['$eflags & 0x41'], 'max=400'),
    ('53E0 @0x1CD8 the lure id is no zone', 'loc:*%s+0x5d' % LURED, ['$eflags & 0x40'], 'max=400'),
    ('53E0 mesh point not found', 'loc:*%s+0xd3' % LURED, ['$eflags & 0x40'], 'max=400'),
    ('53E0 @0x5474 PathExists', 'loc:*%s+0x13c' % LURED, ['$eflags & 0x40'], 'max=400'),
    ('53E0 entry', 'CIng::SetLuredDest(', [], 'calls max=400'),
    ('5714 @0x5748 no hint', 'loc:*%s+0x54' % COVER, ['$eflags & 0x40'], 'max=400'),
    ('5714 no host path area', 'loc:*%s+0x62' % COVER, ['$eflags & 0x40'], 'max=400'),
    ('5714 @0x5788 no closest point', 'loc:*%s+0xad' % COVER, ['$eflags & 0x40'], 'max=400'),
    ('5714 @0x57C8 point for the hint', 'loc:*%s+0xed' % COVER, ['$eflags & 0x40'], 'max=400'),
    ('5714 @0x57F4 point search', 'loc:*%s+0x144' % COVER, ['$eflags & 0x40'], 'max=400'),
    ('5714 entry', 'CIng::SetWallPointCoverDest(', [], 'calls max=400'),
    ('8CF4 entry', 'CIng::SafeZoneReaction(', ['$rdx'], 'calls max=400'),
    ('8CF4 msg 0 Activate', 'loc:*%s+0xd' % REACT, ['$eflags & 0x40'], 'max=400'),
    ('8CF4 msg 1 Update', 'loc:*%s+0x12' % REACT, ['$eflags & 0x40'], 'max=400'),
    ('8CF4 unknown msg', 'loc:*%s+0x17' % REACT, ['$eflags & 0x40'], 'max=400'),
    ('8CF4 msg 2 Deactivate', 'loc:*%s+0x19' % REACT, [], 'max=400'),
    ('8CF4 @0x8D64 hurtful zone', 'loc:*%s+0x37' % REACT, ['$eflags & 0x40'], 'max=400'),
    ('8CF4 @0x8D70 evaporating', 'loc:*%s+0x44' % REACT, ['$eflags & 0x40'], 'max=400'),
    ('8CF4 @0x8E68 time under a second', 'loc:*%s+0x71' % REACT, ['$eflags & 0x41'], 'max=400'),
    ('8CF4 @0x8E70 time over a second', 'loc:*%s+0x73' % REACT, [], 'max=400'),
    ('8CF4 @0x8D8C CanIssueCommand', 'loc:*%s+0x104' % REACT, ['$eflags & 0x40'], 'max=400'),
    ('10D50 ConstructEffect entry', 'loc:*%s' % EFFECT, [], 'calls max=400'),
    ('10D50 @0x10D60 HasReference guard', 'loc:*%s+0x6' % EFFECT, ['$eflags & 0x40'], 'max=400'),
    ('10E64 Render entry', 'CIngExitHostEffect::Render(', [], 'calls max=400'),
    ('10DEC AddToRenderer entry', 'CIngExitHostEffect::AddToRenderer(', [], 'calls max=400'),
    ('ExitHostEffect ctor', 'CIngExitHostEffect::CIngExitHostEffect(', [], 'max=40'),
]
