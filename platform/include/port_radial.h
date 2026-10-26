#pragma once

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
private:
  bool mAvailable = false, mPrevious[2] = {}, mReleased = false;
  bool mResolved = false, mConsumed = false;
  bool mStick = false;
  int mActive = -1, mPick = -1;
  Point mVector;
  float mOpen[2] = {};
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

}

class CStateManager;
namespace PortRadial {
void UpdateHud(float dt, const CStateManager& mgr, int player);
void DrawHud(int player);
void DestroyHud(int player);
bool GameplayAllowed(const CStateManager& mgr);
}
