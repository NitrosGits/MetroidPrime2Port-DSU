// Port of the Dark Samus REL (module 10) helpers: the stateless functions of the module that are
// not FSM handlers (the ones below 0xA000). The function map, the caller and the arm tables are
// in docs/module-analysis/DarkSamus_helpers.md; the bodies marked `// @ fn_10_` were run in the
// Sanctuary 10_cliff_rooftop fight (boss u04f7).
#include "MetroidPrime/Enemies/CDarkSamus.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"

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
