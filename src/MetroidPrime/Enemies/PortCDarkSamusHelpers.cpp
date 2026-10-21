// Port of the Dark Samus REL (module 10) helpers: the stateless functions of the module that are
// not FSM handlers (the ones below 0xA000). The function map, the caller and the arm tables are
// in docs/module-analysis/DarkSamus_helpers.md; the bodies marked `// @ fn_10_` were run in the
// Sanctuary 10_cliff_rooftop fight (boss u04f7).
#include "MetroidPrime/Enemies/CDarkSamus.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerGun.hpp"
#include "MetroidPrime/ScriptObjects/CHUDBillboardEffect.hpp"
#include "MetroidPrime/Tweaks/CTweakPlayerGun.hpp"
#include "Collision/CCollidableAABox.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Collision/CRayCastResult.hpp"
#include "Kyoto/Math/CCylinder.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/Weapons/CFreezeBeamProjectile.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
#include "Kyoto/Graphics/CGraphics.hpp"
#include "Kyoto/Graphics/CTexture.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CAnimPlaybackParms.hpp"
#include "MetroidPrime/CAnimRes.hpp"
#include "MetroidPrime/CPhysicsActor.hpp"
#include <stdio.h>
#include "MetroidPrime/Weapons/CEnergyProjectile.hpp"

// fn_10_9624 (0x00009624, 0x20 bytes): is the boost ball's uid (guest 0xE38) a real one? The
// compare is the "subf/or/srwi" idiom for `uid != kInvalidUniqueId` (0x00009630..0x00009640).
// fn_10_9624 (ported, unreferenced in the port: nothing calls it yet)
bool CDarkSamus::HasBoostBall() const {
  return mBoostBallUid != kInvalidUniqueId;
}

// fn_10_2EFC (0x00002EFC, 8 bytes): the float at guest 0x314 (0x00002EFC).
float CDarkSamus::GetGuest314() const { return mGuest314; }

// fn_10_2F04 (0x00002F04, 0xC bytes): bit 0x40 of the byte at guest 0x318 - the extrwi at
// 0x00002F08 takes the bit 25, which is 1u << (31 - 25) = 0x40.
bool CDarkSamus::GetGuest318Bit6() const { return (mGuest318 & 0x40) != 0; }

// fn_10_4868 (0x00004868, 0x60 bytes): the actor whose uid is at guest 0xF78 is deleted
// (0x000048A4) and the uid cleared (0x000048B0); a kInvalidUniqueId uid is left alone
// (0x00004890).
void CDarkSamus::DeleteGuestActor(CStateManager& mgr) {
  if (mGuestF78 == kInvalidUniqueId) { // 0x00004890
    return;
  }
  mgr.DeleteObjectRequest(mGuestF78); // 0x000048A4
  mGuestF78 = kInvalidUniqueId;       // 0x000048B0
}

// fn_10_48C8 (0x000048C8, 0x44 bytes): the actor at guest 0xF78 is deleted when the timestamp at
// 0xF7C is older than the elapsed time (0x000048F0 compares it with guest 0x8B0).
void CDarkSamus::DeleteGuestActorIfExpired(CStateManager& mgr) {
  if (mGuestF78 == kInvalidUniqueId) { // 0x000048E4
    return;
  }
  if (!(mGuestF7C < mElapsedTime)) {  // 0x000048F0
    return;
  }
  DeleteGuestActor(mgr); // 0x000048F8
}

// fn_10_812C (0x0000812C, 0x4C bytes): the sfx handle at guest 0xF60 is removed (0x00008158)
// and cleared (0x00008160); a null handle is left alone (0x00008148).
void CDarkSamus::StopGuestSfx() {
  if (mGuestF60 == CSfxHandle()) { // 0x00008148
    return;
  }
  CSfxManager::RemoveEmitter(mGuestF60); // 0x00008158
  mGuestF60 = CSfxHandle();               // 0x00008160
}

// fn_10_2E44 (0x00002E44, 0x20 bytes): the CModelData default constructor (0x00002E50).
// fn_10_2E44 (ported, unreferenced in the port: nothing calls it yet)
void CDarkSamus::InitGuestModelData(CModelData* model) { new (model) CModelData(); }

// fn_10_8698 (0x00008698, 0xCC bytes): the taunt command over a state's messages. Activate
// (0x000086D8) stores 1 (kAS_Ready) to the animation state 0x6B4 and delivers the command
// (0x000086F0); Update (0x00008708) re-delivers it when the DOL's ShouldRetry (0x00008710, the
// host's CAnimationState::CanIssueCommand) answers true; Deactivate (0x00008744) stores 0
// (kAS_NotReady). The guest's 0x6B4 is the base's animation state, which the host keeps in
// CPatterned::mAnimationState.
void CDarkSamus::RunTauntCmd(EStateMsg msg, pas::ETauntType type) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);      // 0x000086E0
    BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(type)); // 0x000086F0
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) { // 0x00008710
      BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(type));           // 0x0000872C
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x00008750
    break;
  default:
    break;
  }
}

// fn_10_6F90 (0x00006F90, 0xC4 bytes): the path find to the destination. The path find itself is
// fn_10_1C778 (the host's SetPathFindDestination, 0x00006FC0); when the path is shagged
// (0x00006FE0) the navigation's destination is set again (0x00006FF8 - the guest's 0x1048 is the
// navigation's own destination, which the host keeps in CPathFindNavigation) and the path find
// runs again (0x00007010).
// fn_10_6F90 (ported, unreferenced in the port: the states call the path find inline)
void CDarkSamus::FindPathToDestination(CStateManager& mgr, const CVector3f& destination, float dt) {
  SetPathFindDestination(mgr, destination, dt); // fn_10_1C778 at 0x00006FC0
  if (PathShagged(mgr, CTriggerData(0.f))) {    // 0x00006FE0
    // 0x00006FF8: the guest's SetDestination(nav, 0x1048) writes the navigation's own
    // destination, which the host's SetPathFindDestination has already set to the same value.
    mPathFindNavigation.PathFind(mgr, kStateMsg_Activate, dt, *this); // 0x00007010
  }
}

// fn_10_69C0 (0x000069C0, 0x80 bytes): the uids at guest 0x934 are deleted from the state manager
// (0x000069FC..0x00006A0C) and the slot cleared (0x00006A14); a kInvalidUniqueId slot is left
// alone (0x000069F4). The loop runs over the count at guest 0x930 (0x00006A20).
void CDarkSamus::DeleteGuestUids(CStateManager& mgr) {
  for (int i = 0; i < mGuest930; ++i) { // 0x00006A20
    if (mGuest934[i] == kInvalidUniqueId.value) { // 0x000069F4
      continue;
    }
    mgr.DeleteObjectRequest(TUniqueId(mGuest934[i])); // 0x00006A0C
    mGuest934[i] = kInvalidUniqueId.value;           // 0x00006A14
  }
}

