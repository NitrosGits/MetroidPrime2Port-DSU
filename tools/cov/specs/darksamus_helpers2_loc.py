# Per-arm coverage of the bodies the helpers2 lane still had open: a 'loc:' breakpoint on every
# arm line of the body (the lines of the source the binary was built from), plus the entry with its
# arm conditions, so the arms that depend on locals are visible too.
#
# Run: tools/cov/run.sh tools/cov/specs/darksamus_helpers2_loc.py <out> "1baa96c2,10_cliff_rooftop"
#         --con tools/cov/specs/darksamus_rooftop.con --fast --frames 6400
CLASS = 'CDarkSamus'
F = 'loc:PortCDarkSamusFsm.cpp:'
S = 'loc:PortCDarkSamus.cpp:'
SPEC = [
    # ---- fn_10_1DD14 Separate: the zero point, the contact time, the strength, the separation ---
    ('1DD14 Separate (entry)', 'CDarkSamus::Separate(', ['$this->mContactTime + 0.3 > $this->mElapsedTime'], 'calls max=4000'),
    ('1DD14 Separate: the point is zero', F + '817', [], 'calls max=4000'),
    ('1DD14 Separate: the contact time is fresh', F + '822', [], 'calls max=4000'),
    ('1DD14 Separate: the repulsion is weak', F + '825', [], 'calls max=4000'),
    ('1DD14 Separate: the separation is normalized', F + '830', [], 'calls max=4000'),
    ('1DD14 Separate: the command is delivered', F + '831', [], 'calls max=4000'),
    # ---- fn_10_16B8C TurnTowardPoint: dt, the two normalisations -------------------------------
    ('16B8C TurnTowardPoint (entry)', 'CDarkSamus::TurnTowardPoint(', ['$xmm0.v4_float[0] <= 0.0'], 'calls max=8000'),
    ('16B8C TurnTowardPoint: dt is not positive', F + '4165', [], 'calls max=8000'),
    ('16B8C TurnTowardPoint: the forward is normalized', F + '4170', [], 'calls max=8000'),
    ('16B8C TurnTowardPoint: the point is normalized', F + '4175', [], 'calls max=8000'),
    # ---- fn_10_1D860 UpdateAdditiveAnimation: the activate, the found, the deactivate -----------
    ('1D860 UpdateAdditiveAnimation (entry)', 'CDarkSamus::UpdateAdditiveAnimation(',
     ['$rdx == 0', '$this->mAdditiveAnim != -1'], 'calls max=4000'),
    ('1D860 UpdateAdditiveAnimation: the activate', F + '852', [], 'calls max=4000'),
    ('1D860 UpdateAdditiveAnimation: an animation was found', F + '856', [], 'calls max=4000'),
    ('1D860 UpdateAdditiveAnimation: the additive animation is deleted', F + '861', [], 'calls max=4000'),
    # ---- fn_10_1E740 ShouldForgetLastAttacker: the four early outs and the distance -------------
    ('1E740 ShouldForgetLastAttacker (entry)', 'CDarkSamus::ShouldForgetLastAttacker(',
     ['$this->mLastAttackerUid.value == 65535'], 'calls max=4000'),
    ('1E740 ShouldForgetLastAttacker: the uid is invalid', F + '708', [], 'calls max=4000'),
    ('1E740 ShouldForgetLastAttacker: the path is shagged', F + '711', [], 'calls max=4000'),
    ('1E740 ShouldForgetLastAttacker: the path find is over', F + '715', [], 'calls max=4000'),
    ('1E740 ShouldForgetLastAttacker: the attacker has no cover point', F + '719', [], 'calls max=4000'),
    ('1E740 ShouldForgetLastAttacker: the distance test', F + '721', [], 'calls max=4000'),
    # ---- fn_10_EFC8 IsInUnsafeSpot: the safe position, the hint filter, the radius ---------------
    ('EFC8 IsInUnsafeSpot (entry)', 'CDarkSamus::IsInUnsafeSpot(', ['$this->mHasSafePos == 1'], 'calls max=4000'),
    ('EFC8 IsInUnsafeSpot: the safe position is too low', F + '877', [], 'calls max=4000'),
    ('EFC8 IsInUnsafeSpot: the hint is skipped', F + '884', [], 'calls max=4000'),
    ('EFC8 IsInUnsafeSpot: the hint is of another type', F + '888', [], 'calls max=4000'),
    ('EFC8 IsInUnsafeSpot: the hint is in the radius', F + '891', [], 'calls max=4000'),
    ('EFC8 IsInUnsafeSpot: no hint is close', F + '894', [], 'calls max=4000'),
    # ---- fn_10_1444C IsLineOnMesh: the height query, the height band ---------------------------
    ('1444C IsLineOnMesh (entry)', 'CDarkSamus::IsLineOnMesh(', [], 'calls max=4000'),
    ('1444C IsLineOnMesh: the height query failed', F + '3894', [], 'calls max=4000'),
    ('1444C IsLineOnMesh: the height is out of the band', F + '3897', [], 'calls max=4000'),
    ('1444C IsLineOnMesh: the line is on the mesh', F + '3900', [], 'calls max=4000'),
    # ---- fn_10_1E3C0 FindBestCoverPoint: the skip, the better score -----------------------------
    ('1E3C0 FindBestCoverPoint (entry)', 'CDarkSamus::FindBestCoverPoint(', [], 'calls max=4000'),
    ('1E3C0 FindBestCoverPoint: the cover point is skipped', F + '631', [], 'calls max=4000'),
    ('1E3C0 FindBestCoverPoint: the cover point is better', F + '635', [], 'calls max=4000'),
    ('1E3C0 FindBestCoverPoint: the best cover point', F + '639', [], 'calls max=4000'),
    # ---- fn_10_1C6C8 ProjectileInfo: the three answers -----------------------------------------
    ('1C6C8 ProjectileInfo (entry)', 'CDarkSamus::ProjectileInfo(', [], 'calls max=4000'),
    ('1C6C8 ProjectileInfo: the second scatter info', S + '613', [], 'calls max=4000'),
    ('1C6C8 ProjectileInfo: the scatter info', S + '615', [], 'calls max=4000'),
    ('1C6C8 ProjectileInfo: no info', S + '617', [], 'calls max=4000'),
    # ---- fn_10_1EE4C CanReflectHit: the four early outs and the two rolls ----------------------
    ('1EE4C CanReflectHit (entry)', 'CDarkSamus::CanReflectHit(', [], 'calls max=4000'),
    ('1EE4C CanReflectHit: the hit is too recent', F + '2205', [], 'calls max=4000'),
    ('1EE4C CanReflectHit: the weapon is not a missile', F + '2208', [], 'calls max=4000'),
    ('1EE4C CanReflectHit: there is no stage', F + '2212', [], 'calls max=4000'),
    ('1EE4C CanReflectHit: the radius damage', F + '2216', [], 'calls max=4000'),
    ('1EE4C CanReflectHit: the normal damage', F + '2218', [], 'calls max=4000'),
    # ---- fn_10_14D14 UpdateDoubleDash: the activate, the blocked, the steps, the deactivate ----
    ('14D14 UpdateDoubleDash (entry)', 'CDarkSamus::UpdateDoubleDash(',
     ['$rdx', '$this->mDash.mBlocked', '$this->mDash.mStepsLeft > 0'], 'calls max=4000'),
    ('14D14 UpdateDoubleDash: the activate', F + '4754', [], 'calls max=4000'),
    ('14D14 UpdateDoubleDash: the dash is not blocked', F + '4765', [], 'calls max=4000'),
    ('14D14 UpdateDoubleDash: the next dash step', F + '4767', [], 'calls max=4000'),
    ('14D14 UpdateDoubleDash: the deactivate', F + '4771', [], 'calls max=4000'),
    # ---- fn_10_19B64 CollectReachAttacks: the seven attack bits ---------------------------------
    ('19B64 CollectReachAttacks (entry)', 'CDarkSamus::CollectReachAttacks(',
     ['(*$rcx & 1) != 0', '(*$rcx & 0x100) != 0', '(*$rcx & 0x20) != 0',
      '(*$rcx & 4) != 0', '(*$rcx & 0x80) != 0', '(*$rcx & 0x10) != 0'], 'calls max=4000'),
    ('19B64 CollectReachAttacks: bit 0x1', F + '3928', [], 'calls max=4000'),
    ('19B64 CollectReachAttacks: bit 0x100', F + '3931', [], 'calls max=4000'),
    ('19B64 CollectReachAttacks: the player cannot be walked to', F + '3934', [], 'calls max=4000'),
    ('19B64 CollectReachAttacks: bit 0x20', F + '3940', [], 'calls max=4000'),
    ('19B64 CollectReachAttacks: bit 0x4', F + '3948', [], 'calls max=4000'),
    ('19B64 CollectReachAttacks: bit 0x80', F + '3956', [], 'calls max=4000'),
    ('19B64 CollectReachAttacks: bit 0x10', F + '3970', [], 'calls max=4000'),
    # ---- fn_10_1905C ClassifyLineToPlayer: the four arms and the three answers ------------------
    ('1905C ClassifyLineToPlayer (entry)', 'CDarkSamus::ClassifyLineToPlayer(', [], 'calls max=4000'),
    ('1905C ClassifyLineToPlayer: the player is close', F + '3603', [], 'calls max=4000'),
    ('1905C ClassifyLineToPlayer: the direction is not normalisable', F + '3607', [], 'calls max=4000'),
    ('1905C ClassifyLineToPlayer: the ray hit', F + '3625', [], 'calls max=4000'),
    ('1905C ClassifyLineToPlayer: no hit', F + '3630', [], 'calls max=4000'),
    ('1905C ClassifyLineToPlayer: the hit is nearer', F + '3633', [], 'calls max=4000'),
    ('1905C ClassifyLineToPlayer: the hit is farther', F + '3635', [], 'calls max=4000'),
    # ---- fn_10_154FC PickDash: the facing, the slots, the clearance ------------------------------
    ('154FC PickDash (entry)', 'CDarkSamus::PickDash(', [], 'calls max=4000'),
    ('154FC PickDash: the facing is normalized', F + '4555', [], 'calls max=4000'),
    ('154FC PickDash: the facing is not normalizable', F + '4557', [], 'calls max=4000'),
    ('154FC PickDash: the step blocks the slot', F + '4569', [], 'calls max=4000'),
    ('154FC PickDash: the last slot is the slot', F + '4576', [], 'calls max=4000'),
    ('154FC PickDash: the height query failed', F + '4611', [], 'calls max=4000'),
    ('154FC PickDash: the actor stood still', F + '4613', [], 'calls max=4000'),
    ('154FC PickDash: the clearance is small', F + '4616', [], 'calls max=4000'),
    ('154FC PickDash: the end is nearer than the player', F + '4622', [], 'calls max=4000'),
]
