// The SpacePirate REL's (module 72) stateless helper half: the bodies that are neither an FSM
// state nor an FSM trigger, ported from the REL's disassembly
// (build/G2ME01/SpacePirate/asm/auto_00_00000000_text.s). The module note with the H/T/B split
// and every offset is docs/module-analysis/SpacePirate_helpers.md; the states and the triggers
// are in PortCSpacePirateStates.cpp and PortCSpacePirateTriggers.cpp, the module's own file is
// PortCSpacePirate.cpp.
//
// This file holds the bodies the module session does not have yet. Every body below carries the
// retail offset it came from. A `// @ fn_72_<off>` marker means the body is retail's and every
// live arm of it ran (the arm table is in the comment above it).
#include "MetroidPrime/Enemies/CSpacePirate.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "MetroidPrime/CRagDoll.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CValidEntityPredicate.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"

// Retail fn_72_2EA8 (0xA4): the Think helper that keeps the pirate's sfx emitter (the handle at
// 0xc00) where the pirate is. The handle is 0 (asm 0x2EBC) -> nothing happens; a handle that is
// playing (asm 0x2ED4) is updated; a handle that is only queued (asm 0x2EEC) is updated as well,
// but one that is neither playing nor queued clears the handle (asm 0x2F30-0x2F34). The emitter is
// moved to the actor's position (0x54, asm 0x2F04) and the world's Y axis of the transform - the
// transform's second column (0x28, 0x38 and 0x48 are the y of its three rows, asm 0x2EF8-0x2F1C)
// - with the volume 0x7f (asm 0x2F10).
// Arms (the 2026-10-21 runs, tools/cov with tools/cov/specs/spacepirate_helpers.py and the gdb
// forcing run): the handle is 0 (asm 0x2EC0-0x2EC4) 11468 entries; the handle plays (asm 0x2ED4)
// 45 entries, the emitter updated; the handle is queued but silent (asm 0x2EEC) 3 entries, the
// emitter updated; the handle is neither (asm 0x2EF4 -> 0x2F30) 1 entry, the handle cleared -
// forced (gdb: a bogus handle 0x3039 written into 0xc00 of a live pirate). The playing / queued
// entries need the handle the retail's KnockBack stores at 0xc00 (asm 0xC944 / 0xC9B8) - the port
// drops that handle, so they are forced (gdb: CSfxManager::AddEmitter of the pirate's hurled
// sound, its handle written into 0xc00).
// @ fn_72_2EA8
void CSpacePirate::UpdateSfxEmitter() {
  const CSfxHandle handle = xc00_;
  if (handle == CSfxHandle::NullHandle()) {
    return; // 0x2EC0-0x2EC4
  }
  if (!CSfxManager::IsPlaying(handle) && !CSfxManager::IsQueued(handle)) {
    xc00_ = CSfxHandle::NullHandle(); // 0x2F30-0x2F34
    return;
  }
  // 0x2EF8-0x2F28: UpdateEmitter(handle, position, the transform's Y axis, 0x7f).
  CSfxManager::UpdateEmitter(handle, GetTranslation(), GetTransform().GetColumn(kDY), 0x7f);
}

// Retail fn_72_1854 (0x68): the actor's own bounding box in the optional that GetTouchBounds
// returns - the box is copied out of CPhysicsActor::GetBoundingBox (asm 0x1868) and the optional's
// valid flag is set (asm 0x1870-0x1874). Branchless, so the run's calls prove all of it.
// @ fn_72_1854
rstl::optional_object< CAABox > CSpacePirate::GetActorTouchBounds() const {
  return GetBoundingBox();
}

// Retail fn_72_17A0 (0x28): the ragdoll (0xb30) is returned only while it is live: the ragdoll's
// flag byte 0x70 has to carry 0x40 (asm 0x17B0-0x17B4) and 0x80 (asm 0x17B8-0x17BC), else null
// (asm 0x17C0).  The DOL's CRagDoll keeps them as mPrimed (0x40, Prime's rlwimi at 0x80160610) and
// mRenderBoundsValid (0x80, UpdateRenderBounds at 0x8015E374); the port's
// 0xb30 is never set (the ragdoll is not in this port), so the body is the null arm in every run.
CRagDoll* CSpacePirate::GetRagDoll() const {
  CRagDoll* ragdoll = static_cast< CRagDoll* >(xb30_);
  if (ragdoll == nullptr) {
    return nullptr; // 0x17A4-0x17A8
  }
  if (!ragdoll->IsPrimed()) {
    return nullptr; // 0x17AC-0x17B4 (the flag byte 0x70's 0x40)
  }
  if (!ragdoll->IsRenderBoundsValid()) {
    return nullptr; // 0x17B8-0x17BC (the flag byte 0x70's 0x80)
  }
  return ragdoll;
}
