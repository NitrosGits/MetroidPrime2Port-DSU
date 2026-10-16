#ifndef _PORT_HARNESS_H
#define _PORT_HARNESS_H

// The test harness's run length, and the only knob that lets a run end itself.
//
// A sweep is normally ended by `timeout`, and the timeout cannot tell a run that finished its work
// from one that hung: both are exit 124. `MP2_FRAMES=<n>` ends the run cleanly once n *game*
// updates have happened - `CStateManager::Update` calls, i.e. frames of a running game, not the
// boot, pak-loading or menu frames - and the exit is the game loop's own quit path (the one the
// window's close button takes): the loop ends, Aurora shuts down and the process exits 0. A run
// that ends this way says so on stderr, so the last lines of a log are the run's verdict.
//
// It is a *count*, not a wait: an unarmed run is unaffected, and a run whose frame limit is never
// reached (the game never started, the area never loaded) still ends on the script's `timeout`,
// which stays in place as the backstop.

namespace PortHarness {

// One game update. Returns true on the update that reaches the limit, so the caller can report
// where the run got to before the loop stops. Always false when MP2_FRAMES is unset or not
// positive.
bool GameUpdate();

// True once a run has reached its limit and asked to end; the platform polls this.
bool ExitRequested();

// Wall seconds since the first call into this file (the game loop's start). A log that says how long
// a phase took is worth more than one that says how many frames it used, because the loop's rate is
// not constant: pak loading runs at tens of iterations a second and a rendered game area at a
// handful.
double Seconds();

} // namespace PortHarness

#endif
