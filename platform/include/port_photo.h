#pragma once

#include <string>

// Photo mode, stage 2a: the capture. A PNG of the picture as the game draws it (HUD and overlays
// excluded), optionally supersampled through the frame-buffer scale. Needs the free camera on.
// Driven by the console's `photo`; the panel and the pause-menu entries come later.
namespace PortPhoto {

// Photo mode is the free camera for now.
bool Active();

// Once per rendered frame (PortOverlay::Draw): the capture sequence.
void Frame();

// Capture size multiplier: 1, 2 or 4.
int Supersample();
void SetSupersample(int factor);

// Saves a PNG of the picture as the game draws it. `factor` 0 uses Supersample(). False when
// the free camera is off or a capture is already running.
bool RequestCapture(int factor = 0);
bool Capturing();

// What the last capture did, for the toast and the console.
struct CaptureResult {
  int state = 0; // 0 none yet, 1 saved, -1 failed
  std::string path;
  unsigned width = 0;
  unsigned height = 0;
  float scale = 1.f; // render scale the shot was drawn at
  bool clamped = false;
};
CaptureResult LastCapture();
// A message for the toast once, then empty.
std::string TakeMessage();

} // namespace PortPhoto
