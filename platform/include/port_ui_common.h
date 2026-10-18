// The F1 window's theme and row helpers, copied from the Prime 1 port's debug_ui/ui_common.cpp:
// its touch style, its "Prime" colour theme (near-black blue, teal frames, amber accent), and the
// dimmed inline help that sits under a control on the phone. Presentation only.
//
// Built by `mp_port_audio`. The rows draw the themed layout (control, then the setting's `MP2_*`
// env var and a one-line description dimmed under it) on Android, and on the desktop the window's
// original look (control, then "(env)" and the description beside and under it).
#pragma once

struct ImGuiStyle;

namespace PortUi {

// The Prime 1 port's touch paddings and Prime colours, scaled for the display. Idempotent: call it
// every frame and it restyles only when the display scale changed. Android only; the desktop window
// is left as it is.
void ApplyTheme(float displayScale);

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
