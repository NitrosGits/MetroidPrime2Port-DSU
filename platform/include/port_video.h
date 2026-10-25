// The F1 > Video rows the Prime 1 port has and Aurora now backs (f1b): dynamic resolution and upscale
// sharpening, Aurora's per-pixel lighting, the GPU self-test, and what the Compatibility tab sets up
// before the GPU device is made (OpenGL ES, the Adreno storage clamp, Dawn driver workarounds, the
// low-power adapter). Ported from MP1's platform/debug_ui/options.cpp and ui_common.cpp; the settings
// themselves are PortOverlay's (port_overlay.h).
#pragma once

#include <aurora/aurora.h>

#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

namespace PortVideo {

// Opt-in Dawn toggles for GPU drivers that draw wrong (MP1 issue #143): one checkbox each under
// Video > Compatibility > Driver workarounds, saved as `driver_workarounds`, applied when the GPU
// device is made (so after a restart). MP2_DAWN_ENABLE / MP2_DAWN_DISABLE still apply after them.
struct DriverWorkaround {
  const char* key; // its name in `driver_workarounds`
  const char* label;
  const char* tooltip;
  const char* enable;  // Dawn toggles it turns on, comma-separated, or nullptr
  const char* disable; // and the ones it turns off
};
inline constexpr DriverWorkaround kDriverWorkarounds[] = {
    {"keep_depth", "Keep depth buffers (Direct3D 11)",
     "Stops telling the driver it may throw away depth buffers the game is done with.\n"
     "Try it if geometry flickers on an older Intel GPU.",
     nullptr, "d3d11_use_discard_view"},
    {"gpu_constants", "Upload draw constants through the GPU (Direct3D 11)",
     "Sends each draw's constants with GPU buffer copies instead of the CPU-side path.\n"
     "Try it if models flicker, stretch or jump around.",
     "d3d11_disable_cpu_buffers", nullptr},
    {"unmonitored_fence", "Alternative GPU sync (Direct3D 11)",
     "Waits for the GPU with a different kind of fence before reusing upload buffers.\n"
     "Try it if the picture flickers from frame to frame.",
     "d3d11_use_unmonitored_fence", nullptr},
    {"validation", "Driver error checks (next start only, slower)",
     "Turns the graphics library's own checks back on for one start, so a command the driver\n"
     "rejects is named in the log. Costs performance, so it turns itself off after that start.",
     nullptr, "skip_validation,disable_robustness"},
};
inline constexpr size_t kDriverWorkaroundCount = std::size(kDriverWorkarounds);
// `validation` is for one start only: BeginGpuStart applies it and unticks it.
inline constexpr unsigned kDriverChecksBit = 1u << 3;
static_assert(std::string_view{kDriverWorkarounds[3].key} == "validation");
// `driver_workarounds` / MP2_DRIVER_WORKAROUNDS: comma-separated names to kDriverWorkarounds bits.
unsigned ParseDriverWorkarounds(std::string_view rest);

// main.cpp, just before aurora_initialize: the backend (OpenGL ES, with MP1's crash marker), the
// storage clamp and low-power adapter environment, and the driver workarounds' Dawn toggles.
void BeginGpuStart(const std::string& userFolder, AuroraConfig& config);
// Right after aurora_initialize: the OpenGL ES start finished.
void GpuStarted();
// What BeginGpuStart found, for the Compatibility tab.
bool DriverChecksThisRun();
const char* DriverWorkaroundsEnv();
bool DriverWorkaroundsCleared();

// Once per presented frame, from PortOverlay::Draw before the render scale is applied: dynamic
// resolution, sharpening and per-pixel lighting, and the driver workarounds' "started" marker.
void Frame();
// The scale ApplyRenderScale hands to VISetFrameBufferScale: dynamic resolution's while it has
// one, else PortOverlay::RenderScale().
float DrawScale();
// Dynamic resolution for the Quality tab.
float DynamicScale(); // the scale it draws at; 0 = the render scale
double DynamicResTargetFps();
double DynamicResGpuMs();
bool DynamicResGpuTimesDropped();
const char* DynamicResReason();
// The scale RenderScale() draws at, auto included (the window's own, as a multiple of what 1x
// draws); the most dynamic resolution draws at (the same); and the lines of the frame buffer drawn
// at `scale` (0 = auto) in the current window.
float BaseRenderScale();
float DynamicResTop();
int DrawnLines(float scale);
// Back to the top scale with nothing learnt (a setting changed).
void ResetDynamicRes();
void ApplyPresentSharpness();

// GPU self-test (F1 > Video > Compatibility, console `gpuselftest`, or MP2_GPU_SELFTEST=1 once after
// the first frames): renders known patterns offscreen, reads them back and logs
// "gpu selftest: <case>: PASS|FAIL". Run from CGraphics::BeginScene.
void RequestGpuSelfTest();
void RunGpuSelfTestIfRequested();

} // namespace PortVideo
