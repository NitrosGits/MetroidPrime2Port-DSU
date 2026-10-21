// The Dark Samus's helpers at 0xA000 and up: the bodies this lane ported from the REL.
//
// The module's own functions live at 0x00000000..0x00022xxx; the ones at 0xA000 and up are the
// boss's helpers (the stand-ins, the path find's helpers, the hint chain's helpers and the
// render helpers). This file holds the ones that are ported here; the rest are listed in
// docs/module-analysis/DarkSamus_helpers2.md with the offsets of what is still missing.
//
// Every body carries the REL's offsets: the load or the branch it came from.
#include "MetroidPrime/Enemies/CDarkSamus.hpp"

#include "MetroidPrime/CStateManager.hpp"

// fn_10_17E48 (0x00017E48, 0x8 bytes): the number of stages the actor has had - the guest's
// 0x978 (the 'XALD' arm of fn_10_1E89C writes 1 there, 0x0001EBB8). The body is a single load:
// the lwz at 0x00017E48 reads the word at 0x978 and the blr at 0x00017E4C returns it. Its only
// caller is fn_10_FD4C (WatchStageChange), which compares the count with the one it saw last
// (the guest's 0xD54) to notice a stage change.
// The port's fn_10_FD4C (WatchStageChange) reads the member directly, so this body does not
// run yet: it is ported and unmarked until the caller uses it.
uint CDarkSamus::GetStageCount() const {
  return mStageCount;
}
