// Port of the Dark Samus REL (module 10) helpers: the stateless functions of the module that are
// not FSM handlers (the ones below 0xA000). The function map, the caller and the arm tables are
// in docs/module-analysis/DarkSamus_helpers.md; the bodies marked `// @ fn_10_` were run in the
// Sanctuary 10_cliff_rooftop fight (boss u04f7).
#include "MetroidPrime/Enemies/CDarkSamus.hpp"

// fn_10_9624 (0x00009624, 0x20 bytes): is the boost ball's uid (guest 0xE38) a real one? The
// compare is the "subf/or/srwi" idiom for `uid != kInvalidUniqueId` (0x00009630..0x00009640).
// fn_10_9624 (ported, unreferenced in the port: nothing calls it yet)
bool CDarkSamus::HasBoostBall() const {
  return mBoostBallUid != kInvalidUniqueId;
}