// fn_10_6374 (0x00006374, 0xEC bytes): the path find along the waypoints. With the message 0
// (`mr. r31, r5` at 0x00006394) the path find starts toward the module's vector at guest 0xEE4
// (fn_10_1C778 at 0x000063AC); otherwise the path find runs with that message (0x000063BC), then
// the separation (fn_10_1DD14, 0x000063C8) and, when the path is shagged (0x000063F4) or the actor
// is on the last waypoint (the waypoint count 0x7C4 and the current one 0x888, 0x000063F8..
// 0x00006408), the destination is set again (0x0000641C). Every message ends with the additive
// animation (fn_10_1D860, 0x0000642C) and the glide emitter (fn_10_11334, 0x00006438).
void CDarkSamus::FindPathToWaypoint(CStateManager& mgr, float dt, EStateMsg msg) {
  if (msg == kStateMsg_Activate) {
    SetPathFindDestination(mgr, mGuestEE4, dt); // fn_10_1C778 at 0x000063AC
  } else {
    mPathFindNavigation.PathFind(mgr, msg, dt, *this); // 0x000063BC
    Separate(mgr);                                     // fn_10_1DD14 at 0x000063C8
    if (PathShagged(mgr, CTriggerData(0.f)) ||         // 0x000063E8
        mPathFindSearch.GetCurrentWaypoint() >=
            static_cast< int >(mPathFindSearch.GetWaypoints().size()) - 1) {
      SetPathFindDestination(mgr, mGuestEE4, dt); // fn_10_1C778 at 0x0000641C
    }
  }
  UpdateAdditiveAnimation(mgr, msg); // fn_10_1D860 at 0x0000642C
  UpdateGlideEmitter(msg);           // fn_10_11334 at 0x00006438
}

// fn_10_21104 (0x00021104): the visor effect's particle token at guest 0xF6C, made from the asset id
// at 0xF68 when it is a real one (`addis r0, r4, 0x1; cmplwi r0, 0xffff` at 0x00021114), else empty.
// Run: the empty arm forced (the id kInvalidAssetId, darksamus_phazonbeam_invalid.py).
// @ fn_10_21104
rstl::optional_object< TToken< CGenDescription > > CDarkSamus::PhazonVisorToken(CAssetId id) {
  if (id == kInvalidAssetId) {
    return rstl::optional_object< TToken< CGenDescription > >();
  }
  return rstl::optional_object< TToken< CGenDescription > >(
      gpSimplePool->GetObj(SObjectTag('PART', id)));
}

// fn_10_452C (0x0000452C, 0xCC bytes), called on the 0xEF0 sub-object: the phazon attack's state
// reset. 0xF50 = 0 (`stfs f2, 0x60(r31)`), the shots 0xF48 and 0xF4C = 0, 0xF5C bits 0x80 and 0x40
// cleared (0x00004564/0x00004570), 0xF14 = 0xF20 = the zero vector, 0xF54 = lbl_10_rodata_140 =
// -1000.0f, 0xF10 = 0; the emitter 0xF60 removed when there is one (0x000045CC..0x000045DC, the
// body of fn_10_812C).
// @ fn_10_452C
void CDarkSamus::ResetPhazonAttack() {
  mGuestF50 = 0.f;
  mGuestF48 = 0;
  mGuestF4C = 0;
  mGuestF5C80 = false;
  mGuestF5C40 = false;
  mGuestF14 = CVector3f::Zero();
  mGuestF54 = -1000.f;
  mGuestF10 = 0.f;
  mGuestF20 = CVector3f::Zero();
  StopGuestSfx(); // 0x000045C4
}

// fn_10_490C (0x0000490C, 0x1BC bytes): the big phazon visor effect. With no effect actor 0xF78
// (0x00004930) a CHUDBillboardEffect "Dark Samus Big Phazon Visor Effect" (lbl_10_rodata_1F0 + 7)
// is made from the token 0xF6C, with a new uid kept in 0xF78 (0x00004988), the near clip distance
// and the POV scale of player 0, white, scale one, offset zero (0x00004A30), and added
// (0x00004A70). Always: 0xF7C = the elapsed time + duration (0x00004A80), when fn_10_48C8 deletes it.
// Run (darksamus_phazon.con): made on a first contact, kept on a later one; the allocation's
// null arm (`mr. r28, r3` at 0x000049B4) has no host counterpart (rs_new does not return null).
// @ fn_10_490C
void CDarkSamus::StartPhazonVisorEffect(CStateManager& mgr, float duration) {
  if (mGuestF78 == kInvalidUniqueId) {
    const float nearClip = CHUDBillboardEffect::GetNearClipDistance(mgr, 0);
    const CVector3f scale = CHUDBillboardEffect::GetScaleForPOV(mgr);
    mGuestF78 = mgr.AllocateUniqueId();
    CHUDBillboardEffect* effect = rs_new CHUDBillboardEffect(
        mGuestF6C, rstl::optional_object_null(), mGuestF78, true,
        rstl::string_l("Dark Samus Big Phazon Visor Effect"), nearClip, scale, 0, CColor::White(),
        CVector3f::One(), CVector3f::Zero(), false);
    mgr.AddObject(effect);
  }
  mGuestF7C = mElapsedTime + duration;
}

// fn_10_7304 (0x00007304, 0x168 bytes): the model scale (CModelData::SetScale at 0x0000732C) and
// the collision box that follows it: k = lbl_10_rodata_1B4 = 2.1 * (scale.z / lbl_10_rodata_1B8 =
// 5.0); the box is the base box 0x102C with min * k and max * k, max.z also raised by the base
// box's height (0x000073A0..0x000073B8); it is the bounding box and the CCollidableAABox with the
// actor's materials (0x68), the transform is dirty and the sorted lists updated (0x00007450).
// @ fn_10_7304
void CDarkSamus::SetPhazonScale(const CVector3f& scale, CStateManager& mgr) {
  ModelData()->SetScale(scale);
  const float k = 2.1f * (scale.GetZ() / 5.f);
  const CVector3f& min = mGuest102C.GetMinPoint();
  const CVector3f& max = mGuest102C.GetMaxPoint();
  const CAABox box(min * k, CVector3f(max.GetX() * k + 0.f, max.GetY() * k + 0.f,
                                      max.GetZ() * k + (max.GetZ() - min.GetZ())));
  SetBoundingBox(box);
  SetCollisionPrimitive(CCollidableAABox(box, GetMaterialList()));
  SetTransformDirty();
  mgr.UpdateActorInSortedLists(this);
}

// fn_10_746C (0x0000746C, 0xCC bytes): the phazon shrink. Nothing for dt <= 0 (0x00007480) or a
// z scale already at most 0xFAC (0x000074A8); else the scale less one * lbl_10_rodata_178 = 2.2 *
// dt, all of it 0xFAC once its z is below 0xFAC (0x000074F0), into fn_10_7304 (0x00007524).
// @ fn_10_746C
void CDarkSamus::ShrinkPhazonScale(CStateManager& mgr, float dt) {
  if (dt <= 0.f) {
    return;
  }
  const CVector3f scale = ModelData()->GetScale();
  if (scale.GetZ() <= mGuestFAC) {
    return;
  }
  CVector3f next = scale - CVector3f::One() * (2.2f * dt);
  if (next.GetZ() < mGuestFAC) {
    next = CVector3f::One() * mGuestFAC;
  }
  SetPhazonScale(next, mgr);
}

// fn_10_7538 (0x00007538, 0xD8 bytes): the phazon growth. Nothing for dt <= 0 (0x0000754C); the
// first time (0xFAC still lbl_10_rodata_13C = 0, 0x00007558) 0xFAC keeps the z scale; nothing past
// an x scale of lbl_10_rodata_1B8 = 5.0 (0x000075AC); else the scale plus one *
// lbl_10_rodata_16C = 0.8 * dt into fn_10_7304 (0x000075FC).
// @ fn_10_7538
void CDarkSamus::GrowPhazonScale(CStateManager& mgr, float dt) {
  if (dt <= 0.f) {
    return;
  }
  if (mGuestFAC == 0.f) {
    mGuestFAC = ModelData()->GetScale().GetZ();
  }
  const CVector3f scale = ModelData()->GetScale();
  if (scale.GetX() > 5.f) {
    return;
  }
  SetPhazonScale(scale + CVector3f::One() * (0.8f * dt), mgr);
}

