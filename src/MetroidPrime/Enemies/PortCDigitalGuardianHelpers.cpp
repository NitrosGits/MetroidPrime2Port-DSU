// Port of the Digital Guardian REL (module 14) helpers: the module's stateless functions that
// are not FSM handlers - the math, target/aim, collider/vulnerability and effect/sound helpers
// that the body's and the head's state machines call. The function map, the caller map and the
// arm tables are in docs/module-analysis/DigitalGuardian_helpers.md.
//
// Every body below carries the retail asm offset of each bit test, compare and constant it came
// from. The bodies that ran in the port's area-53 run are marked `// @ fn_14_<hex>`; the ones
// that did not are listed with the reason in the note.
#include "MetroidPrime/Enemies/CDigitalGuardian.hpp"
#include "MetroidPrime/Enemies/CDigitalGuardianHead.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Tweaks/CTweakBall.hpp"

#include <cmath>

// ---------------------------------------------------------------------------------------------
// CDigitalGuardian - the body (guest 0x7C0 and up).

// fn_14_37FC (0xC0 bytes): the entry of the five-value table (.rodata 0x24C: 0, 0.7853982,
// 1.5707964, 2.3561945, 3.1415927) that is nearest to v. The five compares are
// |v - table[i]| < best (0x381C, 0x383C, 0x385C, 0x387C, 0x389C) and the best starts at the
// table's largest float (.rodata 0x2AC, FLT_MAX), so the first compare always wins once and a
// tie keeps the earlier entry. The return value is the table entry itself (the lfsx at 0x38B4).
float CDigitalGuardian::x37FC_(float v) const {
  static const float kAngles[5] = {0.f, 0.7853982f, 1.5707964f, 2.3561945f, 3.1415927f};
  float best = 3.4028235e38f; // .rodata 0x2AC
  int index = 0;
  for (int i = 0; i < 5; ++i) {
    const float d = std::fabs(v - kAngles[i]);
    if (d < best) { // the fcmpo at 0x381C and the three after it
      best = d;
      index = i;
    }
  }
  return kAngles[index];
}
//


// fn_14_6C74 (0x3C bytes): the guardian's attack angle. The vortex mode (the s32 at guest
// this+0x11B8) picks 4*pi (.rodata 0x2C4, 12.566371); else bit 6 of the flag byte (guest
// this+0x128C - the extrwi at 0x6C90 takes the byte's bit 6, which is 1u << (31 - 25)) picks
// pi/3 (.rodata 0x2C8, 1.0471976); else 2*pi/3 (.rodata 0x2CC, 2.0943952).
float CDigitalGuardian::x6C74_() const {
  if (x11B8_ != 0) { // the cmpwi at 0x6C78
    return 12.566371f; // .rodata 0x2C4
  }
  if ((x128C_ & 0x40) != 0) { // the extrwi at 0x6C90 (1 bit at 25 -> the byte's bit 6)
    return 1.0471976f;         // .rodata 0x2C8
  }
  return 2.0943952f; // .rodata 0x2CC
}
//


// fn_14_3F90 (0xF0 bytes): the four knee armours follow the knee locators. Each armour's id
// (guest this+0xFB0 + 2i) is looked up (the ObjectById call at 0x3FE8); an armour that is gone
// has its id cleared (the store at 0x4054). The armour's transform is the locator's (the
// GetLctrTransform call at 0x401C) times the leg segment's rest rotation (the BuildTransform4f
// call at 0x400C, the __ml__ at 0x402C) and it is set on the actor (the SetTransform at 0x4044).
void CDigitalGuardian::x3F90_(CStateManager& mgr) {
  u16* ids[4] = {&xFB0_, &xFB2_, &xFB4_, &xFB6_}; // guest 0xFB0, 0xFB2, 0xFB4, 0xFB6
  const uchar segs[4] = {x10C0_, x10C1_, x10C2_, x10C3_}; // guest 0x10C0..0x10C3
  for (int i = 0; i < 4; ++i) {
    if (*ids[i] == kInvalidUniqueId.value) { // the cmplw at 0x3FD0
      continue;
    }
    CActor* armour = TCastToPtr< CActor >(mgr.ObjectById(TUniqueId(*ids[i])));
    if (armour == nullptr) { // the retail's test at 0x3FF0 (mr. r24, r3 / beq)
      *ids[i] = kInvalidUniqueId.value; // the store at 0x4054
      continue;
    }
    const CSegId seg(segs[i]);
    // The BuildTransform4f call at 0x400C (the segment's rest rotation) is the port's
    // SegmentRotation_ helper; the locator's transform is the GetLctrTransform call at 0x401C.
    const CTransform4f xf = GetLctrTransform(seg) * SegmentRotation_(seg);
    armour->SetTransform(xf); // the SetTransform call at 0x4044
  }
}


