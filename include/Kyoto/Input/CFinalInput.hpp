#ifndef _CFINALINPUT
#define _CFINALINPUT

#include <types.h>

#ifdef __MWERKS__
#pragma cpp_extensions on
#endif

class CControllerGamepadData;
class COsContext;

class CFinalInput {
  static const float kInput_AnalogOnThreshhold;
  static const float kInput_AnalogTriggerOnThreshhold;

public:
  CFinalInput();
  CFinalInput(int channel, float dt, const CControllerGamepadData& data, float leftDif,
              float rightDiv);
  CFinalInput(int channel, float dt, const COsContext& context);
  CFinalInput(const CFinalInput& other)
  : mDt(other.mDt)
  , mControllerIdx(other.mControllerIdx)
  , mAnaLeftX(other.mAnaLeftX)
  , mAnaLeftY(other.mAnaLeftY)
  , mAnaRightX(other.mAnaRightX)
  , mAnaRightY(other.mAnaRightY)
  , mAnaLeftTrigger(other.mAnaLeftTrigger)
  , mAnaRightTrigger(other.mAnaRightTrigger)
  , mAnaLeftTriggerP(other.mAnaLeftTriggerP)
  , mAnaRightTriggerP(other.mAnaRightTriggerP)
  , btns1(other.btns1)
  , btns2(other.btns2)
  , btns3(other.btns3)
  , btns4(other.btns4) {}

  void InitializeAnalog(float leftDiv, float rightDiv);

  CFinalInput ScaleAnalogueSticks(float leftDiv, float rightDiv) const;

  float DeltaTime() const { return mDt; }

  void SetDeltaTime(float dt) { mDt = dt; }

  float GetAnalogLeftX() const { return mAnaLeftX; }
  float GetAnalogLeftY() const { return mAnaLeftY; }
  float GetAnalogRightX() const { return mAnaRightX; }
  float GetAnalogRightY() const { return mAnaRightY; }

  int ControllerNumber() const { return mControllerIdx; }

  float ALAUp() const { return mAnaLeftY > 0.f ? mAnaLeftY : 0.f; }

  float ALADown() const { return mAnaLeftY < 0.f ? -mAnaLeftY : 0.f; }

  float ALALeft() const { return mAnaLeftX < 0.f ? -mAnaLeftX : 0.f; }

  float ALARight() const { return mAnaLeftX > 0.f ? mAnaLeftX : 0.f; }

  float ARAUp() const { return mAnaRightY > 0.f ? mAnaRightY : 0.f; }

  float ARADown() const { return mAnaRightY < 0.f ? -mAnaRightY : 0.f; }

  float ARALeft() const { return mAnaRightX < 0.f ? -mAnaRightX : 0.f; }

  float ARARight() const { return mAnaRightX > 0.f ? mAnaRightX : 0.f; }

  float ALTrigger() const { return mAnaLeftTrigger; }

  float ARTrigger() const { return mAnaRightTrigger; }

  float ADPUp() const { return mB27_DPUp ? 1.f : 0.f; }

  float ADPDown() const { return mB29_DPDown ? 1.f : 0.f; }

  float ADPLeft() const { return mB30_DPLeft ? 1.f : 0.f; }

  float ADPRight() const { return mB28_DPRight ? 1.f : 0.f; }

  float AA() const { return mB28_A ? 1.f : 0.f; }

  float AB() const { return mB29_B ? 1.f : 0.f; }

  float AX() const { return mB30_X ? 1.f : 0.f; }

  float AY() const { return mB31_Y ? 1.f : 0.f; }

  float AZ() const { return mB24_Z ? 1.f : 0.f; }

  float AL() const { return mB25_L ? 1.f : 0.f; }

  float AR() const { return mB26_R ? 1.f : 0.f; }

  float AStart() const { return mB31_Start ? 1.f : 0.f; }

  bool DLAUp() const { return kInput_AnalogOnThreshhold < mAnaLeftY ? true : false; }

  bool DLADown() const { return -kInput_AnalogOnThreshhold > mAnaLeftY ? true : false; }

  bool DLALeft() const { return -kInput_AnalogOnThreshhold > mAnaLeftX ? true : false; }

  bool DLARight() const { return kInput_AnalogOnThreshhold < mAnaLeftX ? true : false; }

  bool DRAUp() const { return kInput_AnalogOnThreshhold < mAnaRightY ? true : false; }

  bool DRADown() const { return -kInput_AnalogOnThreshhold > mAnaRightY ? true : false; }

  bool DRALeft() const { return -kInput_AnalogOnThreshhold > mAnaRightX ? true : false; }

  bool DRARight() const { return kInput_AnalogOnThreshhold < mAnaRightX ? true : false; }

  bool DLTrigger() const {
    return kInput_AnalogTriggerOnThreshhold < mAnaLeftTrigger ? true : false;
  }

  bool DRTrigger() const {
    return kInput_AnalogTriggerOnThreshhold < mAnaRightTrigger ? true : false;
  }

  bool DDPUp() const { return mB27_DPUp; }

  bool DDPDown() const { return mB29_DPDown; }

  bool DDPLeft() const { return mB30_DPLeft; }

  bool DDPRight() const { return mB28_DPRight; }

  bool DA() const { return mB28_A; }

  bool DB() const { return mB29_B; }

  bool DX() const { return mB30_X; }

  bool DY() const { return mB31_Y; }

  bool DZ() const { return mB24_Z; }

  bool DL() const { return mB25_L; }

  bool DR() const { return mB26_R; }

  bool DStart() const { return mB31_Start; }