// fn_10_7780 (0x00007780, 0x38 bytes): the enrage sphere's position, guest 0x8FC..0x904 (mRootPos)
// plus (lbl_10_rodata_13C = 0, 0, lbl_10_rodata_1C8 = -1).
// @ fn_10_7780
CVector3f CDarkSamus::GetPhazonSpherePos() const {
  return CVector3f(mRootPos.GetX() + 0.f, mRootPos.GetY() + 0.f, mRootPos.GetZ() + -1.f);
}

// fn_10_7610 (0x00007610, 0x170 bytes): the enrage sphere's frame. Nothing for dt <= 0 (0x00007638)
// or no sphere 0x1028 (0x00007650); else its orientation the actor's rotation (vtable 0x14), its
// global translation fn_10_7780 (0x20), and its global scale the model scale, pulsed for
// lbl_10_rodata_1BC = 1.1 s after the volley start 0xFC8 (0x000076D0) by 1 + 0.7 *
// sin(pi * (elapsed - 0xFC8) / 1.1) (0x00007724) (0x24); then its Update(dt) (0xC).
// @ fn_10_7610
void CDarkSamus::UpdatePhazonSphere(float dt) {
  if (dt <= 0.f || mGuest1028.get() == nullptr) {
    return;
  }
  CElementGen* gen = mGuest1028.get();
  gen->SetOrientation(GetTransform().GetRotation());
  gen->SetGlobalTranslation(GetPhazonSpherePos());
  CVector3f scale = ModelData()->GetScale();
  if (1.1f + mGuestFC8 > mElapsedTime) {
    const float pulse =
        static_cast< float >(sin(3.1415927f * ((mElapsedTime - mGuestFC8) / 1.1f)));
    scale = scale * (0.70000005f * pulse + 1.f);
  }
  mGuest1028->SetGlobalScale(scale);
  mGuest1028->Update(dt);
}

// fn_10_786C (0x0000786C, 0x1A0 bytes): the enrage sphere, made once (0x00007894): a CElementGen
// (0x338 bytes) of the PART 0x1020 (mData.mPhazonEnrageSphere), normal orientation, optional
// system flags 1 (0x00007918), into the auto_ptr 0x1024/0x1028 (0x00007970), and it emits
// (vtable 0x2C with 1, 0x000079EC).
// @ fn_10_786C
void CDarkSamus::CreatePhazonSphere() {
  if (mGuest1028.get() != nullptr) {
    return;
  }
  mGuest1028 = rs_new CElementGen(gpSimplePool->GetObj(SObjectTag('PART', mData.mPhazonEnrageSphere)),
                                  CElementGen::kMOT_Normal, CElementGen::kOSF_One);
  mGuest1028->SetParticleEmission(true);
}

// fn_10_7D00 (0x00007D00, 0x298 bytes): the enrage sphere's contact. Nothing within
// lbl_10_rodata_1AC = 0.5 s of the last contact 0x101C (0x00007D28) or with no player touch
// bounds (vtable 0x48, 0x00007DC8); else the box's centre and its half diagonal (0x00007E10), the
// sphere's global scale's length (vtable 0x4C, 0x00007E24), and a contact when the centre is
// nearer fn_10_7780 than their sum (0x00007E78): the push (fn_10_13750 with lbl_10_rodata_174 = 22
// and 0, 0x00007E90), the damage 0x1000 (mData.mPhazonEnrageSphereContactDamage) with the solid
// include filter (1 << lbl_10_data_1A0 = 59, type 3, 0x00007F1C), the visor effect for 0.5 s when
// the player is unmorphed (0x38C == 0, 0x00007F48), and 0x101C = the elapsed time (0x00007F50).
// Run (darksamus_phazon.con): gated, near unmorphed and morphed, far. The no-touch-bounds return
// cannot be taken against the player: CPlayer::GetTouchBounds returns a box in both its arms.
// @ fn_10_7D00
void CDarkSamus::TouchPhazonSphere(CStateManager& mgr) {
  if (0.5f + mGuest101C > mElapsedTime) {
    return;
  }
  CPlayer* player = mgr.GetPlayer(0);
  const rstl::optional_object< CAABox > bounds = player->GetTouchBounds();
  if (!bounds) {
    return;
  }
  // Host-only guard, not a retail arm: retail reads the pointer 0x1028 (the sphere's vtable)
  // unchecked, since the enrage made the sphere first; a forced phazon state with no sphere would
  // crash here.
  if (mGuest1028.get() == nullptr) {
    return;
  }
  const CVector3f center = bounds.data().GetCenterPoint();
  const float boxRadius = (bounds.data().GetMaxPoint() - center).Magnitude();
  const float sphereRadius = mGuest1028->GetGlobalScale().Magnitude();
  if ((center - GetPhazonSpherePos()).Magnitude() >= sphereRadius + boxRadius) {
    return;
  }
  PushPlayer(mgr, 22.f, 0.f);
  mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(),
                  mData.mPhazonEnrageSphereContactDamage,
                  CMaterialFilter(CMaterialList(kMT_Solid), CMaterialList(),
                                  CMaterialFilter::kFT_IncludeExclude),
                  CVector3f::Zero());
  if (player->GetMorphballTransitionState() == CPlayer::kMS_Unmorphed) {
    StartPhazonVisorEffect(mgr, 0.5f);
  }
  mGuest101C = mElapsedTime;
}

// fn_10_7F98 (0x00007F98, 0x16C bytes): the phazon enrage's frame, every message: the attack
// bookkeeping with 0x800000 (fn_10_1B478), the collision failsafe, the ground collision, the
// growth fn_10_7538, the sphere fn_10_7610, its contact fn_10_7D00 and the visor effect's expiry
// fn_10_48C8 (0x00008024). Update (`cmpwi r31, 0x1` at 0x00008028): toward the phazon waypoint of
// type 8 (fn_10_65CC) by lbl_10_rodata_1D8 = 0.6981317 rad unless it is the zero vector
// (0x000080AC); with the player gun tweak (0x000080BC) and the absorbed phazon shots (the gun's
// 0x7FC) at least its maximum (0x000080DC), 0xFCC loses lbl_10_rodata_1DC = 3.5 * dt (0x000080E8).
// Forced (darksamus_phazon.py): the absorbed shots at the maximum, and gpTweakPlayerGun null.
// @ fn_10_7F98
void CDarkSamus::UpdatePhazonEnrage(CStateManager& mgr, EStateMsg msg, float dt) {
  UpdateCurrentAttack(0x800000, mgr, msg);
  RunCollisionFailsafe(mgr);
  UpdateGroundCollision(mgr, msg);
  GrowPhazonScale(mgr, dt);
  UpdatePhazonSphere(dt);
  TouchPhazonSphere(mgr);
  DeleteGuestActorIfExpired(mgr);
  if (msg != kStateMsg_Update) {
    return;
  }
  const CVector3f waypoint = GetPhazonWaypoint(mgr, 8);
  if (!(waypoint == CVector3f::Zero())) {
    TurnTowardPoint(dt, waypoint, 0.6981317f);
  }
  if (gpTweakPlayerGun != nullptr &&
      mgr.GetPlayer(0)->GetPlayerGun()->GetAbsorbedPhazonShots() >=
          gpTweakPlayerGun->GetMaxAbsorbedPhazonShots()) {
    mGuestFCC -= 3.5f * dt;
  }
}