// fn_14_38BC (0xB8 bytes): the leg-stab and jump timers count down (guest this+0x11E0 and
// this+0x11E4, the stores at 0x38EC and 0x38F8). The "protect the knees" timer (this+0x1274)
// counts down too, except when the player is morphed (the player's morph state at +0x38C, which
// counts only when +0x390 is 0) and fewer than four knees are left (the x705C call at 0x38FC):
// then it is reset to the data block's +0x170 (guest this+0x930, the load at 0x393C).
void CDigitalGuardian::x38BC_(CStateManager& mgr, float dt) {
  x11E0_ -= dt; // the store at 0x38EC
  x11E4_ -= dt; // the store at 0x38F8
  if (x705C_() >= 4) { // the cmpwi at 0x3900
    x1274_ -= dt; // the store at 0x3950
    return;
  }
  const CPlayer* player = mgr.GetPlayer(0);
  // The retail reads the player's spawned state (+0x390) first and only then the morph
  // transition state (+0x38C); the morphed state is 1.
  const CPlayer::EPlayerMorphBallState state =
      player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed
          ? player->GetMorphballTransitionState()
          : CPlayer::kMS_Unmorphed;
  if (state == CPlayer::kMS_Morphed) { // the cmpwi at 0x3924
    x1274_ = mData.mUnknownA324E26C; // guest 0x930 (the data block's +0x170), the store at 0x3940
    return;
  }
  x1274_ -= dt; // the store at 0x3934
}


// fn_14_2358 (0x18C bytes): the four toe targets' sound emitters follow the toe locators
// (guest this+0x10F0 holds the handles, this+0x10D4 the toe locator segment ids). The toe
// targets' hit timers (this+0x1158) count down and are clamped at 0 (the fcmpo at 0x2450), and
// a toe target in the flinch state (the word at this+0x10DC == 2, the cmpwi at 0x247C) goes back
// to the target state (fn_14_1A68 with 1, the call at 0x2494).
void CDigitalGuardian::x2358_(CStateManager& mgr, float dt) {
  const uchar segs[4] = {x10D4_, x10D5_, x10D6_, x10D7_}; // guest 0x10D4..0x10D7
  for (int i = 0; i < 4; ++i) {
    if (x10F0_[i] != CSfxHandle()) { // the cmplwi at 0x23C0
      const CSegId seg(segs[i]);
      // The BuildTransform4f call at 0x23E4 (the segment's rest rotation) is the port's
      // SegmentRotation_ helper; the locator's transform is the GetLctrTransform call at 0x23F4.
      const CTransform4f xf = GetLctrTransform(seg) * SegmentRotation_(seg);
      CSfxManager::UpdateEmitter(x10F0_[i], xf.GetTranslation(), CVector3f::Up(), 0x7F);
    }
    float timer = x1158_[i] - dt; // the fsubs at 0x244C
    if (timer < 0.f) { // the fcmpo at 0x2450 (.rodata 0x264 is 0)
      timer = 0.f; // the load at 0x2460
    }
    x1158_[i] = timer; // the store at 0x2464
    // f31 is .rodata 0x264 (0) too: the lfs at 0x23A0 reads 0(r3) after the lfsu at 0x2390.
    if (timer <= 0.f && x10DC_[i] == 2) { // the fcmpo at 0x246C and the cmpwi at 0x247C
      x1A68_(mgr, i, 1); // fn_14_1A68: the toe target's state 1
    }
  }
}


