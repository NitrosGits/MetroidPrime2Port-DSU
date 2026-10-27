#pragma once
#include <cstdint>

// Game-thread owned selector state; only the explicit UI mailbox producers below are any-thread.
// Normalized X right / Y up is shared by desktop input and eligible touch menu gestures.
namespace PortRadial {
enum Kind { kBeam = 0, kVisor = 1 };
struct Point { float x = 0.f, y = 0.f; };

class State {
public:
  void SetAvailable(bool available);
  void Cancel();
  void Tick(bool beam, bool visor, float x, float y);
  void SetSelection(float x, float y);
  void AddTravel(float dx, float dy);
  // Geometry is the authored icon positions in enum order, not donor item semantics.
  int Selection(const Point* positions) const;
  void Resolve(const Point* positions, unsigned owned, int current);
  int TakePick(Kind kind);
  void Animate(float dt);
  bool Capturing() const { return mActive >= 0; }
  bool Available() const { return mAvailable; }
  bool Held(Kind kind) const { return mActive == kind && !mReleased; }
  int Active() const { return mActive; }
  Point Vector() const { return mVector; }
  float Open(Kind kind) const { return mOpen[kind]; }
  uint64_t CancellationEpoch() const { return mCancellationEpoch; }
private:
  friend void ServiceTouchCancellation();
  void ClearCapture(); // source-scoped reset; global Cancel also invalidates the global epoch
  bool mAvailable = false, mPrevious[2] = {}, mReleased = false;
  bool mResolved = false, mConsumed = false;
  bool mStick = false;
  int mActive = -1, mPick = -1;
  Point mVector;
  float mOpen[2] = {};
  uint64_t mCancellationEpoch = 0;
};

// Trilogy hold-to-open gate for the keyboard/pad buttons: a wheel latches only after the button
// has been held kLatchSeconds and while Eligible() (first-person, unmorphed, gun idle; the beam
// wheel not in Scan). Touch sessions and the console's Force are not gated.
constexpr double kLatchSeconds = 0.175;
struct Latch {
  double heldSince[2] = {-1., -1.}; // kBeam, kVisor
  int active = -1;
  // Returns true when a latched wheel was dropped for losing eligibility (caller cancels).
  bool Tick(bool beam, bool visor, double nowSec, const bool eligible[2]);
  bool Latched(Kind kind) const { return active == kind; }
};
void SetEligible(bool beam, bool visor); // from the HUD update; false-initialized
bool Eligible(Kind kind);
// Game thread: raw buttons -> latched buttons for Poll (cancels on lost eligibility).
void GateButtons(bool& beam, bool& visor, double nowSec);

State& Transport();
bool Capturing();
void Cancel();
void Force(int kind, float x = 0.f, float y = 0.f);
void Poll(bool beam, bool visor, float x, float y);
// Deterministic console-only simulated raw pad: right/left stick click holds beam/visor;
// kind 2 releases the click retaining its vector, -1 disconnects the test override.
// Goes through controls.toml and input-engine synthesis; no bindings are installed by this API.
void SimulatePad(int kind, float x = 0.f, float y = 0.f);
int SimulatedPad(Point& vector);
void SetApplied(int beam, int visor);
int Applied(Kind kind);

// Shared input-retry / gameplay-request boundary. Radial requests remain epoch-tagged until
// PortTouch's ApplyRequests consumes them; ordinary touch/direct/cycle requests stay independent.
struct Retry {
  int target = -1;
  uint64_t untilNs = 0;
  bool radial = false;
  uint64_t epoch = 0;
  bool touch = false;
  uint64_t touchEpoch = 0;
  void Start(int pick, bool fromRadial, uint64_t nowNs);
  int PendingTarget() const;
};
// Queues radial requests; returns an ordinary request for the existing PortTouch mailbox, or -1.
int DispatchRetry(Retry& retry, Kind kind, uint64_t nowNs, bool radialEligible, int applied);
// Consumes the queued radial request once, only in its original uncancelled epoch. An ordinary
// request wins if both sources wrote this frame; cancellation never withdraws that other source.
int TakeRequest(Kind kind, int ordinary = -1);

// Any-thread UI entry points. Return 0 to use the legacy wheel, -1 for a busy native menu,
// or a positive session token. Coordinates are drag-from-down / 72 dp, X right / Y up.
int64_t BeginTouch(Kind kind, int64_t owner);
bool MoveTouch(uint64_t token, int64_t owner, float x, float y);
bool EndTouch(uint64_t token, int64_t owner, float x, float y);
void CancelTouch(uint64_t token, int64_t owner);
void CancelAllTouches(); // View teardown / auto-hide: touch-owned capture and retries ONLY
void CancelTouchLifecycle(); // any thread: real focus/background interruption cancels all sources
// Game thread only: process UI cancellation before the gameplay request-consumption boundary.
void ServiceTouchCancellation();
bool TouchOwnsSelection();
void SetTouchContext(bool allowed); // game thread: touch controls / pause / overlay boundary

}

class CStateManager;
namespace PortRadial {
void UpdateHud(float dt, const CStateManager& mgr, int player);
void DrawHud(int player);
void DestroyHud(int player);
bool GameplayAllowed(const CStateManager& mgr);
}