// fn_10_45F8's token at 0x00004644: the beam's hit effect at guest 0xF00, locked, from the asset id
// at 0xEFC when it is a real one (`addis r0, r5, 0x1` at 0x00004604, `cmplwi r0, 0xffff` at
// 0x00004608, the `beq` at 0x00004640), else empty.
rstl::optional_object< TLockedToken< CGenDescription > > CDarkSamus::PhazonBeamHitToken(CAssetId id) {
  if (id == kInvalidAssetId) {
    return rstl::optional_object< TLockedToken< CGenDescription > >();
  }
  return rstl::optional_object< TLockedToken< CGenDescription > >(
      TLockedToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', id))));
}

// fn_10_45F8 (0x000045F8, 0x270 bytes): the 0xEF0 sub-object's constructor, at 0x000201B4 of the
// actor's. Its id/token/damage/sound members (0xEF0 = r4, 0xEFC = r5 and the locked token 0xF00,
// 0xF2C..0xF46 = *r6, 0xF5E = r7) are mData's in the host and PhazonBeamHitToken (the member
// initializer of mGuestF00); the beam 0xEF4/0xEF8 starts empty (0x00004634/0x00004638). The
// scalars: 0xF14 and 0xF20 the zero vector (0x000047BC..0x000047F8), 0xF58 = lbl_10_rodata_13C
// = 0.0f, the emitter 0xF60 none and 0xF64 = lbl_10_rodata_140 = -1000.0f (0x0000483C..0x00004848),
// then fn_10_452C (0x0000484C). Run: every actor's constructor; the token's invalid-id arm
// forced (darksamus_phazonbeam_invalid.py).
// @ fn_10_45F8
void CDarkSamus::ConstructPhazonBeam() {
  mGuestF14 = CVector3f::Zero();
  mGuestF20 = CVector3f::Zero();
  mGuestF58 = 0.f;
  mGuestF60 = CSfxHandle();
  mGuestF64 = -1000.f;
  ResetPhazonAttack();
}

// fn_10_6118 (0x00006118, 0x19C bytes): the phazon beam. A CElementGen of the asset 0xEF0
// (mData.mPart908B06E9) with the orientation kMOT_Normal and the system flags 1 (0x000061B8)
// replaces the auto_ptr 0xEF4/0xEF8 (the old one deleted, 0x0000620C), and it emits (vtable 0x2C
// with 1, 0x00006290). The kUE_EventStart event of the 0x400000 attack calls it (0x0001C270).
// Run: `uevent u04f7 27` (darksamus_phazonbeam.con), a beam made and one replaced.
// @ fn_10_6118
void CDarkSamus::CreatePhazonBeam() {
  mGuestEF8 = rs_new CElementGen(gpSimplePool->GetObj(SObjectTag('PART', mData.mPart908B06E9)),
                                 CElementGen::kMOT_Normal, CElementGen::kOSF_One);
  mGuestEF8->SetParticleEmission(true);
}

// fn_10_58C0 (0x000058C0, 0xB4 bytes): an empty auto_ptr into 0xEF4/0xEF8, which deletes the beam
// (0x00005918). The kUE_EventStop event calls it (0x0001C2A4). Run: `uevent u04f7 28`.
// @ fn_10_58C0
void CDarkSamus::ReleasePhazonBeam() {
  mGuestEF8 = rstl::auto_ptr< CElementGen >();
}

// fn_10_8178 (0x00008178, 0x5C bytes): the beam's sound. The emitter 0xF60 is stopped
// (fn_10_812C) and a new one of the sfx 0xF5E (mData.mSound, kInternalInvalidSfxId for -1: the
// ctor's 0x000201A8) at the actor's position, volume 0x7F, the actor's area, no acoustics, not
// looped, kMedPriority (0x000081B4) is kept in 0xF60. Run: both sound arms forced (the data's -1
// in darksamus_phazonbeam_invalid.py, 1 in darksamus_phazonbeam.py).
// @ fn_10_8178
void CDarkSamus::StartPhazonBeamSfx() {
  StopGuestSfx();
  const ushort sfx = mData.mSound == -1 ? CSfxManager::kInternalInvalidSfxId
                                        : static_cast< ushort >(mData.mSound);
  mGuestF60 = CSfxManager::AddEmitter(sfx, GetTranslation(), 0x7F, GetCurrentAreaId().Value(),
                                      false, false, CSfxManager::kMedPriority);
}

// fn_10_5974 (0x00005974, 0x7A4 bytes): the phazon beam's update, PhazonAttack's Update
// (0x00008340). Without a beam 0xEF8 nothing (0x000059C0). The beam sits at the gun locator 0x8CC
// (vtable 0x20, 0x000059EC). The player's offset from the actor, its z the height
// lbl_10_rodata_14C = 1.0f + 0xF58 less the actor's z, must be normalizable (0x00005A48); the
// angle between it and the actor's forward is kept. The beam's reach 0xF10: within
// lbl_10_rodata_170 = 20 degrees (0x00005A9C) it follows the flat distance to the player by at most
// lbl_10_rodata_174 = 22 * dt either way (0x00005AC0..0x00005ADC). The beam looks from the locator
// at the actor's position + forward * reach at that height (LookAt at 0x00005B64); 0xF14 = its
// forward (0x00005B8C), the generator gets its rotation (0x1C), scale one (0x24) and dt (0x0C).
// With 0xF14 normalizable (0x00005BF8): the player's position, eye or aim position (vtable 0x54,
// dt 0) inside the cylinder of radius lbl_10_rodata_178 = 2.2 along it (0x00005C58/0x00005C7C/
// 0x00005CBC) and the angle under lbl_10_rodata_17C = 90 degrees (0x00005CD8): the damage 0xF2C
// with its amount * dt goes to the player (solid include-exclude filter, 0x00005DA4), 0xF54 = the
// elapsed time and the visor effect for lbl_10_rodata_180 = 0.25 s (fn_10_490C). Then 0xF50 gains
// dt and every lbl_10_rodata_184 = 0.07 s of it (0x000060D8) casts a ray of lbl_10_rodata_18C =
// 100 along 0xF14 from a random point on the beam's 2.2 ring (Range(0, 2 pi), 0x00005E54) against
// solid and AI-block (0x00005EFC); a hit sets the emitter position 0xF20 (the hit less the ring
// offset, 0x00005F58) and adds a "Phazon Attack Collision" CExplosion of 0xF00 at the hit, facing
// the player's eye (0x00006098).
// Run (darksamus_phazonbeam.py, _invalid.py): every arm by a keyed force (the zero offset, the
// turn, the reach clamps, each cylinder test, a NaN locator for the unnormalizable 0xF14, a ray
// too short to hit). The hit token 0xF00 is read unchecked, as retail does (0x00006098); the
// fights' data always carries pART_0x494de4a4. The CExplosion allocation's null/throw edges have no
// host counterpart (rs_new does not return null).
// @ fn_10_5974
void CDarkSamus::UpdatePhazonBeam(CStateManager& mgr, float dt) {
  if (mGuestEF8.get() == nullptr) {
    return;
  }
  mGuestEF8->SetGlobalTranslation(mGunLctrXf.GetTranslation());
  CTransform4f beamXf = mGunLctrXf;
  CPlayer* player = mgr.GetPlayer(0);
  const float height = 1.f + mGuestF58;
  const CVector3f& pos = GetTranslation();
  CVector3f toPlayer(player->GetTranslation().GetX() - pos.GetX(),
                     player->GetTranslation().GetY() - pos.GetY(), height - pos.GetZ());
  if (!toPlayer.CanBeNormalized()) {
    return;
  }
  const CVector3f fwd = GetTransform().GetForward();
  const float angle = CVector3f::GetAngleDiff(fwd, toPlayer.AsNormalized());
  toPlayer.SetZ(0.f);
  float reach = mGuestF10;
  if (angle < 0.34906584f) {
    const float dist = toPlayer.Magnitude();
    const float step = 22.f * dt;
    const float lo = mGuestF10 - step;
    const float hi = mGuestF10 + step;
    if (lo > dist) {
      reach = lo;
    } else if (hi < dist) {
      reach = hi;
    } else {
      reach = dist;
    }
  }
  CVector3f target = fwd * reach + pos;
  target.SetZ(height);
  beamXf = CTransform4f::LookAt(beamXf.GetTranslation(), target, CVector3f::Up());
  mGuestF10 = reach;
  mGuestF14 = beamXf.GetForward();
  mGuestEF8->SetGlobalOrientation(beamXf.GetRotation());
  mGuestEF8->SetGlobalScale(CVector3f::One());
  mGuestEF8->Update(dt);
  if (!mGuestF14.CanBeNormalized()) {
    return;
  }
  mGuestF14.Normalize();
  const CVector3f axis = mGuestF14.AsNormalized();
  const CCylinder beam(CLine(mGunLctrXf.GetTranslation(),
                             CUnitVector3f(axis.GetX(), axis.GetY(), axis.GetZ())),
                       2.2f);
  if ((beam.PointInside(player->GetTranslation()) || beam.PointInside(player->GetEyePosition()) ||
       beam.PointInside(player->GetAimPosition(mgr, 0.f))) &&
      angle < 1.5707964f) {
    CDamageInfo damage(mData.mDamageInfo);
    damage.SetDamage(dt * mData.mDamageInfo.GetDamage());
    mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(), damage,
                    CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
                    CVector3f::Zero());
    mGuestF54 = mElapsedTime;
    StartPhazonVisorEffect(mgr, 0.25f);
  }
  mGuestF50 += dt;
  while (mGuestF50 > 0.07f) {
    mGuestF50 -= 0.07f;
    CVector3f origin = beamXf.GetTranslation();
    const float a = mgr.Random()->Range(0.f, 6.2831855f);
    const float ringY = 2.2f * CMath::FastSinR(a);
    const float ringX = 2.2f * CMath::FastCosR(a);
    origin = CVector3f(origin.GetX() + ringX, origin.GetY() + ringY, origin.GetZ() + 0.f);
    const CRayCastResult hit = mgr.RayStaticIntersection(
        origin, mGuestF14, 100.f,
        CMaterialFilter::MakeInclude(CMaterialList(kMT_Solid, kMT_AIBlock)));
    if (!hit.IsValid()) {
      continue;
    }
    const CVector3f& point = hit.GetPoint();
    mGuestF20 = CVector3f(point.GetX() - ringX, point.GetY() - ringY, point.GetZ() - 0.f);
    const CTransform4f xf = CTransform4f::LookAt(point, player->GetEyePosition(), CVector3f::Up());
    const rstl::string name = rstl::string_l("Phazon Attack Collision");
    const CEntityInfo info(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId);
    const TUniqueId uid = mgr.AllocateUniqueId();
    mgr.AddObject(rs_new CExplosion(*mGuestF00, uid, info, name, xf, 0, CVector3f::One() * 1.f,
                                    CColor::White(), -1));
  }
}