// fn_14_5644 (0x1F4 bytes): the "XRDG" script message - a toe target hit by the player's boost
// ball. The collider (the message's sender) has to be one of the four toe target colliders
// (fn_14_43AC, the call at 0x5690), the toe target must not be skipped (the word at
// this+0x10DC == 3, the cmpwi at 0x56CC), its hit timer must have run out (this+0x1158, the
// fcmpo at 0x56E0), the player must be morphed (the player's +0x38C, read at 0x56F8) and its
// boost shield must be up (the player's +0x1174, the IsBoostShieldActive call at 0x5710).
// Then the boost ball's damage is applied to the toe target's vulnerability (this+0x1110, the
// GetVulnerableDamage call at 0x5784): the toe target's health (this+0x1144) drops and its hit
// timer is set to the damage wait time (this+0x444, the store at 0x57A0). At zero health the
// toe target is destroyed (fn_14_1A68 with 3) and the IS08 message goes to the waypoint
// (fn_14_143C); otherwise the hit sound plays and a toe target in state 1 flinches
// (fn_14_1A68 with 2).
// The arms: (a) the sender is not a collider, (b) it is a collider but not a toe target, (c) the
// toe target is skipped (the word at guest this+0x10DC is 3), (d) its hit timer is running, (e)
// the player is not morphed, (f) the player is morphed without a boost shield, (g) the toe target
// dies: the explosion and the IS08 message, (h) it survives: the hit sound and the flinch, (i)
// it survives without the flinch.
// The run: one force run per arm (a member a force writes stays written, so the arms need their
// own runs), tools/cov/force.sh with the specs /tmp/opencode/dg5644_<arm>.py (the same list as
// in section 3.6 of docs/module-analysis/DigitalGuardian_helpers.md), area 53 (Sanctuary
// Fortress 09_cliff_temple_dark), 3000 frames; the call-site counts of each run are in that
// section.
// @ fn_14_5644
void CDigitalGuardian::x5644_(CStateManager& mgr, TUniqueId sender) {
  CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(sender));
  if (actor == nullptr) { // the retail's test at 0x567C (mr. r31, r3 / beq)
    return;
  }
  if (!x43AC_(sender)) { // fn_14_43AC: is it one of the four toe target colliders?
    return;
  }
  const int toe = x4350_(sender); // fn_14_4350: which toe target, or -1
  if (toe < 0) { // the cmpwi at 0x56B8
    return;
  }
  if (x10DC_[toe] == 3) { // the cmpwi at 0x56CC
    return;
  }
  if (x1158_[toe] > 0.f) { // the fcmpo at 0x56E0
    return;
  }
  const CPlayer* player = mgr.GetPlayer(0);
  const CPlayer::EPlayerMorphBallState state =
      player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed
          ? player->GetMorphballTransitionState()
          : CPlayer::kMS_Unmorphed;
  if (state != CPlayer::kMS_Morphed) { // the cmpwi at 0x5704
    return;
  }
  if (!player->GetMorphBall()->IsBoostShieldActive()) { // the call at 0x5710
    return;
  }
  const CDamageInfo damage = gpTweakBall->GetBoostBallDamage(); // the call at 0x572C
  x1144_[toe] -= damage.GetVulnerableDamage(x1110_);           // the fsubs at 0x5790
  x1158_[toe] = mDamageWaitTime;                               // this+0x444, the store at 0x57A0
  if (x1144_[toe] <= 0.f) {                                    // the fcmpo at 0x57A8
    x1A68_(mgr, toe, 3);              // fn_14_1A68: the toe target is destroyed
    x143C_(mgr, actor->GetTranslation(), kSS_InternalState8); // fn_14_143C: the IS08 message
    return;
  }
  PlayCustomSound(actor->GetTranslation(), CVector3f::Up(), mData.mSoundToeTargetHit, false);
  if (x10DC_[toe] == 1) { // the cmpwi at 0x5808
    x1A68_(mgr, toe, 2); // fn_14_1A68: the flinch
  }
}
//
// Unmarked: the port's "XRDG" case calls it, but the run never sent that message.


