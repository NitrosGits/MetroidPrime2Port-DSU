#ifndef _CINGSPOTDATA
#define _CINGSPOTDATA

#include "Kyoto/SObjectTag.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"

class CIngSpotData {
public:
  CIngSpotData(CAssetId blobEffect, CAssetId normalHitEffect, CAssetId heavyHitEffect,
               CAssetId deathEffect, float maxSpeed, float maxWallSpeed, float ballPursuitSpeed,
               float unknown1c, float turnSpeed, const CDamageVulnerability& vulnerability,
               ushort idleSound, ushort moveSound, ushort normalHitSound, ushort heavyHitSound,
               ushort deathSound);
#ifdef TARGET_PC
  CAssetId GetBlobEffect() const { return mBlobEffect; }
  CAssetId GetNormalHitEffect() const { return mNormalHitEffect; }
  CAssetId GetHeavyHitEffect() const { return mHeavyHitEffect; }
#endif

private:
  CAssetId mBlobEffect;
  CAssetId mNormalHitEffect;
  CAssetId mHeavyHitEffect;
  CAssetId mDeathEffect;
  float mMaxSpeed;
  float mMaxWallSpeed;
  float mBallPursuitSpeed;
  float x1c_;
  float mTurnSpeed;
  CDamageVulnerability mVulnerability;
  ushort mIdleSound;
  ushort mMoveSound;
  ushort mNormalHitSound;
  ushort mHeavyHitSound;
  ushort mDeathSound;
};

CHECK_SIZEOF(CIngSpotData, 0x60)

#endif // _CINGSPOTDATA
