// Photo mode capture (console `photo`); see port_photo.h.
#include "port_photo.h"

#include "port_freecam.h"
#include "port_overlay.h"
#include <dolphin/gx/GXAurora.h>
#include <dolphin/vi.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace aurora {
void request_screenshot_png(const char* path) noexcept;
int screenshot_png_state(uint32_t* width, uint32_t* height) noexcept;
bool frame_buffer_size_for_scale(float scale, uint32_t* width, uint32_t* height) noexcept;
uint32_t max_texture_dimension() noexcept;
} // namespace aurora

namespace PortPhoto {
namespace {

constexpr uint64_t kMaxCapturePixels = 48'000'000;  // a 7680x4320 frame is 33 M
constexpr uint64_t kMaxCaptureSamples = 96'000'000; // pixels x MSAA samples
constexpr int kSettleFrames = 8; // frames for the EFB to resize and draw at the new size
constexpr int kHideFrames = 2;   // frames before the shot
constexpr double kSaveTimeoutSeconds = 20.0;

enum class Phase { Idle, Hiding, Settling, Saving };

int sSupersample = 1;
Phase sPhase = Phase::Idle;
int sPhaseFrames = 0;
double sPhaseAt = 0.0;
int sCaptureFactor = 1;
CaptureResult sLast;

double Now() {
  using namespace std::chrono;
  return duration<double>(steady_clock::now().time_since_epoch()).count();
}

void Say(const std::string& text) {
  std::fprintf(stderr, "[photo] %s\n", text.c_str());
}

// The render scale setting (0 = the window's own size), which the capture puts back when it is done.
float RestoreScale() { return PortOverlay::RenderScale(); }

// The scale that gives the window's own size, from the size one unit of scale gives.
float NativeEquivalentScale(uint32_t w1, uint32_t h1) {
  uint32_t uw = 0;
  uint32_t uh = 0;
  if (!aurora::frame_buffer_size_for_scale(1.f, &uw, &uh) || uh == 0) {
    return 1.f;
  }
  return static_cast<float>(h1) / static_cast<float>(uh);
}

// The smallest render scale whose frame buffer is at least `factor` x the current one in both
// directions. Exactly N x is out of reach (the buffer is rounded to whole pixels and fitted to the
// render mode's aspect), so step finely up from the nominal. Returns 0 when it can't be had.
float ScaleForFactor(int factor, float base, uint32_t w1, uint32_t h1, uint32_t maxDim) {
  const float step = base / static_cast<float>(std::max<uint32_t>(h1, 1) * 2);
  float scale = base * static_cast<float>(factor);
  for (int i = 0; i < 64; ++i, scale += step) {
    uint32_t w = 0;
    uint32_t h = 0;
    if (!aurora::frame_buffer_size_for_scale(scale, &w, &h)) {
      return 0.f;
    }
    if (w < w1 * factor || h < h1 * factor) {
      continue;
    }
    // Over the device's texture limit, or past what the GPU can hold (the EFB, its MSAA copy and
    // the screenshot readback all scale with it).
    const uint64_t pixels = static_cast<uint64_t>(w) * h;
    if (w > maxDim || h > maxDim || pixels > kMaxCapturePixels ||
        pixels * static_cast<uint64_t>(std::max(PortOverlay::Msaa(), 1)) > kMaxCaptureSamples) {
      return 0.f;
    }
    return scale;
  }
  return 0.f;
}

// Picks the scale for the pending capture, stepping the factor down (4 -> 2 -> 1) when the larger
// size won't fit, and says so.
float PickCaptureScale() {
  uint32_t w1 = 0;
  uint32_t h1 = 0;
  AuroraGetRenderSize(&w1, &h1);
  const float base = NativeEquivalentScale(w1, h1);
  const uint32_t limit = aurora::max_texture_dimension();
  const uint32_t maxDim = limit > 0 ? limit : 8192u;
  const int wanted = sCaptureFactor;
  float scale = 0.f;
  for (; sCaptureFactor > 1; sCaptureFactor /= 2) {
    scale = ScaleForFactor(sCaptureFactor, base, w1, h1, maxDim);
    if (scale > 0.f) {
      break;
    }
  }
  sLast.clamped = sCaptureFactor != wanted;
  if (sLast.clamped) {
    Say("A " + std::to_string(wanted) + "x capture is too large for this GPU, taking " +
        std::to_string(sCaptureFactor) + "x");
  }
  return sCaptureFactor > 1 ? scale : RestoreScale();
}

} // namespace

bool Active() { return PortFreeCam::Active(); }

float Fov() { return PortFreeCam::FovOverride(); }
void SetFov(float degrees) {
  if (std::isfinite(degrees)) {
    PortFreeCam::SetFovOverride(degrees > 0.f ? std::clamp(degrees, 20.f, 120.f) : 0.f);
  }
}
float Roll() { return PortFreeCam::GetPose().roll; }
void SetRoll(float degrees) {
  if (std::isfinite(degrees)) {
    PortFreeCam::Pose pose = PortFreeCam::GetPose();
    pose.roll = std::clamp(degrees, -180.f, 180.f);
    PortFreeCam::SetPose(pose);
  }
}
float Speed() { return PortFreeCam::Speed(); }
void SetSpeed(float speed) { PortFreeCam::SetSpeed(speed); }
bool ShowSamus() { return PortFreeCam::ShowPlayer(); }
void SetShowSamus(bool show) { PortFreeCam::SetShowPlayer(show); }

int Supersample() { return sSupersample; }
void SetSupersample(int factor) { sSupersample = factor >= 4 ? 4 : factor >= 2 ? 2 : 1; }

void Frame() {
  const double now = Now();
  if (!Active()) {
    if (sPhase != Phase::Idle) {
      sPhase = Phase::Idle;
      VISetFrameBufferScale(RestoreScale());
    }
    return;
  }
  switch (sPhase) {
  case Phase::Idle:
    break;
  case Phase::Hiding:
    if (++sPhaseFrames >= kHideFrames) {
      const float scale = PickCaptureScale();
      sLast.scale = scale;
      VISetFrameBufferScale(sCaptureFactor > 1 ? scale : RestoreScale());
      sPhase = Phase::Settling;
      sPhaseFrames = 0;
    }
    break;
  case Phase::Settling:
    if (++sPhaseFrames >= (sCaptureFactor > 1 ? kSettleFrames : 2)) {
      const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::system_clock::now().time_since_epoch())
                             .count();
      sLast.path = "screenshots/metroid-prime-2-photo-" + std::to_string(stamp) + ".png";
      aurora::request_screenshot_png(sLast.path.c_str());
      sPhase = Phase::Saving;
      sPhaseAt = now;
    }
    break;
  case Phase::Saving: {
    uint32_t w = 0;
    uint32_t h = 0;
    const int state = aurora::screenshot_png_state(&w, &h);
    if (state == 0 && now - sPhaseAt < kSaveTimeoutSeconds) {
      break;
    }
    VISetFrameBufferScale(RestoreScale());
    sPhase = Phase::Idle;
    sLast.state = state > 0 ? 1 : -1;
    sLast.width = w;
    sLast.height = h;
    if (state > 0) {
      Say("Saved " + sLast.path + " (" + std::to_string(w) + "x" + std::to_string(h) + ")" +
          (sLast.clamped ? ", smaller than asked: too large for this GPU" : ""));
    } else {
      Say("Couldn't save the photo");
    }
    break;
  }
  }
}

bool RequestCapture(int factor) {
  if (!Active() || sPhase != Phase::Idle) {
    return false;
  }
  sCaptureFactor = factor > 0 ? (factor >= 4 ? 4 : factor >= 2 ? 2 : 1) : sSupersample;
  sLast = CaptureResult{};
  sPhase = Phase::Hiding;
  sPhaseFrames = 0;
  return true;
}

bool Capturing() { return sPhase != Phase::Idle; }
CaptureResult LastCapture() { return sLast; }

} // namespace PortPhoto
