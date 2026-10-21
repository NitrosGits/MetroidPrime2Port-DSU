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