  bool PLAUp() const { return kInput_AnalogOnThreshhold < mAnaLeftY && mB25_enableAnaLeftYP ? true : false; }

  bool PLADown() const { return -kInput_AnalogOnThreshhold > mAnaLeftY && mB25_enableAnaLeftYP ? true : false; }

  bool PLALeft() const { return -kInput_AnalogOnThreshhold > mAnaLeftX && mB24_enableAnaLeftXP ? true : false; }

  bool PLARight() const { return kInput_AnalogOnThreshhold < mAnaLeftX && mB24_enableAnaLeftXP ? true : false; }

  bool PRAUp() const { return kInput_AnalogOnThreshhold < mAnaRightY && mB27_enableAnaRightYP ? true : false; }

  bool PRADown() const { return -kInput_AnalogOnThreshhold > mAnaRightY && mB27_enableAnaRightYP ? true : false; }

  bool PRALeft() const { return -kInput_AnalogOnThreshhold > mAnaRightX && mB26_enableAnaRightXP ? true : false; }

  bool PRARight() const { return kInput_AnalogOnThreshhold < mAnaRightX && mB26_enableAnaRightXP ? true : false; }

  bool PLTrigger() const { return kInput_AnalogTriggerOnThreshhold < mAnaLeftTriggerP ? true : false; }

  bool PRTrigger() const { return kInput_AnalogTriggerOnThreshhold < mAnaRightTriggerP ? true : false; }

  bool PDPUp() const { return mB31_PDPUp; }
  
  bool PDPDown() const { return mB25_PDPDown; }
  
  bool PDPLeft() const { return mB26_PDPLeft; }
  
  bool PDPRight() const { return mB24_PDPRight; }

  bool PA() const { return mB24_PA; }

  bool PB() const { return mB25_PB; }

  bool PX() const { return mB26_PX; }

  bool PY() const { return mB27_PY; }

  bool PZ() const { return mB28_PZ; }

  bool PL() const { return mB29_PL; }

  bool PR() const { return mB30_PR; }

  bool PStart() const { return mB27_PStart; }
#ifdef TARGET_PC
  // Hold/toggle options (port_hold_toggle.h) override a button after the fact. L covers both the
  // digital click and the analog trigger, since the game's lock-on and scan commands read either.
  void PortSetL(bool held, bool pressed) {
    mB25_L = held;
    mB29_PL = pressed;
    mAnaLeftTrigger = held ? 1.f : 0.f;
    mAnaLeftTriggerP = pressed ? 1.f : 0.f;
  }
  void PortSetA(bool held, bool pressed) {
    mB28_A = held;
    mB24_PA = pressed;
  }
  // Pointer menus (port_pointer.h) turn a click or right click into a press.
  void PortSetB(bool held, bool pressed) {
    mB29_B = held;
    mB25_PB = pressed;
  }
  void PortSetX(bool held, bool pressed) {
    mB30_X = held;
    mB26_PX = pressed;
  }
  void PortSetY(bool held, bool pressed) {
    mB31_Y = held;
    mB27_PY = pressed;
  }
  void PortSetR(bool held, bool pressed) {
    mB26_R = held;
    mB30_PR = pressed;
    mAnaRightTrigger = held ? 1.f : 0.f;
    mAnaRightTriggerP = pressed ? 1.f : 0.f;
  }
  void PortSetStart(bool held, bool pressed) {
    mB31_Start = held;
    mB27_PStart = pressed;
  }
  // A left stick Y press (paging): value is +1 up, -1 down.
  void PortPressLeftStickY(float value) {
    mAnaLeftY = value;
    mB25_enableAnaLeftYP = true;
  }
#endif

private:
  float mDt;
  int mControllerIdx;
  float mAnaLeftX;
  float mAnaLeftY;
  float mAnaRightX;
  float mAnaRightY;
  float mAnaLeftTrigger;
  float mAnaRightTrigger;
  float mAnaLeftTriggerP;
  float mAnaRightTriggerP;

  // Echoes packs Prime's analog press flags into the button bitfield.
  union {
    struct {
      uchar mB24_enableAnaLeftXP : 1;
      uchar mB25_enableAnaLeftYP : 1;
      uchar mB26_enableAnaRightXP : 1;
      uchar mB27_enableAnaRightYP : 1;
      uchar mB28_A : 1;
      uchar mB29_B : 1;
      uchar mB30_X : 1;
      uchar mB31_Y : 1;
    };
    uchar btns1;
  };
  union {
    struct {
      uchar mB24_Z : 1;
      uchar mB25_L : 1;
      uchar mB26_R : 1;
      uchar mB27_DPUp : 1;
      uchar mB28_DPRight : 1;
      uchar mB29_DPDown : 1;
      uchar mB30_DPLeft : 1;
      uchar mB31_Start : 1;
    };
    uchar btns2;
  };
  union {
    struct {
      uchar mB24_PA : 1;
      uchar mB25_PB : 1;
      uchar mB26_PX : 1;
      uchar mB27_PY : 1;
      uchar mB28_PZ : 1;
      uchar mB29_PL : 1;
      uchar mB30_PR : 1;
      uchar mB31_PDPUp : 1;
    };
    uchar btns3;
  };
  union {
    struct {
      uchar mB24_PDPRight : 1;
      uchar mB25_PDPDown : 1;
      uchar mB26_PDPLeft : 1;
      uchar mB27_PStart : 1;
    };
    uchar btns4;
  };
};

CHECK_SIZEOF(CFinalInput, 0x2c)

#ifdef __MWERKS__
#pragma cpp_extensions reset
#endif

#endif // _CFINALINPUT
