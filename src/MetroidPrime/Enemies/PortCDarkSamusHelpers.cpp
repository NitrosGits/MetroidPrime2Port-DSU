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
  // Host guard: retail reads the sphere's vtable unchecked (the enrage made it first); a forced
  // phazon state with no sphere would crash here.
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
