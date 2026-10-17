// The F1 debug overlay. It is deliberately small: every control here is wired to something that
// exists, so nothing in the window lies about what it does. The teleport buttons drive the same
// PortWarp requests as the F5-F8 hotkeys and the MP2_INPUT tokens, and the aim controls drive
// PortMouse, which the game reads every tick.
//
// Built by `mp_port_audio` (it needs ImGui, which Aurora brings in with SDL).

#include "port_overlay.h"

#include "port_mouse.h"
#include "port_warp.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <cstdlib>
#include <cstring>


namespace {

bool sVisible = false;
bool sSkipCinematics = false;
bool sGodMode = false; // never saved: there is no settings file, and a cheat must not persist
bool sReadEnv = false;
uint32_t sWorldId = 0;
int sAreaId = -1;

// Frames per second, averaged over a second, from the presented-frame clock.
float sFps = 0.f;
uint64_t sFpsWindowStart = 0;
int sFpsFrames = 0;

bool EnvFlag(const char* name) {
  const char* value = std::getenv(name);
  return value != nullptr && *value != '\0' && std::strcmp(value, "0") != 0;
}

void ReadEnv() {
  if (sReadEnv) {
    return;
  }
  sReadEnv = true;
  sSkipCinematics = EnvFlag("MP2_SKIP_CINEMATICS");
  sGodMode = EnvFlag("MP2_GODMODE");
}

void TickFps() {
  const uint64_t now = SDL_GetTicksNS();
  if (sFpsWindowStart == 0) {
    sFpsWindowStart = now;
  }
  ++sFpsFrames;
  const uint64_t elapsed = now - sFpsWindowStart;
  if (elapsed >= 1000000000ull) {
    sFps = static_cast< float >(sFpsFrames) / (static_cast< float >(elapsed) / 1e9f);
    sFpsFrames = 0;
    sFpsWindowStart = now;
  }
}

void DrawGameSection() {
  ImGui::SeparatorText("Game");
  ImGui::Text("world %08X  area %d", sWorldId, sAreaId);
  if (ImGui::Button("Previous area (F6)")) {
    PortWarp::CycleArea(-1);
  }
  ImGui::SameLine();
  if (ImGui::Button("Next area (F5)")) {
    PortWarp::CycleArea(1);
  }
  if (ImGui::Button("Previous world (F8)")) {
    PortWarp::CycleWorld(-1);
  }
  ImGui::SameLine();
  if (ImGui::Button("Next world (F7)")) {
    PortWarp::CycleWorld(1);
  }
  ImGui::TextDisabled("MP2_WORLD=\"<world>[,<area>]\" and MP2_TELEPORT=\"<area>\" at start-up");
}

void DrawAimSection() {
  ImGui::SeparatorText("Aim");
  bool mouseAim = PortMouse::Enabled();
  if (ImGui::Checkbox("Mouse aim", &mouseAim)) {
    PortMouse::SetEnabled(mouseAim);
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_MOUSE_AIM=1)");
  bool invertX = PortMouse::InvertX();
  if (ImGui::Checkbox("Invert X", &invertX)) {
    PortMouse::SetInvertX(invertX);
  }
  ImGui::SameLine();
  bool invertY = PortMouse::InvertY();
  if (ImGui::Checkbox("Invert Y", &invertY)) {
    PortMouse::SetInvertY(invertY);
  }
  float sensitivity = PortMouse::Sensitivity();
  // Radians per pixel is a poor thing to read; degrees per pixel is not much better, so the
  // slider shows a 1x-100x multiplier over the default instead.
  float scale = sensitivity / 0.0035f;
  if (ImGui::SliderFloat("Sensitivity", &scale, 0.1f, 5.f, "%.2fx")) {
    PortMouse::SetSensitivity(0.0035f * scale);
  }
  ImGui::TextDisabled("MP2_MOUSE_SENS=%.5f (radians per pixel)", PortMouse::Sensitivity());

  bool twinStick = PortMouse::StickEnabled();
  if (ImGui::Checkbox("Twin stick", &twinStick)) {
    PortMouse::SetStickEnabled(twinStick);
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_TWIN_STICK=1)");
  float rate = PortMouse::StickAimRate();
  if (ImGui::SliderFloat("Stick aim rate", &rate, 200.f, 2000.f, "%.0f px/s")) {
    PortMouse::SetStickAimRate(rate);
  }
  ImGui::TextDisabled("the right stick aims through the same path as the mouse");
}

void DrawTestingSection() {
  ImGui::SeparatorText("Testing");
  if (ImGui::Checkbox("Skip cinematics", &sSkipCinematics)) {
    // The game reads this every frame in its cinematic flow state.
  }
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_SKIP_CINEMATICS=1)");
  ImGui::TextDisabled("ends every cinematic at once, seen or not; makes tests 3-5x faster");
}

void DrawCheatsSection() {
  ImGui::SeparatorText("Cheats");
  ImGui::Checkbox("God mode", &sGodMode);
  ImGui::SameLine();
  ImGui::TextDisabled("(MP2_GODMODE=1)");
  ImGui::TextDisabled("the player takes no damage; other actors still do. Not saved");
}

void DrawKeyboardSection() {
  ImGui::SeparatorText("Keyboard map");
  ImGui::TextDisabled("WASD move, IJKL C-stick, X/Z/C/V = A/B/X/Y,");
  ImGui::TextDisabled("Q/E = L/R, F = Z, Enter = Start, arrows = D-pad");
}

} // namespace

namespace PortOverlay {

void SetGameState(uint32_t worldId, int areaId) {
  sWorldId = worldId;
  sAreaId = areaId;
}

void Toggle() { sVisible = !sVisible; }

bool Visible() {
  ReadEnv();
  return sVisible;
}

bool SkipCinematics() {
  ReadEnv();
  return sSkipCinematics;
}

void SetSkipCinematics(bool skip) {
  ReadEnv();
  sSkipCinematics = skip;
}

bool GodMode() {
  ReadEnv();
  return sGodMode;
}

void SetGodMode(bool enabled) {
  ReadEnv();
  sGodMode = enabled;
}

void Draw() {
  TickFps();
  // While the window is open the cursor belongs to it, not to the game's aim.
  PortMouse::SetUiActive(sVisible);
  if (!sVisible) {
    return;
  }

  if (ImGui::Begin("Metroid Prime 2 port", &sVisible)) {
    ImGui::Text("%.1f fps", static_cast< double >(sFps));
    DrawGameSection();
    DrawAimSection();
    DrawTestingSection();
    DrawCheatsSection();
    DrawKeyboardSection();
    ImGui::SeparatorText("Hotkeys");
    ImGui::TextDisabled("F1 overlay, F5/F6 area, F7/F8 world");
  }
  ImGui::End();
}

} // namespace PortOverlay
