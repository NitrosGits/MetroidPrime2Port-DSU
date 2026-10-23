# sb12: the SwampBossStage1 tongue block ctor (fn_78_19FC, built in the ctor fn_78_AA50's initializer
# list) and the SwampBossStage2 loader (fn_79_170). One spec for both modules; labels are retail
# offsets, loc lines are the built sources'. Runs: 3dfd2249,04_swamp_temple_dark with
# tools/cov/specs/swampboss_sb12_l3.con (Stage1) and swampboss_sb12_l6.con (Stage2).
CLASS = 'CSwampBossStage1'
SPEC = [
    ('78_AA50 ctor (19FC block, 1B30 ResetTongueState)', 'CSwampBossStage1::CSwampBossStage1(', [], 'calls'),
    ('78_19A4 ResetTongueState: amp==1', 'CSwampBossStage1::ResetTongueState(',
     ['$this->xc40_tongueAmp == 1.0'], 'max=50'),
    # MakeModel is inlined: read the two optionals and the amp where the ctor's body calls 1B30.
    ('78_19FC block built (at 1B30)', 'loc:PortCSwampBossStage1.cpp:264',
     ['this->xc44_tongueModel.m_valid', 'this->xc94_tipModel.m_valid', 'this->xc40_tongueAmp == 1.0',
      'this->xce4_orbitMsg', 'this->xce4_playerTouched'], ''),
    ('79_170 loader', 'PortLoadSwampBossStage2(', [], 'calls retp'),
    ('79_170 loader: model engaged (0x2E4)', 'loc:PortCSwampBossStage2.cpp:2478', ['modelData.m_valid'], ''),
]
