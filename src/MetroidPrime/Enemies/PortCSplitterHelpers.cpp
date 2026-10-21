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
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
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

// The head's DoUserAnimEvent (vtable slot 0x70).  The note's section f row read
// "0x70 | GetSortingBounds | fn_75_6394", but the REL's own vtable (lbl_75_data_448) has the
// DOL's `GetSortingBounds__6CActorCFRC13CStateManager` at slot 0x6C and the module's fn_75_6394
// at slot 0x70 - and docs/module-analysis/vtable-offsets.md names the DOL's slot 0x70
// `DoUserAnimEvent(CStateManager&, const CInt32POINode&, EUserEventType, float)`, which is
// exactly this body's signature (r4 the manager, r5 the node, r6 the type, f1 the dt, asm
// 0x63B0-0x63CC).
//
// The body switches on the type:
//   asm 0x63B4 `cmpwi r31, 0x16` (the type 0x16 is the first case)
//   asm 0x63D8 `cmpwi r31, 0x0` (the type 0 is the second case)
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

// fn_75_1CA0 (not marked yet: the body is ported, but the coverage profile still misses the
// "the head is dead" and the "there is no player" arms - the fight never plays the head's
// projectile event, so the arm is taken by forcing the event's type to 0; see the note).
// The head's projectile aim (0x1CA0-0x1E37): the head shoots at the player when its animation
// plays a projectile event.
//
// The body:
//   asm 0x1CC8 `lbz r0, 0x420(r3)`
//   asm 0x1CCC `extrwi. r0, r0, 1, 25`
//   asm 0x1CD0 `beq .L_00001E18`
//     the alive gate: the base's flag byte at 0x420, its 0x40 bit.  A dead head shoots nothing.
//   asm 0x1CD4 `lhz r0, 0xefe(r28)`
//   asm 0x1CE4 `bl GetObjectById__13CStateManagerCF9TUniqueId`
//     the entity at 0xEFE (the head's copy of the chassis's target).  With no entity the body ends
//     (asm 0x1CE8 `mr. r31, r3`, 0x1CEC `beq .L_00001E18`).
//   asm 0x1CF8 `bl GetLctrTransform__10CPatternedCFRC...`
//   asm 0x1D08 `bl __ct__12CTransform4fFRC12CTransform4f`
//     the node's locator transform (the name is the argument) and the copy of it.
//   asm 0x1D10 `lis r3, lbl_75_rodata_40@ha`
//   asm 0x1D30 `lfs f1, 0x0(r6)`
//   asm 0x1D3C `lwz r12, 0x0(r31)`
//   asm 0x1D40 `lwz r12, 0x54(r12)`
//   asm 0x1D48 `bctrl`
//     the entity's own aim position (its vtable slot 0x54) with the rodata constant
//     lbl_75_rodata_40 (0.0f) as the dt.
//   asm 0x1D4C-0x1DBC the look-at: the "from" point is the copy's translation (the transform's
//     three floats at +0x0c, +0x1c and +0x2c) and the "to" point is that point plus the head's
//     own forward vector (the three floats at +0x28, +0x38 and +0x48) scaled by the
//     direction's projection onto it; the up vector is the world up (asm 0x1D50 `sUpVector`).
//   asm 0x1DC0-0x1DC8 the copy of the look-at that the launch uses.
//   asm 0x1DD0 `bl fn_75_1F50` the projectile info.  The host's LaunchProjectile has no
//     CProjectileInfo parameter, so the host uses the actor's own mProjectileInfo (the head's
//     laser pulse, built by the constructor).
//   asm 0x1E08 `bl LaunchProjectile__10CPatternedFRC12CTransform4fR13CStateManageriUibRC18CImpactVisorEffectRC9CVector3f`
//     the launch.  The register layout is the host's own (r4 the transform, r5 the manager, r6 the
//     max projectiles, r7 the attributes, r8 the homing, r9 the impact visor effect, r10 the
//     scale): r6 is 6 (asm 0x1DF4), r7 and r8 are 0 (asm 0x1DF8, 0x1E00), r9 is the local at
//     r1+0x3c (asm 0x1DE8) - the impact visor effect that fn_75_1F50 built (asm 0x1DD0) and
//     that fn_75_1E38 destroys after the launch (asm 0x1E14) - and r10 is the local at
//     r1+0xc (asm 0x1DF0), the scale: the three floats of the rodata constant lbl_75_rodata_34
//     (1.0f, asm 0x1DDC and the stores at 0x1DEC, 0x1DFC and 0x1E04).  The effect fn_75_1F50
//     builds is the empty one (fn_75_1F98 clears the three optionals' valid bytes, asm 0x1F68,
//     0x1F70 and 0x1F74), so the host's CImpactVisorEffect::None() is the same thing.
void CSplitterCommandModule::PortHeadProjectileAim(CStateManager& mgr,
                                                   const rstl::string& locatorName) {
  // asm 0x1CC8-0x1CD0: the alive gate.
  if (!GetAlive()) {
    return;
  }
  // asm 0x1CD4-0x1CEC: the entity at 0xEFE.
  CPlayer* entity = TCastToPtr< CPlayer >(mgr.ObjectById(mSyncedId));
  if (entity == nullptr) {
    return;
  }
  // asm 0x1CF8-0x1D08: the node's locator transform and the copy of it.
  const CTransform4f copy = GetLctrTransform(locatorName);
  // asm 0x1D10-0x1D48: the entity's own aim position.
  const CVector3f aim = entity->GetAimPosition(mgr, 0.f);
  // asm 0x1D4C-0x1DBC: the look-at.
  const CVector3f from = copy.GetTranslation();
  const CVector3f dir = aim - from;
  const CVector3f fwd = GetTransform().GetForward();
  const float s = fwd.GetX() * dir.GetX() + fwd.GetY() * dir.GetY() + fwd.GetZ() * dir.GetZ();
  const CVector3f to(from.GetX() + s * fwd.GetX(), from.GetY() + s * fwd.GetY(), aim.GetZ());
  const CTransform4f look = CTransform4f::LookAt(from, to, CVector3f::Up());
  // asm 0x1DC0-0x1E08: the launch.
  LaunchProjectile(look, mgr, 6, 0, false, CImpactVisorEffect::None(),
                   CVector3f(1.f, 1.f, 1.f));
}

