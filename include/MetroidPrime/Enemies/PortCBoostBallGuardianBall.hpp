#ifndef _PORTCBOOSTBALLGUARDIANBALL
#define _PORTCBOOSTBALLGUARDIANBALL

// Port-only: the IngBoostBallGuardian REL's second class (module 30), the rolling ball the Boost
// Guardian turns into for its boost attack. 0x5E0 bytes on the GameCube, a CPhysicsActor built by
// fn_30_15924 (vtable lbl_30_data_C44) and spawned by the guardian's fn_30_3478 under the name
// "BoostBallGuardian". Upstream has no source for it; the map of its functions is
// docs/module-analysis/BoostGuardian.md section f.
//
// The guest offsets quoted are the REL's own; the host layout is its own (the members follow the
// guest order, the flag bytes are one-bit members).

#include "Collision/CCollidableSphere.hpp"
#include "Kyoto/Audio/CSfxHandle.hpp"
#include "Kyoto/Math/CVector3f.hpp"
#include "Kyoto/Particles/CParticleGen.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CPhysicsActor.hpp"
#include "MetroidPrime/Enemies/CIngBoostBallGuardian.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include "rstl/optional_object.hpp"
#include "rstl/single_ptr.hpp"
#include "rstl/vector.hpp"

class CBoostBallGuardianBall : public CPhysicsActor {
public:
  // The DOL's type tag for this class (TypesMatch.cpp's "148", parent CPhysicsActor); the
  // guardian's BoostAttack casts its child with it (fn_800979F0 = TryCast(entity, 0x94)).
  static const int kTypeId = 148;

  // fn_30_15924. The guest passes the material list and the health info by pointer; either may be
  // null (0x00015998, 0x00015CB8).
  CBoostBallGuardianBall(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                         const CTransform4f& xf, const CVector3f& pos, const CModelData& modelData,
                         const CActorParameters& actParams, TUniqueId ownerId,
                         const CBoostBallGuardianData& data, int phaseIndex,
                         const CDamageVulnerability& vulnerability,
                         const rstl::vector< TUniqueId >& modifyTargets,
                         const CMaterialList* matList, const CHealthInfo* health);
  ~CBoostBallGuardianBall() override;

  // CEntity
  CEntity* TypesMatch(int typeId) const override;
  void Think(float dt, CStateManager& mgr) override;

  // CActor
  CHealthInfo* HealthInfo() override;
  const CHealthInfo* GetHealthInfo() const override;
  const CDamageVulnerability* GetDamageVulnerability() const override;
  const CDamageVulnerability* GetDamageVulnerability(const CVector3f& pos, const CVector3f& dir,
                                                     const CDamageInfo& info) const override;
  rstl::optional_object< CAABox > GetTouchBounds() const override;
  void Touch(CActor& actor, CStateManager& mgr) override;

  // CPhysicsActor
  const CCollisionPrimitive* GetCollisionPrimitive() const override;

  // fn_30_13B98: the shrink is over (guest 0x5D8 bit 0x04); BoostAttack's Update turns it into
  // the guardian's IsBoostOver.
  bool IsDone() const { return mDone; }
  // BoostAttack's Deactivate sets guest 0x5D8 bit 0x01 (0x00007AC4).
  void SetHalted(bool halted) { mHalted = halted; }

private:
  int mPhaseIndex;                                     // 0x2D0: the guardian's phase at spawn
  int mState;                                          // 0x2D4: 0 grow in, 1 roll, 2 shrink
  CVector3f mHomePos;                                  // 0x2D8
  CBoostBallGuardianData mData;                        // 0x2E4: the guardian's boost data
  CCollidableSphere mSphere;                           // 0x4D0
  TUniqueId mOwnerId;                                  // 0x4F0: the guardian
  TUniqueId mId4F2;                                    // 0x4F2
  TUniqueId mId4F4;                                    // 0x4F4
  TUniqueId mId4F6;                                    // 0x4F6: AcceptScriptMsg's sender (0x000146F8)
  TUniqueId mTouchedId;                                // 0x4F8: Think's damage target
  rstl::vector< TUniqueId > mModifyTargets;            // 0x4FC: the guardian's 0x109C
  float mDamageTimer;                                  // 0x50C, 1.0 (guessed name)
  float m510;                                          // 0x510, 0.0
  float mSplineDuration;                               // 0x514: the speed spline's duration
  float mHalfHeight;                                   // 0x518: half the sphere's local height
  float mSplineTime;                                   // 0x51C: the roll's time on the spline
  float m520;                                          // 0x520, 0.4
  float m524;                                          // 0x524, 0.0
  float mSplineEndTime;                                // 0x528
  float mStateTime;                                    // 0x52C: zeroed on each state change
  float m530;                                          // 0x530, 1.0
  CVector3f mScale;                                    // 0x534, (1, 1, 1)
  CVector3f m540;                                      // 0x540, zero
  CVector3f m54C;                                      // 0x54C, zero
  int m558;                                            // 0x558: index into m5C8
  int m55C;                                            // 0x55C
  int m560;                                            // 0x560: a counter the roll increments
  rstl::single_ptr< CParticleGen > mTrailGen;          // 0x564: data+0xA8 trail ('SPSC')
  rstl::single_ptr< CParticleGen > mShieldGen;         // 0x568: data+0xAC shield ('PART')
  rstl::single_ptr< CParticleGen > mChargeUpGen;       // 0x56C: data+0xB0 charge up ('PART')
  CDamageVulnerability mVulnerability;                 // 0x570
  rstl::optional_object< CHealthInfo > mHealthInfo;    // 0x5A0, present flag 0x5C0
  CSfxHandle mRollingSfx;                              // 0x5C4: fn_30_1282C's handle
  rstl::vector< int > m5C8;                            // 0x5C8: an int list the roll compares
                                                       // with m558 (0x00012CE4); unread so far
  bool m5D8_80 : 1;                                    // 0x5D8 bit 0x80
  bool m5D8_40 : 1;                                    // 0x5D8 bit 0x40
  bool m5D8_20 : 1;                                    // 0x5D8 bit 0x20
  bool m5D8_10 : 1;                                    // 0x5D8 bit 0x10
  bool m5D8_08 : 1;                                    // 0x5D8 bit 0x08
  bool mDone : 1;                                      // 0x5D8 bit 0x04
  bool m5D8_02 : 1;                                    // 0x5D8 bit 0x02
  bool mHalted : 1;                                    // 0x5D8 bit 0x01
  bool m5D9_80 : 1;                                    // 0x5D9 bit 0x80
  bool m5D9_40 : 1;                                    // 0x5D9 bit 0x40
  bool m5D9_20 : 1;                                    // 0x5D9 bit 0x20, set by the ctor
  bool m5D9_10 : 1;                                    // 0x5D9 bit 0x10, set by the ctor
};

template <>
CBoostBallGuardianBall* TCastToPtr< CBoostBallGuardianBall >(CEntity* entity);

#endif // _PORTCBOOSTBALLGUARDIANBALL
