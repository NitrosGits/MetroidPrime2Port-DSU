#ifndef _PORTSKIPFRONTEND_H
#define _PORTSKIPFRONTEND_H

// Port-only: the front-end skip for tests. `MP2_SKIP_FRONTEND=1` boots straight into the memory
// card's save instead of playing the front end's menu, which is the ~3400-iteration, ~1-minute
// phase of every run (see the Status section of AGENTS.md); `MP2_SLOT=<0|1|2>` picks the save slot.
// The implementation, and why the card has to be driven by hand, is in
// src/MetroidPrime/PortSkipFrontEnd.cpp.
//
// Both entry points are called from upstream files under `#ifdef TARGET_PC`, and both are no-ops
// when the flag is unset, so a normal launch is unaffected.

namespace PortSkipFrontEnd {

// Dismisses the boot's splash chain the way a button press does. Called once per game-loop
// iteration from `CMain::RsMain`, because the splash blocks the flow that `Pump` runs in: while a
// splash window is up, the pre-front-end window gets no ticks at all. No-op when the skip is unset.
void DismissSplash();

// One pre-front-end tick (`CPreFrontEnd::OnMessage`). Pumps the card read for the requested save
// slot and returns false while it is still coming in, which holds the flow in the pre-front-end
// window; true means the flow may leave it (the slot is ready, the flag is unset, or the skip gave
// up and the front end should play as usual).
bool Pump();

// Called where the flow would hand over to the front end (`CMainFlow::SetGameState`,
// `kCFS_FrontEnd`). True means the save the card was read into is now the running state and the
// caller must do nothing else: no front-end window, no `StartGameFromFrontEnd`.
bool StartGame();

// True once StartGame has made the card's save the running state (the save states' quick resume
// only replaces a save the skip loaded).
bool Started();

} // namespace PortSkipFrontEnd

#endif