// fn_10_77B8 (0x000077B8, 0xB4 bytes): the enrage sphere's generator (the auto_ptr at guest
// 0x1024/0x1028) is replaced by an empty one built on the stack (0x000077D8..0x000077E0): the old
// generator, if owned, is deleted through its vtable (0x00007810).
// @ fn_10_77B8
void CDarkSamus::ClearEnrageSphere() { mGuest1028 = rstl::auto_ptr< CElementGen >(); }

// fn_10_13998 (0x00013998, 0x11C bytes): the melee hit, once per swing (the record's hit byte
// 0x9DE, 0x000139B4; set at 0x00013A98): the record's damage (meleeAttackDamage, 0x9A4) on the
// player from the actor, filtered include kMT_Solid (lbl_10_data_200 = 59) exclude nothing with
// type 3 (0x000139DC), at the zero vector (0x00013A3C); unless the player is a morph ball (vtable
// 0x1CC = fn_10_112E8 with a 0.0f trigger, 0x00013A68) fn_10_13750 pushes it with
// lbl_10_rodata_4BC = 25.0f across and lbl_10_rodata_3E0 = 2.0f up (0x00013A90).
// @ fn_10_13998
void CDarkSamus::ApplyMeleeDamage(CStateManager& mgr) {
  if (mMeleeContact.mHit == 1) {
    return;
  }
  mgr.ApplyDamage(GetUniqueId(), mgr.GetPlayer(0)->GetUniqueId(), GetUniqueId(),
                  mData.mMeleeAttackDamage,
                  CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
                  CVector3f::Zero());
  if (!PlayerIsMorphball(mgr, CTriggerData(0))) {
    PushPlayer(mgr, 25.f, 2.f);
  }
  mMeleeContact.mHit = 1;
}

// fn_10_13AB4 (0x00013AB4, 0x388 bytes): the melee pulse. From 1.0f above the actor
// (lbl_10_rodata_3E8, 0x00013AFC..0x00013B10) a 20.0f (lbl_10_rodata_4C0) ray straight down finds
// the floor (BuildNearList 0x00013C28, RayWorldIntersection 0x00013C68) through the static filter
// lbl_10_bss_F8 built once (0x00013B30..0x00013BF0): include kMT_Solid / kMT_Platform (data CA0 /
// CA4 = 59 / 49), exclude kMT_Character / kMT_Player / kMT_ProjectilePassthrough (CA8 / CAC /
// CB0 = 33 / 32 / 20), type 3. The explosion "Dark Samus Melee Pulse" (lbl_10_rodata_560 + 0x53A)
// takes the actor's transform moved to the root bone 0x8FC/0x900 at the hit's height (the root's
// own when the ray misses, 0x00013C7C) plus lbl_10_rodata_4B8 = 1.9f (0x00013CAC), the model's
// scale (0x60, 0x00013CF8), white and player -1 (0x00013D98); attack 2 takes the record's pART
// token 0x9D0, the rest meleeAttackFX 0x9C4 (0x00013D7C). Its uid goes to the record's 0x9DC
// (0x00013DFC). Port limit: a token whose asset was invalid is never built, so no explosion.
// Compiler copies, no code of their own here: fn_10_2C8 (0x00013C74) is the module's
// CRayCastResult copy constructor, fn_10_2040 (0x00013DD8) and the string's
// internal_dereference (0x00013DE8) destroy the new-expression's temporaries.
// Run (darksamus_attacks.py): every arm, the ray's miss with key 21 (the actor's position 0x54
// moved 1000 up and 100000 across; run /tmp/opencode/dsfsm2/s9/a4, DarkSamus.md slice 9).
// @ fn_10_13AB4
void CDarkSamus::SpawnMeleePulse(CStateManager& mgr) {
  const CVector3f& pos = GetTranslation();
  const CVector3f start(pos.GetX() + 0.f, pos.GetY() + 0.f, pos.GetZ() + 1.f);
  const float rootX = mRootPos.GetX();
  const float rootY = mRootPos.GetY();
  float floorZ = mRootPos.GetZ();
  // The filter is built each call: retail's once-built static holds the same value.
  const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid, kMT_Platform),
      CMaterialList(kMT_Character, kMT_Player, kMT_ProjectilePassthrough));
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, start, CVector3f::Down(), 20.f, filter, nullptr);
  TUniqueId hitId = kInvalidUniqueId;
  const CRayCastResult hit =
      mgr.RayWorldIntersection(hitId, start, CVector3f::Down(), 20.f, filter, nearList);
  if (hit.IsValid()) {
    floorZ = hit.GetPoint().GetZ();
  }
  CTransform4f xf = GetTransform();
  xf.SetTranslation(CVector3f(rootX + 0.f, rootY + 0.f, floorZ + 1.9f));
  const rstl::optional_object< TLockedToken< CGenDescription > >& token =
      mCurrentAttack == 2 ? mMeleePartToken : mMeleeAttackFXToken;
  if (!token) {
    return;
  }
  const CVector3f scale = GetModelData()->GetScale();
  const rstl::string name = rstl::string_l("Dark Samus Melee Pulse");
  const CEntityInfo info(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId);
  const TUniqueId uid = mgr.AllocateUniqueId();
  CExplosion* explosion =
      rs_new CExplosion(*token, uid, info, name, xf, 0, scale, CColor::White(), -1);
  mgr.AddObject(explosion);
  mMeleeContact.mHitUid = explosion->GetUniqueId();
}