// fn_14_6C24 (0x50 bytes): is the head's turn direction unusable? The guardian's body state
// (the body controller's state id, guest this+0x48C -> +0x37C, the load at 0x6C3C) is 0 -> the
// answer is yes; otherwise the head's turn direction (head+0x1028) is checked: a direction
// whose magnitude is not safe means "no direction" (the IsMagnitudeSafe call at 0x6C4C).
bool CDigitalGuardian::x6C24_(CDigitalGuardianHead& head) {
  if (BodyController()->GetCurrentStateId() == 0) { // the cmpwi at 0x6C40 (the guest's +0x37C)
    return true;
  }
  return !head.x1028_.IsMagnitudeSafe(); // the call at 0x6C4C
}


// fn_14_6CB0 (0x154 bytes): the guardian's aim. When the head has no usable direction (fn_14_6C24)
// and the flag byte's bit 1 (guest this+0x128C, the extrwi at 0x6CE0) is clear, the guardian
// looks along the head's flattened forward (fn_14_13B38) instead: the direction is turned into
// the guardian's frame (the TransposeRotate call at 0x6D08) and the guardian's aim transform
// (guest this+0x1088) becomes the LookAt of that direction (the call at 0x6D3C). With a usable
// direction the aim is the given transform (r5) times the aim transform (the __ml__ at 0x6D60)
// and its forward and up columns are the two results; without one the direction is the head's
// normalized turn direction and the up vector is the world's up (the branch at 0x6DA4). The flag
// byte's bit 1 then says whether the direction was usable (the rlwimi at 0x6DE8).
void CDigitalGuardian::x6CB0_(CDigitalGuardianHead& head, const CTransform4f& xf,
                             CVector3f& outForward, CVector3f& outUp) {
  const bool usable = x6C24_(head); // the call at 0x6CD4
  // The clrlwi. / beq at 0x6CE8: the offset is taken when the aim becomes usable.
  if ((x128C_ & 0x02) == 0 && usable) { // the extrwi at 0x6CE0 (1 bit at 30 -> the byte's bit 1)
    const CVector3f dir = head.x13B38_();           // fn_14_13B38
    const CVector3f local = GetTransform().TransposeRotate(dir); // the call at 0x6D08
    x1088_ = CTransform4f::LookAt(CVector3f::Zero(), local, CVector3f::Up()); // the call at 0x6D3C
  }
  if (usable) { // the clrlwi at 0x6D4C
    const CTransform4f aim = xf * x1088_; // the __ml__ at 0x6D60
    outForward = aim.GetColumn(kDY);     // the loads at 0x6D70..0x6D78
    outUp = aim.GetColumn(kDZ);          // the loads at 0x6D88..0x6D90
  } else {
    outForward = head.x1028_.AsNormalized(); // the call at 0x6DAC
    outUp = CVector3f::Up();                // the loads at 0x6DCC..0x6DD8
  }
  // The rlwimi at 0x6DE8 puts "usable" into the flag byte's bit 1 (1u << (31 - 30) = 0x02).
  if (usable) {
    x128C_ |= 0x02;
  } else {
    x128C_ &= ~0x02;
  }
}
//
// Unmarked: the port has no caller for it yet (fn_14_6E04 calls it).


