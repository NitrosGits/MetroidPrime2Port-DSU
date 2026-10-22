# EmperorIngStage1's warp portal think (fn_16_1150): the two effect arms the natural fight never
# takes, forced by gdb writes (module note §12a).
#   0x1298 (the effect is inactive): the DCTV the portal sends its own effect once the "Whole Body"
#     animation is over (0x1318) also clears the effect's CEntity active bit (+0x20 0x80), so the
#     next thinks find it inactive. Only the portal's own effect ($emp_eff) is touched.
#   0x128C (the effect is gone): while the portal's timer is in 3.5..4.5 its effect id reads
#     invalid, so ObjectById gives null; at 4.5 the saved id is put back, so the portal's kSM_Delete
#     still deletes its effect.
# Run: tools/cov/force.sh (BUILD=build-port-cov) with tools/drivers/emperoring1_heartstage.py cycles.
CLASS = 'CEmperorIngStage1WarpPortal'
INV = 'kInvalidUniqueId.value'
EFF = '$this->mEffectId.value'
GONE = ('($this->mTimer > 3.5 && $this->mTimer < 4.5 && %s != %s) ? '
        '($emp_saved = %s, %s = %s) : '
        '(($this->mTimer >= 4.5 && %s == %s && !$_isvoid($emp_saved)) ? (%s = $emp_saved) : 0)'
        % (EFF, INV, EFF, EFF, INV, EFF, INV, EFF))
TRACK = '%s != %s ? ($emp_eff = %s) : 0' % (EFF, INV, EFF)
MSG = '((CScriptMsg*)$rdx)'
# gdb type-checks both sides of && even when the left is false: give $emp_eff a value first
INIT = '$_isvoid($emp_eff) ? ($emp_eff = -1) : 0'
INACTIVE = ('(%s->m_msg == 0x44435456 && '
            '((CEntity*)$rdi)->mUniqueId.value == $emp_eff) ? (((CEntity*)$rdi)->mActive = 0) : 0'
            % MSG)
FORCE = [
    ('1150 Think', 'CEmperorIngStage1WarpPortal::Think(', ['$this->mTimer * 10', EFF],
     [TRACK, GONE], 0, 'effect id invalid for timer 3.5..4.5 (0x128C), then restored'),
    ('portal effect DCTV', 'CScriptEffect::AcceptScriptMsg(', [MSG + '->m_msg'],
     [INIT, INACTIVE], 0, "the portal effect's DCTV also clears its active bit (0x1298)"),
]
