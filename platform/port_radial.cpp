#include "port_radial.h"
#include "port_radial_touch.h"

#include <algorithm>
#include <cmath>

namespace PortRadial {
namespace {
State sState;
int sForced = -1;
Point sForceVector;
int sPad = -1;
Point sPadVector;
int sApplied[2] = {-1, -1};
struct QueuedRequest { int target = -1; uint64_t epoch = 0; };
QueuedRequest sRequests[2];
uint64_t sTouchSession = 0, sTouchCancellation = 0;
bool sTouchBlockedButtons[2] = {};
bool sTouchContext = true;
TouchMailbox& Touches() { static TouchMailbox mailbox; return mailbox; }
void PublishTouchState() {
  Touches().Publish(sState.Available() && sTouchContext, sState.CancellationEpoch(),
                    sState.Capturing() && !sTouchSession, sTouchCancellation);
}
}

void State::Cancel() {
  ++mCancellationEpoch; // survives mailbox consumption and availability off/on transitions
  mActive = mPick = -1;
  mReleased = mResolved = mConsumed = false;
  mVector = {};
  mStick = false;
  mOpen[0] = mOpen[1] = 0.f;
  if (this == &sState) {
    sTouchSession = 0;
    PublishTouchState();
  }
  // Keep previous holds: cancellation cannot reopen the same held button on resume.
}

void State::SetAvailable(bool available) {
  mAvailable = available;
  if (!available) Cancel();
  else if (this == &sState) PublishTouchState();
}

void State::Tick(bool beam, bool visor, float x, float y) {
  const bool held[2] = {beam, visor};
  // Drain a still-deflected selection stick before returning it to aim/GC beam switching.
  if (mConsumed && x * x + y * y < 0.01f) {
    mActive = mPick = -1;
    mReleased = mResolved = mConsumed = false;
  }
  if (mAvailable && mActive < 0) {
    // Beam wins simultaneous rising edges; an existing wheel always wins over the other.
    for (int i = 0; i < 2; ++i) {
      if (held[i] && !mPrevious[i]) {
        mActive = i;
        mOpen[1 - i] = 0.f; // a new wheel cannot overlap the other's closing presentation
        mVector = {};
        mStick = false;
        break;
      }
    }
  }
  if (mActive >= 0 && !mReleased) {
    // A centered stick explicitly returns to the center, unlike relative mouse travel.
    // Only use a nonzero stick here: mouse selection must survive an idle pad.
    if (x * x + y * y > 0.01f || mStick) {
      SetSelection(x, y);
      mStick = x * x + y * y > 0.01f;
    }
    if (!held[mActive]) mReleased = true;
  }
  mPrevious[0] = beam;
  mPrevious[1] = visor;
}

void State::SetSelection(float x, float y) {
  if (!Capturing() || mResolved || !std::isfinite(x) || !std::isfinite(y)) return;
  const float len = std::hypot(x, y);
  if (len > 1.f) { x /= len; y /= len; }
  mVector = {x, y};
}

void State::AddTravel(float dx, float dy) {
  if (dx == 0.f && dy == 0.f) return;
  mStick = false;
  SetSelection(mVector.x + dx / 200.f, mVector.y - dy / 200.f);
}

int State::Selection(const Point* positions) const {
  if (!Capturing()) return -1;
  if (std::hypot(mVector.x, mVector.y) < 0.35f) return 0;
  const float angle = std::atan2(mVector.y, mVector.x);
  int best = -1;
  float distance = 10.f;
  for (int i = 1; i < 4; ++i) {
    const float dx = positions[i].x - positions[0].x;
    const float dy = positions[i].y - positions[0].y;
    if (std::hypot(dx, dy) < 0.001f) continue;
    const float diff = std::abs(std::remainder(std::atan2(dy, dx) - angle, 6.283185307f));
    if (diff < distance) { distance = diff; best = i; }
  }
  return best;
}

void State::Resolve(const Point* positions, unsigned owned, int current) {
  if (!mReleased || mResolved) return;
  const int slot = Selection(positions);
  mPick = slot >= 0 && slot != current && (owned & (1u << slot)) ? slot : -1;
  mResolved = true;
}

int State::TakePick(Kind kind) {
  if (mActive != kind || !mResolved || mConsumed) return -1;
  mConsumed = true;
  const int result = mPick;
  mPick = -1;
  // Capture remains true throughout this input/aim tick; next Tick acknowledges consumption.
  return result;
}

void State::Animate(float dt) {
  if (!std::isfinite(dt) || dt < 0.f) return;
  for (int i = 0; i < 2; ++i) {
    const float goal = Held(static_cast<Kind>(i)) ? 1.f : 0.f;
    mOpen[i] += std::clamp(goal - mOpen[i], -10.f * dt, 10.f * dt);
  }
}

State& Transport() { return sState; }
void SetApplied(int beam, int visor) { sApplied[kBeam] = beam; sApplied[kVisor] = visor; }
int Applied(Kind kind) { return sApplied[kind]; }
void Retry::Start(int pick, bool fromRadial, uint64_t nowNs) {
  target = pick;
  radial = fromRadial;
  epoch = sState.CancellationEpoch();
  untilNs = nowNs + 1'500'000'000ull;
}
int Retry::PendingTarget() const {
  return radial && epoch != sState.CancellationEpoch() ? -1 : target;
}
int DispatchRetry(Retry& retry, Kind kind, uint64_t nowNs, bool radialEligible, int applied) {
  if (retry.target < 0) return -1;
  if ((retry.radial && (!radialEligible || retry.PendingTarget() < 0)) ||
      nowNs > retry.untilNs || applied == retry.target) {
    retry.target = -1;
    return -1;
  }
  if (!retry.radial) return retry.target;
  sRequests[kind] = {retry.target, retry.epoch};
  return -1;
}
int TakeRequest(Kind kind, int ordinary) {
  ServiceTouchCancellation();
  const QueuedRequest request = sRequests[kind];
  sRequests[kind].target = -1;
  if (ordinary >= 0) return ordinary;
  return sState.Available() && request.epoch == sState.CancellationEpoch() ? request.target : -1;
}
void SimulatePad(int kind, float x, float y) {
  if (kind == 2) { sPad = 2; return; } // release click, retain the current stick vector
  sPad = kind >= 0 && kind < 2 ? kind : -1;
  if (std::isfinite(x) && std::isfinite(y))
    sPadVector = {std::clamp(x, -1.f, 1.f), std::clamp(y, -1.f, 1.f)};
}
int SimulatedPad(Point& vector) { vector = sPadVector; return sPad; }
bool Capturing() { return sState.Capturing(); }
void Cancel() { sState.Cancel(); sForced = -1; }
int64_t BeginTouch(Kind kind, int64_t owner) { return Touches().Begin(kind, owner); }
bool MoveTouch(uint64_t token, int64_t owner, float x, float y) {
  return Touches().Move(token, owner, x, y);
}
bool EndTouch(uint64_t token, int64_t owner, float x, float y) {
  return Touches().End(token, owner, x, y);
}
void CancelTouch(uint64_t token, int64_t owner) { Touches().Cancel(token, owner); }
void CancelAllTouches() { Touches().CancelAll(); }
void ServiceTouchCancellation() {
  const auto sample = Touches().Read();
  if (sample.cancellation != sTouchCancellation) {
    sTouchCancellation = sample.cancellation;
    Cancel();
  }
}
bool TouchOwnsSelection() { return sTouchSession && sState.Capturing(); }
void SetTouchContext(bool allowed) {
  if (sTouchContext && !allowed) CancelAllTouches();
  sTouchContext = allowed;
  PublishTouchState();
}
void Force(int kind, float x, float y) {
  sForced = kind >= 0 && kind < 2 ? kind : -1;
  sForceVector = {x, y};
}
void Poll(bool beam, bool visor, float x, float y) {
  ServiceTouchCancellation();
  const bool physical[2] = {beam, visor};
  for (int i = 0; i < 2; ++i) if (!physical[i]) sTouchBlockedButtons[i] = false;
  const auto touch = Touches().Read();
  if (touch.valid && touch.epoch == sState.CancellationEpoch() && sState.Available()) {
    if (touch.token != sTouchSession && sState.Capturing()) {
      Touches().Reject(touch.token); // an already captured keyboard/pad wheel wins
    } else {
      sForced = -1; // a competing console hold must not open late after the owner lifts
      for (int i = 0; i < 2; ++i) if (physical[i]) sTouchBlockedButtons[i] = true;
      if (touch.token != sTouchSession) {
        sState.Tick(false, false, 0.f, 0.f);
        sTouchSession = touch.token;
        // Preserve an entire down/move/up between two polls, including its final vector.
        sState.Tick(touch.kind == kBeam, touch.kind == kVisor, 0.f, 0.f);
      }
      sState.SetSelection(touch.x, touch.y);
      sState.Tick(touch.held && touch.kind == kBeam, touch.held && touch.kind == kVisor, 0.f, 0.f);
      if (!touch.held && !sState.Capturing()) {
        Touches().Acknowledge(touch.token);
        sTouchSession = 0;
      }
      PublishTouchState();
      return;
    }
  }
  if (!touch.valid) {
    sTouchSession = 0;
    // A release may have arrived before lifecycle invalidation. There is no UI target left
    // to send another lift, so retire that revoked released session here (held ones stay blocked).
    if (touch.token && !touch.held) Touches().Acknowledge(touch.token);
  }
  beam = beam && !sTouchBlockedButtons[kBeam];
  visor = visor && !sTouchBlockedButtons[kVisor];
  sState.Tick(beam || sForced == kBeam, visor || sForced == kVisor, x, y);
  if (sForced >= 0) sState.SetSelection(sForceVector.x, sForceVector.y);
  PublishTouchState();
}
}
