// Port-only: the EmperorIngStage3 REL's light swarm (module 18, docs/module-analysis/
// EmperorIngStage3.md). The retail class is 0x618 bytes; its vtable lbl_18_data_798 keeps
// CSwarmBasics's entries except the destructor (fn_18_DA14), Think (fn_18_B254),
// AcceptScriptMsg (fn_18_B1B0), Render (fn_18_AF9C), CanRenderUnsorted (fn_18_AF94),
// PreRenderAllViewports (fn_18_B0D0), HealthInfo (fn_18_DAE4), GetHealthInfo (fn_18_AEAC),
// GetTouchBounds (fn_18_AE38), GetBoundingBox (fn_18_AEB4), CreateBoid (fn_18_BC1C),
// PreRenderBoid (fn_18_B618), UpdateBoid (fn_18_B884), GetLockOnLocationValid (fn_18_B588),
// GetLockOnLocation (fn_18_B5CC), RenderBoid (fn_18_B614) and UpdateAllBoidMovement
// (fn_18_B7A8). The swarm is one homing point (the actor's translation, guest 0x54) that the
// boids chase; it dies when shot to zero health or 20 seconds after it was spawned. The
// fn_80_ calls are module 80's CSwarmBasics (config/G2ME01/rels/SwarmBasics/symbols.txt).

#include "MetroidPrime/Enemies/CEmperorIngLightSwarm.hpp"

#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CAnimRes.hpp"
#include "MetroidPrime/CEntityInfo.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"

#include "rstl/math.hpp"

// @ fn_18_817C
CTransform4f EmperorIngSwarmLocator(const CActor& owner) {
  return owner.GetTransform() * owner.GetScaledLocatorTransform(rstl::string_l("Swarm_LCTR_SDK"));
}

// The name is rodata 0x728 (0x340 + 0x3E8); the swarm's box extent is sOneVector, its
// transform starts at sIdentity and it has no model (CAnimRes(-1, -1, sOneVector, 0, true),
// 0xBDB0..0xBDDC). The base's `animated` argument is false (the stack word at 0xBDF4).
// @ fn_18_BD84
CEmperorIngLightSwarm::CEmperorIngLightSwarm(TUniqueId uid, const CEntityInfo& info,
                                             const CActorParameters& params,
                                             const CBasicSwarmData& data, CAssetId particleId,
                                             TUniqueId ownerId, const CTransform4f& xf,
                                             const SLdrAudioPlaybackParms& deathSound,
                                             const SLdrAudioPlaybackParms& expireSound)
: CSwarmBasics(uid, rstl::string_l("Emperor Ing Stage 3 Swarm"), info, CVector3f::One(),
               CTransform4f::Identity(),
               CAnimRes(kInvalidAssetId, -1, CVector3f::One(), 0, true), params, data, false)