// fn_14_6E04 (0x258 bytes): the guardian's body turns to the head module. With no head (the
// uid at guest this+0x1082, the cmplw at 0x6E40) nothing happens; with a head that is gone the
// head detaches (fn_14_70E4, the call at 0x7034). Otherwise the body's "Head_1" locator
// transform (the GetLctrTransform call at 0x6E74) is the guardian's origin, and the head's
// forward (head+0x28/0x38/0x48) is turned towards the guardian's forward (fn_14_6CB0, the call
// at 0x6F04). The angle between them (the GetAngleDiff call at 0x6F10) is compared with the
// turn rate (dt times the attack angle, fn_14_6C74, the fmuls at 0x6EC0): while it is smaller
// the head's transform is the LookAt of the guardian's origin and forward (the call at
// 0x6F5C), else the head's forward is slerped towards the guardian's forward (the Slerp call at
// 0x6F90) and the LookAt uses that. At the end, with at most two knees left (the x705C call at
// 0x6FF4), the head's flag byte's bit 5 (head+0x10F4, the extrwi at 0x7004) and the head's
// projectile attack (head+0x10D0 == 1, the cmpwi at 0x7010), the guardian's flag byte's bit 1
// (guest this+0x128C, the rlwimi at 0x7020) is set.
void CDigitalGuardian::x6E04_(CStateManager& mgr, float dt) {
  if (x1082_ == kInvalidUniqueId) { // the cmplw at 0x6E40
    return;
  }
  CDigitalGuardianHead* head = TCastToPtr< CDigitalGuardianHead >(mgr.ObjectById(x1082_));
  if (head == nullptr) { // the retail's test at 0x6E64 (mr. r31, r3 / beq)
    x70E4_(mgr); // fn_14_70E4: the head detaches
    return;
  }
  const CTransform4f lctr = GetLctrTransform(CSegId(x10B8_)); // the call at 0x6E74
  const CVector3f headForward = head->GetTransform().GetColumn(kDY); // head+0x28/0x38/0x48
  const CVector3f bodyForward = lctr.GetColumn(kDY);                // the copy at 0x6E80
  const CVector3f bodyUp = lctr.GetColumn(kDZ);                      // the copy at 0x6E80
  const CVector3f origin = lctr.GetTranslation();                    // the loads at 0x6E84..0x6E9C
  CVector3f aimForward = bodyForward;
  CVector3f aimUp = bodyUp;
  x6CB0_(*head, lctr, aimForward, aimUp); // the call at 0x6F04
  const float turn = dt * x6C74_();      // the fmuls at 0x6EC0
  if (CVector3f::GetAngleDiff(headForward, aimForward) < turn) { // the call at 0x6F10, the fcmpo
    head->SetTransform(
        CTransform4f::LookAt(origin, origin + aimForward, aimUp)); // the call at 0x6F5C (sp 0x40, 0x34)
  } else {
    const CVector3f slerped =
        CVector3f::Slerp(headForward, aimForward, CRelAngle::FromRadians(turn)); // 0x6F90
    head->SetTransform(
        CTransform4f::LookAt(origin, origin + slerped, aimUp)); // the call at 0x6FD4 (up sp 0x34)
  }
  // The extrwi at 0x7004 takes 1 bit at 26 (the byte's bit 5, 1u << (31 - 26) = 0x20).
  if (x705C_() <= 2 && (head->x10F4_ & 0x20) != 0 && head->x10D0_ == 1) { // 0x6FF8, 0x7010
    x128C_ |= 0x01; // the rlwimi at 0x7020 (mask 31,31: the byte's bit 0)
  }
}


// ---------------------------------------------------------------------------------------------
// CDigitalGuardianHead - the head module (guest 0x7C0 and up).

