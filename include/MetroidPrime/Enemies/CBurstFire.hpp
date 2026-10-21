#ifndef _CBURSTFIRE
#define _CBURSTFIRE

#include "Kyoto/Math/CVector3f.hpp"

#include "rstl/reserved_vector.hpp"

class CStateManager;

struct SBurst {
  int mRandomSelectionWeight;
  int mShotAngles[8];
  float mTimeToNextShot;
  float mTimeToNextShotVariance;
};
CHECK_SIZEOF(SBurst, 0x2c)

class CBurstFire {
  int mBurstType;
  int mAngleIdx;
  float mTimeToNextShot;
  int mFirstBurstIdx;
  int mFirstBurstCounter;
  bool mShouldFire : 1;
  bool mAvoidAccuracy : 1;
  const SBurst* mCurBursts;
  rstl::reserved_vector< const SBurst*, 16 > mBurstDefs;

public:
  CBurstFire(const SBurst** burstDefs, int firstBurstCount);

  void SetBurstType(int type) { mBurstType = type; }
  void SetFirstBurstIndex(int idx) { mFirstBurstIdx = idx; }
  int GetBurstType() const { return mBurstType; }
  bool IsBurstSet() const { return mCurBursts != nullptr; }
  bool ShouldFire() const { return mShouldFire; }
  void SetAvoidAccuracy(bool avoid) { mAvoidAccuracy = avoid; }
  void SetTimeToNextShot(float time) {
    if (mTimeToNextShot > 0.f) {
      mTimeToNextShot = time;
    }
  }
#ifdef TARGET_PC
  // The port's own getter: the CommandoPirate's burst-fire helper (fn_9_6390) reads the timer
  // before it writes it, the way the guest reads the float at its own +0xAF8.
  float GetTimeToNextShot() const { return mTimeToNextShot; }
#endif

  void Start(CStateManager& mgr);
  void Update(CStateManager& mgr, float dt);

  CVector3f GetError(float xMag, float zMag) const;
  CVector3f GetDistanceCompensatedError(float dist, float maxErrDist) const;
  float GetMaxXError() const;
  float GetMaxZError() const;
};
CHECK_SIZEOF(CBurstFire, 0x60)

#endif // _CBURSTFIRE
