# Valid-unlock gallery fixture: set existing persistent environment variables,
# leaving GetGalleriesUnlocked and BuildGalleryLists fully native.
import gdb

class UnlockedValue(gdb.FinishBreakpoint):
    def __init__(self,value):
        super().__init__(gdb.newest_frame(),internal=True)
        self.value=value
    def stop(self):
        p=int(gdb.parse_and_eval('$rax'))
        assert p
        gdb.execute('set var ((CEnvironmentVariable*)%d)->mValue=%d'%(p,self.value))
        return False

class Vars(gdb.Breakpoint):
    def stop(self):
        name=gdb.parse_and_eval('(char*)$rsi').string()
        if name in ('PercentScans','NormalModeCompleted','HardModeCompleted'):
            UnlockedValue(100 if name=='PercentScans' else 1)
        return False

Vars('*_ZNK23CGameStateEnvVarManager23FindEnvironmentVariableEPKc',internal=True)
CLASS='CSlideShow'
SPEC=[('CEC ShowSlideShow','CScriptGuiFrontEndScreen::ShowSlideShow(',[],'max=4'),
    ('build unlocked flags','CSlideShow::BuildGalleryLists(',['$rsi'],'max=4'),
    ('loaded gallery','CSlideShow::LoadSlide(',['$this->mPhase','$this->mGalleries.mCount','$this->mTotalSlides','$this->mGallery','$this->mSlide'],'max=4'),
    ('draw slide','CSlideShow::SSlideData::Draw(',[],'max=4'),
    ('destroy slideshow','CSlideShow::~CSlideShow(',[],'max=4')]