// fn_14_10120 (0xC0 bytes): the head's projectile attack block. The number of the guardian's
// knees that are left (fn_14_705C, the call at 0x1015C) picks one of the six
// SLdrDigitalGuardianHeadStruct blocks of the parameter block (guest this+0xA10, 0xA2C, 0xA48,
// 0xA64, 0xA80, 0xA9C - the data block's +0x250, +0x26C, +0x288, +0x2A4, +0x2C0 and +0x2DC).
// With no knee left the first block is used, unless the head's flag byte's bit 4 (head+0x10F4,
// the extrwi at 0x10198) is set: then the last block is used (the default at 0x10168).
SLdrDigitalGuardianHeadStruct* CDigitalGuardianHead::x10120_(CStateManager& mgr) {
  int knees = 0;
  CDigitalGuardian* body =
      TCastToPtr< CDigitalGuardian >(mgr.ObjectById(x1034_)); // the call at 0x1014C
  if (body != nullptr) { // the retail's test at 0x10158 (cmplwi r3, 0x0 / beq)
    knees = body->x705C_();                                  // the call at 0x1015C
  }
  switch (knees) {
  case 0:
    // The extrwi at 0x10198 takes 1 bit at 27 (the byte's bit 4, 1u << (31 - 27) = 0x10).
    if ((x10F4_ & 0x10) == 0) { // the extrwi at 0x10198
      return &mData.digitalGuardianHeadStruct_0x2dd88764; // guest 0xA80 (the data block's +0x2C0)
    }
    return &mData.digitalGuardianHeadStruct_0x48b46e55; // guest 0xA9C (the data block's +0x2DC)
  case 1:
    return &mData.digitalGuardianHeadStruct; // guest 0xA10 (the data block's +0x250)
  case 2:
    return &mData.digitalGuardianHeadStruct_0x8e128141; // guest 0xA2C (the data block's +0x26C)
  case 3:
    return &mData.digitalGuardianHeadStruct_0xea54b390; // guest 0xA48 (the data block's +0x288)
  case 4:
    return &mData.digitalGuardianHeadStruct_0xbbd3e7a7; // guest 0xA64 (the data block's +0x2A4)
  default:
    return &mData.digitalGuardianHeadStruct_0x48b46e55; // guest 0xA9C (the data block's +0x2DC)
  }
}
//
// Unmarked: the port has no caller for it yet (the head's codes call it).


// fn_14_F374 (0x1C bytes): the retreat ride's time limit (FollowRetreatPath, fn_14_1569C). With
// two or more lives left (head+0x10E4) the data block's +0x24 (guest 0x7E4), else its +0x28
// (guest 0x7E8).
// Both arms ran under FollowRetreatPath (area 53): 0x10E4 = 2 naturally, 1 forced at the entry.
// @ fn_14_F374
float CDigitalGuardianHead::xF374_() const {
  if (x10E4_ >= 2) {                 // cmpwi 2 / bge at 0xF378..0xF37C
    return mData.unknown_0x8a83a097; // lfs 0x7E4 at 0xF388 (the data block's +0x24)
  }
  return mData.unknown_0xd919fb13; // lfs 0x7E8 at 0xF380 (the data block's +0x28)
}
//


// fn_14_11EA8 (0x88 bytes): the destroyed bomb slots' looping sounds are stopped. Every handle
// at head+0xF28 is removed (the RemoveEmitter call at 0x11EF0) and cleared (the store at
// 0x11EF4); the walk ends after the count at head+0xF24 entries (the load at 0x11EFC).
void CDigitalGuardianHead::x11EA8_() {
  for (int i = 0; i < 2; ++i) { // the count at head+0xF24 is 2
    if (xF28_[i] != CSfxHandle()) { // the cmplwi at 0x11EDC
      CSfxManager::RemoveEmitter(xF28_[i]); // the call at 0x11EF0
      xF28_[i] = CSfxHandle();               // the store at 0x11EF4
    }
  }
}