// fn_10_1353C (0x0001353C, 0x140 bytes): the missile. The player's aim position (its vtable
// 0x54) led by a random time in [0, distance / 25.0f] (fn_10_17CEC, lbl_10_rodata_4BC, Range at
// 0x00013580); the gun locator 0x8CC turned to look at it with its own up (0x00013604); then
// LaunchProjectile with 5 at most, no attributes, not homing, an empty visor effect (fn_10_4EB4)
// and scale 1.0f (0x00013650). The info is ProjectileInfo's for the attack (0xA0C).
// @ fn_10_1353C
void CDarkSamus::FireMissile(CStateManager& mgr) {
  CPlayer* player = mgr.GetPlayer(0);
  const float lead = mgr.Random()->Range(0.f, GetDistanceToPlayer(mgr) / 25.f);
  const CVector3f aim = player->GetAimPosition(mgr, lead);
  CTransform4f xf = mGunLctrXf;
  xf = CTransform4f::LookAt(xf.GetTranslation(), aim, xf.GetUp());
  LaunchProjectile(xf, mgr, 5, 0, false, CImpactVisorEffect::None(), CVector3f(1.f, 1.f, 1.f));
}

// fn_10_133D0 (0x000133D0, 0x12C bytes): the super missile, as fn_10_1353C but aimed at the
// player's position, 1.9f (lbl_10_rodata_4B8) higher unless it is morphed (0x38C == 1,
// 0x0001340C); the info is 0xA4C.
// @ fn_10_133D0
void CDarkSamus::FireSuperMissile(CStateManager& mgr) {
  const CPlayer* player = mgr.GetPlayer(0);
  CVector3f aim = player->GetTranslation();
  if (player->GetMorphballTransitionState() != CPlayer::kMS_Morphed) {
    aim = CVector3f(aim.GetX() + 0.f, aim.GetY() + 0.f, aim.GetZ() + 1.9f);
  }
  CTransform4f xf = mGunLctrXf;
  xf = CTransform4f::LookAt(xf.GetTranslation(), aim, xf.GetUp());
  LaunchProjectile(xf, mgr, 5, 0, false, CImpactVisorEffect::None(), CVector3f(1.f, 1.f, 1.f));
}

// fn_10_9164 (0x00009164, 0x1B0 bytes): the freeze beam, only with a token (0xA90, 0x0000918C).
// Aimed as fn_10_133D0 (lbl_10_bss_124 = 1.9f, set by the static init fn_10_CDB8) from the gun
// locator; a CFreezeBeamProjectile (0x000092DC) of the token 0xA88, weapon type 10 (kWT_Phazon,
// `li r5, 0xa`), excluding kMT_ProjectilePassthrough (`li r7, 0x14`), the damage 0xA94
// (freezeBeamDamage), a new uid, the actor's area, owned by the actor, freeze 5.0f / ice bound
// 4.0f (lbl_10_rodata_274 / 278), the sound 0xEE0 and particle 0xEDC (iceSpreadSound /
// iceSpreadFX, the constructor at 0x00020174) and the steam texture 0xAB4 (freezeBeamVisorTexture).
// @ fn_10_9164
void CDarkSamus::FireFreezeBeam(CStateManager& mgr) {
  if (!mFreezeBeamToken) {
    return;
  }
  const CPlayer* player = mgr.GetPlayer(0);
  CVector3f aim = player->GetTranslation();
  if (player->GetMorphballTransitionState() != CPlayer::kMS_Morphed) {
    aim = CVector3f(aim.GetX() + 0.f, aim.GetY() + 0.f, aim.GetZ() + 1.9f);
  }
  CTransform4f xf = mGunLctrXf;
  xf = CTransform4f::LookAt(xf.GetTranslation(), aim, xf.GetUp());
  const ushort sfx = mData.mIceSpreadSound == -1 ? CSfxManager::kInternalInvalidSfxId
                                                 : static_cast< ushort >(mData.mIceSpreadSound);
  // The new's null check (0x000092E4) is left out: the host's new does not answer null.
  mgr.AddObject(rs_new CFreezeBeamProjectile(
      *mFreezeBeamToken, kWT_Phazon, xf, kMT_ProjectilePassthrough, mData.mFreezeBeamDamage,
      mgr.AllocateUniqueId(), GetCurrentAreaId(), GetUniqueId(), 5.f, 4.f, sfx, mData.mIceSpreadFX,
      mData.mFreezeBeamVisorTexture));
}

// fn_10_1863C (0x0001863C, 0x2D0 bytes): the flash quad, one unit in front of the camera (the
// view's translation plus its forward column, 0x000186CC..0x00018710), its right and up columns
// scaled by size * alpha (0x000186BC); identity model matrix, additive blend (One / One, 0x00018728),
// Tev stage 0 modulate and stage 1 passthru, depth test off with kE_Always and no writes
// (0x0001875C), the colour (alpha, alpha, alpha, alpha) and a triangle fan (0xA0) from
// (0,0) at C-R+U, (1,0) at C-R-U, (1,1) at C+R-U, (0,1) at C+R+U.
// Run (darksamus_uevent.py, dsfsm2-runs/s10 u4+u9+u10): every line (key 55's flash window under
// `visor dark`, through Render 0x00018A98 and RenderFlash).
// @ fn_10_1863C
void CDarkSamus::DrawFlashQuad(float alpha, float size) const {
  const CTransform4f& view = CGraphics::GetViewMatrix();
  const float scale = size * alpha;
  const CVector3f center(view.Get03() + view.Get01(), view.Get13() + view.Get11(),
                         view.Get23() + view.Get21());
  const CVector3f right(scale * view.Get00(), scale * view.Get10(), scale * view.Get20());
  const CVector3f up(scale * view.Get02(), scale * view.Get12(), scale * view.Get22());
  CGraphics::SetModelMatrix(CTransform4f::Identity());
  CGraphics::SetBlendMode(kBM_Blend, kBF_One, kBF_One, kLO_Clear);
  CGraphics::SetTevOp(kTS_Stage0, CGraphics::kEnvModulate);
  CGraphics::SetTevOp(kTS_Stage1, CGraphics::kEnvPassthru);
  CGraphics::SetDepthWriteMode(false, kE_Always, false);
  CGraphics::StreamColor(CColor(alpha, alpha, alpha, alpha));
  CGraphics::StreamBegin(kP_TriangleFan);
  CGraphics::StreamTexcoord(0.f, 0.f);
  CGraphics::StreamVertex(center - right + up);
  CGraphics::StreamTexcoord(1.f, 0.f);
  CGraphics::StreamVertex(center - right - up);
  CGraphics::StreamTexcoord(1.f, 1.f);
  CGraphics::StreamVertex(center + right - up);
  CGraphics::StreamTexcoord(0.f, 1.f);
  CGraphics::StreamVertex(center + right + up);
  CGraphics::StreamEnd();
}

