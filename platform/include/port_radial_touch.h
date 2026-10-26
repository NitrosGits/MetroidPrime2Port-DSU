#pragma once

#include <cmath>
#include <cstdint>
#include <mutex>

namespace PortRadial {
// A coherent, bounded mailbox. UI producers never read/write State, resource tokens, or retry
// queues. Identity is (session token, pointer id), so a recycled Android pointer cannot release
// an older/newer gesture. A released sample survives until the game consumes it.
class TouchMailbox {
public:
  struct Sample {
    uint64_t token = 0, epoch = 0, cancellation = 0, lifecycleCancellation = 0;
    int64_t owner = 0;
    int kind = 0;
    float x = 0.f, y = 0.f;
    bool held = false, valid = false;
  };
  void Publish(bool available, uint64_t epoch, bool desktopBusy, uint64_t acknowledgedCancel) {
    std::lock_guard lock(mMutex);
    if (!available || epoch != mEpoch) mSample.valid = false;
    mAvailable = available;
    mEpoch = epoch;
    mDesktopBusy = desktopBusy;
    if (acknowledgedCancel == mSample.cancellation) mAwaitCancel = false;
  }
  // 0: unavailable, retain the legacy wheel. -1: eligible but busy/cancelling, consume only
  // this menu-button gesture without opening a second wheel or falling through to look/fire.
  int64_t Begin(int kind, int64_t owner) {
    std::lock_guard lock(mMutex);
    if (!mAvailable || kind < 0 || kind > 1) return 0;
    if (mSample.token || mDesktopBusy || mAwaitCancel) return -1;
    mSample.token = ++mNextToken;
    mSample.epoch = mEpoch;
    mSample.owner = owner;
    mSample.kind = kind;
    mSample.x = mSample.y = 0.f;
    mSample.held = mSample.valid = true;
    return static_cast<int64_t>(mSample.token);
  }
  bool Move(uint64_t token, int64_t owner, float x, float y) {
    std::lock_guard lock(mMutex);
    if (!Matches(token, owner) || !mSample.valid || !mSample.held) return false;
    Point(x, y);
    return true;
  }
  bool End(uint64_t token, int64_t owner, float x, float y) {
    std::lock_guard lock(mMutex);
    if (!Matches(token, owner)) return false;
    if (!mSample.valid) { Clear(); return false; }
    if (!mSample.held) return false;
    Point(x, y);
    mSample.held = false;
    return true;
  }
  void Cancel(uint64_t token, int64_t owner) {
    std::lock_guard lock(mMutex);
    if (!Matches(token, owner)) return;
    Invalidate();
  }
  void CancelAll(bool lifecycle = false) {
    std::lock_guard lock(mMutex);
    if (lifecycle) ++mSample.lifecycleCancellation;
    Invalidate();
  }
  Sample Read() const { std::lock_guard lock(mMutex); return mSample; }
  void Acknowledge(uint64_t token) {
    std::lock_guard lock(mMutex);
    if (mSample.token == token && !mSample.held) Clear();
  }
  void Reject(uint64_t token) {
    std::lock_guard lock(mMutex);
    if (mSample.token == token) mSample.valid = false;
  }
private:
  bool Matches(uint64_t token, int64_t owner) const {
    return token && mSample.token == token && mSample.owner == owner;
  }
  void Clear() { mSample.token = 0; mSample.valid = mSample.held = false; }
  void Invalidate() { ++mSample.cancellation; mAwaitCancel = true; Clear(); }
  void Point(float x, float y) {
    if (!std::isfinite(x) || !std::isfinite(y)) return;
    const double length = std::hypot(double(x), double(y));
    if (length > 1.) { x = float(x / length); y = float(y / length); }
    mSample.x = x; mSample.y = y;
  }
  mutable std::mutex mMutex;
  Sample mSample;
  uint64_t mEpoch = 0, mNextToken = 0;
  bool mAvailable = false, mDesktopBusy = false, mAwaitCancel = false;
};
}
