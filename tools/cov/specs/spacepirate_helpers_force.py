# Forcing spec (tools/cov/force.sh) for the SpacePirate's stateless helpers: the arms of
# TrooperWarpOut (fn_72_51C8) and ChooseTeamTarget (fn_72_EFD4) that the fights never take.
#
# Run 1 (TrooperWarpOut): SP_SETZERO=14 SP_NO_FORCE=1 tools/cov/force.sh
#   tools/cov/specs/spacepirate_helpers_force.py <out> 42b935e4,08_sand_casualpirates
#   --driver "python3 tools/drivers/spacepirate_helpers_drive.py {port} - 2" --frames 20000 --fast
# Run 2 (ChooseTeamTarget): COV_GDB_SCRIPT=<tree>/tools/cov/cov_force.py SP_TEAM_PHASE=1
#   SP_NO_FORCE=1 tools/cov/force.sh tools/cov/specs/spacepirate_helpers_force.py <out>
#   42b935e4,07_sand_alamoportalobservation --driver "python3 tools/drivers/spacepirate_helpers_drive.py
#   {port} 3 2" --frames 20000 --fast
#
# The kSM_SetToZero message reaches TrooperWarpOut (the port's AcceptScriptMsg, asm 0x5200), so the
# driver sends it a few times.  Each call takes one arm: the write below puts the member in the
# state that arm needs, and the arm key (recorded before the write) shows the state the call was
# made in.  The arms of the retail body (fn_72_51C8):
#   0x51F4: the actor is alive (the 0x420 byte's bit 25)  -> mAlive
#   0x5200: the 0x8fa byte's bit 25 is clear               -> x8fa_25_
#   0x520C: the warp point (0xc04) is -1                   -> xc04_warpPoint
#   0x5218: the 0x8f6 byte's bit 24 (a trooper) is set     -> x8f6_24_trooper
#   0x521C: the state "WarpOut" is set and the 0x8fa bit 27 is written
# The arms of fn_72_EFD4 (ChooseTeamTarget): no squad (0xba0 == 0xffff, asm 0xF010), a squad id that
# resolves to nothing (asm 0xF02C), a squad that is out of range (asm 0xF04C) and a squad in range
# (asm 0xF060).  The driver puts the two troopers of the layer together and then apart, so the
# range test takes both ways; the squad id is the one the pirate joined (the layer's TMAI).
# (gdb convenience variables only - not $fs/$gs, which are x86 registers.)
CLASS = 'CSpacePirate'
FORCE = [
    ('51C8 TrooperWarpOut', 'CSpacePirate::TrooperWarpOut(',
     ['$this->mAlive', '$this->x8fa_25_', '$this->xc04_warpPoint', '$this->x8f6_24_trooper'],
     ['$tw = $_isvoid($tw) ? 0 : $tw',
      '$tw_a = ($tw < 5) ? 1 : 0',
      '$this->mAlive = ($tw_a && $tw == 1) ? 0 : 1',
      '$this->x8fa_25_ = ($tw_a && $tw == 2) ? 1 : 0',
      '$this->xc04_warpPoint = ($tw_a && $tw == 3) ? 5 : -1',
      '$this->x8f6_24_trooper = ($tw_a && $tw == 4) ? 1 : 0',
      '$tw = $tw_a ? $tw + 1 : $tw'],
     0, 'one call per arm: not alive, the 0x8fa bit 25 set, a warp point set, not a trooper, then'
        ' the true arm (the WarpOut state)'),
    ('EFD4 ChooseTeamTarget', 'CSpacePirate::ChooseTeamTarget(',
     ['$this->xba0_teamAiMgrId.value', '$this->x8f4_27_melee'],
     ['$et = $_isvoid($et) ? 0 : $et',
      '$et_a = ($et < 3) ? 1 : 0',
      '$sv = $_isvoid($sv) ? 0xffff : $sv',
      '$sv = ($this->xba0_teamAiMgrId.value != 0xffff) ? $this->xba0_teamAiMgrId.value : $sv',
      '$this->xba0_teamAiMgrId.value = ($et_a && $et == 0) ? 0xffff : (($et_a && $et == 1) ? 0x7ffe : $sv)',
      '$et = $et_a ? $et + 1 : $et'],
     0, 'the squad id: none, an id that resolves to nothing, then the team the pirate joined'),
    ('DFD4 SquadJoin', 'CSpacePirate::SquadJoin(',
     ['$this->xba0_teamAiMgrId.value', '$this->x8f4_27_melee'],
     ['$sq = $_isvoid($sq) ? 0 : $sq',
      '$sq_a = ($sq < 4) ? 1 : 0',
      '$sv2 = $_isvoid($sv2) ? 0xffff : $sv2',
      '$sv2 = ($this->xba0_teamAiMgrId.value != 0xffff) ? $this->xba0_teamAiMgrId.value : $sv2',
      '$this->xba0_teamAiMgrId.value = ($sq_a && $sq == 0) ? 0xffff : (($sq_a && $sq == 1) ? 0x7ffe : $sv2)',
      '$this->x8f4_27_melee = ($sq_a && $sq == 3) ? 1 : 0',
      '$sq = $sq_a ? $sq + 1 : $sq'],
     0, 'the squad id: none (the team is looked up), an id that resolves to nothing, then the team'
        ' the pirate joined - and the melee role'),
    ('EDEC ChooseTarget', 'CSpacePirate::ChooseTarget(',
     ['$this->mAttackTarget.value', '((CPlayer*)((CStateManager*)$rsi)->mPlayers[0])->mActive'],
     ['$ct = $_isvoid($ct) ? 0 : $ct',
      '$ct_a = ($ct < 3) ? 1 : 0',
      '$this->mAttackTarget.value = ($ct_a && $ct == 0) ? 0xffff : $this->mAttackTarget.value',
      '((CPlayer*)((CStateManager*)$rsi)->mPlayers[0])->mActive = ($ct_a && $ct == 1) ? 0 : 1',
      '$ct = $ct_a ? $ct + 1 : $ct'],
     0, 'the attack target is not a player, and the player is not active'),
    ('2EA8 UpdateSfxEmitter', 'CSpacePirate::UpdateSfxEmitter(',
     ['$this->xc00_.mID != 0'],
     ['$es = $_isvoid($es) ? 0 : $es',
      '$es_a = ($es < 2) ? 1 : 0',
      # a live emitter handle (the pirate's own hurled sound) and then a bogus one: the playing /
      # queued arm and the dead one
      '$this->xc00_ = ($es_a && $es == 0) ? CSfxManager::AddEmitter(((CSpacePirate*)$rdi)->mData.mHurledSfx, ((CSpacePirate*)$rdi)->mPosition, 127, -1, 0, 0, 0) : $this->xc00_',
      '$this->xc00_.mID = ($es_a && $es == 1) ? 12345 : $this->xc00_.mID',
      '$es = $es_a ? ($es + 1) % 2 : $es'],
     0, 'a live emitter handle and then a bogus one: the playing / queued arm and the dead one'),
    ('8670 Shuffle', 'CSpacePirate::Shuffle(',
     ['$rdx', '$this->x8f4_28_noShuffleCloseCheck', '$this->mMinAttackRange'],
     ['$this->x8f4_28_noShuffleCloseCheck = 0',
      '$this->mMinAttackRange = 1000.0',
      # the random's seed, so the x draw (the second one) is 32768 -> 0.5 - the clamp arm
      # (|x| < 0.001, asm 0xD694-0xD69C) of fn_72_D608
      '((CStateManager*)$rsi)->mRandom.mSeed = 204577074'],
     0, 'the close check (the 0x8f4 bit 28) clear and a big min attack range, so the player counts'
        ' as too close and the shuffle picks a close spot: fn_72_D608 runs'),
]