// fn_14_11F7C (0x70 bytes): the echo targets are deleted. Every id at head+0xF34 is deleted
// (the DeleteObjectRequest call at 0x11FB8) and cleared (the store at 0x11FC4); the walk ends
// after the count at head+0xF30 entries (the load at 0x11FCC).
//
// The arms: (a) the walk deletes an echo target and clears its id, (b) the walk ends when the
// count at head+0xF30 is reached. Both ran in the area-53 run (Sanctuary Fortress
// 09_cliff_temple_dark, 3000 frames, tools/cov/run.sh with
// tools/cov/specs/digitalguardian.py): the entry was hit once and the DeleteObjectRequest call
// site three times, so the three echo targets were deleted and the walk ended.
// @ fn_14_11F7C
void CDigitalGuardianHead::x11F7C_(CStateManager& mgr) {
  for (int i = 0; i < 3; ++i) { // the count at head+0xF30 is 3
    mgr.DeleteObjectRequest(xF34_[i]); // the call at 0x11FB8
    xF34_[i] = kInvalidUniqueId;         // the store at 0x11FC4
  }
}
//
// The arms: (a) the vortex mode (guest this+0x11B8) is not 0 -> 4*pi, (b) the flag byte's
// bit 6 (guest this+0x128C) is set -> pi/3, (c) neither -> 2*pi/3.
// The run: Sanctuary Fortress 09_cliff_temple_dark (area 53), 3000 frames, the coverage harness
// with the forcing (tools/cov/force.sh and tools/cov/specs/digitalguardian_force.py): (c) ran in
// the first 101 frames, (a) was forced (the vortex mode set to 1 at the entry, the hits 101-200)
// and (b) was forced (the flag byte's bit 6 set at the entry, the hits 201+).
// @ fn_14_6C74
//
// The arms: (a) the knee's id is invalid -> the walk skips that knee, (b) the actor the id
// names is gone -> the walk clears the id, (c) the armour is there -> its transform is set.
// The run: area 53, 3000 frames, the coverage harness with the forcing: (c) ran in the first
// 101 frames (both ids valid, the CActor::SetTransform call site), (a) was forced (the second
// knee id cleared at the entry, the hits 101-200) and (b) was forced (the third knee id pointed
// at an actor that does not exist, the hits 201+; the body then cleared the id).
// @ fn_14_3F90
//
// The arms: (a) four knees left -> the protect timer counts down, (b) fewer knees and the
// player is morphed -> the timer is reset to the data block's +0x170, (c) fewer knees and the
// player is not morphed -> the timer counts down.
// The run: area 53, 3000 frames, the coverage harness with the forcing: (a) ran in the first
// 101 frames (four knees, the key "4, 0"), (c) ran with the knee count forced under four (the
// second knee id cleared at the entry, the hits 101-300, the keys "3, 0" and "2, 0") and (b)
// was forced as well (the player's morph state set to 1 at the entry, the hits 301+, the key
// "2, 1").
// @ fn_14_38BC
//
// The arms: (a) the toe target has a sound emitter -> it follows the toe locator, (b) the toe
// target is flinching (the word at guest this+0x10DC is 2) -> it goes back to the target state.
// The run: area 53, 3000 frames, the coverage harness with the forcing: (a) ran every frame
// (the CSfxManager::UpdateEmitter call site) and (b) ran too (the fn_14_1A68 call site; the
// flinch was forced by setting the first toe target's state word to 2 at the entry).
// @ fn_14_2358
//
// The arms: (a) the guardian's body state (the body controller's state id, guest this+0x48C ->
// +0x37C) is 0 -> the head's direction does not matter, (b) the head's turn direction
// (head+0x1028) is not magnitude-safe -> the direction is unusable, (c) neither -> the direction
// is usable.
// The run: area 53, 3000 frames, the coverage harness with the forcing: (c) ran in the frames
// where the head's direction was safe (x6CB0's CTransform4f::operator* call site), (b) ran in
// the frames where it was not (x6CB0's CVector3f::AsNormalized call site) and (a) was forced (the
// body controller's state id set to 0 at the entry, the hits 101+; the
// CVector3f::IsMagnitudeSafe call site then stopped).
// @ fn_14_6C24
//
// The arms: (a) the flag byte's bit 1 (guest this+0x128C) is clear and the head has no usable
// direction -> the aim transform is rebuilt from the head's flattened forward, (b) the direction
// is usable -> the aim is the given transform times the aim transform, (c) it is not -> the
// direction is the head's normalized turn direction and the up vector is the world's up.
// The run: area 53, 3000 frames, the coverage harness with the forcing: (b) ran (the
// CTransform4f::operator* call site), (c) ran (the CVector3f::AsNormalized call site) and (a)
// ran too (the fn_14_13B38, the CTransform4f::TransposeRotate and the CTransform4f::LookAt call
// sites; the flag byte's bit 1 was forced clear at the entry so that the branch was taken).
// @ fn_14_6CB0
//
// The arms: (a) the bomb slot has a sound -> the emitter is removed and the handle cleared,
// (b) the slot has no sound -> nothing happens.
// The run: area 53, 3000 frames, the coverage harness: (b) ran in the plain run (the head's
// "XDEL" reached the body and no bomb slot had a sound) and (a) was forced (a sound handle
// written into the first bomb slot at the entry; the CSfxManager::RemoveEmitter call site then
// fired).
// @ fn_14_11EA8