, mOwnerId(ownerId)                                                     // 0xBE48
, mEffect(nullptr)                                                      // 0xBE58
, mDirection(xf.Get01(), xf.Get11(), xf.Get21())                        // the Y column, 0xBE50
, mOwnHealth(30.f, 0.f)                                                 // rodata 0x2FC, 0x258
, mDyingTime(0.f)                                                       // 0xBE80
, mAge(0.f)                                                             // 0xBE84
, mDeathSound(deathSound)                                               // 0xBE88..0xBEAC
, mExpireSound(expireSound)                                             // 0xBEB0..0xBED4
, mDamageTime(0.f) {                                                    // 0xBED8
  SetTransform(xf);                                                     // 0xBEDC
  if (particleId != kInvalidAssetId) {                                  // 0xBEE0
    mEffectDesc = rstl::optional_object< TLockedToken< CGenDescription > >(
        TLockedToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', particleId))));
    mEffect = rs_new CElementGen(*mEffectDesc); // kMOT_Normal, kOSF_One (0xC048)
    mEffect->SetParticleEmission(false);        // 0xC0A4
  }
  x560_ = 60; // 0xC0A8
}

// fn_18_DA14
CEmperorIngLightSwarm::~CEmperorIngLightSwarm() {}

// The point dies once its own health is gone or it is older than 20 seconds (rodata 0x270).
// @ fn_18_B61C
bool CEmperorIngLightSwarm::IsDying() const {
  return mOwnHealth.GetHP() <= 0.f || mAge > 20.f;
}

// @ fn_18_B254
void CEmperorIngLightSwarm::Think(float dt, CStateManager& mgr) {
  CSwarmBasics::Think(dt, mgr); // fn_80_50B8
  mAge += dt;
  mDamageTime = rstl::max_val(0.f, mDamageTime - dt);
  if (!IsDying()) {
    // Turn the heading toward the player's eye, at up to 90 degrees per second (rodata 0x2CC)
    // ramping in over the first 3 seconds (rodata 0x284); below the eye the climb is kept,
    // above it the dive is flattened (0xB2D4..0xB318).
    CVector3f toEye = mgr.Player(0)->GetEyePosition() - GetTranslation();
    if (GetTranslation().GetZ() < mgr.Player(0)->GetEyePosition().GetZ() && toEye.GetZ() < 0.f) {
      toEye.SetZ(0.f);
    }
    if (toEye.CanBeNormalized()) {
      toEye.Normalize();
      const float ramp = rstl::min_val(mAge / 3.f, 1.f);
      mDirection = ShortestRotationArcWrapped(
                       mDirection, toEye,
                       CRelAngle::FromRadians((M_PIF / 180.f) * ((90.f * ramp) * dt))) *
                   mDirection;
    }
    // 5 units per second (rodata 0x2BC).
    SetTranslation(GetTranslation() + dt * (5.f * mDirection));
  } else {
    if (CMath::AbsF(mDyingTime - 0.f) < 1e-5f) { // rodata 0x32C
      // The first dying frame: the swarm reports its death at the point it reached.
      x4f0_26_ = false;
      mLastKilledOffset = GetTranslation();
      SendScriptMsgs(kSS_Dead, mgr, kInvalidUniqueId, kSM_None);
      PlayCustomSound(GetTranslation(), CVector3f::Zero(), mDeathSound, false);
    } else if (CMath::AbsF(mAge - 20.f) < 1e-5f) {
      PlayCustomSound(GetTranslation(), CVector3f::Zero(), mExpireSound, false);
    }
    mDyingTime += dt;
    if (mDyingTime > 2.f) { // rodata 0x274
      mgr.DeleteObjectRequest(GetUniqueId());
    }
  }
  if (mEffect.get() != nullptr) {
    mEffect->Update(dt);
  }
}

// fn_18_B1B0
void CEmperorIngLightSwarm::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CSwarmBasics::AcceptScriptMsg(mgr, msg); // fn_80_81C8
  switch (msg.GetMessage()) {
  case kSM_Create:
    AddMaterial(kMT_Solid, kMT_SeekerTarget, mgr);
    RemoveMaterial(kMT_NonSolidDamageable, mgr);
    break;
  case kSM_Damage:
    mDamageTime = CPatterned::skDamageHitTime;
    break;
  default:
    break;
  }
}

// The effect fades to black over the first two dying seconds and flashes the damage colour
// for skDamageHitTime after a hit.
// fn_18_AF9C
void CEmperorIngLightSwarm::Render(const CStateManager& mgr) const {
  CSwarmBasics::Render(mgr); // fn_80_3B34
  if (CElementGen* effect = mEffect.get()) {
    const float fade = rstl::min_val(mDyingTime * 0.5f, 1.f); // rodata 0x2A0, 0x264
    const float flash =
        rstl::max_val(0.f, rstl::min_val(mDamageTime / CPatterned::skDamageHitTime, 1.f));
    const CColor& black = CColor::Black();
    const CColor& white = CColor::White();
    effect->SetModulationColor(
        CColor::Lerp(CColor::Lerp(white, CPatterned::skDamageColor, flash), black, fade));
    mEffect->Render();
  }
}

// fn_18_AF94
bool CEmperorIngLightSwarm::CanRenderUnsorted(const CStateManager&) const { return false; }

// fn_18_B0D0
void CEmperorIngLightSwarm::PreRenderAllViewports(CStateManager& mgr) {
  CSwarmBasics::PreRenderAllViewports(mgr); // fn_80_7618
  if (mEffect.get() != nullptr) {
    const rstl::optional_object< CAABox > bounds = mEffect->GetBounds();
    if (bounds) {
      SetOtherBounds(*bounds);  // guest 0xCC
      SetRenderBounds(*bounds); // guest 0xE4
    }
  }
}

CHealthInfo* CEmperorIngLightSwarm::HealthInfo() { return &mOwnHealth; }

// fn_18_AEAC
const CHealthInfo* CEmperorIngLightSwarm::GetHealthInfo() const { return &mOwnHealth; }

// fn_18_AE38
rstl::optional_object< CAABox > CEmperorIngLightSwarm::GetTouchBounds() const {
  if (mOwnHealth.GetHP() > 0.f) {
    return CSwarmBasics::GetTouchBounds(); // fn_80_7544
  }
  return rstl::optional_object< CAABox >();
}

// fn_18_AEB4
CAABox CEmperorIngLightSwarm::GetBoundingBox() const {
  if (mEffect.get() != nullptr) {
    const rstl::optional_object< CAABox > bounds = mEffect->GetBounds();
    if (bounds) {
      return *bounds;
    }
  }
  return CSwarmBasics::GetBoundingBox(); // fn_80_7580
}

