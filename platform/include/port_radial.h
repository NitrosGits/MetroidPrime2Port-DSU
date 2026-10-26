#pragma once
#include <cstdint>

// Game-thread owned desktop transport. Normalized X right / Y up is also the future touch seam;
// this slice does not route either existing touch wheel through it.
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
  bool mAvailable = false, mPrevious[2] = {}, mReleased = false;
  bool mResolved = false, mConsumed = false;
  bool mStick = false;
  int mActive = -1, mPick = -1;
  Point mVector;
  float mOpen[2] = {};
  uint64_t mCancellationEpoch = 0;
};

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
  void Start(int pick, bool fromRadial, uint64_t nowNs);
  int PendingTarget() const;
};
// Queues radial requests; returns an ordinary request for the existing PortTouch mailbox, or -1.
int DispatchRetry(Retry& retry, Kind kind, uint64_t nowNs, bool radialEligible, int applied);
// Consumes the queued radial request once, only in its original uncancelled epoch. An ordinary
// request wins if both sources wrote this frame; cancellation never withdraws that other source.
int TakeRequest(Kind kind, int ordinary = -1);

}

class CStateManager;
namespace PortRadial {
void UpdateHud(float dt, const CStateManager& mgr, int player);
void DrawHud(int player);
void DestroyHud(int player);
bool GameplayAllowed(const CStateManager& mgr);
}