// ---------------------------------------------------------------------------------------------
// The main chassis's DoUserAnimEvent (the REL's vtable slot 0x70).

// The chassis's DoUserAnimEvent (vtable slot 0x70).  The note's section f row read
// "0x70 | GetSortingBounds | fn_75_EFA4", but the REL's own vtable has the DOL's
// `GetSortingBounds__6CActorCFRC13CStateManager` at slot 0x6C and the module's fn_75_EFA4 at
// slot 0x70 - the DOL's slot 0x70 is
// `DoUserAnimEvent(CStateManager&, const CInt32POINode&, EUserEventType, float)`, which is
// this body's signature (r4 the manager, r5 the node, r6 the type, f1 the dt, asm
// 0xEFBC-0xEFD4).
void CSplitterMainChassis::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                           EUserEventType type, float dt) {
  PortDoUserAnimEvent(mgr, node, type, dt);
}

// The chassis's DoUserAnimEvent (vtable slot 0x70): the two type cases and the base class's
// own call after them.
//
// The body switches on the type:
//   asm 0xEFC0 `cmpwi r31, 0x16` (the type 0x16 is the first case)
//   asm 0xEFDC `cmpwi r31, 0x5` (the type 0x5 is the second case)
//   - the type 0x16 (kUE_BreakLockOn): the flag byte at 0xD41 gets its 0x10 bit tested (asm
//     0xEFE8 `lbz`, 0xEFEC `extrwi` with the word's bit 27, 0xEFF0 `beq`).  With it clear the
//     player at 0xCC0 (the chassis's target) is looked up (asm 0xEFF4 `lhz`, 0xF004
//     `ObjectById__13CStateManagerCF9TUniqueId`) and cast to a CPlayer (asm 0xF008
//     `TCastToPtr<7CPlayer>`); with no player the arm ends (asm 0xF010 `beq .L_0000F0B0`).
//     Otherwise the player is asked for two orbit requests: the chassis's own id (the u16 at
//     0x8, asm 0xF014) and the head's id (the u16 at 0xC6C, asm 0xF030), each with the
//     request 8 (kOR_ActivateOrbitSource, asm 0xF020 and 0xF044) - the retail's calls are the
//     `SetOrbitRequestForTarget__7CPlayerF9TUniqueIdQ27CPlayer19EPlayerOrbitRequestR13CStateManager`
//     (asm 0xF02C and 0xF048), which pass a CPlayer* the host's signature has no slot for.
//   - the type 0x5 (kUE_ChangeMaterial): the flag byte at 0xD41 gets its 0x80 bit tested (asm
//     0xF054 `lbz`, 0xF058 `extrwi` with the word's bit 24, 0xF05C `beq .L_0000F0B0`).  With it
//     set the base class's own damage vulnerability is reflected (asm 0xF060
//     `ReflectVulnerabilty__20CDamageVulnerabilityFv`, 0xF06C
//     `GetDamageVulnerability__10CPatternedCFv`) and the collision actors' vulnerability is set
//     (asm 0xF080 `bl fn_75_9AF8`).
//   Then, and only then, the base class's own DoUserAnimEvent runs (asm 0xF0B0).
void CSplitterMainChassis::PortDoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                               EUserEventType type, float dt) {
  bool handled = false;
  if (type == kUE_BreakLockOn) {
    // asm 0xEFE8-0xEFF0: the flag byte at 0xD41.
    if ((mFlagsD41 & 0x10) == 0) {
      // asm 0xEFF4-0xF010: the player at 0xCC0 (the chassis's target).
      CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mSyncId));
      if (player != nullptr) {
        // asm 0xF014-0xF02C: the chassis's own id, then asm 0xF030-0xF048: the head's id.
        player->SetOrbitRequestForTarget(GetUniqueId(), CPlayer::kOR_ActivateOrbitSource, mgr);
        player->SetOrbitRequestForTarget(mCommandModuleId, CPlayer::kOR_ActivateOrbitSource, mgr);
        handled = true;
      }
    }
  } else if (type == kUE_ChangeMaterial) {
    // asm 0xF054-0xF05C: the flag byte at 0xD41.
    if ((mFlagsD41 & 0x80) != 0) {
      // asm 0xF060-0xF080: the reflected vulnerability and the collision actors'.
      const CDamageVulnerability& vuln = *GetDamageVulnerability();
      PortSetCollisionVulnerability(mgr, vuln, CDamageVulnerability::ReflectVulnerabilty());
      handled = true;
    }
  }
  // asm 0xF0B0: the base class's own DoUserAnimEvent runs when a case was taken.
  if (handled) {
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);
  }
}