// Every boid starts at the owner's swarm locator, with the swarm's health and its age as the
// boid's life time. Without an owner the boid is left as it was (0xBC48).
// @ fn_18_BC1C
void CEmperorIngLightSwarm::CreateBoid(CStateManager& mgr, int index) {
  const CEntity* owner = mgr.GetObjectById(mOwnerId);
  if (owner == nullptr) {
    return;
  }
  CBoid& boid = mBoids[index];
  boid.mTransform = EmperorIngSwarmLocator(*static_cast< const CActor* >(owner)); // fn_18_817C
  boid.mActive = true;
  boid.mVelocity = CVector3f::Zero();
  boid.mTargetWaypoint = kInvalidUniqueId;
  boid.mFramesNotOnSurface = 0;
  boid.mFreezeTimer = 0.f;
  boid.mExplodeTimerEnabled = false;
  boid.mHealth = mHealthInfo.GetHP(); // guest 0x42C
  boid.mHasLoopedSound = false;
  boid.mLifeTime = mAge;
  boid.xa8_ = kInvalidUniqueId;
  boid.xaa_ = kInvalidUniqueId;
  boid.mPartitionIndex = -1;
  boid.xb1_ = 0;
}

void CEmperorIngLightSwarm::PreRenderBoid(CBoid*, uint*) {}

// The boids steer to the point (radius 150, rodata 0x288) while it lives, scatter from it
// (radius 1000, rodata 0x260) once it expires and from the swarm's touch-box centre, twice as
// noisily, once it is shot. Noise wobbles each boid off its heading, which turns at up to 720
// degrees per second (rodata 0x330).
// fn_18_B884
void CEmperorIngLightSwarm::UpdateBoid(CAreaCollisionCache&, CStateManager& mgr, float dt,
                                       CBoid& boid, int) {
  CVector3f ahead = 0.3f * boid.GetTransform().GetForward(); // rodata 0x268
  float wobble = 0.5f;                                       // rodata 0x2A0
  if (mOwnHealth.GetHP() <= 0.f) {
    const rstl::optional_object< CAABox > bounds = CSwarmBasics::GetTouchBounds();
    if (bounds) {
      ApplySeparation(boid, bounds->GetCenterPoint(), 1000.f, 1.f, ahead); // fn_80_383C
      wobble = 2.f;                                                        // rodata 0x274
    }
  } else if (mAge > 20.f) {
    ApplySeparation(boid, GetTranslation(), 1000.f, 1.f, ahead); // fn_80_383C
  } else {
    ApplyAttraction(boid, GetTranslation(), 150.f, 1.f, ahead); // fn_80_3504
  }
  UpdateLightComboBeam(boid, mgr); // fn_80_6B14
  const CVector3f pos = boid.GetTranslation();
  const float noiseRight = CMath::Noise3d(pos.GetX(), pos.GetY(), pos.GetZ());
  const float noiseUp = CMath::Noise3d(pos.GetY(), pos.GetX(), pos.GetZ());
  ahead += wobble * (noiseRight * boid.GetTransform().GetRight() +
                     noiseUp * boid.GetTransform().GetUp())
                        .AsNormalized();
  boid.mTransform =
      ShortestRotationArcWrapped(boid.GetTransform().GetForward(), ahead.AsNormalized(),
                                 CRelAngle::FromRadians((M_PIF / 180.f) * (720.f * dt)))
          .MultiplyIgnoreTranslation(boid.GetTransform()); // fn_80_A4B0
}

// fn_18_B588
bool CEmperorIngLightSwarm::GetLockOnLocationValid(int) const {
  return mOwnHealth.GetHP() > 0.f;
}

// Lock-on sits 1.5 units (rodata 0x29C) behind the point.
// fn_18_B5CC
CVector3f CEmperorIngLightSwarm::GetLockOnLocation(int) const {
  return GetTranslation() - 1.5f * mDirection;
}

// fn_18_B614
void CEmperorIngLightSwarm::RenderBoid(CBoid*) const {}

// Every boid moves forward at the swarm's speed (guest 0x1CC), half again as fast once dying.
// @ fn_18_B7A8
void CEmperorIngLightSwarm::UpdateAllBoidMovement(CStateManager& mgr, float dt) {
  const float speed = IsDying() ? 1.5f * mAnimPlaybackSpeed : mAnimPlaybackSpeed;
  for (int i = 0; i < mBoids.size(); ++i) {
    MoveSwarmBoid(mgr, mBoids[i], speed * CVector3f(CVector3f::Forward()), dt);
  }
}

// @ fn_18_B694
// fn_18_B694: an active boid moves by its frame's offset in its own frame, and the effect is
// moved to it and made to emit one particle there.
void CEmperorIngLightSwarm::MoveSwarmBoid(CStateManager&, CBoid& boid, const CVector3f& delta,
                                          float dt) {
  if (!boid.mActive) {
    return;
  }
  boid.mVelocity = (1.f / dt) * boid.GetTransform().Rotate(delta);
  boid.mTransform.SetTranslation(boid.GetTranslation() + dt * boid.mVelocity);
  if (mEffect.get() != nullptr) {
    mEffect->SetTranslation(boid.GetTranslation());
    mEffect->ForceParticleCreation(1);
  }
}
