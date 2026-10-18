#ifndef _CGAMEARCHITECTURESUPPORT
#define _CGAMEARCHITECTURESUPPORT

#include "types.h"

#include "Kyoto/Audio/CAudioSys.hpp"
#include "Kyoto/Basics/COsContext.hpp"
#include "Kyoto/Basics/CStopwatch.hpp"
#include "Kyoto/TOneStatic.hpp"

#include "MetroidPrime/CArchitectureQueue.hpp"
#include "MetroidPrime/CIOWinManager.hpp"
#include "MetroidPrime/CInputGenerator.hpp"
#ifdef TARGET_PC
#include "port_timing.h"
#endif

class CGameArchitectureSupport : public TOneStatic< CGameArchitectureSupport > {
public:
  CGameArchitectureSupport(COsContext&);
  ~CGameArchitectureSupport();

  void PreloadAudio();
  bool UpdateTicks();
  void Update();
  void UnloadAudio();

  inline CStopwatch& GetStopwatch1() { return mTickStopwatch; }
  inline CStopwatch& GetStopwatch2() { return mDrawStopwatch; }
  inline CIOWinManager& GetIOWinManager() { return mIoWinMgr; }
  inline int& GetFramesDrawn() { return mGameFrameCount; }
  OSAlarm& GetInfiniteLoopAlarm() { return mInfiniteLoopAlarm; }
  bool IsInfiniteLoopAlarmSet() const { return mInfiniteLoopAlarmSet; }
  void SetInfiniteLoopAlarmSet(bool set) { mInfiniteLoopAlarmSet = set; }
#ifdef TARGET_PC
  // Port: MP1's fixed-step clock. With the 60 FPS cap off, UpdateTicks runs the whole 1/60 s ticks
  // the elapsed time holds; the leftover fraction is how far the drawn frame is toward the next.
  float GetTickInterpolation() const { return mPortTickClock.Interpolation(); }
#endif

private:
  CAudioSys mAudioSys;
  CArchitectureQueue mArchQueue;
  CStopwatch mTickStopwatch;
  CStopwatch mDrawStopwatch;
  CInputGenerator mInputGenerator;
  CIOWinManager mIoWinMgr;
  int mGameFrameCount;
  float mTickRemainder;
  float mPreviousTickRemainder2;
  float mPreviousTickRemainder;
  OSAlarm mInfiniteLoopAlarm;
  bool mInfiniteLoopAlarmSet;
#ifdef TARGET_PC
  PortTiming::FixedStepClock mPortTickClock;
#endif
};
CHECK_SIZEOF(CGameArchitectureSupport, 0xa8)

#endif // _CGAMEARCHITECTURESUPPORT
