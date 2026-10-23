# DarkCommando (module 3) forcing spec for tools/cov/force.sh: the arms the fight cannot reach.
# Every force is self-restoring or cleared by the state machine (SelectAction clears the knock-back
# flag, SetTargetFaceVect recomputes the face vector), so the fight goes on between the forced hits.
# The labels are the retail offsets; the notes' branch tables say "forced (<label>)".
# Run in the team area (three commandos and a team AI manager on layer 8); the driver's last part
# sends Deactivate three times to the first commando for the LeaveTeam arms:
#   MP2_GODMODE=1 BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darkcommando_force.py <out> \
#     3bfa3eff,11_temple_grandpuzzle --driver 'python3 tools/drivers/darkcommando_melee.py {port} <out>/melee.log 8 leave' \
#     --frames 9000
CLASS = 'CDarkCommando'


def counter(name):
    return '%s = $_isvoid(%s) ? 1 : %s + 1' % (name, name, name)


FORCE = [
    # fn_3_6070: the target closer than the minimum range (the shipped minimum is below what the
    # placement reaches): every 25th call adds 1000 to the minimum, the next call on that commando
    # takes it off again.
    ('6070 ShouldMeleeAttack: min range (forced)', 'CDarkCommando::ShouldMeleeAttack(', [],
     [counter('$dsm_n'),
      '$this->mMinAttackRange = $this->mMinAttackRange >= 1000 ? $this->mMinAttackRange - 1000 : '
      '(($dsm_n % 25 == 0) ? $this->mMinAttackRange + 1000 : $this->mMinAttackRange)'], 0,
     'mMinAttackRange + 1000 on every 25th call, restored on the next'),
    # fn_3_3690: not attacking (every 12th call, offset 6), and a zero blade direction (the blade
    # locator swapped for the wrist one on every 12th call, offset 3, restored on the next call).
    ('3690 ApplyMeleeDamage: not attacking / zero blade (forced)', 'CDarkCommando::ApplyMeleeDamage(',
     ['$this->mMeleeAttacking'],
     [counter('$dam_n'),
      '$dam_seg = $dam_n == 1 ? $this->mMeleeSegId.mId : $dam_seg',
      '$this->mMeleeAttacking = ($dam_n % 12 == 6) ? 0 : $this->mMeleeAttacking',
      '$this->mMeleeSegId.mId = ($dam_n % 12 == 3) ? $this->mWristSegId.mId : $dam_seg'], 0,
     'mMeleeAttacking 0 on calls 6 mod 12; mMeleeSegId = mWristSegId on calls 3 mod 12'),
    # fn_3_5074: no target in the update (every 9th update, offset 4); the deactivate restores
    # the player's id.
    ('5074 FireChargeBeam: no target (forced)', 'CDarkCommando::FireChargeBeam(', ['$rdx'],
     [counter('$dfc_n'),
      '$dfc_t = $this->mTargetId.value != 0xffff ? $this->mTargetId.value : $dfc_t',
      '$this->mTargetId.value = ($rdx == 1 && $dfc_n % 9 == 4) ? 0xffff : '
      '(($rdx == 2 && $this->mTargetId.value == 0xffff) ? $dfc_t : $this->mTargetId.value)'], 0,
     'mTargetId invalid on updates 4 mod 9, the player id again at the deactivate'),
    # fn_3_484C: a zero face vector in the update (every 9th update, offset 5).
    ('484C FaceTarget: zero face vector (forced)', 'CDarkCommando::FaceTarget(', ['$rdx'],
     [counter('$dft_n'),
      '$this->mFaceVector.mX = ($rdx == 1 && $dft_n % 9 == 5) ? 0.0 : $this->mFaceVector.mX',
      '$this->mFaceVector.mY = ($rdx == 1 && $dft_n % 9 == 5) ? 0.0 : $this->mFaceVector.mY',
      '$this->mFaceVector.mZ = ($rdx == 1 && $dft_n % 9 == 5) ? 0.0 : $this->mFaceVector.mZ'], 0,
     'mFaceVector zero on updates 5 mod 9'),
    # fn_3_3B98: opaque but fading out (alpha 1, target 0.5) on every 3rd call.
    ('3B98 DeactivateCloak: alpha 1, target < 1 (forced)', 'CDarkCommando::DeactivateCloak(', [],
     [counter('$ddc_n'),
      '$this->mCloakTargetAlpha = ($ddc_n % 3 == 0 && $this->mCloakAlpha >= 1) ? 0.5 : '
      '$this->mCloakTargetAlpha'], 0,
     'mCloakTargetAlpha 0.5 when mCloakAlpha is 1, on calls 0 mod 3'),
    # fn_3_39AC: a decoy id whose object is gone (every 300th call, when there is no decoy).
    ('39AC UpdateCloak: stale decoy id (forced)', 'CDarkCommando::UpdateCloak(', [],
     [counter('$duc_n'),
      '$this->mDecoyId.value = ($duc_n % 300 == 150 && $this->mDecoyId.value == 0xffff) ? 0xfbfe : '
      '$this->mDecoyId.value'], 0,
     'mDecoyId 0xfbfe (no such object) on calls 150 mod 300'),
    # fn_3_5A2C / fn_3_58D8: knocked back during the update (every 7th update, offset 3).
    ('5A2C Lurk: knocked back (forced)', 'CDarkCommando::Lurk(', ['$rdx'],
     [counter('$dlk_n'),
      '$this->mKnockedBack = ($rdx == 1 && $dlk_n % 7 == 3) ? 1 : $this->mKnockedBack'], 0,
     'mKnockedBack 1 on updates 3 mod 7'),
    ('58D8 Taunt: knocked back (forced)', 'CDarkCommando::Taunt(', ['$rdx'],
     [counter('$dtt_n'),
      '$this->mKnockedBack = ($rdx == 1 && $dtt_n % 7 == 3) ? 1 : $this->mKnockedBack'], 0,
     'mKnockedBack 1 on updates 3 mod 7'),
    # fn_3_2FA4: the target closer than the charge beam's minimum range (as ShouldMeleeAttack).
    ('2FA4 InChargeBeamRange: min range (forced)', 'CDarkCommando::InChargeBeamRange(', [],
     [counter('$dcb_n'),
      '$this->mData.chargeBeamAttackInfo.minAttackRange = '
      '$this->mData.chargeBeamAttackInfo.minAttackRange >= 1000 ? '
      '$this->mData.chargeBeamAttackInfo.minAttackRange - 1000 : (($dcb_n % 4 == 0) ? '
      '$this->mData.chargeBeamAttackInfo.minAttackRange + 1000 : '
      '$this->mData.chargeBeamAttackInfo.minAttackRange)'], 0,
     'chargeBeam minAttackRange + 1000 on every 4th call, restored on the next'),
    # fn_3_8BEC (the grenade, so the cast is spelled out): as darkcommando_emp_force.py.
    ('8BEC Grenade::Think: EMP duration (forced)', 'CDarkCommandoGrenade::Think(',
     ['((CDarkCommandoGrenade*)$rdi)->mEMPTime < 0.3'],
     ['((CDarkCommandoGrenade*)$rdi)->mData.eMPDuration = '
      '((CDarkCommandoGrenade*)$rdi)->mEMPTime < 0.3 ? 100.0 : 0.0'], 0,
     'eMPDuration 100 until 0.3 s after the explosion, then 0'),
    # fn_3_15A8: call 1 is the natural quit; call 2 gets the team id back (not part of the team);
    # call 3 gets the commando's own id (not a team manager).
    ('15A8 LeaveTeam: not a member / not a team (forced)', 'CDarkCommando::LeaveTeam(',
     ['$this->mTeamAiMgrId.value'],
     [counter('$dlt_n'),
      '$dlt_team = $dlt_n == 1 ? $this->mTeamAiMgrId.value : $dlt_team',
      '$this->mTeamAiMgrId.value = $dlt_n == 2 ? $dlt_team : '
      '($dlt_n == 3 ? $this->mUniqueId.value : $this->mTeamAiMgrId.value)'], 0,
     'call 2: the team id of call 1; call 3: the own id'),
]
