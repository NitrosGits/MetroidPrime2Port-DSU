#include "port_radial.h"
#include "port_test.h"
#include <cstdio>
#include <initializer_list>

using namespace PortRadial;

// Exercise the SAME retry dispatcher called by PushCyclePick and the SAME epoch-checked
// consumption boundary used by PortTouch::ApplyRequests, not just the transport TakePick.
int main() {
  auto& state = Transport();
  constexpr uint64_t start = 1'000'000;
  for (Kind kind : {kBeam, kVisor}) {
    for (bool unavailable : {false, true}) {
      state.SetAvailable(true);
      Retry retry;
      retry.Start(2, true, start);
      // An older in-flight switch has not accepted this release target yet.
      REQUIRE(DispatchRetry(retry, kind, start + 1, true, 0) == -1);
      REQUIRE(retry.target == 2);
      // A tagged gameplay request is already queued, with no opportunity to retry first.
      if (unavailable) {
        state.SetAvailable(false);
        state.SetAvailable(true); // disable/re-enable BETWEEN polls must still invalidate it
      } else {
        Cancel(); // console/F1/flow cancellation, even after transport consumption
      }
      REQUIRE(TakeRequest(kind) == -1); // withdraw queued-but-not-applied request
      REQUIRE(retry.PendingTarget() == -1); // cannot become the base for an ordinary cycle pick
      REQUIRE(DispatchRetry(retry, kind, start + 2, true, 0) == -1);
      REQUIRE(retry.target == -1);
      REQUIRE(TakeRequest(kind) == -1);
    }

    state.SetAvailable(true);
    Retry retry;
    retry.Start(2, true, start);
    // Model an older animation applying beam/visor 1. Requests are read while that animation
    // ignores the new pick. Cancellation must stop ALL later retries when it finishes.
    for (uint64_t tick = 1; tick <= 3; ++tick) {
      REQUIRE(DispatchRetry(retry, kind, start + tick, true, 0) == -1);
      REQUIRE(TakeRequest(kind) == 2);
    }
    Cancel();
    int applied = 1;
    for (uint64_t tick = 4; tick <= 20; ++tick) {
      REQUIRE(DispatchRetry(retry, kind, start + tick, true, applied) == -1);
      const int next = TakeRequest(kind);
      if (next >= 0) applied = next;
    }
    REQUIRE(applied == 1); // no later switch to the cancelled target 2

    // Cancellation cannot roll back an already-consumed/applied choice.
    retry.Start(3, true, start);
    REQUIRE(DispatchRetry(retry, kind, start + 1, true, applied) == -1);
    applied = TakeRequest(kind);
    REQUIRE(applied == 3);
    Cancel();
    REQUIRE(DispatchRetry(retry, kind, start + 2, true, applied) == -1);
    REQUIRE(TakeRequest(kind) == -1 && applied == 3);

    // Non-radial pending cycle picks, and an ordinary queued PortTouch request, survive.
    Retry ordinary;
    ordinary.Start(1, false, start);
    retry.Start(2, true, start);
    REQUIRE(DispatchRetry(retry, kind, start + 1, true, 0) == -1);
    state.SetAvailable(false);
    state.SetAvailable(true);
    REQUIRE(DispatchRetry(ordinary, kind, start + 2, false, 0) == 1);
    REQUIRE(ordinary.PendingTarget() == 1);
    REQUIRE(TakeRequest(kind, 1) == 1); // actual PortTouch ordinary-mailbox input takes precedence
    REQUIRE(TakeRequest(kind) == -1);
    REQUIRE(DispatchRetry(ordinary, kind, start + 3, false, 0) == 1);
    REQUIRE(DispatchRetry(ordinary, kind, start + 4, false, 1) == -1);

    // A fresh valid release still works, exactly once at the ApplyRequests boundary.
    retry.Start(2, true, start);
    REQUIRE(DispatchRetry(retry, kind, start + 1, true, 0) == -1);
    REQUIRE(TakeRequest(kind) == 2);
    REQUIRE(TakeRequest(kind) == -1);
    REQUIRE(DispatchRetry(retry, kind, start + 2, true, 2) == -1);
    REQUIRE(retry.target == -1);
  }
  std::puts("port_radial_retry: cancel/availability epochs, queued withdrawal, in-flight retries, ordinary/applied preservation OK");
}