# The arms are counted too (tools/cov/cov_force.py runs this list's breakpoints after force.py's
# writes, so the key shows the state the call was made in).
SPEC = [
    ('51C8 TrooperWarpOut: arms', 'CSpacePirate::TrooperWarpOut(',
     ['$this->mAlive', '$this->x8fa_25_', '$this->xc04_warpPoint', '$this->x8f6_24_trooper'],
     'max=100000'),
    ('2EA8 UpdateSfxEmitter: arms', 'CSpacePirate::UpdateSfxEmitter(',
     ['$this->xc00_.mID != 0'], 'max=1000000'),
    ('EFD4 ChooseTeamTarget: arms', 'CSpacePirate::ChooseTeamTarget(',
     ['$this->xba0_teamAiMgrId.value'], 'calls max=100000'),
    ('DFD4 SquadJoin: arms', 'CSpacePirate::SquadJoin(',
     ['$this->xba0_teamAiMgrId.value', '$this->x8f4_27_melee'], 'calls max=100000'),
    ('EDEC ChooseTarget: arms', 'CSpacePirate::ChooseTarget(',
     ['$this->mAttackTarget.value'], 'calls max=100000'),
    ('8670 Shuffle: arms', 'CSpacePirate::Shuffle(',
     ['$rdx', '$this->x8f4_28_noShuffleCloseCheck', '$this->mMinAttackRange'], 'max=100000'),
]