// fn_14_137A8 (0x88 bytes): the head's armour model. The armour's asset id (the parameter
// block's +0x0C, guest this+0x7CC) is put into the head's model (guest this+0xEA4), unless the
// id is 0xFFFE (the addis at 0x137C0 and the cmplwi at 0x137C4). The model is built from the
// asset id and the actor's model scale (the three lfs at 0x137D8..0x137E0) and assigned into the
// optional (fn_14_6A20 at 0x1380C).
void CDigitalGuardianHead::x137A8_() {
  const uint armour = mData.headArmor; // guest 0x7CC (the data block's +0x0C)
  if (armour + 1 == 0xFFFFu) {       // the addis at 0x137C0 and the cmplwi at 0x137C4
    return;
  }
  const CVector3f scale = GetModelData()->GetScale();
  xEA4_ = CModelData(CStaticRes(armour, scale));
}
//
// Unmarked: the port has no caller for it yet (the head's constructor calls it).

// fn_14_83C0 (0x184 bytes): the knee's knock-back animation parm data. The struct the retail
// fills is {u32 stateId; u32 parmCount; CPASAnimParm parms[8]} - the host's CPASAnimParmData
// holds the same data in the same order (the state id, then the reserved_vector's count and its
// eight parameters):
//  - the state id (the first word): the caller's "li r4, 0x6" (0x81D0) = pas::kAS_KnockBack,
//    stored at 0x83F0;
//  - the parameter count (the second word): the size the callee counts up from 0 (the store at
//    0x83FC) to 8 (the eight stores at 0x8414, 0x8448, 0x846C, 0x848C, 0x84A8, 0x84C8, 0x84EC and
//    0x8514);
//  - the eight parameters, each a pair of words (a CPASAnimParm is 8 bytes: the value and the
//    type): the knee's angle first (.rodata 0x20C, the FromReal32 call at 0x81A0), then the enum
//    2 (the FromEnum call at 0x8184), then the six NoParameter ones (the NoParameter calls at
//    0x8150, 0x8158, 0x8160, 0x8168, 0x8170 and 0x8178).
// The host's layout is the same as the retail's: its reserved_vector is {int mCount; uchar
// mData[N * sizeof(T)]}, so the count sits where the retail's had it.
//
// Unmarked: the port's fn_14_8090 body (KneeDestroyedReaction, the FSM lane's, already marked)
// builds the same data inline with the host's constructor, so this helper has no caller yet.
CPASAnimParmData CDigitalGuardian::x83C0_(pas::EAnimationState state, const CPASAnimParm& p1,
                                         const CPASAnimParm& p2, const CPASAnimParm& p3,
                                         const CPASAnimParm& p4, const CPASAnimParm& p5,
                                         const CPASAnimParm& p6, const CPASAnimParm& p7,
                                         const CPASAnimParm& p8) const {
  return CPASAnimParmData(state, p1, p2, p3, p4, p5, p6, p7, p8);
}
