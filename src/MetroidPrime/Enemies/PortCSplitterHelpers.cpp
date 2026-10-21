// The Splitter REL (module 75)'s stateless helper half: the bodies the module's own classes
// forward to the base class or to the module's other objects.
//
// The FSM lane (decomp/splitter) owns the module's state machine, the two classes' bodies in
// src/MetroidPrime/Enemies/PortCSplitterMainChassis.cpp and PortCSplitterCommandModule.cpp and
// the module's note (docs/module-analysis/Splitter.md); this file holds the helpers this lane
// ports, and its note is docs/module-analysis/Splitter_helpers.md.
//
// The bodies here are the REL's own: every bit test, mask, compare and constant carries the
// retail asm offset it came from. The retail's offsets are the module's own (0x0000... is the
// module offset, 0x... the DOL address); the module's asm is
// <archive>/build/G2ME01/Splitter/asm.
#include "MetroidPrime/Enemies/CSplitterCommandModule.hpp"
#include "MetroidPrime/Enemies/CSplitterMainChassis.hpp"

#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CIngSnatchingSwarm.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "Kyoto/Animation/CInt32POINode.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include "Kyoto/Math/CVector3f.hpp"

// ---------------------------------------------------------------------------------------------
// The head (CSplitterCommandModule).

// @ fn_75_6310
// The head's AddToRenderer (vtable slot 0x28). The base class's call runs only while the actor
// is alive: the base's flag byte at 0x420, its 0x40 bit (the word's bit 25), read at asm 0x6320
// (0x0000631C lbz, 0x00006320 extrwi with the word's bit 25) and tested at asm 0x6324 (beq).
// The port's shell called the base unconditionally, so the head drew itself while it was
// dead.
void CSplitterCommandModule::PortAddToRenderer(const CStateManager& mgr) const {
  if (GetAlive()) {
    CPatterned::AddToRenderer(mgr);
  }
}

// @ fn_75_633C
// The head's PreRender (vtable slot 0x24): the same alive gate as fn_75_6310 - the byte at 0x420,
// its 0x40 bit, read at asm 0x6348 (0x0000634C extrwi with the word's bit 25) and tested at asm
// 0x6350 (beq).
void CSplitterCommandModule::PortPreRender(CStateManager& mgr) {
  if (GetAlive()) {
    CPatterned::PreRender(mgr);
  }
}

// @ fn_75_6368
// The head's Render (vtable slot 0x2C): the same alive gate - the byte at 0x420, its 0x40 bit, read
// at asm 0x6374 (0x00006378 extrwi with the word's bit 25) and tested at asm 0x637C (beq).
void CSplitterCommandModule::PortRender(const CStateManager& mgr) const {
  if (GetAlive()) {
    CPatterned::Render(mgr);
  }
}

// @ fn_75_5E50
// The head's CanBeIngPossessed (vtable slot 0xF8). The head is only possessed while it is
// docked: the chassis's id at 0xEFA (asm 0x5E60 lhz) is compared with kInvalidUniqueId (asm
// 0x5E64, 0x5E68 cmplw) and a valid id ends the body with false (asm 0x5E6C beq, 0x5E70 li 0);
// with no chassis the base class's own answer is returned (asm 0x5E78 bl
// CanBeIngPossessed__10CPatternedCFR13CStateManager).
bool CSplitterCommandModule::PortCanBeIngPossessed(CStateManager& mgr) const {
  if (mChassisId != kInvalidUniqueId) {
    return false;
  }
  return CPatterned::CanBeIngPossessed(mgr);
}

// @ fn_75_5F18
// The head's Listen (vtable slot 0xB0). The body: the alive gate (the byte at 0x420, its 0x40
// bit, asm 0x5F18 lbz and 0x5F20 extrwi with the word's bit 25, tested at asm 0x5F24 beq), the
// noise type (asm 0x5F28 cmpwi 4, 0x5F2C beq - EListenNoiseType 4, kLNT_PathObstruction) and
// the distance to the noise: the difference between the noise's position (r5) and the head's own
// position (the three floats at 0x54, 0x58 and 0x5C, asm 0x5F3C, 0x5F48 and 0x5F54), squared
// (asm 0x5F58, 0x5F64 and 0x5F68) and compared with the rodata constant 400.0f
// (lbl_75_rodata_98, asm 0x5F60; 400 is 20 squared, so the radius is 20) - a distance of 20 or
// more ends the body (asm 0x5F6C fcmpo, 0x5F70 bge). Inside the radius the head's own flag byte
// at 0xF6A gets its 0x40 bit (asm 0x5F74 lbz, 0x5F7C rlwimi with the word's bit 25, 0x5F80 stb)
// and the body returns true (asm 0x5F84 clrlwi).
bool CSplitterCommandModule::PortListen(CStateManager& mgr, const CVector3f& position,
                                        EListenNoiseType type) {
  if (!GetAlive()) {
    return false;
  }
  if (type != kLNT_PathObstruction) {
    return false;
  }
  const CVector3f delta = position - GetTranslation();
  if (delta.MagSquared() >= 400.f) {
    return false;
  }
  mAutoDestructFlags |= 0x40;
  return true;
}

// ---------------------------------------------------------------------------------------------
// The main chassis (CSplitterMainChassis).


// ---------------------------------------------------------------------------------------------
// The head's DoUserAnimEvent (the REL's vtable slot 0x70).

