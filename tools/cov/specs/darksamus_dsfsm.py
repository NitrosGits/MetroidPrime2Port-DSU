# DarkSamus FsmStandIn slice (dsfsm): DiveAfterBoost fn_10_95A8, Null fn_10_14448, Dead fn_10_143A8
# and EnrageHitReact's call of fn_10_7F98 (UpdatePhazonEnrage).
#   COV_GDB_SCRIPT=tools/cov/cov_force.py MP2_INPUT=LAYER2@400 tools/cov/force.sh \
#     tools/cov/specs/darksamus_dsfsm.py <out> "1baa96c2,10_cliff_rooftop" \
#     --con tools/drivers/darksamus_dsfsm.con --fast --frames 1100
# Each saved value defaults to the member's value at the first call, so a 119 before its key
# writes the member back unchanged.
# (darksamus_dsfsm_dead.con: the same without the roll, so Dead finds no roll emitter.)
# DiveAfterBoost runs through `trigcall u04f7 DiveAfterBoost <key>`; the key is the CTriggerData's
# float at $rdx:
#   111 natural; 110 bit 0x20 of 0xEBC set; 112 the stage uid invalid (null stage);
#   113 / 114 the stage info's +0x78 byte forced 1 / 0 (written where the host reads it,
#   DiveAfterBoost+30: $rax is the stage); 119 puts the bit, the uid and the byte back.
# The +0x78 bool is the host's SLdrDSStageInfo::unknown_0x2d7551e6 at host stage offset 0xAC.
CLASS = 'CDarkSamus'

DAB = '_ZNK10CDarkSamus14DiveAfterBoostER13CStateManagerRK12CTriggerData'
UID = '*(unsigned short*)&$this->mBattleStageUid'
BYTE = '*(unsigned char*)($rax + 0xac)'
A = '$dsf_arg'

SPEC = [
    ('95A8 DiveAfterBoost', 'CDarkSamus::DiveAfterBoost(', ['*(float*)$rdx', '$this->mBoostBallFlag20', UID + ' == 0xffff'], 'ret'),
    ('14448 Null', 'CDarkSamus::Null(', ['$rdx'], ''),
    ('143A8 Dead', 'CDarkSamus::Dead(', ['$rdx', '$this->mBoostBallRollSfx.mID != 0'], 'calls'),
    ('7A14 EnrageHitReact', 'CDarkSamus::EnrageHitReact(', ['$rdx'], 'calls'),
]

FORCE = [
    ('95A8 entry|ret', 'CDarkSamus::DiveAfterBoost(', ['*(float*)$rdx'],
     [A + ' = *(float*)$rdx',
      '$dsf_f20 = %s == 110 || $_isvoid($dsf_f20) ? $this->mBoostBallFlag20 : $dsf_f20' % A,
      '$dsf_uid = %s == 112 || $_isvoid($dsf_uid) ? %s : $dsf_uid' % (A, UID),
      '$this->mBoostBallFlag20 = %s == 110 ? 1 : (%s == 119 ? $dsf_f20 : $this->mBoostBallFlag20)' % (A, A),
      '%s = %s == 112 ? 0xffff : (%s == 119 ? $dsf_uid : %s)' % (UID, A, A, UID)],
     0, 'keys 110 (0xEBC 0x20), 112 (no stage), 119 (restore)'),
    ('95A8 stage byte', 'loc:*%s+30' % DAB, [BYTE],
     ['$dsf_b = %s == 113 || $_isvoid($dsf_b) ? %s : $dsf_b' % (A, BYTE),
      '%s = %s == 113 ? 1 : (%s == 114 ? 0 : (%s == 115 || %s == 119 ? $dsf_b : %s))' % (BYTE, A, A, A, A, BYTE)],
     0, 'keys 113 / 114 (stage +0x78 = 1 / 0)'),
]
