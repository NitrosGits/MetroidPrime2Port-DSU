// The F1 window's theme and row helpers, copied from the Prime 1 port's debug_ui/ui_common.cpp:
// its touch style, its "Prime" colour theme (near-black blue, teal frames, amber accent), and the
// dimmed inline help that sits under a control on the phone. Presentation only.
//
// Built by `mp_port_audio`. The rows draw the themed layout (control, then the setting's `MP2_*`
// env var and a one-line description dimmed under it) on Android, and on the desktop the window's
// original look (control, then "(env)" and the description beside and under it).
#pragma once

struct ImGuiStyle;
struct SDL_Window;

namespace PortUi {

// The window that ImGui draws into (the first SDL window), or null before it exists.
SDL_Window* MainWindow();

// The Prime 1 port's UpdateUiScale: the window's scale is the display's own (capped at 2 on the
// phone) times `userScale` (the `ui_scale` setting, 1 = 100%), clamped to 1..4. Call it every
// frame, since the display scale may not be known on the first one; it restyles only when the
// scale changed, and rebuilds the style and io.FontGlobalScale from their unscaled values, so a
// change never compounds.
void UpdateUiScale(float userScale);
// The Prime 1 port's ApplyThemeIfChanged, called every frame after UpdateUiScale: on Android it
// applies the Prime colours, borders and rounding for the current scale when the scale changed. The
// desktop keeps ImGui's own colours, so there it does nothing.
void ApplyThemeIfChanged();
// MP1's PageLayout: the full-screen page window (always on the phone, or MP_TOUCH_UI) unless the
// `overlay_windowed` setting asks for the floating tabbed window.
bool PageLayout();

// Dimmed, wrapped text under a control (ItemHelp on Android).
void Help(const char* text);

// A setting row: the control, then `env` (e.g. "MP2_FAST_MORPH=1", may be null) and `description`
// (may be null). Each returns whether the value changed.
bool Toggle(const char* label, bool* value, const char* env, const char* description = nullptr);
bool SliderFloat(const char* label, float* value, float min, float max, const char* format,
                 const char* env, const char* description = nullptr);
bool SliderInt(const char* label, int* value, int min, int max, const char* format,
               const char* env, const char* description = nullptr);
bool Combo(const char* label, int* current, const char* items, const char* env,
           const char* description = nullptr);
// Just the env and description lines, for a row whose control the caller drew.
void Note(const char* env, const char* description = nullptr, bool sameLine = false);

} // namespace PortUi
