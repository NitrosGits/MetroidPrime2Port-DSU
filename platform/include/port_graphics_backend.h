#ifndef METROID_PRIME_2_PORT_GRAPHICS_BACKEND_H
#define METROID_PRIME_2_PORT_GRAPHICS_BACKEND_H

#include "aurora/aurora.h"
#include "port_env.h"

#include <span>
#include <string>

// The renderer choice (MP1 issue #135), in one place: the mapping from the saved
// selection to Aurora's backends, which choices the build offers, the per-run
// MP2_GRAPHICS_BACKEND override, and what the F1 Video page and the startup log say
// about all of it. main.cpp
// asks Aurora for ToAurora(Resolve(...).backend) and never maps a selection itself;
// the Video page draws Status. The functions are pure so tests/port_graphics_backend.cpp
// can drive every case without a window, a GPU or a settings file.
namespace PortGraphicsBackend {

// Setting `graphics_backend` (Windows and desktop Linux): the graphics API to ask Aurora for at
// the next start. Auto asks for nothing in particular, so Aurora takes its own order (Direct3D 12,
// then Vulkan, then Direct3D 11 on Windows; Vulkan, then OpenGL ES on Linux); an explicit API is
// tried first. Either way a failed start falls back to that order for the run and leaves the
// setting alone. Android ignores it (OpenGL ES is the `opengles` setting). These are the config
// file's values: don't reorder or renumber.
enum EGraphicsBackend {
  kGraphicsBackend_Auto = 0,
  kGraphicsBackend_D3D12,
  kGraphicsBackend_Vulkan,
  kGraphicsBackend_D3D11,
  kGraphicsBackend_OpenGL,
};
// The number of values; a build offers only Choices() of them.
constexpr int kGraphicsBackendCount = 5;

// The choices this build offers, in the order the F1 Video page lists them. An API with no
// Aurora backend here is not offered (no Direct3D outside Windows; OpenGL is Dawn's desktop GL,
// through EGL on desktop Linux and the Dawn fork's EGL-on-WGL shim on Windows); Android keeps
// its `opengles` toggle and offers nothing from this setting.
inline std::span<const EGraphicsBackend> Choices() {
#if defined(_WIN32)
  static constexpr EGraphicsBackend kChoices[] = {kGraphicsBackend_Auto, kGraphicsBackend_D3D12,
                                                  kGraphicsBackend_Vulkan, kGraphicsBackend_D3D11,
                                                  kGraphicsBackend_OpenGL};
#elif defined(__linux__) && !defined(__ANDROID__)
  static constexpr EGraphicsBackend kChoices[] = {kGraphicsBackend_Auto, kGraphicsBackend_Vulkan,
                                                  kGraphicsBackend_OpenGL};
#else
  static constexpr EGraphicsBackend kChoices[] = {kGraphicsBackend_Auto};
#endif
  return kChoices;
}
// The config file's name for a choice; these, and only these, are what ParseGraphicsBackend and
// MP2_GRAPHICS_BACKEND accept.
inline const char* GraphicsBackendConfigName(EGraphicsBackend backend) {
  switch (backend) {
  case kGraphicsBackend_D3D12:
    return "d3d12";
  case kGraphicsBackend_Vulkan:
    return "vulkan";
  case kGraphicsBackend_D3D11:
    return "d3d11";
  case kGraphicsBackend_OpenGL:
    return "opengl";
  default:
    return "auto";
  }
}
// Reads a stored name or an MP2_GRAPHICS_BACKEND value. False for anything else, with `out` left
// as it was, so an unknown value keeps the saved selection.
inline bool ParseGraphicsBackend(const std::string& value, EGraphicsBackend& out) {
  if (value == "auto") {
    out = kGraphicsBackend_Auto;
  } else if (value == "d3d12") {
    out = kGraphicsBackend_D3D12;
  } else if (value == "vulkan") {
    out = kGraphicsBackend_Vulkan;
  } else if (value == "d3d11") {
    out = kGraphicsBackend_D3D11;
  } else if (value == "opengl") {
    out = kGraphicsBackend_OpenGL;
  } else {
    return false;
  }
  return true;
}

// What this run asked Aurora for: the saved selection, or MP2_GRAPHICS_BACKEND when it
// names one. `env` is that variable's value (nullptr when unset). Only `choices` (this
// build's, by default) count: an override naming another platform's API is rejected,
// and a saved one (a settings file from another platform) asks for Auto, which is
// also what the Video page shows for it.
struct Request {
  EGraphicsBackend backend = kGraphicsBackend_Auto;
  // The environment named it: the setting is not what starts this run.
  bool overridden = false;
  // The environment was set but names nothing: `backend` is the saved selection, which
  // the caller warns about.
  bool rejected = false;
};

// MP2_GRAPHICS_BACKEND, read here only, so the start-up choice and the Video page
// describe the same request.
inline const char* EnvRequest() { return port::EnvString("MP2_GRAPHICS_BACKEND"); }

inline Request Resolve(EGraphicsBackend saved, const char* env,
                       std::span<const EGraphicsBackend> choices = Choices()) {
  const auto offered = [choices](EGraphicsBackend backend) {
    for (const EGraphicsBackend choice : choices) {
      if (choice == backend) {
        return true;
      }
    }
    return false;
  };
  Request request;
  request.backend = offered(saved) ? saved : kGraphicsBackend_Auto;
  if (env != nullptr) {
    EGraphicsBackend parsed = saved;
    if (ParseGraphicsBackend(env, parsed) && offered(parsed)) {
      request.backend = parsed;
      request.overridden = true;
    } else {
      request.rejected = true;
    }
  }
  return request;
}

// What a selection asks Aurora for. Auto asks for nothing in particular, so Aurora
// takes its own order: Direct3D 12, then Vulkan, then Direct3D 11 on Windows; Vulkan,
// then OpenGL ES on Linux.
inline AuroraBackend ToAurora(EGraphicsBackend selection) {
  switch (selection) {
  case kGraphicsBackend_D3D12:
    return BACKEND_D3D12;
  case kGraphicsBackend_Vulkan:
    return BACKEND_VULKAN;
  case kGraphicsBackend_D3D11:
    return BACKEND_D3D11;
  case kGraphicsBackend_OpenGL:
    return BACKEND_OPENGL;
  default:
    return BACKEND_AUTO;
  }
}

// The selection an actual backend is, for comparing against what was asked for:
// kGraphicsBackend_Auto for anything the list doesn't name (Metal, WebGPU, OpenGL ES,
// or none at all, BACKEND_NULL). That also means an unnamed backend can never be read
// as "the request started".
inline EGraphicsBackend FromAurora(AuroraBackend backend) {
  switch (backend) {
  case BACKEND_D3D12:
    return kGraphicsBackend_D3D12;
  case BACKEND_VULKAN:
    return kGraphicsBackend_Vulkan;
  case BACKEND_D3D11:
    return kGraphicsBackend_D3D11;
  case BACKEND_OPENGL:
    return kGraphicsBackend_OpenGL;
  default:
    return kGraphicsBackend_Auto;
  }
}

// A backend's name for the page and the log. BACKEND_NULL is what Aurora ends on when
// no device started at all, and nothing draws.
inline const char* DisplayName(AuroraBackend backend) {
  switch (backend) {
  case BACKEND_D3D12:
    return "Direct3D 12";
  case BACKEND_D3D11:
    return "Direct3D 11";
  case BACKEND_VULKAN:
    return "Vulkan";
  case BACKEND_METAL:
    return "Metal";
  case BACKEND_OPENGL:
    return "OpenGL";
  case BACKEND_OPENGLES:
    return "OpenGL ES";
  case BACKEND_WEBGPU:
    return "WebGPU";
  case BACKEND_NULL:
    return "no graphics device";
  default:
    return "Aurora's automatic choice";
  }
}

// What the Video page says about the choice. `saved` is the setting now, `savedAtStart`
// what this run read from the file, `request` how this run resolved it (override and
// all), and `actual` what Aurora started.
struct Status {
  // The saved selection differs from this run's: the next start uses it. Compared on
  // the saved values only, so setting the choice back clears it. An override doesn't
  // enter into it; it outranks the file on the next start too.
  bool restartPending = false;
  // MP2_GRAPHICS_BACKEND named this run's API, so a change to the setting does not
  // affect this run.
  bool overridden = false;
  // This run asked for an API and another one started: Aurora fell back, and the
  // setting is kept as it was. False for Auto, which asks for nothing in particular.
  bool fellBack = false;
  // What this run asked for (over a rejected MP2_GRAPHICS_BACKEND: the saved selection).
  EGraphicsBackend requested = kGraphicsBackend_Auto;
  // What started, as FromAurora reads it (Auto when the list doesn't name it).
  EGraphicsBackend actual = kGraphicsBackend_Auto;
};

inline Status Describe(EGraphicsBackend saved, EGraphicsBackend savedAtStart,
                       const Request& request, AuroraBackend actual) {
  Status status;
  status.restartPending = saved != savedAtStart;
  status.overridden = request.overridden;
  status.requested = request.backend;
  status.actual = FromAurora(actual);
  status.fellBack = request.backend != kGraphicsBackend_Auto && status.actual != request.backend;
  return status;
}

} // namespace PortGraphicsBackend

#endif // METROID_PRIME_2_PORT_GRAPHICS_BACKEND_H