// fn_10_1890C (0x0001890C, 0x58 bytes): with the flash texture (0xDFC, 0x00018920) load it on
// GX_TEXMAP0, repeating (0x00018938), and draw the quad at the fade factor (fn_10_185CC) and size
// 0xE00 (0x0001894C); without it nothing.
void CDarkSamus::RenderFlash() const {
  if (!mFlashTexture) {
    return;
  }
  (*mFlashTexture)->Load(GX_TEXMAP0, CTexture::kCM_Repeat);
  DrawFlashQuad(GetFadeFactor(), mFlashSize);
}

// ---- dsfsm2 slice 9: the phazon volley ----

// fn_10_5840 (0x00005840, 0x80 bytes): one missile of the phazon volley while 0xFB4 has any left
// (0x0000585C, counted down at 0x00005864): 0xFB8 0 the phazon shot fn_10_50B8, which starts the
// volley's clock 0xFCC at the elapsed time 0x8B0 if it is below 0.0f (0x00005898); 1 the phazon
// super missile fn_10_4C40; any other value nothing.
// @ fn_10_5840
void CDarkSamus::FirePhazonVolley(CStateManager& mgr) {
  if (mGuestFB4 <= 0) {
    return;
  }
  --mGuestFB4;
  switch (mGuestFB8) {
  case 0:
    FirePhazonShot(mgr);
    if (mGuestFCC < 0.f) {
      mGuestFCC = mElapsedTime;
    }
    break;
  case 1:
    FirePhazonSuperMissile(mgr);
    break;
  default:
    break;
  }
}

// fn_10_50B8 (0x000050B8, 0x788 bytes): a phazon shot. The dummy-target list 0x934 is padded with
// kInvalidUniqueId to 12 (0x00005110). The shot leaves the root bone 0x8FC moved by a random
// (x, y) in [-1.2, 1.2] (lbl_10_rodata_154 / 158, 0x000051E8 / 0x00005204) and up by 1.2 less
// that offset's length (0x00005334), turned to look along the actor's flat forward plus 1.5 the
// offset's direction plus 3 up (lbl_10_rodata_14C / 160 / 15C, LookAt from the zero vector at
// 0x0000530C, SetRotation 0x00005318; the actor's own transform with the translation 1.3 times the
// model's z scale up, 0x0000513C, is overwritten). It homes on the player unless the player orbits
// the actor (its 0x3B0, 0x00005378); then the first free slot of 0x934 gets a new "DS3 Dummy
// Target" CActor (lbl_10_rodata_1F0 + 0x2A, material kMT_Target = lbl_10_data_1AC 40, no model) at
// the player moved -4..4 (lbl_10_rodata_164 / 168) across the flat direction from the actor
// ((1, 0, 0) when that has no length, 0x0000543C) and 0.8 (lbl_10_rodata_16C) up, and the shot
// homes on the list's first entry (0x00005648). Then, as CPatterned::LaunchProjectile: the info
// cached (0x00005698), CanCreateProjectile kWT_AI with 14 (0x000056D4), a CEnergyProjectile
// excluding kMT_Character, scale one, an empty visor effect (0x000057EC). Port limit: an info whose
// asset was invalid is not built on the host, so no shot. The new's null check is left out.
// Run (darksamus_attacks.py keys 10..28): every arm; forced: the orbit (16), the full dummy list
// (17), the flat forward (18), the player on the actor (25), the zero side vector (26, loc:), the
// uncached (27) and absent (28) info, and CanCreateProjectile's limit (16 shots in one frame).
// @ fn_10_50B8
void CDarkSamus::FirePhazonShot(CStateManager& mgr) {
  while (mGuest930 < 12) {
    mGuest934[mGuest930] = kInvalidUniqueId.value;
    ++mGuest930;
  }
  CTransform4f xf = GetTransform();
  CVector3f forward(xf.Get01(), xf.Get11(), 0.f);
  if (forward.CanBeNormalized() == true) {
    forward.Normalize();
  }
  const float offsetX = mgr.Random()->Range(-1.2f, 1.2f);
  const float offsetY = mgr.Random()->Range(-1.2f, 1.2f);
  CVector3f side(offsetX, offsetY, 0.f);
  if (side.CanBeNormalized() == true) {
    side.Normalize();
  }
  const CVector3f target = forward * 1.f + side * 1.5f + CVector3f::Up() * 3.f;
  xf.SetRotation(CTransform4f::LookAt(CVector3f::Zero(), target, CVector3f::Up()));
  const float rise = 1.2f - CVector3f(offsetX, offsetY, 0.f).Magnitude();
  xf.SetTranslation(CVector3f(mRootPos.GetX() + offsetX, mRootPos.GetY() + offsetY,
                              mRootPos.GetZ() + rise));

  TUniqueId homingTarget = kInvalidUniqueId;
  CPlayer* player = mgr.GetPlayer(0);
  if (player->GetOrbitTargetId() != GetUniqueId()) {
    homingTarget = player->GetUniqueId();
  } else {
    for (int i = 0; i < mGuest930; ++i) {
      if (mGuest934[i] != kInvalidUniqueId.value) {
        continue;
      }
      const TUniqueId uid = mgr.AllocateUniqueId();
      mGuest934[i] = uid.value;
      CTransform4f dummyXf = player->GetTransform();
      const CVector3f& pos = GetTranslation();
      CVector3f dir(dummyXf.Get03() - pos.GetX(), dummyXf.Get13() - pos.GetY(), 0.f);
      if (dir.CanBeNormalized() == true) {
        dir.Normalize();
      } else {
        dir = CVector3f(1.f, 0.f, 0.f);
      }
      const float across = mgr.Random()->Range(-4.f, 4.f);
      dummyXf.SetTranslation(CVector3f(dummyXf.Get03() + across * dir.GetY(),
                                       dummyXf.Get13() + across * -dir.GetX(),
                                       dummyXf.Get23() + (across * 0.f + 0.8f)));
      const rstl::string name = rstl::string_l("DS3 Dummy Target");
      const CEntityInfo info(GetCurrentAreaId(), CEntity::NullConnectionList, true,
                             kInvalidEditorId);
      const CModelData model;
      const CMaterialList material(kMT_Target);
      const CActorParameters params;
      mgr.AddObject(rs_new CActor(uid, name, info, 0, dummyXf, model, material, params,
                                  kInvalidUniqueId));
      break;
    }
    // Retail tests the target for kInvalidUniqueId here (0x00005640), which it always is.
    homingTarget = TUniqueId(mGuest934[0]);
  }

  CProjectileInfo* info = ProjectileInfo();
  if (info == nullptr || !info->Token().TryCache()) {
    return;
  }
  if (!mgr.CanCreateProjectile(GetUniqueId(), kWT_AI, 14)) {
    return;
  }
  const CImpactVisorEffect visorEffect = CImpactVisorEffect::None();
  const CVector3f scale(1.f, 1.f, 1.f);
  const TUniqueId uid = mgr.AllocateUniqueId();
  mgr.AddObject(rs_new CEnergyProjectile(true, info->Token(), kWT_AI, xf, kMT_Character,
                                         info->GetDamage(), uid, GetCurrentAreaId(), GetUniqueId(),
                                         homingTarget, 0, false, scale, visorEffect, false, true,
                                         false, 1.f, 4.f, 4.f));
}

