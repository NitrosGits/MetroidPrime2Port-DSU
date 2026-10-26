#include "port_radial.h"
#include "port_radial_touch.h"
#include "port_test.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <thread>

using namespace PortRadial;
const Point beam[] = {{0,.6f}, {-3.961959f,-2.037475f}, {3.961959f,-2.037475f}, {0,5.088521f}};
const Point visor[] = {{0,.6f}, {3.961959f,-2.037475f}, {0,5.088521f}, {-3.961959f,-2.037475f}};

void Reset() {
  CancelAllTouches();
  ServiceTouchCancellation();
  SetTouchContext(true);
  Transport().SetAvailable(true);
  Poll(false, false, 0, 0);
}
void Finish(Kind kind, const Point* geometry, int expected, unsigned owned = 15) {
  Poll(false, false, 0, 0);
  REQUIRE(TouchOwnsSelection());
  Transport().Resolve(geometry, owned, (expected + 1) % 4);
  Poll(false, false, 0, 0);
  REQUIRE(Transport().TakePick(kind) == ((owned & (1u << expected)) ? expected : -1));
  REQUIRE(Transport().TakePick(kind) == -1);
  Poll(false, false, 0, 0);
  REQUIRE(!Capturing());
}
int main() {
  for (int k = 0; k < 2; ++k) for (int i = 0; i < 4; ++i) {
    Reset();
    const auto kind = Kind(k);
    const auto* geometry = k ? visor : beam;
    const int64_t token = BeginTouch(kind, 42);
    REQUIRE(token > 0);
    // Entire gesture before a game poll, including the UP's final position.
    REQUIRE(EndTouch(token, 42, geometry[i].x - geometry[0].x, geometry[i].y - geometry[0].y));
    REQUIRE(!EndTouch(token, 42, 0, 0));
    Finish(kind, geometry, i);
  }
  Reset();
  auto noop = BeginTouch(kBeam, 3);
  REQUIRE(noop > 0 && EndTouch(noop, 3, 0, 0));
  Poll(false, false, 0, 0);
  Transport().Resolve(beam, 15, 0); // current center item: no request, still consumes release
  REQUIRE(Transport().TakePick(kBeam) == -1);
  Poll(false, false, 0, 0);
  REQUIRE(!Capturing());
  Reset();
  auto paused = BeginTouch(kVisor, 3);
  Poll(false, false, 0, 0);
  REQUIRE(TouchOwnsSelection());
  SetTouchContext(false); // pause/F1/background/touch-controls boundary
  REQUIRE(TakeRequest(kVisor) == -1 && !Capturing());
  SetTouchContext(true);
  REQUIRE(!EndTouch(paused, 3, 0, 1));
  REQUIRE(BeginTouch(kVisor, 3) > 0);
  Reset();
  auto token = BeginTouch(kBeam, 7);
  REQUIRE(token > 0);
  REQUIRE(BeginTouch(kVisor, 8) == -1);
  REQUIRE(!MoveTouch(token, 8, 1, 0)); // movement/other finger cannot point or lift owner
  REQUIRE(!EndTouch(token, 8, 1, 0));
  CancelTouch(token, 8);
  Poll(false, false, 0, 0);
  REQUIRE(TouchOwnsSelection() && Transport().Vector().x == 0);
  Transport().SetAvailable(false);
  Transport().SetAvailable(true);
  REQUIRE(!MoveTouch(token, 7, 0, 1));
  REQUIRE(BeginTouch(kBeam, 7) == -1); // still-held revoked finger is not a new gesture
  REQUIRE(!EndTouch(token, 7, 0, 1));
  Poll(false, false, 0, 0);
  REQUIRE(!Capturing());
  const auto old = token;
  token = BeginTouch(kBeam, 7); // pointer id reused, token must not be reused
  REQUIRE(token > 0 && token != old);
  REQUIRE(!MoveTouch(old, 7, 0, 1) && !EndTouch(old, 7, 0, 1));
  REQUIRE(EndTouch(token, 7, 0, 1));
  Finish(kBeam, beam, 3, 1); // locked outer item never redirects/commits

  Reset();
  token = BeginTouch(kBeam, 7);
  EndTouch(token, 7, 0, 1);
  Poll(false, false, 0, 0);
  Transport().SetAvailable(false); // release BEFORE loss: UI already removed its target
  Transport().SetAvailable(true);
  Poll(false, false, 0, 0);
  REQUIRE(!Capturing() && Transport().TakePick(kBeam) == -1);
  REQUIRE(BeginTouch(kBeam, 7) > 0); // revoked released owner must not leave the mailbox busy

  // UI cancellation after release/consumption invalidates both the retry and its queued request
  // even if the input poll has not run again. TakeRequest is the real gameplay handoff boundary.
  Reset();
  token = BeginTouch(kBeam, 9);
  EndTouch(token, 9, -1, -1);
  Poll(false, false, 0, 0);
  Transport().Resolve(beam, 15, 0);
  const int pick = Transport().TakePick(kBeam);
  REQUIRE(pick == 1);
  Retry pending;
  pending.Start(pick, true, 100);
  DispatchRetry(pending, kBeam, 101, true, 0);
  CancelAllTouches(); // ACTION_CANCEL, layout, background, focus loss
  REQUIRE(TakeRequest(kBeam) == -1);
  REQUIRE(pending.PendingTarget() == -1);
  REQUIRE(!Capturing());
  REQUIRE(BeginTouch(kBeam, 9) > 0); // a fresh down may rearm after game acknowledgement
  Reset();
  token = BeginTouch(kBeam, 4);
  Force(kVisor, 1, 0);
  Poll(true, true, 0, 1); // independent keyboard/stick cannot take ownership mid-touch
  REQUIRE(TouchOwnsSelection() && Transport().Vector().y == 0);
  EndTouch(token, 4, 0, 1);
  Poll(true, true, 0, 1);
  Transport().Resolve(beam, 15, 0);
  Transport().TakePick(kBeam);
  Poll(true, true, 0, 1);
  Poll(true, true, 0, 1);
  REQUIRE(!Capturing()); // held keys under a touch cannot reopen it after release
  Poll(false, false, 0, 0);
  Poll(true, false, 0, 0);
  REQUIRE(Capturing() && BeginTouch(kVisor, 99) == -1); // desktop owner wins
  Reset();
  SetTouchContext(false);
  REQUIRE(BeginTouch(kBeam, 99) == 0); // unavailable => legacy fallback only on fresh down
  SetTouchContext(true);
  ServiceTouchCancellation();

  // Threaded producer/consumer test: one coherent point and owner token under contention.
  TouchMailbox mailbox;
  mailbox.Publish(true, 3, false, 0);
  const auto session = mailbox.Begin(kBeam, 123);
  std::atomic<bool> done{false};
  std::thread producer([&] {
    for (int i = 0; i < 30000; ++i) {
      mailbox.Move(session, 123, .25f, -.5f);
      mailbox.Move(session, 123, -.25f, .5f);
      REQUIRE(!mailbox.Move(session, 124, 1, 1));
    }
    mailbox.End(session, 123, .25f, -.5f);
    done.store(true);
  });
  do {
    const auto sample = mailbox.Read();
    REQUIRE(sample.token == uint64_t(session) && sample.owner == 123 && sample.epoch == 3);
    REQUIRE(sample.y == -2 * sample.x); // no torn X/Y pairs
  } while (!done.load());
  producer.join();
  REQUIRE(!mailbox.Read().held && mailbox.Read().valid);
  mailbox.CancelAll();
  REQUIRE(mailbox.Begin(kBeam, 123) == -1); // cancel must be acknowledged on game thread
  mailbox.Publish(true, 4, false, mailbox.Read().cancellation);
  REQUIRE(mailbox.Begin(kBeam, 123) > 0);
  mailbox.CancelAll();
  std::thread churn([&] {
    for (int i = 0; i < 3000; ++i) {
      const auto t = mailbox.Begin(kVisor, 55);
      if (t > 0) {
        mailbox.Move(t, 55, .25f, -.5f);
        mailbox.End(t, 55, .25f, -.5f);
      }
    }
  });
  std::thread cancellation([&] {
    for (int i = 0; i < 3000; ++i) mailbox.CancelAll();
  });
  for (uint64_t epoch = 5; epoch < 3005; ++epoch) {
    const auto sample = mailbox.Read();
    REQUIRE(std::isfinite(sample.x) && std::isfinite(sample.y));
    REQUIRE(sample.x * sample.x + sample.y * sample.y <= 1.001f);
    mailbox.Publish(epoch % 2 != 0, epoch, false, sample.cancellation);
    if (!sample.held) mailbox.Acknowledge(sample.token);
  }
  churn.join();
  cancellation.join();
  mailbox.CancelAll();
  mailbox.Publish(true, 4000, false, mailbox.Read().cancellation);
  REQUIRE(mailbox.Begin(kBeam, 55) > 0);
  std::puts("port_radial_touch: slots/ownership/short release/epochs/rearm/threaded mailbox PASS");
}