// @ fn_75_6394
// The head's DoUserAnimEvent (vtable slot 0x70).  The note's section f row read
// "0x70 | GetSortingBounds | fn_75_6394", but the REL's own vtable (lbl_75_data_448) has the
// DOL's `GetSortingBounds__6CActorCFRC13CStateManager` at slot 0x6C and the module's fn_75_6394
// at slot 0x70 - and docs/module-analysis/vtable-offsets.md names the DOL's slot 0x70
// `DoUserAnimEvent(CStateManager&, const CInt32POINode&, EUserEventType, float)`, which is
// exactly this body's signature (r4 the manager, r5 the node, r6 the type, f1 the dt, asm
// 0x63B0-0x63CC).
//
// The body switches on the type (asm 0x63B4 `cmpwi r31, 0x16`, 0x63D8 `cmpwi r31, 0x0`):
//   - the type 0x16 (kUE_BreakLockOn): the head's flag byte at 0xF6A gets its 0x10 bit tested
//     (asm 0x63E4 `lbz`, 0x63E8 `extrwi` with the word's bit 27) and the data's flag word at
//     0x7C0 its 0x20000000 bit (asm 0x63F0 `lwz`, 0x63F4 `rlwinm` with the bits 29..29).  With
//     both clear the head asks the player for an orbit request: the id at 0xEFE (the head's
//     copy of the chassis's target) is looked up (asm 0x63FC `lhz`, 0x640C
//     `ObjectById__13CStateManagerCF9TUniqueId`) and cast to a CPlayer (asm 0x6410
//     `TCastToPtr<7CPlayer>`); with no player the arm ends (asm 0x6414, 0x6418).  Otherwise
//     the player is asked for the orbit request (asm 0x641C-0x6434): the head's own id (the
//     u16 at 0x8) is the target, the request is 8 (kOR_ActivateOrbitSource, asm 0x6428
//     `li r5, 0x8`) and the retail's call
//     `SetOrbitRequestForTarget__7CPlayerF9TUniqueIdQ27CPlayer19EPlayerOrbitRequestR13CStateManager`
//     (asm 0x6434) passes one argument the host's signature has no slot for: the mangled name's
//     `Q27CPlayer` (a CPlayer* by value), which the retail passes in r6 - the host's
//     `CPlayer::SetOrbitRequestForTarget(TUniqueId, EPlayerOrbitRequest, CStateManager&)` has
//     no such parameter, so the host drops it and the player itself is the one that is asked.
//   - the type 0 (kUE_Projectile): the head's own body fn_75_1CA0 runs with the node's locator
//     name (the node's string at +0x30, asm 0x6440 `addi r5, r30, 0x30`, 0x6444 `bl
//     fn_75_1CA0`).
//   Then, and only then, the base class's own DoUserAnimEvent runs (asm 0x644C `clrlwi`, 0x6450
//   `bne`, 0x6454-0x6468: the manager, the node, the type and the dt are put back and the DOL's
//   `DoUserAnimEvent__10CPatternedFR13CStateManagerRC13CInt32POINode14EUserEventTypef` is
//   called).
void CSplitterCommandModule::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                              EUserEventType type, float dt) {
  PortDoUserAnimEvent(mgr, node, type, dt);
}

// @ fn_75_6394
// The head's DoUserAnimEvent (vtable slot 0x70): the two type cases and the base class's own
// call after them (see the comment above).
void CSplitterCommandModule::PortDoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                                EUserEventType type, float dt) {
  bool handled = false;
  if (type == kUE_BreakLockOn) {
    // asm 0x63E4-0x63F8: the head's flag byte at 0xF6A and the data's flag word at 0x7C0.
    if ((mAutoDestructFlags & 0x10) == 0 && (mData.mUnknownBD80FD94 & 0x20000000) == 0) {
      // asm 0x63FC-0x6418: the player at 0xEFE (the head's copy of the chassis's target).
      CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mSyncedId));
      if (player != nullptr) {
        // asm 0x641C-0x6434: the head's own id is the target and the request is 8
        // (kOR_ActivateOrbitSource); the retail's call passes a CPlayer* the host's signature
        // has no slot for, so the host drops it.
        player->SetOrbitRequestForTarget(GetUniqueId(), CPlayer::kOR_ActivateOrbitSource, mgr);
      }
    }
    handled = true;
  } else if (type == kUE_Projectile) {
    // asm 0x6440-0x6444: the retail's own fn_75_1CA0 (the head's projectile aim) - not ported
    // yet, so the case is a shell.
    PortHeadProjectileAim(mgr, node.GetLocatorName());
    handled = true;
  }
  // asm 0x644C-0x6468: the base class's own DoUserAnimEvent runs when a case was taken.
  if (handled) {
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);
  }
}

// fn_75_1CA0 (not marked: the body is not ported yet).  The retail's head's projectile aim
// (0x1CA0-0x1E37): the alive gate (the byte at 0x420, its 0x40 bit, asm 0x1CC8-0x1CD0), the
// player at 0xEFE (asm 0x1CD4-0x1CE4), the node's locator transform (asm 0x1CF8-0x1D08), the
// head's own GetAimPosition (the vtable slot 0x54, asm 0x1D3C-0x1D48, with the rodata constant
// lbl_75_rodata_40 as its dt) and the look-at that follows (asm 0x1D4C-0x1DBC).  The host has
// no body for it yet, so the case is a once-logging shell.
void CSplitterCommandModule::PortHeadProjectileAim(CStateManager& mgr,
                                                   const rstl::string& locatorName) {
  static bool sLogged = false;
  if (!sLogged) {
    sLogged = true;
    fprintf(stderr,
            "[splitter] fn_75_1CA0 (the head's projectile aim) is a port stand-in (see "
            "PortCSplitterHelpers.cpp)\n");
  }
}