// fn_10_4C40 (0x00004C40, 0x15C bytes): the phazon super missile, from 5.3 (lbl_10_rodata_144)
// above the root bone 0x8FC, looking at the player's aim position (its vtable 0x54 with no lead,
// 0x00004CD8) 0.5 lower (lbl_10_rodata_148) with the world's up (0x00004D28); LaunchProjectile with
// 14 at most, no attributes, homing, an empty visor effect and scale 1.0f (0x00004D74). The info
// is ProjectileInfo's 0x1064.
// @ fn_10_4C40
void CDarkSamus::FirePhazonSuperMissile(CStateManager& mgr) {
  const CVector3f from(mRootPos.GetX() + 0.f, mRootPos.GetY() + 0.f, mRootPos.GetZ() + 5.3f);
  const CVector3f aim = mgr.GetPlayer(0)->GetAimPosition(mgr, 0.f);
  const CVector3f to(aim.GetX() + 0.f, aim.GetY() + 0.f, aim.GetZ() + -0.5f);
  CTransform4f xf = GetTransform();
  xf = CTransform4f::LookAt(from, to, CVector3f::Up());
  LaunchProjectile(xf, mgr, 14, 0, true, CImpactVisorEffect::None(), CVector3f(1.f, 1.f, 1.f));
}

// ---- dsfsm2 slice 9: the boost ball's spawn ----

namespace {

// Port stand-in for the module's boost ball actor (vtable lbl_10_data_104, 0x358 bytes; ctors
// fn_10_3FD8 and fn_10_3BC4, which adds a fade 0.5 and the tint fn_10_181E0). Only the
// construction is ported: the CPhysicsActor "Dark Samus BoostBall" (lbl_10_rodata_A4 + 0x15) with
// the materials kMT_Character / Target / Orbit / RadarObject (lbl_10_data_E4..F0), the box
// (-0.2, 0.2) (lbl_10_rodata_9C / 98), mass 0.75 (lbl_10_rodata_A0, fn_10_3F70), the actor
// parameters hot in thermal (fn_10_4368) and the default step data (0x000041E8), then the default
// animation set looping (0x00004340, 0x0000434C). The rest of the class (Think fn_10_3B08 with its
// CCollisionActorManager fn_10_31E4, AcceptScriptMsg fn_10_3998, the render fn_10_3008 / 3174, the
// touch damage from the 0xE10 damage info and the vulnerability, the fade and tint) is a job of its
// own and not in the port: the host ball is drawn and moved (UpdateBoostBallAtActor) but neither
// collides nor hurts.
class CDarkSamusBoostBallPortStub : public CPhysicsActor {
public:
  CDarkSamusBoostBallPortStub(TUniqueId uid, TAreaId area, const CTransform4f& xf,
                              const CModelData& model, int defaultAnim)
  : CPhysicsActor(uid, rstl::string_l("Dark Samus BoostBall"),
                  CEntityInfo(area, CEntity::NullConnectionList, true, kInvalidEditorId), 0, xf,
                  model, CMaterialList(kMT_Character, kMT_Target, kMT_Orbit, kMT_RadarObject),
                  CAABox(CVector3f(-0.2f, -0.2f, -0.2f), CVector3f(0.2f, 0.2f, 0.2f)),
                  SMoverData(0.75f), CActorParameters().HotInThermal(true),
                  CPhysicsActor::skDefaultStepData) {
    if (CAnimData* anim = AnimationData()) {
      anim->SetAnimation(CAnimPlaybackParms(defaultAnim, -1, 1.f, true), false); // 0x00004340
      ModelData()->EnableLooping(true);                                           // 0x0000434C
    }
  }
};

// The two callees of fn_10_9E00 the port lacks, reported once each as PortCDarkSamus.cpp's StandIn.
void SpawnStandIn(bool& seen, const char* fn, const char* what) {
  if (!seen) {
    seen = true;
    fprintf(stderr, "[darksamus] stand-in %s (%s)\n", fn, what);
  }
}

bool sStandIn99B8 = false;
bool sStandIn9B24 = false;

} // namespace

// fn_10_9E00 (0x00009E00, 0x26C bytes): the old ball deleted (fn_10_9AC4, 0x00009E24) and its
// effects stopped (fn_10_99B8, 0x00009E30), a new uid to 0xE38 (0x00009E4C), and the ball built at
// the actor's transform with the z column set to (0, 0, 1) (lbl_10_rodata_25C / 264, 0x00009E6C)
// from the CAnimRes of the boost ball model (the 0xE10 sub-object's 0xE2C / 0xE30 / 0xE34: the
// data's mBoostBallModel), the actor's model scale and looping (0x00009EB8). 0xD14 == 0.0f picks
// the ctor fn_10_3FD8 (0x00009ED0), else fn_10_3BC4; both are the stub above. Then lighting on
// (0x0000A024), AddObject (0x0000A030) and the swooshes fn_10_9B24 (0x0000A03C).
// Port limits: fn_10_99B8 is the darksamus2 lane's StopBoostBallParticles (613d2cbfb, not merged
// here; the merge swaps the call in) and fn_10_9B24 is not in the port, so both stay stand-ins.
void CDarkSamus::SpawnBoostBall(CStateManager& mgr) {
  DeleteBoostBall(mgr); // fn_10_9AC4
  SpawnStandIn(sStandIn99B8, "fn_10_99B8", "SpawnBoostBall (0x00009E30)");
  mBoostBallUid = mgr.AllocateUniqueId();
  CTransform4f xf = GetTransform();
  xf.SetColumn(kDZ, CVector3f(0.f, 0.f, 1.f));
  const CAnimationParameters& res = mData.mBoostBallModel;
  const CModelData model(CAnimRes(res.GetACSFile(), res.GetCharacter(), GetModelData()->GetScale(),
                                  res.GetInitialAnimation(), true));
  CDarkSamusBoostBallPortStub* ball;
  if (mGuestD14 == 0.f) { // 0x00009ED0
    ball = rs_new CDarkSamusBoostBallPortStub(mBoostBallUid, GetCurrentAreaId(), xf, model,
                                              res.GetInitialAnimation()); // fn_10_3FD8
  } else {
    ball = rs_new CDarkSamusBoostBallPortStub(mBoostBallUid, GetCurrentAreaId(), xf, model,
                                              res.GetInitialAnimation()); // fn_10_3BC4
  }
  ball->SetCalculateLighting(true);
  mgr.AddObject(ball);
  SpawnStandIn(sStandIn9B24, "fn_10_9B24", "SpawnBoostBall (0x0000A03C)");
}
